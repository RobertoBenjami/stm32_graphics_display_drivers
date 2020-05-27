/*
 * 16 bit paralell LCD GPIO driver for STM32F1
 * 5 controll pins (CS, RS, WR, RD, RST) + 16 data pins + 1 backlight pin
 */

//=============================================================================
/* Lcd controll pins (A..K, 0..15) (if LCD_RST not used -> X, 0) */
#define LCD_CS            X, 0
#define LCD_RS            X, 0
#define LCD_WR            X, 0
#define LCD_RD            X, 0  /* If not used leave it that way */
#define LCD_RST           X, 0  /* If not used leave it that way */

/* Lcd data pins (A..K, 0..15) */
#define LCD_D0            X, 0
#define LCD_D1            X, 0
#define LCD_D2            X, 0
#define LCD_D3            X, 0
#define LCD_D4            X, 0
#define LCD_D5            X, 0
#define LCD_D6            X, 0
#define LCD_D7            X, 0
#define LCD_D8            X, 0
#define LCD_D9            X, 0
#define LCD_D10           X, 0
#define LCD_D11           X, 0
#define LCD_D12           X, 0
#define LCD_D13           X, 0
#define LCD_D14           X, 0
#define LCD_D15           X, 0

/* Backlight pin
   - BL: A..K, 0..15 (if not used -> X, 0)
   - BL_ON: 0 -> backlight on = low level, 1 -> backlight on = high level */
#define LCD_BL            X, 0  /* If not used leave it that way */
#define LCD_BLON          0

/* A few nsec delay at LCD write and read operations
   first, set it to 10 and 20, then reduce it until the display works well.
   (the values also depend on the processor clock speed and the LCD display speed) */
#define LCD_WRITE_DELAY   10
#define LCD_READ_DELAY    20

/*=============================================================================
I/O group optimalization, so that GPIO operations are not performed bit by bit:
Remark: when defining data legs within a port in ascending order
        GPIO operations will be automatically optimized even if
        we do not define procedures for optimized operation here
The example optimizes for the following pins:
      LCD_D0<-D14,  LCD_D1<-D15,  LCD_D2<-D0,   LCD_D3<-D1
      LCD_D4<-E7,   LCD_D5<-E8,   LCD_D6<-E9,   LCD_D7<-E10
      LCD_D8<-E11,  LCD_D9<-E12,  LCD_D10<-E13, LCD_D11<-E14
      LCD_D12<-E15, LCD_D13<-D8,  LCD_D14<-D9,  LCD_D15<-D10 */
#if 0
/* datapins setting to output (data direction: STM32 -> LCD) */
#define LCD_DIRWRITE { /* D0..D1, D8..D10, D14..D15, E7..E15 <- 0x3 */ \
  GPIOD->CRH = (GPIOD->CRH & ~0xFF000FFF) | 0x33000333; \
  GPIOD->CRL = (GPIOD->CRL & ~0x000000FF) | 0x00000033; \
  GPIOE->CRL = (GPIOE->CRL & ~0xF0000000) | 0x30000000; \
  GPIOE->CRH = 0x33333333;                              }
/* datapins setting to input (data direction: STM32 <- LCD) */
#define LCD_DIRREAD { /* D0..D1, D8..D10, D14..D15, E7..E15 <- 0x4 */ \
  GPIOD->CRH = (GPIOD->CRH & ~0xFF000FFF) | 0x44000444; \
  GPIOD->CRL = (GPIOD->CRL & ~0x000000FF) | 0x00000044; \
  GPIOE->CRL = (GPIOE->CRL & ~0xF0000000) | 0x40000000; \
  GPIOE->CRH = 0x44444444;                              }
/* datapins write, STM32 -> LCD (write I/O pins from dt data) */
#define LCD_WRITE(dt) { /* D14..15 <- dt0..1, D0..1 <- dt2..3, D8..10 <- dt13..15, E7..15 <- dt4..12 */ \
  GPIOD->ODR = (GPIOD->ODR & ~0b1100011100000011) | (((dt & 0b11) << 14) | ((dt & 0b1100) >> 2) | ((dt & 0b1110000000000000) >> 5)); \
  GPIOE->ODR = (GPIOE->ODR & ~0b1111111110000000) | ((dt & 0b0001111111110000) << 3); }
/* datapins read, STM32 <- LCD (read from I/O pins and store to dt data) */
#define LCD_READ(dt) { /* dt0..1 <- D14..15, dt2..3 <- D0..1, dt13..15 <- D8..10, dt4..12 <- E7..15 */ \
  dt = ((GPIOD->IDR & 0b1100000000000000) >> 14) | ((GPIOD->IDR & 0b0000000000000011) << 2) | \
       ((GPIOD->IDR & 0b0000011100000000) << 5)  | ((GPIOE->IDR & 0b1111111110000000) >> 3); }
#endif
