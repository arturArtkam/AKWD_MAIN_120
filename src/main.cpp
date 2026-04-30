#include "../inc/akwd.h"

#include "impl_common.h"
#include "assert4.h"
#include "unused_pinlist.h"

#include <atomic>
#include <cmath>    // Для std::round, std::isnan

sAssertInfo g_assert_info;

static sens_array_t s_short;
static sens_array_t s_long;
static sync_rx_t sync_rx_data;
static uint8_t s_tx_data[128];
static OS::TMutex s_mutex;

/**
 * @brief Умножает int16_t на float с насыщением результата.
 *
 * Вычисляет a * b. Если математический результат выходит за пределы
 * диапазона int16_t [-32768, 32767], то возвращается
 * соответствующее предельное значение (min или max).
 * В противном случае возвращается результат, округленный до ближайшего целого.
 * Обрабатывает NaN, возвращая 0.
 *
 * @param a Множимое типа int16_t.
 * @param b Множитель типа float.
 * @return Результат умножения типа int16_t с насыщением.
 */
int16_t multiply_saturated(int16_t a, float b)
{
    float result_f = static_cast<float>(a) * b;

    constexpr float max_limit_f = static_cast<float>(std::numeric_limits<int16_t>::max()); // 32767.0f
    constexpr float min_limit_f = static_cast<float>(std::numeric_limits<int16_t>::min()); // -32768.0f

    // Проверка на NaN (Not a Number)
    if (std::isnan(result_f))
    {
        return 0; // Или другое значение по умолчанию, если 0 не подходит
    }

    // Проверка на выход за верхний предел
    // Используем >=, т.к. даже 32767.1 должно стать 32767 после округления (или насыщения)
    if (result_f >= max_limit_f)
    {
        return std::numeric_limits<int16_t>::max();
    }

    // Проверка на выход за нижний предел
    // Используем <=, т.к. даже -32768.1 должно стать -32768
    if (result_f <= min_limit_f)
    {
        return std::numeric_limits<int16_t>::min();
    }

    // Если результат находится в допустимых пределах
    // Округляем до ближайшего целого и приводим к int16_t
    // std::round возвращает float/double, каст безопасен, т.к. мы уже проверили пределы
    return static_cast<int16_t>(std::round(result_f));
}

class Sens_data final
{
    friend auto connect_to_pwrmanager(Sens_data &a);

    typedef pin<PORTB, 0, GPIO_Mode_OUT, GPIO_Speed_2MHz, GPIO_OType_OD, GPIO_PuPd_NOPULL> _Scl_pin;
    typedef pin<PORTC, 5, GPIO_Mode_OUT, GPIO_Speed_2MHz, GPIO_OType_OD, GPIO_PuPd_NOPULL> _Sda_pin;
    typedef I2C_sw<_Scl_pin, _Sda_pin> _I2c;

    Ee_gain _short_sens;
    Ee_gain _long_sens;
    _I2c i2c_bus;
    Ltc2944<_I2c> ltc2944_bat;
    Axel_ais axel_3d;
    Adcboard* adc_boards[2];
    Sync_rx_board& _sync_rx_board;
    Sync_input& _ext_trigger;

public:
    Sens_data(Adcboard* board_1, Adcboard* board_2, Sync_rx_board& board_3, Sync_input& inp) :
        i2c_bus(),
        ltc2944_bat(&i2c_bus),
        axel_3d(nullptr),
        adc_boards{board_1, board_2},
        _sync_rx_board(board_3),
        _ext_trigger(inp)
    {
//        static_assert(std::is_trivial<_I2c>::value == true, "wrong!");
    }

    void read_sensors()
    {
        ltc2944_bat.read_registers();
    }

    void map_and_adjust_board(sens_array_t& dst, const sens_array_t& src, const Ee_gain& eep_gains)
    {
        // Карта соответствия: 0->3, 1->2, 2->4, 3->5, 4->6, 5->7, 6->1, 7->0
        static const uint8_t sens_map[8] = {3, 2, 4, 5, 6, 7, 1, 0};

        // Для FKD предполагается, что d0...d7 лежат в памяти подряд
        using fkd_ptr = int16_t (*)[FKD_LEN];
        const fkd_ptr src_fkd = (const fkd_ptr)(const_cast<fkd_t*>(&src.fkd));
        fkd_ptr dst_fkd = (fkd_ptr)&dst.fkd;

        // Аналогично для Gain
        const uint8_t* src_gain = (const uint8_t*)(const_cast<gain_t*>(&src.gain));
        uint8_t* dst_gain = (uint8_t*)&dst.gain;
        const float* eep_g = (const float*)&eep_gains;

        for (int i = 0; i < 8; ++i)
        {
            uint8_t src_idx = sens_map[i];
            dst_gain[i] = src_gain[src_idx];
            adjust_gain_and_copy(dst_fkd[i], src_fkd[src_idx], eep_g[i]);
        }
    }

    void fill_datastruct(DataStructW_t* data_)
    {
        // -- Сбор данных (вне мьютекса, чтобы не держать его долго) --
//        ltc2944_bat.read_registers();
        Fram_vault::Fram::read_buf(&_short_sens, offsetof(EepData_t, EepData_t::gain_short), sizeof(Ee_gain));
        Fram_vault::Fram::read_buf(&_long_sens, offsetof(EepData_t, EepData_t::gain_long), sizeof(Ee_gain));

        volatile float temp = ltc2944_bat.get_temperature();
        volatile float vcc = ltc2944_bat.get_voltage().val;
        volatile float amp_h = lrintf(1000.0f * ltc2944_bat.get_current().val);
        auto xyz_data = axel_3d.read_data();
        uint32_t sync_p = _ext_trigger.get_sync_periode();

        // -- Критическая секция (только копирование) --
        s_mutex.lock();

        data_->SYNC_RECIEVER = sync_rx_data;

        data_->AKWD_RX.T = temp;
        data_->AKWD_RX.AmpH = amp_h;
        data_->AKWD_RX.vcc = vcc;
        data_->AKWD_RX.pwr = amp_h * vcc;
//        data_->AKWD_RX.T = ltc2944_bat.get_temperature();
//        data_->AKWD_RX.AmpH = lrintf(1000.0f * ltc2944_bat.get_current().val); /* lrintf округленное до ближайшего целого */
//        data_->AKWD_RX.vcc = ltc2944_bat.get_voltage().val;
//        data_->AKWD_RX.pwr = data_->AKWD_RX.AmpH * data_->AKWD_RX.vcc;
        data_->AKWD_RX.vcc_pos_5v = 5; //Vcc::read_pos_5v();
        data_->AKWD_RX.vcc_neg_5v = -5; //Vcc::read_neg_5v();
        data_->AKWD_RX.sync_pulses = sync_p;

        data_->AKWD_RX.accel.X = xyz_data.a_x;
        data_->AKWD_RX.accel.Y = xyz_data.a_y;
        data_->AKWD_RX.accel.Z = xyz_data.a_z;

        // -- Обработка плат через вспомогательный метод (устраняем дублирование) --
        map_and_adjust_board(data_->AKWD_RX.SENS_SHORT, s_short, _short_sens);
        map_and_adjust_board(data_->AKWD_RX.SENS_LONG, s_long, _long_sens);

        s_mutex.unlock();
    }

//    void fill_datastruct(DataStructW_t* data_)
//    {
//        ltc2944_bat.read_registers();
//
//        Fram_vault::Fram::read_buf(&_short_sens, offsetof(EepData_t, EepData_t::gain_short), sizeof(Ee_gain));
//        Fram_vault::Fram::read_buf(&_long_sens, offsetof(EepData_t, EepData_t::gain_long), sizeof(Ee_gain));
//
//        data_->AKWD_RX.T = ltc2944_bat.get_temperature();
//        data_->AKWD_RX.AmpH = lrintf(1000.0f * ltc2944_bat.get_current().val); /* lrintf округленное до ближайшего целого */
//        data_->AKWD_RX.vcc = ltc2944_bat.get_voltage().val;
//        data_->AKWD_RX.pwr = data_->AKWD_RX.AmpH * data_->AKWD_RX.vcc;
//        data_->AKWD_RX.vcc_pos_5v = Vcc::read_pos_5v();
//        data_->AKWD_RX.vcc_neg_5v = Vcc::read_neg_5v();
//        data_->AKWD_RX.sync_pulses = _ext_trigger.get_sync_periode();
//
//        auto xyz_data = axel_3d.read_data();
//
//        data_->AKWD_RX.accel.X = xyz_data.a_x;
//        data_->AKWD_RX.accel.Y = xyz_data.a_y;
//        data_->AKWD_RX.accel.Z = xyz_data.a_z;
//
////        akwd::leds.green_on();
////        while (s_mutex.is_locked());
//        s_mutex.lock();
//
//        data_->SYNC_RECIEVER = sync_rx_data;
//
//        // -- Старая версия распайки датчиков --
//
////        data_->AKWD_RX.SENS_SHORT.gain.gain_0 = s_short.gain.gain_0; // (8) -> {1} -> 1 (датчик) -> {канал АЦП} -> канал ФКД
////        data_->AKWD_RX.SENS_SHORT.gain.gain_1 = s_short.gain.gain_1; // (7) -> {2} -> 2
////        data_->AKWD_RX.SENS_SHORT.gain.gain_2 = s_short.gain.gain_6; // (5) -> {7} -> 3
////        data_->AKWD_RX.SENS_SHORT.gain.gain_3 = s_short.gain.gain_7; // (6) -> {8} -> 4
////        data_->AKWD_RX.SENS_SHORT.gain.gain_4 = s_short.gain.gain_4; // (3) -> {5} -> 5
////        data_->AKWD_RX.SENS_SHORT.gain.gain_5 = s_short.gain.gain_5; // (4) -> {6} -> 6
////        data_->AKWD_RX.SENS_SHORT.gain.gain_6 = s_short.gain.gain_2; // (2) -> {3} -> 7
////        data_->AKWD_RX.SENS_SHORT.gain.gain_7 = s_short.gain.gain_3; // (1) -> {4} -> 8
////
////        adjust_gain_and_copy(data_->AKWD_RX.SENS_SHORT.fkd.d0, s_short.fkd.d0, _short_sens.gain_0);
////        adjust_gain_and_copy(data_->AKWD_RX.SENS_SHORT.fkd.d1, s_short.fkd.d1, _short_sens.gain_1);
////        adjust_gain_and_copy(data_->AKWD_RX.SENS_SHORT.fkd.d2, s_short.fkd.d6, _short_sens.gain_2);
////        adjust_gain_and_copy(data_->AKWD_RX.SENS_SHORT.fkd.d3, s_short.fkd.d7, _short_sens.gain_3);
////        adjust_gain_and_copy(data_->AKWD_RX.SENS_SHORT.fkd.d4, s_short.fkd.d4, _short_sens.gain_4);
////        adjust_gain_and_copy(data_->AKWD_RX.SENS_SHORT.fkd.d5, s_short.fkd.d5, _short_sens.gain_5);
////        adjust_gain_and_copy(data_->AKWD_RX.SENS_SHORT.fkd.d6, s_short.fkd.d2, _short_sens.gain_6);
////        adjust_gain_and_copy(data_->AKWD_RX.SENS_SHORT.fkd.d7, s_short.fkd.d3, _short_sens.gain_7);
////
////        data_->AKWD_RX.SENS_LONG.gain.gain_0 = s_long.gain.gain_0;
////        data_->AKWD_RX.SENS_LONG.gain.gain_1 = s_long.gain.gain_1;
////        data_->AKWD_RX.SENS_LONG.gain.gain_2 = s_long.gain.gain_6;
////        data_->AKWD_RX.SENS_LONG.gain.gain_3 = s_long.gain.gain_7;
////        data_->AKWD_RX.SENS_LONG.gain.gain_4 = s_long.gain.gain_4;
////        data_->AKWD_RX.SENS_LONG.gain.gain_5 = s_long.gain.gain_5;
////        data_->AKWD_RX.SENS_LONG.gain.gain_6 = s_long.gain.gain_2;
////        data_->AKWD_RX.SENS_LONG.gain.gain_7 = s_long.gain.gain_3;
////
////        adjust_gain_and_copy(data_->AKWD_RX.SENS_LONG.fkd.d0, s_long.fkd.d0, _long_sens.gain_0);
////        adjust_gain_and_copy(data_->AKWD_RX.SENS_LONG.fkd.d1, s_long.fkd.d1, _long_sens.gain_1);
////        adjust_gain_and_copy(data_->AKWD_RX.SENS_LONG.fkd.d2, s_long.fkd.d6, _long_sens.gain_2);
////        adjust_gain_and_copy(data_->AKWD_RX.SENS_LONG.fkd.d3, s_long.fkd.d7, _long_sens.gain_3);
////        adjust_gain_and_copy(data_->AKWD_RX.SENS_LONG.fkd.d4, s_long.fkd.d4, _long_sens.gain_4);
////        adjust_gain_and_copy(data_->AKWD_RX.SENS_LONG.fkd.d5, s_long.fkd.d5, _long_sens.gain_5);
////        adjust_gain_and_copy(data_->AKWD_RX.SENS_LONG.fkd.d6, s_long.fkd.d2, _long_sens.gain_6);
////        adjust_gain_and_copy(data_->AKWD_RX.SENS_LONG.fkd.d7, s_long.fkd.d3, _long_sens.gain_7);
//
//        // -- Новая версия распайки датчиков 03.2026 --
//
//        data_->AKWD_RX.SENS_SHORT.gain.gain_0 = s_short.gain.gain_3;
//        data_->AKWD_RX.SENS_SHORT.gain.gain_1 = s_short.gain.gain_2;
//        data_->AKWD_RX.SENS_SHORT.gain.gain_2 = s_short.gain.gain_4;
//        data_->AKWD_RX.SENS_SHORT.gain.gain_3 = s_short.gain.gain_5;
//        data_->AKWD_RX.SENS_SHORT.gain.gain_4 = s_short.gain.gain_6;
//        data_->AKWD_RX.SENS_SHORT.gain.gain_5 = s_short.gain.gain_7;
//        data_->AKWD_RX.SENS_SHORT.gain.gain_6 = s_short.gain.gain_1;
//        data_->AKWD_RX.SENS_SHORT.gain.gain_7 = s_short.gain.gain_0;
//
//        adjust_gain_and_copy(data_->AKWD_RX.SENS_SHORT.fkd.d0, s_short.fkd.d3, _short_sens.gain_0);
//        adjust_gain_and_copy(data_->AKWD_RX.SENS_SHORT.fkd.d1, s_short.fkd.d2, _short_sens.gain_1);
//        adjust_gain_and_copy(data_->AKWD_RX.SENS_SHORT.fkd.d2, s_short.fkd.d4, _short_sens.gain_2);
//        adjust_gain_and_copy(data_->AKWD_RX.SENS_SHORT.fkd.d3, s_short.fkd.d5, _short_sens.gain_3);
//        adjust_gain_and_copy(data_->AKWD_RX.SENS_SHORT.fkd.d4, s_short.fkd.d6, _short_sens.gain_4);
//        adjust_gain_and_copy(data_->AKWD_RX.SENS_SHORT.fkd.d5, s_short.fkd.d7, _short_sens.gain_5);
//        adjust_gain_and_copy(data_->AKWD_RX.SENS_SHORT.fkd.d6, s_short.fkd.d1, _short_sens.gain_6);
//        adjust_gain_and_copy(data_->AKWD_RX.SENS_SHORT.fkd.d7, s_short.fkd.d0, _short_sens.gain_7);
//
//        data_->AKWD_RX.SENS_LONG.gain.gain_0 = s_long.gain.gain_3;
//        data_->AKWD_RX.SENS_LONG.gain.gain_1 = s_long.gain.gain_2;
//        data_->AKWD_RX.SENS_LONG.gain.gain_2 = s_long.gain.gain_4;
//        data_->AKWD_RX.SENS_LONG.gain.gain_3 = s_long.gain.gain_5;
//        data_->AKWD_RX.SENS_LONG.gain.gain_4 = s_long.gain.gain_6;
//        data_->AKWD_RX.SENS_LONG.gain.gain_5 = s_long.gain.gain_7;
//        data_->AKWD_RX.SENS_LONG.gain.gain_6 = s_long.gain.gain_1;
//        data_->AKWD_RX.SENS_LONG.gain.gain_7 = s_long.gain.gain_0;
//
//        adjust_gain_and_copy(data_->AKWD_RX.SENS_LONG.fkd.d0, s_long.fkd.d3, _long_sens.gain_0);
//        adjust_gain_and_copy(data_->AKWD_RX.SENS_LONG.fkd.d1, s_long.fkd.d2, _long_sens.gain_1);
//        adjust_gain_and_copy(data_->AKWD_RX.SENS_LONG.fkd.d2, s_long.fkd.d4, _long_sens.gain_2);
//        adjust_gain_and_copy(data_->AKWD_RX.SENS_LONG.fkd.d3, s_long.fkd.d5, _long_sens.gain_3);
//        adjust_gain_and_copy(data_->AKWD_RX.SENS_LONG.fkd.d4, s_long.fkd.d6, _long_sens.gain_4);
//        adjust_gain_and_copy(data_->AKWD_RX.SENS_LONG.fkd.d5, s_long.fkd.d7, _long_sens.gain_5);
//        adjust_gain_and_copy(data_->AKWD_RX.SENS_LONG.fkd.d6, s_long.fkd.d1, _long_sens.gain_6);
//        adjust_gain_and_copy(data_->AKWD_RX.SENS_LONG.fkd.d7, s_long.fkd.d0, _long_sens.gain_7);
//
//        s_mutex.unlock();
//    }

    void adjust_gain_and_copy(int16_t (&buf_dst)[FKD_LEN], int16_t (&buf_src)[FKD_LEN], float ch_gain)
    {
        buf_dst[0] = 0;

        for (uint16_t i = 1; i < FKD_LEN; i++)
            buf_dst[i] = multiply_saturated(buf_src[i], ch_gain);
    }
};

auto connect_to_pwrmanager(Sens_data& a)
{
//    akwd::pwr.register_item(&a.axel_3d);
//    akwd::pwr.register_sens(&a.adc_boards);
    return &a.axel_3d;
}

__BKP_MEM int32_t bkp_tim;
__BKP_MEM uint8_t bkp_fsm;
__BKP_MEM uint32_t bkp_sd;
__BKP_MEM uint32_t bkp_idle;
__BKP_MEM uint32_t bkp_work;
__BKP_MEM int32_t bkp_erase_timeout;

namespace akwd
{

OS::channel<uint8_t, 8> error_chan;
Leds leds;
Power_key pwr_key;
//Pwr_man pwr({connect_to_pwrmanager(akwd::sensors), &pwr_key, &sb1w, &ext_trigger});
Pc1w pc_uart;
Sd_disk sd_card;
Fram_vault eeprom;
/* Sync_rx_board и Adcboard явно получают зависимость Sb1w через свои конструкторы (внедрение зависимостей).
Внедрение зависимостей — это подход, при котором объект не создает свои зависимости внутри, а получает их извне. */
Sb1w sb1w;
Sync_rx_board sync_reciever(&sb1w, Exchange_between_boards::SYNC_RECIEVER);
Adcboard adc_board_1(&sb1w, Exchange_between_boards::ADC_1);
Adcboard adc_board_2(&sb1w, Exchange_between_boards::ADC_2);

Fsm fsm;
Sync_event timer_or_ext_sync;
Ctimer c_timer(2097152u, timer_or_ext_sync);
Sync_input ext_trigger(sb1w, c_timer, timer_or_ext_sync);
Cycle_time_counter op_time(bkp_idle, bkp_work);
Erase_tmout erase_timeout;
Sens_data sensors(&adc_board_1, &adc_board_2, sync_reciever, ext_trigger);
Pwr_man pwr({connect_to_pwrmanager(sensors), &pwr_key, &sb1w, &ext_trigger});

Fram_vars_manager nv_vars;

All_data workdata;


void fill_datastruct(DataStructW_t* data_)
{
    sensors.fill_datastruct(data_);
}

} /* namespace akwd */

//#pragma GCC diagnostic ignored "-Waggregate-return"
//Process types
typedef OS::process<OS::pr0, 512, OS::pssRunning> Proc1;
typedef OS::process<OS::pr1, 512, OS::pssRunning> Proc2;
typedef OS::process<OS::pr2, 512, OS::pssRunning> Proc3;
typedef OS::process<OS::pr3, 512, OS::pssRunning> Proc4;
typedef OS::process<OS::pr4, 512, OS::pssRunning> Proc5;
typedef OS::process<OS::pr5, 512, OS::pssRunning> Proc6;

//Process objects
Proc1 main_loop;
Proc2 proc_2;
Proc3 pwr_loop;
Proc4 proc_4;
Proc5 usb_proc;
Proc6 log_writer;

volatile size_t stack_freespace[OS::PROCESS_COUNT];

void check_stack_freespaces()
{
#if scmRTOS_DEBUG_ENABLE == 1
    for (uint_fast8_t i = 0; i < OS::PROCESS_COUNT; ++i)
    {
//        printf("#%d | CPU %5.2f | Slack %d | %s\n", i,
//        Profiler.get_result(i)/100.0,
        stack_freespace[i] = OS::get_proc(i)->stack_slack();
//        OS::get_proc(i)->name();
    }
#endif
}




int main()
{
    disable_interrupts();

    backup_domain_ini();

    akwd::leds.init_pins();
    akwd::eeprom.init();

    akwd::pc_uart.init(F_CPU, 125000ul);
    akwd::sb1w.init(F_CPU, 500000ul);

    akwd::sd_card.bind_to_bkpmem(&bkp_sd);

#if AKWD_USE_EXTERNAL_SYNC == 1
    #warning("Используется ВНЕШНЯЯ синронизация")
    uint8_t sync_input_type = 0;
    Fram_vault::Fram::read_buf(&sync_input_type, offsetof(EepData_t, EepData_t::sync), sizeof(uint8_t));

    akwd::ext_trigger.select_input(*((Sync_source*)&sync_input_type));
    akwd::ext_trigger.enable();
#else
    #warning("Используется ВНУТРЕНЯЯ синронизация")
#endif

    akwd::c_timer.bind_to_bkpmem(&bkp_tim);
    akwd::c_timer.init();

    akwd::fsm.bind_to_bkpmem(&bkp_fsm);
    akwd::erase_timeout.bind_to_bkpmem(&bkp_erase_timeout);

//    akwd::nv_vars.register_item(&akwd::op_time);

//    adc_init();
    unused_pinlist::init();

    Vcc::io_init();
    Vcc::adc_init();

    /* enable_interrupts() не требуется, поскольку
        функция run() из пространства имен OS разрешает прерывания
        после инициализации и запуска scmRTOS
    */

    OS::run();
}

namespace OS
{

#if scmRTOS_IDLE_HOOK_ENABLE == 1
idle_process_user_hook()
{
    __WFI();
}
#endif

template <>
OS_PROCESS void Proc1::exec()
{
    if (akwd::sd_card.init() == ERR_NULL)
    {
        akwd::erase_timeout() = akwd::sd_card.erase_time_sec() / 2.097152f;

        uint8_t err = 5; // Ошибка №5
        akwd::error_chan.push(err);
    }
    else
    {
        uint8_t err = 6; // Ошибка №6
        akwd::error_chan.push(err);
    }

    for (;;)
    {
        akwd::main_thread();
    }
}

//template <>
//OS_PROCESS void Proc2::exec()
//{
//    using namespace akwd;
//    for (;;)
//    {
//#if AKWD_USE_EXTERNAL_SYNC == 1
//        timer_or_ext_sync.wait();
//
//        if (timer_or_ext_sync.check_source(Sync_event_src::SYNC_EVENT))
//        {
//            OS::sleep(10);
//            if (!sync_reciever.read_data(&sync_rx_data, sizeof(sync_rx_t), s_mutex))
//            {
//                uint8_t err = 3; // Ошибка №3
//                akwd::error_chan.push(err);
//            }
//            if (!adc_board_1.read_data(&s_short, sizeof(sens_array_t), s_mutex))
//            {
//                uint8_t err = 3; // Ошибка №3
//                akwd::error_chan.push(err);
//            }
//
//            if (!adc_board_2.read_data(&s_long, sizeof(sens_array_t), s_mutex))
//            {
//                uint8_t err = 3; // Ошибка №3
//                akwd::error_chan.push(err);
//            }
//            // Программирование коэффициентов усиления платы ближнего пояса в плату дальнего пояса
//            const uint8_t head = uint8_t((Exchange_between_boards::ADC_2 << 4) | Exchange_between_boards::CMD_PRG_KU);
//            s_tx_data[0] = head;
//            memcpy(&s_tx_data[1], &s_short.gain, sizeof(s_short.gain));
//            uint16_t crc = crc16_split(&s_tx_data[0], sizeof(head) + sizeof(s_short.gain), 0xffff);
//            static uint16_t crc_pos = sizeof(head) + sizeof(s_short.gain);
//            s_tx_data[crc_pos] = 0xFF; //crc & 0xFF;
//            s_tx_data[crc_pos + 1] = 0x5A; //crc >> 8;
//            adc_board_2.send_data(s_tx_data, sizeof(head) + sizeof(s_short.gain) + sizeof(crc), s_mutex);
//
//
////            OS::sleep(10);
////            while (workdata.mtx.is_locked());
////            sync_reciever.read_data(&workdata.vault.SYNC_RECIEVER, sizeof(sync_rx_t), workdata.mtx);
////            sync_reciever.read_data(&workdata.vault.AKWD_RX.SENS_SHORT, sizeof(sens_array_t), workdata.mtx);
////            sync_reciever.read_data(&workdata.vault.AKWD_RX.SENS_LONG, sizeof(sens_array_t), workdata.mtx);
//
//            if (fsm.get() == Fsm::APP_WORK)
//            {
//                workdata.vault.Time = c_timer.get();
//                while (workdata.mtx.is_locked());
//                workdata.mtx.lock();
//                leds.red_on();
//                fill_datastruct(&workdata.vault);
//                workdata.mtx.unlock();
//
//                sd_card.write<sizeof(DataStructR_t)>(&workdata.vault.Time);
//                leds.red_off();
//            }
//        }
//#else
//        if (akwd::pwr.check_flag())
//        {
//            sync_start(&sb1w);
//            OS::sleep(5);
//            // sync_reciever.read_data(&sync_rx_data, sizeof(sync_rx_t), s_mutex);
//            sync_rx_data.sync_flag ^= 1;
//            sync_rx_data.sync_timer = 2090000 + rand() % 10000;
//            if (!adc_board_1.read_data(&s_short, sizeof(sens_array_t), s_mutex))
//            {
//                uint8_t err = 3; // Ошибка №3
//                akwd::error_chan.push(err);
//            }
//
//            if (!adc_board_2.read_data(&s_long, sizeof(sens_array_t), s_mutex))
//            {
//                uint8_t err = 3; // Ошибка №3
//                akwd::error_chan.push(err);
//            }
//
//            // Программирование коэффициентов усиления платы ближнего пояса в плату дальнего пояса
//            const uint8_t head = uint8_t((Exchange_between_boards::ADC_2 << 4) | Exchange_between_boards::CMD_PRG_KU);
//            s_tx_data[0] = head;
//            memcpy(&s_tx_data[1], &s_short.gain, sizeof(s_short.gain));
//            uint16_t crc = crc16_split(&s_tx_data[0], sizeof(head) + sizeof(s_short.gain), 0xffff);
//            static uint16_t crc_pos = sizeof(head) + sizeof(s_short.gain);
//            s_tx_data[crc_pos] = 0xFF; //crc & 0xFF;
//            s_tx_data[crc_pos + 1] = 0x5A; //crc >> 8;
//
//            if (!adc_board_2.send_data(s_tx_data, sizeof(head) + sizeof(s_short.gain) + sizeof(crc), s_mutex))
//            {
//                uint8_t err = 4; // Ошибка №4
//                akwd::error_chan.push(err);
//            }
//        }
//        OS::sleep(2000);
//#endif
//    }
//}

template <>
OS_PROCESS void Proc2::exec()
{
    using namespace akwd;

    auto update_sensors_and_sync_gain = [&]() {
        // -- Чтение данных с первой платы --
        if (!adc_board_1.read_data(&s_short, sizeof(sens_array_t), s_mutex)) {
            akwd::error_chan.push(3);
        }

        // -- Чтение данных со второй платы --
        if (!adc_board_2.read_data(&s_long, sizeof(sens_array_t), s_mutex)) {
            akwd::error_chan.push(3);
        }

        // -- Формирование пакета управления коэффициентами усиления (KU) --
        const uint8_t head = uint8_t((Exchange_between_boards::ADC_2 << 4) | Exchange_between_boards::CMD_PRG_KU);
        s_tx_data[0] = head;
        memcpy(&s_tx_data[1], &s_short.gain, sizeof(s_short.gain));

        // --  Расчет CRC --
        crc16_split(&s_tx_data[0], sizeof(head) + sizeof(s_short.gain), 0xffff);
        const uint16_t crc_pos = sizeof(head) + sizeof(s_short.gain);
        s_tx_data[crc_pos]     = 0xFF; // Заглушка
        s_tx_data[crc_pos + 1] = 0x5A; // Заглушка

        // 4. Отправка коэффициентов на плату 2
        if (!adc_board_2.send_data(s_tx_data, sizeof(head) + sizeof(s_short.gain) + 2, s_mutex)) {
            akwd::error_chan.push(4);
        }
    };

    for (;;)
    {
#if AKWD_USE_EXTERNAL_SYNC == 1
        timer_or_ext_sync.wait();

        if (timer_or_ext_sync.check_source(Sync_event_src::SYNC_EVENT))
        {
            sensors.read_sensors();

            OS::sleep(10);

            // Чтение данных синхронизатора
            if (!sync_reciever.read_data(&sync_rx_data, sizeof(sync_rx_t), s_mutex)) {
                akwd::error_chan.push(2);
            }

            // Вызов общей логики АЦП
            update_sensors_and_sync_gain();

            if (fsm.get() == Fsm::APP_WORK)
            {
                workdata.vault.Time = c_timer.get();

                // Исправлено: lock() сам дождется освобождения, check_locked тут вреден
                workdata.mtx.lock();
                leds.red_on();
                fill_datastruct(&workdata.vault);
                workdata.mtx.unlock();

                sd_card.write<sizeof(DataStructR_t)>(&workdata.vault.Time);
                leds.red_off();
            }
        }
#else
        if (akwd::pwr.check_flag())
        {
            sync_start(&sb1w);
            OS::sleep(5);

            sync_rx_data.sync_flag ^= 1;
            sync_rx_data.sync_timer = 2090000 + rand() % 10000;


            sensors.read_sensors();

            // Вызов той же самой общей логики
            update_sensors_and_sync_gain();
        }
        OS::sleep(2000);
#endif
    }
}

template <>
OS_PROCESS void Proc3::exec()
{
    for (;;)
    {
        sleep(1000u);
        akwd::usbvbus_thread();
    }
}

template <>
OS_PROCESS void Proc4::exec()
{
    for (;;)
    {
        akwd::uart_thread();
    }
}

template <>
OS_PROCESS void Proc5::exec()
{
    akwd::nv_vars.restore_variables();

    for (;;)
    {
        akwd::nv_vars.save_variables();
        sleep(5000u);
    }
}

template <>
OS_PROCESS void Proc6::exec()
{
//    for (;;)
//    {
//        check_stack_freespaces();
//        sleep(50u);
//    }
    uint8_t err_code;
    for (;;)
    {
        // Процесс ЗАМРЕТ (не будет выполняться) на этой строке,
        // пока кто-нибудь не вызовет error_chan.push()
        akwd::error_chan.pop(err_code);

        // Как только данные появились - выполняем индикацию
        if (err_code > 0)
        {
            for (uint8_t i = 0; i < err_code; ++i)
            {
                Leds::red_on();
                sleep(10u);
                Leds::red_off();
                sleep(500u);
            }
            // Пауза после серии вспышек
//            Leds::red_on();
            sleep(1000u);
//            Leds::red_off();
        }

        // После завершения цикла процесс снова дойдет до pop()
        // и уснет, если новых ошибок нет.
        check_stack_freespaces();
    }
}

} /* namespace OS */

void* operator new(std::size_t n)
{
    void* const p = std::malloc(n);
    // handle p == 0
    return p;
}

void operator delete(void* p, std::size_t)
{
    std::free(p);
}

void operator delete(void* p)
{
    std::free(p);
}

extern "C" __attribute__((__noreturn__)) void __cxa_pure_virtual() { while (1); }
