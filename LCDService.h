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
    uint8_t tela = 1, iconeClima = 0;
    unsigned long interacaoEm = 0, rolagemEm = 0;
    String textoRolagem;

    static String icone(uint8_t codigo) { String s; s += char(codigo); return s; }
    bool desenharIcone(uint8_t slot, const uint8_t* pixels) {
        if(!enviar(0x40 | (slot << 3)))return false;
        for(uint8_t i=0;i<8;++i)if(!enviar(pixels[i],true))return false;
        return true;
    }
    bool carregarIcones() {
        const uint8_t pixels[7][8]={
            {10,31,17,31,17,21,17,31}, // calendario
            {4,14,21,4,4,4,0,0},      // maxima
            {4,4,4,21,14,4,0,0},      // minima
            {4,10,10,14,14,31,31,14}, // termometro
            {0,14,17,4,10,0,4,0},     // rede
            {4,21,14,31,14,21,4,0},   // sol
            {6,9,9,6,0,0,0,0}        // grau
        };
        for(uint8_t i=0;i<7;++i)if(!desenharIcone(i+1,pixels[i]))return false;
        return true;
    }
    void atualizarIconeClima(int codigo) {
        uint8_t tipo=codigo>=0&&codigo<=1?0:(codigo<=48?1:2);
        if(tipo==iconeClima)return;
        const uint8_t pixels[3][8]={
            {4,21,14,31,14,21,4,0},
            {0,0,6,15,31,31,0,0},
            {0,6,15,31,31,0,10,20}
        };
        if(!desenharIcone(6,pixels[tipo]))ativo=false;
        else iconeClima=tipo;
    }
    String rolar(const String& texto,uint8_t largura) {
        if(texto!=textoRolagem){textoRolagem=texto;rolagemEm=millis();}
        if(texto.length()<=largura)return texto;
        String faixa=texto+"   ",saida;
        unsigned long idade=millis()-rolagemEm;
        size_t inicio=idade<2000?0:((idade-2000)/1000)%faixa.length();
        for(uint8_t i=0;i<largura;++i)saida+=faixa[(inicio+i)%faixa.length()];
        return saida;
    }
    static String temperatura(float valor,bool disponivel) {
        return disponivel&&isfinite(valor)?String(valor,1):String("--");
    }

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
            nova[i] = ((c >= 1 && c <= 7) || (c >= 32 && c <= 126)) ? c : '?';
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

    uint8_t obterTela() const { return tela; }
    void registrarInteracao() { interacaoEm=millis(); }
    bool alternarTela() {
        if(!ativo||!ligado)return false;
        tela=tela==1?2:1;registrarInteracao();textoRolagem="";
        ultimaAtualizacao=millis()-1000UL;atualizar();return ativo;
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
            tela=1;registrarInteracao();textoRolagem="";
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
        if(numero>=4)return String();
        const char* simbolos[]={"", "▦", "↑", "↓", "♨", "≋", "☀", "°"};
        String texto;
        for(uint8_t i=0;i<20&&linhas[numero][i];++i) {
            uint8_t c=linhas[numero][i];
            if(c==6)texto+=iconeClima==0?"☀":(iconeClima==1?"☁":"☂");
            else if(c>=1&&c<=7)texto+=simbolos[c];
            else texto+=char(c);
        }
        return texto;
    }

    bool begin()
    {
        ligado = true;
        tela=1;iconeClima=0;registrarInteracao();textoRolagem="";
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
        ok = ok && carregarIcones();
        ok = ok && enviar(0x0C); // Display ligado, cursor oculto.
        ativo = ok;
        if (!ativo)
        {
            Serial.println(F("[LCD] Falha ao inicializar display."));
            return false;
        }
        Serial.printf("[LCD] 20x4 em 0x%02X; SDA=G2, SCL=G1.\n", endereco);
        memset(linhas,0,sizeof(linhas));
        ultimaAtualizacao=millis()-1000UL;atualizar();
        return true;
    }

    void atualizar()
    {
        if(ativo&&ligado&&tela==2&&millis()-interacaoEm>=15000UL) {
            tela=1;textoRolagem="";ultimaAtualizacao=millis()-1000UL;
        }
        if (!ativo || !ligado || millis() - ultimaAtualizacao < 1000UL)
            return;
        ultimaAtualizacao = millis();
        bool conectado = WiFi.status() == WL_CONNECTED;
        bool portal = WiFi.getMode() == WIFI_AP;
        if(tela==1) {
            auto estado=clima.obterEstado();
            atualizarIconeClima(estado.disponivel?estado.codigo:-1);
            linha(0,icone(1)+ntp.getData()+" "+ntp.getHora());
            linha(1,icone(2)+" "+temperatura(estado.maxima,estado.disponivel)+icone(7)+"C  "+
                    icone(3)+" "+temperatura(estado.minima,estado.disponivel)+icone(7)+"C");
            bool anterior=estado.disponivel&&(estado.falhou||!conectado);
            String umidade=estado.disponivel&&isfinite(estado.umidade)&&estado.umidade>=0&&estado.umidade<=100
                              ?String(estado.umidade,0):String("--");
            linha(2,icone(4)+" "+temperatura(estado.temperatura,estado.disponivel)+icone(7)+"C Umid:"+umidade+"%"+
                    (anterior?"*":""));
            String descricao=estado.disponivel?String(WeatherService::descricaoCompleta(estado.codigo)):clima.getLinha();
            linha(3,icone(6)+" "+rolar(descricao,18));
        } else {
            linha(0,icone(5)+(conectado?" Wi-Fi conectado":(portal?" Portal Wi-Fi AP":" Sem conexao")));
            linha(1,"IP: "+(conectado?WiFi.localIP().toString():(portal?WiFi.softAPIP().toString():String("--"))));
            String rede=conectado?WiFi.SSID():(portal?WiFi.softAPSSID():String("--"));
            linha(2,rolar("Rede: "+rede,20));
            linha(3,conectado?"RSSI:"+String(WiFi.RSSI())+"dBm Ch:"+String(WiFi.channel()):String("RSSI:--  Canal:--"));
        }
    }
};

extern LCDService lcd;

#endif
