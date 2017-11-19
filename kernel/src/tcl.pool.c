/*************************************************************************************************
 *                                     Trochili RTOS Kernel                                      *
 *                                  Copyright(C) 2017 LIUXUMING                                  *
 *                                       www.trochili.com                                        *
 *************************************************************************************************/
#include "string.h"

#include "tcl.types.h"
#include "tcl.config.h"
#include "tcl.cpu.h"
#include "tcl.kernel.h"
#include "tcl.pool.h"

#if (TCLC_POOL_ENABLE)
/*************************************************************************************************
 *  ����: ��ʼ�������                                                                           *
 *  ����: (1) pPool      ����ؽṹ��ַ                                                          *
 *        (2) pAddr      �������������ַ                                                        *
 *        (3) pages      ������ж�����Ŀ                                                        *
 *        (4) pgsize     �����С                                                                *
 *        (5) pError     ��ϸ���ý��                                                            *
 *  ����: (1) eSuccess   �����ɹ�                                                                *
 *        (2) eError     ����ʧ��                                                                *
 *  ˵����                                                                                       *
 *************************************************************************************************/
TState TclCreatePool(TObjPool* pPool, void* pAddr, TBase32 number, TBase32 size, TError* pError)
{
    TState state = eError;
    TError error = OS_POOL_ERR_FAULT;
    TBase32 imask;
    TBase32 index;
    TChar* pTemp;

    OS_ASSERT((pPool != (TObjPool*)0), "");
    OS_ASSERT((pAddr != (void*)0), "");
    OS_ASSERT((number != 0U), "");
    OS_ASSERT((size > 0U), "");
    OS_ASSERT((pError != (TError*)0), "");

    OsCpuEnterCritical(&imask);

    if (!(pPool->Property & OS_POOL_PROP_READY))
    {
        /* ��ձ�����Ķ���ռ� */
        memset(pAddr, 0U, number * size);

        /* �������ö������� */
        pTemp = (TChar*)pAddr;
        for (index = 0; index < number; index++)
        {
            OsObjListAddNode(&(pPool->ObjList), (TLinkNode*)pTemp, OsLinkHead);
            pTemp += size;
        }

        pPool->ObjAddr  = pAddr;
        pPool->ObjAvail = number;
        pPool->ObjNbr   = number;
        pPool->ObjSize  = size;
        pPool->Property = OS_POOL_PROP_READY;

        error = OS_POOL_ERR_NONE;
        state = eSuccess;
    }

    OsCpuLeaveCritical(imask);

    *pError = error;
    return state;
}


/*************************************************************************************************
 *  ����: ���ٶ����                                                                             *
 *  ����: (1) pPool      ����ؽṹ��ַ                                                          *
 *        (2) pError     ��ϸ���ý��                                                            *
 *  ����: (1) eSuccess   �����ɹ�                                                                *
 *        (2) eError     ����ʧ��                                                                *
 *  ˵����                                                                                       *
 *************************************************************************************************/
TState TclDeletePool(TObjPool* pPool, TError* pError)
{
    TBase32 imask;
    TState state = eError;
    TError error = OS_POOL_ERR_UNREADY;

    OS_ASSERT((pPool != (TObjPool*)0), "");
    OS_ASSERT((pError != (TError*)0), "");

    OsCpuEnterCritical(&imask);
    if (pPool->Property & OS_POOL_PROP_READY)
    {
        if (pPool->ObjAvail == pPool->ObjNbr)
        {
            memset(pPool->ObjAddr, 0U, pPool->ObjNbr * pPool->ObjSize);
            memset(pPool, 0U, sizeof(TObjPool));
            error = OS_POOL_ERR_NONE;
            state = eSuccess;
        }
        else
        {
            error = OS_POOL_ERR_FAULT;
            state = eError;
        }
    }
    OsCpuLeaveCritical(imask);

    *pError = error;
    return state;
}


/*************************************************************************************************
 *  ����: �Ӷ����������������                                                                 *
 *  ����: (1) pPool      ����ؽṹ��ַ                                                          *
 *        (2) pAddr2     �������뵽�Ķ����ָ�����                                              *
 *        (3) pError     ��ϸ���ý��                                                            *
 *  ����: (1) eSuccess   �����ɹ�                                                                *
 *        (2) eError     ����ʧ��                                                                *
 *  ˵����                                                                                       *
 *************************************************************************************************/
TState TclMallocObject(TObjPool* pPool, void** pAddr2, TError* pError)
{
    TState state = eError;
    TError error = OS_POOL_ERR_UNREADY;
    TBase32 imask;
    TChar* pTemp;

    OS_ASSERT((pPool != (TObjPool*)0), "");
    OS_ASSERT((pAddr2 != (void**)0), "");
    OS_ASSERT((pError != (TError*)0), "");

    OsCpuEnterCritical(&imask);

    if (pPool->Property & OS_POOL_PROP_READY)
    {
        /* �������ش��ڿ��Զ��� */
        if (pPool->ObjAvail > 0U)
        {
            /* ���׶�������ȥ,��ոö������� */
            pTemp = (TChar*)(pPool->ObjList);
            OsObjListRemoveNode(&(pPool->ObjList), (TLinkNode*)pTemp);
            memset((void*)pTemp, 0U, pPool->ObjSize);
            *pAddr2 = (void*)pTemp;
            pPool->ObjAvail--;

            error = OS_POOL_ERR_NONE;
            state = eSuccess;
        }
        else
        {
            error = OS_POOL_ERR_EMPTY;
        }
    }

    OsCpuLeaveCritical(imask);

    *pError = error;
    return state;
}


/*************************************************************************************************
 *  ����: ���������ͷŶ���                                                                     *
 *  ����: (1) pPool      ����ؽṹ��ַ                                                          *
 *        (2) pAddr      ���ͷŶ���ĵ�ַ                                                        *
 *        (3) pError     ��ϸ���ý��                                                            *
 *  ����: (1) eSuccess   �����ɹ�                                                                *
 *        (2) eError     ����ʧ��                                                                *
 *  ˵����                                                                                       *
 *************************************************************************************************/
TState TclFreeObject(TObjPool* pPool, void* pAddr, TError* pError)
{
    TState state = eError;
    TError error = OS_POOL_ERR_UNREADY;
    TBase32 imask;
    TBase32 index;
    TChar* pTemp;

    OS_ASSERT((pPool != (TObjPool*)0), "");
    OS_ASSERT((pAddr != (void*)0), "");
    OS_ASSERT((pError != (TError*)0), "");

    OsCpuEnterCritical(&imask);

    if (pPool->Property & OS_POOL_PROP_READY)
    {
        /* ��������ȷʵ�ж��󱻷����ȥ��δ���� */
        if (pPool->ObjAvail < pPool->ObjNbr)
        {
            /* ����ͷŵĶ����ַ�Ƿ���Ĵ��ں��ʵĿ���ʼ��ַ�ϡ�
               �˴�����Ҫ�󱻹���Ķ���ռ������������ */
            index = ((TChar*)pAddr - pPool->ObjAddr) / (pPool->ObjSize);
            //  index = (index < pPool->ObjNbr) ? index : pPool->ObjNbr;
            pTemp = pPool->ObjAddr + index * pPool->ObjSize;

            /* ����õ�ַ����������ȷʵ�Ǵ���ĳ��������׵�ַ */
            if (pTemp == (TChar*)pAddr)
            {
                /* ��ոö���,�ջظõ�ַ�Ķ��� */
                memset(pAddr, 0U, pPool->ObjSize);                
                OsObjListAddNode(&(pPool->ObjList), (TLinkNode*)pAddr, OsLinkTail);
                pPool->ObjAvail++;

                error = OS_POOL_ERR_NONE;
                state = eSuccess;
            }
            else
            {
                error = OS_POOL_ERR_ADDR;
            }
        }
        else
        {
            error = OS_POOL_ERR_FULL;
        }
    }

    OsCpuLeaveCritical(imask);

    *pError = error;
    return state;
}

#endif

