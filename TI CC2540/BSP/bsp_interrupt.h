#ifndef _BSP_INTERRUPT_H_
#define _BSP_INTERRUPT_H_

#define SLP_TIMER_INT_NUM             5
#define UART0_TX_INT_NUM              7
#define P0_INT_NUM                    13
#define IRQ_NUM_MAX                   18

typedef void (*interrupt_handle_sub_t)(void);

BSP_EXT CPU_BOOLEAN IRQ_register(unsigned char IRQ_num, interrupt_handle_sub_t handle_entry);

#endif