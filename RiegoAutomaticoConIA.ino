#include <WiFi.h>
#include <WebServer.h>
#include <WiFiUdp.h>
#include <WiFiClientSecure.h>
#include <NTPClient.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>
#include "interfaz.h"
#include <Preferences.h>

Preferences prefs;

// --- CONFIGURACIÓN WIFI (ROUTER) ---
#define WIFI_SSID "FAMILIA SANCHEZ "
#define WIFI_PASSWORD "willpower"
#define WIFI_CHANNEL 6

// --- RED PROPIA DEL ESP32 (AP) ---
#define AP_SSID "RiegoIA"
#define AP_PASSWORD "riego1234"   // mínimo 8 caracteres

// --- API METEOROLÓGICA REAL (Open-Meteo) ---
// Open-Meteo es gratuita y no requiere API key.
// Coordenadas de La Dorada, Caldas, Colombia.
const float LATITUD_LA_DORADA = 5.44783;
const float LONGITUD_LA_DORADA = -74.66311;

// Si la probabilidad de lluvia en las próximas 2 horas supera este
// valor, se suspende el riego.
const int UMBRAL_SUSPENSION_RIEGO = 60;

// --- CONFIGURACIÓN DE PINES (4 ELECTROVÁLVULAS) ---
const int RELAY_PINS[4] = {22, 18, 4, 2}; // RELE1=D22, RELE2=D18, RELE3=D4, RELE4=D2 (según el esquema)

// --- CONFIGURACIÓN DEL BOTÓN FÍSICO MANUAL (NORMALMENTE ABIERTO) ---
// Conecta el botón entre BUTTON_PIN y GND. Usamos INPUT_PULLUP,
// así que el pin lee HIGH en reposo y LOW al presionar el botón.
#define BUTTON_PIN 33
bool lastButtonReading = HIGH;
bool buttonState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long DEBOUNCE_DELAY = 50; // ms

// --- VARIABLES GLOBALES ---
WebServer server(80);
bool systemRunning = false;   // Hay un ciclo de riego activo (manual, físico o automático)
bool rainForecast = false;    // true = pronóstico de lluvia, false = despejado
int fallosConsecutivos = 0;
int moistureValue = 0;
int moisturePercentage = 100;
String moistureStatusText = "Monitoreo por Clima (Previsión)";
String treeColor = "#66BB6A";

// Última probabilidad de lluvia real reportada por Open-Meteo (0-100).
int ultimaProbabilidadLluvia = 20;

// --- VARIABLES PARA LA PROGRAMACIÓN DE RIEGO ---
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "co.pool.ntp.org", -18000, 60000);

String wateringTime1 = "07:00";       // Valor por defecto (se sobreescribe con lo guardado en flash)
String wateringTime2 = "19:00";
int wateringDurationSeconds = 30;

bool hasWateredToday1 = false;
bool hasWateredToday2 = false;
int lastCheckedDay = -1;

// Apaga todas las electroválvulas inmediatamente
void apagarTodo() {
  for (int i = 0; i < 4; i++) {
    digitalWrite(RELAY_PINS[i], LOW);
  }
  systemRunning = false;
}

// Actualiza los elementos visuales de la interfaz web según el clima
void actualizarEstadoInterfaz(bool vaALlover, int probabilidadLluvia) {
  moisturePercentage = probabilidadLluvia;

  if (vaALlover) {
    moistureStatusText = "Pronostico: LLUVIA (" + String(probabilidadLluvia) + "% - Riego Suspendido)";
    treeColor = "#2E7D32";
  } else {
    moistureStatusText = "Pronostico: DESPEJADO (" + String(probabilidadLluvia) + "% - Listo para Riego)";
    treeColor = "#66BB6A";
  }
}

// Consulta Open-Meteo y obtiene la probabilidad real de lluvia (0-100) para
// la hora actual y las 2 siguientes (se toma el máximo del rango).
bool consultarClimaReal(int &probabilidadOut) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Sin WiFi: no se puede consultar Open-Meteo.");
    return false;
  }

  if (!timeClient.update() && !timeClient.isTimeSet()) {
    Serial.println("No se pudo obtener la hora NTP para comparar con Open-Meteo.");
    return false;
  }

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;

  String url = "https://api.open-meteo.com/v1/forecast?latitude=" + String(LATITUD_LA_DORADA, 4) +
               "&longitude=" + String(LONGITUD_LA_DORADA, 4) +
               "&hourly=precipitation_probability&timezone=America%2FBogota&forecast_days=2";

  http.begin(client, url);
  int httpCode = http.GET();

  if (httpCode != 200) {
    Serial.print("Error Open-Meteo HTTP: ");
    Serial.println(httpCode);
    http.end();
    return false;
  }

  String response = http.getString();
  http.end();

  DynamicJsonDocument doc(16384);
  DeserializationError error = deserializeJson(doc, response);
  if (error) {
    Serial.print("Error al parsear JSON de Open-Meteo: ");
    Serial.println(error.c_str());
    return false;
  }

  JsonArray tiempos = doc["hourly"]["time"];
  JsonArray probabilidades = doc["hourly"]["precipitation_probability"];

  if (tiempos.size() == 0 || probabilidades.size() == 0) {
    Serial.println("Open-Meteo no devolvio datos horarios validos.");
    return false;
  }

  // Hora actual en el formato de Open-Meteo: "YYYY-MM-DDTHH:00"
  time_t rawTime = timeClient.getEpochTime();
  struct tm *horaLocal = localtime(&rawTime);
  char horaActualStr[20];
  sprintf(horaActualStr, "%04d-%02d-%02dT%02d:00",
          horaLocal->tm_year + 1900, horaLocal->tm_mon + 1,
          horaLocal->tm_mday, horaLocal->tm_hour);

  int indiceActual = -1;
  for (size_t i = 0; i < tiempos.size(); i++) {
    if (String(tiempos[i].as<const char*>()) == String(horaActualStr)) {
      indiceActual = i;
      break;
    }
  }

  if (indiceActual == -1) {
    Serial.println("No se encontro la hora actual dentro de los datos de Open-Meteo.");
    return false;
  }

  // Máximo de probabilidad entre la hora actual y las 2 siguientes
  int maxProbabilidad = 0;
  for (int i = indiceActual; i < indiceActual + 3 && i < (int)probabilidades.size(); i++) {
    int p = probabilidades[i].as<int>();
    if (p > maxProbabilidad) maxProbabilidad = p;
  }

  probabilidadOut = maxProbabilidad;
  return true;
}

bool preguntarAIARegar() {
  int probabilidad = ultimaProbabilidadLluvia; // respaldo si la consulta falla
  bool consultaExitosa = consultarClimaReal(probabilidad);

  bool pronosticoLluvia;
  bool decisionRiego;

  if (consultaExitosa) {
    Serial.println("Open-Meteo: probabilidad de lluvia (proximas 2h) = " + String(probabilidad) + "%");
    pronosticoLluvia = (probabilidad > UMBRAL_SUSPENSION_RIEGO);
    decisionRiego = !pronosticoLluvia;
    rainForecast = pronosticoLluvia;
    ultimaProbabilidadLluvia = probabilidad;
    fallosConsecutivos = 0;
  } else {
    // Si falla la consulta, NO regamos si el último pronóstico conocido era lluvia.
    Serial.println("Fallo la consulta a Open-Meteo. Usando ultimo pronostico conocido.");
    pronosticoLluvia = rainForecast;
    decisionRiego = !rainForecast;
    fallosConsecutivos++;
  }

  if (!systemRunning) {
    actualizarEstadoInterfaz(pronosticoLluvia, ultimaProbabilidadLluvia);
  }

  return decisionRiego;
}

void ejecutarCicloRiego(String tipoOrigen) {
  systemRunning = true;
  Serial.println("Iniciando ciclo secuencial de riego por " + tipoOrigen);

  for (int i = 0; i < 4; i++) {
    moistureStatusText = "Regando " + tipoOrigen + ": Aspersor " + String(i + 1);
    moisturePercentage = 100 - (i * 20); // Simulación visual
    treeColor = "#00E676";

    digitalWrite(RELAY_PINS[i], HIGH);
    Serial.println(" -> Abriendo Electroválvula " + String(i + 1));

    // Espera atendiendo la web y el botón físico para poder cancelar en cualquier momento
    unsigned long inicioPaso = millis();
    while (millis() - inicioPaso < ((unsigned long)wateringDurationSeconds * 1000UL)) {
      server.handleClient();
      manejarBotonFisico();

      if (!systemRunning) {
        Serial.println("Ciclo de riego cancelado durante la espera.");
        return;
      }
      delay(10);
    }

    digitalWrite(RELAY_PINS[i], LOW);
    Serial.println(" -> Cerrando Electroválvula " + String(i + 1));
  }

  apagarTodo();
  moistureStatusText = "Ciclo de Riego Terminado";
  moisturePercentage = ultimaProbabilidadLluvia;
  treeColor = "#66BB6A";
  Serial.println("Ciclo secuencial de riego finalizado exitosamente.");
}

// --- BOTÓN FÍSICO MANUAL (NORMALMENTE ABIERTO, CON ANTIRREBOTE) ---
// Funciona independiente del WiFi y de internet.
void manejarBotonFisico() {
  bool reading = digitalRead(BUTTON_PIN);

  if (reading != lastButtonReading) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
    if (reading != buttonState) {
      buttonState = reading;

      if (buttonState == LOW) {
        if (!systemRunning) {
          Serial.println("Botón físico: iniciando riego manual.");
          ejecutarCicloRiego("BOTÓN FÍSICO");
        } else {
          Serial.println("Botón físico: cancelando riego en curso.");
          apagarTodo();
          moistureStatusText = "Riego cancelado con botón físico";
          moisturePercentage = ultimaProbabilidadLluvia;
          treeColor = "#66BB6A";
        }
      }
    }
  }

  lastButtonReading = reading;
}

void handleRoot() {
  sendHtml();
}

// Botón de riego inmediato desde la página web (ignora el pronóstico).
void handleManualToggle() {
  if (systemRunning) {
    apagarTodo();
    moistureStatusText = "Riego cancelado manualmente";
    moisturePercentage = ultimaProbabilidadLluvia;
    treeColor = "#66BB6A";
    server.sendHeader("Location", "/");
    server.send(303);
    return;
  }
  server.sendHeader("Location", "/");
  server.send(303);
  ejecutarCicloRiego("MANUAL WEB");
}

// Fuerza el riego ignorando el pronóstico climático
void handleForceWater() {
  if (!systemRunning) {
    Serial.println("FORZADO: Riego iniciado ignorando pronóstico de lluvia.");
    rainForecast = false;
    fallosConsecutivos = 0;
    moistureStatusText = "Riego FORZADO (ignorando pronóstico)";
    treeColor = "#00E676";
    server.sendHeader("Location", "/");
    server.send(303);
    ejecutarCicloRiego("FORZADO");
  } else {
    server.sendHeader("Location", "/");
    server.send(303);
  }
}

// Fuerza una nueva consulta a Open-Meteo
void handleRefreshWeather() {
  Serial.println("Actualización manual del pronóstico solicitada.");
  fallosConsecutivos = 0;
  preguntarAIARegar();
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleSetSchedule() {
  if (server.hasArg("time1") && server.hasArg("time2") && server.hasArg("duration")) {
    wateringTime1 = server.arg("time1");
    wateringTime2 = server.arg("time2");
    wateringDurationSeconds = server.arg("duration").toInt();

    // Guardado en flash para que sobreviva reinicios y cortes de luz
    prefs.begin("riego", false);
    prefs.putString("time1", wateringTime1);
    prefs.putString("time2", wateringTime2);
    prefs.putInt("duracion", wateringDurationSeconds);
    prefs.end();
    Serial.println("Programación guardada en flash.");
  }
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleAutomaticWatering() {
  if (!timeClient.update()) return;

  int currentDay = timeClient.getDay();
  if (currentDay != lastCheckedDay) {
    hasWateredToday1 = false;
    hasWateredToday2 = false;
    lastCheckedDay = currentDay;
  }

  String currentTime = timeClient.getFormattedTime().substring(0, 5);

  // Si ya hay un riego en curso, no lanzamos otro encima
  if (systemRunning) return;

  if (currentTime == wateringTime1 && !hasWateredToday1) {
    Serial.println("Horario 1 alcanzado. Consultando clima...");
    if (preguntarAIARegar()) {
      ejecutarCicloRiego("AUTOMÁTICO 1");
    }
    hasWateredToday1 = true;
  }

  if (currentTime == wateringTime2 && !hasWateredToday2) {
    Serial.println("Horario 2 alcanzado. Consultando clima...");
    if (preguntarAIARegar()) {
      ejecutarCicloRiego("AUTOMÁTICO 2");
    }
    hasWateredToday2 = true;
  }
}

void setup() {
  Serial.begin(115200);

  // Cargamos la programación guardada en flash (si existe)
  prefs.begin("riego", true); // true = solo lectura
  wateringTime1           = prefs.getString("time1", "07:00");
  wateringTime2           = prefs.getString("time2", "19:00");
  wateringDurationSeconds = prefs.getInt("duracion", 30);
  prefs.end();
  Serial.println("Programación cargada: " + wateringTime1 + " / " + wateringTime2 + " / " + String(wateringDurationSeconds) + "s");

  // Inicializamos las 4 salidas de los relés
  for (int i = 0; i < 4; i++) {
    pinMode(RELAY_PINS[i], OUTPUT);
  }
  apagarTodo();

  // Botón físico normalmente abierto, con pull-up interna
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // --- WiFi: AP propio + conexión al router ---
  WiFi.mode(WIFI_AP_STA);
  WiFi.setAutoReconnect(true);

  // Red propia del ESP32
  WiFi.softAPConfig(IPAddress(192,168,4,1), IPAddress(192,168,4,1), IPAddress(255,255,255,0));
  WiFi.softAP(AP_SSID, AP_PASSWORD, WIFI_CHANNEL);
  Serial.println("AP creado. Página en http://" + WiFi.softAPIP().toString());

  // Conexión al router (para hora y clima)
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD, WIFI_CHANNEL);
  unsigned long inicioConexion = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - inicioConexion < 15000) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nRouter conectado. IP: " + WiFi.localIP().toString());
  } else {
    Serial.println("\nSin router: funciona el AP y el botón físico (sin hora ni clima).");
  }
  timeClient.begin(); // siempre, así la hora se obtiene si el router vuelve después

  server.on("/", handleRoot);
  server.on("/manualToggle", handleManualToggle);
  server.on("/forceWater", handleForceWater);
  server.on("/refreshWeather", handleRefreshWeather);
  server.on("/setSchedule", handleSetSchedule);
  server.begin();
  Serial.println("Servidor Web activo.");

  // Consulta inicial para que el porcentaje mostrado sea real desde el arranque
  preguntarAIARegar();
}

void loop() {
  server.handleClient();
  manejarBotonFisico();      // Siempre activo, incluso sin WiFi/internet
  handleAutomaticWatering();

  // Actualización climática en segundo plano cada 10 minutos (si no está regando)
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate > 600000 && !systemRunning) {
    lastUpdate = millis();
    preguntarAIARegar();
  }
}
