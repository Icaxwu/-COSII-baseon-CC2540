#define BSP_EXT_MODULE
#include <bsp_inner.h>
#include <bsp.h>

typedef enum{
    PORT_0,
    PORT_1,
    PORT_2,
} port_e;

typedef struct {
    port_e port;
    CPU_INT08U pin;
    CPU_INT08U default_status;  // 0表示关闭， 1表示打开
} leds_cfg_t;

const leds_cfg_t bsp_leds_cfg[] = {
  {PORT_1, 0, 0},  // led1
  {PORT_1, 1, 1},  // led2
};



/****************************
LED初始化程序
*****************************/
void BSP_LED_Init(void)
{
  const leds_cfg_t  *ptr = bsp_leds_cfg;
  CPU_INT08U i, led_num = sizeof (bsp_leds_cfg) / sizeof(leds_cfg_t);
  CPU_INT08U val;
    
  for (i = 0; i < led_num; i++)
  {
    val = (1 << ptr->pin);
    switch (ptr->port)
    {
        case PORT_0:
            P0DIR |= val;
            if (ptr->default_status)
            {
                P0 |= val;
            } else {
                P0 &= ~val;
            }
            break;
        case PORT_1:
            P1DIR |= val;
            if (ptr->default_status)
            {
                P1 |= val;
            } else {
                P1 &= ~val;
            }
            break;
        case PORT_2:
            P2DIR |= val;
            if (ptr->default_status)
            {
                P2 |= val;
            } else {
                P2 &= ~val;
            }
            break;
        default:
            ;
    }
    ptr++;
  }
}
void  BSP_LED_Toggle (CPU_INT08U led)
{
    const leds_cfg_t  *ptr = &bsp_leds_cfg[led];
    CPU_INT08U val = (1 << ptr->pin);
    switch (ptr->port)
    {
        case PORT_0:
            if (P0 & val)
            {
                P0 &= ~val;
            }
            else
            {
                P0 |= val; 
            }
            break;
        case PORT_1:
            if (P1 & val)
            {
                P1 &= ~val; 
            }
            else
            {
                P1 |= val; 
            }

            break;
        case PORT_2:
            if (P2 & val)
            {
                P2 &= ~val; 
            }
            else
            {
                P2 |= val; 
            }

            break;
        default:
            ;
    }
}

void  BSP_LED_ON (CPU_INT08U led)
{
    const leds_cfg_t  *ptr = &bsp_leds_cfg[led];
    CPU_INT08U val = (1 << ptr->pin);
    port_e prt = ptr->port;
    
    if (prt == PORT_0)
    {
        P0 |= val;
    }
    else if (prt == PORT_1)
    {
        P1 |= val;
    }
    else
    {
        P2 |= val;
    }
}

void  BSP_LED_OFF (CPU_INT08U led)
{
    const leds_cfg_t  *ptr = &bsp_leds_cfg[led];
    CPU_INT08U val = (1 << ptr->pin);
    port_e prt = ptr->port;
    
    if (prt == PORT_0)
    {
        P0 &= ~val;
    }
    else if (prt == PORT_1)
    {
        P1 &= ~val;  
    }
    else
    {
        P2 &= ~val;
    }
}