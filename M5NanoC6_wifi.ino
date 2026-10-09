#include "HardwareController.h"
#include "DeviceNetwork.h"
#include "CommandHandler.h"
#include "NTPService.h"

// --- INSTÂNCIAS GLOBAIS REAIS (ADICIONE ESTAS 3 LINHAS ABAIXO DOS INCLUDES) ---
HardwareController hardware;
DeviceNetwork network;
NTPService ntp;

void setup() {
    Serial.begin(115200);
    
    // Inicializa os pinos e periféricos
    hardware.begin();
    
    // Inicia a tentativa de conexão na lista circular por RSSI
    network.conectar();
}

void loop() {
    // Processa o servidor Web e o OTA
    network.processarWebServer();
    network.processarOTA();

    // Atualiza efeitos visuais de LED (Blink/Breath) de forma assíncrona
    hardware.atualizarEfeitos();

    // Monitora a conexão Wi-Fi e recupera via lista circular se cair
    network.monitorarConexao();

    // Verifica mensagens UDP recebidas
    String cmdUdp;
    if (network.checarMensagensUDP(cmdUdp)) {
        CommandHandler::executar(cmdUdp);
    }

    delay(1); // Alimenta o Watchdog do ESP32
}
