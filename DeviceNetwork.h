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
    static const int MAX_REDES = 5;

    String paginaConfig() {
        prefs.begin("m5wifi", true);
        String h = "<!DOCTYPE html><html lang='pt-BR'><head><meta charset='UTF-8'>"
                   "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
                   "<title>M5NanoC6 - Configuracao</title><style>"
                   "body{font-family:Arial;margin:20px;background:#1a1a1a;color:#fff;text-align:center}"
                   ".c{background:#2d2d2d;max-width:340px;margin:auto;padding:20px;border-radius:12px}"
                   "input{width:100%;padding:9px;margin:5px 0;box-sizing:border-box;border-radius:6px;border:1px solid #444;background:#222;color:#fff}"
                   "input[type=submit]{background:#ff5e00;border:none;font-weight:bold;cursor:pointer}"
                   "</style></head><body><div class='c'><h2>M5NanoC6 WiFi</h2>"
                   "<p>Ate 5 redes (vazio = remover; senha vazia mantem a atual)</p>"
                   "<form action='/salvar' method='POST'>";
        for (int i = 0; i < MAX_REDES; i++) {
            String s = prefs.getString(("s" + String(i)).c_str(), "");
            s.replace("&", "&amp;"); s.replace("'", "&#39;"); s.replace("<", "&lt;");
            h += "<b>Rede " + String(i + 1) + "</b><input name='ssid" + String(i) + "' value='" + s + "' placeholder='SSID'>"
                 "<input type='password' name='senha" + String(i) + "' placeholder='Senha'>";
        }
        prefs.end();
        h += "<input type='submit' value='Salvar e Reiniciar'></form></div></body></html>";
        return h;
    }

    bool tentarRede(const String& ssid, const String& senha) {
        WiFi.disconnect(true);
        delay(100);
        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid.c_str(), senha.c_str());
        Serial.printf("[WIFI] Conectando a %s...\n", ssid.c_str());
        for (int i = 0; i < 25 && WiFi.status() != WL_CONNECTED; i++) {
            hardware.setLedColor(255, 255, 0, 20);
            delay(150);
            hardware.setLedColor(0, 0, 0, 0);
            delay(350);
        }
        return WiFi.status() == WL_CONNECTED;
    }

    void tratarRotaInfo() {
        String html = String(htmlInfoPage);
        
        // 1. Processa Uptime básico do sistema
        unsigned long segs = millis() / 1000;
        int hrs = segs / 3600;
        int mins = (segs % 3600) / 60;
        int s = segs % 60;
        char uptimeBuffer[32];
        snprintf(uptimeBuffer, sizeof(uptimeBuffer), "%02dh %02dm %02ds", hrs, mins, s);

        // 2. Coleta dados de rede atuais
        String ssidAtual = (WiFi.status() == WL_CONNECTED) ? WiFi.SSID() : "Desconectado";
        String rssiAtual = (WiFi.status() == WL_CONNECTED) ? String(WiFi.RSSI()) : "0";
        String ntpStatusStr = ntp.isSincronizado() ? "Sincronizado ✔" : "Falhou ❌";
        String dataHoraStr = ntp.getHora() + " - " + ntp.getData();

        // 3. Substituições padrão do painel de hardware
        html.replace("%FIRMWARE_VER%", Config::obterVersaoAutomatica());
        html.replace("%BUILD_DATA%", String(__DATE__) + " " + String(__TIME__));
        html.replace("%UPTIME%", String(uptimeBuffer));
        html.replace("%MOTIVO_RESET%", Config::obterMotivoReset());
        html.replace("%CPU_TEMP%", String(temperatureRead(), 1));
        html.replace("%NTP_STATUS%", ntpStatusStr);
        html.replace("%DATA_HORA%", dataHoraStr);
        
        html.replace("%CHIP_MODELO%", String(ESP.getChipModel()));
        html.replace("%CHIP_CORES%", String(ESP.getChipCores()));
        html.replace("%CPU_FREQ%", String(ESP.getCpuFreqMHz()));
        html.replace("%RAM_LIVRE%", String(ESP.getFreeHeap() / 1024));
        html.replace("%RAM_MIN_LIVRE%", String(ESP.getMinFreeHeap() / 1024));
        html.replace("%FLASH_TAM%", String(ESP.getFlashChipSize() / 1024 / 1024));
        html.replace("%FLASH_LIVRE%", String(ESP.getFreeSketchSpace() / 1024));

        html.replace("%WIFI_SSID%", ssidAtual);
        html.replace("%WIFI_RSSI%", rssiAtual);
        html.replace("%IP_LOCAL%", WiFi.localIP().toString());
        html.replace("%MAC_ADDRESS%", WiFi.macAddress());
        html.replace("%NET_GATEWAY%", WiFi.gatewayIP().toString());
        html.replace("%NET_SUBNET%", WiFi.subnetMask().toString());

        // =========================================================================
        // 4. NOVA LÓGICA: ESCANEAMENTO E RENDERIZAÇÃO DAS BARRAS DE SINAL DAS REDES SALVAS
        // =========================================================================
        prefs.begin("m5wifi", true);
        String salvas[MAX_REDES];
        for (int i = 0; i < MAX_REDES; i++) {
            salvas[i] = prefs.getString(("s" + String(i)).c_str(), "");
        }
        prefs.end();

        // Faz uma varredura rápida sem travar (asynchronous = false para leitura linear imediata)
        int n = WiFi.scanNetworks(false, false, false, 150);

        for (int i = 0; i < MAX_REDES; i++) {
            String prefix = "%" + String(i) + "%";
            
            if (salvas[i].length() == 0) {
                // Configuração da posição vazia na memória
                html.replace("%SSID_" + String(i) + "%", "[Espaço Vazio]");
                html.replace("%RSSI_TXT_" + String(i) + "%", "-");
                html.replace("%PCT_" + String(i) + "%", "0");
                html.replace("%COLOR_" + String(i) + "%", "#444454");
                continue;
            }

            // Procura se a rede salva está visível no espectro escaneado
            int rssiEncontrado = -100;
            bool encontradaNoAr = false;
            for (int r = 0; r < n; r++) {
                if (WiFi.SSID(r) == salvas[i]) {
                    rssiEncontrado = WiFi.RSSI(r);
                    encontradaNoAr = true;
                    break;
                }
            }

            html.replace("%SSID_" + String(i) + "%", salvas[i]);

            if (!encontradaNoAr) {
                // Rede configurada, mas desligada ou distante
                html.replace("%RSSI_TXT_" + String(i) + "%", "Fora de Alcance");
                html.replace("%PCT_" + String(i) + "%", "0");
                html.replace("%COLOR_" + String(i) + "%", "#718096"); // Cinza fosco
            } else {
                // Converte RSSI (dBm de -100 a -50) para uma porcentagem de sinal amigável de 0% a 100%
                int pct = 2 * (rssiEncontrado + 100);
                if (pct > 100) pct = 100;
                if (pct < 0) pct = 0;

                // Define uma cor gradiente dinâmica baseada na qualidade da conexão
                String corHex = "#ef4444"; // Vermelho (Sinal Ruim < 50%)
                if (pct >= 75) {
                    corHex = "#10b981";    // Verde (Sinal Excelente >= 75%)
                } else if (pct >= 50) {
                    corHex = "#f59e0b";    // Amarelo/Laranja (Sinal Médio >= 50%)
                }

                html.replace("%RSSI_TXT_" + String(i) + "%", String(rssiEncontrado) + " dBm (" + String(pct) + "%)");
                html.replace("%PCT_" + String(i) + "%", String(pct));
                html.replace("%COLOR_" + String(i) + "%", corHex);
            }
        }
        WiFi.scanDelete(); // Limpa a memória do escaneamento após o uso

        // Envia o HTML finalizado e processado ao cliente
        server.send(200, "text/html", html);
    }

    void configurarRotasWeb() {
        server.on("/", HTTP_GET, [this]() {
            if (modoAP) {
                server.send(200, "text/html", paginaConfig());
            } else {
                tratarRotaInfo(); // No Wi-Fi de casa redireciona direto para as informações
            }
        });

        server.on("/info", HTTP_GET, [this]() {
            tratarRotaInfo();
        });

        server.on("/salvar", HTTP_POST, [this]() {
            prefs.begin("m5wifi", false);
            for (int i = 0; i < MAX_REDES; i++) {
                String n = String(i);
                String ssid = server.arg("ssid" + n);
                String senha = server.arg("senha" + n);
                ssid.trim();
                if (ssid.length() == 0) {
                    prefs.remove(("s" + n).c_str());
                    prefs.remove(("p" + n).c_str());
                } else {
                    if (senha.length() == 0 && prefs.getString(("s" + n).c_str(), "") == ssid)
                        senha = prefs.getString(("p" + n).c_str(), "");
                    prefs.putString(("s" + n).c_str(), ssid);
                    prefs.putString(("p" + n).c_str(), senha);
                }
            }
            prefs.remove("ssid");
            prefs.remove("senha");
            prefs.putInt("idx", -1);
            prefs.end();

            server.send(200, "text/html", "<h2>Configuracao Salva! Reiniciando M5NanoC6...</h2>");
            delay(1500);
            ESP.restart();
        });

        // Rota principal e atualização
        server.on("/info", HTTP_GET, [this]() { tratarRotaInfo(); });

        // Ações rápidas executadas em background pelo iframe
        server.on("/led_on", HTTP_GET, [this]() {
            hardware.setLedColor(255, 255, 255, 40);
            server.send(200, "text/plain", "OK");
        });

        server.on("/led_off", HTTP_GET, [this]() {
            hardware.setLedColor(0, 0, 0, 0);
            server.send(200, "text/plain", "OK");
        });

        server.on("/ir_tx", HTTP_GET, [this]() {
            hardware.dispararPulsoIR();
            server.send(200, "text/plain", "OK");
        });

        server.on("/set_fuso", HTTP_POST, [this]() {
            if (server.hasArg("fuso")) {
                int novoFuso = server.arg("fuso").toInt();
                salvarFuso(novoFuso);
                ntp.configurarRelogio(novoFuso, obterDst());
            }
            server.send(200, "text/plain", "OK");
        });

        server.on("/reboot", HTTP_GET, [this]() {
            server.send(200, "text/html", "<h2>Reiniciando...</h2>");
            delay(500);
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

    // Modifica o método conectar existente para limpar estados e preparar a busca
    // Método conectar revisado para iniciar a busca baseada em RSSI
    bool conectar() {
        WiFi.persistent(false); // Evita desgaste desnecessário da Flash
        WiFi.disconnect(true);
        delay(100);

        prefs.begin("m5wifi", false);
        // Migração de segurança caso tenha configurações antigas soltas
        String antigo = prefs.getString("ssid", "");
        if (antigo.length() && prefs.getString("s0", "").length() == 0) {
            prefs.putString("s0", antigo);
            prefs.putString("p0", prefs.getString("senha", ""));
        }
        
        int totalRedes = 0;
        for (int i = 0; i < MAX_REDES; i++) {
            if (prefs.getString(("s" + String(i)).c_str(), "").length() > 0) {
                totalRedes++;
            }
        }
        prefs.end();

        if (totalRedes == 0) {
            Serial.println(F("[WIFI] Nenhuma rede salva encontrada. Iniciando portal..."));
            iniciarPortal();
            return false;
        }

        Serial.println(F("[WIFI] Iniciando busca automatica priorizada por forca de sinal (RSSI)..."));
        executarVarreduraCircular();
        
        return !modoAP;
    }

    // Executa a busca mapeando e ordenando as redes visíveis por melhor sinal
    void executarVarreduraCircular() {
        prefs.begin("m5wifi", false);
        String ssids[MAX_REDES], senhas[MAX_REDES];
        for (int i = 0; i < MAX_REDES; i++) {
            ssids[i] = prefs.getString(("s" + String(i)).c_str(), "");
            senhas[i] = prefs.getString(("p" + String(i)).c_str(), "");
        }
        prefs.end();

        WiFi.mode(WIFI_STA);
        Serial.println(F("[WIFI] Escaneando espectro de redes locais..."));
        int n = WiFi.scanNetworks();
        
        // Estrutura para parear as redes salvas encontradas com seus respectivos RSSIs
        struct RedeDisponivel {
            int indiceLista; // Posição original de 0 a 4 na EEPROM/Preferences
            int rssi;        // Força do sinal (ex: -50 dBm é melhor que -80 dBm)
        };
        
        RedeDisponivel redesParaTentar[MAX_REDES];
        int qtdEncontrada = 0;

        // 1. Cruza as redes salvas com o escaneamento de ar para coletar o RSSI
        for (int i = 0; i < MAX_REDES; i++) {
            if (ssids[i].length() == 0) continue;

            for (int r = 0; r < n; r++) {
                if (WiFi.SSID(r) == ssids[i]) {
                    redesParaTentar[qtdEncontrada].indiceLista = i;
                    redesParaTentar[qtdEncontrada].rssi = WiFi.RSSI(r);
                    qtdEncontrada++;
                    break; // Passa para a próxima rede salva
                }
            }
        }

        // Se nenhuma das 5 redes salvas foi vista no escaneamento atual
        if (qtdEncontrada == 0) {
            Serial.println(F("[WIFI] Nenhuma rede salva foi detectada no ar neste momento."));
            WiFi.scanDelete();
            iniciarPortal();
            return;
        }

        // 2. Ordena o array usando Bubble Sort (Decrescente: do maior RSSI/sinal mais forte para o menor)
        for (int i = 0; i < qtdEncontrada - 1; i++) {
            for (int j = 0; j < qtdEncontrada - i - 1; j++) {
                if (redesParaTentar[j].rssi < redesParaTentar[j + 1].rssi) {
                    RedeDisponivel temp = redesParaTentar[j];
                    redesParaTentar[j] = redesParaTentar[j + 1];
                    redesParaTentar[j + 1] = temp;
                }
            }
        }

        // 3. Tenta a conexão seguindo rigorosamente a ordem de melhor sinal
        for (int k = 0; k < qtdEncontrada; k++) {
            int idxOriginal = redesParaTentar[k].indiceLista;
            String ssidAlvo = ssids[idxOriginal];
            String senhaAlvo = senhas[idxOriginal];
            int rssiAlvo = redesParaTentar[k].rssi;

            Serial.printf("[WIFI] Tentando prioridade [%d]: %s (Sinal: %d dBm)\n", k + 1, ssidAlvo.c_str(), rssiAlvo);
            
            if (tentarRede(ssidAlvo, senhaAlvo)) {
                Serial.printf("[WIFI] Conectado a melhor rede disponivel: %s!\n", ssidAlvo.c_str());
                
                prefs.begin("m5wifi", false);
                prefs.putInt("idx", idxOriginal); // Atualiza o índice da última rede com sucesso
                prefs.end();
                
                modoAP = false;
                WiFi.scanDelete();
                
                // Inicializa os serviços de background do dispositivo
                configurarOTA();
                configurarRotasWeb();
                server.begin();
                iniciarUDP();
                return; 
            }
            Serial.printf("[WIFI] Falha de autenticacao na rede: %s. Tentando proxima do ranking...\n", ssidAlvo.c_str());
        }

        // Se falhar em todas as redes filtradas por sinal
        WiFi.scanDelete();
        Serial.println(F("[WIFI] Nenhuma das redes visiveis aceitou as credenciais de conexao."));
        iniciarPortal();
    }

    // NOVA FUNÇÃO: Monitor não-bloqueante para colocar no loop principal
    void monitorarConexao() {
        static unsigned long ultimaChecagem = 0;
        unsigned long agora = millis();

        // Se estiver em modo AP, não precisa monitorar queda de estação
        if (modoAP) return;

        // Verifica a saúde da conexão a cada 10 segundos de forma assíncrona
        if (agora - ultimaChecagem >= 10000) {
            ultimaChecagem = agora;
            
            if (WiFi.status() != WL_CONNECTED) {
                Serial.println(F("[WIFI] Alerta: Conexao perdida! Iniciando recuperacao circular..."));
                // Avisa visualmente piscando o LED em amarelo antes de reiniciar a busca
                hardware.piscarSincrono(2, 150, 255, 120, 0); 
                executarVarreduraCircular();
            }
        }
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
