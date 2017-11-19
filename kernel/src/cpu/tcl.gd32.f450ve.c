/*************************************************************************************************
 *                                     Trochili RTOS Kernel                                      *
 *                                  Copyright(C) 2017 LIUXUMING                                  *
 *                                       www.trochili.com                                        *
 *************************************************************************************************/
#include "tcl.types.h"
#include "tcl.config.h"
#include "tcl.cpu.h"
#include "tcl.kernel.h"

/* ��д�⺯�� */
void SysTick_Handler(void)
{
    OsKernelEnterIntrState();
    OsKernelTickISR();
    OsKernelLeaveIntrState();
}

/* ��д�⺯�� */
void EXTI0_IRQHandler(void)
{
#if (TCLC_IRQ_ENABLE)
    OsKernelEnterIntrState();
    OsIrqEnterISR(GD32F450VE_EXTI0_IRQn);
    OsKernelLeaveIntrState();
#else
    return;
#endif
}
/* ��д�⺯�� */ 
void EXTI1_IRQHandler(void)
{
#if (TCLC_IRQ_ENABLE)
    OsKernelEnterIntrState();
    OsIrqEnterISR(GD32F450VE_EXTI1_IRQn);
    OsKernelLeaveIntrState();
#else
    return;
#endif
}

/* ��д�⺯�� */
void TIM2_IRQHandler(void)
{
#if (TCLC_IRQ_ENABLE)
    OsKernelEnterIntrState();
    OsIrqEnterISR(GD32F450VE_TIMER2_IRQn);
    OsKernelLeaveIntrState();
#else
    return;
#endif
}

/* ��д�⺯�� */
void USART2_IRQHandler(void)
{
#if (TCLC_IRQ_ENABLE)
    OsKernelEnterIntrState();
    OsIrqEnterISR(GD32F450VE_USART2_IRQn);
    OsKernelLeaveIntrState();
#else
    return;
#endif

}
