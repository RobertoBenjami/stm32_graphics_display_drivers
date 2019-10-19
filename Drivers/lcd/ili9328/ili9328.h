/* Orientation:
   - 0: 240x320 reset button in the top (portrait)
   - 1: 320x240 reset button in the left (landscape)
   - 2: 240x320 reset button in the bottom (portrait)
   - 3: 320x240 reset button in the right (landscape)
*/
#define  ILI9328_ORIENTATION       0

/* Color mode
   - 0: RGB565 (R:bit15..11, G:bit10..5, B:bit4..0)
   - 1: BRG565 (B:bit15..11, G:bit10..5, R:bit4..0)
*/
#define  ILI9328_COLORMODE         0

/* Analog touchscreen
   - 0: touchscreen disabled
   - 1: touchscreen enabled
*/
#define  ILI9328_TOUCH             0

/* Touchscreen calibration data for 4 orientations */
#define  TS_CINDEX_0        {-2776428, -209145, -6464, 123162313, 269742, -542860, 95435474}
#define  TS_CINDEX_1        {-2776428, 269742, -542860, 95435474, 209145, 6464, -786728605}
#define  TS_CINDEX_2        {-2776428, 209145, 6464, -786728605, -269742, 542860, -981116006}
#define  TS_CINDEX_3        {-2776428, -269742, 542860, -981116006, -209145, -6464, 123162313}

/* For multi-threaded or intermittent use, Lcd and Touchscreen simultaneous use can cause confusion (since it uses common I/O resources)
   Lcd functions wait for the touchscreen header, the touchscreen query is not executed when Lcd is busy.
   Note: If the priority of the Lcd is higher than that of the Touchscreen, it may end up in an infinite loop!
   - 0: multi-threaded protection disabled
   - 1: multi-threaded protection enabled
*/
#define  ILI9328_MULTITASK_MUTEX   0

/* Physical resolution in default orientation */
#define  ILI9328_LCD_PIXEL_WIDTH   240
#define  ILI9328_LCD_PIXEL_HEIGHT  320
