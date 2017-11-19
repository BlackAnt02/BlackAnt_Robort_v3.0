#include "example.h"

#if (EVB_EXAMPLE == CH1_RTOS_SIMPLE_EXAMPLE)

#define THREAD_STACK_BYTES (512)
#define THREAD_PRIORITY    (5)
#define THREAD_SLICE       (20)

static TThread ThreadLed;
static TBase32 ThreadLedStack[THREAD_STACK_BYTES/4];


static void delay(TBase32 count)
{
    while (count--)
        ;
}

/* �߳�Led����ں��� */
static void ThreadLedEntry(TArgument data)
{
    int i = 0;
    while (eTrue)
    {
        delay(0x8FFFFF);
        i++;
    }
}

/* �û�Ӧ����ں��� */
static void AppSetupEntry(void)
{
    TError error;
    TState state;

    /* ��ʼ���߳� */
    state = TclCreateThread(&ThreadLed, "thread",
                          &ThreadLedEntry, (TArgument)0,
                          ThreadLedStack,  THREAD_STACK_BYTES,
                          THREAD_PRIORITY, THREAD_SLICE,
                          &error);
    TCLM_ASSERT((state == eSuccess), "");
    TCLM_ASSERT((error == TCLE_THREAD_NONE), "");
	
    /* �����߳� */
    state =TclActivateThread(&ThreadLed, &error);
    TCLM_ASSERT((state == eSuccess), "");
    TCLM_ASSERT((error == TCLE_THREAD_NONE), "");

    TclTrace("example start!\r\n");
}

extern void EvbSetupEntry(void);
extern void EvbTraceEntry(const char* str);

/* ������BOOT֮������main�����������ṩ */
int main(void)
{
    /* ע������ں˺���,�����ں� */
    TclStartKernel(&AppSetupEntry,
                   &OsCpuSetupEntry,
                   &EvbSetupEntry,
                   &EvbTraceEntry);
    return 1;
}

#endif
