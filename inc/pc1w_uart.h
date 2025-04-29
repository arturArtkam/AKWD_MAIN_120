#ifndef PC1W_UART_H_INCLUDED
#define PC1W_UART_H_INCLUDED

#include "../../AKWD_MAIN_2/uart_dma_test/stm32_dma_f4xx.h"
#include "../../AKWD_MAIN_2/uart_dma_test/stm32_uart_f4xx.h"
#include "../../../Lib/crc16.h"
#include "../inc/protokol.h"
#include "usart_event.h"

#define USE_EXT_PACKET
#define ERR_MASK (USART_SR_ORE | USART_SR_NE | USART_SR_FE | USART_SR_PE)

#pragma pack(push, 1)
struct packet_t
{
    uint8_t head;
    uint8_t const* body;
    uint16_t lengh;
    uint16_t crc;
};

struct body_t
{
    uint8_t const* data;
    uint16_t lengh;
};

struct ext_packet_t
{
    uint8_t head;
    body_t body[8];
    uint8_t body_idx;
    uint32_t data_lenght;
    uint16_t crc;
};
#pragma pack(pop)

template<Uart_num uart_n, DmaChannelNum rx_ch, DmaChannelNum tx_ch, class io_pin>
class uart_pc1w_t
{
private:
    typedef uart_x<uart_n, io_pin> _Uart;
    typedef dma_stream<tx_ch> _Tx_dma;

    packet_t _tx_pack;
    ext_packet_t _ext_packet;
    size_t _cursor;
    size_t _data_len;

    OS::TMutex* _mutex;

public:
    enum
    {
        BUF_SIZE = 128,
        BUF_MASK = BUF_SIZE - 1
    };

    Usart_event event;
    uint8_t buf[BUF_SIZE];
    typedef protokol_t Protocol;

public:
    ~uart_pc1w_t() = default;
    uart_pc1w_t() :
        _tx_pack(),
        _cursor(0),
        _data_len(0),
        _mutex(nullptr),
        event(),
        buf{}
    {
        static_assert((BUF_SIZE > 0 && ((BUF_SIZE & BUF_MASK) == 0)) != 0,
        "BUF IS NOT POWER OF TWO");
    }

    size_t get_count()
    {
        return _data_len;
    }

    void init(uint32_t periph_clock, uint32_t baud_rate);
    void transmitter_dma_config();
    void send_via_dma(uint8_t const* buf, uint32_t len);
    void build_tx_packet(uint8_t cmd, const void* src, size_t len);
    void start_tx_packet_send();
    void start_tx_packet_send(OS::TMutex* mutex);
    void uart_isr();
    void dma_isr();

#if defined USE_EXT_PACKET
    void ext_packet_init(size_t expected_length)
    {
        _ext_packet.data_lenght = expected_length;
    }
    void push_ext_packet_head(uint8_t cmd);
    void push_ext_body_piece(const void* src, size_t len);
    bool start_ext_packet_send(OS::TMutex* mutex);
#endif
};

template<Uart_num uart_n, DmaChannelNum rx_ch, DmaChannelNum tx_ch, class io_pin>
void uart_pc1w_t<uart_n, rx_ch, tx_ch, io_pin>::init(uint32_t periph_clock, uint32_t baud_rate)
{
    io_pin::init();
    io_pin::pullup();

    _Uart::enable_clocks();
    _Tx_dma::enable_clocks();

    set_baud(_Uart::USARTn, periph_clock, baud_rate);

    /* ѕереводим в полудуплексный режим до включени€ приемо-передатчика */
    SET_BIT(_Uart::USARTn->CR3, USART_CR3_HDSEL);

    SET_BIT(_Uart::USARTn->CR1, USART_CR1_RXNEIE);
    SET_BIT(_Uart::USARTn->CR1, USART_CR1_RE);
    SET_BIT(_Uart::USARTn->CR1, USART_CR1_UE);
    /* polling idle frame Transmission */
    while ((_Uart::USARTn->SR & USART_SR_TC) != USART_SR_TC) {}

    transmitter_dma_config();

    NVIC_EnableIRQ(_Uart::USART_IRQn);
    NVIC_EnableIRQ(_Tx_dma::DMAChannel_IRQn);
}

template<Uart_num uart_n, DmaChannelNum rx_ch, DmaChannelNum tx_ch, class io_pin>
void uart_pc1w_t<uart_n, rx_ch, tx_ch, io_pin>::transmitter_dma_config()
{
    /* выбираем 4 канал */
    _Tx_dma::stream->CR |= DMA_SxCR_CHSEL_2;

    /* инкремент указател€ на пам€ть */
    _Tx_dma::stream->CR |= DMA_SxCR_MINC;

    /* направление из пам€ти в периферию */
    _Tx_dma::stream->CR |= DMA_SxCR_DIR_0;

    /* разрешаем прерывание по окончанию передачи */
    _Tx_dma::stream->CR |= DMA_SxCR_TCIE;

    /* адрес периферии */
    _Tx_dma::stream->PAR  = reinterpret_cast<uint32_t>(&_Uart::USARTn->DR);

}

template<Uart_num uart_n, DmaChannelNum rx_ch, DmaChannelNum tx_ch, class io_pin>
void uart_pc1w_t<uart_n, rx_ch, tx_ch, io_pin>::send_via_dma(uint8_t const* src, uint32_t len)
{
    _Tx_dma::stream->CR  &= ~DMA_SxCR_EN;               //запрещаем передачу
    _Tx_dma::stream->NDTR = len;               //длина посылки
    _Tx_dma::stream->M0AR = (uint32_t)src;               //адрес пам€ти

    /* разрешаеем DMA дл€ USATR на передачу */
    BIT_BAND_PER(_Uart::USARTn->CR3, USART_CR3_DMAT) = 1;

    /* ¬ыключил приемник и его прерывани€ */
    BIT_BAND_PER(_Uart::USARTn->CR1, USART_CR1_RE) = 0;
    BIT_BAND_PER(_Uart::USARTn->CR1, USART_CR1_RXNEIE) = 0;

    /* ¬ключил передатчик */
    BIT_BAND_PER(_Uart::USARTn->CR1, USART_CR1_TE) = 1;

    /* —брасываем флаг TC, чтоб не уходило в обработчик */
    BIT_BAND_PER(_Uart::USARTn->SR, USART_SR_TC) = 0;

    /* передаем... */
    _Tx_dma::enable();
}

template<Uart_num uart_n, DmaChannelNum rx_ch, DmaChannelNum tx_ch, class io_pin>
void uart_pc1w_t<uart_n, rx_ch, tx_ch, io_pin>::build_tx_packet(uint8_t cmd, const void* src, size_t len)
{
    _tx_pack.head = cmd;
    _tx_pack.body = static_cast<uint8_t const* >(src);
    _tx_pack.lengh = len;
    // поскольку crc считаетс€ дл€ всей посылки, начинаем считать с начала буфера
    _tx_pack.crc = crc16_split(&_tx_pack.head, sizeof(_tx_pack.head), 0xffff);
    _tx_pack.crc = crc16_split(_tx_pack.body, _tx_pack.lengh, _tx_pack.crc);
}

#if defined USE_EXT_PACKET

template<Uart_num uart_n, DmaChannelNum rx_ch, DmaChannelNum tx_ch, class io_pin>
void uart_pc1w_t<uart_n, rx_ch, tx_ch, io_pin>::push_ext_packet_head(uint8_t cmd)
{
    _ext_packet.head = cmd;
    memset(_ext_packet.body, 0, sizeof(_ext_packet.body));
    _ext_packet.body_idx = 0;
    _ext_packet.crc = crc16_split(&_ext_packet.head, sizeof(_ext_packet.head), 0xffff);
}

/*
        Ёта функци€ добавл€ет очередной кусок данных в тело "расширенного" пакета.
    ќна принимает указатель на исходные данные и их длину, сохран€ет их в структуру _ext_packet
    и вычисл€ет контрольную сумму пакета с помощью функции crc16_split. «атем инкрементируетс€ индекс тела пакета
    дл€ добавлени€ следующего куска данных.
*/
template<Uart_num uart_n, DmaChannelNum rx_ch, DmaChannelNum tx_ch, class io_pin>
void uart_pc1w_t<uart_n, rx_ch, tx_ch, io_pin>::push_ext_body_piece(const void* src, size_t len)
{
    _ext_packet.body[_ext_packet.body_idx].data = static_cast<uint8_t const* >(src);
    _ext_packet.body[_ext_packet.body_idx].lengh = len;

    _ext_packet.crc = crc16_split(_ext_packet.body[_ext_packet.body_idx].data,
                                  _ext_packet.body[_ext_packet.body_idx].lengh,
                                  _ext_packet.crc);
    _ext_packet.body_idx++;
}

template<Uart_num uart_n, DmaChannelNum rx_ch, DmaChannelNum tx_ch, class io_pin>
bool uart_pc1w_t<uart_n, rx_ch, tx_ch, io_pin>::start_ext_packet_send(OS::TMutex* mutex)
{
    if (mutex != nullptr)
    {
        _mutex = mutex;
        _mutex->lock();
    }

    /* ¬ычисление полной длины данных подлежащих отправке */
    uint32_t tx_data_len = 0;
    for (uint32_t i = 0; i < _ext_packet.body_idx; i++)
    {
        tx_data_len += _ext_packet.body[i].lengh;
    }

    _ext_packet.body_idx = 0;

    if (tx_data_len == _ext_packet.data_lenght)
    {
        send_via_dma(static_cast<uint8_t* >(&_ext_packet.head), sizeof(_ext_packet.head));
        return true;
    }
    else
    {
        _mutex->unlock();
        return false;
    }
}

#endif

template<Uart_num uart_n, DmaChannelNum rx_ch, DmaChannelNum tx_ch, class io_pin>
void uart_pc1w_t<uart_n, rx_ch, tx_ch, io_pin>::start_tx_packet_send()
{
    send_via_dma(static_cast<uint8_t* >(&_tx_pack.head), sizeof(_tx_pack.head));
}

template<Uart_num uart_n, DmaChannelNum rx_ch, DmaChannelNum tx_ch, class io_pin>
void uart_pc1w_t<uart_n, rx_ch, tx_ch, io_pin>::start_tx_packet_send(OS::TMutex* mutex)
{
    _mutex = mutex;
    _mutex->lock();

    send_via_dma(static_cast<uint8_t* >(&_tx_pack.head), sizeof(_tx_pack.head));
}

template<Uart_num uart_n, DmaChannelNum rx_ch, DmaChannelNum tx_ch, class io_pin>
void uart_pc1w_t<uart_n, rx_ch, tx_ch, io_pin>::uart_isr()
{
    if (READ_BIT(_Uart::USARTn->SR, ERR_MASK))
    {
        CLEAR_BIT(_Uart::USARTn->SR, ERR_MASK);
        _Uart::USARTn->DR;
    }
    else if (READ_BIT(_Uart::USARTn->SR, USART_SR_RXNE) == USART_SR_RXNE)
    {
        buf[_cursor++ & BUF_MASK] = uint8_t(_Uart::USARTn->DR);
        SET_BIT(_Uart::USARTn->CR1, USART_CR1_IDLEIE);
    }
    else if (READ_BIT(_Uart::USARTn->SR, USART_SR_IDLE) == USART_SR_IDLE)
    {
        BIT_BAND_PER(_Uart::USARTn->CR1, USART_CR1_IDLEIE) = 0;
        _Uart::USARTn->DR;

        _data_len = _cursor;
        _cursor = 0;

        if ((buf[0] & 0xF0) == ADR || buf[0] == 0xF5)
        {
            if (crc16(buf, _data_len) == 0)
            {
                event.signal_from_isr(RX_EVENT);
            }
        }
        else
        {
            WRITE_REG(_Uart::USARTn->SR, 0);
        }
    }
    else if (READ_BIT(_Uart::USARTn->SR, USART_SR_TC) == USART_SR_TC)
    {
        BIT_BAND_PER(_Uart::USARTn->SR, USART_SR_TC) = 0;
        /* запрещаем DMA в UART */
        BIT_BAND_PER(_Uart::USARTn->CR3, USART_CR3_DMAT) = 0;
        /* выкл передатчика, запрещение прерываний по завершению передачи */
        CLEAR_BIT(_Uart::USARTn->CR1, (USART_CR1_TCIE | USART_CR1_TE));
        /* вкл. приемника и его прерывани€ */
        SET_BIT(_Uart::USARTn->CR1, (USART_CR1_RXNEIE | USART_CR1_RE));

        if (_mutex != nullptr)
        {
            _mutex->unlock_isr();
            _mutex = nullptr;
        }

        event.signal_from_isr(TX_EVENT);
    }
    else
    {
        __NOP();
    }
}

template<Uart_num uart_n, DmaChannelNum rx_ch, DmaChannelNum tx_ch, class io_pin>
void uart_pc1w_t<uart_n, rx_ch, tx_ch, io_pin>::dma_isr()
{
#if defined USE_EXT_PACKET

    if (READ_BIT(_Tx_dma::DMAn->ISR[_Tx_dma::LO_HI_OFFSET], _Tx_dma::DMA_MASK_TCIF))
    {
        WRITE_REG(_Tx_dma::DMAn->IFCR[_Tx_dma::LO_HI_OFFSET], _Tx_dma::DMA_MASK_CTCIF);
        _Tx_dma::disable();

        /*
            1. ≈сли DMA передает заголовок пакета и тело пакета не пустое,
            то устанавливает адрес и количество байт дл€ передачи тела пакета и увеличивает индекс тела пакета.

            2. ≈сли DMA передает тело пакета и есть еще тела дл€ передачи,
            то устанавливает адрес и количество байт дл€ передачи следующего тела пакета и увеличивает индекс тела пакета.

            3. ≈сли DMA передает последнее тело пакета, то устанавливает адрес и количество байт
            дл€ передачи контрольной суммы пакета.
        */

        if (_Tx_dma::stream->M0AR == (uint32_t)&_ext_packet.head) /* 1 */
        {
            if (_ext_packet.body[0].data != nullptr)
            {
                _Tx_dma::stream->M0AR = (uint32_t)_ext_packet.body[0].data;
                _Tx_dma::stream->NDTR = _ext_packet.body[0].lengh;

                _ext_packet.body_idx++;
                _Tx_dma::enable();
            }
            else
            {
                goto send_crc;
            }
        }
        else if (_Tx_dma::stream->M0AR == (uint32_t)_ext_packet.body[_ext_packet.body_idx - 1].data &&
                 _ext_packet.body[_ext_packet.body_idx].data != nullptr) /* 2 */
        {
            _Tx_dma::stream->M0AR = (uint32_t)_ext_packet.body[_ext_packet.body_idx].data;
            _Tx_dma::stream->NDTR = _ext_packet.body[_ext_packet.body_idx].lengh;

            _ext_packet.body_idx++;
            _Tx_dma::enable();
        }
        else if (_Tx_dma::stream->M0AR == (uint32_t)_ext_packet.body[_ext_packet.body_idx - 1].data &&
                 _ext_packet.body[_ext_packet.body_idx].data == nullptr)  /* 3 */
        {
    send_crc:

            _Tx_dma::stream->NDTR = sizeof(uint16_t);
            _Tx_dma::stream->M0AR = (uint32_t)&_ext_packet.crc;

            _Tx_dma::enable();
        }
        else
        {
            /* ѕрерывание по окончанию передачи */
            SET_BIT(_Uart::USARTn->CR1, USART_CR1_TCIE);
        }
    }
    else
    {
        __NOP();
    }

#else

    if (READ_BIT(_Tx_dma::DMAn->ISR[_Tx_dma::LO_HI_OFFSET], _Tx_dma::DMA_MASK_TCIF))
    {
        WRITE_REG(_Tx_dma::DMAn->IFCR[_Tx_dma::LO_HI_OFFSET], _Tx_dma::DMA_MASK_CTCIF);
        CLEAR_BIT(_Tx_dma::stream->CR, DMA_SxCR_EN);

        if (_Tx_dma::stream->M0AR == (uint32_t)&this->_tx_pack.head)
        {
            if (this->_tx_pack.lengh > 0)
            {
                _Tx_dma::stream->NDTR =  this->_tx_pack.lengh;
                _Tx_dma::stream->M0AR = (uint32_t)this->_tx_pack.body;

                //SET_BIT(_Tx_dma::stream->CR, DMA_SxCR_EN);
                _Tx_dma::enable();
            }
            else
            {
                goto send_crc;
            }
        }
        else if (_Tx_dma::stream->M0AR == (uint32_t)this->_tx_pack.body)
        {
    send_crc:
            _Tx_dma::stream->NDTR =  sizeof(uint16_t);
            _Tx_dma::stream->M0AR = (uint32_t)&this->_tx_pack.crc;

            //SET_BIT(_Tx_dma::stream->CR, DMA_SxCR_EN);
            _Tx_dma::enable();
        }
        else
        {
            /* ѕрерывание по окончанию передачи */
            SET_BIT(_Uart::USARTn->CR1, USART_CR1_TCIE);
        }
    }
    else
    {
        __NOP();
    }

#endif
}

#undef USE_EXT_PACKET
#undef ERR_MASK

//DMA2_Stream7
typedef af_pin<PORTB, 6, GPIO_AF_USART1> pin_58;
class Pc1w : public uart_pc1w_t<UART1, DMA1_CH0, DMA2_CH7, pin_58>
{
public:
    Pc1w() = default;
    ~Pc1w() = default;
};

#endif // PC1W_UART_H_INCLUDED
