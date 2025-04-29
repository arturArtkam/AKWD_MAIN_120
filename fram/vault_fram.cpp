#include <mutex>

#include "scmRTOS.h"
#include "pin.h"
#include "mr25_ic.h"
#include "vault_fram.h"
#include "struct_of_data.h"
#include "CRC16.h"

namespace
{
    enum
    {
        FRAM_1M = 131072ul
    };

    struct fram_markup_t
    {
        struct __attribute__((packed, aligned(1)))
        {
            EepData_t vars;
            uint16_t  crc;
        } eeprom;

        struct __attribute__((packed, aligned(1)))
        {
            uint32_t freespace_addr;
            char log_data[FRAM_1M - (sizeof(freespace_addr) + sizeof(eeprom))];
        } log;

    } __attribute__((packed, aligned(1)));

    typedef pin<PORTB, 9, GPIO_Mode_OUT, GPIO_Speed_2MHz> mr25_cs_pin;
    typedef hw_spi<SPI3_BASE, spi3_miso, spi3_mosi, spi3_clk> fram_spi;

    typedef ic_mr25<FRAM_1M, fram_spi, mr25_cs_pin> fram_ic;

    OS::TMutex _fram_mutex;
}

void vault_fram_t::init()
{
    static_assert((FRAM_1M - sizeof(vault)) == 0, "!!!");

    fram_ic::init();
}

//void vault_fram_t::clear_log()
//{
//    uint32_t addr = 0x0000;
//
//    fram::enable_write();
//
//    fram::write_buf(&addr, offsetof(vault, vault::log.freespace_addr), sizeof(vault::log.freespace_addr));
//
//    fram::disable_write();
//}
//
//void vault_fram_t::push_log_entry(void const* data, uint32_t len)
//{
//    uint32_t addr;
//
//    fram::enable_write();
//
//    fram::read_buf(&addr, offsetof(vault, vault::log.freespace_addr), sizeof(vault::log.freespace_addr));
//    fram::write_buf(data, addr, len);
//
//    addr += len;
//
//    fram::write_buf(&addr, offsetof(vault, vault::log.freespace_addr), sizeof(vault::log.freespace_addr));
//
//    fram::disable_write();
//}

void vault_fram_t::read_fields(void* data)
{
    fram_ic::read_buf(data, offsetof(vault, vault::eeprom), sizeof(vault::eeprom));
}

void vault_fram_t::print_fields(pc1w_t* uart_p)
{
    fram_ic::read_buf(uart_p->buf, offsetof(vault, vault::eeprom), sizeof(vault::eeprom));

    uart_p->build_tx_packet(0, uart_p->buf, sizeof(vault::eeprom));
    uart_p->start_tx_packet_send();
}

void vault_fram_t::read_entry(void* data, uint32_t offset, size_t lenght)
{
    std::lock_guard<OS::TMutex> lock(_fram_mutex);

    fram_ic::read_buf(data, offsetof(vault, vault::eeprom) + offset, lenght);
}

void vault_fram_t::write_entry(const void* data, uint32_t offset, size_t lenght)
{
    std::lock_guard<OS::TMutex> lock(_fram_mutex);

    fram_ic::enable_write();

    fram_ic::write_buf(data, offsetof(vault, vault::eeprom) + offset, lenght);

    fram_ic::disable_write();
}
