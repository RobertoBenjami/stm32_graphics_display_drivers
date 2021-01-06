/*
 * 16 bit paralell LCD GPIO driver for STM32F1
 * 5 controll pins (CS, RS, WR, RD, RST) + 16 data pins + 1 backlight pin
 */

/* Author: Roberto Benjami
   version:  2020.05.27
*/
#include "main.h"
#include "lcd.h"
#include "lcd_io_gpio16.h"

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

/* pin modes (PP: push-pull, OD: open drain, FF: input floating) */
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
/* Command/data pin setting */
#define LCD_RS_CMD            GPIOX_ODR(LCD_RS) = 0
#define LCD_RS_DATA           GPIOX_ODR(LCD_RS) = 1

/* Reset pin on/off */
#define LCD_RST_ON            GPIOX_ODR(LCD_RST) = 0
#define LCD_RST_OFF           GPIOX_ODR(LCD_RST) = 1

#define LCD_CS_ON             GPIOX_ODR(LCD_CS) = 0
#define LCD_CS_OFF            GPIOX_ODR(LCD_CS) = 1

//-----------------------------------------------------------------------------
/* if the 16 datapins are in order -> automatic optimalization */
#if ((GPIOX_PORTNUM(LCD_D0) == GPIOX_PORTNUM(LCD_D1))\
  && (GPIOX_PORTNUM(LCD_D1) == GPIOX_PORTNUM(LCD_D2))\
  && (GPIOX_PORTNUM(LCD_D2) == GPIOX_PORTNUM(LCD_D3))\
  && (GPIOX_PORTNUM(LCD_D3) == GPIOX_PORTNUM(LCD_D4))\
  && (GPIOX_PORTNUM(LCD_D4) == GPIOX_PORTNUM(LCD_D5))\
  && (GPIOX_PORTNUM(LCD_D5) == GPIOX_PORTNUM(LCD_D6))\
  && (GPIOX_PORTNUM(LCD_D6) == GPIOX_PORTNUM(LCD_D7))\
  && (GPIOX_PORTNUM(LCD_D7) == GPIOX_PORTNUM(LCD_D8))\
  && (GPIOX_PORTNUM(LCD_D8) == GPIOX_PORTNUM(LCD_D9))\
  && (GPIOX_PORTNUM(LCD_D9) == GPIOX_PORTNUM(LCD_D10))\
  && (GPIOX_PORTNUM(LCD_D10) == GPIOX_PORTNUM(LCD_D11))\
  && (GPIOX_PORTNUM(LCD_D11) == GPIOX_PORTNUM(LCD_D12))\
  && (GPIOX_PORTNUM(LCD_D12) == GPIOX_PORTNUM(LCD_D13))\
  && (GPIOX_PORTNUM(LCD_D13) == GPIOX_PORTNUM(LCD_D14))\
  && (GPIOX_PORTNUM(LCD_D14) == GPIOX_PORTNUM(LCD_D15)))
#if ((GPIOX_PIN(LCD_D0) + 1 == GPIOX_PIN(LCD_D1))\
  && (GPIOX_PIN(LCD_D1) + 1 == GPIOX_PIN(LCD_D2))\
  && (GPIOX_PIN(LCD_D2) + 1 == GPIOX_PIN(LCD_D3))\
  && (GPIOX_PIN(LCD_D3) + 1 == GPIOX_PIN(LCD_D4))\
  && (GPIOX_PIN(LCD_D4) + 1 == GPIOX_PIN(LCD_D5))\
  && (GPIOX_PIN(LCD_D5) + 1 == GPIOX_PIN(LCD_D6))\
  && (GPIOX_PIN(LCD_D6) + 1 == GPIOX_PIN(LCD_D7))\
  && (GPIOX_PIN(LCD_D7) + 1 == GPIOX_PIN(LCD_D8))\
  && (GPIOX_PIN(LCD_D8) + 1 == GPIOX_PIN(LCD_D9))\
  && (GPIOX_PIN(LCD_D9) + 1 == GPIOX_PIN(LCD_D10))\
  && (GPIOX_PIN(LCD_D10) + 1 == GPIOX_PIN(LCD_D11))\
  && (GPIOX_PIN(LCD_D11) + 1 == GPIOX_PIN(LCD_D12))\
  && (GPIOX_PIN(LCD_D12) + 1 == GPIOX_PIN(LCD_D13))\
  && (GPIOX_PIN(LCD_D13) + 1 == GPIOX_PIN(LCD_D14))\
  && (GPIOX_PIN(LCD_D14) + 1 == GPIOX_PIN(LCD_D15)))
/* LCD_D0..LCD_D15 pins are in order */
#define LCD_AUTOOPT
#endif // D0..D15 pin order
#endif // D0..D15 port equivalence

//-----------------------------------------------------------------------------
/* data pins setting to out */
#ifndef LCD_DIRWRITE
#ifdef  LCD_AUTOOPT
#define LCD_DIRWRITE  {GPIOX_PORT(LCD_D0)->CRL = 0x33333333, GPIOX_PORT(LCD_D0)->CRH = 0x33333333;}
#else
#define LCD_DIRWRITE { \
  GPIOX_MODE(MODE_PP_OUT_50MHZ, LCD_D0); GPIOX_MODE(MODE_PP_OUT_50MHZ, LCD_D1);\
  GPIOX_MODE(MODE_PP_OUT_50MHZ, LCD_D2); GPIOX_MODE(MODE_PP_OUT_50MHZ, LCD_D3);\
  GPIOX_MODE(MODE_PP_OUT_50MHZ, LCD_D4); GPIOX_MODE(MODE_PP_OUT_50MHZ, LCD_D5);\
  GPIOX_MODE(MODE_PP_OUT_50MHZ, LCD_D6); GPIOX_MODE(MODE_PP_OUT_50MHZ, LCD_D7);\
  GPIOX_MODE(MODE_PP_OUT_50MHZ, LCD_D8); GPIOX_MODE(MODE_PP_OUT_50MHZ, LCD_D9);\
  GPIOX_MODE(MODE_PP_OUT_50MHZ, LCD_D10); GPIOX_MODE(MODE_PP_OUT_50MHZ, LCD_D11);\
  GPIOX_MODE(MODE_PP_OUT_50MHZ, LCD_D12); GPIOX_MODE(MODE_PP_OUT_50MHZ, LCD_D13);\
  GPIOX_MODE(MODE_PP_OUT_50MHZ, LCD_D14); GPIOX_MODE(MODE_PP_OUT_50MHZ, LCD_D15);}
#endif
#endif

//-----------------------------------------------------------------------------
/* data pins setting to input */
#ifndef LCD_DIRREAD
#ifdef  LCD_AUTOOPT
#define LCD_DIRREAD  {GPIOX_PORT(LCD_D0)->CRL = 0x44444444; GPIOX_PORT(LCD_D0)->CRH = 0x44444444;}
#else
#define LCD_DIRREAD { \
  GPIOX_MODE(MODE_FF_DIGITAL_INPUT, LCD_D0); GPIOX_MODE(MODE_FF_DIGITAL_INPUT, LCD_D1);\
  GPIOX_MODE(MODE_FF_DIGITAL_INPUT, LCD_D2); GPIOX_MODE(MODE_FF_DIGITAL_INPUT, LCD_D3);\
  GPIOX_MODE(MODE_FF_DIGITAL_INPUT, LCD_D4); GPIOX_MODE(MODE_FF_DIGITAL_INPUT, LCD_D5);\
  GPIOX_MODE(MODE_FF_DIGITAL_INPUT, LCD_D6); GPIOX_MODE(MODE_FF_DIGITAL_INPUT, LCD_D7);\
  GPIOX_MODE(MODE_FF_DIGITAL_INPUT, LCD_D8); GPIOX_MODE(MODE_FF_DIGITAL_INPUT, LCD_D9);\
  GPIOX_MODE(MODE_FF_DIGITAL_INPUT, LCD_D10); GPIOX_MODE(MODE_FF_DIGITAL_INPUT, LCD_D11);\
  GPIOX_MODE(MODE_FF_DIGITAL_INPUT, LCD_D12); GPIOX_MODE(MODE_FF_DIGITAL_INPUT, LCD_D13);\
  GPIOX_MODE(MODE_FF_DIGITAL_INPUT, LCD_D14); GPIOX_MODE(MODE_FF_DIGITAL_INPUT, LCD_D15);}
#endif
#endif

//-----------------------------------------------------------------------------
/* data pins write */
#ifndef LCD_WRITE
#ifdef  LCD_AUTOOPT
#define LCD_WRITE(dt) GPIOX_PORT(LCD_D0)->ODR = dt
#else
#define LCD_WRITE(dt) {;                  \
  GPIOX_ODR(LCD_D0) = BITBAND_ACCESS(dt, 0); \
  GPIOX_ODR(LCD_D1) = BITBAND_ACCESS(dt, 1); \
  GPIOX_ODR(LCD_D2) = BITBAND_ACCESS(dt, 2); \
  GPIOX_ODR(LCD_D3) = BITBAND_ACCESS(dt, 3); \
  GPIOX_ODR(LCD_D4) = BITBAND_ACCESS(dt, 4); \
  GPIOX_ODR(LCD_D5) = BITBAND_ACCESS(dt, 5); \
  GPIOX_ODR(LCD_D6) = BITBAND_ACCESS(dt, 6); \
  GPIOX_ODR(LCD_D7) = BITBAND_ACCESS(dt, 7); \
  GPIOX_ODR(LCD_D8) = BITBAND_ACCESS(dt, 8); \
  GPIOX_ODR(LCD_D9) = BITBAND_ACCESS(dt, 9); \
  GPIOX_ODR(LCD_D10) = BITBAND_ACCESS(dt, 10); \
  GPIOX_ODR(LCD_D11) = BITBAND_ACCESS(dt, 11); \
  GPIOX_ODR(LCD_D12) = BITBAND_ACCESS(dt, 12); \
  GPIOX_ODR(LCD_D13) = BITBAND_ACCESS(dt, 13); \
  GPIOX_ODR(LCD_D14) = BITBAND_ACCESS(dt, 14); \
  GPIOX_ODR(LCD_D15) = BITBAND_ACCESS(dt, 15); }
#endif
#endif

//-----------------------------------------------------------------------------
// data pins read
#ifndef LCD_READ
#ifdef  LCD_AUTOOPT
#define LCD_READ(dt) dt = GPIOX_PORT(LCD_D0)->IDR
#else
#define LCD_READ(dt) {                  \
  BITBAND_ACCESS(dt, 0) = GPIOX_IDR(LCD_D0); \
  BITBAND_ACCESS(dt, 1) = GPIOX_IDR(LCD_D1); \
  BITBAND_ACCESS(dt, 2) = GPIOX_IDR(LCD_D2); \
  BITBAND_ACCESS(dt, 3) = GPIOX_IDR(LCD_D3); \
  BITBAND_ACCESS(dt, 4) = GPIOX_IDR(LCD_D4); \
  BITBAND_ACCESS(dt, 5) = GPIOX_IDR(LCD_D5); \
  BITBAND_ACCESS(dt, 6) = GPIOX_IDR(LCD_D6); \
  BITBAND_ACCESS(dt, 7) = GPIOX_IDR(LCD_D7); \
  BITBAND_ACCESS(dt, 8) = GPIOX_IDR(LCD_D8); \
  BITBAND_ACCESS(dt, 9) = GPIOX_IDR(LCD_D9); \
  BITBAND_ACCESS(dt, 10) = GPIOX_IDR(LCD_D10); \
  BITBAND_ACCESS(dt, 11) = GPIOX_IDR(LCD_D11); \
  BITBAND_ACCESS(dt, 12) = GPIOX_IDR(LCD_D12); \
  BITBAND_ACCESS(dt, 13) = GPIOX_IDR(LCD_D13); \
  BITBAND_ACCESS(dt, 14) = GPIOX_IDR(LCD_D14); \
  BITBAND_ACCESS(dt, 15) = GPIOX_IDR(LCD_D15); }
#endif
#endif

//-----------------------------------------------------------------------------
/* Write delay */
#if     LCD_WRITE_DELAY == 0
#define LCD_WR_DELAY
#elif   LCD_WRITE_DELAY == 1
#define LCD_WR_DELAY          __NOP()
#elif   LCD_WRITE_DELAY == 2
#define LCD_WR_DELAY          GPIOX_ODR(LCD_WR) = 0
#else
#define LCD_WR_DELAY          LCD_IO_Delay(LCD_WRITE_DELAY - 3)
#endif

/* Read delay */
#if     LCD_READ_DELAY == 0
#define LCD_RD_DELAY
#elif   LCD_READ_DELAY == 1
#define LCD_RD_DELAY          __NOP()
#elif   LCD_READ_DELAY == 2
#define LCD_RD_DELAY          GPIOX_ODR(LCD_RD) = 0
#else
#define LCD_RD_DELAY          LCD_IO_Delay(LCD_READ_DELAY - 3)
#endif

#define LCD_DUMMY_READ        { GPIOX_ODR(LCD_RD) = 0; LCD_RD_DELAY; GPIOX_ODR(LCD_RD) = 1; }
#define LCD_DATA16_WRITE(dt)  { lcd_data16 = dt; LCD_WRITE(lcd_data16); GPIOX_ODR(LCD_WR) = 0; LCD_WR_DELAY; GPIOX_ODR(LCD_WR) = 1; }
#define LCD_DATA16_READ(dt)   { GPIOX_ODR(LCD_RD) = 0; LCD_RD_DELAY; LCD_READ(lcd_data16); dt = lcd_data16; GPIOX_ODR(LCD_RD) = 1; }
#define LCD_CMD16_WRITE(cmd16) { LCD_RS_CMD; LCD_DATA16_WRITE(cmd16); LCD_RS_DATA; }

/* temp data */
volatile uint16_t  lcd_data16;

//-----------------------------------------------------------------------------
#ifdef  __GNUC__
#pragma GCC push_options
#pragma GCC optimize("O0")
#elif   defined(__CC_ARM)
#pragma push
#pragma O0
#endif
void LCD_IO_Delay(uint32_t c)
{
  while(c--);
}
#ifdef  __GNUC__
#pragma GCC pop_options
#elif   defined(__CC_ARM)
#pragma pop
#endif

//-----------------------------------------------------------------------------
void LCD_Delay(uint32_t Delay)
{
  HAL_Delay(Delay);
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
  #define  GPIOX_CLOCK_LCD_RST  GPIOX_CLOCK(LCD_RST)
  #else
  #define  GPIOX_CLOCK_LCD_RST  0
  #endif

  /* Backlight pin clock */
  #if GPIOX_PORTNUM(LCD_BL) >=  GPIOX_PORTNUM_A
  #define  GPIOX_CLOCK_LCD_BL   GPIOX_CLOCK(LCD_BL)
  #else
  #define  GPIOX_CLOCK_LCD_BL   0
  #endif

  /* RD pin clock */
  #if GPIOX_PORTNUM(LCD_RD) >=  GPIOX_PORTNUM_A
  #define GPIOX_CLOCK_LCD_RD    GPIOX_CLOCK(LCD_RD)
  #else
  #define GPIOX_CLOCK_LCD_RD    0
  #endif
  
  RCC->APB2ENR |= (GPIOX_CLOCK(LCD_CS) | GPIOX_CLOCK(LCD_RS) | GPIOX_CLOCK(LCD_WR) |
                   GPIOX_CLOCK(LCD_D0) | GPIOX_CLOCK(LCD_D1) | GPIOX_CLOCK(LCD_D2) | GPIOX_CLOCK(LCD_D3) |
                   GPIOX_CLOCK(LCD_D4) | GPIOX_CLOCK(LCD_D5) | GPIOX_CLOCK(LCD_D6) | GPIOX_CLOCK(LCD_D7) |
                   GPIOX_CLOCK(LCD_D8) | GPIOX_CLOCK(LCD_D9) | GPIOX_CLOCK(LCD_D10)| GPIOX_CLOCK(LCD_D11)|
                   GPIOX_CLOCK(LCD_D12)| GPIOX_CLOCK(LCD_D13)| GPIOX_CLOCK(LCD_D14)| GPIOX_CLOCK(LCD_D15)|
                   GPIOX_CLOCK_LCD_RST | GPIOX_CLOCK_LCD_BL  | GPIOX_CLOCK_LCD_RD);

  #if GPIOX_PORTNUM(LCD_RST) >= GPIOX_PORTNUM_A
  LCD_RST_OFF;
  GPIOX_MODE(MODE_PP_OUT_2MHZ, LCD_RST);
  #endif

  #if GPIOX_PORTNUM(LCD_BL) >= GPIOX_PORTNUM_A
  GPIOX_ODR(LCD_BL) = LCD_BLON;
  GPIOX_MODE(MODE_PP_OUT_2MHZ, LCD_BL);
  #endif

  /* LCD controll pins setting to high */
  GPIOX_ODR(LCD_CS) = 1;                // CS = 1
  LCD_RS_DATA;                          // RS = 1
  GPIOX_ODR(LCD_WR) = 1;                // WR = 1
  #if GPIOX_PORTNUM(LCD_RD) >=  GPIOX_PORTNUM_A
  GPIOX_ODR(LCD_RD) = 1;                // RD = 1
  #endif

  /* LCD controll pins setting to output */
  GPIOX_MODE(MODE_PP_OUT_50MHZ, LCD_CS);
  GPIOX_MODE(MODE_PP_OUT_50MHZ, LCD_RS);
  GPIOX_MODE(MODE_PP_OUT_50MHZ, LCD_WR);
  #if GPIOX_PORTNUM(LCD_RD) >=  GPIOX_PORTNUM_A
  GPIOX_MODE(MODE_PP_OUT_50MHZ, LCD_RD);
  #endif

  /* data pins directions = out */
  LCD_DIRWRITE;

  /* Reset the LCD */
  #if GPIOX_PORTNUM(LCD_RST) >= GPIOX_PORTNUM_A
  LCD_Delay(1);
  LCD_RST_ON;                           // RST = 0
  LCD_Delay(1);
  LCD_RST_OFF;                          // RST = 1
  #endif
  LCD_Delay(1);
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteCmd8(uint8_t Cmd)
{
  LCD_CS_ON;
  LCD_CMD16_WRITE(Cmd);
  LCD_CS_OFF;
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteCmd16(uint16_t Cmd)
{
  LCD_CS_ON;
  LCD_CMD16_WRITE(Cmd);
  LCD_CS_OFF;
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteData8(uint8_t Data)
{
  LCD_CS_ON;
  LCD_DATA16_WRITE(Data);
  LCD_CS_OFF;
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteData16(uint16_t Data)
{
  LCD_CS_ON;
  LCD_DATA16_WRITE(Data);
  LCD_CS_OFF;
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteCmd8DataFill16(uint8_t Cmd, uint16_t Data, uint32_t Size)
{
  LCD_CS_ON;
  LCD_CMD16_WRITE(Cmd);
  while(Size--)
  {
    LCD_DATA16_WRITE(Data);
  }
  LCD_CS_OFF;
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteCmd8MultipleData8(uint8_t Cmd, uint8_t *pData, uint32_t Size)
{
  LCD_CS_ON;
  LCD_CMD16_WRITE(Cmd);

  while(Size--)
  {
    LCD_DATA16_WRITE(*(uint8_t *)pData);
    pData ++;
  }
  LCD_CS_OFF;
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteCmd8MultipleData16(uint8_t Cmd, uint16_t *pData, uint32_t Size)
{
  LCD_CS_ON;
  LCD_CMD16_WRITE(Cmd);
  while(Size--)
  {
    LCD_DATA16_WRITE(*pData);
    pData ++;
  }
  LCD_CS_OFF;
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteCmd16DataFill16(uint16_t Cmd, uint16_t Data, uint32_t Size)
{
  LCD_CS_ON;
  LCD_CMD16_WRITE(Cmd);
  while(Size--)
  {
    LCD_DATA16_WRITE(Data);
  }
  LCD_CS_OFF;
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteCmd16MultipleData8(uint16_t Cmd, uint8_t *pData, uint32_t Size)
{
  LCD_CS_ON;
  LCD_CMD16_WRITE(Cmd);
  while(Size--)
  {
    LCD_DATA16_WRITE(*(uint8_t *)pData);
    pData ++;
  }
  LCD_CS_OFF;
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteCmd16MultipleData16(uint16_t Cmd, uint16_t *pData, uint32_t Size)
{
  LCD_CS_ON;
  LCD_CMD16_WRITE(Cmd);
  while(Size--)
  {
    LCD_DATA16_WRITE(*pData);
    pData ++;
  }
  LCD_CS_OFF;
}

//-----------------------------------------------------------------------------
#if GPIOX_PORTNUM(LCD_RD) >=  GPIOX_PORTNUM_A
void LCD_IO_ReadCmd8MultipleData8(uint8_t Cmd, uint8_t *pData, uint32_t Size, uint32_t DummySize)
{
  uint16_t d16;
  LCD_CS_ON;
  LCD_CMD16_WRITE(Cmd);
  LCD_DIRREAD;
  while(DummySize--)
    LCD_DUMMY_READ;
  while(Size--)
  {
    LCD_DATA16_READ(d16);
    *pData = (uint8_t)d16;
    pData++;
  }
  LCD_CS_OFF;
  LCD_DIRWRITE;
}

//-----------------------------------------------------------------------------
void LCD_IO_ReadCmd8MultipleData16(uint8_t Cmd, uint16_t *pData, uint32_t Size, uint32_t DummySize)
{
  static uint16_t d16;
  LCD_CS_ON;
  LCD_CMD16_WRITE(Cmd);
  LCD_DIRREAD;
  while(DummySize--)
    LCD_DUMMY_READ;
  while(Size--)
  {
    LCD_DATA16_READ(d16);
    *pData = d16;
    pData++;
  }
  LCD_CS_OFF;
  LCD_DIRWRITE;
}

//-----------------------------------------------------------------------------
void LCD_IO_ReadCmd8MultipleData24to16(uint8_t Cmd, uint16_t *pData, uint32_t Size, uint32_t DummySize)
{
  union packed
  {
    uint8_t  rgb888[6];
    uint16_t rgb888_16[3];
  }u;
  LCD_CS_ON;
  LCD_CMD16_WRITE(Cmd);
  LCD_DIRREAD;
  while(DummySize--)
    LCD_DUMMY_READ;
  while(Size--)
  {
    LCD_DATA16_READ(u.rgb888_16[0]);
    LCD_DATA16_READ(u.rgb888_16[1]);
    LCD_DATA16_READ(u.rgb888_16[2]);
    *pData = ((u.rgb888[1] & 0xF8) << 8 | (u.rgb888[0] & 0xFC) << 3 | u.rgb888[3] >> 3);
    pData++;
    if(Size)
    {
      *pData = ((u.rgb888[2] & 0xF8) << 8 | (u.rgb888[5] & 0xFC) << 3 | u.rgb888[4] >> 3);
      pData++;
      Size--;
    }
  }
  LCD_CS_OFF;
  LCD_DIRWRITE;
}

//-----------------------------------------------------------------------------
void LCD_IO_ReadCmd16MultipleData8(uint16_t Cmd, uint8_t *pData, uint32_t Size, uint32_t DummySize)
{
  uint16_t d16;
  LCD_CS_ON;
  LCD_CMD16_WRITE(Cmd);
  LCD_DIRREAD;
  while(DummySize--)
    LCD_DUMMY_READ;
  while(Size--)
  {
    LCD_DATA16_READ(d16);
    *pData = (uint8_t)d16;
    pData++;
  }
  LCD_CS_OFF;
  LCD_DIRWRITE;
}

//-----------------------------------------------------------------------------
void LCD_IO_ReadCmd16MultipleData16(uint16_t Cmd, uint16_t *pData, uint32_t Size, uint32_t DummySize)
{
  uint16_t  d16;
  LCD_CS_ON;
  LCD_CMD16_WRITE(Cmd);
  LCD_DIRREAD;
  while(DummySize--)
    LCD_DUMMY_READ;
  while(Size--)
  {
    LCD_DATA16_READ(d16);
    *pData = d16;
    pData++;
  }
  LCD_CS_OFF;
  LCD_DIRWRITE;
}

//-----------------------------------------------------------------------------
void LCD_IO_ReadCmd16MultipleData24to16(uint16_t Cmd, uint16_t *pData, uint32_t Size, uint32_t DummySize)
{
  union packed
  {
    uint8_t  rgb888[6];
    uint16_t rgb888_16[3];
  }u;
  LCD_CS_ON;
  LCD_CMD16_WRITE(Cmd);
  LCD_DIRREAD;
  while(DummySize--)
    LCD_DUMMY_READ;
  while(Size--)
  {
    LCD_DATA16_READ(u.rgb888_16[0]);
    LCD_DATA16_READ(u.rgb888_16[1]);
    LCD_DATA16_READ(u.rgb888_16[2]);
    *pData = ((u.rgb888[1] & 0xF8) << 8 | (u.rgb888[0] & 0xFC) << 3 | u.rgb888[3] >> 3);
    pData++;
    if(Size)
    {
      *pData = ((u.rgb888[2] & 0xF8) << 8 | (u.rgb888[5] & 0xFC) << 3 | u.rgb888[4] >> 3);
      pData++;
      Size--;
    }
  }
  LCD_CS_OFF;
  LCD_DIRWRITE;
}

#else
void LCD_IO_ReadCmd8MultipleData8(uint8_t Cmd, uint8_t *pData, uint32_t Size, uint32_t DummySize) { }
void LCD_IO_ReadCmd8MultipleData16(uint8_t Cmd, uint16_t *pData, uint32_t Size, uint32_t DummySize) { }
void LCD_IO_ReadCmd8MultipleData24to16(uint8_t Cmd, uint16_t *pData, uint32_t Size, uint32_t DummySize) { }
void LCD_IO_ReadCmd16MultipleData8(uint16_t Cmd, uint8_t *pData, uint32_t Size, uint32_t DummySize) { }
void LCD_IO_ReadCmd16MultipleData16(uint16_t Cmd, uint16_t *pData, uint32_t Size, uint32_t DummySize) { }
void LCD_IO_ReadCmd16MultipleData24to16(uint16_t Cmd, uint16_t *pData, uint32_t Size, uint32_t DummySize) { }
#endif
