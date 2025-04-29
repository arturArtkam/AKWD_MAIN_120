#pragma once
#ifndef SPI_DRV_H_INCLUDED
#define SPI_DRV_H_INCLUDED

#include "stm32f4xx_spi.h"

template< uint32_t spi_n,
          class miso_pin,
          class mosi_pin,
          class clk_pin >
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
        if (spi_p() == SPI1) RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);
        else if (spi_p() == SPI2) RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);
        else if (spi_p() == SPI3) RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI3, ENABLE);
    }

    static void rcc_deinit()
    {
        if(spi_p() == SPI1) RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, DISABLE);
        else if(spi_p() == SPI2) RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, DISABLE);
        else if (spi_p() == SPI3) RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI3, DISABLE);
    }

    static void mode_full_duplex_8t(uint16_t cpol, uint16_t cpha, uint16_t baud_presc)
    {
        SPI_InitTypeDef spi_struct;

        ///rcc_config();
        periph_powerup();
        SPI_StructInit(&spi_struct);

        spi_struct.SPI_Direction = SPI_Direction_2Lines_FullDuplex; //полный дуплекс
        spi_struct.SPI_DataSize = SPI_DataSize_8b; // передаем по 8 бит
        //spi_struct.SPI_CPOL = SPI_CPOL_Low; // Полярность и
        spi_struct.SPI_CPOL = cpol;
        spi_struct.SPI_CPHA = cpha; // фаза тактового сигнала
        spi_struct.SPI_NSS = SPI_NSS_Soft; // Управлять состоянием сигнала NSS программно
        spi_struct.SPI_BaudRatePrescaler = baud_presc; // Предделитель SCK
        spi_struct.SPI_FirstBit = SPI_FirstBit_MSB; // Первым отправляется старший бит
        spi_struct.SPI_Mode = SPI_Mode_Master; // Режим - мастер
        SPI_Init(spi_p(), &spi_struct);
        SPI_Cmd(spi_p(), ENABLE);
    }

    static void mode_full_duplex_8b(uint16_t cpol, uint16_t cpha, uint16_t baud_presc)
    {
        periph_powerup();

        spi_p()->CR2 = 0x0000;
        spi_p()->CR1 = 0x0000;

        spi_p()->CR1 |=
            (//SPI_CR1_BR_0 |
            //SPI_CR1_BR_1 |
            //SPI_CR1_BR_2 |
            baud_presc |
            SPI_CR1_MSTR |
            SPI_CR1_SSI  |
            cpha |
            cpol |
            SPI_CR1_SSM);

        spi_p()->CR2  = 0x700;   //  8 bit

        ///spi_p()->CR2 |= SPI_CR2_FRXTH;
        spi_p()->CR1 |= SPI_CR1_SPE;
    }

    static void mode_full_duplex_16t()
    {
        SPI_InitTypeDef spi_struct;

        SPI_StructInit(&spi_struct);
        rcc_config();

        spi_struct.SPI_Direction = SPI_Direction_2Lines_FullDuplex; //полный дуплекс
        spi_struct.SPI_DataSize = SPI_DataSize_16b;
        spi_struct.SPI_CPOL = SPI_CPOL_High; // Полярность и
        spi_struct.SPI_CPHA = SPI_CPHA_2Edge; // фаза тактового сигнала
        spi_struct.SPI_NSS = SPI_NSS_Soft; // Управлять состоянием сигнала NSS программно
        spi_struct.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_16; // Предделитель SCK
        spi_struct.SPI_FirstBit = SPI_FirstBit_MSB; // Первым отправляется старший бит
        spi_struct.SPI_Mode = SPI_Mode_Master; // Режим - мастер
        SPI_Init(spi_p(), &spi_struct);
        SPI_Cmd(spi_p(), ENABLE);
    }

    static void start_reading(){ spi_p()->CR1 |= SPI_CR1_SPE; }

    static void disable_periph(){ spi_p()->CR1 &= ~SPI_CR1_SPE; }

    static bool wait_rxne()
    {
        volatile auto data_tim = 0xffff;

        while (!(spi_p()->SR & SPI_SR_RXNE) && data_tim--);
        if(data_tim == 0) return false;
        else return true;
    }

    static void wait_txe()
    {
        while(!(spi_p()->SR & SPI_SR_TXE));
    }

    static bool wait_busy()
    {
        volatile auto timer = 0xffff;

        while((spi_p()->SR & SPI_SR_BSY) && timer--);
        if(timer == 0) return false;
        else return true;
    }

//    static uint8_t exchange_8t(uint8_t dat = 0)
//    {
//        *reinterpret_cast<volatile uint8_t* >(&(spi_p()->DR)) = dat;
//        //wait_rxne();
//        wait_txe();
//        return static_cast<uint8_t>(spi_p()->DR);
//    }

    static uint8_t exchange_8t(uint8_t data = 0)
    {
        #define SPI_DR_8bit (*(__IO uint8_t* )((uint32_t)&(spi_p()->DR)))

        while (!(spi_p()->SR & SPI_SR_TXE));
        __NOP();
        SPI_DR_8bit = data;
        __NOP();
        while((spi_p()->SR & SPI_SR_BSY));
        __NOP();
        while (!(spi_p()->SR & SPI_SR_RXNE));
        __NOP();
        return (SPI_DR_8bit);
    }

    static void transmit_only_8b(uint8_t data = 0)
    {
        #define SPI_DR_8bit (*(__IO uint8_t* )((uint32_t)&(spi_p()->DR)))

        SPI_DR_8bit = data;

        while (!(spi_p()->SR & SPI_SR_TXE));
        while((spi_p()->SR & SPI_SR_BSY));
    }


//    template<class type_t = uint8_t>
//    type_t exchange(type_t dat = 0)
//    {
//        spi_p()->DR = (type_t)dat;
//        wait_rxne();
//        return (type_t )spi_p()->DR;
//    }

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

#endif /* SPI_DRV_H_INCLUDED */
