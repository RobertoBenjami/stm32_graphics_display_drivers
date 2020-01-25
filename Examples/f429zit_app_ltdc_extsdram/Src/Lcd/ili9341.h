/* Interface mode
   - 1: SPI or paralell interface mode
   - 2: RGB mode (LTDC hardware, HSYNC, VSYNC, pixel clock, RGB bits data, framebuffer memory)
*/
#define  ILI9341_INTERFACE_MODE   2

/* Orientation
   - 0: 240x320 portrait (plug in top)
   - 1: 320x240 landscape (plug in left)
   - 2: 240x320 portrait (plug in botton)
   - 3: 320x240 landscape (plug in right)
*/
#define  ILI9341_ORIENTATION      3

/* Color mode
   - 0: RGB565 (R:bit15..11, G:bit10..5, B:bit4..0)
   - 1: BRG565 (B:bit15..11, G:bit10..5, R:bit4..0)
*/
#define  ILI9341_COLORMODE        0

#if  ILI9341_INTERFACE_MODE == 2

/* please see in the main.c what is the LTDC_HandleTypeDef name */
extern   LTDC_HandleTypeDef       hltdc;

/* Frambuffer memory alloc, free */
#define  ILI9341_MALLOC           eramMalloc
#define  ILI9341_FREE             eramFree

/* include for memory alloc/free */
#include "multi_heap_4.h"

#endif  /* #if ILI9341_INTERFACE_MODE == 2 */

//-----------------------------------------------------------------------------
// ILI9341 physic resolution (in 0 orientation)
#define  ILI9341_LCD_PIXEL_WIDTH  240
#define  ILI9341_LCD_PIXEL_HEIGHT 320
