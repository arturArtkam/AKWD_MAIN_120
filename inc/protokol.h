#ifndef PROTOKOL_H_INCLUDED
#define PROTOKOL_H_INCLUDED

typedef enum
{
    ADDRESS = 0x7, //адрес устройства в сети

    ADR = (ADDRESS << 4),
    CMD_TIME      = 0xF5, //синхронизация времени на поверхности
    CMD_INFO      = (ADR | 0x2), // чтение информации об устройстве (метаданные)
    CMD_CMD       = (ADR | 0x3), // управление состоянием устройства (переход в новое состояние конечного автомата, запись таймера состояний)
    CMD_READ_EE   = (ADR | 0x5), // чтение EEPROM памяти
    CMD_WRITE_EE  = (ADR | 0x6), // запись EEPROM памяти
    CMD_R_INFORM  = (ADR | 0x7), //команда получения данных в режиме информации
    CMD_BOOT      = (ADR | 0x8),
    CMD_META      = (ADR | 0xD),
    CMD_BOOT_EXIT = (ADR | 0xE),
    CMD_WRITE     = (ADR | 0xF)
} protokol_t;


#endif // PROTOKOL_H_INCLUDED
