#ifndef KADR_TIM_H_INCLUDED
#define KADR_TIM_H_INCLUDED

#include "sync_event.h"
#include "../../../Lib/bkp_domain.h"

class Ctimer : public bkp_mem_t<int32_t>
{
private:
    constexpr static uint16_t _presc = F_APB1 / 1000000;
    uint32_t _period_us;
    Sync_event& _sync_event;
    bool _tim_enabled;

public:
    Ctimer(uint32_t period_us, Sync_event& sync) :
        _period_us(period_us),
        _sync_event(sync),
        _tim_enabled(false)
    {

    }
    ~Ctimer() = default;

    INLINE void init();
    INLINE void isr();

    void enable()
    {
        SET_BIT(TIM5->CR1, TIM_CR1_CEN);
        _tim_enabled = true;
    }

    void disable()
    {
        CLEAR_BIT(TIM5->CR1, TIM_CR1_CEN);
        _tim_enabled = false;
    }

    void inc()
    {
        if (!_tim_enabled)
            (*bkp::addr_ptr)++;
    }
};

void Ctimer::init()
{
    RCC->APB1ENR |= RCC_APB1ENR_TIM5EN;

    TIM5->PSC = (_presc - 1);
    TIM5->ARR = (_period_us - 1); //Кадр - 2.097.152us

    //апдейт для прескелера
    SET_BIT(TIM5->EGR, TIM_EGR_UG);
    //
    CLEAR_BIT(TIM5->SR, TIM_SR_UIF);

    // разрешаем прерывание по событию
    SET_BIT(TIM5->DIER, TIM_DIER_UIE);
    // разрешаем счет таймера
    enable();

    NVIC_SetPriority(TIM5_IRQn, 0);
    NVIC_EnableIRQ(TIM5_IRQn);
}

void Ctimer::isr()
{
    if ((TIM5->SR & TIM_SR_UIF) == TIM_SR_UIF)
    {
        TIM5->SR &= uint16_t(~TIM_SR_UIF);
        (*bkp::addr_ptr)++;
//        _sync_event.signal_isr();
        _sync_event.signal_from_isr(Sync_event_src::TIMER_EVENT);
    }
}

#endif // KADR_TIM_H_INCLUDED
