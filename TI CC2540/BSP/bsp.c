#include <bsp_inner.h>

/****************************
LED初始化程序
*****************************/
void BSP_LED_Init(void)
{
  P1DIR |= 0x01;   //P1_0定义为输出
  LED1 = 0;        //LED1灯熄灭     
}

void  BSP_LED_Toggle (void)
{
    LED1 = ~LED1;
}