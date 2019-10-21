/* Orientácio:
   - 0: 128x160 8pin catlakozo felül (portrait)
   - 1: 160x128 8pin catlakozo bal oldalt (landscape)
   - 2: 128x160 8pin catlakozo alul (portrait)
   - 3: 160x128 8pin catlakozo jobb oldalt (landscape)
*/
#define  ST7735_ORIENTATION       0

/* Color mode
   - 0: RGB565 (R:bit15..11, G:bit10..5, B:bit4..0)
   - 1: BRG565 (B:bit15..11, G:bit10..5, R:bit4..0)
*/
#define  ST7735_COLORMODE         0

//-----------------------------------------------------------------------------
// ST7735 Size (fizikai felbontás, az alapértelmezett (0) orientáciora vonatkoztatva)
#define  ST7735_LCD_PIXEL_WIDTH   128
#define  ST7735_LCD_PIXEL_HEIGHT  160
