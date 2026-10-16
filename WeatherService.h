#ifndef WEATHER_SERVICE_H
#define WEATHER_SERVICE_H

#include <Arduino.h>
#include <WiFi.h>
#include <NetworkClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <math.h>
#include <time.h>
#include "Config.h"

class WeatherService
{
  private:
    portMUX_TYPE trava = portMUX_INITIALIZER_UNLOCKED;
    bool valido = false;
    bool falhou = false;
    float temperatura = 0;
    int codigo = -1;
    unsigned long ultimaLeitura = 0;
    time_t instanteDado = 0;
    float umidade = NAN;
    float sensacao = NAN;
    float vento = NAN;
    float maxima = NAN;
    float minima = NAN;
    time_t diaPrevisao = 0;
    bool iniciado = false;

    static const char* descricao(int c)
    {
        switch (c)
        {
        case 0:
            return "Ceu limpo";
        case 1:
            return "Poucas nuvens";
        case 2:
            return "Parc. nublado";
        case 3:
            return "Nublado";
        case 45:
        case 48:
            return "Nevoeiro";
        case 51:
        case 53:
        case 55:
            return "Garoa";
        case 56:
        case 57:
            return "Garoa gelada";
        case 61:
            return "Chuva leve";
        case 63:
            return "Chuva";
        case 65:
            return "Chuva forte";
        case 66:
        case 67:
            return "Chuva gelada";
        case 71:
        case 73:
        case 75:
        case 77:
        case 85:
        case 86:
            return "Neve";
        case 80:
        case 81:
        case 82:
            return "Panc. chuva";
        case 95:
        case 97:
            return "Trovoadas";
        case 96:
        case 99:
            return "Granizo";
        default:
            return "Sem descricao";
        }
    }

    bool consultar()
    {
        NetworkClientSecure cliente;
        cliente.useBuiltinCACertBundle();
        cliente.setHandshakeTimeout(8);
        HTTPClient http;
        http.setConnectTimeout(5000);
        http.setTimeout(5000);
        http.useHTTP10(true);
        String url =
            "https://api.open-meteo.com/v1/forecast?latitude=" + String(Config::CLIMA_LATITUDE, 5) +
            "&longitude=" + String(Config::CLIMA_LONGITUDE, 5) +
            "&current=temperature_2m,weather_code,relative_humidity_2m,apparent_temperature,wind_"
            "speed_10m"
            "&daily=temperature_2m_max,temperature_2m_min&timezone=America%2FSao_Paulo"
            "&temperature_unit=celsius&wind_speed_unit=kmh&timeformat=unixtime&forecast_days=1";
        if (!http.begin(cliente, url))
            return false;
        int status = http.GET();
        bool ok = false;
        if (status == HTTP_CODE_OK && http.getSize() <= 2048)
        {
            String corpo = http.getString();
            JsonDocument dados;
            if (corpo.length() <= 2048 && !deserializeJson(dados, corpo))
            {
                JsonVariant t = dados["current"]["temperature_2m"];
                JsonVariant c = dados["current"]["weather_code"];
                JsonVariant instante = dados["current"]["time"];
                if (t.is<float>() && c.is<int>() && instante.is<long>())
                {
                    float temp = t.as<float>();
                    time_t momento = instante.as<long>();
                    time_t agora = time(nullptr);
                    if (isfinite(temp) && temp >= -90 && temp <= 60 && momento >= agora - 3600 &&
                        momento <= agora + 900)
                    {
                        portENTER_CRITICAL(&trava);
                        temperatura = temp;
                        codigo = c.as<int>();
                        instanteDado = momento;
                        umidade = dados["current"]["relative_humidity_2m"].is<float>()
                                      ? dados["current"]["relative_humidity_2m"].as<float>()
                                      : NAN;
                        sensacao = dados["current"]["apparent_temperature"].is<float>()
                                       ? dados["current"]["apparent_temperature"].as<float>()
                                       : NAN;
                        vento = dados["current"]["wind_speed_10m"].is<float>()
                                    ? dados["current"]["wind_speed_10m"].as<float>()
                                    : NAN;
                        maxima = dados["daily"]["temperature_2m_max"][0].is<float>()
                                     ? dados["daily"]["temperature_2m_max"][0].as<float>()
                                     : NAN;
                        minima = dados["daily"]["temperature_2m_min"][0].is<float>()
                                     ? dados["daily"]["temperature_2m_min"][0].as<float>()
                                     : NAN;
                        diaPrevisao = dados["daily"]["time"][0].as<long>();
                        if (!isfinite(maxima) || !isfinite(minima) || minima < -90 || maxima > 60 ||
                            minima > maxima)
                            maxima = minima = NAN;
                        ultimaLeitura = millis();
                        valido = true;
                        portEXIT_CRITICAL(&trava);
                        ok = true;
                        Serial.printf("[CLIMA] Porto Alegre: %.1f C, codigo %d.\n", temp,
                                      c.as<int>());
                    }
                }
            }
        }
        http.end();
        if (!ok)
            Serial.printf("[CLIMA] Consulta falhou (HTTP %d). Nova tentativa em 2 min.\n", status);
        return ok;
    }

    static void tarefa(void* argumento)
    {
        auto* servico = static_cast<WeatherService*>(argumento);
        bool tentou = false;
        unsigned long ultimaTentativa = 0;
        unsigned long intervalo = 0;
        for (;;)
        {
            if (WiFi.status() == WL_CONNECTED && time(nullptr) > 1700000000 &&
                (!tentou || millis() - ultimaTentativa >= intervalo))
            {
                bool ok = servico->consultar();
                portENTER_CRITICAL(&servico->trava);
                servico->falhou = !ok;
                portEXIT_CRITICAL(&servico->trava);
                ultimaTentativa = millis();
                intervalo = ok ? 600000UL : 120000UL;
                tentou = true;
            }
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

  public:
    struct Estado
    {
        bool disponivel;
        bool falhou;
        float temperatura;
        float umidade;
        float sensacao;
        float vento;
        int codigo;
        unsigned long idade;
        time_t instante;
        float maxima;
        float minima;
    };

    Estado obterEstado()
    {
        portENTER_CRITICAL(&trava);
        unsigned long idade = millis() - ultimaLeitura;
        if (valido && idade >= 1800000UL)
        {
            valido = false;
            falhou = true;
        }
        Estado estado{valido, falhou, temperatura,  umidade, sensacao, vento,
                      codigo, idade,  instanteDado, maxima,  minima};
        if ((diaPrevisao - 10800) / 86400 != (time(nullptr) - 10800) / 86400)
            estado.maxima = estado.minima = NAN;
        portEXIT_CRITICAL(&trava);
        return estado;
    }

    static const char* descricaoCompleta(int c)
    {
        switch (c)
        {
        case 0:
            return "Ceu limpo";
        case 1:
            return "Predominantemente limpo";
        case 2:
            return "Parcialmente nublado";
        case 3:
            return "Encoberto";
        case 45:
            return "Nevoeiro";
        case 48:
            return "Nevoeiro gelado";
        case 51:
            return "Garoa leve";
        case 53:
            return "Garoa moderada";
        case 55:
            return "Garoa intensa";
        case 56:
            return "Garoa gelada leve";
        case 57:
            return "Garoa gelada forte";
        case 61:
            return "Chuva leve";
        case 63:
            return "Chuva moderada";
        case 65:
            return "Chuva forte";
        case 66:
            return "Chuva gelada leve";
        case 67:
            return "Chuva gelada forte";
        case 71:
            return "Neve leve";
        case 73:
            return "Neve moderada";
        case 75:
            return "Neve forte";
        case 77:
            return "Graos de neve";
        case 80:
            return "Pancadas leves";
        case 81:
            return "Pancadas moderadas";
        case 82:
            return "Pancadas fortes";
        case 85:
            return "Pancadas de neve";
        case 86:
            return "Neve forte";
        case 95:
            return "Trovoadas";
        case 97:
            return "Trovoadas fortes";
        case 96:
            return "Trovoadas e granizo";
        case 99:
            return "Trovoadas e granizo forte";
        default:
            return "Sem descricao";
        }
    }

    void begin()
    {
        if (iniciado)
            return;
        iniciado = xTaskCreate(tarefa, "clima", 8192, this, 1, nullptr) == pdPASS;
        if (!iniciado)
        {
            portENTER_CRITICAL(&trava);
            falhou = true;
            portEXIT_CRITICAL(&trava);
            Serial.println(F("[CLIMA] Sem memoria para iniciar consulta."));
        }
    }

    String getLinha()
    {
        Estado estado = obterEstado();
        if (estado.disponivel)
        {
            String marca = (estado.falhou || WiFi.status() != WL_CONNECTED) ? "*" : "";
            String detalhada = descricaoCompleta(estado.codigo);
            String decimal = String(estado.temperatura, 1) + "C ";
            String inteira = String(estado.temperatura, 0) + "C ";
            String linha;
            if (decimal.length() + detalhada.length() + marca.length() <= 20)
                linha = decimal + detalhada;
            else if (inteira.length() + detalhada.length() + marca.length() <= 20)
                linha = inteira + detalhada;
            String curta = descricao(estado.codigo);
            if (linha.isEmpty())
            {
                linha = decimal.length() + curta.length() + marca.length() <= 20 ? decimal + curta
                                                                                 : inteira + curta;
            }
            return linha + marca;
        }
        if (WiFi.status() != WL_CONNECTED)
            return "Clima: sem WiFi";
        if (time(nullptr) <= 1700000000)
            return "Clima: aguarda hora";
        if (estado.falhou)
            return "Clima: indisponivel";
        return "Clima: buscando...";
    }

    String getResumo()
    {
        Estado estado = obterEstado();
        String maximo =
            estado.disponivel && isfinite(estado.maxima) ? String(estado.maxima, 0) : "--";
        String minimo =
            estado.disponivel && isfinite(estado.minima) ? String(estado.minima, 0) : "--";
        String hum = estado.disponivel && isfinite(estado.umidade) && estado.umidade >= 0 &&
                             estado.umidade <= 100
                         ? String(estado.umidade, 0)
                         : "--";
        String linha = "Max:" + maximo + " Min:" + minimo + " U" + hum + "%";
        // Mantem a umidade inteira mesmo com duas temperaturas negativas de tres caracteres.
        if (linha.length() > 20)
            linha = "Mx" + maximo + " Mn" + minimo + " U" + hum + "%";
        return linha;
    }
};

extern WeatherService clima;

#endif
