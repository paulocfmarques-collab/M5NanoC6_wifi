#ifndef DEVICE_NETWORK_H
#define DEVICE_NETWORK_H

#include <WiFi.h>
#include <WiFiUdp.h>
#include <WebServer.h>
#include <Preferences.h>
#include "Config.h"
#include "HardwareController.h"
#include "NTPService.h"
#include "WeatherService.h"
#include "LCDService.h"
#include "OTAService.h"
#include "WebDashboard.h"
#include <ArduinoJson.h>
#include "MetricsService.h"
#include "MetricsPage.h"
#include "HomeAssistantService.h"
#include "HomeAssistantPage.h"

class DeviceNetwork
{
  private:
    WebServer server;
    OTAService ota;
    WebServer metricsServer;
    uint16_t activeMetricsPort = 80;
    bool metricsPortPending = false;
    unsigned long metricsPortSavedAt = 0;
    WiFiUDP udp;
    Preferences prefs;
    bool modoAP;
    static const int MAX_REDES = 5;
    bool rotasConfiguradas = false;
    bool sinaisMedidos = false;
    unsigned long sinaisEm = 0;
    String ssidsMedidos[MAX_REDES];
    int rssiMedidos[MAX_REDES] = {};

    String paginaConfig()
    {
        prefs.begin("m5wifi", true);
        String h =
            "<!DOCTYPE html><html lang='pt-BR'><head><meta charset='UTF-8'>"
            "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
            "<title>M5NanoC6 - Configuracao</title><style>"
            "body{font-family:Arial;margin:20px;background:#1a1a1a;color:#fff;text-align:center}"
            ".c{background:#2d2d2d;max-width:340px;margin:auto;padding:20px;border-radius:12px}"
            "input{width:100%;padding:9px;margin:5px "
            "0;box-sizing:border-box;border-radius:6px;border:1px solid "
            "#444;background:#222;color:#fff}"
            "input[type=submit]{background:#ff5e00;border:none;font-weight:bold;cursor:pointer}"
            "</style></head><body><div class='c'><h2>M5NanoC6 WiFi</h2>"
            "<p>Ate 5 redes (vazio = remover; senha vazia mantem a atual)</p>"
            "<form action='/salvar' method='POST'>";
        for (int i = 0; i < MAX_REDES; i++)
        {
            String s = prefs.getString(("s" + String(i)).c_str(), "");
            s.replace("&", "&amp;");
            s.replace("'", "&#39;");
            s.replace("<", "&lt;");
            h += "<b>Rede " + String(i + 1) + "</b><input name='ssid" + String(i) + "' value='" +
                 s +
                 "' placeholder='SSID'>"
                 "<input type='password' name='senha" +
                 String(i) + "' placeholder='Senha'>";
        }
        prefs.end();
        h += "<input type='submit' value='Salvar e Reiniciar'></form>"
             "<p><a href='/prometheus'>Configurar Prometheus</a></p><p><a href='/info' "
             "style='color:#ff8b47'>Ver painel do dispositivo</a></p></div></body></html>";
        return h;
    }

    bool tentarRede(const String& ssid, const String& senha)
    {
        WiFi.disconnect(true);
        delay(100);
        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid.c_str(), senha.c_str());
        Serial.printf("[WIFI] Conectando a %s...\n", ssid.c_str());
        for (int i = 0; i < 25 && WiFi.status() != WL_CONNECTED; i++)
        {
            hardware.setLedColor(255, 255, 0, 20);
            delay(150);
            hardware.setLedColor(0, 0, 0, 0);
            delay(350);
        }
        return WiFi.status() == WL_CONNECTED;
    }

    static int nivelSinal(int rssi)
    {
        return constrain(2 * (rssi + 100), 0, 100);
    }

    void tratarRotaInfo()
    {
        server.sendHeader("Cache-Control", "no-store");
        server.send_P(200, "text/html; charset=utf-8", htmlInfoPage);
    }

    void tratarStatus()
    {
        bool conectada = WiFi.status() == WL_CONNECTED;
        String ssidAtual = conectada ? WiFi.SSID() : String();
        JsonDocument dados;
        JsonObject firmware = dados["firmware"].to<JsonObject>();
        firmware["versao"] = Config::obterVersaoAutomatica();
        firmware["compilacao"] = String(__DATE__) + " " + __TIME__;
        firmware["atualizacao"] = "OTA / USB";

        JsonObject sistema = dados["sistema"].to<JsonObject>();
        sistema["uptime_s"] = millis() / 1000;
        sistema["reset"] = Config::obterMotivoReset();
        float temperaturaCpu = temperatureRead();
        if (isfinite(temperaturaCpu))
            sistema["temperatura_cpu"] = temperaturaCpu;
        else
            sistema["temperatura_cpu"] = nullptr;
        sistema["chip"] = ESP.getChipModel();
        sistema["revisao"] = ESP.getChipRevision();
        sistema["nucleos"] = ESP.getChipCores();
        sistema["cpu_mhz"] = ESP.getCpuFreqMHz();
        sistema["heap_total"] = ESP.getHeapSize();
        sistema["heap_livre"] = ESP.getFreeHeap();
        sistema["heap_minimo"] = ESP.getMinFreeHeap();
        sistema["heap_maior_bloco"] = ESP.getMaxAllocHeap();
        sistema["flash_bytes"] = ESP.getFlashChipSize();
        sistema["flash_mhz"] = ESP.getFlashChipSpeed() / 1000000;
        sistema["firmware_bytes"] = ESP.getSketchSize();

        JsonObject relogio = dados["relogio"].to<JsonObject>();
        relogio["epoca"] = time(nullptr);
        relogio["data"] = ntp.getData();
        relogio["hora"] = ntp.getHora();
        relogio["ntp"] = ntp.getStatus();
        relogio["ultimo_ntp"] = ntp.obterUltimaSincronia();
        relogio["fuso"] = obterFuso();
        relogio["dst"] = obterDst();

        JsonObject rede = dados["rede"].to<JsonObject>();
        rede["conectada"] = conectada;
        rede["modo"] = modoAP ? "Configuracao AP" : "Estacao";
        rede["ssid"] = modoAP ? "M5NanoC6_CONFIG" : ssidAtual;
        rede["ip"] = modoAP ? WiFi.softAPIP().toString() : WiFi.localIP().toString();
        rede["mac"] = modoAP ? WiFi.softAPmacAddress() : WiFi.macAddress();
        rede["gateway"] = conectada ? WiFi.gatewayIP().toString() : String();
        rede["mascara"] = conectada ? WiFi.subnetMask().toString() : String();
        rede["dns1"] = conectada ? WiFi.dnsIP(0).toString() : String();
        rede["dns2"] = conectada ? WiFi.dnsIP(1).toString() : String();
        rede["udp_porta"] = Config::UDP_PORT;
        if (conectada)
        {
            rede["rssi"] = WiFi.RSSI();
            rede["qualidade"] = nivelSinal(WiFi.RSSI());
        }
        else
        {
            rede["rssi"] = nullptr;
            rede["qualidade"] = nullptr;
        }

        JsonObject update = dados["ota"].to<JsonObject>();
        update["disponivel"] = ota.capacity() > 0;
        update["limite_bytes"] = ota.capacity();
        update["pagina"] = "/ota";

        JsonObject display = dados["lcd"].to<JsonObject>();
        display["ativo"] = lcd.estaAtivo();
        display["ligado"] = lcd.estaLigado();
        display["tela"] = lcd.obterTela();
        if (lcd.estaAtivo())
        {
            char endereco[8];
            snprintf(endereco, sizeof(endereco), "0x%02X", lcd.obterEndereco());
            display["endereco"] = endereco;
        }
        else
            display["endereco"] = nullptr;
        display["sda"] = Config::PIN_LCD_SDA;
        display["scl"] = Config::PIN_LCD_SCL;
        JsonArray linhas = display["texto"].to<JsonArray>();
        for (uint8_t i = 0; i < 4; ++i)
            linhas.add(lcd.obterLinha(i));

        WeatherService::Estado estado = clima.obterEstado();
        JsonObject tempo = dados["clima"].to<JsonObject>();
        tempo["cidade"] = "Porto Alegre, RS";
        tempo["fonte"] = "Open-Meteo";
        tempo["disponivel"] = estado.disponivel;
        tempo["falhou"] = estado.falhou || !conectada;
        tempo["mensagem"] = clima.getLinha();
        tempo["observado_em"] = estado.instante;
        if (estado.instante)
            tempo["idade_s"] = estado.idade / 1000;
        else
            tempo["idade_s"] = nullptr;
        if (estado.disponivel)
        {
            tempo["temperatura"] = estado.temperatura;
            if (isfinite(estado.maxima))
                tempo["maxima"] = estado.maxima;
            else
                tempo["maxima"] = nullptr;
            if (isfinite(estado.minima))
                tempo["minima"] = estado.minima;
            else
                tempo["minima"] = nullptr;
            if (isfinite(estado.umidade))
                tempo["umidade"] = estado.umidade;
            else
                tempo["umidade"] = nullptr;
            if (isfinite(estado.sensacao))
                tempo["sensacao"] = estado.sensacao;
            else
                tempo["sensacao"] = nullptr;
            if (isfinite(estado.vento))
                tempo["vento"] = estado.vento;
            else
                tempo["vento"] = nullptr;
            tempo["codigo"] = estado.codigo;
            tempo["condicao"] = WeatherService::descricaoCompleta(estado.codigo);
        }
        else
        {
            tempo["temperatura"] = nullptr;
            tempo["maxima"] = nullptr;
            tempo["minima"] = nullptr;
            tempo["umidade"] = nullptr;
            tempo["sensacao"] = nullptr;
            tempo["vento"] = nullptr;
            tempo["codigo"] = nullptr;
            tempo["condicao"] = "Aguardando dados";
        }

        JsonObject led = dados["led"].to<JsonObject>();
        led["r"] = hardware.obterR();
        led["g"] = hardware.obterG();
        led["b"] = hardware.obterB();
        led["brilho"] = hardware.obterBrilho();
        led["efeito"] = hardware.obterEfeito();

        if (sinaisMedidos && millis() - sinaisEm >= 3600000UL)
            sinaisMedidos = false;
        JsonObject scan = dados["scan"].to<JsonObject>();
        scan["disponivel"] = sinaisMedidos;
        if (sinaisMedidos)
            scan["idade_s"] = (millis() - sinaisEm) / 1000;
        else
            scan["idade_s"] = nullptr;
        JsonArray redes = dados["redes"].to<JsonArray>();
        prefs.begin("m5wifi", true);
        for (int i = 0; i < MAX_REDES; ++i)
        {
            String nome = prefs.getString(("s" + String(i)).c_str(), "");
            JsonObject item = redes.add<JsonObject>();
            bool atual = conectada && nome.length() && nome == ssidAtual;
            bool medida = nome.length() && sinaisMedidos && nome == ssidsMedidos[i];
            int rssi = atual ? WiFi.RSSI() : rssiMedidos[i];
            bool visivel = atual || (medida && rssi != -127);
            item["slot"] = i + 1;
            item["ssid"] = nome;
            item["atual"] = atual;
            item["medida"] = atual || medida;
            item["visivel"] = visivel;
            if (visivel)
            {
                item["rssi"] = rssi;
                item["qualidade"] = nivelSinal(rssi);
            }
            else
            {
                item["rssi"] = nullptr;
                item["qualidade"] = nullptr;
            }
        }
        prefs.end();
        String resposta;
        serializeJson(dados, resposta);
        server.sendHeader("Cache-Control", "no-store");
        server.send(200, "application/json; charset=utf-8", resposta);
    }

    void medirSinais()
    {
        if (WiFi.status() != WL_CONNECTED)
        {
            server.send(503, "text/plain; charset=utf-8",
                        "Conecte a uma rede Wi-Fi para medir os sinais.");
            return;
        }
        prefs.begin("m5wifi", true);
        for (int i = 0; i < MAX_REDES; ++i)
            ssidsMedidos[i] = prefs.getString(("s" + String(i)).c_str(), "");
        prefs.end();
        int quantidade = WiFi.scanNetworks(false, false, false, 150);
        if (quantidade < 0)
        {
            WiFi.scanDelete();
            server.send(503, "text/plain; charset=utf-8",
                        "Nao foi possivel medir os sinais. Tente novamente.");
            return;
        }
        for (int i = 0; i < MAX_REDES; ++i)
        {
            rssiMedidos[i] = -127;
            for (int j = 0; j < quantidade; ++j)
                if (WiFi.SSID(j) == ssidsMedidos[i] && WiFi.RSSI(j) > rssiMedidos[i])
                    rssiMedidos[i] = WiFi.RSSI(j);
        }
        WiFi.scanDelete();
        sinaisMedidos = true;
        sinaisEm = millis();
        server.send(200, "text/plain; charset=utf-8", "Sinais das redes atualizados.");
    }

    void responderMetricas(WebServer& web)
    {
        web.sendHeader("Cache-Control", "no-store");
        web.send(200, "text/plain; version=0.0.4; charset=utf-8", metrics.exposition());
    }

    void iniciarServidorMetricas()
    {
        metricsServer.stop();
        activeMetricsPort = metrics.configuration().port;
        if (activeMetricsPort != 80)
            metricsServer.begin(activeMetricsPort);
        metricsPortPending = false;
        Serial.printf("[METRICS] Coleta em /metrics na porta %u; interface web na porta 80.\n",
                      activeMetricsPort);
    }

    void configurarRotasWeb()
    {
        if (rotasConfiguradas)
            return;
        rotasConfiguradas = true;
        ota.begin(server);
        server.on("/homeassistant", HTTP_GET, [this]() {
            server.sendHeader("Cache-Control", "no-store");
            server.send(200,"text/html; charset=utf-8",homeAssistantPage);
        });
        server.on("/api/homeassistant/config",HTTP_GET,[this](){responderConfigHomeAssistant();});
        server.on("/api/homeassistant/config",HTTP_POST,[this](){
            String host=server.arg("host");host.trim();
            uint16_t port,interval;
            String enabled=server.arg("enabled"),clear=server.arg("clear_password");
            if((enabled!="0"&&enabled!="1")||(clear!="0"&&clear!="1")||
                ((enabled=="1"||host.length())&&!MetricsService::validHost(host))||
                !MetricsService::parseNumber(server.arg("port"),1,65535,port)||
                !MetricsService::parseNumber(server.arg("interval"),5,3600,interval)||
                server.arg("user").length()>64||server.arg("password").length()>128) {
                server.send(400,"text/plain; charset=utf-8","Confira broker, porta, credenciais e intervalo 5..3600 s.");return;
            }
            String password=server.arg("password");
            if(clear=="1")password="";
            else if(password.isEmpty())password=homeassistant.configuration().password;
            if(!homeassistant.save(enabled=="1",host,port,server.arg("user"),password,interval)) {
                server.send(503,"text/plain; charset=utf-8","Nao foi possivel salvar nas preferencias.");return;
            }
            responderConfigHomeAssistant();
        });
        auto registrarHttp =
            [](WebServer& web, Middleware::Callback next)
            {
                int64_t start = esp_timer_get_time();
                bool handled = next();
                metrics.recordHttp(esp_timer_get_time() - start, web.responseCode());
                return handled;
            };
        server.addMiddleware(registrarHttp);
        metricsServer.addMiddleware(registrarHttp);
        metricsServer.on("/metrics", HTTP_GET, [this]() { responderMetricas(metricsServer); });
        server.on("/metrics", HTTP_GET,
                  [this]()
                  {
                      if (activeMetricsPort == 80)
                          responderMetricas(server);
                      else
                          server.send(404, "text/plain; charset=utf-8",
                                      "Metricas disponiveis na porta configurada do Prometheus.");
                  });
        server.on("/prometheus", HTTP_GET,
                  [this]()
                  {
                      server.sendHeader("Cache-Control", "no-store");
                      server.send_P(200, "text/html; charset=utf-8", metricsPage);
                  });
        server.on("/prometheus.yml", HTTP_GET,
                  [this]()
                  {
                      server.sendHeader("Cache-Control", "no-store");
                      server.sendHeader("Content-Disposition",
                                        "attachment; filename=prometheus-m5nanoc6.yml");
                      server.send(200, "text/plain; charset=utf-8", metrics.scrapeConfiguration());
                  });
        server.on("/api/prometheus/config", HTTP_GET, [this]() { responderConfigPrometheus(); });
        server.on(
            "/api/prometheus/config", HTTP_POST,
            [this]()
            {
                String host = server.arg("host");
                host.trim();
                uint16_t port, interval;
                if (!MetricsService::validHost(host) ||
                    !MetricsService::parseNumber(server.arg("port"), 1, 65535, port) ||
                    !MetricsService::parseNumber(server.arg("interval"), 5, 3600, interval))
                {
                    server.send(
                        400, "text/plain; charset=utf-8",
                        "Use IP/nome sem protocolo, porta 1..65535 e intervalo 5..3600 segundos.");
                    return;
                }
                if (!metrics.save(host, port, interval))
                {
                    server.send(503, "text/plain; charset=utf-8",
                                "Nao foi possivel salvar nas preferencias.");
                    return;
                }
                responderConfigPrometheus();
                metricsPortPending = port != activeMetricsPort;
                metricsPortSavedAt = millis();
            });
        server.on("/api/status", HTTP_GET, [this]() { tratarStatus(); });
        server.on("/config", HTTP_GET,
                  [this]() { server.send(200, "text/html; charset=utf-8", paginaConfig()); });
        server.on("/ntp_sync", HTTP_POST,
                  [this]()
                  {
                      bool iniciou = ntp.sincronizar(obterFuso(), obterDst());
                      server.send(iniciou ? 202 : 503, "text/plain; charset=utf-8",
                                  iniciou
                                      ? "Sincronizacao iniciada. Acompanhe o estado NTP no painel."
                                      : "Conecte a uma rede Wi-Fi com Internet para sincronizar.");
                  });
        server.on("/scan_wifi", HTTP_POST, [this]() { medirSinais(); });
        server.on(
            "/", HTTP_GET,
            [this]()
            {
                if (modoAP)
                {
                    server.send(200, "text/html", paginaConfig());
                }
                else
                {
                    tratarRotaInfo(); // No Wi-Fi de casa redireciona direto para as informações
                }
            });

        server.on("/info", HTTP_GET, [this]() { tratarRotaInfo(); });

        server.on("/salvar", HTTP_POST,
                  [this]()
                  {
                      prefs.begin("m5wifi", false);
                      for (int i = 0; i < MAX_REDES; i++)
                      {
                          String n = String(i);
                          String ssid = server.arg("ssid" + n);
                          String senha = server.arg("senha" + n);
                          ssid.trim();
                          if (ssid.length() == 0)
                          {
                              prefs.remove(("s" + n).c_str());
                              prefs.remove(("p" + n).c_str());
                          }
                          else
                          {
                              if (senha.length() == 0 &&
                                  prefs.getString(("s" + n).c_str(), "") == ssid)
                                  senha = prefs.getString(("p" + n).c_str(), "");
                              prefs.putString(("s" + n).c_str(), ssid);
                              prefs.putString(("p" + n).c_str(), senha);
                          }
                      }
                      prefs.remove("ssid");
                      prefs.remove("senha");
                      prefs.putInt("idx", -1);
                      prefs.end();

                      server.send(200, "text/html",
                                  "<h2>Configuracao Salva! Reiniciando M5NanoC6...</h2>");
                      delay(1500);
                      ESP.restart();
                  });

        // Ações rápidas executadas em background pelo iframe
        server.on("/led_on", HTTP_GET,
                  [this]()
                  {
                      hardware.setLedColor(255, 255, 255, 40);
                      server.send(200, "text/plain", "OK");
                  });

        server.on("/lcd_on", HTTP_POST,
                  [this]()
                  {
                      bool ok = lcd.definirLigado(true);
                      server.send(ok ? 200 : 503, "text/plain; charset=utf-8",
                                  ok ? "LCD ligado." : "LCD indisponivel ou falha I2C.");
                  });
        server.on("/lcd_off", HTTP_POST,
                  [this]()
                  {
                      bool ok = lcd.definirLigado(false);
                      server.send(ok ? 200 : 503, "text/plain; charset=utf-8",
                                  ok ? "LCD desligado." : "LCD indisponivel ou falha I2C.");
                  });

        server.on("/led_off", HTTP_GET,
                  [this]()
                  {
                      hardware.setLedColor(0, 0, 0, 0);
                      server.send(200, "text/plain", "OK");
                  });

        server.on("/ir_tx", HTTP_GET,
                  [this]()
                  {
                      hardware.dispararPulsoIR();
                      server.send(200, "text/plain", "OK");
                  });

        server.on("/set_fuso", HTTP_POST,
                  [this]()
                  {
                      String valor = server.arg("fuso");
                      valor.trim();
                      char* fim = nullptr;
                      long novoFuso = strtol(valor.c_str(), &fim, 10);
                      if (valor.isEmpty() || *fim != '\0' || novoFuso < -12 || novoFuso > 14)
                      {
                          server.send(400, "text/plain; charset=utf-8",
                                      "Use um fuso inteiro entre -12 e 14.");
                          return;
                      }
                      bool dst = obterDst();
                      if (server.hasArg("dst"))
                      {
                          String valorDst = server.arg("dst");
                          if (valorDst != "0" && valorDst != "1")
                          {
                              server.send(400, "text/plain; charset=utf-8",
                                          "Horario de verao invalido.");
                              return;
                          }
                          dst = valorDst == "1";
                      }
                      salvarFuso(static_cast<int>(novoFuso));
                      if (server.hasArg("dst"))
                          salvarDst(dst);
                      ntp.configurarRelogio(static_cast<int>(novoFuso), dst);
                      server.send(200, "text/plain; charset=utf-8",
                                  "Fuso e horario de verao atualizados.");
                  });

        server.on("/reboot", HTTP_GET,
                  [this]()
                  {
                      server.send(200, "text/html", "<h2>Reiniciando...</h2>");
                      delay(500);
                      ESP.restart();
                  });
    }

    void responderConfigHomeAssistant() {
        const auto& config=homeassistant.configuration();JsonDocument doc;
        doc["enabled"]=config.enabled;doc["host"]=config.host;doc["port"]=config.port;
        doc["user"]=config.user;doc["interval"]=config.interval;doc["has_password"]=config.password[0]!=0;
        doc["connected"]=homeassistant.connected();doc["connection_state"]=homeassistant.connectionState();
        doc["device_id"]=homeassistant.deviceId();String json;serializeJson(doc,json);
        server.sendHeader("Cache-Control","no-store");server.send(200,"application/json; charset=utf-8",json);
    }

    void responderConfigPrometheus()
    {
        const auto& config = metrics.configuration();
        JsonDocument data;
        data["host"] = config.host;
        data["port"] = config.port;
        data["active_port"] = activeMetricsPort;
        data["web_port"] = 80;
        data["interval"] = config.interval;
        data["mode"] = "pull";
        data["scrape_config"] = metrics.scrapeConfiguration();
        data["persistent_counters_ok"] = metrics.persistenceOk();
        String json;
        serializeJson(data, json);
        server.sendHeader("Cache-Control", "no-store");
        server.send(200, "application/json; charset=utf-8", json);
    }

  public:
    DeviceNetwork() : server(80), metricsServer(80), modoAP(false)
    {
    }

    // Modifica o método conectar existente para limpar estados e preparar a busca
    // Método conectar revisado para iniciar a busca baseada em RSSI
    bool conectar()
    {
        WiFi.persistent(false); // Evita desgaste desnecessário da Flash
        WiFi.disconnect(true);
        delay(100);

        prefs.begin("m5wifi", false);
        // Migração de segurança caso tenha configurações antigas soltas
        String antigo = prefs.getString("ssid", "");
        if (antigo.length() && prefs.getString("s0", "").length() == 0)
        {
            prefs.putString("s0", antigo);
            prefs.putString("p0", prefs.getString("senha", ""));
        }

        int totalRedes = 0;
        for (int i = 0; i < MAX_REDES; i++)
        {
            if (prefs.getString(("s" + String(i)).c_str(), "").length() > 0)
            {
                totalRedes++;
            }
        }
        prefs.end();

        if (totalRedes == 0)
        {
            Serial.println(F("[WIFI] Nenhuma rede salva encontrada. Iniciando portal..."));
            iniciarPortal();
            return false;
        }

        Serial.println(
            F("[WIFI] Iniciando busca automatica priorizada por forca de sinal (RSSI)..."));
        executarVarreduraCircular();

        return !modoAP;
    }

    // Executa a busca mapeando e ordenando as redes visíveis por melhor sinal
    void executarVarreduraCircular()
    {
        prefs.begin("m5wifi", false);
        String ssids[MAX_REDES], senhas[MAX_REDES];
        for (int i = 0; i < MAX_REDES; i++)
        {
            ssids[i] = prefs.getString(("s" + String(i)).c_str(), "");
            senhas[i] = prefs.getString(("p" + String(i)).c_str(), "");
        }
        prefs.end();

        WiFi.mode(WIFI_STA);
        Serial.println(F("[WIFI] Escaneando espectro de redes locais..."));
        int n = WiFi.scanNetworks();

        // Estrutura para parear as redes salvas encontradas com seus respectivos RSSIs
        struct RedeDisponivel
        {
            int indiceLista; // Posição original de 0 a 4 na EEPROM/Preferences
            int rssi;        // Força do sinal (ex: -50 dBm é melhor que -80 dBm)
        };

        RedeDisponivel redesParaTentar[MAX_REDES];
        int qtdEncontrada = 0;

        // 1. Cruza as redes salvas com o escaneamento de ar para coletar o RSSI
        for (int i = 0; i < MAX_REDES; i++)
        {
            if (ssids[i].length() == 0)
                continue;

            for (int r = 0; r < n; r++)
            {
                if (WiFi.SSID(r) == ssids[i])
                {
                    redesParaTentar[qtdEncontrada].indiceLista = i;
                    redesParaTentar[qtdEncontrada].rssi = WiFi.RSSI(r);
                    qtdEncontrada++;
                    break; // Passa para a próxima rede salva
                }
            }
        }

        // Se nenhuma das 5 redes salvas foi vista no escaneamento atual
        if (qtdEncontrada == 0)
        {
            Serial.println(F("[WIFI] Nenhuma rede salva foi detectada no ar neste momento."));
            WiFi.scanDelete();
            iniciarPortal();
            return;
        }

        // 2. Ordena o array usando Bubble Sort (Decrescente: do maior RSSI/sinal mais forte para o
        // menor)
        for (int i = 0; i < qtdEncontrada - 1; i++)
        {
            for (int j = 0; j < qtdEncontrada - i - 1; j++)
            {
                if (redesParaTentar[j].rssi < redesParaTentar[j + 1].rssi)
                {
                    RedeDisponivel temp = redesParaTentar[j];
                    redesParaTentar[j] = redesParaTentar[j + 1];
                    redesParaTentar[j + 1] = temp;
                }
            }
        }

        // 3. Tenta a conexão seguindo rigorosamente a ordem de melhor sinal
        for (int k = 0; k < qtdEncontrada; k++)
        {
            int idxOriginal = redesParaTentar[k].indiceLista;
            String ssidAlvo = ssids[idxOriginal];
            String senhaAlvo = senhas[idxOriginal];
            int rssiAlvo = redesParaTentar[k].rssi;

            Serial.printf("[WIFI] Tentando prioridade [%d]: %s (Sinal: %d dBm)\n", k + 1,
                          ssidAlvo.c_str(), rssiAlvo);

            if (tentarRede(ssidAlvo, senhaAlvo))
            {
                Serial.printf("[WIFI] Conectado a melhor rede disponivel: %s!\n", ssidAlvo.c_str());

                prefs.begin("m5wifi", false);
                prefs.putInt("idx", idxOriginal); // Atualiza o índice da última rede com sucesso
                prefs.end();

                modoAP = false;
                WiFi.scanDelete();

                // Inicializa os serviços de background do dispositivo
                configurarRotasWeb();
                server.begin();
                iniciarServidorMetricas();
                iniciarUDP();
                ntp.begin(obterFuso(), obterDst());
                return;
            }
            Serial.printf(
                "[WIFI] Falha de autenticacao na rede: %s. Tentando proxima do ranking...\n",
                ssidAlvo.c_str());
        }

        // Se falhar em todas as redes filtradas por sinal
        WiFi.scanDelete();
        Serial.println(F("[WIFI] Nenhuma das redes visiveis aceitou as credenciais de conexao."));
        iniciarPortal();
    }

    // NOVA FUNÇÃO: Monitor não-bloqueante para colocar no loop principal
    void monitorarConexao()
    {
        static unsigned long ultimaChecagem = 0;
        unsigned long agora = millis();

        // Se estiver em modo AP, não precisa monitorar queda de estação
        if (modoAP)
            return;

        // Verifica a saúde da conexão a cada 10 segundos de forma assíncrona
        if (agora - ultimaChecagem >= 10000)
        {
            ultimaChecagem = agora;

            if (WiFi.status() != WL_CONNECTED)
            {
                udp.stop();
                metrics.setUdpListener(false);
                Serial.println(
                    F("[WIFI] Alerta: Conexao perdida! Iniciando recuperacao circular..."));
                // Avisa visualmente piscando o LED em amarelo antes de reiniciar a busca
                hardware.piscarSincrono(2, 150, 255, 120, 0);
                executarVarreduraCircular();
            }
        }
    }

    void iniciarPortal()
    {
        udp.stop();
        metrics.setUdpListener(false);
        modoAP = true;
        WiFi.softAPdisconnect(true);
        WiFi.disconnect(true);
        delay(100);

        WiFi.mode(WIFI_AP);

        if (WiFi.softAP("M5NanoC6_CONFIG", nullptr, 1, 0, 4))
        {
            Serial.print(F("[PORTAL] Ativo com sucesso. IP: "));
            Serial.println(WiFi.softAPIP());
            hardware.iniciarBlinkAsync(0, 0, 255, 400, 20);
        }
        else
        {
            hardware.setLedColor(255, 0, 0, 30);
        }

        configurarRotasWeb();
        server.begin();
        iniciarServidorMetricas();
        Serial.println(F("[PORTAL] Servidor Web inicializado."));
    }

    void iniciarUDP()
    {
        udp.stop();
        bool opened = udp.begin(Config::UDP_PORT) == 1;
        metrics.setUdpListener(opened);
        if (!opened)
        {
            Serial.println(F("[UDP] Falha ao abrir a porta de comandos."));
            return;
        }
        hardware.setLedColor(0, 255, 0, 15);
        Serial.println(F("[UDP] Escutando comandos..."));
    }

    void responderUDP(const String& resposta)
    {
        if (udp.beginPacket(udp.remoteIP(), udp.remotePort()) != 1)
        {
            metrics.recordUdpReply(false);
            return;
        }
        size_t written = udp.print(resposta);
        bool sent = udp.endPacket() == 1;
        metrics.recordUdpReply(sent && written == resposta.length());
    }

    bool checarMensagensUDP(String& msgOut)
    {
        if (modoAP)
            return false;

        int packetSize = udp.parsePacket();
        if (packetSize > 0)
        {
            metrics.recordUdpPacket();
            char buffer[256];
            int len = udp.read(buffer, sizeof(buffer) - 1);
            udp.flush(); // Descarta o restante deste datagrama, sem bloquear os proximos.
            if (len > 0)
            {
                buffer[len] = '\0';
                msgOut = String(buffer);
                msgOut.trim();
                return true;
            }
        }
        return false;
    }

    void processarWebServer()
    {
        server.handleClient();
        ota.update();
        if (activeMetricsPort != 80)
            metricsServer.handleClient();
        if (metricsPortPending && millis() - metricsPortSavedAt >= 1000UL)
            iniciarServidorMetricas();
        if (modoAP)
        {
            hardware.atualizarEfeitos(); // Atualizado aqui também
        }
    }

    bool estaConectado()
    {
        return WiFi.status() == WL_CONNECTED;
    }

    int obterFuso()
    {
        prefs.begin("m5wifi", true);
        int fuso = prefs.getInt("fuso", Config::FUSO_PADRAO);
        prefs.end();
        return fuso;
    }

    void salvarFuso(int novoFuso)
    {
        prefs.begin("m5wifi", false);
        prefs.putInt("fuso", novoFuso);
        prefs.end();
    }

    bool obterDst()
    {
        prefs.begin("m5wifi", true);
        bool dst = prefs.getBool("dst", false);
        prefs.end();
        return dst;
    }

    void salvarDst(bool ativo)
    {
        prefs.begin("m5wifi", false);
        prefs.putBool("dst", ativo);
        prefs.end();
    }

    void resetarFabrica()
    {
        hardware.piscarSincrono(6, 100, 255, 0, 0);
        prefs.begin("m5wifi", false);
        prefs.clear();
        prefs.end();
        ESP.restart();
    }

    bool getIP(String& sIP)
    {
        if (estaConectado())
        {
            sIP = WiFi.localIP().toString();
            return true;
        }
        return false;
    }
};

extern DeviceNetwork network;

#endif
