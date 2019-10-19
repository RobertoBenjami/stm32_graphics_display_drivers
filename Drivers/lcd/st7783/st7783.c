#include "main.h"
#include "lcd.h"
#include "st7783.h"

#if  ST7783_TOUCH == 1
#include "ts.h"
#endif

#if LCD_REVERSE16 == 0
#define  RC(a)   a
#define  RD(a)   a
#endif

/* Konstans szám bájtjainak cseréje, változó bájtjainak cseréje */
#if LCD_REVERSE16 == 1
#define  RC(a)   ((((a) & 0xFF) << 8) | (((a) & 0xFF00) >> 8))
#define  RD(a)   __REVSH(a)
#endif

// Lcd
void      st7783_Init(void);
uint16_t  st7783_ReadID(void);
void      st7783_DisplayOn(void);
void      st7783_DisplayOff(void);
void      st7783_SetCursor(uint16_t Xpos, uint16_t Ypos);
void      st7783_WritePixel(uint16_t Xpos, uint16_t Ypos, uint16_t RGB_Code);
uint16_t  st7783_ReadPixel(uint16_t Xpos, uint16_t Ypos);
void      st7783_SetDisplayWindow(uint16_t Xpos, uint16_t Ypos, uint16_t Width, uint16_t Height);
void      st7783_DrawHLine(uint16_t RGBCode, uint16_t Xpos, uint16_t Ypos, uint16_t Length);
void      st7783_DrawVLine(uint16_t RGBCode, uint16_t Xpos, uint16_t Ypos, uint16_t Length);
uint16_t  st7783_GetLcdPixelWidth(void);
uint16_t  st7783_GetLcdPixelHeight(void);
void      st7783_DrawBitmap(uint16_t Xpos, uint16_t Ypos, uint8_t *pbmp);
void      st7783_DrawRGBImage(uint16_t Xpos, uint16_t Ypos, uint16_t Xsize, uint16_t Ysize, uint8_t *pdata);
void      st7783_ReadRGBImage(uint16_t Xpos, uint16_t Ypos, uint16_t Xsize, uint16_t Ysize, uint8_t *pdata);
void      st7783_FillRect(uint16_t Xpos, uint16_t Ypos, uint16_t Xsize, uint16_t Ysize, uint16_t RGBCode);

// Touchscreen
void      st7783_ts_Init(uint16_t DeviceAddr);
uint8_t   st7783_ts_DetectTouch(uint16_t DeviceAddr);
void      st7783_ts_GetXY(uint16_t DeviceAddr, uint16_t *X, uint16_t *Y);

LCD_DrvTypeDef   st7783_drv =
{
  st7783_Init,
  st7783_ReadID,
  st7783_DisplayOn,
  st7783_DisplayOff,
  st7783_SetCursor,
  st7783_WritePixel,
  st7783_ReadPixel,
  st7783_SetDisplayWindow,
  st7783_DrawHLine,
  st7783_DrawVLine,
  st7783_GetLcdPixelWidth,
  st7783_GetLcdPixelHeight,
  st7783_DrawBitmap,
  st7783_DrawRGBImage,
  #ifdef   LCD_DRVTYPE_V1_1
  st7783_FillRect,
  st7783_ReadRGBImage,
  #endif
};

LCD_DrvTypeDef  *lcd_drv = &st7783_drv;

#define ST7783_START_OSC          0x00
#define ST7783_DRIV_OUT_CTRL      0x01
#define ST7783_DRIV_WAV_CTRL      0x02
#define ST7783_ENTRY_MOD          0x03
#define ST7783_RESIZE_CTRL        0x04
#define ST7783_DISP_CTRL1         0x07
#define ST7783_DISP_CTRL2         0x08
#define ST7783_DISP_CTRL3         0x09
#define ST7783_DISP_CTRL4         0x0A
#define ST7783_RGB_DISP_IF_CTRL1  0x0C
#define ST7783_FRM_MARKER_POS     0x0D
#define ST7783_RGB_DISP_IF_CTRL2  0x0F
#define ST7783_POW_CTRL1          0x10
#define ST7783_POW_CTRL2          0x11
#define ST7783_POW_CTRL3          0x12
#define ST7783_POW_CTRL4          0x13
#define ST7783_GRAM_HOR_AD        0x20
#define ST7783_GRAM_VER_AD        0x21
#define ST7783_RW_GRAM            0x22
#define ST7783_POW_CTRL7          0x29
#define ST7783_FRM_RATE_COL_CTRL  0x2B
#define ST7783_GAMMA_CTRL1        0x30
#define ST7783_GAMMA_CTRL2        0x31
#define ST7783_GAMMA_CTRL3        0x32
#define ST7783_GAMMA_CTRL4        0x35
#define ST7783_GAMMA_CTRL5        0x36
#define ST7783_GAMMA_CTRL6        0x37
#define ST7783_GAMMA_CTRL7        0x38
#define ST7783_GAMMA_CTRL8        0x39
#define ST7783_GAMMA_CTRL9        0x3C
#define ST7783_GAMMA_CTRL10       0x3D
#define ST7783_HOR_START_AD       0x50
#define ST7783_HOR_END_AD         0x51
#define ST7783_VER_START_AD       0x52
#define ST7783_VER_END_AD         0x53
#define ST7783_GATE_SCAN_CTRL1    0x60
#define ST7783_GATE_SCAN_CTRL2    0x61
#define ST7783_GATE_SCAN_CTRL3    0x6A
#define ST7783_PART_IMG1_DISP_POS 0x80
#define ST7783_PART_IMG1_START_AD 0x81
#define ST7783_PART_IMG1_END_AD   0x82
#define ST7783_PART_IMG2_DISP_POS 0x83
#define ST7783_PART_IMG2_START_AD 0x84
#define ST7783_PART_IMG2_END_AD   0x85
#define ST7783_PANEL_IF_CTRL1     0x90
#define ST7783_PANEL_IF_CTRL2     0x92
#define ST7783_PANEL_IF_CTRL3     0x93
#define ST7783_PANEL_IF_CTRL4     0x95
#define ST7783_PANEL_IF_CTRL5     0x97
#define ST7783_PANEL_IF_CTRL6     0x98

// entry mode bitjei (16 vs 18 bites szinkód, szinsorrend, rajzolási irány)
#define ST7783_ENTRY_18BITCOLOR   0x8000
#define ST7783_ENTRY_18BITBCD     0x4000

#define ST7783_ENTRY_RGB          0x1000
#define ST7783_ENTRY_BGR          0x0000

#define ST7783_ENTRY_VERTICAL     0x0008
#define ST7783_ENTRY_X_RIGHT      0x0010
#define ST7783_ENTRY_X_LEFT       0x0000
#define ST7783_ENTRY_Y_DOWN       0x0020
#define ST7783_ENTRY_Y_UP         0x0000

#if ST7783_COLORMODE == 0
#define ST7783_ENTRY_COLORMODE    ST7783_ENTRY_RGB
#else
#define ST7783_ENTRY_COLORMODE    ST7783_ENTRY_BGR
#endif

#if (ST7783_ORIENTATION == 0)
#define ST7783_XSIZE                      ST7783_LCD_PIXEL_WIDTH
#define ST7783_YSIZE                      ST7783_LCD_PIXEL_HEIGHT
#define ST7783_ENTRY_DATA_RIGHT_THEN_UP   ST7783_ENTRY_COLORMODE | ST7783_ENTRY_X_RIGHT | ST7783_ENTRY_Y_UP
#define ST7783_ENTRY_DATA_RIGHT_THEN_DOWN ST7783_ENTRY_COLORMODE | ST7783_ENTRY_X_RIGHT | ST7783_ENTRY_Y_DOWN
#define ST7783_ENTRY_DATA_DOWN_THEN_RIGHT ST7783_ENTRY_COLORMODE | ST7783_ENTRY_X_RIGHT | ST7783_ENTRY_Y_DOWN | ST7783_ENTRY_VERTICAL
#define ST7783_DRIV_OUT_CTRL_DATA         0x0100
#define ST7783_GATE_SCAN_CTRL1_DATA       0xA700
#define ST7783_SETCURSOR(x, y)            {LCD_IO_WriteCmd16(RC(ST7783_GRAM_HOR_AD)); LCD_IO_WriteData16(RD(x)); LCD_IO_WriteCmd16(RC(ST7783_GRAM_VER_AD)); LCD_IO_WriteData16(RD(y));}
#elif (ST7783_ORIENTATION == 1)
#define ST7783_XSIZE                      ST7783_LCD_PIXEL_HEIGHT
#define ST7783_YSIZE                      ST7783_LCD_PIXEL_WIDTH
#define ST7783_ENTRY_DATA_RIGHT_THEN_UP   ST7783_ENTRY_COLORMODE | ST7783_ENTRY_X_LEFT  | ST7783_ENTRY_Y_DOWN | ST7783_ENTRY_VERTICAL
#define ST7783_ENTRY_DATA_RIGHT_THEN_DOWN ST7783_ENTRY_COLORMODE | ST7783_ENTRY_X_RIGHT | ST7783_ENTRY_Y_DOWN | ST7783_ENTRY_VERTICAL
#define ST7783_ENTRY_DATA_DOWN_THEN_RIGHT ST7783_ENTRY_COLORMODE | ST7783_ENTRY_X_RIGHT | ST7783_ENTRY_Y_DOWN
#define ST7783_DRIV_OUT_CTRL_DATA         0x0000
#define ST7783_GATE_SCAN_CTRL1_DATA       0xA700
#define ST7783_SETCURSOR(x, y)            {LCD_IO_WriteCmd16(RC(ST7783_GRAM_HOR_AD)); LCD_IO_WriteData16(RD(y)); LCD_IO_WriteCmd16(RC(ST7783_GRAM_VER_AD)); LCD_IO_WriteData16(RD(x));}
#elif (ST7783_ORIENTATION == 2)
#define ST7783_XSIZE                      ST7783_LCD_PIXEL_WIDTH
#define ST7783_YSIZE                      ST7783_LCD_PIXEL_HEIGHT
#define ST7783_ENTRY_DATA_RIGHT_THEN_UP   ST7783_ENTRY_COLORMODE | ST7783_ENTRY_X_RIGHT | ST7783_ENTRY_Y_UP
#define ST7783_ENTRY_DATA_RIGHT_THEN_DOWN ST7783_ENTRY_COLORMODE | ST7783_ENTRY_X_RIGHT | ST7783_ENTRY_Y_DOWN
#define ST7783_ENTRY_DATA_DOWN_THEN_RIGHT ST7783_ENTRY_COLORMODE | ST7783_ENTRY_X_RIGHT | ST7783_ENTRY_Y_DOWN | ST7783_ENTRY_VERTICAL
#define ST7783_DRIV_OUT_CTRL_DATA         0x0000
#define ST7783_GATE_SCAN_CTRL1_DATA       0x2700
#define ST7783_SETCURSOR(x, y)            {LCD_IO_WriteCmd16(RC(ST7783_GRAM_HOR_AD)); LCD_IO_WriteData16(RD(x)); LCD_IO_WriteCmd16(RC(ST7783_GRAM_VER_AD)); LCD_IO_WriteData16(RD(y));}
#else
#define ST7783_XSIZE                      ST7783_LCD_PIXEL_HEIGHT
#define ST7783_YSIZE                      ST7783_LCD_PIXEL_WIDTH
#define ST7783_ENTRY_DATA_RIGHT_THEN_UP   ST7783_ENTRY_COLORMODE | ST7783_ENTRY_X_LEFT  | ST7783_ENTRY_Y_DOWN | ST7783_ENTRY_VERTICAL
#define ST7783_ENTRY_DATA_RIGHT_THEN_DOWN ST7783_ENTRY_COLORMODE | ST7783_ENTRY_X_RIGHT | ST7783_ENTRY_Y_DOWN | ST7783_ENTRY_VERTICAL
#define ST7783_ENTRY_DATA_DOWN_THEN_RIGHT ST7783_ENTRY_COLORMODE | ST7783_ENTRY_X_RIGHT | ST7783_ENTRY_Y_DOWN
#define ST7783_DRIV_OUT_CTRL_DATA         0x0100
#define ST7783_GATE_SCAN_CTRL1_DATA       0x2700
#define ST7783_SETCURSOR(x, y)            {LCD_IO_WriteCmd16(RC(ST7783_GRAM_HOR_AD)); LCD_IO_WriteData16(RD(y)); LCD_IO_WriteCmd16(RC(ST7783_GRAM_VER_AD)); LCD_IO_WriteData16(RD(x));}
#endif

#define ST7783_LCD_INITIALIZED    0x0001
#define ST7783_IO_INITIALIZED     0x0002
static  uint8_t  Is_st7783_Initialized = 0;

#if      ST7783_MULTITASK_MUTEX == 1 && ST7783_TOUCH == 1
volatile uint8_t io_lcd_busy = 0;
volatile uint8_t io_ts_busy = 0;
#define  ST7783_LCDMUTEX_PUSH()    while(io_ts_busy); io_lcd_busy++;
#define  ST7783_LCDMUTEX_POP()     io_lcd_busy--
#else
#define  ST7783_LCDMUTEX_PUSH()
#define  ST7783_LCDMUTEX_POP()
#endif

//-----------------------------------------------------------------------------
#if ST7783_TOUCH == 1

// Touch paraméterek
// nyomáserõsség értékek honnan hova konvertálodjanak
#define TOUCHMINPRESSRC    8192
#define TOUCHMAXPRESSRC    4096
#define TOUCHMINPRESTRG       0
#define TOUCHMAXPRESTRG     255
#define TOUCH_FILTER         16

// fixpontos Z indexek (16bit egész, 16bit tört)
#define ZINDEXA  ((65536 * (TOUCHMAXPRESTRG - TOUCHMINPRESTRG)) / (TOUCHMAXPRESSRC - TOUCHMINPRESSRC))
#define ZINDEXB  (-ZINDEXA * TOUCHMINPRESSRC)

#define ABS(N)   (((N)<0) ? (-(N)) : (N))

TS_DrvTypeDef   st7783_ts_drv =
{
  st7783_ts_Init,
  0,
  0,
  0,
  st7783_ts_DetectTouch,
  st7783_ts_GetXY,
  0,
  0,
  0,
  0,
};

TS_DrvTypeDef  *ts_drv = &st7783_ts_drv;

#if (ST7783_ORIENTATION == 0)
int32_t  ts_cindex[] = TS_CINDEX_0;
#elif (ST7783_ORIENTATION == 1)
int32_t  ts_cindex[] = TS_CINDEX_1;
#elif (ST7783_ORIENTATION == 2)
int32_t  ts_cindex[] = TS_CINDEX_2;
#elif (ST7783_ORIENTATION == 3)
int32_t  ts_cindex[] = TS_CINDEX_3;
#endif

uint16_t tx, ty;

/* Link function for Touchscreen */
uint8_t  TS_IO_DetectToch(void);
uint16_t TS_IO_GetX(void);
uint16_t TS_IO_GetY(void);
uint16_t TS_IO_GetZ1(void);
uint16_t TS_IO_GetZ2(void);

#endif   // #if ST7783_TOUCH == 1

/* Link function for LCD peripheral */
void     LCD_Delay (uint32_t delay);
void     LCD_IO_Init(void);
void     LCD_IO_Bl_OnOff(uint8_t Bl);
void     LCD_IO_WriteCmd16(uint16_t Cmd);
void     LCD_IO_WriteData8(uint8_t Data);
void     LCD_IO_WriteData16(uint16_t Data);
void     LCD_IO_WriteCmd16DataFill16(uint16_t Cmd, uint16_t Data, uint32_t Size);
void     LCD_IO_WriteCmd16MultipleData8(uint16_t Cmd, uint8_t *pData, uint32_t Size);
void     LCD_IO_WriteCmd16MultipleData16(uint16_t Cmd, uint16_t *pData, uint32_t Size);
void     LCD_IO_ReadCmd16MultipleData8(uint16_t Cmd, uint8_t *pData, uint32_t Size, uint32_t DummySize);
void     LCD_IO_ReadCmd16MultipleData16(uint16_t Cmd, uint16_t *pData, uint32_t Size, uint32_t DummySize);

//-----------------------------------------------------------------------------
void st7783_Init(void)
{
  if((Is_st7783_Initialized & ST7783_LCD_INITIALIZED) == 0)
  {
    Is_st7783_Initialized |= ST7783_LCD_INITIALIZED;
    if((Is_st7783_Initialized & ST7783_IO_INITIALIZED) == 0)
      LCD_IO_Init();
    Is_st7783_Initialized |= ST7783_IO_INITIALIZED;

    LCD_IO_WriteCmd16(RC(0xF3)); LCD_IO_WriteData16(RC(0x0008));

    LCD_Delay(5);

    LCD_IO_WriteCmd16(RC(ST7783_DRIV_OUT_CTRL)); LCD_IO_WriteData16(RC(ST7783_DRIV_OUT_CTRL_DATA));
    LCD_IO_WriteCmd16(RC(ST7783_DRIV_WAV_CTRL)); LCD_IO_WriteData16(RC(0x0700));
    LCD_IO_WriteCmd16(RC(ST7783_ENTRY_MOD)); LCD_IO_WriteData16(RC(ST7783_ENTRY_DATA_RIGHT_THEN_DOWN));
    LCD_IO_WriteCmd16(RC(ST7783_DISP_CTRL2)); LCD_IO_WriteData16(RC(0x0302));
    LCD_IO_WriteCmd16(RC(ST7783_DISP_CTRL3)); LCD_IO_WriteData16(RC(0x0000));
    /*POWER CONTROL REGISTER INITIAL*/
    LCD_IO_WriteCmd16(RC(ST7783_POW_CTRL1)); LCD_IO_WriteData16(RC(0x0000));
    LCD_IO_WriteCmd16(RC(ST7783_POW_CTRL2)); LCD_IO_WriteData16(RC(0x0007));
    LCD_IO_WriteCmd16(RC(ST7783_POW_CTRL3)); LCD_IO_WriteData16(RC(0x0000));
    LCD_IO_WriteCmd16(RC(ST7783_POW_CTRL4)); LCD_IO_WriteData16(RC(0x0000));
    LCD_Delay(5);
    /*POWER SUPPPLY STARTUP 1 SETTING*/
    LCD_IO_WriteCmd16(RC(ST7783_POW_CTRL1)); LCD_IO_WriteData16(RC(0x14B0));
    LCD_Delay(5);
    LCD_IO_WriteCmd16(RC(ST7783_POW_CTRL2)); LCD_IO_WriteData16(RC(0x0007));
    LCD_Delay(5);

    /*POWER SUPPLY STARTUP 2 SETTING*/
    LCD_IO_WriteCmd16(RC(ST7783_POW_CTRL3)); LCD_IO_WriteData16(RC(0x008E));
    LCD_IO_WriteCmd16(RC(ST7783_POW_CTRL4)); LCD_IO_WriteData16(RC(0x0C00));
    LCD_IO_WriteCmd16(RC(ST7783_POW_CTRL7)); LCD_IO_WriteData16(RC(0x0015));
    LCD_Delay(5);

    /****GAMMA CLUSTER SETTING****/
    LCD_IO_WriteCmd16(RC(ST7783_GAMMA_CTRL1)); LCD_IO_WriteData16(RC(0x0000));
    LCD_IO_WriteCmd16(RC(ST7783_GAMMA_CTRL2)); LCD_IO_WriteData16(RC(0x0107));
    LCD_IO_WriteCmd16(RC(ST7783_GAMMA_CTRL3)); LCD_IO_WriteData16(RC(0x0000));
    LCD_IO_WriteCmd16(RC(ST7783_GAMMA_CTRL4)); LCD_IO_WriteData16(RC(0x0203));
    LCD_IO_WriteCmd16(RC(ST7783_GAMMA_CTRL5)); LCD_IO_WriteData16(RC(0x0402));
    LCD_IO_WriteCmd16(RC(ST7783_GAMMA_CTRL6)); LCD_IO_WriteData16(RC(0x0000));
    LCD_IO_WriteCmd16(RC(ST7783_GAMMA_CTRL7)); LCD_IO_WriteData16(RC(0x0207));
    LCD_IO_WriteCmd16(RC(ST7783_GAMMA_CTRL8)); LCD_IO_WriteData16(RC(0x0000));
    LCD_IO_WriteCmd16(RC(ST7783_GAMMA_CTRL9)); LCD_IO_WriteData16(RC(0x0203));
    LCD_IO_WriteCmd16(RC(ST7783_GAMMA_CTRL10)); LCD_IO_WriteData16(RC(0x0403));

    //-DISPLAY WINDOWS 240*320-
    LCD_IO_WriteCmd16(RC(ST7783_HOR_START_AD)); LCD_IO_WriteData16(RC(0));
    LCD_IO_WriteCmd16(RC(ST7783_HOR_END_AD)); LCD_IO_WriteData16(RC(ST7783_LCD_PIXEL_WIDTH - 1));
    LCD_IO_WriteCmd16(RC(ST7783_VER_START_AD)); LCD_IO_WriteData16(RC(0));
    LCD_IO_WriteCmd16(RC(ST7783_VER_END_AD)); LCD_IO_WriteData16(RC(ST7783_LCD_PIXEL_HEIGHT - 1));

    //----FRAME RATE SETTING-----
    LCD_IO_WriteCmd16(RC(ST7783_GATE_SCAN_CTRL1)); LCD_IO_WriteData16(RC(ST7783_GATE_SCAN_CTRL1_DATA));
    LCD_IO_WriteCmd16(RC(ST7783_GATE_SCAN_CTRL2)); LCD_IO_WriteData16(RC(0x0001));
    LCD_IO_WriteCmd16(RC(ST7783_PANEL_IF_CTRL1)); LCD_IO_WriteData16(RC(0x0029)); /* RTNI setting */
    LCD_Delay(5);

    //------DISPLAY ON------
    LCD_IO_WriteCmd16(RC(ST7783_FRM_RATE_COL_CTRL)); LCD_IO_WriteData16(RC(0x000E)); // 110Hz, hogy ne vibráljon
    LCD_IO_WriteCmd16(RC(ST7783_DISP_CTRL1)); LCD_IO_WriteData16(RC(0x0133));
  }
}

//-----------------------------------------------------------------------------
/**
  * @brief  Enables the Display.
  * @param  None
  * @retval None
  */
void st7783_DisplayOn(void)
{
  ST7783_LCDMUTEX_PUSH();

  /* Power On sequence -------------------------------------------------------*/
  LCD_IO_WriteCmd16(RC(ST7783_POW_CTRL1)); LCD_IO_WriteData16(RC(0x14B0));
  LCD_IO_Bl_OnOff(1);

  ST7783_LCDMUTEX_POP();
}

//-----------------------------------------------------------------------------
/**
  * @brief  Disables the Display.
  * @param  None
  * @retval None
  */
void st7783_DisplayOff(void)
{
  ST7783_LCDMUTEX_PUSH();

  /* Power Off sequence ------------------------------------------------------*/
  LCD_IO_WriteCmd16(RC(ST7783_POW_CTRL1)); LCD_IO_WriteData16(RC(0x0002));
  LCD_IO_Bl_OnOff(0);

  ST7783_LCDMUTEX_POP();
}

//-----------------------------------------------------------------------------
/**
  * @brief  Get the LCD pixel Width.
  * @param  None
  * @retval The Lcd Pixel Width
  */
uint16_t st7783_GetLcdPixelWidth(void)
{
  #if ((ST7783_ORIENTATION == 0) || (ST7783_ORIENTATION == 2))
  return (uint16_t)ST7783_LCD_PIXEL_WIDTH;
  #else
  return (uint16_t)ST7783_LCD_PIXEL_HEIGHT;
  #endif
}

//-----------------------------------------------------------------------------
/**
  * @brief  Get the LCD pixel Height.
  * @param  None
  * @retval The Lcd Pixel Height
  */
uint16_t st7783_GetLcdPixelHeight(void)
{
  #if ((ST7783_ORIENTATION == 0) || (ST7783_ORIENTATION == 2))
  return (uint16_t)ST7783_LCD_PIXEL_HEIGHT;
  #else
  return (uint16_t)ST7783_LCD_PIXEL_WIDTH;
  #endif
}

//-----------------------------------------------------------------------------
/**
  * @brief  Get the ST7783 ID.
  * @param  None
  * @retval The ST7783 ID
  */
uint16_t st7783_ReadID(void)
{
  uint16_t ret;

  ST7783_LCDMUTEX_PUSH();

  if(Is_st7783_Initialized == 0)
  {
    st7783_Init();
  }
  LCD_IO_ReadCmd16MultipleData16(RC(0x00), &ret, 1, 2);

  ST7783_LCDMUTEX_POP();
  return RD(ret);
}

//-----------------------------------------------------------------------------
/**
  * @brief  Set Cursor position.
  * @param  Xpos: specifies the X position.
  * @param  Ypos: specifies the Y position.
  * @retval None
  */
void st7783_SetCursor(uint16_t Xpos, uint16_t Ypos)
{
  ST7783_LCDMUTEX_PUSH();
  ST7783_SETCURSOR(Xpos, Ypos);
  ST7783_LCDMUTEX_POP();
}

//-----------------------------------------------------------------------------
/**
  * @brief  Write pixel.
  * @param  Xpos: specifies the X position.
  * @param  Ypos: specifies the Y position.
  * @param  RGBCode: the RGB pixel color
  * @retval None
  */
void st7783_WritePixel(uint16_t Xpos, uint16_t Ypos, uint16_t RGBCode)
{
  ST7783_LCDMUTEX_PUSH();
  ST7783_SETCURSOR(Xpos, Ypos);
  LCD_IO_WriteCmd16(RC(ST7783_RW_GRAM)); LCD_IO_WriteData16(RGBCode);  // Write Data to GRAM (R22h)
  ST7783_LCDMUTEX_POP();
}

//-----------------------------------------------------------------------------
/**
  * @brief  Read pixel.
  * @param  None
  * @retval the RGB pixel color
  */
uint16_t st7783_ReadPixel(uint16_t Xpos, uint16_t Ypos)
{
  uint16_t ret;
  ST7783_LCDMUTEX_PUSH();
  ST7783_SETCURSOR(Xpos, Ypos);
  LCD_IO_ReadCmd16MultipleData16(RC(ST7783_RW_GRAM), &ret, 1, 2);
  ST7783_LCDMUTEX_POP();
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
void st7783_SetDisplayWindow(uint16_t Xpos, uint16_t Ypos, uint16_t Width, uint16_t Height)
{
  ST7783_LCDMUTEX_PUSH();

  #if (ST7783_ORIENTATION == 0) || (ST7783_ORIENTATION == 2)
  LCD_IO_WriteCmd16(RC(ST7783_HOR_START_AD)); LCD_IO_WriteData16(RD(Xpos));
  LCD_IO_WriteCmd16(RC(ST7783_HOR_END_AD)); LCD_IO_WriteData16(RD(Xpos + Width - 1));
  LCD_IO_WriteCmd16(RC(ST7783_VER_START_AD)); LCD_IO_WriteData16(RD(Ypos));
  LCD_IO_WriteCmd16(RC(ST7783_VER_END_AD)); LCD_IO_WriteData16(RD(Ypos + Height - 1));
  #elif (ST7783_ORIENTATION == 1) || (ST7783_ORIENTATION == 3)
  LCD_IO_WriteCmd16(RC(ST7783_HOR_START_AD)); LCD_IO_WriteData16(RD(Ypos));
  LCD_IO_WriteCmd16(RC(ST7783_HOR_END_AD)); LCD_IO_WriteData16(RD(Ypos + Height - 1));
  LCD_IO_WriteCmd16(RC(ST7783_VER_START_AD)); LCD_IO_WriteData16(RD(Xpos));
  LCD_IO_WriteCmd16(RC(ST7783_VER_END_AD)); LCD_IO_WriteData16(RD(Xpos + Width - 1));
  #endif

  ST7783_LCDMUTEX_POP();
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
void st7783_DrawHLine(uint16_t RGBCode, uint16_t Xpos, uint16_t Ypos, uint16_t Length)
{
  ST7783_LCDMUTEX_PUSH();
  ST7783_SETCURSOR(Xpos, Ypos);
  LCD_IO_WriteCmd16DataFill16(RC(ST7783_RW_GRAM), RGBCode, Length);
  ST7783_LCDMUTEX_POP();
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
void st7783_DrawVLine(uint16_t RGBCode, uint16_t Xpos, uint16_t Ypos, uint16_t Length)
{
  ST7783_LCDMUTEX_PUSH();
  LCD_IO_WriteCmd16(RC(ST7783_ENTRY_MOD)); LCD_IO_WriteData16(RC(ST7783_ENTRY_DATA_DOWN_THEN_RIGHT));
  ST7783_SETCURSOR(Xpos, Ypos);
  LCD_IO_WriteCmd16DataFill16(RC(ST7783_RW_GRAM), RGBCode, Length);
  LCD_IO_WriteCmd16(RC(ST7783_ENTRY_MOD)); LCD_IO_WriteData16(RC(ST7783_ENTRY_DATA_RIGHT_THEN_DOWN));
  ST7783_LCDMUTEX_POP();
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
void st7783_FillRect(uint16_t Xpos, uint16_t Ypos, uint16_t Xsize, uint16_t Ysize, uint16_t RGBCode)
{
  ST7783_LCDMUTEX_PUSH();
  st7783_SetDisplayWindow(Xpos, Ypos, Xsize, Ysize);
  ST7783_SETCURSOR(Xpos, Ypos);
  LCD_IO_WriteCmd16DataFill16(RC(ST7783_RW_GRAM), RGBCode, Xsize * Ysize);
  st7783_SetDisplayWindow(0, 0, ST7783_XSIZE, ST7783_YSIZE);
  ST7783_LCDMUTEX_POP();
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
void st7783_DrawBitmap(uint16_t Xpos, uint16_t Ypos, uint8_t *pbmp)
{
  uint32_t index = 0, size = 0;
  /* Read bitmap size */

  ST7783_LCDMUTEX_PUSH();

  // Ypos += pbmp[22] + (pbmp[23] << 8) - 1;
  size = *(volatile uint16_t *) (pbmp + 2);
  size |= (*(volatile uint16_t *) (pbmp + 4)) << 16;
  /* Get bitmap data address offset */
  index = *(volatile uint16_t *) (pbmp + 10);
  index |= (*(volatile uint16_t *) (pbmp + 12)) << 16;
  size = (size - index)/2;
  pbmp += index;

  LCD_IO_WriteCmd16(RC(ST7783_ENTRY_MOD)); LCD_IO_WriteData16(RC(ST7783_ENTRY_DATA_RIGHT_THEN_UP));
  ST7783_SETCURSOR(Xpos, Ypos);
  LCD_IO_WriteCmd16MultipleData16(RC(ST7783_RW_GRAM), (uint16_t *)pbmp, size);
  LCD_IO_WriteCmd16(RC(ST7783_ENTRY_MOD)); LCD_IO_WriteData16(RC(ST7783_ENTRY_DATA_RIGHT_THEN_DOWN));
  st7783_SetDisplayWindow(0, 0, ST7783_XSIZE, ST7783_YSIZE);
  ST7783_LCDMUTEX_POP();
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
void st7783_DrawRGBImage(uint16_t Xpos, uint16_t Ypos, uint16_t Xsize, uint16_t Ysize, uint8_t *pdata)
{
  ST7783_LCDMUTEX_PUSH();
  st7783_SetDisplayWindow(Xpos, Ypos, Xsize, Ysize);
  ST7783_SETCURSOR(Xpos, Ypos);
  LCD_IO_WriteCmd16MultipleData16(RC(ST7783_RW_GRAM), (uint16_t *)pdata, Xsize * Ysize);
  st7783_SetDisplayWindow(0, 0, ST7783_XSIZE, ST7783_YSIZE);
  ST7783_LCDMUTEX_POP();
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
void st7783_ReadRGBImage(uint16_t Xpos, uint16_t Ypos, uint16_t Xsize, uint16_t Ysize, uint8_t *pdata)
{
  ST7783_LCDMUTEX_PUSH();
  st7783_SetDisplayWindow(Xpos, Ypos, Xsize, Ysize);
  ST7783_SETCURSOR(Xpos, Ypos);
  LCD_IO_ReadCmd16MultipleData16(RC(ST7783_RW_GRAM), (uint16_t *)pdata, Xsize * Ysize, 2);
  st7783_SetDisplayWindow(0, 0, ST7783_XSIZE, ST7783_YSIZE);
  ST7783_LCDMUTEX_POP();
}
  
//=============================================================================
#if ST7783_TOUCH == 1
void st7783_ts_Init(uint16_t DeviceAddr)
{
  if((Is_st7783_Initialized & ST7783_IO_INITIALIZED) == 0)
    LCD_IO_Init();
  Is_st7783_Initialized |= ST7783_IO_INITIALIZED;
}

//-----------------------------------------------------------------------------
uint8_t st7783_ts_DetectTouch(uint16_t DeviceAddr)
{
  static uint16_t tp = 0;
  int32_t x1, x2, y1, y2, z11, z12, z21, z22, i, tpr;

  #if  ST7783_MULTITASK_MUTEX == 1
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
        #if  ST7783_MULTITASK_MUTEX == 1
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

  #if  ST7783_MULTITASK_MUTEX == 1
  io_ts_busy = 0;
  #endif

  return tp;
}

//-----------------------------------------------------------------------------
void st7783_ts_GetXY(uint16_t DeviceAddr, uint16_t *X, uint16_t *Y)
{
  *X = tx,
  *Y = ty;
}
#endif // #if ST7783_TOUCH == 1
