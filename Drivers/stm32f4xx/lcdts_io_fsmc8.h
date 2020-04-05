/*
 * 8 bit paralell LCD/TOUCH FSMC driver
 * 5 controll pins (CS, RS, WR, RD, RST) + 8 data pins + backlight pin
 * FSMC_NE1<-LCD_CS, FSMC_NOE<-LCD_RD, FSMC_NWE<-LCD_WR, FSMC_Ax<-LCD_RS
 * FSMC_D0<-LCD_D0, FSMC_D1<-LCD_D1, FSMC_D2<-LCD_D2, FSMC_D3<-LCD_D3
 * FSMC_D4<-LCD_D4, FSMC_D5<-LCD_D5, FSMC_D6<-LCD_D6, FSMC_D7<-LCD_D7

 * note: since the LCD and touch are on the same pins, 
 * the simultaneous use of these should be excluded.
 * In multi-threaded interruption mode, collision prevention must be avoided! */

//=============================================================================
/* Lcd controll pins assign (A..K, 0..15, LCD_CS pin specified for FSMC) */
#define LCD_CS            X, 0
#define LCD_RST           X, 0  /* If not used leave it that way */

/* Backlight control
   - BL: A..K, 0..15 (if not used -> X, 0)
   - BL_ON: 0 = active low level, 1 = active high level */
#define LCD_BL            X, 0  /* If not used leave it that way */
#define LCD_BLON          0

//=============================================================================
/* Memory address
  - Bank1 (NE1) 0x60000000
  - Bank2 (NE2) 0x64000000
  - Bank3 (NE3) 0x68000000
  - Bank4 (NE4) 0x6C000000
  - REGSELECT_BIT: if example A18 pin -> 18 */
#define LCD_ADDR_BASE     0x60000000
#define LCD_REGSELECT_BIT 18

/* DMA settings
   - 0..2: 0 = no DMA, 1 = DMA1, 2 = DMA2 (DMA2-t is good!)
   - 0..7: DMA channel
   - 0..7: Stream
   - 1..3: DMA priority (0=low..3=very high) */
#define LCD_DMA           0, 0, 0, 0

/*=============================================================================
Touch I/O pins and A/D channels
In the lcd board are paralell pins
 - TS_XM <- LCD_RS (FSMC_Ax)
 - TS_XP <- LCD_D6 (FSMC_D6)
 - TS_YM <- LCD_D7 (FSMC_D7)
 - TS_YP <- LCD_WR (FSMC_NWE)
*/

/* ADC converter number (1, 2, 3, or 0 if not used)
   - 0: analog touchscreen driver not used
   - 1..3: A/D converter number */
#define TS_ADC            0

/* This pins specified the FSMC controller (f407: D13, D5, E9, E10) */
#define TS_XM             X, 0  /* If not used leave it that way */
#define TS_YP             X, 0  /* If not used leave it that way */
#define TS_XP             X, 0  /* If not used leave it that way */
#define TS_YM             X, 0  /* If not used leave it that way */

/* Because FSMC pins cannot be used as AD inputs, they must be connected in parallel to one of the pins that can be used in AD */
#define TS_XM_AN          X, 0  /* If not used leave it that way */
#define TS_YP_AN          X, 0  /* If not used leave it that way */

/* Select the AD channels */
#define TS_XM_ADCCH       0
#define TS_YP_ADCCH       0

/* Wait time for LCD touchscreen AD converter
   - First, give 500 values, then lower them to speed up the program.
     (values also depend on processor speed and LCD display speed) */
#define TS_AD_DELAY     500
