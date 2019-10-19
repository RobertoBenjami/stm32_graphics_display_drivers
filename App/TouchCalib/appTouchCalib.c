/*
 * touch_calibrate.c
 *
 *  Created on: 2019. febr. 6.
 *      Author: Benjami
 *  Lcd touchscreen calibration program
 *      output: printf...
 */

#define CALIBBOXSIZE          6
#define CALIBBOXPOS          15
#define CALIBDELAY          500

#include <stdio.h>
#include "main.h"
#include "ts.h"
#include "stm32_adafruit_lcd.h"
#include "stm32_adafruit_ts.h"

extern   TS_DrvTypeDef     *ts_drv;
extern   int32_t            ts_cindex[7];

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
void touchcalib(void)
{
  uint16_t x, y;
  int32_t tx0, ty0, tx1, ty1, tx2, ty2;

  int32_t dx0 = 20;
  int32_t dy0 = 20;
  int32_t dx1 = BSP_LCD_GetXSize() >> 1;
  int32_t dy1 = BSP_LCD_GetYSize() - 1 - 20;
  int32_t dx2 = BSP_LCD_GetXSize() - 1 - 20;
  int32_t dy2 = BSP_LCD_GetYSize() >> 1;

  BSP_LCD_Clear(LCD_COLOR_BLACK);

  #if 0
  for(x = 10; x < BSP_LCD_GetXSize(); x+=10)
    for(y = 10; y < BSP_LCD_GetYSize(); y+=10)
      BSP_LCD_DrawPixel(x, y, LCD_COLOR_GREY);
  #endif

  touchcalib_drawBox(dx0, dy0, LCD_COLOR_YELLOW);
  while(!ts_drv->DetectTouch(0))
    Delay(50);
  ts_drv->GetXY(0, &x, &y);
  tx0 = x; ty0 = y;
  while(ts_drv->DetectTouch(0))
  Delay(CALIBDELAY);
  touchcalib_drawBox(dx0, dy0, LCD_COLOR_GRAY);

  touchcalib_drawBox(dx1, dy1, LCD_COLOR_YELLOW);
  while(!ts_drv->DetectTouch(0))
  Delay(50);
  ts_drv->GetXY(0, &x, &y);
  tx1 = x; ty1 = y;
  while(ts_drv->DetectTouch(0))
  Delay(CALIBDELAY);
  touchcalib_drawBox(dx1, dy1, LCD_COLOR_GRAY);

  touchcalib_drawBox(dx2, dy2, LCD_COLOR_YELLOW);
  while(!ts_drv->DetectTouch(0))
    Delay(50);
  ts_drv->GetXY(0, &x, &y);
  tx2 = x; ty2 = y;

  while(ts_drv->DetectTouch(0))
  HAL_Delay(CALIBDELAY);

  ts_cindex[0] = (tx0-tx2)*(ty1-ty2) - (tx1-tx2)*(ty0-ty2);
  ts_cindex[1] = (dx0-dx2)*(ty1-ty2) - (dx1-dx2)*(ty0-ty2);
  ts_cindex[2] = (tx0-tx2)*(dx1-dx2) - (dx0-dx2)*(tx1-tx2);
  ts_cindex[3] = ty0*(tx2*dx1 - tx1*dx2) + ty1*(tx0*dx2 - tx2*dx0) + ty2*(tx1*dx0 - tx0*dx1);
  ts_cindex[4] = (dy0-dy2)*(ty1-ty2) - (dy1-dy2)*(ty0-ty2);
  ts_cindex[5] = (tx0-tx2)*(dy1-dy2) - (dy0-dy2)*(tx1-tx2);
  ts_cindex[6] = ty0*(tx2*dy1 - tx1*dy2) + ty1*(tx0*dy2 - tx2*dy0) + ty2*(tx1*dy0 - tx0*dy1);

  #if 1
  printf("%d, ", (int)ts_cindex[0]);
  printf("%d, ", (int)ts_cindex[1]);
  printf("%d, ", (int)ts_cindex[2]);
  printf("%d, ", (int)ts_cindex[3]);
  printf("%d, ", (int)ts_cindex[4]);
  printf("%d, ", (int)ts_cindex[5]);
  printf("%d\r\n", (int)ts_cindex[6]);
  #endif

  #if 0
  char  s[16];
  BSP_LCD_SetBackColor(LCD_COLOR_BLACK);
  sprintf(s, "c0:%d", (int)*p_cindex);
  BSP_LCD_DisplayStringAtLine(1, (uint8_t *)&s);
  sprintf(s, "c1:%d", (int)*(p_cindex + 1));
  BSP_LCD_DisplayStringAtLine(2, (uint8_t *)&s);
  sprintf(s, "c2:%d", (int)*(p_cindex + 2));
  BSP_LCD_DisplayStringAtLine(3, (uint8_t *)&s);
  sprintf(s, "c3:%d", (int)*(p_cindex + 3));
  BSP_LCD_DisplayStringAtLine(4, (uint8_t *)&s);
  sprintf(s, "c4:%d", (int)*(p_cindex + 4));
  BSP_LCD_DisplayStringAtLine(5, (uint8_t *)&s);
  sprintf(s, "c5:%d", (int)*(p_cindex + 5));
  BSP_LCD_DisplayStringAtLine(6, (uint8_t *)&s);
  sprintf(s, "c6:%d", (int)*(p_cindex + 6));
  BSP_LCD_DisplayStringAtLine(7, (uint8_t *)&s);
  osDelay(1000);

  while(!ts_drv->DetectTouch(0))
    Delay(50);
  while(ts_drv->DetectTouch(0))
    Delay(50);
  #else
  Delay(1000);
  #endif
}

//-----------------------------------------------------------------------------
// dx0..dy2 : képernyö koordináták
// tx0..tx2 : touchscreen által visszaadott koordináták
void cindexgen(int32_t dx0, int32_t dy0, int32_t dx1, int32_t dy1, int32_t dx2, int32_t dy2,
               int32_t tx0, int32_t ty0, int32_t tx1, int32_t ty1, int32_t tx2, int32_t ty2)
{
  Delay(10);
  printf("{%d, ", (int)((tx0-tx2)*(ty1-ty2) - (tx1-tx2)*(ty0-ty2)));
  Delay(10);
  printf("%d, ", (int)((dx0-dx2)*(ty1-ty2) - (dx1-dx2)*(ty0-ty2)));
  Delay(10);
  printf("%d, ", (int)((tx0-tx2)*(dx1-dx2) - (dx0-dx2)*(tx1-tx2)));
  Delay(10);
  printf("%d, ", (int)(ty0*(tx2*dx1 - tx1*dx2) + ty1*(tx0*dx2 - tx2*dx0) + ty2*(tx1*dx0 - tx0*dx1)));
  Delay(10);
  printf("%d, ", (int)((dy0-dy2)*(ty1-ty2) - (dy1-dy2)*(ty0-ty2)));
  Delay(10);
  printf("%d, ", (int)((tx0-tx2)*(dy1-dy2) - (dy0-dy2)*(tx1-tx2)));
  Delay(10);
  printf("%d}\r\n", (int)(ty0*(tx2*dy1 - tx1*dy2) + ty1*(tx0*dy2 - tx2*dy0) + ty2*(tx1*dy0 - tx0*dy1)));
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
  while(!ts_drv->DetectTouch(0))
    Delay(50);
  ts_drv->GetXY(0, &x, &y);
  tx0 = x; ty0 = y;
  while(ts_drv->DetectTouch(0))
  HAL_Delay(CALIBDELAY);
  touchcalib_drawBox(dx0, dy0, LCD_COLOR_GRAY);

  touchcalib_drawBox(dx1, dy1, LCD_COLOR_YELLOW);
  while(!ts_drv->DetectTouch(0))
    Delay(50);
  ts_drv->GetXY(0, &x, &y);
  tx1 = x; ty1 = y;
  while(ts_drv->DetectTouch(0))
  HAL_Delay(CALIBDELAY);
  touchcalib_drawBox(dx1, dy1, LCD_COLOR_GRAY);

  touchcalib_drawBox(dx2, dy2, LCD_COLOR_YELLOW);
  while(!ts_drv->DetectTouch(0))
    Delay(10);
  ts_drv->GetXY(0, &x, &y);
  tx2 = x; ty2 = y;
  while(ts_drv->DetectTouch(0))
  HAL_Delay(CALIBDELAY);

  Delay(10);
  printf("#define  TS_CINDEX_0        ");
  Delay(10);
  cindexgen(dx0, dy0,
            dx1, dy1,
            dx2, dy2,
            tx0, ty0, tx1, ty1, tx2, ty2);

  Delay(10);
  printf("#define  TS_CINDEX_1        ");
  Delay(10);
  cindexgen(dy0, BSP_LCD_GetXSize() - 1 - dx0,
            dy1, BSP_LCD_GetXSize() - 1 - dx1,
            dy2, BSP_LCD_GetXSize() - 1 - dx2,
            tx0, ty0, tx1, ty1, tx2, ty2);
  Delay(10);
  printf("#define  TS_CINDEX_2        ");

  Delay(10);
  cindexgen(BSP_LCD_GetXSize() - 1 - dx0, BSP_LCD_GetYSize() - 1 - dy0,
            BSP_LCD_GetXSize() - 1 - dx1, BSP_LCD_GetYSize() - 1 - dy1,
            BSP_LCD_GetXSize() - 1 - dx2, BSP_LCD_GetYSize() - 1 - dy2,
            tx0, ty0, tx1, ty1, tx2, ty2);

  Delay(10);
  printf("#define  TS_CINDEX_3        ");
  Delay(10);
  cindexgen(BSP_LCD_GetYSize() - 1 - dy0, dx0,
            BSP_LCD_GetYSize() - 1 - dy1, dx1,
            BSP_LCD_GetYSize() - 1 - dy2, dx2,
            tx0, ty0, tx1, ty1, tx2, ty2);

  while(1);
}
