#include <bsp_inner.h>

/****************************
LED��ʼ������
*****************************/
void BSP_LED_Init(void)
{
  P1DIR |= 0x01;   //P1_0����Ϊ���
  LED1 = 0;        //LED1��Ϩ��     
}

void  BSP_LED_Toggle (void)
{
    LED1 = ~LED1;
}