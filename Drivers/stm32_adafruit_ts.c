#include "main.h"
#include "ts.h"
#include "stm32_adafruit_ts.h"

extern TS_DrvTypeDef     *ts_drv;
extern int32_t            ts_cindex[];
static uint16_t          TsXBoundary, TsYBoundary;

/**
  * @brief  Initializes and configures the touch screen functionalities and 
  *         configures all necessary hardware resources (GPIOs, clocks..).
  * @param  XSize: The maximum X size of the TS area on LCD
  * @param  YSize: The maximum Y size of the TS area on LCD  
  * @retval TS_OK: if all initializations are OK. Other value if error.
  */
uint8_t BSP_TS_Init(uint16_t XSize, uint16_t YSize)
{
  uint8_t ret = TS_ERROR;

  /* Initialize x and y positions boundaries */
  TsXBoundary = XSize;
  TsYBoundary = YSize;

  if(ts_drv)
    ret = TS_OK;

  if(ret == TS_OK)
  {
    /* Initialize the LL TS Driver */
    ts_drv->Init(0);
  }

  return ret;
}

/**
  * @brief  Returns status and positions of the touch screen.
  * @param  TsState: Pointer to touch screen current state structure
  */
void BSP_TS_GetState(TS_StateTypeDef* TsState)
{
  uint16_t x, y;
  int32_t  x1, y1, x2, y2;
  
  TsState->TouchDetected = ts_drv->DetectTouch(0);
  if(TsState->TouchDetected)
  {
    ts_drv->GetXY(0, &x, &y);
    x1 = x; y1 = y;
    x2 = (ts_cindex[1] * x1 + ts_cindex[2] * y1 + ts_cindex[3]) / ts_cindex[0];
    y2 = (ts_cindex[4] * x1 + ts_cindex[5] * y1 + ts_cindex[6]) / ts_cindex[0];

    if(x2 < 0)
      x2 = 0;
    else if(x2 >= TsXBoundary)
      x2 = TsXBoundary - 1;

    if(y2 < 0)
      y2 = 0;
    else if(y2 >= TsYBoundary)
      y2 = TsYBoundary;
    
    TsState->X = x2;
    TsState->Y = y2;
  }
}
