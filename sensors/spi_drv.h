#pragma once
#ifndef SPI_DRV_H_INCLUDED
#define SPI_DRV_H_INCLUDED

#include "pin.h"
#include "stm32f4xx_spi.h"

#define GPIO_AF_SPI4            ((uint8_t)0x05)  /* SPI4 Alternate Function mapping */
#define SPI4_BASE               (APB2PERIPH_BASE + 0x3400)
#define SPI4                    ((SPI_TypeDef* ) SPI4_BASE)
#define RCC_APB2Periph_SPI4     ((uint32_t)0x00002000)

template <uint32_t spi_n = 0,
          class miso_pin = af_pin<>,
          class mosi_pin = af_pin<>,
          class clk_pin = af_pin<>>
class hw_spi
{
public:
    hw_spi() = delete;
    ~hw_spi() = delete;

    static void init()
    {
        miso_pin::init();
        mosi_pin::init();
        clk_pin::init();

        miso_pin::pullup();
    }

    constexpr static auto spi_p()
    {
        return reinterpret_cast<SPI_TypeDef* >(spi_n);
    }

    static void rcc_config()
    {
        if(spi_p() == SPI1) RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);
        else if(spi_p() == SPI2) RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);
        else if(spi_p() == SPI3) RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI3, ENABLE);
        else if(spi_p() == SPI4) RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI4, ENABLE);
    }

    static void rcc_deinit()
    {
        if(spi_p() == SPI1) RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, DISABLE);
        else if(spi_p() == SPI2) RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, DISABLE);
        else if(spi_p() == SPI3) RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI3, DISABLE);
        else if(spi_p() == SPI4) RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI4, DISABLE);
    }

    static void mode_full_duplex_8t(uint16_t cpol, uint16_t cpha, uint16_t baud_presc)
    {
        SPI_InitTypeDef spi_struct;

        periph_powerup();
        SPI_StructInit(&spi_struct);

        spi_struct.SPI_Direction = SPI_Direction_2Lines_FullDuplex; //полный дуплекс
        spi_struct.SPI_DataSize = SPI_DataSize_8b; // передача по 8 бит
        spi_struct.SPI_CPOL = cpol; // Полярность и
        spi_struct.SPI_CPHA = cpha; // фаза тактового сигнала
        spi_struct.SPI_NSS = SPI_NSS_Soft; // Управлять состоянием сигнала NSS программно
        spi_struct.SPI_BaudRatePrescaler = baud_presc; // Предделитель SCK
        spi_struct.SPI_FirstBit = SPI_FirstBit_MSB; // Первым отправляется старший бит
        spi_struct.SPI_Mode = SPI_Mode_Master; // Режим - мастер

        SPI_Init(spi_p(), &spi_struct);
        SPI_Cmd(spi_p(), ENABLE);
    }

    static void mode_full_duplex_16t(uint16_t cpol, uint16_t cpha, uint16_t baud_presc)
    {
        SPI_InitTypeDef spi_struct;

        periph_powerup();
        SPI_StructInit(&spi_struct);

        spi_struct.SPI_Direction = SPI_Direction_2Lines_FullDuplex; //полный дуплекс
        spi_struct.SPI_DataSize = SPI_DataSize_16b; // передача по 16 бит
        spi_struct.SPI_CPOL = cpol; // Полярность и
        spi_struct.SPI_CPHA = cpha; // фаза тактового сигнала
        spi_struct.SPI_NSS = SPI_NSS_Soft; // Управлять состоянием сигнала NSS программно
        spi_struct.SPI_BaudRatePrescaler = baud_presc; // Предделитель SCK
        spi_struct.SPI_FirstBit = SPI_FirstBit_MSB; // Первым отправляется старший бит
        spi_struct.SPI_Mode = SPI_Mode_Master; // Режим - мастер

        SPI_Init(spi_p(), &spi_struct);
        SPI_Cmd(spi_p(), ENABLE);
    }

    static void start_reading(){ spi_p()->CR1 |= SPI_CR1_SPE; }

    static void disable_periph(){ spi_p()->CR1 &= ~SPI_CR1_SPE; }

    static bool wait_rxne()
    {
        volatile auto data_tim = 0x0fff;

        while (!(spi_p()->SR & SPI_SR_RXNE) && --data_tim);

        if (data_tim == 0)
            return false;
        else
            return true;
    }

    static void wait_txe()
    {
        while (!(spi_p()->SR & SPI_SR_TXE));
    }

    static bool wait_busy()
    {
        volatile auto timer = 0xffff;

        while ((spi_p()->SR & SPI_SR_BSY) && timer--);

        if (timer == 0)
            return false;
        else
            return true;
    }

    static uint8_t exchange_8t(uint8_t dat = 0)
    {
        spi_p()->DR = dat;
        wait_rxne();

        return static_cast<uint8_t>(spi_p()->DR);
    }

    static void periph_powerup()
    {
        rcc_config();
        clk_pin::pin_mode(GPIO_Mode_AF);
        miso_pin::pin_mode(GPIO_Mode_AF);
        mosi_pin::pin_mode(GPIO_Mode_AF);
    }

    static void periph_lopwr()
    {
        rcc_deinit();
        clk_pin::pin_mode(GPIO_Mode_AN);
        miso_pin::pin_mode(GPIO_Mode_AN);
        mosi_pin::pin_mode(GPIO_Mode_AN);
    }
};

typedef af_pin<PORTA, 6, GPIO_AF_SPI1> spi1_miso;
typedef af_pin<PORTA, 7, GPIO_AF_SPI1> spi1_mosi;
typedef af_pin<PORTA, 5, GPIO_AF_SPI1> spi1_clk;

typedef af_pin<PORTB, 14, GPIO_AF_SPI2> spi2_miso;
typedef af_pin<PORTB, 15, GPIO_AF_SPI2> spi2_mosi;
typedef af_pin<PORTB, 13, GPIO_AF_SPI2> spi2_clk;

typedef af_pin<PORTB, 4, GPIO_AF_SPI3> spi3_miso;
typedef af_pin<PORTB, 5, GPIO_AF_SPI3> spi3_mosi;
typedef af_pin<PORTB, 3, GPIO_AF_SPI3> spi3_clk;

typedef af_pin<PORTE, 5, GPIO_AF_SPI4> spi4_miso;
typedef af_pin<PORTE, 6, GPIO_AF_SPI4> spi4_mosi;
typedef af_pin<PORTE, 2, GPIO_AF_SPI4> spi4_clk;

#endif /* SPI_DRV_H_INCLUDED */
