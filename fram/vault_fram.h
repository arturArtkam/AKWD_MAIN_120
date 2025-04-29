#pragma once
#ifndef VAULT_H
#define VAULT_H

#include "scmRTOS.h"
#include "mb85_ic.h"
#include "fram_markup.h"
#include "../inc/observer.h"
#include "../../../Lib/pin.h"

class Fram_ic
{
private:
    typedef pin<PORTB, 9, GPIO_Mode_OUT, GPIO_Speed_2MHz> _Fram_cs_pin;
    typedef hw_spi<SPI3_BASE, spi3_miso, spi3_mosi, spi3_clk> _Fram_spi;

public:
    typedef ic_mb85<Fram_markup::FRAM_SIZE, _Fram_spi, _Fram_cs_pin> Fram;
};

class Fram_vault final : public Fram_ic
{
    OS::TMutex _fram_mutex;

public:
    INLINE void init();
    INLINE void read_fields(void* data);
    INLINE void read_entry(void* data, uint32_t offset, size_t lenght);
    INLINE void write_entry(const void* data, uint32_t offset, size_t lenght);
};

void Fram_vault::init()
{
    Fram::init();
}

void Fram_vault::read_fields(void* data)
{
    Fram::read_buf(data, offsetof(Fram_markup, Fram_markup::eeprom), sizeof(Fram_markup::eeprom));
}

void Fram_vault::read_entry(void* data, uint32_t offset, size_t lenght)
{
    _fram_mutex.lock();

    Fram::read_buf(data, offsetof(Fram_markup, Fram_markup::eeprom) + offset, lenght);

    _fram_mutex.unlock();
}

void Fram_vault::write_entry(const void* data, uint32_t offset, size_t lenght)
{
    _fram_mutex.lock();

    Fram::enable_write();

    Fram::write_buf(data, offsetof(Fram_markup, Fram_markup::eeprom) + offset, lenght);

    Fram::disable_write();

    _fram_mutex.unlock();
}

class Ifram_var
{
protected:
    ~Ifram_var() = default;

public:
    virtual std::size_t serialize(bool (*fram_write)(void const*, uint32_t, uint32_t), size_t& offset) = 0;
    virtual std::size_t deserialize(bool (*fram_read)(void*, uint32_t, uint32_t), size_t& offset) = 0;
    virtual std::size_t serialized_size() const = 0;
};

/* Циклически сохраняемые переменные*/
class Fram_vars_manager final : public Observable<Ifram_var, 8>, private Fram_ic
{
public:
    Fram_vars_manager()
    {

    }

    void save_variables()
    {
//        size_t offset = offsetof(EepData_t, EepData_t::OPERATIONAL_TIME);
//
//        for (auto each_var : _observers)
//        {
//            if (each_var != nullptr)
//            {
//                each_var->serialize(Fram::write_buf, offset);
//            }
//        }
//
//        Fram::disable_write();
    }

    void restore_variables()
    {
//        size_t offset = offsetof(EepData_t, EepData_t::OPERATIONAL_TIME);
//
//        for (auto each_var : _observers)
//        {
//            if (each_var != nullptr)
//                each_var->deserialize(Fram::read_buf, offset);
//        }
    }
};


#endif /* VAULT_H */
