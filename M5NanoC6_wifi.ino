#include <WiFi.h>
#include <WiFiUdp.h>
#include <WebServer.h>
#include <Preferences.h>
#include <Adafruit_NeoPixel.h>
#include "Zigbee.h"

#define RGB_PIN 20
#define ENABLE_PIN  19
#define NUM_LEDS 1
#define LED 7
#define BOTAO_RESET 9

Adafruit_NeoPixel led(NUM_LEDS, RGB_PIN, NEO_GRB + NEO_KHZ800);
void setStatusRGB(uint8_t r, uint8_t g, uint8_t b, uint8_t brilho = 20);

bool estadoAnterior = HIGH;
bool StateBlue = false;

WiFiUDP udp;
const int udpPort = 4210;

const char* ntpServer = "pool.ntp.org";
// UTC-3 (Brasília)
const long gmtOffset_sec = -3 * 3600;
// Sem horário de verão
const int daylightOffset_sec = 0;

WebServer server(80);
Preferences prefs;

bool blinkAtivo = false;
bool estadoLed = false;
unsigned long ultimoToggle = 0;
unsigned long intervaloBlink = 500; 

// HTML da página de configuração
const char* htmlPage PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Configuração WiFi</title>
<style>
  body { font-family: Arial, sans-serif; margin: 40px; background-color: #f4f4f9; text-align: center; }
  .container { background: white; max-width: 300px; margin: auto; padding: 20px; border-radius: 8px; box-shadow: 0 4px 8px rgba(0,0,0,0.1); }
  input { width: 100%; padding: 8px; margin: 10px 0; box-sizing: border-box; }
  input[type="submit"] { background: #007bff; color: white; border: none; cursor: pointer; }
</style>
</head>
<body>
<div class="container">
  <h2>Configuração WiFi - M5NanoC6</h2>
  <form action="/salvar" method="POST">
    <label>SSID:</label>
    <input type="text" name="ssid" placeholder="Nome da rede" required>
    <label>Senha:</label>
    <input type="password" name="senha" placeholder="Senha da rede">
    <input type="submit" value="Salvar">
  </form>
</div>
</body>
</html>
)rawliteral";

void setup() 
{
  Serial.begin(115200);

  pinMode(ENABLE_PIN, OUTPUT);
  pinMode(LED, OUTPUT);
  pinMode(BOTAO_RESET, INPUT_PULLUP);
  
  digitalWrite(ENABLE_PIN, HIGH);

  led.begin();
  rainbowBoot();

  if (conectarWifi()) 
  {
    setStatusRGB(0, 255, 0);
    Serial.println("Wifi Conectado!");
    Serial.println(WiFi.localIP().toString());
    udp.begin(udpPort);
    configureNTP();
  }
  else
  {
    iniciarPortal();
  }
}

void loop() 
{
  if (WiFi.getMode() == WIFI_AP) 
  {
    server.handleClient();
  }
  
  int packetSize = udp.parsePacket();
  if (packetSize) {
    char packetBuffer[255];
    int len = udp.read(packetBuffer, 255);
    if (len > 0) {
      packetBuffer[len] = 0;
    }
    String comando = String(packetBuffer);
    comando.trim();
    executa_comando(comando);
  }

  if (digitalRead(BOTAO_RESET) == LOW) {
    delay(50); 
    if (digitalRead(BOTAO_RESET) == LOW) {
      zerarConfiguracoes();
    }
  }

  if (blinkAtivo) {
    unsigned long atual = millis();
    if (atual - ultimoToggle >= intervaloBlink) {
      ultimoToggle = atual;
      estadoLed = !estadoLed;
      digitalWrite(LED, estadoLed);
    }
  }
}