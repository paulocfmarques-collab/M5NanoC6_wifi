#ifndef LCD_BUTTON_H
#define LCD_BUTTON_H
#include <stdint.h>

class LCDButton {
    bool raw=false, stable=false, consumed=false;
    uint32_t changedAt=0, pressedAt=0;
public:
    enum Action { None, Wake, NextPage, TurnOff };
    Action update(bool pressed,bool screenOn,uint32_t now) {
        if(pressed!=raw){raw=pressed;changedAt=now;}
        if(raw!=stable&&uint32_t(now-changedAt)>=35) {
            stable=raw;
            if(stable) {
                pressedAt=now;consumed=false;
                if(!screenOn){consumed=true;return Wake;}
            } else if(!consumed&&screenOn) {
                consumed=true;return uint32_t(changedAt-pressedAt)>=3000?TurnOff:NextPage;
            }
        }
        if(stable&&raw&&!consumed&&screenOn&&uint32_t(now-pressedAt)>=3000) {
            consumed=true;return TurnOff;
        }
        return None;
    }
};
#endif
