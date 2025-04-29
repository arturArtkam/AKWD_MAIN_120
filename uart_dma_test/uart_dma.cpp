#include "stm32_uart_f4xx.h"
#include "stm32_dma_f4xx.h"

template<uart_num_t uart_n, DmaChannelNum rx_ch, DmaChannelNum tx_ch, class io_pin>
class uart_sb_t
{
private:
    typedef uart_x<uart_n, io_pin> uart;
    typedef dma_stream<rx_ch> rx_dma;
    typedef dma_stream<tx_ch> tx_dma;

    size_t _old_pos;
    size_t _pos;
    size_t _buf_size;
    uint8_t* _buf_ptr;

public:
    uart_sb_t() :   _old_pos(0),
                    _pos(0),
                    _buf_size(0),
                    _buf_ptr(nullptr){}

    bool usart_rx_check();
    void transmitter_dma_config();
    void send_via_dma(uint8_t const* buf, uint32_t len);
    void tx_dma_isr();

    void configure(uint32_t periph_clock, uint32_t baud_rate)
    {
        io_pin::init();

        uart::enable_clocks();
        rx_dma::enable_clocks();
        tx_dma::enable_clocks();

        set_baud(uart::USARTn, periph_clock, baud_rate);

        NVIC_EnableIRQ(uart::USART_IRQn);
        NVIC_EnableIRQ(rx_dma::DMAChannel_IRQn);
        NVIC_EnableIRQ(tx_dma::DMAChannel_IRQn);
    }

    void set_rx_pointer(uint8_t* rx_ptr, uint32_t size)
    {
        rx_dma::stream->NDTR = size;
        rx_dma::stream->M0AR = reinterpret_cast<uint32_t>(rx_ptr);

        _buf_ptr = rx_ptr;
        _buf_size = size;

        _pos = 0;
        _old_pos = 0;
    }

    void process_data(const void* data, size_t start_position, size_t len)
    {
        memcpy(&_buf_ptr[start_position], data, len);
    }

    void reciever_dma_config()
    {
        /* 4 канал */
        SET_BIT(rx_dma::stream->CR, DMA_SxCR_CHSEL_2);

        SET_BIT(rx_dma::stream->CR, DMA_SxCR_MINC);
        SET_BIT(rx_dma::stream->CR, DMA_SxCR_CIRC);

        rx_dma::stream->PAR = reinterpret_cast<uint32_t>(&uart::USARTn->DR);
    }

    void start_receiving()
    {
        SET_BIT(uart::USARTn->CR3, USART_CR3_DMAR | USART_CR3_EIE);
        SET_BIT(uart::USARTn->CR1, USART_CR1_IDLEIE | USART_CR1_RE);

        SET_BIT(rx_dma::stream->CR, DMA_SxCR_EN);
    }

    void stop_receiving()
    {
        CLEAR_BIT(uart::USARTn->CR3, USART_CR3_DMAR | USART_CR3_EIE);
        CLEAR_BIT(uart::USARTn->CR1, USART_CR1_RE);

        CLEAR_BIT(rx_dma::stream->CR, DMA_SxCR_EN);
    }
};

template<uart_num_t uart_n, DmaChannelNum rx_ch, DmaChannelNum tx_ch, class io_pin>
bool uart_sb_t<uart_n, rx_ch, tx_ch, io_pin>::usart_rx_check()
{
    /* ¬ычисление текущей позиции в буфере и проверка наличи€ новых доступных данных */
    _pos = _buf_size - rx_dma::stream->NDTR;

    if (_pos != _old_pos) /* ѕровека изменений в полученных данных */
    {
        if (_pos > _old_pos)
        {
            /*
             * ќбработка в "линейном" режиме.
             *
             * ѕриложение обрабатывает данные в один прием,
             * длина данных вычисл€етс€ путем вычитани€ указателей
             * [   0   ]
             * [   1   ] <- old_pos |------------------------------------|
             * [   2   ]            |                                    |
             * [   3   ]            | Single block (len = pos - old_pos) |
             * [   4   ]            |                                    |
             * [   5   ]            |------------------------------------|
             * [   6   ] <- pos
             * [   7   ]
             * [ N - 1 ]
             */
            process_data(_buf_ptr + _old_pos, 0, _pos - _old_pos);
        }
        else
        {
            /* ќбработка в режиме "переполнени€"..
             *
             * ѕриложение обрабатывает данные в два приема
             * [   0   ]            |---------------------------------|
             * [   1   ]            | Second block (len = pos)        |
             * [   2   ]            |---------------------------------|
             * [   3   ] <- pos
             * [   4   ] <- old_pos |---------------------------------|
             * [   5   ]            |                                 |
             * [   6   ]            | First block (len = N - old_pos) |
             * [   7   ]            |                                 |
             * [ N - 1 ]            |---------------------------------|
             */
            process_data(_buf_ptr + _old_pos, 0, _buf_size - _old_pos);

            if (_pos > 0)
            {
                process_data(_buf_ptr, _buf_size - _old_pos, _pos);
            }
        }

        _old_pos = _pos;
        return true;
    }

    return false;
}

template<uart_num_t uart_n, DmaChannelNum rx_ch, DmaChannelNum tx_ch, class io_pin>
void uart_sb_t<uart_n, rx_ch, tx_ch, io_pin>::transmitter_dma_config()
{
    /* выбираем 4 канал */
    tx_dma::stream->CR |= DMA_SxCR_CHSEL_2;

    /* инкремент указател€ на пам€ть */
    tx_dma::stream->CR |= DMA_SxCR_MINC;

    /* направление из пам€ти в периферию */
    tx_dma::stream->CR |= DMA_SxCR_DIR_0;

    /* разрешаем прерывание по окончанию передачи */
    tx_dma::stream->CR |= DMA_SxCR_TCIE;

    /* адрес периферии */
    tx_dma::stream->PAR = reinterpret_cast<uint32_t>(&uart::USARTn->DR);
}

template<uart_num_t uart_n, DmaChannelNum rx_ch, DmaChannelNum tx_ch, class io_pin>
void uart_sb_t<uart_n, rx_ch, tx_ch, io_pin>::send_via_dma(uint8_t const* buf, uint32_t len)
{
    tx_dma::stream->CR  &= ~DMA_SxCR_EN;               //запрещаем передачу
    tx_dma::stream->NDTR = len;               //длина посылки
    tx_dma::stream->M0AR = (uint32_t)buf;               //адрес пам€ти

    /* разрешаеем DMA дл€ USATR на передачу */
    BIT_BAND_PER(uart::USARTn->CR3, USART_CR3_DMAT) = 1;

    /* ¬ыключил приемник и его прерывани€ */
    BIT_BAND_PER(uart::USARTn->CR1, USART_CR1_RE) = 0;
    BIT_BAND_PER(uart::USARTn->CR1, USART_CR1_RXNEIE) = 0;

    /* ¬ключил передатчик */
    BIT_BAND_PER(uart::USARTn->CR1, USART_CR1_TE) = 1;

    /* —брасываем флаг TC, чтоб не уходило в обработчик */
    BIT_BAND_PER(uart::USARTn->SR, USART_SR_TC) = 0;

    /* передаем... */
    BIT_BAND_PER(tx_dma::stream->CR, DMA_SxCR_EN) = 1;
}

template<uart_num_t uart_n, DmaChannelNum rx_ch, DmaChannelNum tx_ch, class io_pin>
void uart_sb_t<uart_n, rx_ch, tx_ch, io_pin>::tx_dma_isr()
{
    if (READ_BIT(tx_dma::DMAn->ISR[tx_dma::LO_HI_OFFSET], tx_dma::DMA_MASK_TEIF))
    {
        WRITE_REG(tx_dma::DMAn->IFCR[tx_dma::LO_HI_OFFSET], tx_dma::DMA_MASK_CTEIF);

        CLEAR_BIT(uart::USARTn->CR3, USART_CR3_DMAT);
        CLEAR_BIT(tx_dma::stream->CR, DMA_SxCR_EN);
    }
    else if (READ_BIT(tx_dma::DMAn->ISR[tx_dma::LO_HI_OFFSET], tx_dma::DMA_MASK_TCIF))
    {
        WRITE_REG(tx_dma::DMAn->IFCR[tx_dma::LO_HI_OFFSET], tx_dma::DMA_MASK_CTCIF);

        CLEAR_BIT(uart::USARTn->CR3, USART_CR3_DMAT);
        SET_BIT(uart::USARTn->CR1, USART_CR1_TCIE);
    }
}

namespace akwd
{
    typedef af_pin<PORTA, 4> uart_pin;

    void uart_dma_test()
    {
         auto sb_p = new uart_sb_t<UART1, DMA1_CH2, DMA1_CH6, uart_pin>;

        sb_p->configure(24000000, 500000);
        sb_p->tx_dma_isr();
        ///pc_uart.dma_isr();
    }
}
