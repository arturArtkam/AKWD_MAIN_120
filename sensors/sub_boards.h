#ifndef SUB_BOARDS_H
#define SUB_BOARDS_H

#include "../../../Lib/crc16.h"
#include "../inc/sb1w_uart.h"
#include "../leds/led.h"
#include "../../boards_exchange.h"

static INLINE void sync_start(Sb1w* interface)
{
    const static uint8_t buf[] = {Exchange_between_boards::CMD_SYNC_START, 0x7E, 0xC4};

    interface->send_via_dma(buf, sizeof(buf), true);
}

class Boards_link
{
protected:
    Sb1w* _uart_ptr;
    uint8_t _addr;

public:
    Boards_link() :
        _uart_ptr(nullptr),
        _addr(0)
    {}

    bool wait_exchange_end(uint32_t timeout)
    {
        if (_uart_ptr->event.wait(timeout))
        {
            volatile bool result = _uart_ptr->event.check_source(Uart_event_id::RX_EVENT);
            return result;
        }
        else
        {
            return false;
        }
    }
};

class Adcboard : public Boards_link
{
public:
    Adcboard(Sb1w* interface, uint8_t addr)
    {
        _uart_ptr = interface;
        _addr = addr;
    }

    void read_data(void* buf, size_t buf_len, OS::TMutex& mutex)
    {
        uint8_t query[] = {uint8_t((_addr << 4) | Exchange_between_boards::CMD_GET_DATA), 0x00, 0x00};
        *(uint16_t* )&query[1] = crc16_split(&query[0], sizeof(uint8_t), 0xffff);

        while (mutex.is_locked());
        mutex.lock();
        _uart_ptr->set_rx_pointer(static_cast<uint8_t* >(buf), buf_len);
        _uart_ptr->send_via_dma(query, sizeof(query), false);

        if (!wait_exchange_end(250))
        {
            _uart_ptr->disable_reciever();
            Leds::blink_red_n_times(3);
        }

        mutex.unlock();
    }
};

class Sync_rx_board : public Boards_link
{
public:
    Sync_rx_board(Sb1w* interface, uint8_t addr)
    {
        _uart_ptr = interface;
        _addr = addr;
    }

    void read_data(void* buf, size_t buf_len, OS::TMutex& mutex)
    {
//        uint8_t query_semen[] = {uint8_t((_addr << 4) | Exchange_between_boards::CMD_GET_DATA), 0x00, 0x00};
//        *(uint16_t* )&query_semen[1] = crc16_split(&query_semen[0], sizeof(uint8_t), 0xffff);
        uint8_t query_semen[] = {uint8_t((_addr << 4) | Exchange_between_boards::CMD_GET_DATA)};

        while (mutex.is_locked());
        mutex.lock();
        _uart_ptr->set_rx_pointer(static_cast<uint8_t* >(buf), buf_len);
        _uart_ptr->send_via_dma(query_semen, sizeof(query_semen), false);

        if (!wait_exchange_end(10))
        {
            _uart_ptr->disable_reciever();
            Leds::blink_red_n_times(3);
        }

        mutex.unlock();
    }
};

#endif /* SUB_BOARDS_H */
