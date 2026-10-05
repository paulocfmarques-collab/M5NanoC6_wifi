#ifndef DEVICE_NETWORK_H
#define DEVICE_NETWORK_H

#include <WiFi.h>
#include <WiFiUdp.h>
#include <WebServer.h>
#include <Preferences.h>
#include <ArduinoOTA.h>
#include "Config.h"
#include "HardwareController.h"
#include "NTPService.h" 

class DeviceNetwork {
private:
    WebServer server;
    WiFiUDP udp;
    Preferences prefs;
    bool modoAP;

    void tratarRotaInfo() {
        String html = String(htmlInfoPage);
        
        // Conversão de uptime simples (ms para hh:mm:ss)
        unsigned long segs = millis() / 1000;
        int hrs = segs / 3600;
        int mins = (segs % 3600) / 60;
        int s = segs % 60;
        char uptimeBuffer[32];
        snprintf(uptimeBuffer, sizeof(uptimeBuffer), "%02dh %02dm %02ds", hrs, mins, s);

        String ssidAtual = (WiFi.status() == WL_CONNECTED) ? WiFi.SSID() : "Desconectado";
        String rssiAtual = (WiFi.status() == WL_CONNECTED) ? String(WiFi.RSSI()) : "0";
        String dataHoraStr = "Hora: " + ntp.getHora() + " | Data: " + ntp.getData();

        // Substituições dinâmicas no HTML
        html.replace("%CHIP_MODELO%", String(ESP.getChipModel()));
        html.replace("%CHIP_CORES%", String(ESP.getChipCores()));
        html.replace("%RAM_LIVRE%", String(ESP.getFreeHeap()));
        html.replace("%UPTIME%", String(uptimeBuffer));
        html.replace("%WIFI_SSID%", ssidAtual);
        html.replace("%WIFI_RSSI%", rssiAtual);
        html.replace("%DATA_HORA%", dataHoraStr);

        server.send(200, "text/html", html);
    }

    void configurarRotasWeb() {
        server.on("/", HTTP_GET, [this]() {
            if (modoAP) {
                server.send(200, "text/html", htmlPage);
            } else {
                tratarRotaInfo(); // No Wi-Fi de casa redireciona direto para as informações
            }
        });

        server.on("/info", HTTP_GET, [this]() {
            tratarRotaInfo();
        });

        server.on("/salvar", HTTP_POST, [this]() {
            String novoSSID = server.arg("ssid");
            String novaSenha = server.arg("senha");

            prefs.begin("m5wifi", false);
            prefs.putString("ssid", novoSSID);
            prefs.putString("senha", novaSenha);
            prefs.end();

            server.send(200, "text/html", "<h2>Configuracao Salva! Reiniciando M5NanoC6...</h2>");
            delay(1500);
            ESP.restart();
        });
    }

    void configurarOTA() {
        ArduinoOTA.setHostname("M5NanoC6-Dispositivo");

        ArduinoOTA.onStart([]() {
            String tipo = (ArduinoOTA.getCommand() == U_FLASH) ? "firmware" : "filesystem";
            Serial.println("[OTA] Iniciando atualizacao de " + tipo);
            hardware.setLedColor(0, 0, 255, 30); 
        });

        ArduinoOTA.onEnd([]() {
            Serial.println("\n[OTA] Sucesso! Reiniciando...");
            hardware.piscarSincrono(5, 50, 0, 255, 0); 
        });

        ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
            int pct = (progress / (total / 100));
            Serial.printf("[OTA] Progresso: %d%%\r", pct);
            if (pct % 2 == 0) hardware.setLedColor(0, 0, 255, 20);
            else hardware.setLedColor(0, 0, 0, 0);
        });

        ArduinoOTA.onError([](ota_error_t error) {
            hardware.piscarSincrono(3, 200, 255, 0, 0); 
        });

        ArduinoOTA.begin();
        Serial.println("[OTA] Servidor OTA Inicializado.");
    }

public:
    DeviceNetwork() : server(80), modoAP(false) {}

    bool conectar() {
        prefs.begin("m5wifi", true);
        String ssid = prefs.getString("ssid", "");
        String senha = prefs.getString("senha", "");
        prefs.end();

        if (ssid == "") return false;

        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid.c_str(), senha.c_str());
        Serial.printf("[WIFI] Conectando a %s...\n", ssid.c_str());

        int tentativas = 0;
        while (WiFi.status() != WL_CONNECTED && tentativas < 25) {
            hardware.setLedColor(255, 255, 0, 20); 
            delay(150);
            hardware.setLedColor(0, 0, 0, 0);
            delay(350);
            tentativas++;
        }

        modoAP = (WiFi.status() != WL_CONNECTED);
        
        if (modoAP) {
            hardware.setLedColor(255, 0, 0, 20);
            delay(2000);
        } else {
            configurarOTA();
            configurarRotasWeb();
            server.begin();
            Serial.println(F("[WEB] Servidor de Informações iniciado em modo STA."));
        }
        
        return !modoAP;
    }

    void iniciarPortal() {
        modoAP = true;
        WiFi.softAPdisconnect(true);
        WiFi.disconnect(true);
        delay(100); 

        WiFi.mode(WIFI_AP);
        
        if (WiFi.softAP("M5NanoC6_CONFIG", nullptr, 1, 0, 4)) {
            Serial.print(F("[PORTAL] Ativo com sucesso. IP: "));
            Serial.println(WiFi.softAPIP());
            hardware.iniciarBlinkAsync(0, 0, 255, 400, 20);
        } else {
            hardware.setLedColor(255, 0, 0, 30); 
        }

        configurarRotasWeb();
        server.begin();
        Serial.println(F("[PORTAL] Servidor Web inicializado."));
    }

    void iniciarUDP() {
        udp.begin(Config::UDP_PORT);
        hardware.setLedColor(0, 255, 0, 15); 
        Serial.println(F("[UDP] Escutando comandos..."));
    }

    void responderUDP(const String& resposta) {
        udp.beginPacket(udp.remoteIP(), udp.remotePort());
        udp.print(resposta);
        udp.endPacket();
    }

    bool checarMensagensUDP(String& msgOut) {
        if (modoAP) return false;

        int packetSize = udp.parsePacket();
        if (packetSize) {
            char buffer[256]; 
            int len = udp.read(buffer, sizeof(buffer) - 1);
            if (len > 0) {
                buffer[len] = '\0';
                msgOut = String(buffer);
                msgOut.trim();
                return true;
            }
        }
        return false;
    }

    void processarWebServer() {
        server.handleClient();
        if (modoAP) {
            hardware.atualizarEfeitos(); // Atualizado aqui também
        }
    }

    void processarOTA() {
        if (!modoAP && WiFi.status() == WL_CONNECTED) {
            ArduinoOTA.handle();
        }
    }

    bool estaConectado() { return WiFi.status() == WL_CONNECTED; }

    int obterFuso() {
        prefs.begin("m5wifi", true);
        int fuso = prefs.getInt("fuso", Config::FUSO_PADRAO);
        prefs.end();
        return fuso;
    }

    void salvarFuso(int novoFuso) {
        prefs.begin("m5wifi", false);
        prefs.putInt("fuso", novoFuso);
        prefs.end();
    }

    bool obterDst() {
        prefs.begin("m5wifi", true);
        bool dst = prefs.getBool("dst", false);
        prefs.end();
        return dst;
    }

    void salvarDst(bool ativo) {
        prefs.begin("m5wifi", false);
        prefs.putBool("dst", ativo);
        prefs.end();
    }

    void resetarFabrica() {
        hardware.piscarSincrono(6, 100, 255, 0, 0); 
        prefs.begin("m5wifi", false);
        prefs.clear();
        prefs.end();
        ESP.restart();
    }

    bool getIP(String& sIP) {
        if(estaConectado()) {
            sIP = WiFi.localIP().toString();
            return true;
        }
        return false;
    }
};

extern DeviceNetwork network;

#endif
