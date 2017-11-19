/*************************************************************************************************
 *                                     Trochili RTOS Kernel                                      *
 *                                  Copyright(C) 2017 LIUXUMING                                  *
 *                                       www.trochili.com                                        *
 *************************************************************************************************/
#ifndef _TCL_THREAD_H
#define _TCL_THREAD_H

#include "tcl.types.h"
#include "tcl.config.h"
#include "tcl.object.h"
#include "tcl.ipc.h"
#include "tcl.timer.h"

/* �߳����д����붨��                 */
#define OS_THREAD_DIAG_NORMAL            (TBitMask)(0x0)     /* �߳�����                                */
#define OS_THREAD_DIAG_STACK_OVERFLOW    (TBitMask)(0x1<<0)  /* �߳�ջ���                              */
#define OS_THREAD_DIAG_STACK_ALARM       (TBitMask)(0x1<<1)  /* �߳�ջ�澯                              */
#define OS_THREAD_DIAG_INVALID_EXIT      (TBitMask)(0x1<<2)  /* �̷߳Ƿ��˳�                            */
#define OS_THREAD_DIAG_INVALID_STATE     (TBitMask)(0x1<<3)  /* �̲߳���ʧ��                            */
#define OS_THREAD_DIAG_INVALID_STATUS    (TBitMask)(0x1<<4)  /* �̲߳���ʧ��                            */
#define OS_THREAD_DIAG_INVALID_TIMEO     (TBitMask)(0x1<<5)  /* �߳�ʱ��������ֹ                        */

/* �̵߳��ô����붨��                 */
#define OS_THREAD_ERR_NONE               (TError)(0x0)
#define OS_THREAD_ERR_UNREADY            (TError)(0x1<<0)    /* �߳̽ṹδ��ʼ��                        */
#define OS_THREAD_ERR_DENIED             (TError)(0x1<<1)    /* �̲߳����ܲ���                          */
#define OS_THREAD_ERR_STATUS             (TError)(0x1<<2)    /* �߳�״̬����                            */
#define OS_THREAD_ERR_VALUE              (TError)(0x1<<3)    /* �߳����ȼ�����                          */
#define OS_THREAD_ERR_PROTECTED          (TError)(0x1<<4)    /* �߳����ȼ�����                          */
#define OS_THREAD_ERR_SCHED_LOCKED       (TError)(0x1<<5)    /* �߳����ȼ�����                          */
#define OS_THREAD_ERR_KERNEL_STATE       (TError)(0x1<<6)    /* һ���Դ��󣬲�������������              */
#define OS_THREAD_ERR_OWNLOCKS           (TError)(0x1<<7)    /* һ���Դ��󣬲�������������              */

/* �߳����Զ���                       */
#define OS_THREAD_PROP_NONE              (TBitMask)(0x0)
#define OS_THREAD_PROP_READY             (TBitMask)(0x1<<0)  /* �̳߳�ʼ����ϱ��λ,                   */
                                                             /* ����Ա�ڽṹ���е�λ�ø����������    */
#define OS_THREAD_PROP_PRIORITY_FIXED    (TBitMask)(0x1<<1)  /* �߳����ȼ��������                      */
#define OS_THREAD_PROP_PRIORITY_SAFE     (TBitMask)(0x1<<2)  /* �߳����ȼ���ȫ���                      */
#define OS_THREAD_PROP_CLEAN_STACK       (TBitMask)(0x1<<3)  /* ��������߳�ջ�ռ�                      */
#define OS_THREAD_PROP_ROOT              (TBitMask)(0x1<<4)  /* ROOT�̱߳��λ                          */
#define OS_THREAD_PROP_DAEMON            (TBitMask)(0x1<<5)  /* �ں��ػ��̱߳��λ                      */

/* �߳�״̬����  */
#define OS_THREAD_STATUS_RUNNING         (TBitMask)(0x1<<0)  /* ����                                    */
#define OS_THREAD_STATUS_READY           (TBitMask)(0x1<<1)  /* ����                                    */
#define OS_THREAD_STATUS_DORMANT         (TBitMask)(0x1<<2)  /* ����                                    */
#define OS_THREAD_STATUS_BLOCKED         (TBitMask)(0x1<<3)  /* ����                                    */
#define OS_THREAD_STATUS_DELAYED         (TBitMask)(0x1<<4)  /* ��ʱ                                    */
#define OS_THREAD_STATUS_SUSPENDED       (TBitMask)(0x1<<5)  /* ��������                                */


/*
 * �̶߳��нṹ���壬�ýṹ��С���ں�֧�ֵ����ȼ���Χ���仯��
 * ����ʵ�̶ֹ�ʱ����߳����ȼ������㷨
 */
struct ThreadQueueDef
{
    TBitMask   PriorityMask;                 /* �����о������ȼ�����                             */
    TLinkNode* Handle[TCLC_PRIORITY_NUM];    /* �������̷ֶ߳���                                 */
};
typedef struct ThreadQueueDef TThreadQueue;


/* �߳���ʱ��ʱ���ṹ���� */
struct ThreadTimerDef
{
    TTimeTick     Ticks;                     /* �̶߳�ʱ����ʱ��                                 */
    TLinkNode     LinkNode;                  /* �̶߳�ʱ�����е�����ڵ�                         */
};
typedef struct ThreadTimerDef TThreadTimer;

/* �߳����������Ͷ���                                                                       */
typedef void (*TThreadEntry)(TArgument data);

/* �ں��߳̽ṹ���壬���ڱ����̵߳Ļ�����Ϣ                                                 */
struct ThreadDef
{
    TBitMask      Property;                  /* �̵߳�����,����Ա�ڽṹ���е�λ�ø����������  */
    TBitMask      Status;                    /* �߳�״̬,����Ա�ڽṹ���е�λ�ø����������    */
    TAddr32       StackTop;                  /* �߳�ջ��ָ��,����Ա�ڽṹ���е�λ�ø����������*/
    TAddr32       StackBase;                 /* �߳�ջ��ָ��                                     */
#if (TCLC_THREAD_STACK_CHECK_ENABLE)
    TBase32       StackAlarm;                /* �߳�ջ��������                                   */
    TBase32       StackBarrier;              /* �߳�ջ��Χ��                                     */
#endif
    TBase32       Priority;                  /* �̵߳�ǰ���ȼ�                                   */
    TBase32       BasePriority;              /* �̻߳������ȼ�                                   */
    TTimeTick     Ticks;                     /* ʱ��Ƭ�л�ʣ�µ�ticks��Ŀ                        */
    TTimeTick     BaseTicks;                 /* ʱ��Ƭ���ȣ�ticks��Ŀ��                          */
    TTimeTick     Jiffies;                   /* �߳��ܵ�����ʱ�ӽ�����                           */
    TThreadEntry  Entry;                     /* �̵߳�������                                     */
    TArgument     Argument;                  /* �߳����������û�����,�û�����ֵ                  */
    TBitMask      Diagnosis;                 /* �߳����д�����                                   */
    TThreadTimer  Timer;                     /* �����߳���ʱ�����߳�ʱ��������ʱ�����ṹ       */
#if (TCLC_IPC_ENABLE)
    TIpcContext*  IpcContext;                /* �̻߳��⡢ͬ������ͨ�ŵ�������                   */
#endif
#if ((TCLC_IPC_ENABLE) && (TCLC_IPC_MUTEX_ENABLE))
    TLinkNode*    LockList;                  /* �߳�ռ�е����Ķ���                               */
#endif
    TThreadQueue* Queue;                     /* ָ���߳������̶߳��е�ָ��                       */
    TLinkNode     LinkNode;                  /* �߳����ڶ��еĽڵ�                               */
    TObject       Object;                    /* �̵߳��ں˶���ڵ�                               */
};
typedef struct ThreadDef TThread;


extern void OsThreadModuleInit(void);
extern void OsThreadEnterQueue(TThreadQueue* pQueue, TThread* pThread, TLinkPos pos);
extern void OsThreadLeaveQueue(TThreadQueue* pQueue, TThread* pThread);
extern void OsThreadTickUpdate(void);
extern void OsThreadTimerTickUpdate(void);
extern void OsThreadSchedule(void);
extern void OsThreadCreate(TThread* pThread, TChar* pName, TBitMask status, TBitMask property,
                           TThreadEntry pEntry, TArgument argument,
                           void* pStack, TBase32 bytes, TBase32 priority, TTimeTick ticks);
extern TState OsThreadDelete(TThread* pThread, TError* pError);
extern TState OsThreadSetPriority(TThread* pThread, TBase32 priority, TBool flag, TBool* pHiRP, TError* pError);
extern void OsThreadResume(TThread* pThread);
extern void OsThreadSuspend(void);

extern TState TclCreateThread(TThread* pThread, TChar* pName, TThreadEntry pEntry, TBase32 argument,
                              void* pStack,TBase32 bytes, TBase32 priority, TTimeTick ticks, TError* pError);
extern TState TclDeleteThread(TThread* pThread, TError* pError);
extern TState TclActivateThread(TThread* pThread, TError* pError);
extern TState TclDeactivateThread(TThread* pThread, TError* pError);
extern TState TclSuspendThread(TThread* pThread, TError* pError);
extern TState TclResumeThread(TThread* pThread, TError* pError);
extern TState TclSetThreadPriority(TThread* pThread, TBase32 priority, TError* pError);
extern TState TclSetThreadSlice(TThread* pThread, TTimeTick ticks, TError* pError);
extern TState TclYieldThread(TError* pError);
extern TState TclDelayThread(TTimeTick ticks, TError* pError);
extern TState TclUndelayThread(TThread* pThread, TError* pError);
#if (TCLC_IPC_ENABLE)
extern TState TclUnblockThread(TThread* pThread, TError* pError);
#endif

#endif /*_TCL_THREAD_H */

