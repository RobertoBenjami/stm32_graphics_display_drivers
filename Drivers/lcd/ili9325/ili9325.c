#include "main.h"
#include "lcd.h"
#include "ili9325.h"

#if  ILI9325_TOUCH == 1
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

void     ili9325_Init(void);
uint16_t ili9325_ReadID(void);
void     ili9325_DisplayOn(void);
void     ili9325_DisplayOff(void);
void     ili9325_SetCursor(uint16_t Xpos, uint16_t Ypos);
void     ili9325_WritePixel(uint16_t Xpos, uint16_t Ypos, uint16_t RGB_Code);
uint16_t ili9325_ReadPixel(uint16_t Xpos, uint16_t Ypos);
void     ili9325_SetDisplayWindow(uint16_t Xpos, uint16_t Ypos, uint16_t Width, uint16_t Height);
void     ili9325_DrawHLine(uint16_t RGBCode, uint16_t Xpos, uint16_t Ypos, uint16_t Length);
void     ili9325_DrawVLine(uint16_t RGBCode, uint16_t Xpos, uint16_t Ypos, uint16_t Length);
uint16_t ili9325_GetLcdPixelWidth(void);
uint16_t ili9325_GetLcdPixelHeight(void);
void     ili9325_DrawBitmap(uint16_t Xpos, uint16_t Ypos, uint8_t *pbmp);
void     ili9325_DrawRGBImage(uint16_t Xpos, uint16_t Ypos, uint16_t Xsize, uint16_t Ysize, uint8_t *pdata);
void     ili9325_ReadRGBImage(uint16_t Xpos, uint16_t Ypos, uint16_t Xsize, uint16_t Ysize, uint8_t *pdata);
void     ili9325_FillRect(uint16_t Xpos, uint16_t Ypos, uint16_t Xsize, uint16_t Ysize, uint16_t RGBCode);

LCD_DrvTypeDef   ili9325_drv =
{
  ili9325_Init,
  ili9325_ReadID,
  ili9325_DisplayOn,
  ili9325_DisplayOff,
  ili9325_SetCursor,
  ili9325_WritePixel,
  ili9325_ReadPixel,
  ili9325_SetDisplayWindow,
  ili9325_DrawHLine,
  ili9325_DrawVLine,
  ili9325_GetLcdPixelWidth,
  ili9325_GetLcdPixelHeight,
  ili9325_DrawBitmap,
  ili9325_DrawRGBImage,
  #ifdef   LCD_DRVTYPE_V1_1
  ili9325_FillRect,
  ili9325_ReadRGBImage,
  #endif
};

LCD_DrvTypeDef  *lcd_drv = &ili9325_drv;

#define ILI9325_START_OSC          0x00
#define ILI9325_DRIV_OUT_CTRL      0x01
#define ILI9325_DRIV_WAV_CTRL      0x02
#define ILI9325_ENTRY_MOD          0x03
#define ILI9325_RESIZE_CTRL        0x04
#define ILI9325_DISP_CTRL1         0x07
#define ILI9325_DISP_CTRL2         0x08
#define ILI9325_DISP_CTRL3         0x09
#define ILI9325_DISP_CTRL4         0x0A
#define ILI9325_RGB_DISP_IF_CTRL1  0x0C
#define ILI9325_FRM_MARKER_POS     0x0D
#define ILI9325_RGB_DISP_IF_CTRL2  0x0F
#define ILI9325_POW_CTRL1          0x10
#define ILI9325_POW_CTRL2          0x11
#define ILI9325_POW_CTRL3          0x12
#define ILI9325_POW_CTRL4          0x13
#define ILI9325_GRAM_HOR_AD        0x20
#define ILI9325_GRAM_VER_AD        0x21
#define ILI9325_RW_GRAM            0x22
#define ILI9325_POW_CTRL7          0x29
#define ILI9325_FRM_RATE_COL_CTRL  0x2B
#define ILI9325_GAMMA_CTRL1        0x30
#define ILI9325_GAMMA_CTRL2        0x31
#define ILI9325_GAMMA_CTRL3        0x32
#define ILI9325_GAMMA_CTRL4        0x35
#define ILI9325_GAMMA_CTRL5        0x36
#define ILI9325_GAMMA_CTRL6        0x37
#define ILI9325_GAMMA_CTRL7        0x38
#define ILI9325_GAMMA_CTRL8        0x39
#define ILI9325_GAMMA_CTRL9        0x3C
#define ILI9325_GAMMA_CTRL10       0x3D
#define ILI9325_HOR_START_AD       0x50
#define ILI9325_HOR_END_AD         0x51
#define ILI9325_VER_START_AD       0x52
#define ILI9325_VER_END_AD         0x53
#define ILI9325_GATE_SCAN_CTRL1    0x60
#define ILI9325_GATE_SCAN_CTRL2    0x61
#define ILI9325_GATE_SCAN_CTRL3    0x6A
#define ILI9325_PART_IMG1_DISP_POS 0x80
#define ILI9325_PART_IMG1_START_AD 0x81
#define ILI9325_PART_IMG1_END_AD   0x82
#define ILI9325_PART_IMG2_DISP_POS 0x83
#define ILI9325_PART_IMG2_START_AD 0x84
#define ILI9325_PART_IMG2_END_AD   0x85
#define ILI9325_PANEL_IF_CTRL1     0x90
#define ILI9325_PANEL_IF_CTRL2     0x92
#define ILI9325_PANEL_IF_CTRL3     0x93
#define ILI9325_PANEL_IF_CTRL4     0x95
#define ILI9325_PANEL_IF_CTRL5     0x97
#define ILI9325_PANEL_IF_CTRL6     0x98

// entry mode bitjei (16 vs 18 bites szinkód, szinsorrend, rajzolási irány)
#define ILI9325_ENTRY_18BITCOLOR   0x8000
#define ILI9325_ENTRY_18BITBCD     0x4000

#define ILI9325_ENTRY_RGB          0x1000
#define ILI9325_ENTRY_BGR          0x0000

#define ILI9325_ENTRY_VERTICAL     0x0008
#define ILI9325_ENTRY_X_LEFT       0x0000
#define ILI9325_ENTRY_X_RIGHT      0x0010
#define ILI9325_ENTRY_Y_UP         0x0000
#define ILI9325_ENTRY_Y_DOWN       0x0020

#if ILI9325_COLORMODE == 0
#define ILI9325_ENTRY_COLORMODE    ILI9325_ENTRY_RGB
#else
#define ILI9325_ENTRY_COLORMODE    ILI9325_ENTRY_BGR
#endif

// Az orientáciokhoz tartozo ENTRY modok jobra/fel és jobbra/le rajzolási irányhoz
#if (ILI9325_ORIENTATION == 0)
#define ILI9325_ENTRY_DATA_RIGHT_THEN_UP     ILI9325_ENTRY_COLORMODE | ILI9325_ENTRY_X_RIGHT | ILI9325_ENTRY_Y_UP
#define ILI9325_ENTRY_DATA_RIGHT_THEN_DOWN   ILI9325_ENTRY_COLORMODE | ILI9325_ENTRY_X_RIGHT | ILI9325_ENTRY_Y_DOWN
#define ILI9325_ENTRY_DATA_DOWN_THEN_RIGHT   ILI9325_ENTRY_COLORMODE | ILI9325_ENTRY_X_RIGHT | ILI9325_ENTRY_Y_DOWN | ILI9325_ENTRY_VERTICAL
#define ILI9325_DRIV_OUT_CTRL_DATA           0x0100
#define ILI9325_GATE_SCAN_CTRL1_DATA         0xA700
#define ILI9325_SETCURSOR(x, y)              {LCD_IO_WriteCmd16(RC(ILI9325_GRAM_HOR_AD)); LCD_IO_WriteData16(RD(x)); LCD_IO_WriteCmd16(RC(ILI9325_GRAM_VER_AD)); LCD_IO_WriteData16(RD(y));}
#elif (ILI9325_ORIENTATION == 1)
#define ILI9325_ENTRY_DATA_RIGHT_THEN_UP     ILI9325_ENTRY_COLORMODE | ILI9325_ENTRY_X_LEFT  | ILI9325_ENTRY_Y_DOWN | ILI9325_ENTRY_VERTICAL
#define ILI9325_ENTRY_DATA_RIGHT_THEN_DOWN   ILI9325_ENTRY_COLORMODE | ILI9325_ENTRY_X_RIGHT | ILI9325_ENTRY_Y_DOWN | ILI9325_ENTRY_VERTICAL
#define ILI9325_ENTRY_DATA_DOWN_THEN_RIGHT   ILI9325_ENTRY_COLORMODE | ILI9325_ENTRY_X_RIGHT | ILI9325_ENTRY_Y_DOWN
#define ILI9325_DRIV_OUT_CTRL_DATA           0x0000
#define ILI9325_GATE_SCAN_CTRL1_DATA         0xA700
#define ILI9325_SETCURSOR(x, y)              {LCD_IO_WriteCmd16(RC(ILI9325_GRAM_HOR_AD)); LCD_IO_WriteData16(RD(y)); LCD_IO_WriteCmd16(RC(ILI9325_GRAM_VER_AD)); LCD_IO_WriteData16(RD(x));}
#elif (ILI9325_ORIENTATION == 2)
#define ILI9325_ENTRY_DATA_RIGHT_THEN_UP     ILI9325_ENTRY_COLORMODE | ILI9325_ENTRY_X_RIGHT | ILI9325_ENTRY_Y_UP
#define ILI9325_ENTRY_DATA_RIGHT_THEN_DOWN   ILI9325_ENTRY_COLORMODE | ILI9325_ENTRY_X_RIGHT | ILI9325_ENTRY_Y_DOWN
#define ILI9325_ENTRY_DATA_DOWN_THEN_RIGHT   ILI9325_ENTRY_COLORMODE | ILI9325_ENTRY_X_RIGHT | ILI9325_ENTRY_Y_DOWN | ILI9325_ENTRY_VERTICAL
#define ILI9325_DRIV_OUT_CTRL_DATA           0x0000
#define ILI9325_GATE_SCAN_CTRL1_DATA         0x2700
#define ILI9325_SETCURSOR(x, y)              {LCD_IO_WriteCmd16(RC(ILI9325_GRAM_HOR_AD)); LCD_IO_WriteData16(RD(x)); LCD_IO_WriteCmd16(RC(ILI9325_GRAM_VER_AD)); LCD_IO_WriteData16(RD(y));}
#else
#define ILI9325_ENTRY_DATA_RIGHT_THEN_UP     ILI9325_ENTRY_COLORMODE | ILI9325_ENTRY_X_LEFT  | ILI9325_ENTRY_Y_DOWN | ILI9325_ENTRY_VERTICAL
#define ILI9325_ENTRY_DATA_RIGHT_THEN_DOWN   ILI9325_ENTRY_COLORMODE | ILI9325_ENTRY_X_RIGHT | ILI9325_ENTRY_Y_DOWN | ILI9325_ENTRY_VERTICAL
#define ILI9325_ENTRY_DATA_DOWN_THEN_RIGHT   ILI9325_ENTRY_COLORMODE | ILI9325_ENTRY_X_RIGHT | ILI9325_ENTRY_Y_DOWN
#define ILI9325_DRIV_OUT_CTRL_DATA           0x0100
#define ILI9325_GATE_SCAN_CTRL1_DATA         0x2700
#define ILI9325_SETCURSOR(x, y)              {LCD_IO_WriteCmd16(RC(ILI9325_GRAM_HOR_AD)); LCD_IO_WriteData16(RD(y)); LCD_IO_WriteCmd16(RC(ILI9325_GRAM_VER_AD)); LCD_IO_WriteData16(RD(x));}
#endif

#define ILI9325_LCD_INITIALIZED    0x01
#define ILI9325_IO_INITIALIZED     0x02
static  uint8_t   Is_ili9325_Initialized = 0;

#if      ILI9325_MULTITASK_MUTEX == 1 && ILI9325_TOUCH == 1
volatile uint8_t io_lcd_busy = 0;
volatile uint8_t io_ts_busy = 0;
#define  ILI9325_LCDMUTEX_PUSH()    while(io_ts_busy); io_lcd_busy++;
#define  ILI9325_LCDMUTEX_POP()     io_lcd_busy--
#else
#define  ILI9325_LCDMUTEX_PUSH()
#define  ILI9325_LCDMUTEX_POP()
#endif

//-----------------------------------------------------------------------------
#if ILI9325_TOUCH == 1

// Touch paraméterek
// nyomáserõsség értékek honnan hova konvertálodjanak

#define TOUCHMINPRESSRC    8192
#define TOUCHMAXPRESSRC    4096
#define TOUCHMINPRESTRG       0
#define TOUCHMAXPRESTRG     255
#define TOUCH_FILTER          8

// fixpontos Z indexek (16bit egész, 16bit tört)
#define ZINDEXA  ((65536 * (TOUCHMAXPRESTRG - TOUCHMINPRESTRG)) / (TOUCHMAXPRESSRC - TOUCHMINPRESSRC))
#define ZINDEXB  (-ZINDEXA * TOUCHMINPRESSRC)

#define ABS(N)   (((N)<0) ? (-(N)) : (N))

void     ili9325_ts_Init(uint16_t DeviceAddr);
uint8_t  ili9325_ts_DetectTouch(uint16_t DeviceAddr);
void     ili9325_ts_GetXY(uint16_t DeviceAddr, uint16_t *X, uint16_t *Y);

TS_DrvTypeDef   ili9325_ts_drv =
{
  ili9325_ts_Init,
  0,
  0,
  0,
  ili9325_ts_DetectTouch,
  ili9325_ts_GetXY,
  0,
  0,
  0,
  0,
};

TS_DrvTypeDef  *ts_drv = &ili9325_ts_drv;

#if (ILI9325_ORIENTATION == 0)
int32_t  ts_cindex[] = TS_CINDEX_0;
#elif (ILI9325_ORIENTATION == 1)
int32_t  ts_cindex[] = TS_CINDEX_1;
#elif (ILI9325_ORIENTATION == 2)
int32_t  ts_cindex[] = TS_CINDEX_2;
#elif (ILI9325_ORIENTATION == 3)
int32_t  ts_cindex[] = TS_CINDEX_3;
#endif

uint16_t tx, ty;

/* Link function for Touchscreen */
uint8_t  TS_IO_DetectToch(void);
uint16_t TS_IO_GetX(void);
uint16_t TS_IO_GetY(void);
uint16_t TS_IO_GetZ1(void);
uint16_t TS_IO_GetZ2(void);

#endif   // #if ILI9325_TOUCH == 1

/* Link function for LCD peripheral */
void     LCD_Delay (uint32_t delay);
void     LCD_IO_Init(void);
void     LCD_IO_Bl_OnOff(uint8_t Bl);

void     LCD_IO_WriteCmd16(uint16_t Cmd);
void     LCD_IO_WriteData16(uint16_t Data);
void     LCD_IO_WriteCmd16DataFill16(uint16_t Cmd, uint16_t Data, uint32_t Size);
void     LCD_IO_WriteCmd16MultipleData16(uint16_t Cmd, uint16_t *pData, uint32_t Size);
void     LCD_IO_ReadCmd16MultipleData16(uint16_t Cmd, uint16_t *pData, uint32_t Size, uint32_t DummySize);

//-----------------------------------------------------------------------------
void ili9325_Init(void)
{
  if((Is_ili9325_Initialized & ILI9325_LCD_INITIALIZED) == 0)
  {
    Is_ili9325_Initialized |= ILI9325_LCD_INITIALIZED;
    if((Is_ili9325_Initialized & ILI9325_IO_INITIALIZED) == 0)
      LCD_IO_Init();
    Is_ili9325_Initialized |= ILI9325_IO_INITIALIZED;

    LCD_IO_WriteCmd16(RC(0xF3)); LCD_IO_WriteData16(RC(0x0008));

    LCD_Delay(5);

    LCD_IO_WriteCmd16(RC(ILI9325_DRIV_OUT_CTRL)); LCD_IO_WriteData16(RC(ILI9325_DRIV_OUT_CTRL_DATA));
    LCD_IO_WriteCmd16(RC(ILI9325_DRIV_WAV_CTRL)); LCD_IO_WriteData16(RC(0x0700));
    LCD_IO_WriteCmd16(RC(ILI9325_ENTRY_MOD)); LCD_IO_WriteData16(RC(ILI9325_ENTRY_DATA_RIGHT_THEN_DOWN));
    LCD_IO_WriteCmd16(RC(ILI9325_DISP_CTRL2)); LCD_IO_WriteData16(RC(0x0302));
    LCD_IO_WriteCmd16(RC(ILI9325_DISP_CTRL3)); LCD_IO_WriteData16(RC(0x0000));
    /*POWER CONTROL REGISTER INITIAL*/
    LCD_IO_WriteCmd16(RC(ILI9325_POW_CTRL1)); LCD_IO_WriteData16(RC(0x0000));
    LCD_IO_WriteCmd16(RC(ILI9325_POW_CTRL2)); LCD_IO_WriteData16(RC(0x0007));
    LCD_IO_WriteCmd16(RC(ILI9325_POW_CTRL3)); LCD_IO_WriteData16(RC(0x0000));
    LCD_IO_WriteCmd16(RC(ILI9325_POW_CTRL4)); LCD_IO_WriteData16(RC(0x0000));
    LCD_Delay(5);
    /*POWER SUPPPLY STARTUP 1 SETTING*/
    LCD_IO_WriteCmd16(RC(ILI9325_POW_CTRL1)); LCD_IO_WriteData16(RC(0x14B0));
    LCD_Delay(5);
    LCD_IO_WriteCmd16(RC(ILI9325_POW_CTRL2)); LCD_IO_WriteData16(RC(0x0007));
    LCD_Delay(5);
    /*POWER SUPPLY STARTUP 2 SETTING*/
    LCD_IO_WriteCmd16(RC(ILI9325_POW_CTRL3)); LCD_IO_WriteData16(RC(0x008E));
    LCD_IO_WriteCmd16(RC(ILI9325_POW_CTRL4)); LCD_IO_WriteData16(RC(0x0C00));
    LCD_IO_WriteCmd16(RC(ILI9325_POW_CTRL7)); LCD_IO_WriteData16(RC(0x0015));

    LCD_Delay(5);
    /****GAMMA CLUSTER SETTING****/
    LCD_IO_WriteCmd16(RC(ILI9325_GAMMA_CTRL1)); LCD_IO_WriteData16(RC(0x0000));
    LCD_IO_WriteCmd16(RC(ILI9325_GAMMA_CTRL2)); LCD_IO_WriteData16(RC(0x0107));
    LCD_IO_WriteCmd16(RC(ILI9325_GAMMA_CTRL3)); LCD_IO_WriteData16(RC(0x0000));
    LCD_IO_WriteCmd16(RC(ILI9325_GAMMA_CTRL4)); LCD_IO_WriteData16(RC(0x0203));
    LCD_IO_WriteCmd16(RC(ILI9325_GAMMA_CTRL5)); LCD_IO_WriteData16(RC(0x0402));
    LCD_IO_WriteCmd16(RC(ILI9325_GAMMA_CTRL6)); LCD_IO_WriteData16(RC(0x0000));
    LCD_IO_WriteCmd16(RC(ILI9325_GAMMA_CTRL7)); LCD_IO_WriteData16(RC(0x0207));
    LCD_IO_WriteCmd16(RC(ILI9325_GAMMA_CTRL8)); LCD_IO_WriteData16(RC(0x0000));
    LCD_IO_WriteCmd16(RC(ILI9325_GAMMA_CTRL9)); LCD_IO_WriteData16(RC(0x0203));
    LCD_IO_WriteCmd16(RC(ILI9325_GAMMA_CTRL10)); LCD_IO_WriteData16(RC(0x0403));
    //-DISPLAY WINDOWS 240*320-
    LCD_IO_WriteCmd16(RC(ILI9325_HOR_START_AD)); LCD_IO_WriteData16(RC(0));
    LCD_IO_WriteCmd16(RC(ILI9325_HOR_END_AD)); LCD_IO_WriteData16(RC(ILI9325_LCD_PIXEL_WIDTH - 1));  // 240 - 1
    LCD_IO_WriteCmd16(RC(ILI9325_VER_START_AD)); LCD_IO_WriteData16(RC(0));
    LCD_IO_WriteCmd16(RC(ILI9325_VER_END_AD)); LCD_IO_WriteData16(RC(ILI9325_LCD_PIXEL_HEIGHT - 1)); // 320 - 1
    //----FRAME RATE SETTING-----
    LCD_IO_WriteCmd16(RC(ILI9325_GATE_SCAN_CTRL1)); LCD_IO_WriteData16(RC(ILI9325_GATE_SCAN_CTRL1_DATA));
    LCD_IO_WriteCmd16(RC(ILI9325_GATE_SCAN_CTRL2)); LCD_IO_WriteData16(RC(0x0001));
    LCD_IO_WriteCmd16(RC(ILI9325_PANEL_IF_CTRL1)); LCD_IO_WriteData16(RC(0x0029)); /* RTNI setting */
    LCD_Delay(5);

    //------DISPLAY ON------
    LCD_IO_WriteCmd16(RC(ILI9325_FRM_RATE_COL_CTRL)); LCD_IO_WriteData16(RC(0x000E)); // 110Hz, hogy ne vibráljon
    LCD_IO_WriteCmd16(RC(ILI9325_DISP_CTRL1)); LCD_IO_WriteData16(RC(0x0133));
  }
}

//-----------------------------------------------------------------------------
/**
  * @brief  Enables the Display.
  * @param  None
  * @retval None
  */
void ili9325_DisplayOn(void)
{
  ILI9325_LCDMUTEX_PUSH();

  /* Power On sequence -------------------------------------------------------*/
  LCD_IO_WriteCmd16(RC(ILI9325_POW_CTRL1)); LCD_IO_WriteData16(RC(0x0000)); /* SAP, BT[3:0], AP, DSTB, SLP, STB */
  LCD_IO_WriteCmd16(RC(ILI9325_POW_CTRL2)); LCD_IO_WriteData16(RC(0x0000)); /* DC1[2:0], DC0[2:0], VC[2:0] */
  LCD_IO_WriteCmd16(RC(ILI9325_POW_CTRL3)); LCD_IO_WriteData16(RC(0x0000)); /* VREG1OUT voltage */
  LCD_IO_WriteCmd16(RC(ILI9325_POW_CTRL4)); LCD_IO_WriteData16(RC(0x0000)); /* VDV[4:0] for VCOM amplitude*/
  LCD_Delay(5);

  LCD_IO_WriteCmd16(RC(ILI9325_POW_CTRL1)); LCD_IO_WriteData16(RC(0x14B0));
  LCD_IO_WriteCmd16(RC(ILI9325_POW_CTRL2)); LCD_IO_WriteData16(RC(0x0007));

  LCD_IO_WriteCmd16(RC(ILI9325_POW_CTRL3)); LCD_IO_WriteData16(RC(0x008E)); /* VREG1OUT voltage */

  LCD_IO_WriteCmd16(RC(ILI9325_POW_CTRL4)); LCD_IO_WriteData16(RC(0x0C00));
  LCD_IO_WriteCmd16(RC(ILI9325_POW_CTRL7)); LCD_IO_WriteData16(RC(0x0015));

  /* Display On */
  LCD_IO_WriteCmd16(RC(ILI9325_FRM_RATE_COL_CTRL)); LCD_IO_WriteData16(RC(0x000E)); // 110Hz, hogy ne vibráljon
  LCD_IO_WriteCmd16(RC(ILI9325_DISP_CTRL1)); LCD_IO_WriteData16(RC(0x0133)); /* display ON */

  LCD_IO_Bl_OnOff(1);

  ILI9325_LCDMUTEX_POP();
}

//-----------------------------------------------------------------------------
/**
  * @brief  Disables the Display.
  * @param  None
  * @retval None
  */
void ili9325_DisplayOff(void)
{
  ILI9325_LCDMUTEX_PUSH();

  /* Power Off sequence ------------------------------------------------------*/
  LCD_IO_WriteCmd16(RC(ILI9325_POW_CTRL1)); LCD_IO_WriteData16(RC(0x0000)); /* SAP, BT[3:0], AP, DSTB, SLP, STB */
  LCD_IO_WriteCmd16(RC(ILI9325_POW_CTRL2)); LCD_IO_WriteData16(RC(0x0000)); /* DC1[2:0], DC0[2:0], VC[2:0] */
  LCD_IO_WriteCmd16(RC(ILI9325_POW_CTRL3)); LCD_IO_WriteData16(RC(0x0000)); /* VREG1OUT voltage */
  LCD_IO_WriteCmd16(RC(ILI9325_POW_CTRL4)); LCD_IO_WriteData16(RC(0x0000)); /* VDV[4:0] for VCOM amplitude*/

  LCD_IO_WriteCmd16(RC(ILI9325_POW_CTRL7)); LCD_IO_WriteData16(RC(0x0000)); /* VCM[4:0] for VCOMH */
  
  /* Display Off */
  LCD_IO_WriteCmd16(RC(ILI9325_DISP_CTRL1)); LCD_IO_WriteData16(RC(0x0));

  LCD_IO_Bl_OnOff(0);

  ILI9325_LCDMUTEX_POP();
}

//-----------------------------------------------------------------------------
/**
  * @brief  Get the LCD pixel Width.
  * @param  None
  * @retval The Lcd Pixel Width
  */
uint16_t ili9325_GetLcdPixelWidth(void)
{
  #if ((ILI9325_ORIENTATION == 0) || (ILI9325_ORIENTATION == 2))
  return (uint16_t)ILI9325_LCD_PIXEL_WIDTH;
  #else
  return (uint16_t)ILI9325_LCD_PIXEL_HEIGHT;
  #endif
}

//-----------------------------------------------------------------------------
/**
  * @brief  Get the LCD pixel Height.
  * @param  None
  * @retval The Lcd Pixel Height
  */
uint16_t ili9325_GetLcdPixelHeight(void)
{
  #if ((ILI9325_ORIENTATION == 0) || (ILI9325_ORIENTATION == 2))
  return (uint16_t)ILI9325_LCD_PIXEL_HEIGHT;
  #else
  return (uint16_t)ILI9325_LCD_PIXEL_WIDTH;
  #endif
}

//-----------------------------------------------------------------------------
/**
  * @brief  Get the ILI9325 ID.
  * @param  None
  * @retval The ILI9325 ID
  */
uint16_t ili9325_ReadID(void)
{
  uint16_t ret;
  ILI9325_LCDMUTEX_PUSH();

  if(Is_ili9325_Initialized == 0)
  {
    ili9325_Init();
  }
  LCD_IO_ReadCmd16MultipleData16(RC(0x00), &ret, 1, 2);

  ILI9325_LCDMUTEX_POP();

  return RD(ret);
}

//-----------------------------------------------------------------------------
/**
  * @brief  Set Cursor position.
  * @param  Xpos: specifies the X position.
  * @param  Ypos: specifies the Y position.
  * @retval None
  */
void ili9325_SetCursor(uint16_t Xpos, uint16_t Ypos)
{
  ILI9325_LCDMUTEX_PUSH();
  ILI9325_SETCURSOR(Xpos, Ypos);
  ILI9325_LCDMUTEX_POP();
}

//-----------------------------------------------------------------------------
/**
  * @brief  Write pixel.
  * @param  Xpos: specifies the X position.
  * @param  Ypos: specifies the Y position.
  * @param  RGBCode: the RGB pixel color
  * @retval None
  */
void ili9325_WritePixel(uint16_t Xpos, uint16_t Ypos, uint16_t RGBCode)
{
  ILI9325_LCDMUTEX_PUSH();
  ILI9325_SETCURSOR(Xpos, Ypos);
  LCD_IO_WriteCmd16(RC(ILI9325_RW_GRAM)); LCD_IO_WriteData16(RGBCode);
  ILI9325_LCDMUTEX_POP();
}

//-----------------------------------------------------------------------------
/**
  * @brief  Read pixel.
  * @param  None
  * @retval the RGB pixel color
  */
uint16_t ili9325_ReadPixel(uint16_t Xpos, uint16_t Ypos)
{
  uint16_t ret;
  ILI9325_LCDMUTEX_PUSH();
  ILI9325_SETCURSOR(Xpos, Ypos);
  LCD_IO_ReadCmd16MultipleData16(RC(ILI9325_RW_GRAM), &ret, 1, 2);
  ILI9325_LCDMUTEX_POP();
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
void ili9325_SetDisplayWindow(uint16_t Xpos, uint16_t Ypos, uint16_t Width, uint16_t Height)
{
  ILI9325_LCDMUTEX_PUSH();

  #if (ILI9325_ORIENTATION == 0) || (ILI9325_ORIENTATION == 2)
  LCD_IO_WriteCmd16(RC(ILI9325_HOR_START_AD)); LCD_IO_WriteData16(RD(Xpos));
  LCD_IO_WriteCmd16(RC(ILI9325_HOR_END_AD)); LCD_IO_WriteData16(RD(Xpos + Width - 1));
  LCD_IO_WriteCmd16(RC(ILI9325_VER_START_AD)); LCD_IO_WriteData16(RD(Ypos));
  LCD_IO_WriteCmd16(RC(ILI9325_VER_END_AD)); LCD_IO_WriteData16(RD(Ypos + Height - 1));
  #elif (ILI9325_ORIENTATION == 1) || (ILI9325_ORIENTATION == 3)
  LCD_IO_WriteCmd16(RC(ILI9325_HOR_START_AD)); LCD_IO_WriteData16(RD(Ypos));
  LCD_IO_WriteCmd16(RC(ILI9325_HOR_END_AD)); LCD_IO_WriteData16(RD(Ypos + Height - 1));
  LCD_IO_WriteCmd16(RC(ILI9325_VER_START_AD)); LCD_IO_WriteData16(RD(Xpos));
  LCD_IO_WriteCmd16(RC(ILI9325_VER_END_AD)); LCD_IO_WriteData16(RD(Xpos + Width - 1));
  #endif

  ILI9325_LCDMUTEX_POP();
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
void ili9325_DrawHLine(uint16_t RGBCode, uint16_t Xpos, uint16_t Ypos, uint16_t Length)
{
  ILI9325_LCDMUTEX_PUSH();
  ILI9325_SETCURSOR(Xpos, Ypos);
  LCD_IO_WriteCmd16DataFill16(RC(ILI9325_RW_GRAM), RGBCode, Length);
  ILI9325_LCDMUTEX_POP();
}

//-----------------------------------------------------------------------------
/**
  * @brief  Draw vertical line.
  * @param  RGBCode:  specifies the RGB color
  * @param  Xpos:     specifies the X position.
  * @param  Ypos:     specifies the Y position.
  * @param  Length:   specifies the Line length.
  * @retval None
  */
void ili9325_DrawVLine(uint16_t RGBCode, uint16_t Xpos, uint16_t Ypos, uint16_t Length)
{
  ILI9325_LCDMUTEX_PUSH();
  LCD_IO_WriteCmd16(RC(ILI9325_ENTRY_MOD)); LCD_IO_WriteData16(RC(ILI9325_ENTRY_DATA_DOWN_THEN_RIGHT));
  ILI9325_SETCURSOR(Xpos, Ypos);
  LCD_IO_WriteCmd16DataFill16(RC(ILI9325_RW_GRAM), RGBCode, Length);
  LCD_IO_WriteCmd16(RC(ILI9325_ENTRY_MOD)); LCD_IO_WriteData16(RC(ILI9325_ENTRY_DATA_RIGHT_THEN_DOWN));
  ILI9325_LCDMUTEX_POP();
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
void ili9325_FillRect(uint16_t Xpos, uint16_t Ypos, uint16_t Xsize, uint16_t Ysize, uint16_t RGBCode)
{
  ILI9325_LCDMUTEX_PUSH();
  ili9325_SetDisplayWindow(Xpos, Ypos, Xsize, Ysize);
  ILI9325_SETCURSOR(Xpos, Ypos);
  LCD_IO_WriteCmd16DataFill16(RC(ILI9325_RW_GRAM), RGBCode, Xsize * Ysize);
  LCD_IO_WriteCmd16(RC(ILI9325_HOR_START_AD)); LCD_IO_WriteData16(RC(0));
  LCD_IO_WriteCmd16(RC(ILI9325_HOR_END_AD)); LCD_IO_WriteData16(RC(ILI9325_LCD_PIXEL_WIDTH - 1));
  LCD_IO_WriteCmd16(RC(ILI9325_VER_START_AD)); LCD_IO_WriteData16(RC(0));
  LCD_IO_WriteCmd16(RC(ILI9325_VER_END_AD)); LCD_IO_WriteData16(RC(ILI9325_LCD_PIXEL_HEIGHT - 1));
  ILI9325_LCDMUTEX_POP();
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
void ili9325_DrawBitmap(uint16_t Xpos, uint16_t Ypos, uint8_t *pbmp)
{
  uint32_t index = 0, size = 0;
  /* Read bitmap size */
  Ypos += pbmp[22] + (pbmp[23] << 8) - 1;
  size = *(volatile uint16_t *) (pbmp + 2);
  size |= (*(volatile uint16_t *) (pbmp + 4)) << 16;
  /* Get bitmap data address offset */
  index = *(volatile uint16_t *) (pbmp + 10);
  index |= (*(volatile uint16_t *) (pbmp + 12)) << 16;
  size = (size - index) / 2;
  pbmp += index;

  ILI9325_LCDMUTEX_PUSH();
  LCD_IO_WriteCmd16(RC(ILI9325_ENTRY_MOD)); LCD_IO_WriteData16(RC(ILI9325_ENTRY_DATA_RIGHT_THEN_UP));
  ILI9325_SETCURSOR(Xpos, Ypos);
  LCD_IO_WriteCmd16MultipleData16(RC(ILI9325_RW_GRAM), (uint16_t *)pbmp, size);
  LCD_IO_WriteCmd16(RC(ILI9325_ENTRY_MOD)); LCD_IO_WriteData16(RC(ILI9325_ENTRY_DATA_RIGHT_THEN_DOWN));

  LCD_IO_WriteCmd16(RC(ILI9325_HOR_START_AD)); LCD_IO_WriteData16(RC(0));
  LCD_IO_WriteCmd16(RC(ILI9325_HOR_END_AD)); LCD_IO_WriteData16(RC(ILI9325_LCD_PIXEL_WIDTH - 1));
  LCD_IO_WriteCmd16(RC(ILI9325_VER_START_AD)); LCD_IO_WriteData16(RC(0));
  LCD_IO_WriteCmd16(RC(ILI9325_VER_END_AD)); LCD_IO_WriteData16(RC(ILI9325_LCD_PIXEL_HEIGHT - 1));

  ILI9325_LCDMUTEX_POP();
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
void ili9325_DrawRGBImage(uint16_t Xpos, uint16_t Ypos, uint16_t Xsize, uint16_t Ysize, uint8_t *pdata)
{
  ILI9325_LCDMUTEX_PUSH();
  ili9325_SetDisplayWindow(Xpos, Ypos, Xsize, Ysize);
  ILI9325_SETCURSOR(Xpos, Ypos);
  LCD_IO_WriteCmd16MultipleData16(RC(ILI9325_RW_GRAM), (uint16_t *)pdata, Xsize * Ysize);

  LCD_IO_WriteCmd16(RC(ILI9325_HOR_START_AD)); LCD_IO_WriteData16(RC(0));
  LCD_IO_WriteCmd16(RC(ILI9325_HOR_END_AD)); LCD_IO_WriteData16(RC(ILI9325_LCD_PIXEL_WIDTH - 1));
  LCD_IO_WriteCmd16(RC(ILI9325_VER_START_AD)); LCD_IO_WriteData16(RC(0));
  LCD_IO_WriteCmd16(RC(ILI9325_VER_END_AD)); LCD_IO_WriteData16(RC(ILI9325_LCD_PIXEL_HEIGHT - 1));
  ILI9325_LCDMUTEX_POP();
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
void ili9325_ReadRGBImage(uint16_t Xpos, uint16_t Ypos, uint16_t Xsize, uint16_t Ysize, uint8_t *pdata)
{
  ILI9325_LCDMUTEX_PUSH();
  #if 0
  ili9325_SetDisplayWindow(Xpos, Ypos, Xsize, Ysize);
  ILI9325_SETCURSOR(Xpos, Ypos);
  LCD_IO_ReadMultipleData16(RC(ILI9325_RW_GRAM), (uint16_t *)pdata, Xsize * Ysize);
  LCD_IO_WriteCmd16(RC(ILI9325_HOR_START_AD)); LCD_IO_WriteData16(RC(0));
  LCD_IO_WriteCmd16(RC(ILI9325_HOR_END_AD)); LCD_IO_WriteData16(RC(ILI9325_LCD_PIXEL_WIDTH - 1));
  LCD_IO_WriteCmd16(RC(ILI9325_VER_START_AD)); LCD_IO_WriteData16(RC(0));
  LCD_IO_WriteCmd16(RC(ILI9325_VER_END_AD)); LCD_IO_WriteData16(RC(ILI9325_LCD_PIXEL_HEIGHT - 1));
  #else
  for(uint16_t yp = Ypos; yp < Ypos + Ysize; yp++)
    for(uint16_t xp = Xpos; xp < Xpos + Xsize; xp++)
    { // mivel memoria olvasáskor nem lépteti a memoriacimet automatikusan csak ez a modszer marad :(
      ILI9325_SETCURSOR(xp, yp);
      LCD_IO_ReadCmd16MultipleData16(RC(ILI9325_RW_GRAM), (uint16_t *)pdata, 1, 2);
      pdata += 2;
    }
  #endif
  ILI9325_LCDMUTEX_POP();
}

//=============================================================================
#if ILI9325_TOUCH == 1
void ili9325_ts_Init(uint16_t DeviceAddr)
{
  if((Is_ili9325_Initialized & ILI9325_IO_INITIALIZED) == 0)
    LCD_IO_Init();
  Is_ili9325_Initialized |= ILI9325_IO_INITIALIZED;
}

//-----------------------------------------------------------------------------
uint8_t ili9325_ts_DetectTouch(uint16_t DeviceAddr)
{
  static uint8_t tp = 0;
  int32_t x1, x2, y1, y2, z11, z12, z21, z22, i, tpr;

  #if  ILI9325_MULTITASK_MUTEX == 1
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
        #if  ILI9325_MULTITASK_MUTEX == 1
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
    // ha sokadik probára sem sikerült stabil koordinátát kiolvasni -> nincs változás
  }
  else
    tp = 0; // nincs lenyomva

  #if  ILI9325_MULTITASK_MUTEX == 1
  io_ts_busy = 0;
  #endif

  return tp;
}

//-----------------------------------------------------------------------------
void ili9325_ts_GetXY(uint16_t DeviceAddr, uint16_t *X, uint16_t *Y)
{
  *X = tx,
  *Y = ty;
}
#endif // #if ILI9325_TOUCH == 1
