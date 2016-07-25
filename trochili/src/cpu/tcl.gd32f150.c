/*************************************************************************************************
 *                                     Trochili RTOS Kernel                                      *
 *                                  Copyright(C) 2016 LIUXUMING                                  *
 *                                       www.trochili.com                                        *
 *************************************************************************************************/
#include "tcl.types.h"
#include "tcl.config.h"
#include "tcl.cpu.h"
#include "tcl.kernel.h"
#include "tcl.gd32f150.h"

/* SysTick Ctrl & Status Reg.          */
#define CM3_SYSTICK_CTRL     (0xE000E010)
#define CM3_SYSTICK_CLKSRC   (0x00000004)   /* Clock Source.                    */
#define CM3_SYSTICK_INTEN    (0x00000002)   /* Interrupt enable.                */
#define CM3_SYSTICK_ENABLE   (0x00000001)   /* Counter mode.                    */

/* SysTick Reload  Value Reg.          */
#define CM3_SYSTICK_RELOAD   (0xE000E014)

/* SysTick Current Value Reg.          */
#define CM3_SYSTICK_CURRENT  (0xE000E018)

/* SysTick Cal     Value Reg.          */
#define CM3_SYSTICK_TENMS    (0xE000E01C)

/* Interrupt control & state register. */
#define CM3_ICSR             (0xE000ED04)
#define CM3_ICSR_PENDSVSET   (0x1<<28)       /* Value to trigger PendSV exception.  */
#define CM3_ICSR_PENDSVCLR   (0x1<<27)       /* Value to clear PendSV exception.    */
#define CM3_ICSR_PENDSTSET   (0x1<<26)       /* Value to trigger PendST exception.  */
#define CM3_ICSR_PENDSTCLR   (0x1<<25)       /* Value to clear PendST exception.    */

/* PendSV priority register            */
#define CM3_PRIO_PENDSV      (0xE000ED22)
#define CM3_PENDSV_PRIORITY  (0xFF)


/*************************************************************************************************
 *  ���ܣ������ں˽��Ķ�ʱ��                                                                     *
 *  ��������                                                                                     *
 *  ���أ���                                                                                     *
 *  ˵����                                                                                       *
 *************************************************************************************************/
void CpuStartTickClock(void)
{
    TBase32 value��

    value = TCLC_CPU_CLOCK_FREQ / TCLC_TIME_TICK_RATE;
    TCLM_SET_REG32(CM3_SYSTICK_RELOAD, value - 1u);
    TCLM_SET_REG32(CM3_SYSTICK_CTRL, CM3_SYSTICK_CLKSRC|CM3_SYSTICK_INTEN|CM3_SYSTICK_ENABLE);
}


/*************************************************************************************************
 *  ���ܣ��ں˼��ص�һ���߳�                                                                     *
 *  ��������                                                                                     *
 *  ���أ���                                                                                     *
 *  ˵����                                                                                       *
 *************************************************************************************************/
void CpuLoadRootThread()
{
    TCLM_SET_REG32(CM3_ICSR, CM3_ICSR_PENDSVSET);
}


/*************************************************************************************************
 *  ���ܣ������̵߳���                                                                           *
 *  ��������                                                                                     *
 *  ���أ���                                                                                     *
 *  ˵����                                                                                       *
 *************************************************************************************************/
void CpuConfirmThreadSwitch(void)
{
    TCLM_SET_REG32(CM3_ICSR, CM3_ICSR_PENDSVSET);
}


/*************************************************************************************************
 *  ���ܣ�ȡ���̵߳���                                                                           *
 *  ��������                                                                                     *
 *  ���أ���                                                                                     *
 *  ˵����                                                                                       *
 *************************************************************************************************/
void CpuCancelThreadSwitch(void)
{
    TCLM_SET_REG32(CM3_ICSR, CM3_ICSR_PENDSVCLR);
}


/*************************************************************************************************
 *  ���ܣ��߳�ջ��ջ֡��ʼ������                                                                 *
 *  ������(1) pTop      �߳�ջ����ַ                                                             *
 *        (2) pStack    �߳�ջ�׵�ַ                                                             *
 *        (3) bytes     �߳�ջ��С���Խ�Ϊ��λ                                                   *
 *        (4) pEntry    �̺߳�����ַ                                                             *
 *        (5) pData     �̺߳�������                                                             *
 *  ���أ���                                                                                     *
 *  ˵�����߳�ջ��ʼ��ַ����4�ֽڶ���                                                            *
 *************************************************************************************************/
void CpuBuildThreadStack(TAddr32* pTop, void* pStack, TBase32 bytes,
                         void* pEntry, TArgument argument)
{
    TReg32* pTemp;
    pTemp = (TReg32*)((TBase32)pStack + bytes);

    /* α�촦�����ж�ջ�ֳ������̵߳�һ�α���������ʱʹ�á�
       ע��LR��ֵ�Ǹ��Ƿ�ֵ����������߳�û��ͨ��LR�˳� */
    *(--pTemp) = (TReg32)0x01000000;    /* PSR                     */
    *(--pTemp) = (TReg32)pEntry;         /* �̺߳���                */
    *(--pTemp) = (TReg32)0xFFFFFFFE;    /* R14 (LR)                */
    *(--pTemp) = (TReg32)0x12121212;    /* R12                     */
    *(--pTemp) = (TReg32)0x03030303;    /* R3                      */
    *(--pTemp) = (TReg32)0x02020202;    /* R2                      */
    *(--pTemp) = (TReg32)0x01010101;    /* R1                      */
    *(--pTemp) = (TReg32)argument;       /* R0, �̲߳���            */

    /* ��ʼ���ڴ�����Ӳ���ж�ʱ�����Զ�������߳������ģ�
       �⼸���Ĵ�����ֵû��ʲô����,�����ں˵�ָ�ư� */
    *(--pTemp) = (TReg32)0x00000054;    /* R11 ,T                  */
    *(--pTemp) = (TReg32)0x00000052;    /* R10 ,R                  */
    *(--pTemp) = (TReg32)0x0000004F;    /* R9  ,O                  */
    *(--pTemp) = (TReg32)0x00000043;    /* R8  ,C                  */
    *(--pTemp) = (TReg32)0x00000048;    /* R7  ,H                  */
    *(--pTemp) = (TReg32)0x00000049;    /* R6  ,I                  */
    *(--pTemp) = (TReg32)0x0000004C;    /* R5  ,L                  */
    *(--pTemp) = (TReg32)0x00000049;    /* R4  ,I                  */

    *pTop = (TReg32)pTemp;
}


/*************************************************************************************************
 *  ���ܣ���ʼ��������                                                                           *
 *  ��������                                                                                     *
 *  ���أ���                                                                                     *
 *  ˵����                                                                                       *
 *************************************************************************************************/
void CpuSetupEntry(void)
{
    /* ����PENDSV�ж����ȼ� */
    TCLM_SET_REG32(CM3_PRIO_PENDSV, CM3_PENDSV_PRIORITY);
}



/* ��д�⺯�� */
void SysTick_Handler(void)
{
    xKernelEnterIntrState();
    xKernelTickISR();
    xKernelLeaveIntrState();
}

/* ��д�⺯�� */
void EXTI4_15_IRQHandler(void)
{
#if (TCLC_IRQ_ENABLE)
    xKernelEnterIntrState();
    xIrqEnterISR(EXTI4_15_IRQ_ID);
    xKernelLeaveIntrState();
#else
    return;
#endif
}


/* ��д�⺯�� */
void EXTI0_1_IRQHandler(void)
{
#if (TCLC_IRQ_ENABLE)
    xKernelEnterIntrState();
    xIrqEnterISR(EXTI0_1_IRQ_ID);
    xKernelLeaveIntrState();
#else
    return;
#endif
}


/* ��д�⺯�� */
void TIM2_IRQHandler(void)
{
#if (TCLC_IRQ_ENABLE)
    xKernelEnterIntrState();
    xIrqEnterISR(TIM2_IRQ_ID);
    xKernelLeaveIntrState();
#else
    return;
#endif
}

/* ��д�⺯�� */
void USART2_IRQHandler(void)
{
#if (TCLC_IRQ_ENABLE)
    xKernelEnterIntrState();
    xIrqEnterISR(USART2_IRQ_ID);
    xKernelLeaveIntrState();
#else
    return;
#endif
}
