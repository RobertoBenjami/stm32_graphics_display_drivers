/*
 * XPT2046 touch driver STM32L5 (untested)
 * author: Roberto Benjami
 * v.2021.05.24
 *
 * - hardware, software SPI
 */
#include <stdlib.h>
#include <stdio.h>

//-----------------------------------------------------------------------------
#include "main.h"
#include "lcd.h"
#include "ts_xpt2046.h"

//=============================================================================

/* if not used the TS_IRQ pin -> Z1-Z2 touch sensitivy */
#define   TS_ZSENS            128

//=============================================================================
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

//=============================================================================
#ifdef  __GNUC__
#pragma GCC push_options
#pragma GCC optimize("O0")
#elif   defined(__CC_ARM)
#pragma push
#pragma O0
#endif
void TS_IO_Delay(uint32_t c)
{
  while(c--);
}
#ifdef  __GNUC__
#pragma GCC pop_options
#elif   defined(__CC_ARM)
#pragma pop
#endif

//=============================================================================
/* TS chip select pin set */
#define TS_CS_ON              GPIOX_CLR(TS_CS)
#define TS_CS_OFF             GPIOX_SET(TS_CS)

#if TS_SPI == 0
/* Software SPI */

/* SPI delay (0: none, 1: NOP, 2: clock pin keeping, 3.. TS_IO_Delay) */
#if     TS_SPI_SPD == 0
#define TS_DELAY
#elif   TS_SPI_SPD == 1
#define TS_DELAY              __NOP()
#elif   TS_SPI_SPD == 2
#define TS_DELAY              GPIOX_ODR(TS_SCK) = 0
#else
#define TS_DELAY              TS_IO_Delay(TS_SPI_SPD - 3)
#endif

#define TS_WRITE_CLK          GPIOX_CLR(TS_SCK); TS_DELAY; GPIOX_SET(TS_SCK);
#define TS_READ_CLK           GPIOX_SET(TS_SCK); GPIOX_CLR(TS_SCK); TS_DELAY;

//-----------------------------------------------------------------------------
void TsWrite8(uint8_t d8)
{
  if(d8 & 0x80) GPIOX_SET(TS_MOSI); else GPIOX_CLR(TS_MOSI);
  TS_WRITE_CLK;
  if(d8 & 0x40) GPIOX_SET(TS_MOSI); else GPIOX_CLR(TS_MOSI);
  TS_WRITE_CLK;
  if(d8 & 0x20) GPIOX_SET(TS_MOSI); else GPIOX_CLR(TS_MOSI);
  TS_WRITE_CLK;
  if(d8 & 0x10) GPIOX_SET(TS_MOSI); else GPIOX_CLR(TS_MOSI);
  TS_WRITE_CLK;
  if(d8 & 0x08) GPIOX_SET(TS_MOSI); else GPIOX_CLR(TS_MOSI);
  TS_WRITE_CLK;
  if(d8 & 0x04) GPIOX_SET(TS_MOSI); else GPIOX_CLR(TS_MOSI);
  TS_WRITE_CLK;
  if(d8 & 0x02) GPIOX_SET(TS_MOSI); else GPIOX_CLR(TS_MOSI);
  TS_WRITE_CLK;
  if(d8 & 0x01) GPIOX_SET(TS_MOSI); else GPIOX_CLR(TS_MOSI);
  TS_WRITE_CLK;
}

//-----------------------------------------------------------------------------
uint16_t TsRead16(void)
{
  uint16_t d16 = 0;
  GPIOX_CLR(TS_SCK);
  TS_DELAY;
  if(GPIOX_IDR(TS_MISO)) d16 = 0x8000; else d16 = 0;
  TS_READ_CLK;
  if(GPIOX_IDR(TS_MISO)) d16 |= 0x4000;
  TS_READ_CLK;
  if(GPIOX_IDR(TS_MISO)) d16 |= 0x2000;
  TS_READ_CLK;
  if(GPIOX_IDR(TS_MISO)) d16 |= 0x1000;
  TS_READ_CLK;
  if(GPIOX_IDR(TS_MISO)) d16 |= 0x800;
  TS_READ_CLK;
  if(GPIOX_IDR(TS_MISO)) d16 |= 0x400;
  TS_READ_CLK;
  if(GPIOX_IDR(TS_MISO)) d16 |= 0x200;
  TS_READ_CLK;
  if(GPIOX_IDR(TS_MISO)) d16 |= 0x100;
  TS_READ_CLK;
  if(GPIOX_IDR(TS_MISO)) d16 |= 0x80;
  TS_READ_CLK;
  if(GPIOX_IDR(TS_MISO)) d16 |= 0x40;
  TS_READ_CLK;
  if(GPIOX_IDR(TS_MISO)) d16 |= 0x20;
  TS_READ_CLK;
  if(GPIOX_IDR(TS_MISO)) d16 |= 0x10;
  TS_READ_CLK;
  if(GPIOX_IDR(TS_MISO)) d16 |= 0x8;
  TS_READ_CLK;
  if(GPIOX_IDR(TS_MISO)) d16 |= 0x4;
  TS_READ_CLK;
  if(GPIOX_IDR(TS_MISO)) d16 |= 0x2;
  TS_READ_CLK;
  if(GPIOX_IDR(TS_MISO)) d16 |= 0x1;
  GPIOX_SET(TS_SCK);
  return d16;
}

#else
/* Hardware SPI */

#if TS_SPI == 1
#define SPIX                  SPI1
#define TS_SPI_RCC_EN         RCC->APB2ENR |= RCC_APB2ENR_SPI1EN
#elif TS_SPI == 2
#define SPIX                  SPI2
#define TS_SPI_RCC_EN         RCC->APB1ENR1 |= RCC_APB1ENR1_SPI2EN
#elif TS_SPI == 3
#define SPIX                  SPI3
#define TS_SPI_RCC_EN         RCC->APB1ENR1 |= RCC_APB1ENR1_SPI3EN
#endif

//-----------------------------------------------------------------------------
void TsWrite8(uint8_t d8)
{
  *(volatile uint8_t *)&SPIX->DR = d8;
  TS_IO_Delay(2);
  while(SPIX->SR & SPI_SR_BSY);
  d8 = *(volatile uint8_t *)&SPIX->DR;
}

//-----------------------------------------------------------------------------
uint16_t TsRead16(void)
{
  uint8_t d8_h, d8_l;
  *(volatile uint8_t *)&SPIX->DR = 0;
  TS_IO_Delay(2);
  while(SPIX->SR & SPI_SR_BSY_Pos);
  d8_h = *(uint8_t *)&SPIX->DR;
  *(volatile uint8_t *)&SPIX->DR = 0;
  TS_IO_Delay(2);
  while(SPIX->SR & SPI_SR_BSY_Pos);
  d8_l = *(uint8_t *)&SPIX->DR;
  return (d8_h << 8) + d8_l;
}

#endif  /* #if TS_SPI == 0 */

//-----------------------------------------------------------------------------
void TS_IO_Init(void)
{
  #if GPIOX_PORTNUM(TS_IRQ) >= GPIOX_PORTNUM_A
  #define GPIOX_CLOCK_TS_IRQ   GPIOX_CLOCK(TS_IRQ)
  #else
  #define GPIOX_CLOCK_TS_IRQ   0
  #endif

  RCC->AHB1ENR |= GPIOX_CLOCK(TS_CS) | GPIOX_CLOCK(TS_SCK) | GPIOX_CLOCK(TS_MOSI) | GPIOX_CLOCK(TS_MISO) | GPIOX_CLOCK_TS_IRQ;

  TS_CS_OFF;
  GPIOX_SET(TS_SCK);
  GPIOX_SET(TS_MOSI);
  #if GPIOX_PORTNUM(TS_IRQ) >= GPIOX_PORTNUM_A
  GPIOX_MODER(MODE_DIGITAL_INPUT, TS_IRQ);
  #endif
  GPIOX_MODER(MODE_OUT, TS_CS);
  GPIOX_OSPEEDR(MODE_SPD_VHIGH, TS_CS);
  GPIOX_OSPEEDR(MODE_SPD_VHIGH, TS_SCK);
  GPIOX_OSPEEDR(MODE_SPD_VHIGH, TS_MOSI);
  #if TS_SPI == 0
  /* Software SPI */
  GPIOX_MODER(MODE_OUT, TS_SCK);
  GPIOX_MODER(MODE_OUT, TS_MOSI);
  GPIOX_MODER(MODE_DIGITAL_INPUT, TS_MISO);
  #else
  /* Hardware SPI */
  GPIOX_AFR(TS_SPI_AFR, TS_SCK);
  GPIOX_MODER(MODE_ALTER, TS_SCK);
  GPIOX_AFR(TS_SPI_AFR, TS_MOSI);
  GPIOX_MODER(MODE_ALTER, TS_MOSI);
  GPIOX_AFR(TS_SPI_AFR, TS_MISO);
  GPIOX_MODER(MODE_ALTER, TS_MISO);

  TS_SPI_RCC_EN;
  SPIX->CR1 = SPI_CR1_CPHA | SPI_CR1_CPOL | SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI | (TS_SPI_SPD << SPI_CR1_BR_Pos);
  SPIX->CR1 |= SPI_CR1_SPE;
  #endif
}

//-----------------------------------------------------------------------------
/* read the X position */
uint16_t TS_IO_GetX(void)
{
  uint16_t ret;
  TS_CS_ON;
  TsWrite8(0x90);
  ret = TsRead16();
  TS_CS_OFF;
  return ret >> 3;
}

//-----------------------------------------------------------------------------
/* read the Y position */
uint16_t TS_IO_GetY(void)
{
  uint16_t ret;
  TS_CS_ON;
  TsWrite8(0xD0);
  ret = TsRead16();
  TS_CS_OFF;
  return ret >> 3;
}

//-----------------------------------------------------------------------------
/* read the Z1 position */
uint16_t TS_IO_GetZ1(void)
{
  uint16_t ret;
  TS_CS_ON;
  TsWrite8(0xB0);
  ret = TsRead16();
  TS_CS_OFF;
  return ret >> 3;
}

//-----------------------------------------------------------------------------
/* read the Z2 position */
uint16_t TS_IO_GetZ2(void)
{
  uint16_t ret;
  TS_CS_ON;
  TsWrite8(0xC0);
  ret = TsRead16();
  TS_CS_OFF;
  return ret >> 3;
}

//-----------------------------------------------------------------------------
/* return:
   - 0 : touchscreen is not pressed
   - 1 : touchscreen is pressed */
uint8_t TS_IO_DetectToch(void)
{
  uint8_t  ret;
  static uint8_t ts_inited = 0;
  if(!ts_inited)
  {	  
    TS_IO_Init();
    ts_inited = 1;
  }	
  #if GPIOX_PORTNUM(TS_IRQ) >= GPIOX_PORTNUM_A
  if(GPIOX_IDR(TS_IRQ))
    ret = 0;
  else
    ret = 1;
  #else
  if((TS_IO_GetZ1() > TS_ZSENS) || (TS_IO_GetZ2() < (4095 - TS_ZSENS)))
    ret = 1;
  else
    ret = 0;
  #endif
  return ret;
}
