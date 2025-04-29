#include "../inc/akwd.h"

namespace akwd
{

void main_thread()
{
    timer_or_ext_sync.wait();
    operating_time_proc();

    if (fsm.get() > Fsm::APP_IDLE && fsm.get() < Fsm::APP_WORK)
    {
        leds.red_xor();
    }

    switch (fsm.get())
    {
        case Fsm::APP_IDLE:
            break;

        case Fsm::APP_SET_TIME:
            if (std::abs(c_timer.get()) > erase_timeout())
            {
                if (sd_card.init(true) == Sd_message::ERR_NULL)
                {
                    sd_card.full_erase();
                    ext_trigger.disable();
                    fsm.set(Fsm::APP_CLEAR_FLASH);
                }
                else
                {
                    fsm.set(Fsm::APP_ERR);
                }
            }
            else
            {
                fsm.set(Fsm::APP_ERR);
            }

            break;

        case Fsm::APP_CLEAR_FLASH:
            //проверка статуса таймера стирания sd
            if (--erase_timeout() <= 0)
            {
                fsm.set(Fsm::APP_DELAY);
            }

            break;

        case Fsm::APP_DELAY:
            if (c_timer.get() == -10)
            {
                pwr.turn_on(false);
            }
            else if (c_timer.get() == -5)
            {
//                if(akwd.init_card().err_flg == 0)
//                {
//                    for (auto i = 0; i < 6; i++)
//                    {
//                        akwd.leds.green_xor();
//                        sleep(50u);
//                    }
//                }
            }
            else if (c_timer.get() == 0)
            {
                fsm.set(Fsm::APP_WORK);
                c_timer.disable();
                ext_trigger.enable();
            }
            else
            {
                ///__WFI();
            }

            break;

        case Fsm::APP_WORK:
//            if (timer_or_ext_sync.check_source(Sync_event_src::TIMER_EVENT))
//            {
//                c_timer.disable();
//                ext_trigger.enable();
//            }
//            else if (timer_or_ext_sync.check_source(Sync_event_src::SYNC_EVENT))
//            {
                pwr.turn_on(false);
//                c_timer.inc();

//                workdata._vault.automat = fsm.get_corrected(true);
//                workdata._vault.Time = c_timer.get();
//                fill_datastruct(&workdata._vault);
//
//                sd_card.write<sizeof(DataStructR_t)>(&workdata._vault);
//            }

            break;

        default:
            ;
            break;
    } /* switch (fsm.get()) */
} /* void main_thread() */

} /* namespace akwd */
