#pragma once
#ifndef SD_SUN_H
#define SD_SUN_H

#include "sdcard_sdiodrv.h"
#include "../../../Lib/pin.h"
#include "../../../Lib/bkp_domain.h"

class
{
public:
    void delay_ms(uint32_t arg)
    {
        OS::sleep(arg);
    }
} dwt;

class sd_card_t : public sdio_drv_t, public bkp_mem_t<uint32_t>
{
private:
    /* Размер карты в блоках 512 байт */
    volatile struct
    {
        uint32_t blocks_per_512b = 0;
    } _card_capacity;

    sd_result_t hard_reset()
    {
        do
        {
            mux.to_comp();
            Pwr_key::lo();

            dwt.delay_ms(100);

            mux.to_mcu();
            Pwr_key::hi();

            dwt.delay_ms(100);
        }
        while (sdio_drv_t::init() != SDR_Success);

        return SDR_Success;
    }

public:
    typedef pin<PORTA, 15, GPIO_Mode_OUT, GPIO_Speed_2MHz> Pwr_key;
    typedef pin<PORTB, 7,  GPIO_Mode_OUT, GPIO_Speed_2MHz, GPIO_OType_OD, GPIO_PuPd_NOPULL> Ncd_flg;

    sd_card_t() :
        _card_capacity ()
    {
        Pwr_key::init();
        Ncd_flg::init();

        mux.init_pins();

        Pwr_key::hi();
    }

    template<uint32_t size>
    Sd_message write(void* data)
    {
        sd_result_t res = SDR_Success;
        Sd_message err = ERR_NULL;

        enum
        {
            block_size = 512u,
            datasize_in_blocks = size / block_size /* Размер данных в блоках */
        };

//        #pragma message("Исправить размер блока данных")
        static_assert((size % block_size) == 0, "Исправить размер блока данных");

        if (_card_capacity.blocks_per_512b == 0)
        {
            err = INCORRECT_CAPACITY;
        }
        else
        {
            if ( ((*bkp::addr_ptr) + datasize_in_blocks) < _card_capacity.blocks_per_512b )
            {
                sdio_drv_t::init_fast();
                res = sdio_drv_t::write_block_dma(*bkp::addr_ptr, static_cast<uint32_t* >(data), datasize_in_blocks * block_size);

                if (res != SDR_Success)
                {
                    err = WRITEBLOCK_FAIL;
                    goto exit;
                }

                (*bkp::addr_ptr) += datasize_in_blocks;
            }
            else
            {
                err = ADDR_OUT_OF_RANGE;
            }
        }

    exit:
        return err;
    }

    typedef enum
    {
        INIT_STATE = 0,
        CONNECTED_TO_PC = 1,
        CONNECTED_TO_MCU

    } Mux_status;

    struct Mux
    {
        typedef pin<PORTC, 10, GPIO_Mode_OUT, GPIO_Speed_2MHz> Mux_in1;
        typedef pin<PORTC, 11, GPIO_Mode_OUT, GPIO_Speed_2MHz> Mux_in2;

        Mux_status status;

        Mux() :
            status(INIT_STATE)
        {
        }

        void init_pins()
        {
            Mux_in1::init();
            Mux_in2::init();
        }

        void to_mcu()
        {
            Ncd_flg::hi();
            dwt.delay_ms(100);
            Mux_in1::lo();
            Mux_in2::lo();

            status = CONNECTED_TO_MCU;
        }

        void to_comp()
        {
            Mux_in1::hi();
            Mux_in2::hi();
            dwt.delay_ms(100);
            Ncd_flg::lo();

            status = CONNECTED_TO_PC;
        }
    } mux;

    void sdio_lopwr()
    {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_SDIO, DISABLE);
    }


    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Waggregate-return"

    Sd_message init(bool addr_rst_flag = true)
    {
        sd_result_t sd_state = SDR_Success;
        Sd_message err = ERR_NULL;

        if (addr_rst_flag) *bkp::addr_ptr = 0;

        for (auto i = 0; i < 5; i++)
        {
            mux.to_comp();
            Pwr_key::lo();
            dwt.delay_ms(100);

            mux.to_mcu();
            Pwr_key::hi();
            dwt.delay_ms(100);

            sd_state = sdio_drv_t::init();

            if (sd_state == SDR_Success)
            {
                 break;
            }
            else
            {
                if (i >= 4)
                {
                    err = CARD_INIT_FAIL;

                    goto exit;
                }
            }
        }

        _card_capacity.blocks_per_512b = sdio_drv_t::get_info_struct().BlockCount * 1024u;

    exit:
        return err;
    }

    #pragma GCC diagnostic pop

    uint16_t erase_time_sec()
    {
        return sdio_drv_t::get_info_struct().CardEraseTmoutSec;
    }


    template<uint8_t denom = 16>
    sd_result_t full_erase()
    {
        static_assert(denom != 0, "denom template parameter must be greather than 0!");

        return sdio_drv_t::erase(0, (get_info_struct().BlockCount * 1024) / denom);
    }
};

#endif
