/*************************************************************************************************
 *                                     Trochili RTOS Kernel                                      *
 *                                  Copyright(C) 2017 LIUXUMING                                  *
 *                                       www.trochili.com                                        *
 *************************************************************************************************/
#ifndef _TCL_TIMER_H_
#define _TCL_TIMER_H_

#include "tcl.config.h"
#include "tcl.types.h"
#include "tcl.object.h"

#if (TCLC_TIMER_ENABLE)

#define OS_TIMER_ERR_NONE            (0x0)
#define OS_TIMER_ERR_FAULT           (0x1<<0)            /* һ���Դ���                          */
#define OS_TIMER_ERR_UNREADY         (0x1<<1)            /* ��ʱ������ṹδ��ʼ��              */
#define OS_TIMER_ERR_STATUS          (0x1<<2)            /* ��ʱ��״̬����                      */

/* ��ʱ��״̬ö�ٶ��� */
enum TimerStatusDef
{
    OsTimerDormant = 0,                                  /* ��ʱ���ĳ�ʼ״̬                    */
    OsTimerActive,                                       /* ��ʱ������״̬                      */
};
typedef enum TimerStatusDef TTimerStatus;

/* ��ʱ�����Ա�Ƕ��� */
#define OS_TIMER_PROP_DEAULT         (0x0)               /* ��ʱ�������Ա��                    */
#define OS_TIMER_PROP_READY          (0x1<<0)            /* ��ʱ���������                      */
#define OS_TIMER_PROP_EXPIRED        (0x1<<1)            /* ��ʱ���������                      */
#define OS_TIMER_PROP_PERIODIC       (0x1<<2)            /* �û����ڻص���ʱ��                  */
#define OS_TIMER_PROP_ACCURATE       (0x1<<3)            /* �û���׼��ʱ��                      */

#define OS_TIMER_USER_PROPERTY       (OS_TIMER_PROP_PERIODIC| OS_TIMER_PROP_ACCURATE)

/* ��ʱ�����д����붨�� */
#define OS_TIMER_DIAG_NORMAL         (TBitMask)(0x0)    /* ��ʱ������                           */
#define OS_TIMER_DIAG_OVERFLOW       (TBitMask)(0x1<<0) /* ��ʱ���������                       */

/* �û���ʱ���ص��������Ͷ��� */
typedef void(*TTimerRoutine)(TArgument data, TBase32 cycles, TTimeTick ticks);

/* ��ʱ���ṹ���� */
struct TimerDef
{
    TBitMask      Property;                              /* ��ʱ������                          */
    TTimerStatus  Status;                                /* ��ʱ��״̬                          */
    TTimeTick     PeriodTicks;                           /* ��ʱ����ʱ����                      */
    TTimeTick     MatchTicks;                            /* ��ʱ����ʱʱ��                      */
    TTimerRoutine Routine;                               /* �û���ʱ���ص�����                  */
    TArgument     Argument;                              /* ��ʱ����ʱ�ص�����                  */
    TBase32       Priority;                              /* ��ʱ���ص����ȼ�                    */
    TBitMask      Diagnosis;                             /* ��ʱ�����д�����                    */
    TBase32       ExpiredTicksCycles;                    /* ��ʱ������ʱϵͳjiffy�ֻش���       */
    TTimeTick     ExpiredTicks;                          /* ��ʱ������ʱ��                      */
    TBase32       ExpiredTimes;                          /* ��ʱ����������                      */
    TLinkNode     ExpiredNode;                           /* ��ʱ���������е�����ָ��            */
    TLinkNode     LinkNode;                              /* ��ʱ�����ڶ��е�����ָ��            */
    TObject       Object;                                /* ��ʱ�����������ָ��                */
};
typedef struct TimerDef TTimer;

/* ��ʱ�����нṹ���� */
struct TimerListDef
{
    TLinkNode* Handle[TCLC_TIMER_WHEEL_SIZE];
};
typedef struct TimerListDef TTimerList;

extern void OsTimerModuleInit(void);
extern void OsTimerTickUpdate(void);

extern TState TclCreateTimer(TTimer* pTimer, TChar* pName, TBitMask property, TTimeTick ticks,
                             TTimerRoutine routine, TArgument data, TBase32 priority,
                             TError* pError);
extern TState TclDeleteTimer(TTimer* pTimer, TError* pError);
extern TState TclStartTimer(TTimer* pTimer, TTimeTick lagticks, TError* pError);
extern TState TclStopTimer(TTimer* pTimer, TError* pError);
extern TState TclConfigTimer(TTimer* pTimer, TTimeTick ticks, TBase32 priority,
                             TError* pError);
#endif


#endif /*_TCL_TIMER_H_*/

