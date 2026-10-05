#ifndef HARDWARE_CONTROLLER_H
#define HARDWARE_CONTROLLER_H

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <math.h> // Incluído para a função sin()
#include "Config.h"

class HardwareController {
private:
    Adafruit_NeoPixel led;
    bool blinkAtivo;
    unsigned long ultimoToggle;
    unsigned long intervaloBlink;
    bool estadoBlink;
    
    // Variáveis para o efeito Breath
    bool breathAtivo;
    unsigned long tempoInicialBreath;
    unsigned long duracaoCicloBreath; // Em milissegundos (ex: 3000ms para um ciclo completo)

    uint8_t atualR, atualG, atualB, atualBrilho;

public:
    HardwareController() : led(1, Config::PIN_LED_RGB, NEO_GRB + NEO_KHZ800), 
                           blinkAtivo(false), ultimoToggle(0), intervaloBlink(500), estadoBlink(false), 
                           breathAtivo(false), tempoInicialBreath(0), duracaoCicloBreath(3000),
                           atualR(0), atualG(0), atualB(0), atualBrilho(20) {}

    void begin() {
        pinMode(Config::PIN_BOTAO, INPUT_PULLUP);
        pinMode(Config::PIN_IR_TX, OUTPUT);
        pinMode(Config::PIN_LED_AZUL, OUTPUT);
        digitalWrite(Config::PIN_IR_TX, LOW);
        digitalWrite(Config::PIN_LED_AZUL, LOW);
        
        pinMode(Config::PIN_RGB_ENABLE, OUTPUT);
        digitalWrite(Config::PIN_RGB_ENABLE, HIGH);
        
        led.begin();
        led.clear();
        led.show();
        
        setLedColor(255, 255, 255, 20); 
    }

    void setLedColor(uint8_t r, uint8_t g, uint8_t b, uint8_t brilho = 20) {
        blinkAtivo = false;
        breathAtivo = false; // Desativa o efeito breath se uma cor estática for definida
        atualR = r;
        atualG = g;
        atualB = b;
        atualBrilho = brilho;
        
        led.setBrightness(brilho);
        led.setPixelColor(0, led.Color(r, g, b));
        led.show();
    }

    void iniciarBlinkAsync(uint8_t r, uint8_t g, uint8_t b, unsigned long intervalo, uint8_t brilho = 20) {
        breathAtivo = false;
        atualR = r;
        atualG = g;
        atualB = b;
        atualBrilho = brilho;
        intervaloBlink = intervalo;
        blinkAtivo = true;
    }

    // Método para ativar o efeito de Respiração
    void iniciarBreathAsync(uint8_t r, uint8_t g, uint8_t b, unsigned long duracaoCiclo = 3000, uint8_t brilhoMaximo = 40) {
        blinkAtivo = false;
        atualR = r;
        atualG = g;
        atualB = b;
        atualBrilho = brilhoMaximo; // Usado como o pico máximo da respiração
        duracaoCicloBreath = duracaoCiclo;
        tempoInicialBreath = millis();
        breathAtivo = true;
    }

    void piscarSincrono(int piscadas, int tempoMs, uint8_t r, uint8_t g, uint8_t b) {
        blinkAtivo = false;
        breathAtivo = false;
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

    // Atualiza os efeitos temporizados (Blink ou Breath) de forma assíncrona
    void atualizarEfeitos() {
        unsigned long agora = millis();

        // 1. Processa o efeito Blink
        if (blinkAtivo) {
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
        // 2. Processa o efeito Breath (Respiração)
        else if (breathAtivo) {
            // Mapeia o tempo decorrido dentro do ciclo atual para um ângulo em radianos (0 a PI)
            float progressoCiclo = (float)((agora - tempoInicialBreath) % duracaoCicloBreath) / duracaoCicloBreath;
            
            // Usamos sin() de forma que varie de 0 a 1 e volte a 0 suavemente
            float fatorBrilho = sin(progressoCiclo * 2.0 * M_PI);
            fatorBrilho = (fatorBrilho + 1.0) / 2.0; // Transforma o range de [-1, 1] para [0, 1]

            // Calcula o brilho dinâmico baseado no brilho máximo estipulado
            uint8_t brilhoDinamico = (uint8_t)(fatorBrilho * atualBrilho);

            led.setBrightness(brilhoDinamico);
            led.setPixelColor(0, led.Color(atualR, atualG, atualB));
            led.show();
        }
    }

    void setDuracaoCicloBreath(unsigned long duracaoMs) {
        if (duracaoMs > 0) {
            duracaoCicloBreath = duracaoMs;
        }
}    
};

extern HardwareController hardware;

#endif
