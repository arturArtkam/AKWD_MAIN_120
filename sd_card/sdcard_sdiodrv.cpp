#include "sdcard_sdiodrv.h"

sd_result_t sdio_drv_t::init()
{
    uint32_t trials = 0;
    uint32_t response[4] = {0, 0, 0, 0};
    uint32_t sd_type = SD_STD_CAPACITY;
    uint32_t tempreg = 0;
    sd_result_t cmd_res = SDR_Success;

    // Populate SDCard structure with default values
    _card_info.Capacity = 0;
    _card_info.MaxBusClkFreq = 0;
    _card_info.BlockSize = 0;
    _card_info.CSDVer = 0;
    _card_info.Type = SDCT_UNKNOWN;
    _card_info.RCA = 0;

    _d0_pin::init();
    _clk_pin::init();
    _cmd_pin::init();

    rcc_disable();
    rcc_conf();

    //Initialize the SDIO (with initial <400Khz Clock)
    tempreg |= SDIO_CLKCR_CLKEN;  //Clock is enabled
    tempreg |= clk_div_init();  //Clock Divider. Clock=48000/(118+2)=400Khz
    //Keep the rest at 0 => HW_Flow Disabled, Rising Clock Edge, Disable CLK ByPass, Bus Width=0, Power save Disable
    SDIO->CLKCR = tempreg;

    SDIO->POWER = pwr_on(); // Enable SDIO clock

    cmd_res = send_cmd(SD_CMD_GO_IDLE_STATE, 0x00, SDIO_RESP_NONE);

    // CMD8: SEND_IF_COND. Send this command to verify SD card interface operating condition
    // Argument: - [31:12]: Reserved (shall be set to '0')
    //           - [11:8]: Supply Voltage (VHS) 0x1 (Range: 2.7-3.6 V)
    //           - [7:0]: Check Pattern (recommended 0xAA)
    cmd_res = send_cmd(SD_CMD_HS_SEND_EXT_CSD, check_pattern(), SDIO_RESP_SHORT); // CMD8

    /* SD v2.0 or later */
    if (cmd_res == SDR_Success)
    {
        // Get and check R7 response
        if (sd_response(SD_R7, response) != SDR_Success)
        {
            return SDR_BadResponse;
        }

        /* Check echo-back of check pattern */
        if ((response[0] & 0x01ff) != (check_pattern() & 0x01ff))
        {
            return SDR_Unsupported;
        }

        sd_type = SD_HIGH_CAPACITY; // SD v2.0 or later

        // Issue ACMD41 command
        trials = SDIO_ACMD41_TRIALS();
        /*
            Периодически выдаем ACMD41 по крайней мере в течение 1 секунды до тех пор,
            пока 31й бит (бит занятости)  в response[0] не будет установлен в 1.
        */
        while (--trials)
        {
            /* Send leading command for ACMD<n> command */
            send_cmd(SD_CMD_APP_CMD, 0, SDIO_RESP_SHORT); // CMD55 with RCA 0

            if (sd_response(SD_R1, response) != SDR_Success)
            {
                return SDR_BadResponse;
            }
            // ACMD41 - initiate initialization process.
            // Set 3.0-3.3V voltage window (bit 20)
            // Set HCS bit (30) (Host Capacity Support) to inform card what host support high capacity
            // Set XPC bit (28) (SDXC Power Control) to use maximum performance (SDXC only)
            //send_cmd(SD_CMD_SD_SEND_OP_COND(), ocr_voltage() | sd_type, SDIO_RESP_SHORT);
            send_cmd(SD_CMD_SD_SEND_OP_COND(), SD_OCR_VOLTAGE | sd_type, SDIO_RESP_SHORT);

            if (sd_response(SD_R3, response) != SDR_Success)
            {
                return SDR_BadResponse;
            }

            /* Check if card finished power up routine */
            if (response[0] & (1 << 31))
            {
                break;
            }
        }

        if (!trials)
        {
            return SDR_InvalidVoltage; // Unsupported voltage range
        }
        // Check if card is SDHC/SDXC
        _card_info.Type = (response[0] & SD_HIGH_CAPACITY) ? SDCT_SDHC : SDCT_SDSC_V2;
    }
    else
    {
        return cmd_res;
    }
    /*
        Команды CMD2 и CMD3 должны быть выданы в цикле до таймаута, для обнаружения карт на шине.
        Поскольку этот модуль подходит для работы с одной картой, команда выдается только один раз.
    */
    cmd_res = send_cmd(SD_CMD_ALL_SEND_CID, 0, SDIO_RESP_LONG); // CMD2
    if (cmd_res != SDR_Success)
        return cmd_res;

    sd_response(SD_R2, (uint32_t *)_card_info.CID); // Retrieve CID register from the card
    /*
        Отправка команды SEND_REL_ADDR (просим карту опубликовать новую RCA (относительный адрес карты)
        После того, как RCA получена, карта перейдет в состояние stand-by
    */
    cmd_res = send_cmd(SD_CMD_SEND_REL_ADDR, 0, SDIO_RESP_SHORT); // CMD3
    if (cmd_res != SDR_Success)
        return cmd_res;

    sd_response(SD_R6, response);

    _card_info.RCA = response[0] >> 16;

    cmd_res = send_cmd(SD_CMD_SEND_CSD, _card_info.RCA << 16, SDIO_RESP_LONG); // CMD9
    if (cmd_res != SDR_Success)
        return cmd_res;

    sd_response(SD_R2, (uint32_t* )_card_info.CSD); // Retrieve CSD register from the card

    // Parse CID/CSD registers
    get_info();

    // Now card must be in stand-by mode, from this point it is possible to increase bus speed.

    // Configure SDIO peripheral clock
    // HW flow control disabled, Rising edge of SDIOCLK, 1-bit bus, Power saving disabled, SDIOCLK bypass disabled
    SDIO->CLKCR = SD_BUS_1BIT | clk_div_tran() | SDIO_CLKCR_CLKEN;
    __DSB();

    /* Put the SD card in transfer mode */
    send_cmd(SD_CMD_SEL_DESEL_CARD, _card_info.RCA << 16, SDIO_RESP_SHORT); // CMD7

    cmd_res = sd_response(SD_R1b, response);
    if (cmd_res != SDR_Success)
        return cmd_res;

    /*  Disable the pull-up resistor on CD/DAT3 pin of card
        Send leading command for ACMD<n> command */
    send_cmd(SD_CMD_APP_CMD, _card_info.RCA << 16, SDIO_RESP_SHORT); // CMD55

    cmd_res = sd_response(SD_R1, response);
    if (cmd_res != SDR_Success)
        return cmd_res;

    /* Send SET_CLR_CARD_DETECT command */
    send_cmd(SD_CMD_SET_CLR_CARD_DETECT(), 0, SDIO_RESP_SHORT); // ACMD42

    cmd_res = sd_response(SD_R1, response);
    if (cmd_res != SDR_Success)
        return cmd_res;

    // Read the SCR register
    if (_card_info.Type != SDCT_MMC)
    {
        // Warning: this function set block size to 8 bytes
        if (get_scr() != SDR_Success) return cmd_res;
        if (get_ssr() != SDR_Success) return cmd_res;
    }

    return SDR_Success;
}

sd_result_t sdio_drv_t::init_fast()
{
    uint32_t response[4] = {0, 0, 0, 0};
    uint32_t sd_type = SD_STD_CAPACITY;
    uint32_t tempreg = 0;

    volatile sd_result_t cmd_res;

    //Initialize the SDIO (with initial <400Khz Clock)
    tempreg |= SDIO_CLKCR_CLKEN;  //Clock is enabled
    tempreg |= uint32_t(0x4E);  //Clock Divider. Clock=24000/(78+2)=300Khz
    //Keep the rest at 0 => HW_Flow Disabled, Rising Clock Edge, Disable CLK ByPass, Bus Width=0, Power save Disable
    SDIO->CLKCR = tempreg;

    SDIO->POWER = pwr_on(); // Enable SDIO clock

    send_cmd(SD_CMD_GO_IDLE_STATE, 0x00, SDIO_RESP_NONE);

    /* CMD8: SEND_IF_COND. Отправляем эту команду, чтобы проверить условие работы интерфейса SD Card */
    // Argument: - [31:12]: Reserved (shall be set to '0')
    //           - [11:8]: Supply Voltage (VHS) 0x1 (Range: 2.7-3.6 V)
    //           - [7:0]: Check Pattern (recommended 0xAA)
    cmd_res = send_cmd(SD_CMD_HS_SEND_EXT_CSD, check_pattern(), SDIO_RESP_SHORT); // CMD8

    if (cmd_res == SDR_Success)
    {
        // Get and check R7 response
        if (sd_response(SD_R7, response) != SDR_Success)
        {
            return SDR_BadResponse;
        }
        // Check echo-back of check pattern
        if ((response[0] & 0x01ff) != (check_pattern() & 0x01ff))
        {
            return SDR_Unsupported;
        }

        sd_type = SD_HIGH_CAPACITY; // SD v2.0 or later

        // Issue ACMD41 command
        uint32_t trials = SDIO_ACMD41_TRIALS();

        while (--trials)
        {
            // Send leading command for ACMD<n> command
            send_cmd(SD_CMD_APP_CMD, 0, SDIO_RESP_SHORT); // CMD55 with RCA 0
            if (sd_response(SD_R1, response) != SDR_Success)
            {
                return SDR_BadResponse;
            }
            // ACMD41 - initiate initialization process.
            // Set 3.0-3.3V voltage window (bit 20)
            // Set HCS bit (30) (Host Capacity Support) to inform card what host support high capacity
            // Set XPC bit (28) (SDXC Power Control) to use maximum performance (SDXC only)
            send_cmd(SD_CMD_SD_SEND_OP_COND(), SD_OCR_VOLTAGE | sd_type, SDIO_RESP_SHORT);

            if (sd_response(SD_R3, response) != SDR_Success)
            {
                return SDR_BadResponse;
            }
            // Check if card finished power up routine
            if (response[0] & (1 << 31)) break;
        }

        if (!trials)
        {
            return SDR_InvalidVoltage; // Unsupported voltage range
        }
        // Check if card is SDHC/SDXC
        _card_info.Type = (response[0] & SD_HIGH_CAPACITY) ? SDCT_SDHC : SDCT_SDSC_V2;
    }
    else
    {
        return cmd_res;
    }

    cmd_res = send_cmd(SD_CMD_ALL_SEND_CID, 0, SDIO_RESP_LONG); // CMD2
    if (cmd_res != SDR_Success)
        return cmd_res;

    /*  Отправка команды SEND_REL_ADDR (просим карту опубликовать новую RCA (относительный адрес карты))
        После того, как RCA получена, карта перейдет в состояние stand-by. */
    if (_card_info.Type != SDCT_MMC)
    {
        cmd_res = send_cmd(SD_CMD_SEND_REL_ADDR, 0, SDIO_RESP_SHORT); // CMD3
        if (cmd_res != SDR_Success)
            return cmd_res;

        sd_response(SD_R6, response);
    }

    /* Теперь карта должна быть в режиме ожидания, с этого момента можно увеличить скорость шины. */
    SDIO->CLKCR = SD_BUS_1BIT | clk_div_tran() | SDIO_CLKCR_CLKEN;
    __DSB();

    /* Переключение SD-карты в режим передачи */
    send_cmd(SD_CMD_SEL_DESEL_CARD, _card_info.RCA << 16, SDIO_RESP_SHORT); // CMD7
    cmd_res = sd_response(SD_R1b, response);
    if (cmd_res != SDR_Success)
        return cmd_res;

    return SDR_Success;
}

sd_result_t sdio_drv_t::erase(uint32_t start_block, uint32_t total_blocks)
{
    sd_result_t cmd_res = SDR_Success;
    uint32_t response = 0;
    uint8_t card_state = 0;

    send_cmd(SD_CMD_ERASE_BLOCK_START_ADDR, start_block, SDIO_RESP_SHORT); //send starting block address
    cmd_res = sd_response(SD_R1, &response);
    if (cmd_res != SDR_Success)
        goto exit;

    do
    {
        if (get_card_state(&card_state) != SDR_Success) break;
    }
    while (card_state != SD_STATE_TRAN);

    send_cmd(SD_CMD_ERASE_BLOCK_END_ADDR, (start_block + total_blocks - 1), SDIO_RESP_SHORT); //send end block address
    cmd_res = sd_response(SD_R1, &response);
    if (cmd_res != SDR_Success)
        goto exit;

    do
    {
        if (get_card_state(&card_state) != SDR_Success) break;
    }
    while (card_state != SD_STATE_TRAN);

    send_cmd(SD_CMD_ERASE_SEL_BLOCKS, 0, SDIO_RESP_SHORT); //erase all selected blocks
    cmd_res = sd_response(SD_R1b, &response);
    if (cmd_res != SDR_Success)
        goto exit;

    /* Wait till the card is in programming state */
    do
    {
        if (get_card_state(&card_state) != SDR_Success) break;
    }
    while (card_state != SD_STATE_PRG);

    do
    {
        if (get_card_state(&card_state) != SDR_Success) break;
    }
    while (card_state == SD_STATE_PRG);

exit:
    return cmd_res;
}

sd_result_t sdio_drv_t::get_ssr()
{
    sd_result_t cmd_res = SDR_Success;
    uint32_t response = 0;
    uint16_t au_size = 0;
    uint16_t erase_size = 0;
    uint8_t erase_offset = 0;
    uint8_t erase_tmout = 0;

    uint32_t* ssr_p {reinterpret_cast<uint32_t* >(&_card_info.SSR)};

    // Set block size to 64 bytes
    send_cmd(SD_CMD_SET_BLOCKLEN, 64, SDIO_RESP_SHORT); // CMD16

    cmd_res = sd_response(SD_R1, &response);
    if (cmd_res != SDR_Success)
        return cmd_res;

    /* Send leading command for ACMD<n> command */
    send_cmd(SD_CMD_APP_CMD, _card_info.RCA << 16, SDIO_RESP_SHORT); // CMD55
    cmd_res = sd_response(SD_R1, &response);
    if (cmd_res != SDR_Success)
        return cmd_res;

    /* Clear the data flags */
    SDIO->ICR = SDIO_ICR_RXOVERRC | SDIO_ICR_DCRCFAILC | SDIO_ICR_DTIMEOUTC | SDIO_ICR_DBCKENDC | SDIO_ICR_STBITERRC;

    /* Configure the SDIO data transfer */
    SDIO->DTIMER = data_read_timeout(); // Data read timeout
    /* Data length in bytes */
    SDIO->DLEN = 64;

    // Data transfer: block, card -> controller, size: 2^6 = 64bytes, enable data transfer
    SDIO->DCTRL = SDIO_DCTRL_DTDIR | (6 << 4) | SDIO_DCTRL_DTEN;

    /* Send SEND_SSR command */
    send_cmd(SD_CMD_SEND_STATUS, 0, SDIO_RESP_SHORT); // ACMD13
    cmd_res = sd_response(SD_R1, &response);

    if (cmd_res != SDR_Success)
        return cmd_res;

    /* Read a SSR register value from SDIO FIFO */
    while (!(SDIO->STA & (SDIO_STA_RXOVERR | SDIO_STA_DCRCFAIL | SDIO_STA_DTIMEOUT | SDIO_STA_DBCKEND | SDIO_STA_STBITERR)))
    {
        // Read word when data available in receive FIFO
        if (SDIO->STA & SDIO_STA_RXDAVL) *ssr_p++ = SDIO->FIFO;
        __DSB();
    }
    /* Check for errors */
    if (SDIO->STA & SDIO_STA_DTIMEOUT) cmd_res = SDR_DataTimeout;
    if (SDIO->STA & SDIO_STA_DCRCFAIL) cmd_res = SDR_DataCRCFail;
    if (SDIO->STA & SDIO_STA_RXOVERR)  cmd_res = SDR_RXOverrun;
    if (SDIO->STA & SDIO_STA_STBITERR) cmd_res = SDR_StartBitError;

    SDIO->ICR = SDIO_ICR_STATIC;
    //За время erase_tmout стирается (au_size * erase_size)(KБ) информации
    ///"\x000\ x000\ x000\ x000\ x004\ x000\ x000\ x000\ x004\ x000\ x090\ x000\ x00f\ x005\n"
    au_size = 1 << ((_card_info.SSR[10] >> 4) + 3);
    //11 - hi byte, 12 - lo byte
    erase_size = _card_info.SSR[11];
    erase_size <<= 8;
    erase_size |= _card_info.SSR[12];
    erase_offset = _card_info.SSR[13] & 0x03;
    erase_tmout = _card_info.SSR[13] >> 2;

    _card_info.CardEraseSpeed = (au_size * erase_size) / erase_tmout;

    _card_info.CardEraseTmoutSec =
        ((static_cast<float>(_card_info.Capacity) / (au_size * erase_size)) * erase_tmout) + erase_offset;

    return cmd_res;
}

/*  Find the SD card SCR register value
    input: none
    return: SDResult value
    note: card must be in transfer mode, not supported by MMC */
sd_result_t sdio_drv_t::get_scr()
{
    sd_result_t cmd_res = SDR_Success;
    uint32_t response = 0 ;
    uint32_t* scr_p = reinterpret_cast<uint32_t* >(&_card_info.SCR);

    /* Set block size to 8 bytes */
    send_cmd(SD_CMD_SET_BLOCKLEN, 8, SDIO_RESP_SHORT); // CMD16
    cmd_res = sd_response(SD_R1, &response);
    if (cmd_res != SDR_Success)
        return cmd_res;

    /* Send leading command for ACMD<n> command */
    send_cmd(SD_CMD_APP_CMD, _card_info.RCA << 16, SDIO_RESP_SHORT); // CMD55
    cmd_res = sd_response(SD_R1, &response);
    if (cmd_res != SDR_Success)
        return cmd_res;

    /* Clear the data flags */
    SDIO->ICR = SDIO_ICR_RXOVERRC | SDIO_ICR_DCRCFAILC | SDIO_ICR_DTIMEOUTC | SDIO_ICR_DBCKENDC | SDIO_ICR_STBITERRC;

    /* Configure the SDIO data transfer */
    SDIO->DTIMER = data_read_timeout(); // Data read timeout
    SDIO->DLEN   = 8; // Data length in bytes
    // Data transfer: block, card -> controller, size: 2^3 = 8bytes, enable data transfer
    SDIO->DCTRL = SDIO_DCTRL_DTDIR | (3 << 4) | SDIO_DCTRL_DTEN;

    /* Send SEND_SCR command */
    send_cmd(SD_CMD_SEND_SCR(), 0, SDIO_RESP_SHORT); // ACMD51
    cmd_res = sd_response(SD_R1, &response);
    if (cmd_res != SDR_Success)
        return cmd_res;

    /* Read a SCR register value from SDIO FIFO */
    while (!(SDIO->STA & (SDIO_STA_RXOVERR | SDIO_STA_DCRCFAIL | SDIO_STA_DTIMEOUT | SDIO_STA_DBCKEND | SDIO_STA_STBITERR)))
    {
        if (SDIO->STA & SDIO_STA_RXDAVL) *scr_p++ = SDIO->FIFO; // Read word when data available in receive FIFO
    }

    /* Check for errors */
    if (SDIO->STA & SDIO_STA_DTIMEOUT) cmd_res = SDR_DataTimeout;
    if (SDIO->STA & SDIO_STA_DCRCFAIL) cmd_res = SDR_DataCRCFail;
    if (SDIO->STA & SDIO_STA_RXOVERR)  cmd_res = SDR_RXOverrun;
    if (SDIO->STA & SDIO_STA_STBITERR) cmd_res = SDR_StartBitError;

    /* Clear the static SDIO flags */
    SDIO->ICR = SDIO_ICR_STATIC;

    return cmd_res;
}

void sdio_drv_t::get_info()
{
    uint32_t dev_size = 0;
    uint32_t dev_size_mul = 0;
    uint32_t rd_block_len = 0;

    /* Parse the CSD register */
    _card_info.CSDVer = _card_info.CSD[0] >> 6; // CSD version

    if (_card_info.Type != SDCT_MMC)
    {
        _card_info.MaxBusClkFreq = _card_info.CSD[3];
        rd_block_len = _card_info.CSD[5] & 0x0f; // Max. read data block length

        if (_card_info.CSDVer == 0)
        {
            // CSD v1.00 (SDSCv1, SDSCv2)
            dev_size = (uint32_t)(_card_info.CSD[6] & 0x03) << 10; // Device size
            dev_size |= (uint32_t)_card_info.CSD[7] << 2;
            dev_size |= (_card_info.CSD[8] & 0xc0) >> 6;

            dev_size_mul = (_card_info.CSD[ 9] & 0x03) << 1; // Device size multiplier
            dev_size_mul |= (_card_info.CSD[10] & 0x80) >> 7;

            _card_info.BlockCount = (dev_size + 1);
            _card_info.BlockCount *= (1 << (dev_size_mul + 2));
            _card_info.BlockSize = 1 << (rd_block_len);
        }
        else
        {
            // CSD v2.00 (SDHC, SDXC)
            dev_size  = (_card_info.CSD[7] & 0x3f) << 16;
            dev_size |=  _card_info.CSD[8] << 8;
            dev_size |=  _card_info.CSD[9];

            _card_info.BlockSize = 512u;
            _card_info.BlockCount = dev_size + 1;
        }

        /* The user data area capacity is calculated from C_SIZE as follows:
                memory capacity = (C_SIZE+1) * 512KByte */
        _card_info.Capacity = _card_info.BlockCount * _card_info.BlockSize;
        //Parse the CID register
        _card_info.MID = _card_info.CID[0];
        _card_info.OID = (_card_info.CID[1] << 8) | _card_info.CID[2];
        _card_info.PNM[0] = _card_info.CID[3];
        _card_info.PNM[1] = _card_info.CID[4];
        _card_info.PNM[2] = _card_info.CID[5];
        _card_info.PNM[3] = _card_info.CID[6];
        _card_info.PNM[4] = _card_info.CID[7];
        _card_info.PRV = _card_info.CID[8];
        _card_info.PSN = (_card_info.CID[9] << 24) | (_card_info.CID[10] << 16) | (_card_info.CID[11] << 8) | _card_info.CID[12];
        _card_info.MDT = ((_card_info.CID[13] << 8) | _card_info.CID[14]) & 0x0fff;
    }
}
