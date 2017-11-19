/*************************************************************************************************
 *                                     Trochili RTOS Kernel                                      *
 *                                  Copyright(C) 2017 LIUXUMING                                  *
 *                                       www.trochili.com                                        *
 *************************************************************************************************/
#ifndef _TCL_POOL_H
#define _TCL_POOL_H

#include "tcl.types.h"
#include "tcl.config.h"
#include "tcl.object.h"
#include "tcl.memory.h"

#if (TCLC_POOL_ENABLE)

#define OS_POOL_ERR_NONE                 (0x0U)
#define OS_POOL_ERR_FAULT                (0x1<<1)
#define OS_POOL_ERR_UNREADY              (0x1<<2)
#define OS_POOL_ERR_EMPTY                (0x1<<3)
#define OS_POOL_ERR_ADDR                 (0x1<<4)
#define OS_POOL_ERR_FULL                 (0x1<<5)

#define OS_POOL_PROP_READY               (0x1<<0)

/* ����ؿ��ƿ�ṹ */
struct ObjPoolDef
{
    TBitMask   Property;                  /* ���������                      */
    TChar*     ObjAddr;                   /* �����С                        */
    TBase32    ObjNbr;                    /* ������Ķ������ʼ��ַ          */
    TBase32    ObjSize;                   /* ������Ŀ                        */
    TBase32    ObjAvail;                  /* ���ö�����Ŀ                    */
    TLinkNode* ObjList;                   /* ���ö�������ͷָ��              */
};
typedef struct ObjPoolDef TObjPool;


extern TState TclCreatePool(TObjPool* pPool, void* pAddr, TBase32 pages, TBase32 pgsize, 
                            TError* pError);
extern TState TclDeletePool(TObjPool* pPool, TError* pError);
extern TState TclMallocObject (TObjPool* pPool, void** pAddr2, TError* pError);
extern TState TclFreeObject (TObjPool* pPool, void* pAddr, TError* pError);
#endif

#endif /* _TCL_POOL_H  */

