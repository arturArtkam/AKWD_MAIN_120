#ifndef AIS2IH_IC_H
#define AIS2IH_IC_H

//								           default
#define AIS_OUT_T_L			0x0D // R 00001101 00000000 Temp sensor output
#define AIS_OUT_T_H			0x0E // R 00001110 00000000
#define AIS_WHO_AM_I		0x0F // R 00001111 01000100 Who am I ID
// RESERVED - 10-1F - RESERVED
#define AIS_CTRL1			0x20 //R/W 00100000 00000000 Control registers
#define AIS_CTRL2			0x21 //R/W 00100001 00000100

#define CTRL2_BOOT			(1<<7)
#define CTRL2_RESET			(1<<6)
//#define CTRL2_BOOT (1<<5)
#define CTRL2_CS_PU_DISC	(1<<4)
#define CTRL2_BDU			(1<<3)
#define CTRL2_IF_ADD_INC	(1<<2)
#define CTRL2_I2C_DISABLE	(1<<1)
#define CTRL2_SIM			(1<<0)

#define AIS_CTRL3			0x22 //R/W 00100010 00000000
#define AIS_CTRL4_INT1_PAD_CTRL 0x23 //R/W 00100011 00000000
#define AIS_CTRL5_INT2_PAD_CTRL 0x24 //R/W 00100100 00000000

#define AIS_CTRL6			0x25 //R/W 00100101 00000000


#define AIS_OUT_T			0x26 //R 00100110 00000000 Temp sensor output
#define AIS_STATUS			0x27 //R 00100111 00000000 Status data register
// Output registers
#define AIS_OUT_X_L			0x28 // R00101000 00000000
#define AIS_OUT_X_H			0x29 //R 00101001 00000000
#define AIS_OUT_Y_L			0x2A //R 00101010 00000000
#define AIS_OUT_Y_H			0x2B //R 00101011 00000000
#define AIS_OUT_Z_L			0x2C //R 00101100 00000000
#define AIS_OUT_Z_H			0x2D //R 00101101 00000000

#define AIS_MODE_HI	1

typedef struct
{
	uint8_t reserv: 2;
	uint8_t lowNoise: 1;
	uint8_t fds: 1;
	uint8_t fs: 2;
	uint8_t bw: 2;
} ctrl6_t;

enum
{
	BW_ODR_DIV_2 = 0,
	BW_ODR_DIV_4 = 1,
	BW_ODR_DIV_10 = 2,
	BW_ODR_DIV_20 = 3,
};
enum
{
	FS_2g = 0,
	FS_4g = 1,
	FS_8g = 2,
	FS_16g = 3,
};

typedef struct
{
	uint8_t lpmode: 2;
	uint8_t mode: 2;
	uint8_t odr: 4;
} ctrl1_t;

enum
{
	ODR_12HZ = 2,
	ODR_25HZ = 3,
	ODR_50HZ = 4,
	ODR_100HZ = 5,
	ODR_200HZ = 6,
};

template<class spi, class cs_pin>
class Ais2ih_ic
{
public:
    typedef enum
    {
        MAIN_ID     = 0x01,
        PART_ID     = 0x02,
        XOUT_L      = 0x08,
        XOUT_H,
        YOUT_L,
        YOUT_H,
        ZOUT_L,
        ZOUT_H,
        COTR        = 0X12,
        WHO_AM_I    = 0x13,
        INS1        = 0x16,
        INS2        = 0x17,
        INS3        = 0x18,
        CNTL1       = 0x1B,
        CNTL2,
        CNTL3,
        CNTL4,
        CNTL5,
        CNTL6,
        ODCNTL      = 0X21
    } addr;

    struct xyz_data
    {
        int16_t a_x;
        int16_t a_y;
        int16_t a_z;
    } __attribute__((packed, aligned(1)));

public:
    Ais2ih_ic()
    {
        cs_pin::init();
        spi::init();

        cs_pin::hi();
    }

    void spibus_conf(uint16_t cpol, uint16_t cpha, uint16_t baud_presc)
    {
        Ais2ih_ic();
        spi::mode_full_duplex_8t(cpol, cpha, baud_presc);
    }

    void write_reg(uint8_t adr, uint8_t data)
    {
        cs_pin::lo();

        __NOP();
        __NOP();
        __NOP();

        spi::exchange_8t(adr);
        spi::exchange_8t(data);

        __NOP();
        __NOP();
        __NOP();

        cs_pin::hi();
    }

    uint8_t read_8b(uint8_t adr)
    {
        uint8_t tmp = 0;

        cs_pin::lo();

        __NOP();
        __NOP();
        __NOP();

        /* Для чтения регистров необходимо выставить старший бит в адресе. */
//        if (!spi::exchange_8t(0b10000000 | adr).err)
        spi::exchange_8t(0b10000000 | adr);
        {
            tmp = spi::exchange_8t();
        }

        __NOP();
        __NOP();
        __NOP();

        cs_pin::hi();

        return tmp;
    }

    int16_t read_16b(uint8_t axis)
    {
        union
        {
            int16_t out;
            uint8_t p[2];
        };

        cs_pin::lo();

        //программирование режима работы (чтение с автоинкрементом)
//        if (!spi::exchange_8t(0x80 | axis).err)
        spi::exchange_8t(0x80 | axis);
        {
            //1й байт данных
            p[0] = spi::exchange_8t();
            //2й байт данных
            p[1] = spi::exchange_8t();
        }

        cs_pin::hi();

        return out;
    }

    bool check_whoiam()
    {
        return read_8b(AIS_WHO_AM_I) == 0x44;
    }
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wpedantic"
	bool init()	{		uint8_t res = read_8b(AIS_WHO_AM_I);
		ctrl1_t c1 = {.lpmode = 0,.mode = AIS_MODE_HI,.odr = ODR_12HZ};		write_reg(AIS_CTRL1, *(uint8_t*) &c1);		write_reg(AIS_CTRL2, CTRL2_CS_PU_DISC|CTRL2_BDU|CTRL2_IF_ADD_INC|CTRL2_I2C_DISABLE);		ctrl6_t c6 = {.reserv = 0,.lowNoise = 1,.fds = 0,.fs = FS_2g,.bw = BW_ODR_DIV_20};		write_reg(AIS_CTRL6, *(uint8_t*) &c6);		return res == 0x44;	}
    #pragma GCC diagnostic pop
	int8_t read_T(void)	{		int8_t r = read_8b(AIS_OUT_T);		return 25+r;	}

    auto read_xyz()
    {
        xyz_data  out{};

        out.a_x = read_16b(AIS_OUT_X_L);
        out.a_y = read_16b(AIS_OUT_Y_L);
        out.a_z = read_16b(AIS_OUT_Z_L);

        return out;
    }
};

#endif /* AIS2IH_IC_H */
