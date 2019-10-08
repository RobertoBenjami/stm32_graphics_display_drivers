/*
 * 8 bites párhuzamos LCD GPIO driver STM32F1-re
 * 5 vezárlöláb (CS, RS, WR, RD, RST) + 8 adatláb + háttérvilágitás vezérlés
 */

//=============================================================================
// Lcd vezérlö lábak hozzárendelése (A..M, 0..15) (RST láb opcionális)
#define LCD_CS            X, 0
#define LCD_RS            X, 0
#define LCD_WR            X, 0
#define LCD_RD            X, 0
#define LCD_RST           X, 0

// Lcd adat lábak hozzárendelése (A..M, 0..15)
#define LCD_D0            X, 0
#define LCD_D1            X, 0
#define LCD_D2            X, 0
#define LCD_D3            X, 0
#define LCD_D4            X, 0
#define LCD_D5            X, 0
#define LCD_D6            X, 0
#define LCD_D7            X, 0

// Háttérvilágitás vezérlés (opcionális, láb hozzárendelés és aktiv állapot)
#define LCD_BL            X, 0
#define LCD_BLON          0

/* nsec nagyságrendü várakozás az LCD irási és az olvasási impulzusnál
   - kezdö értéknek érdemes 10, 20-bol elindulni, aztán lehet csökkenteni a sebesség növelése érdekében
     (az érték függ a processzor orajelétöl és az LCD kijelzö sebességétöl is)
*/
#define LCD_WRITE_DELAY   10
#define LCD_READ_DELAY    20

/*=============================================================================
I/O csoport optimalizáció, hogy ne bitenként történjenek a GPIO mûveletek:
Megj: ha az adat lábakat egy porton belül emelkedö sorrendben definiáljuk
      automatikusan optimalizálva fognak történni a GPIO mûveletek akkor is, ha
      itt nem definiáljuk az optimalizált müködéshez szükséges eljárásokat
A lenti példa a következö lábakhoz optimalizál:
      LCD_D0<-D14, LCD_D1<-D15, LCD_D2<-D0, LCD_D3<-D1
      LCD_D4<-E7,  LCD_D5<-E8,  LCD_D6<-E9, LCD_D7<-E10 */
#if 0
// datapins setting to output (data direction: STM32 -> LCD)
#define LCD_DIRWRITE { /* D0..D1, D14..D15, E7..E10 <- 0x3 */ \
GPIOD->CRH = (GPIOD->CRH & ~0xFF000000) | 0x33000000; \
GPIOD->CRL = (GPIOD->CRH & ~0x000000FF) | 0x00000033; \
GPIOE->CRL = (GPIOE->CRL & ~0xF0000000) | 0x30000000; \
GPIOE->CRH = (GPIOE->CRH & ~0x00000FFF) | 0x00000333; }
// datapins setting to input (data direction: STM32 <- LCD)
#define LCD_DIRREAD { /* D0..D1, D14..D15, E7..E10 <- 0x4 */ \
GPIOD->CRH = (GPIOD->CRH & ~0xFF000000) | 0x44000000; \
GPIOD->CRL = (GPIOD->CRH & ~0x000000FF) | 0x00000044; \
GPIOE->CRL = (GPIOE->CRL & ~0xF0000000) | 0x40000000; \
GPIOE->CRH = (GPIOE->CRH & ~0x00000FFF) | 0x00000444; }
// datapins write, STM32 -> LCD (write I/O pins from dt data)
#define LCD_WRITE(dt) { /* D14..15 <- dt0..1, D0..1 <- dt2..3, E7..10 <- dt4..7 */ \
GPIOD->ODR = (GPIOD->ODR & ~0b1100000000000011) | (((dt & 0b00000011) << 14) | ((dt & 0b00001100) >> 2)); \
GPIOE->ODR = (GPIOE->ODR & ~0b0000011110000000) | ((dt & 0b11110000) << 3); }
// datapins read, STM32 <- LCD (read from I/O pins and store to dt data)
#define LCD_READ(dt) { /* dt0..1 <- D14..15, dt2..3 <- D0..1, dt4..7 <- E7..10 */ \
dt = ((GPIOD->IDR & 0b1100000000000000) >> 14) | ((GPIOD->IDR & 0b0000000000000011) << 2) | \
     ((GPIOE->IDR & 0b0000011110000000) >> 3); }
#endif
