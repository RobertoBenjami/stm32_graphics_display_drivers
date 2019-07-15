/* Orientácio:
   - 0: 240x320 catlakozo felül (portrait)
   - 1: 320x240 catlakozo bal oldalt (landscape)
   - 2: 240x320 catlakozo alul (portrait)
   - 3: 320x240 catlakozo jobb oldalt (landscape)
*/
#define  ILI9341_ORIENTATION      1

/* Color mode
   - 0: RGB565 (R:bit15..11, G:bit10..5, B:bit4..0)
   - 1: BRG565 (B:bit15..11, G:bit10..5, R:bit4..0)
*/
#define  ILI9341_COLORMODE        0

//-----------------------------------------------------------------------------
// ST7735 Size (fizikai felbontás, az alapértelmezett (0) orientáciora vonatkoztatva)
#define  ILI9341_LCD_PIXEL_WIDTH  240
#define  ILI9341_LCD_PIXEL_HEIGHT 320
