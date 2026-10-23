#ifndef OTA_SERVICE_H
#define OTA_SERVICE_H

#include <Arduino.h>
#include <WebServer.h>
#include <Update.h>
#include <esp_ota_ops.h>
#include <ArduinoOTA.h>
#include <WiFi.h>
#include "OTAUploadPage.h"

class OTAService {
    String failure;
    bool accepted = false, completed = false, rebootPending = false;
    unsigned long rebootAt = 0;
    size_t received = 0;
    bool networkStarted = false;
    uint32_t networkAddress = 0;
    uint8_t networkProgress = 255;

    void upload(WebServer& web) {
        HTTPUpload& file = web.upload();
        if (file.status == UPLOAD_FILE_START) {
            if (accepted) Update.abort();
            accepted = false;
            completed = false;
            received = 0;
            failure = "";
            if (rebootPending) return;
            String name = file.filename;
            name.toLowerCase();
            if (file.name != "firmware" || !name.endsWith(".bin") ||
                name.indexOf("bootloader") >= 0 || name.indexOf("partitions") >= 0 ||
                name.indexOf("merged") >= 0) {
                failure = "Selecione somente o binario de aplicacao .ino.bin.";
                return;
            }
            if (!capacity()) {
                failure = "Sem segunda particao OTA. Grave a nova tabela por USB primeiro.";
                return;
            }
            accepted = Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH);
            if (!accepted) failure = Update.errorString();
        } else if (file.status == UPLOAD_FILE_WRITE && accepted) {
            if (file.currentSize > capacity() - received) {
                failure = "Firmware maior que a particao OTA.";
                Update.abort();
                accepted = false;
            } else if (Update.write(file.buf, file.currentSize) != file.currentSize) {
                failure = Update.errorString();
                Update.abort();
                accepted = false;
            } else received += file.currentSize;
        } else if (file.status == UPLOAD_FILE_END && accepted) {
            completed = received > 0;
        } else if (file.status == UPLOAD_FILE_ABORTED) {
            if (accepted) Update.abort();
            accepted = completed = false;
            failure = "Envio interrompido. O firmware anterior continua ativo.";
        }
    }

public:
    size_t capacity() const {
        const esp_partition_t* running = esp_ota_get_running_partition();
        const esp_partition_t* next = esp_ota_get_next_update_partition(nullptr);
        return running && next && next->address != running->address ? next->size : 0;
    }

    void begin(WebServer& web) {
        Serial.println("[OTA] Atualizacao sem senha disponivel em /ota.");
        if (!capacity()) {
            Serial.println(
                "[OTA] ArduinoOTA indisponivel: grave por USB para instalar a tabela de particoes OTA.");
        }
        ArduinoOTA.setHostname("M5NanoC6");
        ArduinoOTA.setPort(3232);
        // No core 3.3.12, setPassword("") ativa autenticacao com hash da senha vazia.
        ArduinoOTA.setMdnsEnabled(true);
        ArduinoOTA.onStart([this]() {
            networkProgress = 255;
            Serial.println("[ArduinoOTA] Recebendo atualizacao pela rede...");
        });
        ArduinoOTA.onProgress([this](unsigned int progress, unsigned int total) {
            uint8_t percent = total ? (uint64_t(progress) * 100 / total) : 0;
            if (percent != networkProgress) {
                networkProgress = percent;
                Serial.printf("[ArduinoOTA] Progresso: %u%%\n", percent);
            }
        });
        ArduinoOTA.onEnd([]() { Serial.println("[ArduinoOTA] Concluido. Reiniciando..."); });
        ArduinoOTA.onError([](ota_error_t error) {
            Serial.printf("[ArduinoOTA] Falha na atualizacao: %u\n", unsigned(error));
        });

        web.on("/ota", HTTP_GET, [this, &web]() {
            String page = F("<!doctype html><html lang='pt-BR'><meta charset='utf-8'>"
                "<meta name='viewport' content='width=device-width,initial-scale=1'>"
                "<title>Atualizar firmware</title><body style='max-width:700px;margin:30px auto;"
                "padding:20px;background:#10141c;color:#eee;font:16px system-ui'>"
                "<a style='color:#ff9751' href='/info'>Voltar ao painel</a>"
                "<h1>Atualizar firmware por Wi-Fi</h1>"
                "<p>Envie somente M5NanoC6_wifi.ino.bin compilado para este projeto. "
                "Nao envie bootloader, partitions ou merged. Mantenha a alimentacao ligada.</p>");
            if (capacity()) {
                page += "<p id='ota-limit' data-bytes='" + String(capacity()) +
                        "'>Limite da particao: " + String(capacity()) + " bytes.</p>";
                page += otaUploadForm;
            } else page += F("<p>Grave primeiro por USB com a tabela de duas particoes OTA.</p>");
            web.sendHeader("Cache-Control", "no-store");
            web.send(200, "text/html; charset=utf-8", page + "</body></html>");
        });
        web.on("/ota", HTTP_POST, [this, &web]() {
            web.sendHeader("Connection", "close");
            web.sendHeader("Cache-Control", "no-store");
            if (accepted) {
                completed = completed && Update.end(true);
                if (!completed) {
                    failure = Update.errorString();
                    if (Update.isRunning()) Update.abort();
                }
                accepted = false;
            }
            if (!completed) {
                web.send(400, "text/plain; charset=utf-8",
                         failure.length() ? failure : "Nenhum firmware valido recebido.");
                return;
            }
            completed = false;
            web.send(200, "text/html; charset=utf-8",
                "<meta charset='utf-8'><meta http-equiv='refresh' content='10;url=/info'>"
                "<h2>Firmware validado. Reiniciando...</h2><a href='/info'>Abrir painel</a>");
            rebootPending = true;
            rebootAt = millis();
        }, [this, &web]() { upload(web); });
    }

    void update() {
        if (rebootPending) {
            if (millis() - rebootAt >= 1500UL) ESP.restart();
            return;
        }
        // Os dois transportes compartilham Update; nao atende ArduinoOTA durante upload HTTP.
        if (accepted) return;
        bool connected = WiFi.status() == WL_CONNECTED && capacity() > 0;
        uint32_t address = connected ? uint32_t(WiFi.localIP()) : 0;
        if (networkStarted && (!connected || address != networkAddress)) {
            ArduinoOTA.end();
            networkStarted = false;
        }
        if (!connected) return;
        if (!networkStarted) {
            ArduinoOTA.begin();
            networkStarted = true;
            networkAddress = address;
            Serial.println("[ArduinoOTA] M5NanoC6 anunciado na rede, porta 3232, sem senha.");
        }
        ArduinoOTA.handle();
    }
};
#endif
