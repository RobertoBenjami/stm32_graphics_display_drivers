/* 
 * 16 bit paralell LCD FSMC driver for STM32F1
 * 5 controll pins (CS, RS, WR, RD, RST) + 16 data pins + 1 backlight pin
 */

/* 
 * Author: Roberto Benjami
 * version:  2020.05
 */

#include "main.h"
#include "lcd.h"
#include "lcd_io_fsmc16.h"

#define  LCD_ADDR_DATA       (LCD_ADDR_BASE + (1 << (LCD_REGSELECT_BIT + 2)) - 2)
#define  DMA_MAXSIZE         0xFFFE

//-----------------------------------------------------------------------------
/* Link function for LCD peripheral */
void     LCD_Delay (uint32_t delay);
void     LCD_IO_Init(void);
void     LCD_IO_Bl_OnOff(uint8_t Bl);

void     LCD_IO_WriteCmd8(uint8_t Cmd);
void     LCD_IO_WriteCmd16(uint16_t Cmd);
void     LCD_IO_WriteData8(uint8_t Data);
void     LCD_IO_WriteData16(uint16_t Data);

void     LCD_IO_WriteCmd8DataFill16(uint8_t Cmd, uint16_t Data, uint32_t Size);
void     LCD_IO_WriteCmd8MultipleData8(uint8_t Cmd, uint8_t *pData, uint32_t Size);
void     LCD_IO_WriteCmd8MultipleData16(uint8_t Cmd, uint16_t *pData, uint32_t Size);
void     LCD_IO_WriteCmd16DataFill16(uint16_t Cmd, uint16_t Data, uint32_t Size);
void     LCD_IO_WriteCmd16MultipleData8(uint16_t Cmd, uint8_t *pData, uint32_t Size);
void     LCD_IO_WriteCmd16MultipleData16(uint16_t Cmd, uint16_t *pData, uint32_t Size);

void     LCD_IO_ReadCmd8MultipleData8(uint8_t Cmd, uint8_t *pData, uint32_t Size, uint32_t DummySize);
void     LCD_IO_ReadCmd8MultipleData16(uint8_t Cmd, uint16_t *pData, uint32_t Size, uint32_t DummySize);
void     LCD_IO_ReadCmd8MultipleData24to16(uint8_t Cmd, uint16_t *pData, uint32_t Size, uint32_t DummySize);
void     LCD_IO_ReadCmd16MultipleData8(uint16_t Cmd, uint8_t *pData, uint32_t Size, uint32_t DummySize);
void     LCD_IO_ReadCmd16MultipleData16(uint16_t Cmd, uint16_t *pData, uint32_t Size, uint32_t DummySize);
void     LCD_IO_ReadCmd16MultipleData24to16(uint16_t Cmd, uint16_t *pData, uint32_t Size, uint32_t DummySize);

//-----------------------------------------------------------------------------
#define BITBAND_ACCESS(a, b)  *(volatile uint32_t*)(((uint32_t)&a & 0xF0000000) + 0x2000000 + (((uint32_t)&a & 0x000FFFFF) << 5) + (b << 2))

/* GPIO mode */
#define MODE_ANALOG_INPUT     0x0
#define MODE_PP_OUT_10MHZ     0x1
#define MODE_PP_OUT_2MHZ      0x2
#define MODE_PP_OUT_50MHZ     0x3
#define MODE_FF_DIGITAL_INPUT 0x4
#define MODE_OD_OUT_10MHZ     0x5
#define MODE_OD_OUT_2MHZ      0x6
#define MODE_OD_OUT_50MHZ     0x7
#define MODE_PU_DIGITAL_INPUT 0x8
#define MODE_PP_ALTER_10MHZ   0x9
#define MODE_PP_ALTER_2MHZ    0xA
#define MODE_PP_ALTER_50MHZ   0xB
#define MODE_RESERVED         0xC
#define MODE_OD_ALTER_10MHZ   0xD
#define MODE_OD_ALTER_2MHZ    0xE
#define MODE_OD_ALTER_50MHZ   0xF

#define GPIOX_PORT_(a, b)     GPIO ## a
#define GPIOX_PORT(a)         GPIOX_PORT_(a)

#define GPIOX_PIN_(a, b)      b
#define GPIOX_PIN(a)          GPIOX_PIN_(a)

#define GPIOX_MODE_(a,b,c)    ((GPIO_TypeDef*)(((c & 8) >> 1) + GPIO ## b ## _BASE))->CRL = (((GPIO_TypeDef*)(((c & 8) >> 1) + GPIO ## b ## _BASE))->CRL & ~(0xF << ((c & 7) << 2))) | (a << ((c & 7) << 2))
#define GPIOX_MODE(a, b)      GPIOX_MODE_(a, b)

#define GPIOX_ODR_(a, b)      BITBAND_ACCESS(GPIO ## a ->ODR, b)
#define GPIOX_ODR(a)          GPIOX_ODR_(a)

#define GPIOX_IDR_(a, b)      BITBAND_ACCESS(GPIO ## a ->IDR, b)
#define GPIOX_IDR(a)          GPIOX_IDR_(a)

#define GPIOX_LINE_(a, b)     EXTI_Line ## b
#define GPIOX_LINE(a)         GPIOX_LINE_(a)

#define GPIOX_CLOCK_(a, b)    RCC_APB2ENR_IOP ## a ## EN
#define GPIOX_CLOCK(a)        GPIOX_CLOCK_(a)

#define GPIOX_PORTNUM_A       1
#define GPIOX_PORTNUM_B       2
#define GPIOX_PORTNUM_C       3
#define GPIOX_PORTNUM_D       4
#define GPIOX_PORTNUM_E       5
#define GPIOX_PORTNUM_F       6
#define GPIOX_PORTNUM_G       7
#define GPIOX_PORTNUM_H       8
#define GPIOX_PORTNUM_I       9
#define GPIOX_PORTNUM_J       10
#define GPIOX_PORTNUM_K       11
#define GPIOX_PORTNUM_(a, b)  GPIOX_PORTNUM_ ## a
#define GPIOX_PORTNUM(a)      GPIOX_PORTNUM_(a)

#define GPIOX_PORTNAME_(a, b) a
#define GPIOX_PORTNAME(a)     GPIOX_PORTNAME_(a)

//-----------------------------------------------------------------------------
/* DMA */
#define DMANUM_(a, b, c)                a
#define DMANUM(a)                       DMANUM_(a)

#define DMACHN_(a, b, c)                b
#define DMACHN(a)                       DMACHN_(a)

#define DMAPRIORITY_(a, b, c)           c
#define DMAPRIORITY(a)                  DMAPRIORITY_(a)

#define DMAX_(a, b, c)                  DMA ## a
#define DMAX(a)                         DMAX_(a)

#define DMAX_CHANNEL_(a, b, c)          DMA ## a ## _Channel ## b
#define DMAX_CHANNEL(a)                 DMAX_CHANNEL_(a)

#define DMAX_CHANNEL_IRQ_(a, b, c)      DMA ## a ## _Channel ## b ## _IRQn
#define DMAX_CHANNEL_IRQ(a)             DMAX_CHANNEL_IRQ_(a)

#define DMAX_CHANNEL_IRQHANDLER_(a, b, c) DMA ## a ## _Channel ## b ## _IRQHandler
#define DMAX_CHANNEL_IRQHANDLER(a)      DMAX_CHANNEL_IRQHANDLER_(a)

// Interrupt event pl: if(DMAX_ISR(LCD_DMA_TX) & DMAX_ISR_TCIF(LCD_DMA_TX))...
#define DMAX_ISR_TCIF_(a, b, c)         DMA_ISR_TCIF ## b
#define DMAX_ISR_TCIF(a)                DMAX_ISR_TCIF_(a)

#define DMAX_ISR_HTIF_(a, b, c)         DMA_ISR_HTIF ## b
#define DMAX_ISR_HTIF(a)                DMAX_ISR_HTIF_(a)

#define DMAX_ISR_TEIF_(a, b, c)         DMA_ISR_TEIF ## b
#define DMAX_ISR_TEIF(a)                DMAX_ISR_TEIF_(a)

#define DMAX_ISR_GIF_(a, b, c)          DMA_ISR_GIF ## b
#define DMAX_ISR_GIF(a)                 DMAX_ISR_GIF_(a)

// Interrupt clear pl: DMAX_IFCR(LCD_DMA_TX) = DMAX_IFCR_CTCIF(LCD_DMA_TX) | DMAX_IFCR_CFEIF(LCD_DMA_TX);
#define DMAX_IFCR_CTCIF_(a, b, c)       DMA_IFCR_CTCIF ## b
#define DMAX_IFCR_CTCIF(a)              DMAX_IFCR_CTCIF_(a)

#define DMAX_IFCR_CHTIF_(a, b, c)       DMA_IFCR_CHTIF ## b
#define DMAX_IFCR_CHTIF(a)              DMAX_IFCR_CHTIF_(a)

#define DMAX_IFCR_CTEIF_(a, b, c)       DMA_IFCR_CTEIF ## b
#define DMAX_IFCR_CTEIF(a)              DMAX_IFCR_CTEIF_(a)

#define DMAX_IFCR_CGIF_(a, b, c)        DMA_IFCR_CGIF ## b
#define DMAX_IFCR_CGIF(a)               DMAX_IFCR_CGIF_(a)

//-----------------------------------------------------------------------------
/* freertos vs HAL */
#ifdef  osCMSIS
#define DelayMs(t)            osDelay(t)
#define GetTime()             osKernelSysTick()
#else
#define DelayMs(t)            HAL_Delay(t)
#define GetTime()             HAL_GetTick()
#endif

//-----------------------------------------------------------------------------
#if DMANUM(LCD_DMA) > 0
/* DMA transfer end check */

/* DMA status
   - 0: all DMA transfers are completed
   - 1: last DMA transfer in progress
   - 2: DMA transfer in progress */
volatile uint32_t LCD_IO_DmaTransferStatus = 0;

//-----------------------------------------------------------------------------
/* Waiting for all DMA processes to complete */
#ifndef osFeature_Semaphore
/* no FreeRtos */

extern inline void WaitForDmaEnd(void);
inline void WaitForDmaEnd(void)
{
  while(LCD_IO_DmaTransferStatus);
}

//-----------------------------------------------------------------------------
#else   /* osFeature_Semaphore */
/* FreeRtos */
osSemaphoreId spiDmaBinSemHandle;

extern inline void WaitForDmaEnd(void);
inline void WaitForDmaEnd(void)
{
  if(LCD_IO_DmaTransferStatus)
  {
    osSemaphoreWait(spiDmaBinSemHandle, 500);
    if(LCD_IO_DmaTransferStatus == 1)
      LCD_IO_DmaTransferStatus = 0;
  }
}
#endif  /* #else osFeature_Semaphore */

//-----------------------------------------------------------------------------
/* DMA interrupt handler */
void DMAX_CHANNEL_IRQHANDLER(LCD_DMA)(void)
{
  if(DMAX(LCD_DMA)->ISR & DMAX_ISR_TCIF(LCD_DMA))
  {
    DMAX(LCD_DMA)->IFCR = DMAX_IFCR_CTCIF(LCD_DMA);
    DMAX_CHANNEL(LCD_DMA)->CCR = 0;
    while(DMAX_CHANNEL(LCD_DMA)->CCR & DMA_CCR_EN);
    #ifndef osFeature_Semaphore
    /* no FreeRtos */
    LCD_IO_DmaTransferStatus = 0;
    #else
    /* FreeRtos */
    osSemaphoreRelease(spiDmaBinSemHandle);
    #endif /* #else osFeature_Semaphore */
  }
  else
    DMAX(LCD_DMA)->IFCR = DMAX_IFCR_CGIF(LCD_DMA);
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteMultiData(void * pData, uint32_t Size, uint32_t dmacr)
{
  DMAX(LCD_DMA)->IFCR = DMAX_IFCR_CGIF(LCD_DMA);
  DMAX_CHANNEL(LCD_DMA)->CCR = 0;       /* DMA stop */
  DMAX_CHANNEL(LCD_DMA)->CMAR = LCD_ADDR_DATA;
  DMAX_CHANNEL(LCD_DMA)->CPAR = (uint32_t)pData;
  DMAX_CHANNEL(LCD_DMA)->CNDTR = Size;
  DMAX_CHANNEL(LCD_DMA)->CCR = dmacr;
  DMAX_CHANNEL(LCD_DMA)->CCR |= DMA_CCR_EN;
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteMultiData8(uint8_t * pData, uint32_t Size, uint32_t dinc)
{
  uint32_t dmacr;
  #if LCD_DMA_TXWAIT != 2
  static uint8_t d8s;
  #endif
  if(!dinc)
  {
    #if LCD_DMA_TXWAIT != 2
    d8s = *pData;
    pData = &d8s;
    #endif
    dmacr = DMA_CCR_TCIE | (1 << DMA_CCR_MEM2MEM_Pos) |
            (0 << DMA_CCR_MSIZE_Pos) | (0 << DMA_CCR_PSIZE_Pos) |
            (0 << DMA_CCR_PINC_Pos) | (0 << DMA_CCR_DIR_Pos) |
            (DMAPRIORITY(LCD_DMA) << DMA_CCR_PL_Pos);
  }
  else
    dmacr = DMA_CCR_TCIE | (1 << DMA_CCR_MEM2MEM_Pos) |
            (0 << DMA_CCR_MSIZE_Pos) | (0 << DMA_CCR_PSIZE_Pos) |
            (1 << DMA_CCR_PINC_Pos) | (0 << DMA_CCR_DIR_Pos) |
            (DMAPRIORITY(LCD_DMA) << DMA_CCR_PL_Pos);

  #ifdef LCD_DMA_UNABLE
  if(LCD_DMA_UNABLE((uint32_t)(pData)))
  {
    while(Size--)
    {
      *(volatile uint16_t *)LCD_ADDR_DATA = (uint16_t)*pData;
      if(dinc)
        pData++;
    }
  }
  else
  #endif
  {
    while(Size)
    {
      if(Size <= DMA_MAXSIZE)
      {
        LCD_IO_DmaTransferStatus = 1;     /* last transfer */
        LCD_IO_WriteMultiData((void *)pData, Size, dmacr);
        Size = 0;
        #if LCD_DMA_TXWAIT == 1
        if(dinc)
          WaitForDmaEnd();
        #endif
      }
      else
      {
        LCD_IO_DmaTransferStatus = 2;     /* no last transfer */
        LCD_IO_WriteMultiData((void *)pData, DMA_MAXSIZE, dmacr);
        if(dinc)
          pData+= DMA_MAXSIZE;
        Size-= DMA_MAXSIZE;
        #if LCD_DMA_TXWAIT != 2
        WaitForDmaEnd();
        #endif
      }
      #if LCD_DMA_TXWAIT == 2
      WaitForDmaEnd();
      #endif
    }
  }
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteMultiData16(uint16_t * pData, uint32_t Size, uint32_t dinc)
{
  uint32_t dmacr;
  #if LCD_DMA_TXWAIT != 2
  static uint16_t d16s;
  #endif
  if(!dinc)
  {
    #if LCD_DMA_TXWAIT != 2
    d16s = *pData;
    pData = &d16s;
    #endif
    dmacr = DMA_CCR_TCIE | (1 << DMA_CCR_MEM2MEM_Pos) |
            (1 << DMA_CCR_MSIZE_Pos) | (1 << DMA_CCR_PSIZE_Pos) |
            (0 << DMA_CCR_PINC_Pos) | (0 << DMA_CCR_DIR_Pos) |
            (DMAPRIORITY(LCD_DMA) << DMA_CCR_PL_Pos);
  }
  else
    dmacr = DMA_CCR_TCIE | (1 << DMA_CCR_MEM2MEM_Pos) |
            (1 << DMA_CCR_MSIZE_Pos) | (1 << DMA_CCR_PSIZE_Pos) |
            (1 << DMA_CCR_PINC_Pos) | (0 << DMA_CCR_DIR_Pos) |
            (DMAPRIORITY(LCD_DMA) << DMA_CCR_PL_Pos);

  #ifdef LCD_DMA_UNABLE
  if(LCD_DMA_UNABLE((uint32_t)(pData)))
  {
    while(Size--)
    {
      *(volatile uint16_t *)LCD_ADDR_DATA = *pData;
      if(dinc)
        pData++;
    }
  }
  else
  #endif
  {
    while(Size)
    {
      if(Size <= DMA_MAXSIZE)
      {
        LCD_IO_DmaTransferStatus = 1;     /* last transfer */
        LCD_IO_WriteMultiData((void *)pData, Size, dmacr);
        Size = 0;
        #if LCD_DMA_TXWAIT == 1
        if(dinc)
          WaitForDmaEnd();
        #endif
      }
      else if(Size < 2 * DMA_MAXSIZE)
      {
        LCD_IO_DmaTransferStatus = 2;     /* no last transfer */
        LCD_IO_WriteMultiData((void *)pData, Size - DMA_MAXSIZE, dmacr);
        if(dinc)
          pData+= Size - DMA_MAXSIZE;
        Size = DMA_MAXSIZE;
        #if LCD_DMA_TXWAIT != 2
        WaitForDmaEnd();
        #endif
      }
      else
      {
        LCD_IO_DmaTransferStatus = 2;     /* no last transfer */
        LCD_IO_WriteMultiData((void *)pData, DMA_MAXSIZE, dmacr);
        if(dinc)
          pData+= DMA_MAXSIZE;
        Size-= DMA_MAXSIZE;
        #if LCD_DMA_TXWAIT != 2
        WaitForDmaEnd();
        #endif
      }
      #if LCD_DMA_TXWAIT == 2
      WaitForDmaEnd();
      #endif
    }
  }
}

//-----------------------------------------------------------------------------
void LCD_IO_ReadMultiData(void * pData, uint32_t Size, uint32_t dmacr)
{
  DMAX(LCD_DMA)->IFCR = DMAX_IFCR_CGIF(LCD_DMA);
  DMAX_CHANNEL(LCD_DMA)->CCR = 0;       /* DMA stop */
  DMAX_CHANNEL(LCD_DMA)->CMAR = (uint32_t)pData;
  DMAX_CHANNEL(LCD_DMA)->CPAR = LCD_ADDR_DATA;
  DMAX_CHANNEL(LCD_DMA)->CNDTR = Size;
  DMAX_CHANNEL(LCD_DMA)->CCR = dmacr;
  DMAX_CHANNEL(LCD_DMA)->CCR |= DMA_CCR_EN;
}

//-----------------------------------------------------------------------------
void LCD_IO_ReadMultiData8(uint8_t * pData, uint32_t Size)
{
  uint32_t dmacr;
  dmacr = DMA_CCR_TCIE | (1 << DMA_CCR_MEM2MEM_Pos) |
          (0 << DMA_CCR_MSIZE_Pos) | (0 << DMA_CCR_PSIZE_Pos) |
          (1 << DMA_CCR_MINC_Pos) | (0 << DMA_CCR_DIR_Pos) |
          (DMAPRIORITY(LCD_DMA) << DMA_CCR_PL_Pos);
  #ifdef LCD_DMA_UNABLE
  if(LCD_DMA_UNABLE((uint32_t)(pData)))
  {
    while(Size--)
    {
      *pData = (uint8_t)(*(volatile uint16_t *)LCD_ADDR_DATA);
      pData++;
    }
  }
  else
  #endif
  {
    while(Size)
    {
      if(Size > DMA_MAXSIZE)
      {
        LCD_IO_DmaTransferStatus = 2;     /* no last transfer */
        LCD_IO_ReadMultiData((void *)pData, DMA_MAXSIZE, dmacr);
        Size-= DMA_MAXSIZE;
        pData+= DMA_MAXSIZE;
      }
      else
      {
        LCD_IO_DmaTransferStatus = 1;     /* last transfer */
        LCD_IO_ReadMultiData((void *)pData, Size, dmacr);
        Size = 0;
      }
      WaitForDmaEnd();
    }
  }
}

//-----------------------------------------------------------------------------
void LCD_IO_ReadMultiData16(uint16_t * pData, uint32_t Size)
{
  uint32_t dmacr;
  dmacr = DMA_CCR_TCIE | (1 << DMA_CCR_MEM2MEM_Pos) |
          (1 << DMA_CCR_MSIZE_Pos) | (1 << DMA_CCR_PSIZE_Pos) |
          (1 << DMA_CCR_MINC_Pos) | (0 << DMA_CCR_DIR_Pos) |
          (DMAPRIORITY(LCD_DMA) << DMA_CCR_PL_Pos);
  #ifdef LCD_DMA_UNABLE
  if(LCD_DMA_UNABLE((uint32_t)(pData)))
  {
    while(Size--)
    {
      *pData = *(volatile uint16_t *)LCD_ADDR_DATA;
      pData++;
    }
  }
  else
  #endif
  {
    while(Size)
    {
      if(Size > DMA_MAXSIZE)
      {
        LCD_IO_DmaTransferStatus = 2;     /* no last transfer */
        LCD_IO_ReadMultiData((void *)pData, DMA_MAXSIZE, dmacr);
        Size-= DMA_MAXSIZE;
        pData+= DMA_MAXSIZE;
      }
      else
      {
        LCD_IO_DmaTransferStatus = 1;     /* last transfer */
        LCD_IO_ReadMultiData((void *)pData, Size, dmacr);
        Size = 0;
      }
      WaitForDmaEnd();
    }
  }
}

#else   /* #if DMANUM(LCD_DMA) > 0 */

#define  WaitForDmaEnd()                /* if DMA is not used -> no need to wait */

#endif  /* #else DMANUM(LCD_DMA) > 0 */

//-----------------------------------------------------------------------------
/* reset pin setting */
#define LCD_RST_ON            GPIOX_ODR(LCD_RST) = 0
#define LCD_RST_OFF           GPIOX_ODR(LCD_RST) = 1

//-----------------------------------------------------------------------------
void LCD_Delay(uint32_t Delay)
{
  DelayMs(Delay);
}

//-----------------------------------------------------------------------------
void LCD_IO_Bl_OnOff(uint8_t Bl)
{
  #if GPIOX_PORTNUM(LCD_BL) >= GPIOX_PORTNUM_A
  if(Bl)
    GPIOX_ODR(LCD_BL) = LCD_BLON;
  else
    GPIOX_ODR(LCD_BL) = 1 - LCD_BLON;
  #endif
}

//-----------------------------------------------------------------------------
void LCD_IO_Init(void)
{
  /* Reset pin clock */
  #if GPIOX_PORTNUM(LCD_RST) >= GPIOX_PORTNUM_A
  #define GPIOX_CLOCK_LCD_RST   GPIOX_CLOCK(LCD_RST)
  #else
  #define GPIOX_CLOCK_LCD_RST   0
  #endif

  /* Backlight pin clock */
  #if GPIOX_PORTNUM(LCD_BL) >=  GPIOX_PORTNUM_A
  #define GPIOX_CLOCK_LCD_BL    GPIOX_CLOCK(LCD_BL)
  #else
  #define GPIOX_CLOCK_LCD_BL    0
  #endif

  /* DMA clock */
  #if DMANUM(LCD_DMA) == 1
  #define DMA_CLOCK             RCC_AHBENR_DMA1EN
  #elif DMANUM(LCD_DMA) == 2
  #define DMA_CLOCK             RCC_AHBENR_DMA2EN
  #else
  #define DMA_CLOCK             0
  #endif

  /* GPIO, DMA Clocks */
  #if GPIOX_PORTNUM(LCD_RST) >= GPIOX_PORTNUM_A || GPIOX_PORTNUM(LCD_BL) >= GPIOX_PORTNUM_A || DMANUM(LCD_DMA) > 0
  RCC->APB2ENR |= GPIOX_CLOCK_LCD_RST | GPIOX_CLOCK_LCD_BL;
  #endif
  #if DMANUM(LCD_DMA) >= 1
  RCC->AHBENR |= DMA_CLOCK;
  #endif

  #if GPIOX_PORTNUM(LCD_RST) >= GPIOX_PORTNUM_A
  GPIOX_MODE(MODE_PP_OUT_2MHZ, LCD_RST);/* RST = GPIO OUT */
  GPIOX_ODR(LCD_RST) = 1;               /* RST = 1 */
  #endif

  #if GPIOX_PORTNUM(LCD_BL) >= GPIOX_PORTNUM_A
  GPIOX_ODR(LCD_BL) = LCD_BLON;
  GPIOX_MODE(MODE_PP_OUT_2MHZ, LCD_BL);
  #endif

  /* Reset the LCD */
  #if GPIOX_PORTNUM(LCD_RST) >= GPIOX_PORTNUM_A
  LCD_Delay(1);
  LCD_RST_ON;
  LCD_Delay(1);
  LCD_RST_OFF;
  #endif
  LCD_Delay(1);

  #if DMANUM(LCD_DMA) > 0
  NVIC_SetPriority(DMAX_CHANNEL_IRQ(LCD_DMA), LCD_DMA_IRQ_PR);
  NVIC_EnableIRQ(DMAX_CHANNEL_IRQ(LCD_DMA));
  #ifdef osFeature_Semaphore
  osSemaphoreDef(spiDmaBinSem);
  spiDmaBinSemHandle = osSemaphoreCreate(osSemaphore(spiDmaBinSem), 1);
  osSemaphoreWait(spiDmaBinSemHandle, 1);
  #endif
  #endif  /* #if DMANUM(LCD_DMA) > 0 */
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteCmd8(uint8_t Cmd)
{
  WaitForDmaEnd();
  *(volatile uint16_t *)LCD_ADDR_BASE = (uint16_t)Cmd;
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteCmd16(uint16_t Cmd)
{
  WaitForDmaEnd();
  *(volatile uint16_t *)LCD_ADDR_BASE = Cmd;
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteData8(uint8_t Data)
{
  WaitForDmaEnd();
  *(volatile uint16_t *)LCD_ADDR_DATA = (uint16_t)Data;
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteData16(uint16_t Data)
{
  WaitForDmaEnd();
  *(volatile uint16_t *)LCD_ADDR_DATA = Data;
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteCmd8DataFill16(uint8_t Cmd, uint16_t Data, uint32_t Size)
{
  WaitForDmaEnd();
  *(volatile uint16_t *)LCD_ADDR_BASE = (uint16_t)Cmd;

  #if DMANUM(LCD_DMA) == 0
  while(Size--)
    *(volatile uint16_t *)LCD_ADDR_DATA = Data;

  #else
  LCD_IO_WriteMultiData16(&Data, Size, 0);
  #endif
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteCmd8MultipleData8(uint8_t Cmd, uint8_t *pData, uint32_t Size)
{
  WaitForDmaEnd();
  *(volatile uint16_t *)LCD_ADDR_BASE = (uint16_t)Cmd;

  #if DMANUM(LCD_DMA) == 0
  while(Size--)
  {
    *(volatile uint16_t *)LCD_ADDR_DATA = (uint16_t)*pData;
    pData++;
  }

  #else
  LCD_IO_WriteMultiData8(pData, Size, 1);
  #endif
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteCmd8MultipleData16(uint8_t Cmd, uint16_t *pData, uint32_t Size)
{
  WaitForDmaEnd();
  *(volatile uint16_t *)LCD_ADDR_BASE = (uint16_t)Cmd;

  #if DMANUM(LCD_DMA) == 0
  while(Size--)
  {
    *(volatile uint16_t *)LCD_ADDR_DATA = *pData;
    pData++;
  }

  #else
  LCD_IO_WriteMultiData16(pData, Size, 1);
  #endif
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteCmd16DataFill16(uint16_t Cmd, uint16_t Data, uint32_t Size)
{
  WaitForDmaEnd();
  *(volatile uint16_t *)LCD_ADDR_BASE = Cmd;

  #if DMANUM(LCD_DMA) == 0
  while(Size--)
    *(volatile uint16_t *)LCD_ADDR_DATA = Data;

  #else
  LCD_IO_WriteMultiData16(&Data, Size, 0);
  #endif
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteCmd16MultipleData8(uint16_t Cmd, uint8_t *pData, uint32_t Size)
{
  WaitForDmaEnd();
  *(volatile uint16_t *)LCD_ADDR_BASE = Cmd;

  #if DMANUM(LCD_DMA) == 0
  while(Size--)
  {
    *(volatile uint16_t *)LCD_ADDR_DATA = (uint16_t)*pData;
    pData++;
  }

  #else
  LCD_IO_WriteMultiData8(pData, Size, 1);
  #endif
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteCmd16MultipleData16(uint16_t Cmd, uint16_t *pData, uint32_t Size)
{
  WaitForDmaEnd();
  *(volatile uint16_t *)LCD_ADDR_BASE = Cmd;

  #if DMANUM(LCD_DMA) == 0
  while(Size--)
  {
    *(volatile uint16_t *)LCD_ADDR_DATA = *pData;
    pData++;
  }

  #else
  LCD_IO_WriteMultiData16(pData, Size, 1);
  #endif
}

//-----------------------------------------------------------------------------
void LCD_IO_ReadCmd8MultipleData8(uint8_t Cmd, uint8_t *pData, uint32_t Size, uint32_t DummySize)
{
  #ifdef  __GNUC__
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wunused-but-set-variable"
  uint16_t DummyData;
  #pragma GCC diagnostic pop
  #elif   defined(__CC_ARM)
  uint16_t DummyData;
  #endif

  WaitForDmaEnd();
  *(volatile uint16_t *)LCD_ADDR_BASE = (uint16_t)Cmd;

  while(DummySize--)
    DummyData = *(volatile uint16_t *)LCD_ADDR_DATA;

  #if DMANUM(LCD_DMA) == 0
  while(Size--)
  {
    *pData = (uint8_t)(*(volatile uint16_t *)LCD_ADDR_DATA);
    pData++;
  }

  #else
  LCD_IO_ReadMultiData8(pData, Size);
  #endif
}

//-----------------------------------------------------------------------------
void LCD_IO_ReadCmd8MultipleData16(uint8_t Cmd, uint16_t *pData, uint32_t Size, uint32_t DummySize)
{
  #ifdef  __GNUC__
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wunused-but-set-variable"
  uint16_t DummyData;
  #pragma GCC diagnostic pop
  #elif   defined(__CC_ARM)
  uint16_t DummyData;
  #endif

  WaitForDmaEnd();
  *(volatile uint16_t *)LCD_ADDR_BASE = (uint16_t)Cmd;

  while(DummySize--)
    DummyData = *(volatile uint16_t *)LCD_ADDR_DATA;

  #if DMANUM(LCD_DMA) == 0
  while(Size--)
  {
    *pData = *(volatile uint16_t *)LCD_ADDR_DATA;
    pData++;
  }

  #else
  LCD_IO_ReadMultiData16(pData, Size);
  #endif
}

//-----------------------------------------------------------------------------
void LCD_IO_ReadCmd8MultipleData24to16(uint8_t Cmd, uint16_t *pData, uint32_t Size, uint32_t DummySize)
{
  union packed
  {
    uint8_t  rgb888[6];
    uint16_t rgb888_16[3];
  }u;

  #ifdef  __GNUC__
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wunused-but-set-variable"
  uint16_t DummyData;
  #pragma GCC diagnostic pop
  #elif   defined(__CC_ARM)
  uint16_t DummyData;
  #endif

  WaitForDmaEnd();
  *(volatile uint16_t *)LCD_ADDR_BASE = (uint16_t)Cmd;

  while(DummySize--)
    DummyData = *(volatile uint16_t *)LCD_ADDR_DATA;

  while(Size--)
  {
    u.rgb888_16[0] = *(volatile uint16_t*)LCD_ADDR_DATA;
    u.rgb888_16[1] = *(volatile uint16_t*)LCD_ADDR_DATA;
    u.rgb888_16[2] = *(volatile uint16_t*)LCD_ADDR_DATA;

    *pData = ((u.rgb888[1] & 0xF8) << 8 | (u.rgb888[0] & 0xFC) << 3 | u.rgb888[3] >> 3);
    pData++;
    if(Size)
    {
      *pData = ((u.rgb888[2] & 0xF8) << 8 | (u.rgb888[5] & 0xFC) << 3 | u.rgb888[4] >> 3);
      pData++;
      Size--;
    }
  }
}

//-----------------------------------------------------------------------------
void LCD_IO_ReadCmd16MultipleData8(uint16_t Cmd, uint8_t *pData, uint32_t Size, uint32_t DummySize)
{
  #ifdef  __GNUC__
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wunused-but-set-variable"
  uint16_t DummyData;
  #pragma GCC diagnostic pop
  #elif   defined(__CC_ARM)
  uint16_t DummyData;
  #endif

  WaitForDmaEnd();
  *(volatile uint16_t *)LCD_ADDR_BASE = Cmd;

  while(DummySize--)
    DummyData = *(volatile uint16_t *)LCD_ADDR_DATA;

  #if DMANUM(LCD_DMA) == 0
  while(Size--)
  {
    *pData = (uint8_t)(*(volatile uint16_t *)LCD_ADDR_DATA);
    pData++;
  }

  #else
  LCD_IO_ReadMultiData8(pData, Size);
  #endif
}

//-----------------------------------------------------------------------------
void LCD_IO_ReadCmd16MultipleData16(uint16_t Cmd, uint16_t *pData, uint32_t Size, uint32_t DummySize)
{
  #ifdef  __GNUC__
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wunused-but-set-variable"
  uint16_t DummyData;
  #pragma GCC diagnostic pop
  #elif   defined(__CC_ARM)
  uint16_t DummyData;
  #endif

  WaitForDmaEnd();
  *(volatile uint16_t *)LCD_ADDR_BASE = Cmd;

  while(DummySize--)
    DummyData = *(volatile uint16_t *)LCD_ADDR_DATA;

  #if DMANUM(LCD_DMA) == 0
  while(Size--)
  {
    *pData = *(volatile uint16_t *)LCD_ADDR_DATA;
    pData++;
  }

  #else
  LCD_IO_ReadMultiData16(pData, Size);
  #endif
}

//-----------------------------------------------------------------------------
void LCD_IO_ReadCmd16MultipleData24to16(uint16_t Cmd, uint16_t *pData, uint32_t Size, uint32_t DummySize)
{
  union packed
  {
    uint8_t  rgb888[6];
    uint16_t rgb888_16[3];
  }u;

  #ifdef  __GNUC__
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wunused-but-set-variable"
  uint16_t DummyData;
  #pragma GCC diagnostic pop
  #elif   defined(__CC_ARM)
  uint16_t DummyData;
  #endif

  WaitForDmaEnd();
  *(volatile uint16_t *)LCD_ADDR_BASE = Cmd;

  while(DummySize--)
    DummyData = *(volatile uint16_t *)LCD_ADDR_DATA;

  while(Size--)
  {
    u.rgb888_16[0] = *(volatile uint16_t*)LCD_ADDR_DATA;
    u.rgb888_16[1] = *(volatile uint16_t*)LCD_ADDR_DATA;
    u.rgb888_16[2] = *(volatile uint16_t*)LCD_ADDR_DATA;

    *pData = ((u.rgb888[1] & 0xF8) << 8 | (u.rgb888[0] & 0xFC) << 3 | u.rgb888[3] >> 3);
    pData++;
    if(Size)
    {
      *pData = ((u.rgb888[2] & 0xF8) << 8 | (u.rgb888[5] & 0xFC) << 3 | u.rgb888[4] >> 3);
      pData++;
      Size--;
    }
  }
}
