#include <WiFi.h>
#include <WebServer.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "interfaz.h"

// --- CONFIGURACIÓN WIFI ---
#define WIFI_SSID "FAMILIA SANCHEZ "
#define WIFI_PASSWORD "willpower"
#define WIFI_CHANNEL 6

// --- CREDENCIALES DE GOOGLE GEMINI ---
const char* GEMINI_API_KEY = "TU_API_KEY_DE_GEMINI_AQUÍ";

// --- CONFIGURACIÓN DE PINES ---
const int RELAY_PIN = 26;          
const int MOISTURE_SENSOR_PIN = 34; 

// --- CONFIGURACIÓN DEL SENSOR ---
const int MOISTURE_WET_VALUE = 1500;  
const int MOISTURE_DRY_VALUE = 3300;  

// --- VARIABLES GLOBALES ---
WebServer server(80);
bool relayState = false;
int moistureValue = 0;
int moisturePercentage = 0;
String moistureStatusText = "Desconocido";
String treeColor = "#A1887F"; 

// --- VARIABLES PARA LA PROGRAMACIÓN DE RIEGO ---
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", -18000, 60000);

String wateringTime1 = "07:00";
String wateringTime2 = "19:00";
int wateringDurationSeconds = 30; 

bool hasWateredToday1 = false;
bool hasWateredToday2 = false;
int lastCheckedDay = -1;

void checkMoisture() {
  moistureValue = analogRead(MOISTURE_SENSOR_PIN);
  moisturePercentage = map(moistureValue, MOISTURE_DRY_VALUE, MOISTURE_WET_VALUE, 0, 100);
  moisturePercentage = constrain(moisturePercentage, 0, 100);

  if (moistureValue < MOISTURE_WET_VALUE) {
    moistureStatusText = "Tierra Humeda";
    treeColor = "#2E7D32"; 
  } else if (moistureValue >= MOISTURE_WET_VALUE && moistureValue < MOISTURE_DRY_VALUE) {
    moistureStatusText = "Humedad Normal";
    treeColor = "#66BB6A"; 
  } else {
    moistureStatusText = "Tierra Seca";
    treeColor = "#A1887F"; 
  }
}

bool preguntarAIARegar() {
  if (WiFi.status() != WL_CONNECTED) return true; 

  HTTPClient http;
  String url = "https://googleapis.com" + String(GEMINI_API_KEY);
  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  String prompt = "Actuas como un agronomo. El sensor mide un " + String(moisturePercentage) + "% de humedad. ";
  prompt += "Responde estrictamente con una palabra: REGAR o ESPERAR.";

  String payload = "{\"contents\":[{\"parts\":[{\"text\":\"" + prompt + "\"}]}]}";
  int httpResponseCode = http.POST(payload);
  bool decisionRiego = false;

  if (httpResponseCode > 0) {
    String response = http.getString();
    DynamicJsonDocument doc(4096);
    if (!deserializeJson(doc, response)) {
      String respuestaIA = doc["candidates"][0]["content"]["parts"][0]["text"].as<String>();
      respuestaIA.trim();
      if (respuestaIA == "REGAR") decisionRiego = true;
    }
  }
  http.end();
  return decisionRiego;
}

void setRelay(bool state) {
  relayState = state;
  digitalWrite(RELAY_PIN, relayState ? HIGH : LOW); 
}

void handleRoot() {
  checkMoisture();
  sendHtml();
}

void handleManualToggle() {
  setRelay(!relayState);
  sendHtml();
}

void handleSetSchedule() {
  if (server.hasArg("time1") && server.hasArg("time2") && server.hasArg("duration")) {
    wateringTime1 = server.arg("time1");
    wateringTime2 = server.arg("time2");
    wateringDurationSeconds = server.arg("duration").toInt();
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

  if (currentTime == wateringTime1 && !hasWateredToday1) {
    checkMoisture();
    if (preguntarAIARegar()) {
      setRelay(true);
      delay(wateringDurationSeconds * 1000);
      setRelay(false);
    }
    hasWateredToday1 = true;
  }

  if (currentTime == wateringTime2 && !hasWateredToday2) {
    checkMoisture();
    if (preguntarAIARegar()) {
      setRelay(true);
      delay(wateringDurationSeconds * 1000);
      setRelay(false);
    }
    hasWateredToday2 = true;
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(RELAY_PIN, OUTPUT);
  setRelay(false); 

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD, WIFI_CHANNEL);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(WiFi.localIP());

  timeClient.begin();
  server.on("/", handleRoot);
  server.on("/manualToggle", handleManualToggle);
  server.on("/setSchedule", handleSetSchedule);
  server.begin();
}

void loop() {
  server.handleClient();
  handleAutomaticWatering();
  
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate > 5000) {
    lastUpdate = millis();
    checkMoisture();
  }
}
