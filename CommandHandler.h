#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

#include <Arduino.h>
#include "HardwareController.h"
#include "DeviceNetwork.h"
#include "NTPService.h"

class CommandHandler {
public:
    static void executar(const String& cmd) {
        Serial.printf("[CMD] Recebido: %s\n", cmd.c_str());

        if (cmd == "RESET_WIFI") {
            network.resetarFabrica();
        }
        else if (cmd == "TIME") {
            String resposta = "Hora: " + ntp.getHora() + " | Data: " + ntp.getData();
            network.responderUDP(resposta + "\n");
        }
        else if (cmd == "DST_ON") {
            network.salvarDst(true);
            ntp.configurarRelogio(network.obterFuso(), true);
            network.responderUDP("Horario de Verao: ATIVADO\n");
        }
        else if (cmd == "DST_OFF") {
            network.salvarDst(false);
            ntp.configurarRelogio(network.obterFuso(), false);
            network.responderUDP("Horario de Verao: DESATIVADO\n");
        }
        else if (cmd.startsWith("SET_FUSO")) {
            int p = cmd.indexOf(':');
            if (p > 0) {
                int novoFuso = cmd.substring(p + 1).toInt();
                network.salvarFuso(novoFuso);
                ntp.configurarRelogio(novoFuso, network.obterDst());
                network.responderUDP("Fuso atualizado para: " + String(novoFuso) + "\n");
            }
        }
        else if (cmd.startsWith("SET_RGB")) {
            int p = cmd.indexOf(':');
            if (p > 0) {
                String valores = cmd.substring(p + 1);
                int idx1 = valores.indexOf(',');
                int idx2 = valores.indexOf(',', idx1 + 1);
                
                if (idx1 > 0 && idx2 > idx1) {
                    uint8_t r = valores.substring(0, idx1).toInt();
                    uint8_t g = valores.substring(idx1 + 1, idx2).toInt();
                    uint8_t b = valores.substring(idx2 + 1).toInt();
                    
                    hardware.setLedColor(r, g, b, 30);
                    
                    char buffer[64];
                    snprintf(buffer, sizeof(buffer), "Cor alterada -> R:%d G:%d B:%d\n", r, g, b);
                    network.responderUDP(String(buffer));
                } else {
                    network.responderUDP("Erro: Use SET_RGB:R,G,B (Ex: SET_RGB:255,0,0)\n");
                }
            }
        }
        else if (cmd == "LED_ON") {
            hardware.setLedColor(255, 255, 255, 40); 
            network.responderUDP("LED ligado em Branco\n");
        }
        else if (cmd == "LED_OFF") {
            hardware.setLedColor(0, 0, 0, 0); 
            network.responderUDP("LED desligado\n");
        }
        else if (cmd == "IR_TX") {
            hardware.dispararPulsoIR();
            network.responderUDP("Pulso Infravermelho emitido\n");
        }
        else if (cmd == "SYS_INFO") {
            char buffer[128];
            snprintf(buffer, sizeof(buffer), "Chip: %s | Cores: %d | RAM Livre: %u bytes\n",
                     ESP.getChipModel(), ESP.getChipCores(), ESP.getFreeHeap());
            network.responderUDP(String(buffer));
        }
        else {
            network.responderUDP("Comando nao reconhecido\n");
        }
    }
};

#endif
