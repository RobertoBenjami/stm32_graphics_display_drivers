/*
 * 8 bites párhuzamos LCD/TOUCH GPIO driver STM32F1-re
 * 5 vezárlöláb (CS, RS, WR, RD, RST) + 8 adatláb + háttérvilágitás vezérlés

 * Figyelem: mivel azonos lábakon van az Lcd ás a Touchscreen,
 * ezért ezek ki kell zárni az Lcd és a Touchscreen egyidejü használatát!
 * Tábbszálas/megszakitásos környezetben igy gondoskodni kell az összeakadások megelözéséröl!
 */
#ifndef __LCDTS_IO8P_GPIO_H
#define __LCDTS_IO8P_GPIO_H

//=============================================================================
// Lcd vezérlö lábak hozzárendelése (A..M, 0..15)
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
#define TS_ADC            0

// Itt kell megadni, hogy melyik csatornát kiválasztva lehet az adott lábat az AD bemenetére kapcsolni
#define TS_XM_ADCCH       0
#define TS_YP_ADCCH       1

/* nsec nagyságrendü várakozás az LCD irási és az olvasási impulzusnál és a touchscreen AD átalakitonál
   - kezdö értéknek érdemes 10 illetve 500-bol elindulni, aztán lehet csökkenteni a sebesség növelése érdekében
     (az értékek függnek a processzor orajelétöl és az LCD kijelzö sebességétöl is)
*/
#define LCD_IO_RW_DELAY   1
#define TS_AD_DELAY     500

//=============================================================================
/* I/O csoport optimalizáció, hogy ne bitenként történjenek a GPIO mûveletek:
   Megj: ha az adat lábakat egy porton belül 0..7, vagy 8..15 lábakra definiáljuk
         automatikusan optimalizálva fognak történni a GPIO mûveletek akkor is, ha
         itt nem definiáljuk az optimalizált müködéshez szükséges eljárásokat
*/

// 5 vezérlöláb kimenetre állítása (mivel csak egyszer hajtódik végre, ez nem kritikus)
// #define LCD_CTR_DIRWRITE  GPIOA->CRL = (GPIOA->CRL & 0xFFFFFF00) | 0x00000033; GPIOB->CRL = (GPIOB->CRL & 0xFF0FFF00) | 0x00300033;

// 8 adatláb kimenetre állítása (adatirány: STM32 -> LCD)
// #define LCD_DATA_DIRWRITE GPIOB->CRH = 0x33333333

// 8 adatláb bemenetre állítása (adatirány: STM32 <- LCD)
// #define LCD_DATA_DIRREAD  GPIOB->CRH = 0x44444444

// 8 adatláb írása (a kiirandó adat a makro dt paraméterében van)
// #define LCD_DATA_WRITE(dt) GPIOB->ODR = (GPIOB->ODR & 0x00FF) | (dt << 8)

// 8 adatláb olvasása (az olvasott adat dt paraméterben megadott változoba kerül)
// #define LCD_DATA_READ(dt) dt = GPIOB->IDR >> 8

#endif // __LCDTS_IO8P_GPIO_H