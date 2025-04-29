#ifndef AXEL_H_INCLUDED
#define AXEL_H_INCLUDED

#include "../pwr/ipower.h"
#include "kx132_ic.h"
#include "ais2ih.h"

class spi_t;

class Axel_kx : public Ipower
{
    typedef hw_spi<SPI1_BASE, spi1_miso, spi1_mosi, spi1_clk> Spi_bus;
    typedef pin<PORTC, 4, GPIO_Mode_OUT, GPIO_Speed_2MHz> Kxcs_pin;

    kx132_ic<Spi_bus, Kxcs_pin> _kx_132;

public:
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wunused-parameter"
    Axel_kx(spi_t* interface) :
        _kx_132()
    {
        _kx_132.spibus_conf(SPI_CPOL_High, SPI_CPHA_2Edge, SPI_BaudRatePrescaler_64);
    }
    #pragma GCC diagnostic pop

    auto read_data()
    {
        return _kx_132.read_xyz();
    }

    void pwr_up() override
    {
        _kx_132.pwr_up();
        _kx_132.norm_mode_2g_50hz();
    }

    void pwr_down() override
    {
        _kx_132.pwr_down();
    }
};

class Axel_ais : public Ipower
{
    typedef hw_spi<SPI1_BASE, spi1_miso, spi1_mosi, spi1_clk> Spi_bus;
    typedef pin<PORTC, 4, GPIO_Mode_OUT, GPIO_Speed_2MHz> Aiscs_pin;

    Ais2ih_ic<Spi_bus, Aiscs_pin> _ais_2h;

public:
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wunused-parameter"
    Axel_ais(spi_t* interface) :
        _ais_2h()
    {
        _ais_2h.spibus_conf(SPI_CPOL_High, SPI_CPHA_2Edge, SPI_BaudRatePrescaler_64);
    }
    #pragma GCC diagnostic pop

    auto read_data()
    {
        return _ais_2h.read_xyz();
    }

    void pwr_up() override
    {
//        _ais_2h.pwr_up();
        _ais_2h.init();
    }

    void pwr_down() override
    {
//        _ais_2h.pwr_down();
    }
};

#endif /* AXEL_H_INCLUDED */
