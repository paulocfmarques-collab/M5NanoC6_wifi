#ifndef HARDWARE_CONTROLLER_H
#define HARDWARE_CONTROLLER_H

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "Config.h"

class HardwareController {
private:
    Adafruit_NeoPixel led;
    bool blinkAtivo;
    unsigned long ultimoToggle;
    unsigned long intervaloBlink;
    bool estadoBlink;
    
    uint8_t atualR, atualG, atualB, atualBrilho;

public:
    HardwareController() : led(1, Config::PIN_LED_RGB, NEO_GRB + NEO_KHZ800), blinkAtivo(false), ultimoToggle(0), intervaloBlink(500), estadoBlink(false), atualR(0), atualG(0), atualB(0), atualBrilho(20) {}

    void begin() {
        pinMode(Config::PIN_BOTAO, INPUT_PULLUP);
        pinMode(Config::PIN_IR_TX, OUTPUT);
        pinMode(Config::PIN_LED_AZUL, OUTPUT);
        digitalWrite(Config::PIN_IR_TX, LOW);
        digitalWrite(Config::PIN_LED_AZUL, LOW);
        
        // Ativa o regulador elétrico do circuito NeoPixel (Pino 19) conforme o manual
        pinMode(Config::PIN_RGB_ENABLE, OUTPUT);
        digitalWrite(Config::PIN_RGB_ENABLE, HIGH);
        
        led.begin();
        led.clear();
        led.show();
        
        // MANUAL STATUS: Inicializando -> Branco (255, 255, 255)
        setLedColor(255, 255, 255, 20); 
    }

    void setLedColor(uint8_t r, uint8_t g, uint8_t b, uint8_t brilho = 20) {
        blinkAtivo = false;
        atualR = r;
        atualG = g;
        atualB = b;
        atualBrilho = brilho;
        
        led.setBrightness(brilho);
        led.setPixelColor(0, led.Color(r, g, b));
        led.show();
    }

    void iniciarBlinkAsync(uint8_t r, uint8_t g, uint8_t b, unsigned long intervalo, uint8_t brilho = 20) {
        atualR = r;
        atualG = g;
        atualB = b;
        atualBrilho = brilho;
        intervaloBlink = intervalo;
        blinkAtivo = true;
    }

    void piscarSincrono(int piscadas, int tempoMs, uint8_t r, uint8_t g, uint8_t b) {
        blinkAtivo = false;
        for (int i = 0; i < piscadas; i++) {
            led.setBrightness(20);
            led.setPixelColor(0, led.Color(r, g, b)); led.show(); delay(tempoMs);
            led.setPixelColor(0, led.Color(0, 0, 0)); led.show(); delay(tempoMs);
        }
        led.setPixelColor(0, led.Color(atualR, atualG, atualB));
        led.show();
    }

    bool botaoPressionado() {
        return (digitalRead(Config::PIN_BOTAO) == LOW);
    }

    void dispararPulsoIR() {
        for(int i = 0; i < 30; i++) {
            digitalWrite(Config::PIN_IR_TX, HIGH); delayMicroseconds(13);
            digitalWrite(Config::PIN_IR_TX, LOW);  delayMicroseconds(13);
        }
    }

    void atualizarBlink() {
        if (!blinkAtivo) return;

        unsigned long agora = millis();
        if (agora - ultimoToggle >= intervaloBlink) {
            ultimoToggle = agora;
            estadoBlink = !estadoBlink;
            led.setBrightness(atualBrilho);
            if (estadoBlink) {
                led.setPixelColor(0, led.Color(atualR, atualG, atualB));
            } else {
                led.setPixelColor(0, led.Color(0, 0, 0));
            }
            led.show();
        }
    }
};

extern HardwareController hardware;

#endif
