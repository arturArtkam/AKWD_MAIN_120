#include "../inc/akwd.h"

extern "C"
{

void USART1_IRQHandler(void)
{
    OS::TISRW tisrw;
    akwd::pc_uart.uart_isr();
}

void DMA2_Stream7_IRQHandler(void)
{
    OS::TISRW tisrw;
    akwd::pc_uart.dma_isr();
}

void USART2_IRQHandler(void)
{
    OS::TISRW tisrw;
    akwd::sb1w.uart_isr();
}

void DMA1_Stream5_IRQHandler(void)
{
    OS::TISRW tisrw;
    akwd::sb1w.rx_dma_isr();
}

void DMA1_Stream6_IRQHandler(void)
{
    OS::TISRW tisrw;
    akwd::sb1w.tx_dma_isr();
}

void SDIO_IRQHandler()
{
    OS::TISRW tisrw;
    akwd::sd_card.sdio_irq();
}

void TIM2_IRQHandler(void)
{
    OS::TISRW tisrw;
//    akwd::txrx_sync_pulse.timer_isr();
}

void TIM4_IRQHandler(void)
{
    OS::TISRW tisrw;
    akwd::pwr.timer_isr();
}

void TIM5_IRQHandler(void)
{
    OS::TISRW tisrw;
    akwd::c_timer.isr();
}

void EXTI0_IRQHandler(void)
{
    OS::TISRW tisrw;
    akwd::ext_trigger.isr();
}

} /* extern "C" */
