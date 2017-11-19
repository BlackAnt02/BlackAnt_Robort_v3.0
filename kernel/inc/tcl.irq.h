/*************************************************************************************************
 *                                     Trochili RTOS Kernel                                      *
 *                                  Copyright(C) 2017 LIUXUMING                                  *
 *                                       www.trochili.com                                        *
 *************************************************************************************************/
#ifndef _TCL_IRQ_H
#define _TCL_IRQ_H

#include "tcl.types.h"
#include "tcl.config.h"
#include "tcl.object.h"

#if (TCLC_IRQ_ENABLE)

/* ISR����ֵ */
#define OS_IRQ_DONE               (TBitMask)(0x0)              /* �жϴ���������              */
#define OS_IRQ_DAEMON             (TBitMask)(0x1<<0)           /* ������ø߼��첽�жϴ����߳�  */

#define OS_IRQ_ERR_NONE           (TError)(0x0)
#define OS_IRQ_ERR_FAULT          (TError)(0x1<<0)             /* һ���Դ���                    */
#define OS_IRQ_ERR_UNREADY        (TError)(0x1<<1)             /* �ж��������δ��ʼ��          */
#define OS_IRQ_ERR_LOCKED         (TError)(0x1<<2)             /* �ж�����������              */

#define OS_IRQ_PROP_NONE          (TBitMask)(0x0)             /* IRQ�������                   */
#define OS_IRQ_PROP_READY         (TBitMask)(0x1<<0)          /* IRQ�������                   */

/* ISR�������Ͷ��� */
typedef TBitMask (*TISR)(TArgument data);

/* �ж������ṹ���� */
typedef struct
{
    TBitMask  Property;
    TBase32     IRQn;                                           /* �����жϺ�                    */
    TISR       ISR;                                            /* ͬ���жϴ�����              */
    TArgument  Argument;                                       /* �ж���������                  */
} TIrqVector;

/* ISR�������Ͷ��� */
typedef TBitMask (*TISR)(TArgument data);

#if (TCLC_IRQ_DAEMON_ENABLE)
/* IRQ�ص��������Ͷ��� */
typedef void(*TIrqEntry)(TArgument data);

/* IRQ����ṹ���� */
typedef struct IrqDef
{
    TBitMask Property;
    TBase32 Priority;                                        /* IRQ���ȼ�                     */
    TIrqEntry Entry;                                           /* IRQ�ص�����                   */
    TArgument Argument;                                        /* IRQ�ص�����                   */
    TLinkNode LinkNode;                                        /* IRQ���ڶ��е�����ָ��         */
} TIrq;
#endif

extern void OsIrqModuleInit(void);
extern void OsIrqEnterISR(TBase32 irqn);

extern TState TclSetIrqVector(TBase32 irqn, TISR pISR, TArgument data, TError* pError);
extern TState TclCleanIrqVector(TBase32 vector, TError* pError);

#if (TCLC_IRQ_DAEMON_ENABLE)
extern TState TclPostIRQ(TIrq* pIRQ, TIrqEntry pEntry, TArgument data, TBase32 priority,
                         TError* pError);
extern TState TclCancelIRQ(TIrq* pIRQ, TError* pError);
#endif

#endif

#endif /* _TCL_IRQ_H */

