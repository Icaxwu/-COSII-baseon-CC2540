#ifndef __BSP_H__
#define __BSP_H__

#ifdef BSP_EXT_MODULE
#define BSP_EXT
#else
#define BSP_EXT extern
#endif

#include <os.h>

#define SLP_TIMER_INT_NUM             5
#define P0_INT_NUM                    13
#define IRQ_NUM_MAX                   18

#define  LED1_ID        0
#define  LED2_ID        1

typedef void (*interrupt_handle_sub_t)(void);

BSP_EXT void BSP_LED_Init(void);
BSP_EXT void  BSP_LED_Toggle (CPU_INT08U led);
BSP_EXT void SysClkSet_32M(void);
BSP_EXT void Set_ST_Period(CPU_INT16U cnts);
BSP_EXT CPU_BOOLEAN IRQ_register(unsigned char IRQ_num, interrupt_handle_sub_t handle_entry);


extern __data void * sysstktop;

#endif