#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

#include <Arduino.h>
#include "HardwareController.h"
#include "DeviceNetwork.h"
#include "NTPService.h"

class CommandHandler {
private:

    static String obterMotivoReset() {
        esp_reset_reason_t motivo = esp_reset_reason();
        switch (motivo) {
            case ESP_RST_UNKNOWN: return "Desconhecido";
            case ESP_RST_POWERON: return "Ligado";
            case ESP_RST_EXT: return "Reset Externo";
            case ESP_RST_SW: return "Reset por Software";
            case ESP_RST_PANIC: return "Pânico";
            case ESP_RST_INT_WDT: return "Watchdog Interno";
            case ESP_RST_TASK_WDT: return "Watchdog de Tarefa";
            case ESP_RST_WDT: return "Watchdog Geral";
            case ESP_RST_DEEPSLEEP: return "Saída do Deep Sleep";
            case ESP_RST_BROWNOUT: return "Brownout";
            case ESP_RST_SDIO: return "Reset via SDIO";
            default: return "Desconhecido";
        }
    }

public:

    static void executar(const String& cmd) {
        Serial.printf("[CMD] Recebido: %s\n", cmd.c_str());
        String resposta;

        if (cmd == "help") {
            resposta = "==============================================\n"
                      "Comandos Disponiveis:\n"
                      "PING             - Teste de Conexao\n"
                      "RESET_WIFI       - Reinicia configuracoes de rede\n"
                      "TIME             - Retorna hora e data atual\n"
                      "DATE             - Retorna apenas a data atual\n"
                      "DST_STATUS       - Verifica status do horario de verao\n"
                      "FUSO_STATUS      - Verifica fuso horario atual\n"
                      "DST_ON           - Ativa horario de verao\n"
                      "DST_OFF          - Desativa horario de verao\n"
                      "SET_FUSO:<valor> - Define novo fuso horario (Ex: SET_FUSO:-3)\n"
                      "SET_RGB:R,G,B    - Define cor do LED RGB (Ex: SET_RGB:255,0,0)\n"
                      "LED_ON           - Liga LED RGB em branco\n"
                      "LED_OFF          - Desliga LED RGB\n"
                      "IR_TX            - Dispara pulso infravermelho\n"
                      "OTA_INFO         - Informacoes do servidor OTA\n"
                      "ALIVE            - Verifica se o dispositivo esta vivo\n"
                      "INFO             - Informacoes detalhadas do dispositivo\n"
                      "REASON           - Motivo do ultimo reset\n"
                      "VERSION          - Versao do firmware\n"
                      "BUILD            - Data e hora da compilacao\n"
                      "STATUS           - Status geral do dispositivo\n"
                      "CPU              - Informacoes do processador\n"
                      "FLASH            - Informacoes da memoria flash\n"
                      "RAM              - Informacoes da memoria RAM\n"
                      "NET_INFO         - Informacoes da rede\n"
                      "TEMP             - Temperatura do processador\n"
                      "MAC              - Endereco MAC do dispositivo\n"
                      "PSRAM            - Informacoes da memoria PSRAM\n"
                      "UPTIME           - Tempo de atividade do dispositivo\n"
                      "==============================================";
            network.responderUDP(resposta + "\n");
        }
        else if (cmd == "info") {
            String dataStr = ntp.getData(), horaStr = ntp.getHora();
            uint32_t heapLivre = ESP.getFreeHeap() / 1024;
            float flashLivre = (float)ESP.getFreeSketchSpace() / (1024.0 * 1024.0);

            resposta = "===== DEVICE INFO =====\n"
                       "Hostname: ESP32_CENTRAL\n"
                       "Firmware: 1.0.0\n"
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
                       "Reset: " + obterMotivoReset() + "\n" + 
                       "=======================";
            network.responderUDP(resposta + "\n");
        }
        else if (cmd == "reason") {
            resposta = "===== ULTIMO RESET =====\n"
                "Motivo: " + obterMotivoReset() + "\n"
                "Uptime Atual: " + String(millis() / 1000) + " s\n"
                "========================";
            network.responderUDP(resposta + "\n");
        }
        else if (cmd == "version") {
            resposta = "Versao Firmware: v1.0.0";
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
        else if (cmd == "reset_wifi") {
            network.resetarFabrica();
        }
        else if (cmd == "dst_status") {
            bool dstAtivo = network.obterDst();
            resposta = dstAtivo ? "Horario de Verao: ATIVADO" : "Horario de Verao: DESATIVADO";
            network.responderUDP(resposta + "\n");
        }
        else if (cmd == "fuso_status") {
            int fuso = network.obterFuso();
            resposta = "Fuso Atual: GMT" + String(fuso);
            network.responderUDP(resposta + "\n");
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
        else if (cmd == "ir_tx") {
            hardware.dispararPulsoIR();
            network.responderUDP("Pulso Infravermelho emitido\n");
        }
        else if (cmd == "ota_info") {
            char buffer[128];
            snprintf(buffer, sizeof(buffer), "Hostname OTA: M5NanoC6-Dispositivo\nIP Local: %s\nStatus: Pronto para gravacao\n",
                     WiFi.localIP().toString().c_str());
            network.responderUDP(String(buffer));
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
        else if (cmd == "psram") {
            bool psramPresente = ESP.getPsramSize() > 0;
            resposta = "PSRAM:\nPSRAM Presente: " + String(psramPresente ? "SIM" : "NAO") + 
                "\nTamanho PSRAM: " + String(ESP.getPsramSize() / 1024 / 1024) + " MB" +
                "\nPSRAM Livre: " + String(ESP.getFreePsram() / 1024 / 1024) + " MB" +
                "\nMaior Bloco Livre PSRAM: " + String(ESP.getMaxAllocPsram() / 1024 / 1024) + " MB" +
                "\nPSRAM Utilizada: " + String(100.0 * (ESP.getPsramSize() - ESP.getFreePsram()) / ESP.getPsramSize()) + " %";
            network.responderUDP(resposta + "\n");
        }
        else 
        {
            network.responderUDP("Comando nao reconhecido\n");
        }
    }
};

#endif
