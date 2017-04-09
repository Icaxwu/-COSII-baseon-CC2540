#define BSP_EXT_MODULE
#include <bsp_inner.h>
#include <bsp.h>

__data void * sysstktop;

void SysClkSet_32M(void)
{
  CLKCONCMD &= ~0x40;
  while(CLKCONSTA & 0x40);    
  CLKCONCMD &= ~0x47;  
}

void BSPInit(void)
{
    SysClkSet_32M();
    BSP_LED_Init(); 
    BSPUARTInitDMA();
    
    // 注册中断处理函数
    IRQ_register(SLP_TIMER_INT_NUM, OS_CPU_SysTickHandler);
    IRQ_register(UART0_TX_INT_NUM, BSPUart0TxIsr);
    
    OS_CPU_SysTickInit(OSCfg_TickRate_Cnt);
}









