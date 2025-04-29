#ifndef USART_EVENT_H_INCLUDED
#define USART_EVENT_H_INCLUDED

enum Uart_event_id
{
    EMPTY = 0,
    RX_EVENT = 1,
    TX_EVENT
};

class Usart_event : public OS::message<Uart_event_id>
{
public:
    INLINE void signal_from_isr(Uart_event_id msg)
    {
        Msg = msg;
        send_isr();
    }

    INLINE bool check_source(Uart_event_id msg)
    {
        return Msg == msg;
    }

    INLINE void clr()
    {
        reset();
        Msg = EMPTY;
    }
};

#endif /* USART_EVENT_H_INCLUDED */
