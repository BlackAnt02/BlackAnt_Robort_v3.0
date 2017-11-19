/*************************************************************************************************
 *                                     Trochili RTOS Kernel                                      *
 *                                  Copyright(C) 2017 LIUXUMING                                  *
 *                                       www.trochili.com                                        *
 *************************************************************************************************/
#include <string.h>

#include "tcl.types.h"
#include "tcl.config.h"
#include "tcl.object.h"
#include "tcl.cpu.h"
#include "tcl.ipc.h"
#include "tcl.kernel.h"
#include "tcl.timer.h"
#include "tcl.thread.h"

/* �ں˽��������ж���,���ھ��������е��̶߳�������������� */
static TThreadQueue ThreadReadyQueue;

/* �ں��̸߳������ж��壬������ʱ���������ߵ��̶߳�������������� */
static TThreadQueue ThreadAuxiliaryQueue;


/*************************************************************************************************
 *  ���ܣ��߳����м��������̵߳����ж�����Ϊ����                                               *
 *  ������(1) pThread  �̵߳�ַ                                                                  *
 *  ���أ���                                                                                     *
 *  ˵����                                                                                       *
 *************************************************************************************************/
static void SuperviseThread(TThread* pThread)
{
    /* ��ͨ�߳���Ҫע���û���С���˳����·Ƿ�ָ������������� */
    OS_ASSERT((pThread == OsKernelVariable.CurrentThread), "");
    pThread->Entry(pThread->Argument);

    OsKernelVariable.Diagnosis |= OS_KERNEL_DIAG_THREAD_ERROR;
    pThread->Diagnosis |= OS_THREAD_DIAG_INVALID_EXIT;
    OsDebugPanic("", __FILE__, __FUNCTION__, __LINE__);
}


/*************************************************************************************************
 *  ���ܣ���������̶߳����е�������ȼ�����                                                     *
 *  ��������                                                                                     *
 *  ���أ�HiRP (Highest Ready Priority)                                                          *
 *  ˵����                                                                                       *
 *************************************************************************************************/
static TBase32 CalcThreadHiRP(void)
{
    TBase32 priority;

    /* ����������ȼ���������˵���ں˷����������� */
    if (ThreadReadyQueue.PriorityMask == (TBitMask)0)
    {
        OsKernelVariable.Diagnosis |= OS_KERNEL_DIAG_PRIORITY_ERROR;
        OsDebugPanic("", __FILE__, __FUNCTION__, __LINE__);
    }
    priority = OsCpuCalcHiPRIO(ThreadReadyQueue.PriorityMask);
    return priority;
}


#if (TCLC_THREAD_STACK_CHECK_ENABLE)
/*************************************************************************************************
 *  ���ܣ��澯�ͼ���߳�ջ�������                                                               *
 *  ������(1) pThread  �̵߳�ַ                                                                  *
 *  ���أ���                                                                                     *
 *  ˵����                                                                                       *
 *************************************************************************************************/
static void CheckThreadStack(TThread* pThread)
{
    if ((pThread->StackTop < pThread->StackBarrier) ||
            (*(TBase32*)(pThread->StackBarrier) != TCLC_THREAD_STACK_BARRIER_VALUE))
    {
        OsKernelVariable.Diagnosis |= OS_KERNEL_DIAG_THREAD_ERROR;
        pThread->Diagnosis |= OS_THREAD_DIAG_STACK_OVERFLOW;
        OsDebugPanic("", __FILE__, __FUNCTION__, __LINE__);
    }

    if (pThread->StackTop < pThread->StackAlarm)
    {
        pThread->Diagnosis |= OS_THREAD_DIAG_STACK_ALARM;
    }
}

#endif


/* RULE
 * 1 ��ǰ�߳��뿪�������к��ٴμ����������ʱ��
 *   �����Ȼ�ǵ�ǰ�߳���һ��������Ӧ�Ķ���ͷ�������Ҳ����¼���ʱ��Ƭ��
 *   ����Ѿ����ǵ�ǰ�߳���һ��������Ӧ�Ķ���β�������Ҳ����¼���ʱ��Ƭ��
 * 2 ��ǰ�߳��ھ��������ڲ��������ȼ�ʱ�����µĶ�����Ҳһ��Ҫ�ڶ���ͷ��
 */

/*************************************************************************************************
 *  ���ܣ����̼߳��뵽ָ�����̶߳�����                                                           *
 *  ������(1) pQueue  �̶߳��е�ַ��ַ                                                           *
 *        (2) pThread �߳̽ṹ��ַ                                                               *
 *        (3) pos     �߳����̶߳����е�λ��                                                     *
 *  ���أ���                                                                                     *
 *  ˵����                                                                                       *
 *************************************************************************************************/
void OsThreadEnterQueue(TThreadQueue* pQueue, TThread* pThread, TLinkPos pos)
{
    TBase32 priority;
    TLinkNode** pHandle;

    /* ����̺߳��̶߳��� */
    OS_ASSERT((pThread != (TThread*)0), "");
    OS_ASSERT((pThread->Queue == (TThreadQueue*)0), "");

    /* �����߳����ȼ��ó��߳�ʵ�������ֶ��� */
    priority = pThread->Priority;
    pHandle = &(pQueue->Handle[priority]);

    /* ���̼߳���ָ���ķֶ��� */
    OsObjQueueAddFifoNode(pHandle, &(pThread->LinkNode), pos);

    /* �����߳��������� */
    pThread->Queue = pQueue;

    /* �趨���߳����ȼ�Ϊ�������ȼ� */
    pQueue->PriorityMask |= (0x1 << priority);
}


/*************************************************************************************************
 *  ���ܣ����̴߳�ָ�����̶߳������Ƴ�                                                           *
 *  ������(1) pQueue  �̶߳��е�ַ��ַ                                                           *
 *        (2) pThread �߳̽ṹ��ַ                                                               *
 *  ���أ���                                                                                     *
 *  ˵����FIFO PRIO���ַ�����Դ�ķ�ʽ                                                            *
 *************************************************************************************************/
void OsThreadLeaveQueue(TThreadQueue* pQueue, TThread* pThread)
{
    TBase32 priority;
    TLinkNode** pHandle;

    /* ����߳��Ƿ����ڱ�����,������������ں˷����������� */
    OS_ASSERT((pThread != (TThread*)0), "");
    OS_ASSERT((pQueue == pThread->Queue), "");

    /* �����߳����ȼ��ó��߳�ʵ�������ֶ��� */
    priority = pThread->Priority;
    pHandle = &(pQueue->Handle[priority]);

    /* ���̴߳�ָ���ķֶ�����ȡ�� */
    OsObjQueueRemoveNode(pHandle, &(pThread->LinkNode));

    /* �����߳��������� */
    pThread->Queue = (TThreadQueue*)0;

    /* �����߳��뿪���к�Զ������ȼ�������ǵ�Ӱ�� */
    if (pQueue->Handle[priority] == (TLinkNode*)0)
    {
        /* �趨���߳����ȼ�δ���� */
        pQueue->PriorityMask &= (~(0x1 << priority));
    }
}


/*************************************************************************************************
 *  ���ܣ��߳�ʱ��Ƭ����������ʱ��Ƭ�жϴ���ISR�л���ñ�����                                  *
 *  ��������                                                                                     *
 *  ���أ���                                                                                     *
 *  ˵��������������˵�ǰ�̵߳�ʱ��Ƭ��������û��ѡ����Ҫ���ȵĺ���̺߳ͽ����߳��л�         *
 *************************************************************************************************/
/*
 * ��ǰ�߳̿��ܴ���3��λ��
 * 1 �������е�ͷλ��(�κ����ȼ�)
 * 2 �������е�����λ��(�κ����ȼ�)
 * 3 �������л�������������
 * ֻ�����1����Ҫ����ʱ��Ƭ��ת�Ĵ�������ʱ���漰�߳��л�,��Ϊ������ֻ��ISR�е��á�
 */
void OsThreadTickUpdate(void)
{
    TThread* pThread;
    TLinkNode* pHandle;

    /* ����ǰ�߳�ʱ��Ƭ��ȥ1��������,�߳������ܽ�������1 */
    pThread = OsKernelVariable.CurrentThread;
    pThread->Jiffies++;
    pThread->Ticks--;

    /* �������ʱ��Ƭ������� */
    if (pThread->Ticks == 0U)
    {
        /* �ָ��̵߳�ʱ�ӽ����� */
        pThread->Ticks = pThread->BaseTicks;

        /* �ж��߳��ǲ��Ǵ����ں˾����̶߳��е�ĳ�����ȼ��Ķ���ͷ */
        pHandle = ThreadReadyQueue.Handle[pThread->Priority];
        if (OS_CONTAINER_OF(pHandle, TThread, LinkNode) == pThread)
        {
            /* ����ں˴�ʱ�����̵߳���,����ʱ��Ƭ���ȣ�֮��pThread����
             * �̶߳���β������ǰ�߳������̶߳���Ҳ����ֻ�е�ǰ�߳�Ψһ1���߳�
             */
            if (OsKernelVariable.SchedLocks == 0U)
            {
                ThreadReadyQueue.Handle[pThread->Priority] =
                    (ThreadReadyQueue.Handle[pThread->Priority])->Next;

                /* ���߳�״̬��Ϊ����,׼���߳��л� */
                pThread->Status = OS_THREAD_STATUS_READY;
            }
        }
    }
}


/*************************************************************************************************
 *  ���ܣ��̶߳�ʱ��������                                                                     *
 *  ��������                                                                                     *
 *  ���أ���                                                                                     *
 *  ˵��:                                                                                        *
 *************************************************************************************************/
void OsThreadTimerTickUpdate(void)
{
    TThread* pThread;
    TThreadTimer* pTimer;
    TBool HiRP = eFalse;

    /* �õ����ڶ���ͷ���̶߳�ʱ��������Ӧ�Ķ�ʱ������1 */
    if (OsKernelVariable.ThreadTimerList != (TLinkNode*)0)
    {
        pTimer = OS_CONTAINER_OF(OsKernelVariable.ThreadTimerList, TThreadTimer, LinkNode);
        pTimer->Ticks--;

        /* �������Ϊ0���̶߳�ʱ�� */
        while (pTimer->Ticks == 0U)
        {
            /*
             * �����̣߳�����̶߳��к�״̬ת��,ע��ֻ���жϴ���ʱ��
             * ��ǰ�̲߳Żᴦ���ں��̸߳���������(��Ϊ��û���ü��߳��л�)
             * ��ǰ�̷߳��ؾ�������ʱ��һ��Ҫ�ص���Ӧ�Ķ���ͷ
             * ���߳̽�����������ʱ������Ҫ�����̵߳�ʱ�ӽ�����
             */
            pThread = OS_CONTAINER_OF(pTimer, TThread, Timer);
            switch (pThread->Status)
            {
                case (OS_THREAD_STATUS_DELAYED):
                {
                    OsThreadLeaveQueue(OsKernelVariable.ThreadAuxiliaryQueue, pThread);
                    if (pThread == OsKernelVariable.CurrentThread)
                    {
                        OsThreadEnterQueue(OsKernelVariable.ThreadReadyQueue, pThread, OsLinkHead);
                        pThread->Status = OS_THREAD_STATUS_RUNNING;
                    }
                    else
                    {
                        OsThreadEnterQueue(OsKernelVariable.ThreadReadyQueue, pThread, OsLinkTail);
                        pThread->Status = OS_THREAD_STATUS_READY;
                    }
                    /* ���̶߳�ʱ���Ӳ�ֶ������Ƴ� */
                    OsObjListRemoveDiffNode(&(OsKernelVariable.ThreadTimerList), &(pTimer->LinkNode));
                    break;
                }
                case (OS_THREAD_STATUS_DELAYED | OS_THREAD_STATUS_SUSPENDED):
                {
                    pThread->Status &= ~OS_THREAD_STATUS_DELAYED;
                    /* ���̶߳�ʱ���Ӳ�ֶ������Ƴ� */
                    OsObjListRemoveDiffNode(&(OsKernelVariable.ThreadTimerList), &(pTimer->LinkNode));
                    break;
                }
#if (TCLC_IPC_ENABLE)
                /* ���̴߳����������н������ */
                case (OS_THREAD_STATUS_BLOCKED):
                case (OS_THREAD_STATUS_BLOCKED | OS_THREAD_STATUS_SUSPENDED):
                {
                    OsIpcUnblockThread(pThread->IpcContext, eError, OS_IPC_ERR_TIMEO, &HiRP);
                    break;
                }
#endif
                default:
                {
                    OsKernelVariable.Diagnosis |= OS_KERNEL_DIAG_THREAD_ERROR;
                    pThread->Diagnosis |= OS_THREAD_DIAG_INVALID_STATUS;
                    OsDebugPanic("", __FILE__, __FUNCTION__, __LINE__);
                }
            }

            if (OsKernelVariable.ThreadTimerList == (TLinkNode*)0)
            {
                break;
            }

            /* �����һ���̶߳�ʱ�� */
            pTimer = OS_CONTAINER_OF(OsKernelVariable.ThreadTimerList, TThreadTimer, LinkNode);
        }
    }
}


/*************************************************************************************************
 *  ���ܣ����������̵߳���                                                                       *
 *  ��������                                                                                     *
 *  ���أ���                                                                                     *
 *  ˵�����̵߳ĵ���������ܱ�ISR����ȡ��                                                        *
 *************************************************************************************************/
/*
 * 1 ��ǰ�߳��뿪���м������������������У���ǰ�̷߳��ؾ�������ʱ��һ��Ҫ�ص���Ӧ�Ķ���ͷ
     ���߳̽�����������ʱ������Ҫ�����̵߳�ʱ�ӽ�����
 * 2 ���µ�ǰ�̲߳�����߾������ȼ���ԭ����
 *   1 ������ȼ����ߵ��߳̽����������
 *   2 ��ǰ�߳��Լ��뿪����
 *   3 ����̵߳����ȼ������
 *   4 ��ǰ�̵߳����ȼ�������
 *   5 ��ǰ�߳�Yiled
 *   6 ʱ��Ƭ�ж��У���ǰ�̱߳���ת
 * 3 ��cortex��������, ������һ�ֿ���:
 *   ��ǰ�߳��ͷ��˴�����������PendSV�жϵõ���Ӧ֮ǰ���������������ȼ��жϷ�����
 *   �ڸ߼�isr���ְѵ�ǰ�߳���Ϊ���У�
 *   1 ���ҵ�ǰ�߳���Ȼ����߾������ȼ���
 *   2 ���ҵ�ǰ�߳���Ȼ����߾����̶߳��еĶ���ͷ��
 *   ��ʱ��Ҫ����ȡ��PENDSV�Ĳ��������⵱ǰ�̺߳��Լ��л�
 */
void OsThreadSchedule(void)
{
    TBase32 priority;

    /* ������߾������ȼ�����ú���̣߳��������߳�ָ��Ϊ����˵���ں˷����������� */
    priority = CalcThreadHiRP();
    if (priority >= TCLC_PRIORITY_NUM)
    {
        OsDebugPanic("", __FILE__, __FUNCTION__, __LINE__);
    }

    if (ThreadReadyQueue.Handle[priority] == (TLinkNode*)0)
    {
        OsDebugPanic("", __FILE__, __FUNCTION__, __LINE__);
    }

    OsKernelVariable.NomineeThread =
        OS_CONTAINER_OF(ThreadReadyQueue.Handle[priority], TThread, LinkNode);
    if (OsKernelVariable.NomineeThread == (TThread*)0)
    {
        OsDebugPanic("", __FILE__, __FUNCTION__, __LINE__);
    }

    /*
     * �˴������߼����ӣ��漰���ܶ����̵߳����龰���ر���ʱ��Ƭ��Yiled��
     * �жϡ��ж���ռ����ĵ�ǰ�̵߳�״̬�仯��
     */
    if (OsKernelVariable.NomineeThread != OsKernelVariable.CurrentThread)
    {
#if (TCLC_THREAD_STACK_CHECK_ENABLE)
        CheckThreadStack(OsKernelVariable.NomineeThread);
#endif
        /*
         * ������֮�����������Ҫ����ǰ�߳�״̬������ֻ���Ǳ���ռ�������
         * ��ʱ�����ֿ��ܣ�һ���߳�����ִ�У�Ȼ���и������ȼ����߳̾�����
         * ���ǵ�ǰ�̶߳��ݲ��������Ǻܿ��ַ�������״̬��(ͬʱ/Ȼ��)�и������ȼ����߳̾�����
         * �����������������Ҫ����ǰ�߳�����Ϊ����״̬��
         */
        if (OsKernelVariable.CurrentThread->Status == OS_THREAD_STATUS_RUNNING)
        {
            OsKernelVariable.CurrentThread->Status = OS_THREAD_STATUS_READY;
        }
        OsCpuConfirmThreadSwitch();
    }
    else
    {
        /*
         * ��ǰ�̶߳��ݲ��������Ǻܿ��ַ�������״̬�����Ҵ�ʱ������������ȼ��ľ����̣߳�
         * ���·Żؾ������У���ʱ����ش������Ѿ�����ǰ�߳��������ó�����״̬��
         * ����yield��tick isr��п��ܽ���ǰ�߳����óɾ���̬������ʱ��ǰ�߳�����
         * ������ֻ��Ψһһ���߳̾�����������ʱ��Ҫ����ǰ�߳��������ó�����״̬��
         */
        if (OsKernelVariable.CurrentThread->Status == OS_THREAD_STATUS_READY)
        {
            OsKernelVariable.CurrentThread->Status = OS_THREAD_STATUS_RUNNING;
        }
        OsCpuCancelThreadSwitch();
    }
}

/*************************************************************************************************
 *  ���ܣ��߳̽ṹ��ʼ������                                                                     *
 *  ������(1)  pThread  �߳̽ṹ��ַ                                                             *
  *       (2)  pName    �̵߳ĳ�ʼ״̬                                                           *
 *        (3)  status   �̵߳ĳ�ʼ״̬                                                           *
 *        (4)  property �߳�����                                                                 *
 *        (5)  pEntry   �̺߳�����ַ                                                             *
 *        (6)  argument �̺߳�������                                                             *
 *        (7)  pStack   �߳�ջ��ַ                                                               *
 *        (8)  bytes    �߳�ջ��С������Ϊ��λ                                                   *
 *        (9)  priority �߳����ȼ�                                                               *
 *        (10) ticks    �߳�ʱ��Ƭ����                                                           *
 *  ���أ�(1)  eError                                                                            *
 *        (2)  eSuccess                                                                          *
 *  ˵����ע��ջ��ʼ��ַ��ջ��С��ջ�澯��ַ���ֽڶ�������                                       *
 *************************************************************************************************/
void OsThreadCreate(TThread* pThread, TChar* pName, TBitMask status, TBitMask property,
                    TThreadEntry pEntry, TArgument argument,
                    void* pStack, TBase32 bytes, TBase32 priority, TTimeTick ticks)
{
    TThreadQueue* pQueue;

    /* ��ʼ���̻߳���������Ϣ */
    OsKernelAddObject(&(pThread->Object), pName, OsObjTypeThread);

    /* �����߳�ջ������ݺ͹����̳߳�ʼջջ֡ */
    OS_ASSERT((bytes >= TCLC_CPU_MINIMAL_STACK), "");

    /* ջ��С���¶��� */
    bytes &= (~((TBase32)(TCLC_CPU_STACK_ALIGNED - 1U)));
    pThread->StackBase = (TBase32)pStack + bytes;

    /* ����߳�ջ�ռ� */
    if (property & OS_THREAD_PROP_CLEAN_STACK)
    {
        memset(pStack, 0U, bytes);
    }

    /* ����(α��)�̳߳�ʼջ֡,���ｫ�߳̽ṹ��ַ��Ϊ��������SuperviseThread()���� */
    OsCpuBuildThreadStack(&(pThread->StackTop), pStack, bytes, (void*)(&SuperviseThread),
                          (TArgument)pThread);

    /* �����߳�ջ�澯��ַ */
#if (TCLC_THREAD_STACK_CHECK_ENABLE)
    pThread->StackAlarm = (TBase32)pStack + bytes - (bytes* TCLC_THREAD_STACK_ALARM_RATIO) / 100;
    pThread->StackBarrier = (TBase32)pStack;
    (*(TAddr32*)pStack) = TCLC_THREAD_STACK_BARRIER_VALUE;
#endif

    /* �����߳�ʱ��Ƭ��ز��� */
    pThread->Ticks = ticks;
    pThread->BaseTicks = ticks;
    pThread->Jiffies = 0U;

    /* �����߳����ȼ� */
    pThread->Priority = priority;
    pThread->BasePriority = priority;

    /* �����߳���ں������̲߳��� */
    pThread->Entry = pEntry;
    pThread->Argument = argument;

    /* �����߳�����������Ϣ */
    pThread->Queue = (TThreadQueue*)0;

    /* �����̶߳�ʱ�� */
    pThread->Timer.LinkNode.Data   = (TBase32*)(&(pThread->Timer.Ticks));
    pThread->Timer.LinkNode.Prev   = (TLinkNode*)0;
    pThread->Timer.LinkNode.Next   = (TLinkNode*)0;
    pThread->Timer.LinkNode.Handle = (TLinkNode**)0;
    pThread->Timer.Ticks = (TTimeTick)0;

    /*
     * �߳�IPC���������Ľṹ��û��ֱ�Ӷ������߳̽ṹ���������Ҫ������ʱ��
     * ��ʱ���߳�ջ�ﰲ�ŵġ��ô��Ǽ������߳̽ṹռ�õ��ڴ档
     */
#if (TCLC_IPC_ENABLE)
    pThread->IpcContext = (TIpcContext*)0;
#endif

    /* �߳�ռ�е���(MUTEX)���� */
#if ((TCLC_IPC_ENABLE) && (TCLC_IPC_MUTEX_ENABLE))
    pThread->LockList = (TLinkNode*)0;
#endif

    /* ��ʼ�߳����������Ϣ */
    pThread->Diagnosis = OS_THREAD_DIAG_NORMAL;

    /* �����߳�����ڵ���Ϣ���̴߳�ʱ�������κ��̶߳��� */
    pThread->LinkNode.Data = (TBase32*)(&(pThread->Priority));
    pThread->LinkNode.Prev = (TLinkNode*)0;
    pThread->LinkNode.Next = (TLinkNode*)0;
    pThread->LinkNode.Handle = (TLinkNode**)0;

    /* ���̼߳����ں��̶߳��У������߳�״̬ */
    pQueue = (status == OS_THREAD_STATUS_READY) ?
             (&ThreadReadyQueue): (&ThreadAuxiliaryQueue);
    OsThreadEnterQueue(pQueue, pThread, OsLinkTail);
    pThread->Status = status;

    /* ����߳��Ѿ���ɳ�ʼ�� */
    pThread->Property = (property| OS_THREAD_PROP_READY);
}


/*************************************************************************************************
 *  ���ܣ������߳����ȼ�                                                                         *
 *  ������(1) pThread  �߳̽ṹ��ַ                                                              *
 *        (2) priority �߳����ȼ�                                                                *
 *        (3) flag     �Ƿ�SetPriority API����                                                 *
 *        (4) pHiRP    �Ƿ���Ҫ�̵߳��ȱ��                                                      *
 *        (4) pError   ����������                                                              *
 *  ���أ�(1) eError   �����߳����ȼ�ʧ��                                                        *
 *        (2) eSuccess �����߳����ȼ��ɹ�                                                        *
 *  ˵�����������ʱ�޸����ȼ������޸��߳̽ṹ�Ļ������ȼ�                                     *
 *************************************************************************************************/
TState OsThreadSetPriority(TThread* pThread, TBase32 priority, TBool flag, TBool* pHiRP,
                           TError* pError)
{
    TState state;
    TError error;
    TBase32 HiRP;
    TBitMask property;

    switch (pThread->Status)
    {
        case (OS_THREAD_STATUS_BLOCKED):
        case (OS_THREAD_STATUS_BLOCKED |OS_THREAD_STATUS_SUSPENDED):
        {
            /* ����״̬���̶߳��ڸ���������޸������ȼ� */
            OsThreadLeaveQueue(&ThreadAuxiliaryQueue, pThread);
            pThread->Priority = priority;
            OsThreadEnterQueue(&ThreadAuxiliaryQueue, pThread, OsLinkTail);

            /* �޸��߳���IPC�������������ȼ� */
            property =*(pThread->IpcContext->Queue->Property);
            if (property & OS_IPC_PROP_PREEMP)
            {
                OsObjQueueRemoveNode(&(pThread->IpcContext->Queue->Handle),
                                     &(pThread->IpcContext->LinkNode));
                OsObjQueueAddPriorityNode(&(pThread->IpcContext->Queue->Handle),
                                          &(pThread->IpcContext->LinkNode));
            }
            state = eSuccess;
            error = OS_THREAD_ERR_NONE;
            break;
        }
        case (OS_THREAD_STATUS_READY):
        {
            /*
             * �����̵߳������ȼ�ʱ������ֱ�ӵ������ھ����̶߳����еķֶ���
             * ���ڴ��ھ����̶߳����еĵ�ǰ�̣߳�����޸��������ȼ���
             * ��Ϊ��������Ƴ��߳̾������У����Լ�ʹ�ں˲��������Ҳû����
             */
            OsThreadLeaveQueue(&ThreadReadyQueue, pThread);
            pThread->Priority = priority;
            OsThreadEnterQueue(&ThreadReadyQueue, pThread, OsLinkTail);

            /*
             * �õ���ǰ�������е���߾������ȼ�����Ϊ�����߳�(������ǰ�߳�)
             * ���߳̾��������ڵ����ڻᵼ�µ�ǰ�߳̿��ܲ���������ȼ���
             */
            if (priority < OsKernelVariable.CurrentThread->Priority)
            {
                *pHiRP = eTrue;
            }
            state = eSuccess;
            error = OS_THREAD_ERR_NONE;
            break;
        }
        case (OS_THREAD_STATUS_RUNNING):
        {
            /*
             * ���赱ǰ�߳����ȼ������Ψһ����������������ȼ�֮����Ȼ����ߣ�
             * �������µ����ȼ����ж�������̣߳���ô��ðѵ�ǰ�̷߳����µľ�������
             * ��ͷ������������������ʽ��ʱ��Ƭ��ת����ǰ�߳��Ⱥ󱻶�ε������ȼ�ʱ��
             * ֻ��ÿ�ζ��������ڶ���ͷ���ܱ�֤�����һ�ε������ȼ��󻹴��ڶ���ͷ��
             */
            OsThreadLeaveQueue(&ThreadReadyQueue, pThread);
            pThread->Priority = priority;
            OsThreadEnterQueue(&ThreadReadyQueue, pThread, OsLinkHead);

            /*
             * ��Ϊ��ǰ�߳����߳̾��������ڵ����ڻᵼ�µ�ǰ�߳̿��ܲ���������ȼ���
             * ������Ҫ���¼��㵱ǰ�������е���߾������ȼ���
             */
            HiRP = CalcThreadHiRP();
            if (HiRP < OsKernelVariable.CurrentThread->Priority)
            {
                *pHiRP = eTrue;
            }

            state = eSuccess;
            error = OS_THREAD_ERR_NONE;
            break;
        }
        case (OS_THREAD_STATUS_DORMANT):
        case (OS_THREAD_STATUS_SUSPENDED):
        case (OS_THREAD_STATUS_DELAYED):
        case (OS_THREAD_STATUS_DELAYED |OS_THREAD_STATUS_SUSPENDED):
        {
            /*����״̬���̶߳��ڸ���������޸������ȼ� */
            OsThreadLeaveQueue(&ThreadAuxiliaryQueue, pThread);
            pThread->Priority = priority;
            OsThreadEnterQueue(&ThreadAuxiliaryQueue, pThread, OsLinkTail);
            state = eSuccess;
            error = OS_THREAD_ERR_NONE;
            break;
        }
        default:
        {
            error = OS_THREAD_ERR_STATUS;
            state = eError;
            break;
        }
    }

    /* �����Ҫ���޸��̶̹߳����ȼ� */
    if (flag == eTrue)
    {
        pThread->BasePriority = priority;
    }

    *pError = error;
    return state;
}


/*************************************************************************************************
 *  ���ܣ����̴߳ӹ���״̬ת��������̬��ʹ���߳��ܹ������ں˵���                                 *
 *  ������(1) pThread   �߳̽ṹ��ַ                                                             *
 *  ���أ���                                                                                     *
 *  ˵����                                                                                       *
 *************************************************************************************************/
void OsThreadResume(TThread* pThread)
{
    /*
     * �����̣߳�����̶߳��к�״̬ת��,ע��ֻ���жϴ���ʱ��
     * ��ǰ�̲߳Żᴦ���ں��̸߳���������(��Ϊ��û���ü��߳��л�)
     * ��ǰ�̷߳��ؾ�������ʱ��һ��Ҫ�ص���Ӧ�Ķ���ͷ
     * ���߳̽�����������ʱ������Ҫ�����̵߳�ʱ�ӽ�����
     */
    if (pThread->Property & OS_THREAD_PROP_DAEMON)
    {
        if (pThread->Status == OS_THREAD_STATUS_SUSPENDED)
        {
            OsThreadLeaveQueue(&ThreadAuxiliaryQueue, pThread);
            if (pThread == OsKernelVariable.CurrentThread)
            {
                OsThreadEnterQueue(&ThreadReadyQueue, pThread, OsLinkHead);
                pThread->Status = OS_THREAD_STATUS_RUNNING;
            }
            else
            {
                OsThreadEnterQueue(&ThreadReadyQueue, pThread, OsLinkTail);
                pThread->Status = OS_THREAD_STATUS_READY;
            }
        }
    }
}


/*************************************************************************************************
 *  ���ܣ����߳��Լ�����                                                                         *
 *  ��������                                                                                     *
 *  ���أ���                                                                                     *
 *  ˵����                                                                                       *
 *************************************************************************************************/
void OsThreadSuspend(void)
{
    if (OsKernelVariable.CurrentThread->Property & OS_THREAD_PROP_DAEMON)
    {
        if (OsKernelVariable.SchedLocks == 0U)
        {
            OsThreadLeaveQueue(&ThreadReadyQueue, OsKernelVariable.CurrentThread);
            OsThreadEnterQueue(&ThreadAuxiliaryQueue,
                               OsKernelVariable.CurrentThread, OsLinkTail);
            OsKernelVariable.CurrentThread->Status = OS_THREAD_STATUS_SUSPENDED;
            OsThreadSchedule();
        }
    }
}


/* �ں�ROOT�̶߳����ջ���� */
static TThread RootThread;
static TBase32 RootThreadStack[TCLC_ROOT_THREAD_STACK_BYTES >> 2];

/*************************************************************************************************
 *  ���ܣ��ں�ROOT�̺߳���                                                                       *
 *  ������(1) argument �̵߳Ĳ���                                                                *
 *  ���أ���                                                                                     *
 *  ˵�����ú������ȿ�����������ƣ�Ȼ����������߳�����                                         *
 *        ע���߳�ջ������С�����⣬����̺߳�����Ҫ��̫�๤��                                   *
 *************************************************************************************************/
static void RootThreadEntry(TBase32 argument)
{
    /* �رմ������ж� */
    OsCpuDisableInt();

    /* ����ں˽�����߳�ģʽ */
    OsKernelVariable.State = OsThreadState;

    /* ��ʱ�ر��̵߳��ȹ��� */
    OsKernelVariable.SchedLocks = 1U;

    /*
     * �����û���ں�������ʼ���û�����
     * �ú���������OsThreadState,���ǽ�ֹSchedulable��״̬��
     */
    if(OsKernelVariable.UserEntry == (TUserEntry)0)
    {
        OsDebugPanic("", __FILE__, __FUNCTION__, __LINE__);
    }
    OsKernelVariable.UserEntry();

    /* �����̵߳��ȹ��� */
    OsKernelVariable.SchedLocks = 0U;

    /* ��ϵͳʱ�ӽ��� */
    OsCpuStartTickClock();

    /* �򿪴������ж� */
    OsCpuEnableInt();

    /* ����IDLE Hook��������ʱ���̻߳����Ѿ��� */
    while (eTrue)
    {
        if (OsKernelVariable.SysIdleEntry != (TSysIdleEntry)0)
        {
            OsKernelVariable.SysIdleEntry();
        }
    }
}


/*************************************************************************************************
 *  ���ܣ���ʼ���ں��̹߳���ģ��                                                                 *
 *  ��������                                                                                     *
 *  ���أ���                                                                                     *
 *  ˵�����ں��е��̶߳�����Ҫ��һ�¼��֣�                                                       *
 *        (1) �߳̾�������,���ڴ洢���еľ����ߺ������̡߳��ں���ֻ��һ���������С�              *
 *        (2) �̸߳�������, ���й���״̬����ʱ״̬������״̬���̶߳��洢����������С�           *
 *            ͬ���ں���ֻ��һ�����߶���                                                         *
 *        (3) IPC������߳��������У�������������������״̬���̶߳���������Ӧ���߳����������  *
 *************************************************************************************************/
void OsThreadModuleInit(void)
{
    /* ����ں��Ƿ��ڳ�ʼ״̬ */
    if (OsKernelVariable.State != OsOriginState)
    {
        OsDebugPanic("", __FILE__, __FUNCTION__, __LINE__);
    }

    memset(&ThreadReadyQueue, 0U, sizeof(ThreadReadyQueue));
    memset(&ThreadAuxiliaryQueue, 0U, sizeof(ThreadAuxiliaryQueue));

    OsKernelVariable.ThreadReadyQueue = &ThreadReadyQueue;
    OsKernelVariable.ThreadAuxiliaryQueue = &ThreadAuxiliaryQueue;

    /* ��ʼ���ں�ROOT�߳� */
    OsThreadCreate(&RootThread,
                   "ROOTD",/* kernel root thread */
                   OS_THREAD_STATUS_READY,
                   OS_THREAD_PROP_PRIORITY_FIXED|\
                   OS_THREAD_PROP_CLEAN_STACK|\
                   OS_THREAD_PROP_ROOT| OS_THREAD_PROP_DAEMON,
                   RootThreadEntry,
                   (TArgument)0,
                   (void*)RootThreadStack,
                   (TBase32)TCLC_ROOT_THREAD_STACK_BYTES,
                   (TBase32)TCLC_ROOT_THREAD_PRIORITY,
                   (TTimeTick)TCLC_ROOT_THREAD_SLICE);

    /* ��ʼ����ص��ں˱��� */
    OsKernelVariable.RootThread    = &RootThread;
    OsKernelVariable.NomineeThread = &RootThread;
    OsKernelVariable.CurrentThread = &RootThread;
}


/*************************************************************************************************
 *  ���ܣ��߳̽ṹ��ʼ������                                                                     *
 *  ������(1)  pThread  �߳̽ṹ��ַ                                                             *
 *        (2)  pName    �̵߳ĳ�ʼ״̬                                                           *
 *        (3)  pEntry   �̺߳�����ַ                                                             *
 *        (4)  argument �̺߳�������                                                             *
 *        (5)  pStack   �߳�ջ��ַ                                                               *
 *        (6)  bytes    �߳�ջ��С������Ϊ��λ                                                   *
 *        (7)  priority �߳����ȼ�                                                               *
 *        (8)  ticks    �߳�ʱ��Ƭ����                                                           *
 *        (9)  pError   ��ϸ���ý��                                                             *
 *  ���أ�(1)  eError                                                                            *
 *        (2)  eSuccess                                                                          *
 *  ˵����                                                                                       *
 *************************************************************************************************/
TState TclCreateThread(TThread* pThread, TChar* pName, TThreadEntry pEntry, TArgument argument,
                       void* pStack, TBase32 bytes, TBase32 priority, TTimeTick ticks, TError* pError)
{
    TBase32 imask;

    /* ��Ҫ�Ĳ������ */
    OS_ASSERT((pThread != (TThread*)0), "");
    OS_ASSERT((pName != (TChar*)0), "");
    OS_ASSERT((pEntry != (void*)0), "");
    OS_ASSERT((pStack != (void*)0), "");
    OS_ASSERT((bytes > 0U), "");
    OS_ASSERT((priority <= TCLC_USER_PRIORITY_LOW), "");
    OS_ASSERT((priority >= TCLC_USER_PRIORITY_HIGH), "");
    OS_ASSERT((ticks > 0U), "");

    OsCpuEnterCritical(&imask);

    /* ֻ�������̴߳�������ñ����� */
    if (OsKernelVariable.State != OsThreadState)
    {
        OsCpuLeaveCritical(imask);
        *pError = OS_THREAD_ERR_KERNEL_STATE;
        return eError;
    }

    /* ����߳��Ƿ��Ѿ�����ʼ�� */
    if (pThread->Property & OS_THREAD_PROP_READY)
    {
        OsCpuLeaveCritical(imask);
        *pError = OS_THREAD_ERR_UNREADY;
        return eError;
    }

    OsThreadCreate(pThread, pName, OS_THREAD_STATUS_DORMANT, OS_THREAD_PROP_PRIORITY_SAFE,
                   pEntry, argument, pStack, bytes, priority, ticks);

    OsCpuLeaveCritical(imask);

    *pError = OS_THREAD_ERR_NONE;
    return eSuccess;
}


/*************************************************************************************************
 *  ���ܣ��߳�ע��                                                                               *
 *  ������(1) pThread �߳̽ṹ��ַ                                                               *
 *        (2) pError  ��ϸ���ý��                                                               *
 *  ���أ�(1) eError                                                                             *
 *        (2) eSuccess                                                                           *
 *  ˵����IDLE�̡߳��жϴ����̺߳Ͷ�ʱ���̲߳��ܱ�ע��                                           *
 *************************************************************************************************/
TState TclDeleteThread(TThread* pThread, TError* pError)
{
    TState state;
    TError error;
    TBase32 imask;

    /* ��Ҫ�Ĳ������ */
    OS_ASSERT((pError != (TError*)0), "");

    OsCpuEnterCritical(&imask);

    /* ֻ�������̴߳�������ñ����� */
    if (OsKernelVariable.State != OsThreadState)
    {
        OsCpuLeaveCritical(imask);
        *pError = OS_THREAD_ERR_KERNEL_STATE;
        return eError;
    }

    /* ���û�и������������̵߳�ַ����ǿ��ʹ�õ�ǰ�߳� */
    if (pThread == (TThread*)0)
    {
        pThread = OsKernelVariable.CurrentThread;
    }

    /* ����߳��Ƿ��Ѿ�����ʼ�� */
    if (!(pThread->Property & OS_THREAD_PROP_READY))
    {
        OsCpuLeaveCritical(imask);
        *pError = OS_THREAD_ERR_UNREADY;
        return eError;
    }

    /* �ں��ػ��̲߳�����ĳЩAPI���� */
    if (pThread->Property & OS_THREAD_PROP_DAEMON)
    {
        *pError = OS_THREAD_ERR_DENIED;
        OsCpuLeaveCritical(imask);
        return eError;
    }

    if (pThread->Status == OS_THREAD_STATUS_DORMANT)
    {
#if ((TCLC_IPC_ENABLE) && (TCLC_IPC_MUTEX_ENABLE))
        if (pThread->LockList)
        {
            error = OS_THREAD_ERR_OWNLOCKS;
            state = eError;
        }
        else
#endif
        {
            OsKernelRemoveObject(&(pThread->Object));
            OsThreadLeaveQueue(&ThreadAuxiliaryQueue, pThread);
            memset(pThread, 0U, sizeof(TThread));
            error = OS_THREAD_ERR_NONE;
            state = eSuccess;
        }
    }
    else
    {
        error = OS_THREAD_ERR_STATUS;
        state = eError;
    }

    OsCpuLeaveCritical(imask);

    *pError = error;
    return state;
}


/*************************************************************************************************
 *  ���ܣ������߳����ȼ�                                                                         *
 *  ������(1) pThread  �߳̽ṹ��ַ                                                              *
 *        (2) priority �߳����ȼ�                                                                *
 *        (3) pError   ��ϸ���ý��                                                              *
 *  ���أ�(1) eError   �����߳����ȼ�ʧ��                                                        *
 *        (2) eSuccess �����߳����ȼ��ɹ�                                                        *
 *  ˵����(1) �������ʱ�޸����ȼ������޸��߳̽ṹ�Ļ������ȼ�����                             *
 *        (2) ������ʵʩ���ȼ��̳�Э���ʱ����AUTHORITY����                                    *
 *************************************************************************************************/
TState TclSetThreadPriority(TThread* pThread, TBase32 priority, TError* pError)
{
    TState state;
    TError error;
    TBool HiRP = eFalse;
    TBase32 imask;

    OS_ASSERT((priority < TCLC_PRIORITY_NUM), "");
    OS_ASSERT((pError != (TError*)0), "");

    OsCpuEnterCritical(&imask);

    /* ֻ�������̴߳�������ñ����� */
    if (OsKernelVariable.State != OsThreadState)
    {
        OsCpuLeaveCritical(imask);
        *pError = OS_THREAD_ERR_KERNEL_STATE;
        return eError;
    }

    /* ���û�и������������̵߳�ַ����ǿ��ʹ�õ�ǰ�߳� */
    if (pThread == (TThread*)0)
    {
        pThread = OsKernelVariable.CurrentThread;
    }

    /* ����߳��Ƿ��Ѿ�����ʼ�� */
    if (!(pThread->Property & OS_THREAD_PROP_READY))
    {
        OsCpuLeaveCritical(imask);
        *pError = OS_THREAD_ERR_UNREADY;
        return eError;
    }

    /* �ں��ػ��̲߳�����ĳЩAPI���� */
    if (pThread->Property & OS_THREAD_PROP_DAEMON)
    {
        OsCpuLeaveCritical(imask);
        *pError = OS_THREAD_ERR_DENIED;
        return eError;
    }

    if (pThread->Priority == priority)
    {
        OsCpuLeaveCritical(imask);
        *pError = OS_THREAD_ERR_NONE;
        return eSuccess;
    }

    if ((!(pThread->Property & OS_THREAD_PROP_PRIORITY_FIXED)) &&
            (pThread->Property & OS_THREAD_PROP_PRIORITY_SAFE))
    {
        state = OsThreadSetPriority(pThread, priority, eTrue, &HiRP, &error);
        if ((OsKernelVariable.SchedLocks == 0U) && (HiRP == eTrue))
        {
            OsThreadSchedule();
        }
    }
    else
    {
        error = OS_THREAD_ERR_PROTECTED;
        state = eError;
    }

    OsCpuLeaveCritical(imask);
    *pError = error;
    return state;

}


/*************************************************************************************************
 *  ���ܣ��޸��߳�ʱ��Ƭ����                                                                     *
 *  ������(1) pThread �߳̽ṹ��ַ                                                               *
 *        (2) slice   �߳�ʱ��Ƭ����                                                             *
 *        (3) pError  ��ϸ���ý��                                                               *
 *  ���أ�(1) eSuccess                                                                           *
 *        (2) eError                                                                           *
 *  ˵����                                                                                       *
 *************************************************************************************************/
TState TclSetThreadSlice(TThread* pThread, TTimeTick ticks, TError* pError)
{
    TBase32 imask;

    OS_ASSERT((ticks > 0U), "");
    OS_ASSERT((pError != (TError*)0), "");

    OsCpuEnterCritical(&imask);

    /* ֻ�������̴߳�������ñ����� */
    if (OsKernelVariable.State != OsThreadState)
    {
        OsCpuLeaveCritical(imask);
        *pError = OS_THREAD_ERR_KERNEL_STATE;
        return eError;
    }

    /* ���û�и������������̵߳�ַ����ǿ��ʹ�õ�ǰ�߳� */
    if (pThread == (TThread*)0)
    {
        pThread = OsKernelVariable.CurrentThread;
    }

    /* ����߳��Ƿ��Ѿ�����ʼ�� */
    if (!(pThread->Property & OS_THREAD_PROP_READY))
    {
        OsCpuLeaveCritical(imask);
        *pError = OS_THREAD_ERR_UNREADY;
        return eError;
    }

    /* �ں��ػ��̲߳�����ĳЩAPI���� */
    if (pThread->Property & OS_THREAD_PROP_DAEMON)
    {
        *pError = OS_THREAD_ERR_DENIED;
        OsCpuLeaveCritical(imask);
        return eError;
    }

    /* �����߳�ʱ��Ƭ���� */
    if (pThread->BaseTicks > ticks)
    {
        pThread->Ticks = (pThread->Ticks < ticks) ? (pThread->Ticks): ticks;
    }
    else
    {
        pThread->Ticks += (ticks - pThread->BaseTicks);
    }

    pThread->BaseTicks = ticks;
    OsCpuLeaveCritical(imask);

    *pError = OS_THREAD_ERR_NONE;
    return eSuccess;
}


/*************************************************************************************************
 *  ���ܣ��̼߳��̵߳��Ⱥ�������ǰ�߳������ó�������(���־���״̬)                               *
 *  ������(1) pError    ��ϸ���ý��                                                             *
 *  ���أ�(1) eError                                                                           *
 *        (2) eSuccess                                                                           *
 *  ˵������Ϊ�����ƻ���߾������ȼ�ռ�ô�������ԭ��                                           *
 *        ����Yield����ֻ����ӵ����߾������ȼ����߳�֮�����                                    *
 *************************************************************************************************/
TState TclYieldThread(TError* pError)
{
    TState state;
    TError error;
    TBase32 imask;
    TThread* pThread;

    OS_ASSERT((pError != (TError*)0), "");

    OsCpuEnterCritical(&imask);

    /* ֻ�������̴߳�������ñ����� */
    if (OsKernelVariable.State != OsThreadState)
    {
        OsCpuLeaveCritical(imask);
        *pError = OS_THREAD_ERR_KERNEL_STATE;
        return eError;
    }

    /* ǿ��ʹ�õ�ǰ�߳� */
    pThread = OsKernelVariable.CurrentThread;

    /* ����߳��Ƿ��Ѿ�����ʼ�� */
    if (!(pThread->Property & OS_THREAD_PROP_READY))
    {
        OsCpuLeaveCritical(imask);
        *pError = OS_THREAD_ERR_UNREADY;
        return eError;
    }

    /* �ں��ػ��̲߳�����ĳЩAPI���� */
    if (pThread->Property & OS_THREAD_PROP_DAEMON)
    {
        *pError = OS_THREAD_ERR_DENIED;
        OsCpuLeaveCritical(imask);
        return eError;
    }

    /* ֻ�����ں������̵߳��ȵ������²��ܵ��ñ����� */
    if (OsKernelVariable.SchedLocks == 0U)
    {
        /*
         * ������ǰ�߳����ڶ��е�ͷָ��
         * ��ǰ�߳������̶߳���Ҳ����ֻ�е�ǰ�߳�Ψһ1���߳�
         */
        ThreadReadyQueue.Handle[pThread->Priority] =
            (ThreadReadyQueue.Handle[pThread->Priority])->Next;
        pThread->Status = OS_THREAD_STATUS_READY;

        OsThreadSchedule();
        error = OS_THREAD_ERR_NONE;
        state = eSuccess;
    }
    else
    {
        error = OS_THREAD_ERR_SCHED_LOCKED;
        state = eError;
    }

    OsCpuLeaveCritical(imask);

    *pError = error;
    return state;
}


/*************************************************************************************************
 *  ���ܣ��߳���ֹ��ʹ���̲߳��ٲ����ں˵���                                                     *
 *  ������(1) pThread �߳̽ṹ��ַ                                                               *
 *        (2) pError  ��ϸ���ý��                                                               *
 *  ���أ�(1) eError                                                                           *
 *        (2) eSuccess                                                                           *
 *  ˵����(1) ��ʼ���̺߳Ͷ�ʱ���̲߳��ܱ�����                                                   *
 *************************************************************************************************/
TState TclDeactivateThread(TThread* pThread, TError* pError)
{
    TState state;
    TError error;
    TBase32 imask;
    OS_ASSERT((pError != (TError*)0), "");

    OsCpuEnterCritical(&imask);

    /* ֻ�������̴߳�������ñ����� */
    if (OsKernelVariable.State != OsThreadState)
    {
        OsCpuLeaveCritical(imask);
        *pError = OS_THREAD_ERR_KERNEL_STATE;
        return eError;
    }

    /* ���û�и������������̵߳�ַ����ǿ��ʹ�õ�ǰ�߳� */
    if (pThread == (TThread*)0)
    {
        pThread = OsKernelVariable.CurrentThread;
    }

    /* ����߳��Ƿ��Ѿ�����ʼ�� */
    if (!(pThread->Property & OS_THREAD_PROP_READY))
    {
        OsCpuLeaveCritical(imask);
        *pError = OS_THREAD_ERR_UNREADY;
        return eError;
    }

    /* �ں��ػ��̲߳�����ĳЩAPI���� */
    if (pThread->Property & OS_THREAD_PROP_DAEMON)
    {
        *pError = OS_THREAD_ERR_DENIED;
        OsCpuLeaveCritical(imask);
        return eError;
    }

    switch (pThread->Status)
    {
        case OS_THREAD_STATUS_RUNNING:
        {
            /* ����ں˴�ʱ��ֹ�̵߳��ȣ���ô��ǰ�̲߳��ܱ����� */
            if (OsKernelVariable.SchedLocks == 0U)
            {
                OsThreadLeaveQueue(&ThreadReadyQueue, pThread);
                OsThreadEnterQueue(&ThreadAuxiliaryQueue, pThread, OsLinkTail);
                pThread->Status = OS_THREAD_STATUS_DORMANT;
                OsThreadSchedule();
                error = OS_THREAD_ERR_NONE;
                state = eSuccess;
            }
            else
            {
                error = OS_THREAD_ERR_SCHED_LOCKED;
                state = eError;
            }
            break;
        }
        case (OS_THREAD_STATUS_READY):
        {
            /* ������������̲߳��ǵ�ǰ�̣߳��򲻻������̵߳��ȣ�����ֱ�Ӵ����̺߳Ͷ��� */
            OsThreadLeaveQueue(&ThreadReadyQueue, pThread);
            OsThreadEnterQueue(&ThreadAuxiliaryQueue, pThread, OsLinkTail);
            pThread->Status = OS_THREAD_STATUS_DORMANT;

            error = OS_THREAD_ERR_NONE;
            state = eSuccess;
            break;
        }
        default:
        {
            error = OS_THREAD_ERR_STATUS;
            state = eError;
            break;
        }
    }

    OsCpuLeaveCritical(imask);

    *pError = error;
    return state;
}


/*************************************************************************************************
 *  ���ܣ������̣߳�ʹ���߳��ܹ������ں˵���                                                     *
 *  ������(1) pThread  �߳̽ṹ��ַ                                                              *
 *        (2) pError   ��ϸ���ý��                                                              *
 *  ���أ�(1) eError                                                                           *
 *        (2) eSuccess                                                                           *
 *  ˵����                                                                                       *
 *************************************************************************************************/
TState TclActivateThread(TThread* pThread, TError* pError)
{
    TState state;
    TError error;
    TBase32 imask;

    OS_ASSERT((pThread != (TThread*)0), "");
    OS_ASSERT((pError != (TError*)0), "");

    OsCpuEnterCritical(&imask);

    /* ֻ�������̴߳�������ñ����� */
    if (OsKernelVariable.State != OsThreadState)
    {
        OsCpuLeaveCritical(imask);
        *pError = OS_THREAD_ERR_KERNEL_STATE;
        return eError;
    }

    /* ����߳��Ƿ��Ѿ�����ʼ�� */
    if (!(pThread->Property & OS_THREAD_PROP_READY))
    {
        OsCpuLeaveCritical(imask);
        *pError = OS_THREAD_ERR_UNREADY;
        return eError;
    }

    /* �ں��ػ��̲߳�����ĳЩAPI���� */
    if (pThread->Property & OS_THREAD_PROP_DAEMON)
    {
        OsCpuLeaveCritical(imask);
        *pError = OS_THREAD_ERR_DENIED;
        return eError;
    }

    /* �߳�״̬У��,ֻ��״̬���ϵ��̲߳��ܱ����� ,
       ��Ϊ�����̻߳����£����Դ�ʱpThreadһ�����ǵ�ǰ�߳� */
    if (pThread->Status == OS_THREAD_STATUS_DORMANT)
    {
        OsThreadLeaveQueue(&ThreadAuxiliaryQueue, pThread);
        OsThreadEnterQueue(&ThreadReadyQueue, pThread, OsLinkTail);
        pThread->Status = OS_THREAD_STATUS_READY;

        if ((OsKernelVariable.SchedLocks == 0U) &&
                (pThread->Priority < OsKernelVariable.CurrentThread->Priority))
        {
            OsThreadSchedule();
        }

        state = eSuccess;
        error = OS_THREAD_ERR_NONE;
    }

    OsCpuLeaveCritical(imask);

    *pError = error;
    return state;
}


/*************************************************************************************************
 *  ���ܣ��̹߳�����                                                                           *
 *  ������(1) pThread �߳̽ṹ��ַ                                                               *
 *        (2) pError  ��ϸ���ý��                                                               *
 *  ���أ�(1) eError                                                                           *
 *        (2) eSuccess                                                                           *
 *  ˵����(1) �ں˳�ʼ���̲߳��ܱ�����                                                           *
 *************************************************************************************************/
TState TclSuspendThread(TThread* pThread, TError* pError)
{
    TState state;
    TError error;
    TBase32 imask;

    OS_ASSERT((pError != (TError*)0), "");

    OsCpuEnterCritical(&imask);

    /* ֻ�������̴߳�������ñ����� */
    if (OsKernelVariable.State != OsThreadState)
    {
        OsCpuLeaveCritical(imask);
        *pError = OS_THREAD_ERR_KERNEL_STATE;
        return eError;
    }

    /* ���û�и������������̵߳�ַ����ǿ��ʹ�õ�ǰ�߳� */
    if (pThread == (TThread*)0)
    {
        pThread = OsKernelVariable.CurrentThread;
    }

    /* ����߳��Ƿ��Ѿ�����ʼ�� */
    if (!(pThread->Property & OS_THREAD_PROP_READY))
    {
        OsCpuLeaveCritical(imask);
        *pError = OS_THREAD_ERR_UNREADY;
        return eError;
    }

    /* �ں��ػ��̲߳�����ĳЩAPI���� */
    if (pThread->Property & OS_THREAD_PROP_DAEMON)
    {
        OsCpuLeaveCritical(imask);
        *pError = OS_THREAD_ERR_DENIED;
        return eError;
    }

    switch (pThread->Status)
    {
        case (OS_THREAD_STATUS_RUNNING):
        {
            /* ����ں˴�ʱ��ֹ�̵߳��ȣ���ô��ǰ�̲߳��ܱ����� */
            if (OsKernelVariable.SchedLocks == 0U)
            {
                OsThreadLeaveQueue(&ThreadReadyQueue, pThread);
                OsThreadEnterQueue(&ThreadAuxiliaryQueue, pThread, OsLinkTail);
                pThread->Status = OS_THREAD_STATUS_SUSPENDED;
                error = OS_THREAD_ERR_NONE;
                state = eSuccess;
                OsThreadSchedule();
            }
            else
            {
                error = OS_THREAD_ERR_SCHED_LOCKED;
                state = eError;
            }
            break;
        }
        case (OS_THREAD_STATUS_READY):
        {
            /* ������������̲߳��ǵ�ǰ�̣߳��򲻻������̵߳��ȣ�����ֱ�Ӵ����̺߳Ͷ��� */
            OsThreadLeaveQueue(&ThreadReadyQueue, pThread);
            OsThreadEnterQueue(&ThreadAuxiliaryQueue, pThread, OsLinkTail);
            pThread->Status = OS_THREAD_STATUS_SUSPENDED;
            error = OS_THREAD_ERR_NONE;
            state = eSuccess;
            break;
        }
        case (OS_THREAD_STATUS_BLOCKED):
        case (OS_THREAD_STATUS_DELAYED):
        {
            pThread->Status |= OS_THREAD_STATUS_SUSPENDED;
            error = OS_THREAD_ERR_NONE;
            state = eSuccess;
            break;
        }
        case (OS_THREAD_STATUS_SUSPENDED):
        case (OS_THREAD_STATUS_SUSPENDED|OS_THREAD_STATUS_BLOCKED):
        case (OS_THREAD_STATUS_SUSPENDED|OS_THREAD_STATUS_DELAYED):
        {
            error = OS_THREAD_ERR_NONE;
            state = eSuccess;
            break;
        }
        default:
        {
            error = OS_THREAD_ERR_STATUS;
            state = eError;
            break;
        }
    }

    OsCpuLeaveCritical(imask);
    *pError = error;
    return state;
}


/*************************************************************************************************
 *  ���ܣ��߳̽�Һ���                                                                           *
 *  ������(1) pThread �߳̽ṹ��ַ                                                               *
 *        (2) pError  ��ϸ���ý��                                                               *
 *  ���أ�(1) eError                                                                           *
 *        (2) eSuccess                                                                           *
 *  ˵����                                                                                       *
 *************************************************************************************************/
TState TclResumeThread(TThread* pThread, TError* pError)
{
    TState state;
    TError error;
    TBase32 imask;

    OS_ASSERT((pThread != (TThread*)0), "");
    OS_ASSERT((pError != (TError*)0), "");

    OsCpuEnterCritical(&imask);

    /* ֻ�������̴߳�������ñ����� */
    if (OsKernelVariable.State != OsThreadState)
    {
        OsCpuLeaveCritical(imask);
        *pError = OS_THREAD_ERR_KERNEL_STATE;
        return eError;
    }

    /* ����߳��Ƿ��Ѿ�����ʼ�� */
    if (!(pThread->Property & OS_THREAD_PROP_READY))
    {
        OsCpuLeaveCritical(imask);
        *pError = OS_THREAD_ERR_UNREADY;
        return eError;
    }

    /* �ں��ػ��̲߳�����ĳЩAPI���� */
    if (pThread->Property & OS_THREAD_PROP_DAEMON)
    {
        OsCpuLeaveCritical(imask);
        *pError = OS_THREAD_ERR_DENIED;
        return eError;
    }

    /* �߳�״̬У��,ֻ��״̬���ϵ��̲߳��ܱ����� */
    switch (pThread->Status)
    {
        case OS_THREAD_STATUS_SUSPENDED:
        {
            /* ��Ϊ�����̻߳����£����Դ�ʱpThreadһ�����ǵ�ǰ�߳� */
            OsThreadLeaveQueue(&ThreadAuxiliaryQueue, pThread);
            OsThreadEnterQueue(&ThreadReadyQueue, pThread, OsLinkTail);
            pThread->Status = OS_THREAD_STATUS_READY;

            if (pThread->Priority < OsKernelVariable.CurrentThread->Priority)
            {
                OsThreadSchedule();
            }

            error = OS_THREAD_ERR_NONE;
            state = eSuccess;
            break;
        }
        case (OS_THREAD_STATUS_SUSPENDED|OS_THREAD_STATUS_BLOCKED):
        case (OS_THREAD_STATUS_SUSPENDED|OS_THREAD_STATUS_DELAYED):
        {
            pThread->Status &= (~OS_THREAD_STATUS_SUSPENDED);
            error = OS_THREAD_ERR_NONE;
            state = eSuccess;
            break;
        }
        default:
        {
            error = OS_THREAD_ERR_STATUS;
            state = eError;
            break;
        }
    }

    OsCpuLeaveCritical(imask);

    *pError = error;
    return state;
}


/*************************************************************************************************
 *  ���ܣ��߳���ʱģ��ӿں���                                                                   *
 *  ������(1) pThread �߳̽ṹ��ַ                                                               *
 *        (2) ticks   ��Ҫ��ʱ�ĵδ���Ŀ                                                         *
 *        (3) pError  ��ϸ���ý��                                                               *
 *  ���أ�(1) eSuccess                                                                           *
 *        (2) eError                                                                           *
 *  ˵����                                                                                       *
 *************************************************************************************************/
TState TclDelayThread(TTimeTick ticks, TError* pError)
{
    TState state;
    TError error;
    TBase32 imask;
    TThread* pThread;

    OS_ASSERT((ticks > 0U), "");
    OS_ASSERT((pError != (TError*)0), "");

    OsCpuEnterCritical(&imask);

    /* ֻ�������̴߳�������ñ����� */
    if (OsKernelVariable.State != OsThreadState)
    {
        OsCpuLeaveCritical(imask);
        *pError = OS_THREAD_ERR_KERNEL_STATE;
        return eError;
    }

    /* ǿ��ʹ�õ�ǰ�߳� */
    pThread = OsKernelVariable.CurrentThread;

    /* ����߳��Ƿ��Ѿ�����ʼ�� */
    if (!(pThread->Property & OS_THREAD_PROP_READY))
    {
        OsCpuLeaveCritical(imask);
        *pError = OS_THREAD_ERR_UNREADY;
        return eError;
    }

    /* �ں��ػ��̲߳�����ĳЩAPI���� */
    if (pThread->Property & OS_THREAD_PROP_DAEMON)
    {
        OsCpuLeaveCritical(imask);
        *pError = OS_THREAD_ERR_DENIED;
        return eError;
    }

    switch (pThread->Status)
    {
        case (OS_THREAD_STATUS_RUNNING):
        {
            /* ����ں˴�ʱ��ֹ�̵߳��ȣ���ô��ǰ�̲߳��ܱ����� */
            if (OsKernelVariable.SchedLocks == 0U)
            {
                OsThreadLeaveQueue(&ThreadReadyQueue, pThread);
                OsThreadEnterQueue(&ThreadAuxiliaryQueue, pThread, OsLinkTail);
                pThread->Status = OS_THREAD_STATUS_DELAYED;

                /* ���ò������̶߳�ʱ�� */
                pThread->Timer.Ticks = ticks;
                OsObjListAddDiffNode(&(OsKernelVariable.ThreadTimerList), &(pThread->Timer.LinkNode));

                OsThreadSchedule();
                error = OS_THREAD_ERR_NONE;
                state= eSuccess;
            }
            else
            {
                error = OS_THREAD_ERR_SCHED_LOCKED;
                state = eError;
            }
            break;
        }
        case (OS_THREAD_STATUS_READY):
        {
            /* ������������̲߳��ǵ�ǰ�̣߳��򲻻������̵߳��ȣ�����ֱ�Ӵ����̺߳Ͷ��� */
            OsThreadLeaveQueue(&ThreadReadyQueue, pThread);
            OsThreadEnterQueue(&ThreadAuxiliaryQueue, pThread, OsLinkTail);
            pThread->Status = OS_THREAD_STATUS_DELAYED;

            /* ���ò������̶߳�ʱ�� */
            pThread->Timer.Ticks = ticks;
            OsObjListAddDiffNode(&(OsKernelVariable.ThreadTimerList), &(pThread->Timer.LinkNode));

            error = OS_THREAD_ERR_NONE;
            state = eSuccess;
            break;
        }
        case (OS_THREAD_STATUS_DELAYED):
        case (OS_THREAD_STATUS_DELAYED |OS_THREAD_STATUS_SUSPENDED):
        {
            /* ���̶߳�ʱ���Ӳ�ֶ������Ƴ�,���ò������̶߳�ʱ�� */
            OsObjListRemoveDiffNode(&(OsKernelVariable.ThreadTimerList), &(pThread->Timer.LinkNode));
            pThread->Timer.Ticks = ticks;
            OsObjListAddDiffNode(&(OsKernelVariable.ThreadTimerList), &(pThread->Timer.LinkNode));
            error = OS_THREAD_ERR_NONE;
            state = eSuccess;
            break;
        }
        default:
        {
            error = OS_THREAD_ERR_STATUS;
            state = eError;
            break;
        }
    }

    OsCpuLeaveCritical(imask);

    *pError = error;
    return state;
}


/*************************************************************************************************
 *  ���ܣ��߳���ʱȡ������                                                                       *
 *  ������(1) pThread �߳̽ṹ��ַ                                                               *
 *        (2) pError  ��ϸ���ý��                                                               *
 *  ���أ�(1) eSuccess                                                                           *
 *        (2) eError                                                                           *
 *  ˵����(1) �����������ʱ�޵ȴ���ʽ������IPC�߳����������ϵ��߳���Ч                          *
 *************************************************************************************************/
TState TclUndelayThread(TThread* pThread, TError* pError)
{
    TState state;
    TError error;
    TBase32 imask;

    OS_ASSERT((pThread != (TThread*)0), "");
    OS_ASSERT((pError != (TError*)0), "");

    OsCpuEnterCritical(&imask);

    /* ֻ�������̴߳�������ñ����� */
    if (OsKernelVariable.State != OsThreadState)
    {
        OsCpuLeaveCritical(imask);
        *pError = OS_THREAD_ERR_KERNEL_STATE;
        return eError;
    }

    /* ����߳��Ƿ��Ѿ�����ʼ�� */
    if (!(pThread->Property & OS_THREAD_PROP_READY))
    {
        OsCpuLeaveCritical(imask);
        *pError = OS_THREAD_ERR_UNREADY;
        return eError;
    }

    /* �ں��ػ��̲߳�����ĳЩAPI���� */
    if (pThread->Property & OS_THREAD_PROP_DAEMON)
    {
        OsCpuLeaveCritical(imask);
        *pError = OS_THREAD_ERR_DENIED;
        return eError;
    }

    switch (pThread->Status)
    {
        case OS_THREAD_STATUS_DELAYED:
        {
            /* ��Ϊ�����̻߳����£����Դ�ʱpThreadһ�����ǵ�ǰ�߳� */
            OsThreadLeaveQueue(&ThreadAuxiliaryQueue, pThread);
            OsThreadEnterQueue(&ThreadReadyQueue, pThread, OsLinkTail);
            pThread->Status = OS_THREAD_STATUS_READY;

            /* ֹͣ�̶߳�ʱ�� */
            OsObjListRemoveDiffNode(&(OsKernelVariable.ThreadTimerList), &(pThread->Timer.LinkNode));

            if (pThread->Priority < OsKernelVariable.CurrentThread->Priority)
            {
                OsThreadSchedule();
            }
            error = OS_THREAD_ERR_NONE;
            state = eSuccess;
            break;
        }
        case (OS_THREAD_STATUS_SUSPENDED|OS_THREAD_STATUS_DELAYED):
        {
            pThread->Status &= (~OS_THREAD_STATUS_DELAYED);
            /* ֹͣ�̶߳�ʱ�� */
            OsObjListRemoveDiffNode(&(OsKernelVariable.ThreadTimerList), &(pThread->Timer.LinkNode));
            error = OS_THREAD_ERR_NONE;
            state = eSuccess;
            break;
        }
        default:
        {
            error = OS_THREAD_ERR_STATUS;
            state = eError;
            break;
        }
    }

    OsCpuLeaveCritical(imask);

    *pError = error;
    return state;
}


#if (TCLC_IPC_ENABLE)
/*************************************************************************************************
 *  ���ܣ�����߳���������                                                                       *
 *  ������(1) pThread �߳̽ṹ��ַ                                                               *
 *        (2) pError  ��ϸ���ý��                                                               *
 *  ���أ�(1) eError                                                                           *
 *        (2) eSuccess                                                                           *
 *  ˵����                                                                                       *
 *************************************************************************************************/
TState TclUnblockThread(TThread* pThread, TError* pError)
{
    TState state;
    TError error;
    TBool HiRP = eFalse;
    TBase32 imask;

    OS_ASSERT((pThread != (TThread*)0), "");
    OS_ASSERT((pError != (TError*)0), "");

    OsCpuEnterCritical(&imask);

    /* ֻ�������̴߳�������ñ����� */
    if (OsKernelVariable.State != OsThreadState)
    {
        OsCpuLeaveCritical(imask);
        *pError = OS_THREAD_ERR_KERNEL_STATE;
        return eError;
    }

    /* ����߳��Ƿ��Ѿ�����ʼ�� */
    if (!(pThread->Property & OS_THREAD_PROP_READY))
    {
        OsCpuLeaveCritical(imask);
        *pError = OS_THREAD_ERR_UNREADY;
        return eError;
    }

    /* �ں��ػ��̲߳�����ĳЩAPI���� */
    if (pThread->Property & OS_THREAD_PROP_DAEMON)
    {
        OsCpuLeaveCritical(imask);
        *pError = OS_THREAD_ERR_DENIED;
        return eError;
    }

    if (pThread->Status & OS_THREAD_STATUS_BLOCKED)
    {
        /*
         * �����������ϵ�ָ�������߳��ͷ�
         * ���̻߳����£������ǰ�̵߳����ȼ��Ѿ��������߳̾������е�������ȼ���
         * �����ں˴�ʱ��û�йر��̵߳��ȣ���ô����Ҫ����һ���߳���ռ
         */
        OsIpcUnblockThread(pThread->IpcContext, eError, OS_IPC_ERR_ABORT, &HiRP);
        if ((OsKernelVariable.SchedLocks == 0U) && (HiRP == eTrue))
        {
            OsThreadSchedule();
        }
        error = OS_THREAD_ERR_NONE;
        state = eSuccess;
    }
    else
    {
        error = OS_THREAD_ERR_STATUS;
        state = eError;
    }

    OsCpuLeaveCritical(imask);

    *pError = error;
    return state;
}
#endif

