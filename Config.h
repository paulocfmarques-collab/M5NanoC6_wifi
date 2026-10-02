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

#endif
