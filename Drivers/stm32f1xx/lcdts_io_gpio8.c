/*
 * 8 bit paralell LCD/TOUCH GPIO driver for STM32F1
 * 5 controll pins (CS, RS, WR, RD, RST) + 8 data pins + backlight pin

 * note: whereas the LCD and Touchscreen are on the same pins,
 * therefore, the simultaneous use of Lcd and Touchscreen must be excluded!
 * In a multithreaded / intermittent environment, care must be taken to prevent collisions!
 */

/* 
 * Author: Roberto Benjami
 * version:  2020.05.27
 */

/* ADC sample time 
   - 0: 1.5cycles, 1: 7.5c, 2:13.5c, 3:28.5c, 4:41.5c, 5:55.5c, 6:71.5c, 7:239.5cycles
*/
#define  TS_SAMPLETIME        3

/* In the LCD board are multifunction pins: */
#define  TS_XP                LCD_D6
#define  TS_XM                LCD_RS
#define  TS_YP                LCD_WR
#define  TS_YM                LCD_D7

#include "main.h"
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

/* GPIO mode (PP: push-pull, OD: open drain, FF: input floating) */
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
/* command/data pin setting */
#define LCD_RS_CMD            GPIOX_ODR(LCD_RS) = 0
#define LCD_RS_DATA           GPIOX_ODR(LCD_RS) = 1

/* reset pin setting */
#define LCD_RST_ON            GPIOX_ODR(LCD_RST) = 0
#define LCD_RST_OFF           GPIOX_ODR(LCD_RST) = 1

/* chip select pin setting */
#define LCD_CS_ON             GPIOX_ODR(LCD_CS) = 0
#define LCD_CS_OFF            GPIOX_ODR(LCD_CS) = 1

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
#if GPIOX_PIN(LCD_D0) == 0
/* LCD data pins on 0..7 pin (ex. B0,B1,B2,B3,B4,B5,B6,B7) */
#define LCD_AUTOOPT  1
#elif GPIOX_PIN(LCD_D0) == 8
/* LCD data pins on 8..15 pin (ex. B8,B9,B10,B11,B12,B13,B14,B15) */
#define LCD_AUTOOPT  2
#else
/* LCD data pins on n..n+7 pin (ex. B6,B7,B8,B9,B10,B11,B12,B13) */
#define LCD_AUTOOPT  3
#define LCD_DATA_DIRSET_(a,b,c)   *(uint64_t *)GPIO ## b ## _BASE = (*(uint64_t *)GPIO ## b ## _BASE & ~(0xFFFFFFFFLL << (c << 2))) | ((uint64_t)a << (c << 2))
#define LCD_DATA_DIRSET(a, b)     LCD_DATA_DIRSET_(a, b)
#endif
#endif /* D0..D7 pin order */
#endif /* D0..D7 port same */

//-----------------------------------------------------------------------------
/* data pins set to output direction */
#ifndef LCD_DIRWRITE
#if     (LCD_AUTOOPT == 1)
#define LCD_DIRWRITE  GPIOX_PORT(LCD_D0)->CRL = 0x33333333
#elif   (LCD_AUTOOPT == 2)
#define LCD_DIRWRITE  GPIOX_PORT(LCD_D0)->CRH = 0x33333333
#elif   (LCD_AUTOOPT == 3)
#define LCD_DIRWRITE  LCD_DATA_DIRSET(0x33333333, LCD_D0)
#else
#define LCD_DIRWRITE { \
  GPIOX_MODE(MODE_PP_OUT_50MHZ, LCD_D0); GPIOX_MODE(MODE_PP_OUT_50MHZ, LCD_D1);\
  GPIOX_MODE(MODE_PP_OUT_50MHZ, LCD_D2); GPIOX_MODE(MODE_PP_OUT_50MHZ, LCD_D3);\
  GPIOX_MODE(MODE_PP_OUT_50MHZ, LCD_D4); GPIOX_MODE(MODE_PP_OUT_50MHZ, LCD_D5);\
  GPIOX_MODE(MODE_PP_OUT_50MHZ, LCD_D6); GPIOX_MODE(MODE_PP_OUT_50MHZ, LCD_D7);}
#endif
#endif
 
//-----------------------------------------------------------------------------
/* data pins set to input direction */
#ifndef LCD_DIRREAD
#if     LCD_AUTOOPT == 1
#define LCD_DIRREAD  GPIOX_PORT(LCD_D0)->CRL = 0x44444444
#elif   LCD_AUTOOPT == 2
#define LCD_DIRREAD  GPIOX_PORT(LCD_D0)->CRH = 0x44444444
#elif   (LCD_AUTOOPT == 3)
#define LCD_DIRREAD  LCD_DATA_DIRSET(0x44444444, LCD_D0)
#else
#define LCD_DIRREAD { \
  GPIOX_MODE(MODE_FF_DIGITAL_INPUT, LCD_D0); GPIOX_MODE(MODE_FF_DIGITAL_INPUT, LCD_D1);\
  GPIOX_MODE(MODE_FF_DIGITAL_INPUT, LCD_D2); GPIOX_MODE(MODE_FF_DIGITAL_INPUT, LCD_D3);\
  GPIOX_MODE(MODE_FF_DIGITAL_INPUT, LCD_D4); GPIOX_MODE(MODE_FF_DIGITAL_INPUT, LCD_D5);\
  GPIOX_MODE(MODE_FF_DIGITAL_INPUT, LCD_D6); GPIOX_MODE(MODE_FF_DIGITAL_INPUT, LCD_D7);}
#endif
#endif 

//-----------------------------------------------------------------------------
/* 8 bit data write to the data pins */
#ifndef LCD_WRITE
#ifdef  LCD_AUTOOPT
#define LCD_WRITE(dt) { \
  GPIOX_PORT(LCD_D0)->BSRR = (dt << GPIOX_PIN(LCD_D0)) | (0xFF << (GPIOX_PIN(LCD_D0) + 16));}
#else
#define LCD_WRITE(dt) {;                  \
  GPIOX_ODR(LCD_D0) = BITBAND_ACCESS(dt, 0); \
  GPIOX_ODR(LCD_D1) = BITBAND_ACCESS(dt, 1); \
  GPIOX_ODR(LCD_D2) = BITBAND_ACCESS(dt, 2); \
  GPIOX_ODR(LCD_D3) = BITBAND_ACCESS(dt, 3); \
  GPIOX_ODR(LCD_D4) = BITBAND_ACCESS(dt, 4); \
  GPIOX_ODR(LCD_D5) = BITBAND_ACCESS(dt, 5); \
  GPIOX_ODR(LCD_D6) = BITBAND_ACCESS(dt, 6); \
  GPIOX_ODR(LCD_D7) = BITBAND_ACCESS(dt, 7); }
#endif
#endif 

//-----------------------------------------------------------------------------
/* 8 bit data read from the data pins */
#ifndef LCD_READ
#ifdef  LCD_AUTOOPT
#define LCD_READ(dt) {                          \
  dt = GPIOX_PORT(LCD_D0)->IDR >> GPIOX_PIN(LCD_D0); }
#else
#define LCD_READ(dt) {                  \
  BITBAND_ACCESS(dt, 0) = GPIOX_IDR(LCD_D0); \
  BITBAND_ACCESS(dt, 1) = GPIOX_IDR(LCD_D1); \
  BITBAND_ACCESS(dt, 2) = GPIOX_IDR(LCD_D2); \
  BITBAND_ACCESS(dt, 3) = GPIOX_IDR(LCD_D3); \
  BITBAND_ACCESS(dt, 4) = GPIOX_IDR(LCD_D4); \
  BITBAND_ACCESS(dt, 5) = GPIOX_IDR(LCD_D5); \
  BITBAND_ACCESS(dt, 6) = GPIOX_IDR(LCD_D6); \
  BITBAND_ACCESS(dt, 7) = GPIOX_IDR(LCD_D7); }
#endif
#endif

//-----------------------------------------------------------------------------
/* Write / Read spd */
#if     LCD_WRITE_DELAY == 0
#define LCD_WR_DELAY
#elif   LCD_WRITE_DELAY == 1
#define LCD_WR_DELAY          GPIOX_ODR(LCD_WR) = 0
#else
#define LCD_WR_DELAY          LCD_IO_Delay(LCD_WRITE_DELAY - 2)
#endif
#if     LCD_READ_DELAY == 0
#define LCD_RD_DELAY
#elif   LCD_READ_DELAY == 1
#define LCD_RD_DELAY          GPIOX_ODR(LCD_RD) = 0
#else
#define LCD_RD_DELAY          LCD_IO_Delay(LCD_READ_DELAY - 2)
#endif 

#define LCD_DUMMY_READ        { GPIOX_ODR(LCD_RD) = 0; LCD_RD_DELAY; GPIOX_ODR(LCD_RD) = 1; }
#define LCD_DATA8_WRITE(dt)   { lcd_data8 = dt; LCD_WRITE(lcd_data8); GPIOX_ODR(LCD_WR) = 0; LCD_WR_DELAY; GPIOX_ODR(LCD_WR) = 1; }
#define LCD_DATA8_READ(dt)    { GPIOX_ODR(LCD_RD) = 0; LCD_RD_DELAY; LCD_READ(dt); GPIOX_ODR(LCD_RD) = 1; }
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
#define  ADCX  ADC1
#define  RCC_APB2ENR_ADCXEN  RCC_APB2ENR_ADC1EN
#endif

#if TS_ADC == 2
#define  ADCX  ADC2
#define  RCC_APB2ENR_ADCXEN  RCC_APB2ENR_ADC2EN
#endif

#if TS_ADC == 3
#define  ADCX  ADC3
#define  RCC_APB2ENR_ADCXEN  RCC_APB2ENR_ADC3EN
#endif

//-----------------------------------------------------------------------------
/* 8 bit data variable */
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
	
  RCC->APB2ENR |= (GPIOX_CLOCK(LCD_CS) | GPIOX_CLOCK(LCD_RS) | GPIOX_CLOCK(LCD_WR) |
                   GPIOX_CLOCK(LCD_D0) | GPIOX_CLOCK(LCD_D1) | GPIOX_CLOCK(LCD_D2) | GPIOX_CLOCK(LCD_D3) |
                   GPIOX_CLOCK(LCD_D4) | GPIOX_CLOCK(LCD_D5) | GPIOX_CLOCK(LCD_D6) | GPIOX_CLOCK(LCD_D7) |
                   GPIOX_CLOCK_LCD_RST | GPIOX_CLOCK_LCD_BL  | GPIOX_CLOCK_LCD_RD);
	
  #if GPIOX_PORTNUM(LCD_RST) >= GPIOX_PORTNUM_A
  LCD_RST_OFF;                          // RST = 1
  GPIOX_MODE(MODE_PP_OUT_2MHZ, LCD_RST);
  #endif 
  
  #if GPIOX_PORTNUM(LCD_BL) >= GPIOX_PORTNUM_A    /* backlight */
  GPIOX_MODE(MODE_PP_OUT_2MHZ, LCD_BL);
  LCD_IO_Bl_OnOff(1);
  #endif

  GPIOX_ODR(LCD_CS) = 1;                /* CS = 1 */
  LCD_RS_DATA;                          /* RS = 1 */
  GPIOX_ODR(LCD_WR) = 1;                /* WR = 1 */
  #if GPIOX_PORTNUM(LCD_RD) >=  GPIOX_PORTNUM_A
  GPIOX_ODR(LCD_RD) = 1;                /* RD = 1 */
  #endif

  GPIOX_MODE(MODE_PP_OUT_50MHZ, LCD_CS);
  #if GPIOX_PORTNUM(LCD_RD) >=  GPIOX_PORTNUM_A
  GPIOX_MODE(MODE_PP_OUT_50MHZ, LCD_RD);
  #endif
  GPIOX_MODE(MODE_PP_OUT_50MHZ, LCD_WR);
  GPIOX_MODE(MODE_PP_OUT_50MHZ, LCD_RS);

  LCD_DIRWRITE;                         /* data pins set the output direction */

  /* Reset the LCD */
  #if GPIOX_PORTNUM(LCD_RST) >= GPIOX_PORTNUM_A
  LCD_Delay(1);
  LCD_RST_ON;                           /* RST = 0 */
  LCD_Delay(1);
  LCD_RST_OFF;                          /* RST = 1 */
  #endif
  LCD_Delay(1);

  #if TS_ADC > 0
  RCC->CFGR |= RCC_CFGR_ADCPRE_DIV6;    /* ADC clock = cpuclock / 6 */
  RCC->APB2ENR |= RCC_APB2ENR_ADCXEN;
  LCD_Delay(1);
  ADCX->CR1 = ADC_CR1_DISCEN;
  ADCX->CR2 = (7 << ADC_CR2_EXTSEL_Pos) | ADC_CR2_EXTTRIG | ADC_CR2_ADON;
  #if TS_XM_ADCCH >= 10
  ADCX->SMPR1 |= TS_SAMPLETIME << (3 * (TS_XM_ADCCH - 10));
  #else
  ADCX->SMPR2 |= TS_SAMPLETIME << (3 * (TS_XM_ADCCH));
  #endif
  #if TS_YP_ADCCH >= 10
  ADCX->SMPR1 |= TS_SAMPLETIME << (3 * (TS_YP_ADCCH - 10));
  #else
  ADCX->SMPR2 |= TS_SAMPLETIME << (3 * (TS_YP_ADCCH));
  #endif
  ADCX->CR2 |= ADC_CR2_CAL;
  while(ADCX->CR2 & ADC_CR2_CAL);
  #endif /* #if TS_ADC > 0 */
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

//-----------------------------------------------------------------------------
#ifdef ADCX
/* CS = 1, X+ = 0, X- = 0; Y+ = in PU, Y- = in PU */
uint8_t TS_IO_DetectToch(void)
{
  uint8_t  ret;

  GPIOX_MODE(MODE_PU_DIGITAL_INPUT, TS_YP);/* YP = D_INPUT PULL */
  GPIOX_MODE(MODE_FF_DIGITAL_INPUT, TS_YM);/* YM = D_INPUT */
  GPIOX_ODR(TS_XP) = 0;                 /* XP = 0 */
  GPIOX_ODR(TS_XM) = 0;                 /* XM = 0 */

  /* pullup resistor on */
  GPIOX_ODR(TS_YP) = 1;                 /* YP = 1 (pullup) */

  LCD_IO_Delay(TS_AD_DELAY);

  if(GPIOX_IDR(TS_YP))
    ret = 0;                            /* Touchscreen is not touch */
  else
    ret = 1;                            /* Touchscreen is touch */

  GPIOX_ODR(TS_XP) = 1;                 /* XP = 1 */
  GPIOX_ODR(TS_XM) = 1;                 /* XM = 1 */
  GPIOX_MODE(MODE_PP_OUT_50MHZ, TS_YP); /* YP = OUT */
  GPIOX_MODE(MODE_PP_OUT_50MHZ, TS_YM); /* YM = OUT */

  return ret;
}

//-----------------------------------------------------------------------------
/* read the X position */
uint16_t TS_IO_GetX(void)
{
  uint16_t ret;

  GPIOX_MODE(MODE_FF_DIGITAL_INPUT, TS_YM);/* YM = D_INPUT */
  GPIOX_MODE(MODE_ANALOG_INPUT, TS_YP);    /* YP = AN_INPUT */
  GPIOX_ODR(TS_XP) = 0;                 /* XP = 0 */
  GPIOX_ODR(TS_XM) = 1;                 /* XM = 1 */

  ADCX->SQR3 = TS_YP_ADCCH;
  CLEAR_BIT(ADCX->SR, ADC_SR_EOC);
  LCD_IO_Delay(TS_AD_DELAY);
  SET_BIT(ADCX->CR2, ADC_CR2_SWSTART);
  while(!READ_BIT(ADCX->SR, ADC_SR_EOC));
  ret = ADCX->DR;

  GPIOX_ODR(TS_XP) = 1;                 /* XM = 1 */
  GPIOX_MODE(MODE_PP_OUT_50MHZ, TS_YP);
  GPIOX_MODE(MODE_PP_OUT_50MHZ, TS_YM);

  return ret;
}

//-----------------------------------------------------------------------------
/* read the Y position */
uint16_t TS_IO_GetY(void)
{
  uint16_t ret;

  GPIOX_MODE(MODE_FF_DIGITAL_INPUT, TS_XP);/* XP = D_INPUT */
  GPIOX_MODE(MODE_ANALOG_INPUT, TS_XM);    /* XM = AN_INPUT */
  GPIOX_ODR(TS_YM) = 0;                 /* YP = 0 */
  GPIOX_ODR(TS_YP) = 1;                 /* YM = 1 */

  ADCX->SQR3 = TS_XM_ADCCH;
  CLEAR_BIT(ADCX->SR, ADC_SR_EOC);
  LCD_IO_Delay(TS_AD_DELAY);
  SET_BIT(ADCX->CR2, ADC_CR2_SWSTART);
  while(!READ_BIT(ADCX->SR, ADC_SR_EOC));
  ret = ADCX->DR;

  GPIOX_ODR(TS_YM) = 1;
  GPIOX_MODE(MODE_PP_OUT_50MHZ, TS_XP);
  GPIOX_MODE(MODE_PP_OUT_50MHZ, TS_XM);

  return ret;
}

//-----------------------------------------------------------------------------
/* read the Z1 position */
uint16_t TS_IO_GetZ1(void)
{
  uint16_t ret;

  GPIOX_MODE(MODE_ANALOG_INPUT, TS_XM); /* XM = AN_INPUT */
  GPIOX_MODE(MODE_ANALOG_INPUT, TS_YP); /* YP = AN_INPUT */
  GPIOX_ODR(TS_XP) = 0;                 /* XP = 0 */
  GPIOX_ODR(TS_YM) = 1;                 /* YM = 1 */

  ADCX->SQR3 = TS_YP_ADCCH;
  CLEAR_BIT(ADCX->SR, ADC_SR_EOC);
  LCD_IO_Delay(TS_AD_DELAY);
  SET_BIT(ADCX->CR2, ADC_CR2_SWSTART);
  while(!READ_BIT(ADCX->SR, ADC_SR_EOC));
  ret = ADCX->DR;

  GPIOX_ODR(TS_XP) = 1;
  GPIOX_MODE(MODE_PP_OUT_50MHZ, TS_XM);
  GPIOX_MODE(MODE_PP_OUT_50MHZ, TS_YP);

  return ret;
}

//-----------------------------------------------------------------------------
/* read the Z2 position */
uint16_t TS_IO_GetZ2(void)
{
  uint16_t ret;

  GPIOX_MODE(MODE_ANALOG_INPUT, TS_XM); /* XM = AN_INPUT */
  GPIOX_MODE(MODE_ANALOG_INPUT, TS_YP); /* YP = AN_INPUT */
  GPIOX_ODR(TS_XP) = 0;                 /* XP = 0 */
  GPIOX_ODR(TS_YM) = 1;                 /* YM = 1 */

  ADCX->SQR3 = TS_XM_ADCCH;
  CLEAR_BIT(ADCX->SR, ADC_SR_EOC);
  LCD_IO_Delay(TS_AD_DELAY);
  SET_BIT(ADCX->CR2, ADC_CR2_SWSTART);
  while(!READ_BIT(ADCX->SR, ADC_SR_EOC));
  ret = ADCX->DR;

  GPIOX_ODR(TS_XP) = 1;
  GPIOX_MODE(MODE_PP_OUT_50MHZ, TS_XM);
  GPIOX_MODE(MODE_PP_OUT_50MHZ, TS_YP);

  return ret;
}

//-----------------------------------------------------------------------------
#else /* #ifdef ADCX */
__weak uint8_t   TS_IO_DetectToch(void) { return 0;}
__weak uint16_t  TS_IO_GetX(void)       { return 0;}
__weak uint16_t  TS_IO_GetY(void)       { return 0;}
__weak uint16_t  TS_IO_GetZ1(void)      { return 0;}
__weak uint16_t  TS_IO_GetZ2(void)      { return 0;}
#endif 
