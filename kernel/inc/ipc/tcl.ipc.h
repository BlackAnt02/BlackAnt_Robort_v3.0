/*************************************************************************************************
 *                                     Trochili RTOS Kernel                                      *
 *                                  Copyright(C) 2017 LIUXUMING                                  *
 *                                       www.trochili.com                                        *
 *************************************************************************************************/
#ifndef _TCLC_IPC_H
#define _TCLC_IPC_H

#include "tcl.config.h"
#include "tcl.types.h"
#include "tcl.object.h"

#if (TCLC_IPC_ENABLE)

/* IPC�߳��������нṹ���� */
struct IpcBlockedQueueDef
{
    TBitMask*  Property;                      /* �߳�������������                       */
    TLinkNode* Handle;                        /* �����л����̷ֶ߳���                   */
};
typedef struct IpcBlockedQueueDef TIpcQueue;


/* IPC����������ں˴���ʹ�� */
#define OS_IPC_ERR_NONE           (TError)(0x0)       /* �����ɹ�                               */
#define OS_IPC_ERR_FAULT          (TError)(0x1<<0)    /* ����/�����÷�����                      */
#define OS_IPC_ERR_UNREADY        (TError)(0x1<<1)    /* IPC����û�б���ʼ��                    */
#define OS_IPC_ERR_NORMAL         (TError)(0x1<<2)    /* �ź���:�ź�������ֵ�����������
                                                       * �������Ϣ����:״̬���ܱ��������
                                                       * �¼����:�����͵��¼��Ѿ����� or
                                                       * �����¼�ʱ�ڴ����¼����ܱ�����
                                                       * �������� �������ѱ������߳�ռ�� or
                                                       * �����������ڵ�ǰ�߳�
                                                       */
#define OS_IPC_ERR_TIMEO          (TError)(0x1<<3)    /* ������ʱ�޵����̱߳�����               */
#define OS_IPC_ERR_DELETE         (TError)(0x1<<4)    /* IPC�������٣��̱߳�����              */
#define OS_IPC_ERR_RESET          (TError)(0x1<<5)    /* IPC�������ã��̱߳�����              */
#define OS_IPC_ERR_FLUSH          (TError)(0x1<<6)    /* IPC���������ϵ��̱߳�����ֹ            */
#define OS_IPC_ERR_ABORT          (TError)(0x1<<7)    /* IPC���������ϵ��̱߳�����ֹ            */
#define OS_IPC_ERR_DENIDED        (TError)(0x1<<8)    /* �̲߳�����������ʽ����IPC����          */

/* IPC�������ԣ��ں˴���ʹ�� */
#define OS_IPC_PROP_DEFAULT       (TBitMask)(0x0)
#define OS_IPC_PROP_READY         (TBitMask)(0x1<<0) /* IPC�����Ѿ�����ʼ��                    */
#define OS_IPC_PROP_PREEMP        (TBitMask)(0x1<<1) /* �߳��������в������ȼ����ȷ���     */

#define OS_RESET_SEMAPHORE_PROP   (OS_IPC_PROP_READY | OS_IPC_PROP_PREEMP)
#define OS_RESET_MUTEX_PROP       (OS_IPC_PROP_READY | OS_IPC_PROP_PREEMP)
#define OS_RESET_MBOX_PROP        (OS_IPC_PROP_READY | OS_IPC_PROP_PREEMP)
#define OS_RESET_MQUE_PROP        (OS_IPC_PROP_READY | OS_IPC_PROP_PREEMP)
#define OS_RESET_FLAG_PROP        (OS_IPC_PROP_READY | OS_IPC_PROP_PREEMP)

#define OS_USER_SEMAPHORE_PROP    (OS_IPC_PROP_PREEMP)
#define OS_USER_MUTEX_PROP        (OS_IPC_PROP_PREEMP)
#define OS_USER_MBOX_PROP         (OS_IPC_PROP_PREEMP)
#define OS_USER_MQUE_PROP         (OS_IPC_PROP_PREEMP)
#define OS_USER_FLAG_PROP         (OS_IPC_PROP_PREEMP)

/* �߳�IPCѡ��ں˴���ʹ�� */
#define OS_IPC_OPT_DEFAULT        (TBitMask)(0x0)
#define OS_IPC_OPT_WAIT           (TBitMask)(0x1<<1)   /* ���÷�ʽ�ȴ�IPC                        */
#define OS_IPC_OPT_TIMEO          (TBitMask)(0x1<<2)   /* ʱ�޷�ʽ�ȴ����                       */
#define OS_IPC_OPT_UARGENT        (TBitMask)(0x1<<3)   /* ��Ϣ���С��ʼ�ʹ��                     */
#define OS_IPC_OPT_AND            (TBitMask)(0x1<<4)   /* ����¼���ǲ�����AND����              */
#define OS_IPC_OPT_OR             (TBitMask)(0x1<<5)   /* ����¼���ǲ�����OR����               */
#define OS_IPC_OPT_CONSUME        (TBitMask)(0x1<<6)   /* �¼����ʹ��                           */

#define OS_IPC_OPT_SEMAPHORE      (TBitMask)(0x1<<16)  /* ����߳��������ź������߳�����������   */
#define OS_IPC_OPT_MUTEX          (TBitMask)(0x1<<17)  /* ����߳������ڻ��������߳�����������   */
#define OS_IPC_OPT_MAILBOX        (TBitMask)(0x1<<18)  /* ����߳�������������߳�����������     */
#define OS_IPC_OPT_MSGQUEUE       (TBitMask)(0x1<<19)  /* ����߳���������Ϣ���е��߳����������� */
#define OS_IPC_OPT_FLAGS          (TBitMask)(0x1<<20)  /* ����߳��������¼���ǵ��߳����������� */

#define OS_IPC_OPT_READ_DATA      (TBitMask)(0x1<<24)  /* �����ʼ�������Ϣ                       */
#define OS_IPC_OPT_WRITE_DATA     (TBitMask)(0x1<<25)  /* �����ʼ�������Ϣ                       */

#define OS_USER_SEMAPHORE_OPTION  (OS_IPC_OPT_WAIT | OS_IPC_OPT_TIMEO)
#define OS_USER_MUTEX_OPTION      (OS_IPC_OPT_WAIT | OS_IPC_OPT_TIMEO)
#define OS_USER_MBOX_OPTION       (OS_IPC_OPT_WAIT | OS_IPC_OPT_TIMEO | OS_IPC_OPT_UARGENT)
#define OS_USER_MSGQ_OPTION       (OS_IPC_OPT_WAIT | OS_IPC_OPT_TIMEO | OS_IPC_OPT_UARGENT)
#define OS_USER_FLAG_OPTION       (OS_IPC_OPT_WAIT | OS_IPC_OPT_TIMEO | OS_IPC_OPT_AND |\
                                   OS_IPC_OPT_OR | OS_IPC_OPT_CONSUME)

#define OS_ISR_MSGQ_OPTION        (OS_IPC_OPT_UARGENT)

/* NOTE: not compliant MISRA2004 18.4: Unions shall not be used. */
union IpcDataDef
{
    TBase32 Value;                                    /* ���汻�������ݱ����ĵ�ֵַַ           */
    void*   Addr1;                                    /* ָ���¼���ǵ�һ��ָ��                 */
    void**  Addr2;                                    /* ָ����Ϣ�����ʼ��Ķ���ָ��             */
};
typedef union IpcDataDef TIpcData;

/* �߳����ڼ�¼IPC�������ϸ��Ϣ�ļ�¼�ṹ */
struct IpcContextDef
{
    void*        Object;                              /* ָ��IPC�����ַ��ָ��                  */
    TIpcQueue*   Queue;                               /* �߳�����IPC�̶߳���ָ��                */
    TIpcData     Data;                                /* ��IPC���������ص�����ָ��            */
    TBase32      Length;                              /* ��IPC���������ص����ݳ���            */
    TBitMask     Option;                              /* ����IPC����Ĳ�������                  */
    TState*      State;                               /* IPC��������ķ���ֵ                    */
    TError*      Error;                               /* IPC��������Ĵ������                  */
    void*        Owner;                               /* IPC���������߳�                        */
    TLinkNode    LinkNode;                            /* �߳�����IPC���е�����ڵ�              */
};
typedef struct IpcContextDef TIpcContext;


extern void OsIpcBlockThread(TIpcContext* pContext, void* pIpc, TIpcQueue* pQueue, TBase32 data, TBase32 len,
                        TBitMask option, TTimeTick ticks, TState* pState, TError* pError, TBool* pHiRP);
extern void OsIpcUnblockThread(TIpcContext* pContext, TState state, TError error, TBool* pHiRP);
extern void OsIpcUnblockAll(TIpcQueue* pQueue, TState state, TError error, void** pData2, TBool* pHiRP);
extern void OsIpcCleanContext(TIpcContext* pContext);
extern void OsIpcSetPriority(TIpcContext* pContext, TBase32 priority);

#endif

#endif /* _TCLC_IPC_H */

