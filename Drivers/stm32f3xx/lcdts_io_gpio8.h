/*
 * 8 bites párhuzamos LCD/TOUCH GPIO driver STM32F3-ra
 * 5 vezárlöláb (CS, RS, WR, RD, RST) + 8 adatláb + háttérvilágitás vezérlés

 * Figyelem: mivel azonos lábakon van az Lcd ás a Touchscreen,
 * ezért ezek ki kell zárni az Lcd és a Touchscreen egyidejü használatát!
 * Tábbszálas/megszakitásos környezetben igy gondoskodni kell az összeakadások megelözéséröl!
 */

//=============================================================================
// Lcd vezérlö lábak hozzárendelése (A..K, 0..15)
#define LCD_CS            X, 0
#define LCD_RS            X, 0
#define LCD_WR            X, 0
#define LCD_RD            X, 0
#define LCD_RST           X, 0

// Lcd adat lábak hozzárendelése (A..K, 0..15)
#define LCD_D0            X, 0
#define LCD_D1            X, 0
#define LCD_D2            X, 0
#define LCD_D3            X, 0
#define LCD_D4            X, 0
#define LCD_D5            X, 0
#define LCD_D6            X, 0
#define LCD_D7            X, 0

/* Háttérvilágitás vezérlés
   - BL: A..K, 0..15 (ha nem használjuk, akkor rendeljük hozzá az X, 0 értéket)
   - BL_ON: 0 vagy 1, a bekapcsolt állapothoz tartozó logikai szint */
#define LCD_BL            X, 0
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
   - 1..4: a használni kivánt A/D konverter száma
*/
#define TS_ADC            0 // ADC1

// Megadhatunk az analog touchscreen beolvasásához más lábakat is
// (ha nem adunk meg itt semmilyen lábat, akkor adjuk lábnak az X, 0 -t,
//  ekkor az LCD_RS és LCD_WR lesz a touchscreen kiválasztott AD lába)
#define TS_XM_AN          X, 0
#define TS_YP_AN          X, 0

// Itt kell megadni, hogy melyik csatornát kiválasztva lehet az adott lábat az AD bemenetére kapcsolni
#define TS_XM_ADCCH       0
#define TS_YP_ADCCH       0

/* nsec nagyságrendü várakozás az LCD irási és az olvasási impulzusnál és a touchscreen AD átalakitonál
   - kezdö értéknek érdemes 10, 20 illetve 500-bol elindulni, aztán lehet csökkenteni a sebesség növelése érdekében
     (az értékek függnek a processzor orajelétöl és az LCD kijelzö sebességétöl is)
*/
#define LCD_WRITE_DELAY  10 // (72MHz)
#define LCD_READ_DELAY   20 // (72MHz)
#define TS_AD_DELAY     500

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
#define LCD_DIRWRITE { /* D0..D1, D14..D15, E7..E10 <- 0b01 */ \
GPIOD->MODER = (GPIOD->MODER & ~0b11110000000000000000000000001111) | 0b01010000000000000000000000000101; \
GPIOE->MODER = (GPIOE->MODER & ~0b00000000001111111100000000000000) | 0b00000000000101010100000000000000; }
// datapins setting to input (data direction: STM32 <- LCD)
#define LCD_DIRREAD { /* D0..D1, D14..D15, E7..E10 <- 0b00 */ \
GPIOD->MODER = (GPIOD->MODER & ~0b11110000000000000000000000001111); \
GPIOE->MODER = (GPIOE->MODER & ~0b00000000001111111100000000000000); }
// datapins write, STM32 -> LCD (write I/O pins from dt data)
#define LCD_WRITE(dt) { /* D14..15 <- dt0..1, D0..1 <- dt2..3, E7..10 <- dt4..7 */ \
GPIOD->ODR = (GPIOD->ODR & ~0b1100000000000011) | (((dt & 0b00000011) << 14) | ((dt & 0b00001100) >> 2)); \
GPIOE->ODR = (GPIOE->ODR & ~0b0000011110000000) | ((dt & 0b11110000) << 3); }
// datapins read, STM32 <- LCD (read from I/O pins and store to dt data)
#define LCD_READ(dt) { /* dt0..1 <- D14..15, dt2..3 <- D0..1, dt4..7 <- E7..10 */ \
dt = ((GPIOD->IDR & 0b1100000000000000) >> 14) | ((GPIOD->IDR & 0b0000000000000011) << 2) | \
     ((GPIOE->IDR & 0b0000011110000000) >> 3); }
#endif
