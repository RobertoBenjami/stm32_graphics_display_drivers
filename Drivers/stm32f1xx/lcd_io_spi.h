  /*
 * SPI LCD driver STM32F1
 * készitö: Roberto Benjami
 * verzio:  2019.06
*/

//=============================================================================
/* SPI kiválasztása (0, 1, 2 3)
   - 0: szoftveres SPI driver (a lábhozzárendelés szabadon kiválaszthato)
   - 1..3: hardver SPI driver (az LCD_SCK, LCD_MOSI, LCD_MISO lábak kötöttek) */
#define LCD_SPI           0

/* SPI üzemmod
   - 0: csak SPI TX (csak irni lehet a kijelzöt, LCD_MISO lábat nem szükséges megadni, nem lesz használatban)
   - 1: half duplex (LCD_MOSI láb két irányban lesz müködtetve, LCD_MISO láb nem lesz használva)
   - 2: full duplex (SPI TX: LCD_MOSI, SPI RX: LCD_MISO) */
#define LCD_SPI_MODE      0

/* SPI sebessége
   - szoftver SPI: 0..
     - 0: semmi várakozás nem lesz
     - 1: GPIOX_ODR(LCD_SCK) = 0 idönyi várakozás
     - 2..: LCD_IO_Delay(LCD_SPI_SPD - 2)
   - hardver SPI: 0..7 oszto: fPCLK/oszto, 0=/2, 1=/4, 2=/8, 3=/16, 4=/32, 5=/64, 6=/128, 7=/256 */
#define LCD_SPI_SPD       1
/* Megadhato az olvasáshoz tartozo orajel (ha azonos vagy nincs megadva akkor nem vált sebességet olvasáskor) */
#define LCD_SPI_SPD_READ  3

/* Lcd vezérlö lábak hozzárendelése ((A..M, 0..15)
   - LCD_RST megadása nem kötelezö (X, 0)
   - LCD_MISO megadása csak full duplex (LCD_SPI_MODE 2) üzemmodban szükséges
   - hardver SPI esetén az SCK, MOSI, MISO lábak hozzárendelése kötött */
#define LCD_RST           X, 0
#define LCD_RS            X, 0

#define LCD_CS            X, 0
#define LCD_SCK           X, 0
#define LCD_MOSI          X, 0
#define LCD_MISO          X, 0

/* Háttérvilágitás vezérlés
   - BL: A..M, 0..15 (ha nem használjuk, akkor rendeljük hozzá az X, 0 értéket)
   - BL_ON: 0 vagy 1, a bekapcsolt állapothoz tartozó logikai szint */
#define LCD_BL            X, 0   // ha nem akarjuk használni X, 0 -t adjunk
#define LCD_BLON          0

/* Adatirány váltáskor (OUT->IN) van olyan kijelzõ, amelyik extra órajele(ke)t kér az SCK lábra
   - 0.. (ST7735: 1,  ILI9341: 0) */
#define LCD_SCK_EXTRACLK  0

/* DMA beállitások
   - 0..2: 0 = nincs DMA, 1 = DMA1, 2 = DMA2 (DMA request mapping)
   - 0..7: DMA csatorna (DMA request mapping)
   - 1..3: DMA prioritás (0=low..3=very high) */
#define LCD_DMA_TX        0, 0, 0
#define LCD_DMA_RX        0, 0, 0

/* DMA RX buffer [byte] (csak a ...24to16 függvények esetében lesz használatban)
   (2 egész számu hatványa legyen: 16, 32, 64, 128, 256, 512, 1024...  32768) */
#define LCD_DMA_RX_BUFSIZE 128

/* DMA RX buffer helye
   - 0: stack
   - 1: static buffer */
#define LCD_DMA_RX_BUFMODE 0
