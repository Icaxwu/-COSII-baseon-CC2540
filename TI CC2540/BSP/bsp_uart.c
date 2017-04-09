#include "BSP_uart.h"
#include <string.h>
#define st(x)      do { x } while (__LINE__ == -1)
#define HI_UINT16(a) (((a) >> 8) & 0xFF)
#define LO_UINT16(a) ((a) & 0xFF)

/*
#define _PRAGMA(x) _Pragma(#x)
#define BSP_ISR_FUNC_DECLARATION(f,v)   _PRAGMA(vector=v) __near_func __interrupt void f(void)
#define BSP_ISR_FUNC_PROTOTYPE(f,v)     _PRAGMA(vector=v) __near_func __interrupt void f(void)
#define BSP_ISR_FUNCTION(f,v)           BSP_ISR_FUNC_PROTOTYPE(f,v); BSP_ISR_FUNC_DECLARATION(f,v)
*/

#define BSP_UART_DMA_RX_MAX        128
#define BSP_UART_ISR_TX_MAX        BSP_UART_DMA_RX_MAX

/* 
    ���㻹�ж��ٿ���ռ䣨������ж��������෴�� 
    txHead��txTail����ָ�����ݵģ���������ĵ�ʽ����Ҫ-1
*/
#define BSP_UART_ISR_TX_AVAIL() \
  ((uartCfg.txHead > uartCfg.txTail) ? \
  (uartCfg.txHead - uartCfg.txTail - 1) : \
  (BSP_UART_ISR_TX_MAX - uartCfg.txTail + uartCfg.txHead - 1))
    
#define UTX0IE                     0x04

#define BSP_UART_PERCFG_BIT        0x01         // USART0 on P0, Alt-1; so clear this bit.
#define BSP_UART_PRIPO             0x00         // USART0 priority over UART1.
#define BSP_UART_Px_CTS            0x10         // Peripheral I/O Select for CTS flow control.
#define BSP_UART_Px_RTS            0x20         // Peripheral I/O Select for RTS must be manual.
#define BSP_UART_Px_SEL            0x0C         // Peripheral I/O Select for Rx/Tx.

    
#define DMA_RDYIn                  P0_4
#define DMA_RDYOut                 P0_5   
#define DMA_RDYIn_BIT              BV(4)        // Same as the I/O Select for CTS flow control.
#define DMA_RDYOut_BIT             BV(5)        // Same as the I/O Select for manual RTS flow ctrl.
#define BSP_UART_DMA_CLR_RDY_OUT()     (DMA_RDYOut = 1)
#define BSP_UART_DMA_SET_RDY_OUT()     (DMA_RDYOut = 0)

#define BSP_UART_DMA_RDY_IN()          (DMA_RDYIn == 0)
#define BSP_UART_DMA_RDY_OUT()         (DMA_RDYOut == 0)   
    
// UxUCR - USART UART Control Register.
#define UCR_FLUSH                  0x80
#define UCR_FLOW                   0x40    
#define UCR_STOP                   0x02
    
// UxCSR - USART Control and Status Register.
#define CSR_MODE                   0x80   
#define CSR_RE                     0x40
    
#define P2DIR_PRIPO                0xC0    
    
#define BSP_DMA_CH_RX                  3   
#define BSP_DMA_U0DBUF             0x70C1
#define DMA_UxDBUF                 BSP_DMA_U0DBUF    
#define BSP_DMA_VLEN_USE_LEN            0x00   // Use LEN for transfer count
// Bit fields of the 'lenModeH'
#define BSP_DMA_LEN_V     0xE0
#define BSP_DMA_LEN_H     0x1F

// Bit fields of the 'ctrlA'
#define BSP_DMA_WORD_SIZE 0x80
#define BSP_DMA_TRIG_MODE 0x60
#define BSP_DMA_TRIG_SRC  0x1F

// Bit fields of the 'ctrlB'
#define BSP_DMA_SRC_INC   0xC0
#define BSP_DMA_DST_INC   0x30
#define BSP_DMA_IRQ_MASK  0x08
#define BSP_DMA_M8        0x04
#define BSP_DMA_PRIORITY  0x03
    
#define BSP_DMA_WORDSIZE_WORD           0x01 /* Transfer a 16-bit word at a time. */   
#define BSP_DMA_TMODE_SINGLE_REPEATED   0x02 /* Transfer single byte/word (after len transfers, rearm DMA). */
#define BSP_DMA_TRIG_URX0          14   /* USART0 RX complete. */
#define DMATRIG_RX                 BSP_DMA_TRIG_URX0
#define BSP_DMA_SRCINC_0         0x00 /* Increment source pointer by 0 bytes/words after each transfer. */
#define BSP_DMA_DSTINC_1         0x01 /* Increment destination pointer by 1 bytes/words after each transfer. */
#define BSP_DMA_IRQMASK_DISABLE  0x00 /* Disable interrupt generation. */    
#define BSP_DMA_M8_USE_8_BITS    0x00 /* Use all 8 bits for transfer count. */
#define BSP_DMA_PRI_HIGH         0x02 /* High, DMA has priority. */
#define DMA_PAD                    U0BAUD    
 
#define BSP_DMA_GET_DESC1234( a )     (dmaCh1234+((a)-1))
#define BSP_DMA_ABORT_CH( ch )         DMAARM = (0x80 | (0x01 << (ch)))
    
#define BSP_DMA_SET_ADDR_DESC1234( a ) \
  st( \
    DMA1CFGH = (CPU_INT08U)( (CPU_INT16U)(a) >> 8 );  \
    DMA1CFGL = (CPU_INT08U)( (CPU_INT16U)(a) & 0xFF); \
  ) 
// Macro for quickly setting the source address of a DMA structure.
#define BSP_DMA_SET_SOURCE( pDesc, src ) \
  st( \
    pDesc->srcAddrH = (CPU_INT08U)((CPU_INT16U)(src) >> 8); \
    pDesc->srcAddrL = (CPU_INT08U)((CPU_INT16U)(src) & 0xFF); \
  )
// Macro for quickly setting the destination address of a DMA structure.
#define BSP_DMA_SET_DEST( pDesc, dst ) \
  st( \
    pDesc->dstAddrH = (CPU_INT08U)((CPU_INT16U)(dst) >> 8); \
    pDesc->dstAddrL = (CPU_INT08U)((CPU_INT16U)(dst) & 0xFF); \
  )
    // xferLenV��bit[5:7]��Ӧ��VLEN[0:2]
#define BSP_DMA_SET_VLEN( pDesc, vMode ) \
  st( \
    pDesc->xferLenV &= ~BSP_DMA_LEN_V; \
    pDesc->xferLenV |= (vMode << 5); \
  )
// Macro for quickly setting the number of bytes to be transferred by the DMA,
// max length is 0x1FFF.
#define BSP_DMA_SET_LEN( pDesc, len ) \
  st( \
    pDesc->xferLenL = (CPU_INT08U)(CPU_INT16U)(len); \
    pDesc->xferLenV &= ~BSP_DMA_LEN_H; \
    pDesc->xferLenV |= (CPU_INT08U)((CPU_INT16U)(len) >> 8); \
  )
#define BSP_DMA_SET_WORD_SIZE( pDesc, xSz ) \
  st( \
    pDesc->ctrlA &= ~BSP_DMA_WORD_SIZE; \
    pDesc->ctrlA |= (xSz << 7); \
  )

#define BSP_DMA_SET_TRIG_MODE( pDesc, tMode ) \
  st( \
    pDesc->ctrlA &= ~BSP_DMA_TRIG_MODE; \
    pDesc->ctrlA |= (tMode << 5); \
  )

#define BSP_DMA_GET_TRIG_MODE( pDesc ) ((pDesc->ctrlA >> 5) & 0x3)

#define BSP_DMA_SET_TRIG_SRC( pDesc, tSrc ) \
  st( \
    pDesc->ctrlA &= ~BSP_DMA_TRIG_SRC; \
    pDesc->ctrlA |= tSrc; \
  )

#define BSP_DMA_SET_SRC_INC( pDesc, srcInc ) \
  st( \
    pDesc->ctrlB &= ~BSP_DMA_SRC_INC; \
    pDesc->ctrlB |= (srcInc << 6); \
  )

#define BSP_DMA_SET_DST_INC( pDesc, dstInc ) \
  st( \
    pDesc->ctrlB &= ~BSP_DMA_DST_INC; \
    pDesc->ctrlB |= (dstInc << 4); \
  )

#define BSP_DMA_SET_IRQ( pDesc, enable ) \
  st( \
    pDesc->ctrlB &= ~BSP_DMA_IRQ_MASK; \
    pDesc->ctrlB |= (enable << 3); \
  )

#define BSP_DMA_SET_M8( pDesc, m8 ) \
  st( \
    pDesc->ctrlB &= ~BSP_DMA_M8; \
    pDesc->ctrlB |= (m8 << 2); \
  )

#define BSP_DMA_SET_PRIORITY( pDesc, pri ) \
  st( \
    pDesc->ctrlB &= ~BSP_DMA_PRIORITY; \
    pDesc->ctrlB |= pri; \
  )  
#define BSP_DMA_CLEAR_IRQ( ch )        DMAIRQ = (~(1 << (ch)) & 0x1F)
#define BSP_DMA_ARM_CH( ch )           DMAARM = (0x01 << (ch))

#define BUILD_UINT16(loByte, hiByte) \
          ((CPU_INT16U)(((loByte) & 0x00FF) + (((hiByte) & 0x00FF) << 8)))    
#define BSP_UART_DMA_NEW_RX_BYTE(IDX)  ((CPU_INT08U)DMA_PAD == HI_UINT16(uartCfg.rxBuf[(IDX)]))
#define BSP_UART_DMA_GET_RX_BYTE(IDX)  (*(volatile CPU_INT08U *)(uartCfg.rxBuf+(IDX)))
#define BSP_UART_DMA_CLR_RX_BYTE(IDX)  (uartCfg.rxBuf[(IDX)] = BUILD_UINT16(0, (DMA_PAD ^ 0xFF)))
#define BSP_UART_RX_IDX_T_INCR(IDX) st (  \
  if (++(IDX) >= BSP_UART_DMA_RX_MAX) \
  { \
    (IDX) = 0; \
  } \
)       
    
typedef struct {
  CPU_INT08U srcAddrH;
  CPU_INT08U srcAddrL;
  CPU_INT08U dstAddrH;
  CPU_INT08U dstAddrL;
  CPU_INT08U xferLenV;  // bit[0:4]��Ϊlen��[8:12]�� bit[5:7]��ΪVLEN[0:2]��
  CPU_INT08U xferLenL;  // len��bit[0:7]
  CPU_INT08U ctrlA;
  CPU_INT08U ctrlB;
} BSPDMADesc_t;    
    
typedef struct
{
  CPU_INT16U rxBuf[BSP_UART_DMA_RX_MAX];  // ע��������CPU_INT16U����txBuf��ͬ
  CPU_INT08U rxHead;
  CPU_INT08U rxTail;

  CPU_INT08U txBuf[BSP_UART_ISR_TX_MAX];
  volatile CPU_INT08U txHead;
  CPU_INT08U txTail;
  CPU_INT08U txMT;    // true��ʾtxBufΪ��   false��ʾtxBuf��Ϊ��        

  BSPUARTCBack_t uartCB;
} uartCfg_t;

static uartCfg_t uartCfg;
BSPDMADesc_t dmaCh1234[4];

/* 
	NPI_InitTransport ����� BSPUARTOpenDMA ���� 
	�ڱ�����ǰ����BSP uart��ʼ��ʱ����� BSPUARTInitDMA �ȳ�ʼ��һ�´���
*/
void BSPUARTOpenDMA(BSPUARTInfo_t *config)
{
  uartCfg.uartCB = config->callBackFunc;


  if (config->baudRate == BSP_UART_BR_57600 ||
      config->baudRate == BSP_UART_BR_115200)
  {
    U0BAUD = 216;
  }
  else
  {
    U0BAUD = 59;
  }

  switch (config->baudRate)
  {
    case BSP_UART_BR_9600:
      U0GCR = 8;
      break;
    case BSP_UART_BR_19200:
      U0GCR = 9;
      break;
    case BSP_UART_BR_38400:
    case BSP_UART_BR_57600:
      U0GCR = 10;
      break;
    default:
      // BSP_UART_BR_115200
      U0GCR = 11;
      break;
  }
  /*
    ���� POWER_SAVING ���ʱ��DMA_PM��ֵΪ1
  */
  if (config->flowControl)
  {
    U0UCR = UCR_FLOW | UCR_STOP;      // ����;  8 bits/char; no parity; 1 stop bit; stop bit high.
    P0SEL |= BSP_UART_Px_CTS;         // Enable Peripheral control of CTS flow control on Px.ʹ��CTS���ŵ����蹦��
    
    // DMA Rx is always on (self-resetting). So flow must be controlled by the S/W polling the
    // circular Rx queue depth. Start by allowing flow.
    BSP_UART_DMA_SET_RDY_OUT();  // RTS ����
    P0DIR |= BSP_UART_Px_RTS;   //  RTS pin����Ϊ���
  }

  U0CSR = (CSR_MODE | CSR_RE);       // uartģʽ��ʹ�ܽ���

  UTX0IF = 1;  // Prime the ISR pump.     // �ֶ�pendһ��TX interrupt
}    
 
void BSPUARTInitDMA(void)
{
  BSPDMADesc_t *ch, dmaCh;
  ch = &dmaCh;

  PERCFG &= ~BSP_UART_PERCFG_BIT;    // Set UART0 I/O to Alt. 1 location on P0.
  P0SEL  |= BSP_UART_Px_SEL;         // Enable Peripheral control of Rx/Tx on Px.
  U0CSR = CSR_MODE;                  // Mode is UART Mode.
  U0UCR = UCR_FLUSH;                 // Flush it.

  P2DIR &= ~P2DIR_PRIPO;             // ��� PRIP0[1:0]
  P2DIR |= BSP_UART_PRIPO;           // ���� 1st priority: USART 0 2nd priority: USART 1

  BSP_UART_DMA_CLR_RDY_OUT();
  P0DIR |= DMA_RDYOut_BIT;   // RTS pin����Ϊ���
  
  // ����DMA��config �ڴ���ַ
  BSP_DMA_SET_ADDR_DESC1234(dmaCh1234);
  
  ch = BSP_DMA_GET_DESC1234( BSP_DMA_CH_RX );
  // Abort any pending DMA operations (in case of a soft reset).
  BSP_DMA_ABORT_CH( BSP_DMA_CH_RX );

  // The start address of the source.
  BSP_DMA_SET_SOURCE(ch, DMA_UxDBUF );         //Դ��ַ �Ĵ���UxDBUF��xdata�ռ�ĵ�ַ   

  // Using the length field to determine how many bytes to transfer.
  BSP_DMA_SET_VLEN( ch, BSP_DMA_VLEN_USE_LEN );

  /* The trick is to cfg DMA to xfer 2 bytes for every 1 byte of Rx.
   * The byte after the Rx Data Buffer is the Baud Cfg Register, U0DBUF�Ĵ�����U0BAUD�Ĵ����ĵ�ַ��������
   * which always has a known value. So init Rx buffer to inverse of that
   * known value. DMA word xfer will flip the bytes, so every valid Rx byte
   * in the Rx buffer will be preceded by a DMA_PAD char equal to the
   * Baud Cfg Register value.
   */
  BSP_DMA_SET_WORD_SIZE( ch, BSP_DMA_WORDSIZE_WORD );   // 16bit Ϊ���䵥Ԫ

  // The bytes are transferred 1-by-1 on Rx Complete trigger.
  /* ÿ������len���ֽ����ݣ��ͻ��ظ�ǰ��Ķ�����Դ��ַ���䣬Ŀ�ĵ�ַ�ָ�����ʼ��ֵ */
  BSP_DMA_SET_TRIG_MODE( ch, BSP_DMA_TMODE_SINGLE_REPEATED );  // �����ظ�����ģʽ
  BSP_DMA_SET_TRIG_SRC( ch, DMATRIG_RX );

  // The source address is constant - the Rx Data Buffer.
  BSP_DMA_SET_SRC_INC( ch, BSP_DMA_SRCINC_0 );

  // The destination address is incremented by 1 word after each transfer.
  BSP_DMA_SET_DST_INC( ch, BSP_DMA_DSTINC_1 );
  BSP_DMA_SET_DEST( ch, uartCfg.rxBuf );                      // Ŀ�ĵ�ַ dmaCfg.rxBuf
  BSP_DMA_SET_LEN( ch, BSP_UART_DMA_RX_MAX );                // 

  // The DMA is to be polled and sBSPl not issue an IRQ upon completion.
  BSP_DMA_SET_IRQ( ch, BSP_DMA_IRQMASK_DISABLE );           // ��ֹ�ж�

  // Xfer all 8 bits of a byte xfer.
  BSP_DMA_SET_M8( ch, BSP_DMA_M8_USE_8_BITS );

  // DMA has highest priority for memory access.
  BSP_DMA_SET_PRIORITY( ch, BSP_DMA_PRI_HIGH);

  volatile CPU_INT08U dummy = *(volatile CPU_INT08U *)DMA_UxDBUF;  // Clear the DMA Rx trigger.
  BSP_DMA_CLEAR_IRQ(BSP_DMA_CH_RX);
  BSP_DMA_ARM_CH(BSP_DMA_CH_RX);
  (void)memset(uartCfg.rxBuf, (DMA_PAD ^ 0xFF), BSP_UART_DMA_RX_MAX * sizeof(CPU_INT16U));   // ���õĳ�ʼֵ(DMA_PAD ^ 0xFF)
}
      

    
CPU_INT08U BSPUARTWriteISR(const CPU_INT08U *buf, CPU_INT08U len)
{
  // Enforce all or none.Ҫôȫ�����䣬Ҫô������
  if (BSP_UART_ISR_TX_AVAIL() < len) // ���ʣ���tx buf����len���򲻷���
  {
    return 0;
  }

  for (CPU_INT08U cnt = 0; cnt < len; cnt++)
  {
    uartCfg.txBuf[uartCfg.txTail] = *buf++;
    uartCfg.txMT = 0;

    if (uartCfg.txTail >= BSP_UART_ISR_TX_MAX-1)
    {
      uartCfg.txTail = 0;
    }
    else
    {
      uartCfg.txTail++;
    }

    // Keep re-enabling ISR as it might be keeping up with this loop due to other ints.
	/* 
	  ʹ��TX interrupt����������������жϣ��жϳ���Ҳ��ܿ챻ִ����ϣ����ص����
	  ���ǲ�����ζ���ж�����ġ�UxDBUF = dmaCfg.txBuf[dmaCfg.txHead++];��ִ����󣬾ͻ����ϲ�����һ��
	  TX interrupt�жϣ���ΪCPU���ٶ�Զ�ȴ�������Ҫ�죬TX interrupt�жϵ��ٴβ����ǵ����������Ĵ�
	  TX�����Ϸ������֮��
	*/
    IEN2 |= UTX0IE;   
  }

  return len;
}

/*
  ��ȡ��ǰ���ջ�����rxBuf�е��������ݵ�����
*/		  
CPU_INT08U BSPUARTRxAvailDMA(void)
{
  CPU_INT08U cnt = 0;
  // First, synchronize the Rx tail marker with where the DMA Rx engine is working.
  CPU_INT08U tail = uartCfg.rxTail;

  if (U0UCR & UCR_FLOW)
  {
    BSP_UART_DMA_CLR_RDY_OUT();  // Stop the inflow for counting the bytes
  }

  do
  {
	  /*
	    BSP_UART_DMA_NEW_RX_BYTE�жϵĸ�����ϸ�Ľ��ܲμ�BSPUARTInitDMA�����е�ע��
	  */
    if (!BSP_UART_DMA_NEW_RX_BYTE(tail))
    {
      break;
    }
    else
    {
      cnt++;
    }
    BSP_UART_RX_IDX_T_INCR(tail);
  } while (cnt  < BSP_UART_DMA_RX_MAX);  //  �����������ܴ��� BSP_UART_DMA_RX_MAX


  if (U0UCR & UCR_FLOW) 
  {
    BSP_UART_DMA_SET_RDY_OUT();  // Re-enable the flow asap
  }
  return cnt;
}

CPU_INT08U BSPUARTReadDMA(CPU_INT08U *buf, CPU_INT08U len)
{
  CPU_INT08U cnt;

  for (cnt = 0; cnt < len; cnt++)
  {
    if (!BSP_UART_DMA_NEW_RX_BYTE(uartCfg.rxHead))
    {
      break;
    }
    *buf++ = BSP_UART_DMA_GET_RX_BYTE(uartCfg.rxHead);
	/*
    	����֮������������ķ�ʽ������Ϊ BUILD_UINT16(0, (DMA_PAD ^ 0xFF)
		�൱������һ����־����ʾavail
	*/
    BSP_UART_DMA_CLR_RX_BYTE(uartCfg.rxHead); 
    /*
	  rxHeadֻ��������ᱻ���£�Ҳ����˵BSPUARTReadDMA��ÿ�ε��ö��ǻ���ϴζ���ĵط���ʼ��
	*/    
    BSP_UART_RX_IDX_T_INCR(uartCfg.rxHead);
  }

  /* Update pointers after reading the bytes 
     ���׳�����β�롣������Ҫע����ǣ����ڴ�ʱ��������DMA���UxDBUF��rxBuf�Ĵ��ͣ�����DMAӲ�����������
	 rxTail�������������rxTail����Ҫ�ڴ˸��£����Ǵ�ʱ��rxTail������ִ�ж�β��Ԫ�أ�����ָ���ϴζ������
	 λ�ã���һ�����Ҫ��
	 ������ BSPUARTRxAvailDMA�����У�������BSP_UART_RX_IDX_T_INCR(tail)������head��ͳ�ƽ������ݵĸ���
  */
  uartCfg.rxTail = uartCfg.rxHead;

  if (U0UCR & UCR_FLOW)
  {
	/*
	  Re-enable the flow asap (i.e. not wait until next uart poll).
	  ��
	*/
    BSP_UART_DMA_SET_RDY_OUT();       
  }

  return cnt;
}

void BSPUart0TxIsr(void)
{

  if (uartCfg.txHead == uartCfg.txTail)
  {
	/* 
	  ���txBufΪ�յĻ���������� TX Interrupt��
	  ���ǲ�������жϱ�־�������������ٴα���ʱ�ͻ����̷����жϡ�
	  BSPUARTOpenDMA�����л�ͨ����UTXxIF = 1;��pend��һ���ж�
	*/
    IEN2 &= ~UTX0IE;   
    uartCfg.txMT = 1;
  }
  else
  {
    UTX0IF = 0;
	/*
	  ������UxDBUF�Ĵ���д�����Ǻܿ�ģ�����ֻ�е�����tx�������ϵ����ݷ�����Ϻ�Ż�pend��UTXxIF��־��
	  Ҳ��˵��ǰ�жϷ�����򷵻ص� BSPUARTWriteDMA�����ܻ���װ�غܶ����ݵ�txBuf��UTXxIF��־�ű�����Ȼ�����ִ�б��жϷ������
	  ��Ҳ����Ϊ���ϲ����д����BSP��Ļ�������������ֱ��д���ڼĴ����������Ͳ���Ҫ�ȴ�UTXxIF��ɱ�־������������˷���Ч�ʣ�CPU����Ҫ�ȴ��ٶȺ����Ĵ��ڣ���
	*/
    U0DBUF = uartCfg.txBuf[uartCfg.txHead++];

    if ((BSP_UART_ISR_TX_MAX != 256) && (uartCfg.txHead >= BSP_UART_ISR_TX_MAX))
    {
      uartCfg.txHead = 0;
    }
  }

}  