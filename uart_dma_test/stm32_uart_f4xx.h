#pragma once
#ifndef UARTX_H
#define UARTX_H

#include <type_traits>

#include "stm32f4xx.h"
#include "pin.h"
#include "bitbanding.h"
#include "function.h"

enum baudrate
{
    BAUD_19200  = 19200UL,
    BAUD_115200 = 115200UL,
    BAUD_125000 = 125000UL,
    BAUD_460800 = 460800UL,
    BAUD_500000 = 500000UL,
    BAUD_921600 = 921600UL,
    BAUD_1M     = 1000000UL,
    BAUD_1M5    = 1500000UL
};

enum Uart_num
{
    UART1 = USART1_BASE,
    UART2 = USART2_BASE,
    UART6 = USART6_BASE
};

static void set_baud(USART_TypeDef* uart_p, uint32_t periph_clock, uint32_t baud_rate)
{
    uint32_t tmpreg = 0;
    uint32_t integerdivider = 0;
    uint32_t fractionaldivider = 0;

    /*    Определение  целочисленной части */
    if ((uart_p->CR1 & USART_CR1_OVER8) != 0)
    {
        /* Вычисление целочисленной части в случае, если режим передискретизации составляет 8 выборок... */
        integerdivider = ((25 * periph_clock) / (2 * (baud_rate)));
    }
    else
    {
        /* ... если 16 выброк. */
        integerdivider = ((25 * periph_clock) / (4 * (baud_rate)));
    }

    tmpreg = (integerdivider / 100) << 4;

    /* Определение дробной части */
    fractionaldivider = integerdivider - (100 * (tmpreg >> 4));

    if ((uart_p->CR1 & USART_CR1_OVER8) != 0)
    {
        tmpreg |= ((((fractionaldivider * 8) + 50) / 100)) & ((uint8_t)0x07);
    }
    else
    {
        tmpreg |= ((((fractionaldivider * 16) + 50) / 100)) & ((uint8_t)0x0F);
    }

    uart_p->BRR = (uint16_t)tmpreg;
}

template <uint32_t, class> class uart_x;

/* USART1 */
template <class io_pin>
class uart_x<UART1, io_pin>
{
public:
    static const IRQn USART_IRQn = USART1_IRQn;
    static constexpr USART_TypeDef* USARTn = USART1;

    static void enable_clocks()  { SET_BIT(RCC->APB2ENR, RCC_APB2ENR_USART1EN); __DSB(); }
	static void disable_clocks() { CLEAR_BIT(RCC->APB2ENR, RCC_APB2ENR_USART1EN); __DSB(); }
};

/* USART2 */
template <class io_pin>
class uart_x<UART2, io_pin>
{
public:
    static const IRQn USART_IRQn = USART2_IRQn;
    static constexpr USART_TypeDef* USARTn = USART2;

    static void enable_clocks()  { SET_BIT(RCC->APB1ENR, RCC_APB1ENR_USART2EN);  __DSB(); }
	static void disable_clocks() { CLEAR_BIT(RCC->APB1ENR, RCC_APB1ENR_USART2EN); __DSB(); }
};

/* USART6 */
template <class io_pin>
class uart_x<UART6, io_pin>
{
public:
    static const IRQn USART_IRQn = USART6_IRQn;
    static constexpr USART_TypeDef* USARTn = USART6;

    static void enable_clocks()  { SET_BIT(RCC->APB2ENR, RCC_APB2ENR_USART6EN);  __DSB(); }
	static void disable_clocks() { CLEAR_BIT(RCC->APB2ENR, RCC_APB2ENR_USART6EN); __DSB(); }
};

#endif
