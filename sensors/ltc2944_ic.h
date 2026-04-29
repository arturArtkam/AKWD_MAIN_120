#pragma once
#ifndef LTC2944_HPP
#define LTC2944_HPP

#include "iic_sw.h"

//#pragma GCC diagnostic push
//#pragma GCC diagnostic ignored "-Waggregate-return"
//
//#pragma GCC diagnostic pop

template<class iic>
class Ltc2944
{
private:
    //automatic mode, 4096 prescaler
    uint8_t ctrl_reg[5] = {0x01, 0b11000000, 0x00, 0x00, 0x00};

    struct regStat_t
    {
        uint8_t underVoltage_lock : 1;
        uint8_t voltage_alert : 1;
        uint8_t charge_alert_lo : 1;
        uint8_t charge_alert_hi : 1;
        uint8_t temp_alert : 1;
        uint8_t acc_charge : 1;
        uint8_t current_alert : 1;
        uint8_t reserved : 1;
    };

    struct regCtrl_t
    {
        uint8_t shdn : 1;
        uint8_t alcc_conf : 2;
        uint8_t prescaler : 3;
        uint8_t adc_mode : 2;
    };

    #pragma pack(push, 1)
    struct
    {
        regStat_t status;
        regCtrl_t ctrl;
        uint16_t  acc_charge;
        uint16_t  charge_treshold_hi;
        uint16_t  charge_treshold_lo;
        uint16_t  voltage;
        uint16_t  voltage_treshold_hi;
        uint16_t  voltage_treshold_lo;
        uint16_t  current;
        uint16_t  current_treshold_hi;
        uint16_t  current_treshold_lo;
        uint16_t  temp_sens;
        uint8_t   temp_treshold_hi;
        uint8_t   temp_treshold_lo;
    } _regList;
    #pragma pack(pop)

    enum reg_map : uint8_t
    {
        Status = 0x00,
        Control = 0x01,
        Accumulated_Charge_MSB = 0x02,
        Accumulated_Charge_LSB = 0x03,
        Charge_Threshold_High_MSB = 0x04,
        Charge_Threshold_High_LSB = 0x05,
        Charge_Threshold_Low_MSB = 0x06,
        Charge_Threshold_Low_LSB = 0x07,
        Voltage_MSB = 0x08,
        Voltage_LSB = 0x09,
        Voltage_Threshold_High_MSB = 0x0A,
        Voltage_Threshold_High_LSB = 0x0B,
        Voltage_Threshold_Low_MSB = 0x0C,
        Voltage_Threshold_Low_LSB = 0x0D,
        Current_MSB = 0x0E,
        Current_LSB = 0x0F,
        Current_Threshold_High_MSB = 0x10,
        Current_Threshold_High_LSB = 0x11,
        Current_Threshold_Low_MSB = 0x12,
        Current_Threshold_Low_LSB = 0x13,
        Temperature_MSB = 0x14,
        Temperature_LSB = 0x15,
        Temperature_Threshold_High = 0x16,
        Temperature_Threshold_Low = 0x17
    };

    enum ic_errors
    {
        ERR_NONE = 0,
        ERR_UNDER_VOLTAGE_BIT = 1
    };

    enum
    {
        I2C_ADR = 0xC8,
        R_SENS_mOHM = 50
    };

    iic* _i2c_bus;

public:
    struct outcome
    {
        float val = 0;
        ic_errors err = ERR_NONE;
    };

    Ltc2944(iic* const iic_bus) :
        _regList(),
        _i2c_bus(iic_bus)
    {
        memset(&_regList, 0, sizeof(_regList));

        _regList.ctrl.shdn = 0;
        _regList.ctrl.alcc_conf = 0;
        _regList.ctrl.prescaler = 0;
        _regList.ctrl.adc_mode = 3;

        ctrl_reg[0] = 0x01;
        ctrl_reg[1] = *(reinterpret_cast<uint8_t* >(&_regList.ctrl));

        read_registers();

        if (_regList.charge_treshold_hi == 0xFFFF)
        {
            write_charge_cnt(0.0f);
        }
        //_i2c_bus->write_buf(I2C_ADR, ctrl_reg, 2);
    }

    void read_registers()
    {
        const uint8_t status_reg_addr = reg_map::Status;
        uint8_t data_len = 1;

        volatile uint8_t ctrl = get_ctrl();

        if (ctrl != 0b11000000)
        {
            init();
        }

        if (_i2c_bus->write_buf(I2C_ADR, &status_reg_addr, data_len) == _i2c_bus->RESULT_SUCCESS)
        {
            _i2c_bus->read_buf(I2C_ADR, (uint8_t* )&_regList.status, sizeof(_regList));

            _regList.status.underVoltage_lock = 0;

            /* Если переполнение регистра счетчика заряда. */
            if (_regList.acc_charge == 0xFFFF)
            {
                /* rst_charge_cnt(); */
                /* Дли расширения диапазона измерений используется дооплнительный регистр. */
                uint16_t tpm = __REV16(_regList.charge_treshold_hi);
                tpm++;

                ctrl_reg[0] = 0x02;
                ctrl_reg[1] = 0;
                ctrl_reg[2] = 0;
                *(uint16_t* )&ctrl_reg[3] = __REV16(tpm);

                data_len = 5;

                if (_i2c_bus->write_buf(I2C_ADR, (uint8_t* )&ctrl_reg[0], data_len) == _i2c_bus->RESULT_SUCCESS)
                {
                    ;
                }
            }
        }

        if (_regList.charge_treshold_hi == 0xFFFF)
        {
            write_charge_cnt(0.0f);
        }
    }

    void write_charge_cnt(float val)
    {
        ctrl_reg[0] = 0x02;

        *(uint32_t* )&ctrl_reg[1] = __REV16( static_cast<uint32_t>( (val * 128.0f) / (0.085f * (float)(_regList.ctrl.prescaler + 1)) ) );

        if (_i2c_bus->write_buf(I2C_ADR, (uint8_t* )&ctrl_reg[0], 5) == _i2c_bus->RESULT_SUCCESS)
        {
            ;
        }
    }

    uint8_t get_ctrl()
    {
        const uint8_t ctrl_reg_addr = reg_map::Control;
        const uint8_t data_len = 1;
        uint8_t ctrl_reg_val = 0;

        if (_i2c_bus->write_buf(I2C_ADR, &ctrl_reg_addr, data_len) == _i2c_bus->RESULT_SUCCESS)
        {
            _i2c_bus->read_buf(I2C_ADR, &ctrl_reg_val, data_len);
        }

        return ctrl_reg_val;
    }

    void init()
    {
        memset(&_regList, 0, sizeof(_regList));

        _regList.ctrl.shdn = 0;
        _regList.ctrl.alcc_conf = 0;
        _regList.ctrl.prescaler = 0;
        _regList.ctrl.adc_mode = 3;

        ctrl_reg[0] = 0x01;
        ctrl_reg[1] = *(reinterpret_cast<uint8_t* >(&_regList.ctrl));

        _i2c_bus->write_buf(I2C_ADR, ctrl_reg, 2);
        //_i2c_bus->write_buf(I2C_ADR, ctrl_reg, 5);
    }

    float get_temperature()
    {
        uint16_t temperature = 0;

        if (_regList.status.underVoltage_lock == 0)
        {
            temperature = __REV16(_regList.temp_sens);

            return static_cast<float>(510 * temperature / 65535u) - 273.0f;
        }
        else
        {
            return -273.0f;
        }
    }

    outcome get_voltage()
    {
        uint16_t voltage = 0;
        outcome out_voltage;

        if (_regList.status.underVoltage_lock == 0)
        {
            voltage = __REV16(_regList.voltage);

            out_voltage.err = ERR_NONE;
            out_voltage.val = (70.8f * static_cast<float>(voltage)) / 65535.0f;
        }
        else
        {
            out_voltage.err = ERR_UNDER_VOLTAGE_BIT;
            out_voltage.val = 0;
        }

        return out_voltage;
    }


    outcome get_current()
    {
        outcome out;

        if (_regList.status.underVoltage_lock == 0)
        {
            uint16_t  current = __REV16(_regList.current);

            out.err = ERR_NONE;
            out.val = (64.0f / float(R_SENS_mOHM)) * (float(current - 32767) / 32767.0f);
        }
        else
        {
            out.err = ERR_UNDER_VOLTAGE_BIT;
            out.val = 0.0f;
        }

        return out;
    }

    outcome get_current(uint32_t poll_period)
    {
        outcome out;
        float q_lsb = 0.34f * 50.0f / (float)R_SENS_mOHM * float((_regList.ctrl.prescaler + 1) / 4096);

        if (_regList.status.underVoltage_lock == 0)
        {
            uint16_t  charge = __REV16(_regList.acc_charge);
            write_charge_cnt(0.0f);

            out.err = ERR_NONE;
            out.val = (int32_t)((float)charge * q_lsb) * (3600 / poll_period);
        }
        else
        {
            out.err = ERR_UNDER_VOLTAGE_BIT;
            out.val = 0.0f;
        }

        return out;
    }
};

#endif /* LTC2944_HPP */
