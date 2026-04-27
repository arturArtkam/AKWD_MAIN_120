#ifndef SYNC_INPUT_H
#define SYNC_INPUT_H

#include "sync_event.h"
#include "../inc/sb1w_uart.h"
#include "../inc/kadr_tim.h"
#include "../../boards_exchange.h"
#include "../../../Lib/dwt.h"

enum class Sync_source : uint8_t
{
    INPUT_A,
    INPUT_B
};


class Sync_input : public Ipower
{
    typedef Interrupt_pin<PORTA, 0, trigger_t::RISING, GPIO_PuPd_DOWN> Synchro_exti;
    typedef Interrupt_pin<PORTB, 12, trigger_t::RISING, GPIO_PuPd_DOWN> Line_exti;

    Sb1w& _uart_ptr;
    Sync_event& _sync_event;
    Ctimer& _ctimer_lnk;
    uint32_t _start_time;
    uint32_t _sync_period;
    bool _enabled;
    volatile Sync_source _active_input;

public:
    Sync_input(Sb1w& interface, Ctimer& ctimer, Sync_event& sync) :
        _uart_ptr(interface),
        _sync_event(sync),
        _ctimer_lnk(ctimer),
        _start_time(Dwt::cycles()),
        _sync_period(0),
        _enabled(false),
        _active_input(Sync_source::INPUT_A)
    {
        init();
        Dwt::start();
    }

    void pwr_up() override
    {
//        enable();
    }

    void pwr_down() override
    {
//        disable();
    }

    float get_sync_periode()
    {
        return (float)_sync_period / ((float)SystemCoreClock / 1000000.0f);
    }

    void init()
    {
        Synchro_exti::init();
        Line_exti::init();
    }

    void select_input(Sync_source src)
    {
        if (src == Sync_source::INPUT_A)
        {
            Line_exti::deinit();
            Line_exti::disable_irq();
            Synchro_exti::init();
            Synchro_exti::enable_irq();
        }
        else
        {
            Synchro_exti::deinit();
            Synchro_exti::disable_irq();
            Line_exti::init();
            Line_exti::enable_irq();
        }

        _active_input = src;
    }

    void pc1w_sync()
    {
        const static uint8_t query[] = {Exchange_between_boards::CMD_SYNC_START, 0x7E, 0xC4};

        if (_enabled)
        {
            Dwt::delay_us(600);

            _ctimer_lnk.inc();
            _uart_ptr.send_via_dma(query, sizeof(query), true);
            _sync_event.signal_from_isr(Sync_event_src::SYNC_EVENT);
        }
    }

    void enable()
    {
        _enabled = true;
    }

    void disable()
    {
        _enabled = false;
    }

    INLINE void isr()
    {
        const static uint8_t query[] = {Exchange_between_boards::CMD_SYNC_START, 0x7E, 0xC4};

        if (READ_BIT(EXTI->PR, EXTI_Line0))
        {
            EXTI->PR = EXTI_Line0;
            _sync_period = (Dwt::cycles() - _start_time);
            _start_time = Dwt::cycles();

            if (_enabled)
            {
                Dwt::delay_us(600);

                _ctimer_lnk.inc();
                _uart_ptr.send_via_dma(query, sizeof(query), true);
                _sync_event.signal_from_isr(Sync_event_src::SYNC_EVENT);
            }
        }
    }
};

#endif /* SYNC_INPUT_H */
