#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

namespace Config
{
// Pinos nativos do hardware M5NanoC6 validados pelo manual
constexpr uint8_t PIN_IR_TX = 3;       // Emissor de Infravermelho
constexpr uint8_t PIN_LED_AZUL = 7;    // LED azul de status
constexpr uint8_t PIN_BOTAO = 9;       // Botão de usuário integrado
constexpr uint8_t PIN_RGB_ENABLE = 19; // Habilitação do barramento do NeoPixel
constexpr uint8_t PIN_LED_RGB = 20;    // Pino de dados do NeoPixel RGB
constexpr uint8_t PIN_LCD_SDA = 2;
constexpr uint8_t PIN_LCD_SCL = 1;
// 0 = detectar automaticamente um unico PCF8574 (0x20 a 0x27).
constexpr uint8_t LCD_ENDERECO = 0;
// Porto Alegre, RS (centro), coordenadas do Open-Meteo Geocoding.
constexpr double CLIMA_LATITUDE = -30.03283;
constexpr double CLIMA_LONGITUDE = -51.23019;

// Configurações de Rede UDP
constexpr uint16_t UDP_PORT = 4210;

// Fuso Horário Padrão (GMT-3 Brasília)
constexpr int FUSO_PADRAO = -3;

// --- FUNÇÕES UTILITÁRIAS DE DIAGNÓSTICO MOVIDAS PARA EVITAR DEPENDÊNCIA CÍCLICA ---
static String obterMotivoReset()
{
    esp_reset_reason_t motivo = esp_reset_reason();
    switch (motivo)
    {
    case ESP_RST_UNKNOWN:
        return "Desconhecido";
    case ESP_RST_POWERON:
        return "Ligado";
    case ESP_RST_EXT:
        return "Reset Externo";
    case ESP_RST_SW:
        return "Reset por Software";
    case ESP_RST_PANIC:
        return "Pânico";
    case ESP_RST_INT_WDT:
        return "Watchdog Interno";
    case ESP_RST_TASK_WDT:
        return "Watchdog de Tarefa";
    case ESP_RST_WDT:
        return "Watchdog Geral";
    case ESP_RST_DEEPSLEEP:
        return "Saída do Deep Sleep";
    case ESP_RST_BROWNOUT:
        return "Brownout";
    case ESP_RST_SDIO:
        return "Reset via SDIO";
    default:
        return "Desconhecido";
    }
}

static String obterVersaoAutomatica()
{
    int ano = ((__DATE__[9] - '0') * 10) + (__DATE__[10] - '0');
    int mes = (__DATE__[0] == 'J' && __DATE__[1] == 'a' && __DATE__[2] == 'n')   ? 1
              : (__DATE__[0] == 'F')                                             ? 2
              : (__DATE__[0] == 'M' && __DATE__[1] == 'a' && __DATE__[2] == 'r') ? 3
              : (__DATE__[0] == 'A' && __DATE__[1] == 'p')                       ? 4
              : (__DATE__[0] == 'M' && __DATE__[1] == 'a' && __DATE__[2] == 'y') ? 5
              : (__DATE__[0] == 'J' && __DATE__[1] == 'u' && __DATE__[2] == 'n') ? 6
              : (__DATE__[0] == 'J' && __DATE__[1] == 'u' && __DATE__[2] == 'l') ? 7
              : (__DATE__[0] == 'A' && __DATE__[1] == 'u')                       ? 8
              : (__DATE__[0] == 'S')                                             ? 9
              : (__DATE__[0] == 'O')                                             ? 10
              : (__DATE__[0] == 'N')                                             ? 11
              : (__DATE__[0] == 'D')                                             ? 12
                                                                                 : 0;
    int dia = (__DATE__[4] == ' ' ? 0 : __DATE__[4] - '0') * 10 + (__DATE__[5] - '0');
    int hora = ((__TIME__[0] - '0') * 10) + (__TIME__[1] - '0');
    int minuto = ((__TIME__[3] - '0') * 10) + (__TIME__[4] - '0');

    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%02d%02d%02d.%02d.%02d", ano, mes, dia, hora, minuto);
    return String(buffer);
}
} // namespace Config

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

#endif
