#ifndef HOME_ASSISTANT_SERVICE_H
#define HOME_ASSISTANT_SERVICE_H
#include <WiFi.h>
#include <PubSubClient.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <esp_mac.h>
#include <functional>
#include "MetricsService.h"
#include "LCDService.h"
#include "HardwareController.h"

class HomeAssistantService {
public:
    struct Settings {
        uint32_t version;
        bool enabled;
        char host[254], user[65], password[129];
        uint16_t port, interval;
    };
private:
    Settings settings{};
    WiFiClient socket;
    PubSubClient client{socket};
    String id, base;
    std::function<void(String)> execute;
    bool ready=false, attempted=false, publishPending=true;
    unsigned long lastAttempt=0, lastState=0, lastDiscovery=0;
    int discoveryIndex=-1;

    struct Entity { const char *component, *key, *name, *unit, *deviceClass; };
    static const Entity* entities() {
        static const Entity list[] = {
            {"switch","lcd","Tela LCD","",""},
            {"switch","led","LED RGB","",""},
            {"button","ntp","Sincronizar NTP","",""},
            {"button","ir","Pulso infravermelho","",""},
            {"sensor","cpu_temperature","Temperatura CPU","°C","temperature"},
            {"sensor","heap","Memoria livre","B","data_size"},
            {"sensor","uptime","Tempo ligado","s","duration"},
            {"sensor","rssi","Sinal Wi-Fi","dBm","signal_strength"},
            {"sensor","temperature","Temperatura Porto Alegre","°C","temperature"},
            {"sensor","humidity","Umidade Porto Alegre","%","humidity"},
            {"sensor","maximum","Maxima Porto Alegre","°C","temperature"},
            {"sensor","minimum","Minima Porto Alegre","°C","temperature"}
        };
        return list;
    }

    bool publish(const String& topic, const String& payload, bool retained) {
        bool ok=client.publish(topic.c_str(),payload.c_str(),retained);
        metrics.recordMqttPublish(ok);
        return ok;
    }

    bool discovery(unsigned index) {
        const Entity& e=entities()[index];
        JsonDocument doc;
        doc["name"]=e.name;
        doc["unique_id"]=id+"_"+e.key;
        doc["availability_topic"]=base+"/availability";
        doc["payload_available"]="online";doc["payload_not_available"]="offline";
        JsonObject device=doc["device"].to<JsonObject>();
        device["identifiers"].to<JsonArray>().add(id);
        device["name"]="M5NanoC6";device["manufacturer"]="M5Stack";device["model"]="NanoC6";
        device["sw_version"]=Config::obterVersaoAutomatica();
        device["configuration_url"]="http://"+WiFi.localIP().toString()+"/homeassistant";
        doc["origin"]["name"]="M5NanoC6 firmware";
        doc["origin"]["sw_version"]=Config::obterVersaoAutomatica();
        String component=e.component;
        if(component=="switch") {
            doc["command_topic"]=base+"/"+e.key+"/set";
            doc["state_topic"]=base+"/state";
            doc["value_template"]="{{ value_json."+String(e.key)+" }}";
            doc["payload_on"]="ON";doc["payload_off"]="OFF";
            doc["retain"]=false;
        } else if(component=="button") {
            doc["command_topic"]=base+"/"+e.key+"/set";
            doc["payload_press"]="PRESS";doc["retain"]=false;
        } else {
            doc["state_topic"]=base+"/state";
            doc["value_template"]="{{ value_json."+String(e.key)+" }}";
            doc["unit_of_measurement"]=e.unit;
            doc["device_class"]=e.deviceClass;
            doc["state_class"]="measurement";
            if(index>=8) {
                JsonArray availability=doc["availability"].to<JsonArray>();
                JsonObject online=availability.add<JsonObject>();
                online["topic"]=base+"/availability";
                JsonObject weather=availability.add<JsonObject>();
                weather["topic"]=base+"/state";
                weather["value_template"]="{{ 'online' if value_json."+String(e.key)+" is number else 'offline' }}";
                doc.remove("availability_topic");doc["availability_mode"]="all";
            }
        }
        String payload;serializeJson(doc,payload);
        return publish("homeassistant/"+component+"/"+id+"/"+e.key+"/config",payload,true);
    }

    bool state() {
        JsonDocument doc;
        if(lcd.estaAtivo()) doc["lcd"]=lcd.estaLigado()?"ON":"OFF";
        else doc["lcd"]=nullptr;
        doc["led"]=hardware.obterBrilho() && (hardware.obterR()||hardware.obterG()||hardware.obterB())?"ON":"OFF";
        float temperature=temperatureRead();
        if(isfinite(temperature))doc["cpu_temperature"]=temperature;else doc["cpu_temperature"]=nullptr;
        doc["heap"]=ESP.getFreeHeap();doc["uptime"]=esp_timer_get_time()/1000000;
        doc["rssi"]=WiFi.RSSI();
        auto weather=clima.obterEstado();
        const char* keys[]={"temperature","humidity","maximum","minimum"};
        float values[]={weather.temperatura,weather.umidade,weather.maxima,weather.minima};
        for(unsigned i=0;i<4;++i) {
            if(weather.disponivel&&isfinite(values[i]))doc[keys[i]]=values[i];
            else doc[keys[i]]=nullptr;
        }
        String payload;serializeJson(doc,payload);
        return publish(base+"/state",payload,false);
    }

    void receive(char* topic, uint8_t* payload, unsigned length) {
        metrics.recordMqttReceived();
        if(length>16)return;
        String value;for(unsigned i=0;i<length;++i)value+=char(payload[i]);
        if(String(topic)=="homeassistant/status") {
            if(value=="online"){discoveryIndex=0;publishPending=true;}
            return;
        }
        String command;
        if(String(topic)==base+"/lcd/set"&&(value=="ON"||value=="OFF"))command="LCD_"+value;
        else if(String(topic)==base+"/led/set"&&(value=="ON"||value=="OFF"))command="LED_"+value;
        else if(String(topic)==base+"/ntp/set"&&value=="PRESS")command="NTP_SYNC";
        else if(String(topic)==base+"/ir/set"&&value=="PRESS")command="IR_TX";
        if(command.length()&&execute){execute(command);publishPending=true;}
    }

    void disconnect() {
        if(client.connected()){publish(base+"/availability","offline",true);client.disconnect();}
        socket.stop();metrics.setMqttConnected(false);
    }

public:
    const Settings& configuration()const{return settings;}
    bool connected(){return client.connected();}
    int connectionState(){return client.state();}
    const String& deviceId()const{return id;}
    bool requestDiscovery() {
        if(!ready||!settings.enabled)return false;
        discoveryIndex=0;publishPending=true;
        if(!client.connected())attempted=false;
        return true;
    }
    String status() {
        bool online=client.connected();
        String result=String("HA_STATUS: ")+(settings.enabled?"ATIVADO":"DESATIVADO");
        result+=String("\nWiFi: ")+(WiFi.status()==WL_CONNECTED?"CONECTADO":"DESCONECTADO");
        result+=String("\nMQTT: ")+(online?"CONECTADO":"DESCONECTADO")+
                " (codigo "+String(client.state())+")";
        result+="\nBroker: "+String(settings.host)+":"+String(settings.port);
        result+="\nDispositivo: "+id+"\nIntervalo: "+String(settings.interval)+" s";
        result+="\nDiscovery: ";
        if(!settings.enabled||!ready)result+="INATIVO";
        else if(discoveryIndex>=0)result+=(online?String("ENVIANDO "):String("AGUARDANDO MQTT "))+
                                        String(discoveryIndex)+"/12";
        else result+="SEM ENVIO PENDENTE";
        return result+"\n";
    }
    bool save(bool enabled,const String& host,uint16_t port,const String& user,
              const String& password,uint16_t interval) {
        if((enabled||host.length())&&!MetricsService::validHost(host))return false;
        if(!port||user.length()>64||password.length()>128||interval<5||interval>3600)return false;
        Settings next{};next.version=1;next.enabled=enabled;next.port=port;next.interval=interval;
        host.toCharArray(next.host,sizeof(next.host));user.toCharArray(next.user,sizeof(next.user));
        password.toCharArray(next.password,sizeof(next.password));
        Preferences prefs;if(!prefs.begin("m5ha",false))return false;
        bool ok=prefs.putBytes("settings",&next,sizeof(next))==sizeof(next);prefs.end();
        if(!ok)return false;
        disconnect();settings=next;client.setServer(settings.host,settings.port);
        attempted=false;return true;
    }

    void begin(std::function<void(String)> handler) {
        execute=handler;settings.version=1;settings.port=1883;settings.interval=15;
        Preferences prefs;
        if(prefs.begin("m5ha",true)) {
            Settings saved{};
            if(prefs.getBytesLength("settings")==sizeof(saved)&&prefs.getBytes("settings",&saved,sizeof(saved))==sizeof(saved)
                &&saved.version==1&&memchr(saved.host,0,sizeof(saved.host))&&memchr(saved.user,0,sizeof(saved.user))
                &&memchr(saved.password,0,sizeof(saved.password))&&saved.port&&saved.interval>=5&&saved.interval<=3600
                &&(!saved.enabled||MetricsService::validHost(String(saved.host))))settings=saved;
            prefs.end();
        }
        uint8_t mac[6];esp_read_mac(mac,ESP_MAC_WIFI_STA);char suffix[13];
        snprintf(suffix,sizeof(suffix),"%02x%02x%02x%02x%02x%02x",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
        id="m5nanoc6_"+String(suffix);base=id;
        ready=client.setBufferSize(2048);client.setSocketTimeout(1);client.setKeepAlive(30);
        socket.setConnectionTimeout(1000);
        client.setServer(settings.host,settings.port);
        client.setCallback([this](char* t,uint8_t* p,unsigned n){receive(t,p,n);});
    }

    void update() {
        if(!ready||!settings.enabled||WiFi.status()!=WL_CONNECTED) {
            if(client.connected())disconnect();
            metrics.setMqttConnected(false);return;
        }
        unsigned long now=millis();
        if(!client.connected()) {
            metrics.setMqttConnected(false);
            if(attempted&&now-lastAttempt<30000UL)return;
            attempted=true;lastAttempt=now;
            const char* user=settings.user[0]?settings.user:nullptr;
            const char* password=settings.user[0]?settings.password:nullptr;
            if(!client.connect(id.c_str(),user,password,(base+"/availability").c_str(),1,true,"offline"))return;
            if(!client.subscribe((base+"/+/set").c_str())||!client.subscribe("homeassistant/status")) {
                disconnect();return;
            }
            if(!publish(base+"/availability","online",true)){disconnect();return;}
            discoveryIndex=0;publishPending=true;lastDiscovery=now-100;
        }
        client.loop();metrics.setMqttConnected(client.connected());
        if(!client.connected())return;
        if(discoveryIndex>=0&&now-lastDiscovery>=100UL) {
            lastDiscovery=now;
            if(discovery(discoveryIndex)&&++discoveryIndex==12)discoveryIndex=-1;
        }
        if(publishPending||now-lastState>=uint32_t(settings.interval)*1000UL) {
            lastState=now;publishPending=false;state();
        }
    }
};
extern HomeAssistantService homeassistant;
#endif
