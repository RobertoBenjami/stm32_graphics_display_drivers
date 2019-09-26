/*
 * ILI9341 LCD driver v1.1
 * 2019.07. Add v1.1 extension (#ifdef LCD_DRVTYPE_V1_1)
*/

#include <string.h>
#include "main.h"
#include "lcd.h"
#include "ili9341.h"

void     ili9341_Init(void);
uint16_t ili9341_ReadID(void);
void     ili9341_DisplayOn(void);
void     ili9341_DisplayOff(void);
void     ili9341_SetCursor(uint16_t Xpos, uint16_t Ypos);
void     ili9341_WritePixel(uint16_t Xpos, uint16_t Ypos, uint16_t RGB_Code);
uint16_t ili9341_ReadPixel(uint16_t Xpos, uint16_t Ypos);
void     ili9341_SetDisplayWindow(uint16_t Xpos, uint16_t Ypos, uint16_t Width, uint16_t Height);
void     ili9341_DrawHLine(uint16_t RGBCode, uint16_t Xpos, uint16_t Ypos, uint16_t Length);
void     ili9341_DrawVLine(uint16_t RGBCode, uint16_t Xpos, uint16_t Ypos, uint16_t Length);
void     ili9341_FillRect(uint16_t Xpos, uint16_t Ypos, uint16_t Xsize, uint16_t Ysize, uint16_t RGBCode);
uint16_t ili9341_GetLcdPixelWidth(void);
uint16_t ili9341_GetLcdPixelHeight(void);
void     ili9341_DrawBitmap(uint16_t Xpos, uint16_t Ypos, uint8_t *pbmp);
void     ili9341_DrawRGBImage(uint16_t Xpos, uint16_t Ypos, uint16_t Xsize, uint16_t Ysize, uint8_t *pData);
void     ili9341_ReadRGBImage(uint16_t Xpos, uint16_t Ypos, uint16_t Xsize, uint16_t Ysize, uint8_t *pData);

LCD_DrvTypeDef   ili9341_drv =
{
  ili9341_Init,
  ili9341_ReadID,
  ili9341_DisplayOn,
  ili9341_DisplayOff,
  ili9341_SetCursor,
  ili9341_WritePixel,
  ili9341_ReadPixel,
  ili9341_SetDisplayWindow,
  ili9341_DrawHLine,
  ili9341_DrawVLine,
  ili9341_GetLcdPixelWidth,
  ili9341_GetLcdPixelHeight,
  ili9341_DrawBitmap,
  ili9341_DrawRGBImage,
  #ifdef   LCD_DRVTYPE_V1_1
  ili9341_FillRect,
  ili9341_ReadRGBImage,
  #endif
};

LCD_DrvTypeDef  *lcd_drv = &ili9341_drv;

#define ILI9341_NOP            0x00
#define ILI9341_SWRESET        0x01

#define ILI9341_RDDID          0x04
#define ILI9341_RDDST          0x09
#define ILI9341_RDMODE         0x0A
#define ILI9341_RDMADCTL       0x0B
#define ILI9341_RDPIXFMT       0x0C
#define ILI9341_RDIMGFMT       0x0D
#define ILI9341_RDSELFDIAG     0x0F

#define ILI9341_SLPIN          0x10
#define ILI9341_SLPOUT         0x11
#define ILI9341_PTLON          0x12
#define ILI9341_NORON          0x13

#define ILI9341_INVOFF         0x20
#define ILI9341_INVON          0x21
#define ILI9341_GAMMASET       0x26
#define ILI9341_DISPOFF        0x28
#define ILI9341_DISPON         0x29

#define ILI9341_CASET          0x2A
#define ILI9341_PASET          0x2B
#define ILI9341_RAMWR          0x2C
#define ILI9341_RAMRD          0x2E

#define ILI9341_PTLAR          0x30
#define ILI9341_MADCTL         0x36
#define ILI9341_VSCRSADD       0x37     /* Vertical Scrolling Start Address */
#define ILI9341_PIXFMT         0x3A     /* COLMOD: Pixel Format Set */

#define ILI9341_FRMCTR1        0xB1
#define ILI9341_FRMCTR2        0xB2
#define ILI9341_FRMCTR3        0xB3
#define ILI9341_INVCTR         0xB4
#define ILI9341_DFUNCTR        0xB6     /* Display Function Control */

#define ILI9341_PWCTR1         0xC0
#define ILI9341_PWCTR2         0xC1
#define ILI9341_PWCTR3         0xC2
#define ILI9341_PWCTR4         0xC3
#define ILI9341_PWCTR5         0xC4
#define ILI9341_VMCTR1         0xC5
#define ILI9341_VMCTR2         0xC7

#define ILI9341_RDID1          0xDA
#define ILI9341_RDID2          0xDB
#define ILI9341_RDID3          0xDC
#define ILI9341_RDID4          0xDD

#define ILI9341_GMCTRP1        0xE0
#define ILI9341_GMCTRN1        0xE1

#define ILI9341_PWCTR6         0xFC

// entry mode bitjei szinsorrend, rajzolási irány)
#define ILI9341_MAD_RGB        0x08
#define ILI9341_MAD_BGR        0x00

#define ILI9341_MAD_VERTICAL   0x20
#define ILI9341_MAD_X_LEFT     0x00
#define ILI9341_MAD_X_RIGHT    0x40
#define ILI9341_MAD_Y_UP       0x80
#define ILI9341_MAD_Y_DOWN     0x00

#if ILI9341_COLORMODE == 0
#define ILI9341_MAD_COLORMODE  ILI9341_MAD_RGB
#else
#define ILI9341_MAD_COLORMODE  ILI9341_MAD_BGR
#endif

// Az orientáciokhoz tartozo ENTRY modok jobra/fel és jobbra/le rajzolási irányhoz és a maximális koordináták
#if (ILI9341_ORIENTATION == 0)
#define ILI9341_SIZE_X                     ILI9341_LCD_PIXEL_WIDTH
#define ILI9341_SIZE_Y                     ILI9341_LCD_PIXEL_HEIGHT
#define ILI9341_MAD_DATA_RIGHT_THEN_UP     ILI9341_MAD_COLORMODE | ILI9341_MAD_X_RIGHT | ILI9341_MAD_Y_UP
#define ILI9341_MAD_DATA_RIGHT_THEN_DOWN   ILI9341_MAD_COLORMODE | ILI9341_MAD_X_RIGHT | ILI9341_MAD_Y_DOWN
#elif (ILI9341_ORIENTATION == 1)
#define ILI9341_SIZE_X                     ILI9341_LCD_PIXEL_HEIGHT
#define ILI9341_SIZE_Y                     ILI9341_LCD_PIXEL_WIDTH
#define ILI9341_MAD_DATA_RIGHT_THEN_UP     ILI9341_MAD_COLORMODE | ILI9341_MAD_X_RIGHT | ILI9341_MAD_Y_DOWN | ILI9341_MAD_VERTICAL
#define ILI9341_MAD_DATA_RIGHT_THEN_DOWN   ILI9341_MAD_COLORMODE | ILI9341_MAD_X_LEFT  | ILI9341_MAD_Y_DOWN | ILI9341_MAD_VERTICAL
#elif (ILI9341_ORIENTATION == 2)
#define ILI9341_SIZE_X                     ILI9341_LCD_PIXEL_WIDTH
#define ILI9341_SIZE_Y                     ILI9341_LCD_PIXEL_HEIGHT
#define ILI9341_MAD_DATA_RIGHT_THEN_UP     ILI9341_MAD_COLORMODE | ILI9341_MAD_X_LEFT  | ILI9341_MAD_Y_DOWN
#define ILI9341_MAD_DATA_RIGHT_THEN_DOWN   ILI9341_MAD_COLORMODE | ILI9341_MAD_X_LEFT  | ILI9341_MAD_Y_UP
#elif (ILI9341_ORIENTATION == 3)
#define ILI9341_SIZE_X                     ILI9341_LCD_PIXEL_HEIGHT
#define ILI9341_SIZE_Y                     ILI9341_LCD_PIXEL_WIDTH
#define ILI9341_MAD_DATA_RIGHT_THEN_UP     ILI9341_MAD_COLORMODE | ILI9341_MAD_X_LEFT  | ILI9341_MAD_Y_UP   | ILI9341_MAD_VERTICAL
#define ILI9341_MAD_DATA_RIGHT_THEN_DOWN   ILI9341_MAD_COLORMODE | ILI9341_MAD_X_RIGHT | ILI9341_MAD_Y_UP   | ILI9341_MAD_VERTICAL
#endif

#define ILI9341_SETCURSOR(x, y)            {LCD_IO_WriteCmd8(ILI9341_CASET); LCD_IO_WriteData16_to_2x8(x); LCD_IO_WriteData16_to_2x8(x); \
                                            LCD_IO_WriteCmd8(ILI9341_PASET); LCD_IO_WriteData16_to_2x8(y); LCD_IO_WriteData16_to_2x8(y); }

//-----------------------------------------------------------------------------
#define ILI9341_LCD_INITIALIZED    0x01
#define ILI9341_IO_INITIALIZED     0x02
static  uint8_t   Is_ili9341_Initialized = 0;
static  uint16_t  yStart, yEnd;

//-----------------------------------------------------------------------------
/* Link function for LCD peripheral */
void     LCD_Delay (uint32_t delay);
void     LCD_IO_Init(void);
void     LCD_IO_Bl_OnOff(uint8_t Bl);

void     LCD_IO_WriteCmd8(uint8_t Cmd);
void     LCD_IO_WriteCmd16(uint16_t Cmd);
void     LCD_IO_WriteData8(uint8_t Data);
void     LCD_IO_WriteData16(uint16_t Data);

void     LCD_IO_WriteCmd8DataFill16(uint8_t Cmd, uint16_t Data, uint32_t Size);
void     LCD_IO_WriteCmd8MultipleData8(uint8_t Cmd, uint8_t *pData, uint32_t Size);
void     LCD_IO_WriteCmd8MultipleData16(uint8_t Cmd, uint16_t *pData, uint32_t Size);
void     LCD_IO_WriteCmd16DataFill16(uint16_t Cmd, uint16_t Data, uint32_t Size);
void     LCD_IO_WriteCmd16MultipleData8(uint16_t Cmd, uint8_t *pData, uint32_t Size);
void     LCD_IO_WriteCmd16MultipleData16(uint16_t Cmd, uint16_t *pData, uint32_t Size);

void     LCD_IO_ReadCmd8MultipleData8(uint8_t Cmd, uint8_t *pData, uint32_t Size, uint32_t DummySize);
void     LCD_IO_ReadCmd8MultipleData16(uint8_t Cmd, uint16_t *pData, uint32_t Size, uint32_t DummySize);
void     LCD_IO_ReadCmd8MultipleData24to16(uint8_t Cmd, uint16_t *pData, uint32_t Size, uint32_t DummySize);
void     LCD_IO_ReadCmd16MultipleData8(uint16_t Cmd, uint8_t *pData, uint32_t Size, uint32_t DummySize);
void     LCD_IO_ReadCmd16MultipleData16(uint16_t Cmd, uint16_t *pData, uint32_t Size, uint32_t DummySize);
void     LCD_IO_ReadCmd16MultipleData24to16(uint16_t Cmd, uint16_t *pData, uint32_t Size, uint32_t DummySize);

#define  LCD_IO_WriteData16_to_2x8(dt)    {LCD_IO_WriteData8((dt) >> 8); LCD_IO_WriteData8(dt); }

//-----------------------------------------------------------------------------
void ili9341_Init(void)
{
  if((Is_ili9341_Initialized & ILI9341_LCD_INITIALIZED) == 0)
  {
    Is_ili9341_Initialized |= ILI9341_LCD_INITIALIZED;
    if((Is_ili9341_Initialized & ILI9341_IO_INITIALIZED) == 0)
      LCD_IO_Init();
    Is_ili9341_Initialized |= ILI9341_IO_INITIALIZED;
  }

  LCD_Delay(1);
  LCD_IO_WriteCmd8(ILI9341_SWRESET);
  LCD_Delay(1);

  LCD_IO_WriteCmd8MultipleData8(0xEF, (uint8_t *)"\x03\x80\x02", 3);
  LCD_IO_WriteCmd8MultipleData8(0xCF, (uint8_t *)"\x00\xC1\x30", 3);
  LCD_IO_WriteCmd8MultipleData8(0xED, (uint8_t *)"\x64\x03\x12\x81", 4);
  LCD_IO_WriteCmd8MultipleData8(0xE8, (uint8_t *)"\x85\x00\x78", 3);
  LCD_IO_WriteCmd8MultipleData8(0xCB, (uint8_t *)"\x39\x2C\x00\x34\x02", 5);
  LCD_IO_WriteCmd8MultipleData8(0xF7, (uint8_t *)"\x20", 1);
  LCD_IO_WriteCmd8MultipleData8(0xEA, (uint8_t *)"\x00\x00", 2);

  // Power Control 1 (Vreg1out, Verg2out)
  LCD_IO_WriteCmd8MultipleData8(ILI9341_PWCTR1, (uint8_t *)"\x23", 1);

  // Power Control 2 (VGH,VGL)
  LCD_IO_WriteCmd8MultipleData8(ILI9341_PWCTR2, (uint8_t *)"\x10", 1);

  // Power Control 3 (Vcom)
  LCD_IO_WriteCmd8MultipleData8(ILI9341_VMCTR1, (uint8_t *)"\x3E\x28", 2);

  // Power Control 3 (Vcom)
  LCD_IO_WriteCmd8MultipleData8(ILI9341_VMCTR2, (uint8_t *)"\x86", 1);

  // Vertical scroll zero
  LCD_IO_WriteCmd8MultipleData8(ILI9341_VSCRSADD, (uint8_t *)"\x00", 1);
  LCD_IO_WriteCmd8MultipleData8(ILI9341_PIXFMT, (uint8_t *)"\x55", 1);

  // LCD_IO_WriteCmd8MultipleData8(0xF6, (uint8_t *)"\x01\x00\x06", 3);

  LCD_IO_WriteCmd8MultipleData8(ILI9341_FRMCTR1, (uint8_t *)"\x00\x18", 2);
  LCD_IO_WriteCmd8MultipleData8(ILI9341_DFUNCTR, (uint8_t *)"\x08\x82\x27", 3);  // Display Function Control
  LCD_IO_WriteCmd8MultipleData8(0xF2, (uint8_t *)"\x00", 1);            // 3Gamma Function Disable
  LCD_IO_WriteCmd8MultipleData8(ILI9341_GAMMASET, (uint8_t *)"\x01", 1);// Gamma curve selected

  // positive gamma control
  LCD_IO_WriteCmd8MultipleData8(ILI9341_GMCTRP1, (uint8_t *)"\x0F\x31\x2B\x0C\x0E\x08\x4E\xF1\x37\x07\x10\x03\x0E\x09\x00", 15);

  // negative gamma control
  LCD_IO_WriteCmd8MultipleData8(ILI9341_GMCTRN1, (uint8_t *)"\x00\x0E\x14\x03\x11\x07\x31\xC1\x48\x08\x0F\x0C\x31\x36\x0F", 15);

  LCD_IO_WriteCmd8(ILI9341_MADCTL); LCD_IO_WriteData8(ILI9341_MAD_DATA_RIGHT_THEN_DOWN);
  LCD_IO_WriteCmd8(ILI9341_SLPOUT);    // Exit Sleep
  LCD_IO_WriteCmd8(ILI9341_DISPON);    // Display on
}

//-----------------------------------------------------------------------------
/**
  * @brief  Enables the Display.
  * @param  None
  * @retval None
  */
void ili9341_DisplayOn(void)
{
  LCD_IO_Bl_OnOff(1);
  LCD_IO_WriteCmd8(ILI9341_SLPOUT);    // Exit Sleep
}

//-----------------------------------------------------------------------------
/**
  * @brief  Disables the Display.
  * @param  None
  * @retval None
  */
void ili9341_DisplayOff(void)
{
  LCD_IO_WriteCmd8(ILI9341_SLPIN);    // Sleep
  LCD_IO_Bl_OnOff(0);
}

//-----------------------------------------------------------------------------
/**
  * @brief  Get the LCD pixel Width.
  * @param  None
  * @retval The Lcd Pixel Width
  */
uint16_t ili9341_GetLcdPixelWidth(void)
{
  return ILI9341_SIZE_X;
}

//-----------------------------------------------------------------------------
/**
  * @brief  Get the LCD pixel Height.
  * @param  None
  * @retval The Lcd Pixel Height
  */
uint16_t ili9341_GetLcdPixelHeight(void)
{
  return ILI9341_SIZE_Y;
}

//-----------------------------------------------------------------------------
/**
  * @brief  Get the ILI9341 ID.
  * @param  None
  * @retval The ILI9341 ID
  * @rem    On the my lcd is unusable (stm32f429 discovery)
  */
uint16_t ili9341_ReadID(void)
{
  uint32_t dt = 0;
  LCD_IO_ReadCmd8MultipleData8(0xD3, (uint8_t *)&dt, 3, 1);
  if(dt == 0x419300)
    return 0x9341;
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
void ili9341_SetCursor(uint16_t Xpos, uint16_t Ypos)
{
  ILI9341_SETCURSOR(Xpos, Ypos);
}

//-----------------------------------------------------------------------------
/**
  * @brief  Write pixel.
  * @param  Xpos: specifies the X position.
  * @param  Ypos: specifies the Y position.
  * @param  RGBCode: the RGB pixel color
  * @retval None
  */
void ili9341_WritePixel(uint16_t Xpos, uint16_t Ypos, uint16_t RGBCode)
{
  ILI9341_SETCURSOR(Xpos, Ypos);
  LCD_IO_WriteCmd8(ILI9341_RAMWR); LCD_IO_WriteData16(RGBCode);
}


//-----------------------------------------------------------------------------
/**
  * @brief  Read pixel.
  * @param  None
  * @retval the RGB pixel color
  */
uint16_t ili9341_ReadPixel(uint16_t Xpos, uint16_t Ypos)
{
  uint16_t ret;
  LCD_IO_WriteCmd8MultipleData8(ILI9341_PIXFMT, (uint8_t *)"\x66", 1); // Read: only 24bit pixel mode
  ILI9341_SETCURSOR(Xpos, Ypos);
  LCD_IO_ReadCmd8MultipleData24to16(ILI9341_RAMRD, (uint16_t *)&ret, 1, 1);
  LCD_IO_WriteCmd8MultipleData8(ILI9341_PIXFMT, (uint8_t *)"\x55", 1); // Return to 16bit pixel mode
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
void ili9341_SetDisplayWindow(uint16_t Xpos, uint16_t Ypos, uint16_t Width, uint16_t Height)
{
  yStart = Ypos; yEnd = Ypos + Height - 1;
  LCD_IO_WriteCmd8(ILI9341_CASET); LCD_IO_WriteData16_to_2x8(Xpos); LCD_IO_WriteData16_to_2x8(Xpos + Width - 1);
  LCD_IO_WriteCmd8(ILI9341_PASET); LCD_IO_WriteData16_to_2x8(Ypos); LCD_IO_WriteData16_to_2x8(Ypos + Height - 1);
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
void ili9341_DrawHLine(uint16_t RGBCode, uint16_t Xpos, uint16_t Ypos, uint16_t Length)
{
  LCD_IO_WriteCmd8(ILI9341_CASET); LCD_IO_WriteData16_to_2x8(Xpos); LCD_IO_WriteData16_to_2x8(Xpos + Length - 1);
  LCD_IO_WriteCmd8(ILI9341_PASET); LCD_IO_WriteData16_to_2x8(Ypos); LCD_IO_WriteData16_to_2x8(Ypos);
  LCD_IO_WriteCmd8DataFill16(ILI9341_RAMWR, RGBCode, Length);
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
void ili9341_DrawVLine(uint16_t RGBCode, uint16_t Xpos, uint16_t Ypos, uint16_t Length)
{
  LCD_IO_WriteCmd8(ILI9341_CASET); LCD_IO_WriteData16_to_2x8(Xpos); LCD_IO_WriteData16_to_2x8(Xpos);
  LCD_IO_WriteCmd8(ILI9341_PASET); LCD_IO_WriteData16_to_2x8(Ypos); LCD_IO_WriteData16_to_2x8(Ypos + Length - 1);
  LCD_IO_WriteCmd8DataFill16(ILI9341_RAMWR, RGBCode, Length);
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
void ili9341_FillRect(uint16_t Xpos, uint16_t Ypos, uint16_t Xsize, uint16_t Ysize, uint16_t RGBCode)
{
  LCD_IO_WriteCmd8(ILI9341_CASET); LCD_IO_WriteData16_to_2x8(Xpos); LCD_IO_WriteData16_to_2x8(Xpos + Xsize - 1);
  LCD_IO_WriteCmd8(ILI9341_PASET); LCD_IO_WriteData16_to_2x8(Ypos); LCD_IO_WriteData16_to_2x8(Ypos + Ysize - 1);
  LCD_IO_WriteCmd8DataFill16(ILI9341_RAMWR, RGBCode, Xsize * Ysize);
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
void ili9341_DrawBitmap(uint16_t Xpos, uint16_t Ypos, uint8_t *pbmp)
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

  LCD_IO_WriteCmd8(ILI9341_MADCTL); LCD_IO_WriteData8(ILI9341_MAD_DATA_RIGHT_THEN_UP);
  LCD_IO_WriteCmd8(ILI9341_PASET); LCD_IO_WriteData16_to_2x8(ILI9341_SIZE_Y - 1 - yEnd); LCD_IO_WriteData16_to_2x8(ILI9341_SIZE_Y - 1 - yStart);
  LCD_IO_WriteCmd8MultipleData16(ILI9341_RAMWR, (uint16_t *)pbmp, size);
  LCD_IO_WriteCmd8(ILI9341_MADCTL); LCD_IO_WriteData8(ILI9341_MAD_DATA_RIGHT_THEN_DOWN);
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
void ili9341_DrawRGBImage(uint16_t Xpos, uint16_t Ypos, uint16_t Xsize, uint16_t Ysize, uint8_t *pData)
{
  ili9341_SetDisplayWindow(Xpos, Ypos, Xsize, Ysize);
  LCD_IO_WriteCmd8MultipleData16(ILI9341_RAMWR, (uint16_t *)pData, Xsize * Ysize);
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
void ili9341_ReadRGBImage(uint16_t Xpos, uint16_t Ypos, uint16_t Xsize, uint16_t Ysize, uint8_t *pData)
{
  ili9341_SetDisplayWindow(Xpos, Ypos, Xsize, Ysize);
  LCD_IO_WriteCmd8MultipleData8(ILI9341_PIXFMT, (uint8_t *)"\x66", 1); // Read: only 24bit pixel mode
  LCD_IO_ReadCmd8MultipleData24to16(ILI9341_RAMRD, (uint16_t *)pData, Xsize * Ysize, 1);
  LCD_IO_WriteCmd8MultipleData8(ILI9341_PIXFMT, (uint8_t *)"\x55", 1); // Return to 16bit pixel mode

}
