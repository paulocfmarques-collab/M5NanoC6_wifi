#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

namespace Config {
    // Pinos nativos do hardware M5NanoC6 validados pelo manual
    constexpr uint8_t PIN_IR_TX = 3;         // Emissor de Infravermelho
    constexpr uint8_t PIN_LED_AZUL = 7;      // LED azul de status
    constexpr uint8_t PIN_BOTAO = 9;         // Botão de usuário integrado
    constexpr uint8_t PIN_RGB_ENABLE = 19;   // Habilitação do barramento do NeoPixel
    constexpr uint8_t PIN_LED_RGB = 20;      // Pino de dados do NeoPixel RGB

    // Configurações de Rede UDP
    constexpr uint16_t UDP_PORT = 4210;

    // Fuso Horário Padrão (GMT-3 Brasília)
    constexpr int FUSO_PADRAO = -3;

    // --- FUNÇÕES UTILITÁRIAS DE DIAGNÓSTICO MOVIDAS PARA EVITAR DEPENDÊNCIA CÍCLICA ---
    static String obterMotivoReset() {
        esp_reset_reason_t motivo = esp_reset_reason();
        switch (motivo) {
            case ESP_RST_UNKNOWN: return "Desconhecido";
            case ESP_RST_POWERON: return "Ligado";
            case ESP_RST_EXT: return "Reset Externo";
            case ESP_RST_SW: return "Reset por Software";
            case ESP_RST_PANIC: return "Pânico";
            case ESP_RST_INT_WDT: return "Watchdog Interno";
            case ESP_RST_TASK_WDT: return "Watchdog de Tarefa";
            case ESP_RST_WDT: return "Watchdog Geral";
            case ESP_RST_DEEPSLEEP: return "Saída do Deep Sleep";
            case ESP_RST_BROWNOUT: return "Brownout";
            case ESP_RST_SDIO: return "Reset via SDIO";
            default: return "Desconhecido";
        }
    }
    
    static String obterVersaoAutomatica() {
        int ano = ((__DATE__[9] - '0') * 10) + (__DATE__[10] - '0');
        int mes = (__DATE__[0] == 'J' && __DATE__[1] == 'a' && __DATE__[2] == 'n') ? 1 :
                  (__DATE__[0] == 'F')                                             ? 2 :
                  (__DATE__[0] == 'M' && __DATE__[1] == 'a' && __DATE__[2] == 'r') ? 3 :
                  (__DATE__[0] == 'A' && __DATE__[1] == 'p')                       ? 4 :
                  (__DATE__[0] == 'M' && __DATE__[1] == 'a' && __DATE__[2] == 'y') ? 5 :
                  (__DATE__[0] == 'J' && __DATE__[1] == 'u' && __DATE__[2] == 'n') ? 6 :
                  (__DATE__[0] == 'J' && __DATE__[1] == 'u' && __DATE__[2] == 'l') ? 7 :
                  (__DATE__[0] == 'A' && __DATE__[1] == 'u')                       ? 8 :
                  (__DATE__[0] == 'S')                                             ? 9 :
                  (__DATE__[0] == 'O')                                             ? 10 :
                  (__DATE__[0] == 'N')                                             ? 11 :
                  (__DATE__[0] == 'D')                                             ? 12 : 0;
        int dia = (__DATE__[4] == ' ' ? 0 : __DATE__[4] - '0') * 10 + (__DATE__[5] - '0');
        int hora   = ((__TIME__[0] - '0') * 10) + (__TIME__[1] - '0');
        int minuto = ((__TIME__[3] - '0') * 10) + (__TIME__[4] - '0');

        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%02d%02d%02d.%02d.%02d", ano, mes, dia, hora, minuto);
        return String(buffer);
    }
}

// Interface HTML limpa para o portal Web de configuração
const char htmlPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
<meta charset="UTF-8"><meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>M5NanoC6 - Configuração</title>
<style>
  body { font-family: Arial, sans-serif; margin: 40px; background-color: #1a1a1a; color: #ffffff; text-align: center; }
  .container { background: #2d2d2d; max-width: 320px; margin: auto; padding: 25px; border-radius: 12px; box-shadow: 0 4px 15px rgba(0,0,0,0.5); }
  input { width: 100%; padding: 10px; margin: 12px 0; box-sizing: border-box; border-radius: 6px; border: 1px solid #444; background: #222; color: #fff; }
  input[type="submit"] { background: #ff5e00; color: white; border: none; cursor: pointer; font-weight: bold; }
  input[type="submit"]:hover { background: #e05300; }
</style>
</head>
<body>
<div class="container">
  <h2>M5NanoC6 WiFi</h2>
  <form action="/salvar" method="POST">
    <label>SSID da Rede:</label><input type="text" name="ssid" placeholder="Nome do WiFi" required>
    <label>Senha:</label><input type="password" name="senha" placeholder="Senha do WiFi">
    <input type="submit" value="Salvar e Conectar">
  </form>
</div>
</body>
</html>
)rawliteral";

const char htmlInfoPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>M5NanoC6 - Dashboard de Controle</title>
<style>
  :root { --primary: #ff5e00; --bg: #0f0f12; --card: #191922; --text: #e2e8f0; --text-muted: #718096; --success: #10b981; }
  body { font-family: 'Segoe UI', system-ui, sans-serif; margin: 0; padding: 20px; background-color: var(--bg); color: var(--text); }
  .dashboard { max-width: 1000px; margin: 0 auto; display: grid; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); gap: 20px; padding: 20px 0; }
  header { text-align: center; margin-bottom: 10px; border-bottom: 2px solid #2d2d3d; padding-bottom: 20px; }
  header h1 { color: var(--primary); margin: 0; font-size: 2rem; }
  header p { color: var(--text-muted); margin: 5px 0 0 0; font-family: monospace; }
  .card { background: var(--card); border-radius: 12px; padding: 20px; box-shadow: 0 4px 20px rgba(0,0,0,0.4); border: 1px solid #252535; }
  .card h2 { font-size: 1.2rem; margin-top: 0; margin-bottom: 15px; color: var(--primary); display: flex; align-items: center; justify-content: space-between; border-bottom: 1px solid #2d2d3d; padding-bottom: 8px; }
  .row { display: flex; justify-content: space-between; padding: 8px 0; border-bottom: 1px solid #1f1f2e; font-family: monospace; }
  .row:last-child { border-bottom: none; }
  .label { color: var(--text-muted); font-weight: 500; }
  .value { color: #ffffff; font-weight: 600; text-align: right; }
  .status-badge { background: var(--success); color: #000; padding: 2px 8px; border-radius: 20px; font-size: 0.8rem; font-weight: bold; }
  .actions { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; margin-top: 15px; }
  .btn { background: #2d2d3d; color: #fff; border: 1px solid #3d3d52; padding: 10px; border-radius: 6px; cursor: pointer; font-weight: bold; text-decoration: none; text-align: center; transition: all 0.2s; font-size: 0.9rem; }
  .btn:hover { background: var(--primary); border-color: var(--primary); color: #fff; }
  .btn-danger:hover { background: #ef4444; border-color: #ef4444; }
  form { margin: 0; }
  .inline-form { display: flex; gap: 5px; margin-top: 10px; }
  .inline-form input, .inline-form select { background: #252535; border: 1px solid #3d3d52; color: #fff; padding: 8px; border-radius: 6px; flex-grow: 1; }
</style>
</head>
<body>

<header>
  <h1>M5NanoC6 IoT Dashboard</h1>
  <p>Firmware v%FIRMWARE_VER% | Compilação: %BUILD_DATA%</p>
</header>

<div class="dashboard">
  <!-- SEÇÃO: STATUS OPERACIONAL -->
  <div class="card">
    <h2>⚡ Operação <span class="status-badge">ONLINE</span></h2>
    <div class="row"><span class="label">Uptime do Sistema</span><span class="value">%UPTIME%</span></div>
    <div class="row"><span class="label">Último Reset</span><span class="value">%MOTIVO_RESET%</span></div>
    <div class="row"><span class="label">Temperatura CPU</span><span class="value">%CPU_TEMP% °C</span></div>
    <div class="row"><span class="label">Status NTP</span><span class="value">%NTP_STATUS%</span></div>
    <div class="row"><span class="label">Data & Hora</span><span class="value">%DATA_HORA%</span></div>
  </div>

  <!-- SEÇÃO: HARDWARE & MEMÓRIA -->
  <div class="card">
    <h2>🧠 Hardware & Recursos</h2>
    <div class="row"><span class="label">Processador</span><span class="value">%CHIP_MODELO% (%CHIP_CORES% Cores)</span></div>
    <div class="row"><span class="label">Frequência CPU</span><span class="value">%CPU_FREQ% MHz</span></div>
    <div class="row"><span class="label">RAM Livre (Heap)</span><span class="value">%RAM_LIVRE% KB</span></div>
    <div class="row"><span class="label">Menor Heap Registrado</span><span class="value">%RAM_MIN_LIVRE% KB</span></div>
    <div class="row"><span class="label">Armazenamento Flash</span><span class="value">%FLASH_TAM% MB (%FLASH_LIVRE% KB Livres)</span></div>
  </div>

  <!-- SEÇÃO: REDE & CONEXÃO -->
  <div class="card">
    <h2>🌐 Conectividade</h2>
    <div class="row"><span class="label">SSID Conectado</span><span class="value">%WIFI_SSID%</span></div>
    <div class="row"><span class="label">Força do Sinal (RSSI)</span><span class="value">%WIFI_RSSI% dBm</span></div>
    <div class="row"><span class="label">Endereço IP</span><span class="value">%IP_LOCAL%</span></div>
    <div class="row"><span class="label">Endereço MAC</span><span class="value">%MAC_ADDRESS%</span></div>
    <div class="row"><span class="label">Gateway / Subnet</span><span class="value">%NET_GATEWAY% / %NET_SUBNET%</span></div>
  </div>

  <!-- SEÇÃO: CONTROLE INTERATIVO -->
  <div class="card">
    <h2>🎛️ Ações do Dispositivo</h2>
    <div class="actions">
      <a href="/info" class="btn">🔄 Atualizar</a>
      <a href="/ir_tx" class="btn" target="iframe_proc">📡 Pulso IR</a>
      <a href="/led_on" class="btn" target="iframe_proc">💡 LED Ligar</a>
      <a href="/led_off" class="btn" target="iframe_proc">🔌 LED Desligar</a>
    </div>
    
    <!-- Configuração Dinâmica de Fuso -->
    <form action="/set_fuso" method="POST" class="inline-form" target="iframe_proc">
      <select name="fuso">
        <option value="-3">Brasília (GMT-3)</option>
        <option value="-4">Manaus (GMT-4)</option>
        <option value="-5">Acre (GMT-5)</option>
        <option value="0">UTC (GMT+0)</option>
      </select>
      <button type="submit" class="btn" style="margin:0; padding:8px;">Definir Fuso</button>
    </form>

    <div class="actions">
      <a href="/reboot" class="btn btn-danger" style="grid-column: span 2; border-color:#dc2626;" onclick="return confirm('Deseja reiniciar o dispositivo?')">🛑 Reiniciar Dispositivo</a>
    </div>
    <!-- Frame oculto para processar ações sem recarregar a tela inteira -->
    <iframe name="iframe_proc" style="display:none;"></iframe>
  </div>
  <!-- INSIRA ESTE CARD DENTRO DA DIV CLASS="DASHBOARD" DO SEU HTMLINFOPAGE ANTERIOR -->
  <div class="card" style="grid-column: span 1;">
    <h2>💾 Redes Configuradas</h2>
    
    <!-- SLOT REDE 1 -->
    <div style="margin-bottom: 12px;">
      <div class="row" style="border:none; padding-bottom:2px;">
        <span class="label" style="color:#ffffff;">1. %SSID_0%</span>
        <span class="value" style="color:%COLOR_0%;">%RSSI_TXT_0%</span>
      </div>
      <div style="background: #252535; height: 8px; border-radius: 4px; overflow: hidden;">
        <div style="background: %COLOR_0%; width: %PCT_0%%; height: 100%; transition: width 0.5s;"></div>
      </div>
    </div>

    <!-- SLOT REDE 2 -->
    <div style="margin-bottom: 12px;">
      <div class="row" style="border:none; padding-bottom:2px;">
        <span class="label" style="color:#ffffff;">2. %SSID_1%</span>
        <span class="value" style="color:%COLOR_1%;">%RSSI_TXT_1%</span>
      </div>
      <div style="background: #252535; height: 8px; border-radius: 4px; overflow: hidden;">
        <div style="background: %COLOR_1%; width: %PCT_1%%; height: 100%; transition: width 0.5s;"></div>
      </div>
    </div>

    <!-- SLOT REDE 3 -->
    <div style="margin-bottom: 12px;">
      <div class="row" style="border:none; padding-bottom:2px;">
        <span class="label" style="color:#ffffff;">3. %SSID_2%</span>
        <span class="value" style="color:%COLOR_2%;">%RSSI_TXT_2%</span>
      </div>
      <div style="background: #252535; height: 8px; border-radius: 4px; overflow: hidden;">
        <div style="background: %COLOR_2%; width: %PCT_2%%; height: 100%; transition: width 0.5s;"></div>
      </div>
    </div>

    <!-- SLOT REDE 4 -->
    <div style="margin-bottom: 12px;">
      <div class="row" style="border:none; padding-bottom:2px;">
        <span class="label" style="color:#ffffff;">4. %SSID_3%</span>
        <span class="value" style="color:%COLOR_3%;">%RSSI_TXT_3%</span>
      </div>
      <div style="background: #252535; height: 8px; border-radius: 4px; overflow: hidden;">
        <div style="background: %COLOR_3%; width: %PCT_3%%; height: 100%; transition: width 0.5s;"></div>
      </div>
    </div>

    <!-- SLOT REDE 5 -->
    <div style="margin-bottom: 4px;">
      <div class="row" style="border:none; padding-bottom:2px;">
        <span class="label" style="color:#ffffff;">5. %SSID_4%</span>
        <span class="value" style="color:%COLOR_4%;">%RSSI_TXT_4%</span>
      </div>
      <div style="background: #252535; height: 8px; border-radius: 4px; overflow: hidden;">
        <div style="background: %COLOR_4%; width: %PCT_4%%; height: 100%; transition: width 0.5s;"></div>
      </div>
    </div>
  </div>
</div>

</body>
</html>
)rawliteral";

#endif
