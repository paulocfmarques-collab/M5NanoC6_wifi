#ifndef NTP_SERVICE_H
#define NTP_SERVICE_H

#include <Arduino.h>
#include <time.h>
#include "HardwareController.h"

class NTPService {
public:
    bool begin(int fuso, bool dstAtivo) {
        Serial.println(F("[NTP] Inicializando..."));
        configurarRelogio(fuso, dstAtivo);

        for (int i = 0; i < 15; i++) {
            struct tm timeinfo;
            if (getLocalTime(&timeinfo, 1000)) {
                Serial.println(F("[NTP] Sincronizado com sucesso."));
                // STATUS MANUAL: NTP Sincronizado -> Magenta Fixo
                hardware.setLedColor(255, 0, 255, 15);
                delay(1500); 
                hardware.setLedColor(0, 255, 0, 15); // Retorna ao verde estável do UDP
                return true;
            }
            Serial.print(".");
            delay(500);
        }
        return false;
    }

    void configurarRelogio(int fuso, bool dstAtivo) {
        char tzString[32]; // Buffer de tamanho estático seguro para POSIX
        int fusoInvertido = -fuso; 

        if (dstAtivo) {
            snprintf(tzString, sizeof(tzString), "GMT%dGMT%d", fusoInvertido, fusoInvertido - 1);
        } else {
            snprintf(tzString, sizeof(tzString), "GMT%d", fusoInvertido);
        }
        
        configTzTime(tzString, "a.st1.ntp.br", "pool.ntp.org", "time.nist.gov");
    }

    String getHora() {
        struct tm timeinfo;
        if (!getLocalTime(&timeinfo, 50)) return "00:00:00";
        char buffer[16];
        strftime(buffer, sizeof(buffer), "%H:%M:%S", &timeinfo);
        return String(buffer);
    }

    String getData() {
        struct tm timeinfo;
        if (!getLocalTime(&timeinfo, 50)) return "00/00/0000";
        char buffer[16];
        strftime(buffer, sizeof(buffer), "%d/%m/%Y", &timeinfo);
        return String(buffer);
    }

    bool isSincronizado() {
        struct tm timeinfo;
        return getLocalTime(&timeinfo, 1000);
    }
};

extern NTPService ntp;

#endif
