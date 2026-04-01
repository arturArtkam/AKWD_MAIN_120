/* "Copyright 2022 <Copyright artkam>" */

#include "../inc/akwd.h"
//#include "../inc/protokol.h"

namespace
{

#include "struct_meta.h"

}

namespace akwd
{

void uart_thread()
{
    pc_uart.event.wait();

    if (pc_uart.event.check_source(Uart_event_id::RX_EVENT))
    {
//        leds.red_on();
        volatile uint8_t cmd = pc_uart.buf[0];

        switch (cmd)
        {
            //если команда чтения метаданных
            case Pc1w::Protocol::CMD_META:
//                pc_uart.build_tx_packet(cmd, cmetaAll, sizeof(cmetaAll));
//                pc_uart.start_tx_packet_send();
                break;

            case Pc1w::Protocol::CMD_INFO:
            {
                uint8_t& size = pc_uart.buf[1];

                if (pc_uart.get_count() == 6)
                {
                    uint16_t& offset = *reinterpret_cast<uint16_t* >(&pc_uart.buf[2]);

                    pc_uart.ext_packet_init(size);
                    pc_uart.push_ext_packet_head(Pc1w::Protocol::CMD_INFO);
                    pc_uart.push_ext_body_piece(&cmetaAll[offset], size);
                }
                else
                {
                    pc_uart.ext_packet_init(size);
                    pc_uart.push_ext_packet_head(Pc1w::Protocol::CMD_INFO);
                    pc_uart.push_ext_body_piece(cmetaAll, size);
                }

                pc_uart.start_ext_packet_send(nullptr);
            }
                break;

            case Pc1w::Protocol::CMD_TXRX_SYNC:
            {
                ext_trigger.pc1w_sync();
            }
                break;

            case Pc1w::Protocol::CMD_R_INFORM:
                /* "только время" запрос */
                if (pc_uart.get_count() == 4)
                {
                    enum{ SIZE = sizeof(DataStructW_t::automat) + sizeof(DataStructW_t::Time) };

                    workdata.vault.automat = fsm.get_corrected();
                    workdata.vault.Time = c_timer.get();

                    pc_uart.ext_packet_init(SIZE);
                    pc_uart.push_ext_packet_head(Pc1w::Protocol::CMD_R_INFORM);
                    pc_uart.push_ext_body_piece(&workdata.vault, SIZE);
                    pc_uart.start_ext_packet_send(&workdata.mtx);
                }
                else if (pc_uart.get_count() == 5) /* длина для режима полной информации */
                {
                    if (fsm.get() != Fsm::APP_WORK)
                    {
                        pwr.turn_on(true);
                        workdata.vault.automat = fsm.get_corrected(true);
                        workdata.vault.Time = c_timer.get();
                        fill_datastruct(&workdata.vault);

                        enum{ SIZE = sizeof(DataStructW_t::automat) + sizeof(DataStructW_t::Time) };

                        pc_uart.ext_packet_init(sizeof(DataStructW_t));
                        pc_uart.push_ext_packet_head(Pc1w::Protocol::CMD_R_INFORM);
                        pc_uart.push_ext_body_piece(&workdata.vault.automat, SIZE);
                        pc_uart.push_ext_body_piece(&workdata.vault.SYNC_RECIEVER, sizeof(sync_rx_t));
                        pc_uart.push_ext_body_piece(&workdata.vault.AKWD_RX.T, 6 * sizeof(float) + sizeof(accel_t) + sizeof(uint16_t));
                        pc_uart.push_ext_body_piece(&workdata.vault.AKWD_RX.SENS_SHORT, sizeof(sens_array_t));
                        pc_uart.push_ext_body_piece(&workdata.vault.AKWD_RX.SENS_LONG, sizeof(sens_array_t));
                //        uart->push_ext_body_piece(&_vault.AKWD_TX, sizeof(akwd_tx_t));

                        pc_uart.start_ext_packet_send(&workdata.mtx);
                    }
                }

                break;

            case Pc1w::Protocol::CMD_TIME:
            {
                //принятые данные об установке времени
                int32_t time_info = *reinterpret_cast<int32_t* >(&pc_uart.buf[1]);

                if (time_info < 0)
                {
                    if (fsm.get() == Fsm::APP_IDLE)
                    {
                        fsm.set(Fsm::APP_SET_TIME);
                    }
                    //задержка в кадрах
                    c_timer.set(time_info);
                }
                else if (time_info == 0)
                {
                    fsm.set(Fsm::APP_IDLE);
                    pwr.shutdown();
                    sd_card.mux.to_comp();
                    NVIC_SystemReset();
                }
            }
                break;

            /* bootloader */
            case Pc1w::Protocol::CMD_BOOT:
            {
//                if (pc_uart.buf[1] == 0x00)
//                {
                NVIC_SystemReset();
//                }
            }
                break;

            case Pc1w::Protocol::CMD_READ_EE:
                eeprom.read_fields(pc_uart.buf);

                pc_uart.ext_packet_init(sizeof(Fram_markup::eeprom.vars));
                pc_uart.push_ext_packet_head(Pc1w::Protocol::CMD_READ_EE);
                pc_uart.push_ext_body_piece(pc_uart.buf, sizeof(Fram_markup::eeprom.vars));
                pc_uart.start_ext_packet_send(nullptr);
                break;

            case Pc1w::Protocol::CMD_WRITE_EE:
                eeprom.write_entry(&pc_uart.buf[3], 0, sizeof(Fram_markup::eeprom.vars));

                pc_uart.ext_packet_init(0);
                pc_uart.push_ext_packet_head(Pc1w::Protocol::CMD_WRITE_EE);
                pc_uart.start_ext_packet_send(nullptr);
                break;

            default:
                break;

        } /* switch (cmd) */
    }
    else if (pc_uart.event.check_source(Uart_event_id::TX_EVENT))
    {
//        leds.red_off();
    }
} /* void uart_thread() */

}  /* namespace akwd */
