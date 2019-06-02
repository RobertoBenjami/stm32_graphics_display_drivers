/* 8 bites párhuzamos LCD/TOUCH FSMC driver STM32F4-re
5 vezárlöláb (CS, RS, WR, RD, RST) + 8 adatláb

Figyelem: mivel azonos lábakon van az Lcd ás a Touchscreen,
ezért ezek ki kell zárni az Lcd és a Touchscreen egyidejü használatát!
Tábbszálas/megszakitásos környezetben igy gondoskodni kell az összeakadások megelözéséröl!

Kászitö: Roberto Benjami
verzio:  2019.02 */

/* CS láb vezérlési stratégia
   - 0: CS láb minden irás/olvasás müvelet során állitva van (igy a touchscreen olvasásakor nem szükséges lekapcsolni
   - 1: CS láb folyamatosan 0-ba van állitva (a touchscreen olvasásakor ezért le kell kapcsolni)
*/

// ADC sample time (0:3cycles, 1:15c, 2:28c, 3:55c, 4:84c, 5:112c, 6:144c, 7:480cycles)
#define  TS_SAMPLETIME        2

#include "main.h"
#include "lcdts_io8p_fsmc.h"

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

// portláb mádok (PP: push-pull, OD: open drain, FF: input floating)
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

#define BITBAND_ACCESS(variable, bitnumber) *(volatile uint32_t*)(((uint32_t)&variable & 0xF0000000) + 0x2000000 + (((uint32_t)&variable & 0x000FFFFF) << 5) + (bitnumber << 2))

#define GPIOX_PORT_(a, b)     GPIO ## a
#define GPIOX_PORT(a)         GPIOX_PORT_(a)

#define GPIOX_PIN_(a, b)      b
#define GPIOX_PIN(x)          GPIOX_PIN_(x)

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

#define GPIOX_PORTTONUM_A     1
#define GPIOX_PORTTONUM_B     2
#define GPIOX_PORTTONUM_C     3
#define GPIOX_PORTTONUM_D     4
#define GPIOX_PORTTONUM_E     5
#define GPIOX_PORTTONUM_F     6
#define GPIOX_PORTTONUM_G     7
#define GPIOX_PORTTONUM_H     8
#define GPIOX_PORTTONUM_J     9
#define GPIOX_PORTTONUM_K     10
#define GPIOX_PORTTONUM_L     11
#define GPIOX_PORTTONUM_M     12
#define GPIOX_PORTNUM_(p, m)  GPIOX_PORTTONUM_ ## p
#define GPIOX_PORTNAME_(p, m) p
#define GPIOX_PORTNUM(x)      GPIOX_PORTNUM_(x)
#define GPIOX_PORTNAME(x)     GPIOX_PORTNAME_(x)

//-----------------------------------------------------------------------------
// Reset láb aktiv/passziv
#define LCD_RST_ON            GPIOX_ODR(LCD_RST) = 0
#define LCD_RST_OFF           GPIOX_ODR(LCD_RST) = 1

// Touch kiolvasásakor a chip select lábat és még néhány lábat le kell választani az FSMC-röl
#define LCD_TS_ON             GPIOX_MODER(MODE_OUT, LCD_CS)
#define LCD_TS_OFF            GPIOX_MODER(MODE_ALTER, LCD_CS)

#define LCD_ADDR_DATA         (LCD_ADDR_BASE + (1 << LCD_REGSELECT_BIT))

#if TS_ADC == 1
#define  RCC_APB2ENR_ADCXEN  RCC_APB2ENR_ADC1EN
#define  ADCX  ADC1
#endif

#if TS_ADC == 2
#define  RCC_APB2ENR_ADCXEN  RCC_APB2ENR_ADC2EN
#define  ADCX  ADC2
#endif

#if TS_ADC == 3
#define  RCC_APB2ENR_ADCXEN  RCC_APB2ENR_ADC3EN
#define  ADCX  ADC3
#endif

#ifndef  TS_XM_AN
#define  TS_XM_AN  TS_XM
#endif

#ifndef  TS_YP_AN
#define  TS_YP_AN  TS_YP
#endif

/* Ha a touchscreen AD átalakito lábai különböznek RS és WR lábaktol, és párhuzamositva vannak azokkal
   (ez azért szükséges, mert az FSMC ezen lábai nem választhatoak ki AD átalakito bemeneteknek) */
#ifdef   ADCX
#if (GPIOX_PORTNUM(TS_XM) != GPIOX_PORTNUM(TS_XM_AN)) || (GPIOX_PIN(TS_XM) != GPIOX_PIN(TS_XM_AN)) || \
    (GPIOX_PORTNUM(TS_YP) != GPIOX_PORTNUM(TS_YP_AN)) || (GPIOX_PIN(TS_YP) != GPIOX_PIN(TS_YP_AN))
#define  TS_AD_PIN_PARALELL
#endif
#endif

//-----------------------------------------------------------------------------
#pragma GCC push_options
#pragma GCC optimize("O0")
void LCD_IO_Delay(volatile uint32_t c)
{
  while(c--);
}
#pragma GCC pop_options

//-----------------------------------------------------------------------------
void LCD_Delay(uint32_t Delay)
{
  HAL_Delay(Delay);
}

//-----------------------------------------------------------------------------
void LCD_IO_Bl_OnOff(uint8_t Bl)
{
  #if (GPIOX_PORTNUM(LCD_BL) >= 1) && (GPIOX_PORTNUM(LCD_BL) <= 12)
//  #ifdef LCD_BL
  if(Bl)
    GPIOX_ODR(LCD_BL) = LCD_BLON;
  else
    GPIOX_ODR(LCD_BL) = 1 - LCD_BLON;
  #endif
}

//-----------------------------------------------------------------------------
void LCD_IO_Init(void)
{
  // GPIO Ports Clock Enable
  #ifdef ADCX
  RCC->APB2ENR |= GPIOX_CLOCK(LCD_RST) | GPIOX_CLOCK(TS_XM) | GPIOX_CLOCK(TS_XP) | GPIOX_CLOCK(TS_YM) | GPIOX_CLOCK(TS_YP);
  #else
  RCC->APB2ENR |= GPIOX_CLOCK(LCD_RST);
  #endif

  GPIOX_MODER(MODE_OUT, LCD_RST);
  GPIOX_ODR(LCD_RST) = 1;               // RST = 1

  /* Set or Reset the control line */
  LCD_Delay(1);
  GPIOX_ODR(LCD_RST) = 0;               // RST = 0
  LCD_Delay(1);
  GPIOX_ODR(LCD_RST) = 1;               // RST = 1
  LCD_Delay(1);

  GPIOX_ODR(LCD_CS) = 1;

  #ifdef ADCX

  #ifdef TS_AD_PIN_PARALELL
  GPIOX_MODER(MODE_ANALOG_INPUT, TS_XM_AN); // XM = AN_INPUT
  GPIOX_MODER(MODE_ANALOG_INPUT, TS_YP_AN); // YP = AN_INPUT
  #endif

  RCC->APB2ENR |= RCC_APB2ENR_ADCXEN;
  ADCX->CR1 = ADC_CR1_DISCEN;
  ADCX->CR2 = ADC_CR2_ADON;

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

  #endif
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteCmd8(uint8_t Cmd)
{
  *(uint8_t *)LCD_ADDR_BASE = Cmd;
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteCmd16(uint16_t Cmd)
{
  *(volatile uint16_t *)LCD_ADDR_BASE = __REVSH(Cmd);
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteData8(uint8_t Data)
{
  *(volatile uint8_t *)LCD_ADDR_DATA = Data;
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteData16(uint16_t Data)
{
  *(volatile uint16_t *)LCD_ADDR_DATA = __REVSH(Data);
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteCmd8DataFill16(uint8_t Cmd, uint16_t Data, uint32_t Size)
{
  uint32_t counter;
  uint16_t d = __REVSH(Data);
  *(volatile uint8_t *)LCD_ADDR_BASE = Cmd;
  for (counter = Size; counter != 0; counter--)
  {
    *(volatile uint16_t *)LCD_ADDR_DATA = d;
  }
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteCmd8MultipleData8(uint8_t Cmd, uint8_t *pData, uint32_t Size)
{
  uint32_t counter;
  *(volatile uint8_t *)LCD_ADDR_BASE = Cmd;
  for (counter = Size; counter != 0; counter--)
  {
    *(volatile uint8_t *)LCD_ADDR_DATA =*pData;
    pData++;
  }
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteCmd8MultipleData16(uint8_t Cmd, uint16_t *pData, uint32_t Size)
{
  uint32_t counter;
  *(volatile uint8_t *)LCD_ADDR_BASE = Cmd;
  for (counter = Size; counter != 0; counter--)
  {
    *(volatile uint16_t *)LCD_ADDR_DATA = __REVSH(*(uint16_t *)pData);
    pData++;
  }
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteCmd16DataFill16(uint16_t Cmd, uint16_t Data, uint32_t Size)
{
  uint32_t counter;
  uint16_t d = __REVSH(Data);
  *(volatile uint16_t *)LCD_ADDR_BASE = __REVSH(Cmd);
  for (counter = Size; counter != 0; counter--)
  {
    *(volatile uint16_t *)LCD_ADDR_DATA = d;
  }
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteCmd16MultipleData8(uint16_t Cmd, uint8_t *pData, uint32_t Size)
{
  uint32_t counter;
  *(volatile uint16_t *)LCD_ADDR_BASE = __REVSH(Cmd);
  for (counter = Size; counter != 0; counter--)
  {
    *(volatile uint8_t *)LCD_ADDR_DATA =*pData;
    pData++;
  }
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteCmd16MultipleData16(uint16_t Cmd, uint16_t *pData, uint32_t Size)
{
  uint32_t counter;
  *(volatile uint16_t *)LCD_ADDR_BASE = __REVSH(Cmd);
  for (counter = Size; counter != 0; counter--)
  {
    *(volatile uint16_t *)LCD_ADDR_DATA = __REVSH(*pData);
    pData++;
  }
}

//-----------------------------------------------------------------------------
void LCD_IO_ReadCmd8MultipleData8(uint8_t Cmd, uint8_t *pData, uint32_t Size, uint32_t DummySize)
{
  uint32_t counter;
  *(volatile uint8_t *)LCD_ADDR_BASE = Cmd;
  for (counter = DummySize; counter != 0; counter--)
    *pData = *(volatile uint8_t *)LCD_ADDR_DATA;
  for (counter = Size; counter != 0; counter--)
  {
    *pData = *(volatile uint8_t *)LCD_ADDR_DATA;
    pData++;
  }
}

//-----------------------------------------------------------------------------
void LCD_IO_ReadCmd8MultipleData16(uint8_t Cmd, uint16_t *pData, uint32_t Size, uint32_t DummySize)
{
  uint32_t counter;
  *(volatile uint8_t *)LCD_ADDR_BASE = Cmd;
  for (counter = DummySize; counter != 0; counter--)
    *pData = *(volatile uint8_t *)LCD_ADDR_DATA;
  for (counter = Size; counter != 0; counter--)
  {
    *pData = __REVSH(*(volatile uint16_t *)LCD_ADDR_DATA);
    pData++;
  }
}

//-----------------------------------------------------------------------------
void LCD_IO_ReadCmd8MultipleData24to16(uint8_t Cmd, uint16_t *pData, uint32_t Size, uint32_t DummySize)
{
  uint32_t counter;
  uint8_t  rgb888[3];

  *(volatile uint8_t *)LCD_ADDR_BASE = Cmd;
  for (counter = DummySize; counter != 0; counter--)
    rgb888[0] = *(volatile uint8_t*)LCD_ADDR_DATA;
  for (counter = Size; counter != 0; counter--)
  {
    rgb888[0] = *(volatile uint8_t*)LCD_ADDR_DATA;
    rgb888[1] = *(volatile uint8_t*)LCD_ADDR_DATA;
    rgb888[2] = *(volatile uint8_t*)LCD_ADDR_DATA;
    *pData = ((rgb888[0] & 0b11111000) << 8 | (rgb888[1] & 0b11111100) << 3 | rgb888[2] >> 3);
    pData++;
  }
}

//-----------------------------------------------------------------------------
void LCD_IO_ReadCmd16MultipleData8(uint16_t Cmd, uint8_t *pData, uint32_t Size, uint32_t DummySize)
{
  uint32_t counter;
  *(volatile uint16_t *)LCD_ADDR_BASE = __REVSH(Cmd);
  for (counter = DummySize; counter != 0; counter--)
    *pData = *(volatile uint8_t *)LCD_ADDR_DATA;
  for (counter = Size; counter != 0; counter--)
  {
    *pData = *(volatile uint8_t *)LCD_ADDR_DATA;
    pData++;
  }
}

//-----------------------------------------------------------------------------
void LCD_IO_ReadCmd16MultipleData16(uint16_t Cmd, uint16_t *pData, uint32_t Size, uint32_t DummySize)
{
  uint32_t counter;
  *(volatile uint16_t *)LCD_ADDR_BASE = __REVSH(Cmd);
  for (counter = DummySize; counter != 0; counter--)
    *pData = *(volatile uint8_t *)LCD_ADDR_DATA;
  for (counter = Size; counter != 0; counter--)
  {
    *pData = __REVSH(*(volatile uint16_t *)LCD_ADDR_DATA);
    pData++;
  }
}

//-----------------------------------------------------------------------------
void LCD_IO_ReadCmd16MultipleData24to16(uint16_t Cmd, uint16_t *pData, uint32_t Size, uint32_t DummySize)
{
  uint32_t counter;
  uint8_t  rgb888[3];
  *(volatile uint16_t *)LCD_ADDR_BASE = __REVSH(Cmd);
  for (counter = DummySize; counter != 0; counter--)
  {
    rgb888[0] = *(volatile uint8_t*)LCD_ADDR_DATA;
  }
  for (counter = Size; counter != 0; counter--)
  {
    rgb888[0] = *(volatile uint8_t*)LCD_ADDR_DATA;
    rgb888[1] = *(volatile uint8_t*)LCD_ADDR_DATA;
    rgb888[2] = *(volatile uint8_t*)LCD_ADDR_DATA;
    *pData = ((rgb888[0] & 0b11111000) << 8 | (rgb888[1] & 0b11111100) << 3 | rgb888[2] >> 3);
    pData++;
  }
}

//=============================================================================
#ifdef ADCX
// CS = 1, X+ = 0, X- = 0; Y+ = in PU, Y- = in PU
uint8_t TS_IO_DetectToch(void)
{
  uint8_t  ret;
  LCD_TS_ON;

  // TS_XM<-LCD_RS, TS_XP<-LCD_D6, TS_YM<-LCD_D7, TS_YP<-LCD_WR
  GPIOX_MODER(MODE_OUT, TS_XM);
  GPIOX_MODER(MODE_OUT, TS_XP);
  GPIOX_MODER(MODE_DIGITAL_INPUT, TS_YP);// YP = D_INPUT
  GPIOX_MODER(MODE_DIGITAL_INPUT, TS_YM);// YM = D_INPUT
  GPIOX_PUPDR(MODE_PU_UP, TS_YP);       // Felhuzo ell. be

  GPIOX_ODR(TS_XP) = 0;                 // XP = 0
  GPIOX_ODR(TS_XM) = 0;                 // XM = 0

  LCD_IO_Delay(TS_AD_DELAY);

  if(GPIOX_IDR(TS_YP))
    ret = 0;
  else
    ret = 1;

  // Felhuzo ell. ki
  GPIOX_PUPDR(MODE_PU_NONE, TS_YP);

  GPIOX_MODER(MODE_ALTER, TS_XM);
  GPIOX_MODER(MODE_ALTER, TS_XP);
  GPIOX_MODER(MODE_ALTER, TS_YM);
  GPIOX_MODER(MODE_ALTER, TS_YP);

  LCD_TS_OFF;
  return ret;
}

//-----------------------------------------------------------------------------
// X poz analog olvasása
uint16_t TS_IO_GetX(void)
{
  uint16_t ret;
  LCD_TS_ON;

  // TS_XM<-LCD_RS, TS_XP<-LCD_D6, TS_YM<-LCD_D7, TS_YP<-LCD_WR
  GPIOX_MODER(MODE_DIGITAL_INPUT, TS_YM);// YM = D_INPUT
  GPIOX_MODER(MODE_ANALOG_INPUT, TS_YP); // YP = AN_INPUT
  GPIOX_MODER(MODE_OUT, TS_XM);
  GPIOX_MODER(MODE_OUT, TS_XP);

  GPIOX_ODR(TS_XP) = 0;                 // XP = 0
  GPIOX_ODR(TS_XM) = 1;                 // XM = 1

  ADCX->SQR3 = TS_YP_ADCCH;
  LCD_IO_Delay(TS_AD_DELAY);
  ADCX->CR2 |= ADC_CR2_SWSTART;
  while(!(ADCX->SR & ADC_SR_EOC));
  ret = ADCX->DR;

  GPIOX_MODER(MODE_ALTER, TS_XM);
  GPIOX_MODER(MODE_ALTER, TS_XP);
  GPIOX_MODER(MODE_ALTER, TS_YM);
  GPIOX_MODER(MODE_ALTER, TS_YP);

  LCD_TS_OFF;
  return ret;
}

//-----------------------------------------------------------------------------
// Y poz analog olvasása
uint16_t TS_IO_GetY(void)
{
  uint16_t ret;
  LCD_TS_ON;

  // TS_XM<-LCD_RS, TS_XP<-LCD_D6, TS_YM<-LCD_D7, TS_YP<-LCD_WR
  GPIOX_MODER(MODE_DIGITAL_INPUT, TS_XP);// XP = D_INPUT
  GPIOX_MODER(MODE_ANALOG_INPUT, TS_XM); // XM = AN_INPUT
  GPIOX_MODER(MODE_OUT, TS_YM);
  GPIOX_MODER(MODE_OUT, TS_YP);

  GPIOX_ODR(TS_YM) = 0;                 // YM = 0
  GPIOX_ODR(TS_YP) = 1;                 // YP = 1

  ADCX->SQR3 = TS_XM_ADCCH;
  LCD_IO_Delay(TS_AD_DELAY);
  ADCX->CR2 |= ADC_CR2_SWSTART;
  while(!(ADCX->SR & ADC_SR_EOC));
  ret = ADCX->DR;

  GPIOX_MODER(MODE_ALTER, TS_XM);
  GPIOX_MODER(MODE_ALTER, TS_XP);
  GPIOX_MODER(MODE_ALTER, TS_YM);
  GPIOX_MODER(MODE_ALTER, TS_YP);

  LCD_TS_OFF;
  return ret;
}

//-----------------------------------------------------------------------------
// Z1 poz analog olvasása
uint16_t TS_IO_GetZ1(void)
{
  uint16_t ret;
  LCD_TS_ON;

  // TS_XM<-LCD_RS, TS_XP<-LCD_D6, TS_YM<-LCD_D7, TS_YP<-LCD_WR
  GPIOX_MODER(MODE_ANALOG_INPUT, TS_XM); // XM = AN_INPUT
  GPIOX_MODER(MODE_ANALOG_INPUT, TS_YP); // YP = AN_INPUT
  GPIOX_MODER(MODE_OUT, TS_XP);
  GPIOX_MODER(MODE_OUT, TS_YM);

  GPIOX_ODR(TS_XP) = 0;                 // XP = 0
  GPIOX_ODR(TS_YM) = 1;                 // YM = 1

  ADCX->SQR3 = TS_YP_ADCCH;
  LCD_IO_Delay(TS_AD_DELAY);
  ADCX->CR2 |= ADC_CR2_SWSTART;
  while(!(ADCX->SR & ADC_SR_EOC));
  ret = ADCX->DR;

  GPIOX_MODER(MODE_ALTER, TS_XM);
  GPIOX_MODER(MODE_ALTER, TS_XP);
  GPIOX_MODER(MODE_ALTER, TS_YM);
  GPIOX_MODER(MODE_ALTER, TS_YP);

  LCD_TS_OFF;
  return ret;
}

//-----------------------------------------------------------------------------
// Z2 poz analog olvasása
uint16_t TS_IO_GetZ2(void)
{
  uint16_t ret;
  LCD_TS_ON;

  // TS_XM<-LCD_RS, TS_XP<-LCD_D6, TS_YM<-LCD_D7, TS_YP<-LCD_WR
  GPIOX_MODER(MODE_ANALOG_INPUT, TS_XM); // XM = AN_INPUT
  GPIOX_MODER(MODE_ANALOG_INPUT, TS_YP); // YP = AN_INPUT
  GPIOX_MODER(MODE_OUT, TS_XP);
  GPIOX_MODER(MODE_OUT, TS_YM);

  GPIOX_ODR(TS_XP) = 0;                 // XP = 0
  GPIOX_ODR(TS_YM) = 1;                 // YM = 1

  ADCX->SQR3 = TS_XM_ADCCH;
  LCD_IO_Delay(TS_AD_DELAY);
  ADCX->CR2 |= ADC_CR2_SWSTART;
  while(!(ADCX->SR & ADC_SR_EOC));
  ret = ADCX->DR;

  GPIOX_MODER(MODE_ALTER, TS_XM);
  GPIOX_MODER(MODE_ALTER, TS_XP);
  GPIOX_MODER(MODE_ALTER, TS_YM);
  GPIOX_MODER(MODE_ALTER, TS_YP);

  LCD_TS_OFF;
  return ret;
}

#else  // #ifdef ADCX
__weak uint8_t   TS_IO_DetectToch(void) { return 0;}
__weak uint16_t  TS_IO_GetX(void)       { return 0;}
__weak uint16_t  TS_IO_GetY(void)       { return 0;}
__weak uint16_t  TS_IO_GetZ1(void)      { return 0;}
__weak uint16_t  TS_IO_GetZ2(void)      { return 0;}
#endif // #else ADCX
