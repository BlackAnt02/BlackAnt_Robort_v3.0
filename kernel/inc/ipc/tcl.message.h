/*************************************************************************************************
 *                                     Trochili RTOS Kernel                                      *
 *                                  Copyright(C) 2017 LIUXUMING                                  *
 *                                       www.trochili.com                                        *
 *************************************************************************************************/
#ifndef _TCL_MQUEUE_H
#define _TCL_MQUEUE_H

#include "tcl.config.h"
#include "tcl.types.h"
#include "tcl.object.h"
#include "tcl.ipc.h"

#if ((TCLC_IPC_ENABLE)&&(TCLC_IPC_MSGQUE_ENABLE))

/* ��Ϣ����״̬���� */
enum MQStatus
{
    OsMQEmpty,                /* ��Ϣ���п�              */
    OsMQFull,                 /* ��Ϣ������              */
    OsMQPartial               /* ��Ϣ���зǿշ���        */
};
typedef enum MQStatus TMQStatus;

/* ��Ϣ���ݽṹ���� */
typedef void* TMessage;

/* ��Ϣ���нṹ���� */
struct MessageQueueCB
{
    TBitMask  Property;       /* ��Ϣ������������        */
    void**    MsgPool;        /* ��Ϣ�����              */
    TBase32   Capacity;       /* ��Ϣ��������            */
    TBase32   MsgEntries;     /* ��Ϣ��������Ϣ����Ŀ    */
    TBase32   Head;           /* ��Ϣ����дָ��λ��      */
    TBase32   Tail;           /* ��Ϣ���ж�ָ��λ��      */
    TMQStatus Status;         /* ��Ϣ����״̬            */
    TIpcQueue Queue;          /* ��Ϣ���е��߳���������  */
    TObject   Object;
};
typedef struct MessageQueueCB TMsgQueue;


extern TState TclCreateMsgQueue(TMsgQueue* pMsgQue, TChar* pName, void** pPool2, TBase32 capacity,
                                TBitMask property, TError* pError);
extern TState TclDeleteMsgQueue(TMsgQueue* pMsgQue, TError* pError);
extern TState TclReceiveMessage(TMsgQueue* pMsgQue, TMessage* pMsg2, TBitMask option,
                                TTimeTick timeo, TError* pError);
extern TState TclSendMessage(TMsgQueue* pMsgQue, TMessage* pMsg2, TBitMask option,
                             TTimeTick timeo, TError* pError);
extern TState TclIsrSendMessage(TMsgQueue* pMsgQue, TMessage* pMsg2, TBitMask option, TError* pError);
extern TState TclBroadcastMessage(TMsgQueue* pMsgQue, TMessage* pMsg2, TError* pError);
extern TState TclFlushMsgQueue(TMsgQueue* pMsgQue, TError* pError);
extern TState TclResetMsgQueue(TMsgQueue* pMsgQue, TError* pError);
#endif

#endif /* _TCL_MQUEUE_H */
