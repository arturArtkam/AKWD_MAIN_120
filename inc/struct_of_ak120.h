#ifndef STRUCTOFDATA_H
#define STRUCTOFDATA_H

#include <cstdint>
#include "automat.h"

#define FKD_LEN 512
#define VCAP_LEN 128

#pragma pack(push, 1)
//-name = accel
//-metr = CLA1
typedef struct
{
    int16_t X;
    int16_t Y;
    int16_t Z;
} accel_t;
#pragma pack(pop)

#pragma pack(push, 1)
//-name = Ku_каналов
typedef struct
{
    int8_t gain_0;
    int8_t gain_1;
    int8_t gain_2;
    int8_t gain_3;
    int8_t gain_4;
    int8_t gain_5;
    int8_t gain_6;
    int8_t gain_7;
} gain_t;
#pragma pack(pop)

#pragma pack(push, 1)
//-name = ФКД_каналов
typedef struct
{
    int16_t d0[FKD_LEN];
    int16_t d1[FKD_LEN];
    int16_t d2[FKD_LEN];
    int16_t d3[FKD_LEN];
    int16_t d4[FKD_LEN];
    int16_t d5[FKD_LEN];
    int16_t d6[FKD_LEN];
    int16_t d7[FKD_LEN];
} fkd_t;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct
{
    int16_t d0[VCAP_LEN];
} vcap_t;
#pragma pack(pop)

#pragma pack(push, 1)
//-name = AKWD
typedef struct
{
    //-structname
    gain_t gain;

    //-structname
	fkd_t fkd;
} sens_array_t;
#pragma pack(pop)

#pragma pack(push, 1)
//-name = AKWD_RX
typedef struct
{
	//-digits = 8
	//-precision = 2
	//-eu = Град
	float T;

	//-name = Потребление_мА
	//-eu = мА
	uint16_t AmpH;

    //-name = Напряжение_питания_В
	//-eu = В
	//-digits = 8
	//-precision = 3
	float vcc;

    //-name = Потребляемая_мощность_мВт
	//-eu = мВт
	//-digits = 8
	//-precision = 3
	float pwr;

    //-name = Шина_питания_5В
	//-eu = В
	//-digits = 8
	//-precision = 3
	float vcc_pos_5v;

    //-name = Шина_питания_-5В
	//-eu = В
	//-digits = 8
	//-precision = 3
	float vcc_neg_5v;

    //-name = Период_синхроимпульсов
    //-digits = 8
	//-precision = 2
	//-eu = мс
	float sync_pulses;

	//-structname
	accel_t accel;

    //-name = Ближний_зонд
	sens_array_t SENS_SHORT;

    //-name = Дальний_зонд
	sens_array_t SENS_LONG;
} akwd_rx_t;
#pragma pack(pop)

#pragma pack(push, 1)
//-name = SYNC_RECIEVER
typedef struct
{
	//-name = flag
	uint8_t sync_flag;

    //-name = Период_синхронизации
	float sync_timer;

	//-name = SNR
	float snr;

	int32_t convolution_data[16];
} sync_rx_t;
#pragma pack(pop)

#pragma pack(push, 1)
//-name = AKWD_TX
typedef struct
{
	//-digits = 8
	//-precision = 2
	//-eu = Град
	float T;

	//-name = Потребление_мА
	//-eu = мА
	uint16_t AmpH;

    //-name = Напряжение_питания_В
	//-eu = В
	//-digits = 8
	//-precision = 3
	float vcc;

    //-name = Потребляемая_мощность_мВт
	//-eu = мВт
	//-digits = 8
	//-precision = 3
	float pwr;

    //-name = Шина_питания_5В
	//-eu = В
	//-digits = 8
	//-precision = 3
	float vcc_pos_5v;

	//-structname
	accel_t accel;

    //-name = Эпюра_VCAP
	vcap_t VCAP;
} akwd_tx_t;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct
{
	//-name = автомат
    //-metr = AU
	uint8_t automat;

    //-name = время
    //-metr = WT
	int32_t Time;

    //-structname
	sync_rx_t SYNC_RECIEVER;

	//-structname
    akwd_rx_t AKWD_RX;
} DataStructW_t;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct
{
    //-name = время
    //-metr = WT
	int32_t Time;

    //-structname
	sync_rx_t SYNC_RECIEVER;

	//-structname
    akwd_rx_t AKWD_RX;
    uint8_t unused[387];
} DataStructR_t;
#pragma pack(pop)

//char (*__kaboom)[512 - (sizeof(DataStructR_t) % 512)] = 1;
//char (*__kaboom)[sizeof(DataStructR_t) - (sizeof(DataStructR_t) - offsetof(DataStructR_t, AKWD_RX.SENS_SHORT.gain))] = 1;
//char (*__kaboom)[sizeof(DataStructR_t::AKWD_RX.SENS_LONG)] = 1;
//char (*__kaboom_1)[offsetof(DataStructR_t, AKWD_RX.SENS_LONG.fkd)] = 1;
//char (*__kaboom_2)[sizeof(DataStructR_t)] = 1;

#pragma pack(push, 1)
typedef struct
{
    //-name = режим_простоя_Ч
    //-metr = WT
    uint32_t idle_time;

    //-name = режим_записи_Ч
    //-metr = WT
    uint32_t work_time;
} working_time_t;
#pragma pack(pop)

#pragma pack(push, 1)
//-name = Ku_каналов
typedef struct
{
    float gain_0;
    float gain_1;
    float gain_2;
    float gain_3;
    float gain_4;
    float gain_5;
    float gain_6;
    float gain_7;
} Ee_gain;
#pragma pack(pop)

#pragma pack(push, 1)
//-name = Синхронизация
typedef struct
{
    //-name = Синхронизатор-1_Линия-2
    uint8_t flag;
} Sync_type;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct
{
	//-structname
	Sync_type sync;

	//-name = Чувствительность_БЗ
    Ee_gain gain_short;

	//-name = Чувствительность_ДЗ
    Ee_gain gain_long;
} EepData_t;
#pragma pack(pop)

#define ADDRESS_PROC 7

#pragma pack(push, 1)
//-adr = ADDRESS_PROC
//-info = "Версия платы: AKWD_3. __DATE__ - __TIME__"
//-chip = 5
//-serial = 1
//-name = BAK_RX
//-SupportUartSpeed = 18667
//-export
typedef struct
{
    //-WRK
	//-noname
    DataStructW_t Wrk;

    //#warning("RamSize must be 16000")
    //-RamSize = 16000
	//-RAM
	//-noname
    DataStructR_t Ram;

    //-EEP
	//-noname
    EepData_t Eep;
} AllDataStruct_t;
#pragma pack(pop)

#endif /* STRUCTOFDATA_H */




