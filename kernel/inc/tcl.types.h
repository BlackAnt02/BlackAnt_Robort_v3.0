/*************************************************************************************************
 *                                     Trochili RTOS Kernel                                      *
 *                                  Copyright(C) 2017 LIUXUMING                                  *
 *                                       www.trochili.com                                        *
 *************************************************************************************************/
#ifndef _TCL_TYPES_H
#define _TCL_TYPES_H

/* �����������Ͷ��壬�ں���ֲʱ����ȷ�� */
typedef unsigned char      TByte;
typedef          char      TChar;
typedef unsigned int       TBase32;
typedef unsigned int       TAddr32;
typedef unsigned int       TBitMask;
typedef unsigned long long TTimeTick;
typedef unsigned int       TArgument;
typedef unsigned int       TError;

/* �������Ͷ���                    */
typedef enum
{
    eFalse = 0U,
    eTrue  = 1U
} TBool;

/* API�����������ֵ��ͳһ����  */
typedef enum
{
    eError   = 0U,
    eSuccess = 1U,
} TState;


/* �Ĵ�����д�궨�� */
#define TCLM_GET_REG32(r)   (*((volatile unsigned int*)(r)))
#define TCLM_SET_REG32(r,v) (*((volatile unsigned int*)(r)) = ((unsigned int)(v)))

#endif /* _TCL_TYPES_H */

