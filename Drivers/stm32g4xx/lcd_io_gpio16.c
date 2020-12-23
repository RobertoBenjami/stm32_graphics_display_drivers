/*
 * 16 bit paralell LCD GPIO driver for STM32G4
 * 5 controll pins (CS, RS, WR, RD, RST) + 16 data pins + backlight pin
 */

/* 
 * Author: Roberto Benjami
 * version:  2020.12.23
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
/* GPIO mode */
#define MODE_DIGITAL_INPUT    0x0
#define MODE_OUT              0x1
#define MODE_ALTER            0x2
#define MODE_ANALOG_INPUT     0x3

#define MODE_SPD_LOW          0x0
#define MODE_SPD_MEDIUM       0x1
#define MODE_SPD_HIGH         0x2
#define MODE_SPD_VHIGH        0x3

#define MODE_PU_NONE          0x0
#define MODE_PU_UP            0x1
#define MODE_PU_DOWN          0x2

#define GPIOX_PORT_(a, b)     GPIO ## a
#define GPIOX_PORT(a)         GPIOX_PORT_(a)

#define GPIOX_PIN_(a, b)      b
#define GPIOX_PIN(a)          GPIOX_PIN_(a)

#define GPIOX_AFR_(a,b,c)     GPIO ## b->AFR[c >> 3] = (GPIO ## b->AFR[c >> 3] & ~(0x0F << (4 * (c & 7)))) | (a << (4 * (c & 7)));
#define GPIOX_AFR(a, b)       GPIOX_AFR_(a, b)

#define GPIOX_MODER_(a,b,c)   GPIO ## b->MODER = (GPIO ## b->MODER & ~(3 << (2 * c))) | (a << (2 * c));
#define GPIOX_MODER(a, b)     GPIOX_MODER_(a, b)

#define GPIOX_OSPEEDR_(a,b,c) GPIO ## b->OSPEEDR = (GPIO ## b->OSPEEDR & ~(3 << (2 * c))) | (a << (2 * c));
#define GPIOX_OSPEEDR(a, b)   GPIOX_OSPEEDR_(a, b)

#define GPIOX_PUPDR_(a,b,c)   GPIO ## b->PUPDR = (GPIO ## b->PUPDR & ~(3 << (2 * c))) | (a << (2 * c));
#define GPIOX_PUPDR(a, b)     GPIOX_PUPDR_(a, b)

#define GPIOX_SET_(a, b)      GPIO ## a ->BSRR = 1 << b
#define GPIOX_SET(a)          GPIOX_SET_(a)

#define GPIOX_CLR_(a, b)      GPIO ## a ->BSRR = 1 << (b + 16)
#define GPIOX_CLR(a)          GPIOX_CLR_(a)

#define GPIOX_IDR_(a, b)      (GPIO ## a ->IDR & (1 << b))
#define GPIOX_IDR(a)          GPIOX_IDR_(a)

#define GPIOX_CLOCK_(a, b)    RCC_AHB2ENR_GPIO ## a ## EN
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
/* command/data pin setting */
#define LCD_RS_CMD            GPIOX_CLR(LCD_RS)
#define LCD_RS_DATA           GPIOX_SET(LCD_RS)

/* reset pin setting */
#define LCD_RST_ON            GPIOX_CLR(LCD_RST)
#define LCD_RST_OFF           GPIOX_SET(LCD_RST)

/* chip select pin setting */
#define LCD_CS_ON             GPIOX_CLR(LCD_CS)
#define LCD_CS_OFF            GPIOX_SET(LCD_CS)

//-----------------------------------------------------------------------------
/* if the 16 data pins are in order -> automatic optimalization */
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
/* LCD_D0..LCD_D15 pins are order */
#define LCD_AUTOOPT
#endif /* D0..D15 pin order */
#endif /* D0..D15 port same */

//-----------------------------------------------------------------------------
/* data pins set to output direction */
#ifndef LCD_DIRWRITE
#ifdef  LCD_AUTOOPT
#define LCD_DIRWRITE  GPIOX_PORT(LCD_D0)->MODER = 0x55555555;
#else
#define LCD_DIRWRITE { \
  GPIOX_MODER(MODE_OUT, LCD_D0); GPIOX_MODER(MODE_OUT, LCD_D1);\
  GPIOX_MODER(MODE_OUT, LCD_D2); GPIOX_MODER(MODE_OUT, LCD_D3);\
  GPIOX_MODER(MODE_OUT, LCD_D4); GPIOX_MODER(MODE_OUT, LCD_D5);\
  GPIOX_MODER(MODE_OUT, LCD_D6); GPIOX_MODER(MODE_OUT, LCD_D7);\
  GPIOX_MODER(MODE_OUT, LCD_D8); GPIOX_MODER(MODE_OUT, LCD_D9);\
  GPIOX_MODER(MODE_OUT, LCD_D10); GPIOX_MODER(MODE_OUT, LCD_D11);\
  GPIOX_MODER(MODE_OUT, LCD_D12); GPIOX_MODER(MODE_OUT, LCD_D13);\
  GPIOX_MODER(MODE_OUT, LCD_D14); GPIOX_MODER(MODE_OUT, LCD_D15);}
#endif
#endif

//-----------------------------------------------------------------------------
/* data pins set to input direction */
#ifndef LCD_DIRREAD
#ifdef  LCD_AUTOOPT
#define LCD_DIRREAD  GPIOX_PORT(LCD_D0)->MODER = 0x00000000;
#else
#define LCD_DIRREAD { \
  GPIOX_MODER(MODE_DIGITAL_INPUT, LCD_D0); GPIOX_MODER(MODE_DIGITAL_INPUT, LCD_D1);\
  GPIOX_MODER(MODE_DIGITAL_INPUT, LCD_D2); GPIOX_MODER(MODE_DIGITAL_INPUT, LCD_D3);\
  GPIOX_MODER(MODE_DIGITAL_INPUT, LCD_D4); GPIOX_MODER(MODE_DIGITAL_INPUT, LCD_D5);\
  GPIOX_MODER(MODE_DIGITAL_INPUT, LCD_D6); GPIOX_MODER(MODE_DIGITAL_INPUT, LCD_D7);\
  GPIOX_MODER(MODE_DIGITAL_INPUT, LCD_D8); GPIOX_MODER(MODE_DIGITAL_INPUT, LCD_D9);\
  GPIOX_MODER(MODE_DIGITAL_INPUT, LCD_D10); GPIOX_MODER(MODE_DIGITAL_INPUT, LCD_D11);\
  GPIOX_MODER(MODE_DIGITAL_INPUT, LCD_D12); GPIOX_MODER(MODE_DIGITAL_INPUT, LCD_D13);\
  GPIOX_MODER(MODE_DIGITAL_INPUT, LCD_D14); GPIOX_MODER(MODE_DIGITAL_INPUT, LCD_D15);}
#endif
#endif

//-----------------------------------------------------------------------------
/* 16 bit data write to the data pins */
#ifndef LCD_WRITE
#ifdef  LCD_AUTOOPT
#define LCD_WRITE(dt) {                      \
  GPIOX_PORT(LCD_D0)->ODR = (dt);            }
#else
#define LCD_WRITE(dt) {;                     \
  if(dt & 0x0001) GPIOX_SET(LCD_D0); else GPIOX_CLR(LCD_D0); \
  if(dt & 0x0002) GPIOX_SET(LCD_D1); else GPIOX_CLR(LCD_D1); \
  if(dt & 0x0004) GPIOX_SET(LCD_D2); else GPIOX_CLR(LCD_D2); \
  if(dt & 0x0008) GPIOX_SET(LCD_D3); else GPIOX_CLR(LCD_D3); \
  if(dt & 0x0010) GPIOX_SET(LCD_D4); else GPIOX_CLR(LCD_D4); \
  if(dt & 0x0020) GPIOX_SET(LCD_D5); else GPIOX_CLR(LCD_D5); \
  if(dt & 0x0040) GPIOX_SET(LCD_D6); else GPIOX_CLR(LCD_D6); \
  if(dt & 0x0080) GPIOX_SET(LCD_D7); else GPIOX_CLR(LCD_D7); \
  if(dt & 0x0100) GPIOX_SET(LCD_D8); else GPIOX_CLR(LCD_D8); \
  if(dt & 0x0200) GPIOX_SET(LCD_D9); else GPIOX_CLR(LCD_D9); \
  if(dt & 0x0400) GPIOX_SET(LCD_D10); else GPIOX_CLR(LCD_D10); \
  if(dt & 0x0800) GPIOX_SET(LCD_D11); else GPIOX_CLR(LCD_D11); \
  if(dt & 0x1000) GPIOX_SET(LCD_D12); else GPIOX_CLR(LCD_D12); \
  if(dt & 0x2000) GPIOX_SET(LCD_D13); else GPIOX_CLR(LCD_D13); \
  if(dt & 0x4000) GPIOX_SET(LCD_D14); else GPIOX_CLR(LCD_D14); \
  if(dt & 0x8000) GPIOX_SET(LCD_D15); else GPIOX_CLR(LCD_D15); }
#endif
#endif

//-----------------------------------------------------------------------------
/* 16 bit data read from the data pins */
#ifndef LCD_READ
#ifdef  LCD_AUTOOPT
#define LCD_READ(dt) {                       \
  dt = GPIOX_PORT(LCD_D0)->IDR;              }
#else
#define LCD_READ(dt) {                       \
  if(GPIOX_IDR(LCD_D0)) dt = 1; else dt = 0; \
  if(GPIOX_IDR(LCD_D1)) dt |= 0x0002;        \
  if(GPIOX_IDR(LCD_D2)) dt |= 0x0004;        \
  if(GPIOX_IDR(LCD_D3)) dt |= 0x0008;        \
  if(GPIOX_IDR(LCD_D4)) dt |= 0x0010;        \
  if(GPIOX_IDR(LCD_D5)) dt |= 0x0020;        \
  if(GPIOX_IDR(LCD_D6)) dt |= 0x0040;        \
  if(GPIOX_IDR(LCD_D7)) dt |= 0x0080;        \
  if(GPIOX_IDR(LCD_D8)) dt |= 0x0100;        \
  if(GPIOX_IDR(LCD_D9)) dt |= 0x0200;        \
  if(GPIOX_IDR(LCD_D10)) dt |= 0x0400;       \
  if(GPIOX_IDR(LCD_D11)) dt |= 0x0800;       \
  if(GPIOX_IDR(LCD_D12)) dt |= 0x1000;       \
  if(GPIOX_IDR(LCD_D13)) dt |= 0x2000;       \
  if(GPIOX_IDR(LCD_D14)) dt |= 0x4000;       \
  if(GPIOX_IDR(LCD_D15)) dt |= 0x8000;       }
#endif
#endif

//-----------------------------------------------------------------------------
/* Write / Read spd */
#if     LCD_WRITE_DELAY == 0
#define LCD_WR_DELAY
#elif   LCD_WRITE_DELAY == 1
#define LCD_WR_DELAY          GPIOX_CLR(LCD_WR)
#else
#define LCD_WR_DELAY          LCD_IO_Delay(LCD_WRITE_DELAY - 2)
#endif
#if     LCD_READ_DELAY == 0
#define LCD_RD_DELAY
#elif   LCD_READ_DELAY == 1
#define LCD_RD_DELAY          GPIOX_CLR(LCD_RD)
#else
#define LCD_RD_DELAY          LCD_IO_Delay(LCD_READ_DELAY - 2)
#endif

#define LCD_DUMMY_READ        { GPIOX_CLR(LCD_RD); LCD_RD_DELAY; GPIOX_SET(LCD_RD); }
#define LCD_DATA16_WRITE(dt)  { lcd_data16 = dt; LCD_WRITE(lcd_data16); GPIOX_CLR(LCD_WR); LCD_WR_DELAY; GPIOX_SET(LCD_WR); }
#define LCD_DATA16_READ(dt)   { GPIOX_CLR(LCD_RD); LCD_RD_DELAY; LCD_READ(lcd_data16); dt = lcd_data16; GPIOX_SET(LCD_RD); }
#define LCD_CMD16_WRITE(cmd16) { LCD_RS_CMD; LCD_DATA16_WRITE(cmd16); LCD_RS_DATA; }

/* 16 bit temp data */
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
  #if LCD_BLON == 1
  if(Bl)
    GPIOX_SET(LCD_BL);
  else
    GPIOX_CLR(LCD_BL);
  #else
  if(Bl)
    GPIOX_CLR(LCD_BL);
  else
    GPIOX_SET(LCD_BL);
  #endif
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
  #if GPIOX_PORTNUM(LCD_BL)  >= GPIOX_PORTNUM_A
  #define GPIOX_CLOCK_LCD_BL    GPIOX_CLOCK(LCD_BL)
  #else
  #define GPIOX_CLOCK_LCD_BL    0
  #endif

  /* RD pin clock */
  #if GPIOX_PORTNUM(LCD_RD) >=  GPIOX_PORTNUM_A
  #define GPIOX_CLOCK_LCD_RD    GPIOX_CLOCK(LCD_RD)
  #else
  #define GPIOX_CLOCK_LCD_RD    0
  #endif
  
  RCC->AHB2ENR |= (GPIOX_CLOCK(LCD_CS) | GPIOX_CLOCK(LCD_RS) | GPIOX_CLOCK(LCD_WR) |
                   GPIOX_CLOCK(LCD_D0) | GPIOX_CLOCK(LCD_D1) | GPIOX_CLOCK(LCD_D2) | GPIOX_CLOCK(LCD_D3) |
                   GPIOX_CLOCK(LCD_D4) | GPIOX_CLOCK(LCD_D5) | GPIOX_CLOCK(LCD_D6) | GPIOX_CLOCK(LCD_D7) |
                   GPIOX_CLOCK(LCD_D8) | GPIOX_CLOCK(LCD_D9) | GPIOX_CLOCK(LCD_D10)| GPIOX_CLOCK(LCD_D11)|
                   GPIOX_CLOCK(LCD_D12)| GPIOX_CLOCK(LCD_D13)| GPIOX_CLOCK(LCD_D14)| GPIOX_CLOCK(LCD_D15)|
                   GPIOX_CLOCK_LCD_RST | GPIOX_CLOCK_LCD_BL  | GPIOX_CLOCK_LCD_RD);
	
  #if GPIOX_PORTNUM(LCD_RST) >= GPIOX_PORTNUM_A
  LCD_RST_OFF;
  GPIOX_MODER(MODE_OUT, LCD_RST);
  #endif

  #if GPIOX_PORTNUM(LCD_BL) >= GPIOX_PORTNUM_A
  #if LCD_BLON == 0
  GPIOX_CLR(LCD_BL);
  #else
  GPIOX_SET(LCD_BL);
  #endif
  GPIOX_MODER(MODE_OUT, LCD_BL);
  GPIOX_OSPEEDR(MODE_SPD_LOW, LCD_BL);
  #endif

  GPIOX_SET(LCD_CS);                    /* CS = 1 */
  LCD_RS_DATA;                          /* RS = 1 */ 
  GPIOX_SET(LCD_WR);                    /* WR = 1 */
  #if GPIOX_PORTNUM(LCD_RD) >=  GPIOX_PORTNUM_A
  GPIOX_SET(LCD_RD);                    /* RD = 1 */
  #endif

  GPIOX_MODER(MODE_OUT, LCD_CS);
  GPIOX_MODER(MODE_OUT, LCD_RS);
  GPIOX_MODER(MODE_OUT, LCD_WR);
  #if GPIOX_PORTNUM(LCD_RD) >=  GPIOX_PORTNUM_A
  GPIOX_MODER(MODE_OUT, LCD_RD);
  #endif

  LCD_DIRWRITE;                         /* data pins set the output direction */

  /* GPIO speed */
  GPIOX_OSPEEDR(MODE_SPD_VHIGH, LCD_CS);
  GPIOX_OSPEEDR(MODE_SPD_VHIGH, LCD_RS);
  GPIOX_OSPEEDR(MODE_SPD_VHIGH, LCD_WR);
  #if GPIOX_PORTNUM(LCD_RD) >=  GPIOX_PORTNUM_A
  GPIOX_OSPEEDR(MODE_SPD_VHIGH, LCD_RD);
  #endif
  GPIOX_OSPEEDR(MODE_SPD_VHIGH, LCD_D0);
  GPIOX_OSPEEDR(MODE_SPD_VHIGH, LCD_D1);
  GPIOX_OSPEEDR(MODE_SPD_VHIGH, LCD_D2);
  GPIOX_OSPEEDR(MODE_SPD_VHIGH, LCD_D3);
  GPIOX_OSPEEDR(MODE_SPD_VHIGH, LCD_D4);
  GPIOX_OSPEEDR(MODE_SPD_VHIGH, LCD_D5);
  GPIOX_OSPEEDR(MODE_SPD_VHIGH, LCD_D6);
  GPIOX_OSPEEDR(MODE_SPD_VHIGH, LCD_D7);
  GPIOX_OSPEEDR(MODE_SPD_VHIGH, LCD_D8);
  GPIOX_OSPEEDR(MODE_SPD_VHIGH, LCD_D9);
  GPIOX_OSPEEDR(MODE_SPD_VHIGH, LCD_D10);
  GPIOX_OSPEEDR(MODE_SPD_VHIGH, LCD_D11);
  GPIOX_OSPEEDR(MODE_SPD_VHIGH, LCD_D12);
  GPIOX_OSPEEDR(MODE_SPD_VHIGH, LCD_D13);
  GPIOX_OSPEEDR(MODE_SPD_VHIGH, LCD_D14);
  GPIOX_OSPEEDR(MODE_SPD_VHIGH, LCD_D15);

  /* Reset the LCD */
  #if GPIOX_PORTNUM(LCD_RST) >= GPIOX_PORTNUM_A
  LCD_Delay(1);
  LCD_RST_ON;                           /* RST = 0 */
  LCD_Delay(1);
  LCD_RST_OFF;                          /* RST = 1 */
  #endif
  LCD_Delay(1);
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteCmd8(uint8_t Cmd)
{
  LCD_CS_ON;
  LCD_CMD16_WRITE((uint8_t)Cmd);
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
  LCD_DATA16_WRITE((uint8_t)Data);
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
  LCD_CMD16_WRITE((uint8_t)Cmd);
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
  LCD_CMD16_WRITE((uint8_t)Cmd);
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
  LCD_CMD16_WRITE((uint8_t)Cmd);
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
