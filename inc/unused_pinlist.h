#ifndef UNUSED_PINLIST_H_INCLUDED
#define UNUSED_PINLIST_H_INCLUDED

#include "pin.h"

class unused_pinlist
{
    typedef pin<PORTA, 1, GPIO_Mode_AN, GPIO_Speed_2MHz, GPIO_OType_OD, GPIO_PuPd_DOWN> unused_pin_15;
    typedef pin<PORTA, 8, GPIO_Mode_AN, GPIO_Speed_2MHz, GPIO_OType_OD, GPIO_PuPd_DOWN> unused_pin_41;
    typedef pin<PORTA, 9, GPIO_Mode_AN, GPIO_Speed_2MHz, GPIO_OType_OD, GPIO_PuPd_DOWN> unused_pin_42;

    typedef pin<PORTB, 1, GPIO_Mode_AN, GPIO_Speed_2MHz, GPIO_OType_OD, GPIO_PuPd_DOWN> unused_pin_27;
    typedef pin<PORTB, 2, GPIO_Mode_AN, GPIO_Speed_2MHz, GPIO_OType_OD, GPIO_PuPd_DOWN> unused_pin_28;
    typedef pin<PORTB, 8, GPIO_Mode_AN, GPIO_Speed_2MHz, GPIO_OType_OD, GPIO_PuPd_DOWN> unused_pin_61;
    typedef pin<PORTB, 10, GPIO_Mode_AN, GPIO_Speed_2MHz, GPIO_OType_OD, GPIO_PuPd_DOWN> unused_pin_29;
//    typedef pin<PORTB, 12, GPIO_Mode_AN, GPIO_Speed_2MHz, GPIO_OType_OD, GPIO_PuPd_DOWN> unused_pin_33;
    typedef pin<PORTB, 13, GPIO_Mode_AN, GPIO_Speed_2MHz, GPIO_OType_OD, GPIO_PuPd_DOWN> unused_pin_34;
    typedef pin<PORTB, 14, GPIO_Mode_AN, GPIO_Speed_2MHz, GPIO_OType_OD, GPIO_PuPd_DOWN> unused_pin_35;
    typedef pin<PORTB, 15, GPIO_Mode_AN, GPIO_Speed_2MHz, GPIO_OType_OD, GPIO_PuPd_DOWN> unused_pin_36;

    typedef pin<PORTC, 1, GPIO_Mode_AN, GPIO_Speed_2MHz, GPIO_OType_OD, GPIO_PuPd_DOWN> unused_pin_9;
    typedef pin<PORTC, 3, GPIO_Mode_AN, GPIO_Speed_2MHz, GPIO_OType_OD, GPIO_PuPd_DOWN> unused_pin_11;
    typedef pin<PORTC, 6, GPIO_Mode_AN, GPIO_Speed_2MHz, GPIO_OType_OD, GPIO_PuPd_DOWN> unused_pin_37;
    typedef pin<PORTC, 7, GPIO_Mode_AN, GPIO_Speed_2MHz, GPIO_OType_OD, GPIO_PuPd_DOWN> unused_pin_38;
    typedef pin<PORTC, 9, GPIO_Mode_AN, GPIO_Speed_2MHz, GPIO_OType_OD, GPIO_PuPd_DOWN> unused_pin_40;
    typedef pin<PORTC, 13, GPIO_Mode_AN, GPIO_Speed_2MHz, GPIO_OType_OD, GPIO_PuPd_DOWN> unused_pin_2;
    typedef pin<PORTC, 14, GPIO_Mode_AN, GPIO_Speed_2MHz, GPIO_OType_OD, GPIO_PuPd_DOWN> unused_pin_3;
    typedef pin<PORTC, 15, GPIO_Mode_AN, GPIO_Speed_2MHz, GPIO_OType_OD, GPIO_PuPd_DOWN> unused_pin_4;

public:
    static void init()
    {
        unused_pin_15::init();
        unused_pin_41::init();
        unused_pin_42::init();

        unused_pin_27::init();
        unused_pin_28::init();
        unused_pin_61::init();
        unused_pin_29::init();
//        unused_pin_33::init();
        unused_pin_34::init();
        unused_pin_35::init();
        unused_pin_36::init();

        unused_pin_9::init();
        unused_pin_11::init();
        unused_pin_36::init();
        unused_pin_37::init();
        unused_pin_40::init();
        unused_pin_2::init();
        unused_pin_3::init();
        unused_pin_4::init();
    }
};

#endif /* UNUSED_PINLIST_H_INCLUDED */
