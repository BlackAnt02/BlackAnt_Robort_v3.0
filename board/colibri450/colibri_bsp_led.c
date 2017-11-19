/*************************************************************************************************
 *                                     Trochili RTOS Kernel                                      *
 *                                  Copyright(C) 2017 LIUXUMING                                  *
 *                                       www.trochili.com                                        *
 *************************************************************************************************/
#include "gd32f4xx.h"
#include "colibri_bsp_led.h"

/*************************************************************************************************
 *  ���ܣ���ʼ���û�Led�豸                                                                      *
 *  ��������                                                                                     *
 *  ���أ���                                                                                     *
 *  ˵����                                                                                       *
 *************************************************************************************************/
void EvbLedConfig(void)
{
    rcu_periph_clock_enable(RCU_GPIOB);
    gpio_mode_set(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_PIN_3);
    gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_3);
    gpio_mode_set(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_PIN_4);
    gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_4);

    rcu_periph_clock_enable(RCU_GPIOD);
    gpio_mode_set(GPIOD, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_PIN_7);
    gpio_output_options_set(GPIOD, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_7);
}



/*************************************************************************************************
 *  ���ܣ�����Led�ĵ�����Ϩ��                                                                    *
 *  ������(1) index Led�Ʊ��                                                                    *
 *        (2) cmd   Led�Ƶ�������Ϩ�������                                                      *
 *  ���أ���                                                                                     *
 *  ˵����                                                                                       *
 *************************************************************************************************/
void EvbLedControl(int index, int cmd)
{
    switch (index)
    {
        case LED1:
        {
            if (cmd == LED_ON)
            {
                gpio_bit_set(GPIOB, GPIO_PIN_4); /*����Led1��*/
            }
            else
            {
                gpio_bit_reset(GPIOB, GPIO_PIN_4); /*Ϩ��Led1��*/
            }
            break;
        }
        case LED2:
        {
            if (cmd == LED_ON)
            {
                gpio_bit_set(GPIOB, GPIO_PIN_3); /*����Led2��*/
            }
            else
            {
                gpio_bit_reset(GPIOB, GPIO_PIN_3); /*Ϩ��Led2��*/
            }
            break;
        }
        case LED3:
        {
            if (cmd == LED_ON)
            {
                gpio_bit_set(GPIOD, GPIO_PIN_7); /*����Led3��*/
            }
            else
            {
                gpio_bit_reset(GPIOD, GPIO_PIN_7); /*Ϩ��Led3��*/
            }
            break;
        }
        default:
        {
            break;
        }
    }
}

