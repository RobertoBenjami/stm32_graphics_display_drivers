// 3D Filled Vector Graphics
// (c) 2019 Pawel A. Hernik
// YouTube videos:
// https://youtu.be/YLf2WXjunyg
// https://youtu.be/5y28ipwQs-E

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "main.h"

#include "3d_filled_vector.h"

/* BSP_LCD_... */
#include "stm32_adafruit_lcd.h"

//=============================================================================

#define BITBAND_ACCESS(a, b)  *(volatile uint32_t*)(((uint32_t)&a & 0xF0000000) + 0x2000000 + (((uint32_t)&a & 0x000FFFFF) << 5) + (b << 2))

//-----------------------------------------------------------------------------
/* GPIO mode */
#define MODE_DIGITAL_INPUT    0x0
#define MODE_OUT              0x1
#define MODE_ALTER            0x2
#define MODE_ANALOG_INPUT     0x3

#define MODE_SPD_LOW          0x0
#define MODE_SPD_MEDIUM       0x1
#define MODE_SPD_HIGH         0x2
#define MODE_SPD_VHIGH        0x3

#define MODE_PU_NONE          0x0
#define MODE_PU_UP            0x1
#define MODE_PU_DOWN          0x2

#define GPIOX_PORT_(a, b)     GPIO ## a
#define GPIOX_PORT(a)         GPIOX_PORT_(a)

#define GPIOX_PIN_(a, b)      b
#define GPIOX_PIN(a)          GPIOX_PIN_(a)

#define GPIOX_AFR_(a,b,c)     GPIO ## b->AFR[c >> 3] = (GPIO ## b->AFR[c >> 3] & ~(0x0F << (4 * (c & 7)))) | (a << (4 * (c & 7)));
#define GPIOX_AFR(a, b)       GPIOX_AFR_(a, b)

#define GPIOX_MODER_(a,b,c)   GPIO ## b->MODER = (GPIO ## b->MODER & ~(3 << (2 * c))) | (a << (2 * c));
#define GPIOX_MODER(a, b)     GPIOX_MODER_(a, b)

#define GPIOX_OSPEEDR_(a,b,c) GPIO ## b->OSPEEDR = (GPIO ## b->OSPEEDR & ~(3 << (2 * c))) | (a << (2 * c));
#define GPIOX_OSPEEDR(a, b)   GPIOX_OSPEEDR_(a, b)

#define GPIOX_PUPDR_(a,b,c)   GPIO ## b->PUPDR = (GPIO ## b->PUPDR & ~(3 << (2 * c))) | (a << (2 * c));
#define GPIOX_PUPDR(a, b)     GPIOX_PUPDR_(a, b)

#define GPIOX_ODR_(a, b)      BITBAND_ACCESS(GPIO ## a ->ODR, b)
#define GPIOX_ODR(a)          GPIOX_ODR_(a)

#define GPIOX_IDR_(a, b)      BITBAND_ACCESS(GPIO ## a ->IDR, b)
#define GPIOX_IDR(a)          GPIOX_IDR_(a)

#define GPIOX_LINE_(a, b)     EXTI_Line ## b
#define GPIOX_LINE(a)         GPIOX_LINE_(a)

#define GPIOX_PORTSRC_(a, b)  GPIO_PortSourceGPIO ## a
#define GPIOX_PORTSRC(a)      GPIOX_PORTSRC_(a)

#define GPIOX_PINSRC_(a, b)   GPIO_PinSource ## b
#define GPIOX_PINSRC(a)       GPIOX_PINSRC_(a)

// GPIO Ports Clock Enable
#define GPIOX_CLOCK_(a, b)    RCC_AHB1ENR_GPIO ## a ## EN
#define GPIOX_CLOCK(a)        GPIOX_CLOCK_(a)

#define GPIOX_PORTNUM_A       1
#define GPIOX_PORTNUM_B       2
#define GPIOX_PORTNUM_C       3
#define GPIOX_PORTNUM_D       4
#define GPIOX_PORTNUM_E       5
#define GPIOX_PORTNUM_F       6
#define GPIOX_PORTNUM_G       7
#define GPIOX_PORTNUM_H       8
#define GPIOX_PORTNUM_I       9
#define GPIOX_PORTNUM_J       10
#define GPIOX_PORTNUM_K       11
#define GPIOX_PORTNUM_(a, b)  GPIOX_PORTNUM_ ## a
#define GPIOX_PORTNUM(a)      GPIOX_PORTNUM_(a)

#define GPIOX_PORTNAME_(a, b) a
#define GPIOX_PORTNAME(a)     GPIOX_PORTNAME_(a)

//-----------------------------------------------------------------------------// freertos vs HAL
#ifdef  osCMSIS
#define Delay(t)              osDelay(t)
#define GetTime()             osKernelSysTick()
#else
#define Delay(t)              HAL_Delay(t)
#define GetTime()             HAL_GetTick()
#endif

/*
 Implemented features:
 - optimized rendering without local framebuffer, in STM32 case 1 to 32 lines buffer can be used
 - pattern based background
 - 3D starfield
 - no floating point arithmetic
 - no slow trigonometric functions
 - rotations around X and Y axes
 - simple outside screen culling
 - rasterizer working for all convex polygons
 - backface culling
 - visible faces sorting by Z axis
*/

int  buttonState;
int  prevState = 1;
long btDebounce    = 30;
long btMultiClick  = 600;
long btLongClick   = 500;
long btLongerClick = 2000;
long btTime = 0, btTime2 = 0, millis;
int  clickCnt = 1;

// 0=idle, 1,2,3=click, -1,-2=longclick
int checkButton()
{
  int state;
  millis = GetTime();
  #if GPIOX_PORTNUM(BUTTON) >= GPIOX_PORTNUM_A
  if(GPIOX_IDR(BUTTON))
    state = 1 - BUTTON_ON;
  else
    state = BUTTON_ON;
  #else
  state = 1;
  #endif

  if(state == 0 && prevState == 1)
  {
    btTime = millis; prevState = state;
    return 0;
  } // button just pressed

  if(state == 1 && prevState == 0)
  { // button just released
    prevState = state;
    if(millis - btTime >= btDebounce && millis - btTime < btLongClick)
    {
      if(millis - btTime2 < btMultiClick)
        clickCnt++;
      else
        clickCnt = 1;
      btTime2 = millis;
      return clickCnt; 
    } 
  }

  if(state == 0 && millis - btTime >= btLongerClick)
  {
    prevState = state;
    return -2;
  }

  if(state == 0 && millis - btTime >= btLongClick)
  {
    prevState = state;
    return -1;
  }

  return 0;
}

int prevButtonState = 0;

int handleButton()
{
  prevButtonState = buttonState;
  buttonState = checkButton();
  return buttonState;
}

// --------------------------------------------------------------------------
char txt[30];
#define MAX_OBJ 12
int bgMode = 3;
int object = 6;
int bfCull = 1;
int orient = 0;
int polyMode = 0;

#include "pat2.h"
#include "pat7.h"
#include "pat8.h"
#include "gfx3d.h"

void setup() 
{
  uint8_t  e;
  Delay(300);

  #if GPIOX_PORTNUM(BUTTON) >= GPIOX_PORTNUM_A
  RCC->AHB1ENR |= GPIOX_CLOCK(BUTTON);
  GPIOX_MODER(MODE_DIGITAL_INPUT, BUTTON);
  #if BUTTON_PU == 1
  GPIOX_PUPDR(MODE_PU_UP, BUTTON);
  #elif BUTTON_PU == 2
  GPIOX_PUPDR(MODE_PU_DOWN, BUTTON);
  #endif
  #endif

  e = BSP_LCD_Init();
  if(e == LCD_ERROR)
  {
    printf("\r\nLcd Init Error\r\n");
    while(1);
  }

  BSP_LCD_Clear(LCD_COLOR_BLACK);
  initStars();
}

unsigned int ms, msMin = 1000, msMax = 0, stats = 1, optim = 0; // optim=1 for ST7735, 0 for ST7789

void showStats()
{
  if(ms < msMin) msMin = ms;
  if(ms > msMax) msMax = ms;
  BSP_LCD_SetBackColor(LCD_COLOR_BLACK);
  if(optim == 0)
  {
    snprintf(txt, 30, "%d ms     %d fps ", ms, 1000 / ms);
    BSP_LCD_SetTextColor(LCD_COLOR_YELLOW);
    BSP_LCD_DisplayStringAt(0, SCR_HT - 3 * CHARSIZEY, (uint8_t *)txt, LEFT_MODE);
    snprintf(txt, 30, "%d-%d ms  %d-%d fps   ", msMin, msMax, 1000 / msMax, 1000 / msMin);
    BSP_LCD_SetTextColor(LCD_COLOR_GREEN);
    BSP_LCD_DisplayStringAt(0, SCR_HT - 2 * CHARSIZEY, (uint8_t *)txt, LEFT_MODE);
    snprintf(txt, 30, "total/vis %d / %d   ", numPolys, numVisible);
    BSP_LCD_SetTextColor(LCD_COLOR_MAGENTA);
    BSP_LCD_DisplayStringAt(0, SCR_HT - CHARSIZEY, (uint8_t *)txt, LEFT_MODE);
  }
  else if(optim == 1)
  {
    optim = 2;
    snprintf(txt, 30, "00 ms     00 fps");
    BSP_LCD_SetTextColor(LCD_COLOR_YELLOW);
    BSP_LCD_DisplayStringAt(0, SCR_HT - 3 * CHARSIZEY, (uint8_t *)txt, LEFT_MODE);
    snprintf(txt, 30, "00-00 ms  00-00 fps");
    BSP_LCD_SetTextColor(LCD_COLOR_GREEN);
    BSP_LCD_DisplayStringAt(0, SCR_HT - 2 * CHARSIZEY, (uint8_t *)txt, LEFT_MODE);
    snprintf(txt, 30, "total/vis 000 / 000");
    BSP_LCD_SetTextColor(LCD_COLOR_MAGENTA);
    BSP_LCD_DisplayStringAt(0, SCR_HT - CHARSIZEY, (uint8_t *)txt, LEFT_MODE);
  }
  else
  {
    snprintf(txt, 30, "%2d", ms);
    BSP_LCD_SetTextColor(LCD_COLOR_YELLOW);
    BSP_LCD_DisplayStringAt(0, SCR_HT - 3 * CHARSIZEY, (uint8_t *)txt, LEFT_MODE);
    snprintf(txt, 30,"%2d", 1000 / ms);
    BSP_LCD_DisplayStringAt(10 * CHARSIZEX, SCR_HT - 3 * CHARSIZEY, (uint8_t *)txt, LEFT_MODE);
    snprintf(txt, 30,"%2d-%2d", msMin, msMax);
    BSP_LCD_SetTextColor(LCD_COLOR_GREEN);
    BSP_LCD_DisplayStringAt(0, SCR_HT - 2 * CHARSIZEY, (uint8_t *)txt, LEFT_MODE);
    snprintf(txt, 30, "%2d-%2d", 1000 / msMax, 1000 / msMin);
    BSP_LCD_DisplayStringAt(10 * CHARSIZEX, SCR_HT - 2 * CHARSIZEY, (uint8_t *)txt, LEFT_MODE);
    snprintf(txt, 30, "%3d", numPolys);
    BSP_LCD_SetTextColor(LCD_COLOR_MAGENTA);
    BSP_LCD_DisplayStringAt(10 * CHARSIZEX, SCR_HT - CHARSIZEY, (uint8_t *)txt, LEFT_MODE);
    snprintf(txt, 30, "%3d", numVisible);
    BSP_LCD_DisplayStringAt(16 * CHARSIZEX, SCR_HT - CHARSIZEY, (uint8_t *)txt, LEFT_MODE);
  }
}

void loop()
{
  handleButton();
  if(buttonState < 0)
  {
    if(buttonState == -1 && prevButtonState >= 0 && ++bgMode > 4)
      bgMode = 0;
    if(buttonState == -2 && prevButtonState == -1)
    {
      stats = !stats;
      BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
      BSP_LCD_FillRect(0, HT_3D, SCR_WD, SCR_HT - HT_3D);
      if(optim)
        optim = 1;
    }
  }
  else if(buttonState > 0)
  {
    if(++object > MAX_OBJ)
      object = 0;
    msMin = 1000;
    msMax = 0;
  }
  polyMode = 0;
  orient = 0;
  bfCull = 1;
  lightShade = 0;
  switch(object)
  {
    case 0:
      numVerts  = numVertsCubeQ;
      verts     = (int16_t*)vertsCubeQ;
      numPolys  = numQuadsCubeQ;
      polys     = (uint8_t*)quadsCubeQ;
      polyColors = (uint16_t*)colsCubeQ;
      break;
    case 1:
      numVerts  = numVertsCubeQ;
      verts     = (int16_t*)vertsCubeQ;
      numPolys  = numQuadsCubeQ;
      polys     = (uint8_t*)quadsCubeQ;
      lightShade = 44000;
      break;
   case 2:
      numVerts  = numVertsCross;
      verts     = (int16_t*)vertsCross;
      numPolys  = numQuadsCross;
      polys     = (uint8_t*)quadsCross;
      polyColors = (uint16_t*)colsCross;
      break;
   case 3:
      numVerts  = numVertsCross;
      verts     = (int16_t*)vertsCross;
      numPolys  = numQuadsCross;
      polys     = (uint8_t*)quadsCross;
      lightShade = 14000;
      break;
   case 4:
      numVerts  = numVerts3;
      verts     = (int16_t*)verts3;
      numPolys  = numQuads3;
      polys     = (uint8_t*)quads3;
      polyColors = (uint16_t*)cols3;
      break;
   case 5:
      numVerts  = numVerts3;
      verts     = (int16_t*)verts3;
      numPolys  = numQuads3;
      polys     = (uint8_t*)quads3;
      lightShade = 20000;
      break;
   case 6:
      numVerts  = numVertsCubes;
      verts     = (int16_t*)vertsCubes;
      numPolys  = numQuadsCubes;
      polys     = (uint8_t*)quadsCubes;
      polyColors = (uint16_t*)colsCubes;
      bfCull    = 0;
      break;
   case 7:
      numVerts  = numVertsCubes;
      verts     = (int16_t*)vertsCubes;
      numPolys  = numQuadsCubes;
      polys     = (uint8_t*)quadsCubes;
      bfCull    = 1;
      lightShade = 14000;
      break;
   case 8:
      numVerts  = numVertsCone;
      verts     = (int16_t*)vertsCone;
      numPolys  = numTrisCone;
      polys     = (uint8_t*)trisCone;
      polyColors = (uint16_t*)colsCone;
      bfCull    = 1;
      orient    = 1;
      polyMode  = 1;
      break;
   case 9:
      numVerts  = numVertsSphere;
      verts     = (int16_t*)vertsSphere;
      numPolys  = numTrisSphere;
      polys     = (uint8_t*)trisSphere;
      //polyColors = (uint16_t*)colsSphere;
      lightShade = 58000;
      bfCull    = 1;
      orient    = 1;
      polyMode  = 1;
      break;
   case 10:
      numVerts  = numVertsTorus;
      verts     = (int16_t*)vertsTorus;
      numPolys  = numTrisTorus;
      polys     = (uint8_t*)trisTorus;
      polyColors = (uint16_t*)colsTorus;
      bfCull    = 1;
      orient    = 1;
      polyMode  = 1;
      break;
   case 11:
      numVerts  = numVertsTorus;
      verts     = (int16_t*)vertsTorus;
      numPolys  = numTrisTorus;
      polys     = (uint8_t*)trisTorus;
      lightShade = 20000;
      bfCull    = 1;
      orient    = 1;
      polyMode  = 1;
      break;
   case 12:
      numVerts  = numVertsMonkey;
      verts     = (int16_t*)vertsMonkey;
      numPolys  = numTrisMonkey;
      polys     = (uint8_t*)trisMonkey;
      //polyColors = (uint16_t*)colsMonkey;
      lightShade = 20000;
      bfCull    = 1;
      orient    = 1;
      polyMode  = 1;
      break;
  }
  ms = GetTime();
  render3D(polyMode);
  ms = GetTime() - ms;
  if(stats)
    showStats();
}

//-----------------------------------------------------------------------------
#ifdef osCMSIS
void StartDefaultTask(void const * argument)
#else
void mainApp(void)
#endif
{
  setup();
  while(1)
    loop();
}

#ifdef osCMSIS

//-----------------------------------------------------------------------------
/* The other task constantly increases one counter */
void StartTask02(void const * argument)
{
  for(;;)
  {
    if(task02_run)
      task02_count++;
  }
}

#endif
