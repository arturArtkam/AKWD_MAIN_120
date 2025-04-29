#ifndef SD_EVENT_H_INCLUDED
#define SD_EVENT_H_INCLUDED

#include "../scmRTOS/core/scmRTOS.h"

enum Sd_message
{
    ERR_NULL = 0,
    CARD_INIT_FAIL = 1,
    READEBLOCK_FAIL,
    WRITEBLOCK_FAIL,
    ADDR_OUT_OF_RANGE,
    INCORRECT_CAPACITY,
    STA_CTIMEOUT = 20, /* Command timeout */
    STA_DTIMEOUT /* Data timeout */
};

class sd_event_t : public OS::message<Sd_message>
{
public:
    void signal_from_isr(Sd_message msg)
    {
        Msg = msg;
        send_isr();
    }

    bool check_source(Sd_message msg)
    {
        return Msg == msg;
    }
};

#endif /* SD_EVENT_H_INCLUDED */
