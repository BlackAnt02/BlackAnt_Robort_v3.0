/*************************************************************************************************
 *                                     Trochili RTOS Kernel                                      *
 *                                  Copyright(C) 2017 LIUXUMING                                  *
 *                                       www.trochili.com                                        *
 *************************************************************************************************/
#ifndef _TCL_MEMORY_H
#define _TCL_MEMORY_H

#include "tcl.types.h"
#include "tcl.config.h"

#if ((TCLC_MEMORY_ENABLE) && (TCLC_MEMORY_BUDDY_ENABLE))

#define OS_MEM_ERR_NONE               (0x0U)
#define OS_MEM_ERR_FAULT              (0x1<<1)                      /* һ���Դ���                 */
#define OS_MEM_ERR_UNREADY            (0x1<<2)                      /* �ڴ����ṹδ��ʼ��       */
#define OS_MEM_ERR_NO_MEM             (0x1<<3)                      /* �ɷ����ڴ治��             */
#define OS_MEM_ERR_BAD_ADDR           (0x1<<4)                      /* �ͷŵ��ڴ��ַ�Ƿ�         */
#define OS_MEM_ERR_DBL_FREE           (0x1<<5)                      /* �ͷŵ��ڴ�û�б�����       */
#define OS_MEM_ERR_POOL_FULL          (0x1<<6)                      /* �ͷŵ��ڴ�û�б�����       */

#define OS_MEM_PROP_READY (0x1 << 0)

#define OS_MEM_BUDDY_PAGE_TAGS  ((TCLC_MEMORY_BUDDY_PAGES + 31U) >> 5U)
#define OS_MEM_BUDDY_NODE_TAGS (TCLC_MEMORY_BUDDY_PAGES * 2U - 1U)

typedef struct MemBuddyDef
{
    TBitMask Property;                       /* �ڴ�ҳ������                      */
    TChar*    PageAddr;                       /* ��������ڴ����ʼ��ַ            */
    TBase32   PageSize;                       /* �ڴ�ҳ��С                        */
    TBase32   PageNbr;                        /* �ڴ�ҳ��Ŀ                        */
    TBase32   PageAvail;                      /* �����ڴ�ҳ��Ŀ                    */
    TBitMask  PageTags[OS_MEM_BUDDY_PAGE_TAGS];  /* �ڴ�ҳ�Ƿ���ñ��                */
    TBase32   NodeNbr;
    TByte     NodeTags[OS_MEM_BUDDY_NODE_TAGS];
} TMemBuddy;


extern TState TclCreateMemoryBuddy(TMemBuddy* pBuddy, TChar* pAddr, TBase32 pages, TBase32 pagesize, TError* pError);
extern TState TclDeleteMemoryBuddy(TMemBuddy* pBuddy, TError* pError);
extern TState TclMallocBuddyMemory(TMemBuddy* pBuddy, TBase32 length, void** pAddr2, TError* pError);
extern TState TclFreeBuddyMemory(TMemBuddy* pBuddy,  void* pAddr, TError* pError);

#endif

#endif /* _TCL_MEMORY_H  */

