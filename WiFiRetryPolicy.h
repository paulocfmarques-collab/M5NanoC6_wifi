#ifndef WIFI_RETRY_POLICY_H
#define WIFI_RETRY_POLICY_H
#include <stdint.h>
class WiFiRetryPolicy {
    bool saved=false, waiting=false;
    uint32_t scheduledAt=0;
public:
    static constexpr uint32_t interval=600000UL;
    void configure(bool hasSaved){saved=hasSaved;waiting=false;}
    bool hasSaved()const{return saved;}
    void schedule(uint32_t now){scheduledAt=now;waiting=saved;}
    void connected(){waiting=false;}
    bool due(uint32_t now)const{return waiting&&uint32_t(now-scheduledAt)>=interval;}
    uint32_t remaining(uint32_t now)const {
        uint32_t elapsed=uint32_t(now-scheduledAt);
        return waiting&&elapsed<interval?interval-elapsed:0;
    }
};
#endif
