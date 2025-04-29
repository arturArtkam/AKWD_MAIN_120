#ifndef PWR_H_INCLUDED
#define PWR_H_INCLUDED

#include <initializer_list>
#include "ipower.h"
#include "../inc/observer.h"
#include "../../../Lib/pin.h"
#include "../../../Lib/stm32_tim.h"

template <TIM::TimerNum num>
class Ptim
{
    typedef TIM::Timer<num> _Tim;

    bool _enabled;
    uint32_t _cnt_ms;

public:
    ~Ptim() = default;
    Ptim(uint32_t period_ms) :
        _enabled(false),
        _cnt_ms(0)
    {
        _Tim::EnableClocks();

        _Tim::TIMx->PSC = (F_APB1 / 1000 - 1);  //предделитель
        _Tim::TIMx->EGR |= TIM_EGR_UG;          //апдейт для прескелера
        _Tim::TIMx->SR &= ~TIM_SR_UIF;        //сбрасываем флаг прерывания по переполнению
        _Tim::TIMx->ARR = period_ms - 1;

        _Tim::UpdateCount(0);
        _Tim::UpdateInterrupt::Enable(); //разрешаем прерывание по событию переполнение таймера (ARR)

        NVIC_EnableIRQ(_Tim::TIMx_IRQn);
    }

    void start()
    {
        if (!_enabled)
        {
            _Tim::UpdateCount(0);
            _Tim::Enable();

            _enabled = true;
        }
    }

    void stop()
    {
        _enabled = false;

        _cnt_ms = _Tim::Count();
        _Tim::Disable();
    }

    void restart()
    {
        if (_enabled)
        {
            _cnt_ms = _Tim::Count();

            _Tim::Disable();
            _Tim::UpdateCount(0);
            _Tim::Enable();
        }
    }

    void isr()
    {
        if (_Tim::UpdateInterrupt::Triggered())
        {
            _Tim::Disable();
            _Tim::UpdateInterrupt::Clear();
            stop();
        }
    }
};

class Power_key : public Ipower
{
    typedef pin<PORTA, 3, GPIO_Mode_OUT, GPIO_Speed_2MHz> _Va_pin;

public:
    Power_key()
    {
        _Va_pin::init();
    }

    void pwr_up() override
    {
        _Va_pin::hi();
    }

    void pwr_down() override
    {
        _Va_pin::lo();
    }
};

class Pwr : public Observable<Ipower, 8>
{
    bool _power_on;
    Ptim<TIM::TIM_4> _pwr_timer;

public:
    Pwr(std::initializer_list<Ipower*> observers = {}) :
        _power_on(false),
        _pwr_timer(10000)
    {
        for (auto observer : observers)
        {
            if (!register_item(observer))
            {
                // Если не удаётся добавить больше наблюдателей, можно обработать ситуацию:
                // Например, выбросить исключение или вывести сообщение об ошибке
                break;
            }
        }
//    #pragma GCC diagnostic push
//    #pragma GCC diagnostic ignored "-Wunused-but-set-variable"
//
//        for (auto each_ptr : _observers)
//            each_ptr = nullptr;
//
//    #pragma GCC diagnostic pop
    }

    const bool& check_flag()
    {
        return _power_on;
    }

    void shutdown()
    {
        if (_power_on)
        {
            for (auto each_dev : _observers)
            {
                if (each_dev != nullptr)
                    each_dev->pwr_down();
            }

            _power_on = false;
        }

        //_Pwrimer.stop();
    }

    void turn_on(bool use_timer)
    {
        if (!_power_on)
        {
            for (auto each_dev : _observers)
            {
                if (each_dev != nullptr)
                    each_dev->pwr_up();
            }

            _power_on = true;
        }

        if (use_timer == true)
        {
            _pwr_timer.start();
            _pwr_timer.restart();
        }
    }

    void timer_isr()
    {
        _pwr_timer.isr();
        this->shutdown();
    }
}; /* class Pwr */

#endif /* PWR_H_INCLUDED */
