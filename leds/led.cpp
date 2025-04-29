#include "scmRTOS.h"
#include "led.h"
#include "pin.h"

namespace
{

typedef pin<PORTA, 12, GPIO_Mode_OUT> red_led_pin;
typedef pin<PORTA, 10, GPIO_Mode_OUT> green_led_pin;

}

void Leds::init_pins()
{
    red_led_pin::init();
    green_led_pin::init();
}

void Leds::red_xor()
{
    red_led_pin::toggle();
}

void Leds::red_on()
{
    red_led_pin::hi();
}

void Leds::red_off()
{
    red_led_pin::lo();
}

void Leds::blink_red_n_times(uint8_t n)
{
    for (uint8_t repeats = 2 * n; repeats > 0; repeats--)
    {
        OS::sleep(50);
        red_xor();
    }
}

void Leds::green_xor()
{
    green_led_pin::toggle();
}

void Leds::green_on()
{
    green_led_pin::hi();
}

void Leds::green_off()
{
    green_led_pin::lo();
}
