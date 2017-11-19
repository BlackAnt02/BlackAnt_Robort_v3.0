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
#include "tcl.mailbox.h"

#if ((TCLC_IPC_ENABLE)&&(TCLC_IPC_MAILBOX_ENABLE))

/*************************************************************************************************
 *  ����: ���Զ�ȡ�����е��ʼ�                                                                   *
 *  ����: (1) pMailbox ����ṹ��ַ                                                              *
 *        (2) pMail2   �����ʼ��ṹ��ַ��ָ�����                                                *
 *        (3) pHiRP    �Ƿ��ں����л��ѹ������߳�                                                *
 *        (4) pError   ��ϸ���ý��                                                              *
 *  ����: (1) eError   ����ʧ��                                                                  *
 *        (2) eSuccess �����ɹ�                                                                  *
 *  ˵������������Զ�ȡ��ʱ��,������̴߳����������������,˵���ǵ�ǰ������߳������������ʼ�   *
 *        ���Ͷ���, ��ʱ��Ҫ������������������ҵ�һ�����ʵ��߳�,ֱ��ʹ�÷����ʼ��ɹ�������Ҫ    *
 *        ���������״̬����                                                                     *
 *************************************************************************************************/
static TState ReceiveMail(TMailbox* pMailbox, void** pMail2, TBool* pHiRP, TError* pError)
{
    TState state = eSuccess;
    TError error = OS_IPC_ERR_NONE;
    TIpcContext* pContext;

    if (pMailbox->Status == OsMailboxFull)
    {
        /* �������ж�ȡ�ʼ� */
        *pMail2 = pMailbox->Mail;

        /*
         * �����ʱ��������߳��������������̴߳���,�����̵߳ȴ������ʼ�,
         * ����н�һ�����ʵ��߳̽���������������̴߳����͵��ʼ����浽������,
         * ͬʱ��������״̬����
         */
        if (pMailbox->Queue.Handle != (TLinkNode*)0)
        {
            pContext = OS_CONTAINER_OF(pMailbox->Queue.Handle, TIpcContext, LinkNode);
            OsIpcUnblockThread(pContext, eSuccess, OS_IPC_ERR_NONE, pHiRP);
            pMailbox->Mail = *((TMail*)(pContext->Data.Addr2));
        }
        /* �����������,�����ʼ�Ϊ��,��������״̬Ϊ�� */
        else
        {
            pMailbox->Mail = (void*)0;
            pMailbox->Status = OsMailboxEmpty;
        }
    }
    else
    {
        error = OS_IPC_ERR_NORMAL;
        state = eError;
    }

    *pError = error;
    return state;
}


/*************************************************************************************************
 *  ����: ���������䷢���ʼ�                                                                     *
 *  ����: (1) pMailbox ����ṹ��ַ                                                              *
 *        (2) pMail2   �����ʼ��ṹ��ַ��ָ�����                                                *
 *        (3) pHiRP    �Ƿ��ں����л��ѹ������߳�                                                *
 *        (4) pError   ��ϸ���ý��                                                              *
 *  ����: (1) eError   ����ʧ��                                                                  *
 *        (2) eSuccess �����ɹ�                                                                  *
 *  ˵����                                                                                       *
 *************************************************************************************************/
static TState SendMail(TMailbox* pMailbox, void** pMail2, TBool* pHiRP, TError* pError)
{
    TState state = eSuccess;
    TError error = OS_IPC_ERR_NONE;
    TIpcContext* pContext;

    if (pMailbox->Status == OsMailboxEmpty)
    {
        /*
         * ������Ϊ�յ�ʱ��,������̴߳����������������,˵���ǵ�ǰ����ĵ��߳�����������
         * �ʼ���ȡ����, ��ʱ��Ҫ������������������ҵ�һ�����ʵ��߳�,��ֱ��ʹ������ȡ�ʼ��ɹ���
         * ͬʱ�����״̬����
         */
        if (pMailbox->Queue.Handle != (TLinkNode*)0)
        {
            pContext = OS_CONTAINER_OF(pMailbox->Queue.Handle, TIpcContext, LinkNode);
            OsIpcUnblockThread(pContext, eSuccess, OS_IPC_ERR_NONE, pHiRP);
            *(pContext->Data.Addr2) = *pMail2;
        }
        else
        {
            /* �����ʼ�д������,��������״̬Ϊ�� */
            pMailbox->Mail = *pMail2;
            pMailbox->Status = OsMailboxFull;
        }
    }
    else
    {
        /* �������Ѿ����ʼ��ˣ������ٷ��������ʼ� */
        error = OS_IPC_ERR_NORMAL;
        state = eError;
    }

    *pError = error;
    return state;
}


/*************************************************************************************************
 *  ����: �߳�/ISR�������ж�ȡ�ʼ�                                                               *
 *  ����: (1) pMailbox ����ṹ��ַ                                                              *
 *        (2) pMail2   �����ʼ��ṹ��ַ��ָ�����                                                *
 *        (3) option   ���������ģʽ                                                            *
 *        (4) timeo    ʱ������ģʽ�·��������ʱ�޳���                                          *
 *        (5) pError   ��ϸ���ý��                                                              *
 *  ����: (1) eError   ����ʧ��                                                                  *
 *        (2) eSuccess �����ɹ�                                                                  *
 *  ˵����                                                                                       *
 *************************************************************************************************/
TState TclReceiveMail(TMailbox* pMailbox, TMail* pMail2, TBitMask option, TTimeTick timeo,
                      TError* pError)
{
    TState state = eError;
    TError error = OS_IPC_ERR_UNREADY;
    TBool HiRP = eFalse;
    TIpcContext context;

    TBase32 imask;

    OS_ASSERT((pMailbox != (TMailbox*)0), "");
    OS_ASSERT((pMail2 != (TMail*)0), "");
    OS_ASSERT((pError != (TError*)0), "");

    OsCpuEnterCritical(&imask);
    if (pMailbox->Property & OS_IPC_PROP_READY)
    {
        /*
         * ������жϳ�����ñ�������ֻ���Է�������ʽ�������ж�ȡ�ʼ�,
         * ������ʱ�������̵߳������⡣
         * ���ж���,��ǰ�߳�δ������߾������ȼ��߳�,Ҳδ�ش����ں˾����̶߳��У�
         * �����ڴ˴��õ���HiRP������κ����塣
         */
        state = ReceiveMail(pMailbox, (void**)pMail2, &HiRP, &error);
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
                 * �����ǰ�̲߳��ܵõ��ʼ�,���Ҳ��õ��ǵȴ���ʽ,�����ں�û�йر��̵߳���,
                 * ��ô��ǰ�̱߳������������������,����ǿ���̵߳���
                 */
                /* ��������ѡ����β���Ҫ֧�ֵ�ѡ�� */
                option &= OS_USER_MBOX_OPTION;
                if (option & OS_IPC_OPT_WAIT)
                {
                    /* ��ǰ�߳������ڸ�������������У�ʱ�޻������޵ȴ�����OS_IPC_OPT_TIMEO�������� */
                    OsIpcBlockThread(&context, (void*)pMailbox, &(pMailbox->Queue),
                                     (TBase32)pMail2, sizeof(TBase32),
                                     option | OS_IPC_OPT_MAILBOX | OS_IPC_OPT_READ_DATA, timeo,
                                     &state, &error, &HiRP);

                    /* ��ǰ�̱߳������������̵߳���ִ�� */
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
 *  ����: �߳�/ISR�������з����ʼ�                                                               *
 *  ����: (1) pMailbox ����ṹ��ַ                                                              *
 *        (2) pMail2   �����ʼ��ṹ��ַ��ָ�����                                                *
 *        (3) option   ���������ģʽ                                                            *
 *        (4) timeo    ʱ������ģʽ�·��������ʱ�޳���                                          *
 *        (5) pError   ��ϸ���ý��                                                              *
 *  ����: (1) eError   ����ʧ��                                                                  *
 *        (2) eSuccess �����ɹ�                                                                  *
 *  ˵����                                                                                       *
 *************************************************************************************************/
TState TclSendMail(TMailbox* pMailbox, TMail* pMail2, TBitMask option, TTimeTick timeo,
                   TError* pError)
{
    TState state = eError;
    TError error = OS_IPC_ERR_UNREADY;
    TBool  HiRP = eFalse;
    TIpcContext context;
    TBase32 imask;

    OS_ASSERT((pMailbox != (TMailbox*)0), "");
    OS_ASSERT((pMail2 != (TMail*)0), "");
    OS_ASSERT((pError != (TError*)0), "");

    OsCpuEnterCritical(&imask);
    if (pMailbox->Property & OS_IPC_PROP_READY)
    {
        /*
         * ������жϳ�����ñ�������ֻ���Է�������ʽ�������з����ʼ�,
         * ������ʱ�������̵߳������⡣
         * ���ж���,��ǰ�߳�δ������߾������ȼ��߳�,Ҳδ�ش����ں˾����̶߳��У�
         * �����ڴ˴��õ���HiRP������κ����塣
         */
        state = SendMail(pMailbox, (void**)pMail2, &HiRP, &error);
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
                * �����ǰ�̲߳��ܷ����ʼ�,���Ҳ��õ��ǵȴ���ʽ,�����ں�û�йر��̵߳���,
                * ��ô��ǰ�̱߳������������������,����ǿ���̵߳���
                */
                /* ��������ѡ����β���Ҫ֧�ֵ�ѡ�� */
                option &= OS_USER_MBOX_OPTION;
                if (option & OS_IPC_OPT_WAIT)
                {
                    /*
                    * ��ǰ�̱߳������������̵߳���ִ��
                    * ������ͨ�ʼ����߳̽��������������еĻ���������,
                    * ��ǰ�߳������ڸ��������������ʱ���������ʱ�޻������޵ȴ��ķ�ʽ��
                    * ��OS_IPC_OPT_TIMEO��������
                    */
                    OsIpcBlockThread(&context, (void*)pMailbox, &(pMailbox->Queue),
                                     (TBase32)pMail2, sizeof(TBase32),
                                     option | OS_IPC_OPT_MAILBOX | OS_IPC_OPT_WRITE_DATA, timeo,
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
 *  ���ܣ�ISR�����䷢���ʼ�                                                                      *
 *  ����: (1) pMailbox ����ṹ��ַ                                                              *
 *        (2) pMail2   �����ʼ��ṹ��ַ��ָ�����                                                *
 *        (3) pError   ��ϸ���ý��                                                              *
 *  ����: (1) eError ����ʧ��                                                                  *
 *        (2) eSuccess �����ɹ�                                                                  *
 *  ˵����                                                                                       *
 *************************************************************************************************/
TState TclIsrSendMail(TMailbox* pMailbox, TMail* pMail2, TError* pError)
{
    TState state = eError;
    TError error = OS_IPC_ERR_UNREADY;
    TBool HiRP = eFalse;
    TBase32 imask;

    OS_ASSERT((pMailbox != (TMailbox*)0), "");
    OS_ASSERT((pMail2 != (TMail*)0), "");
    OS_ASSERT((pError != (TError*)0), "");

    OsCpuEnterCritical(&imask);
    if (pMailbox->Property & OS_IPC_PROP_READY)
    {
        /*
         * ������жϳ�����ñ�������ֻ���Է�������ʽ�������з����ʼ�,
         * ������ʱ�������̵߳������⡣
         * ���ж���,��ǰ�߳�δ������߾������ȼ��߳�,Ҳδ�ش����ں˾����̶߳��У�
         * �����ڴ˴��õ���HiRP������κ����塣
         */
        state = SendMail(pMailbox, (void**)pMail2, &HiRP, &error);
    }
    OsCpuLeaveCritical(imask);

    *pError = error;
    return state;
}


/*************************************************************************************************
 *  ���ܣ���ʼ������                                                                             *
 *  ������(1) pMailbox   ����ĵ�ַ                                                              *
 *        (2) pName      ���������                                                              *
 *        (3) property   ����ĳ�ʼ����                                                          *
 *        (4) pError     ��ϸ���ý��                                                            *
 *  ����: (1) eError     ����ʧ��                                                                *
 *        (2) eSuccess   �����ɹ�                                                                *
 *  ˵����                                                                                       *
 *************************************************************************************************/
TState TclCreateMailbox(TMailbox* pMailbox, TChar* pName, TBitMask property, TError* pError)
{
    TState state = eError;
    TError error = OS_IPC_ERR_FAULT;
    TBase32 imask;

    OS_ASSERT((pMailbox != (TMailbox*)0), "");
    OS_ASSERT((pName != (TChar*)0), "");
    OS_ASSERT((pError != (TError*)0), "");

    property &= OS_USER_MBOX_PROP;

    OsCpuEnterCritical(&imask);

    if (!(pMailbox->Property & OS_IPC_PROP_READY))
    {
        /* ��ʼ�����������Ϣ */
        OsKernelAddObject(&(pMailbox->Object), pName, OsObjTypeMailbox);

        /* ��ʼ�����������Ϣ */
        property |= OS_IPC_PROP_READY;
        pMailbox->Property = property;
        pMailbox->Status = OsMailboxEmpty;
        pMailbox->Mail = (void*)0;

        pMailbox->Queue.Handle   = (TLinkNode*)0;
        pMailbox->Queue.Property = &(pMailbox->Property);

        error = OS_IPC_ERR_NONE;
        state = eSuccess;
    }

    OsCpuLeaveCritical(imask);

    *pError = error;
    return state;
}


/*************************************************************************************************
 *  ���ܣ���������                                                                               *
 *  ������(1) pMailbox   ����ĵ�ַ                                                              *
 *        (2) pError     ��ϸ���ý��                                                            *
 *  ����: (1) eError     ����ʧ��                                                                *
 *        (2) eSuccess   �����ɹ�                                                                *
 *  ˵����ע���̵߳ĵȴ��������OS_IPC_ERR_DELETE                                                *
 *************************************************************************************************/
TState TclDeleteMailbox(TMailbox* pMailbox, TError* pError)
{
    TState state = eError;
    TError error = OS_IPC_ERR_UNREADY;
    TBool HiRP = eFalse;

    TBase32 imask;

    OS_ASSERT((pMailbox != (TMailbox*)0), "");
    OS_ASSERT((pError != (TError*)0), "");

    OsCpuEnterCritical(&imask);

    if (pMailbox->Property & OS_IPC_PROP_READY)
    {
        /* ���������������ϵ����еȴ��̶߳��ͷ�,�����̵߳ĵȴ��������OS_IPC_ERR_DELETE */
        OsIpcUnblockAll(&(pMailbox->Queue), eError, OS_IPC_ERR_DELETE, (void**)0, &HiRP);

        /* ���ں����Ƴ�������� */
        OsKernelRemoveObject(&(pMailbox->Object));

        /* �����������ȫ������ */
        memset(pMailbox, 0U, sizeof(TMailbox));

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
 *  ����: ���������������                                                                       *
 *  ����: (1) pMailbox  ����ṹ��ַ                                                             *
 *        (2) pError    ��ϸ���ý��                                                             *
 *  ����: (1) eError    ����ʧ��                                                                 *
 *        (2) eSuccess  �����ɹ�                                                                 *
 *  ˵����                                                                                       *
 *************************************************************************************************/
TState TclResetMailbox(TMailbox* pMailbox, TError* pError)
{
    TState state = eError;
    TError error = OS_IPC_ERR_UNREADY;
    TBool HiRP = eFalse;
    TBase32 imask;


    OS_ASSERT((pMailbox != (TMailbox*)0), "");
    OS_ASSERT((pError != (TError*)0), "");

    OsCpuEnterCritical(&imask);
    if (pMailbox->Property & OS_IPC_PROP_READY)
    {
        /* �����������ϵ����еȴ��̶߳��ͷ�,�����̵߳ĵȴ��������OS_IPC_ERR_RESET */
        OsIpcUnblockAll(&(pMailbox->Queue), eError, OS_IPC_ERR_RESET, (void**)0, &HiRP);

        /* ���������״̬Ϊ��,��������е��ʼ� */
        pMailbox->Property &= OS_RESET_MBOX_PROP;
        pMailbox->Status = OsMailboxEmpty;
        pMailbox->Mail = (TMail*)0;

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
 *  ���ܣ�����������ֹ����,��ָ�����̴߳�����Ķ�������������ֹ����������                        *
 *  ����: (1) pMailbox ����ṹ��ַ                                                              *
 *        (2) pThread  �߳̽ṹ��ַ                                                              *
 *        (3) option   ����ѡ��                                                                  *
 *        (4) pError   ��ϸ���ý��                                                              *
 *  ����: (1) eError   ����ʧ��                                                                  *
 *        (2) eSuccess �����ɹ�                                                                  *
 *  ˵��?                                                                                        *
 *************************************************************************************************/
TState TclFlushMailbox(TMailbox* pMailbox, TError* pError)
{
    TState state = eError;
    TError error = OS_IPC_ERR_UNREADY;
    TBool HiRP = eFalse;
    TBase32 imask;


    OS_ASSERT((pMailbox != (TMailbox*)0), "");
    OS_ASSERT((pError != (TError*)0), "");

    OsCpuEnterCritical(&imask);

    if (pMailbox->Property & OS_IPC_PROP_READY)
    {
        /* ���������������ϵ����еȴ��̶߳��ͷţ������̵߳ĵȴ��������OS_IPC_ERR_FLUSH  */
        OsIpcUnblockAll(&(pMailbox->Queue), eError, OS_IPC_ERR_FLUSH, (void**)0, &HiRP);

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
 *  ���ܣ�����㲥����,�����ж����������е��̹߳㲥�ʼ�                                          *
 *  ����: (1) pMailbox  ����ṹ��ַ                                                             *
 *        (2) pMail2    �����ʼ��ṹ��ַ��ָ�����                                               *
 *        (3) pError    ��ϸ���ý��                                                             *
 *  ����: (1) eError    ����ʧ��                                                                 *
 *        (2) eSuccess  �����ɹ�                                                                 *
 *  ˵����                                                                                       *
 *************************************************************************************************/
TState TclBroadcastMail(TMailbox* pMailbox, TMail* pMail2, TError* pError)
{
    TState state = eError;
    TError error = OS_IPC_ERR_UNREADY;
    TBool  HiRP = eFalse;
    TBase32 imask;

    OS_ASSERT((pMailbox != (TMailbox*)0), "");
    OS_ASSERT((pMail2 != (TMail*)0), "");
    OS_ASSERT((pError != (TError*)0), "");

    OsCpuEnterCritical(&imask);

    if (pMailbox->Property & OS_IPC_PROP_READY)
    {
        /* ֻ������ղ������̵߳ȴ���ȡ�ʼ���ʱ����ܽ��й㲥 */
        if (pMailbox->Status == OsMailboxEmpty)
        {
            OsIpcUnblockAll(&(pMailbox->Queue), eSuccess, OS_IPC_ERR_NONE, (void**)pMail2, &HiRP);

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

