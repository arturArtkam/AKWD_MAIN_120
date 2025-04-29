#pragma once

#include "spi_drv.h"

template<size_t density, class spi_n, class cs_pin>
class ic_mb85
{
private:
    //typedef enum : uint8_t
    enum op_codes
    {
        WREN = 0x06, //Write Enable
        WRDI = 0x04, //Write Disable
        RDSR = 0x05, //Read Status Register
        WRSR = 0x01, //Write Status Register
        READ = 0x03, //Read Data Bytes
        WRITE = 0x02, //Write Data Bytes
    };

public:
    ic_mb85() = delete;
    ~ic_mb85() = delete;


    static void init()
    {
        cs_pin::init();
        spi_n::init();

        cs_pin::hi();
//        spi_n::mode_full_duplex_8t(SPI_CPOL_High, SPI_CPHA_2Edge, SPI_BaudRatePrescaler_4);
        spi_n::mode_full_duplex_8t(SPI_CPOL_Low, SPI_CPHA_1Edge, SPI_BaudRatePrescaler_2);
    }

    static uint8_t read_status()
    {
        uint8_t tmp;

        cs_pin::lo();

        spi_n::exchange_8t(op_codes::RDSR);
        tmp = spi_n::exchange_8t();

        cs_pin::hi();

        return tmp;
    }

    static void enable_write()
    {
        cs_pin::lo();

        spi_n::exchange_8t(op_codes::WREN);

        cs_pin::hi();
    }


    static void wakeup()
    {

    }

    static void sleep()
    {

    }

    static void disable_write()
    {
        cs_pin::lo();

        spi_n::exchange_8t(op_codes::WRDI);

        cs_pin::hi();
    }

    static uint16_t calc_crc(uint16_t (*crc16)(uint8_t const* , uint16_t, uint16_t), uint32_t start_addr, uint32_t len)
    {
        uint16_t crc {0xffff};

        for(uint32_t mram_addr = start_addr; mram_addr < (start_addr + len); mram_addr++)
        {
            uint8_t tmp;

            read_buf(&tmp, mram_addr, sizeof(tmp));
            crc = crc16(&tmp, sizeof(tmp), crc);
        }

        return crc;
    }

    static void erase_memory()
    {
        ;
    }

    static uint8_t read_byte(uint32_t at_adr){ return 0; }

    static bool read_buf(void* buf, uint32_t from_adr, uint32_t lengh)
    {
        uint8_t* ptr = static_cast<uint8_t* >(buf);

        if (from_adr + lengh < density)
        {
            cs_pin::lo();

            spi_n::exchange_8t(op_codes::READ);
            //16-Bit Address
            spi_n::exchange_8t(from_adr >> 8);
            spi_n::exchange_8t(from_adr);

            while (lengh-- > 0)
            {
                *ptr = spi_n::exchange_8t();
                ptr++;
            }

            cs_pin::hi();

            return true;
        }
        else
        {
            return false;
        }
    }

    static bool write_buf(void const* buf, uint32_t from_adr, uint32_t lengh)
    {
        uint8_t* ptr = static_cast<uint8_t* >(const_cast<void* >(buf));

        enable_write();

        if (from_adr + lengh < density)
        {
            cs_pin::lo();
            //Instruction (02h)
            spi_n::exchange_8t(op_codes::WRITE);
            //16-Bit Address MSB
            spi_n::exchange_8t(from_adr >> 8);
            spi_n::exchange_8t(from_adr);

            while (lengh-- > 0)
            {
                spi_n::exchange_8t(*ptr);
                ptr++;
            }

            cs_pin::hi();

            return true;
        }
        else
        {
            return false;
        }
    }
};


