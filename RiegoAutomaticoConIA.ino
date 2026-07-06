#include <WiFi.h>
#include <WebServer.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "interfaz.h"
#include <Preferences.h>

Preferences prefs;

// --- CONFIGURACIÓN WIFI ---
#define WIFI_SSID "FAMILIA SANCHEZ "
#define WIFI_PASSWORD "willpower"
#define WIFI_CHANNEL 6

// --- CREDENCIALES DE GOOGLE GEMINI ---
const char* GEMINI_API_KEY = "AQ.Ab8RN6JpotzahNznOlpQsgKYk2XVukc50oISh79JjIDRiCNphw";
const String GEMINI_URL = "https://generativelanguage.googleapis.com/v1beta/models/gemini-2.5-flash-lite:generateContent?key=";

// --- CONFIGURACIÓN DE PINES (4 ELECTROVÁLVULAS) ---
const int RELAY_PINS[4] = {26, 27, 25, 32}; // Cambia estos números por tus pines físicos disponibles

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
bool systemRunning = false;   // Indica si hay un ciclo de riego activo (manual, físico o automático)
bool rainForecast = false;    // true = la IA detectó lluvia, false = despejado
int fallosConsecutivos = 0;    // Si Gemini falla muchas veces seguidas, reseteamos el bloqueo
int moistureValue = 0;
int moisturePercentage = 100;
String moistureStatusText = "Monitoreo por Clima (Previsión)";
String treeColor = "#66BB6A";

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

// Actualiza los elementos visuales de la interfaz web según el estado climático
void actualizarEstadoInterfaz(bool vaALlover) {
  if (vaALlover) {
    moistureStatusText = "Pronóstico: LLUVIA (Riego Suspendido)";
    moisturePercentage = 90;
    treeColor = "#2E7D32";
  } else {
    moistureStatusText = "Pronóstico: DESPEJADO (Listo para Riego)";
    moisturePercentage = 30;
    treeColor = "#66BB6A";
  }
}

bool preguntarAIARegar() {
  if (WiFi.status() != WL_CONNECTED) {
    // Sin WiFi: usamos el ultimo pronostico conocido para no regar si antes habia lluvia.
    Serial.println("Sin WiFi: usando ultimo pronostico conocido.");
    return !rainForecast;
  }

  HTTPClient http;
  String urlCompleta = GEMINI_URL + String(GEMINI_API_KEY);
  http.begin(urlCompleta);
  http.addHeader("Content-Type", "application/json");

  String prompt = "Eres un agronomo experto. Consulta el pronostico meteorologico actual para La Dorada, Caldas, Colombia. ";
  prompt += "Necesito saber si debo regar mis cultivos en las proximas 2 horas. ";
  prompt += "Regla: responde ESPERAR solo si la probabilidad de lluvia supera el 60% O si ya esta lloviendo con intensidad moderada o fuerte. ";
  prompt += "Una llovizna leve, humedad alta o probabilidad menor al 60% NO es motivo para suspender el riego. ";
  prompt += "Responde unicamente con una sola palabra en mayusculas: REGAR o ESPERAR. Sin explicaciones ni signos.";

  String payload = "{\"contents\":[{\"parts\":[{\"text\":\"" + prompt + "\"}]}]}";
  int httpResponseCode = http.POST(payload);
  bool decisionRiego = false;
  bool pronosticoLluvia = false;
  bool consultaExitosa = false;

  if (httpResponseCode == 200) {
    String response = http.getString();
    DynamicJsonDocument doc(4096);
    if (!deserializeJson(doc, response)) {
      String respuestaIA = doc["candidates"][0]["content"]["parts"][0]["text"].as<String>();
      respuestaIA.trim();
      respuestaIA.toUpperCase();
      Serial.println("Respuesta de Gemini: " + respuestaIA);

      // indexOf en vez de comparacion exacta: tolera puntuacion o texto extra de Gemini
      if (respuestaIA.indexOf("REGAR") >= 0) {
        decisionRiego = true;
        pronosticoLluvia = false;
        consultaExitosa = true;
      } else if (respuestaIA.indexOf("ESPERAR") >= 0) {
        decisionRiego = false;
        pronosticoLluvia = true;
        consultaExitosa = true;
      } else {
        // Gemini respondio algo inesperado: respetamos el ultimo pronostico conocido
        Serial.println("Respuesta inesperada de Gemini. Usando ultimo pronostico.");
        pronosticoLluvia = rainForecast;
        decisionRiego = !rainForecast;
      }

      if (consultaExitosa) {
        rainForecast = pronosticoLluvia;
        fallosConsecutivos = 0; // Resetear contador al tener respuesta válida
      }
      if (!systemRunning) {
        actualizarEstadoInterfaz(pronosticoLluvia);
      }
    }
  } else {
    // CORRECCIÓN: antes decisionRiego=true siempre que fallara la llamada,
    // por eso regaba aunque la pantalla mostrara LLUVIA (el display quedaba del
    // chequeo anterior exitoso, pero el automatico fallaba y usaba el fallback).
    // Ahora: si el ultimo pronostico conocido era lluvia, NO regamos.
    Serial.print("Error Gemini HTTP: ");
    Serial.println(httpResponseCode);
    pronosticoLluvia = rainForecast;
    decisionRiego = !rainForecast;
    if (!systemRunning) {
      actualizarEstadoInterfaz(pronosticoLluvia);
    }
  }
  http.end();
  return decisionRiego;
}

void ejecutarCicloRiego(String tipoOrigen) {
  systemRunning = true;
  Serial.println("Iniciando ciclo secuencial de riego por " + tipoOrigen);

  for (int i = 0; i < 4; i++) {
    // Informamos a la web qué aspersor está trabajando
    moistureStatusText = "Regando " + tipoOrigen + ": Aspersor " + String(i + 1);
    moisturePercentage = 100 - (i * 20); // Simulación visual
    treeColor = "#00E676";

    // Encendemos la electroválvula actual
    digitalWrite(RELAY_PINS[i], HIGH);
    Serial.println(" -> Abriendo Electroválvula " + String(i + 1));

    // Esperamos el tiempo programado atendiendo peticiones web y el botón físico
    // para no congelar el servidor ni perder la posibilidad de cancelar manualmente
    unsigned long inicioPaso = millis();
    while (millis() - inicioPaso < (wateringDurationSeconds * 1000)) {
      server.handleClient();   // Mantiene la interfaz web responsiva durante la espera
      manejarBotonFisico();    // Permite cancelar con el botón físico en cualquier momento

      // Si el botón físico (u otra fuente) apagó el sistema, abortamos el ciclo
      if (!systemRunning) {
        Serial.println("Ciclo de riego cancelado durante la espera.");
        return;
      }
      delay(10);
    }

    // Apagamos la válvula actual antes de pasar a la siguiente
    digitalWrite(RELAY_PINS[i], LOW);
    Serial.println(" -> Cerrando Electroválvula " + String(i + 1));
  }

  // Al finalizar todo el ciclo devolvemos los valores normales
  apagarTodo();
  moistureStatusText = "Ciclo de Riego Terminado";
  treeColor = "#66BB6A";
  Serial.println("Ciclo secuencial de riego finalizado exitosamente.");
}

// --- MANEJO DEL BOTÓN FÍSICO MANUAL (NORMALMENTE ABIERTO, CON ANTIRREBOTE) ---
// Funciona de forma completamente independiente del WiFi y de la IA:
// solo enciende/apaga la cascada de electroválvulas directamente.
void manejarBotonFisico() {
  bool reading = digitalRead(BUTTON_PIN);

  if (reading != lastButtonReading) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
    if (reading != buttonState) {
      buttonState = reading;

      // Flanco de bajada = botón presionado (pull-up activo en reposo = HIGH)
      if (buttonState == LOW) {
        if (!systemRunning) {
          // El botón físico siempre riega, independientemente del pronóstico.
          Serial.println("Botón físico: iniciando riego manual.");
          ejecutarCicloRiego("BOTÓN FÍSICO");
        } else {
          Serial.println("Botón físico: cancelando riego en curso.");
          apagarTodo();
          moistureStatusText = "Riego cancelado con botón físico";
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

// Maneja el botón de riego inmediato desde la página web.
// El modo manual se ejecuta siempre, independientemente del pronóstico climático.
void handleManualToggle() {
  if (systemRunning) {
    apagarTodo();
    moistureStatusText = "Riego cancelado manualmente";
    treeColor = "#66BB6A";
    sendHtml();
    return;
  }
  sendHtml();
  ejecutarCicloRiego("MANUAL WEB");
}

// Fuerza el riego ignorando el pronóstico climático (acción deliberada del usuario)
void handleForceWater() {
  if (!systemRunning) {
    Serial.println("FORZADO: Riego iniciado ignorando pronóstico de lluvia.");
    rainForecast = false;
    fallosConsecutivos = 0;
    moistureStatusText = "Riego FORZADO (ignorando pronóstico)";
    treeColor = "#00E676";
    sendHtml();
    ejecutarCicloRiego("FORZADO");
  } else {
    server.sendHeader("Location", "/");
    server.send(303);
  }
}

// Fuerza una nueva consulta a Gemini y actualiza el pronóstico en pantalla
void handleRefreshWeather() {
  Serial.println("Actualización manual del pronóstico solicitada.");
  fallosConsecutivos = 0; // Resetear para que no entre en modo degradado
  preguntarAIARegar();    // Consulta y actualiza rainForecast + interfaz
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleSetSchedule() {
  if (server.hasArg("time1") && server.hasArg("time2") && server.hasArg("duration")) {
    wateringTime1 = server.arg("time1");
    wateringTime2 = server.arg("time2");
    wateringDurationSeconds = server.arg("duration").toInt();

    // Guardamos en flash para que sobreviva reinicios y cortes de luz
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

  // Si ya hay un riego en curso, evitamos lanzar otro encima
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
  wateringTime1         = prefs.getString("time1",   "07:00");
  wateringTime2         = prefs.getString("time2",   "19:00");
  wateringDurationSeconds = prefs.getInt("duracion", 30);
  prefs.end();
  Serial.println("Programación cargada: " + wateringTime1 + " / " + wateringTime2 + " / " + String(wateringDurationSeconds) + "s");

  // Inicializamos las 4 salidas de los relés
  for (int i = 0; i < 4; i++) {
    pinMode(RELAY_PINS[i], OUTPUT);
  }
  apagarTodo();

  // Botón físico normalmente abierto, con resistencia pull-up interna
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD, WIFI_CHANNEL);
  unsigned long inicioConexion = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - inicioConexion < 15000) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Conectado. IP Local:");
    Serial.println(WiFi.localIP());
    timeClient.begin();
  } else {
    Serial.println("\nNo se pudo conectar a WiFi. El botón físico seguirá funcionando.");
  }

  server.on("/", handleRoot);
  server.on("/manualToggle", handleManualToggle);
  server.on("/forceWater", handleForceWater);
  server.on("/refreshWeather", handleRefreshWeather);
  server.on("/setSchedule", handleSetSchedule);
  server.begin();
  Serial.println("Servidor Web activo.");
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
