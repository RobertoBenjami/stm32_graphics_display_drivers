/*
 * 8 bit paralell LCD/TOUCH GPIO driver for STM32F2
 * 5 controll pins (CS, RS, WR, RD, RST) + 8 data pins + backlight pin

 * note: since the LCD and touch are on the same pins, 
 * the simultaneous use of these should be excluded.
 * In multi-threaded interruption mode, collision prevention must be avoided!
 */

//=============================================================================
/* Lcd controll pins assign (A..K, 0..15) */
#define LCD_CS            X, 0
#define LCD_RS            X, 0
#define LCD_WR            X, 0
#define LCD_RD            X, 0
#define LCD_RST           X, 0  /* If not used leave it that way */

/* Lcd data pins assign (A..K, 0..15) */
#define LCD_D0            X, 0
#define LCD_D1            X, 0
#define LCD_D2            X, 0
#define LCD_D3            X, 0
#define LCD_D4            X, 0
#define LCD_D5            X, 0
#define LCD_D6            X, 0
#define LCD_D7            X, 0

/* Backlight control
   - BL: A..K, 0..15 (if not used -> X, 0)
   - BL_ON: 0 = active low level, 1 = active high level */
#define LCD_BL            X, 0  /* If not used leave it that way */
#define LCD_BLON          0

/*-----------------------------------------------------------------------------
Touch I/O pins and A/D channels
In the lcd board are paralell pins
 - TS_XM <- LCD_RS
 - TS_XP <- LCD_D6
 - TS_YM <- LCD_D7
 - TS_YP <- LCD_WR */

/* ADC converter number (1, 2, 3, or 0 if not used)
   - 0: analog touchscreen driver not used
   - 1..3: A/D converter number */
#define TS_ADC            0

/* If you use different pins for touch AD conversion than for lcd control, specify it
   (When using the same pins or not using a touchscreen -> X, 0
    in this case, LCD_RS and LCD_WR will be the selected AD pin on the touchscreen) */
#define TS_XM_AN          X, 0  /* If not used leave it that way */
#define TS_YP_AN          X, 0  /* If not used leave it that way */

/* Select the AD channels */
#define TS_XM_ADCCH       0
#define TS_YP_ADCCH       0

/* Wait time for LCD write and read pulse and touchscreen AD converter
   - First, give 10, 20, 500 values, then lower them to speed up the program.
     (values also depend on processor speed and LCD display speed) */
#define LCD_WRITE_DELAY  10
#define LCD_READ_DELAY   20
#define TS_AD_DELAY     500

/*=============================================================================
I/O group optimization so that GPIO operations are not performed bit by bit:
Note: If the pins are in order, they will automatically optimize.
The example belongs to the following pins:
      LCD_D0<-D14, LCD_D1<-D15, LCD_D2<-D0, LCD_D3<-D1
      LCD_D4<-E7,  LCD_D5<-E8,  LCD_D6<-E9, LCD_D7<-E10 */
#if 0
/* datapins setting to output (data direction: STM32 -> LCD) */
#define LCD_DIRWRITE { /* D0..D1, D14..D15, E7..E10 <- 0b01 */ \
GPIOD->MODER = (GPIOD->MODER & ~0b11110000000000000000000000001111) | 0b01010000000000000000000000000101; \
GPIOE->MODER = (GPIOE->MODER & ~0b00000000001111111100000000000000) | 0b00000000000101010100000000000000; }
/* datapins setting to input (data direction: STM32 <- LCD) */
#define LCD_DIRREAD { /* D0..D1, D14..D15, E7..E10 <- 0b00 */ \
GPIOD->MODER = (GPIOD->MODER & ~0b11110000000000000000000000001111); \
GPIOE->MODER = (GPIOE->MODER & ~0b00000000001111111100000000000000); }
/* datapins write, STM32 -> LCD (write I/O pins from dt data) */
#define LCD_WRITE(dt) { /* D14..15 <- dt0..1, D0..1 <- dt2..3, E7..10 <- dt4..7 */ \
GPIOD->ODR = (GPIOD->ODR & ~0b1100000000000011) | (((dt & 0b00000011) << 14) | ((dt & 0b00001100) >> 2)); \
GPIOE->ODR = (GPIOE->ODR & ~0b0000011110000000) | ((dt & 0b11110000) << 3); }
/* datapins read, STM32 <- LCD (read from I/O pins and store to dt data) */
#define LCD_READ(dt) { /* dt0..1 <- D14..15, dt2..3 <- D0..1, dt4..7 <- E7..10 */ \
dt = ((GPIOD->IDR & 0b1100000000000000) >> 14) | ((GPIOD->IDR & 0b0000000000000011) << 2) | \
     ((GPIOE->IDR & 0b0000011110000000) >> 3); }
/* Note: the keil compiler cannot use binary numbers, convert it to hexadecimal */	 
#endif
