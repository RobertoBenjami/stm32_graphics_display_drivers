/*
 * ILI9486 LCD driver (optional builtin touchscreen driver)
 * 2020.01
*/

#include <string.h>
#include "main.h"
#include "lcd.h"
#include "bmp.h"
#include "ili9486.h"

#if  ILI9486_TOUCH == 1
#include "ts.h"
#endif

/* Lcd */
void     ili9486_Init(void);
uint16_t ili9486_ReadID(void);
void     ili9486_DisplayOn(void);
void     ili9486_DisplayOff(void);
void     ili9486_SetCursor(uint16_t Xpos, uint16_t Ypos);
void     ili9486_WritePixel(uint16_t Xpos, uint16_t Ypos, uint16_t RGB_Code);
uint16_t ili9486_ReadPixel(uint16_t Xpos, uint16_t Ypos);
void     ili9486_SetDisplayWindow(uint16_t Xpos, uint16_t Ypos, uint16_t Width, uint16_t Height);
void     ili9486_DrawHLine(uint16_t RGBCode, uint16_t Xpos, uint16_t Ypos, uint16_t Length);
void     ili9486_DrawVLine(uint16_t RGBCode, uint16_t Xpos, uint16_t Ypos, uint16_t Length);
void     ili9486_FillRect(uint16_t Xpos, uint16_t Ypos, uint16_t Xsize, uint16_t Ysize, uint16_t RGBCode);
uint16_t ili9486_GetLcdPixelWidth(void);
uint16_t ili9486_GetLcdPixelHeight(void);
void     ili9486_DrawBitmap(uint16_t Xpos, uint16_t Ypos, uint8_t *pbmp);
void     ili9486_DrawRGBImage(uint16_t Xpos, uint16_t Ypos, uint16_t Xsize, uint16_t Ysize, uint8_t *pData);
void     ili9486_ReadRGBImage(uint16_t Xpos, uint16_t Ypos, uint16_t Xsize, uint16_t Ysize, uint8_t *pData);

/* Touchscreen */
void     ili9486_ts_Init(uint16_t DeviceAddr);
uint8_t  ili9486_ts_DetectTouch(uint16_t DeviceAddr);
void     ili9486_ts_GetXY(uint16_t DeviceAddr, uint16_t *X, uint16_t *Y);

LCD_DrvTypeDef   ili9486_drv =
{
  ili9486_Init,
  ili9486_ReadID,
  ili9486_DisplayOn,
  ili9486_DisplayOff,
  ili9486_SetCursor,
  ili9486_WritePixel,
  ili9486_ReadPixel,
  ili9486_SetDisplayWindow,
  ili9486_DrawHLine,
  ili9486_DrawVLine,
  ili9486_GetLcdPixelWidth,
  ili9486_GetLcdPixelHeight,
  ili9486_DrawBitmap,
  ili9486_DrawRGBImage,
  #ifdef   LCD_DRVTYPE_V1_1
  ili9486_FillRect,
  ili9486_ReadRGBImage,
  #endif
};

LCD_DrvTypeDef  *lcd_drv = &ili9486_drv;

#define ILI9486_NOP            0x00
#define ILI9486_SWRESET        0x01

#define ILI9486_RDDID          0x04
#define ILI9486_RDDST          0x09
#define ILI9486_RDMODE         0x0A
#define ILI9486_RDMADCTL       0x0B
#define ILI9486_RDPIXFMT       0x0C
#define ILI9486_RDIMGFMT       0x0D
#define ILI9486_RDSELFDIAG     0x0F

#define ILI9486_SLPIN          0x10
#define ILI9486_SLPOUT         0x11
#define ILI9486_PTLON          0x12
#define ILI9486_NORON          0x13

#define ILI9486_INVOFF         0x20
#define ILI9486_INVON          0x21
#define ILI9486_GAMMASET       0x26
#define ILI9486_DISPOFF        0x28
#define ILI9486_DISPON         0x29

#define ILI9486_CASET          0x2A
#define ILI9486_PASET          0x2B
#define ILI9486_RAMWR          0x2C
#define ILI9486_RAMRD          0x2E

#define ILI9486_PTLAR          0x30
#define ILI9486_MADCTL         0x36
#define ILI9486_VSCRSADD       0x37     /* Vertical Scrolling Start Address */
#define ILI9486_PIXFMT         0x3A     /* COLMOD: Pixel Format Set */

#define ILI9486_RGB_INTERFACE  0xB0     /* RGB Interface Signal Control */
#define ILI9486_FRMCTR1        0xB1
#define ILI9486_FRMCTR2        0xB2
#define ILI9486_FRMCTR3        0xB3
#define ILI9486_INVCTR         0xB4
#define ILI9486_DFUNCTR        0xB6     /* Display Function Control */

#define ILI9486_PWCTR1         0xC0
#define ILI9486_PWCTR2         0xC1
#define ILI9486_PWCTR3         0xC2
#define ILI9486_PWCTR4         0xC3
#define ILI9486_PWCTR5         0xC4
#define ILI9486_VMCTR1         0xC5
#define ILI9486_VMCTR2         0xC7

#define ILI9486_RDID1          0xDA
#define ILI9486_RDID2          0xDB
#define ILI9486_RDID3          0xDC
#define ILI9486_RDID4          0xDD

#define ILI9486_GMCTRP1        0xE0
#define ILI9486_GMCTRN1        0xE1
#define ILI9486_DGCTR1         0xE2
#define ILI9486_DGCTR2         0xE3

//-----------------------------------------------------------------------------
#define ILI9486_MAD_RGB        0x08
#define ILI9486_MAD_BGR        0x00

#define ILI9486_MAD_VERTICAL   0x20
#define ILI9486_MAD_X_LEFT     0x00
#define ILI9486_MAD_X_RIGHT    0x40
#define ILI9486_MAD_Y_UP       0x80
#define ILI9486_MAD_Y_DOWN     0x00

#if ILI9486_COLORMODE == 0
#define ILI9486_MAD_COLORMODE  ILI9486_MAD_RGB
#else
#define ILI9486_MAD_COLORMODE  ILI9486_MAD_BGR
#endif

#if (ILI9486_ORIENTATION == 0)
#define ILI9486_SIZE_X                     ILI9486_LCD_PIXEL_WIDTH
#define ILI9486_SIZE_Y                     ILI9486_LCD_PIXEL_HEIGHT
#define ILI9486_MAD_DATA_RIGHT_THEN_UP     ILI9486_MAD_COLORMODE | ILI9486_MAD_X_RIGHT | ILI9486_MAD_Y_UP
#define ILI9486_MAD_DATA_RIGHT_THEN_DOWN   ILI9486_MAD_COLORMODE | ILI9486_MAD_X_RIGHT | ILI9486_MAD_Y_DOWN
#define ILI9486_MAD_DATA_RGBMODE           ILI9486_MAD_COLORMODE | ILI9486_MAD_X_LEFT  | ILI9486_MAD_Y_DOWN
#elif (ILI9486_ORIENTATION == 1)
#define ILI9486_SIZE_X                     ILI9486_LCD_PIXEL_HEIGHT
#define ILI9486_SIZE_Y                     ILI9486_LCD_PIXEL_WIDTH
#define ILI9486_MAD_DATA_RIGHT_THEN_UP     ILI9486_MAD_COLORMODE | ILI9486_MAD_X_RIGHT | ILI9486_MAD_Y_DOWN | ILI9486_MAD_VERTICAL
#define ILI9486_MAD_DATA_RIGHT_THEN_DOWN   ILI9486_MAD_COLORMODE | ILI9486_MAD_X_LEFT  | ILI9486_MAD_Y_DOWN | ILI9486_MAD_VERTICAL
#define ILI9486_MAD_DATA_RGBMODE           ILI9486_MAD_COLORMODE | ILI9486_MAD_X_RIGHT | ILI9486_MAD_Y_DOWN
#elif (ILI9486_ORIENTATION == 2)
#define ILI9486_SIZE_X                     ILI9486_LCD_PIXEL_WIDTH
#define ILI9486_SIZE_Y                     ILI9486_LCD_PIXEL_HEIGHT
#define ILI9486_MAD_DATA_RIGHT_THEN_UP     ILI9486_MAD_COLORMODE | ILI9486_MAD_X_LEFT  | ILI9486_MAD_Y_DOWN
#define ILI9486_MAD_DATA_RIGHT_THEN_DOWN   ILI9486_MAD_COLORMODE | ILI9486_MAD_X_LEFT  | ILI9486_MAD_Y_UP
#define ILI9486_MAD_DATA_RGBMODE           ILI9486_MAD_COLORMODE | ILI9486_MAD_X_RIGHT | ILI9486_MAD_Y_UP
#elif (ILI9486_ORIENTATION == 3)
#define ILI9486_SIZE_X                     ILI9486_LCD_PIXEL_HEIGHT
#define ILI9486_SIZE_Y                     ILI9486_LCD_PIXEL_WIDTH
#define ILI9486_MAD_DATA_RIGHT_THEN_UP     ILI9486_MAD_COLORMODE | ILI9486_MAD_X_LEFT  | ILI9486_MAD_Y_UP   | ILI9486_MAD_VERTICAL
#define ILI9486_MAD_DATA_RIGHT_THEN_DOWN   ILI9486_MAD_COLORMODE | ILI9486_MAD_X_RIGHT | ILI9486_MAD_Y_UP   | ILI9486_MAD_VERTICAL
#define ILI9486_MAD_DATA_RGBMODE           ILI9486_MAD_COLORMODE | ILI9486_MAD_X_LEFT  | ILI9486_MAD_Y_UP
#endif

#define ILI9486_SETCURSOR(x, y)            {LCD_IO_WriteCmd8(ILI9486_CASET); LCD_IO_WriteData16_to_2x8(x); LCD_IO_WriteData16_to_2x8(x); \
                                            LCD_IO_WriteCmd8(ILI9486_PASET); LCD_IO_WriteData16_to_2x8(y); LCD_IO_WriteData16_to_2x8(y); }

//-----------------------------------------------------------------------------
#define ILI9486_LCD_INITIALIZED    0x01
#define ILI9486_IO_INITIALIZED     0x02
static  uint8_t   Is_ili9486_Initialized = 0;

#if      ILI9486_MULTITASK_MUTEX == 1 && ILI9486_TOUCH == 1
volatile uint8_t io_lcd_busy = 0;
volatile uint8_t io_ts_busy = 0;
#define  ILI9486_LCDMUTEX_PUSH()    while(io_ts_busy); io_lcd_busy++;
#define  ILI9486_LCDMUTEX_POP()     io_lcd_busy--
#else
#define  ILI9486_LCDMUTEX_PUSH()
#define  ILI9486_LCDMUTEX_POP()
#endif

//-----------------------------------------------------------------------------
#if ILI9486_TOUCH == 1

// Touch parameters
#define TOUCHMINPRESSRC    8192
#define TOUCHMAXPRESSRC    4096
#define TOUCHMINPRESTRG       0
#define TOUCHMAXPRESTRG     255
#define TOUCH_FILTER         16

// fixpoints Z indexs (16bit integer, 16bit fractional)
#define ZINDEXA  ((65536 * (TOUCHMAXPRESTRG - TOUCHMINPRESTRG)) / (TOUCHMAXPRESSRC - TOUCHMINPRESSRC))
#define ZINDEXB  (-ZINDEXA * TOUCHMINPRESSRC)

#define ABS(N)   (((N)<0) ? (-(N)) : (N))

TS_DrvTypeDef   ili9486_ts_drv =
{
  ili9486_ts_Init,
  0,
  0,
  0,
  ili9486_ts_DetectTouch,
  ili9486_ts_GetXY,
  0,
  0,
  0,
  0,
};

TS_DrvTypeDef  *ts_drv = &ili9486_ts_drv;

#if (ILI9486_ORIENTATION == 0)
int32_t  ts_cindex[] = TS_CINDEX_0;
#elif (ILI9486_ORIENTATION == 1)
int32_t  ts_cindex[] = TS_CINDEX_1;
#elif (ILI9486_ORIENTATION == 2)
int32_t  ts_cindex[] = TS_CINDEX_2;
#elif (ILI9486_ORIENTATION == 3)
int32_t  ts_cindex[] = TS_CINDEX_3;
#endif

uint16_t tx, ty;

/* Link function for Touchscreen */
uint8_t   TS_IO_DetectToch(void);
uint16_t  TS_IO_GetX(void);
uint16_t  TS_IO_GetY(void);
uint16_t  TS_IO_GetZ1(void);
uint16_t  TS_IO_GetZ2(void);

#endif   /* #if ILI9486_TOUCH == 1 */

//-----------------------------------------------------------------------------
/* Link functions for LCD IO peripheral */
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

static  uint16_t  yStart, yEnd;

//-----------------------------------------------------------------------------
/**
  * @brief  Enables the Display.
  * @param  None
  * @retval None
  */
void ili9486_DisplayOn(void)
{
  ILI9486_LCDMUTEX_PUSH();
  LCD_IO_WriteCmd8(ILI9486_SLPOUT);    // Exit Sleep
  ILI9486_LCDMUTEX_POP();
  LCD_IO_Bl_OnOff(1);
}

//-----------------------------------------------------------------------------
/**
  * @brief  Disables the Display.
  * @param  None
  * @retval None
  */
void ili9486_DisplayOff(void)
{
  LCD_IO_Bl_OnOff(0);
  ILI9486_LCDMUTEX_PUSH();
  LCD_IO_WriteCmd8(ILI9486_SLPIN);    // Sleep
  ILI9486_LCDMUTEX_POP();
}

//-----------------------------------------------------------------------------
/**
  * @brief  Get the LCD pixel Width.
  * @param  None
  * @retval The Lcd Pixel Width
  */
uint16_t ili9486_GetLcdPixelWidth(void)
{
  return ILI9486_SIZE_X;
}

//-----------------------------------------------------------------------------
/**
  * @brief  Get the LCD pixel Height.
  * @param  None
  * @retval The Lcd Pixel Height
  */
uint16_t ili9486_GetLcdPixelHeight(void)
{
  return ILI9486_SIZE_Y;
}

//-----------------------------------------------------------------------------
/**
  * @brief  Get the ILI9486 ID.
  * @param  None
  * @retval The ILI9486 ID
  */
uint16_t ili9486_ReadID(void)
{
  uint32_t id = 0;
  ILI9486_LCDMUTEX_PUSH();
  LCD_IO_ReadCmd8MultipleData8(0xD3, (uint8_t *)&id, 3, 1);
  ILI9486_LCDMUTEX_POP();
  if(id == 0x869400)
    return 0x9486;
  else
    return 0;
}

//-----------------------------------------------------------------------------
/**
  * @brief  ILI9486 initialization
  * @param  None
  * @retval None
  */
void ili9486_Init(void)
{
  if((Is_ili9486_Initialized & ILI9486_LCD_INITIALIZED) == 0)
  {
    Is_ili9486_Initialized |= ILI9486_LCD_INITIALIZED;
    if((Is_ili9486_Initialized & ILI9486_IO_INITIALIZED) == 0)
      LCD_IO_Init();
    Is_ili9486_Initialized |= ILI9486_IO_INITIALIZED;
  }
  LCD_Delay(10);
  LCD_IO_WriteCmd8(ILI9486_SWRESET);
  LCD_Delay(100);

  LCD_IO_WriteCmd8MultipleData8(ILI9486_RGB_INTERFACE, (uint8_t *)"\x00", 1); // RGB mode off (0xB0)
  LCD_IO_WriteCmd8(ILI9486_SLPOUT);    // Exit Sleep (0x11)
  LCD_Delay(10);

  LCD_IO_WriteCmd8MultipleData8(ILI9486_PIXFMT, (uint8_t *)"\x55", 1); // interface format (0x3A)

  LCD_IO_WriteCmd8(ILI9486_MADCTL); LCD_IO_WriteData8(ILI9486_MAD_DATA_RIGHT_THEN_DOWN);

  LCD_IO_WriteCmd8MultipleData8(ILI9486_PWCTR3, (uint8_t *)"\x44", 1); // 0xC2
  LCD_IO_WriteCmd8MultipleData8(ILI9486_VMCTR1, (uint8_t *)"\x00\x00\x00\x00", 4); // 0xC5

  // positive gamma control (0xE0)
  LCD_IO_WriteCmd8MultipleData8(ILI9486_GMCTRP1, (uint8_t *)"\x0F\x1F\x1C\x0C\x0F\x08\x48\x98\x37\x0A\x13\x04\x11\x0D\x00", 15);

  // negative gamma control (0xE1)
  LCD_IO_WriteCmd8MultipleData8(ILI9486_GMCTRN1, (uint8_t *)"\x0F\x32\x2E\x0B\x0D\x05\x47\x75\x37\x06\x10\x03\x24\x20\x00", 15);

  // Digital gamma control1 (0xE2)
  LCD_IO_WriteCmd8MultipleData8(ILI9486_DGCTR1, (uint8_t *)"\x0F\x32\x2E\x0B\x0D\x05\x47\x75\x37\x06\x10\x03\x24\x20\x00", 15);

  LCD_IO_WriteCmd8(ILI9486_NORON);     // Normal display on (0x13)
  LCD_IO_WriteCmd8(ILI9486_INVOFF);    // Display inversion off (0x20)
  LCD_IO_WriteCmd8(ILI9486_SLPOUT);    // Exit Sleep (0x11)
  LCD_Delay(200);
  LCD_IO_WriteCmd8(ILI9486_DISPON);    // Display on (0x29)
  LCD_Delay(10);
}

//-----------------------------------------------------------------------------
/**
  * @brief  Set Cursor position.
  * @param  Xpos: specifies the X position.
  * @param  Ypos: specifies the Y position.
  * @retval None
  */
void ili9486_SetCursor(uint16_t Xpos, uint16_t Ypos)
{
  ILI9486_LCDMUTEX_PUSH();
  ILI9486_SETCURSOR(Xpos, Ypos);
  ILI9486_LCDMUTEX_POP();
}

//-----------------------------------------------------------------------------
/**
  * @brief  Write pixel.
  * @param  Xpos: specifies the X position.
  * @param  Ypos: specifies the Y position.
  * @param  RGBCode: the RGB pixel color
  * @retval None
  */
void ili9486_WritePixel(uint16_t Xpos, uint16_t Ypos, uint16_t RGBCode)
{
  ILI9486_LCDMUTEX_PUSH();
  ILI9486_SETCURSOR(Xpos, Ypos);
  LCD_IO_WriteCmd8(ILI9486_RAMWR); LCD_IO_WriteData16(RGBCode);
  ILI9486_LCDMUTEX_POP();
}


//-----------------------------------------------------------------------------
/**
  * @brief  Read pixel.
  * @param  None
  * @retval the RGB pixel color
  */
uint16_t ili9486_ReadPixel(uint16_t Xpos, uint16_t Ypos)
{
  uint16_t ret;
  ILI9486_LCDMUTEX_PUSH();
  LCD_IO_WriteCmd8MultipleData8(ILI9486_PIXFMT, (uint8_t *)"\x66", 1); // Read: only 24bit pixel mode
  ILI9486_SETCURSOR(Xpos, Ypos);
  LCD_IO_ReadCmd8MultipleData24to16(ILI9486_RAMRD, (uint16_t *)&ret, 1, 1);
  LCD_IO_WriteCmd8MultipleData8(ILI9486_PIXFMT, (uint8_t *)"\x55", 1); // Return to 16bit pixel mode
  ILI9486_LCDMUTEX_POP();
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
void ili9486_SetDisplayWindow(uint16_t Xpos, uint16_t Ypos, uint16_t Width, uint16_t Height)
{
  yStart = Ypos; yEnd = Ypos + Height - 1;
  ILI9486_LCDMUTEX_PUSH();
  LCD_IO_WriteCmd8(ILI9486_CASET); LCD_IO_WriteData16_to_2x8(Xpos); LCD_IO_WriteData16_to_2x8(Xpos + Width - 1);
  LCD_IO_WriteCmd8(ILI9486_PASET); LCD_IO_WriteData16_to_2x8(Ypos); LCD_IO_WriteData16_to_2x8(Ypos + Height - 1);
  ILI9486_LCDMUTEX_POP();
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
void ili9486_DrawHLine(uint16_t RGBCode, uint16_t Xpos, uint16_t Ypos, uint16_t Length)
{
  ILI9486_LCDMUTEX_PUSH();
  LCD_IO_WriteCmd8(ILI9486_CASET); LCD_IO_WriteData16_to_2x8(Xpos); LCD_IO_WriteData16_to_2x8(Xpos + Length - 1);
  LCD_IO_WriteCmd8(ILI9486_PASET); LCD_IO_WriteData16_to_2x8(Ypos); LCD_IO_WriteData16_to_2x8(Ypos);
  LCD_IO_WriteCmd8DataFill16(ILI9486_RAMWR, RGBCode, Length);
  ILI9486_LCDMUTEX_POP();
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
void ili9486_DrawVLine(uint16_t RGBCode, uint16_t Xpos, uint16_t Ypos, uint16_t Length)
{
  ILI9486_LCDMUTEX_PUSH();
  LCD_IO_WriteCmd8(ILI9486_CASET); LCD_IO_WriteData16_to_2x8(Xpos); LCD_IO_WriteData16_to_2x8(Xpos);
  LCD_IO_WriteCmd8(ILI9486_PASET); LCD_IO_WriteData16_to_2x8(Ypos); LCD_IO_WriteData16_to_2x8(Ypos + Length - 1);
  LCD_IO_WriteCmd8DataFill16(ILI9486_RAMWR, RGBCode, Length);
  ILI9486_LCDMUTEX_POP();
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
void ili9486_FillRect(uint16_t Xpos, uint16_t Ypos, uint16_t Xsize, uint16_t Ysize, uint16_t RGBCode)
{
  ILI9486_LCDMUTEX_PUSH();
  LCD_IO_WriteCmd8(ILI9486_CASET); LCD_IO_WriteData16_to_2x8(Xpos); LCD_IO_WriteData16_to_2x8(Xpos + Xsize - 1);
  LCD_IO_WriteCmd8(ILI9486_PASET); LCD_IO_WriteData16_to_2x8(Ypos); LCD_IO_WriteData16_to_2x8(Ypos + Ysize - 1);
  LCD_IO_WriteCmd8DataFill16(ILI9486_RAMWR, RGBCode, Xsize * Ysize);
  ILI9486_LCDMUTEX_POP();
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
void ili9486_DrawBitmap(uint16_t Xpos, uint16_t Ypos, uint8_t *pbmp)
{
  uint32_t index, size;
  /* Read bitmap size */
  size = ((BITMAPSTRUCT *)pbmp)->fileHeader.bfSize;
  /* Get bitmap data address offset */
  index = ((BITMAPSTRUCT *)pbmp)->fileHeader.bfOffBits;
  size = (size - index) / 2;
  pbmp += index;

  ILI9486_LCDMUTEX_PUSH();
  LCD_IO_WriteCmd8(ILI9486_MADCTL); LCD_IO_WriteData8(ILI9486_MAD_DATA_RIGHT_THEN_UP);
  LCD_IO_WriteCmd8(ILI9486_PASET); LCD_IO_WriteData16_to_2x8(ILI9486_SIZE_Y - 1 - yEnd); LCD_IO_WriteData16_to_2x8(ILI9486_SIZE_Y - 1 - yStart);
  LCD_IO_WriteCmd8MultipleData16(ILI9486_RAMWR, (uint16_t *)pbmp, size);
  LCD_IO_WriteCmd8(ILI9486_MADCTL); LCD_IO_WriteData8(ILI9486_MAD_DATA_RIGHT_THEN_DOWN);
  ILI9486_LCDMUTEX_POP();
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
void ili9486_DrawRGBImage(uint16_t Xpos, uint16_t Ypos, uint16_t Xsize, uint16_t Ysize, uint8_t *pData)
{
  ili9486_SetDisplayWindow(Xpos, Ypos, Xsize, Ysize);
  ILI9486_LCDMUTEX_PUSH();
  LCD_IO_WriteCmd8MultipleData16(ILI9486_RAMWR, (uint16_t *)pData, Xsize * Ysize);
  ILI9486_LCDMUTEX_POP();
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
void ili9486_ReadRGBImage(uint16_t Xpos, uint16_t Ypos, uint16_t Xsize, uint16_t Ysize, uint8_t *pData)
{
  ili9486_SetDisplayWindow(Xpos, Ypos, Xsize, Ysize);
  ILI9486_LCDMUTEX_PUSH();
  LCD_IO_WriteCmd8MultipleData8(ILI9486_PIXFMT, (uint8_t *)"\x66", 1); // Read: only 24bit pixel mode
  LCD_IO_ReadCmd8MultipleData24to16(ILI9486_RAMRD, (uint16_t *)pData, Xsize * Ysize, 1);
  LCD_IO_WriteCmd8MultipleData8(ILI9486_PIXFMT, (uint8_t *)"\x55", 1); // Return to 16bit pixel mode
  ILI9486_LCDMUTEX_POP();
}

//=============================================================================
#if ILI9486_TOUCH == 1
void ili9486_ts_Init(uint16_t DeviceAddr)
{
  if((Is_ili9486_Initialized & ILI9486_IO_INITIALIZED) == 0)
    LCD_IO_Init();
  Is_ili9486_Initialized |= ILI9486_IO_INITIALIZED;
}

//-----------------------------------------------------------------------------
uint8_t ili9486_ts_DetectTouch(uint16_t DeviceAddr)
{
  static uint8_t tp = 0;
  int32_t x1, x2, y1, y2, z11, z12, z21, z22, i, tpr;

  #if  ILI9486_MULTITASK_MUTEX == 1
  io_ts_busy = 1;

  if(io_lcd_busy)
  {
    io_ts_busy = 0;
    return tp;
  }
  #endif

  if(TS_IO_DetectToch())
  {
    x1 = TS_IO_GetX();
    y1 = TS_IO_GetY();
    z11 = TS_IO_GetZ1();
    z21 = TS_IO_GetZ2();
    i = 32;
    while(i--)
    {
      x2 = TS_IO_GetX();
      y2 = TS_IO_GetY();
      z12 = TS_IO_GetZ1();
      z22 = TS_IO_GetZ2();

      if((ABS(x1 - x2) < TOUCH_FILTER) && (ABS(y1 - y2) < TOUCH_FILTER) && (ABS(z11 - z12) < TOUCH_FILTER) && (ABS(z21 - z22) < TOUCH_FILTER))
      {
        x1 = (x1 + x2) >> 1;
        y1 = (x1 + y2) >> 1;
        z11 = (z11 + z12) >> 1;
        z21 = (z21 + z22) >> 1;

        tpr = (((4096 - x1) * ((z21 << 10) / z11 - 1024)) >> 10);
        tpr = (tpr * ZINDEXA + ZINDEXB) >> 16;
        if(tpr > TOUCHMAXPRESTRG)
          tpr = TOUCHMAXPRESTRG;
        if(tpr < TOUCHMINPRESTRG)
          tpr = TOUCHMINPRESTRG;
        tx = x1;
        ty = y1;
        tp = tpr;
        #if  ILI9486_MULTITASK_MUTEX == 1
        io_ts_busy = 0;
        #endif
        return tp;
      }
      else
      {
        x1 = x2;
        y1 = y2;
        z11 = z12;
        z21 = z22;
      }
    }
    // sokadik probára sem sikerült stabil koordinátát kiolvasni -> nincs lenyomva
    tp = 0;
  }
  else
    tp = 0;

  #if  ILI9486_MULTITASK_MUTEX == 1
  io_ts_busy = 0;
  #endif

  return tp;
}

//-----------------------------------------------------------------------------
void ili9486_ts_GetXY(uint16_t DeviceAddr, uint16_t *X, uint16_t *Y)
{
  *X = tx,
  *Y = ty;
}
#endif /* #if ILI9486_TOUCH == 1 */
