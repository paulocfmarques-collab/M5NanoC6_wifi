#ifndef DEVICE_NETWORK_H
#define DEVICE_NETWORK_H

#include <WiFi.h>
#include <WiFiUdp.h>
#include <WebServer.h>
#include <Preferences.h>
#include "Config.h"
#include "HardwareController.h"

class DeviceNetwork {
private:
    WebServer server;
    WiFiUDP udp;
    Preferences prefs;
    bool modoAP;

    void configurarRotasWeb() {
        server.on("/", HTTP_GET, [this]() {
            server.send(200, "text/html", htmlPage);
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
        // MANUAL STATUS: Conectando Wi-Fi -> Amarelo Piscante (255, 255, 0)
        while (WiFi.status() != WL_CONNECTED && tentativas < 25) {
            hardware.setLedColor(255, 255, 0, 20); 
            delay(150);
            hardware.setLedColor(0, 0, 0, 0);
            delay(350);
            tentativas++;
        }

        modoAP = (WiFi.status() != WL_CONNECTED);
        
        // MANUAL STATUS: Erro Wi-Fi -> Vermelho Fixo (255, 0, 0)
        if (modoAP) {
            hardware.setLedColor(255, 0, 0, 20);
            delay(2000);
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
            // MANUAL STATUS: Portal Ativo -> Azul Piscante (0, 0, 255)
            hardware.iniciarBlinkAsync(0, 0, 255, 400, 20);
        } else {
            Serial.println(F("[PORTAL] Erro grave ao iniciar SoftAP!"));
            hardware.setLedColor(255, 0, 0, 30); 
        }

        configurarRotasWeb();
        server.begin();
        Serial.println(F("[PORTAL] Servidor Web inicializado."));
    }

    void iniciarUDP() {
        udp.begin(Config::UDP_PORT);
        // MANUAL STATUS: Conectado Wi-Fi -> Verde Fixo (0, 255, 0)
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
            char buffer[256]; // CORREÇÃO: Declarado corretamente como array de char de 256 bytes
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
        if (modoAP) {
            server.handleClient();
            hardware.atualizarBlink();
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

    bool getIP(String& sIP)
    {
        if(estaConectado())
        {
            sIP = WiFi.localIP().toString();
            return true;
        }
        return false;
    }
};

// Vinculação externa para o compilador encontrar a instância global do arquivo principal
extern DeviceNetwork network;

#endif
