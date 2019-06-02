/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __STM32_ADAFRUIT_TS_H
#define __STM32_ADAFRUIT_TS_H

#ifdef __cplusplus
 extern "C" {
#endif   
   
extern  int32_t* p_cindex;

typedef struct
{
  uint16_t TouchDetected;
  uint16_t X;
  uint16_t Y;
  uint16_t Z;
}TS_StateTypeDef;

#define TS_SWAP_NONE                    0x00
#define TS_SWAP_X                       0x01
#define TS_SWAP_Y                       0x02
#define TS_SWAP_XY                      0x04

typedef enum 
{
  TS_OK       = 0x00,
  TS_ERROR    = 0x01,
  TS_TIMEOUT  = 0x02
}TS_StatusTypeDef;

uint8_t BSP_TS_Init(uint16_t XSize, uint16_t YSize);
void    BSP_TS_GetState(TS_StateTypeDef *TsState);

#ifdef __cplusplus
}
#endif

#endif /* __STM32_ADAFRUIT_TS_H */
