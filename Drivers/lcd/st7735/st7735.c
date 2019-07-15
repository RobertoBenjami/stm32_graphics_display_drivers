/*
 * ST7735 LCD driver v1.1
 * 2019.05. Add v1.1 extension (#ifdef LCD_DRVTYPE_V1_1)
*/

#include <string.h>
#include "main.h"
#include "lcd.h"
#include "st7735.h"

void     st7735_Init(void);
uint16_t st7735_ReadID(void);
void     st7735_DisplayOn(void);
void     st7735_DisplayOff(void);
void     st7735_SetCursor(uint16_t Xpos, uint16_t Ypos);
void     st7735_WritePixel(uint16_t Xpos, uint16_t Ypos, uint16_t RGB_Code);
uint16_t st7735_ReadPixel(uint16_t Xpos, uint16_t Ypos);
void     st7735_SetDisplayWindow(uint16_t Xpos, uint16_t Ypos, uint16_t Width, uint16_t Height);
void     st7735_DrawHLine(uint16_t RGBCode, uint16_t Xpos, uint16_t Ypos, uint16_t Length);
void     st7735_DrawVLine(uint16_t RGBCode, uint16_t Xpos, uint16_t Ypos, uint16_t Length);
void     st7735_FillRect(uint16_t Xpos, uint16_t Ypos, uint16_t Xsize, uint16_t Ysize, uint16_t RGBCode);
uint16_t st7735_GetLcdPixelWidth(void);
uint16_t st7735_GetLcdPixelHeight(void);
void     st7735_DrawBitmap(uint16_t Xpos, uint16_t Ypos, uint8_t *pbmp);
void     st7735_DrawRGBImage(uint16_t Xpos, uint16_t Ypos, uint16_t Xsize, uint16_t Ysize, uint8_t *pData);
void     st7735_ReadRGBImage(uint16_t Xpos, uint16_t Ypos, uint16_t Xsize, uint16_t Ysize, uint8_t *pData);

LCD_DrvTypeDef   st7735_drv =
{
  st7735_Init,
  st7735_ReadID,
  st7735_DisplayOn,
  st7735_DisplayOff,
  st7735_SetCursor,
  st7735_WritePixel,
  st7735_ReadPixel,
  st7735_SetDisplayWindow,
  st7735_DrawHLine,
  st7735_DrawVLine,
  st7735_GetLcdPixelWidth,
  st7735_GetLcdPixelHeight,
  st7735_DrawBitmap,
  st7735_DrawRGBImage,
  #ifdef   LCD_DRVTYPE_V1_1
  st7735_FillRect,
  st7735_ReadRGBImage,
  #endif
};

LCD_DrvTypeDef  *lcd_drv = &st7735_drv;

#define ST7735_NOP            0x00
#define ST7735_SWRESET        0x01

#define ST7735_RDDID          0x04
#define ST7735_RDDST          0x09
#define ST7735_RDMODE         0x0A
#define ST7735_RDMADCTL       0x0B
#define ST7735_RDPIXFMT       0x0C
#define ST7735_RDIMGFMT       0x0D
#define ST7735_RDSELFDIAG     0x0F

#define ST7735_SLPIN          0x10
#define ST7735_SLPOUT         0x11
#define ST7735_PTLON          0x12
#define ST7735_NORON          0x13

#define ST7735_INVOFF         0x20
#define ST7735_INVON          0x21
#define ST7735_GAMMASET       0x26
#define ST7735_DISPOFF        0x28
#define ST7735_DISPON         0x29

#define ST7735_CASET          0x2A
#define ST7735_PASET          0x2B
#define ST7735_RAMWR          0x2C
#define ST7735_RAMRD          0x2E

#define ST7735_PTLAR          0x30
#define ST7735_MADCTL         0x36
#define ST7735_COLMOD         0x3A

#define ST7735_FRMCTR1        0xB1
#define ST7735_FRMCTR2        0xB2
#define ST7735_FRMCTR3        0xB3
#define ST7735_INVCTR         0xB4
#define ST7735_DISSET5        0xB6

#define ST7735_PWCTR1         0xC0
#define ST7735_PWCTR2         0xC1
#define ST7735_PWCTR3         0xC2
#define ST7735_PWCTR4         0xC3
#define ST7735_PWCTR5         0xC4
#define ST7735_VMCTR1         0xC5
#define ST7735_VMCTR2         0xC7

#define ST7735_RDID1          0xDA
#define ST7735_RDID2          0xDB
#define ST7735_RDID3          0xDC
#define ST7735_RDID4          0xDD

#define ST7735_GMCTRP1        0xE0
#define ST7735_GMCTRN1        0xE1

#define ST7735_PWCTR6         0xFC

// entry mode bitjei szinsorrend, rajzolási irány)
#define ST7735_MAD_RGB        0x00
#define ST7735_MAD_BGR        0x08

#define ST7735_MAD_VERTICAL   0x20
#define ST7735_MAD_X_LEFT     0x00
#define ST7735_MAD_X_RIGHT    0x40
#define ST7735_MAD_Y_UP       0x80
#define ST7735_MAD_Y_DOWN     0x00

#if ST7735_COLORMODE == 0
#define ST7735_MAD_COLORMODE  ST7735_MAD_RGB
#else
#define ST7735_MAD_COLORMODE  ST7735_MAD_BGR
#endif

// Az orientáciokhoz tartozo ENTRY modok jobra/fel és jobbra/le rajzolási irányhoz és a maximális koordináták
#if (ST7735_ORIENTATION == 0)
#define ST7735_SIZE_X                     ST7735_LCD_PIXEL_WIDTH
#define ST7735_SIZE_Y                     ST7735_LCD_PIXEL_HEIGHT
#define ST7735_MAD_DATA_RIGHT_THEN_UP     ST7735_MAD_COLORMODE | ST7735_MAD_X_LEFT  | ST7735_MAD_Y_UP
#define ST7735_MAD_DATA_RIGHT_THEN_DOWN   ST7735_MAD_COLORMODE | ST7735_MAD_X_LEFT  | ST7735_MAD_Y_DOWN
#elif (ST7735_ORIENTATION == 1)
#define ST7735_SIZE_X                     ST7735_LCD_PIXEL_HEIGHT
#define ST7735_SIZE_Y                     ST7735_LCD_PIXEL_WIDTH
#define ST7735_MAD_DATA_RIGHT_THEN_UP     ST7735_MAD_COLORMODE | ST7735_MAD_X_LEFT  | ST7735_MAD_Y_DOWN | ST7735_MAD_VERTICAL
#define ST7735_MAD_DATA_RIGHT_THEN_DOWN   ST7735_MAD_COLORMODE | ST7735_MAD_X_RIGHT | ST7735_MAD_Y_DOWN | ST7735_MAD_VERTICAL
#elif (ST7735_ORIENTATION == 2)
#define ST7735_SIZE_X                     ST7735_LCD_PIXEL_WIDTH
#define ST7735_SIZE_Y                     ST7735_LCD_PIXEL_HEIGHT
#define ST7735_MAD_DATA_RIGHT_THEN_UP     ST7735_MAD_COLORMODE | ST7735_MAD_X_RIGHT | ST7735_MAD_Y_DOWN
#define ST7735_MAD_DATA_RIGHT_THEN_DOWN   ST7735_MAD_COLORMODE | ST7735_MAD_X_RIGHT | ST7735_MAD_Y_UP
#elif (ST7735_ORIENTATION == 3)
#define ST7735_SIZE_X                     ST7735_LCD_PIXEL_HEIGHT
#define ST7735_SIZE_Y                     ST7735_LCD_PIXEL_WIDTH
#define ST7735_MAD_DATA_RIGHT_THEN_UP     ST7735_MAD_COLORMODE | ST7735_MAD_X_RIGHT | ST7735_MAD_Y_UP   | ST7735_MAD_VERTICAL
#define ST7735_MAD_DATA_RIGHT_THEN_DOWN   ST7735_MAD_COLORMODE | ST7735_MAD_X_LEFT  | ST7735_MAD_Y_UP   | ST7735_MAD_VERTICAL
#endif

#define ST7735_SETCURSOR(x, y) {  \
  LCD_IO_WriteCmd8(ST7735_CASET); \
  LCD_IO_WriteData16_to_2x8(x);   \
  LCD_IO_WriteData16_to_2x8(x);   \
  LCD_IO_WriteCmd8(ST7735_PASET); \
  LCD_IO_WriteData16_to_2x8(y);   \
  LCD_IO_WriteData16_to_2x8(y);   }

//-----------------------------------------------------------------------------
#define ST7735_LCD_INITIALIZED    0x01
#define ST7735_IO_INITIALIZED     0x02
static  uint8_t   Is_st7735_Initialized = 0;
static  uint16_t  yStart, yEnd;

//-----------------------------------------------------------------------------
/* Link function for LCD peripheral */
void     LCD_Delay (uint32_t delay);
void     LCD_IO_Init(void);
void     LCD_IO_Bl_OnOff(uint8_t Bl);

void     LCD_IO_WriteCmd8(uint8_t Cmd);
void     LCD_IO_WriteData8(uint8_t Data);
void     LCD_IO_WriteData16(uint16_t Data);
void     LCD_IO_WriteCmd8DataFill16(uint8_t Cmd, uint16_t Data, uint32_t Size);
void     LCD_IO_WriteCmd8MultipleData8(uint8_t Cmd, uint8_t *pData, uint32_t Size);
void     LCD_IO_WriteCmd8MultipleData16(uint8_t Cmd, uint16_t *pData, uint32_t Size);
void     LCD_IO_ReadCmd8MultipleData8(uint8_t Cmd, uint8_t *pData, uint32_t Size, uint32_t DummySize);
void     LCD_IO_ReadCmd8MultipleData24to16(uint8_t Cmd, uint16_t *pData, uint32_t Size, uint32_t DummySize);

#define  LCD_IO_WriteData16_to_2x8(dt)    {LCD_IO_WriteData8((dt) >> 8); LCD_IO_WriteData8(dt);}

//-----------------------------------------------------------------------------
void st7735_Init(void)
{
  if((Is_st7735_Initialized & ST7735_LCD_INITIALIZED) == 0)
  {
    Is_st7735_Initialized |= ST7735_LCD_INITIALIZED;
    if((Is_st7735_Initialized & ST7735_IO_INITIALIZED) == 0)
      LCD_IO_Init();
    Is_st7735_Initialized |= ST7735_IO_INITIALIZED;
  }

  LCD_Delay(1);
  LCD_IO_WriteCmd8(ST7735_SWRESET);
  LCD_Delay(1);

  // positive gamma control
  LCD_IO_WriteCmd8MultipleData8(ST7735_GMCTRP1, (uint8_t *)"\x09\x16\x09\x20\x21\x1B\x13\x19\x17\x15\x1E\x2B\x04\x05\x02\x0E", 16);

  // negative gamma control
  LCD_IO_WriteCmd8MultipleData8(ST7735_GMCTRN1, (uint8_t *)"\x0B\x14\x08\x1E\x22\x1D\x18\x1E\x1B\x1A\x24\x2B\x06\x06\x02\x0F", 16);

  // Power Control 1 (Vreg1out, Verg2out)
  LCD_IO_WriteCmd8MultipleData8(ST7735_PWCTR1, (uint8_t *)"\x17\x15", 2);

  // Power Control 2 (VGH,VGL)
  LCD_IO_WriteCmd8MultipleData8(ST7735_PWCTR2, (uint8_t *)"\x41", 1);

  // Power Control 3 (Vcom)
  LCD_IO_WriteCmd8MultipleData8(ST7735_VMCTR1, (uint8_t *)"\x00\x12\x80", 3);

  // LCD_IO_WriteMultipleData8(ST7735_COLMOD, (uint8_t *)"\x55", 1); // Interface Pixel Format (16 bit)
  LCD_IO_WriteCmd8MultipleData8(ST7735_COLMOD, (uint8_t *)"\x05", 1); // Interface Pixel Format (16 bit)
  LCD_IO_WriteCmd8MultipleData8(0xB0, (uint8_t *)"\x80", 1); // Interface Mode Control (SDO NOT USE)
  LCD_IO_WriteCmd8MultipleData8(0xB1, (uint8_t *)"\xA0", 1);// Frame rate (60Hz)
  LCD_IO_WriteCmd8MultipleData8(0xB4, (uint8_t *)"\x02", 1);// Display Inversion Control (2-dot)
  LCD_IO_WriteCmd8MultipleData8(0xB6, (uint8_t *)"\x02\x02", 2); // Display Function Control RGB/MCU Interface Control
  LCD_IO_WriteCmd8MultipleData8(0xE9, (uint8_t *)"\x00", 1);// Set Image Functio (Disable 24 bit data)
  LCD_IO_WriteCmd8MultipleData8(0xF7, (uint8_t *)"\xA9\x51\x2C\x82", 4);// Adjust Control (D7 stream, loose)

  LCD_IO_WriteCmd8(ST7735_MADCTL); LCD_IO_WriteData8(ST7735_MAD_DATA_RIGHT_THEN_DOWN);
  LCD_IO_WriteCmd8(ST7735_SLPOUT);    // Exit Sleep
  LCD_IO_WriteCmd8(ST7735_DISPON);    // Display on
}

//-----------------------------------------------------------------------------
/**
  * @brief  Enables the Display.
  * @param  None
  * @retval None
  */
void st7735_DisplayOn(void)
{
  LCD_IO_Bl_OnOff(1);
  LCD_IO_WriteCmd8(ST7735_SLPOUT);    // Exit Sleep
}

//-----------------------------------------------------------------------------
/**
  * @brief  Disables the Display.
  * @param  None
  * @retval None
  */
void st7735_DisplayOff(void)
{
  LCD_IO_WriteCmd8(ST7735_SLPIN);    // Sleep
  LCD_IO_Bl_OnOff(0);
}

//-----------------------------------------------------------------------------
/**
  * @brief  Get the LCD pixel Width.
  * @param  None
  * @retval The Lcd Pixel Width
  */
uint16_t st7735_GetLcdPixelWidth(void)
{
  return ST7735_SIZE_X;
}

//-----------------------------------------------------------------------------
/**
  * @brief  Get the LCD pixel Height.
  * @param  None
  * @retval The Lcd Pixel Height
  */
uint16_t st7735_GetLcdPixelHeight(void)
{
  return ST7735_SIZE_Y;
}

//-----------------------------------------------------------------------------
/**
  * @brief  Get the ST7735 ID.
  * @param  None
  * @retval The ST7735 ID
  */
uint16_t st7735_ReadID(void)
{
  uint32_t dt = 0;
  LCD_IO_ReadCmd8MultipleData8(ST7735_RDDID, (uint8_t *)&dt, 3, 0);
  if(dt == 0xF0897C) // ID1 = 0x7C, ID2 = 0x89, ID3 = 0xF0
    return 0x7735;
  else
    return 0;
}

//-----------------------------------------------------------------------------
/**
  * @brief  Set Cursor position.
  * @param  Xpos: specifies the X position.
  * @param  Ypos: specifies the Y position.
  * @retval None
  */
void st7735_SetCursor(uint16_t Xpos, uint16_t Ypos)
{
  ST7735_SETCURSOR(Xpos, Ypos);
}

//-----------------------------------------------------------------------------
/**
  * @brief  Write pixel.
  * @param  Xpos: specifies the X position.
  * @param  Ypos: specifies the Y position.
  * @param  RGBCode: the RGB pixel color
  * @retval None
  */
void st7735_WritePixel(uint16_t Xpos, uint16_t Ypos, uint16_t RGBCode)
{
  ST7735_SETCURSOR(Xpos, Ypos);
  LCD_IO_WriteCmd8DataFill16(ST7735_RAMWR, RGBCode, 1);
}


//-----------------------------------------------------------------------------
/**
  * @brief  Read pixel.
  * @param  None
  * @retval the RGB pixel color
  */
uint16_t st7735_ReadPixel(uint16_t Xpos, uint16_t Ypos)
{
  uint16_t ret;
  ST7735_SETCURSOR(Xpos, Ypos);
  LCD_IO_ReadCmd8MultipleData24to16(ST7735_RAMRD, &ret, 1, 1);
  return(ret);
}

//-----------------------------------------------------------------------------
/**
  * @brief  Sets a display window
  * @param  Xpos:   specifies the X bottom left position.
  * @param  Ypos:   specifies the Y bottom left position.
  * @param  Height: display window height.
  * @param  Width:  display window width.
  * @retval None
  */
void st7735_SetDisplayWindow(uint16_t Xpos, uint16_t Ypos, uint16_t Width, uint16_t Height)
{
  yStart = Ypos; yEnd = Ypos + Height - 1;
  LCD_IO_WriteCmd8(ST7735_CASET); LCD_IO_WriteData16_to_2x8(Xpos); LCD_IO_WriteData16_to_2x8(Xpos + Width - 1);
  LCD_IO_WriteCmd8(ST7735_PASET); LCD_IO_WriteData16_to_2x8(Ypos); LCD_IO_WriteData16_to_2x8(Ypos + Height - 1);
}

//-----------------------------------------------------------------------------
/**
  * @brief  Draw vertical line.
  * @param  RGBCode: Specifies the RGB color
  * @param  Xpos:     specifies the X position.
  * @param  Ypos:     specifies the Y position.
  * @param  Length:   specifies the Line length.
  * @retval None
  */
void st7735_DrawHLine(uint16_t RGBCode, uint16_t Xpos, uint16_t Ypos, uint16_t Length)
{
  LCD_IO_WriteCmd8(ST7735_CASET); LCD_IO_WriteData16_to_2x8(Xpos); LCD_IO_WriteData16_to_2x8(Xpos + Length - 1);
  LCD_IO_WriteCmd8(ST7735_PASET); LCD_IO_WriteData16_to_2x8(Ypos); LCD_IO_WriteData16_to_2x8(Ypos);
  LCD_IO_WriteCmd8DataFill16(ST7735_RAMWR, RGBCode, Length);
}

//-----------------------------------------------------------------------------
/**
  * @brief  Draw vertical line.
  * @param  RGBCode: Specifies the RGB color
  * @param  Xpos:     specifies the X position.
  * @param  Ypos:     specifies the Y position.
  * @param  Length:   specifies the Line length.
  * @retval None
  */
void st7735_DrawVLine(uint16_t RGBCode, uint16_t Xpos, uint16_t Ypos, uint16_t Length)
{
  LCD_IO_WriteCmd8(ST7735_CASET); LCD_IO_WriteData16_to_2x8(Xpos); LCD_IO_WriteData16_to_2x8(Xpos);
  LCD_IO_WriteCmd8(ST7735_PASET); LCD_IO_WriteData16_to_2x8(Ypos); LCD_IO_WriteData16_to_2x8(Ypos + Length - 1);
  LCD_IO_WriteCmd8DataFill16(ST7735_RAMWR, RGBCode, Length);
}

//-----------------------------------------------------------------------------
/**
  * @brief  Draw Filled rectangle
  * @param  Xpos:     specifies the X position.
  * @param  Ypos:     specifies the Y position.
  * @param  Xsize:    specifies the X size
  * @param  Ysize:    specifies the Y size
  * @param  RGBCode:  specifies the RGB color
  * @retval None
  */
void st7735_FillRect(uint16_t Xpos, uint16_t Ypos, uint16_t Xsize, uint16_t Ysize, uint16_t RGBCode)
{
  LCD_IO_WriteCmd8(ST7735_CASET); LCD_IO_WriteData16_to_2x8(Xpos); LCD_IO_WriteData16_to_2x8(Xpos + Xsize - 1);
  LCD_IO_WriteCmd8(ST7735_PASET); LCD_IO_WriteData16_to_2x8(Ypos); LCD_IO_WriteData16_to_2x8(Ypos + Ysize - 1);
  LCD_IO_WriteCmd8DataFill16(ST7735_RAMWR, RGBCode, Xsize * Ysize);
}

//-----------------------------------------------------------------------------
/**
  * @brief  Displays a 16bit bitmap picture..
  * @param  BmpAddress: Bmp picture address.
  * @param  Xpos:  Bmp X position in the LCD
  * @param  Ypos:  Bmp Y position in the LCD
  * @retval None
  * @brief  Draw direction: right then up
  */
void st7735_DrawBitmap(uint16_t Xpos, uint16_t Ypos, uint8_t *pbmp)
{
  uint32_t index, size;
  /* Read bitmap size */
  size = *(volatile uint16_t *) (pbmp + 2);
  size |= (*(volatile uint16_t *) (pbmp + 4)) << 16;
  /* Get bitmap data address offset */
  index = *(volatile uint16_t *) (pbmp + 10);
  index |= (*(volatile uint16_t *) (pbmp + 12)) << 16;
  size = (size - index)/2;
  pbmp += index;

  LCD_IO_WriteCmd8(ST7735_MADCTL); LCD_IO_WriteData8(ST7735_MAD_DATA_RIGHT_THEN_UP);
  LCD_IO_WriteCmd8(ST7735_PASET); LCD_IO_WriteData16_to_2x8(ST7735_SIZE_Y - 1 - yEnd); LCD_IO_WriteData16_to_2x8(ST7735_SIZE_Y - 1 - yStart);
  LCD_IO_WriteCmd8MultipleData16(ST7735_RAMWR, (uint16_t *)pbmp, size);
  LCD_IO_WriteCmd8(ST7735_MADCTL); LCD_IO_WriteData8(ST7735_MAD_DATA_RIGHT_THEN_DOWN);
}

//-----------------------------------------------------------------------------
/**
  * @brief  Displays 16bit/pixel picture..
  * @param  pdata: picture address.
  * @param  Xpos:  Image X position in the LCD
  * @param  Ypos:  Image Y position in the LCD
  * @param  Xsize: Image X size in the LCD
  * @param  Ysize: Image Y size in the LCD
  * @retval None
  * @brief  Draw direction: right then down
  */
void st7735_DrawRGBImage(uint16_t Xpos, uint16_t Ypos, uint16_t Xsize, uint16_t Ysize, uint8_t *pData)
{
  st7735_SetDisplayWindow(Xpos, Ypos, Xsize, Ysize);
  LCD_IO_WriteCmd8MultipleData16(ST7735_RAMWR, (uint16_t *)pData, Xsize * Ysize);
}

//-----------------------------------------------------------------------------
/**
  * @brief  Read 16bit/pixel picture from Lcd and store to RAM
  * @param  pdata: picture address.
  * @param  Xpos:  Image X position in the LCD
  * @param  Ypos:  Image Y position in the LCD
  * @param  Xsize: Image X size in the LCD
  * @param  Ysize: Image Y size in the LCD
  * @retval None
  * @brief  Draw direction: right then down
  */
void st7735_ReadRGBImage(uint16_t Xpos, uint16_t Ypos, uint16_t Xsize, uint16_t Ysize, uint8_t *pData)
{
  st7735_SetDisplayWindow(Xpos, Ypos, Xsize, Ysize);
  LCD_IO_ReadCmd8MultipleData24to16(ST7735_RAMRD, (uint16_t *)pData, Xsize * Ysize, 1);
}
