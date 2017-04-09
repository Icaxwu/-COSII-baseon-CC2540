#ifndef __BSP_H__
#define __BSP_H__

#ifdef BSP_EXT_MODULE
#define BSP_EXT
#else
#define BSP_EXT extern
#endif

#include <os.h>
#include "bsp_uart.h"
#include "bsp_led.h"
#include "bsp_interrupt.h"

void BSPInit(void);


extern __data void * sysstktop;

#endif