#ifndef __ST7783_H
#define __ST7783_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Többszálas vagy megszakításból történö használat miatt az Lcd és a Touchscreen
   egyidejü használata összeakadást tud okozni (mivel közös I/O eröforrásokat használ)
   Ezzel a mutex-el ki lehet zárni az összeakadást.
   Az Lcd függvényei kivárják a Touchscreen fefejeztét,
   a touchscreen lekérdezés meg nem hajtodik végre, ha az Lcd éppen foglalt.
   Figyelem: Ha az Lcd prioritása magasabb mint a Touchscreen-é, végtelen ciklusba kerülhet!
   - 0: nincs védelem az LCD és a touchscreen egyidejüségének kizárására
   - 1: van védelem az LCD és a touchscreen egyidejüségének kizárására
*/
#define  ST7783_MULTITASK_MUTEX   0

/* Orientácio:
   - 0: 240x320 Reset gomb felül (portrait)
   - 1: 320x240 Reset gomb bal oldalt (landscape)
   - 2: 240x320 Reset gomb alul (portrait)
   - 3: 320x240 Reset gomb jobb oldalt (landscape)
*/
#define  ST7783_ORIENTATION       1

/* Color mode
   - 0: RGB565 (R:bit15..11, G:bit10..5, B:bit4..0)
   - 1: BRG565 (B:bit15..11, G:bit10..5, R:bit4..0)
*/
#define  ST7783_COLORMODE         0

// ST7783 Size (fizikai felbontás, az alapértelmezett orientáciora vonatkoztatva)
#define  ST7783_LCD_PIXEL_WIDTH   240
#define  ST7783_LCD_PIXEL_HEIGHT  320

/* LCD driver structure */
extern   LCD_DrvTypeDef   *lcd_drv;
extern   TS_DrvTypeDef    *ts_drv;

#ifdef __cplusplus
}
#endif

#endif /* __ST7783_H */
