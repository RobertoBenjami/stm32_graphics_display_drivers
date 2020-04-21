/*
 * 2019.05 Add v1.1 extension (#ifdef   LCD_DRVTYPE_V1_1)
 * 2019.11 Add BSP_LCD_FillTriangle
 * 2019.12 Add LCD_DEFAULT_FONT, LCD_DEFAULT_BACKCOLOR, LCD_DEFAULT_TEXTCOLOR, LCD_INIT_CLEAR
*/

/**
  ******************************************************************************
  * @file    stm32_adafruit_lcd.h
  * @author  MCD Application Team
  * @brief   This file contains the common defines and functions prototypes for
  *          the stm32_adafruit_lcd.c driver.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2016 STMicroelectronics</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */ 

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __STM32_ADAFRUIT_LCD_H
#define __STM32_ADAFRUIT_LCD_H

#ifdef __cplusplus
 extern "C" {
#endif 

/* Includes ------------------------------------------------------------------*/
#include "lcd.h"
#include "Fonts/fonts.h"

//-----------------------------------------------------------------------------
/* Config section (you can change this defines) */

/* LCD default font (Font8 or Font12 or Font16 or Font20 or Font24) */
#define LCD_DEFAULT_FONT         Font8

/* LCD default colors */
#define LCD_DEFAULT_BACKCOLOR    LCD_COLOR_BLACK
#define LCD_DEFAULT_TEXTCOLOR    LCD_COLOR_WHITE

/* LCD clear with LCD_DEFAULT_BACKCOLOR in the BSP_LCD_Init (0:diasble, 1:enable) */
#define LCD_INIT_CLEAR           0

/* some colors */
#define LCD_COLOR_BLACK         RC(0x0000)
#define LCD_COLOR_GRAY          RC(0xF7DE)
#define LCD_COLOR_BLUE          RC(0x001F)
#define LCD_COLOR_RED           RC(0xF800)
#define LCD_COLOR_GREEN         RC(0x07E0)
#define LCD_COLOR_CYAN          RC(0x07FF)
#define LCD_COLOR_MAGENTA       RC(0xF81F)
#define LCD_COLOR_YELLOW        RC(0xFFE0)
#define LCD_COLOR_WHITE         RC(0xFFFF)
  
//-----------------------------------------------------------------------------
/* Interface section (no modify) */   

/** 
  * @brief  Draw Properties structures definition
  */ 
typedef struct 
{ 
  uint32_t TextColor;
  uint32_t BackColor;
  sFONT    *pFont; 
}LCD_DrawPropTypeDef;

/** 
  * @brief  Point structures definition
  */ 
typedef struct 
{
  int16_t X;
  int16_t Y;
}Point, * pPoint;

/** 
  * @brief  Line mode structures definition
  */ 
typedef enum
{
  CENTER_MODE             = 0x01,    /*!< Center mode */
  RIGHT_MODE              = 0x02,    /*!< Right mode  */
  LEFT_MODE               = 0x03     /*!< Left mode   */
}Line_ModeTypdef;
 
#define __IO    volatile  

/** 
  * @brief  LCD status structure definition  
  */     
#define LCD_OK         0x00
#define LCD_ERROR      0x01
#define LCD_TIMEOUT    0x02

#if LCD_REVERSE == 1
#define  RC(a)   ((((a) & 0xFF) << 8) | (((a) & 0xFF00) >> 8))
#else
#define  RC(a)   a
#endif

/** 
  * @brief  LCD color  
  */
#define LCD_COLOR(r, g, b)      RC((r & 0xF8) << 8 | (g & 0xFC) << 3 | (b & 0xF8) >> 3)

/** @defgroup STM32_ADAFRUIT_LCD_Exported_Functions
  * @{
  */   
uint8_t  BSP_LCD_Init(void);
uint16_t BSP_LCD_GetXSize(void);
uint16_t BSP_LCD_GetYSize(void);
 
uint16_t BSP_LCD_GetTextColor(void);
uint16_t BSP_LCD_GetBackColor(void);
void     BSP_LCD_SetTextColor(__IO uint16_t Color);
void     BSP_LCD_SetBackColor(__IO uint16_t Color);
void     BSP_LCD_SetFont(sFONT *fonts);
sFONT    *BSP_LCD_GetFont(void);

void     BSP_LCD_Clear(uint16_t Color);
void     BSP_LCD_ClearStringLine(uint16_t Line);
void     BSP_LCD_DisplayStringAtLine(uint16_t Line, uint8_t *ptr);
void     BSP_LCD_DisplayStringAt(uint16_t Xpos, uint16_t Ypos, uint8_t *Text, Line_ModeTypdef Mode);
void     BSP_LCD_DisplayChar(uint16_t Xpos, uint16_t Ypos, uint8_t Ascii);

void     BSP_LCD_DrawPixel(uint16_t Xpos, uint16_t Ypos, uint16_t RGB_Code);
void     BSP_LCD_DrawHLine(uint16_t Xpos, uint16_t Ypos, uint16_t Length);
void     BSP_LCD_DrawVLine(uint16_t Xpos, uint16_t Ypos, uint16_t Length);
void     BSP_LCD_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
void     BSP_LCD_DrawRect(uint16_t Xpos, uint16_t Ypos, uint16_t Width, uint16_t Height);
void     BSP_LCD_DrawCircle(uint16_t Xpos, uint16_t Ypos, uint16_t Radius);
void     BSP_LCD_DrawPolygon(pPoint Points, uint16_t PointCount);
void     BSP_LCD_DrawEllipse(uint16_t Xpos, uint16_t Ypos, uint16_t XRadius, uint16_t YRadius);
void     BSP_LCD_DrawBitmap(uint16_t Xpos, uint16_t Ypos, uint8_t *pBmp);
void     BSP_LCD_FillRect(uint16_t Xpos, uint16_t Ypos, uint16_t Width, uint16_t Height);
void     BSP_LCD_FillCircle(uint16_t Xpos, uint16_t Ypos, uint16_t Radius);
void     BSP_LCD_FillPolygon(pPoint Points, uint16_t PointCount);
void     BSP_LCD_FillEllipse(uint16_t Xpos, uint16_t Ypos, uint16_t XRadius, uint16_t YRadius);
void     BSP_LCD_FillTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3);

void     BSP_LCD_DisplayOff(void);
void     BSP_LCD_DisplayOn(void);

#ifdef   LCD_DRVTYPE_V1_1
uint16_t BSP_LCD_ReadID(void);
uint16_t BSP_LCD_ReadPixel(uint16_t Xpos, uint16_t Ypos);
void     BSP_LCD_DrawRGB16Image(uint16_t Xpos, uint16_t Ypos, uint16_t Xsize, uint16_t Ysize, uint16_t *pData);
void     BSP_LCD_ReadRGB16Image(uint16_t Xpos, uint16_t Ypos, uint16_t Xsize, uint16_t Ysize, uint16_t *pData);
#endif
 
#ifdef __cplusplus
}
#endif

#endif /* __STM32_ADAFRUIT_LCD_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
