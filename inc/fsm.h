#ifndef FSM_H_INCLUDED
#define FSM_H_INCLUDED

#include "alldata.h"
#include "../../../Lib/bkp_domain.h"

class Fsm : public bkp_mem_t<uint8_t>
{
public:
    Fsm() = default;
    ~Fsm() = default;

    uint8_t get_corrected(bool set_control_flag = false)
    {
        if (*bkp::addr_ptr == APP_IDLE)
        {
            if (set_control_flag == true)
            {
                return 4 | 0x80;
            }
            else
            {
                return 4;
            }
        }
        else if (*bkp::addr_ptr <= APP_WORK)
        {
            if (set_control_flag == true)
            {
                return (*bkp::addr_ptr - 1) | 0x80;
            }
            else
            {
                return (*bkp::addr_ptr) - 1;
            }
        }
        else
        {
            if (set_control_flag == true)
            {
                return (*bkp::addr_ptr) | 0x80;
            }
            else
            {
                return *bkp::addr_ptr;
            }
        }
    }

    const char* to_str()
    {
        static const char* const buffer[] = {
            "IDLE",
            "SET_TIME",
            "CLEAR_FLASH",
            "DELAY",
            "WORK",
            "ERR"
        };

        return buffer[*bkp::addr_ptr];
    }

    enum
    {
        APP_IDLE,
        APP_SET_TIME, /* установка задержки (этот флаг ставится на 1 кадр */
        APP_CLEAR_FLASH, /* идет стирание памяти */
        APP_DELAY, /* прибор на задержке */
        APP_WORK, /* рабочий режим */
        APP_ERR
    };
};

#endif // FSM_H_INCLUDED
