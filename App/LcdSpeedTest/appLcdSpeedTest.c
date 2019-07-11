/* LCD sebesség test program
 * Az eredmény a printf-el lesz kiiratva.
 * A printf átirányithato SWO ra, vagy soros portra
 *
 * készitö: Roberto Benjami
 * verzio:  2019.05
 */

/* Freertos alatt lehetséges Task02 teljesitményének mérése
   - 0: mérés ki
   - 1: mérés be */
#define POWERMETER    0

/* Tesztfoto */
#define rombitmap  beer_60x100_16

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "main.h" // a main.h-ba illesszük be az aktuális #include "stm32fxxx_hal.h"-t, freertos esetén pedig #include "cmsis_os.h"-t is!

#include "lcd.h"
#include "bmp.h"

/* BSP_LCD_... */
#include "stm32_adafruit_lcd.h"

//-----------------------------------------------------------------------------
// freertos vs HAL
#ifdef  osCMSIS
#define Delay(t)              osDelay(t)
#define GetTime()             osKernelSysTick()

volatile uint32_t task02_count = 0, task02_reset = 0, task02_run = 0;
volatile uint32_t task02_powermin = 0, task02_powermax = 0, task02_poweravg = 0;

#if     POWERMETER == 1
#define POWERMETER_RUN        task02_run = 1
#define POWERMETER_STOP       task02_run = 0
#define POWERMETER_PRINT      {             \
  if(task02_powermin != 0xFFFFFFFF)         \
    printf("Pp min:%d, max:%d, avg:%d\r\n", \
           (unsigned int)task02_powermin,   \
           (unsigned int)task02_powermax,   \
           (unsigned int)task02_poweravg);  \
  task02_reset = 1;                         }
#endif

osTimerId myTimer01Handle;
void cbTimer(void const * argument);

#else
#define Delay(t)              HAL_Delay(t)
#define GetTime()             HAL_GetTick()
#endif

#ifndef POWERMETER_RUN
#define POWERMETER_RUN
#define POWERMETER_STOP
#define POWERMETER_PRINT
#endif

// 16bites szin elöállitása RGB (ill. BGR) összetevökböl
#define RGB888TORGB565(r, g, b) ((r & 0b11111000) << 8 | (g & 0b11111100) << 3 | b >> 3)
#define RGB888TOBGR565(r, g, b) (r >> 3 | (g & 0b11111100) << 3 | (b & 0b11111000) << 8)

//-----------------------------------------------------------------------------
#if LCD_REVERSE16 == 0
#define RD(a)                 a
#endif

/* Konstans szám bájtjainak cseréje, változó bájtjainak cseréje */
#if LCD_REVERSE16 == 1
#define RD(a)                 __REVSH(a)
#endif

extern const BITMAPSTRUCT rombitmap;
uint16_t bitmap[60 * 100];

//-----------------------------------------------------------------------------
uint32_t ClearTest(void)
{
  uint32_t ctStartT = GetTime();
  BSP_LCD_Clear(LCD_COLOR_BLACK);
  return(GetTime() - ctStartT);
}

//-----------------------------------------------------------------------------
uint32_t PixelTest(uint32_t n)
{
  uint16_t c, x, y;

  uint32_t ctStartT = GetTime();
  for(uint32_t i = 0; i < n; i++)
  {
    x = random() % BSP_LCD_GetXSize();
    y = random() % BSP_LCD_GetYSize();
    c = random() % 0xFFFF;
    BSP_LCD_DrawPixel(x, y, c);
  }
  return(GetTime() - ctStartT);
}

//-----------------------------------------------------------------------------
uint32_t LineTest(uint32_t n)
{
  uint16_t x1, y1, x2, y2;

  uint32_t ctStartT = GetTime();
  for(uint32_t i = 0; i < n; i++)
  {
    x1 = random() % BSP_LCD_GetXSize();
    y1 = random() % BSP_LCD_GetYSize();
    x2 = random() % BSP_LCD_GetXSize();
    y2 = random() % BSP_LCD_GetYSize();
    BSP_LCD_SetTextColor(RD(random() % 0xFFFF));
    BSP_LCD_DrawLine(x1, y1, x2, y2);
  }
  return(GetTime() - ctStartT);
}

//-----------------------------------------------------------------------------
uint32_t FillRectTest(uint32_t n)
{
  uint16_t x, y, w, h;

  uint32_t ctStartT = GetTime();
  for(uint32_t i = 0; i < n; i++)
  {
    w = random() % (BSP_LCD_GetXSize() >> 1);
    h = random() % (BSP_LCD_GetYSize() >> 1);
    x = random() % (BSP_LCD_GetXSize() - w);
    y = random() % (BSP_LCD_GetYSize() - h);
    BSP_LCD_SetTextColor(RD(random() % 0xFFFF));
    BSP_LCD_FillRect(x, y, w, h);
  }
  return(GetTime() - ctStartT);
}

//-----------------------------------------------------------------------------
uint32_t CharTest(uint32_t n)
{
  uint16_t x, y;
  uint8_t  c;
  sFONT * fp;

  uint32_t ctStartT = GetTime();
  for(uint32_t i = 0; i < n; i++)
  {
    c = random() % 5;
    if(c == 0)
      BSP_LCD_SetFont(&Font8);
    else if(c == 1)
      BSP_LCD_SetFont(&Font12);
    else if(c == 2)
      BSP_LCD_SetFont(&Font16);
    else if(c == 3)
      BSP_LCD_SetFont(&Font20);
    else if(c == 4)
      BSP_LCD_SetFont(&Font24);
    fp = BSP_LCD_GetFont();

    x = random() % (BSP_LCD_GetXSize() - fp->Width);
    y = random() % (BSP_LCD_GetYSize() - fp->Height);
    BSP_LCD_SetTextColor(RD(random() % 0xFFFF));
    BSP_LCD_SetBackColor(RD(random() % 0xFFFF));

    c = random() % 96 + ' ';
    BSP_LCD_DisplayChar(x, y, c);
  }
  return(GetTime() - ctStartT);
}

//-----------------------------------------------------------------------------
uint32_t CircleTest(uint32_t n)
{
  uint16_t c, x, y, r, rmax;

  rmax = BSP_LCD_GetXSize();
  if(rmax > BSP_LCD_GetYSize())
    rmax = BSP_LCD_GetYSize();
  rmax >>= 2;

  uint32_t ctStartT = GetTime();
  for(uint32_t i = 0; i < n; i++)
  {
    do
      r = random() % rmax;
    while(r == 0);

    x = random() % (BSP_LCD_GetXSize() - (r << 1)) + r;
    y = random() % (BSP_LCD_GetYSize() - (r << 1)) + r;
    c = random() % 0xFFFF;
    BSP_LCD_SetTextColor(RD(c));
    BSP_LCD_DrawCircle(x, y, r);
  }
  return(GetTime() - ctStartT);
}

//-----------------------------------------------------------------------------
uint32_t FillCircleTest(uint32_t n)
{
  uint16_t c, x, y, r, rmax;

  rmax = BSP_LCD_GetXSize();
  if(rmax > BSP_LCD_GetYSize())
    rmax = BSP_LCD_GetYSize();
  rmax >>= 2;

  uint32_t ctStartT = GetTime();
  for(uint32_t i = 0; i < n; i++)
  {
    do
      r = random() % rmax;
    while(r == 0);

    x = random() % (BSP_LCD_GetXSize() - (r << 1)) + r;
    y = random() % (BSP_LCD_GetYSize() - (r << 1)) + r;
    c = random() % 0xFFFF;
    BSP_LCD_SetTextColor(RD(c));
    BSP_LCD_FillCircle(x, y, r);
  }
  return(GetTime() - ctStartT);
}

//-----------------------------------------------------------------------------
uint32_t ColorTest(void)
{
  uint16_t c_rgb565, xs, ys;
  uint8_t  cy;

  uint32_t ctStartT = GetTime();
  xs = BSP_LCD_GetXSize();
  ys = BSP_LCD_GetYSize();
  for(uint16_t x = 0; x < xs; x++)
  {
    cy = (uint32_t)(x << 8) / xs;
    c_rgb565 = RGB888TORGB565(cy, cy, cy);
    BSP_LCD_SetTextColor(RD(c_rgb565));
    BSP_LCD_DrawVLine(x, 0, ys >> 2);

    c_rgb565 = RGB888TORGB565(cy, 0, 0);
    BSP_LCD_SetTextColor(RD(c_rgb565));
    BSP_LCD_DrawVLine(x, ys >> 2, ys >> 2);

    c_rgb565 = RGB888TORGB565(0, cy, 0);
    BSP_LCD_SetTextColor(RD(c_rgb565));
    BSP_LCD_DrawVLine(x, ys >> 1, ys >> 2);

    c_rgb565 = RGB888TORGB565(0, 0, cy);
    BSP_LCD_SetTextColor(RD(c_rgb565));
    BSP_LCD_DrawVLine(x, (ys >> 1) + (ys >> 2), ys >> 2);
  }
  return(GetTime() - ctStartT);
}

//-----------------------------------------------------------------------------
uint32_t BitmapTest(uint32_t n)
{
  extern const BITMAPSTRUCT beer_60x100_16;
  uint16_t x, y;

  uint32_t ctStartT = GetTime();
  for(uint32_t i = 0; i < n; i++)
  {
    x = random() % (BSP_LCD_GetXSize() - rombitmap.infoHeader.biWidth);
    y = random() % (BSP_LCD_GetYSize() - rombitmap.infoHeader.biHeight);
    BSP_LCD_DrawBitmap(x, y, (uint8_t *)&rombitmap);
  }
  return(GetTime() - ctStartT);
}

//-----------------------------------------------------------------------------
uint32_t ReadPixelTest(uint32_t n)
{
  uint16_t x, y, x0, y0, xsize, ysize;
  uint32_t error = 0;

  x0 = 20;
  y0 =  5;
  xsize = rombitmap.infoHeader.biWidth;
  ysize = rombitmap.infoHeader.biHeight;

  /* Draw bitmap */
  BSP_LCD_DrawBitmap(x0, y0, (uint8_t *)&rombitmap);

  /* Read bitmap (BSP_LCD_ReadPixel) */
  uint32_t ctStartT = GetTime();
  while(n--)
    for(y = 0; y < ysize; y++)
      for(x = 0; x < xsize; x++)
        bitmap[y * xsize + x] = BSP_LCD_ReadPixel(x0 + x, y0 + y);
  ctStartT = GetTime() - ctStartT;

  /* Check the read error */
  for(y = 0; y < ysize; y++)
    for(x = 0; x < xsize; x++)
    {
      if(bitmap[y * xsize + x] != rombitmap.data[(ysize - 1 - y) * xsize + x])
        error++;
    }

  if(error)
    printf("ReadPixelTest error: %d\r\n", (int)error);

  BSP_LCD_DrawRGB16Image(x0 + 45, y0 + 10, xsize, ysize, &bitmap[0]);
  return(ctStartT);
}

//-----------------------------------------------------------------------------
uint32_t ReadImageTest(uint32_t n)
{
  uint16_t x, y, x0, y0, xsize, ysize;
  uint32_t error = 0;

  x0 = 20;
  y0 =  5;
  xsize = rombitmap.infoHeader.biWidth;
  ysize = rombitmap.infoHeader.biHeight;

  /* Draw bitmap */
  BSP_LCD_DrawBitmap(x0, y0, (uint8_t *)&rombitmap);

  /* Read bitmap (BSP_LCD_ReadRGB16Image) */
  uint32_t ctStartT = GetTime();
  while(n--)
    BSP_LCD_ReadRGB16Image(x0, y0, xsize, ysize, &bitmap[0]);
  ctStartT = GetTime() - ctStartT;

  /* Check the read error */
  for(y = 0; y < ysize; y++)
    for(x = 0; x < xsize; x++)
    {
      if(bitmap[y * xsize + x] != rombitmap.data[(ysize - 1 - y) * xsize + x])
        error++;
    }

  if(error)
    printf("ReadImageTest error: %d\r\n", (int)error);

  BSP_LCD_DrawRGB16Image(x0 - 15, y0 + 20, xsize, ysize, &bitmap[0]);
  return(ctStartT);
}

//-----------------------------------------------------------------------------
#ifdef osCMSIS
void StartDefaultTask(void const * argument)
#else
void mainApp(void)
#endif
{
  uint32_t t;

  Delay(300);

  BSP_LCD_Init();

  t = random();

  Delay(100);
  printf("Display ID = %X\r\n", (unsigned int)BSP_LCD_ReadID());
  POWERMETER_RUN;
  Delay(100);
  POWERMETER_STOP;
  POWERMETER_PRINT;

  #if 0
  BSP_LCD_Clear(LCD_COLOR_BLACK);
  Delay(1000);
  t = ReadPixelTest(1);
  t = ReadImageTest(1);
  Delay(1000);
  #endif

  while(1)
  {
    _impure_ptr->_r48->_rand_next = 0;

    POWERMETER_RUN;
    t = ClearTest();
    POWERMETER_STOP;
    printf("Clear Test: %d ms\r\n", (int)t);
    POWERMETER_PRINT;
    Delay(1000);

    POWERMETER_RUN;
    t = PixelTest(100000);
    POWERMETER_STOP;
    printf("Pixel Test: %d ms\r\n", (int)t);
    POWERMETER_PRINT;
    Delay(1000);

    BSP_LCD_Clear(LCD_COLOR_BLACK);
    POWERMETER_RUN;
    t = LineTest(1000);
    POWERMETER_STOP;
    printf("Line Test: %d ms\r\n", (int)t);
    POWERMETER_PRINT;
    Delay(1000);

    BSP_LCD_Clear(LCD_COLOR_BLACK);
    POWERMETER_RUN;
    t = FillRectTest(250);
    POWERMETER_STOP;
    printf("Fill Rect Test: %d ms\r\n", (int)t);
    POWERMETER_PRINT;
    Delay(1000);

    BSP_LCD_Clear(LCD_COLOR_BLACK);
    POWERMETER_RUN;
    t = CircleTest(1000);
    POWERMETER_STOP;
    printf("Circle Test: %d ms\r\n", (int)t);
    POWERMETER_PRINT;
    Delay(1000);

    BSP_LCD_Clear(LCD_COLOR_BLACK);
    POWERMETER_RUN;
    t = FillCircleTest(250);
    POWERMETER_STOP;
    printf("Fill Circle Test: %d ms\r\n", (int)t);
    POWERMETER_PRINT;
    Delay(1000);

    BSP_LCD_Clear(LCD_COLOR_BLACK);
    POWERMETER_RUN;
    t = CharTest(5000);
    POWERMETER_STOP;
    printf("Char Test: %d ms\r\n", (int)t);
    POWERMETER_PRINT;
    Delay(1000);

    BSP_LCD_Clear(LCD_COLOR_BLACK);
    POWERMETER_RUN;
    t = BitmapTest(100);
    POWERMETER_STOP;
    printf("Bitmap Test: %d ms\r\n", (int)t);
    POWERMETER_PRINT;
    Delay(1000);

    BSP_LCD_Clear(LCD_COLOR_BLACK);
    POWERMETER_RUN;
    t = ReadPixelTest(20);
    POWERMETER_STOP;
    printf("ReadPixel Test: %d ms\r\n", (int)t);
    POWERMETER_PRINT;
    Delay(1000);

    BSP_LCD_Clear(LCD_COLOR_BLACK);
    POWERMETER_RUN;
    t = ReadImageTest(20);
    POWERMETER_STOP;
    printf("ReadImage Test: %d ms\r\n", (int)t);
    POWERMETER_PRINT;
    Delay(1000);

    BSP_LCD_Clear(LCD_COLOR_BLACK);
    POWERMETER_RUN;
    t = ColorTest();
    POWERMETER_STOP;
    printf("Color Test: %d ms\r\n", (int)t);
    POWERMETER_PRINT;
    Delay(3000);

    BSP_LCD_DisplayOff();
    Delay(1000);
    BSP_LCD_DisplayOn();
    Delay(1000);

    printf("\r\n");
  }
}

#ifdef osCMSIS

//-----------------------------------------------------------------------------
void StartTask02(void const * argument)
{
  osTimerDef(myTimer01, cbTimer);
  myTimer01Handle = osTimerCreate(osTimer(myTimer01), osTimerPeriodic, NULL);
  osTimerStart(myTimer01Handle, 100);
  for(;;)
  {
    task02_count++;
  }
}

//-----------------------------------------------------------------------------
void cbTimer(void const * argument)
{
  static uint64_t task02_power = 0;
  static uint32_t task02_measurenum = 1;
  uint32_t t32;

  if(task02_reset)
  {
    task02_reset = 0;
    task02_power = 0;
    task02_powermin = 0xFFFFFFFF;
    task02_powermax = 0;
    task02_poweravg = 0;
    task02_measurenum = 0;
    return;
  }

  t32 = task02_count;
  task02_count = 0;

  if(task02_run)
  {
    if(t32 < task02_powermin)
      task02_powermin = t32;

    if(t32 > task02_powermax)
      task02_powermax = t32;

    task02_power += t32;
    task02_measurenum++;
    task02_poweravg = task02_power / task02_measurenum;
  }
}
#endif
