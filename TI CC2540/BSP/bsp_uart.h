#ifndef _BSP_UART_H_
#define _BSP_UART_H_
#include <bsp_inner.h>

typedef void (*BSPUARTCBack_t) (CPU_INT08U port, CPU_INT08U event);

typedef struct
{
  CPU_INT08U               baudRate;
  CPU_INT08U               flowControl;
  BSPUARTCBack_t      callBackFunc;
}BSPUARTInfo_t;

#define TRUE 1
#define FALSE 0
/*
   Serial Port Baudrate Settings
   Have to match with baudrate table
*/
#define BSP_UART_BR_9600   0x00
#define BSP_UART_BR_19200  0x01
#define BSP_UART_BR_38400  0x02
#define BSP_UART_BR_57600  0x03
#define BSP_UART_BR_115200 0x04

#define BV(n)      (1 << (n))

CPU_INT08U BSPUARTWriteISR(const CPU_INT08U *buf, CPU_INT08U len);
CPU_INT08U BSPUARTReadDMA(CPU_INT08U *buf, CPU_INT08U len);
CPU_INT08U BSPUARTRxAvailDMA(void);
void BSPUARTOpenDMA(BSPUARTInfo_t *config);
void BSPUARTInitDMA(void);
void BSPUart0TxIsr(void);

#endif