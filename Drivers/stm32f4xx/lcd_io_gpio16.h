/*
 * 16 bites párhuzamos LCD GPIO driver STM32F4-re
 * 5 vezárlöláb (CS, RS, WR, RD, RST) + 16 adatláb + háttérvilágitás vezérlés
 */

//=============================================================================
/* Lcd vezérlö lábak hozzárendelése (A..M, 0..15) */
#define LCD_CS            G, 12
#define LCD_RS            F, 12
#define LCD_WR            D, 5
#define LCD_RD            D, 4
#define LCD_RST           X, 0

/* Lcd adat lábak hozzárendelése (A..M, 0..15) */
#define LCD_D0            D, 14
#define LCD_D1            D, 15
#define LCD_D2            D, 0
#define LCD_D3            D, 1
#define LCD_D4            E, 7
#define LCD_D5            E, 8
#define LCD_D6            E, 9
#define LCD_D7            E, 10
#define LCD_D8            E, 11
#define LCD_D9            E, 12
#define LCD_D10           E, 13
#define LCD_D11           E, 14
#define LCD_D12           E, 15
#define LCD_D13           D, 8
#define LCD_D14           D, 9
#define LCD_D15           D, 10

/* Háttérvilágitás vezérlés
   - BL: A..M, 0..15 (ha nem használjuk, akkor rendeljük hozzá az X, 0 értéket)
   - BL_ON: 0 vagy 1, a bekapcsolt állapothoz tartozó logikai szint */
#define LCD_BL            B, 15
#define LCD_BLON          0

/* nsec nagyságrendü várakozás az LCD irási és az olvasási impulzusnál és a touchscreen AD átalakitonál
   - kezdö értéknek érdemes 10 illetve 500-bol elindulni, aztán lehet csökkenteni a sebesség növelése érdekében
     (az értékek függnek a processzor orajelétöl és az LCD kijelzö sebességétöl is)
*/
#define LCD_IO_RW_DELAY   1

/*=============================================================================
I/O csoport optimalizáció, hogy ne bitenként történjenek a GPIO mûveletek:
Megj: ha az adat lábakat egy porton belül emelkedö sorrendben definiáljuk
      automatikusan optimalizálva fognak történni a GPIO mûveletek akkor is, ha
      itt nem definiáljuk az optimalizált müködéshez szükséges eljárásokat
A lenti példa a következö lábakhoz optimalizál:
      LCD_D0<-D14,  LCD_D1<-D15, LCD_D2<-D0,   LCD_D3<-D1
      LCD_D4<-E7,   LCD_D5<-E8,  LCD_D6<-E9,   LCD_D7<-E10
      LCD_D8<-E11,  LCD_D9<-E12, LCD_D10<-E13, LCD_D11<-E14
      LCD_D12<-E15, LCD_D13<-D8, LCD_D14<-D9,  LCD_D15<-D10 */
#if 1
// 16 adatláb kimenetre állítása (adatirány: STM32 -> LCD)
#define LCD_DIRWRITE { \
GPIOD->MODER = (GPIOD->MODER & ~((3 << 2 * 14) | (3 << 2 * 15) | (3 << 2 * 0) | (3 << 2 * 1)  | (3 << 2 * 8)  | (3 << 2 * 9)  | (3 << 2 * 10))) | \
                                ((1 << 2 * 14) | (1 << 2 * 15) | (1 << 2 * 0) | (1 << 2 * 1)  | (1 << 2 * 8)  | (1 << 2 * 9)  | (1 << 2 * 10));   \
GPIOE->MODER = (GPIOE->MODER & ~((3 << 2 * 7)  | (3 << 2 * 8)  | (3 << 2 * 9) | (3 << 2 * 10) | (3 << 2 * 11) | (3 << 2 * 12) | (3 << 2 * 13) | (3 << 2 * 14) | (3 << 2 * 15))) | \
                                ((1 << 2 * 7)  | (1 << 2 * 8)  | (1 << 2 * 9) | (1 << 2 * 10) | (1 << 2 * 11) | (1 << 2 * 12) | (1 << 2 * 13) | (1 << 2 * 14) | (1 << 2 * 15));   }
// 16 adatláb bemenetre állítása (adatirány: STM32 <- LCD)
#define LCD_DIRREAD { \
GPIOD->MODER = (GPIOD->MODER & ~((3 << 2 * 14) | (3 << 2 * 15) | (3 << 2 * 0) | (3 << 2 * 1)  | (3 << 2 * 8)  | (3 << 2 * 9)  | (3 << 2 * 10))) | \
                                ((0 << 2 * 14) | (0 << 2 * 15) | (0 << 2 * 0) | (0 << 2 * 1)  | (0 << 2 * 8)  | (0 << 2 * 9)  | (0 << 2 * 10));   \
GPIOE->MODER = (GPIOE->MODER & ~((3 << 2 * 7)  | (3 << 2 * 8)  | (3 << 2 * 9) | (3 << 2 * 10) | (3 << 2 * 11) | (3 << 2 * 12) | (3 << 2 * 13) | (3 << 2 * 14) | (3 << 2 * 15))) | \
                                ((0 << 2 * 7)  | (0 << 2 * 8)  | (0 << 2 * 9) | (0 << 2 * 10) | (0 << 2 * 11) | (0 << 2 * 12) | (0 << 2 * 13) | (0 << 2 * 14) | (0 << 2 * 15));   }

// 16 adatláb írása, STM32 -> LCD (a kiirandó adat a makro dt paraméterében van)
#define LCD_WRITE(dt) { \
GPIOD->ODR = (GPIOD->ODR & ~(0b1100011100000011)) | \
                            (((dt & 0b11) << 14) | ((dt & 0b1100) >> 2)| ((dt & 0b1110000000000000) >> 5)); \
GPIOE->ODR = (GPIOE->ODR & ~(0b1111111110000000)) |         \
                            ((dt & 0b1111111110000) << 3);                    }

// 16 adatláb olvasása, STM32 <- LCD (az olvasott adat dt paraméterben megadott változoba kerül)
#define LCD_READ(dt) { \
dt = ((GPIOD->IDR & 0b1100000000000000) >> 14) | ((GPIOD->IDR & 0b0000000000000011) << 2) | ((GPIOD->IDR & 0b0000011100000000) << 5) | \
     ((GPIOE->IDR & 0b1111111110000000) >> 3); }
#endif
