#ifndef I2C_HPP
#define I2C_HPP

template <class scl, class sda>
class IIC_sw
{
public:
    enum result_t : int8_t
    {
        RESULT_SUCCESS = 0,
        RESULT_ERROR = -1
    };

private:
    __INLINE_ALWAYS void delay()
    //__attribute__ ((noinline)) void delay()
    {
        volatile auto i = 10;
        //#warning "kgkvkbhjk!"
        while(i)
        {
            i--;
        }
    }

    __INLINE_ALWAYS bool start()
    {
        scl::hi();						// отпустить обе линии, на случай
        sda::hi();						// на случай, если они были прижаты

        delay();

        if (!sda::read())			// если линия SDA прижата слейвом,
            return false;			// то сформировать старт невозможно, выход с ошибкой

        sda::lo();						// прижимаем SDA к земле
        delay();

        if (sda::read())				// если не прижалась, то шина неисправна
            return false;			// выход с ошибкой

        delay();

        return true;				// старт успешно сформирован
    }
    // последовательность для формирования Стопа
    void stop()
    {
        scl::lo();
        delay();
        sda::lo();
        delay();
        scl::hi();
        delay();
        sda::hi();
        delay();
    }

    void ack()
    {
        scl::lo();
        delay();
        sda::lo();						// прижимаем линию SDA к земле
        delay();
        scl::hi();						// и делаем один клик линием SCL
        delay();
        scl::lo();
        delay();
    }
    //Отправка последовательности NO ACK в шину
    void no_ack()	//
    {
        scl::lo();
        delay();
        sda::hi();						// отпускаем линию SDA
        delay();
        scl::hi();						// и делаем один клик линием SCL
        delay();
        scl::lo();
        delay();
    }
    // проверка шины на наличие ACK от слейва
    bool wait_ack()
    {
        scl::lo();
        delay();
        sda::hi();
        delay();
        scl::hi();						// делаем половину клика линией SCL
        delay();
        if (sda::read())
        {			// и проверяем, прижал ли слейв линию SDA
            scl::lo();
            return false;
        }
        scl::lo();						// завершаем клик линией SCL
        return true;
    }

public:
    IIC_sw()
    {
        scl::init();
        sda::init();

        scl::hi();
        sda::hi();
    }

    void put_byte(uint8_t data)
    {
        uint_fast8_t i = 8;				// нужно отправить 8 бит данных
        while (i--)
        {				// пока не отправили все биты
            scl::lo();					// прижимаем линию SCL к земле
            delay();

            if ( data & 0x80 )		// и выставляем на линии SDA нужный уровень
                sda::hi();
            else
                sda::lo();

            data <<= 1;

            delay();

            scl::hi();					// отпускаем линию SCL
            delay();		// после этого слейв сразу же прочитает значение на линии SDA
        }

        scl::lo();
    }

    uint8_t get_byte()
    {
        volatile auto i = 8;		// нужно отправить 8 бит данных
        uint8_t data = 0;

        sda::hi();						// отпускаем линию SDA. управлять ей будет слейв
        while (i--)
        {				// пока не получили все биты
            data <<= 1;
            scl::lo();					// делаем клик линией SCL
            delay();
            scl::hi();
            delay();

            if (sda::read())
            {		// читаем значение на линии SDA
                data |= 0x01;
            }
        }
        scl::lo();
        return data;				// возвращаем прочитанное значение
    }

    result_t write_buf(uint8_t chipAddress, uint8_t const* buffer, uint32_t sizeOfBuffer)
    {
        if (!start())
            return RESULT_ERROR;

        put_byte(chipAddress);
        if (!wait_ack())
        {
            stop();
            return RESULT_ERROR;
        }

        while (sizeOfBuffer != 0)
        {
            put_byte(*buffer);
            if (!wait_ack())
            {
                stop();
                return RESULT_ERROR;
            }

            buffer++;
            sizeOfBuffer--;
        }
        stop();
        return RESULT_SUCCESS;
    }

    result_t read_buf(uint8_t chipAddress, uint8_t* buffer, uint32_t sizeOfBuffer)
    {
        if (!start())
            return RESULT_ERROR;

        put_byte(chipAddress + 1);

        if (!wait_ack())
        {
            stop();
            return RESULT_ERROR;
        }

        while (sizeOfBuffer != 0)
        {
            *buffer = get_byte();

            buffer++;
            sizeOfBuffer--;
            if (sizeOfBuffer == 0)
            {
                no_ack();
                break;
            }
            else
                ack();
        }

        stop();

        return RESULT_SUCCESS;
    }
};

#endif // I2C_HPP
