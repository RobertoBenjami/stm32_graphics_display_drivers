/*
 * XPT2046 touch driver STM32F1
 * author: Roberto Benjami
 * v.2021.09.21
 */

//=============================================================================

/* SPI select (0, 1, 2, 3)
   - 0: software SPI driver (the pins assign are full free)
   - 1..3: hardware SPI driver (the TS_SCK, TS_MOSI, TS_MISO pins are lock to hardware) */
#define TS_SPI            0

/* SPI write and read speed
   - software SPI: 0=none delay, 1=nop, 2=CLK pin double write, 3.. = TS_IO_Delay(TS_SPI_SPD - 3)
   - hardware SPI clock div fPCLK: 0=/2, 1=/4, 2=/8, 3=/16, 4=/32, 5=/64, 6=/128, 7=/256 */
#define TS_SPI_SPD        5

/* If hardware SPI pins don't are on default pins -> please setting and this macro
   (will be set at LCD_IO_Init, and set the RCC_APB2ENR_AFIOEN) */
// #define TS_SPI_ALTERSET  AFIO->MAPR |= 1 << AFIO_MAPR_SPI1_REMAP_Pos

/* Lcd control pins assign (A..K, 0..15)
   - if hardware SPI: SCK, MOSI, MISO pins assign is lock to hardware */
#define TS_CS             X, 0
#define TS_SCK            X, 0
#define TS_MOSI           X, 0
#define TS_MISO           X, 0
#define TS_IRQ            X, 0  /* If not used leave it that way */
