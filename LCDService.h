#ifndef LCD_SERVICE_H
#define LCD_SERVICE_H

#include <Arduino.h>
#include <string.h>
#include <Wire.h>
#include <WiFi.h>
#include "Config.h"
#include "NTPService.h"
#include "WeatherService.h"

// Backpack comum: P0=RS, P1=RW, P2=EN, P3=backlight, P4..P7=D4..D7.
// HD44780 em modo 4 bits. Usa apenas Wire, incluida no core ESP32.
class LCDService
{
  private:
    uint8_t endereco = 0;
    bool ativo = false;
    bool ligado = true;
    unsigned long ultimaAtualizacao = 0;
    char linhas[4][21] = {};

    bool escreverPorta(uint8_t valor)
    {
        Wire.beginTransmission(endereco);
        Wire.write((valor & ~0x08) | (ligado ? 0x08 : 0)); // Backlight em P3; RW baixo.
        return Wire.endTransmission() == 0;
    }

    bool nibble(uint8_t valor)
    {
        if (!escreverPorta(valor))
            return false;
        if (!escreverPorta(valor | 0x04))
            return false;
        delayMicroseconds(1);
        if (!escreverPorta(valor & ~0x04))
            return false;
        delayMicroseconds(50);
        return true;
    }

    bool enviar(uint8_t valor, bool dado = false)
    {
        uint8_t rs = dado ? 0x01 : 0;
        return nibble((valor & 0xF0) | rs) && nibble(((valor << 4) & 0xF0) | rs);
    }

    bool responder(uint8_t alvo)
    {
        Wire.beginTransmission(alvo);
        return Wire.endTransmission() == 0;
    }

    void linha(uint8_t numero, const String& texto)
    {
        char nova[21];
        memset(nova, ' ', 20);
        nova[20] = '\0';
        for (size_t i = 0; i < texto.length() && i < 20; ++i)
        {
            uint8_t c = texto[i];
            nova[i] = (c >= 32 && c <= 126) ? c : '?';
        }
        if (!ativo || memcmp(nova, linhas[numero], 20) == 0)
            return;
        const uint8_t offsets[] = {0x00, 0x40, 0x14, 0x54};
        bool ok = enviar(0x80 | offsets[numero]);
        for (uint8_t i = 0; ok && i < 20; ++i)
            ok = enviar(nova[i], true);
        if (!ok)
        {
            ativo = false;
            Serial.println(F("[LCD] Falha I2C; display desativado ate reiniciar."));
            return;
        }
        memcpy(linhas[numero], nova, sizeof(nova));
    }

  public:
    bool estaAtivo() const
    {
        return ativo;
    }

    bool estaLigado() const
    {
        return ativo && ligado;
    }

    bool definirLigado(bool habilitar)
    {
        if (!ativo)
            return false;
        ligado = habilitar;
        if (!enviar(habilitar ? 0x0C : 0x08))
        {
            ativo = false;
            Serial.println(F("[LCD] Falha I2C ao controlar a tela; reinicie para recuperar."));
            return false;
        }
        if (habilitar)
        {
            memset(linhas, 0, sizeof(linhas));
            ultimaAtualizacao = millis() - 1000UL;
            atualizar();
        }
        return ativo;
    }

    uint8_t obterEndereco() const
    {
        return endereco;
    }

    String obterLinha(uint8_t numero) const
    {
        return numero < 4 ? String(linhas[numero]) : String();
    }

    bool begin()
    {
        ligado = true;
        if (!Wire.begin(Config::PIN_LCD_SDA, Config::PIN_LCD_SCL, 100000))
        {
            Serial.println(F("[LCD] Falha ao iniciar I2C."));
            return false;
        }
        Wire.setTimeOut(20);
        delay(50);
        if (Config::LCD_ENDERECO != 0)
        {
            endereco = Config::LCD_ENDERECO;
            if (!responder(endereco))
                endereco = 0;
        }
        else
        {
            uint8_t encontrados = 0;
            for (uint8_t alvo = 0x20; alvo <= 0x27; ++alvo)
            {
                if (responder(alvo))
                {
                    endereco = alvo;
                    ++encontrados;
                }
            }
            if (encontrados > 1)
            {
                Serial.println(F("[LCD] Mais de um endereco; defina LCD_ENDERECO em Config.h."));
                return false;
            }
        }
        if (!endereco)
        {
            Serial.println(
                F("[LCD] PCF8574 nao encontrado. Confira SDA=G2, SCL=G1 e alimentacao."));
            return false;
        }
        // Sequencia de inicializacao independente do estado anterior do LCD.
        bool ok = escreverPorta(0);
        ok = ok && nibble(0x30);
        delay(5);
        ok = ok && nibble(0x30);
        delayMicroseconds(150);
        ok = ok && nibble(0x30);
        ok = ok && nibble(0x20);
        ok = ok && enviar(0x28); // 4 bits, 2 linhas logicas (LCD 20x4).
        ok = ok && enviar(0x08); // Display desligado durante inicializacao.
        ok = ok && enviar(0x01);
        delay(2);
        ok = ok && enviar(0x06); // Incrementa cursor, sem deslocar tela.
        ok = ok && enviar(0x0C); // Display ligado, cursor oculto.
        ativo = ok;
        if (!ativo)
        {
            Serial.println(F("[LCD] Falha ao inicializar display."));
            return false;
        }
        Serial.printf("[LCD] 20x4 em 0x%02X; SDA=G2, SCL=G1.\n", endereco);
        linha(0, "M5NanoC6 LCD 20x4");
        linha(1, "Iniciando Wi-Fi...");
        return true;
    }

    void atualizar()
    {
        if (!ativo || !ligado || millis() - ultimaAtualizacao < 1000UL)
            return;
        ultimaAtualizacao = millis();
        linha(0, ntp.getData() + " " + ntp.getHora());
        bool conectado = WiFi.status() == WL_CONNECTED;
        bool portal = WiFi.getMode() == WIFI_AP;
        linha(1, "IP: " + (conectado ? WiFi.localIP().toString()
                                     : (portal ? WiFi.softAPIP().toString() : String("--"))));
        linha(2, clima.getResumo());
        linha(3, clima.getLinha());
    }
};

extern LCDService lcd;

#endif
