/*
 * SPI LCD driver STM32F4
 * author: Roberto Benjami
 * v.2020.04
 */

//=============================================================================
/* SPI select (0, 1, 2, 3, 4, 5, 6)
   - 0: software SPI driver (the pins assign are full free)
   - 1..6: hardware SPI driver (the LCD_SCK, LCD_MOSI, LCD_MISO pins are lock to hardware) */
#define LCD_SPI           0

/* SPI mode
   - 0: only TX (only to write on the display, LCD_MISO is not used)
   - 1: half duplex (LCD_MOSI is bidirectional pin, LCD_MISO is not used)
   - 2: full duplex (SPI TX: LCD_MOSI, SPI RX: LCD_MISO) */
#define LCD_SPI_MODE      0

/* SPI write and read speed
   - software SPI: 0=none delay, 1=nop, 2=CLK pin double write, 3.. = LCD_IO_Delay(LCD_SPI_SPD - 3)
   - hardware SPI clock div fPCLK: 0=/2, 1=/4, 2=/8, 3=/16, 4=/32, 5=/64, 6=/128, 7=/256 */
#define LCD_SPI_SPD_WRITE 1
#define LCD_SPI_SPD_READ  4

/* SPI pins alternative function assign (0..15), (only hardware SPI) */
#define LCD_SPI_AFR       0

/* Lcd control pins assign (A..K, 0..15)
   - if hardware SPI: SCK, MOSI, MISO pins assign is lock to hardware */
#define LCD_RST           X, 0  /* If not used leave it that way */
#define LCD_RS            X, 0

#define LCD_CS            X, 0
#define LCD_SCK           X, 0
#define LCD_MOSI          X, 0
#define LCD_MISO          X, 0  /* If not used leave it that way */

/* Backlight control
   - BL: A..K, 0..15 (if not used -> X, 0)
   - BL_ON: the logical level of the active state */
#define LCD_BL            X, 0  /* If not used leave it that way */
#define LCD_BLON          0

/* When data direction change (OUT->IN) there is a display that requires extra clock
   example ST7735: 1, ILI9341: 0 */
#define LCD_SCK_EXTRACLK  0

/* DMA settings (only hardware SPI)
   - 0..2: 0 = no DMA, 1 = DMA1, 2 = DMA2
   - 0..7: DMA channel (DMA request mapping)
   - 0..7: Stream (DMA request mapping)
   - 1..3: DMA priority (0=low..3=very high) */
#define LCD_DMA_TX        0, 0, 0, 0
#define LCD_DMA_RX        0, 0, 0, 0

/* In dma mode the bitmap drawing function is completed before the actual drawing.
 * If the content of the image changes (because it is in a stack), the drawing will be corrupted.
 * If you want to wait for the drawing operation to complete, set it here.
 * This will slow down the program, but will not cause a bad drawing.
 * When drawing a non-bitmap (example: FillRect), you do not wait for the end of the drawing
 * because it stores the drawing color in a static variable.
 * The "LCD_IO_DmaTransferStatus" variable is content this status (if 0 -> DMA transfers are completed)
   - 0: bitmap and fill drawing function end wait off
   - 1: bitmap drawing function end wait on, fill drawing function end wait off (default mode)
   - 2: bitmap and fill drawing function end wait on */
#define LCD_DMA_TXWAIT    1

/* Because there are DMA capable and DMA unable memory regions
 * here we can set what is the DMA unable region condition
 * note: where the condition is true, it is considered a DMA-unable region
 * If you delete this definition: all memory are DMA capable */
#define LCD_DMA_UNABLE(addr)  addr >= 0x10000000 && addr < 0x20000000

/* DMA RX buffer [byte] (only in ...24to16 function) */
#define LCD_DMA_RX_BUFSIZE  256

/* DMA RX buffer place (only in ...24to16 function)
   - 0: stack
   - 1: static buffer
   - 2: memory manager (malloc/free) */
#define LCD_DMA_RX_BUFMODE  1

/* If LCD_DMA_RX_BUFMODE == 2 : memory management functions name */
#define LCD_DMA_RX_MALLOC   malloc
#define LCD_DMA_RX_FREE     free
/* Inlude for malloc/free functions */
#include <stdlib.h>
