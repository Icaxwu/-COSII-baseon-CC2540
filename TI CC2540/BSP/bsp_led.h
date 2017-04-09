#ifndef _BSP_LED_H_
#define _BSP_LED_H_

#define  LED1_ID        0
#define  LED2_ID        1

BSP_EXT void BSP_LED_Init(void);
BSP_EXT void  BSP_LED_Toggle (CPU_INT08U led);


#endif