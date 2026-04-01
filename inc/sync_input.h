#ifndef SYNC_INPUT_H
#define SYNC_INPUT_H

#include "sync_event.h"
#include "../inc/sb1w_uart.h"
#include "../inc/kadr_tim.h"
#include "../../boards_exchange.h"
#include "../../../Lib/dwt.h"

class Sync_input : public Ipower
{
    Sb1w& _uart_ptr;
    Sync_event& _sync_event;
    Ctimer& _ctimer_lnk;
    uint32_t _start_time;
    uint32_t _sync_period;
    bool _enabled;

public:
    Sync_input(Sb1w& interface, Ctimer& ctimer, Sync_event& sync) :
        _uart_ptr(interface),
        _sync_event(sync),
        _ctimer_lnk(ctimer),
        _start_time(Dwt::cycles()),
        _sync_period(0),
        _enabled(false)
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
        pin<PORTA, 0, GPIO_Mode_IN>::init();
        pin<PORTA, 0, GPIO_Mode_IN>::pulldown();

        EXTI_InitTypeDef exti;
        NVIC_InitTypeDef nvic;

        RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);

        exti.EXTI_Line = EXTI_Line0;
        exti.EXTI_Mode = EXTI_Mode_Interrupt;
        exti.EXTI_Trigger = EXTI_Trigger_Rising;
        exti.EXTI_LineCmd = ENABLE;

        EXTI_Init(&exti);

        nvic.NVIC_IRQChannel = EXTI0_IRQn;
        nvic.NVIC_IRQChannelPreemptionPriority = 0x00;
        nvic.NVIC_IRQChannelCmd = ENABLE;

        NVIC_Init(&nvic);
    }

    void deinit()
    {
        EXTI_InitTypeDef exti;
        NVIC_InitTypeDef nvic;

        exti.EXTI_Line = EXTI_Line0;
        exti.EXTI_Mode = EXTI_Mode_Interrupt;
        exti.EXTI_Trigger = EXTI_Trigger_Rising;
        exti.EXTI_LineCmd = DISABLE;

        nvic.NVIC_IRQChannel = EXTI0_IRQn;
        nvic.NVIC_IRQChannelPreemptionPriority = 0x00;
        nvic.NVIC_IRQChannelCmd = DISABLE;

        EXTI_Init(&exti);
        NVIC_Init(&nvic);
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
