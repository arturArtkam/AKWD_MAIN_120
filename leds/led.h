#pragma once
#ifndef LED_H
#define LED_H

class Leds
{
public:
    static void init_pins();

    static void red_xor();
    static void red_on();
    static void red_off();

    static void green_xor();
    static void green_on();
    static void green_off();

    static void blink_red_n_times(uint8_t n);
};

#endif // LED_H
