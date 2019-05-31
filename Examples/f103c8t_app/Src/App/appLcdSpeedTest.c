#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "main.h"

#include "lcd.h"
#include "bmp.h"

// BSP_LCD_xx függvények
#include "stm32_adafruit_lcd.h"

extern LCD_DrvTypeDef  *lcd_drv;

#define  Delay(t)     HAL_Delay(t)
#define  GetTime()    HAL_GetTick()

// #define  Delay(t)     osDelay(t)
// #define  GetTime()    osKernelSysTick()

// 16bites szin elöállitása RGB (ill. BGR) összetevökböl
#define RGB888TORGB565(r, g, b)  ((r & 0b11111000) << 8 | (g & 0b11111100) << 3 | b >> 3)
#define RGB888TOBGR565(r, g, b)  (r >> 3 | (g & 0b11111100) << 3 | (b & 0b11111000) << 8)

uint32_t ClearTest(void)
{
  uint32_t ctStartT = GetTime();
  BSP_LCD_Clear(LCD_COLOR_BLACK);
  return(GetTime() - ctStartT);
}

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
    BSP_LCD_SetTextColor(random() % 0xFFFF);
    BSP_LCD_DrawLine(x1, y1, x2, y2);
  }
  return(GetTime() - ctStartT);
}

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
    BSP_LCD_SetTextColor(random() % 0xFFFF);
    BSP_LCD_FillRect(x, y, w, h);
  }
  return(GetTime() - ctStartT);
}

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
    BSP_LCD_SetTextColor(random() % 0xFFFF);
    BSP_LCD_SetBackColor(random() % 0xFFFF);

    c = random() % 96 + ' ';
    BSP_LCD_DisplayChar(x, y, c);
  }
  return(GetTime() - ctStartT);
}

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
    BSP_LCD_SetTextColor(c);
    BSP_LCD_DrawCircle(x, y, r);
  }
  return(GetTime() - ctStartT);
}

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
    BSP_LCD_SetTextColor(c);
    BSP_LCD_FillCircle(x, y, r);
  }
  return(GetTime() - ctStartT);
}

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
    BSP_LCD_SetTextColor(c_rgb565);
    BSP_LCD_DrawVLine(x, 0, ys >> 2);

    c_rgb565 = RGB888TORGB565(cy, 0, 0);
    BSP_LCD_SetTextColor(c_rgb565);
    BSP_LCD_DrawVLine(x, ys >> 2, ys >> 2);

    c_rgb565 = RGB888TORGB565(0, cy, 0);
    BSP_LCD_SetTextColor(c_rgb565);
    BSP_LCD_DrawVLine(x, ys >> 1, ys >> 2);

    c_rgb565 = RGB888TORGB565(0, 0, cy);
    BSP_LCD_SetTextColor(c_rgb565);
    BSP_LCD_DrawVLine(x, (ys >> 1) + (ys >> 2), ys >> 2);
  }
  return(GetTime() - ctStartT);
}

uint32_t BitmapTest(uint32_t n)
{
  extern const BITMAPSTRUCT beer_60x100_16;
  uint16_t x, y;

  uint32_t ctStartT = GetTime();
  for(uint32_t i = 0; i < n; i++)
  {
    x = random() % (BSP_LCD_GetXSize() - beer_60x100_16.infoHeader.biWidth);
    y = random() % (BSP_LCD_GetYSize() - beer_60x100_16.infoHeader.biHeight);
    BSP_LCD_DrawBitmap(x, y, (uint8_t *)&beer_60x100_16);
  }
  return(GetTime() - ctStartT);
}

uint16_t bitmap[256];
void LcdReadPixelTest(void)
{
  uint16_t x, y;

  for(y = 0; y < 16; y++)
    for(x = 0; x < 16; x++)
      bitmap[y * 16 + x] = BSP_LCD_ReadPixel(x, y);

  for(y = 16; y < BSP_LCD_GetYSize(); y+=16)
    for(x = 16; x < BSP_LCD_GetXSize(); x+=16)
      BSP_LCD_DrawRGB16Image(x, y, 16, 16, &bitmap[0]);
}

void LcdReadImageTest(void)
{
  uint16_t x, y;

  BSP_LCD_ReadRGB16Image(0, 0, 16, 16, &bitmap[0]);

  for(y = 16; y < BSP_LCD_GetYSize(); y+=16)
    for(x = 16; x < BSP_LCD_GetXSize(); x+=16)
      BSP_LCD_DrawRGB16Image(x, y, 16, 16, &bitmap[0]);
}

void mainApp(void)
{
  uint32_t t;

  HAL_Delay(300);

  BSP_LCD_Init();

  t = random();

  HAL_Delay(1000);
  printf("Display ID = %X\r\n", (unsigned int)BSP_LCD_ReadID());
  HAL_Delay(1000);

  while(1)
  {
    _impure_ptr->_r48->_rand_next = 0;

    t = ClearTest();
    printf("Clear Test: %d ms\r\n", (int)t);
    t = PixelTest(100000);
    printf("Pixel Test: %d ms\r\n", (int)t);
    Delay(1000);

    BSP_LCD_Clear(LCD_COLOR_BLACK);
    t = LineTest(1000);
    printf("Line Test: %d ms\r\n", (int)t);
    Delay(1000);
    LcdReadPixelTest();
    Delay(1000);
    LcdReadImageTest();
    Delay(1000);

    BSP_LCD_Clear(LCD_COLOR_BLACK);
    t = FillRectTest(250);
    printf("Fill Rect Test: %d ms\r\n", (int)t);
    Delay(1000);

    BSP_LCD_Clear(LCD_COLOR_BLACK);
    t = CircleTest(1000);
    printf("Circle Test: %d ms\r\n", (int)t);
    Delay(1000);

    BSP_LCD_Clear(LCD_COLOR_BLACK);
    t = FillCircleTest(250);
    printf("Fill Circle Test: %d ms\r\n", (int)t);
    Delay(1000);
    LcdReadPixelTest();
    Delay(1000);
    LcdReadImageTest();
    Delay(1000);

    BSP_LCD_Clear(LCD_COLOR_BLACK);
    t = CharTest(5000);
    printf("Char Test: %d ms\r\n", (int)t);
    Delay(1000);

    BSP_LCD_Clear(LCD_COLOR_BLACK);
    t = BitmapTest(100);
    printf("Bitmap Test: %d ms\r\n", (int)t);
    Delay(1000);
    LcdReadPixelTest();
    Delay(1000);
    LcdReadImageTest();
    Delay(1000);

    BSP_LCD_Clear(LCD_COLOR_BLACK);
    t = ColorTest();
    printf("Color Test: %d ms\r\n", (int)t);
    Delay(3000);

    BSP_LCD_DisplayOff();
    Delay(1000);
    BSP_LCD_DisplayOn();
    Delay(1000);

    printf("\r\n");
  }
}
