/*
 * 8 bit paralell LCD FSMC driver
 * 5 controll pins (CS, RS, WR, RD, RST) + 8 data pins + backlight pin
 * FSMC_NE1<-LCD_CS, FSMC_NOE<-LCD_RD, FSMC_NWE<-LCD_WR, FSMC_Ax<-LCD_RS
 * FSMC_D0<-LCD_D0, FSMC_D1<-LCD_D1, FSMC_D2<-LCD_D2, FSMC_D3<-LCD_D3
 * FSMC_D4<-LCD_D4, FSMC_D5<-LCD_D5, FSMC_D6<-LCD_D6, FSMC_D7<-LCD_D7 
 */

//=============================================================================
/* Lcd controll pins assign (A..K, 0..15) */
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
