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
#include "tcl.semaphore.h"


#if ((TCLC_IPC_ENABLE)&&(TCLC_IPC_SEMAPHORE_ENABLE))

/*************************************************************************************************
 *  ����: �ͷż����ź���                                                                         *
 *  ����: (1) pSemaphore �����ź����ṹ��ַ                                                      *
 *        (2) pHiRP      �Ƿ���ڸ߾������ȼ����                                                *
 *        (3) pError     ��ϸ���ý��                                                            *
 *  ����: (1) eSuccess   �����ɹ�                                                                *
 *        (2) eError   ����ʧ��                                                                *
 *  ˵�������ź��������ͷŵ�ʱ��������ź��������������д����̣߳���ô˵���ź���������������   *
 *        Obtain����,��Ҫ���ź����������������ҵ�һ�����ʵ��̣߳�����ֱ��ʹ�����ɹ�����ź���,   *
 *        ͬʱ�����ź����ļ�������                                                               *
 *************************************************************************************************/
static TState ReleaseSemaphore(TSemaphore* pSemaphore, TBool* pHiRP, TError* pError)
{
    TState state = eSuccess;
    TError error = OS_IPC_ERR_NONE;
    TIpcContext* pContext;

    if (pSemaphore->Value == pSemaphore->LimitedValue)
    {
        state = eError;
        error = OS_IPC_ERR_NORMAL;
    }
    else if (pSemaphore->Value == 0U)
    {
        /*
         * ���Դ��ź����������������ҵ�һ�����ʵ��̲߳�����,�����ź����������䣬
         * ��������ѵ��̵߳����ȼ����ڵ�ǰ�߳����ȼ��������̵߳���������
         */
        if (pSemaphore->Queue.Handle != (TLinkNode*)0)
        {
            pContext = OS_CONTAINER_OF(pSemaphore->Queue.Handle, TIpcContext, LinkNode);
            OsIpcUnblockThread(pContext, eSuccess, OS_IPC_ERR_NONE, pHiRP);
        }
        else
        {
            /* ���û���ҵ����ʵ��̣߳����ź���������1 */
            pSemaphore->Value++;
        }
    }
    else
    {
        /* �ź�������ֱ�Ӽ�1 */
        pSemaphore->Value++;
    }

    *pError = error;
    return state;
}


/*************************************************************************************************
 *  ����: ���Ի�ü����ź���                                                                     *
 *  ����: (1) pSemaphore �����ź����ṹ��ַ                                                      *
 *        (2) pHiRP     �Ƿ����Ѹ������ȼ���������Ҫ�����̵߳��ȵı��                         *
 *        (3) pError     ��ϸ���ý��                                                            *
 *  ����: (1) eSuccess   �����ɹ�                                                                *
 *        (2) eError   ����ʧ��                                                                *
 *  ˵�������ź������������ʱ��������ź��������������д����̣߳���ô˵���ź���������������   *
 *        Release����,��Ҫ���ź����������������ҵ�һ�����ʵ��̣߳�����ֱ��ʹ�����ͷ��ź����ɹ�,  *
 *        ͬʱ�����ź����ļ�������                                                               *
 *************************************************************************************************/
static TState ObtainSemaphore(TSemaphore* pSemaphore, TBool* pHiRP, TError* pError)
{
    TState state = eSuccess;
    TError error = OS_IPC_ERR_NONE;
    TIpcContext* pContext;

    if (pSemaphore->Value == 0U)
    {
        state = eError;
        error = OS_IPC_ERR_NORMAL;
    }
    else if (pSemaphore->Value == pSemaphore->LimitedValue)
    {
        /*
         * ���Դ��ź����������������ҵ�һ�����ʵ��̲߳�����,�����ź����������䣬
         * ��������ѵ��̵߳����ȼ����ڵ�ǰ�߳����ȼ��������̵߳���������
         */
        if (pSemaphore->Queue.Handle != (TLinkNode*)0)
        {
            pContext = OS_CONTAINER_OF(pSemaphore->Queue.Handle, TIpcContext, LinkNode);
            OsIpcUnblockThread(pContext, eSuccess, OS_IPC_ERR_NONE, pHiRP);
        }
        else
        {
            /* ���û���ҵ����ʵ��̣߳����ź���������1 */
            pSemaphore->Value--;
        }
    }
    else
    {
        /* �ź�������ֱ�Ӽ�1 */
        pSemaphore->Value--;
    }

    *pError = error;
    return state;
}


/*************************************************************************************************
 *  ����: �߳�/ISR ����ź�����û����ź���                                                      *
 *  ����: (1) pSemaphore �ź����ṹ��ַ                                                          *
 *        (2) option     �����ź����ĵ�ģʽ                                                      *
 *        (3) timeo      ʱ������ģʽ�·����ź�����ʱ�޳���                                      *
 *        (5) pError     ��ϸ���ý��                                                            *
 *  ����: (1) eSuccess   �����ɹ�                                                                *
 *        (2) eError   ����ʧ��                                                                *
 *  ˵����                                                                                       *
 *************************************************************************************************/
TState TclObtainSemaphore(TSemaphore* pSemaphore, TBitMask option, TTimeTick timeo, TError* pError)
{
    TState state = eError;
    TError error = OS_IPC_ERR_UNREADY;
    TIpcContext context;
    TBool HiRP = eFalse;
    TBase32 imask;

    OsCpuEnterCritical(&imask);

    if (pSemaphore->Property & OS_IPC_PROP_READY)
    {
        state = ObtainSemaphore(pSemaphore, &HiRP, &error);

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
                 * �����ǰ�̲߳��ܵõ��ź��������Ҳ��õ��ǵȴ���ʽ��
                 * ��ô��ǰ�̱߳����������ź���������
                 */
                if (option & OS_IPC_OPT_WAIT)
                {
                    /* ��ǰ�߳������ڸ��ź������������У�ʱ�޻������޵ȴ�����OS_IPC_OPT_TIMEO�������� */
                    OsIpcBlockThread(&context, (void*)pSemaphore, &(pSemaphore->Queue),
                                     0U, 0U,
                                     option | OS_IPC_OPT_SEMAPHORE, timeo,
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

    * pError = error;
    return state;
}



/*************************************************************************************************
 *  ����: �߳�/ISR�����ͷ��ź���                                                                 *
 *  ����: (1) pSemaphore �ź����ṹ��ַ                                                          *
 *        (2) option     �߳��ͷ��ź����ķ�ʽ                                                    *
 *        (3) timeo      �߳��ͷ��ź�����ʱ��                                                    *
 *        (4) pError     ��ϸ���ý��                                                            *
 *  ����: (1) eSuccess   �����ɹ�                                                                *
 *        (2) eError   ����ʧ��                                                                *
 *  ˵����                                                                                       *
 *************************************************************************************************/
TState TclReleaseSemaphore(TSemaphore* pSemaphore, TBitMask option, TTimeTick timeo, TError* pError)
{
    TState state = eError;
    TError error = OS_IPC_ERR_UNREADY;
    TIpcContext context;
    TBool HiRP = eFalse;
    TBase32 imask;

    OS_ASSERT((pSemaphore != (TSemaphore*)0), "");
    OS_ASSERT((pError != (TError*)0), "");

    OsCpuEnterCritical(&imask);
    if (pSemaphore->Property & OS_IPC_PROP_READY)
    {
        /*
         * ������жϳ�����ñ�������ֻ���Է�������ʽ�ͷ��ź���,
         * ������ʱ�������̵߳������⡣
         * ���ж���,��ǰ�߳�δ������߾������ȼ��߳�,Ҳδ�ش����ں˾����̶߳��У�
         * �����ڴ˴��õ���HiRP������κ����塣
         */
        state = ReleaseSemaphore(pSemaphore, &HiRP, &error);
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
                 * �����ǰ�̲߳����ͷ��ź��������Ҳ��õ��ǵȴ���ʽ��
                 * ��ô��ǰ�̱߳����������ź���������
                 */
                /* ��������ѡ����β���Ҫ֧�ֵ�ѡ�� */
                option &= OS_USER_SEMAPHORE_OPTION;
                if (option & OS_IPC_OPT_WAIT)
                {
                    /* ��ǰ�߳������ڸ��ź������������У�ʱ�޻������޵ȴ�����OS_IPC_OPT_TIMEO�������� */
                    OsIpcBlockThread(&context, (void*)pSemaphore, &(pSemaphore->Queue),
                                     0U, 0U,
                                     option | OS_IPC_OPT_SEMAPHORE, timeo,
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

    * pError = error;
    return state;
}


/*************************************************************************************************
 *  ����: ISR�ͷ��ź���                                                                          *
 *  ����: (1) pSemaphore �ź����ṹ��ַ                                                          *
 *        (2) pError     ��ϸ���ý��                                                            *
 *  ����: (1) eSuccess   �����ɹ�                                                                *
 *        (2) eError   ����ʧ��                                                                *
 *  ˵����                                                                                       *
 *************************************************************************************************/
TState TclIsrReleaseSemaphore(TSemaphore* pSemaphore, TError* pError)
{
    TState state = eError;
    TError error = OS_IPC_ERR_UNREADY;
    TBool HiRP = eFalse;
    TBase32 imask;

    OS_ASSERT((pSemaphore != (TSemaphore*)0), "");
    OS_ASSERT((pError != (TError*)0), "");

    OsCpuEnterCritical(&imask);
    if (pSemaphore->Property & OS_IPC_PROP_READY)
    {
        /*
         * ������жϳ�����ñ�������ֻ���Է�������ʽ�ͷ��ź���,
         * ������ʱ�������̵߳������⡣
         * ���ж���,��ǰ�߳�δ������߾������ȼ��߳�,Ҳδ�ش����ں˾����̶߳��У�
         * �����ڴ˴��õ���HiRP������κ����塣
         */
        state = ReleaseSemaphore(pSemaphore, &HiRP, &error);
    }
    OsCpuLeaveCritical(imask);

    *pError = error;
    return state;
}


/*************************************************************************************************
 *  ����: ��ʼ�������ź���                                                                       *
 *  ����: (1) pSemaphore �����ź����ṹ��ַ                                                      *
 *        (2) pName      �����ź�������                                                          *
 *        (3) value      �����ź�����ʼֵ                                                        *
 *        (4) mvalue     �����ź���������ֵ                                                    *
 *        (5) property   �ź����ĳ�ʼ����                                                        *
 *        (6) pError     ��ϸ���ý��                                                            *
 *  ����: (1) eSuccess   �����ɹ�                                                                *
 *        (2) eError   ����ʧ��                                                                *
 *  ˵�����ź���ֻʹ�û���IPC����                                                                *
 *************************************************************************************************/
TState TclCreateSemaphore(TSemaphore* pSemaphore, TChar* pName, TBase32 value, TBase32 mvalue,
                          TBitMask property, TError* pError)
{
    TState state = eError;
    TError error = OS_IPC_ERR_FAULT;
    TBase32 imask;

    OS_ASSERT((pSemaphore != (TSemaphore*)0), "");
    OS_ASSERT((pName != (TChar*)0), "");
    OS_ASSERT((mvalue >= 1U), "");
    OS_ASSERT((mvalue >= value), "");
    OS_ASSERT((pError != (TError*)0), "");

    property &= OS_USER_SEMAPHORE_PROP;

    OsCpuEnterCritical(&imask);

    if (!(pSemaphore->Property & OS_IPC_PROP_READY))
    {
        /* ��ʼ���ź���������Ϣ */
        OsKernelAddObject(&(pSemaphore->Object), pName, OsObjTypeSemaphore);

        /* ��ʼ���ź���������Ϣ */
        property |= OS_IPC_PROP_READY;
        pSemaphore->Property     = property;
        pSemaphore->Value        = value;
        pSemaphore->LimitedValue = mvalue;
        pSemaphore->InitialValue = value;
        pSemaphore->Queue.Handle   = (TLinkNode*)0;
        pSemaphore->Queue.Property = &(pSemaphore->Property);

        error = OS_IPC_ERR_NONE;
        state = eSuccess;
    }

    OsCpuLeaveCritical(imask);

    *pError = error;
    return state;
}


/*************************************************************************************************
 *  ����: �ź���ɾ��                                                                             *
 *  ����: (1) pSemaphore �ź����ṹ��ַ                                                          *
 *        (2) pError     ��ϸ���ý��                                                            *
 *  ����: (1) eSuccess   �����ɹ�                                                                *
 *        (2) eError   ����ʧ��                                                                *
 *  ˵����                                                                                       *
 *************************************************************************************************/
TState TclDeleteSemaphore(TSemaphore* pSemaphore, TError* pError)
{
    TState state = eError;
    TError error = OS_IPC_ERR_UNREADY;
    TBase32 imask;
    TBool HiRP = eFalse;

    OS_ASSERT((pSemaphore != (TSemaphore*)0), "");
    OS_ASSERT((pError != (TError*)0), "");

    OsCpuEnterCritical(&imask);

    if (pSemaphore->Property & OS_IPC_PROP_READY)
    {
        /*
         * ��������ĺ���ʱ���¼�Ƿ������ȼ����ߵ��߳̾�����������¼�������ĸ��߳�
         * ���ź������������ϵ����еȴ��̶߳��ͷţ������̵߳ĵȴ��������TCLE_IPC_DELETE
         */
        OsIpcUnblockAll(&(pSemaphore->Queue), eError, OS_IPC_ERR_DELETE, (void**)0, &HiRP);

        /* ���ں����Ƴ��ź��� */
        OsKernelRemoveObject(&(pSemaphore->Object));

        /* ����ź��������ȫ������ */
        memset(pSemaphore, 0U, sizeof(TSemaphore));

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
 *  ����: ���ü����ź���                                                                         *
 *  ����: (1) pSemaphore �ź����ṹ��ַ                                                          *
 *        (2) pError     ��ϸ���ý��                                                            *
 *  ����: (1) eSuccess   �����ɹ�                                                                *
 *        (2) eError   ����ʧ��                                                                *
 *  ˵����                                                                                       *
 *************************************************************************************************/
TState TclResetSemaphore(TSemaphore* pSemaphore, TError* pError)
{
    TState state = eError;
    TError error = OS_IPC_ERR_UNREADY;
    TBase32 imask;
    TBool HiRP = eFalse;

    OS_ASSERT((pSemaphore != (TSemaphore*)0), "");
    OS_ASSERT((pError != (TError*)0), "");

    OsCpuEnterCritical(&imask);

    if (pSemaphore->Property & OS_IPC_PROP_READY)
    {
        /*
         * ��������ĺ���ʱ���¼�Ƿ������ȼ����ߵ��߳̾�����������¼�������ĸ��߳�
         * ���ź������������ϵ����еȴ��̶߳��ͷţ������̵߳ĵȴ��������TCLE_IPC_RESET
         */
        OsIpcUnblockAll(&(pSemaphore->Queue), eError, OS_IPC_ERR_RESET, (void**)0, &HiRP);

        /* �����ź������������� */
        pSemaphore->Property &= OS_RESET_SEMAPHORE_PROP;
        pSemaphore->Value = pSemaphore->InitialValue;

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
 *  ���ܣ��ź���ˢ�º���,���������ź����ϵ��̴߳��ź������߳����������н������                  *
 *  ������(1) pSemaphore �ź����ṹ��ַ                                                          *
 *        (2) pError     ��ϸ���ý��                                                            *
 *  ����: (1) eSuccess   �����ɹ�                                                                *
 *        (2) eError   ����ʧ��                                                                *
 *  ˵����                                                                                       *
 *************************************************************************************************/
TState TclFlushSemaphore(TSemaphore* pSemaphore, TError* pError)
{
    TState state = eError;
    TError error = OS_IPC_ERR_UNREADY;
    TBase32 imask;
    TBool HiRP = eFalse;

    OS_ASSERT((pSemaphore != (TSemaphore*)0), "");
    OS_ASSERT((pError != (TError*)0), "");

    OsCpuEnterCritical(&imask);

    if (pSemaphore->Property & OS_IPC_PROP_READY)
    {
        /*
         * ��������ĺ���ʱ���¼�Ƿ������ȼ����ߵ��߳̾�����������¼�������ĸ��߳�
         * ���ź������������ϵ����еȴ��̶߳��ͷţ������̵߳ĵȴ��������TCLE_IPC_FLUSH
         */
        OsIpcUnblockAll(&(pSemaphore->Queue), eError, OS_IPC_ERR_FLUSH, (void**)0, &HiRP);

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

#endif

