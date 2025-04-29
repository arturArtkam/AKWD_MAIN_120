#ifndef AKWD_H_INCLUDED
#define AKWD_H_INCLUDED

#include <float.h>
#include <math.h>
#include <cstdint>
#include <cstdlib>
#include <type_traits>

#include "stm32f4xx.h"
#include "../scmRTOS/core/scmRTOS.h"

#include "../../../Lib/atomic.h"
#include "../Leds/led.h"
#include "../pwr/pwr.h"
#include "../sensors/sub_boards.h"
#include "../sensors/iic_sw.h"
#include "../sensors/ltc2944_ic.h"
#include "../sensors/axel.h"
#include "../sensors/vcc_control.h"
#include "../fram/vault_fram.h"
#include "pc1w_uart.h"
#include "sd.h"
#include "fsm.h"
#include "kadr_tim.h"
#include "op_time.h"
#include "sync_input.h"
#include "usb_vbus.h"

namespace akwd
{

typedef struct : bkp_mem_t<int32_t>
{
    int32_t& operator()()
    {
        return *addr_ptr;
    }
} Erase_tmout;

typedef Pwr Pwr_man;

extern Leds leds;
extern Pwr_man pwr;
extern Pc1w pc_uart;
extern Sd_disk sd_card;
extern Fram_vault eeprom;

extern Sb1w sb1w;
extern Adcboard adc_board_1;
extern Adcboard adc_board_2;

extern Fsm fsm;
extern Ctimer c_timer;
extern Sync_event timer_or_ext_sync;
extern Sync_input ext_trigger;
extern Cycle_time_counter op_time;
extern Erase_tmout erase_timeout;

extern All_data workdata;

void main_thread();
void uart_thread();
void sd_thread();
void usbvbus_thread();
void fill_datastruct(DataStructW_t* );

static INLINE void operating_time_proc()
{
    if (fsm.get() == Fsm::APP_WORK || pwr.check_flag())
    {
        op_time.inc_work();
    }
    else
    {
        op_time.inc_idle();
    }
}

} /* namespace akwd */

#endif // AKWD_H_INCLUDED
