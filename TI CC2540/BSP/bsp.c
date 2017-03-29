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










