#ifndef INTERFAZ_H
#define INTERFAZ_H

#include <WebServer.h>

extern WebServer server;
extern bool systemRunning;
extern bool rainForecast;
extern int moisturePercentage;
extern String moistureStatusText;
extern String treeColor;
extern String wateringTime1;
extern String wateringTime2;
extern int wateringDurationSeconds;

// showRainWarning=true muestra advertencia de lluvia con botón para forzar el riego
void sendHtml(bool showRainWarning = false) {
  String response = R"(
<!DOCTYPE html>
<html lang="es">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>🌱 Sistema de Riego Automático IA</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }

        :root {
            --primary: #00695C;
            --primary-dark: #004D40;
            --primary-light: #00897B;
            --accent: #4CAF50;
            --accent-dark: #388E3C;
            --danger: #F44336;
            --warning: #FF9800;
            --info: #2196F3;
            --success: #4CAF50;
            --bg-light: #f5f7fa;
            --bg-card: #ffffff;
            --text-dark: #212529;
            --text-light: #6c757d;
            --border: #e9ecef;
            --shadow: 0 2px 12px rgba(0, 105, 92, 0.1);
            --shadow-lg: 0 8px 24px rgba(0, 105, 92, 0.15);
        }

        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #f5f7fa 0%, #e8eef5 100%);
            color: var(--text-dark);
            min-height: 100vh;
            padding: 10px;
        }

        .container {
            max-width: 700px;
            margin: 0 auto;
        }

        /* ═══ HEADER ═══ */
        .header {
            background: linear-gradient(135deg, var(--primary) 0%, var(--primary-light) 100%);
            color: white;
            padding: 25px;
            border-radius: 12px;
            margin-bottom: 25px;
            box-shadow: var(--shadow-lg);
            display: flex;
            justify-content: space-between;
            align-items: center;
        }

        .header-title {
            flex: 1;
        }

        .header h1 {
            font-size: 1.8em;
            margin-bottom: 5px;
            display: flex;
            align-items: center;
            gap: 10px;
        }

        .header-status {
            display: flex;
            gap: 15px;
            font-size: 0.9em;
        }

        .status-item {
            display: flex;
            align-items: center;
            gap: 5px;
            background: rgba(255, 255, 255, 0.2);
            padding: 8px 12px;
            border-radius: 6px;
            backdrop-filter: blur(10px);
        }

        .wifi-indicator {
            width: 12px;
            height: 12px;
            border-radius: 50%;
            background: #4CAF50;
            animation: pulse 2s infinite;
        }

        .wifi-indicator.offline {
            background: #F44336;
            animation: none;
        }

        @keyframes pulse {
            0%, 100% { opacity: 1; }
            50% { opacity: 0.5; }
        }

        /* ═══ CARDS ═══ */
        .card {
            background: var(--bg-card);
            border-radius: 12px;
            padding: 20px;
            margin-bottom: 20px;
            box-shadow: var(--shadow);
            transition: all 0.3s ease;
            border: 1px solid var(--border);
        }

        .card:hover {
            box-shadow: var(--shadow-lg);
            transform: translateY(-2px);
        }

        .card-header {
            display: flex;
            align-items: center;
            gap: 10px;
            margin-bottom: 15px;
            padding-bottom: 12px;
            border-bottom: 2px solid var(--border);
        }

        .card-header h2 {
            font-size: 1.3em;
            color: var(--primary);
            margin: 0;
        }

        .card-icon {
            font-size: 1.8em;
        }

        .card-description {
            color: var(--text-light);
            font-size: 0.9em;
            margin-bottom: 15px;
        }

        /* ═══ ÁRBOL DE HUMEDAD ═══ */
        .tree-container {
            display: flex;
            flex-direction: column;
            align-items: center;
            gap: 15px;
            margin: 20px 0;
        }

        .tree-svg-wrapper {
            position: relative;
            width: 150px;
            height: 180px;
            display: flex;
            align-items: center;
            justify-content: center;
        }

        #tree-svg {
            filter: drop-shadow(0 4px 8px rgba(0, 0, 0, 0.1));
            transition: all 0.5s ease;
        }

        .moisture-bar {
            width: 100%;
            height: 8px;
            background: var(--border);
            border-radius: 10px;
            overflow: hidden;
            margin-top: 10px;
        }

        .moisture-fill {
            height: 100%;
            background: linear-gradient(90deg, #ff6b6b, #ffa500, #4CAF50);
            transition: width 0.5s ease;
        }

        .moisture-status {
            text-align: center;
            padding: 15px;
            background: var(--bg-light);
            border-radius: 8px;
            border-left: 4px solid var(--primary);
        }

        .moisture-status-text {
            font-size: 1em;
            font-weight: 600;
            color: var(--primary);
            margin-bottom: 5px;
        }

        .moisture-percentage {
            font-size: 1.4em;
            font-weight: bold;
            color: var(--text-dark);
        }

        /* ═══ BOTONES ═══ */
        .btn {
            display: inline-block;
            padding: 14px 28px;
            border: none;
            border-radius: 8px;
            font-size: 1em;
            font-weight: 600;
            text-decoration: none;
            cursor: pointer;
            transition: all 0.3s ease;
            box-shadow: 0 2px 8px rgba(0, 0, 0, 0.1);
            width: 100%;
            text-align: center;
            margin-bottom: 10px;
        }

        .btn-primary {
            background: var(--accent);
            color: white;
        }

        .btn-primary:hover {
            background: var(--accent-dark);
            box-shadow: 0 4px 12px rgba(76, 175, 80, 0.3);
            transform: translateY(-2px);
        }

        .btn-danger {
            background: var(--danger);
            color: white;
        }

        .btn-danger:hover {
            background: #d32f2f;
            box-shadow: 0 4px 12px rgba(244, 67, 54, 0.3);
            transform: translateY(-2px);
        }

        .btn-info {
            background: var(--info);
            color: white;
        }

        .btn-info:hover {
            background: #1976D2;
            box-shadow: 0 4px 12px rgba(33, 150, 243, 0.3);
            transform: translateY(-2px);
        }

        .btn-small {
            padding: 8px 16px;
            font-size: 0.9em;
            width: auto;
            display: inline-block;
        }

        /* ═══ FORMULARIOS ═══ */
        .form-group {
            margin-bottom: 16px;
        }

        .form-group label {
            display: block;
            margin-bottom: 8px;
            font-weight: 600;
            color: var(--text-dark);
            font-size: 0.95em;
        }

        .form-group input,
        .form-group select {
            width: 100%;
            padding: 10px 12px;
            border: 2px solid var(--border);
            border-radius: 6px;
            font-size: 1em;
            font-family: inherit;
            transition: all 0.3s ease;
            background: var(--bg-light);
        }

        .form-group input:focus,
        .form-group select:focus {
            outline: none;
            border-color: var(--primary);
            background: white;
            box-shadow: 0 0 0 3px rgba(0, 105, 92, 0.1);
        }

        .form-row {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 12px;
        }

        @media (max-width: 600px) {
            .form-row {
                grid-template-columns: 1fr;
            }
        }

        /* ═══ INFO BOXES ═══ */
        .info-box {
            background: linear-gradient(135deg, var(--bg-light), #e8f5e9);
            border-left: 4px solid var(--accent);
            padding: 12px;
            border-radius: 6px;
            margin-bottom: 12px;
            font-size: 0.9em;
            color: var(--text-dark);
        }

        .info-box.warning {
            background: linear-gradient(135deg, #fff3e0, #ffe0b2);
            border-left-color: var(--warning);
        }

        .info-box.error {
            background: linear-gradient(135deg, #ffebee, #ffcdd2);
            border-left-color: var(--danger);
        }

        .info-box strong {
            color: var(--primary);
        }

        /* ═══ SCHEDULE INFO ═══ */
        .schedule-box {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 12px;
            margin-top: 15px;
        }

        .schedule-item {
            background: var(--bg-light);
            padding: 12px;
            border-radius: 8px;
            text-align: center;
            border: 2px solid var(--border);
        }

        .schedule-item-label {
            font-size: 0.85em;
            color: var(--text-light);
            text-transform: uppercase;
            margin-bottom: 5px;
        }

        .schedule-item-time {
            font-size: 1.5em;
            font-weight: bold;
            color: var(--primary);
        }

        .schedule-item-status {
            font-size: 0.75em;
            color: var(--text-light);
            margin-top: 5px;
        }

        /* ═══ TOGGLE SWITCH ═══ */
        .toggle-switch {
            display: flex;
            align-items: center;
            justify-content: center;
            gap: 10px;
            margin: 15px 0;
        }

        .switch {
            position: relative;
            display: inline-block;
            width: 60px;
            height: 34px;
        }

        .switch input {
            opacity: 0;
            width: 0;
            height: 0;
        }

        .slider {
            position: absolute;
            cursor: pointer;
            top: 0;
            left: 0;
            right: 0;
            bottom: 0;
            background-color: #ccc;
            transition: 0.4s;
            border-radius: 34px;
        }

        .slider:before {
            position: absolute;
            content: "";
            height: 26px;
            width: 26px;
            left: 4px;
            bottom: 4px;
            background-color: white;
            transition: 0.4s;
            border-radius: 50%;
        }

        input:checked + .slider {
            background-color: var(--accent);
        }

        input:checked + .slider:before {
            transform: translateX(26px);
        }

        /* ═══ NOTIFICATIONS ═══ */
        .notification {
            position: fixed;
            bottom: 20px;
            right: 20px;
            background: white;
            padding: 16px 20px;
            border-radius: 8px;
            box-shadow: var(--shadow-lg);
            border-left: 4px solid var(--success);
            animation: slideIn 0.3s ease;
            z-index: 1000;
            max-width: 300px;
        }

        .notification.error {
            border-left-color: var(--danger);
        }

        @keyframes slideIn {
            from {
                transform: translateX(400px);
                opacity: 0;
            }
            to {
                transform: translateX(0);
                opacity: 1;
            }
        }

        /* ═══ BADGE ═══ */
        .badge {
            display: inline-block;
            padding: 4px 8px;
            border-radius: 12px;
            font-size: 0.75em;
            font-weight: 600;
            text-transform: uppercase;
        }

        .badge-success {
            background: rgba(76, 175, 80, 0.2);
            color: var(--accent-dark);
        }

        .badge-warning {
            background: rgba(255, 152, 0, 0.2);
            color: #E65100;
        }

        .badge-danger {
            background: rgba(244, 67, 54, 0.2);
            color: #C62828;
        }

        /* ═══ RESPONSIVE ═══ */
        @media (max-width: 600px) {
            .header {
                flex-direction: column;
                gap: 12px;
                text-align: center;
            }

            .header h1 {
                justify-content: center;
                font-size: 1.5em;
            }

            .header-status {
                flex-direction: column;
                width: 100%;
            }

            .status-item {
                justify-content: center;
            }

            .card {
                padding: 16px;
            }

            .schedule-box {
                grid-template-columns: 1fr;
            }
        }

        /* ═══ ANIMACIONES ═══ */
        @keyframes fadeIn {
            from {
                opacity: 0;
                transform: translateY(10px);
            }
            to {
                opacity: 1;
                transform: translateY(0);
            }
        }

        .card {
            animation: fadeIn 0.5s ease;
        }

        /* ═══ FOOTER ═══ */
        .footer {
            text-align: center;
            color: var(--text-light);
            font-size: 0.85em;
            margin-top: 30px;
            padding-bottom: 20px;
        }

        .footer-link {
            color: var(--primary);
            text-decoration: none;
            font-weight: 600;
        }

        .footer-link:hover {
            text-decoration: underline;
        }
    </style>
</head>
<body>
    <div class="container">
        <!-- HEADER -->
        <div class="header">
            <div class="header-title">
                <h1>🌱 Riego Automático IA</h1>
            </div>
            <div class="header-status">
                <div class="status-item">
                    <span class="wifi-indicator"></span>
                    <span>WiFi Conectado</span>
                </div>
            </div>
        </div>

        <!-- CARD: ESTADO DE HUMEDAD -->
        <div class="card">
            <div class="card-header">
                <span class="card-icon">💧</span>
                <h2>Estado de Humedad</h2>
            </div>
            <p class="card-description">Nivel de humedad actual y pronóstico de riego (4 electroválvulas en cascada)</p>
            
            <div class="tree-container">
                <div class="tree-svg-wrapper">
                    <svg id="tree-svg" width="140" height="170" viewBox="0 0 120 140">
                        <!-- Tronco -->
                        <rect x="45" y="90" width="30" height="50" fill="#795548" rx="5"/>
                        <!-- Ramas principales -->
                        <circle cx="60" cy="60" r="50" fill="TREE_COLOR"/>
                        <circle cx="30" cy="70" r="30" fill="TREE_COLOR"/>
                        <circle cx="90" cy="70" r="30" fill="TREE_COLOR"/>
                        <!-- Raíces -->
                        <path d="M 45 140 Q 40 145 35 145" stroke="#795548" stroke-width="2" fill="none"/>
                        <path d="M 75 140 Q 80 145 85 145" stroke="#795548" stroke-width="2" fill="none"/>
                    </svg>
                </div>

                <div class="moisture-bar">
                    <div class="moisture-fill" style="width: HUMIDITY_PERCENT%"></div>
                </div>

                <div class="moisture-status">
                    <div class="moisture-status-text">MOISTURE_STATUS_TEXT</div>
                    <div class="moisture-percentage">HUMIDITY_PERCENT%</div>
                </div>
            </div>
        </div>

        <!-- CARD: CONTROL MANUAL -->
        <div class="card">
            <div class="card-header">
                <span class="card-icon">🚀</span>
                <h2>Control Manual (Web)</h2>
            </div>
            <p class="card-description">Inicia o cancela el ciclo completo de riego en cascada (Aspersor 1 a 4) desde la web</p>
            
            RELAY_BUTTON_HTML

            <div class="info-box">
                <strong>🔘 Botón físico:</strong> El sistema también cuenta con un botón físico normalmente abierto conectado directamente al ESP32. Funciona aunque no haya WiFi ni internet: un toque inicia la cascada de riego y otro toque la cancela.
            </div>

            <div class="info-box warning">
                <strong>⚠️ Nota:</strong> El ciclo recorre las 4 electroválvulas en cascada (una a la vez); puedes cancelarlo en cualquier momento desde la web o con el botón físico.
            </div>
        </div>

        <!-- CARD: PROGRAMACIÓN -->
        <div class="card">
            <div class="card-header">
                <span class="card-icon">⏰</span>
                <h2>Programación de Riego</h2>
            </div>
            <p class="card-description">Configura los horarios de evaluación automática</p>

            <form action="/setSchedule" method="POST" id="scheduleForm">
                <div class="form-row">
                    <div class="form-group">
                        <label for="time1">🕐 Primera Evaluación:</label>
                        <input type="time" id="time1" name="time1" value="TIME1_VALUE" required>
                    </div>
                    <div class="form-group">
                        <label for="time2">🕑 Segunda Evaluación:</label>
                        <input type="time" id="time2" name="time2" value="TIME2_VALUE" required>
                    </div>
                </div>

                <div class="form-group">
                    <label for="duration">⏱️ Duración por Aspersor:</label>
                    <input type="number" id="duration" name="duration" min="1" max="300" value="DURATION_VALUE" required>
                    <small style="color: var(--text-light);">En segundos por cada una de las 4 electroválvulas (máximo 300)</small>
                </div>

                <button type="submit" class="btn btn-info">💾 Guardar Programación</button>
            </form>

            <div class="schedule-box">
                <div class="schedule-item">
                    <div class="schedule-item-label">Próxima Evaluación 1</div>
                    <div class="schedule-item-time">TIME1_VALUE</div>
                    <div class="schedule-item-status"><span class="badge badge-warning">Pendiente</span></div>
                </div>
                <div class="schedule-item">
                    <div class="schedule-item-label">Próxima Evaluación 2</div>
                    <div class="schedule-item-time">TIME2_VALUE</div>
                    <div class="schedule-item-status"><span class="badge badge-warning">Pendiente</span></div>
                </div>
            </div>

            <div class="info-box" style="margin-top: 15px;">
                <strong>ℹ️ Cómo funciona:</strong> En cada horario programado, el sistema consultará a la IA Gemini si hay pronóstico de lluvia. Si está despejado, regará automáticamente en cascada (Aspersor 1, 2, 3 y 4) durante DURATION_VALUE segundos cada uno.
            </div>
        </div>

        <!-- CARD: INFORMACIÓN TÉCNICA -->
        <div class="card">
            <div class="card-header">
                <span class="card-icon">⚙️</span>
                <h2>Información del Sistema</h2>
            </div>

            <div style="display: grid; grid-template-columns: 1fr 1fr; gap: 12px; font-size: 0.9em;">
                <div class="schedule-item">
                    <div class="schedule-item-label">Estado del Sistema</div>
                    <div class="schedule-item-status">RELAY_STATUS_BADGE</div>
                </div>
                <div class="schedule-item">
                    <div class="schedule-item-label">Modo</div>
                    <div class="schedule-item-status"><span class="badge badge-success">Automático IA + Botón Físico</span></div>
                </div>
            </div>

            <div class="info-box" style="margin-top: 12px;">
                <strong>🔒 Seguridad:</strong> El botón físico y el botón web pueden cancelar el riego en curso en cualquier momento, incluso sin conexión a internet.
            </div>
        </div>

        <!-- FOOTER -->
        <div class="footer">
            <p>🌍 Sistema de Riego Inteligente con IA Gemini • Powered by ESP32<br/>
            <small>La Dorada, Caldas | Colombia 🇨🇴</small></p>
        </div>
    </div>

    <script>
        // Mostrar notificación de guardado
        const urlParams = new URLSearchParams(window.location.search);
        if (urlParams.has('saved')) {
            showNotification('✓ Configuración guardada correctamente', 'success');
        }

        function showNotification(message, type = 'success') {
            const notification = document.createElement('div');
            notification.className = `notification ${type === 'error' ? 'error' : ''}`;
            notification.textContent = message;
            document.body.appendChild(notification);
            
            setTimeout(() => {
                notification.style.animation = 'slideOut 0.3s ease';
                setTimeout(() => notification.remove(), 300);
            }, 3000);
        }

        // Validar formulario antes de enviar
        document.getElementById('scheduleForm').addEventListener('submit', function(e) {
            const time1 = document.getElementById('time1').value;
            const time2 = document.getElementById('time2').value;
            const duration = parseInt(document.getElementById('duration').value);

            if (!time1 || !time2) {
                e.preventDefault();
                showNotification('⚠️ Por favor completa ambos horarios', 'error');
                return false;
            }

            if (duration < 1 || duration > 300) {
                e.preventDefault();
                showNotification('⚠️ La duración debe estar entre 1 y 300 segundos', 'error');
                return false;
            }

            showNotification('💾 Guardando configuración...', 'success');
        });

        // Animar el árbol según el porcentaje
        function updateTreeColor(color) {
            const treeSvg = document.getElementById('tree-svg');
            if (treeSvg) {
                treeSvg.style.fill = color;
            }
        }

        // Actualizar color del árbol al cargar
        updateTreeColor('TREE_COLOR');
    </script>
</body>
</html>
  )";

  // Reemplazos de variables
  response.replace("TREE_COLOR", treeColor);
  response.replace("MOISTURE_STATUS_TEXT", moistureStatusText);
  response.replace("HUMIDITY_PERCENT", String(moisturePercentage));
  
  if (systemRunning) {
    response.replace("RELAY_BUTTON_HTML",
      "<a href='/manualToggle' class='btn btn-danger'><strong>🛑 CANCELAR RIEGO EN CURSO</strong></a>"
    );
    response.replace("RELAY_STATUS_BADGE", "<span class='badge badge-danger'>REGANDO 🔴</span>");
  } else if (showRainWarning) {
    // Lluvia pronosticada: mostramos advertencia + opción de forzar
    response.replace("RELAY_BUTTON_HTML",
      "<div class='info-box error' style='margin-bottom:12px;'>"
        "<strong>🌧️ Riego bloqueado por pronóstico de LLUVIA.</strong><br/>"
        "La IA detectó lluvia para hoy. Si aun así necesitas regar, pulsa el botón de abajo."
      "</div>"
      "<a href='/forceWater' class='btn btn-danger'>"
        "<strong>⚠️ FORZAR RIEGO (ignorar lluvia)</strong>"
      "</a>"
      "<a href='/' class='btn btn-info' style='margin-top:6px;'>"
        "<strong>↩️ Cancelar</strong>"
      "</a>"
    );
    response.replace("RELAY_STATUS_BADGE", "<span class='badge badge-warning'>BLOQUEADO 🌧️</span>");
  } else {
    response.replace("RELAY_BUTTON_HTML",
      "<a href='/manualToggle' class='btn btn-primary'><strong>▶️ INICIAR RIEGO EN CASCADA (4 ASPERSORES)</strong></a>"
    );
    response.replace("RELAY_STATUS_BADGE", "<span class='badge badge-success'>INACTIVO ✓</span>");
  }
  
  response.replace("TIME1_VALUE", wateringTime1);
  response.replace("TIME2_VALUE", wateringTime2);
  response.replace("DURATION_VALUE", String(wateringDurationSeconds));
  
  server.send(200, "text/html", response);
}

#endif
