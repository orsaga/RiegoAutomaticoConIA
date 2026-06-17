#ifndef INTERFAZ_H
#define INTERFAZ_H

#include <WebServer.h>

extern WebServer server;
extern bool relayState;
extern int moisturePercentage;
extern String moistureStatusText;
extern String treeColor;
extern String wateringTime1;
extern String wateringTime2;
extern int wateringDurationSeconds;

void sendHtml() {
  String response = "<!DOCTYPE html><html><head>";
  response += "<title>Sistema de Riego ESP32 + IA</title>";
  response += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  response += "<style>";
  response += "html { font-family: 'Segoe UI', sans-serif; text-align: center; background-color: #f0f0f0; }";
  response += "body { max-width: 600px; margin: 0 auto; padding: 20px; }";
  response += "h1 { color: #00695C; }"; 
  response += ".card { background-color: white; padding: 20px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); margin-bottom: 20px; }";
  response += ".tree-container { margin: 20px 0; }";
  response += "#tree-svg { transition: fill 0.5s ease; }";
  response += ".status { font-size: 1.5em; font-weight: bold; margin-bottom: 15px; color: #333; }";
  response += ".btn { display: block; background-color: #4CAF50; border: none; color: #fff; padding: 15px 30px; font-size: 1.2em; text-decoration: none; border-radius: 5px; cursor: pointer; transition: background-color 0.3s; }";
  response += ".btn.OFF { background-color: #F44336; }";
  response += ".form-group { margin-bottom: 15px; text-align: left; }";
  response += ".form-group label { display: block; margin-bottom: 5px; color: #555; }";
  response += ".form-group input { width: 100%; padding: 8px; box-sizing: border-box; border: 1px solid #ccc; border-radius: 4px; }";
  response += ".submit-btn { background-color: #007BFF; color: white; border: none; padding: 10px 15px; border-radius: 5px; font-size: 1em; cursor: pointer; }";
  response += "</style></head><body>";
  
  response += "<h1>Riego Autom&aacutetico con IA</h1>";
  response += "<div class='card'><h2>Estado de Humedad</h2><div class='tree-container'>";
  response += "<svg id='tree-svg' width='100' height='120' viewBox='0 0 120 140'>";
  response += "<rect x='45' y='90' width='30' height='50' fill='#795548'/>";
  response += "<circle cx='60' cy='60' r='50' fill='TREE_COLOR'/>";
  response += "<circle cx='30' cy='70' r='30' fill='TREE_COLOR'/>";
  response += "<circle cx='90' cy='70' r='30' fill='TREE_COLOR'/>";
  response += "</svg></div>";
  response += "<p class='status'>MOISTURE_STATUS_TEXT (HUMIDITY_PERCENT%)</p></div>";

  response += "<div class='card'><h2>Control Manual Force</h2><p>Activa o desactiva la bomba de agua manualmente.</p>";
  response += "<a href='/manualToggle' class='btn RELAY_BUTTON_CLASS'>RELAY_BUTTON_TEXT</a></div>";

  response += "<div class='card'><h2>Evaluacion por Horario Programado</h2>";
  response += "<p>Al llegar la hora, el ESP32 consultara a la IA Gemini si es necesario regar.</p>";
  response += "<form action='/setSchedule' method='POST'>";
  response += "<div class='form-group'><label for='time1'>Primera hora de evaluacion:</label>";
  response += "<input type='time' id='time1' name='time1' value='TIME1_VALUE'></div>";
  response += "<div class='form-group'><label for='time2'>Segunda hora de evaluacion:</label>";
  response += "<input type='time' id='time2' name='time2' value='TIME2_VALUE'></div>";
  response += "<div class='form-group'><label for='duration'>Duracion maxima del riego (segundos):</label>";
  response += "<input type='number' id='duration' name='duration' min='1' value='DURATION_VALUE'></div>";
  response += "<button type='submit' class='submit-btn'>Guardar Programacion</button></form></div>";
  response += "</body></html>";

  response.replace("TREE_COLOR", treeColor);
  response.replace("MOISTURE_STATUS_TEXT", moistureStatusText);
  response.replace("HUMIDITY_PERCENT", String(moisturePercentage));
  
  if (relayState) {
    response.replace("RELAY_BUTTON_TEXT", "Apagar Riego Manualmente");
    response.replace("RELAY_BUTTON_CLASS", "OFF");
  } else {
    response.replace("RELAY_BUTTON_TEXT", "Encender Riego Manualmente");
    response.replace("RELAY_BUTTON_CLASS", "");
  }
  
  response.replace("TIME1_VALUE", wateringTime1);
  response.replace("TIME2_VALUE", wateringTime2);
  response.replace("DURATION_VALUE", String(wateringDurationSeconds));
  
  server.send(200, "text/html", response);
}

#endif
