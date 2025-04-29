#ifndef USB_VBUS_H_INCLUDED
#define USB_VBUS_H_INCLUDED

#include "pin.h"

class Usb_bus final
{
    typedef pin<PORTA, 11, GPIO_Mode_IN, GPIO_Speed_2MHz, GPIO_OType_PP, GPIO_PuPd_NOPULL> _Vbus_pin;

public:
    typedef enum : uint8_t
    {
        USB_DISCONN = 1,
        USB_CONN
    } Stat;

    Stat usb_plug_flg;

private:
    Usb_bus() :
        usb_plug_flg(USB_DISCONN)
    {
        _Vbus_pin::init();
    }

public:
    Usb_bus(const Usb_bus&) = delete;
    Usb_bus& operator=(const Usb_bus &) = delete;

public:
    static auto& instance()
    {
        static Usb_bus usb_bus;
        return usb_bus;
    }

    INLINE void discharge_capacitance();
    INLINE Stat read();
};

/* Usb_bus class */

void Usb_bus::discharge_capacitance()
{
    _Vbus_pin::pin_mode(GPIO_Mode_OUT);

    _Vbus_pin::lo();
    OS::sleep(50);

    _Vbus_pin::hi();
    _Vbus_pin::pin_mode(GPIO_Mode_IN);
    OS::sleep(50);
}

Usb_bus::Stat Usb_bus::read()
{
    if (_Vbus_pin::read())
    {
        return USB_CONN;
    }
    else
    {
        return USB_DISCONN;
    }
}

#endif /* USB_VBUS_H_INCLUDED */
