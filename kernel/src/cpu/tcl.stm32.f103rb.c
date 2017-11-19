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
void EXTI15_10_IRQHandler(void)
{
#if (TCLC_IRQ_ENABLE)
    OsKernelEnterIntrState();
    OsIrqEnterISR(EXTI15_10_IRQ_ID);
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
    OsIrqEnterISR(TIM2_IRQ_ID);
    OsKernelLeaveIntrState();
#else
    return;
#endif
}

///* ��д�⺯�� */
//void USART1_IRQHandler(void)	
//{
//#if (TCLC_IRQ_ENABLE)
//    OsKernelEnterIntrState();
//    OsIrqEnterISR(USART1_IRQ_ID);
//    OsKernelLeaveIntrState();
//#else
//    return;
//#endif    
//    
//}
