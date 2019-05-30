/*
 * SPI LCD driver STM32F1
 * készitö: Roberto Benjami
 * verzio:  2019.05
 *
 * - hardver és szoftver SPI
 * - 3 féle üzemmód (csak TX, half duplex, full duplex)
*/

//-----------------------------------------------------------------------------
#include "main.h"
#include "lcd_io_spi.h"

//-----------------------------------------------------------------------------
/* Link function for LCD peripheral */
void    LCD_Delay (uint32_t delay);
void    LCD_IO_Init(void);
void    LCD_IO_Bl_OnOff(uint8_t Bl);

void    LCD_IO_WriteCmd(uint8_t Cmd);
void    LCD_IO_WriteData8(uint8_t Data);
void    LCD_IO_WriteData16(uint16_t Data);
void    LCD_IO_WriteDataFill16(uint8_t Cmd, uint16_t Data, uint32_t Size);
void    LCD_IO_WriteMultipleData8(uint8_t Cmd, uint8_t *pData, uint32_t Size);
void    LCD_IO_WriteMultipleData16(uint8_t Cmd, uint16_t *pData, uint32_t Size);

void    LCD_IO_ReadMultipleData8(uint8_t Cmd, uint8_t *pData, uint32_t Size);
void    LCD_IO_ReadMultipleData16(uint8_t Cmd, uint16_t *pData, uint32_t Size);

//-----------------------------------------------------------------------------
#define BITBAND_ACCESS(variable, bitnumber) *(volatile uint32_t*)(((uint32_t)&variable & 0xF0000000) + 0x2000000 + (((uint32_t)&variable & 0x000FFFFF) << 5) + (bitnumber << 2))

//-----------------------------------------------------------------------------
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
#define MODE_PP_ALTER_50MHZ   0xB
#define MODE_RESERVED         0xC
#define MODE_OD_ALTER_10MHZ   0xD
#define MODE_OD_ALTER_2MHZ    0xE
#define MODE_OD_ALTER_50MHZ   0xF

#define GPIOX_SETMODE_(a,b,c) ((GPIO_TypeDef*)(((c & 8) >> 1) + GPIO ## b ## _BASE))->CRL = (((GPIO_TypeDef*)(((c & 8) >> 1) + GPIO ## b ## _BASE))->CRL & ~(0xF << ((c & 7) << 2))) | (a << ((c & 7) << 2));
#define GPIOX_SETMODE(a, b)   GPIOX_SETMODE_(a, b)

#define GPIOX_ODR_(a, b)      BITBAND_ACCESS(GPIO ## a ->ODR, b)
#define GPIOX_ODR(a)          GPIOX_ODR_(a)

#define GPIOX_IDR_(a, b)      BITBAND_ACCESS(GPIO ## a ->IDR, b)
#define GPIOX_IDR(a)          GPIOX_IDR_(a)

#define GPIOX_CLOCK_(a, b)    RCC_APB2ENR_IOP ## a ## EN
#define GPIOX_CLOCK(a)        GPIOX_CLOCK_(a)

//-----------------------------------------------------------------------------
// Parancs/adat láb üzemmod
#define LCD_RS_CMD            GPIOX_ODR(LCD_RS) = 0
#define LCD_RS_DATA           GPIOX_ODR(LCD_RS) = 1

// Reset láb aktiv/passziv
#define LCD_RST_ON            GPIOX_ODR(LCD_RST) = 0
#define LCD_RST_OFF           GPIOX_ODR(LCD_RST) = 1

// Chip select láb
#define LCD_CS_ON             GPIOX_ODR(LCD_CS) = 0
#define LCD_CS_OFF            GPIOX_ODR(LCD_CS) = 1

//-----------------------------------------------------------------------------
#if LCD_SPI == 0
// Szoftver SPI
uint8_t data;

#if     LCD_SPI_MODE == 1
// Kétirányu (halfduplex) SPI esetén az adatirányt váltogatni kell, és MISO láb = MOSI láb
#undef  LCD_MISO
#define LCD_MISO              LCD_MOSI
#define LCD_DATA_DIRREAD      GPIOX_SETMODE(MODE_FF_DIGITAL_INPUT, LCD_MOSI)
#define LCD_DATA_DIRWRITE     GPIOX_SETMODE(MODE_PP_OUT_50MHZ, LCD_MOSI)
#else
// TX és fullduplex SPI esetén az adatirány fix
#define LCD_DATA_DIRREAD
#define LCD_DATA_DIRWRITE
#endif

// Szoftver SPI esetén nincs foglaltsági várakozás
#define LCD_SPI_WAIT_TX_BUSY
#define LCD_SPI_WAIT_TX_FREE

#define LCD_SPI_DUMMYCLK      {GPIOX_ODR(LCD_SCK) = 0; GPIOX_ODR(LCD_SCK) = 1;}

#define LCD_DATA_WRITE(dt) {;                   \
data = dt;                                      \
GPIOX_ODR(LCD_MOSI) = BITBAND_ACCESS(data, 7);  \
GPIOX_ODR(LCD_SCK) = 0; GPIOX_ODR(LCD_SCK) = 1; \
GPIOX_ODR(LCD_MOSI) = BITBAND_ACCESS(data, 6);  \
GPIOX_ODR(LCD_SCK) = 0; GPIOX_ODR(LCD_SCK) = 1; \
GPIOX_ODR(LCD_MOSI) = BITBAND_ACCESS(data, 5);  \
GPIOX_ODR(LCD_SCK) = 0; GPIOX_ODR(LCD_SCK) = 1; \
GPIOX_ODR(LCD_MOSI) = BITBAND_ACCESS(data, 4);  \
GPIOX_ODR(LCD_SCK) = 0; GPIOX_ODR(LCD_SCK) = 1; \
GPIOX_ODR(LCD_MOSI) = BITBAND_ACCESS(data, 3);  \
GPIOX_ODR(LCD_SCK) = 0; GPIOX_ODR(LCD_SCK) = 1; \
GPIOX_ODR(LCD_MOSI) = BITBAND_ACCESS(data, 2);  \
GPIOX_ODR(LCD_SCK) = 0; GPIOX_ODR(LCD_SCK) = 1; \
GPIOX_ODR(LCD_MOSI) = BITBAND_ACCESS(data, 1);  \
GPIOX_ODR(LCD_SCK) = 0; GPIOX_ODR(LCD_SCK) = 1; \
GPIOX_ODR(LCD_MOSI) = BITBAND_ACCESS(data, 0);  \
GPIOX_ODR(LCD_SCK) = 0; GPIOX_ODR(LCD_SCK) = 1; }

#define LCD_DATA_READ(dt) {;                    \
GPIOX_ODR(LCD_SCK) = 0; GPIOX_ODR(LCD_SCK) = 1; \
BITBAND_ACCESS(data, 7) = GPIOX_IDR(LCD_MISO);  \
GPIOX_ODR(LCD_SCK) = 0; GPIOX_ODR(LCD_SCK) = 1; \
BITBAND_ACCESS(data, 6) = GPIOX_IDR(LCD_MISO);  \
GPIOX_ODR(LCD_SCK) = 0; GPIOX_ODR(LCD_SCK) = 1; \
BITBAND_ACCESS(data, 5) = GPIOX_IDR(LCD_MISO);  \
GPIOX_ODR(LCD_SCK) = 0; GPIOX_ODR(LCD_SCK) = 1; \
BITBAND_ACCESS(data, 4) = GPIOX_IDR(LCD_MISO);  \
GPIOX_ODR(LCD_SCK) = 0; GPIOX_ODR(LCD_SCK) = 1; \
BITBAND_ACCESS(data, 3) = GPIOX_IDR(LCD_MISO);  \
GPIOX_ODR(LCD_SCK) = 0; GPIOX_ODR(LCD_SCK) = 1; \
BITBAND_ACCESS(data, 2) = GPIOX_IDR(LCD_MISO);  \
GPIOX_ODR(LCD_SCK) = 0; GPIOX_ODR(LCD_SCK) = 1; \
BITBAND_ACCESS(data, 1) = GPIOX_IDR(LCD_MISO);  \
GPIOX_ODR(LCD_SCK) = 0; GPIOX_ODR(LCD_SCK) = 1; \
BITBAND_ACCESS(data, 0) = GPIOX_IDR(LCD_MISO);  \
dt = data;                                      }

#else    // #if LCD_SPI == 0
// Hardver SPI

#if LCD_SPI == 1
#define SPIX                  SPI1
#define LCD_SPI_RCC_EN        BITBAND_ACCESS(RCC->APB2ENR, RCC_APB2ENR_SPI1EN_Pos) = 1
#elif LCD_SPI == 2
#define SPIX                  SPI2
#define LCD_SPI_RCC_EN        BITBAND_ACCESS(RCC->APB1ENR, RCC_APB1ENR_SPI2EN_Pos) = 1
#elif LCD_SPI == 3
#define SPIX                  SPI3
#define LCD_SPI_RCC_EN        BITBAND_ACCESS(RCC->APB1ENR, RCC_APB1ENR_SPI3EN_Pos) = 1
#endif

#if     LCD_SPI_MODE == 1
// Kétirányu (halfduplex) SPI esetén az adatirányt váltogatni kell

#if (defined(LCD_SPI_SPD_READ) && (LCD_SPI != 0) && (LCD_SPI_SPD != LCD_SPI_SPD_READ)) // Eltérö olvasási/irási sebesség
#define LCD_DATA_DIRREAD      SPIX->CR1 = (SPIX->CR1 & ~(SPI_CR1_BR_Msk | SPI_CR1_BIDIOE_Msk)) | (LCD_SPI_SPD_READ << SPI_CR1_BR_Pos);
#define LCD_DATA_DIRWRITE     {SPIX->CR1 = (SPIX->CR1 & ~SPI_CR1_BR_Msk) | ((LCD_SPI_SPD << SPI_CR1_BR_Pos) | SPI_CR1_BIDIOE_Msk); \
                               BITBAND_ACCESS(SPIX->CR1, SPI_CR1_SPE_Pos) = 0;    \
                               LCD_IO_Delay(3);                                   \
                               BITBAND_ACCESS(SPIX->CR1, SPI_CR1_SPE_Pos) = 1;    }
#else   // csak LCD_MOSI irányváltás
#define LCD_DATA_DIRREAD      BITBAND_ACCESS(SPIX->CR1, SPI_CR1_BIDIOE_Pos) = 0
#define LCD_DATA_DIRWRITE     {BITBAND_ACCESS(SPIX->CR1, SPI_CR1_BIDIOE_Pos) = 1; \
                               BITBAND_ACCESS(SPIX->CR1, SPI_CR1_SPE_Pos) = 0;    \
                               LCD_IO_Delay(3);                                   \
                               BITBAND_ACCESS(SPIX->CR1, SPI_CR1_SPE_Pos) = 1;    }
#endif

#else
// TX és fullduplex SPI esetén az adatirány fix
#if (defined(LCD_SPI_SPD_READ) && (LCD_SPI != 0) && (LCD_SPI_SPD != LCD_SPI_SPD_READ)) // Eltérö olvasási/irási sebesség
#define LCD_DATA_DIRREAD      SPIX->CR1 = (SPIX->CR1 & ~SPI_CR1_BR_Msk) | (LCD_SPI_SPD_READ << SPI_CR1_BR_Pos)
#define LCD_DATA_DIRWRITE     SPIX->CR1 = (SPIX->CR1 & ~SPI_CR1_BR_Msk) | (LCD_SPI_SPD << SPI_CR1_BR_Pos)
#else   // csak LCD_MOSI irányváltás
#define LCD_DATA_DIRREAD
#define LCD_DATA_DIRWRITE
#endif
#endif

#define LCD_DATA_WRITE(data)  SPIX->DR = data
#define LCD_DATA_READ(data)   {while(!BITBAND_ACCESS(SPIX->SR, SPI_SR_RXNE_Pos)); data = SPIX->DR;}

#define LCD_SPI_WAIT_TX_BUSY  {LCD_IO_Delay(1); while(BITBAND_ACCESS(SPIX->SR, SPI_SR_BSY_Pos)); /*while(!GPIOX_IDR(LCD_SCK));*/}
#define LCD_SPI_WAIT_TX_FREE  while(!BITBAND_ACCESS(SPIX->SR, SPI_SR_TXE_Pos))

#define LCD_SPI_DUMMYCLK      {GPIOX_SETMODE(MODE_PP_OUT_50MHZ, LCD_SCK);      \
                               GPIOX_ODR(LCD_SCK) = 0;                         \
                               GPIOX_ODR(LCD_SCK) = 1;                         \
                               GPIOX_SETMODE(MODE_PP_ALTER_50MHZ, LCD_SCK);    }

#endif   // #else LCD_SPI == 0

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
  #if LCD_SPI_MODE == 2                 // Full duplex
  RCC->APB2ENR |= (GPIOX_CLOCK(LCD_RS) | GPIOX_CLOCK(LCD_RST) | GPIOX_CLOCK(LCD_CS) | GPIOX_CLOCK(LCD_SCK) | GPIOX_CLOCK(LCD_MOSI) | GPIOX_CLOCK(LCD_MISO));
  GPIOX_SETMODE(MODE_FF_DIGITAL_INPUT, LCD_MISO);
  #else                                 // TX vagy half duplex
  RCC->APB2ENR |= GPIOX_CLOCK(LCD_RS) | GPIOX_CLOCK(LCD_RST) | GPIOX_CLOCK(LCD_CS) | GPIOX_CLOCK(LCD_SCK) | GPIOX_CLOCK(LCD_MOSI);
  #endif

  #ifdef LCD_BL
  RCC->APB2ENR |= GPIOX_CLOCK(LCD_BL);
  GPIOX_SETMODE(MODE_PP_OUT_2MHZ, LCD_BL);
  LCD_IO_Bl_OnOff(1);
  #endif

  RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;

  LCD_RST_OFF;
  LCD_RS_DATA;
  GPIOX_SETMODE(MODE_PP_OUT_50MHZ, LCD_RST);
  GPIOX_SETMODE(MODE_PP_OUT_50MHZ, LCD_RS);

  #if LCD_SPI == 0                      // Szoftver SPI
  LCD_CS_OFF;
  GPIOX_ODR(LCD_SCK) = 1;               // SCK = 0
  GPIOX_SETMODE(MODE_PP_OUT_50MHZ, LCD_CS); // CS
  GPIOX_SETMODE(MODE_PP_OUT_50MHZ, LCD_SCK);
  GPIOX_SETMODE(MODE_PP_OUT_50MHZ, LCD_MOSI);

  #else                                 // Hardver SPI
  LCD_SPI_RCC_EN;

  GPIOX_SETMODE(MODE_PP_ALTER_50MHZ, LCD_SCK);
  GPIOX_SETMODE(MODE_PP_ALTER_50MHZ, LCD_MOSI);

  GPIOX_SETMODE(MODE_PP_OUT_50MHZ, LCD_CS);
  GPIOX_ODR(LCD_CS) = 1;

  #if LCD_SPI_MODE == 1
  // Half duplex (adatirány váltogatás)
  SPIX->CR1 = SPI_CR1_CPHA | SPI_CR1_CPOL | SPI_CR1_MSTR | SPI_CR1_SPE | SPI_CR1_SSM | SPI_CR1_SSI | (LCD_SPI_SPD << SPI_CR1_BR_Pos) | SPI_CR1_BIDIMODE | SPI_CR1_BIDIOE;
  #else // #if LCD_SPI_MODE == 1
  // TX vagy full duplex mod, szoftver CS vezérlés
  SPIX->CR1 = SPI_CR1_CPHA | SPI_CR1_CPOL | SPI_CR1_MSTR | SPI_CR1_SPE | SPI_CR1_SSM | SPI_CR1_SSI | (LCD_SPI_SPD << SPI_CR1_BR_Pos);
  #endif // #else LCD_SPI_MODE == 1

  #endif // #else LCD_SPI == 0

  /* Set or Reset the control line */
  LCD_Delay(10);
  LCD_RST_ON;
  LCD_Delay(10);
  LCD_RST_OFF;
  LCD_Delay(10);
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteCmd(uint8_t Cmd)
{
  LCD_RS_CMD;
  LCD_CS_ON;
  LCD_DATA_WRITE(Cmd);
  LCD_SPI_WAIT_TX_BUSY;
  LCD_CS_OFF;
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteData8(uint8_t Data)
{
  LCD_RS_DATA;
  LCD_CS_ON;
  LCD_DATA_WRITE(Data);
  LCD_SPI_WAIT_TX_BUSY;
  LCD_CS_OFF;
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteData16(uint16_t Data)
{
  LCD_RS_DATA;
  LCD_CS_ON;
  LCD_DATA_WRITE(Data >> 8);
  LCD_SPI_WAIT_TX_FREE;
  LCD_DATA_WRITE(Data);
  LCD_SPI_WAIT_TX_BUSY;
  LCD_CS_OFF;
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteDataFill16(uint8_t Cmd, uint16_t Data, uint32_t Size)
{
  uint32_t counter;

  LCD_RS_CMD;
  LCD_CS_ON;
  LCD_DATA_WRITE(Cmd);
  LCD_SPI_WAIT_TX_BUSY;

  LCD_RS_DATA;
  for (counter = Size; counter != 0; counter--)
  {
    LCD_SPI_WAIT_TX_FREE;
    LCD_DATA_WRITE(Data >> 8);
    LCD_SPI_WAIT_TX_FREE;
    LCD_DATA_WRITE(Data);
  }
  LCD_SPI_WAIT_TX_BUSY;
  LCD_CS_OFF;
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteMultipleData8(uint8_t Cmd, uint8_t *pData, uint32_t Size)
{
  uint32_t counter = 0;

  LCD_RS_CMD;
  LCD_CS_ON;
  LCD_DATA_WRITE(Cmd);
  LCD_SPI_WAIT_TX_BUSY;

  LCD_RS_DATA;
  for (counter = Size; counter != 0; counter--)
  {
    LCD_SPI_WAIT_TX_FREE;
    LCD_DATA_WRITE(*pData);
    pData ++;
  }
  LCD_SPI_WAIT_TX_BUSY;
  LCD_CS_OFF;
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteMultipleData16(uint8_t Cmd, uint16_t *pData, uint32_t Size)
{
  uint32_t counter = 0;

  LCD_RS_CMD;
  LCD_CS_ON;
  LCD_DATA_WRITE(Cmd);
  LCD_SPI_WAIT_TX_BUSY;

  LCD_RS_DATA;
  for (counter = Size; counter != 0; counter--)
  {
    /* Need to invert bytes for LCD*/
    LCD_SPI_WAIT_TX_FREE;
    LCD_DATA_WRITE(*((uint8_t *)pData + 1));
    LCD_SPI_WAIT_TX_FREE;
    LCD_DATA_WRITE(*pData);
    pData ++;
  }
  LCD_SPI_WAIT_TX_BUSY;
  LCD_CS_OFF;
}

//-----------------------------------------------------------------------------
#if LCD_SPI_MODE == 0
__weak void LCD_IO_ReadMultipleData8(uint8_t Cmd, uint8_t *pData, uint32_t Size) {}
__weak void LCD_IO_ReadMultipleData16(uint8_t Cmd, uint16_t *pData, uint32_t Size) {}
__weak void LCD_IO_ReadMultipleData24to16(uint8_t Cmd, uint16_t *pData, uint32_t Size) {}
#else

//-----------------------------------------------------------------------------
void LCD_IO_ReadMultipleData8(uint8_t Cmd, uint8_t *pData, uint32_t Size)
{
  uint32_t counter;

  LCD_RS_CMD;
  LCD_CS_ON;
  LCD_DATA_WRITE(Cmd);
  LCD_SPI_WAIT_TX_BUSY;

  LCD_RS_DATA;

  if(Size == 1)
  {
    LCD_DATA_DIRREAD;
    LCD_DATA_READ(*pData);
  }
  else
  {
    LCD_SPI_DUMMYCLK;         // Dummy clock
    LCD_DATA_DIRREAD;

    for (counter = Size; counter != 0; counter--)
    {
      LCD_DATA_READ(*pData);
      pData++;
    }
  }

  LCD_CS_OFF;
  LCD_DATA_DIRWRITE;
}

//-----------------------------------------------------------------------------
#if  LCD_READMULTIPLEDATA24TO16 == 0
void LCD_IO_ReadMultipleData16(uint8_t Cmd, uint16_t *pData, uint32_t Size)
{
  uint32_t counter;

  LCD_RS_CMD;
  LCD_CS_ON;
  LCD_DATA_WRITE(Cmd);

  LCD_SPI_WAIT_TX_BUSY;
  LCD_RS_DATA;

  LCD_SPI_DUMMYCLK;           // Dummy clock
  LCD_DATA_DIRREAD;

  for (counter = Size; counter != 0; counter--)
  {
    /* Need to invert bytes for LCD*/
    LCD_DATA_READ(*((uint8_t *)pData + 1));
    LCD_DATA_READ(*pData);
    pData++;
  }
  LCD_CS_OFF;
  LCD_DATA_DIRWRITE;
}
#endif

//-----------------------------------------------------------------------------
#if  LCD_READMULTIPLEDATA24TO16 == 1
void LCD_IO_ReadMultipleData16(uint8_t Cmd, uint16_t *pData, uint32_t Size)
{
  uint32_t counter;
  uint8_t rgb888[3];

  LCD_RS_CMD;
  LCD_CS_ON;
  LCD_DATA_WRITE(Cmd);

  LCD_SPI_WAIT_TX_BUSY;
  LCD_RS_DATA;

  LCD_SPI_DUMMYCLK;           // Dummy clock
  LCD_DATA_DIRREAD;

  LCD_DATA_READ(rgb888[0]);   // Dummy data
  for (counter = Size; counter != 0; counter--)
  {
    LCD_DATA_READ(rgb888[0]);
    LCD_DATA_READ(rgb888[1]);
    LCD_DATA_READ(rgb888[2]);
    *pData = ((rgb888[0] & 0b11111000) << 8 | (rgb888[1] & 0b11111100) << 3 | rgb888[2] >> 3);
    pData++;
  }
  LCD_CS_OFF;
  LCD_DATA_DIRWRITE;
}
#endif

#endif // #else LCD_SPI_MODE == 0
