/*
 * SPI LCD driver STM32F1
 * készitö: Roberto Benjami
 * verzio:  2019.06
 *
 * - hardver és szoftver SPI
 * - 3 féle üzemmód (csak TX, half duplex, full duplex)
 * - hardver SPI: DMA
*/

//-----------------------------------------------------------------------------
#include "main.h"
#include "lcd.h"
#include "lcd_io_spi.h"

#if LCD_REVERSE16 == 0
#define RD(a)                 a
#endif

#if LCD_REVERSE16 == 1
#define RD(a)                 __REVSH(a)
#endif

#define DMA_MAXSIZE           0xFFFE

//-----------------------------------------------------------------------------
/* Link function for LCD peripheral */
void  LCD_Delay (uint32_t delay);
void  LCD_IO_Init(void);
void  LCD_IO_Bl_OnOff(uint8_t Bl);

void  LCD_IO_WriteCmd8(uint8_t Cmd);
void  LCD_IO_WriteCmd16(uint16_t Cmd);
void  LCD_IO_WriteData8(uint8_t Data);
void  LCD_IO_WriteData16(uint16_t Data);

void  LCD_IO_WriteCmd8DataFill16(uint8_t Cmd, uint16_t Data, uint32_t Size);
void  LCD_IO_WriteCmd8MultipleData8(uint8_t Cmd, uint8_t *pData, uint32_t Size);
void  LCD_IO_WriteCmd8MultipleData16(uint8_t Cmd, uint16_t *pData, uint32_t Size);
void  LCD_IO_WriteCmd16DataFill16(uint16_t Cmd, uint16_t Data, uint32_t Size);
void  LCD_IO_WriteCmd16MultipleData8(uint16_t Cmd, uint8_t *pData, uint32_t Size);
void  LCD_IO_WriteCmd16MultipleData16(uint16_t Cmd, uint16_t *pData, uint32_t Size);

void  LCD_IO_ReadCmd8MultipleData8(uint8_t Cmd, uint8_t *pData, uint32_t Size, uint32_t DummySize);
void  LCD_IO_ReadCmd8MultipleData16(uint8_t Cmd, uint16_t *pData, uint32_t Size, uint32_t DummySize);
void  LCD_IO_ReadCmd8MultipleData24to16(uint8_t Cmd, uint16_t *pData, uint32_t Size, uint32_t DummySize);
void  LCD_IO_ReadCmd16MultipleData8(uint16_t Cmd, uint8_t *pData, uint32_t Size, uint32_t DummySize);
void  LCD_IO_ReadCmd16MultipleData16(uint16_t Cmd, uint16_t *pData, uint32_t Size, uint32_t DummySize);
void  LCD_IO_ReadCmd16MultipleData24to16(uint16_t Cmd, uint16_t *pData, uint32_t Size, uint32_t DummySize);

//-----------------------------------------------------------------------------
#define BITBAND_ACCESS(a, b)  *(volatile uint32_t*)(((uint32_t)&a & 0xF0000000) + 0x2000000 + (((uint32_t)&a & 0x000FFFFF) << 5) + (b << 2))

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

#define GPIOX_MODER_(a, b, c) ((GPIO_TypeDef*)(((c & 8) >> 1) + GPIO ## b ## _BASE))->CRL = (((GPIO_TypeDef*)(((c & 8) >> 1) + GPIO ## b ## _BASE))->CRL & ~(0xF << ((c & 7) << 2))) | (a << ((c & 7) << 2));
#define GPIOX_MODER(a, b)     GPIOX_MODER_(a, b)

#define GPIOX_ODR_(a, b)      BITBAND_ACCESS(GPIO ## a ->ODR, b)
#define GPIOX_ODR(a)          GPIOX_ODR_(a)

#define GPIOX_IDR_(a, b)      BITBAND_ACCESS(GPIO ## a ->IDR, b)
#define GPIOX_IDR(a)          GPIOX_IDR_(a)

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
#define GPIOX_PORTNUM_J       9
#define GPIOX_PORTNUM_K       10
#define GPIOX_PORTNUM_L       11
#define GPIOX_PORTNUM_M       12
#define GPIOX_PORTNUM_(a, b)  GPIOX_PORTNUM_ ## a
#define GPIOX_PORTNAME_(a, b) a
#define GPIOX_PORTNUM(a)      GPIOX_PORTNUM_(a)
#define GPIOX_PORTNAME(a)     GPIOX_PORTNAME_(a)

//-----------------------------------------------------------------------------
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
/* Parancs/adat láb üzemmod */
#define LCD_RS_CMD            GPIOX_ODR(LCD_RS) = 0
#define LCD_RS_DATA           GPIOX_ODR(LCD_RS) = 1

/* Reset láb aktiv/passziv */
#define LCD_RST_ON            GPIOX_ODR(LCD_RST) = 0
#define LCD_RST_OFF           GPIOX_ODR(LCD_RST) = 1

/* Chip select láb */
#define LCD_CS_ON             GPIOX_ODR(LCD_CS) = 0
#define LCD_CS_OFF            GPIOX_ODR(LCD_CS) = 1

/* Ha az olvasási sebesség nincs smegadva akkor megegyezik az irási sebességgel */
#ifndef LCD_SPI_SPD_READ
#define LCD_SPI_SPD_READ      LCD_SPI_SPD
#endif

//-----------------------------------------------------------------------------
#if LCD_SPI == 0
/* Szoftver SPI */
volatile uint16_t tmp16;

#if     LCD_SPI_MODE == 1
/* Kétirányu (halfduplex) SPI esetén az adatirányt váltogatni kell, és MISO láb = MOSI láb */
#undef  LCD_MISO
#define LCD_MISO              LCD_MOSI

#define LCD_DIRREAD(d)        {                 \
  GPIOX_MODER(MODE_PU_DIGITAL_INPUT, LCD_MOSI); \
  GPIOX_ODR(LCD_MOSI) = 0;                      \
  LCD_READ_DELAY;                               \
  while(d--)                                    \
  {                                             \
    GPIOX_ODR(LCD_SCK) = 0;                     \
    LCD_READ_DELAY;                             \
    GPIOX_ODR(LCD_SCK) = 1;                     \
  }                                             }

#define LCD_DIRWRITE(d)       GPIOX_MODER(MODE_PP_OUT_50MHZ, LCD_MOSI)
#else
/* TX és fullduplex SPI esetén az adatirány fix */
#define LCD_DIRREAD(d)
#define LCD_DIRWRITE(d)
#endif

/* Write SPI sebesség */
#if     LCD_SPI_SPD == 0
#define LCD_WRITE_DELAY
#elif   LCD_SPI_SPD == 1
#define LCD_WRITE_DELAY       GPIOX_ODR(LCD_SCK) = 0
#else
#define LCD_WRITE_DELAY       LCD_IO_Delay(LCD_SPI_SPD - 2)
#endif

/* Read SPI sebesség */
#if     LCD_SPI_SPD_READ == 0
#define LCD_READ_DELAY
#elif   LCD_SPI_SPD_READ == 1
#define LCD_READ_DELAY        GPIOX_ODR(LCD_SCK) = 0
#else
#define LCD_READ_DELAY        LCD_IO_Delay(LCD_SPI_SPD_READ - 2)
#endif

#define LCD_WRITE_CLK         GPIOX_ODR(LCD_SCK) = 0; LCD_WRITE_DELAY; GPIOX_ODR(LCD_SCK) = 1;
#define LCD_READ_CLK          GPIOX_ODR(LCD_SCK) = 1; GPIOX_ODR(LCD_SCK) = 0; LCD_READ_DELAY;

#define LCD_WRITE8(d8) {                         \
  GPIOX_ODR(LCD_MOSI) = BITBAND_ACCESS(d8, 7);   \
  LCD_WRITE_CLK;                                 \
  GPIOX_ODR(LCD_MOSI) = BITBAND_ACCESS(d8, 6);   \
  LCD_WRITE_CLK;                                 \
  GPIOX_ODR(LCD_MOSI) = BITBAND_ACCESS(d8, 5);   \
  LCD_WRITE_CLK;                                 \
  GPIOX_ODR(LCD_MOSI) = BITBAND_ACCESS(d8, 4);   \
  LCD_WRITE_CLK;                                 \
  GPIOX_ODR(LCD_MOSI) = BITBAND_ACCESS(d8, 3);   \
  LCD_WRITE_CLK;                                 \
  GPIOX_ODR(LCD_MOSI) = BITBAND_ACCESS(d8, 2);   \
  LCD_WRITE_CLK;                                 \
  GPIOX_ODR(LCD_MOSI) = BITBAND_ACCESS(d8, 1);   \
  LCD_WRITE_CLK;                                 \
  GPIOX_ODR(LCD_MOSI) = BITBAND_ACCESS(d8, 0);   \
  LCD_WRITE_CLK;                                 }

#define LCD_READ8(d8) {                          \
  GPIOX_ODR(LCD_SCK) = 0;                        \
  LCD_READ_DELAY;                                \
  BITBAND_ACCESS(d8, 7) = GPIOX_IDR(LCD_MISO);   \
  LCD_READ_CLK;                                  \
  BITBAND_ACCESS(d8, 6) = GPIOX_IDR(LCD_MISO);   \
  LCD_READ_CLK;                                  \
  BITBAND_ACCESS(d8, 5) = GPIOX_IDR(LCD_MISO);   \
  LCD_READ_CLK;                                  \
  BITBAND_ACCESS(d8, 4) = GPIOX_IDR(LCD_MISO);   \
  LCD_READ_CLK;                                  \
  BITBAND_ACCESS(d8, 3) = GPIOX_IDR(LCD_MISO);   \
  LCD_READ_CLK;                                  \
  BITBAND_ACCESS(d8, 2) = GPIOX_IDR(LCD_MISO);   \
  LCD_READ_CLK;                                  \
  BITBAND_ACCESS(d8, 1) = GPIOX_IDR(LCD_MISO);   \
  LCD_READ_CLK;                                  \
  BITBAND_ACCESS(d8, 0) = GPIOX_IDR(LCD_MISO);   \
  GPIOX_ODR(LCD_SCK) = 1;                        }

#if LCD_REVERSE16 == 0
#define LCD_WRITE16(d16) {                       \
  GPIOX_ODR(LCD_MOSI) = BITBAND_ACCESS(d16, 15); \
  LCD_WRITE_CLK;                                 \
  GPIOX_ODR(LCD_MOSI) = BITBAND_ACCESS(d16, 14); \
  LCD_WRITE_CLK;                                 \
  GPIOX_ODR(LCD_MOSI) = BITBAND_ACCESS(d16, 13); \
  LCD_WRITE_CLK;                                 \
  GPIOX_ODR(LCD_MOSI) = BITBAND_ACCESS(d16, 12); \
  LCD_WRITE_CLK;                                 \
  GPIOX_ODR(LCD_MOSI) = BITBAND_ACCESS(d16, 11); \
  LCD_WRITE_CLK;                                 \
  GPIOX_ODR(LCD_MOSI) = BITBAND_ACCESS(d16, 10); \
  LCD_WRITE_CLK;                                 \
  GPIOX_ODR(LCD_MOSI) = BITBAND_ACCESS(d16, 9);  \
  LCD_WRITE_CLK;                                 \
  GPIOX_ODR(LCD_MOSI) = BITBAND_ACCESS(d16, 8);  \
  LCD_WRITE_CLK;                                 \
  GPIOX_ODR(LCD_MOSI) = BITBAND_ACCESS(d16, 7);  \
  LCD_WRITE_CLK;                                 \
  GPIOX_ODR(LCD_MOSI) = BITBAND_ACCESS(d16, 6);  \
  LCD_WRITE_CLK;                                 \
  GPIOX_ODR(LCD_MOSI) = BITBAND_ACCESS(d16, 5);  \
  LCD_WRITE_CLK;                                 \
  GPIOX_ODR(LCD_MOSI) = BITBAND_ACCESS(d16, 4);  \
  LCD_WRITE_CLK;                                 \
  GPIOX_ODR(LCD_MOSI) = BITBAND_ACCESS(d16, 3);  \
  LCD_WRITE_CLK;                                 \
  GPIOX_ODR(LCD_MOSI) = BITBAND_ACCESS(d16, 2);  \
  LCD_WRITE_CLK;                                 \
  GPIOX_ODR(LCD_MOSI) = BITBAND_ACCESS(d16, 1);  \
  LCD_WRITE_CLK;                                 \
  GPIOX_ODR(LCD_MOSI) = BITBAND_ACCESS(d16, 0);  \
  LCD_WRITE_CLK;                                 }

#define LCD_READ16(d16) {                        \
  GPIOX_ODR(LCD_SCK) = 0;                        \
  LCD_READ_DELAY;                                \
  BITBAND_ACCESS(d16, 15) = GPIOX_IDR(LCD_MISO); \
  LCD_READ_CLK;                                  \
  BITBAND_ACCESS(d16, 14) = GPIOX_IDR(LCD_MISO); \
  LCD_READ_CLK;                                  \
  BITBAND_ACCESS(d16, 13) = GPIOX_IDR(LCD_MISO); \
  LCD_READ_CLK;                                  \
  BITBAND_ACCESS(d16, 12) = GPIOX_IDR(LCD_MISO); \
  LCD_READ_CLK;                                  \
  BITBAND_ACCESS(d16, 11) = GPIOX_IDR(LCD_MISO); \
  LCD_READ_CLK;                                  \
  BITBAND_ACCESS(d16, 10) = GPIOX_IDR(LCD_MISO); \
  LCD_READ_CLK;                                  \
  BITBAND_ACCESS(d16, 9) = GPIOX_IDR(LCD_MISO);  \
  LCD_READ_CLK;                                  \
  BITBAND_ACCESS(d16, 8) = GPIOX_IDR(LCD_MISO);  \
  LCD_READ_CLK;                                  \
  BITBAND_ACCESS(d16, 7) = GPIOX_IDR(LCD_MISO);  \
  LCD_READ_CLK;                                  \
  BITBAND_ACCESS(d16, 6) = GPIOX_IDR(LCD_MISO);  \
  LCD_READ_CLK;                                  \
  BITBAND_ACCESS(d16, 5) = GPIOX_IDR(LCD_MISO);  \
  LCD_READ_CLK;                                  \
  BITBAND_ACCESS(d16, 4) = GPIOX_IDR(LCD_MISO);  \
  LCD_READ_CLK;                                  \
  BITBAND_ACCESS(d16, 3) = GPIOX_IDR(LCD_MISO);  \
  LCD_READ_CLK;                                  \
  BITBAND_ACCESS(d16, 2) = GPIOX_IDR(LCD_MISO);  \
  LCD_READ_CLK;                                  \
  BITBAND_ACCESS(d16, 1) = GPIOX_IDR(LCD_MISO);  \
  LCD_READ_CLK;                                  \
  BITBAND_ACCESS(d16, 0) = GPIOX_IDR(LCD_MISO);  \
  GPIOX_ODR(LCD_SCK) = 1;                        }
#endif

#if LCD_REVERSE16 == 1
#define LCD_WRITE16(d16) {                       \
  GPIOX_ODR(LCD_MOSI) = BITBAND_ACCESS(d16, 7);  \
  LCD_WRITE_CLK;                                 \
  GPIOX_ODR(LCD_MOSI) = BITBAND_ACCESS(d16, 6);  \
  LCD_WRITE_CLK;                                 \
  GPIOX_ODR(LCD_MOSI) = BITBAND_ACCESS(d16, 5);  \
  LCD_WRITE_CLK;                                 \
  GPIOX_ODR(LCD_MOSI) = BITBAND_ACCESS(d16, 4);  \
  LCD_WRITE_CLK;                                 \
  GPIOX_ODR(LCD_MOSI) = BITBAND_ACCESS(d16, 3);  \
  LCD_WRITE_CLK;                                 \
  GPIOX_ODR(LCD_MOSI) = BITBAND_ACCESS(d16, 2);  \
  LCD_WRITE_CLK;                                 \
  GPIOX_ODR(LCD_MOSI) = BITBAND_ACCESS(d16, 1);  \
  LCD_WRITE_CLK;                                 \
  GPIOX_ODR(LCD_MOSI) = BITBAND_ACCESS(d16, 0);  \
  LCD_WRITE_CLK;                                 \
  GPIOX_ODR(LCD_MOSI) = BITBAND_ACCESS(d16, 15); \
  LCD_WRITE_CLK;                                 \
  GPIOX_ODR(LCD_MOSI) = BITBAND_ACCESS(d16, 14); \
  LCD_WRITE_CLK;                                 \
  GPIOX_ODR(LCD_MOSI) = BITBAND_ACCESS(d16, 13); \
  LCD_WRITE_CLK;                                 \
  GPIOX_ODR(LCD_MOSI) = BITBAND_ACCESS(d16, 12); \
  LCD_WRITE_CLK;                                 \
  GPIOX_ODR(LCD_MOSI) = BITBAND_ACCESS(d16, 11); \
  LCD_WRITE_CLK;                                 \
  GPIOX_ODR(LCD_MOSI) = BITBAND_ACCESS(d16, 10); \
  LCD_WRITE_CLK;                                 \
  GPIOX_ODR(LCD_MOSI) = BITBAND_ACCESS(d16, 9);  \
  LCD_WRITE_CLK;                                 \
  GPIOX_ODR(LCD_MOSI) = BITBAND_ACCESS(d16, 8);  \
  LCD_WRITE_CLK;                                 \

#define LCD_READ16(d16) {                        \
  GPIOX_ODR(LCD_SCK) = 0;                        \
  LCD_READ_DELAY;                                \
  BITBAND_ACCESS(d16, 7) = GPIOX_IDR(LCD_MISO);  \
  LCD_READ_CLK;                                  \
  BITBAND_ACCESS(d16, 6) = GPIOX_IDR(LCD_MISO);  \
  LCD_READ_CLK;                                  \
  BITBAND_ACCESS(d16, 5) = GPIOX_IDR(LCD_MISO);  \
  LCD_READ_CLK;                                  \
  BITBAND_ACCESS(d16, 4) = GPIOX_IDR(LCD_MISO);  \
  LCD_READ_CLK;                                  \
  BITBAND_ACCESS(d16, 3) = GPIOX_IDR(LCD_MISO);  \
  LCD_READ_CLK;                                  \
  BITBAND_ACCESS(d16, 2) = GPIOX_IDR(LCD_MISO);  \
  LCD_READ_CLK;                                  \
  BITBAND_ACCESS(d16, 1) = GPIOX_IDR(LCD_MISO);  \
  LCD_READ_CLK;                                  \
  BITBAND_ACCESS(d16, 0) = GPIOX_IDR(LCD_MISO);  \
  LCD_READ_CLK;                                  \
  BITBAND_ACCESS(d16, 15) = GPIOX_IDR(LCD_MISO); \
  LCD_READ_CLK;                                  \
  BITBAND_ACCESS(d16, 14) = GPIOX_IDR(LCD_MISO); \
  LCD_READ_CLK;                                  \
  BITBAND_ACCESS(d16, 13) = GPIOX_IDR(LCD_MISO); \
  LCD_READ_CLK;                                  \
  BITBAND_ACCESS(d16, 12) = GPIOX_IDR(LCD_MISO); \
  LCD_READ_CLK;                                  \
  BITBAND_ACCESS(d16, 11) = GPIOX_IDR(LCD_MISO); \
  LCD_READ_CLK;                                  \
  BITBAND_ACCESS(d16, 10) = GPIOX_IDR(LCD_MISO); \
  LCD_READ_CLK;                                  \
  BITBAND_ACCESS(d16, 9) = GPIOX_IDR(LCD_MISO);  \
  LCD_READ_CLK;                                  \
  BITBAND_ACCESS(d16, 8) = GPIOX_IDR(LCD_MISO);  \
  GPIOX_ODR(LCD_SCK) = 1;                        }
#endif

#define LCD_DATA8_WRITE(d8)     {tmp8 = d8; LCD_WRITE8(tmp8);}
#define LCD_DATA8_READ(d8)      LCD_READ8(tmp8); d8 = tmp8;
#define LCD_CMD8_WRITE(cmd8)    {LCD_RS_CMD; LCD_WRITE8(cmd8); LCD_RS_DATA;}
#define LCD_DATA16_WRITE(d16)   {tmp16 = d16; LCD_WRITE16(tmp16);}
#define LCD_DATA16_READ(d16)    LCD_READ16(tmp16); d16 = tmp16;
#define LCD_CMD16_WRITE(cmd16)  {LCD_RS_CMD; LCD_WRITE16(cmd16); LCD_RS_DATA;}
#define LCD_SPI_MODE8
#define LCD_SPI_MODE16

#else    // #if LCD_SPI == 0
/* Hardver SPI */

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

/* Read SPI sebesség */
#define LCD_READ_DELAY        LCD_IO_Delay(LCD_SPI_SPD_READ * 4)

#define LCD_DUMMY_READ(d)     {GPIOX_MODER(MODE_PP_OUT_50MHZ, LCD_SCK);  \
                               while(d--)                                \
                               {                                         \
                                 GPIOX_ODR(LCD_SCK) = 0;                 \
                                 LCD_READ_DELAY;                         \
                                 GPIOX_ODR(LCD_SCK) = 1;                 \
                               }                                         \
                               GPIOX_MODER(MODE_PP_ALTER_50MHZ, LCD_SCK);}

#define LCD_SPI_MODE8         BITBAND_ACCESS(SPIX->CR1, SPI_CR1_DFF_Pos) = 0
#define LCD_SPI_MODE16        BITBAND_ACCESS(SPIX->CR1, SPI_CR1_DFF_Pos) = 1

#if     LCD_SPI_MODE == 1

/* Kétirányu (halfduplex) SPI esetén az adatirányt váltogatni kell */
#if (defined(LCD_SPI_SPD_READ) && (LCD_SPI_SPD != LCD_SPI_SPD_READ)) // Eltérö olvasási/irási sebesség
#define LCD_DIRREAD(d)  { LCD_DUMMY_READ(d); SPIX->CR1 = (SPIX->CR1 & ~(SPI_CR1_BR | SPI_CR1_BIDIOE)) | (LCD_SPI_SPD_READ << SPI_CR1_BR_Pos); }
#define LCD_DIRWRITE(d8) {                           \
  while(BITBAND_ACCESS(SPIX->SR, SPI_SR_RXNE_Pos))   \
    d8 = *(uint8_t *)&SPIX->DR;                      \
  SPIX->CR1 = (SPIX->CR1 & ~SPI_CR1_BR) |            \
    ((LCD_SPI_SPD << SPI_CR1_BR_Pos) |               \
    SPI_CR1_BIDIOE);                                 \
  LCD_IO_Delay(2 ^ LCD_SPI_SPD_READ);                \
  while(BITBAND_ACCESS(SPIX->SR, SPI_SR_RXNE_Pos))   \
    d8 = *(uint8_t *)&SPIX->DR;                      }

#else   // azonos sebesség, csak LCD_MOSI irányváltás
#define LCD_DIRREAD(d)   { LCD_DUMMY_READ(d); BITBAND_ACCESS(SPIX->CR1, SPI_CR1_BIDIOE_Pos) = 0; }
#define LCD_DIRWRITE(d8) {                           \
  while(BITBAND_ACCESS(SPIX->SR, SPI_SR_RXNE_Pos))   \
    d8 = *(uint8_t *)&SPIX->DR;                      \
  SPIX->CR1 |= SPI_CR1_BIDIOE_Msk;                   \
  LCD_IO_Delay(2 ^ LCD_SPI_SPD_READ);                \
  while(BITBAND_ACCESS(SPIX->SR, SPI_SR_RXNE_Pos))   \
    d8 = *(uint8_t *)&SPIX->DR;                      }
#endif

#else   // LCD_SPI_MODE == 1
// TX és fullduplex SPI esetén az adatirány fix
#if (defined(LCD_SPI_SPD_READ) && (LCD_SPI_SPD != LCD_SPI_SPD_READ)) // Eltérö olvasási/irási sebesség
#define LCD_DIRREAD(d)   { LCD_DUMMY_READ(d); SPIX->CR1 = (SPIX->CR1 & ~SPI_CR1_BR) | (LCD_SPI_SPD_READ << SPI_CR1_BR_Pos); }
#define LCD_DIRWRITE(d8) { SPIX->CR1 = (SPIX->CR1 & ~SPI_CR1_BR) | (LCD_SPI_SPD << SPI_CR1_BR_Pos); }
#else   // nincs irányváltás és sebességeltérés sem
#define LCD_DIRREAD(d)   LCD_DUMMY_READ(d)
#define LCD_DIRWRITE(d8)
#endif
#endif  // #else LCD_SPI_MODE == 1

#define LCD_DATA8_WRITE(d8)    { *(volatile uint8_t *)&SPIX->DR = d8; LCD_IO_Delay(1); while(BITBAND_ACCESS(SPIX->SR, SPI_SR_BSY_Pos)); }
#define LCD_DATA8_READ(d8)     { while(!BITBAND_ACCESS(SPIX->SR, SPI_SR_RXNE_Pos)); d8 = (uint8_t)SPIX->DR; }
#define LCD_CMD8_WRITE(cmd8)   { LCD_RS_CMD; LCD_DATA8_WRITE(cmd8); LCD_RS_DATA; }
#define LCD_DATA16_WRITE(d16)  { SPIX->DR = RD(d16); LCD_IO_Delay(1); while(BITBAND_ACCESS(SPIX->SR, SPI_SR_BSY_Pos)); }
#define LCD_DATA16_READ(d16)   { while(!BITBAND_ACCESS(SPIX->SR, SPI_SR_RXNE_Pos)); d16 = RD(SPIX->DR); }
#define LCD_CMD16_WRITE(cmd16) { LCD_RS_CMD; LCD_DATA16_WRITE(cmd16); LCD_RS_DATA; }

#if DMANUM(LCD_DMA_TX) > 0 || DMANUM(LCD_DMA_RX) > 0
#ifdef  osFeature_Semaphore
#if DMANUM(LCD_DMA_TX) > 0
#define WAIT_FOR_DMA_END_TX   osSemaphoreWait(BinarySemDmaHandle, osWaitForever)
#endif
#if DMANUM(LCD_DMA_RX) > 0
#define WAIT_FOR_DMA_END_RX   osSemaphoreWait(BinarySemDmaHandle, osWaitForever)
#endif
#define DMAX_CCR_TCIE         DMA_CCR_TCIE
#define LCD_DMA_IRQ
#else  // #ifdef  osFeature_Semaphore
#if DMANUM(LCD_DMA_TX) > 0
#define WAIT_FOR_DMA_END_TX   while(!(DMAX(LCD_DMA_TX)->ISR & DMAX_ISR_TCIF(LCD_DMA_TX)));
#endif
#if DMANUM(LCD_DMA_RX) > 0
#define WAIT_FOR_DMA_END_RX   while(!(DMAX(LCD_DMA_RX)->ISR & DMAX_ISR_TCIF(LCD_DMA_RX)));
#endif
#define DMAX_CCR_TCIE         0
#endif // #else  osFeature_Semaphore
#endif // #if DMANUM(LCD_DMA_TX) > 0 || DMANUM(LCD_DMA_RX) > 0

#if DMANUM(LCD_DMA_TX) > 0
/* SPI DMA WRITE(a: data pointer, b: number of data, c: 0=8 bit, 1=16bit, d: 0:MINC=off, 1:MINC=on */
#define LCD_SPI_DMA_WRITE(a, b, c, d) {                                         \
  DMAX(LCD_DMA_TX)->IFCR = DMAX_IFCR_CGIF(LCD_DMA_TX);                          \
  while(b)                                                                      \
  {                                                                             \
    DMAX_CHANNEL(LCD_DMA_TX)->CCR = 0;   /* DMA stop */                         \
    while(DMAX_CHANNEL(LCD_DMA_TX)->CCR & DMA_CCR_EN);                          \
    DMAX_CHANNEL(LCD_DMA_TX)->CMAR = (uint32_t)a;                               \
    DMAX_CHANNEL(LCD_DMA_TX)->CPAR = (uint32_t)&SPIX->DR;                       \
    if(b > DMA_MAXSIZE)                                                         \
    {                                                                           \
      DMAX_CHANNEL(LCD_DMA_TX)->CNDTR = DMA_MAXSIZE;                            \
      b -= DMA_MAXSIZE;                                                         \
    }                                                                           \
    else                                                                        \
    {                                                                           \
      DMAX_CHANNEL(LCD_DMA_TX)->CNDTR = b;                                      \
      b = 0;                                                                    \
    }                                                                           \
    DMAX_CHANNEL(LCD_DMA_TX)->CCR = DMAX_CCR_TCIE | (c << DMA_CCR_MSIZE_Pos) |  \
      (c << DMA_CCR_PSIZE_Pos) | DMA_CCR_DIR | (d << DMA_CCR_MINC_Pos) |        \
      (DMAPRIORITY(LCD_DMA_TX) << DMA_CCR_PL_Pos);                              \
    DMAX_CHANNEL(LCD_DMA_TX)->CCR |= DMA_CCR_EN;                                \
    BITBAND_ACCESS(SPIX->CR2, SPI_CR2_TXDMAEN_Pos) = 1;                         \
    WAIT_FOR_DMA_END_TX;                                                        \
  }                                                                             \
  DMAX_CHANNEL(LCD_DMA_TX)->CCR = 0;                                            \
  while(DMAX_CHANNEL(LCD_DMA_TX)->CCR & DMA_CCR_EN);                            \
  BITBAND_ACCESS(SPIX->CR2, SPI_CR2_TXDMAEN_Pos) = 0;                           \
  while(BITBAND_ACCESS(SPIX->SR, SPI_SR_BSY_Pos));                              \
  LCD_IO_Delay(2 ^ LCD_SPI_SPD);                                                }
#endif     // #if DMANUM(LCD_DMA_TX) > 0

#if DMANUM(LCD_DMA_RX) > 0
/* SPI DMA READ(a: data pointer, b: number of data, c: 0=8 bit, 1=16bit */
#define LCD_SPI_DMA_READ(a, b, c) {                                             \
  DMAX(LCD_DMA_RX)->IFCR = DMAX_IFCR_CGIF(LCD_DMA_RX); /* DMA IRQ req törlés */ \
  DMAX_CHANNEL(LCD_DMA_RX)->CCR = 0;  /* DMA stop */                            \
  while(DMAX_CHANNEL(LCD_DMA_RX)->CCR & DMA_CCR_EN);                            \
  DMAX_CHANNEL(LCD_DMA_RX)->CMAR = (uint32_t)a;  /* memory addr */              \
  DMAX_CHANNEL(LCD_DMA_RX)->CPAR = (uint32_t)&SPIX->DR; /* periph addr */       \
  DMAX_CHANNEL(LCD_DMA_RX)->CNDTR = b;           /* number of data */           \
  DMAX_CHANNEL(LCD_DMA_RX)->CCR = DMAX_CCR_TCIE | (c << DMA_CCR_MSIZE_Pos) |    \
    (c << DMA_CCR_PSIZE_Pos) | DMA_CCR_MINC |    /* bitdepht */                 \
    (DMAPRIORITY(LCD_DMA_RX) << DMA_CCR_PL_Pos);                                \
  DMAX_CHANNEL(LCD_DMA_RX)->CCR |= DMA_CCR_EN;  /* DMA start */                 \
  BITBAND_ACCESS(SPIX->CR2, SPI_CR2_RXDMAEN_Pos) = 1; /* SPI DMA eng */         \
  WAIT_FOR_DMA_END_RX;                                                          \
  BITBAND_ACCESS(SPIX->CR2, SPI_CR2_RXDMAEN_Pos) = 0; /* SPI DMA tilt */        \
  while(BITBAND_ACCESS(SPIX->SR, SPI_SR_RXNE_Pos))                              \
    tmp8 = *(uint8_t *)&SPIX->DR;                                               \
  SPIX->CR1 = (SPIX->CR1 & ~SPI_CR1_BR) |                                       \
    ((LCD_SPI_SPD << SPI_CR1_BR_Pos) | SPI_CR1_BIDIOE);                         \
  LCD_IO_Delay(2 ^ LCD_SPI_SPD_READ);                                           \
  while(BITBAND_ACCESS(SPIX->SR, SPI_SR_RXNE_Pos))                              \
    tmp8 = *(uint8_t *)&SPIX->DR;                                               \
  DMAX_CHANNEL(LCD_DMA_RX)->CCR = 0;                                            \
  while(DMAX_CHANNEL(LCD_DMA_RX)->CCR & DMA_CCR_EN);                            }

  /* SPI DMA READ(a: data pointer, b: number of data, c: tempbuffer pointer, d: tempbuffer size */
#define LCD_SPI_DMA_READ24TO16(a, b, c, d) {                                    \
  uint32_t rp = 0, rgbcnt = 0;        /* buffer olv pointer */                  \
  uint32_t nd;                                                                  \
  DMAX(LCD_DMA_RX)->IFCR = DMAX_IFCR_CGIF(LCD_DMA_RX);                          \
  DMAX_CHANNEL(LCD_DMA_RX)->CCR = 0;  /* DMA stop */                            \
  while(DMAX_CHANNEL(LCD_DMA_RX)->CCR & DMA_CCR_EN);                            \
  DMAX_CHANNEL(LCD_DMA_RX)->CMAR = (uint32_t)c;                                 \
  DMAX_CHANNEL(LCD_DMA_RX)->CPAR = (uint32_t)&SPIX->DR;                         \
  DMAX_CHANNEL(LCD_DMA_RX)->CNDTR = d;                                          \
  nd = d;                                                                       \
  DMAX_CHANNEL(LCD_DMA_RX)->CCR = (0b00 << DMA_CCR_MSIZE_Pos) |                 \
    (0b00 << DMA_CCR_PSIZE_Pos) | DMA_CCR_MINC |                                \
    (DMAPRIORITY(LCD_DMA_RX) << DMA_CCR_PL_Pos) | DMA_CCR_CIRC;                 \
  DMAX_CHANNEL(LCD_DMA_RX)->CCR |= DMA_CCR_EN;                                  \
  BITBAND_ACCESS(SPIX->CR2, SPI_CR2_RXDMAEN_Pos) = 1;                           \
  while(b)                                                                      \
  {                                                                             \
    if(nd != DMAX_CHANNEL(LCD_DMA_RX)->CNDTR)                                   \
    {                                                                           \
      if(!--nd)                                                                 \
        nd = d;                                                                 \
      rgb888[rgbcnt++] = da[rp++];                                              \
      rp &= (d - 1);                                                            \
      if(rgbcnt == 3)                                                           \
      {                                                                         \
        rgbcnt = 0;                                                             \
        b--;                                                                    \
        *a++ = RD((rgb888[0] & 0b11111000) << 8 |                               \
                  (rgb888[1] & 0b11111100) << 3 | rgb888[2] >> 3);              \
      }                                                                         \
    }                                                                           \
  }                                                                             \
  BITBAND_ACCESS(SPIX->CR2, SPI_CR2_RXDMAEN_Pos) = 0;                           \
  while(BITBAND_ACCESS(SPIX->SR, SPI_SR_RXNE_Pos))                              \
    tmp8 = *(uint8_t *)&SPIX->DR;                                               \
  SPIX->CR1 = (SPIX->CR1 & ~SPI_CR1_BR) |                                       \
    ((LCD_SPI_SPD << SPI_CR1_BR_Pos) | SPI_CR1_BIDIOE);                         \
  LCD_IO_Delay(2 ^ LCD_SPI_SPD_READ);                                           \
  while(BITBAND_ACCESS(SPIX->SR, SPI_SR_RXNE_Pos))                              \
    tmp8 = *(uint8_t *)&SPIX->DR;                                               \
  DMAX_CHANNEL(LCD_DMA_RX)->CCR = 0;                                            \
  while(DMAX_CHANNEL(LCD_DMA_RX)->CCR & DMA_CCR_EN);                            }
#endif   // #if DMANUM(LCD_DMA_RX) > 0

#endif   // #else LCD_SPI == 0

uint8_t   tmp8;

#if DMANUM(LCD_DMA_RX) > 0 && LCD_DMA_RX_BUFMODE == 1 && LCD_SPI_MODE > 0
uint8_t da[LCD_DMA_RX_BUFSIZE];
#endif

//=============================================================================
#ifdef LCD_DMA_IRQ
osSemaphoreId BinarySemDmaHandle;

#if DMANUM(LCD_DMA_TX) > 0
void DMAX_CHANNEL_IRQHANDLER(LCD_DMA_TX)(void)
{
  if(DMAX(LCD_DMA_TX)->ISR & DMAX_ISR_TCIF(LCD_DMA_TX))
  {
    DMAX(LCD_DMA_TX)->IFCR = DMAX_IFCR_CTCIF(LCD_DMA_TX);
    osSemaphoreRelease(BinarySemDmaHandle);
  }
}

#endif

#if DMANUM(LCD_DMA_RX) > 0
void DMAX_CHANNEL_IRQHANDLER(LCD_DMA_RX)(void)
{
  if(DMAX(LCD_DMA_RX)->ISR & DMAX_ISR_TCIF(LCD_DMA_RX))
  {
    DMAX(LCD_DMA_RX)->IFCR = DMAX_IFCR_CTCIF(LCD_DMA_RX);
    osSemaphoreRelease(BinarySemDmaHandle);
  }
}
#endif

#endif  // #ifdef LCD_DMA_IRQ

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
  #if LCD_SPI_MODE == 2                 // Full duplex
  RCC->APB2ENR |= (GPIOX_CLOCK(LCD_RS) | GPIOX_CLOCK(LCD_CS) | GPIOX_CLOCK(LCD_SCK) | GPIOX_CLOCK(LCD_MOSI) | GPIOX_CLOCK(LCD_MISO));
  GPIOX_MODER(MODE_FF_DIGITAL_INPUT, LCD_MISO);
  #else                                 // TX vagy half duplex
  RCC->APB2ENR |= GPIOX_CLOCK(LCD_RS) | GPIOX_CLOCK(LCD_CS) | GPIOX_CLOCK(LCD_SCK) | GPIOX_CLOCK(LCD_MOSI);
  #endif

  #if GPIOX_PORTNUM(LCD_BL) >= GPIOX_PORTNUM_A    // háttérvilágitás
  RCC->APB2ENR |= GPIOX_CLOCK(LCD_BL);
  GPIOX_MODER(MODE_PP_OUT_2MHZ, LCD_BL);
  LCD_IO_Bl_OnOff(1);
  #endif

  #if GPIOX_PORTNUM(LCD_RST) >= GPIOX_PORTNUM_A   // reset
  RCC->APB2ENR |= GPIOX_CLOCK(LCD_RST);
  GPIOX_MODER(MODE_PP_OUT_2MHZ, LCD_RST);
  LCD_RST_OFF;
  #endif

  LCD_RS_DATA;
  LCD_CS_OFF;
  GPIOX_MODER(MODE_PP_OUT_50MHZ, LCD_RS);
  GPIOX_MODER(MODE_PP_OUT_50MHZ, LCD_CS);
  GPIOX_ODR(LCD_SCK) = 1;               // SCK = 1

  #if LCD_SPI == 0                      // Szoftver SPI
  GPIOX_MODER(MODE_PP_OUT_50MHZ, LCD_SCK);
  GPIOX_MODER(MODE_PP_OUT_50MHZ, LCD_MOSI);

  #else                                 // Hardver SPI
  LCD_SPI_RCC_EN;

  GPIOX_MODER(MODE_PP_ALTER_50MHZ, LCD_SCK);
  GPIOX_MODER(MODE_PP_ALTER_50MHZ, LCD_MOSI);

  #if LCD_SPI_MODE == 1
  // Half duplex (adatirány váltogatás)
  SPIX->CR1 = SPI_CR1_CPHA | SPI_CR1_CPOL | SPI_CR1_MSTR | SPI_CR1_SPE | SPI_CR1_SSM | SPI_CR1_SSI | (LCD_SPI_SPD << SPI_CR1_BR_Pos) | SPI_CR1_BIDIMODE | SPI_CR1_BIDIOE;
  #else // #if LCD_SPI_MODE == 1
  // TX vagy full duplex mod
  SPIX->CR1 = SPI_CR1_CPHA | SPI_CR1_CPOL | SPI_CR1_MSTR | SPI_CR1_SPE | SPI_CR1_SSM | SPI_CR1_SSI | (LCD_SPI_SPD << SPI_CR1_BR_Pos);
  #endif // #else LCD_SPI_MODE == 1

  #if DMANUM(LCD_DMA_TX) > 0 && DMANUM(LCD_DMA_RX) > 0 && DMANUM(LCD_DMA_TX) != DMANUM(LCD_DMA_RX)
  RCC->AHBENR |= RCC_AHBENR_DMA1EN | RCC_AHBENR_DMA2EN;   // DMA1, DMA2 orajel be
  #elif DMANUM(LCD_DMA_TX) == 1 || DMANUM(LCD_DMA_RX) == 1
  RCC->AHBENR |= RCC_AHBENR_DMA1EN;   // DMA1 orajel be
  #elif DMANUM(LCD_DMA_TX) == 2 || DMANUM(LCD_DMA_RX) == 2
  RCC->AHBENR |= RCC_AHBENR_DMA2EN;   // DMA2 orajel be
  #endif

  #endif // #else LCD_SPI == 0

  /* Set or Reset the control line */
  #if GPIOX_PORTNUM(LCD_RST) >= GPIOX_PORTNUM_A   // reset
  LCD_Delay(10);
  LCD_RST_ON;
  LCD_Delay(10);
  LCD_RST_OFF;
  #endif
  LCD_Delay(10);

  #ifdef LCD_DMA_IRQ
  osSemaphoreDef(myBinarySem01);
  BinarySemDmaHandle = osSemaphoreCreate(osSemaphore(myBinarySem01), 1);
  #if DMANUM(LCD_DMA_TX) > 0
  HAL_NVIC_SetPriority(DMAX_CHANNEL_IRQ(LCD_DMA_TX), configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY, 0);
  HAL_NVIC_EnableIRQ(DMAX_CHANNEL_IRQ(LCD_DMA_TX));
  #endif
  #if DMANUM(LCD_DMA_RX) > 0
  HAL_NVIC_SetPriority(DMAX_CHANNEL_IRQ(LCD_DMA_RX), configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY, 0);
  HAL_NVIC_EnableIRQ(DMAX_CHANNEL_IRQ(LCD_DMA_RX));
  #endif
  osSemaphoreWait(BinarySemDmaHandle, 1);
  #endif  // #ifdef LCD_DMA_IRQ
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteCmd8(uint8_t Cmd)
{
  LCD_SPI_MODE8;
  LCD_CS_ON;
  LCD_CMD8_WRITE(Cmd);
  LCD_CS_OFF;
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteCmd16(uint16_t Cmd)
{
  LCD_SPI_MODE16;
  LCD_CS_ON;
  LCD_CMD16_WRITE(Cmd);
  LCD_CS_OFF;
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteData8(uint8_t Data)
{
  LCD_SPI_MODE8;
  LCD_CS_ON;
  LCD_DATA8_WRITE(Data);
  LCD_CS_OFF;
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteData16(uint16_t Data)
{
  LCD_SPI_MODE16;
  LCD_CS_ON;
  LCD_DATA16_WRITE(Data);
  LCD_CS_OFF;
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteCmd8DataFill16(uint8_t Cmd, uint16_t Data, uint32_t Size)
{
  LCD_SPI_MODE8;
  LCD_CS_ON;
  LCD_CMD8_WRITE(Cmd);
  LCD_SPI_MODE16;
  #if DMANUM(LCD_DMA_TX) == 0 || LCD_SPI == 0
  while(Size--)
  {
    LCD_DATA16_WRITE(Data);
  }

  #else
  uint16_t d = RD(Data);
  LCD_SPI_DMA_WRITE(&d, Size, 1, 0);
  #endif

  LCD_CS_OFF;
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteCmd8MultipleData8(uint8_t Cmd, uint8_t *pData, uint32_t Size)
{
  LCD_SPI_MODE8;
  LCD_CS_ON;
  LCD_CMD8_WRITE(Cmd);

  #if DMANUM(LCD_DMA_TX) == 0 || LCD_SPI == 0
  while(Size--)
  {
    LCD_DATA8_WRITE(*pData);
    pData ++;
  }

  #else
  LCD_SPI_DMA_WRITE(pData, Size, 0, 1);
  #endif

  LCD_CS_OFF;
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteCmd8MultipleData16(uint8_t Cmd, uint16_t *pData, uint32_t Size)
{
  LCD_SPI_MODE8;
  LCD_CS_ON;
  LCD_CMD8_WRITE(Cmd);
  LCD_SPI_MODE16;

  #if DMANUM(LCD_DMA_TX) == 0 || LCD_SPI == 0 || LCD_REVERSE16 == 1
  while(Size--)
  {
    LCD_DATA16_WRITE(*pData);
    pData ++;
  }

  #else
  LCD_SPI_DMA_WRITE(pData, Size, 1, 1);
  #endif

  LCD_CS_OFF;
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteCmd16DataFill16(uint16_t Cmd, uint16_t Data, uint32_t Size)
{
  LCD_SPI_MODE16;
  LCD_CS_ON;
  LCD_CMD16_WRITE(Cmd);

  #if DMANUM(LCD_DMA_TX) == 0 || LCD_SPI == 0
  while(Size--)
  {
    LCD_DATA16_WRITE(Data);
  }

  #else
  uint16_t d = RD(Data);
  LCD_SPI_DMA_WRITE(&d, Size, 1, 0);
  #endif

  LCD_CS_OFF;
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteCmd16MultipleData8(uint16_t Cmd, uint8_t *pData, uint32_t Size)
{
  LCD_SPI_MODE16;
  LCD_CS_ON;
  LCD_CMD16_WRITE(Cmd);
  LCD_SPI_MODE8;

  #if DMANUM(LCD_DMA_TX) == 0 || LCD_SPI == 0
  while(Size--)
  {
    LCD_DATA8_WRITE(*pData);
    pData ++;
  }

  #else
  LCD_SPI_DMA_WRITE(pData, Size, 0, 1);
  #endif

  LCD_CS_OFF;
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteCmd16MultipleData16(uint16_t Cmd, uint16_t *pData, uint32_t Size)
{
  LCD_SPI_MODE16;
  LCD_CS_ON;
  LCD_CMD16_WRITE(Cmd);

  #if DMANUM(LCD_DMA_TX) == 0 || LCD_SPI == 0 || LCD_REVERSE16 == 1
  while(Size--)
  {
    LCD_DATA16_WRITE(*pData);
    pData ++;
  }

  #else
  LCD_SPI_DMA_WRITE(pData, Size, 1, 1);
  #endif

  LCD_CS_OFF;
}

#if LCD_SPI_MODE == 0
__weak void LCD_IO_ReadCmd8MultipleData8(uint8_t Cmd, uint8_t *pData, uint32_t Size, uint32_t DummySize) {}
__weak void LCD_IO_ReadCmd8MultipleData16(uint8_t Cmd, uint16_t *pData, uint32_t Size, uint32_t DummySize) {}
__weak void LCD_IO_ReadCmd8MultipleData24to16(uint8_t Cmd, uint16_t *pData, uint32_t Size, uint32_t DummySize) {}
__weak void LCD_IO_ReadCmd16MultipleData8(uint16_t Cmd, uint8_t *pData, uint32_t Size, uint32_t DummySize) {}
__weak void LCD_IO_ReadCmd16MultipleData16(uint16_t Cmd, uint16_t *pData, uint32_t Size, uint32_t DummySize) {}
__weak void LCD_IO_ReadCmd16MultipleData24to16(uint16_t Cmd, uint16_t *pData, uint32_t Size, uint32_t DummySize) {}
#else

//-----------------------------------------------------------------------------
void LCD_IO_ReadCmd8MultipleData8(uint8_t Cmd, uint8_t *pData, uint32_t Size, uint32_t DummySize)
{
  LCD_SPI_MODE8;
  LCD_CS_ON;
  LCD_CMD8_WRITE(Cmd);

  DummySize = (DummySize << 3) + LCD_SCK_EXTRACLK;
  LCD_DIRREAD(DummySize);

  #if DMANUM(LCD_DMA_RX) == 0 || LCD_SPI == 0
  while(Size--)
  {
    LCD_DATA8_READ(*pData);
    pData++;
  }
  LCD_CS_OFF;
  LCD_DIRWRITE(tmp8);

  #else
  LCD_SPI_DMA_READ(pData, Size, 0);
  LCD_CS_OFF;
  #endif
}

//-----------------------------------------------------------------------------
void LCD_IO_ReadCmd8MultipleData16(uint8_t Cmd, uint16_t *pData, uint32_t Size, uint32_t DummySize)
{
  LCD_SPI_MODE8;
  LCD_CS_ON;
  LCD_CMD8_WRITE(Cmd);
  LCD_SPI_MODE16;

  DummySize = (DummySize << 3) + LCD_SCK_EXTRACLK;
  LCD_DIRREAD(DummySize);

  #if DMANUM(LCD_DMA_RX) == 0 || LCD_SPI == 0 || LCD_REVERSE16 == 1
  while(Size--)
  {
    LCD_DATA16_READ(*pData);
    pData++;
  }
  LCD_CS_OFF;
  LCD_DIRWRITE(tmp8);

  #else
  LCD_SPI_DMA_READ(pData, Size, 1);
  LCD_CS_OFF;
  #endif
}

//-----------------------------------------------------------------------------
void LCD_IO_ReadCmd8MultipleData24to16(uint8_t Cmd, uint16_t *pData, uint32_t Size, uint32_t DummySize)
{
  uint8_t rgb888[3];
  LCD_SPI_MODE8;
  LCD_CS_ON;
  LCD_CMD8_WRITE(Cmd);

  DummySize = (DummySize << 3) + LCD_SCK_EXTRACLK;
  LCD_DIRREAD(DummySize);

  #if DMANUM(LCD_DMA_RX) == 0 || LCD_SPI == 0
  while(Size--)
  {
    LCD_DATA8_READ(rgb888[0]);
    LCD_DATA8_READ(rgb888[1]);
    LCD_DATA8_READ(rgb888[2]);
    *pData = RD((rgb888[0] & 0b11111000) << 8 | (rgb888[1] & 0b11111100) << 3 | rgb888[2] >> 3);
    pData++;
  }
  LCD_CS_OFF;
  LCD_DIRWRITE(tmp8);

  #else
  #if LCD_DMA_RX_BUFMODE == 0
  uint8_t da[LCD_DMA_RX_BUFSIZE];
  #endif

  LCD_SPI_DMA_READ24TO16(pData, Size, &da, LCD_DMA_RX_BUFSIZE);
  LCD_CS_OFF;
  #endif
}

//-----------------------------------------------------------------------------
void LCD_IO_ReadCmd16MultipleData8(uint16_t Cmd, uint8_t *pData, uint32_t Size, uint32_t DummySize)
{
  LCD_SPI_MODE16;
  LCD_CS_ON;
  LCD_CMD16_WRITE(Cmd);
  LCD_SPI_MODE8;

  DummySize = (DummySize << 3) + LCD_SCK_EXTRACLK;
  LCD_DIRREAD(DummySize);

  #if DMANUM(LCD_DMA_RX) == 0 || LCD_SPI == 0
  uint8_t  d;

  while(Size--)
  {
    LCD_DATA8_READ(d);
    *pData = d;
    pData++;
  }
  LCD_CS_OFF;
  LCD_DIRWRITE(tmp8);

  #else
  LCD_SPI_DMA_READ(pData, Size, 0);
  LCD_CS_OFF;
  #endif
}

//-----------------------------------------------------------------------------
void LCD_IO_ReadCmd16MultipleData16(uint16_t Cmd, uint16_t *pData, uint32_t Size, uint32_t DummySize)
{
  LCD_SPI_MODE16;
  LCD_CS_ON;
  LCD_CMD16_WRITE(Cmd);

  DummySize = (DummySize << 3) + LCD_SCK_EXTRACLK;
  LCD_DIRREAD(DummySize);

  #if DMANUM(LCD_DMA_RX) == 0 || LCD_SPI == 0 || LCD_REVERSE16 == 1

  LCD_SPI_MODE16;
  while(Size--)
  {
    LCD_DATA16_READ(*pData);
    pData++;
  }
  LCD_CS_OFF;
  LCD_DIRWRITE(tmp8);

  #else
  LCD_SPI_DMA_READ(pData, Size, 1);
  LCD_CS_OFF;
  #endif
}

//-----------------------------------------------------------------------------
void LCD_IO_ReadCmd16MultipleData24to16(uint16_t Cmd, uint16_t *pData, uint32_t Size, uint32_t DummySize)
{
  uint8_t  rgb888[3];
  LCD_SPI_MODE16;
  LCD_CS_ON;
  LCD_CMD16_WRITE(Cmd);
  LCD_SPI_MODE8;

  DummySize = (DummySize << 3) + LCD_SCK_EXTRACLK;
  LCD_DIRREAD(DummySize);

  #if DMANUM(LCD_DMA_RX) == 0 || LCD_SPI == 0
  while(Size--)
  {
    LCD_DATA8_READ(rgb888[0]);
    LCD_DATA8_READ(rgb888[1]);
    LCD_DATA8_READ(rgb888[2]);
    *pData = RD((rgb888[0] & 0b11111000) << 8 | (rgb888[1] & 0b11111100) << 3 | rgb888[2] >> 3);
    pData++;
  }
  LCD_CS_OFF;
  LCD_DIRWRITE(tmp8);

  #else
  #if LCD_DMA_RX_BUFMODE == 0
  uint8_t da[LCD_DMA_RX_BUFSIZE];
  #endif

  LCD_SPI_DMA_READ24TO16(pData, Size, &da, LCD_DMA_RX_BUFSIZE);
  LCD_CS_OFF;
  #endif
}

#endif // #else LCD_SPI_MODE == 0
