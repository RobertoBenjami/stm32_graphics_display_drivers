/* Orientácio:
   - 0: 240x320 Reset gomb felül (portrait)
   - 1: 320x240 Reset gomb bal oldalt (landscape)
   - 2: 240x320 Reset gomb alul (portrait)
   - 3: 320x240 Reset gomb jobb oldalt (landscape)
*/
#define  ILI9328_ORIENTATION       0

/* Color mode
   - 0: RGB565 (R:bit15..11, G:bit10..5, B:bit4..0)
   - 1: BRG565 (B:bit15..11, G:bit10..5, R:bit4..0)
*/
#define  ILI9328_COLORMODE         0

/* Analog touchscreen
   - 0: Touchscreen tiltva
   - 1: Touchscreen engedélyezve
*/
#define  ILI9328_TOUCH             0

/* Többszálas vagy megszakításból történö használat miatt az Lcd és a Touchscreen
   egyidejü használata összeakadást tud okozni (mivel közös I/O eröforrásokat használ)
   Ezzel a mutex-el ki lehet zárni az összeakadást.
   Az Lcd függvényei kivárják a Touchscreen fefejeztét,
   a touchscreen lekérdezés meg nem hajtodik végre, ha az Lcd éppen foglalt.
   Figyelem: Ha az Lcd prioritása magasabb mint a Touchscreen-é, végtelen ciklusba kerülhet!
   - 0: nincs védelem az LCD és a touchscreen egyidejüségének kizárására
   - 1: van védelem az LCD és a touchscreen egyidejüségének kizárására
*/
#define  ILI9328_MULTITASK_MUTEX   0

// ILI9328 Size (fizikai felbontás, az alapértelmezett orientáciora vonatkoztatva)
#define  ILI9328_LCD_PIXEL_WIDTH   240
#define  ILI9328_LCD_PIXEL_HEIGHT  320
