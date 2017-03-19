#ifndef __BSP_H__
#define __BSP_H__

#include <os.h>

#define SLP_TIMER_INT_NUM             5
#define P0_INT_NUM                    13
#define IRQ_NUM_MAX                   18

typedef void (*interrupt_handle_sub_t)(void);

void BSP_LED_Init(void);
void BSP_LED_Toggle (void);
void SysClkSet_32M(void);
void Set_ST_Period(CPU_INT16U cnts);
CPU_BOOLEAN IRQ_register(unsigned char IRQ_num, interrupt_handle_sub_t handle_entry);


extern __data void * sysstktop;
extern __data unsigned char int_num;

#endif