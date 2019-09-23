/*
 * SPI LCD driver STM32F3
 * készitö: Roberto Benjami
 * verzio:  2019.09
 *
 * - hardver és szoftver SPI
 * - 3 féle üzemmód (csak TX, half duplex, full duplex)
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

#define BITBAND_ACCESS(a, b)  *(volatile uint32_t*)(((uint32_t)&a & 0xF0000000) + 0x2000000 + (((uint32_t)&a & 0x000FFFFF) << 5) + (b << 2))

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

#define GPIOX_PORTSRC_(a, b)  GPIO_PortSourceGPIO ## a
#define GPIOX_PORTSRC(a)      GPIOX_PORTSRC_(a)

#define GPIOX_PINSRC_(a, b)   GPIO_PinSource ## b
#define GPIOX_PINSRC(a)       GPIOX_PINSRC_(a)

// GPIO Ports Clock Enable
#define GPIOX_CLOCK_(a, b)    RCC_AHBENR_GPIO ## a ## EN
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
#define GPIOX_PORTNUM_(a, b)  GPIOX_PORTTONUM_ ## a
#define GPIOX_PORTNAME_(a, b) a
#define GPIOX_PORTNUM(a)      GPIOX_PORTNUM_(a)
#define GPIOX_PORTNAME(a)     GPIOX_PORTNAME_(a)

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
#define LCD_RS_CMD            GPIOX_CLR(LCD_RS)
#define LCD_RS_DATA           GPIOX_SET(LCD_RS)

/* Reset láb aktiv/passziv */
#if (GPIOX_PORTNUM(LCD_RST) >= 1) && (GPIOX_PORTNUM(LCD_RST) <= 12)
#define LCD_RST_ON            GPIOX_CLR(LCD_RST)
#define LCD_RST_OFF           GPIOX_SET(LCD_RST)
#else
#define LCD_RST_ON
#define LCD_RST_OFF
#endif

/* Chip select láb */
#define LCD_CS_ON             GPIOX_CLR(LCD_CS)
#define LCD_CS_OFF            GPIOX_SET(LCD_CS)

/* Ha az olvasási sebesség ninc smegadva akkor megegyezik az irási sebességgel */
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
  GPIOX_MODER(MODE_DIGITAL_INPUT, LCD_MOSI);    \
  LCD_READ_DELAY;                               \
  while(d--)                                    \
  {                                             \
    GPIOX_CLR(LCD_SCK);                         \
    LCD_READ_DELAY;                             \
    GPIOX_SET(LCD_SCK);                         \
  }                                             }

#define LCD_DIRWRITE(d)        GPIOX_MODER(MODE_OUT, LCD_MOSI)

#else
/* TX és fullduplex SPI esetén az adatirány fix */
#define LCD_DIRREAD(d)
#define LCD_DIRWRITE(d)
#endif

/* Write SPI sebesség */
#if     LCD_SPI_SPD == 0
#define LCD_WRITE_DELAY
#elif   LCD_SPI_SPD == 1
#define LCD_WRITE_DELAY       GPIOX_CLR(LCD_SCK)
#else
#define LCD_WRITE_DELAY       LCD_IO_Delay(LCD_SPI_SPD - 2)
#endif

/* Read SPI sebesség */
#if     LCD_SPI_SPD_READ == 0
#define LCD_READ_DELAY
#elif   LCD_SPI_SPD_READ == 1
#define LCD_READ_DELAY        GPIOX_CLR(LCD_SCK)
#else
#define LCD_READ_DELAY        LCD_IO_Delay(LCD_SPI_SPD_READ - 2)
#endif

#define LCD_WRITE_CLK         GPIOX_CLR(LCD_SCK); LCD_WRITE_DELAY; GPIOX_SET(LCD_SCK);
#define LCD_READ_CLK          GPIOX_SET(LCD_SCK); GPIOX_CLR(LCD_SCK); LCD_READ_DELAY;

#define LCD_WRITE8(d8) {                                       \
  if(d8 & 0x80) GPIOX_SET(LCD_MOSI); else GPIOX_CLR(LCD_MOSI); \
  LCD_WRITE_CLK;                                               \
  if(d8 & 0x40) GPIOX_SET(LCD_MOSI); else GPIOX_CLR(LCD_MOSI); \
  LCD_WRITE_CLK;                                               \
  if(d8 & 0x20) GPIOX_SET(LCD_MOSI); else GPIOX_CLR(LCD_MOSI); \
  LCD_WRITE_CLK;                                               \
  if(d8 & 0x10) GPIOX_SET(LCD_MOSI); else GPIOX_CLR(LCD_MOSI); \
  LCD_WRITE_CLK;                                               \
  if(d8 & 0x08) GPIOX_SET(LCD_MOSI); else GPIOX_CLR(LCD_MOSI); \
  LCD_WRITE_CLK;                                               \
  if(d8 & 0x04) GPIOX_SET(LCD_MOSI); else GPIOX_CLR(LCD_MOSI); \
  LCD_WRITE_CLK;                                               \
  if(d8 & 0x02) GPIOX_SET(LCD_MOSI); else GPIOX_CLR(LCD_MOSI); \
  LCD_WRITE_CLK;                                               \
  if(d8 & 0x01) GPIOX_SET(LCD_MOSI); else GPIOX_CLR(LCD_MOSI); \
  LCD_WRITE_CLK; LCD_WRITE_DELAY;                              }

#define LCD_READ8(d8) {                          \
  GPIOX_CLR(LCD_SCK);                            \
  LCD_READ_DELAY;                                \
  if(GPIOX_IDR(LCD_MISO)) d8 = 0x80; else d8 = 0;\
  LCD_READ_CLK;                                  \
  if(GPIOX_IDR(LCD_MISO)) d8 |= 0x40;            \
  LCD_READ_CLK;                                  \
  if(GPIOX_IDR(LCD_MISO)) d8 |= 0x20;            \
  LCD_READ_CLK;                                  \
  if(GPIOX_IDR(LCD_MISO)) d8 |= 0x10;            \
  LCD_READ_CLK;                                  \
  if(GPIOX_IDR(LCD_MISO)) d8 |= 0x08;            \
  LCD_READ_CLK;                                  \
  if(GPIOX_IDR(LCD_MISO)) d8 |= 0x04;            \
  LCD_READ_CLK;                                  \
  if(GPIOX_IDR(LCD_MISO)) d8 |= 0x02;            \
  LCD_READ_CLK;                                  \
  if(GPIOX_IDR(LCD_MISO)) d8 |= 0x01;            \
  GPIOX_SET(LCD_SCK);                            }

#if LCD_REVERSE16 == 0
#define LCD_WRITE16(d16) {                                        \
  if(d16 & 0x8000) GPIOX_SET(LCD_MOSI); else GPIOX_CLR(LCD_MOSI); \
  LCD_WRITE_CLK;                                                  \
  if(d16 & 0x4000) GPIOX_SET(LCD_MOSI); else GPIOX_CLR(LCD_MOSI); \
  LCD_WRITE_CLK;                                                  \
  if(d16 & 0x2000) GPIOX_SET(LCD_MOSI); else GPIOX_CLR(LCD_MOSI); \
  LCD_WRITE_CLK;                                                  \
  if(d16 & 0x1000) GPIOX_SET(LCD_MOSI); else GPIOX_CLR(LCD_MOSI); \
  LCD_WRITE_CLK;                                                  \
  if(d16 & 0x0800) GPIOX_SET(LCD_MOSI); else GPIOX_CLR(LCD_MOSI); \
  LCD_WRITE_CLK;                                                  \
  if(d16 & 0x0400) GPIOX_SET(LCD_MOSI); else GPIOX_CLR(LCD_MOSI); \
  LCD_WRITE_CLK;                                                  \
  if(d16 & 0x0200) GPIOX_SET(LCD_MOSI); else GPIOX_CLR(LCD_MOSI); \
  LCD_WRITE_CLK;                                                  \
  if(d16 & 0x0100) GPIOX_SET(LCD_MOSI); else GPIOX_CLR(LCD_MOSI); \
  LCD_WRITE_CLK;                                                  \
  if(d16 & 0x0080) GPIOX_SET(LCD_MOSI); else GPIOX_CLR(LCD_MOSI); \
  LCD_WRITE_CLK;                                                  \
  if(d16 & 0x0040) GPIOX_SET(LCD_MOSI); else GPIOX_CLR(LCD_MOSI); \
  LCD_WRITE_CLK;                                                  \
  if(d16 & 0x0020) GPIOX_SET(LCD_MOSI); else GPIOX_CLR(LCD_MOSI); \
  LCD_WRITE_CLK;                                                  \
  if(d16 & 0x0010) GPIOX_SET(LCD_MOSI); else GPIOX_CLR(LCD_MOSI); \
  LCD_WRITE_CLK;                                                  \
  if(d16 & 0x0008) GPIOX_SET(LCD_MOSI); else GPIOX_CLR(LCD_MOSI); \
  LCD_WRITE_CLK;                                                  \
  if(d16 & 0x0004) GPIOX_SET(LCD_MOSI); else GPIOX_CLR(LCD_MOSI); \
  LCD_WRITE_CLK;                                                  \
  if(d16 & 0x0002) GPIOX_SET(LCD_MOSI); else GPIOX_CLR(LCD_MOSI); \
  LCD_WRITE_CLK;                                                  \
  if(d16 & 0x0001) GPIOX_SET(LCD_MOSI); else GPIOX_CLR(LCD_MOSI); \
  LCD_WRITE_CLK ;LCD_WRITE_DELAY;                                 }

#define LCD_READ16(d16) {                            \
  GPIOX_CLR(LCD_SCK);                                \
  LCD_READ_DELAY;                                    \
  if(GPIOX_IDR(LCD_MISO)) d16 = 0x8000; else d16 = 0;\
  LCD_READ_CLK;                                      \
  if(GPIOX_IDR(LCD_MISO)) d16 |= 0x4000;             \
  LCD_READ_CLK;                                      \
  if(GPIOX_IDR(LCD_MISO)) d16 |= 0x2000;             \
  LCD_READ_CLK;                                      \
  if(GPIOX_IDR(LCD_MISO)) d16 |= 0x1000;             \
  LCD_READ_CLK;                                      \
  if(GPIOX_IDR(LCD_MISO)) d16 |= 0x0800;             \
  LCD_READ_CLK;                                      \
  if(GPIOX_IDR(LCD_MISO)) d16 |= 0x0400;             \
  LCD_READ_CLK;                                      \
  if(GPIOX_IDR(LCD_MISO)) d16 |= 0x0200;             \
  LCD_READ_CLK;                                      \
  if(GPIOX_IDR(LCD_MISO)) d16 |= 0x0100;             \
  LCD_READ_CLK;                                      \
  if(GPIOX_IDR(LCD_MISO)) d16 |= 0x0080;             \
  LCD_READ_CLK;                                      \
  if(GPIOX_IDR(LCD_MISO)) d16 |= 0x0040;             \
  LCD_READ_CLK;                                      \
  if(GPIOX_IDR(LCD_MISO)) d16 |= 0x0020;             \
  LCD_READ_CLK;                                      \
  if(GPIOX_IDR(LCD_MISO)) d16 |= 0x0010;             \
  LCD_READ_CLK;                                      \
  if(GPIOX_IDR(LCD_MISO)) d16 |= 0x0008;             \
  LCD_READ_CLK;                                      \
  if(GPIOX_IDR(LCD_MISO)) d16 |= 0x0004;             \
  LCD_READ_CLK;                                      \
  if(GPIOX_IDR(LCD_MISO)) d16 |= 0x0002;             \
  LCD_READ_CLK;                                      \
  if(GPIOX_IDR(LCD_MISO)) d16 |= 0x0001;             \
  GPIOX_SET(LCD_SCK);                                }
#endif

#if LCD_REVERSE16 == 1
#define LCD_WRITE16(d16) {                                        \
  if(d16 & 0x0080) GPIOX_SET(LCD_MOSI); else GPIOX_CLR(LCD_MOSI); \
  LCD_WRITE_CLK;                                                  \
  if(d16 & 0x0040) GPIOX_SET(LCD_MOSI); else GPIOX_CLR(LCD_MOSI); \
  LCD_WRITE_CLK;                                                  \
  if(d16 & 0x0020) GPIOX_SET(LCD_MOSI); else GPIOX_CLR(LCD_MOSI); \
  LCD_WRITE_CLK;                                                  \
  if(d16 & 0x0010) GPIOX_SET(LCD_MOSI); else GPIOX_CLR(LCD_MOSI); \
  LCD_WRITE_CLK;                                                  \
  if(d16 & 0x0008) GPIOX_SET(LCD_MOSI); else GPIOX_CLR(LCD_MOSI); \
  LCD_WRITE_CLK;                                                  \
  if(d16 & 0x0004) GPIOX_SET(LCD_MOSI); else GPIOX_CLR(LCD_MOSI); \
  LCD_WRITE_CLK;                                                  \
  if(d16 & 0x0002) GPIOX_SET(LCD_MOSI); else GPIOX_CLR(LCD_MOSI); \
  LCD_WRITE_CLK;                                                  \
  if(d16 & 0x0001) GPIOX_SET(LCD_MOSI); else GPIOX_CLR(LCD_MOSI); \
  LCD_WRITE_CLK;                                                  \
  if(d16 & 0x8000) GPIOX_SET(LCD_MOSI); else GPIOX_CLR(LCD_MOSI); \
  LCD_WRITE_CLK;                                                  \
  if(d16 & 0x4000) GPIOX_SET(LCD_MOSI); else GPIOX_CLR(LCD_MOSI); \
  LCD_WRITE_CLK;                                                  \
  if(d16 & 0x2000) GPIOX_SET(LCD_MOSI); else GPIOX_CLR(LCD_MOSI); \
  LCD_WRITE_CLK;                                                  \
  if(d16 & 0x1000) GPIOX_SET(LCD_MOSI); else GPIOX_CLR(LCD_MOSI); \
  LCD_WRITE_CLK;                                                  \
  if(d16 & 0x0800) GPIOX_SET(LCD_MOSI); else GPIOX_CLR(LCD_MOSI); \
  LCD_WRITE_CLK;                                                  \
  if(d16 & 0x0400) GPIOX_SET(LCD_MOSI); else GPIOX_CLR(LCD_MOSI); \
  LCD_WRITE_CLK;                                                  \
  if(d16 & 0x0200) GPIOX_SET(LCD_MOSI); else GPIOX_CLR(LCD_MOSI); \
  LCD_WRITE_CLK;                                                  \
  if(d16 & 0x0100) GPIOX_SET(LCD_MOSI); else GPIOX_CLR(LCD_MOSI); \
  LCD_WRITE_CLK ;LCD_WRITE_DELAY;                                 }

#define LCD_READ16(d16) {                            \
  GPIOX_CLR(LCD_SCK);                                \
  LCD_READ_DELAY;                                    \
  if(GPIOX_IDR(LCD_MISO)) d16 = 0x0080; else d16 = 0;\
  LCD_READ_CLK;                                      \
  if(GPIOX_IDR(LCD_MISO)) d16 |= 0x0040;             \
  LCD_READ_CLK;                                      \
  if(GPIOX_IDR(LCD_MISO)) d16 |= 0x0020;             \
  LCD_READ_CLK;                                      \
  if(GPIOX_IDR(LCD_MISO)) d16 |= 0x0010;             \
  LCD_READ_CLK;                                      \
  if(GPIOX_IDR(LCD_MISO)) d16 |= 0x0008;             \
  LCD_READ_CLK;                                      \
  if(GPIOX_IDR(LCD_MISO)) d16 |= 0x0004;             \
  LCD_READ_CLK;                                      \
  if(GPIOX_IDR(LCD_MISO)) d16 |= 0x0002;             \
  LCD_READ_CLK;                                      \
  if(GPIOX_IDR(LCD_MISO)) d16 |= 0x0001;             \
  LCD_READ_CLK;                                      \
  if(GPIOX_IDR(LCD_MISO)) d16 |= 0x8000;             \
  LCD_READ_CLK;                                      \
  if(GPIOX_IDR(LCD_MISO)) d16 |= 0x4000;             \
  LCD_READ_CLK;                                      \
  if(GPIOX_IDR(LCD_MISO)) d16 |= 0x2000;             \
  LCD_READ_CLK;                                      \
  if(GPIOX_IDR(LCD_MISO)) d16 |= 0x1000;             \
  LCD_READ_CLK;                                      \
  if(GPIOX_IDR(LCD_MISO)) d16 |= 0x0800;             \
  LCD_READ_CLK;                                      \
  if(GPIOX_IDR(LCD_MISO)) d16 |= 0x0400;             \
  LCD_READ_CLK;                                      \
  if(GPIOX_IDR(LCD_MISO)) d16 |= 0x0200;             \
  LCD_READ_CLK;                                      \
  if(GPIOX_IDR(LCD_MISO)) d16 |= 0x0100;             \
  GPIOX_SET(LCD_SCK);                                }
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
#elif LCD_SPI == 4
#define SPIX                  SPI4
#define LCD_SPI_RCC_EN        BITBAND_ACCESS(RCC->APB2ENR, RCC_APB2ENR_SPI4EN_Pos) = 1
#elif LCD_SPI == 5
#define SPIX                  SPI5
#define LCD_SPI_RCC_EN        BITBAND_ACCESS(RCC->APB2ENR, RCC_APB2ENR_SPI5EN_Pos) = 1
#elif LCD_SPI == 6
#define SPIX                  SPI6
#define LCD_SPI_RCC_EN        BITBAND_ACCESS(RCC->APB2ENR, RCC_APB2ENR_SPI6EN_Pos) = 1
#endif

/* Read SPI sebesség */
#define LCD_READ_DELAY        LCD_IO_Delay(LCD_SPI_SPD_READ * 4)

#define LCD_DUMMY_READ(d)     {GPIOX_MODER(MODE_OUT, LCD_SCK);          \
                               while(d--)                               \
                               {                                        \
                                 GPIOX_CLR(LCD_SCK);                    \
                                 LCD_READ_DELAY;                        \
                                 GPIOX_SET(LCD_SCK);                    \
                               }                                        \
                               GPIOX_MODER(MODE_ALTER, LCD_SCK);        }

#define LCD_SPI_MODE8         SPIX->CR2 &= ~(SPI_CR2_DS_3)
#define LCD_SPI_MODE16        SPIX->CR2 |= (SPI_CR2_DS_3)

#if     LCD_SPI_MODE == 1
/* Kétirányu (halfduplex) SPI esetén az adatirányt váltogatni kell */
#if (defined(LCD_SPI_SPD_READ) && (LCD_SPI != 0) && (LCD_SPI_SPD != LCD_SPI_SPD_READ)) // Eltérö olvasási/irási sebesség
#define LCD_DIRREAD(d)  {                            \
  LCD_DUMMY_READ(d);                                 \
  SPIX->CR1 =                                        \
    (SPIX->CR1 & ~(SPI_CR1_BR | SPI_CR1_BIDIOE)) |   \
    (LCD_SPI_SPD_READ << SPI_CR1_BR_Pos);            }
#define LCD_DIRWRITE(d8) {                           \
  while(BITBAND_ACCESS(SPIX->SR, SPI_SR_RXNE_Pos))   \
    d8 = *(uint8_t *)&SPIX->DR;                      \
  SPIX->CR1 &= ~SPI_CR1_SPE;                         \
  SPIX->CR1 = (SPIX->CR1 & ~SPI_CR1_BR) |            \
    ((LCD_SPI_SPD << SPI_CR1_BR_Pos) |               \
    SPI_CR1_BIDIOE);                                 \
  LCD_IO_Delay(2 ^ LCD_SPI_SPD_READ);                \
  while(BITBAND_ACCESS(SPIX->SR, SPI_SR_RXNE_Pos))   \
    d8 = *(uint8_t *)&SPIX->DR;                      \
  SPIX->CR1 |= SPI_CR1_SPE;                          }

#else   // azonos sebesség, csak LCD_MOSI irányváltás
#define LCD_DIRREAD(d) {                             \
  LCD_DUMMY_READ(d);                                 \
  BITBAND_ACCESS(SPIX->CR1, SPI_CR1_BIDIOE_Pos) = 0; }
#define LCD_DIRWRITE(d8) {                           \
  while(BITBAND_ACCESS(SPIX->SR, SPI_SR_RXNE_Pos))   \
    d8 = *(uint8_t *)&SPIX->DR;                      \
  SPIX->CR1 &= ~SPI_CR1_SPE;                         \
  SPIX->CR1 |= SPI_CR1_BIDIOE;                       \
  LCD_IO_Delay(2 ^ LCD_SPI_SPD_READ);                \
  while(BITBAND_ACCESS(SPIX->SR, SPI_SR_RXNE_Pos))   \
    d8 = *(uint8_t *)&SPIX->DR;                      \
  SPIX->CR1 |= SPI_CR1_SPE;                          }
#endif

#else   // LCD_SPI_MODE == 1
// TX és fullduplex SPI esetén az adatirány fix
#if (defined(LCD_SPI_SPD_READ) && (LCD_SPI != 0) && (LCD_SPI_SPD != LCD_SPI_SPD_READ)) // Eltérö olvasási/irási sebesség
#define LCD_DIRREAD(d) { \
  LCD_DUMMY_READ(d);     \
  SPIX->CR1 = (SPIX->CR1 & ~SPI_CR1_BR) | (LCD_SPI_SPD_READ << SPI_CR1_BR_Pos);}
#define LCD_DIRWRITE(d8)  { \
  SPIX->CR1 = (SPIX->CR1 & ~SPI_CR1_BR) | (LCD_SPI_SPD << SPI_CR1_BR_Pos);}
#else   // nincs irányváltás
#define LCD_DIRREAD(d)   LCD_DUMMY_READ(d)
#define LCD_DIRWRITE
#endif
#endif  // #else LCD_SPI_MODE == 1

#define LCD_DATA8_WRITE(d8)   {                   \
  *(volatile uint8_t *)&SPIX->DR = d8;            \
  LCD_IO_Delay(2);                                \
  while(BITBAND_ACCESS(SPIX->SR, SPI_SR_BSY_Pos));}

#define LCD_DATA8_READ(d8)    {                      \
  while(!BITBAND_ACCESS(SPIX->SR, SPI_SR_RXNE_Pos)); \
  d8 = *(uint8_t *)&SPIX->DR;                        }

#define LCD_CMD8_WRITE(cmd8)   {                  \
  LCD_RS_CMD;                                     \
  LCD_DATA8_WRITE(cmd8);                          \
  LCD_RS_DATA;                                    }

#define LCD_DATA16_WRITE(d16)   {                 \
  SPIX->DR = RD(d16);                             \
  LCD_IO_Delay(1);                                \
  while(BITBAND_ACCESS(SPIX->SR, SPI_SR_BSY_Pos));}
#define LCD_DATA16_READ(d16)    {                    \
  while(!BITBAND_ACCESS(SPIX->SR, SPI_SR_RXNE_Pos)); \
  d16 = RD(SPIX->DR);                                }

#define LCD_CMD16_WRITE(cmd16)  {                 \
  LCD_RS_CMD;                                     \
  LCD_DATA16_WRITE(cmd16);                        \
  LCD_RS_DATA;                                    }

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
    if(b > DMA_MAXSIZE)                                                              \
    {                                                                           \
      DMAX_CHANNEL(LCD_DMA_TX)->CNDTR = DMA_MAXSIZE;                                 \
      b -= DMA_MAXSIZE;                                                              \
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
        nd = LCD_DMA_RX_BUFSIZE;                                                \
      rgb888[rgbcnt++] = da[rp++];                                              \
      rp &= (LCD_DMA_RX_BUFSIZE - 1);                                           \
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

#if DMANUM(LCD_DMA_RX) > 0 && LCD_DMA_RX_BUFMODE == 1 && LCD_SPI_MODE > 0
uint8_t da[LCD_DMA_RX_BUFSIZE] __attribute__((aligned));
#endif

#endif   // #else LCD_SPI == 0

volatile uint8_t   tmp8;

//-----------------------------------------------------------------------------
#ifdef LCD_DMA_IRQ
osSemaphoreId BinarySemDmaHandle;
#if DMANUM(LCD_DMA_TX) > 0
void DMAX_STREAMX_IRQHANDLER(LCD_DMA_TX)(void)
{
  if(DMAX_ISR(LCD_DMA_TX) & DMAX_ISR_TCIF(LCD_DMA_TX))
  {
    DMAX_IFCR(LCD_DMA_TX) = DMAX_IFCR_CTCIF(LCD_DMA_TX);
    osSemaphoreRelease(BinarySemDmaHandle);
  }
}
#endif

#if DMANUM(LCD_DMA_RX) > 0
void DMAX_STREAMX_IRQHANDLER(LCD_DMA_RX)(void)
{
  if(DMAX_ISR(LCD_DMA_RX) & DMAX_ISR_TCIF(LCD_DMA_RX))
  {
    DMAX_IFCR(LCD_DMA_RX) = DMAX_IFCR_CTCIF(LCD_DMA_RX);
    osSemaphoreRelease(BinarySemDmaHandle);
  }
  if(DMAX_ISR(LCD_DMA_RX) & DMAX_ISR_TEIF(LCD_DMA_RX))
  {
    while(1);
  }
  if(DMAX_ISR(LCD_DMA_RX) & DMAX_ISR_DMEIF(LCD_DMA_RX))
  {
    while(1);
  }
}
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
  if(Bl)
    #if LCD_BLON == 1
    GPIOX_SET(LCD_BL);
    #else
    GPIOX_CLR(LCD_BL);
    #endif
  else
    #if LCD_BLON == 1
    GPIOX_CLR(LCD_BL);
    #else
    GPIOX_SET(LCD_BL);
    #endif
  #endif
}

//-----------------------------------------------------------------------------
void LCD_IO_Init(void)
{
  #if LCD_SPI_MODE == 2                 // Full duplex
  RCC->AHBENR |= GPIOX_CLOCK(LCD_RS) | GPIOX_CLOCK(LCD_CS) | GPIOX_CLOCK(LCD_SCK) | GPIOX_CLOCK(LCD_MOSI) | GPIOX_CLOCK(LCD_MISO);
  GPIOX_MODER(MODE_DIGITAL_INPUT, LCD_MISO);
  #else                                 // TX vagy half duplex
  RCC->AHBENR |= GPIOX_CLOCK(LCD_RS) | GPIOX_CLOCK(LCD_CS) | GPIOX_CLOCK(LCD_SCK) | GPIOX_CLOCK(LCD_MOSI);
  #endif

  #if (GPIOX_PORTNUM(LCD_BL) >= 1) && (GPIOX_PORTNUM(LCD_BL) <= 12) // háttérvilágitás
  RCC->AHBENR |= GPIOX_CLOCK(LCD_BL);
  GPIOX_MODER(MODE_OUT, LCD_BL);
  LCD_IO_Bl_OnOff(1);
  #endif

  #if (GPIOX_PORTNUM(LCD_RST) >= 1) && (GPIOX_PORTNUM(LCD_RST) <= 12) // reset
  RCC->AHBENR |= GPIOX_CLOCK(LCD_RST);
  GPIOX_MODER(MODE_OUT, LCD_RST);
  GPIOX_OSPEEDR(MODE_SPD_LOW, LCD_RST);
  LCD_RST_OFF;
  #endif

  LCD_RS_DATA;
  LCD_CS_OFF;
  GPIOX_MODER(MODE_OUT, LCD_RS);
  GPIOX_MODER(MODE_OUT, LCD_CS);

  GPIOX_OSPEEDR(MODE_SPD_VHIGH, LCD_RS);
  GPIOX_OSPEEDR(MODE_SPD_VHIGH, LCD_CS);
  GPIOX_OSPEEDR(MODE_SPD_VHIGH, LCD_SCK);
  GPIOX_OSPEEDR(MODE_SPD_VHIGH, LCD_MOSI);
  GPIOX_SET(LCD_SCK);                   // SCK = 1

  #if LCD_SPI == 0                      // Szoftver SPI
  GPIOX_MODER(MODE_OUT, LCD_SCK);
  GPIOX_MODER(MODE_OUT, LCD_MOSI);

  #else                                 // Hardver SPI
  LCD_SPI_RCC_EN;

  GPIOX_AFR(LCD_SPI_AFR, LCD_SCK);
  GPIOX_MODER(MODE_ALTER, LCD_SCK);
  GPIOX_AFR(LCD_SPI_AFR, LCD_MOSI);
  GPIOX_MODER(MODE_ALTER, LCD_MOSI);

  #if LCD_SPI_MODE == 1
  // Half duplex (adatirány váltogatás)
  SPIX->CR1 = SPI_CR1_CPHA | SPI_CR1_CPOL | SPI_CR1_MSTR/* | SPI_CR1_SPE*/ | SPI_CR1_SSM | SPI_CR1_SSI | (LCD_SPI_SPD << SPI_CR1_BR_Pos) | SPI_CR1_BIDIMODE | SPI_CR1_BIDIOE;
  #else // #if LCD_SPI_MODE == 1
  // TX vagy full duplex mod
  SPIX->CR1 = SPI_CR1_CPHA | SPI_CR1_CPOL | SPI_CR1_MSTR/* | SPI_CR1_SPE*/ | SPI_CR1_SSM | SPI_CR1_SSI | (LCD_SPI_SPD << SPI_CR1_BR_Pos);
  #endif // #else LCD_SPI_MODE == 1
  SPIX->CR2 = (7 << SPI_CR2_DS_Pos);
  SPIX->CR1 |= SPI_CR1_SPE;

  #if DMANUM(LCD_DMA_TX) > 0 && DMANUM(LCD_DMA_RX) > 0 && DMANUM(LCD_DMA_TX) != DMANUM(LCD_DMA_RX)
  RCC->AHBENR |= RCC_AHBENR_DMA1EN; | RCC_AHBENR_DMA2EN;   // DMA1, DMA2 orajel be
  #elif DMANUM(LCD_DMA_TX) == 1 || DMANUM(LCD_DMA_RX) == 1
  RCC->AHBENR |= RCC_AHBENR_DMA1EN;   // DMA1 orajel be
  #elif DMANUM(LCD_DMA_TX) == 2 || DMANUM(LCD_DMA_RX) == 2
  RCC->AHBENR |= RCC_AHBENR_DMA2EN;   // DMA2 orajel be
  #endif

  #endif // #else LCD_SPI == 0

  /* Set or Reset the control line */
  #if (GPIOX_PORTNUM(LCD_RST) >= 1) && (GPIOX_PORTNUM(LCD_RST) <= 12) // reset
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
  HAL_NVIC_SetPriority(DMAX_STREAMX_IRQ(LCD_DMA_TX), configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY, 0);
  HAL_NVIC_EnableIRQ(DMAX_STREAMX_IRQ(LCD_DMA_TX));
  #endif
  #if DMANUM(LCD_DMA_RX) > 0
  HAL_NVIC_SetPriority(DMAX_STREAMX_IRQ(LCD_DMA_RX), configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY, 0);
  HAL_NVIC_EnableIRQ(DMAX_STREAMX_IRQ(LCD_DMA_RX));
  #endif
  osSemaphoreWait(BinarySemDmaHandle, 1);
  #endif
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
    #if LCD_REVERSE16 == 0
    *pData = ((rgb888[0] & 0b11111000) << 8 | (rgb888[1] & 0b11111100) << 3 | rgb888[2] >> 3);
    #endif
    #if LCD_REVERSE16 == 1
    *pData = __REVSH((rgb888[0] & 0b11111000) << 8 | (rgb888[1] & 0b11111100) << 3 | rgb888[2] >> 3);
    #endif
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
