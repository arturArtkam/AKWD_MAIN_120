#ifndef I2C_HPP
#define I2C_HPP

template <class scl, class sda>
class I2C_sw
{
public:
    enum result_t : int8_t
    {
        RESULT_SUCCESS = 0,
        RESULT_ERROR = -1,
        RESULT_TIMEOUT = -2
    };

private:
    static constexpr uint32_t TIMEOUT = 10000;

    static inline void delay()
    {
        for (volatile int i = 0; i < 10; ++i)
            __NOP();
    }

    // Ждём, пока SCL реально станет HIGH (clock stretching)
    static bool wait_scl_high()
    {
        uint32_t timeout = TIMEOUT;
        while (!scl::read())
        {
            if (--timeout == 0)
                return false;
        }
        return true;
    }

    static bool bus_free()
    {
        return scl::read() && sda::read();
    }

    // START или REPEATED START
    static bool start()
    {
        sda::hi();
        scl::hi();

        if (!wait_scl_high())
            return false;

        if (!sda::read())
            return false;

        delay();

        sda::lo();
        delay();

        scl::lo();
        return true;
    }

    static void stop()
    {
        sda::lo();
        delay();

        scl::hi();
        wait_scl_high();
        delay();

        sda::hi();
        delay();
    }

    static bool write_bit(bool bit)
    {
        if (bit)
            sda::hi();
        else
            sda::lo();

        delay();

        scl::hi();
        if (!wait_scl_high())
            return false;

        delay();

        scl::lo();
        return true;
    }

    static bool read_bit(bool &bit)
    {
        sda::hi(); // release line

        delay();

        scl::hi();
        if (!wait_scl_high())
            return false;

        delay();

        bit = sda::read();

        scl::lo();
        return true;
    }

    static bool write_byte(uint8_t data)
    {
        for (int i = 0; i < 8; ++i)
        {
            if (!write_bit(data & 0x80))
                return false;

            data <<= 1;
        }

        // Чтение ACK
        bool ack = false;
        if (!read_bit(ack))
            return false;

        return (ack == 0);
    }

    static bool read_byte(uint8_t &data, bool send_ack)
    {
        data = 0;

        for (int i = 0; i < 8; ++i)
        {
            bool bit;
            if (!read_bit(bit))
                return false;

            data = (data << 1) | bit;
        }

        // ACK/NACK
        if (!write_bit(!send_ack))
            return false;

        return true;
    }

public:
    I2C_sw()
    {
        scl::init();
        sda::init();

        scl::hi();
        sda::hi();
    }

    result_t write_buf(uint8_t addr, const uint8_t* data, uint32_t size)
    {
        if (!start())
            return RESULT_ERROR;

        if (!write_byte((addr << 1) | 0))
        {
            stop();
            return RESULT_ERROR;
        }

        while (size--)
        {
            if (!write_byte(*data++))
            {
                stop();
                return RESULT_ERROR;
            }
        }

        stop();
        return RESULT_SUCCESS;
    }

    result_t read_buf(uint8_t addr, uint8_t* data, uint32_t size)
    {
        if (!start())
            return RESULT_ERROR;

        if (!write_byte((addr << 1) | 1))
        {
            stop();
            return RESULT_ERROR;
        }

        while (size--)
        {
            bool ack = (size != 0);

            if (!read_byte(*data++, ack))
            {
                stop();
                return RESULT_ERROR;
            }
        }

        stop();
        return RESULT_SUCCESS;
    }

    // Комбинированная операция (очень важно для регистров устройств!)
    result_t write_read(uint8_t addr,
                        const uint8_t* wdata, uint32_t wsize,
                        uint8_t* rdata, uint32_t rsize)
    {
        if (!start())
            return RESULT_ERROR;

        if (!write_byte((addr << 1) | 0))
        {
            stop();
            return RESULT_ERROR;
        }

        while (wsize--)
        {
            if (!write_byte(*wdata++))
            {
                stop();
                return RESULT_ERROR;
            }
        }

        // REPEATED START
        if (!start())
            return RESULT_ERROR;

        if (!write_byte((addr << 1) | 1))
        {
            stop();
            return RESULT_ERROR;
        }

        while (rsize--)
        {
            bool ack = (rsize != 0);

            if (!read_byte(*rdata++, ack))
            {
                stop();
                return RESULT_ERROR;
            }
        }

        stop();
        return RESULT_SUCCESS;
    }
};

#endif // I2C_HPP
