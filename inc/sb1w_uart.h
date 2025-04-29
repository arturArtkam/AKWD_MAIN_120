#ifndef SB1W_UART_H
#define SB1W_UART_H

#include "../uart_dma_test/stm32_dma_f4xx.h"
#include "../uart_dma_test/stm32_uart_f4xx.h"
#include "usart_event.h"
#include "ipower.h"

template<Uart_num uart_n, DmaChannelNum rx_ch, DmaChannelNum tx_ch, typename Pins>
class Sb_uart
{
private:
    volatile bool _transmit_only;

protected:
    typedef uart_x<uart_n, typename Pins::Io_pin> _Uart;
    typedef dma_stream<rx_ch> _Rx_dma;
    typedef dma_stream<tx_ch> _Tx_dma;

public:
    Usart_event event;

public:
    Sb_uart() :
        _transmit_only(false),
        event()
    {}

    void init(uint32_t periph_clock, uint32_t baud_rate);
    void send_via_dma(uint8_t const* buf, uint32_t len, bool tx_only);
    void reciever_dma_config();
    void transmitter_dma_config();
    INLINE void rx_dma_isr();
    INLINE void tx_dma_isr();
    INLINE void uart_isr();

    void set_rx_pointer(uint8_t* rx_ptr, uint32_t size)
    {
        _Rx_dma::stream->NDTR = size;
        _Rx_dma::stream->M0AR = reinterpret_cast<uint32_t>(rx_ptr);
    }

    void disable_reciever()
    {
        CLEAR_BIT(_Uart::USARTn->CR1, USART_CR1_RE);
        CLEAR_BIT(_Uart::USARTn->CR3, USART_CR3_DMAR);
        _Rx_dma::disable();
    }
};

template<Uart_num uart_n, DmaChannelNum rx_ch, DmaChannelNum tx_ch, typename Pins>
void Sb_uart<uart_n, rx_ch, tx_ch, Pins>::init(uint32_t periph_clock, uint32_t baud_rate)
{
    Pins::init();

    _Uart::enable_clocks();
    _Rx_dma::enable_clocks();
    _Tx_dma::enable_clocks();

    set_baud(_Uart::USARTn, periph_clock, baud_rate);

    SET_BIT(_Uart::USARTn->CR3, USART_CR3_HDSEL);
    SET_BIT(_Uart::USARTn->CR1, USART_CR1_IDLEIE);
    SET_BIT(_Uart::USARTn->CR1, USART_CR1_UE);

    reciever_dma_config();
    transmitter_dma_config();

    NVIC_EnableIRQ(_Uart::USART_IRQn);
    NVIC_EnableIRQ(_Rx_dma::DMAChannel_IRQn);
    NVIC_EnableIRQ(_Tx_dma::DMAChannel_IRQn);
}

template<Uart_num uart_n, DmaChannelNum rx_ch, DmaChannelNum tx_ch, typename Pins>
void Sb_uart<uart_n, rx_ch, tx_ch, Pins>::reciever_dma_config()
{
    /* 4 канал */
    SET_BIT(_Rx_dma::stream->CR, DMA_SxCR_CHSEL_2);
    SET_BIT(_Rx_dma::stream->CR, DMA_SxCR_MINC);
    /* разрешаем прерывание по окончанию передачи */
    SET_BIT(_Rx_dma::stream->CR, DMA_SxCR_TCIE);
//        SET_BIT(_Rx_dma::stream->CR, DMA_SxCR_TEIE);

    _Rx_dma::stream->PAR = reinterpret_cast<uint32_t>(&(_Uart::USARTn->DR));
}

template<Uart_num uart_n, DmaChannelNum rx_ch, DmaChannelNum tx_ch, typename Pins>
void Sb_uart<uart_n, rx_ch, tx_ch, Pins>::transmitter_dma_config()
{
    /* выбираем 4 канал */
    SET_BIT(_Tx_dma::stream->CR, DMA_SxCR_CHSEL_2);

    /* инкремент указателя на память */
    SET_BIT(_Tx_dma::stream->CR, DMA_SxCR_MINC);

    /* направление из памяти в периферию */
    SET_BIT(_Tx_dma::stream->CR, DMA_SxCR_DIR_0);

    /* разрешаем прерывание по окончанию передачи */
    SET_BIT(_Tx_dma::stream->CR, DMA_SxCR_TCIE);

    /* адрес периферии */
    _Tx_dma::stream->PAR = reinterpret_cast<uint32_t>(&_Uart::USARTn->DR);
}

template<Uart_num uart_n, DmaChannelNum rx_ch, DmaChannelNum tx_ch, typename Pins>
void Sb_uart<uart_n, rx_ch, tx_ch, Pins>::send_via_dma(uint8_t const* buf, uint32_t len, bool tx_only)
{
    Pins::Dir_pin::hi();
    _transmit_only = tx_only;
    event.clr();

    _Tx_dma::stream->NDTR = len;
    _Tx_dma::stream->M0AR = (uint32_t)buf;

    /* разрешаеем DMA для USATR на передачу */
    BIT_BAND_PER(_Uart::USARTn->CR3, USART_CR3_DMAT) = 1;
    /* Вкл передатчик */
    BIT_BAND_PER(_Uart::USARTn->CR1, USART_CR1_TE) = 1;
    /* передаем... */
    _Tx_dma::enable();
}

template<Uart_num uart_n, DmaChannelNum rx_ch, DmaChannelNum tx_ch, typename Pins>
void Sb_uart<uart_n, rx_ch, tx_ch, Pins>::uart_isr()
{
    uint16_t sr = _Uart::USARTn->SR;

    if (READ_BIT(sr, USART_SR_IDLE) == USART_SR_IDLE)
    {
        _Uart::USARTn->DR; /* Последовательное чтение регистров SR и DR сбрасывает флаг IDLE */
//         /* запрещаем DMAR в UART */
//        SET_BIT(Uart::USARTn->CR3, USART_CR3_DMAR);
//        /* Выключение приемника, запрещение прерываний по IDLE флагу*/
//        CLEAR_BIT(Uart::USARTn->CR1, USART_CR1_IDLEIE | USART_CR1_RE);
    }
    else if (READ_BIT(sr, USART_SR_TC) == USART_SR_TC)
    {
        CLEAR_BIT(_Uart::USARTn->SR, USART_SR_TC);

//        /* запрещаем DMAT в UART */
//        BIT_BAND_PER(_Uart::USARTn->CR3, USART_CR3_DMAT) = 0;

        /* выкл передатчика, запрещение прерываний по завершению передачи */
        CLEAR_BIT(_Uart::USARTn->CR1, (USART_CR1_TCIE | USART_CR1_TE));

        Pins::Dir_pin::lo();

        if (_transmit_only)
        {
            event.signal_from_isr(Uart_event_id::TX_EVENT);
        }
        else
        {
            SET_BIT(_Uart::USARTn->CR1, USART_CR1_RE);
            SET_BIT(_Uart::USARTn->CR3, USART_CR3_DMAR);
            _Rx_dma::enable();
        }
    }
    else
    {
        _Uart::USARTn->SR;
        _Uart::USARTn->DR;
    }
}

template<Uart_num uart_n, DmaChannelNum rx_ch, DmaChannelNum tx_ch, typename Pins>
void Sb_uart<uart_n, rx_ch, tx_ch, Pins>::rx_dma_isr()
{
//    if (READ_BIT(_Rx_dma::DMAn->ISR[_Rx_dma::LO_HI_OFFSET], _Rx_dma::DMA_MASK_TEIF))
//    {
//        WRITE_REG(_Rx_dma::DMAn->IFCR[_Rx_dma::LO_HI_OFFSET], _Rx_dma::DMA_MASK_CTEIF);
//
////        CLEAR_BIT(_Uart::USARTn->CR3, (USART_CR3_DMAR | USART_CR3_EIE));
////
////        _Rx_dma::disable();
//    }
//    else
//    #pragma message("Отладить драйвер до конца, не понятно почему вылетает ошибка EIF!")
    if (READ_BIT(_Rx_dma::DMAn->ISR[_Rx_dma::LO_HI_OFFSET], _Rx_dma::DMA_MASK_TCIF))
    {
        WRITE_REG(_Rx_dma::DMAn->IFCR[_Rx_dma::LO_HI_OFFSET], _Rx_dma::DMA_MASK_CTCIF);
        WRITE_REG(_Rx_dma::DMAn->IFCR[_Rx_dma::LO_HI_OFFSET], _Rx_dma::DMA_MASK_CTEIF);

        CLEAR_BIT(_Uart::USARTn->CR3, USART_CR3_DMAR);
        CLEAR_BIT(_Uart::USARTn->CR1, USART_CR1_RE);

        if ((_Rx_dma::stream->NDTR == 0) && (!_transmit_only))
        {
            event.signal_from_isr(Uart_event_id::RX_EVENT);
        }
    }
    else
    {
        __NOP();
    }
}

template<Uart_num uart_n, DmaChannelNum rx_ch, DmaChannelNum tx_ch, typename Pins>
void Sb_uart<uart_n, rx_ch, tx_ch, Pins>::tx_dma_isr()
{
    if (READ_BIT(_Tx_dma::DMAn->ISR[_Tx_dma::LO_HI_OFFSET], _Tx_dma::DMA_MASK_TEIF))
    {
        WRITE_REG(_Tx_dma::DMAn->IFCR[_Tx_dma::LO_HI_OFFSET], _Tx_dma::DMA_MASK_CTEIF);

        CLEAR_BIT(_Uart::USARTn->CR3, USART_CR3_DMAT);
        CLEAR_BIT(_Tx_dma::stream->CR, DMA_SxCR_EN);
    }
    else if (READ_BIT(_Tx_dma::DMAn->ISR[_Tx_dma::LO_HI_OFFSET], _Tx_dma::DMA_MASK_TCIF))
    {
        WRITE_REG(_Tx_dma::DMAn->IFCR[_Tx_dma::LO_HI_OFFSET], _Tx_dma::DMA_MASK_CTCIF);

        CLEAR_BIT(_Uart::USARTn->CR3, USART_CR3_DMAT);
        SET_BIT(_Uart::USARTn->CR1, USART_CR1_TCIE);
    }
}

struct Uart_pins
{
    typedef af_pin<PORTA, 2, GPIO_AF_USART2> Io_pin;
    typedef pin<PORTA, 4, GPIO_Mode_OUT, GPIO_Speed_100MHz> Dir_pin;

    static void init()
    {
        Io_pin::init();
        Io_pin::pullup();
        Dir_pin::init();
    }
};

class Sb1w final : public Sb_uart<UART2, DMA1_CH5, DMA1_CH6, Uart_pins>, public Ipower
{
public:
    Sb1w()
    {
        //init();
    }
    ~Sb1w() = default;

    virtual void pwr_up()
    {
//        init(F_CPU, 500000ul);
    }

    virtual void pwr_down()
    {
//        Uart::disable_clocks();
//        _Rx_dma::disable_clocks();
//        _Tx_dma::disable_clocks();
    }
};


#endif /* SB1W_UART_H */
