/*************************************************************************************************
 *                                     Trochili RTOS Kernel                                      *
 *                                  Copyright(C) 2017 LIUXUMING                                  *
 *                                       www.trochili.com                                        *
 *************************************************************************************************/
#include <string.h>

#include "tcl.types.h"
#include "tcl.config.h"
#include "tcl.object.h"
#include "tcl.kernel.h"
#include "tcl.timer.h"
#include "tcl.thread.h"
#include "tcl.ipc.h"

#if (TCLC_IPC_ENABLE)

/*************************************************************************************************
 *  ���ܣ����̷߳�����Դ��������                                                                 *
 *  ������(1) pContext���������ַ                                                               *
 *        (2) pIpc    ���ڲ�����IPC����ĵ�ַ                                                    *
 *        (3) pQueue  �̶߳��нṹ��ַ                                                           *
 *        (4) data    ָ������Ŀ�����ָ���ָ��                                                 *
 *        (5) len     ���ݵĳ���                                                                 *
 *        (6) option  ����IPC����ʱ�ĸ��ֲ���                                                    *
 *        (7) ticks   ʱ�޵ȴ���ʽ�µ�����ʱ�䳤��                                               *
 *        (8) pState  IPC������ʽ��                                                            *
 *        (9) pError  ��ϸ���ý��                                                               *
 *        (10)pHiRP   �Ƿ���ǰ�߳�������������Ҫ�����̵߳��ȵı��                             *
 *  ���أ���                                                                                     *
 *  ˵����dataָ���ָ�룬������Ҫͨ��IPC���������ݵ��������߳̿ռ��ָ��                        *
 *************************************************************************************************/
void OsIpcBlockThread(TIpcContext* pContext, void* pIpc, TIpcQueue* pQueue,
                      TBase32 data, TBase32 len, TBitMask option, TTimeTick ticks,
                      TState* pState, TError* pError, TBool* pHiRP)
{
    TThread* pThread;

    /* ����̵߳�ַ */
    pThread = OsKernelVariable.CurrentThread;

    /* ֻ�д�������״̬���̲߳ſ��Ա����� */
    if (pThread->Status != OS_THREAD_STATUS_RUNNING)
    {
        OsKernelVariable.Diagnosis |= OS_KERNEL_DIAG_THREAD_ERROR;
        pThread->Diagnosis |= OS_THREAD_DIAG_INVALID_STATE;
        OsDebugPanic("", __FILE__, __FUNCTION__, __LINE__);
    }

    /* �����ǰ�̲߳��ܱ���������ֱ�ӷ��� */
    if (pThread ->Property & OS_THREAD_PROP_DAEMON)
    {
        *pState = eError;
        *pError = OS_IPC_ERR_DENIDED;
        *pHiRP  = eFalse;
        return;
    }

    pThread->IpcContext  = pContext;
    pContext->Owner      = (void*)pThread;
    pContext->Object     = pIpc;
    pContext->Queue      = pQueue;
    pContext->Data.Value = data;
    pContext->Length     = len;
    pContext->Option     = option;
    pContext->State      = pState;
    pContext->Error      = pError;

    pContext->LinkNode.Next   = (TLinkNode*)0;
    pContext->LinkNode.Prev   = (TLinkNode*)0;
    pContext->LinkNode.Handle = (TLinkNode**)0;
    pContext->LinkNode.Data   = (TBase32*)(&(pThread->Priority));

    /* ���̷߳����ں��̸߳������� */
    OsThreadLeaveQueue(OsKernelVariable.ThreadReadyQueue, pThread);
    OsThreadEnterQueue(OsKernelVariable.ThreadAuxiliaryQueue, pThread, OsLinkTail);
    pThread->Status = OS_THREAD_STATUS_BLOCKED;

    /* ���߳�ͬʱ�����������С�
       �������֧����ռģʽ���߳����ȼ����Ŷӣ���������Ϣ�������Ŷӣ�
       ����֮�⣬��Ϣ�ڷ�����Ϣ����ʱ��Ҳ��ͳһ����һ�ν�����Ϣ�ļӼ����ԡ�
       */
    if ((*(pQueue->Property)) & OS_IPC_PROP_PREEMP)
    {
        OsObjQueueAddPriorityNode(&(pQueue->Handle), &(pContext->LinkNode));
    }
    else
    {
        if ((pContext->Option) & OS_IPC_OPT_UARGENT)
        {
            OsObjQueueAddFifoNode(&(pQueue->Handle), &(pContext->LinkNode), OsLinkHead);
        }
        else
        {
            OsObjQueueAddFifoNode(&(pQueue->Handle), &(pContext->LinkNode), OsLinkTail);
        }
    }

    /* �����߳��������� */
    pContext->Queue = pQueue;

    /* �����Ҫ�������߳����ڷ�����Դ��ʱ�޶�ʱ�� */
    if ((option & OS_IPC_OPT_TIMEO) && (ticks > 0U))
    {
        pThread->Timer.Ticks = ticks;
        OsObjListAddDiffNode(&(OsKernelVariable.ThreadTimerList),
                             &(pThread->Timer.LinkNode));
    }

    *pState = eSuccess;
    *pError = OS_IPC_ERR_NONE;
    *pHiRP  = eTrue;
}


/*************************************************************************************************
 *  ���ܣ�����IPC����������ָ�����߳�                                                            *
 *  ������(1) pContext���������ַ                                                               *
 *        (2) state   �߳���Դ���ʷ��ؽ��                                                       *
 *        (3) error   ��ϸ���ý��                                                               *
 *        (4) pHiRP   �Ƿ����Ѹ������ȼ���������Ҫ�����̵߳��ȵı��                           *
 *  ���أ���                                                                                     *
 *  ˵����                                                                                       *
 *************************************************************************************************/
void OsIpcUnblockThread(TIpcContext* pContext, TState state, TError error, TBool* pHiRP)
{
    TThread* pThread;
    pThread = (TThread*)(pContext->Owner);

    /*
     * �����̣߳�����̶߳��к�״̬ת��,ע��ֻ���жϴ���ʱ��
     * ��ǰ�̲߳Żᴦ���ں��̸߳���������(��Ϊ��û���ü��߳��л�)
     * ��ǰ�̷߳��ؾ�������ʱ��һ��Ҫ�ص���Ӧ�Ķ���ͷ
     * ���߳̽�����������ʱ������Ҫ�����̵߳�ʱ�ӽ�����
     */
    if (pThread->Status == OS_THREAD_STATUS_BLOCKED)
    {
        /* ���̴߳��ں��̸߳����������Ƴ� */
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

        /* ���̴߳������������Ƴ� */
        OsObjQueueRemoveNode(&(pContext->Queue->Handle), &(pContext->LinkNode));
        pContext->Queue = (TIpcQueue*)0;

        /* �����̷߳�����Դ�Ľ���ʹ������ */
        *(pContext->State) = state;
        *(pContext->Error) = error;

        /* ����߳�����ʱ�޷�ʽ������Դ��رո��̵߳�ʱ�޶�ʱ�� */
        if (pContext->Option & OS_IPC_OPT_TIMEO)
        {
            OsObjListRemoveDiffNode(&(OsKernelVariable.ThreadTimerList),
                                    &(pThread->Timer.LinkNode));
        }

        /* �����̵߳���������,�˱��ֻ���̻߳�������Ч��
         * ��ISR���ǰ�߳̿������κζ��������ǰ�߳���Ƚ����ȼ�Ҳ��������ġ�
         * ���̻߳����£������ǰ�̵߳����ȼ��Ѿ��������߳̾������е�������ȼ���
         * �����ں˴�ʱ��û�йر��̵߳��ȣ���ô����Ҫ����һ���߳���ռ
         */
        if (pThread->Priority < OsKernelVariable.CurrentThread->Priority)
        {
            *pHiRP = eTrue;
        }
    }
    else if (pThread->Status == OS_THREAD_STATUS_BLOCKED | OS_THREAD_STATUS_SUSPENDED)
    {
        /* ���̴߳������������Ƴ� */
        OsObjQueueRemoveNode(&(pContext->Queue->Handle), &(pContext->LinkNode));
        pContext->Queue = (TIpcQueue*)0;

        /* �����̷߳�����Դ�Ľ���ʹ������ */
        *(pContext->State) = state;
        *(pContext->Error) = error;

        /* ����߳�����ʱ�޷�ʽ������Դ��رո��̵߳�ʱ�޶�ʱ�� */
        if (pContext->Option & OS_IPC_OPT_TIMEO)
        {
            OsObjListRemoveDiffNode(&(OsKernelVariable.ThreadTimerList),
                                    &(pThread->Timer.LinkNode));
        }
        pThread->Status = OS_THREAD_STATUS_SUSPENDED;
    }
    else
    {
        OsKernelVariable.Diagnosis |= OS_KERNEL_DIAG_THREAD_ERROR;
        pThread->Diagnosis |= OS_THREAD_DIAG_INVALID_STATE;
        OsDebugPanic("", __FILE__, __FUNCTION__, __LINE__);
    }
}


/*************************************************************************************************
 *  ���ܣ�ѡ�������������е�ȫ���߳�                                                           *
 *  ������(1) pQueue  �̶߳��нṹ��ַ                                                           *
 *        (2) state   �߳���Դ���ʷ��ؽ��                                                       *
 *        (3) error   ��ϸ���ý��                                                               *
 *        (4) pData   �̷߳���IPC�õ�������                                                      *
 *        (5) pHiRP  �߳��Ƿ���Ҫ���ȵı��                                                      *
 *  ���أ�                                                                                       *
 *  ˵����ֻ���������Ϣ���й㲥ʱ�Żᴫ��pData2����                                             *
 *************************************************************************************************/
void OsIpcUnblockAll(TIpcQueue* pQueue, TState state, TError error, void** pData2, TBool* pHiRP)
{
    TIpcContext* pContext;

    /* �������е��߳���һ������� */
    while (pQueue->Handle != (TLinkNode*)0)
    {
        pContext = OS_CONTAINER_OF(pQueue->Handle, TIpcContext, LinkNode);
        OsIpcUnblockThread(pContext, state, error, pHiRP);

        /* ���������������߳����ڵȴ���ȡ���� */
        if ((pContext->Option & OS_IPC_OPT_READ_DATA) &&
                (pContext->Data.Addr2 != (void**)0) &&
                (pData2 != (void**)0) )
        {
            *(pContext->Data.Addr2) = *pData2;
        }
    }
}


/*************************************************************************************************
 *  ���ܣ���������̵߳�IPC�������Ϣ                                                            *
 *  ������(1) pContext ���������ַ                                                              *
 *  ���أ���                                                                                     *
 *  ˵����                                                                                       *
 *************************************************************************************************/
void OsIpcCleanContext(TIpcContext* pContext)
{
    TThread* pThread;

    pThread = (TThread*)(pContext->Owner);
    pThread->IpcContext = (TIpcContext*)0;

    memset(pContext, 0U, sizeof(TIpcContext));
}

#endif

