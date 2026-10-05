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
<meta charset="UTF-8"><meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>M5NanoC6 - Painel de Informações</title>
<style>
  body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; margin: 0; padding: 20px; background-color: #121212; color: #e0e0e0; text-align: center; }
  .container { background: #1e1e1e; max-width: 450px; margin: 30px auto; padding: 25px; border-radius: 14px; box-shadow: 0 6px 20px rgba(0,0,0,0.7); border: 1px solid #2d2d2d; }
  h2 { color: #ff5e00; margin-bottom: 20px; font-weight: 600; }
  .info-group { text-align: left; background: #252525; padding: 15px; border-radius: 8px; margin-bottom: 12px; border-left: 4px solid #ff5e00; }
  .info-label { font-size: 0.85rem; color: #888; text-transform: uppercase; letter-spacing: 1px; }
  .info-value { font-size: 1.1rem; color: #ffffff; font-weight: bold; margin-top: 4px; font-family: monospace; }
  .btn { display: inline-block; width: 100%; padding: 12px; margin-top: 15px; background: #333; color: #fff; text-decoration: none; border-radius: 6px; font-weight: bold; transition: background 0.2s; box-sizing: border-box;}
  .btn:hover { background: #444; }
</style>
</head>
<body>
<div class="container">
  <h2>M5NanoC6 Status</h2>
  
  <div class="info-group">
    <div class="info-label">Modelo do Chip & Cores</div>
    <div class="info-value">%CHIP_MODELO% (%CHIP_CORES% Cores)</div>
  </div>

  <div class="info-group">
    <div class="info-label">Memória RAM Livre</div>
    <div class="info-value">%RAM_LIVRE% bytes</div>
  </div>

  <div class="info-group">
    <div class="info-label">Tempo de Atividade (Uptime)</div>
    <div class="info-value">%UPTIME%</div>
  </div>

  <div class="info-group">
    <div class="info-label">Rede Wi-Fi Conectada</div>
    <div class="info-value">%WIFI_SSID% (Sinal: %WIFI_RSSI% dBm)</div>
  </div>

  <div class="info-group">
    <div class="info-label">Data & Hora Sincronizada</div>
    <div class="info-value">%DATA_HORA%</div>
  </div>

  <a href="/info" class="btn">🔄 Atualizar Dados</a>
</div>
</body>
</html>
)rawliteral";

#endif
