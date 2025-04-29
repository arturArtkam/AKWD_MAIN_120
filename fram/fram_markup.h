#ifndef FRAM_MARKUP_H_INCLUDED
#define FRAM_MARKUP_H_INCLUDED

#include "../../struct_of_data.h"

struct Fram_markup
{
    enum
    {
        MB85RS256_SIZE = 32768ul,
        MR25H10_SIZE = 131072ul,
        MR25H40_SIZE = 524288ul,
        FRAM_SIZE = MB85RS256_SIZE
    };

    struct __attribute__((packed, aligned(1)))
    {
        EepData_t vars;
        uint16_t  crc;
    } eeprom;

    struct __attribute__((packed, aligned(1)))
    {
        uint32_t freespace_addr;
        char log_data[FRAM_SIZE - (sizeof(freespace_addr) + sizeof(eeprom))];
    } log;

} __attribute__((packed, aligned(1)));

#endif /* FRAM_MARKUP_H_INCLUDED */
