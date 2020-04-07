/*
 * 8 bit paralell LCD GPIO driver for STM32F2
 * 5 controll pins (CS, RS, WR, RD, RST) + 8 data pins + backlight pin
 */

/* 
 * Author: Roberto Benjami
 * version:  2020.03
 */

#include "main.h"
#include "lcd.h"
#include "lcd_io_gpio8.h"

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

#define GPIOX_ODR_(a, b)      BITBAND_ACCESS(GPIO ## a ->ODR, b)
#define GPIOX_ODR(a)          GPIOX_ODR_(a)

#define GPIOX_IDR_(a, b)      BITBAND_ACCESS(GPIO ## a ->IDR, b)
#define GPIOX_IDR(a)          GPIOX_IDR_(a)

#define GPIOX_LINE_(a, b)     EXTI_Line ## b
#define GPIOX_LINE(a)         GPIOX_LINE_(a)

#define GPIOX_PORTSRC_(a, b)  GPIO_PortSourceGPIO ## a
#define GPIOX_PORTSRC(a)      GPIOX_PORTSRC_(a)

#define GPIOX_PINSRC_(a, b)   GPIO_PinSource ## b
#define GPIOX_PINSRC(a)       GPIOX_PINSRC_(a)

// GPIO Ports Clock Enable
#define GPIOX_CLOCK_(a, b)    RCC_AHB1ENR_GPIO ## a ## EN
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
#define LCD_CMD16_WRITE(cmd16)  { LCD_RS_CMD; LCD_DATA8_WRITE(cmd16 >> 8); LCD_DATA8_WRITE(cmd16); LCD_RS_DATA; }
#define LCD_DATA16_WRITE(d16)   { LCD_DATA8_WRITE(d16 >> 8); LCD_DATA8_WRITE(d16); }
#define LCD_DATA16_READ(dh, dl) { LCD_DATA8_READ(dh); LCD_DATA8_READ(dl); }
#else
#define LCD_CMD16_WRITE(cmd)    { LCD_RS_CMD; LCD_DATA8_WRITE(cmd); LCD_DATA8_WRITE(cmd >> 8); LCD_RS_DATA; }
#define LCD_DATA16_WRITE(data)  { LCD_DATA8_WRITE(data); LCD_DATA8_WRITE(data >> 8); }
#define LCD_DATA16_READ(dh, dl) { LCD_DATA8_READ(dl); LCD_DATA8_READ(dh); }
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
  if(Bl)
    GPIOX_ODR(LCD_BL) = LCD_BLON;
  else
    GPIOX_ODR(LCD_BL) = 1 - LCD_BLON;
  #endif
}

//-----------------------------------------------------------------------------
void LCD_IO_Init(void)
{
  #if GPIOX_PORTNUM(LCD_RST) >= GPIOX_PORTNUM_A
  RCC->AHB1ENR |= (GPIOX_CLOCK(LCD_CS) | GPIOX_CLOCK(LCD_RS) | GPIOX_CLOCK(LCD_WR) | GPIOX_CLOCK(LCD_RD) | GPIOX_CLOCK(LCD_RST) |
                   GPIOX_CLOCK(LCD_D0) | GPIOX_CLOCK(LCD_D1) | GPIOX_CLOCK(LCD_D2) | GPIOX_CLOCK(LCD_D3) |
                   GPIOX_CLOCK(LCD_D4) | GPIOX_CLOCK(LCD_D5) | GPIOX_CLOCK(LCD_D6) | GPIOX_CLOCK(LCD_D7));
  LCD_RST_OFF;                          // RST = 1
  GPIOX_MODER(MODE_OUT, LCD_RST);
  #else
  RCC->AHB1ENR |= (GPIOX_CLOCK(LCD_CS) | GPIOX_CLOCK(LCD_RS) | GPIOX_CLOCK(LCD_WR) | GPIOX_CLOCK(LCD_RD) |
                   GPIOX_CLOCK(LCD_D0) | GPIOX_CLOCK(LCD_D1) | GPIOX_CLOCK(LCD_D2) | GPIOX_CLOCK(LCD_D3) |
                   GPIOX_CLOCK(LCD_D4) | GPIOX_CLOCK(LCD_D5) | GPIOX_CLOCK(LCD_D6) | GPIOX_CLOCK(LCD_D7));
  #endif

  #if GPIOX_PORTNUM(LCD_BL) >= GPIOX_PORTNUM_A
  RCC->AHB1ENR |= GPIOX_CLOCK(LCD_BL);
  GPIOX_ODR(LCD_BL) = LCD_BLON;
  GPIOX_MODER(MODE_OUT, LCD_BL);
  #endif

  GPIOX_ODR(LCD_CS) = 1;                /* CS = 1 */
  LCD_RS_DATA;                          /* RS = 1 */
  GPIOX_ODR(LCD_WR) = 1;                /* WR = 1 */
  GPIOX_ODR(LCD_RD) = 1;                /* RD = 1 */

  GPIOX_MODER(MODE_OUT, LCD_CS);
  GPIOX_MODER(MODE_OUT, LCD_RS);
  GPIOX_MODER(MODE_OUT, LCD_WR);
  GPIOX_MODER(MODE_OUT, LCD_RD);

  LCD_DIRWRITE;                         /* data pins set the output direction */

  /* GPIO speed */
  GPIOX_OSPEEDR(MODE_SPD_VHIGH, LCD_CS);
  GPIOX_OSPEEDR(MODE_SPD_VHIGH, LCD_RS);
  GPIOX_OSPEEDR(MODE_SPD_VHIGH, LCD_WR);
  GPIOX_OSPEEDR(MODE_SPD_VHIGH, LCD_RD);
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
