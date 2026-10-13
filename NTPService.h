#ifndef NTP_SERVICE_H
#define NTP_SERVICE_H

#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include <esp_sntp.h>
#include <atomic>

class NTPService {
private:
    static std::atomic<bool>& recebeuResposta() {
        static std::atomic<bool> recebido{false};
        return recebido;
    }
    static void aoSincronizar(struct timeval*) { recebeuResposta().store(true); }
    bool aguardando = false;
    bool iniciado = false;
    unsigned long inicioTentativa = 0;
    int fusoAtual = -3;
    bool dstAtual = false;

public:
    bool begin(int fuso, bool dstAtivo) { return sincronizar(fuso, dstAtivo); }

    bool sincronizar(int fuso, bool dstAtivo) {
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println(F("[NTP] Sem Wi-Fi. Conecte a uma rede com acesso a Internet."));
            return false;
        }
        // Exige uma nova resposta, mesmo se o relogio ja tiver uma hora valida.
        recebeuResposta().store(false);
        aguardando = true;
        iniciado = true;
        inicioTentativa = millis();
        esp_sntp_set_time_sync_notification_cb(aoSincronizar);
        configurarRelogio(fuso, dstAtivo);
        Serial.println(F("[NTP] Sincronizacao iniciada (limite: 30 s)."));
        return true;
    }

    void configurarRelogio(int fuso, bool dstAtivo) {
        fusoAtual = fuso;
        dstAtual = dstAtivo;
        // DST_ON aplica manualmente uma hora extra, sem regras sazonais implicitas.
        char tzString[32];
        snprintf(tzString, sizeof(tzString), "UTC%d", -(fuso + (dstAtivo ? 1 : 0)));
        configTzTime(tzString, "a.st1.ntp.br", "pool.ntp.org", "time.nist.gov");
    }

    void atualizar() {
        if (!iniciado) return;
        if (aguardando && recebeuResposta().load()) {
            aguardando = false;
            Serial.println("[NTP] Sincronizado: " + getData() + " " + getHora());
        } else if (aguardando && millis() - inicioTentativa >= 30000UL) {
            aguardando = false;
            Serial.println(F("[NTP] Sem resposta em 30 s. Verifique Internet, DNS e UDP 123; nova tentativa em 60 s."));
        }
        if (!aguardando && !recebeuResposta().load() &&
            millis() - inicioTentativa >= 90000UL && WiFi.status() == WL_CONNECTED) {
            sincronizar(fusoAtual, dstAtual);
        }
    }

    String getStatus() {
        if (aguardando && !recebeuResposta().load()) return "SINCRONIZANDO";
        return isSincronizado() ? "OK" : "FALHA";
    }

    String getHora() {
        struct tm timeinfo;
        if (!getLocalTime(&timeinfo, 0)) return "00:00:00";
        char buffer[16];
        strftime(buffer, sizeof(buffer), "%H:%M:%S", &timeinfo);
        return String(buffer);
    }

    String getData() {
        struct tm timeinfo;
        if (!getLocalTime(&timeinfo, 0)) return "00/00/0000";
        char buffer[16];
        strftime(buffer, sizeof(buffer), "%d/%m/%Y", &timeinfo);
        return String(buffer);
    }

    bool isSincronizado() { return recebeuResposta().load(); }
};

extern NTPService ntp;

#endif
