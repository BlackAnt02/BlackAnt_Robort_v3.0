/*************************************************************************************************
 *                                     Trochili RTOS Kernel                                      *
 *                                  Copyright(C) 2017 LIUXUMING                                  *
 *                                       www.trochili.com                                        *
 *************************************************************************************************/
#ifndef _TCL_OBJECT_H
#define _TCL_OBJECT_H
#include "tcl.types.h"
#include "tcl.config.h"


/* �ں˶���ڵ�ṹ���� */
struct LinkNodeDef
{
    struct LinkNodeDef*  Prev;
    struct LinkNodeDef*  Next;
    struct LinkNodeDef** Handle;
    TBase32* Data;
};
typedef struct LinkNodeDef TLinkNode;

/* �ں˶����������е�λ�� */
typedef enum LinkPosDef
{
    OsLinkHead,
    OsLinkTail
} TLinkPos;

/* �ں˶������Ͷ��� */
enum ObjectTypeDef
{
    OsObjTypeThread = 0,
    OsObjTypeTimer,
    OsObjTypeSemaphore,
    OsObjTypeMutex,
    OsObjTypeMailbox,
    OsObjTypeMsgQue,
    OsObjTypeFlag
};
typedef enum ObjectTypeDef TObjectType;

/* �ں˶���ṹ���� */
struct ObjectDef
{
    TBase32       ID;                                    /* �ں˶�����     */
    TObjectType   Type;                                  /* �ں˶�������     */
    TChar         Name[TCL_OBJ_NAME_LEN];                /* �ں˶�������     */
    TLinkNode     LinkNode;                              /* �ں˶������ӽڵ� */
};
typedef struct ObjectDef TObject;

#define OS_OFFSET_OF(TYPE, MEMBER)  (TBase32)(&(((TYPE*)0)->MEMBER))  
#define OS_CONTAINER_OF(NODE, TYPE, MEMBER) ((TYPE*)((TByte*)(NODE) - OS_OFFSET_OF(TYPE, MEMBER)))

extern void OsObjQueueAddFifoNode(TLinkNode** pHandle2, TLinkNode* pNode, TLinkPos pos);
extern void OsObjQueueAddPriorityNode(TLinkNode** pHandle2, TLinkNode* pNode);
extern void OsObjQueueRemoveNode(TLinkNode** pHandle2, TLinkNode* pNode);
extern void OsObjListAddNode(TLinkNode** pHandle2, TLinkNode* pNode, TLinkPos pos);
extern void OsObjListRemoveNode(TLinkNode** pHandle2, TLinkNode* pNode);
extern void OsObjListAddPriorityNode(TLinkNode** pHandle2, TLinkNode* pNode);
extern void OsObjListAddDiffNode(TLinkNode** pHandle2, TLinkNode* pNode);
extern void OsObjListRemoveDiffNode(TLinkNode** pHandle2, TLinkNode* pNode);

#endif /* _TCL_OBJECT_H */

