#include "../inc/akwd.h"

namespace akwd
{
/*
 *     Функция вызывается из потока обработки состояния USB VBUS.
 * Эта функция отвечает за мониторинг состояния USB VBUS и выполнение соответствующих действий
 * при подключении или отключении USB. В зависимости от текущего состояния USB и приложения (FSM),
 * она управляет питанием SD-карты и напрвлением потока данных к ней.
 */

void usbvbus_thread()
{
    auto& ubs_vbus = Usb_bus::instance();

    if (ubs_vbus.read() == Usb_bus::USB_CONN)
    {
        if (ubs_vbus.usb_plug_flg == Usb_bus::USB_DISCONN)
        {
            if (fsm.get() == Fsm::APP_IDLE)
            {
                Sd_disk::Pwr_key::hi();
                sd_card.mux.to_comp();
            }

            ubs_vbus.usb_plug_flg = Usb_bus::USB_CONN;
        }
    }
    else
    {
        if (ubs_vbus.usb_plug_flg == Usb_bus::USB_CONN)
        {
            if (fsm.get() == Fsm::APP_IDLE)
            {
                sd_card.mux.to_mcu();
            }

            ubs_vbus.usb_plug_flg = Usb_bus::USB_DISCONN;
        }
    }
    /* Разряд емкостей, подключенных к USB VBUS.*/
    ubs_vbus.discharge_capacitance();
}

} /* namespace akwd */
