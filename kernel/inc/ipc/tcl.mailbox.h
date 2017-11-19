/*************************************************************************************************
 *                                     Trochili RTOS Kernel                                      *
 *                                  Copyright(C) 2017 LIUXUMING                                  *
 *                                       www.trochili.com                                        *
 *************************************************************************************************/
#ifndef _TCL_MAILBOX_H
#define _TCL_MAILBOX_H

#include "tcl.config.h"
#include "tcl.types.h"
#include "tcl.object.h"
#include "tcl.ipc.h"

#if ((TCLC_IPC_ENABLE)&&(TCLC_IPC_MAILBOX_ENABLE))

/* ����״̬���� */
typedef enum
{
    OsMailboxEmpty,                  /* �������ݿ�              */
    OsMailboxFull                    /* ����������              */
} TMailboxStatus;

/* �ʼ��ṹ���� */
typedef void* TMail;

/* ����ṹ���� */
struct MailboxDef
{
    TBitMask       Property;         /* �̵߳ĵ��Ȳ��Ե��������� */
    TMail          Mail;             /* ������ʼ�����           */
    TMailboxStatus Status;           /* �����״̬               */
    TIpcQueue      Queue;            /* ������߳���������       */
    TObject        Object;
};
typedef struct MailboxDef TMailbox;


extern TState TclCreateMailbox(TMailbox* pMailbox, TChar* pName, TBitMask property, TError* pError);
extern TState TclDeleteMailbox(TMailbox* pMailbox, TError* pError);
extern TState TclReceiveMail(TMailbox* pMailbox, TMail* pMail2, TBitMask option,
                             TTimeTick timeo, TError* pError);
extern TState TclSendMail(TMailbox* pMailbox, TMail* pMail2, TBitMask option, TTimeTick timeo,
                          TError* pError);
extern TState TclIsrSendMail(TMailbox* pMailbox, TMail* pMail2,  TError* pError);
extern TState TclBroadcastMail(TMailbox* pMailbox, TMail* pMail2, TError* pError);
extern TState TclResetMailbox(TMailbox* pMailbox, TError* pError);
extern TState TclFlushMailbox(TMailbox* pMailbox, TError* pError);

#endif

#endif /* _TCL_MAILBOX_H */

