#define BSP_EXT_MODULE
#include <bsp_inner.h>
#include <bsp.h>
__data unsigned char int_num;

void SYS_TICK_ISR_sys(void);
void UART0_TX_ISR_sys(void);
struct interrupt_handle_t {
    interrupt_handle_sub_t  handle_for_user;
    interrupt_handle_sub_t  handle_for_sys;
}  interrupt_handles[18] = { { 0, 0 },         //  0 
                             { 0, 0 },         //  1 
                             { 0, 0 },         //  2 
                             { 0, 0 },         //  3 
                             { 0, 0 },         //  4 
                             { 0, SYS_TICK_ISR_sys },         //  5 
                             { 0, 0 },         //  6 
                             { 0, UART0_TX_ISR_sys },         //  7 
                             { 0, 0 },         //  8 
                             { 0, 0 },         //  9 
                             { 0, 0 },         //  10 
                             { 0, 0 },         //  11 
                             { 0, 0 },         //  12 
                             { 0, 0 },         //  13
                             { 0, 0 },         //  14 
                             { 0, 0 },         //  15 
                             { 0, 0 },         //  16 
                             { 0, 0 },         //  17 
};

void interrupt_main_entry(void)  
{ 
    interrupt_handle_sub_t handle;
    
    handle = interrupt_handles[int_num].handle_for_sys;
    if (handle)
      handle();
} 


CPU_BOOLEAN IRQ_register(unsigned char IRQ_num, interrupt_handle_sub_t handle_entry)
{
    if (IRQ_NUM_MAX <= IRQ_num || handle_entry == 0)
      return DEF_FAIL;
    
    interrupt_handles[IRQ_num].handle_for_user = handle_entry;
    
    return DEF_OK;
}

void Set_ST_Period(CPU_INT16U cnts) 
{ 
   CPU_INT32U sleepTimer = 0; 
   sleepTimer |= (CPU_INT32U)ST0; 
   sleepTimer |= (CPU_INT32U)ST1 <<  8; 
   sleepTimer |= (CPU_INT32U)ST2 << 16; 
   sleepTimer += cnts; 
   ST2 = (CPU_INT08U)(sleepTimer >> 16); 
   ST1 = (CPU_INT08U)(sleepTimer >> 8); 
   ST0 = (CPU_INT08U) sleepTimer; 
}

void SYS_TICK_ISR_sys(void)
{
    interrupt_handle_sub_t handle;
    STIF = 0;
    EA = 1; // 打开中断
    Set_ST_Period(OSCfg_TickRate_Cnt);  // 32768/OSCfg_TickRate_Hz
    handle = interrupt_handles[SLP_TIMER_INT_NUM].handle_for_user;
    if (handle)
       handle();  
}

void UART0_TX_ISR_sys(void)
{
    interrupt_handle_sub_t handle;
    EA = 1; // 打开中断
    handle = interrupt_handles[UART0_TX_INT_NUM].handle_for_user;
    if (handle)
       handle();  
}
    
    
