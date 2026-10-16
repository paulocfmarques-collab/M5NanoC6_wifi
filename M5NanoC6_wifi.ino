#include <Wire.h>
#include <HTTPClient.h>
#include <NetworkClientSecure.h>
#include <ArduinoJson.h>
#include <BLEDevice.h>
#include "HardwareController.h"
#include "DeviceNetwork.h"
#include "CommandHandler.h"
#include "NTPService.h"
#include "LCDService.h"

// --- INSTÂNCIAS GLOBAIS REAIS (ADICIONE ESTAS 3 LINHAS ABAIXO DOS INCLUDES) ---
HardwareController hardware;
DeviceNetwork network;
NTPService ntp;
LCDService lcd;
WeatherService clima;
BluetoothService bluetooth;
MetricsService metrics;

// Recepcao limitada por passagem: nao aguarda quebra de linha nem bloqueia o loop.
void processarSerial()
{
    static String comando;
    static bool descartar = false;
    for (uint8_t i = 0; i < 64 && Serial.available(); ++i)
    {
        char c = Serial.read();
        if (c == '\r')
            continue;
        if (c == '\n')
        {
            if (descartar)
                Serial.println("Erro: use ate 255 caracteres ASCII por linha.");
            else if (comando.length())
                CommandHandler::executar(comando, false, true);
            comando = "";
            descartar = false;
        }
        else if (!descartar)
        {
            if (c < 32 || c > 126 || comando.length() >= 255)
            {
                descartar = true;
                comando = "";
            }
            else
                comando += c;
        }
    }
}

void processarBotao()
{
    bool leitura = digitalRead(Config::PIN_BOTAO) == LOW;
    static bool anterior = leitura;
    static bool estavel = leitura;
    static unsigned long alteradoEm = millis();
    if (leitura != anterior)
    {
        anterior = leitura;
        alteradoEm = millis();
    }
    if (leitura == estavel || millis() - alteradoEm < 35UL)
        return;
    estavel = leitura;
    // Uma acao por pressionamento; soltar e manter pressionado nao repetem.
    if (estavel)
    {
        bool ligar = !lcd.estaLigado();
        if (lcd.definirLigado(ligar))
            Serial.println(ligar ? "[BOTAO] LCD ligado." : "[BOTAO] LCD desligado.");
        else
            Serial.println("[BOTAO] LCD indisponivel ou falha I2C.");
    }
}

void setup()
{
    Serial.begin(115200);

    // Inicializa os pinos e periféricos
    hardware.begin();
    metrics.begin();
    lcd.begin();
    bluetooth.begin();

    // Inicia a tentativa de conexão na lista circular por RSSI
    network.conectar();
    clima.begin();
}

void loop()
{
    // Processa o servidor Web e o Bluetooth LE
    network.processarWebServer();
    bluetooth.atualizar();
    processarSerial();
    processarBotao();
    metrics.update();

    // Atualiza efeitos visuais de LED (Blink/Breath) de forma assíncrona
    hardware.atualizarEfeitos();

    // Monitora a conexão Wi-Fi e recupera via lista circular se cair
    network.monitorarConexao();
    ntp.atualizar();
    lcd.atualizar();

    // Verifica mensagens UDP recebidas
    String cmdUdp;
    if (network.checarMensagensUDP(cmdUdp))
    {
        CommandHandler::executar(cmdUdp);
    }

    String cmdBLE;
    if (bluetooth.checarMensagem(cmdBLE))
    {
        CommandHandler::executar(cmdBLE, true);
    }

    delay(1); // Alimenta o Watchdog do ESP32
}
