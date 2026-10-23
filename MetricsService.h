#ifndef METRICS_SERVICE_H
#define METRICS_SERVICE_H

#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include <esp_wifi.h>
#include <esp_mac.h>
#include <esp_timer.h>
#include <esp_ota_ops.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <atomic>
#include "Config.h"
#include "WeatherService.h"

// Coleta local: somente o evento Wi-Fi escreve fora da tarefa principal.
class MetricsService
{
  public:
    struct Settings
    {
        uint32_t version;
        char host[254];
        uint16_t port;
        uint16_t interval;
    };

  private:
    Settings settings{};

    struct BootRecord
    {
        uint32_t version, boots, watchdogs;
    } boots{};

    bool countersOk = false;
    bool started = false;
    String device;
    uint32_t imageBytes = 0, partitionBytes = 0;
    std::atomic<uint32_t> disconnects{0};
    uint64_t requests = 0, errors = 0;
    bool udpListener = false;
    uint64_t udpPackets = 0, udpReplies = 0, udpSendErrors = 0;
    bool mqttConnected = false;
    uint64_t mqttSent = 0, mqttReceived = 0, mqttErrors = 0;
    double responseMs = 0;
    uint32_t lastCpuSample = 0;
    float cpuUsage = NAN;
#if configUSE_TRACE_FACILITY && configGENERATE_RUN_TIME_STATS
    TaskStatus_t tasks[64];
    configRUN_TIME_COUNTER_TYPE previousTotal = 0, previousIdle = 0;
    bool cpuBaseline = false;
#endif

    static String escaped(String value)
    {
        value.replace("\\", "\\\\");
        value.replace("\"", "\\\"");
        value.replace("\n", "\\n");
        value.replace("\r", "\\n");
        return value;
    }

    void sample(String& out, const char* name, const char* type, const String& value,
                const String& labels = "") const
    {
        out += "# HELP " + String(name) + " M5NanoC6: " + name + ".\n";
        out += "# TYPE " + String(name) + " " + type + "\n";
        out += String(name) + "{device=\"" + escaped(device) + "\"";
        if (labels.length())
            out += "," + labels;
        out += "} " + value + "\n";
    }

    static String label(const char* key, const String& value)
    {
        return String(key) + "=\"" + escaped(value) + "\"";
    }

  public:
    static bool validHost(const String& host)
    {
        if (host.isEmpty() || host.length() > 253)
            return false;
        unsigned labelSize = 0;
        char previous = 0;
        for (size_t i = 0; i < host.length(); ++i)
        {
            char c = host[i];
            bool alnum = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
            if (c == '.')
            {
                if (!labelSize || previous == '-')
                    return false;
                labelSize = 0;
            }
            else
            {
                if (!alnum && c != '-')
                    return false;
                if (!labelSize && c == '-')
                    return false;
                if (++labelSize > 63)
                    return false;
            }
            previous = c;
        }
        return labelSize && previous != '-';
    }

    static bool parseNumber(const String& text, uint32_t minimum, uint32_t maximum, uint16_t& value)
    {
        if (text.isEmpty() || text.length() > 5)
            return false;
        uint32_t parsed = 0;
        for (size_t i = 0; i < text.length(); ++i)
        {
            if (text[i] < '0' || text[i] > '9')
                return false;
            parsed = parsed * 10 + text[i] - '0';
        }
        if (parsed < minimum || parsed > maximum)
            return false;
        value = static_cast<uint16_t>(parsed);
        return true;
    }

    const Settings& configuration() const
    {
        return settings;
    }

    bool persistenceOk() const
    {
        return countersOk;
    }

    bool resetCounters()
    {
        Preferences prefs;
        if (!prefs.begin("telemetry", false))
        {
            countersOk = false;
            return false;
        }
        BootRecord zero{1, 0, 0};
        const bool ok = prefs.putBytes("boots", &zero, sizeof(zero)) == sizeof(zero);
        prefs.end();
        countersOk = ok;
        if (ok)
        {
            boots = zero;
        }
        return ok;
    }

    bool save(const String& host, uint16_t port, uint16_t interval)
    {
        if (!validHost(host) || !port || interval < 5 || interval > 3600)
            return false;
        Settings next{};
        next.version = 1;
        host.toCharArray(next.host, sizeof(next.host));
        next.port = port;
        next.interval = interval;
        Preferences prefs;
        if (!prefs.begin("telemetry", false))
            return false;
        bool ok = prefs.putBytes("settings", &next, sizeof(next)) == sizeof(next);
        prefs.end();
        if (ok)
            settings = next;
        return ok;
    }

    void begin()
    {
        if (started)
            return;
        started = true;
        settings.version = 1;
        settings.port = 80;
        settings.interval = 15;
        // MAC da interface STA, estavel inclusive quando o portal AP esta ativo.
        uint8_t mac[6];
        esp_read_mac(mac, ESP_MAC_WIFI_STA);
        char id[18];
        snprintf(id, sizeof(id), "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3],
                 mac[4], mac[5]);
        device = id;
        imageBytes = ESP.getSketchSize();
        const esp_partition_t* partition = esp_ota_get_running_partition();
        partitionBytes = partition ? partition->size : 0;
        Preferences prefs;
        if (prefs.begin("telemetry", false))
        {
            Settings saved{};
            if (prefs.getBytesLength("settings") == sizeof(saved) &&
                prefs.getBytes("settings", &saved, sizeof(saved)) == sizeof(saved) &&
                saved.version == 1 && memchr(saved.host, 0, sizeof(saved.host)) &&
                validHost(String(saved.host)) && saved.port && saved.interval >= 5 &&
                saved.interval <= 3600)
                settings = saved;
            size_t length = prefs.getBytesLength("boots");
            bool valid = length == 0;
            if (length == sizeof(boots))
                valid = prefs.getBytes("boots", &boots, sizeof(boots)) == sizeof(boots) &&
                        boots.version == 1;
            if (valid)
            {
                boots.version = 1;
                if (boots.boots < UINT32_MAX)
                    ++boots.boots;
                esp_reset_reason_t reason = esp_reset_reason();
                if ((reason == ESP_RST_WDT || reason == ESP_RST_INT_WDT ||
                     reason == ESP_RST_TASK_WDT) &&
                    boots.watchdogs < UINT32_MAX)
                    ++boots.watchdogs;
                countersOk = prefs.putBytes("boots", &boots, sizeof(boots)) == sizeof(boots);
            }
            prefs.end();
        }
        if (!countersOk)
            Serial.println("[METRICS] Contadores persistentes indisponiveis (NVS).");
        WiFi.onEvent([this](WiFiEvent_t, WiFiEventInfo_t) { disconnects.fetch_add(1); },
                     ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
        update();
    }

    void setUdpListener(bool opened) { udpListener = opened; }
    void setMqttConnected(bool connected) { mqttConnected = connected; }
    void recordMqttReceived() { ++mqttReceived; }
    void recordMqttPublish(bool success) { if(success) ++mqttSent; else ++mqttErrors; }
    void recordUdpPacket() { ++udpPackets; }
    void recordUdpReply(bool success) {
        if (success) ++udpReplies;
        else ++udpSendErrors;
    }

    void recordHttp(uint64_t elapsedUs, int status)
    {
        ++requests;
        if (status >= 400 || status <= 0)
            ++errors;
        responseMs = elapsedUs / 1000.0;
    }

    void update()
    {
#if configUSE_TRACE_FACILITY && configGENERATE_RUN_TIME_STATS
        if (millis() - lastCpuSample < 5000UL)
            return;
        lastCpuSample = millis();
        if (uxTaskGetNumberOfTasks() > 64)
        {
            cpuUsage = NAN;
            cpuBaseline = false;
            return;
        }
        configRUN_TIME_COUNTER_TYPE total = 0, idle = 0;
        UBaseType_t count = uxTaskGetSystemState(tasks, 64, &total);
        TaskHandle_t idleTask = xTaskGetIdleTaskHandleForCore(0);
        bool found = false;
        for (UBaseType_t i = 0; i < count; ++i)
            if (tasks[i].xHandle == idleTask)
            {
                idle = tasks[i].ulRunTimeCounter;
                found = true;
                break;
            }
        if (!found)
        {
            cpuUsage = NAN;
            cpuBaseline = false;
            return;
        }
        configRUN_TIME_COUNTER_TYPE delta = total - previousTotal;
        configRUN_TIME_COUNTER_TYPE idleDelta = idle - previousIdle;
        if (cpuBaseline && delta && idleDelta <= delta)
            cpuUsage = 100.0f * (1.0f - static_cast<double>(idleDelta) / delta);
        else
            cpuUsage = NAN;
        previousTotal = total;
        previousIdle = idle;
        cpuBaseline = true;
#endif
    }

    String exposition() const
    {
        String out;
        out.reserve(8000);
        sample(out, "esp_info", "gauge", "1",
               label("chip", ESP.getChipModel()) + ",model=\"M5NanoC6\"");
        sample(out, "esp_firmware_version", "gauge", "1",
               label("version", Config::obterVersaoAutomatica()));
        bool connected = WiFi.status() == WL_CONNECTED;
        if (connected || WiFi.getMode() == WIFI_AP)
            sample(out, "esp_ip_address", "gauge", "1",
                   label("ip", connected ? WiFi.localIP().toString() : WiFi.softAPIP().toString()) +
                       labelSeparator(connected));
        sample(out, "esp_mac_address", "gauge", "1", label("mac", device));
        sample(out, "esp_status_online", "gauge", connected ? "1" : "0");
        sample(out, "esp_uptime_seconds", "gauge", String(esp_timer_get_time() / 1000000.0, 3));
        sample(out, "esp_last_restart_reason", "gauge", "1",
               label("reason", Config::obterMotivoReset()));
        if (countersOk)
        {
            sample(out, "esp_boot_count", "gauge", String(boots.boots));
            sample(out, "esp_reboot_count", "gauge", String(boots.boots ? boots.boots - 1 : 0));
            sample(out, "esp_watchdog_resets", "gauge", String(boots.watchdogs));
        }
        if (connected)
        {
            sample(out, "esp_wifi_rssi_dbm", "gauge", String(WiFi.RSSI()));
            sample(out, "esp_wifi_channel", "gauge", String(WiFi.channel()));
            int8_t power;
            if (esp_wifi_get_max_tx_power(&power) == ESP_OK)
                sample(out, "esp_wifi_tx_power", "gauge", String(power / 4.0f, 2));
        }
        sample(out, "esp_wifi_disconnect_count", "gauge", String(disconnects.load()));
        sample(out, "esp_cpu_frequency_mhz", "gauge", String(ESP.getCpuFreqMHz()));
        if (isfinite(cpuUsage))
            sample(out, "esp_cpu_usage_percent", "gauge", String(cpuUsage, 3));
        sample(out, "esp_task_count", "gauge", String(uxTaskGetNumberOfTasks()));
        sample(out, "esp_free_rtos_tasks", "gauge", String(uxTaskGetNumberOfTasks()));
        sample(out, "esp_heap_free_bytes", "gauge", String(ESP.getFreeHeap()));
        sample(out, "esp_heap_total_bytes", "gauge", String(ESP.getHeapSize()));
        sample(out, "esp_heap_minimum_free_bytes", "gauge", String(ESP.getMinFreeHeap()));
        sample(out, "esp_psram_free_bytes", "gauge", String(ESP.getFreePsram()));
        sample(out, "esp_psram_total_bytes", "gauge", String(ESP.getPsramSize()));
        if (partitionBytes)
            sample(out, "esp_flash_usage_percent", "gauge",
                   String(100.0 * imageBytes / partitionBytes, 3));
        float temperature = temperatureRead();
        if (isfinite(temperature))
            sample(out, "esp_cpu_temperature_celsius", "gauge", String(temperature, 2));
        WeatherService::Estado weather = clima.obterEstado();
        if (weather.disponivel)
        {
            String source = "source=\"open_meteo\",location=\"porto_alegre\"";
            sample(out, "sensor_temperature_celsius", "gauge", String(weather.temperatura, 2),
                   source);
            if (isfinite(weather.umidade) && weather.umidade >= 0 && weather.umidade <= 100)
                sample(out, "sensor_humidity_percent", "gauge", String(weather.umidade, 2), source);
            sample(out, "esp_weather_data_age_seconds", "gauge", String(weather.idade / 1000));
        }
        // Somente GPIOs digitais ja configurados pelo projeto; nao reconfigura ADC/I2C.
        const uint8_t pins[] = {Config::PIN_IR_TX, Config::PIN_LED_AZUL, Config::PIN_RGB_ENABLE};
        out += "# HELP gpio_digital_output Configured GPIO output level.\n# TYPE "
               "gpio_digital_output gauge\n";
        for (uint8_t pin : pins)
            out += "gpio_digital_output{device=\"" + escaped(device) + "\",pin=\"" + String(pin) +
                   "\"} " + String(digitalRead(pin) == HIGH ? 1 : 0) + "\n";
        sample(out, "gpio_digital_input", "gauge",
               String(digitalRead(Config::PIN_BOTAO) == HIGH ? 1 : 0),
               label("pin", String(Config::PIN_BOTAO)));
        sample(out, "application_requests_total", "counter", String(requests));
        sample(out, "esp_udp_listener_up", "gauge", udpListener && connected ? "1" : "0");
        sample(out, "mqtt_connected", "gauge", mqttConnected && connected ? "1" : "0");
        sample(out, "mqtt_messages_sent", "counter", String(mqttSent));
        sample(out, "mqtt_messages_received", "counter", String(mqttReceived));
        sample(out, "mqtt_publish_errors", "counter", String(mqttErrors));
        sample(out, "esp_udp_packets_received_total", "counter", String(udpPackets));
        sample(out, "esp_udp_replies_sent_total", "counter", String(udpReplies));
        sample(out, "esp_udp_send_errors_total", "counter", String(udpSendErrors));
        sample(out, "application_errors_total", "counter", String(errors));
        sample(out, "application_response_time_ms", "gauge", String(responseMs, 3));
        return out;
    }

    String scrapeConfiguration() const
    {
        String ip =
            WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : WiFi.softAPIP().toString();
        return "# Adicione este job em scrape_configs no servidor Prometheus.\n"
               "scrape_configs:\n  - job_name: 'm5nanoc6'\n    scrape_interval: " +
               String(settings.interval) +
               "s\n    metrics_path: /metrics\n    static_configs:\n      - targets: ['" + ip +
               ":" + String(settings.port) + "']\n";
    }

  private:
    static String labelSeparator(bool connected)
    {
        return connected ? ",interface=\"sta\"" : ",interface=\"ap\"";
    }
};

extern MetricsService metrics;
#endif
