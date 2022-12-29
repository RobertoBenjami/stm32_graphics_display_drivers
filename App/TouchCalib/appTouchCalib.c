/*
 * touch_calibrate.c
 *
 *  Created on: 2021.04.28
 *      Author: Benjami
 *  Lcd touchscreen calibration program
 *      output: printf...
 */

#define CALIBBOXSIZE          6
#define CALIBBOXPOS          15

#define CALIBDELAY          500
#define TOUCHDELAY           50
#define PRINTDELAY           50

/* overflow limit (cindex 1, 2, 4, 5 and cindex 3, 6) */
#define MAXCINT1245      262144
#define MAXCINT36    1073741824

#include <stdio.h>
#include "main.h"
#include "ts.h"
#include "stm32_adafruit_lcd.h"
#include "stm32_adafruit_ts.h"

extern  TS_DrvTypeDef         *ts_drv;

#ifdef  osCMSIS
#define Delay(t)              osDelay(t)
#define GetTime()             osKernelSysTick()
#else
#define Delay(t)              HAL_Delay(t)
#define GetTime()             HAL_GetTick()
#endif

//-----------------------------------------------------------------------------
void touchcalib_drawBox(int32_t x, int32_t y, uint16_t cl)
{
  BSP_LCD_SetTextColor(cl);
  BSP_LCD_DrawRect(x - CALIBBOXSIZE / 2, y - CALIBBOXSIZE / 2, CALIBBOXSIZE, CALIBBOXSIZE);
}

//-----------------------------------------------------------------------------
// dx0..dy2 : display coordinates
// tx0..tx2 : touchscreen coordinates
void cindexgen(int32_t dx0, int32_t dy0, int32_t dx1, int32_t dy1, int32_t dx2, int32_t dy2,
               int32_t tx0, int32_t ty0, int32_t tx1, int32_t ty1, int32_t tx2, int32_t ty2)
{
  long long int i0, i1, i2, i3, i4, i5, i6, i1t, i2t, i3t, i4t, i5t, i6t;
  int idv = 1, d;
  i0 = (tx0-tx2)*(ty1-ty2) - (tx1-tx2)*(ty0-ty2);
  i1 = (dx0-dx2)*(ty1-ty2) - (dx1-dx2)*(ty0-ty2);
  i2 = (tx0-tx2)*(dx1-dx2) - (dx0-dx2)*(tx1-tx2);
  i3 = (long long int)ty0*(tx2*dx1 - tx1*dx2) + (long long int)ty1*(tx0*dx2 - tx2*dx0) + (long long int)ty2*(tx1*dx0 - tx0*dx1);
  i4 = (dy0-dy2)*(ty1-ty2) - (dy1-dy2)*(ty0-ty2);
  i5 = (tx0-tx2)*(dy1-dy2) - (dy0-dy2)*(tx1-tx2);
  i6 = (long long int)ty0*(tx2*dy1 - tx1*dy2) + (long long int)ty1*(tx0*dy2 - tx2*dy0) + (long long int)ty2*(tx1*dy0 - tx0*dy1);
  i1t = i1; i2t = i2; i3t = i3; i4t = i4; i5t = i5; i6t = i6;
  do
  {
    d = 0;
    if((i1t >= MAXCINT1245) || (i1t < -MAXCINT1245))
    {
      d = 1;
      i1t /= 2;
    }
    if((i2t >= MAXCINT1245) || (i2t < -MAXCINT1245))
    {
      d = 1;
      i2t /= 2;
    }
    if((i4t >= MAXCINT1245) || (i4t < -MAXCINT1245))
    {
      d = 1;
      i4t /= 2;
    }
    if((i5t >= MAXCINT1245) || (i5t < -MAXCINT1245))
    {
      d = 1;
      i5t /= 2;
    }
    if((i3t >= MAXCINT36) || (i3t < -MAXCINT36))
    {
      d = 1;
      i3t /= 2;
    }
    if((i6t >= MAXCINT36) || (i6t < -MAXCINT36))
    {
      d = 1;
      i6t /= 2;
    }
    if(d)
      idv *= 2;
  }while(d);

  if(idv > 1)
  {
    i0 /= idv; i1 /= idv; i2 /= idv; i3 /= idv; i4 /= idv; i5 /= idv; i6 /= idv;
  }

  Delay(10);
  printf("{%d, ", (int)i0);
  Delay(10);
  printf("%d, ", (int)i1);
  Delay(10);
  printf("%d, ", (int)i2);
  Delay(10);
  printf("%d, ", (int)i3);
  Delay(10);
  printf("%d, ", (int)i4);
  Delay(10);
  printf("%d, ", (int)i5);
  Delay(10);
  printf("%d}\r\n", (int)i6);
  /*printf("idv:%d\r\n", idv);*/
}

//-----------------------------------------------------------------------------
#ifdef osCMSIS
void StartDefaultTask(void const * argument)
#else
void mainApp(void)
#endif
{
  uint16_t x, y;
  int32_t tx0, ty0, tx1, ty1, tx2, ty2;

  printf("\r\nPlease: lcd ORIENTATION setting the 0 for correct result\r\n");

  BSP_LCD_Init();
  BSP_TS_Init(BSP_LCD_GetXSize(), BSP_LCD_GetYSize());
  BSP_LCD_Clear(LCD_COLOR_BLACK);

  int32_t dx0 = 20;
  int32_t dy0 = 20;
  int32_t dx1 = BSP_LCD_GetXSize() >> 1;
  int32_t dy1 = BSP_LCD_GetYSize() - 1 - 20;
  int32_t dx2 = BSP_LCD_GetXSize() - 1 - 20;
  int32_t dy2 = BSP_LCD_GetYSize() >> 1;

  #if 0
  for(x = 10; x < BSP_LCD_GetXSize(); x+=10)
    for(y = 10; y < BSP_LCD_GetYSize(); y+=10)
      BSP_LCD_DrawPixel(x, y, LCD_COLOR_GREY);
  #endif

  touchcalib_drawBox(dx0, dy0, LCD_COLOR_YELLOW);
  Delay(CALIBDELAY);
  while(!ts_drv->DetectTouch(0))
    Delay(TOUCHDELAY);
  ts_drv->GetXY(0, &x, &y);
  tx0 = x; ty0 = y;
  while(ts_drv->DetectTouch(0))
    Delay(TOUCHDELAY);

  touchcalib_drawBox(dx0, dy0, LCD_COLOR_GRAY);
  touchcalib_drawBox(dx1, dy1, LCD_COLOR_YELLOW);
  Delay(CALIBDELAY);

  while(!ts_drv->DetectTouch(0))
    Delay(TOUCHDELAY);
  ts_drv->GetXY(0, &x, &y);
  tx1 = x; ty1 = y;
  while(ts_drv->DetectTouch(0))
    Delay(TOUCHDELAY);
  Delay(CALIBDELAY);

  touchcalib_drawBox(dx1, dy1, LCD_COLOR_GRAY);
  touchcalib_drawBox(dx2, dy2, LCD_COLOR_YELLOW);
  Delay(CALIBDELAY);

  while(!ts_drv->DetectTouch(0))
    Delay(TOUCHDELAY);
  ts_drv->GetXY(0, &x, &y);
  tx2 = x; ty2 = y;
  while(ts_drv->DetectTouch(0))
    Delay(TOUCHDELAY);

  Delay(PRINTDELAY);
  /*printf("%d %d, %d %d, %d %d : %d %d, %d %d, %d %d\r\n", (int)dx0, (int)dy0, (int)dx1, (int)dy1, (int)dx2, (int)dy2,
                                                          (int)tx0, (int)ty0, (int)tx1, (int)ty1, (int)tx2, (int)ty2);*/

  Delay(PRINTDELAY);
  printf("#define  TS_CINDEX_0        ");
  Delay(50);
  cindexgen(dx0, dy0,
            dx1, dy1,
            dx2, dy2,
            tx0, ty0, tx1, ty1, tx2, ty2);

  Delay(PRINTDELAY);
  printf("#define  TS_CINDEX_1        ");
  Delay(PRINTDELAY);
  cindexgen(dy0, BSP_LCD_GetXSize() - 1 - dx0,
            dy1, BSP_LCD_GetXSize() - 1 - dx1,
            dy2, BSP_LCD_GetXSize() - 1 - dx2,
            tx0, ty0, tx1, ty1, tx2, ty2);
  Delay(PRINTDELAY);
  printf("#define  TS_CINDEX_2        ");

  Delay(50);
  cindexgen(BSP_LCD_GetXSize() - 1 - dx0, BSP_LCD_GetYSize() - 1 - dy0,
            BSP_LCD_GetXSize() - 1 - dx1, BSP_LCD_GetYSize() - 1 - dy1,
            BSP_LCD_GetXSize() - 1 - dx2, BSP_LCD_GetYSize() - 1 - dy2,
            tx0, ty0, tx1, ty1, tx2, ty2);

  Delay(PRINTDELAY);
  printf("#define  TS_CINDEX_3        ");
  Delay(PRINTDELAY);
  cindexgen(BSP_LCD_GetYSize() - 1 - dy0, dx0,
            BSP_LCD_GetYSize() - 1 - dy1, dx1,
            BSP_LCD_GetYSize() - 1 - dy2, dx2,
            tx0, ty0, tx1, ty1, tx2, ty2);

  while(1);
}
