/*
8 bites párhuzamos LCD/TOUCH FSMC driver
5 vezérlöláb (CS, RS, WR, RD, RST) + 8 adatláb + háttérvilágitás vezérlés
FSMC_NE1<-LCD_CS), FSMC_NOE<-LCD_RD, FSMC_NWE<-LCD_WR, FSMC_Ax<-LCD_RS
FSMC_D0<-LCD_D0, FSMC_D1<-LCD_D1, FSMC_D2<-LCD_D2, FSMC_D3<-LCD_D3
FSMC_D4<-LCD_D4, FSMC_D5<-LCD_D5, FSMC_D6<-LCD_D6, FSMC_D7<-LCD_D7

Figyelem: mivel azonos lábakon van az Lcd ás a Touchscreen,
ezért ezek ki kell zárni az Lcd és a Touchscreen egyidejü használatát!
Többszálas/megszakitásos környezetben igy gondoskodni kell az összeakadások megelözéséröl! */

//=============================================================================
/* Lcd vezérlö lábak hozzárendelése (A..M, 0..15, LCD_CS helye az FSMC által kötött) */
#define LCD_CS            X, 0
#define LCD_RST           X, 0

/* Háttérvilágitás vezérlés
   - BL: A..M, 0..15 (ha nem használjuk, akkor rendeljük hozzá az X, 0 értéket)
   - BL_ON: 0 vagy 1, a bekapcsolt állapothoz tartozó logikai szint */
#define LCD_BL            X, 0
#define LCD_BLON          0

//=============================================================================
/* Memoria cimek hozzárendelése, tipikus értékek:
  - Bank1 (NE1) 0x60000000
  - Bank2 (NE2) 0x64000000
  - Bank3 (NE3) 0x68000000
  - Bank4 (NE4) 0x6C000000
  - REGSELECT_BIT: ha pl. A18 lábra van kötve, akkor -> 18 */
#define LCD_ADDR_BASE     0x60000000
#define LCD_REGSELECT_BIT 18

/* DMA beállitások
   - 0..2: 0 = nincs DMA, 1 = DMA1, 2 = DMA2 (DMA2-t kell használni!)
   - 0..7: DMA csatorna
   - 0..7: Stream
   - 1..3: DMA prioritás (0=low..3=very high) */
#define LCD_DMA           0, 0, 0, 0

/*=============================================================================
Touch I/O lábak és A/D csatornák
A kijelzön belül a következö lábak vannak párhuzamositva:
 - TS_XM <- LCD_RS (FSMC_Ax)
 - TS_XP <- LCD_D6 (FSMC_D6)
 - TS_YM <- LCD_D7 (FSMC_D7)
 - TS_YP <- LCD_WR (FSMC_NWE)
*/

/* ADC konverter száma (értéke lehet 1, 2, 3, vagy 0 ha nem használjuk)
   - 0: analog touchscreen driver nem lesz használva
   - 1..3: a használni kivánt A/D konverter száma
*/
#define TS_ADC            0

// Ezen lábak helyzetét az FSMC vezérlö határozza meg (f407: D13, D5, E9, E10)
#define TS_XM             X, 0
#define TS_YP             X, 0
#define TS_XP             X, 0
#define TS_YM             X, 0

// Mivel az FSMC lábai jellemzöen nem használhatoak AD bemenetként, párhuzamosan köthetjük valamelyik AD bemenetnek is használhato lábbal.
// (ha nem adunk meg itt semmilyen lábat, akkor az LCD_RS és LCD_WR lesz a touchscreen kiválasztott AD lába)
#define TS_XM_AN          X, 0
#define TS_YP_AN          X, 0

// Itt kell megadni, hogy melyik csatornát kiválasztva lehet az adott lábat az AD bemenetére kapcsolni
#define TS_XM_ADCCH       0
#define TS_YP_ADCCH       0

/* nsec nagyságrendü várakozás a touchscreen AD átalakitonál
   - kezdö értéknek érdemes 500-bol elindulni, aztán lehet csökkenteni a sebesség növelése érdekében
     (az értékek függnek a processzor orajelétöl és az LCD kijelzö sebességétöl is)
*/
#define TS_AD_DELAY     500
