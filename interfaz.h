#ifndef INTERFAZ_H
#define INTERFAZ_H

#include <WiFi.h>
#include <WebServer.h>

// Variables que viven en riego_ia.ino
extern WebServer server;
extern bool systemRunning;
extern bool rainForecast;
extern int moisturePercentage;
extern String moistureStatusText;
extern String treeColor;
extern String wateringTime1;
extern String wateringTime2;
extern int wateringDurationSeconds;

void sendHtml() {
  String response = R"HTML(
<!DOCTYPE html>
<html lang="es">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Sistema de Riego Automático</title>
<style>
  * { margin: 0; padding: 0; box-sizing: border-box; }
  :root {
    --primary: #00695C; --primary-light: #00897B;
    --accent: #4CAF50; --accent-dark: #388E3C;
    --danger: #F44336; --warning: #FF9800; --info: #2196F3;
    --bg-light: #f5f7fa; --bg-card: #ffffff;
    --text-dark: #212529; --text-light: #6c757d; --border: #e9ecef;
    --shadow: 0 2px 12px rgba(0,105,92,0.1);
  }
  body {
    font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
    background: linear-gradient(135deg, #f5f7fa 0%, #e8eef5 100%);
    color: var(--text-dark); min-height: 100vh; padding: 10px;
  }
  .container { max-width: 700px; margin: 0 auto; }
  .header {
    background: linear-gradient(135deg, var(--primary), var(--primary-light));
    color: white; padding: 22px; border-radius: 12px; margin-bottom: 20px;
    display: flex; justify-content: space-between; align-items: center;
    flex-wrap: wrap; gap: 10px;
  }
  .header h1 { font-size: 1.6em; }
  .status-item {
    display: flex; align-items: center; gap: 8px;
    background: rgba(255,255,255,0.2); padding: 8px 12px;
    border-radius: 6px; font-size: 0.9em;
  }
  .dot { width: 12px; height: 12px; border-radius: 50%; background: #4CAF50; }
  .dot.off { background: #FFC107; }
  .card {
    background: var(--bg-card); border-radius: 12px; padding: 20px;
    margin-bottom: 20px; box-shadow: var(--shadow); border: 1px solid var(--border);
  }
  .card h2 {
    font-size: 1.2em; color: var(--primary); margin-bottom: 6px;
  }
  .card-description { color: var(--text-light); font-size: 0.9em; margin-bottom: 15px; }
  .tree-container { display: flex; flex-direction: column; align-items: center; gap: 12px; margin: 10px 0 15px; }
  .moisture-bar { width: 100%; height: 8px; background: var(--border); border-radius: 10px; overflow: hidden; }
  .moisture-fill { height: 100%; background: linear-gradient(90deg, #ff6b6b, #ffa500, #4CAF50); }
  .moisture-status {
    text-align: center; padding: 14px; width: 100%;
    background: var(--bg-light); border-radius: 8px; border-left: 4px solid var(--primary);
  }
  .moisture-status-text { font-weight: 600; color: var(--primary); margin-bottom: 4px; }
  .moisture-percentage { font-size: 1.4em; font-weight: bold; }
  .btn {
    display: block; padding: 14px 20px; border: none; border-radius: 8px;
    font-size: 1em; font-weight: 600; text-decoration: none; cursor: pointer;
    width: 100%; text-align: center; margin-bottom: 10px; color: white;
  }
  .btn-primary { background: var(--accent); }
  .btn-danger { background: var(--danger); }
  .btn-info { background: var(--info); }
  .btn-warning { background: var(--warning); }
  .form-group { margin-bottom: 14px; }
  .form-group label { display: block; margin-bottom: 6px; font-weight: 600; font-size: 0.95em; }
  .form-group input {
    width: 100%; padding: 10px 12px; border: 2px solid var(--border);
    border-radius: 6px; font-size: 1em; background: var(--bg-light);
  }
  .form-row { display: grid; grid-template-columns: 1fr 1fr; gap: 12px; }
  @media (max-width: 600px) { .form-row { grid-template-columns: 1fr; } }
  .info-box {
    background: #e8f5e9; border-left: 4px solid var(--accent);
    padding: 12px; border-radius: 6px; margin-top: 12px; font-size: 0.9em;
  }
  .badge { display: inline-block; padding: 4px 10px; border-radius: 12px; font-size: 0.8em; font-weight: 600; }
  .badge-success { background: rgba(76,175,80,0.2); color: #2E7D32; }
  .badge-danger { background: rgba(244,67,54,0.2); color: #C62828; }
  .grid2 { display: grid; grid-template-columns: 1fr 1fr; gap: 12px; }
  .box { background: var(--bg-light); padding: 12px; border-radius: 8px; text-align: center; border: 2px solid var(--border); }
  .box-label { font-size: 0.8em; color: var(--text-light); text-transform: uppercase; margin-bottom: 6px; }
  .box-value { font-size: 1.4em; font-weight: bold; color: var(--primary); }
  .footer { text-align: center; color: var(--text-light); font-size: 0.85em; margin: 25px 0 15px; }
  .notification {
    position: fixed; bottom: 20px; right: 20px; background: white;
    padding: 14px 18px; border-radius: 8px; box-shadow: 0 4px 16px rgba(0,0,0,0.2);
    border-left: 4px solid var(--accent); max-width: 300px; z-index: 1000;
  }
  .notification.error { border-left-color: var(--danger); }
</style>
</head>
<body>
<div class="container">

  <div class="header">
    <h1>🌱 Riego Automático</h1>
    <div class="status-item">
      <span class="dot WIFI_DOT_CLASS"></span>
      <span>WIFI_STATUS_TEXT</span>
    </div>
  </div>

  <div class="card">
    <h2>💧 Pronóstico y estado</h2>
    <p class="card-description">Probabilidad de lluvia en las próximas horas (La Dorada)</p>
    <div class="tree-container">
      <svg id="tree-svg" width="140" height="170" viewBox="0 0 120 140">
        <rect x="45" y="90" width="30" height="50" fill="#795548" rx="5"/>
        <circle cx="60" cy="60" r="50" fill="TREE_COLOR"/>
        <circle cx="30" cy="70" r="30" fill="TREE_COLOR"/>
        <circle cx="90" cy="70" r="30" fill="TREE_COLOR"/>
      </svg>
      <div class="moisture-bar"><div class="moisture-fill" style="width: HUMIDITY_PERCENT%"></div></div>
      <div class="moisture-status">
        <div class="moisture-status-text">MOISTURE_STATUS_TEXT</div>
        <div class="moisture-percentage">HUMIDITY_PERCENT%</div>
      </div>
    </div>
    <a href="/refreshWeather" class="btn btn-info">🔄 Actualizar pronóstico</a>
  </div>

  <div class="card">
    <h2>🚀 Control manual</h2>
    <p class="card-description">Riega los 4 aspersores uno tras otro, sin importar el pronóstico</p>
    RELAY_BUTTON_HTML
    FORCE_BUTTON_HTML
    <div class="info-box">
      <strong>ℹ️ Nota:</strong> Cada aspersor riega la duración configurada abajo. Puedes cancelar desde esta página o con el botón físico.
    </div>
  </div>

  <div class="card">
    <h2>⏰ Programación de riego</h2>
    <p class="card-description">A estas horas el sistema revisa el clima y riega si está despejado</p>
    <form action="/setSchedule" method="POST" id="scheduleForm">
      <div class="form-row">
        <div class="form-group">
          <label for="time1">🕐 Primera evaluación</label>
          <input type="time" id="time1" name="time1" value="TIME1_VALUE" required>
        </div>
        <div class="form-group">
          <label for="time2">🕑 Segunda evaluación</label>
          <input type="time" id="time2" name="time2" value="TIME2_VALUE" required>
        </div>
      </div>
      <div class="form-group">
        <label for="duration">⏱️ Duración por aspersor (segundos)</label>
        <input type="number" id="duration" name="duration" min="1" max="300" value="DURATION_VALUE" required>
      </div>
      <button type="submit" class="btn btn-info">💾 Guardar programación</button>
    </form>
  </div>

  <div class="card">
    <h2>⚙️ Información del sistema</h2>
    <div class="grid2" style="margin-top:12px;">
      <div class="box">
        <div class="box-label">Estado</div>
        RELAY_STATUS_BADGE
      </div>
      <div class="box">
        <div class="box-label">Riego programado</div>
        <div class="box-value" style="font-size:1.1em;">TIME1_VALUE y TIME2_VALUE</div>
      </div>
    </div>
  </div>

  <div class="footer">
    Sistema de riego con ESP32 y Open-Meteo<br>La Dorada, Caldas | Colombia 🇨🇴
  </div>
</div>

<script>
  function showNotification(message, type) {
    var n = document.createElement('div');
    n.className = 'notification' + (type === 'error' ? ' error' : '');
    n.textContent = message;
    document.body.appendChild(n);
    setTimeout(function() { n.remove(); }, 3000);
  }

  document.getElementById('scheduleForm').addEventListener('submit', function(e) {
    var d = parseInt(document.getElementById('duration').value);
    if (isNaN(d) || d < 1 || d > 300) {
      e.preventDefault();
      showNotification('La duración debe estar entre 1 y 300 segundos', 'error');
    }
  });

  // Mientras hay riego en curso, vuelve a la página principal cada 3 segundos
  // para ver el estado actualizado (aspersor activo).
  if (IS_RUNNING) {
    setTimeout(function() { window.location.href = '/'; }, 3000);
  }
</script>
</body>
</html>
)HTML";

  // --- Reemplazo de variables ---
  response.replace("TREE_COLOR", treeColor);
  response.replace("MOISTURE_STATUS_TEXT", moistureStatusText);
  response.replace("HUMIDITY_PERCENT", String(moisturePercentage));
  response.replace("TIME1_VALUE", wateringTime1);
  response.replace("TIME2_VALUE", wateringTime2);
  response.replace("DURATION_VALUE", String(wateringDurationSeconds));
  response.replace("IS_RUNNING", systemRunning ? "true" : "false");

  // Estado real del router
  if (WiFi.status() == WL_CONNECTED) {
    response.replace("WIFI_DOT_CLASS", "");
    response.replace("WIFI_STATUS_TEXT", "Router conectado");
  } else {
    response.replace("WIFI_DOT_CLASS", "off");
    response.replace("WIFI_STATUS_TEXT", "Sin router (solo red propia)");
  }

  // Botón principal y estado
  if (systemRunning) {
    response.replace("RELAY_BUTTON_HTML",
      "<a href='/manualToggle' class='btn btn-danger'><strong>🛑 DETENER RIEGO</strong></a>");
    response.replace("RELAY_STATUS_BADGE", "<span class='badge badge-danger'>REGANDO 🔴</span>");
    response.replace("FORCE_BUTTON_HTML", "");
  } else {
    response.replace("RELAY_BUTTON_HTML",
      "<a href='/manualToggle' class='btn btn-primary'><strong>▶️ INICIAR RIEGO AHORA</strong></a>");
    response.replace("RELAY_STATUS_BADGE", "<span class='badge badge-success'>EN ESPERA ✓</span>");
    if (rainForecast) {
      response.replace("FORCE_BUTTON_HTML",
        "<a href='/forceWater' class='btn btn-warning'><strong>⚠️ FORZAR RIEGO (hay pronóstico de lluvia)</strong></a>");
    } else {
      response.replace("FORCE_BUTTON_HTML", "");
    }
  }

  server.send(200, "text/html", response);
}

#endif
