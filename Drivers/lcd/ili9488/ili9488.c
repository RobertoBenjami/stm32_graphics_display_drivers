#include "main.h"
#include "lcd.h"
#include "ili9488.h"

// Lcd
void     ili9488_Init(void);
uint16_t ili9488_ReadID(void);
void     ili9488_DisplayOn(void);
void     ili9488_DisplayOff(void);
void     ili9488_SetCursor(uint16_t Xpos, uint16_t Ypos);
void     ili9488_WritePixel(uint16_t Xpos, uint16_t Ypos, uint16_t RGB_Code);
uint16_t ili9488_ReadPixel(uint16_t Xpos, uint16_t Ypos);
void     ili9488_SetDisplayWindow(uint16_t Xpos, uint16_t Ypos, uint16_t Width, uint16_t Height);
void     ili9488_DrawHLine(uint16_t RGBCode, uint16_t Xpos, uint16_t Ypos, uint16_t Length);
void     ili9488_DrawVLine(uint16_t RGBCode, uint16_t Xpos, uint16_t Ypos, uint16_t Length);
uint16_t ili9488_GetLcdPixelWidth(void);
uint16_t ili9488_GetLcdPixelHeight(void);
void     ili9488_DrawBitmap(uint16_t Xpos, uint16_t Ypos, uint8_t *pbmp);
void     ili9488_DrawRGBImage(uint16_t Xpos, uint16_t Ypos, uint16_t Xsize, uint16_t Ysize, uint16_t *pdata);
void     ili9488_ReadRGBImage(uint16_t Xpos, uint16_t Ypos, uint16_t Xsize, uint16_t Ysize, uint16_t *pdata);
void     ili9488_FillRect(uint16_t Xpos, uint16_t Ypos, uint16_t Xsize, uint16_t Ysize, uint16_t RGBCode);
void     ili9488_Scroll(int16_t Scroll, uint16_t TopFix, uint16_t BottonFix); 

// Touchscreen
void     ili9488_ts_Init(uint16_t DeviceAddr);
uint8_t  ili9488_ts_DetectTouch(uint16_t DeviceAddr);
void     ili9488_ts_GetXY(uint16_t DeviceAddr, uint16_t *X, uint16_t *Y);

LCD_DrvTypeDef   ili9488_drv =
{
  ili9488_Init,
  ili9488_ReadID,
  ili9488_DisplayOn,
  ili9488_DisplayOff,
  ili9488_SetCursor,
  ili9488_WritePixel,
  ili9488_ReadPixel,
  ili9488_SetDisplayWindow,
  ili9488_DrawHLine,
  ili9488_DrawVLine,
  ili9488_GetLcdPixelWidth,
  ili9488_GetLcdPixelHeight,
  ili9488_DrawBitmap,
  ili9488_DrawRGBImage,
  ili9488_FillRect,
  ili9488_ReadRGBImage,
  ili9488_Scroll,
};

LCD_DrvTypeDef  *lcd_drv = &ili9488_drv;

#define ILI9488_NOP           0x00
#define ILI9488_SWRESET       0x01
#define ILI9488_RDDID         0x04
#define ILI9488_RDDST         0x09

#define ILI9488_SLPIN         0x10
#define ILI9488_SLPOUT        0x11
#define ILI9488_PTLON         0x12
#define ILI9488_NORON         0x13

#define ILI9488_RDMODE        0x0A
#define ILI9488_RDMADCTL      0x0B
#define ILI9488_RDPIXFMT      0x0C
#define ILI9488_RDIMGFMT      0x0D
#define ILI9488_RDSELFDIAG    0x0F

#define ILI9488_INVOFF        0x20
#define ILI9488_INVON         0x21
#define ILI9488_GAMMASET      0x26
#define ILI9488_DISPOFF       0x28
#define ILI9488_DISPON        0x29

#define ILI9488_CASET         0x2A
#define ILI9488_PASET         0x2B
#define ILI9488_RAMWR         0x2C
#define ILI9488_RAMRD         0x2E

#define ILI9488_PTLAR         0x30
#define ILI9488_VSCRDEF       0x33
#define ILI9488_MADCTL        0x36
#define ILI9488_VSCRSADD      0x37
#define ILI9488_PIXFMT        0x3A
#define ILI9488_RAMWRCONT     0x3C
#define ILI9488_RAMRDCONT     0x3E

#define ILI9488_IMCTR         0xB0
#define ILI9488_FRMCTR1       0xB1
#define ILI9488_FRMCTR2       0xB2
#define ILI9488_FRMCTR3       0xB3
#define ILI9488_INVCTR        0xB4
#define ILI9488_DFUNCTR       0xB6

#define ILI9488_PWCTR1        0xC0
#define ILI9488_PWCTR2        0xC1
#define ILI9488_PWCTR3        0xC2
#define ILI9488_PWCTR4        0xC3
#define ILI9488_PWCTR5        0xC4
#define ILI9488_VMCTR1        0xC5
#define ILI9488_VMCTR2        0xC7

#define ILI9488_RDID1         0xDA
#define ILI9488_RDID2         0xDB
#define ILI9488_RDID3         0xDC
#define ILI9488_RDID4         0xDD

#define ILI9488_GMCTRP1       0xE0
#define ILI9488_GMCTRN1       0xE1
#define ILI9488_IMGFUNCT      0xE9

#define ILI9488_ADJCTR3       0xF7

#define ILI9488_MAD_RGB       0x08
#define ILI9488_MAD_BGR       0x00

#define ILI9488_MAD_VERTICAL  0x20
#define ILI9488_MAD_X_LEFT    0x00
#define ILI9488_MAD_X_RIGHT   0x40
#define ILI9488_MAD_Y_UP      0x80
#define ILI9488_MAD_Y_DOWN    0x00

#if ILI9488_COLORMODE == 0
#define ILI9488_MAD_COLORMODE    ILI9488_MAD_RGB
#else
#define ILI9488_MAD_COLORMODE    ILI9488_MAD_BGR
#endif

#define LCD_ORIENTATION  ILI9488_ORIENTATION

/* the drawing directions of the 4 orientations */
#if ILI9488_INTERFACE == 0 /* SPI interface */
#define ILI9488_SETCURSOR(x, y)            {LCD_IO_WriteCmd8(ILI9488_CASET); LCD_IO_WriteData16_to_2x8(x); LCD_IO_WriteData16_to_2x8(x); LCD_IO_WriteCmd8(ILI9488_PASET); LCD_IO_WriteData16_to_2x8(y); LCD_IO_WriteData16_to_2x8(y);}
#if (LCD_ORIENTATION == 0)
#define ILI9488_MAX_X                      (ILI9488_LCD_PIXEL_WIDTH - 1)
#define ILI9488_MAX_Y                      (ILI9488_LCD_PIXEL_HEIGHT - 1)
#define ILI9488_MAD_DATA_RIGHT_THEN_UP     ILI9488_MAD_COLORMODE | ILI9488_MAD_X_RIGHT | ILI9488_MAD_Y_UP
#define ILI9488_MAD_DATA_RIGHT_THEN_DOWN   ILI9488_MAD_COLORMODE | ILI9488_MAD_X_RIGHT | ILI9488_MAD_Y_DOWN
#elif (LCD_ORIENTATION == 1)
#define ILI9488_MAX_X                      (ILI9488_LCD_PIXEL_HEIGHT - 1)
#define ILI9488_MAX_Y                      (ILI9488_LCD_PIXEL_WIDTH - 1)
#define ILI9488_MAD_DATA_RIGHT_THEN_UP     ILI9488_MAD_COLORMODE | ILI9488_MAD_X_RIGHT | ILI9488_MAD_Y_DOWN | ILI9488_MAD_VERTICAL
#define ILI9488_MAD_DATA_RIGHT_THEN_DOWN   ILI9488_MAD_COLORMODE | ILI9488_MAD_X_LEFT  | ILI9488_MAD_Y_DOWN | ILI9488_MAD_VERTICAL
#elif (LCD_ORIENTATION == 2)
#define ILI9488_MAX_X                      (ILI9488_LCD_PIXEL_WIDTH - 1)
#define ILI9488_MAX_Y                      (ILI9488_LCD_PIXEL_HEIGHT - 1)
#define ILI9488_MAD_DATA_RIGHT_THEN_UP     ILI9488_MAD_COLORMODE | ILI9488_MAD_X_LEFT  | ILI9488_MAD_Y_DOWN
#define ILI9488_MAD_DATA_RIGHT_THEN_DOWN   ILI9488_MAD_COLORMODE | ILI9488_MAD_X_LEFT  | ILI9488_MAD_Y_UP
#else
#define ILI9488_MAX_X                      (ILI9488_LCD_PIXEL_HEIGHT - 1)
#define ILI9488_MAX_Y                      (ILI9488_LCD_PIXEL_WIDTH - 1)
#define ILI9488_MAD_DATA_RIGHT_THEN_UP     ILI9488_MAD_COLORMODE | ILI9488_MAD_X_LEFT  | ILI9488_MAD_Y_UP   | ILI9488_MAD_VERTICAL
#define ILI9488_MAD_DATA_RIGHT_THEN_DOWN   ILI9488_MAD_COLORMODE | ILI9488_MAD_X_RIGHT | ILI9488_MAD_Y_UP   | ILI9488_MAD_VERTICAL
#endif
#elif ILI9488_INTERFACE == 1 /* paralell interface */
#if (LCD_ORIENTATION == 0)
#define ILI9488_MAX_X                      (ILI9488_LCD_PIXEL_WIDTH - 1)
#define ILI9488_MAX_Y                      (ILI9488_LCD_PIXEL_HEIGHT - 1)
#define ILI9488_MAD_DATA_RIGHT_THEN_UP     ILI9488_MAD_COLORMODE | ILI9488_MAD_X_RIGHT | ILI9488_MAD_Y_UP
#define ILI9488_MAD_DATA_RIGHT_THEN_DOWN   ILI9488_MAD_COLORMODE | ILI9488_MAD_X_RIGHT | ILI9488_MAD_Y_DOWN
#define ILI9488_SETCURSOR(x, y)            {LCD_IO_WriteCmd8(ILI9488_CASET); LCD_IO_WriteData16_to_2x8(ILI9488_MAX_X - x); LCD_IO_WriteData16_to_2x8(ILI9488_MAX_X - x); LCD_IO_WriteCmd8(ILI9488_PASET); LCD_IO_WriteData16_to_2x8(y); LCD_IO_WriteData16_to_2x8(y);}
#elif (LCD_ORIENTATION == 1)
#define ILI9488_MAX_X                      (ILI9488_LCD_PIXEL_HEIGHT - 1)
#define ILI9488_MAX_Y                      (ILI9488_LCD_PIXEL_WIDTH - 1)
#define ILI9488_MAD_DATA_RIGHT_THEN_UP     ILI9488_MAD_COLORMODE | ILI9488_MAD_X_RIGHT | ILI9488_MAD_Y_DOWN | ILI9488_MAD_VERTICAL
#define ILI9488_MAD_DATA_RIGHT_THEN_DOWN   ILI9488_MAD_COLORMODE | ILI9488_MAD_X_LEFT  | ILI9488_MAD_Y_DOWN | ILI9488_MAD_VERTICAL
#define ILI9488_SETCURSOR(x, y)            {LCD_IO_WriteCmd8(ILI9488_CASET); LCD_IO_WriteData16_to_2x8(x); LCD_IO_WriteData16_to_2x8(x); LCD_IO_WriteCmd8(ILI9488_PASET); LCD_IO_WriteData16_to_2x8(y); LCD_IO_WriteData16_to_2x8(y);}
#elif (LCD_ORIENTATION == 2)
#define ILI9488_MAX_X                      (ILI9488_LCD_PIXEL_WIDTH - 1)
#define ILI9488_MAX_Y                      (ILI9488_LCD_PIXEL_HEIGHT - 1)
#define ILI9488_MAD_DATA_RIGHT_THEN_UP     ILI9488_MAD_COLORMODE | ILI9488_MAD_X_LEFT  | ILI9488_MAD_Y_DOWN
#define ILI9488_MAD_DATA_RIGHT_THEN_DOWN   ILI9488_MAD_COLORMODE | ILI9488_MAD_X_LEFT  | ILI9488_MAD_Y_UP
#define ILI9488_SETCURSOR(x, y)            {LCD_IO_WriteCmd8(ILI9488_CASET); LCD_IO_WriteData16_to_2x8(x); LCD_IO_WriteData16_to_2x8(x); LCD_IO_WriteCmd8(ILI9488_PASET); LCD_IO_WriteData16_to_2x8(ILI9488_MAX_Y - y); LCD_IO_WriteData16_to_2x8(ILI9488_MAX_Y - y);}
#else
#define ILI9488_MAX_X                      (ILI9488_LCD_PIXEL_HEIGHT - 1)
#define ILI9488_MAX_Y                      (ILI9488_LCD_PIXEL_WIDTH - 1)
#define ILI9488_MAD_DATA_RIGHT_THEN_UP     ILI9488_MAD_COLORMODE | ILI9488_MAD_X_LEFT  | ILI9488_MAD_Y_UP   | ILI9488_MAD_VERTICAL
#define ILI9488_MAD_DATA_RIGHT_THEN_DOWN   ILI9488_MAD_COLORMODE | ILI9488_MAD_X_RIGHT | ILI9488_MAD_Y_UP   | ILI9488_MAD_VERTICAL
#define ILI9488_SETCURSOR(x, y)            {LCD_IO_WriteCmd8(ILI9488_CASET); LCD_IO_WriteData16_to_2x8(ILI9488_MAX_X - x); LCD_IO_WriteData16_to_2x8(ILI9488_MAX_X - x); LCD_IO_WriteCmd8(ILI9488_PASET); LCD_IO_WriteData16_to_2x8(ILI9488_MAX_Y - y); LCD_IO_WriteData16_to_2x8(ILI9488_MAX_Y - y);}
#endif
#endif

#define ILI9488_LCD_INITIALIZED    0x01
#define ILI9488_IO_INITIALIZED     0x02
static  uint8_t   Is_ili9488_Initialized = 0;

#if      ILI9488_MULTITASK_MUTEX == 1 && ILI9488_TOUCH == 1 && ILI9488_INTERFACE == 1
volatile uint8_t io_lcd_busy = 0;
volatile uint8_t io_ts_busy = 0;
#define  ILI9488_LCDMUTEX_PUSH()    while(io_ts_busy); io_lcd_busy++;
#define  ILI9488_LCDMUTEX_POP()     io_lcd_busy--
#else
#define  ILI9488_LCDMUTEX_PUSH()
#define  ILI9488_LCDMUTEX_POP()
#endif

#if ILI9488_INTERFACE == 0
static  uint16_t  yStart, yEnd;
#endif

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
void     LCD_IO_ReadCmd8MultipleData16(uint8_t Cmd, uint16_t *pData, uint32_t Size, uint32_t DummySize);
void     LCD_IO_ReadCmd8MultipleData24to16(uint8_t Cmd, uint16_t *pData, uint32_t Size, uint32_t DummySize);

#define  LCD_IO_WriteData16_to_2x8(dt)    {LCD_IO_WriteData8((dt) >> 8); LCD_IO_WriteData8(dt); }

//-----------------------------------------------------------------------------
void ili9488_Init(void)
{
  if((Is_ili9488_Initialized & ILI9488_LCD_INITIALIZED) == 0)
  {
    Is_ili9488_Initialized |= ILI9488_LCD_INITIALIZED;
    if((Is_ili9488_Initialized & ILI9488_IO_INITIALIZED) == 0)
      LCD_IO_Init();
    Is_ili9488_Initialized |= ILI9488_IO_INITIALIZED;
  }

  LCD_Delay(105);
  LCD_IO_WriteCmd8(ILI9488_SWRESET);
  LCD_Delay(5);
  // positive gamma control
  LCD_IO_WriteCmd8MultipleData8(ILI9488_GMCTRP1, (uint8_t *)"\x00\x03\x09\x08\x16\x0A\x3F\x78\x4C\x09\x0A\x08\x16\x1A\x0F", 15);
  // negative gamma control
  LCD_IO_WriteCmd8MultipleData8(ILI9488_GMCTRN1, (uint8_t *)"\x00\x16\x19\x03\x0F\x05\x32\x45\x46\x04\x0E\x0D\x35\x37\x0F", 15);
  // Power Control 1 (Vreg1out, Verg2out)
  LCD_IO_WriteCmd8MultipleData8(ILI9488_PWCTR1, (uint8_t *)"\x17\x15", 2);
  LCD_Delay(5);
  // Power Control 2 (VGH,VGL)
  LCD_IO_WriteCmd8(ILI9488_PWCTR2); LCD_IO_WriteData8(0x41);
  LCD_Delay(5);
  // Power Control 3 (Vcom)
  LCD_IO_WriteCmd8MultipleData8(ILI9488_VMCTR1, (uint8_t *)"\x00\x12\x80", 3);
  LCD_Delay(5);
  #if ILI9488_INTERFACE == 0
  LCD_IO_WriteCmd8(ILI9488_PIXFMT); LCD_IO_WriteData8(0x66); // Interface Pixel Format (24 bit)
  #if LCD_SPI_MODE != 2
  // LCD_IO_WriteCmd8(0xFB); LCD_IO_WriteData8(0x80);
  LCD_IO_WriteCmd8(ILI9488_IMCTR); LCD_IO_WriteData8(0x80); // Interface Mode Control (SDO NOT USE)
  #else
  LCD_IO_WriteCmd8(ILI9488_IMCTR); LCD_IO_WriteData8(0x00); // Interface Mode Control (SDO USE)
  #endif
  #elif ILI9488_INTERFACE == 1
  LCD_IO_WriteCmd8(ILI9488_PIXFMT); LCD_IO_WriteData8(0x55); // Interface Pixel Format (16 bit)
  #endif
  LCD_IO_WriteCmd8(ILI9488_FRMCTR1); LCD_IO_WriteData8(0xA0); // Frame rate (60Hz)
  LCD_IO_WriteCmd8(ILI9488_INVCTR); LCD_IO_WriteData8(0x02); // Display Inversion Control (2-dot)
  LCD_IO_WriteCmd8MultipleData8(ILI9488_DFUNCTR, (uint8_t *)"\x02\x02", 2); // Display Function Control RGB/MCU Interface Control
  LCD_IO_WriteCmd8(ILI9488_IMGFUNCT); LCD_IO_WriteData8(0x00); // Set Image Functio (Disable 24 bit data)
  LCD_IO_WriteCmd8MultipleData8(ILI9488_ADJCTR3, (uint8_t *)"\xA9\x51\x2C\x82", 4); // Adjust Control (D7 stream, loose)
  LCD_Delay(5);
  LCD_IO_WriteCmd8(ILI9488_SLPOUT);      // Exit Sleep
  LCD_Delay(120);
  LCD_IO_WriteCmd8(ILI9488_DISPON);      // Display on
  LCD_Delay(5);
  LCD_IO_WriteCmd8(ILI9488_MADCTL); LCD_IO_WriteData8(ILI9488_MAD_DATA_RIGHT_THEN_DOWN);
}

//-----------------------------------------------------------------------------
/**
  * @brief  Enables the Display.
  * @param  None
  * @retval None
  */
void ili9488_DisplayOn(void)
{
  ILI9488_LCDMUTEX_PUSH();
  LCD_IO_WriteCmd8(ILI9488_SLPOUT);      // Display on
  LCD_IO_Bl_OnOff(1);
  ILI9488_LCDMUTEX_POP();
}

//-----------------------------------------------------------------------------
/**
  * @brief  Disables the Display.
  * @param  None
  * @retval None
  */
void ili9488_DisplayOff(void)
{
  ILI9488_LCDMUTEX_PUSH();
  LCD_IO_WriteCmd8(ILI9488_SLPIN);       // Display off
  LCD_IO_Bl_OnOff(0);
  ILI9488_LCDMUTEX_POP();
}

//-----------------------------------------------------------------------------
/**
  * @brief  Get the LCD pixel Width.
  * @param  None
  * @retval The Lcd Pixel Width
  */
uint16_t ili9488_GetLcdPixelWidth(void)
{
  return ILI9488_MAX_X + 1;
}

//-----------------------------------------------------------------------------
/**
  * @brief  Get the LCD pixel Height.
  * @param  None
  * @retval The Lcd Pixel Height
  */
uint16_t ili9488_GetLcdPixelHeight(void)
{
  return ILI9488_MAX_Y + 1;
}

//-----------------------------------------------------------------------------
/**
  * @brief  Get the ILI9488 ID.
  * @param  None
  * @retval The ILI9488 ID
  */
uint16_t ili9488_ReadID(void)
{
  uint32_t id = 0;
  ILI9488_LCDMUTEX_PUSH();

  if(Is_ili9488_Initialized == 0)
  {
    ili9488_Init();
  }

  #if ILI9488_INTERFACE == 0
  LCD_IO_ReadCmd8MultipleData8(0x04, (uint8_t *)&id, 3, 0);
  id <<= 1;
  #elif ILI9488_INTERFACE == 1
  LCD_IO_ReadCmd8MultipleData8(0x04, (uint8_t *)&id, 3, 1);
  #endif
  // printf("ID:%08X\r\n", (unsigned int)id);

  ILI9488_LCDMUTEX_POP();

  if(id == 0x00668054)
    return 0x9488;
  return 0;
}

//-----------------------------------------------------------------------------
/**
  * @brief  Set Cursor position.
  * @param  Xpos: specifies the X position.
  * @param  Ypos: specifies the Y position.
  * @retval None
  */
void ili9488_SetCursor(uint16_t Xpos, uint16_t Ypos)
{
  ILI9488_LCDMUTEX_PUSH();
  ILI9488_SETCURSOR(Xpos, Ypos);
  ILI9488_LCDMUTEX_POP();
}

/* The SPI mode not capable the 16bpp mode -> convert to 24bpp */
#if ILI9488_INTERFACE == 0
extern inline void ili9488_write16to24(uint16_t RGBCode);
inline void ili9488_write16to24(uint16_t RGBCode)
{
  LCD_IO_WriteData8((RGBCode & 0xF800) >> 8);
  LCD_IO_WriteData8((RGBCode & 0x07E0) >> 3);
  LCD_IO_WriteData8((RGBCode & 0x001F) << 3);
}
#endif

//-----------------------------------------------------------------------------
/**
  * @brief  Write pixel.
  * @param  Xpos: specifies the X position.
  * @param  Ypos: specifies the Y position.
  * @param  RGBCode: the RGB pixel color
  * @retval None
  */
void ili9488_WritePixel(uint16_t Xpos, uint16_t Ypos, uint16_t RGBCode)
{
  ILI9488_LCDMUTEX_PUSH();
  ILI9488_SETCURSOR(Xpos, Ypos);
  #if ILI9488_INTERFACE == 0
  LCD_IO_WriteCmd8(ILI9488_RAMWR);
  ili9488_write16to24(RGBCode);
  #elif ILI9488_INTERFACE == 1
  LCD_IO_WriteCmd8(ILI9488_RAMWR); LCD_IO_WriteData16(RGBCode);
  #endif
  ILI9488_LCDMUTEX_POP();
}

//-----------------------------------------------------------------------------
/**
  * @brief  Read pixel.
  * @param  None
  * @retval the RGB pixel color
  */
uint16_t ili9488_ReadPixel(uint16_t Xpos, uint16_t Ypos)
{
  uint16_t ret;
  ILI9488_LCDMUTEX_PUSH();
  ILI9488_SETCURSOR(Xpos, Ypos);
  #if ILI9488_INTERFACE == 0
  LCD_IO_ReadCmd8MultipleData24to16(ILI9488_RAMRD, &ret, 1, 1);
  #elif ILI9488_INTERFACE == 1
  LCD_IO_ReadCmd8MultipleData16(ILI9488_RAMRD, &ret, 1, 1);
  #endif
  ILI9488_LCDMUTEX_POP();
  return ret;
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
void ili9488_SetDisplayWindow(uint16_t Xpos, uint16_t Ypos, uint16_t Width, uint16_t Height)
{
  ILI9488_LCDMUTEX_PUSH();

  #if ILI9488_INTERFACE == 0
  yStart = Ypos; yEnd = Ypos + Height - 1;
  LCD_IO_WriteCmd8(ILI9488_CASET); LCD_IO_WriteData16_to_2x8(Xpos); LCD_IO_WriteData16_to_2x8(Xpos + Width - 1);
  LCD_IO_WriteCmd8(ILI9488_PASET); LCD_IO_WriteData16_to_2x8(Ypos); LCD_IO_WriteData16_to_2x8(Ypos + Height - 1);
  #elif ILI9488_INTERFACE == 1
  #if (LCD_ORIENTATION == 0)
  LCD_IO_WriteCmd8(ILI9488_CASET); LCD_IO_WriteData16_to_2x8(ILI9488_LCD_PIXEL_WIDTH - Width - Xpos); LCD_IO_WriteData16_to_2x8(ILI9488_LCD_PIXEL_WIDTH - 1 - Xpos);
  LCD_IO_WriteCmd8(ILI9488_PASET); LCD_IO_WriteData16_to_2x8(Ypos); LCD_IO_WriteData16_to_2x8(Ypos + Height - 1);
  #elif (LCD_ORIENTATION == 1)
  LCD_IO_WriteCmd8(ILI9488_CASET); LCD_IO_WriteData16_to_2x8(Xpos); LCD_IO_WriteData16_to_2x8(Xpos + Width - 1);
  LCD_IO_WriteCmd8(ILI9488_PASET); LCD_IO_WriteData16_to_2x8(Ypos); LCD_IO_WriteData16_to_2x8(Ypos + Height - 1);
  #elif (LCD_ORIENTATION == 2)
  LCD_IO_WriteCmd8(ILI9488_CASET); LCD_IO_WriteData16_to_2x8(Xpos); LCD_IO_WriteData16_to_2x8(Xpos + Width - 1);
  LCD_IO_WriteCmd8(ILI9488_PASET); LCD_IO_WriteData16_to_2x8(ILI9488_LCD_PIXEL_HEIGHT - Height - Ypos); LCD_IO_WriteData16_to_2x8(ILI9488_LCD_PIXEL_HEIGHT - 1 - Ypos);
  #else
  LCD_IO_WriteCmd8(ILI9488_CASET); LCD_IO_WriteData16_to_2x8(ILI9488_LCD_PIXEL_HEIGHT - Width - Xpos); LCD_IO_WriteData16_to_2x8(ILI9488_LCD_PIXEL_HEIGHT - 1 - Xpos);
  LCD_IO_WriteCmd8(ILI9488_PASET); LCD_IO_WriteData16_to_2x8(ILI9488_LCD_PIXEL_WIDTH - Height - Ypos); LCD_IO_WriteData16_to_2x8(ILI9488_LCD_PIXEL_WIDTH - 1 - Ypos);
  #endif
  #endif

  ILI9488_LCDMUTEX_POP();
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
void ili9488_DrawHLine(uint16_t RGBCode, uint16_t Xpos, uint16_t Ypos, uint16_t Length)
{
  ili9488_FillRect(Xpos, Ypos, Length, 1, RGBCode);
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
void ili9488_DrawVLine(uint16_t RGBCode, uint16_t Xpos, uint16_t Ypos, uint16_t Length)
{
  ili9488_FillRect(Xpos, Ypos, 1, Length, RGBCode);
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
void ili9488_FillRect(uint16_t Xpos, uint16_t Ypos, uint16_t Xsize, uint16_t Ysize, uint16_t RGBCode)
{
  ILI9488_LCDMUTEX_PUSH();
  ili9488_SetDisplayWindow(Xpos, Ypos, Xsize, Ysize);
  #if ILI9488_INTERFACE == 0
  LCD_IO_WriteCmd8(ILI9488_RAMWR);
  uint32_t XYsize = Xsize * Ysize;
  while(XYsize--)
    ili9488_write16to24(RGBCode);
  #elif ILI9488_INTERFACE == 1
  LCD_IO_WriteCmd8DataFill16(ILI9488_RAMWR, RGBCode, Xsize * Ysize);
  #endif
  ILI9488_LCDMUTEX_POP();
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
void ili9488_DrawBitmap(uint16_t Xpos, uint16_t Ypos, uint8_t *pbmp)
{
  uint32_t index = 0, size = 0;
  /* Read bitmap size */
  Ypos += pbmp[22] + (pbmp[23] << 8) - 1;
  size = *(volatile uint16_t *) (pbmp + 2);
  size |= (*(volatile uint16_t *) (pbmp + 4)) << 16;
  /* Get bitmap data address offset */
  index = *(volatile uint16_t *) (pbmp + 10);
  index |= (*(volatile uint16_t *) (pbmp + 12)) << 16;
  size = (size - index)/2;
  pbmp += index;

  ILI9488_LCDMUTEX_PUSH();

  #if ILI9488_INTERFACE == 0
  LCD_IO_WriteCmd8(ILI9488_MADCTL); LCD_IO_WriteData8(ILI9488_MAD_DATA_RIGHT_THEN_UP);
  LCD_IO_WriteCmd8(ILI9488_PASET); LCD_IO_WriteData16_to_2x8(ILI9488_MAX_Y - yEnd); LCD_IO_WriteData16_to_2x8(ILI9488_MAX_Y - yStart);
  LCD_IO_WriteCmd8(ILI9488_RAMWR);
  while(size--)
  {
    ili9488_write16to24(*(uint16_t *)pbmp);
    pbmp+= 2;
  }
  LCD_IO_WriteCmd8(ILI9488_MADCTL); LCD_IO_WriteData8(ILI9488_MAD_DATA_RIGHT_THEN_DOWN);
  #elif ILI9488_INTERFACE == 1
  LCD_IO_WriteCmd8(ILI9488_MADCTL); LCD_IO_WriteData8(ILI9488_MAD_DATA_RIGHT_THEN_UP);
  LCD_IO_WriteCmd8MultipleData16(ILI9488_RAMWR, (uint16_t *)pbmp, size);
  LCD_IO_WriteCmd8(ILI9488_MADCTL); LCD_IO_WriteData8(ILI9488_MAD_DATA_RIGHT_THEN_DOWN);
  #endif

  ILI9488_LCDMUTEX_POP();
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
void ili9488_DrawRGBImage(uint16_t Xpos, uint16_t Ypos, uint16_t Xsize, uint16_t Ysize, uint16_t *pdata)
{
  uint32_t size;

  size = (Xsize * Ysize);

  ILI9488_LCDMUTEX_PUSH();
  ili9488_SetDisplayWindow(Xpos, Ypos, Xsize, Ysize);
  #if ILI9488_INTERFACE == 0
  LCD_IO_WriteCmd8(ILI9488_RAMWR);
  while(size--)
  {
    ili9488_write16to24(*pdata);
    pdata++;
  }
  #elif ILI9488_INTERFACE == 1
  LCD_IO_WriteCmd8MultipleData16(ILI9488_RAMWR, pdata, size);
  #endif
  ILI9488_LCDMUTEX_POP();
}

//-----------------------------------------------------------------------------
/**
  * @brief  Read 16bit/pixel vitmap from Lcd..
  * @param  pdata: picture address.
  * @param  Xpos:  Image X position in the LCD
  * @param  Ypos:  Image Y position in the LCD
  * @param  Xsize: Image X size in the LCD
  * @param  Ysize: Image Y size in the LCD
  * @retval None
  * @brief  Draw direction: right then down
  */
void ili9488_ReadRGBImage(uint16_t Xpos, uint16_t Ypos, uint16_t Xsize, uint16_t Ysize, uint16_t *pdata)
{
  uint32_t size = 0;
  size = (Xsize * Ysize);
  ILI9488_LCDMUTEX_PUSH();
  ili9488_SetDisplayWindow(Xpos, Ypos, Xsize, Ysize);
  #if ILI9488_INTERFACE == 0
  LCD_IO_ReadCmd8MultipleData24to16(ILI9488_RAMRD, pdata, size, 1);
  #elif ILI9488_INTERFACE == 1
  LCD_IO_ReadCmd8MultipleData16(ILI9488_RAMRD, pdata, size, 1);
  #endif
  ILI9488_LCDMUTEX_POP();
}

//-----------------------------------------------------------------------------
/**
  * @brief  Set display scroll parameters
  * @param  Scroll    : Scroll size [pixel]
  * @param  TopFix    : Top fix size [pixel]
  * @param  BottonFix : Botton fix size [pixel]
  * @retval None
  */
void ili9488_Scroll(int16_t Scroll, uint16_t TopFix, uint16_t BottonFix)
{
  static uint16_t scrparam[4] = {0, 0, 0, 0};
  ILI9488_LCDMUTEX_PUSH();
  #if (ILI9488_ORIENTATION == 0)
  if((TopFix != scrparam[1]) || (BottonFix != scrparam[3]))
  {
    scrparam[1] = TopFix;
    scrparam[3] = BottonFix;
    scrparam[2] = ILI9488_LCD_PIXEL_HEIGHT - TopFix - BottonFix;
    LCD_IO_WriteCmd8MultipleData16(ILI9488_VSCRDEF, &scrparam[1], 3);
  }
  Scroll = (0 - Scroll) % scrparam[2];
  if(Scroll < 0)
    Scroll = scrparam[2] + Scroll + scrparam[1];
  else
    Scroll = Scroll + scrparam[1];
  #elif (ILI9488_ORIENTATION == 1)
  if((TopFix != scrparam[1]) || (BottonFix != scrparam[3]))
  {
    scrparam[1] = TopFix;
    scrparam[3] = BottonFix;
    scrparam[2] = ILI9488_LCD_PIXEL_HEIGHT - TopFix - BottonFix;
    LCD_IO_WriteCmd8MultipleData16(ILI9488_VSCRDEF, &scrparam[1], 3);
  }
  Scroll = (0 - Scroll) % scrparam[2];
  if(Scroll < 0)
    Scroll = scrparam[2] + Scroll + scrparam[1];
  else
    Scroll = Scroll + scrparam[1];
  #elif (ILI9488_ORIENTATION == 2)
  if((TopFix != scrparam[3]) || (BottonFix != scrparam[1]))
  {
    scrparam[3] = TopFix;
    scrparam[1] = BottonFix;
    scrparam[2] = ILI9488_LCD_PIXEL_HEIGHT - TopFix - BottonFix;
    LCD_IO_WriteCmd8MultipleData16(ILI9488_VSCRDEF, &scrparam[1], 3);
  }
  Scroll %= scrparam[2];
  if(Scroll < 0)
    Scroll = scrparam[2] + Scroll + scrparam[1];
  else
    Scroll = Scroll + scrparam[1];
  #elif (ILI9488_ORIENTATION == 3)
  if((TopFix != scrparam[3]) || (BottonFix != scrparam[1]))
  {
    scrparam[3] = TopFix;
    scrparam[1] = BottonFix;
    scrparam[2] = ILI9488_LCD_PIXEL_HEIGHT - TopFix - BottonFix;
    LCD_IO_WriteCmd8MultipleData16(ILI9488_VSCRDEF, &scrparam[1], 3);
  }
  Scroll %= scrparam[2];
  if(Scroll < 0)
    Scroll = scrparam[2] + Scroll + scrparam[1];
  else
    Scroll = Scroll + scrparam[1];
  #endif
  if(Scroll != scrparam[0])
  {
    scrparam[0] = Scroll;
    LCD_IO_WriteCmd8DataFill16(ILI9488_VSCRSADD, scrparam[0], 1);
  }
  ILI9488_LCDMUTEX_POP();
}

//=============================================================================
#if ILI9488_TOUCH == 1

#include "ts.h"

#define TS_MULTITASK_MUTEX    ILI9488_MULTITASK_MUTEX
#define TOUCH_FILTER          16
#define TOUCH_MAXREPEAT       8

#define ABS(N)   (((N)<0) ? (-(N)) : (N))

TS_DrvTypeDef   ili9488_ts_drv =
{
  ili9488_ts_Init,
  0,
  0,
  0,
  ili9488_ts_DetectTouch,
  ili9488_ts_GetXY,
  0,
  0,
  0,
  0,
};

TS_DrvTypeDef  *ts_drv = &ili9488_ts_drv;

#if (LCD_ORIENTATION == 0)
int32_t  ts_cindex[] = TS_CINDEX_0;
#elif (LCD_ORIENTATION == 1)
int32_t  ts_cindex[] = TS_CINDEX_1;
#elif (LCD_ORIENTATION == 2)
int32_t  ts_cindex[] = TS_CINDEX_2;
#elif (LCD_ORIENTATION == 3)
int32_t  ts_cindex[] = TS_CINDEX_3;
#endif

uint16_t tx, ty;

/* Link function for Touchscreen */
uint8_t   TS_IO_DetectToch(void);
uint16_t  TS_IO_GetX(void);
uint16_t  TS_IO_GetY(void);
uint16_t  TS_IO_GetZ1(void);
uint16_t  TS_IO_GetZ2(void);

//-----------------------------------------------------------------------------
void ili9488_ts_Init(uint16_t DeviceAddr)
{
  if((Is_ili9488_Initialized & ILI9488_IO_INITIALIZED) == 0)
    LCD_IO_Init();
  Is_ili9488_Initialized |= ILI9488_IO_INITIALIZED;
}

//-----------------------------------------------------------------------------
uint8_t ili9488_ts_DetectTouch(uint16_t DeviceAddr)
{
  static uint8_t ret = 0;
  int32_t x1, x2, y1, y2, i;

  #if TS_MULTITASK_MUTEX == 1
  io_ts_busy = 1;

  if(io_lcd_busy)
  {
    io_ts_busy = 0;
    return ret;
  }
  #endif

  ret = 0;
  if(TS_IO_DetectToch())
  {
    x1 = TS_IO_GetX();
    y1 = TS_IO_GetY();
    i = TOUCH_MAXREPEAT;
    while(i--)
    {
      x2 = TS_IO_GetX();
      y2 = TS_IO_GetY();
      if((ABS(x1 - x2) < TOUCH_FILTER) && (ABS(y1 - y2) < TOUCH_FILTER))
      {
        x1 = (x1 + x2) >> 1;
        y1 = (y1 + y2) >> 1;
        i = 0;
        if(TS_IO_DetectToch())
        {
          tx = x1;
          ty = y1;
          ret = 1;
        }
      }
      else
      {
        x1 = x2;
        y1 = y2;
      }
    }
  }

  #if TS_MULTITASK_MUTEX == 1
  io_ts_busy = 0;
  #endif

  return ret;
}

//-----------------------------------------------------------------------------
void ili9488_ts_GetXY(uint16_t DeviceAddr, uint16_t *X, uint16_t *Y)
{
  *X = tx,
  *Y = ty;
}
#endif // #if ILI9488_TOUCH == 1
