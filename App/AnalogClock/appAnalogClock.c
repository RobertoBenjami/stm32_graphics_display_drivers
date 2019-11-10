/* Analog clock
 *
 * Roberto Benjami
 * v: 2019.11
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "main.h"

/* BSP_LCD_... */
#include "stm32_adafruit_lcd.h"

extern   RTC_HandleTypeDef hrtc;

/* 1:time register contain binary data
 * 2:time register contain bcd data
 */
#define  TIMEREG_MODE            1

/* RTC time register read (f103:CNTL-CNTH, f407:TR) */
#define  TIMEREG_READ            hrtc.Instance->CNTL | (hrtc.Instance->CNTH << 16)
// #define  TIMEREG_READ            hrtc.Instance->TR

/* CLOCK colors */
#define  CLOCK_COLOR_BACKGROUND  LCD_COLOR_BLACK
#define  CLOCK_COLOR_FACE        LCD_COLOR(30, 30, 30)
#define  CLOCK_COLOR_BORDER      LCD_COLOR_CYAN
#define  CLOCK_COLOR_NUMBERS     LCD_COLOR_GREEN
#define  CLOCK_COLOR_DIGITS      LCD_COLOR_WHITE
#define  CLOCK_COLOR_HP          LCD_COLOR_RED
#define  CLOCK_COLOR_MP          LCD_COLOR_BLUE
#define  CLOCK_COLOR_SP          LCD_COLOR_YELLOW

/* CLOCK_SIZE... : 0..256 */
#define  CLOCK_SIZE_HP           120
#define  CLOCK_SIZE_MP           180
#define  CLOCK_SIZE_SP           220
#define  CLOCK_SIZE_NMBCIRC      240

/* Pointer form (width:rad, shape:width position) */
#define  CLOCK_WIDTH_HP          0.25
#define  CLOCK_WIDTH_MP          0.15
#define  CLOCK_WIDTH_SP          0.08
#define  CLOCK_SHAPE_HP          0.4
#define  CLOCK_SHAPE_MP          0.3
#define  CLOCK_SHAPE_SP          0.2

/* Clock pointer type
 * - 0: wireframe
 * - 1: fill pointer
 * */
#define  CLOCK_POINTER_TYPE      1

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
uint32_t  task02_run = 0, task02_count = 0;
#else
#define Delay(t)              HAL_Delay(t)
#define GetTime()             HAL_GetTick()
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
    BSP_LCD_FillCircle(rx + sin(i * M_TWOPI / 360) * nps , ry - cos(i * M_TWOPI / 360) * nps, 2);

  BSP_LCD_SetBackColor(CLOCK_COLOR_FACE);

  for(i = 0; i < 12; i++)
  {
    ap[i].X = rx; ap[i].Y = ry;
    lp[i].X = rx; lp[i].Y = ry;
  }
  lasttime.t = 0;

  while(1)
  {
    nowtime.t = TIMEREG_READ;
    if(nowtime.t != lasttime.t)
    {
      #if TIMEREG_MODE == 1 // bin mode
      sec = nowtime.t % 86400;
      i = sec / 3600;
      s[0] = i / 10 + '0';
      s[1] = i % 10 + '0';
      i = sec / 60 % 60;
      s[3] = i / 10 + '0';
      s[4] = i % 10 + '0';
      i = sec % 60;
      s[6] = i / 10 + '0';
      s[7] = i % 10 + '0';
      #endif

      #if TIMEREG_MODE == 2 // bcd mode
      sec = lasttime.hu * 36000 + lasttime.ht * 3600 + lasttime.mu * 600 + lasttime.mt * 60 + lasttime.su * 10 + lasttime.st;
      s[0] = nowtime.hu + '0';
      s[1] = nowtime.ht + '0';
      s[3] = nowtime.mu + '0';
      s[4] = nowtime.mt + '0';
      s[6] = nowtime.su + '0';
      s[7] = nowtime.st + '0';
      #endif

      fsec = sec * M_TWOPI / 43200.0;
      ap[2].X = rx + sin(fsec) * hps; ap[2].Y = ry - cos(fsec) * hps;
      ap[1].X = rx + sin(fsec - CLOCK_WIDTH_HP) * hps * CLOCK_SHAPE_HP; ap[1].Y = ry - cos(fsec - CLOCK_WIDTH_HP) * hps * CLOCK_SHAPE_HP;
      ap[3].X = rx + sin(fsec + CLOCK_WIDTH_HP) * hps * CLOCK_SHAPE_HP; ap[3].Y = ry - cos(fsec + CLOCK_WIDTH_HP) * hps * CLOCK_SHAPE_HP;

      fsec = sec * M_TWOPI / 3600.0;
      ap[6].X = rx + sin(fsec) * mps; ap[6].Y = ry - cos(fsec) * mps;
      ap[5].X = rx + sin(fsec - CLOCK_WIDTH_MP) * mps * CLOCK_SHAPE_MP; ap[5].Y = ry - cos(fsec - CLOCK_WIDTH_MP) * mps * CLOCK_SHAPE_MP;
      ap[7].X = rx + sin(fsec + CLOCK_WIDTH_MP) * mps * CLOCK_SHAPE_MP; ap[7].Y = ry - cos(fsec + CLOCK_WIDTH_MP) * mps * CLOCK_SHAPE_MP;

      fsec = (sec % 60) * M_TWOPI / 60.0;
      ap[10].X = rx + sin(fsec) * sps; ap[10].Y = ry - cos(fsec) * sps;
      ap[9].X = rx + sin(fsec - CLOCK_WIDTH_SP) * sps * CLOCK_SHAPE_SP; ap[9].Y = ry - cos(fsec - CLOCK_WIDTH_SP) * sps * CLOCK_SHAPE_SP;
      ap[11].X = rx + sin(fsec + CLOCK_WIDTH_SP) * sps * CLOCK_SHAPE_SP; ap[11].Y = ry - cos(fsec + CLOCK_WIDTH_SP) * sps * CLOCK_SHAPE_SP;

      #if  CLOCK_POINTER_TYPE == 0
      /* clear the previsous clock pointer */
      BSP_LCD_SetTextColor(CLOCK_COLOR_FACE);
      BSP_LCD_DrawPolygon(&lp[0], 4);
      BSP_LCD_DrawPolygon(&lp[4], 4);
      BSP_LCD_DrawPolygon(&lp[8], 4);

      /* draw the digital clock */
      BSP_LCD_SetTextColor(CLOCK_COLOR_DIGITS);
      BSP_LCD_DisplayStringAt(0, ry - (r >> 1), (uint8_t *)&s, CENTER_MODE);

      /* draw the clock pointer */
      BSP_LCD_SetTextColor(CLOCK_COLOR_SP);
      BSP_LCD_DrawPolygon(&ap[8], 4);
      BSP_LCD_SetTextColor(CLOCK_COLOR_MP);
      BSP_LCD_DrawPolygon(&ap[4], 4);
      BSP_LCD_SetTextColor(CLOCK_COLOR_HP);
      BSP_LCD_DrawPolygon(&ap[0], 4);

      #else
      /* clear the previsous clock pointer */
      BSP_LCD_SetTextColor(CLOCK_COLOR_FACE);
      BSP_LCD_FillTriangle(lp[0].X, lp[0].Y, lp[1].X, lp[1].Y, lp[3].X, lp[3].Y);
      BSP_LCD_FillTriangle(lp[2].X, lp[2].Y, lp[1].X, lp[1].Y, lp[3].X, lp[3].Y);
      BSP_LCD_FillTriangle(lp[4].X, lp[4].Y, lp[5].X, lp[5].Y, lp[7].X, lp[7].Y);
      BSP_LCD_FillTriangle(lp[6].X, lp[6].Y, lp[5].X, lp[5].Y, lp[7].X, lp[7].Y);
      BSP_LCD_FillTriangle(lp[8].X, lp[8].Y, lp[9].X, lp[9].Y, lp[11].X, lp[11].Y);
      BSP_LCD_FillTriangle(lp[10].X, lp[10].Y, lp[9].X, lp[9].Y, lp[11].X, lp[11].Y);
      BSP_LCD_DrawPolygon(&lp[0], 4); /* because the thin filled triangle is may be incomplete */
      BSP_LCD_DrawPolygon(&lp[4], 4);
      BSP_LCD_DrawPolygon(&lp[8], 4);

      /* draw the digital clock */
      BSP_LCD_SetTextColor(CLOCK_COLOR_DIGITS);
      BSP_LCD_DisplayStringAt(0, ry - (r >> 1), (uint8_t *)&s, CENTER_MODE);

      /* draw the clock pointer */
      BSP_LCD_SetTextColor(CLOCK_COLOR_SP);
      BSP_LCD_FillTriangle(ap[8].X, ap[8].Y, ap[9].X, ap[9].Y, ap[11].X, ap[11].Y);
      BSP_LCD_FillTriangle(ap[10].X, ap[10].Y, ap[9].X, ap[9].Y, ap[11].X, ap[11].Y);
      BSP_LCD_DrawPolygon(&ap[8], 4); /* because the thin filled triangle is may be incomplete */
      BSP_LCD_SetTextColor(CLOCK_COLOR_MP);
      BSP_LCD_FillTriangle(ap[4].X, ap[4].Y, ap[5].X, ap[5].Y, ap[7].X, ap[7].Y);
      BSP_LCD_FillTriangle(ap[6].X, ap[6].Y, ap[5].X, ap[5].Y, ap[7].X, ap[7].Y);
      BSP_LCD_DrawPolygon(&ap[4], 4);
      BSP_LCD_SetTextColor(CLOCK_COLOR_HP);
      BSP_LCD_FillTriangle(ap[0].X, ap[0].Y, ap[1].X, ap[1].Y, ap[3].X, ap[3].Y);
      BSP_LCD_FillTriangle(ap[2].X, ap[2].Y, ap[1].X, ap[1].Y, ap[3].X, ap[3].Y);
      BSP_LCD_DrawPolygon(&ap[0], 4);

      #endif

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
