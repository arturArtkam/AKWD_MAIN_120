#pragma once
#ifndef SDIO_DRV_H
#define SDIO_DRV_H

#include "../inc/stm32f4xx.h"
#include "../../../Lib/pin.h"
#include "sd_event.h"

// SDIO CMD response type
#define SDIO_RESP_NONE         0x00                // No response
#define SDIO_RESP_SHORT        SDIO_CMD_WAITRESP_0 // Short response
#define SDIO_RESP_LONG         SDIO_CMD_WAITRESP   // Long response

// Bitmap to clear the SDIO static flags (command and data)
#define SDIO_ICR_STATIC        ((uint32_t)(SDIO_ICR_CCRCFAILC | SDIO_ICR_DCRCFAILC | SDIO_ICR_CTIMEOUTC | \
                                           SDIO_ICR_DTIMEOUTC | SDIO_ICR_TXUNDERRC | SDIO_ICR_RXOVERRC  | \
                                           SDIO_ICR_CMDRENDC  | SDIO_ICR_CMDSENTC  | SDIO_ICR_DATAENDC  | \
                                           SDIO_ICR_DBCKENDC))
// Mask for ACMD41
#define SD_STD_CAPACITY        ((uint32_t)0x00000000)
#define SD_HIGH_CAPACITY       ((uint32_t)0x40000000)

// SD card response type
enum
{
	SD_R1  = 0x01, // R1
	SD_R1b = 0x02, // R1b
	SD_R2  = 0x03, // R2
	SD_R3  = 0x04, // R3
	SD_R6  = 0x05, // R6 (SDIO only)
	SD_R7  = 0x06  // R7
};
// Card type
enum : uint8_t
{
	SDCT_UNKNOWN = 0x00,
	SDCT_SDSC_V1 = 0x01,  // Standard capacity SD card v1.0
	SDCT_SDSC_V2 = 0x02,  // Standard capacity SD card v2.0
	SDCT_MMC     = 0x03,  // MMC
	SDCT_SDHC    = 0x04   // High capacity SD card (SDHC or SDXC)
};

enum
{
    SD_STATE_IDLE,          // Idle
    SD_STATE_READY,         // Ready
    SD_STATE_IDENT,         // Identification
    SD_STATE_STBY,          // Stand-by
    SD_STATE_TRAN,          // Transfer
    SD_STATE_DATA,          // Sending data
    SD_STATE_RCV,           // Receive data
    SD_STATE_PRG,           // Programming
    SD_STATE_DIS,           // Disconnect
    SD_STATE_ERROR = 0xFF   // Error or unknown state
};

/* SD commands  index */
enum
{
    SD_CMD_GO_IDLE_STATE          = 0,
    SD_CMD_SEND_OP_COND           = 1,  // MMC only
    SD_CMD_ALL_SEND_CID           = 2,  // Not supported in SPI mode
    SD_CMD_SEND_REL_ADDR          = 3,  // Not supported in SPI mode
    SD_CMD_SEL_DESEL_CARD         = 7,  // Not supported in SPI mode
    SD_CMD_HS_SEND_EXT_CSD        = 8,
    SD_CMD_SEND_CSD               = 9,
    SD_CMD_SEND_CID               = 10,
    SD_CMD_READ_DAT_UNTIL_STOP    = 11, // Not supported in SPI mode
    SD_CMD_STOP_TRANSMISSION      = 12,
    SD_CMD_SEND_STATUS            = 13,
    SD_CMD_GO_INACTIVE_STATE      = 15, // Not supported in SPI mode
    SD_CMD_SET_BLOCKLEN           = 16,
    SD_CMD_READ_SINGLE_BLOCK      = 17,
    SD_CMD_READ_MULT_BLOCK        = 18,
    SD_CMD_WRITE_DAT_UNTIL_STOP   = 20, // Not supported in SPI mode
    SD_CMD_WRITE_BLOCK            = 24,
    SD_CMD_WRITE_MULTIPLE_BLOCK   = 25,
    SD_CMD_PROG_CSD               = 27,
    SD_CMD_SET_WRITE_PROT         = 28, // Not supported in SPI mode
    SD_CMD_CLR_WRITE_PROT         = 29, // Not supported in SPI mode
    SD_CMD_SEND_WRITE_PROT        = 30, // Not supported in SPI mode
    SD_CMD_ERASE_BLOCK_START_ADDR = 32,
    SD_CMD_ERASE_BLOCK_END_ADDR   = 33,
    SD_CMD_ERASE_SEL_BLOCKS       = 38,
    SD_CMD_ERASE                  = 38,
    SD_CMD_LOCK_UNLOCK            = 42,
    SD_CMD_APP_CMD                = 55,
    SD_CMD_READ_OCR               = 58, // Read OCR register
    SD_CMD_CRC_ON_OFF             = 59 // On/Off CRC check by SD Card (in SPI mode)
};

// SD functions result
typedef enum
{
	SDR_Success             = 0x00,
	SDR_Timeout             = 0x01,
	SDR_CRCError            = 0x02,  // Computed CRC not equal to received from SD card
	SDR_ReadError           = 0x03,  // Read block error (response for CMD17)
	SDR_WriteError          = 0x04,  // Write block error (response for CMD24)
	SDR_WriteErrorInternal  = 0x05,  // Write block error due to internal card error
	SDR_Unsupported         = 0x06,  // Unsupported card found
	SDR_BadResponse         = 0x07,
	SDR_SetBlockSizeFailed  = 0x08,  // Set block size command failed (response for CMD16)
	SDR_UnknownCard         = 0x09,
	SDR_NoResponse          = 0x0a,
	SDR_AddrOutOfRange      = 0x0b,  // Address out of range
	SDR_WriteCRCError       = 0x0c,  // Data write rejected due to a CRC error
	SDR_InvalidVoltage      = 0x0d,  // Unsupported voltage range
	SDR_DataTimeout         = 0x0e,  // Data block transfer timeout
	SDR_DataCRCFail         = 0x0f,  // Data block transfer CRC failed
	SDR_RXOverrun           = 0x10,  // Receive FIFO overrun
	SDR_TXUnderrun          = 0x11,  // Transmit FIFO underrun
	SDR_StartBitError       = 0x12,  // Start bit not detected on all data signals
	SDR_AddrMisaligned      = 0x13,  // A misaligned address which did not match the block length was used in the command
	SDR_BlockLenError       = 0x14,  // The transfer block length is not allowed for this card
	SDR_EraseSeqError       = 0x15,  // An error in the sequence of erase commands occurred
	SDR_EraseParam          = 0x16,  // An invalid selection of write-blocks for erase occurred
	SDR_WPViolation         = 0x17,  // Attempt to write to a protected block or to the write protected card
	SDR_LockUnlockFailed    = 0x18,  // Error in lock/unlock command
	SDR_ComCRCError         = 0x19,  // The CRC check of the previous command failed
	SDR_IllegalCommand      = 0x1a,  // Command is not legal for the the current card state
	SDR_CardECCFailed       = 0x1b,  // Card internal ECC was applied but failed to correct the data
	SDR_CCError             = 0x1c,  // Internal card controller error
	SDR_GeneralError        = 0x1d,  // A general or an unknown error occurred during the operation
	SDR_StreamUnderrun      = 0x1e,  // The card could not sustain data transfer in stream read operation
	SDR_StreamOverrun       = 0x1f,  // The card could not sustain data programming in stream mode
	SDR_CSDOverwrite        = 0x20,  // CSD overwrite error
	SDR_WPEraseSkip         = 0x21,  // Only partial address space was erased
	SDR_ECCDisabled         = 0x22,  // The command has been executed without using the internal ECC
	SDR_EraseReset          = 0x23,  // An erase sequence was cleared before executing
	SDR_AKESeqError         = 0x24   // Error in the sequence of the authentication process
} sd_result_t;

typedef struct
{
	uint8_t     Type;            // Card type (detected by SD_Init())
	uint32_t    Capacity;        // Card capacity (KBytes for SDHC/SDXC, bytes otherwise)
	uint32_t    BlockCount;      // SD card blocks count
	uint32_t    BlockSize;       // SD card block size (bytes), determined in SD_ReadCSD()
	uint32_t    MaxBusClkFreq;   // Maximum card bus frequency (MHz)
	uint16_t    CardEraseTmoutMs;
	uint16_t    CardEraseTmoutSec;
	uint16_t    CardEraseSpeed;
	uint8_t     CSDVer;          // SD card CSD register version
	uint16_t    RCA;             // SD card RCA address (only for SDIO)
	uint8_t     MID;             // SD card manufacturer ID
	uint16_t    OID;             // SD card OEM/Application ID
	uint8_t     PNM[5];          // SD card product name (5-character ASCII string)
	uint8_t     PRV;             // SD card product revision (two BCD digits: '6.2' will be 01100010b)
	uint32_t    PSN;             // SD card serial number
	uint16_t    MDT;             // SD card manufacturing date
	uint8_t     CSD[16];         // SD card CSD register (card structure data)
	uint8_t     CID[16];         // SD card CID register (card identification number)
	uint8_t     SCR[8];          // SD card SCR register (SD card configuration)
	uint8_t     SSR[64];         // SD card SSR register
} sd_t;

enum : uint32_t
{
    SD_CS_ERROR_BITS            = 0xFDFFE008, /* All possible error bits */
    SD_CS_OUT_OF_RANGE          = 0x80000000, /* The command's argument was out of allowed range */
    SD_CS_ADDRESS_ERROR         = 0x40000000, // A misaligned address used in the command
    SD_CS_BLOCK_LEN_ERROR       = 0x20000000, // The transfer block length is not allowed for this card
    SD_CS_ERASE_SEQ_ERROR       = 0x10000000, // An error in the sequence of erase commands occurred
    SD_CS_ERASE_PARAM           = 0x08000000, // An invalid selection of write-blocks for erase occurred
    SD_CS_WP_VIOLATION          = 0x04000000, // Attempt to write to a protected block or to the write protected card
    SD_CS_LOCK_UNLOCK_FAILED    = 0x01000000, // Sequence or password error in lock/unlock card command
    SD_CS_COM_CRC_ERROR         = 0x00800000, // The CRC check of the previous command failed
    SD_CS_ILLEGAL_COMMAND       = 0x00400000, // Command not legal for the card state
    SD_CS_CARD_ECC_FAILED       = 0x00200000, // Card internal ECC was applied but failed to correct the data
    SD_CS_CC_ERROR              = 0x00100000, // Internal card controller error
    SD_CS_ERROR                 = 0x00080000, // A general or an unknown error occurred during the operation
    SD_CS_STREAM_R_UNDERRUN     = 0x00040000, // The card could not sustain data transfer in stream read operation
    SD_CS_STREAM_W_OVERRUN      = 0x00020000, // The card could not sustain data programming in stream mode
    SD_CS_CSD_OVERWRITE         = 0x00010000, // CSD overwrite error
    SD_CS_WP_ERASE_SKIP         = 0x00008000, // Only partial address space was erased
    SD_CS_CARD_ECC_DISABLED     = 0x00004000, // The command has been executed without using the internal ECC
    SD_CS_ERASE_RESET           = 0x00002000, // An erase sequence was cleared before executing
    SD_CS_AKE_SEQ_ERROR         = 0x00000008 // Error in the sequence of the authentication process
};

class sdio_drv_t
{
private:
    // SDIO DMA direction
    enum
    {
        SD_DMA_RX = 0x00, // SDIO -> MEMORY
        SD_DMA_TX = 0x01  // MEMORY -> SDIO
    };

    constexpr static uint32_t SDIO_CMD_TIMEOUT() { return 0x00010000; };
    constexpr static uint32_t SDIO_ACMD41_TRIALS() { return 0x00000FFF; };

    /* Following commands are SD Card Specific commands.
        SD_CMD_APP_CMD should be sent before sending these commands. */
    constexpr static uint8_t SD_CMD_SET_BUS_WIDTH()       { return 6; };  // ACMD6
    constexpr static uint8_t SD_CMD_SD_SEND_OP_COND()     { return 41; }; // ACMD41
    constexpr static uint8_t SD_CMD_SET_CLR_CARD_DETECT() { return 42; }; // ACMD42
    constexpr static uint8_t SD_CMD_SEND_SCR()            { return 51; }; // ACMD51

    /* Argument for ACMD41 to select voltage window */
    //constexpr static uint32_t ocr_voltage() { return 0b10000000000111111000000000000000; }
    constexpr static uint32_t ocr_voltage() { return 0x80100000; }

    /* SDIO power supply control bits */
    constexpr static uint32_t pwr_off() { return 0x00; } // Power off: откл тактирование
    constexpr static uint32_t pwr_on()  { return 0x03; } // Power on: вкл тактирование

    /* Pattern for R6 response */
    constexpr static uint32_t check_pattern() { return 0x000001AA; }
    //constexpr static uint32_t check_pattern() { return 0x000000AA; }

    #define SD_BUS_1BIT     0 // 1-bit wide bus (SDIO_D0 used)
    #define SD_BUS_4BIT     SDIO_CLKCR_WIDBUS_0 // 4-bit wide bus (SDIO_D[3:0] used)
    #define SD_BUS_8BIT     SDIO_CLKCR_WIDBUS_1 // 8-bit wide bus (SDIO_D[7:0] used)

    #define SD_OCR_VOLTAGE  0b10000000000111111000000000000000

    /* SDIO clock 400kHz (48MHz / (0x76 + 2) = 400kHz) */
    template<uint32_t clk_out>
    static constexpr uint32_t clk_div()
    {
        return F_SDIO/clk_out - 2;
    }
    /*  SDIO initialization frequency (400kHz) */
    constexpr static uint32_t clk_div_init(){ return clk_div<400000u>(); }
    /* SDIO data transfer clock freq */
    constexpr static uint32_t clk_div_tran(){ return clk_div<4000000u>(); }
    /* Data read timeout is 100ms */
    constexpr static uint32_t data_read_timeout(){ return (uint32_t)((F_SDIO / (clk_div_tran() + 2u) / 1000u) * 100u); }
    /* Data write timeout is 250ms */
    constexpr static uint32_t data_write_timeout(){ return (uint32_t)((F_SDIO / (clk_div_tran() + 2u) / 1000u) * 250u); }

    typedef af_pin<PORTC, 8,  GPIO_AF_SDIO>  _d0_pin;
    typedef af_pin<PORTC, 12, GPIO_AF_SDIO>  _clk_pin;
    typedef af_pin<PORTD, 2,  GPIO_AF_SDIO>  _cmd_pin;

    sd_t _card_info;

public:
    sd_event_t event;

public:
    sd_result_t init();
    sd_result_t init_fast();
    sd_result_t erase(uint32_t start_block, uint32_t total_blocks);
    sd_result_t get_ssr();
    sd_result_t get_scr();
    void get_info();

    sdio_drv_t() : _card_info()
    {
        memset(&_card_info, 0, sizeof(sd_t));
    }

    sd_t const& get_info_struct()
    {
        return _card_info;
    }

    sd_result_t sd_response(uint16_t resp_type, uint32_t* resp_p)
    {
        sd_result_t result = SDR_Success;
        uint32_t resp;
        // Get first 32-bit value, it similar for all types of response except R2
        *resp_p = SDIO->RESP1;

        __DSB();

        switch(resp_type)
        {
        case SD_R1:
        case SD_R1b:
            /* RESP1 contains card status information
                Check for error bits in card status */
            result = get_error(*resp_p);
            break;

        case SD_R2:
            // RESP1..4 registers contain the CID/CSD register value
            *resp_p++ = __builtin_bswap32(SDIO->RESP1);
            *resp_p++ = __builtin_bswap32(SDIO->RESP2);
            *resp_p++ = __builtin_bswap32(SDIO->RESP3);
            *resp_p   = __builtin_bswap32(SDIO->RESP4);
            break;

        case SD_R3:
            // RESP1 contains the OCR register value
            // Check for correct OCR header
            if (SDIO->RESPCMD != 0x3f) result = SDR_BadResponse;
            break;

        case SD_R6:
            // RESP1 contains the RCA response value
            // Only CMD3 generates R6 response, so RESPCMD must be 0x03
            if (SDIO->RESPCMD != 0x03) result = SDR_BadResponse;
            break;

        case SD_R7:
            // RESP1 contains 'Voltage accepted' and echo-back of check pattern
            // Only CMD8 generates R7 response, so RESPCMD must be 0x08
            resp = SDIO->RESPCMD;
            //if (SDIO->RESPCMD != 0x08) result = SDR_BadResponse;
            if ((resp & 0xff) != 0x08)
            {
                result = SDR_BadResponse;
            }
            break;

        default:
            // Unknown response
            result = SDR_BadResponse;
            break;
        }

        return result;
    }

    sd_result_t get_error(uint32_t cs)
    {
        sd_result_t result = SDR_Success;

        if (cs & SD_CS_ERROR_BITS)
        {
            if (cs & SD_CS_OUT_OF_RANGE)       result = SDR_AddrOutOfRange;
            if (cs & SD_CS_ADDRESS_ERROR)      result = SDR_AddrMisaligned;
            if (cs & SD_CS_BLOCK_LEN_ERROR)    result = SDR_BlockLenError;
            if (cs & SD_CS_ERASE_SEQ_ERROR)    result = SDR_EraseSeqError;
            if (cs & SD_CS_ERASE_PARAM)        result = SDR_EraseParam;
            if (cs & SD_CS_WP_VIOLATION)       result = SDR_WPViolation;
            if (cs & SD_CS_LOCK_UNLOCK_FAILED) result = SDR_LockUnlockFailed;
            if (cs & SD_CS_COM_CRC_ERROR)      result = SDR_ComCRCError;
            if (cs & SD_CS_ILLEGAL_COMMAND)    result = SDR_IllegalCommand;
            if (cs & SD_CS_CARD_ECC_FAILED)    result = SDR_CardECCFailed;
            if (cs & SD_CS_CC_ERROR)           result = SDR_CCError;
            if (cs & SD_CS_ERROR)              result = SDR_GeneralError;
            if (cs & SD_CS_STREAM_R_UNDERRUN)  result = SDR_StreamUnderrun;
            if (cs & SD_CS_STREAM_W_OVERRUN)   result = SDR_StreamOverrun;
            if (cs & SD_CS_CSD_OVERWRITE)      result = SDR_CSDOverwrite;
            if (cs & SD_CS_WP_ERASE_SKIP)      result = SDR_WPEraseSkip;
            if (cs & SD_CS_CARD_ECC_DISABLED)  result = SDR_ECCDisabled;
            if (cs & SD_CS_ERASE_RESET)        result = SDR_EraseReset;
            if (cs & SD_CS_AKE_SEQ_ERROR)      result = SDR_AKESeqError;
        }

        return result;
    }

    sd_result_t send_cmd(uint8_t cmd, uint32_t arg, uint16_t resp_type)
    {
        volatile uint32_t wait = SDIO_CMD_TIMEOUT(); //добавил квалификатор volatile

        SDIO->ICR = SDIO_ICR_CCRCFAILC | SDIO_ICR_CTIMEOUTC | SDIO_ICR_CMDRENDC | SDIO_ICR_CMDSENTC; /* Clear the command flags */

        __DSB();

        SDIO->ARG = arg;

        __DSB();

        SDIO->CMD = (uint32_t)(cmd & SDIO_CMD_CMDINDEX) | (resp_type & SDIO_CMD_WAITRESP) | (0x0400); //The last argument is to enable CSPM

        __DSB();

        /* Block till get a response */
        if (resp_type == SDIO_RESP_NONE)
        {
            /* Wait for timeout or CMD sent flag */
            while ( !(SDIO->STA & (SDIO_STA_CTIMEOUT | SDIO_STA_CMDSENT)) && --wait );
        }
        else
        {
            /* Wait for CMDSENT or CRCFAIL */
            while ( !(SDIO->STA & (SDIO_STA_CTIMEOUT | SDIO_STA_CMDREND | SDIO_STA_CCRCFAIL)) && --wait );
        }

        /* Check response */
        if ((SDIO->STA & SDIO_STA_CTIMEOUT) || !wait)
            return SDR_Timeout;

        if ((SDIO->STA & SDIO_STA_CCRCFAIL) == SDIO_STA_CCRCFAIL)
            return SDR_CRCError; // CRC fail will be always for R3 response

        return SDR_Success;
    }

    __INLINE_ALWAYS void sdio_irq()
    {
        volatile uint8_t card_state;

        if ((SDIO->STA & SDIO_STA_DATAEND) == SDIO_STA_DATAEND)
        {
            SDIO->ICR |= SDIO_ICR_DATAENDC;
            SDIO->MASK &= ~SDIO_MASK_DATAENDIE;

            stop_transfer();

            do
            {
                /* ∆дем, посыла€ команду CMD13... */
                if (get_card_state(&card_state) != SDR_Success) while(1);
            } /* карточка сама переедет в состо€ние prg(SD_STATE_PRG) и пробудет там некоторое врем€ (пока данные запишутс€) */
            while ((card_state == SD_STATE_PRG) || (card_state == SD_STATE_RCV));

            event.signal_from_isr(ERR_NULL);
        }
        else if ((SDIO->STA & SDIO_STA_DTIMEOUT) == SDIO_STA_DTIMEOUT)
        {
            SDIO->ICR = SDIO_ICR_DTIMEOUTC;
            SDIO->MASK &= ~SDIO_MASK_DTIMEOUTIE;

            event.signal_from_isr(STA_DTIMEOUT);
        }
        else if ((SDIO->STA & SDIO_STA_CTIMEOUT) == SDIO_STA_CTIMEOUT)
        {
            SDIO->ICR = SDIO_ICR_CTIMEOUTC;
            SDIO->MASK &= ~SDIO_MASK_CTIMEOUTIE;

            event.signal_from_isr(STA_CTIMEOUT);
        }
    }

    void idle_mode()
    {
        send_cmd(SD_CMD_GO_IDLE_STATE, 0x00, SDIO_RESP_NONE);

        SDIO->POWER = pwr_off();
    }

    void rcc_conf()
    {
        RCC->APB2ENR |= RCC_APB2ENR_SDIOEN;
        __DSB();
        RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN;
        __DSB();
    }

    void rcc_disable()
    {
        RCC->APB2ENR &= ~RCC_APB2ENR_SDIOEN;
        __DSB();
        RCC->AHB1ENR &= ~RCC_AHB1ENR_DMA2EN;
        __DSB();
    }


    __INLINE_ALWAYS sd_result_t stop_transfer()
    {
        uint32_t response;

        /* Send CMD12 command */
        send_cmd(SD_CMD_STOP_TRANSMISSION, 0, SDIO_RESP_SHORT);

        return sd_response(SD_R1, &response);
    }

    sd_result_t get_card_state(volatile uint8_t* stat_p)
    {
        uint32_t response = 0;

        send_cmd(SD_CMD_SEND_STATUS, _card_info.RCA << 16, SDIO_RESP_SHORT); //Send CMD13 command
        sd_response(SD_R1, &response);

        *stat_p = (response & 0x1e00) >> 9; //Find out card status

        return get_error(response);
    }

    template<uint8_t denom = 16>
    sd_result_t full_erase(void)
    {
        static_assert(denom != 0, "denom template parameter must be greather than 0!");
        return erase(0,  (_card_info.BlockCount * 1024) / denom);
    }

    // ‘ункционирование DMA определ€етс€ периферийным регул€тором потока (SD карта).
    // Initialize the DMA channel for SDIO peripheral (DMA2 Channel4)
    // input:
    //   direction - DMA channel direction (Memory -> SDIO or SDIO -> Memory), one of SD_DMA_XX values
    //   pBuf - pointer to memory buffer
    //   length - memory buffer size in bytes (must be a multiple of 4, since SDIO operates 32-bit words)
    // note: the DMA2 peripheral clock must be already enabled
    void cofigure_dma(uint8_t direction, uint32_t* buf_p)
    {
        uint32_t reg;

        DMA2_Stream3->CR = 0;

        //Clear all the flags
        DMA2->LIFCR = DMA_LIFCR_CTCIF3 | DMA_LIFCR_CTEIF3 | DMA_LIFCR_CDMEIF3 | DMA_LIFCR_CFEIF3 | DMA_LIFCR_CHTIF3;

        //Set the DMA Addresses
        DMA2_Stream3->PAR = ((uint32_t) 0x40012C80);  //SDIO FIFO Address (=SDIO Base+0x80)
        DMA2_Stream3->M0AR = (uint32_t) buf_p;    //Memory address

        //Set the number of data to transfer
        DMA2_Stream3->NDTR = 0;   //Peripheral controls, therefore we don't need to indicate a size
        //Set the DMA CR
        reg = 0;
        /*
            Select Channel 4
        */
        reg |= (0x04 << 25) & DMA_SxCR_CHSEL;
        /*
            4 beat memory burst (memory is 32word. Therefore, each time dma access memory, it reads 4*32 bits)
            (FIFO size must be integer multiple of memory burst)(FIFO is 4byte. Therefore we can only use 4 beat in this case)
        */
        /*
            ѕеренастроил DMA на побайтный доступ к пам€ти.
            ƒл€ этого надо заменить размер MSIZE на 8 бит, и MBURST на 16.
        */
        reg |= (0b11 << 23) & DMA_SxCR_MBURST;
        //Note: Ref manual (p173 (the node at the end of 8.3.11) says that burst mode is not allowed when Pinc=0.
        //However, it appears that this is not true at all. Furthermore. when I set pBurst=0, the SDIO's dma control does not work at all.)
        reg |= (0x01 << 21) & DMA_SxCR_PBURST;  //4 beat memory burst Mode ([Burst Size*Psize] must be equal to [FIFO size] to prevent FIFO underrun and overrun errors) (burst also does not work in direct mode).
        /*
            Disable double buffer mode (when this is set, circluar mode is also automatically set.
                                          (the actual value is don't care)
        */
        reg |= (0x00 << 18) & DMA_SxCR_DBM;
        /*
             Priority is very_high
        */
        reg |= (0x03 << 16) & DMA_SxCR_PL;
        reg |= (0x00 << 15) & DMA_SxCR_PINCOS;  //Peripheral increment offset (if this is 1 and Pinc=1, then Peripheral will be incremented by 4 regardless of Psize)
        ///reg |= (0x02<<13) & DMA_SxCR_MSIZE;  //Memory data size is 32bit (word)
        reg |= (0x00 << 13) & DMA_SxCR_MSIZE;  /* Memory data size is 8bit (byte) */
        reg |= (0x02 << 11) & DMA_SxCR_PSIZE;  //Peripheral data size is 32bit (word)
        reg |= (0x01 << 10) & DMA_SxCR_MINC;  //Enable Memory Increment
        reg |= (0x00 << 9) & DMA_SxCR_MINC;  //Disable Peripheral Increment
        reg |= (0x00 << 8) & DMA_SxCR_CIRC;   //Disable Circular mode
        //tempreg|=(0x00<<6) & DMA_SxCR_DIR;  //Direction 0:P2M, 1:M2P
        reg |= (0x01 << 5) & DMA_SxCR_PFCTRL; //Peripheral controls the flow control. (The DMA tranfer ends when the data issues end of transfer signal regardless of ndtr value)
        //Bit [4..1] is for interupt mask. I don't use interrupts here
        //Bit 0 is EN. I will set it after I set the FIFO CR. (FIFO CR cannot be modified when EN=1)
        DMA2_Stream3->CR = reg;

        //Set the FIFO CR
        reg = 0x21; //Reset value
        reg |= (0 << 7); //FEIE is disabled
        reg |= (1 << 2); //Fifo is enabled (Direct mode is disabled);
        reg |= 3;   //Full fifo (Fifo threshold selection)
        DMA2_Stream3->FCR = reg;

        //Set the Direction of transfer
        if (direction != SD_DMA_RX) DMA2_Stream3->CR |= DMA_SxCR_DIR_0; // DMA will read from memory

        //Enable the DMA (When it is enabled, it starts to respond dma requests)
        DMA2_Stream3->CR |= DMA_SxCR_EN;
    }

    sd_result_t write_block_dma(uint32_t addr, uint32_t* buf_p, uint32_t length)
    {
        sd_result_t cmd_res = SDR_Success;
        uint32_t response; // SDIO command response
        uint32_t blk_count = length / 512u; // Sectors in block

        SDIO->DCTRL = 0; // Initialize the data control register

        SDIO->ICR = SDIO_ICR_STATIC; // Clear the static SDIO flags

        // Configure the SDIO data transfer
        SDIO->DTIMER = data_write_timeout();
        SDIO->DLEN  = length;

        // SDHC/SDXC cards use block unit address (1 unit = 512 bytes)
        if (blk_count > 1)
        {
            send_cmd(SD_CMD_WRITE_MULTIPLE_BLOCK, addr, SDIO_RESP_SHORT); // CMD25
        }
        else
        {
            send_cmd(SD_CMD_WRITE_BLOCK, addr, SDIO_RESP_SHORT); // CMD24
        }

        cmd_res = sd_response(SD_R1, &response);

        if (cmd_res != SDR_Success)
        {
            return cmd_res;
        }

        /* Configure the DMA to send data to SDIO */
        cofigure_dma(SD_DMA_TX, buf_p);

        /* Data transfer: DMA enable, block, controller -> card, size: 2^9 = 512bytes, enable transfer */
        SDIO->DCTRL = (9 << 4) | SDIO_DCTRL_DMAEN;

        SDIO->MASK |= (SDIO_MASK_DATAENDIE | SDIO_MASK_DTIMEOUTIE | SDIO_MASK_CTIMEOUTIE);

        if (!NVIC_GetActive(SDIO_IRQn))
        {
            NVIC_EnableIRQ(SDIO_IRQn);
        }

        SDIO->DCTRL |= SDIO_DCTRL_DTEN;

        return cmd_res;
    }

    // Start reading of data block from the SD card with DMA transfer
    // input:
    //   addr - address of the block to be read
    //   pBuf - pointer to the buffer that will contain the received data
    //   length - buffer length (must be multiple of 512)
    // return: SDResult value
    sd_result_t read_block_dma(uint32_t addr, uint32_t* buf_p, uint32_t length)
    {
        sd_result_t cmd_res = SDR_Success;
        uint32_t response;
        uint32_t blk_count = length / 512u; // Sectors in block

        /* Initialize the data control register */
        SDIO->DCTRL = 0;

        /* Clear the static SDIO flags */
        SDIO->ICR = SDIO_ICR_STATIC;

        /* Configure the SDIO data transfer */
        SDIO->DTIMER = data_read_timeout(); // Data read timeout
        SDIO->DLEN = length; // Data length

        // SDSC card uses byte unit address and
        // SDHC/SDXC cards use block unit address (1 unit = 512 bytes)
        // For SDHC card addr must be converted to block unit address
        //if (SDCard.Type == SDCT_SDHC) addr >>= 9;

        if (blk_count > 1)
        {
            send_cmd(SD_CMD_READ_MULT_BLOCK, addr, SDIO_RESP_SHORT); // CMD18
        }
        else
        {
            send_cmd(SD_CMD_READ_SINGLE_BLOCK, addr, SDIO_RESP_SHORT); // CMD17
        }

        cmd_res = sd_response(SD_R1, &response);

        if (cmd_res != SDR_Success)
        {
             return cmd_res;
        }

        /* Configure the DMA to receive data from SDIO */
        cofigure_dma(SD_DMA_RX, buf_p);

        NVIC_EnableIRQ(SDIO_IRQn);
        // Data transfer: DMA enable, block, card -> controller, size: 2^9 = 512bytes, enable transfer
        // ≈сли DTDIR = 1, данные передаютс€ из карты в контроллер.
        SDIO->DCTRL = (9 << 4) | SDIO_DCTRL_DMAEN | SDIO_DCTRL_DTDIR;

//        SDIO->MASK |= SDIO_MASK_DATAENDIE;

        SDIO->DCTRL |= SDIO_DCTRL_DTEN;

        while((SDIO->STA & SDIO_STA_DATAEND) != SDIO_STA_DATAEND)
        {}

        SDIO->ICR |= SDIO_ICR_DATAENDC;

        DMA2_Stream3->CR &= ~DMA_SxCR_EN;

        return cmd_res;
    }
};


#endif //SDIO_DRV_H

