#include "Config.h"
#include "HardwareController.h"
#include "DeviceNetwork.h"
#include "NTPService.h"
#include "CommandHandler.h"

HardwareController hardware;
DeviceNetwork network;
NTPService ntp;

bool udpPronto = false;

void setup() {
    Serial.begin(115200);
    delay(400); 
    
    hardware.begin();

    if (network.conectar()) {
        String sIP;

        network.iniciarUDP();
        if(network.getIP(sIP)) {
            Serial.println(sIP);
        }
        udpPronto = true;

        int fusoSalvo = network.obterFuso();
        bool dstSalvo = network.obterDst();
        ntp.begin(fusoSalvo, dstSalvo);
    } else {
        network.iniciarPortal();
    }
}

void loop() {
    hardware.atualizarEfeitos();
    
    network.processarOTA(); 
    network.processarWebServer(); 

    if (!network.estaConectado()) {
        udpPronto = false;
    } 
    else if (hardware.botaoPressionado()) {
        network.resetarFabrica();
    } 
    else {
        if (!udpPronto) {
            network.iniciarUDP();
            udpPronto = true;
        }

        String comando;
        if (network.checarMensagensUDP(comando)) {
            CommandHandler::executar(comando);
        }
    }
}
