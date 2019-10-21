/* Orientation:
   - 0: 320x480 micro-sd in the top (portrait)
   - 1: 480x320 micro-sd in the left (landscape)
   - 2: 320x480 micro-sd in the bottom (portrait)
   - 3: 480x320 micro-sd in the right (landscape)
*/
#define  ILI9488_ORIENTATION       0

/* Color mode
   - 0: RGB565 (R:bit15..11, G:bit10..5, B:bit4..0)
   - 1: BRG565 (B:bit15..11, G:bit10..5, R:bit4..0)
*/
#define  ILI9488_COLORMODE         0

/* Analog touchscreen
   - 0: touchscreen disabled
   - 1: touchscreen enabled
*/
#define  ILI9488_TOUCH             0

/* Touchscreen calibration data for 4 orientations */
#define  TS_CINDEX_0        {3385020, 333702, -667424, 1243070964, -458484, -13002, 1806391572}
#define  TS_CINDEX_1        {3385020, -458484, -13002, 1806391572, -333702, 667424, -163249584}
#define  TS_CINDEX_2        {3385020, -333702, 667424, -163249584, 458484, 13002, -184966992}
#define  TS_CINDEX_3        {3385020, 458484, 13002, -184966992, 333702, -667424, 1243070964}

/* For multi-threaded or intermittent use, Lcd and Touchscreen simultaneous use can cause confusion (since it uses common I/O resources)
   Lcd functions wait for the touchscreen header, the touchscreen query is not executed when Lcd is busy.
   Note: If the priority of the Lcd is higher than that of the Touchscreen, it may end up in an infinite loop!
   - 0: multi-threaded protection disabled
   - 1: multi-threaded protection enabled
*/
#define  ILI9488_MULTITASK_MUTEX   0

/* ILI9488 Size (physical resolution in default orientation) */
#define  ILI9488_LCD_PIXEL_WIDTH   320
#define  ILI9488_LCD_PIXEL_HEIGHT  480
