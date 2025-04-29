#ifndef TXRX_SYNC_PULSE_H_INCLUDED
#define TXRX_SYNC_PULSE_H_INCLUDED

template <TIM::TimerNum num, port_t port, uint8_t pinout>
class Pulse_len_timer
{
    typedef TIM::Timer<num> _tim;
    typedef pin<port, pinout, GPIO_Mode_OUT, GPIO_Speed_25MHz, GPIO_OType_PP, GPIO_PuPd_UP> _Pulse_control_pin;

    bool _enabled;
    uint32_t _cnt_ms;
    Pin_settings _pulse_pin_settings;

public:
    ~Pulse_len_timer() = default;
    Pulse_len_timer(uint32_t period_ms) :
        _enabled(false),
        _cnt_ms(0),
        _pulse_pin_settings()
    {
        _tim::EnableClocks();

        _tim::TIMx->PSC = (F_APB1 / 1000 - 1);  //предделитель
        _tim::TIMx->EGR |= TIM_EGR_UG;          //апдейт для прескелера
        _tim::TIMx->SR &= ~TIM_SR_UIF;        //сбрасываем флаг прерывания по переполнению
        _tim::TIMx->ARR = period_ms - 1;

        _tim::UpdateCount(0);
        _tim::UpdateInterrupt::Enable(); //разрешаем прерывание по событию переполнение таймера (ARR)

        NVIC_EnableIRQ(_tim::TIMx_IRQn);
    }

    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wunused-but-set-variable"
    void start()
    {
        if (!_enabled)
        {
            _Pulse_control_pin::save_settings(_pulse_pin_settings);
            _Pulse_control_pin::init();
            _Pulse_control_pin::lo();

            _tim::UpdateCount(0);
            _tim::Enable();

            _enabled = true;
        }
    }
    #pragma GCC diagnostic pop

    void stop()
    {
        _enabled = false;

        _Pulse_control_pin::hi();
        _Pulse_control_pin::restore_settings(_pulse_pin_settings);

        _cnt_ms = _tim::Count();
        _tim::Disable();
    }

    void restart()
    {
        if (_enabled)
        {
            _cnt_ms = _tim::Count();

            _tim::Disable();
            _tim::UpdateCount(0);
            _tim::Enable();
        }
    }

    void isr()
    {
        if (_tim::UpdateInterrupt::Triggered())
        {
            _tim::Disable();
            _tim::UpdateInterrupt::Clear();
            stop();
        }
    }
};

/* импульс для запуска платы излучатель звукового импульса и платы ацп*/
class Txrx_sync_pulse
{
    typedef Uart_pins::dir_pin _Pulse_en_pin;
    typedef Uart_pins::io_pin  _Pulse_pin;

    Pulse_len_timer<TIM::TIM_2, PORTA, 2> _pulse_tim;

public:
    Txrx_sync_pulse() :
        _pulse_tim(10)
    {
//        _pulse_tim.start();
    }

    void start()
    {
        _Pulse_en_pin::hi();
        _pulse_tim.start();
    }

    void timer_isr()
    {
        _pulse_tim.isr();
        _Pulse_en_pin::lo();
    }
};

#endif /* TXRX_SYNC_PULSE_H_INCLUDED */
