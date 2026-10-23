#include <Wire.h>
#include <HTTPClient.h>
#include <NetworkClientSecure.h>
#include <ArduinoJson.h>
#include "HardwareController.h"
#include "DeviceNetwork.h"
#include "CommandHandler.h"
#include "NTPService.h"
#include "LCDService.h"
#include "LCDButton.h"

// --- INSTÂNCIAS GLOBAIS REAIS (ADICIONE ESTAS 3 LINHAS ABAIXO DOS INCLUDES) ---
HardwareController hardware;
DeviceNetwork network;
NTPService ntp;
LCDService lcd;
WeatherService clima;
MetricsService metrics;
HomeAssistantService homeassistant;

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
                CommandHandler::executar(comando, true);
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
    static LCDButton botao;
    bool pressionado=digitalRead(Config::PIN_BOTAO)==LOW;
    if(pressionado)lcd.registrarInteracao();
    auto acao=botao.update(pressionado,lcd.estaLigado(),millis());
    if(acao==LCDButton::Wake)lcd.definirLigado(true);
    else if(acao==LCDButton::NextPage)lcd.alternarTela();
    else if(acao==LCDButton::TurnOff)lcd.definirLigado(false);
}

void setup()
{
    Serial.begin(115200);

    // Inicializa os pinos e periféricos
    hardware.begin();
    metrics.begin();
    homeassistant.begin([](String command){CommandHandler::executar(command,true);});
    lcd.begin();

    // Inicia a tentativa de conexão na lista circular por RSSI
    network.conectar();
    clima.begin();
}

void loop()
{
    // Processa o servidor Web e a atualizacao OTA
    network.processarWebServer();
    processarSerial();
    processarBotao();
    metrics.update();

    // Atualiza efeitos visuais de LED (Blink/Breath) de forma assíncrona
    hardware.atualizarEfeitos();

    // Monitora a conexão Wi-Fi e recupera via lista circular se cair
    network.monitorarConexao();
    ntp.atualizar();
    lcd.atualizar();
    homeassistant.update();

    // Verifica mensagens UDP recebidas
    String cmdUdp;
    if (network.checarMensagensUDP(cmdUdp))
    {
        CommandHandler::executar(cmdUdp);
    }

    delay(1); // Alimenta o Watchdog do ESP32
}
