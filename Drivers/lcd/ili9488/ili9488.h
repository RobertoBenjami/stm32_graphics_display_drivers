#ifndef __ILI9488_H
#define __ILI9488_H

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
 #define  ILI9488_MULTITASK_MUTEX  0

/* Orientácio:
   - 0: 320x480 micro-sd kártya felül (portrait)
   - 1: 480x320 micro-sd kártya gomb bal oldalt (landscape)
   - 2: 320x480 micro-sd kártya gomb alul (portrait)
   - 3: 480x320 micro-sd kártya jobb oldalt (landscape)
*/
#define  ILI9488_ORIENTATION       1

/* Color mode
   - 0: RGB565 (R:bit15..11, G:bit10..5, B:bit4..0)
   - 1: BRG565 (B:bit15..11, G:bit10..5, R:bit4..0)
*/
#define  ILI9488_COLORMODE         0

// ILI9488 Size (fizikai felbontás, az alapértelmezett orientáciora vonatkoztatva)
#define  ILI9488_LCD_PIXEL_WIDTH   320
#define  ILI9488_LCD_PIXEL_HEIGHT  480

/* LCD driver structure */
extern   LCD_DrvTypeDef   *lcd_drv;
extern   TS_DrvTypeDef    *ts_drv;

#ifdef __cplusplus
}
#endif

#endif /* __ILI9488_H */
