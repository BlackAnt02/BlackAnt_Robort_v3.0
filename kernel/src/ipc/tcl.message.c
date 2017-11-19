/*************************************************************************************************
 *                                     Trochili RTOS Kernel                                      *
 *                                  Copyright(C) 2017 LIUXUMING                                  *
 *                                       www.trochili.com                                        *
 *************************************************************************************************/
#include <string.h>

#include "tcl.types.h"
#include "tcl.config.h"
#include "tcl.cpu.h"
#include "tcl.thread.h"
#include "tcl.kernel.h"
#include "tcl.ipc.h"
#include "tcl.message.h"

#if ((TCLC_IPC_ENABLE)&&(TCLC_IPC_MSGQUE_ENABLE))


/*************************************************************************************************
 *  ���ܣ�����Ϣ���浽��Ϣ����                                                                   *
 *  ������(1) pMsgQue ��Ϣ���нṹָ��                                                           *
 *        (2) pMsg2   ������Ϣ�ṹ��ַ��ָ�����                                                 *
 *        (3) type    ��Ϣ�����ͣ�������Ϣ����ͨ��Ϣ��                                           *
 *  ���أ���                                                                                     *
 *  ˵����(1) ������Ϣ���Ͳ�ͬ������Ϣ���浽��Ϣ���ж�ͷ���߶�β                                 *
 *        (2) ����������ܴ�����Ϣ����״̬                                                       *
 *************************************************************************************************/
static void SaveMessage(TMsgQueue* pMsgQue, void** pMsg2, TBitMask option)
{
    /* ������Ϣ���͵���Ϣ����β */
    if (option & OS_IPC_OPT_UARGENT)
    {
        if (pMsgQue->Tail == 0U)
        {
            pMsgQue->Tail = pMsgQue->Capacity - 1U;
        }
        else
        {
            pMsgQue->Tail--;
        }
        *(pMsgQue->MsgPool + pMsgQue->Tail) = *pMsg2;
    }
    /* ��ͨ��Ϣֱ�ӷ��͵���Ϣ����ͷ */
    else
    {
        *(pMsgQue->MsgPool + pMsgQue->Head) = *pMsg2;
        pMsgQue->Head++;
        if (pMsgQue->Head == pMsgQue->Capacity)
        {
            pMsgQue->Head = 0U;
        }
    }

    /* ������Ϣ������Ϣ��Ŀ */
    pMsgQue->MsgEntries++;
}


/*************************************************************************************************
 *  ���ܣ�����Ϣ����Ϣ�����ж���                                                                 *
 *  ������(1) pMsgQue ��Ϣ���нṹָ��                                                           *
 *        (2) pMsg2   ������Ϣ�ṹ��ַ��ָ�����                                                 *
 *  ���أ���                                                                                     *
 *  ˵����(1) ����Ϣ�����ж�ȡ��Ϣ��ʱ��ֻ�ܴӶ�β��ȡ                                         *
 *        (2) ����������ܴ�����Ϣ����״̬                                                       *
 *************************************************************************************************/
static void ConsumeMessage(TMsgQueue* pMsgQue, void** pMsg2)
{
    /* ����Ϣ�����ж�ȡһ����Ϣ����ǰ�߳� */
    *pMsg2 = *(pMsgQue->MsgPool + pMsgQue->Tail);

    /* ������Ϣ������Ϣ��Ŀ */
    pMsgQue->MsgEntries--;

    /* ������Ϣ���е���Ϣ��д�α� */
    pMsgQue->Tail++;
    if (pMsgQue->Tail == pMsgQue->Capacity)
    {
        pMsgQue->Tail = 0U;
    }
}


/*************************************************************************************************
 *  ���ܣ��߳�/ISR���Դ���Ϣ�����ж�ȡ��Ϣ                                                       *
 *  ������(1) pMsgQue ��Ϣ���еĵ�ַ                                                             *
 *        (2) pMsg2   ������Ϣ�ṹ��ַ��ָ�����                                                 *
 *        (3) pHiRP   �Ƿ���Ҫ�̵߳��ȱ��                                                       *
 *        (4) pError  ��ϸ���ý��                                                               *
 *  ����: (1) eError  ����ʧ��                                                                   *
 *        (2) eSuccess�����ɹ�                                                                   *
 *  ˵����                                                                                       *
 *************************************************************************************************/
/* ������Ϣ���ж����ݵ����
   (1) ������Ϣ����Ϊ��������������������1����Ϣ���У���ȡ�����ᵼ����Ϣ����״̬����ֱ�ӵ��գ�
       ������Ϣ���н��� OsMQPartial ״̬
   (2) ������Ϣ������ͨ������������Ϣ������ֻ��1����Ϣ�����ж�ȡ�������ܵ�����Ϣ���н����״̬
       ������Ϣ���б��� OsMQPartial ״̬
   (3) ��������Ϣ������������Ϣ������ͨ״̬������״ֻ̬����ͨ�Ϳա�
 */
static TState ReceiveMessage(TMsgQueue* pMsgQue, void** pMsg2, TBool* pHiRP, TError* pError)
{
    TState state = eSuccess;
    TError error = OS_IPC_ERR_NONE;
    TIpcContext* pContext;

    /* �����Ϣ����״̬ */
    if (pMsgQue->Status == OsMQEmpty)
    {
        error = OS_IPC_ERR_NORMAL;
        state = eError;
    }
    else if (pMsgQue->Status == OsMQFull)
    {
        /* ����Ϣ�����ж�ȡһ����Ϣ����ǰ�߳� */
        ConsumeMessage(pMsgQue, pMsg2);

        /*
         * ����Ϣ�������������߳�������д�����е�����£�
         * ��Ҫ�����ʵ��߳̽Ӵ��������ҽ����߳�Я������Ϣд����У�
         * ���Ա�����Ϣ����״̬����
         */
        if (pMsgQue->Queue.Handle != (TLinkNode*)0)
        {
            pContext = OS_CONTAINER_OF(pMsgQue->Queue.Handle, TIpcContext, LinkNode);
            OsIpcUnblockThread(pContext, eSuccess, OS_IPC_ERR_NONE, pHiRP);

            /* �����߳������ķֶ����ж���Ϣ����,���ҽ����̷߳��͵���Ϣд����Ϣ���� */
            SaveMessage(pMsgQue, pContext->Data.Addr2, pContext->Option);
        }
        else
        {
            pMsgQue->Status = (pMsgQue->Tail == pMsgQue->Head) ? OsMQEmpty : OsMQPartial;
        }
    }
    else
        /* if (mq->Status == OsMQPartial) */
    {
        /* ����Ϣ�����ж�ȡһ����Ϣ����ǰ�߳� */
        ConsumeMessage(pMsgQue, pMsg2);
        pMsgQue->Status = (pMsgQue->Tail == pMsgQue->Head) ? OsMQEmpty : OsMQPartial;
    }

    *pError = error;
    return state;
}


/*************************************************************************************************
 *  ���ܣ��߳�/ISR��������Ϣ�����з�����Ϣ                                                       *
 *  ������(1) pMsgQue ��Ϣ���еĵ�ַ                                                             *
 *        (2) pMsg2   ������Ϣ�ṹ��ַ��ָ�����                                                 *
 *        (3) type    ��Ϣ����                                                                   *
 *        (4) pHiRP   �Ƿ���Ҫ�̵߳��ȱ��                                                       *
 *        (5) pError  ��ϸ���ý��                                                               *
 *  ����: (1) eError   ����ʧ��                                                                *
 *        (2) eSuccess   �����ɹ�                                                                *
 *  ˵����                                                                                       *
 *************************************************************************************************/
static TState SendMessage(TMsgQueue* pMsgQue, void** pMsg2, TBitMask option, TBool* pHiRP,
                          TError* pError)
{
    TState state = eSuccess;
    TError error = OS_IPC_ERR_NONE;
    TIpcContext* pContext;

    /* �����Ϣ����״̬�������Ϣ�������򷵻�ʧ�� */
    if (pMsgQue->Status == OsMQFull)
    {
        error = OS_IPC_ERR_NORMAL;
        state = eError;
    }
    else if (pMsgQue->Status == OsMQEmpty)
    {
        /*
         * ����Ϣ����Ϊ�յ�����£�������������̵߳ȴ�����˵���Ƕ��������У�
         * ������һ���߳̽��������������Ϣ���͸����̣߳�ͬʱ������Ϣ����״̬����
         */
        if (pMsgQue->Queue.Handle != (TLinkNode*)0)
        {
            pContext = OS_CONTAINER_OF(pMsgQue->Queue.Handle, TIpcContext, LinkNode);
            OsIpcUnblockThread(pContext, eSuccess, OS_IPC_ERR_NONE, pHiRP);
            *(pContext->Data.Addr2) = *pMsg2;
        }
        else
        {
            /* ���̷߳��͵���Ϣд����Ϣ���� */
            SaveMessage(pMsgQue, pMsg2, option);
            pMsgQue->Status = (pMsgQue->Tail == pMsgQue->Head) ? OsMQFull : OsMQPartial;
        }
    }
    else
        /* if (mq->Status == OsMQPartial) */
    {
        /* ���̷߳��͵���Ϣд����Ϣ���� */
        SaveMessage(pMsgQue, pMsg2, option);

        /*
         * ��Ϣ���пյ�������������Ϣ���е�������1����ô����״̬�ӿ�ֱ�ӵ� OsMQFull��
         * ������Ϣ���н��� OsMQPartial ״̬;
         * ��Ϣ������ͨ���������Ϣ����д�������ܵ�����Ϣ���н��� OsMQFull
         * ״̬���߱��� OsMQPartial ״̬
         */
        pMsgQue->Status = (pMsgQue->Tail == pMsgQue->Head) ? OsMQFull : OsMQPartial;
    }

    *pError = error;
    return state;
}

/*************************************************************************************************
 *  ����: �����߳�/ISR������Ϣ�����е���Ϣ                                                       *
 *  ����: (1) pMsgQue  ��Ϣ���нṹ��ַ                                                          *
 *        (2) pMsg2    ������Ϣ�ṹ��ַ��ָ�����                                                *
 *        (3) option   ������Ϣ���е�ģʽ                                                        *
 *        (4) timeo    ʱ������ģʽ�·��������ʱ�޳���                                          *
 *        (5) pError   ��ϸ���ý��                                                              *
 *  ����: (1) eError   ����ʧ��                                                                *
 *        (2) eSuccess   �����ɹ�                                                                *
 *  ˵����                                                                                       *
 *************************************************************************************************/
extern TState TclReceiveMessage(TMsgQueue* pMsgQue, TMessage* pMsg2, TBitMask option,
                                TTimeTick timeo, TError* pError)
{
    TState state = eError;
    TError error = OS_IPC_ERR_UNREADY;
    TIpcContext context;
    TBool  HiRP = eFalse;
    TBase32 imask;

    OS_ASSERT((pMsgQue != (TMsgQueue*)0), "");
    OS_ASSERT((pMsg2 != (TMessage*)0), "");
    OS_ASSERT((pError != (TError*)0), "");

    /* ��������ѡ����β���Ҫ֧�ֵ�ѡ�� */
    option &= OS_USER_MSGQ_OPTION;

    OsCpuEnterCritical(&imask);

    if (pMsgQue->Property & OS_IPC_PROP_READY)
    {
        /*
         * ������жϳ�����ñ�������ֻ���Է�������ʽ������Ϣ,
         * ������ʱ�������̵߳������⡣
         * ���ж���,��ǰ�߳�δ������߾������ȼ��߳�,Ҳδ�ش����ں˾����̶߳��У�
         * �����ڴ˴��õ���HiRP������κ����塣
         */
        state = ReceiveMessage(pMsgQue, (void**)pMsg2, &HiRP, &error);
        if ((OsKernelVariable.State == OsThreadState) &&
                (OsKernelVariable.SchedLocks == 0U))
        {
            /* �����ǰ�߳̽���˸������ȼ��̵߳���������е��ȡ�*/
            if (state == eSuccess)
            {
                if (HiRP == eTrue)
                {
                    OsThreadSchedule();
                }
            }
            else
            {
                /*
                * �����ǰ�̲߳��ܽ�����Ϣ�����Ҳ��õ��ǵȴ���ʽ��
                * ��ô��ǰ�̱߳�����������Ϣ������
                */
                if (option & OS_IPC_OPT_WAIT)
                {
                    /* ��ǰ�߳������ڸ���Ϣ���е��������У�ʱ�޻������޵ȴ���
                    * ��OS_IPC_OPT_TIMEO��������
                    */
                    OsIpcBlockThread(&context, (void*)pMsgQue, &(pMsgQue->Queue),
                                     (TBase32)pMsg2, sizeof(TBase32),
                                     option | OS_IPC_OPT_MSGQUEUE | OS_IPC_OPT_READ_DATA, timeo,
                                     &state, &error, &HiRP);
                    if (HiRP == eTrue)
                    {
                        OsThreadSchedule();
                        OsCpuLeaveCritical(imask);
                        /* �˴������̵߳��� */
                        OsCpuEnterCritical(&imask);
                        OsIpcCleanContext(&context);
                    }
                }
            }
        }
    }

    OsCpuLeaveCritical(imask);

    *pError = error;
    return state;
}


/*************************************************************************************************
 *  ����: �����߳�/ISR����Ϣ�����з�����Ϣ                                                       *
 *  ����: (1) pMsgQue  ��Ϣ���нṹ��ַ                                                          *
 *        (2) pMsg2    ������Ϣ�ṹ��ַ��ָ�����                                                *
 *        (3) option   ������Ϣ���е�ģʽ                                                        *
 *        (4) timeo    ʱ������ģʽ�·��������ʱ�޳���                                          *
 *        (5) pError   ��ϸ���ý��                                                              *
 *  ����: (1) eError ����ʧ��                                                                  *
 *        (2) eSuccess �����ɹ�                                                                  *
 *  ˵����                                                                                       *
 *************************************************************************************************/
TState TclSendMessage(TMsgQueue* pMsgQue, TMessage* pMsg2, TBitMask option, TTimeTick timeo,
                      TError* pError)
{
    TState state = eError;
    TError error = OS_IPC_ERR_UNREADY;
    TIpcContext context;
    TBool HiRP = eFalse;
    TBase32 imask;

    OsCpuEnterCritical(&imask);
    if (pMsgQue->Property & OS_IPC_PROP_READY)
    {
        /*
         * ������жϳ�����ñ�������ֻ���Է�������ʽ������Ϣ,
         * ������ʱ�������̵߳������⡣
         * ���ж���,��ǰ�߳�δ������߾������ȼ��߳�,Ҳδ�ش����ں˾����̶߳��У�
         * �����ڴ˴��õ���HiRP������κ����塣
         */
        state = SendMessage(pMsgQue, (void**)pMsg2, option, &HiRP, &error);
        if ((OsKernelVariable.State == OsThreadState) &&
                (OsKernelVariable.SchedLocks == 0U))
        {
            /* �����ǰ�߳̽���˸������ȼ��̵߳���������е��ȡ�*/
            if (state == eSuccess)
            {
                if (HiRP == eTrue)
                {
                    OsThreadSchedule();
                }
            }
            else
            {
                /*
                * �����ǰ�̲߳��ܷ�����Ϣ�����Ҳ��õ��ǵȴ���ʽ��
                * ��ô��ǰ�̱߳�����������Ϣ������
                */
                if (option & OS_IPC_OPT_WAIT)
                {
                    /*
                    * ���ͽ�����Ϣ���߳̽�����Ϣ�����������еĸ���������,
                    * ������ͨ��Ϣ���߳̽�����Ϣ�����������еĻ���������,
                    * ��ǰ�߳������ڸ���Ϣ���е���������ʱ���������ʱ�޻������޵ȴ��ķ�ʽ��
                    * ��OS_IPC_OPT_TIMEO��������
                    */
                    OsIpcBlockThread(&context, (void*)pMsgQue, &(pMsgQue->Queue),
                                     (TBase32)pMsg2, sizeof(TBase32),
                                     option | OS_IPC_OPT_MSGQUEUE | OS_IPC_OPT_WRITE_DATA,
                                     timeo, &state, &error, &HiRP);
                    if (HiRP == eTrue)
                    {
                        OsThreadSchedule();
                        OsCpuLeaveCritical(imask);
                        /* �˴������̵߳��� */
                        OsCpuEnterCritical(&imask);
                        OsIpcCleanContext(&context);
                    }
                }
            }
        }
    }
    OsCpuLeaveCritical(imask);

    *pError = error;
    return state;
}


/*************************************************************************************************
 *  ����: ����ISR����Ϣ�����з�����Ϣ                                                            *
 *  ����: (1) pMsgQue  ��Ϣ���нṹ��ַ                                                          *
 *        (2) pMsg2    ������Ϣ�ṹ��ַ��ָ�����                                                *
 *        (3) option   ���������ģʽ                                                            *
 *        (4) pError   ��ϸ���ý��                                                              *
 *  ����: (1) eError ����ʧ��                                                                  *
 *        (2) eSuccess �����ɹ�                                                                  *
 *  ˵����                                                                                       *
 *************************************************************************************************/
TState TclIsrSendMessage(TMsgQueue* pMsgQue, TMessage* pMsg2, TBitMask option, TError* pError)
{
    TState state = eError;
    TError error = OS_IPC_ERR_UNREADY;
    TBool HiRP = eFalse;
    TBase32 imask;

    OS_ASSERT((pMsgQue != (TMsgQueue*)0), "");
    OS_ASSERT((pMsg2 != (TMessage*)0), "");
    OS_ASSERT((pError != (TError*)0), "");

    /* ��������ѡ����β���Ҫ֧�ֵ�ѡ�� */
    option &= OS_ISR_MSGQ_OPTION;

    OsCpuEnterCritical(&imask);
    if (pMsgQue->Property & OS_IPC_PROP_READY)
    {
        /*
         * ������жϳ�����ñ�������ֻ���Է�������ʽ������Ϣ,
         * ������ʱ�������̵߳������⡣
         * ���ж���,��ǰ�߳�δ������߾������ȼ��߳�,Ҳδ�ش����ں˾����̶߳��У�
         * �����ڴ˴��õ���HiRP������κ����塣
         */
        state = SendMessage(pMsgQue, (void**)pMsg2, option, &HiRP, &error);
    }
    OsCpuLeaveCritical(imask);

    *pError = error;
    return state;
}


/*************************************************************************************************
 *  ���ܣ���Ϣ���г�ʼ������                                                                     *
 *  ���룺(1) pMsgQue   ��Ϣ���нṹ��ַ                                                         *
 *        (2) pName     ��Ϣ��������                                                             *
 *        (3) pPool2    ��Ϣ�����ַ                                                             *
 *        (4) capacity  ��Ϣ��������������Ϣ�����С                                             *
 *        (5) policy    ��Ϣ�����̵߳��Ȳ���                                                     *
 *        (6) pError    ��ϸ���ý��                                                             *
 *  ���أ�(1) eSuccess  �����ɹ�                                                                 *
 *        (2) eError  ����ʧ��                                                                 *
 *  ˵����                                                                                       *
 *************************************************************************************************/
TState TclCreateMsgQueue(TMsgQueue* pMsgQue, TChar* pName, void** pPool2, TBase32 capacity,
                         TBitMask property, TError* pError)
{
    TState state = eError;
    TError error = OS_IPC_ERR_FAULT;
    TBase32 imask;

    OS_ASSERT((pMsgQue != (TMsgQueue*)0), "");
    OS_ASSERT((pName != (TChar*)0), "");
    OS_ASSERT((pPool2 != (void*)0), "");
    OS_ASSERT((capacity != 0U), "");
    OS_ASSERT((pError != (TError*)0), "");

    property &= OS_USER_MQUE_PROP;

    OsCpuEnterCritical(&imask);

    if (!(pMsgQue->Property & OS_IPC_PROP_READY))
    {
        /* ��ʼ����Ϣ���ж�����Ϣ */
        OsKernelAddObject(&(pMsgQue->Object), pName, OsObjTypeMsgQue);

        /* ��ʼ����Ϣ���л�����Ϣ */
        property |= OS_IPC_PROP_READY;
        pMsgQue->Property = property;
        pMsgQue->Capacity = capacity;
        pMsgQue->MsgPool  = pPool2;
        pMsgQue->MsgEntries = 0U;
        pMsgQue->Head = 0U;
        pMsgQue->Tail = 0U;
        pMsgQue->Status = OsMQEmpty;

        pMsgQue->Queue.Handle   = (TLinkNode*)0;
        pMsgQue->Queue.Property = &(pMsgQue->Property);

        error = OS_IPC_ERR_NONE;
        state = eSuccess;
    }

    OsCpuLeaveCritical(imask);

    *pError = error;
    return state;
}


/*************************************************************************************************
 *  ���ܣ���Ϣ�������ú���                                                                       *
 *  ���룺(1) pMsgQue   ��Ϣ���нṹ��ַ                                                         *
 *        (2) pError    ��ϸ���ý��                                                             *
 *  ���أ�(1) eSuccess  �����ɹ�                                                                 *
 *        (2) eError  ����ʧ��                                                                 *
 *  ˵����                                                                                       *
 *************************************************************************************************/
TState TclDeleteMsgQueue(TMsgQueue* pMsgQue, TError* pError)
{
    TState state = eError;
    TError error = OS_IPC_ERR_UNREADY;
    TBool HiRP = eFalse;
    TBase32 imask;

    OS_ASSERT((pMsgQue != (TMsgQueue*)0), "");
    OS_ASSERT((pError != (TError*)0), "");

    OsCpuEnterCritical(&imask);

    if (pMsgQue->Property & OS_IPC_PROP_READY)
    {
        /* �����������е��̷ַ߳���Ϣ */
        OsIpcUnblockAll(&(pMsgQue->Queue), eError, OS_IPC_ERR_DELETE, (void**)0, &HiRP);

        /* ���ں����Ƴ���Ϣ���ж��� */
        OsKernelRemoveObject(&(pMsgQue->Object));

        /* �����Ϣ���ж����ȫ������ */
        memset(pMsgQue, 0U, sizeof(TMsgQueue));

        /*
         * ���̻߳����£������ǰ�̵߳����ȼ��Ѿ��������߳̾������е�������ȼ���
         * �����ں˴�ʱ��û�йر��̵߳��ȣ���ô����Ҫ����һ���߳���ռ
         */
        if ((OsKernelVariable.State == OsThreadState) &&
                (OsKernelVariable.SchedLocks == 0U) &&
                (HiRP == eTrue))
        {
            OsThreadSchedule();
        }
        error = OS_IPC_ERR_NONE;
        state = eSuccess;
    }

    OsCpuLeaveCritical(imask);

    *pError = error;
    return state;
}


/*************************************************************************************************
 *  ����: �����Ϣ������������                                                                   *
 *  ����: (1) pMsgQue   ��Ϣ���нṹ��ַ                                                         *
 *        (2) pError    ��ϸ���ý��                                                             *
 *  ���أ�(1) eSuccess  �����ɹ�                                                                 *
 *        (2) eError  ����ʧ��                                                                 *
 *  ˵����                                                                                       *
 *************************************************************************************************/
TState TclResetMsgQueue(TMsgQueue* pMsgQue, TError* pError)
{
    TState state = eError;
    TError error = OS_IPC_ERR_UNREADY;
    TBool HiRP = eFalse;
    TBase32 imask;

    OS_ASSERT((pMsgQue != (TMsgQueue*)0), "");
    OS_ASSERT((pError != (TError*)0), "");

    OsCpuEnterCritical(&imask);

    if (pMsgQue->Property & OS_IPC_PROP_READY)
    {
        /* �����������ϵ����еȴ��̶߳��ͷţ������̵߳ĵȴ��������OS_IPC_ERR_RESET    */
        OsIpcUnblockAll(&(pMsgQue->Queue), eError, OS_IPC_ERR_RESET, (void**)0, &HiRP);

        /* ����������Ϣ���нṹ */
        pMsgQue->Property &= OS_RESET_MQUE_PROP;
        pMsgQue->MsgEntries = 0U;
        pMsgQue->Head = 0U;
        pMsgQue->Tail = 0U;
        pMsgQue->Status = OsMQEmpty;

        /*
         * ���̻߳����£������ǰ�̵߳����ȼ��Ѿ��������߳̾������е�������ȼ���
         * �����ں˴�ʱ��û�йر��̵߳��ȣ���ô����Ҫ����һ���߳���ռ
         */
        if ((OsKernelVariable.State == OsThreadState) &&
                (OsKernelVariable.SchedLocks == 0U) &&
                (HiRP == eTrue))
        {
            OsThreadSchedule();
        }
        error = OS_IPC_ERR_NONE;
        state = eSuccess;
    }

    OsCpuLeaveCritical(imask);

    *pError = error;
    return state;
}


/*************************************************************************************************
 *  ���ܣ���Ϣ����������ֹ����,��ָ�����̴߳���Ϣ���е�������������ֹ����������                  *
 *  ������(1) pMsgQue  ��Ϣ���нṹ��ַ                                                          *
 *        (2) option   ����ѡ��                                                                  *
 *        (3) pThread  �̵߳�ַ                                                                  *
 *        (4) pError   ��ϸ���ý��                                                              *
 *  ���أ�(1) eSuccess �ɹ�                                                                      *
 *        (2) eError ʧ��                                                                      *
 *  ˵����                                                                                       *
 *************************************************************************************************/
TState TclFlushMsgQueue(TMsgQueue* pMsgQue, TError* pError)
{
    TState state = eError;
    TError error = OS_IPC_ERR_UNREADY;
    TBase32 imask;
    TBool HiRP = eFalse;

    OS_ASSERT((pMsgQue != (TMsgQueue*)0), "");
    OS_ASSERT((pError != (TError*)0), "");

    OsCpuEnterCritical(&imask);

    if (pMsgQue->Property & OS_IPC_PROP_READY)
    {
        /* ����Ϣ�������������ϵ����еȴ��̶߳��ͷţ������̵߳ĵȴ��������OS_IPC_ERR_FLUSH  */
        OsIpcUnblockAll(&(pMsgQue->Queue), eError, OS_IPC_ERR_FLUSH, (void**)0, &HiRP);

        /*
         * ���̻߳����£������ǰ�̵߳����ȼ��Ѿ��������߳̾������е�������ȼ���
         * �����ں˴�ʱ��û�йر��̵߳��ȣ���ô����Ҫ����һ���߳���ռ
         */
        if ((OsKernelVariable.State == OsThreadState) &&
                (OsKernelVariable.SchedLocks == 0U) &&
                (HiRP == eTrue))
        {
            OsThreadSchedule();
        }
        state = eSuccess;
        error = OS_IPC_ERR_NONE;
    }

    OsCpuLeaveCritical(imask);

    *pError = error;
    return state;
}


/*************************************************************************************************
 *  ���ܣ���Ϣ���й㲥����,�����ж����������е��̹߳㲥��Ϣ                                      *
 *  ������(1) pMsgQue    ��Ϣ���нṹ��ַ                                                        *
 *        (2) pMsg2      ������Ϣ�ṹ��ַ��ָ�����                                              *
 *        (3) pError     ��ϸ���ý��                                                            *
 *  ���أ�(1) eSuccess   �ɹ��㲥������Ϣ                                                        *
 *        (2) eError   �㲥������Ϣʧ��                                                        *
 *  ˵����ֻ�ж����ж���Ϣ���е�ʱ�򣬲��ܰ���Ϣ���͸������е��߳�                               *
 *************************************************************************************************/
TState TclBroadcastMessage(TMsgQueue* pMsgQue, TMessage* pMsg2, TError* pError)
{
    TState state = eError;
    TError error = OS_IPC_ERR_UNREADY;
    TBase32 imask;
    TBool HiRP = eFalse;

    OS_ASSERT((pMsgQue != (TMsgQueue*)0), "");
    OS_ASSERT((pMsg2 != (TMessage*)0), "");
    OS_ASSERT((pError != (TError*)0), "");

    OsCpuEnterCritical(&imask);

    if (pMsgQue->Property & OS_IPC_PROP_READY)
    {
        /* �ж���Ϣ�����Ƿ���ã�ֻ����Ϣ���пղ������̵߳ȴ���ȡ��Ϣ��ʱ����ܽ��й㲥 */
        if (pMsgQue->Status == OsMQEmpty)
        {
            /* ����Ϣ���еĶ����������е��̹߳㲥���� */
            OsIpcUnblockAll(&(pMsgQue->Queue), eSuccess, OS_IPC_ERR_NONE, (void**)pMsg2, &HiRP);

            /*
             * ���̻߳����£������ǰ�̵߳����ȼ��Ѿ��������߳̾������е�������ȼ���
             * �����ں˴�ʱ��û�йر��̵߳��ȣ���ô����Ҫ����һ���߳���ռ
             */
            if ((OsKernelVariable.State == OsThreadState) &&
                    (OsKernelVariable.SchedLocks == 0U) &&
                    (HiRP == eTrue))
            {
                OsThreadSchedule();
            }
            error = OS_IPC_ERR_NONE;
            state = eSuccess;
        }
        else
        {
            error = OS_IPC_ERR_NORMAL;
        }
    }

    OsCpuLeaveCritical(imask);

    *pError = error;
    return state;
}

#endif
