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
        String resposta;

        if (cmd == "help") {
            resposta = "==============================================\n"
                      "Comandos Disponiveis:\n"
                      "--- Sistema ---\n"
                      "INFO / STATUS / VERSION / BUILD / REASON\n"
                      "REBOOT               - Reinicia o ESP32\n"
                      "RESET_WIFI           - Apaga config de rede\n"
                      "ALIVE                - Teste de presenca\n"
                      "--- Hardware ---\n"
                      "TEMP / CPU / RAM / HEAP / FLASH / UPTIME\n"
                      "--- Rede ---\n"
                      "NET_INFO / MAC / RSSI / IP\n"
                      "--- Hora ---\n"
                      "TIME / DATE / SET_FUSO:<n> / DST_ON / DST_OFF\n"
                      "--- LED RGB ---\n"
                      "LED_ON / LED_OFF / LED_BLINK[:ms] / SET_RGB:R,G,B\n"
                      "SET_BREATH:R,G,B,MS\n"
                      "--- Outros ---\n"
                      "IR_TX                - Pulso infravermelho\n"
                      "==============================================";
            network.responderUDP(resposta + "\n");
        }
        else if (cmd == "info") {
            String dataStr = ntp.getData(), horaStr = ntp.getHora();
            uint32_t heapLivre = ESP.getFreeHeap() / 1024;
            float flashLivre = (float)ESP.getFreeSketchSpace() / (1024.0 * 1024.0);

            resposta = "===== DEVICE INFO =====\n"
                       "Hostname: M5NanoC6\n"
                       "Firmware: " + Config::obterVersaoAutomatica() + "\n"
                       "Build: " + String(__DATE__) + " " + String(__TIME__) + "\n" +
                       "SSID: " + WiFi.SSID() + "\n" +
                       "IP: " + WiFi.localIP().toString() + "\n" +
                       "MAC: " + WiFi.macAddress() + "\n" +
                       "RSSI: " + String(WiFi.RSSI()) + " dBm\n" +
                       "Heap Livre: " + String(heapLivre) + " KB\n" +
                       "Flash Livre: " + String(flashLivre, 1) + " MB\n" +
                       "SD Card: N/A\n" + 
                       "Data: " + dataStr + "\n" +
                       "Hora: " + horaStr + "\n" +
                       "Uptime: " + String(millis()) + " ms\n" +
                       "Reset: " + Config::obterMotivoReset() + "\n" + 
                       "=======================";
            network.responderUDP(resposta + "\n");
        }
        else if (cmd == "reason") {
            resposta = "===== ULTIMO RESET =====\n"
                "Motivo: " + Config::obterMotivoReset() + "\n"
                "Uptime Atual: " + String(millis() / 1000) + " s\n"
                "========================";
            network.responderUDP(resposta + "\n");
        }
        else if (cmd == "version") {
            resposta = "Versao Firmware: " + Config::obterVersaoAutomatica();
            network.responderUDP(resposta + "\n");
        }
        else if (cmd == "build") {
            resposta = "Build:\nData: " + String(__DATE__) + "\nHora: " + String(__TIME__);
            network.responderUDP(resposta + "\n");
        }
        else if (cmd == "status") {
            String statusWifi = (WiFi.status() == WL_CONNECTED) ? "OK" : "FALHA";
            uint32_t heapKB = ESP.getFreeHeap() / 1024;
            resposta = "WiFi: " + statusWifi + 
                "\nSD: N/A" + 
                "\nNTP: " + (ntp.isSincronizado() ? "OK" : "FALHA") + 
                "\nHeap: " + String(heapKB) + " KB";
            network.responderUDP(resposta + "\n");
        }        
        else if (cmd == "reboot") {
            network.responderUDP("Reiniciando...\n");
            delay(200);
            ESP.restart();
        }
        else if (cmd == "heap") {
            network.responderUDP("Heap livre: " + String(ESP.getFreeHeap() / 1024) + " KB\n");
        }
        else if (cmd == "rssi") {
            network.responderUDP("RSSI: " + String(WiFi.RSSI()) + " dBm\n");
        }
        else if (cmd == "ip") {
            network.responderUDP("IP: " + WiFi.localIP().toString() + "\n");
        }
        else if (cmd == "reset_wifi") {
            network.resetarFabrica();
        }
        else if (cmd == "dst_on") {
            network.salvarDst(true);
            ntp.configurarRelogio(network.obterFuso(), true);
            network.responderUDP("Horario de Verao: ATIVADO\n");
        }
        else if (cmd == "dst_off") {
            network.salvarDst(false);
            ntp.configurarRelogio(network.obterFuso(), false);
            network.responderUDP("Horario de Verao: DESATIVADO\n");
        }
        else if (cmd.startsWith("set_fuso")) {
            int p = cmd.indexOf(':');
            if (p > 0) {
                int novoFuso = cmd.substring(p + 1).toInt();
                network.salvarFuso(novoFuso);
                ntp.configurarRelogio(novoFuso, network.obterDst());
                network.responderUDP("Fuso atualizado para: " + String(novoFuso) + "\n");
            }
        }
        else if (cmd.startsWith("set_rgb")) {
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
        else if (cmd == "time") {
            resposta = "Hora:\n" + ntp.getHora() + "\n";
            network.responderUDP(resposta + "\n");
        }
        else if (cmd == "date") {
            resposta = "Data:\n" + ntp.getData() + "\n";
            network.responderUDP(resposta + "\n");
        }
        else if (cmd == "led_on") {
            hardware.setLedColor(255, 255, 255, 40); 
            network.responderUDP("LED ligado em Branco\n");
        }
        else if (cmd == "led_off") {
            hardware.setLedColor(0, 0, 0, 0); 
            network.responderUDP("LED desligado\n");
        }
        else if (cmd.startsWith("led_blink")) {
            unsigned long ms = 1000;
            int p = cmd.indexOf(':');
            if (p > 0) ms = (unsigned long)cmd.substring(p + 1).toInt();
            if (ms < 50) ms = 50;
            hardware.iniciarBlinkAsync(255, 255, 255, ms, 40);
            network.responderUDP("LED piscando a cada " + String(ms) + " ms\n");
        }
        else if (cmd.startsWith("set_breath")) {
            int p = cmd.indexOf(':');
            if (p > 0) {
                String valores = cmd.substring(p + 1);
                
                int idx1 = valores.indexOf(',');
                int idx2 = valores.indexOf(',', idx1 + 1);
                int idx3 = valores.indexOf(',', idx2 + 1);
                
                if (idx1 > 0 && idx2 > idx1 && idx3 > idx2) {
                    uint8_t r = valores.substring(0, idx1).toInt();
                    uint8_t g = valores.substring(idx1 + 1, idx2).toInt();
                    uint8_t b = valores.substring(idx2 + 1, idx3).toInt();
                    unsigned long tempoMs = (unsigned long)valores.substring(idx3 + 1).toInt();
                    
                    if (tempoMs < 100) tempoMs = 100; // Evita valores zerados ou excessivamente rápidos
                    
                    hardware.iniciarBreathAsync(r, g, b, tempoMs, 40);
                    
                    char buffer[128]; // Buffer com tamanho definido corretamente
                    snprintf(buffer, sizeof(buffer), "Efeito Breath ativo -> R:%d G:%d B:%d | Ciclo: %lu ms\n", r, g, b, tempoMs);
                    network.responderUDP(String(buffer));
                } else {
                    network.responderUDP("Erro: Use SET_BREATH:R,G,B,MS (Ex: SET_BREATH:0,255,255,2000)\n");
                }
            } else {
                network.responderUDP("Erro: Parametros ausentes. Ex: SET_BREATH:0,255,255,2000\n");
            }
        }
        else if (cmd == "ir_tx") {
            hardware.dispararPulsoIR();
            network.responderUDP("Pulso Infravermelho emitido\n");
        }
        else if (cmd == "cpu") {
            resposta = "CPU:\nModelo: " + String(ESP.getChipModel()) + "\n" +
                "Revision: " + String(ESP.getChipRevision()) + "\n" +
                "Cores: " + String(ESP.getChipCores()) + "\n" +
                "Freq: " + String(ESP.getCpuFreqMHz()) + " MHz";
            network.responderUDP(resposta + "\n");
        }
        else if (cmd == "flash") {
            resposta = "FLASH:\nTamanho: " + String(ESP.getFlashChipSize() / 1024 / 1024) + " MB\n" +
                "Velocidade: " + String(ESP.getFlashChipSpeed() / 1000 / 1000) + " MHz\n" + 
                "Flash Mode: " + String(ESP.getFlashChipMode()) + "\n" +
                "Sketch Size: " + String(ESP.getSketchSize() / 1024) + " kB\n" +
                "Free Sketch Space: " + String(ESP.getFreeSketchSpace() / 1024) + " kB\n" +
                "Flash livre: " + String(100.0 * (ESP.getFlashChipSize() - ESP.getFreeSketchSpace()) / ESP.getFlashChipSize()) + " %\n";
            network.responderUDP(resposta + "\n");
        }
        else if (cmd == "ram") {
            resposta = "RAM:\nHeap Livre: " + String(ESP.getFreeHeap() / 1024) + " KB\n" +
                "Menor Heap Livre: " + String(ESP.getMinFreeHeap() / 1024) + " KB\n" +
                "Maior Bloco Livre: " + String(ESP.getMaxAllocHeap() / 1024) + " KB\n" +
                "RAM Utilizada: " + String(100.0 * (ESP.getHeapSize() - ESP.getFreeHeap()) / ESP.getHeapSize()) + " %\n";
            network.responderUDP(resposta + "\n");
        }
        else if (cmd == "uptime") {
            resposta = "Uptime:\n" + String(millis() / 1000) + " s";
            network.responderUDP(resposta + "\n");
        }
        else if (cmd == "temp") {
            resposta = "Temperatura:\nCPU Temp: " + String(temperatureRead(), 2) + " C";
            network.responderUDP(resposta + "\n");
        }
        else if (cmd == "mac") {
            resposta = "MAC: " + WiFi.macAddress();
            network.responderUDP(resposta + "\n");
        }
        else if (cmd == "net_info") {
            resposta = "Network Info:\nSSID: " + WiFi.SSID() + "\n" +
                "IP: " + WiFi.localIP().toString() + "\n" +
                "Gateway: " + WiFi.gatewayIP().toString() + "\n" +
                "Subnet: " + WiFi.subnetMask().toString() + "\n" +
                "DNS1: " + WiFi.dnsIP(0).toString() + "\n" +
                "DNS2: " + WiFi.dnsIP(1).toString() + "\n" +
                "RSSI: " + String(WiFi.RSSI()) + " dBm";
            network.responderUDP(resposta + "\n");
        }
        else if (cmd == "alive") {
                network.responderUDP("ip: " + WiFi.localIP().toString() + " - yes\n");
        }
        else 
        {
            network.responderUDP("Comando nao reconhecido\n");
        }
    }
};

#endif
