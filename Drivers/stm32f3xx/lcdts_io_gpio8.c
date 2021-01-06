/*
 * 8 bit paralell LCD/TOUCH GPIO driver for STM32F3
 * 5 controll pins (CS, RS, WR, RD, RST) + 8 data pins + backlight pin

 * note: whereas the LCD and Touchscreen are on the same pins,
 * therefore, the simultaneous use of Lcd and Touchscreen must be excluded!
 * In a multithreaded / intermittent environment, care must be taken to prevent collisions!
 */

/* 
 * Author: Roberto Benjami
 * version:  2020.05.28
 */


/* ADC sample time (0:1cycles, 1:2c, 2:4c, 3:7c, 4:19c, 5:61c, 6:181c, 7:601cycles) */
#define  TS_SAMPLETIME        2

#include "main.h"
#include "lcd.h"
#include "lcdts_io_gpio8.h"

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

/* Link function for Touchscreen */
uint8_t  TS_IO_DetectToch(void);
uint16_t TS_IO_GetX(void);
uint16_t TS_IO_GetY(void);
uint16_t TS_IO_GetZ1(void);
uint16_t TS_IO_GetZ2(void);

//-----------------------------------------------------------------------------
#define BITBAND_ACCESS(a, b)  *(volatile uint32_t*)(((uint32_t)&a & 0xF0000000) + 0x2000000 + (((uint32_t)&a & 0x000FFFFF) << 5) + (b << 2))

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

#define GPIOX_LINE_(a, b)     EXTI_Line ## b
#define GPIOX_LINE(a)         GPIOX_LINE_(a)

#define GPIOX_CLOCK_(a, b)    RCC_AHBENR_GPIO ## a ## EN
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
/* if the 8 data pins are in order -> automatic optimalization */
#if ((GPIOX_PORTNUM(LCD_D0) == GPIOX_PORTNUM(LCD_D1))\
  && (GPIOX_PORTNUM(LCD_D1) == GPIOX_PORTNUM(LCD_D2))\
  && (GPIOX_PORTNUM(LCD_D2) == GPIOX_PORTNUM(LCD_D3))\
  && (GPIOX_PORTNUM(LCD_D3) == GPIOX_PORTNUM(LCD_D4))\
  && (GPIOX_PORTNUM(LCD_D4) == GPIOX_PORTNUM(LCD_D5))\
  && (GPIOX_PORTNUM(LCD_D5) == GPIOX_PORTNUM(LCD_D6))\
  && (GPIOX_PORTNUM(LCD_D6) == GPIOX_PORTNUM(LCD_D7)))
#if ((GPIOX_PIN(LCD_D0) + 1 == GPIOX_PIN(LCD_D1))\
  && (GPIOX_PIN(LCD_D1) + 1 == GPIOX_PIN(LCD_D2))\
  && (GPIOX_PIN(LCD_D2) + 1 == GPIOX_PIN(LCD_D3))\
  && (GPIOX_PIN(LCD_D3) + 1 == GPIOX_PIN(LCD_D4))\
  && (GPIOX_PIN(LCD_D4) + 1 == GPIOX_PIN(LCD_D5))\
  && (GPIOX_PIN(LCD_D5) + 1 == GPIOX_PIN(LCD_D6))\
  && (GPIOX_PIN(LCD_D6) + 1 == GPIOX_PIN(LCD_D7)))
/* LCD data pins on n..n+7 pin (ex. B6,B7,B8,B9,B10,B11,B12,B13) */
#define LCD_AUTOOPT
#endif /* D0..D7 pin order */
#endif /* D0..D7 port same */

//-----------------------------------------------------------------------------
/* data pins set to output direction */
#ifndef LCD_DIRWRITE
#ifdef  LCD_AUTOOPT
#define LCD_DIRWRITE  GPIOX_PORT(LCD_D0)->MODER = (GPIOX_PORT(LCD_D0)->MODER & ~(0xFFFF << (2 * GPIOX_PIN(LCD_D0)))) | (0x5555 << (2 * GPIOX_PIN(LCD_D0)));
#else
#define LCD_DIRWRITE { \
  GPIOX_MODER(MODE_OUT, LCD_D0); GPIOX_MODER(MODE_OUT, LCD_D1);\
  GPIOX_MODER(MODE_OUT, LCD_D2); GPIOX_MODER(MODE_OUT, LCD_D3);\
  GPIOX_MODER(MODE_OUT, LCD_D4); GPIOX_MODER(MODE_OUT, LCD_D5);\
  GPIOX_MODER(MODE_OUT, LCD_D6); GPIOX_MODER(MODE_OUT, LCD_D7);}
#endif
#endif

//-----------------------------------------------------------------------------
/* data pins set to input direction */
#ifndef LCD_DIRREAD
#ifdef  LCD_AUTOOPT
#define LCD_DIRREAD  GPIOX_PORT(LCD_D0)->MODER = (GPIOX_PORT(LCD_D0)->MODER & ~(0xFFFF << (2 * GPIOX_PIN(LCD_D0)))) | (0x0000 << (2 * GPIOX_PIN(LCD_D0)));
#else
#define LCD_DIRREAD { \
  GPIOX_MODER(MODE_DIGITAL_INPUT, LCD_D0); GPIOX_MODER(MODE_DIGITAL_INPUT, LCD_D1);\
  GPIOX_MODER(MODE_DIGITAL_INPUT, LCD_D2); GPIOX_MODER(MODE_DIGITAL_INPUT, LCD_D3);\
  GPIOX_MODER(MODE_DIGITAL_INPUT, LCD_D4); GPIOX_MODER(MODE_DIGITAL_INPUT, LCD_D5);\
  GPIOX_MODER(MODE_DIGITAL_INPUT, LCD_D6); GPIOX_MODER(MODE_DIGITAL_INPUT, LCD_D7);}
#endif
#endif

//-----------------------------------------------------------------------------
/* 8 bit data write to the data pins */
#ifndef LCD_WRITE
#ifdef  LCD_AUTOOPT
#define LCD_WRITE(dt) { \
  GPIOX_PORT(LCD_D0)->BSRR = (dt << GPIOX_PIN(LCD_D0)) | (0xFF << (GPIOX_PIN(LCD_D0) + 16));}
#else
#define LCD_WRITE(dt) {;                                   \
  if(dt & 0x01) GPIOX_SET(LCD_D0); else GPIOX_CLR(LCD_D0); \
  if(dt & 0x02) GPIOX_SET(LCD_D1); else GPIOX_CLR(LCD_D1); \
  if(dt & 0x04) GPIOX_SET(LCD_D2); else GPIOX_CLR(LCD_D2); \
  if(dt & 0x08) GPIOX_SET(LCD_D3); else GPIOX_CLR(LCD_D3); \
  if(dt & 0x10) GPIOX_SET(LCD_D4); else GPIOX_CLR(LCD_D4); \
  if(dt & 0x20) GPIOX_SET(LCD_D5); else GPIOX_CLR(LCD_D5); \
  if(dt & 0x40) GPIOX_SET(LCD_D6); else GPIOX_CLR(LCD_D6); \
  if(dt & 0x80) GPIOX_SET(LCD_D7); else GPIOX_CLR(LCD_D7); }
#endif
#endif

//-----------------------------------------------------------------------------
/* 8 bit data read from the pins */
#ifndef LCD_READ
#ifdef  LCD_AUTOOPT
#define LCD_READ(dt) {                          \
  dt = GPIOX_PORT(LCD_D0)->IDR >> GPIOX_PIN(LCD_D0); }
#else
#define LCD_READ(dt) {                       \
  if(GPIOX_IDR(LCD_D0)) dt = 1; else dt = 0; \
  if(GPIOX_IDR(LCD_D1)) dt |= 0x02;          \
  if(GPIOX_IDR(LCD_D2)) dt |= 0x04;          \
  if(GPIOX_IDR(LCD_D3)) dt |= 0x08;          \
  if(GPIOX_IDR(LCD_D4)) dt |= 0x10;          \
  if(GPIOX_IDR(LCD_D5)) dt |= 0x20;          \
  if(GPIOX_IDR(LCD_D6)) dt |= 0x40;          \
  if(GPIOX_IDR(LCD_D7)) dt |= 0x80;          }
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
#define LCD_DATA8_WRITE(dt)   { lcd_data8 = dt; LCD_WRITE(lcd_data8); GPIOX_CLR(LCD_WR); LCD_WR_DELAY; GPIOX_SET(LCD_WR); }
#define LCD_DATA8_READ(dt)    { GPIOX_CLR(LCD_RD); LCD_RD_DELAY; LCD_READ(dt); GPIOX_SET(LCD_RD); }
#define LCD_CMD8_WRITE(cmd)   { LCD_RS_CMD; LCD_DATA8_WRITE(cmd); LCD_RS_DATA; }

#if LCD_REVERSE16 == 0
#define LCD_CMD16_WRITE(cmd16)  {LCD_RS_CMD; LCD_DATA8_WRITE(cmd16 >> 8); LCD_DATA8_WRITE(cmd16); LCD_RS_DATA; }
#define LCD_DATA16_WRITE(d16)   {LCD_DATA8_WRITE(d16 >> 8); LCD_DATA8_WRITE(d16); }
#define LCD_DATA16_READ(dh, dl) {LCD_DATA8_READ(dh); LCD_DATA8_READ(dl); }
#else
#define LCD_CMD16_WRITE(cmd)    {LCD_RS_CMD; LCD_DATA8_WRITE(cmd); LCD_DATA8_WRITE(cmd >> 8); LCD_RS_DATA; }
#define LCD_DATA16_WRITE(data)  {LCD_DATA8_WRITE(data); LCD_DATA8_WRITE(data >> 8); }
#define LCD_DATA16_READ(dh, dl) {LCD_DATA8_READ(dl); LCD_DATA8_READ(dh); }
#endif

//-----------------------------------------------------------------------------
#if TS_ADC == 1
#define RCC_AHBENR_ADCXEN     RCC_AHBENR_ADC12EN
#define ADCX                  ADC1
#endif

#if TS_ADC == 2
#define RCC_AHBENR_ADCXEN     RCC_AHBENR_ADC12EN
#define ADCX                  ADC2
#endif

#if TS_ADC == 3
#define RCC_AHBENR_ADCXEN     RCC_AHBENR_ADC34EN
#define ADCX                  ADC3
#endif

#if TS_ADC == 4
#define RCC_AHBENR_ADCXEN     RCC_AHBENR_ADC34EN
#define ADCX                  ADC4
#endif

#ifdef  ADCX

/* this pins in the LCD are paralell */
#define  TS_XP                LCD_D6
#define  TS_XM                LCD_RS
#define  TS_YP                LCD_WR
#define  TS_YM                LCD_D7

#if defined(TS_XM_AN)
#if GPIOX_PORTNUM(TS_XM_AN) < 1
#undef   TS_XM_AN
#define  TS_XM_AN             TS_XM
#endif
#else
#define  TS_XM_AN             TS_XM
#endif

#if defined(TS_YP_AN)
#if GPIOX_PORTNUM(TS_YP_AN) < 1
#undef   TS_YP_AN
#define  TS_YP_AN             TS_YP
#endif
#else
#define  TS_YP_AN             TS_YP
#endif

/* if the touch AD pins differ from RS and WR pins, and them paralell linked */
#if (GPIOX_PORTNUM(TS_XM) != GPIOX_PORTNUM(TS_XM_AN)) || (GPIOX_PIN(TS_XM) != GPIOX_PIN(TS_XM_AN)) || \
    (GPIOX_PORTNUM(TS_YP) != GPIOX_PORTNUM(TS_YP_AN)) || (GPIOX_PIN(TS_YP) != GPIOX_PIN(TS_YP_AN))
#define TS_AD_PIN_PARALELL
#endif
#endif

/* 8 bit temp data */
uint8_t  lcd_data8;

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

  RCC->AHBENR |= (GPIOX_CLOCK(LCD_CS) | GPIOX_CLOCK(LCD_RS) | GPIOX_CLOCK(LCD_WR) |
                  GPIOX_CLOCK(LCD_D0) | GPIOX_CLOCK(LCD_D1) | GPIOX_CLOCK(LCD_D2) | GPIOX_CLOCK(LCD_D3) |
                  GPIOX_CLOCK(LCD_D4) | GPIOX_CLOCK(LCD_D5) | GPIOX_CLOCK(LCD_D6) | GPIOX_CLOCK(LCD_D7) |
                  GPIOX_CLOCK_LCD_RST | GPIOX_CLOCK_LCD_BL  | GPIOX_CLOCK_LCD_RD);


  #if GPIOX_PORTNUM(LCD_RST) >= GPIOX_PORTNUM_A
  LCD_RST_OFF;                          /* RST = 1 */
  GPIOX_MODER(MODE_OUT, LCD_RST);
  GPIOX_OSPEEDR(MODE_SPD_LOW, LCD_RST);
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

  /* Reset the LCD */
  #if GPIOX_PORTNUM(LCD_RST) >= GPIOX_PORTNUM_A
  LCD_Delay(1);
  LCD_RST_ON;                           /* RST = 0 */
  LCD_Delay(1);
  LCD_RST_OFF;                          /* RST = 1 */
  #endif
  LCD_Delay(1);

  #ifdef ADCX

  #ifdef TS_AD_PIN_PARALELL
  RCC->AHBENR |= GPIOX_CLOCK(TS_XM_AN) | GPIOX_CLOCK(TS_YP_AN);
  GPIOX_MODER(MODE_ANALOG_INPUT, TS_XM_AN); /* XM = AN_INPUT */
  GPIOX_MODER(MODE_ANALOG_INPUT, TS_YP_AN); /* YP = AN_INPUT */
  #endif

  RCC->AHBENR |= RCC_AHBENR_ADCXEN;
  ADCX->CR &= ~ADC_CR_ADVREGEN_1;
  ADCX->CR |= ADC_CR_ADVREGEN_0;
  for (int i = 0; i < 1000; i++);
  ADC1_2_COMMON->CCR |= ADC12_CCR_CKMODE_0;
  ADCX->CR |= ADC_CR_ADCAL;
  while ((ADCX->CR & ADC_CR_ADCAL) == ADC_CR_ADCAL);
  ADCX->CR |= ADC_CR_ADEN;
  while ((ADCX->ISR & ADC_ISR_ADRD) == ADC_ISR_ADRD);
  ADCX->CFGR |= ADC_CFGR_DISCEN;
  #if TS_XM_ADCCH >= 10
  ADCX->SMPR2 |= TS_SAMPLETIME << (3 * (TS_XM_ADCCH - 10));
  #else
  ADCX->SMPR1 |= TS_SAMPLETIME << (3 * (TS_XM_ADCCH) + 3);
  #endif

  #if TS_YP_ADCCH >= 10
  ADCX->SMPR2 |= TS_SAMPLETIME << (3 * (TS_YP_ADCCH - 10));
  #else
  ADCX->SMPR1 |= TS_SAMPLETIME << (3 * (TS_YP_ADCCH) + 3);
  #endif
  #endif  /* #ifdef ADCX */
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteCmd8(uint8_t Cmd)
{
  LCD_CS_ON;
  LCD_CMD8_WRITE(Cmd);
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
  LCD_DATA8_WRITE(Data);
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
  LCD_CMD8_WRITE(Cmd);
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
  LCD_CMD8_WRITE(Cmd);

  while(Size--)
  {
    LCD_DATA8_WRITE(*pData);
    pData ++;
  }
  LCD_CS_OFF;
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteCmd8MultipleData16(uint8_t Cmd, uint16_t *pData, uint32_t Size)
{
  LCD_CS_ON;
  LCD_CMD8_WRITE(Cmd);
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
    LCD_DATA8_WRITE(*pData);
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
  uint8_t  d;
  LCD_CS_ON;
  LCD_CMD8_WRITE(Cmd);
  LCD_DIRREAD;
  while(DummySize--)
    LCD_DUMMY_READ;
  while(Size--)
  {
    LCD_DATA8_READ(d);
    *pData = d;
    pData++;
  }
  LCD_CS_OFF;
  LCD_DIRWRITE;
}

//-----------------------------------------------------------------------------
void LCD_IO_ReadCmd8MultipleData16(uint8_t Cmd, uint16_t *pData, uint32_t Size, uint32_t DummySize)
{
  uint8_t  dl, dh;
  LCD_CS_ON;
  LCD_CMD8_WRITE(Cmd);
  LCD_DIRREAD;
  while(DummySize--)
    LCD_DUMMY_READ;

  while(Size--)
  {
    LCD_DATA16_READ(dh, dl);
    *pData = (dh << 8) | dl;
    pData++;
  }
  LCD_CS_OFF;
  LCD_DIRWRITE;
}

//-----------------------------------------------------------------------------
void LCD_IO_ReadCmd8MultipleData24to16(uint8_t Cmd, uint16_t *pData, uint32_t Size, uint32_t DummySize)
{
  uint8_t  rgb888[3];
  LCD_CS_ON;
  LCD_CMD8_WRITE(Cmd);
  LCD_DIRREAD;
  while(DummySize--)
    LCD_DUMMY_READ;
  while(Size--)
  {
    LCD_DATA8_READ(rgb888[0]);
    LCD_DATA8_READ(rgb888[1]);
    LCD_DATA8_READ(rgb888[2]);
    #if LCD_REVERSE16 == 0
    *pData = ((rgb888[0] & 0xF8) << 8 | (rgb888[1] & 0xFC) << 3 | rgb888[2] >> 3);
    #else
    *pData = __REVSH((rgb888[0] & 0xF8) << 8 | (rgb888[1] & 0xFC) << 3 | rgb888[2] >> 3);
    #endif
    pData++;
  }
  LCD_CS_OFF;
  LCD_DIRWRITE;
}

//-----------------------------------------------------------------------------
void LCD_IO_ReadCmd16MultipleData8(uint16_t Cmd, uint8_t *pData, uint32_t Size, uint32_t DummySize)
{
  uint8_t  d;
  LCD_CS_ON;
  LCD_CMD16_WRITE(Cmd);
  LCD_DIRREAD;
  while(DummySize--)
    LCD_DUMMY_READ;
  while(Size--)
  {
    LCD_DATA8_READ(d);
    *pData = d;
    pData++;
  }
  LCD_CS_OFF;
  LCD_DIRWRITE;
}

//-----------------------------------------------------------------------------
void LCD_IO_ReadCmd16MultipleData16(uint16_t Cmd, uint16_t *pData, uint32_t Size, uint32_t DummySize)
{
  uint8_t  dl, dh;
  LCD_CS_ON;
  LCD_CMD16_WRITE(Cmd);
  LCD_DIRREAD;
  while(DummySize--)
    LCD_DUMMY_READ;
  while(Size--)
  {
    LCD_DATA16_READ(dh, dl);
    *pData = (dh << 8) | dl;
    pData++;
  }
  LCD_CS_OFF;
  LCD_DIRWRITE;
}

//-----------------------------------------------------------------------------
void LCD_IO_ReadCmd16MultipleData24to16(uint16_t Cmd, uint16_t *pData, uint32_t Size, uint32_t DummySize)
{
  uint8_t  rgb888[3];
  LCD_CS_ON;
  LCD_CMD16_WRITE(Cmd);
  LCD_DIRREAD;
  while(DummySize--)
    LCD_DUMMY_READ;
  while(Size--)
  {
    LCD_DATA8_READ(rgb888[0]);
    LCD_DATA8_READ(rgb888[1]);
    LCD_DATA8_READ(rgb888[2]);
    #if LCD_REVERSE16 == 0
    *pData = ((rgb888[0] & 0xF8) << 8 | (rgb888[1] & 0xFC) << 3 | rgb888[2] >> 3);
    #else
    *pData = __REVSH((rgb888[0] & 0xF8) << 8 | (rgb888[1] & 0xFC) << 3 | rgb888[2] >> 3);
    #endif
    pData++;
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

//=============================================================================
#ifdef ADCX
/* CS = 1, X+ = 0, X- = 0; Y+ = in PU, Y- = in PU */
uint8_t TS_IO_DetectToch(void)
{
  uint8_t  ret;

  GPIOX_MODER(MODE_DIGITAL_INPUT, TS_YP);/* YP = D_INPUT */
  GPIOX_MODER(MODE_DIGITAL_INPUT, TS_YM);/* YM = D_INPUT */
  GPIOX_CLR(TS_XP);                     /* XP = 0 */
  GPIOX_CLR(TS_XM);                     /* XM = 0 */

  /* pullup resistor on */
  GPIOX_PUPDR(MODE_PU_UP, TS_YP);

  LCD_IO_Delay(TS_AD_DELAY);

  if(GPIOX_IDR(TS_YP))
    ret = 0;                            /* Touchscreen is not touch */
  else
    ret = 1;                            /* Touchscreen is touch */

  /* pullup resistor off */
  GPIOX_PUPDR(MODE_PU_NONE, TS_YP);

  GPIOX_SET(TS_XP);                     /* XP = 1 */
  GPIOX_SET(TS_XM);                     /* XM = 1 */
  GPIOX_MODER(MODE_OUT, TS_YP);         /* YP = OUT */
  GPIOX_MODER(MODE_OUT, TS_YM);         /* YM = OUT */

  return ret;
}

//-----------------------------------------------------------------------------
/* read the X position */
uint16_t TS_IO_GetX(void)
{
  uint16_t ret;

  GPIOX_MODER(MODE_DIGITAL_INPUT, TS_YM);/* YM = D_INPUT */
  GPIOX_MODER(MODE_ANALOG_INPUT, TS_YP); /* YP = AN_ NPUT */
  GPIOX_CLR(TS_XP);                     /* XP = 0 */
  GPIOX_SET(TS_XM);                     /* XM = 1 */

  ADCX->SQR1 = TS_YP_ADCCH << ADC_SQR1_SQ1_Pos;
  LCD_IO_Delay(TS_AD_DELAY);
  ADCX->CR |= ADC_CR_ADSTART;
  while ((ADCX->ISR & ADC_ISR_EOC) != ADC_ISR_EOC);
  ret = ADCX->DR;

  GPIOX_SET(TS_XP);                     /* XP = 1 */
  GPIOX_MODER(MODE_OUT, TS_YP);
  GPIOX_MODER(MODE_OUT, TS_YM);

  return ret;
}

//-----------------------------------------------------------------------------
/* read the Y position */
uint16_t TS_IO_GetY(void)
{
  uint16_t ret;

  GPIOX_MODER(MODE_DIGITAL_INPUT, TS_XP);/* XP = D_INPUT */
  GPIOX_MODER(MODE_ANALOG_INPUT, TS_XM); /* XM = AN_INPUT */
  GPIOX_CLR(TS_YM);                     /* YM = 0 */
  GPIOX_SET(TS_YP);                     /* YP = 1 */

  ADCX->SQR1 = TS_XM_ADCCH << ADC_SQR1_SQ1_Pos;
  LCD_IO_Delay(TS_AD_DELAY);
  ADCX->CR |= ADC_CR_ADSTART;
  while ((ADCX->ISR & ADC_ISR_EOC) != ADC_ISR_EOC);
  ret = ADCX->DR;

  GPIOX_SET(TS_YM);                     // YM = 1
  GPIOX_MODER(MODE_OUT, TS_XP);
  GPIOX_MODER(MODE_OUT, TS_XM);

  return ret;
}

//-----------------------------------------------------------------------------
/* read the Z1 position */
uint16_t TS_IO_GetZ1(void)
{
  uint16_t ret;

  GPIOX_MODER(MODE_ANALOG_INPUT, TS_XM); /* XM = AN_INPUT */
  GPIOX_MODER(MODE_ANALOG_INPUT, TS_YP); /* YP = AN_INPUT */
  GPIOX_CLR(TS_XP);                     /* XP = 0 */
  GPIOX_SET(TS_YM);                     /* YM = 1 */

  ADCX->SQR1 = TS_YP_ADCCH << ADC_SQR1_SQ1_Pos;
  LCD_IO_Delay(TS_AD_DELAY);
  ADCX->CR |= ADC_CR_ADSTART;
  while ((ADCX->ISR & ADC_ISR_EOC) != ADC_ISR_EOC);
  ret = ADCX->DR;

  GPIOX_SET(TS_XP);                     // XP = 1
  GPIOX_MODER(MODE_OUT, TS_XM);
  GPIOX_MODER(MODE_OUT, TS_YP);

  return ret;
}

//-----------------------------------------------------------------------------
/* read the Z2 position */
uint16_t TS_IO_GetZ2(void)
{
  uint16_t ret;

  GPIOX_MODER(MODE_ANALOG_INPUT, TS_XM); /* XM = AN_INPUT */
  GPIOX_MODER(MODE_ANALOG_INPUT, TS_YP); /* YP = AN_INPUT */
  GPIOX_CLR(TS_XP);                     /* XP = 0 */
  GPIOX_SET(TS_YM);                     /* YM = 1 */

  ADCX->SQR1 = TS_XM_ADCCH << ADC_SQR1_SQ1_Pos;
  LCD_IO_Delay(TS_AD_DELAY);
  ADCX->CR |= ADC_CR_ADSTART;
  while ((ADCX->ISR & ADC_ISR_EOC) != ADC_ISR_EOC);
  ret = ADCX->DR;

  GPIOX_SET(TS_XP);                     /* XP = 1 */
  GPIOX_MODER(MODE_OUT, TS_XM);
  GPIOX_MODER(MODE_OUT, TS_YP);

  return ret;
}

#else  /* #ifdef ADCX */
__weak uint8_t   TS_IO_DetectToch(void) { return 0;}
__weak uint16_t  TS_IO_GetX(void)       { return 0;}
__weak uint16_t  TS_IO_GetY(void)       { return 0;}
__weak uint16_t  TS_IO_GetZ1(void)      { return 0;}
__weak uint16_t  TS_IO_GetZ2(void)      { return 0;}
#endif /* #else ADCX */
