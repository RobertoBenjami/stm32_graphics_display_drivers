/* 8 bites párhuzamos LCD/TOUCH GPIO driver STM32F4-re
5 vezárlöláb (CS, RS, WR, RD, RST) + 8 adatláb

Figyelem: mivel azonos lábakon van az Lcd ás a Touchscreen,
ezért ezek ki kell zárni az Lcd és a Touchscreen egyidejü használatát!
Tábbszálas/megszakitásos környezetben igy gondoskodni kell az összeakadások megelözéséröl!

Készitö: Roberto Benjami
verzio:  2019.05

Megj:
Minden függvány az adatlábak irányát WRITE üzemmodban hagyja, igy nem kell minden irási
müveletkor állitgatni */

// CS láb vezérlési stratégia
// - 0: CS láb minden irás/olvasás müvelet során állitva van (igy a touchscreen olvasásakor nem szükséges lekapcsolni
// - 1: CS láb folyamatosan 0-ba van állitva (a touchscreen olvasásakor ezért le kell kapcsolni)
#define  LCD_CS_MODE          1

// ADC sample time (0: 1.5cycles, 1: 7.5c, 2:13.5c, 3:28.5c, 4:41.5c, 5:55.5c, 6:71.5c, 7:239.5cycles)
#define  TS_SAMPLETIME        3

// A kijelzön belül a következö lábak vannak párhuzamositva:
#define  TS_XP                LCD_D6
#define  TS_XM                LCD_RS
#define  TS_YP                LCD_WR
#define  TS_YM                LCD_D7

#include "main.h"
#include "lcdts_io8p_gpio.h"

/* Link function for LCD peripheral */
void     LCD_Delay (uint32_t delay);
void     LCD_IO_Init(void);
void     LCD_IO_Bl_OnOff(uint8_t Bl);

void     LCD_IO_WriteCmd(uint8_t Cmd);
void     LCD_IO_WriteData8(uint8_t Data);
void     LCD_IO_WriteData16(uint16_t Data);
void     LCD_IO_WriteDataFill16(uint8_t Cmd, uint16_t Data, uint32_t Size);
void     LCD_IO_WriteMultipleData8(uint8_t Cmd, uint8_t *pData, uint32_t Size);
void     LCD_IO_WriteMultipleData16(uint8_t Cmd, uint16_t *pData, uint32_t Size);

void     LCD_IO_ReadMultipleData8(uint8_t Cmd, uint8_t *pData, uint32_t Size);
void     LCD_IO_ReadMultipleData16(uint8_t Cmd, uint16_t *pData, uint32_t Size);

/* Link function for Touchscreen */
uint8_t  TS_IO_DetectToch(void);
uint16_t TS_IO_GetX(void);
uint16_t TS_IO_GetY(void);
uint16_t TS_IO_GetZ1(void);
uint16_t TS_IO_GetZ2(void);

// portláb módok (PP: push-pull, OD: open drain, FF: input floating)
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
#define MODE_PP_ALTER_50MH    0xB
#define MODE_RESERVED         0xC
#define MODE_OD_ALTER_10MHZ   0xD
#define MODE_OD_ALTER_2MHZ    0xE
#define MODE_OD_ALTER_50MHZ   0xF

#define BITBAND_ACCESS(variable, bitnumber) *(volatile uint32_t*)(((uint32_t)&variable & 0xF0000000) + 0x2000000 + (((uint32_t)&variable & 0x000FFFFF) << 5) + (bitnumber << 2))

#define GPIOX_PORT_(a, b)     GPIO ## a
#define GPIOX_PORT(a)         GPIOX_PORT_(a)

#define GPIOX_PIN_(a, b)      b
#define GPIOX_PIN(x)          GPIOX_PIN_(x)

#define GPIOX_MODE_(a,b,c)    ((GPIO_TypeDef*)(((c & 8) >> 1) + GPIO ## b ## _BASE))->CRL = (((GPIO_TypeDef*)(((c & 8) >> 1) + GPIO ## b ## _BASE))->CRL & ~(0xF << ((c & 7) << 2))) | (a << ((c & 7) << 2));
#define GPIOX_MODE(a, b)      GPIOX_MODE_(a, b)

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

#define GPIOX_CLOCK_(a, b)    RCC_APB2ENR_IOP ## a ## EN
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
// Parancs/adat láb üzemmod
#define LCD_RS_CMD            GPIOX_ODR(LCD_RS) = 0
#define LCD_RS_DATA           GPIOX_ODR(LCD_RS) = 1

// Reset láb aktiv/passziv
#define LCD_RST_ON            GPIOX_ODR(LCD_RST) = 0
#define LCD_RST_OFF           GPIOX_ODR(LCD_RST) = 1

// Chip select láb
#if  LCD_CS_MODE ==  0
#define LCD_CS_ON             GPIOX_ODR(LCD_CS) = 0
#define LCD_CS_OFF            GPIOX_ODR(LCD_CS) = 1
#define LCD_TS_ON
#define LCD_TS_OFF
#endif

#if  LCD_CS_MODE ==  1
#define LCD_CS_ON
#define LCD_CS_OFF
#define LCD_TS_ON             GPIOX_ODR(LCD_CS) = 1
#define LCD_TS_OFF            GPIOX_ODR(LCD_CS) = 0
#endif

//-----------------------------------------------------------------------------
// Ha a 8 adatláb egy PORT 0..7 vagy 8..15 lábain van, automatikusan optimalizál
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
// LCD adatlábai 0..7 portlábon vannak (pl. B0,B1,B2,B3,B4,B5,B6,B7)
#define LCD_AUTOOPT  1
#endif
#if GPIOX_PIN(LCD_D0) == 8
// LCD adatlábai 8..15 portlábon vannak (pl. B8,B9,B10,B11,B12,B13,B14,B15)
#define LCD_AUTOOPT  2
#endif
#endif // D0..D7 portláb folytonosság ?
#endif // D0..D7 port azonosság ?

//-----------------------------------------------------------------------------
// adat lábak kimenetre állitása
#ifndef LCD_DATA_DIRWRITE
#if     (LCD_AUTOOPT == 1)
#define LCD_DATA_DIRWRITE  GPIOX_PORT(LCD_D0)->CRL = 0x33333333
#elif   (LCD_AUTOOPT == 2)
#define LCD_DATA_DIRWRITE  GPIOX_PORT(LCD_D0)->CRH = 0x33333333
#else   // #ifdef  LCD_AUTOOPT
#define LCD_DATA_DIRWRITE { \
  GPIOX_MODE(MODE_PP_OUT_50MHZ, LCD_D0); GPIOX_MODE(MODE_PP_OUT_50MHZ, LCD_D1);\
  GPIOX_MODE(MODE_PP_OUT_50MHZ, LCD_D2); GPIOX_MODE(MODE_PP_OUT_50MHZ, LCD_D3);\
  GPIOX_MODE(MODE_PP_OUT_50MHZ, LCD_D4); GPIOX_MODE(MODE_PP_OUT_50MHZ, LCD_D5);\
  GPIOX_MODE(MODE_PP_OUT_50MHZ, LCD_D6); GPIOX_MODE(MODE_PP_OUT_50MHZ, LCD_D7);}
#endif  // #else  LCD_AUTOOPT
#endif  // #ifndef LCD_DATA_DIROUT

//-----------------------------------------------------------------------------
// adat lábak bemenetre állitása
#ifndef LCD_DATA_DIRREAD
#if     LCD_AUTOOPT == 1
#define LCD_DATA_DIRREAD  GPIOX_PORT(LCD_D0)->CRL = 0x44444444
#elif   LCD_AUTOOPT == 2
#define LCD_DATA_DIRREAD  GPIOX_PORT(LCD_D0)->CRH = 0x44444444
#else   // #ifdef  LCD_AUTOOPT
#define LCD_DATA_DIRREAD { \
  GPIOX_MODE(MODE_FF_DIGITAL_INPUT, LCD_D0); GPIOX_MODE(MODE_FF_DIGITAL_INPUT, LCD_D1);\
  GPIOX_MODE(MODE_FF_DIGITAL_INPUT, LCD_D2); GPIOX_MODE(MODE_FF_DIGITAL_INPUT, LCD_D3);\
  GPIOX_MODE(MODE_FF_DIGITAL_INPUT, LCD_D4); GPIOX_MODE(MODE_FF_DIGITAL_INPUT, LCD_D5);\
  GPIOX_MODE(MODE_FF_DIGITAL_INPUT, LCD_D6); GPIOX_MODE(MODE_FF_DIGITAL_INPUT, LCD_D7);}
#endif
#endif

//-----------------------------------------------------------------------------
// adat lábakra 8 bites adat kiirása
#ifndef LCD_DATA_WRITE
#if     LCD_AUTOOPT == 1
#define LCD_DATA_WRITE(dt) {GPIOX_PORT(LCD_D0)->BSRR = (dt & 0xFF) | 0x00FF0000; \
                            GPIOX_ODR(LCD_WR) = 0; LCD_IO_Delay(LCD_IO_RW_DELAY); GPIOX_ODR(LCD_WR) = 1;}
#elif   LCD_AUTOOPT == 2
#define LCD_DATA_WRITE(dt) {GPIOX_PORT(LCD_D0)->BSRR = ((dt << 8) & 0xFF00) | 0xFF000000; \
                            GPIOX_ODR(LCD_WR) = 0; LCD_IO_Delay(LCD_IO_RW_DELAY); GPIOX_ODR(LCD_WR) = 1;}
#else   // #ifdef  LCD_AUTOOPT
uint8_t data;
#define LCD_DATA_WRITE(dt) {;                    \
  data = dt;                                   \
  GPIOX_ODR(LCD_D0) = BITBAND_ACCESS(data, 0); \
  GPIOX_ODR(LCD_D1) = BITBAND_ACCESS(data, 1); \
  GPIOX_ODR(LCD_D2) = BITBAND_ACCESS(data, 2); \
  GPIOX_ODR(LCD_D3) = BITBAND_ACCESS(data, 3); \
  GPIOX_ODR(LCD_D4) = BITBAND_ACCESS(data, 4); \
  GPIOX_ODR(LCD_D5) = BITBAND_ACCESS(data, 5); \
  GPIOX_ODR(LCD_D6) = BITBAND_ACCESS(data, 6); \
  GPIOX_ODR(LCD_D7) = BITBAND_ACCESS(data, 7); \
  GPIOX_ODR(LCD_WR) = 0; LCD_IO_Delay(LCD_IO_RW_DELAY); GPIOX_ODR(LCD_WR) = 1;}
#endif
#endif

//-----------------------------------------------------------------------------
// adat lábakrol 8 bites adat beolvasása
#ifndef LCD_DATA_READ
#if     LCD_AUTOOPT == 1
#define LCD_DATA_READ(dt) {                  \
  GPIOX_ODR(LCD_RD) = 0;                     \
  LCD_IO_Delay(LCD_IO_RW_DELAY);             \
  dt = GPIOX_PORT(LCD_D0)->IDR;              \
  GPIOX_ODR(LCD_RD) = 1;                     }
#elif   LCD_AUTOOPT == 2
#define LCD_DATA_READ(dt) {                  \
  GPIOX_ODR(LCD_RD) = 0;                     \
  LCD_IO_Delay(LCD_IO_RW_DELAY);             \
  dt = GPIOX_PORT(LCD_D0)->IDR >> 8;         \
  GPIOX_ODR(LCD_RD) = 1;                     }
#else   // #ifdef  LCD_AUTOOPT
#define LCD_DATA_READ(dt) {                  \
  GPIOX_ODR(LCD_RD) = 0;                     \
  LCD_IO_Delay(LCD_IO_RW_DELAY);             \
  BITBAND_ACCESS(dt, 0) = GPIOX_IDR(LCD_D0); \
  BITBAND_ACCESS(dt, 1) = GPIOX_IDR(LCD_D1); \
  BITBAND_ACCESS(dt, 2) = GPIOX_IDR(LCD_D2); \
  BITBAND_ACCESS(dt, 3) = GPIOX_IDR(LCD_D3); \
  BITBAND_ACCESS(dt, 4) = GPIOX_IDR(LCD_D4); \
  BITBAND_ACCESS(dt, 5) = GPIOX_IDR(LCD_D5); \
  BITBAND_ACCESS(dt, 6) = GPIOX_IDR(LCD_D6); \
  BITBAND_ACCESS(dt, 7) = GPIOX_IDR(LCD_D7); \
  GPIOX_ODR(LCD_RD) = 1;                     }
#endif
#endif

#if LCD_CMD_SIZE == 8
#define  LCD_CMD_WRITE(cmd) {LCD_RS_CMD; LCD_DATA_WRITE(cmd); LCD_RS_DATA; }
#endif

#if LCD_CMD_SIZE == 16
#define  LCD_CMD_WRITE(cmd) {LCD_RS_CMD; LCD_DATA_WRITE(0); LCD_DATA_WRITE(cmd); LCD_RS_DATA; }
#endif

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
  #ifdef LCD_BL
  if(Bl)
    GPIOX_ODR(LCD_BL) = LCD_BLON;
  else
    GPIOX_ODR(LCD_BL) = 1 - LCD_BLON;
  #endif
}

//-----------------------------------------------------------------------------
void LCD_IO_Init(void)
{
  RCC->APB2ENR |= (GPIOX_CLOCK(LCD_CS) | GPIOX_CLOCK(LCD_RS) | GPIOX_CLOCK(LCD_WR) | GPIOX_CLOCK(LCD_RD) | GPIOX_CLOCK(LCD_RST) |
                   GPIOX_CLOCK(LCD_D0) | GPIOX_CLOCK(LCD_D1) | GPIOX_CLOCK(LCD_D2) | GPIOX_CLOCK(LCD_D3) |
                   GPIOX_CLOCK(LCD_D4) | GPIOX_CLOCK(LCD_D5) | GPIOX_CLOCK(LCD_D6) | GPIOX_CLOCK(LCD_D7));

  // disable the LCD
  GPIOX_ODR(LCD_CS) = 1;                // CS = 1
  LCD_RS_DATA;                          // RS = 1
  GPIOX_ODR(LCD_WR) = 1;                // WR = 1
  GPIOX_ODR(LCD_RD) = 1;                // RD = 1
  LCD_RST_OFF;                          // RST = 1

  GPIOX_MODE(MODE_PP_OUT_50MHZ, LCD_CS);
  GPIOX_MODE(MODE_PP_OUT_50MHZ, LCD_RD);
  GPIOX_MODE(MODE_PP_OUT_50MHZ, LCD_WR);
  GPIOX_MODE(MODE_PP_OUT_50MHZ, LCD_RS);
  GPIOX_MODE(MODE_PP_OUT_50MHZ, LCD_RST);

  LCD_DATA_DIRWRITE;                    // adatlábak kimenetre állitása

  /* Set or Reset the control line */
  LCD_Delay(1);
  LCD_RST_ON;                           // RST = 0
  LCD_Delay(1);
  LCD_RST_OFF;                          // RST = 1
  LCD_Delay(1);

  #if (TS_ADC == 1) || (TS_ADC == 2) || (TS_ADC == 3)
  RCC->CFGR |= RCC_CFGR_ADCPRE_DIV6;    // ADC orajel = 72/6 = 12MHz
  RCC->APB2ENR |= RCC_APB2ENR_ADCXEN;
  LCD_Delay(1);
  ADCX->CR1 = ADC_CR1_DISCEN;
  ADCX->CR2 = (0b111 << ADC_CR2_EXTSEL_Pos) | ADC_CR2_EXTTRIG | ADC_CR2_ADON;
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
  #endif

  LCD_TS_OFF;
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteCmd(uint8_t Cmd)
{
  LCD_CS_ON;                            // CS = 0
  LCD_CMD_WRITE(Cmd);
  LCD_CS_OFF;                           // CS = 1
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteData8(uint8_t Data)
{
  LCD_CS_ON;                            // CS = 0
  LCD_DATA_WRITE(Data);
  LCD_CS_OFF;                           // CS = 1
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteData16(uint16_t Data)
{
  LCD_CS_ON;                            // CS = 0
  LCD_DATA_WRITE((uint8_t)(Data >> 8));
  LCD_IO_Delay(LCD_IO_RW_DELAY);
  LCD_DATA_WRITE((uint8_t)Data);
  LCD_CS_OFF;                           // CS = 1
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteDataFill16(uint8_t Cmd, uint16_t Data, uint32_t Size)
{
  uint32_t counter;

  LCD_CS_ON;                            // CS = 0
  LCD_CMD_WRITE(Cmd);

  for (counter = Size; counter != 0; counter--)
  {
    /* Need to invert bytes for LCD*/
    LCD_DATA_WRITE(Data >> 8);
    LCD_IO_Delay(LCD_IO_RW_DELAY);
    LCD_DATA_WRITE(Data);
  }

  LCD_CS_OFF;                           // CS = 1
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteMultipleData8(uint8_t Cmd, uint8_t *pData, uint32_t Size)
{
  uint32_t counter;

  LCD_CS_ON;                            // CS = 0
  LCD_CMD_WRITE(Cmd);

  for (counter = Size; counter != 0; counter--)
  {
    /* Need to invert bytes for LCD*/
    LCD_DATA_WRITE(*pData >> 8);
    LCD_IO_Delay(LCD_IO_RW_DELAY);
    LCD_DATA_WRITE(*pData);
    pData ++;
  }

  LCD_CS_OFF;                           // CS = 1
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteMultipleData16(uint8_t Cmd, uint16_t *pData, uint32_t Size)
{
  uint32_t counter;

  LCD_CS_ON;                            // CS = 0
  LCD_CMD_WRITE(Cmd);

  for (counter = Size; counter != 0; counter--)
  {
    /* Need to invert bytes for LCD*/
    LCD_DATA_WRITE(*pData >> 8);
    LCD_IO_Delay(LCD_IO_RW_DELAY);
    LCD_DATA_WRITE(*pData);
    pData ++;
  }

  LCD_CS_OFF;                           // CS = 1
}

//-----------------------------------------------------------------------------
void LCD_IO_ReadMultipleData8(uint8_t Cmd, uint8_t *pData, uint32_t Size)
{
  uint32_t counter;
  uint8_t  d;

  LCD_CS_ON;
  LCD_CMD_WRITE(Cmd);

  LCD_DATA_DIRREAD;

  if(Size == 1)
  {
    LCD_DATA_READ(d);
    *pData = d;
  }
  else
  {
    LCD_DATA_READ(d);    // Dummy data

    for (counter = Size; counter != 0; counter--)
    {
      LCD_DATA_READ(d);
      *pData = d;
      pData++;
    }
  }

  LCD_CS_OFF;
  LCD_DATA_DIRWRITE;
}

//-----------------------------------------------------------------------------
void LCD_IO_ReadMultipleData16(uint8_t Cmd, uint16_t *pData, uint32_t Size)
{
  uint32_t counter;
  uint8_t  dl, dh;

  LCD_CS_ON;
  LCD_CMD_WRITE(Cmd);

  LCD_DATA_DIRREAD;

  for (counter = Size; counter != 0; counter--)
  {
    /* Need to invert bytes for LCD*/
    LCD_DATA_READ(dh);
    LCD_DATA_READ(dl);
    *pData = (dh << 8) | dl;
    pData++;
  }
  LCD_CS_OFF;
  LCD_DATA_DIRWRITE;
}

#if (TS_ADC == 1) || (TS_ADC == 2) || (TS_ADC == 3)
//-----------------------------------------------------------------------------
// CS = 1, X+ = 0, X- = 0; Y+ = in PU, Y- = in PU
uint8_t TS_IO_DetectToch(void)
{
  uint8_t  ret;

  LCD_TS_ON;

  GPIOX_MODE(MODE_PU_DIGITAL_INPUT, TS_YP);// YP = D_INPUT PULL
  GPIOX_MODE(MODE_FF_DIGITAL_INPUT, TS_YM);// YM = D_INPUT
  GPIOX_ODR(TS_XP) = 0;                 // XP = 0
  GPIOX_ODR(TS_XM) = 0;                 // XM = 0

  // Felhuzo ell. be
  GPIOX_ODR(TS_YP) = 1;                 // YP = 1 (pullup)

  LCD_IO_Delay(TS_AD_DELAY);

  if(GPIOX_IDR(TS_YP))
    ret = 0;                            // Touchscreen nincs megnyomva
  else
    ret = 1;                            // Touchscreen meg van nyomva

  GPIOX_ODR(TS_XP) = 1;                 // XP = 1
  GPIOX_ODR(TS_XM) = 1;                 // XM = 1
  GPIOX_MODE(MODE_PP_OUT_50MHZ, TS_YP); // YP = OUT
  GPIOX_MODE(MODE_PP_OUT_50MHZ, TS_YM); // YM = OUT

  LCD_TS_OFF;

  return ret;
}

//-----------------------------------------------------------------------------
// X poz analog olvasása
uint16_t TS_IO_GetX(void)
{
  uint16_t ret;

  LCD_TS_ON;

  GPIOX_MODE(MODE_FF_DIGITAL_INPUT, TS_YM);// YM = D_INPUT
  GPIOX_MODE(MODE_ANALOG_INPUT, TS_YP);    // YP = AN_INPUT
  GPIOX_ODR(TS_XP) = 0;                 // XP = 0
  GPIOX_ODR(TS_XM) = 1;                 // XM = 1

  ADCX->SQR3 = TS_YP_ADCCH;
  CLEAR_BIT(ADCX->SR, ADC_SR_EOC);
  LCD_IO_Delay(TS_AD_DELAY);
  SET_BIT(ADCX->CR2, ADC_CR2_SWSTART);
  while(!READ_BIT(ADCX->SR, ADC_SR_EOC));
  ret = ADCX->DR;

  GPIOX_ODR(TS_XP) = 1;                 // XM = 1
  GPIOX_MODE(MODE_PP_OUT_50MHZ, TS_YP);
  GPIOX_MODE(MODE_PP_OUT_50MHZ, TS_YM);

  LCD_TS_OFF;

  return ret;
}

//-----------------------------------------------------------------------------
// Y poz analog olvasása
uint16_t TS_IO_GetY(void)
{
  uint16_t ret;

  LCD_TS_ON;

  GPIOX_MODE(MODE_FF_DIGITAL_INPUT, TS_XP);// XP = D_INPUT
  GPIOX_MODE(MODE_ANALOG_INPUT, TS_XM);    // XM = AN_INPUT
  GPIOX_ODR(TS_YM) = 0;                 // YP = 0
  GPIOX_ODR(TS_YP) = 1;                 // YM = 1

  ADCX->SQR3 = TS_XM_ADCCH;
  CLEAR_BIT(ADCX->SR, ADC_SR_EOC);
  LCD_IO_Delay(TS_AD_DELAY);
  SET_BIT(ADCX->CR2, ADC_CR2_SWSTART);
  while(!READ_BIT(ADCX->SR, ADC_SR_EOC));
  ret = ADCX->DR;

  GPIOX_ODR(TS_YM) = 1;
  GPIOX_MODE(MODE_PP_OUT_50MHZ, TS_XP);
  GPIOX_MODE(MODE_PP_OUT_50MHZ, TS_XM);

  LCD_TS_OFF;

  return ret;
}

//-----------------------------------------------------------------------------
// Z1 poz analog olvasása
uint16_t TS_IO_GetZ1(void)
{
  uint16_t ret;

  LCD_TS_ON;

  GPIOX_MODE(MODE_ANALOG_INPUT, TS_XM); // XM = AN_INPUT
  GPIOX_MODE(MODE_ANALOG_INPUT, TS_YP); // YP = AN_INPUT
  GPIOX_ODR(TS_XP) = 0;                 // XP = 0
  GPIOX_ODR(TS_YM) = 1;                 // YM = 1

  #ifdef LCD_CS_OPT
  GPIOX_ODR(LCD_CS) = 1;                // CS = 1
  #endif

  ADCX->SQR3 = TS_YP_ADCCH;
  CLEAR_BIT(ADCX->SR, ADC_SR_EOC);
  LCD_IO_Delay(TS_AD_DELAY);
  SET_BIT(ADCX->CR2, ADC_CR2_SWSTART);
  while(!READ_BIT(ADCX->SR, ADC_SR_EOC));
  ret = ADCX->DR;

  GPIOX_ODR(TS_XP) = 1;
  GPIOX_MODE(MODE_PP_OUT_50MHZ, TS_XM);
  GPIOX_MODE(MODE_PP_OUT_50MHZ, TS_YP);

  LCD_TS_OFF;

  return ret;
}

//-----------------------------------------------------------------------------
// Z2 poz analog olvasása
uint16_t TS_IO_GetZ2(void)
{
  uint16_t ret;

  LCD_TS_ON;

  GPIOX_MODE(MODE_ANALOG_INPUT, TS_XM); // XM = AN_INPUT
  GPIOX_MODE(MODE_ANALOG_INPUT, TS_YP); // YP = AN_INPUT
  GPIOX_ODR(TS_XP) = 0;                 // XP = 0
  GPIOX_ODR(TS_YM) = 1;                 // YM = 1

  ADCX->SQR3 = TS_XM_ADCCH;
  CLEAR_BIT(ADCX->SR, ADC_SR_EOC);
  LCD_IO_Delay(TS_AD_DELAY);
  SET_BIT(ADCX->CR2, ADC_CR2_SWSTART);
  while(!READ_BIT(ADCX->SR, ADC_SR_EOC));
  ret = ADCX->DR;

  GPIOX_ODR(TS_XP) = 1;
  GPIOX_MODE(MODE_PP_OUT_50MHZ, TS_XM);
  GPIOX_MODE(MODE_PP_OUT_50MHZ, TS_YP);

  LCD_TS_OFF;

  return ret;
}

#else // #if (TS_ADC == 1) || (TS_ADC == 2) || (TS_ADC == 3)
__weak uint8_t   TS_IO_DetectToch(void) { return 0;}
__weak uint16_t  TS_IO_GetX(void)       { return 0;}
__weak uint16_t  TS_IO_GetY(void)       { return 0;}
__weak uint16_t  TS_IO_GetZ1(void)      { return 0;}
__weak uint16_t  TS_IO_GetZ2(void)      { return 0;}
#endif
