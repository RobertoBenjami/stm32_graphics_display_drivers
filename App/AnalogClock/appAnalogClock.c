/* Analog clock program
 *
 * Roberto Benjami
 * v: 2019.11 (only stm32f4 now)
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "main.h" // a main.h-ba illesszük be az aktuális #include "stm32fxxx_hal.h"-t, freertos esetén pedig #include "cmsis_os.h"-t is!

#include "lcd.h"
#include "bmp.h"

/* BSP_LCD_... */
#include "stm32_adafruit_lcd.h"

#define  CLOCK_COLOR_BACKGROUND  LCD_COLOR_BLACK
#define  CLOCK_COLOR_FACE        RGB888TORGB565(30, 30, 30)
#define  CLOCK_COLOR_BORDER      LCD_COLOR_CYAN
#define  CLOCK_COLOR_NUMBERS     LCD_COLOR_GREEN
#define  CLOCK_COLOR_DIGITS      LCD_COLOR_WHITE
#define  CLOCK_COLOR_HP          LCD_COLOR_RED
#define  CLOCK_COLOR_MP          LCD_COLOR_BLUE
#define  CLOCK_COLOR_SP          LCD_COLOR_YELLOW

// CLOCK_SIZE... : 0..256
#define  CLOCK_SIZE_HP           120
#define  CLOCK_SIZE_MP           180
#define  CLOCK_SIZE_SP           220
#define  CLOCK_SIZE_NMBCIRC      240

// pointer form
#define  CLOCK_THICKNESS_HP      0.25
#define  CLOCK_THICKNESS_MP      0.15
#define  CLOCK_THICKNESS_SP      0.08
#define  CLOCK_SHAPE_HP          0.4
#define  CLOCK_SHAPE_MP          0.3
#define  CLOCK_SHAPE_SP          0.2

extern LCD_DrvTypeDef  *lcd_drv;
extern RTC_HandleTypeDef hrtc;

typedef union
{
  __packed struct
  {
    unsigned st:4;
    unsigned su:3;
    unsigned res1:1;
    unsigned mt:4;
    unsigned mu:3;
    unsigned res2:1;
    unsigned ht:4;
    unsigned hu:2;
  };
  uint32_t t;
}TIMEREG;

//-----------------------------------------------------------------------------
// freertos vs HAL
#ifdef  osCMSIS
#define Delay(t)              osDelay(t)
#define GetTime()             osKernelSysTick()
#else
#define Delay(t)              HAL_Delay(t)
#define GetTime()             HAL_GetTick()
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

//-----------------------------------------------------------------------------
#ifdef osCMSIS
void StartDefaultTask(void const * argument)
#else
void mainApp(void)
#endif
{
  uint32_t rx, ry, r;
  TIMEREG nowtime, lasttime;
  uint32_t sec, i;
  float fsec;
  char s[9] = "00:00:00";
  Point ap[12], lp[12];

  Delay(300);
  BSP_LCD_Init();

  rx = BSP_LCD_GetXSize() >> 1;
  ry = BSP_LCD_GetYSize() >> 1;
  r = rx < ry ? rx: ry;

  Delay(100);
  printf("Display ID = %X\r\n", (unsigned int)BSP_LCD_ReadID());

  BSP_LCD_Clear(CLOCK_COLOR_BACKGROUND);
  float hps = (r * CLOCK_SIZE_HP) / 256;
  float mps = (r * CLOCK_SIZE_MP) / 256;
  float sps = (r * CLOCK_SIZE_SP) / 256;
  float nps = (r * CLOCK_SIZE_NMBCIRC) / 256;

  BSP_LCD_SetTextColor(CLOCK_COLOR_FACE);
  BSP_LCD_FillCircle(rx, ry, r - 1);
  BSP_LCD_SetTextColor(CLOCK_COLOR_BORDER);
  BSP_LCD_DrawCircle(rx, ry, r - 1);
  BSP_LCD_SetTextColor(CLOCK_COLOR_NUMBERS);
  for(i = 0; i < 360; i+= 30)
    BSP_LCD_FillCircle(rx + sin(i * 6.28 / 360) * nps , ry - cos(i * 6.28 / 360) * nps, 2);

  BSP_LCD_SetBackColor(CLOCK_COLOR_FACE);

  for(i = 0; i < 12; i++)
  {
    ap[i].X = rx; ap[i].Y = ry;
    lp[i].X = rx; lp[i].Y = ry;
  }

  while(1)
  {
    // HAL_RTC_GetTime(&hrtc, &st_time, RTC_FORMAT_BIN);
    nowtime.t = hrtc.Instance->TR;
    if(nowtime.t != lasttime.t)
    {
      s[0] = nowtime.hu + '0';
      s[1] = nowtime.ht + '0';
      s[3] = nowtime.mu + '0';
      s[4] = nowtime.mt + '0';
      s[6] = nowtime.su + '0';
      s[7] = nowtime.st + '0';

      sec = lasttime.hu * 36000 + lasttime.ht * 3600 + lasttime.mu * 600 + lasttime.mt * 60 + lasttime.su * 10 + lasttime.st;

      fsec = sec * 6.28 / 43200.0;
      ap[2].X = rx + sin(fsec) * hps; ap[2].Y = ry - cos(fsec) * hps;
      ap[1].X = rx + sin(fsec - CLOCK_THICKNESS_HP) * hps * CLOCK_SHAPE_HP; ap[1].Y = ry - cos(fsec - CLOCK_THICKNESS_HP) * hps * CLOCK_SHAPE_HP;
      ap[3].X = rx + sin(fsec + CLOCK_THICKNESS_HP) * hps * CLOCK_SHAPE_HP; ap[3].Y = ry - cos(fsec + CLOCK_THICKNESS_HP) * hps * CLOCK_SHAPE_HP;

      fsec = sec * 6.28 / 3600.0;
      ap[6].X = rx + sin(fsec) * mps; ap[6].Y = ry - cos(fsec) * mps;
      ap[5].X = rx + sin(fsec - CLOCK_THICKNESS_MP) * mps * CLOCK_SHAPE_MP; ap[5].Y = ry - cos(fsec - CLOCK_THICKNESS_MP) * mps * CLOCK_SHAPE_MP;
      ap[7].X = rx + sin(fsec + CLOCK_THICKNESS_MP) * mps * CLOCK_SHAPE_MP; ap[7].Y = ry - cos(fsec + CLOCK_THICKNESS_MP) * mps * CLOCK_SHAPE_MP;

      fsec = (sec % 60) * 6.28 / 60.0;
      ap[10].X = rx + sin(fsec) * sps; ap[10].Y = ry - cos(fsec) * sps;
      ap[9].X = rx + sin(fsec - CLOCK_THICKNESS_SP) * sps * CLOCK_SHAPE_SP; ap[9].Y = ry - cos(fsec - CLOCK_THICKNESS_SP) * sps * CLOCK_SHAPE_SP;
      ap[11].X = rx + sin(fsec + CLOCK_THICKNESS_SP) * sps * CLOCK_SHAPE_SP; ap[11].Y = ry - cos(fsec + CLOCK_THICKNESS_SP) * sps * CLOCK_SHAPE_SP;

      BSP_LCD_SetTextColor(CLOCK_COLOR_FACE);
      BSP_LCD_DrawPolygon(&lp[0], 4);
      BSP_LCD_DrawPolygon(&lp[4], 4);
      BSP_LCD_DrawPolygon(&lp[8], 4);

      BSP_LCD_SetTextColor(CLOCK_COLOR_DIGITS);
      BSP_LCD_DisplayStringAt(0, ry - (r >> 1), (uint8_t *)&s, CENTER_MODE);

      BSP_LCD_SetTextColor(CLOCK_COLOR_SP);
      BSP_LCD_DrawPolygon(&ap[8], 4);
      BSP_LCD_SetTextColor(CLOCK_COLOR_MP);
      BSP_LCD_DrawPolygon(&ap[4], 4);
      BSP_LCD_SetTextColor(CLOCK_COLOR_HP);
      BSP_LCD_DrawPolygon(&ap[0], 4);

      for(i = 0; i < 12; i++)
      {
        lp[i].X = ap[i].X;
        lp[i].Y = ap[i].Y;
      }

      printf("%s\n", s);

      lasttime = nowtime;
    }

  }
}

#ifdef osCMSIS

//-----------------------------------------------------------------------------
void StartTask02(void const * argument)
{
  for(;;)
  {
    if(task02_run)
      task02_count++;
  }
}

#endif
