/*
 * 8 bites párhuzamos LCD/TOUCH GPIO driver STM32F4-re
 * 5 vezárlöláb (CS, RS, WR, RD, RST) + 8 adatláb + háttérvilágitás vezérlés

 * Figyelem: mivel azonos lábakon van az Lcd ás a Touchscreen,
 * ezért ezek ki kell zárni az Lcd és a Touchscreen egyidejü használatát!
 * Tábbszálas/megszakitásos környezetben igy gondoskodni kell az összeakadások megelözéséröl!
 */

//=============================================================================
/* Lcd vezérlö lábak hozzárendelése (A..M, 0..15) */
#define LCD_CS            D, 7
#define LCD_RS            D, 13
#define LCD_WR            D, 5
#define LCD_RD            D, 4
#define LCD_RST           C, 5

/* Lcd adat lábak hozzárendelése (A..M, 0..15) */
#define LCD_D0            D, 14
#define LCD_D1            D, 15
#define LCD_D2            D, 0
#define LCD_D3            D, 1
#define LCD_D4            E, 7
#define LCD_D5            E, 8
#define LCD_D6            E, 9
#define LCD_D7            E, 10

/* Háttérvilágitás vezérlés
   - BL: A..M, 0..15 (ha nem használjuk, akkor rendeljük hozzá az X, 0 értéket)
   - BL_ON: 0 vagy 1, a bekapcsolt állapothoz tartozó logikai szint */
#define LCD_BL            B, 1
#define LCD_BLON          0

/*-----------------------------------------------------------------------------
Touch I/O lábak és A/D csatornák
A kijelzön belül a következö lábak vannak párhuzamositva
 - TS_XM <- LCD_RS
 - TS_XP <- LCD_D6
 - TS_YM <- LCD_D7
 - TS_YP <- LCD_WR */

/* ADC konverter száma (értéke lehet 1, 2, 3, vagy 0 ha nem használjuk)
   - 0: analog touchscreen driver nem lesz használva
   - 1..3: a használni kivánt A/D konverter száma
*/
#define TS_ADC            1

// Megadhatunk az analog touchscreen beolvasásához más lábakat is
// (ha nem adunk meg itt semmilyen lábat, akkor adjuk lábnak az X, 0 -t,
//  ekkor az LCD_RS és LCD_WR lesz a touchscreen kiválasztott AD lába)
#define TS_XM_AN          A, 0
#define TS_YP_AN          A, 1

// Itt kell megadni, hogy melyik csatornát kiválasztva lehet az adott lábat az AD bemenetére kapcsolni
#define TS_XM_ADCCH       0
#define TS_YP_ADCCH       1

/* nsec nagyságrendü várakozás az LCD irási és az olvasási impulzusnál és a touchscreen AD átalakitonál
   - kezdö értéknek érdemes 10 illetve 500-bol elindulni, aztán lehet csökkenteni a sebesség növelése érdekében
     (az értékek függnek a processzor orajelétöl és az LCD kijelzö sebességétöl is)
*/
#define LCD_IO_RW_DELAY   1
#define TS_AD_DELAY     500

/*=============================================================================
I/O csoport optimalizáció, hogy ne bitenként történjenek a GPIO mûveletek:
Megj: ha az adat lábakat egy porton belül emelkedö sorrendben definiáljuk
      automatikusan optimalizálva fognak történni a GPIO mûveletek akkor is, ha
      itt nem definiáljuk az optimalizált müködéshez szükséges eljárásokat
A lenti példa a következö lábakhoz optimalizál:
      LCD_D0<-D14, LCD_D1<-D15, LCD_D2<-D0, LCD_D3<-D1
      LCD_D4<-E7,  LCD_D5<-E8,  LCD_D6<-E9, LCD_D7<-E10 */
#if 1
// 8 adatláb kimenetre állítása (adatirány: STM32 -> LCD)
#define LCD_DIRWRITE { \
GPIOD->MODER = (GPIOD->MODER & ~((3 << 2 * 14) | (3 << 2 * 15) | (3 << 2 * 0) | (3 << 2 * 1)))  | \
                                ((1 << 2 * 14) | (1 << 2 * 15) | (1 << 2 * 0) | (1 << 2 * 1));    \
GPIOE->MODER = (GPIOE->MODER & ~((3 << 2 * 7)  | (3 << 2 * 8)  | (3 << 2 * 9) | (3 << 2 * 10))) | \
                                ((1 << 2 * 7)  | (1 << 2 * 8)  | (1 << 2 * 9) | (1 << 2 * 10));   }
// 8 adatláb bemenetre állítása (adatirány: STM32 <- LCD)
#define LCD_DIRREAD { \
GPIOD->MODER = (GPIOD->MODER & ~((3 << 2 * 14) | (3 << 2 * 15) | (3 << 2 * 0) | (3 << 2 * 1)))  | \
                                ((0 << 2 * 14) | (0 << 2 * 15) | (0 << 2 * 0) | (0 << 2 * 1));    \
GPIOE->MODER = (GPIOE->MODER & ~((3 << 2 * 7)  | (3 << 2 * 8)  | (3 << 2 * 9) | (3 << 2 * 10))) | \
                                ((0 << 2 * 7)  | (0 << 2 * 8)  | (0 << 2 * 9) | (0 << 2 * 10));   }

// 8 adatláb írása, STM32 -> LCD (a kiirandó adat a makro dt paraméterében van)
#define LCD_WRITE(dt) { \
GPIOD->ODR = (GPIOD->ODR & ~((1 << 14) | (1 << 15) | (1 << 0) | (1 << 1))) |        \
                            (((dt & 0b00000011) << 14) | ((dt & 0b00001100) >> 2)); \
GPIOE->ODR = (GPIOE->ODR & ~((1 << 7) | (1 << 8) | (1 << 9) | (1 << 10))) |         \
                            ((dt & 0b11110000) << (7 - 4));                         }

// 8 adatláb olvasása, STM32 <- LCD (az olvasott adat dt paraméterben megadott változoba kerül)
#define LCD_READ(dt) { \
dt = ((GPIOD->IDR & 0b1100000000000000) >> (14 - 0)) | ((GPIOD->IDR & 0b0000000000000011) << (2 - 0)) | \
     ((GPIOE->IDR & 0b0000011110000000) >> (7 - 4)); }
#endif
