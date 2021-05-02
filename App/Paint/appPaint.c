#include <stdlib.h>
#include "main.h"

/* BSP LCD driver */
#include "stm32_adafruit_lcd.h"

/* BSP TS driver */
#include "stm32_adafruit_ts.h"

#ifdef osCMSIS
void StartDefaultTask(void const * argument)
#else
void mainApp(void)
#endif
{
  TS_StateTypeDef ts;
  uint16_t boxsize;
  uint16_t oldcolor, currentcolor;

  BSP_LCD_Init();
  BSP_TS_Init(BSP_LCD_GetXSize(), BSP_LCD_GetYSize());
  BSP_LCD_Clear(LCD_COLOR_BLACK);
  boxsize = BSP_LCD_GetXSize() / 6;

  BSP_LCD_SetTextColor(LCD_COLOR_RED);
  BSP_LCD_FillRect(0, 0, boxsize, boxsize);
  BSP_LCD_SetTextColor(LCD_COLOR_YELLOW);
  BSP_LCD_FillRect(boxsize, 0, boxsize, boxsize);
  BSP_LCD_SetTextColor(LCD_COLOR_GREEN);
  BSP_LCD_FillRect(boxsize * 2, 0, boxsize, boxsize);
  BSP_LCD_SetTextColor(LCD_COLOR_CYAN);
  BSP_LCD_FillRect(boxsize * 3, 0, boxsize, boxsize);
  BSP_LCD_SetTextColor(LCD_COLOR_BLUE);
  BSP_LCD_FillRect(boxsize * 4, 0, boxsize, boxsize);
  BSP_LCD_SetTextColor(LCD_COLOR_MAGENTA);
  BSP_LCD_FillRect(boxsize * 5, 0, boxsize, boxsize);
  BSP_LCD_SetTextColor(LCD_COLOR_WHITE);

  BSP_LCD_DrawRect(0, 0, boxsize, boxsize);
  currentcolor = LCD_COLOR_RED;

  while(1)
  {
    BSP_TS_GetState(&ts);
    if(ts.TouchDetected)
    {
      if(ts.Y < boxsize)
      {
        oldcolor = currentcolor;

        BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
        if (ts.X < boxsize)
        {
          currentcolor = LCD_COLOR_RED;
          BSP_LCD_DrawRect(0, 0, boxsize, boxsize);
        }
        else if (ts.X < boxsize * 2)
        {
          currentcolor = LCD_COLOR_YELLOW;
          BSP_LCD_DrawRect(boxsize, 0, boxsize, boxsize);
        }
        else if (ts.X < boxsize * 3)
        {
          currentcolor = LCD_COLOR_GREEN;
          BSP_LCD_DrawRect(boxsize*2, 0, boxsize, boxsize);
        }
        else if (ts.X < boxsize * 4)
        {
          currentcolor = LCD_COLOR_CYAN;
          BSP_LCD_DrawRect(boxsize*3, 0, boxsize, boxsize);
        }
        else if (ts.X < boxsize * 5)
        {
          currentcolor = LCD_COLOR_BLUE;
          BSP_LCD_DrawRect(boxsize*4, 0, boxsize, boxsize);
        }
        else if (ts.X < boxsize * 6)
        {
          currentcolor = LCD_COLOR_MAGENTA;
          BSP_LCD_DrawRect(boxsize*5, 0, boxsize, boxsize);
        }

        if (oldcolor != currentcolor)
        {
          BSP_LCD_SetTextColor(oldcolor);
          if (oldcolor == LCD_COLOR_RED)
            BSP_LCD_FillRect(0, 0, boxsize, boxsize);
          if (oldcolor == LCD_COLOR_YELLOW)
            BSP_LCD_FillRect(boxsize, 0, boxsize, boxsize);
          if (oldcolor == LCD_COLOR_GREEN)
            BSP_LCD_FillRect(boxsize * 2, 0, boxsize, boxsize);
          if (oldcolor == LCD_COLOR_CYAN)
            BSP_LCD_FillRect(boxsize * 3, 0, boxsize, boxsize);
          if (oldcolor == LCD_COLOR_BLUE)
            BSP_LCD_FillRect(boxsize * 4, 0, boxsize, boxsize);
          if (oldcolor == LCD_COLOR_MAGENTA)
            BSP_LCD_FillRect(boxsize * 5, 0, boxsize, boxsize);
        }
      }
      else
      {
        BSP_LCD_DrawPixel(ts.X, ts.Y, currentcolor);
      }
    }
    HAL_Delay(1);
  }
}
