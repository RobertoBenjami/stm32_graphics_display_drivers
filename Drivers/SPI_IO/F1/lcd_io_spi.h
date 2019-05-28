/*
 * SPI LCD driver STM32F1
 * készitö: Roberto Benjami
 * verzio:  2019.05
*/

#ifndef __LCD_IO_SPI_H
#define __LCD_IO_SPI_H

//=============================================================================
// SPI kiválasztása (0, 1, 2 3)
// - 0: szoftveres SPI driver (a lábhozzárendelés szabadon kiválaszthato)
// - 1..3: hardver SPI driver (az LCD_SCK, LCD_MOSI, LCD_MISO lábak kötöttek)
#define LCD_SPI           2

// SPI üzemmod
// - 0: csak SPI TX (csak irni lehet a kijelzöt, LCD_MISO lábat nem szükséges megadni, nem lesz használatban)
// - 1: half duplex (LCD_MOSI láb két irányban lesz müködtetve, LCD_MISO láb nem lesz használva)
// - 2: full duplex (SPI TX: LCD_MOSI, SPI RX: LCD_MISO)
#define LCD_SPI_MODE      1

// SPI sebessége (csak hardver SPI esetén: az SPI orajele fPCLK / oszto (2..256) által meghatározott)
// - oszto: 0=/2, 1=/4, 2=/8, 3=/16, 4=/32, 5=/64, 6=/128, 7=/256
#define LCD_SPI_SPD       1
// - megadhato az olvasáshoz tartozo orajel (ha azonos vagy nincs megadva akkor nem vált sebességet olvasáskor)
#define LCD_SPI_SPD_READ  4

// Lcd vezérlö lábak hozzárendelése (reset, command/data, blacklight, blacklight bekapcsolt állapothoz tartozo szint)
#define LCD_RST           B, 11
#define LCD_RS            B, 14
#define LCD_BL            C, 13
#define LCD_BLON          0

// LCD_MISO megadása csak full duplex üzemmodban szükséges, TX és half duplex modban nem lesz használva
#define LCD_CS            B, 12
#define LCD_SCK           B, 13
#define LCD_MOSI          B, 15
// #define LCD_MISO          B, 14

// Ha a kijelzö nem képes 16 bit/pixel adatokat szolgáltatni, akkor röptében konvertálni kell (pl. ST7735)
// - 0: nem kell olvasáskor 24 bit/pixel-röl 16 bit/pixel-re konvertálni
// - 1: olvasáskor 24 bit/pixel-röl 16 bit/pixel-re kell konvertálni
#define LCD_READMULTIPLEDATA24TO16   1

#endif // __LCD_IO_SPI_H