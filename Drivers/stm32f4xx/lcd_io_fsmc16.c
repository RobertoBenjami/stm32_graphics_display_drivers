/* 
 * 16 bit paralell LCD FSMC driver for STM32F4
 * 5 controll pins (CS, RS, WR, RD, RST) + 16 data pins + 1 backlight pin
 */

/* 
 * Author: Roberto Benjami
 * version:  2020.05
 */

#include "main.h"
#include "lcd.h"
#include "lcd_io_fsmc16.h"

#define  LCD_ADDR_DATA       (LCD_ADDR_BASE + (1 << (LCD_REGSELECT_BIT + 2)) - 2)
#define  DMA_MAXSIZE         0xFFFE

//-----------------------------------------------------------------------------
/* Link function for LCD peripheral */
void     LCD_Delay (uint32_t delay);
void     LCD_IO_Init(void);
void     LCD_IO_Bl_OnOff(uint8_t Bl);

void     LCD_IO_WriteCmd8(uint8_t Cmd);
void     LCD_IO_WriteCmd16(uint16_t Cmd);
void     LCD_IO_WriteData8(uint8_t Data);
void     LCD_IO_WriteData16(uint16_t Data);

void     LCD_IO_WriteCmd8DataFill16(uint8_t Cmd, uint16_t Data, uint32_t Size);
void     LCD_IO_WriteCmd8MultipleData8(uint8_t Cmd, uint8_t *pData, uint32_t Size);
void     LCD_IO_WriteCmd8MultipleData16(uint8_t Cmd, uint16_t *pData, uint32_t Size);
void     LCD_IO_WriteCmd16DataFill16(uint16_t Cmd, uint16_t Data, uint32_t Size);
void     LCD_IO_WriteCmd16MultipleData8(uint16_t Cmd, uint8_t *pData, uint32_t Size);
void     LCD_IO_WriteCmd16MultipleData16(uint16_t Cmd, uint16_t *pData, uint32_t Size);

void     LCD_IO_ReadCmd8MultipleData8(uint8_t Cmd, uint8_t *pData, uint32_t Size, uint32_t DummySize);
void     LCD_IO_ReadCmd8MultipleData16(uint8_t Cmd, uint16_t *pData, uint32_t Size, uint32_t DummySize);
void     LCD_IO_ReadCmd8MultipleData24to16(uint8_t Cmd, uint16_t *pData, uint32_t Size, uint32_t DummySize);
void     LCD_IO_ReadCmd16MultipleData8(uint16_t Cmd, uint8_t *pData, uint32_t Size, uint32_t DummySize);
void     LCD_IO_ReadCmd16MultipleData16(uint16_t Cmd, uint16_t *pData, uint32_t Size, uint32_t DummySize);
void     LCD_IO_ReadCmd16MultipleData24to16(uint16_t Cmd, uint16_t *pData, uint32_t Size, uint32_t DummySize);

//-----------------------------------------------------------------------------
#define BITBAND_ACCESS(a, b)  *(volatile uint32_t*)(((uint32_t)&a & 0xF0000000) + 0x2000000 + (((uint32_t)&a & 0x000FFFFF) << 5) + (b << 2))

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

//-----------------------------------------------------------------------------
/* DMA mode */
#define DMA_ISR_TCIF0_Pos       (5U)
#define DMA_ISR_TCIF0           (0x1U << DMA_ISR_TCIF0_Pos)                  /*!< 0x00000020 */
#define DMA_ISR_HTIF0_Pos       (4U)
#define DMA_ISR_HTIF0           (0x1U << DMA_ISR_HTIF0_Pos)                  /*!< 0x00000010 */
#define DMA_ISR_TEIF0_Pos       (3U)
#define DMA_ISR_TEIF0           (0x1U << DMA_ISR_TEIF0_Pos)                  /*!< 0x00000008 */
#define DMA_ISR_DMEIF0_Pos      (2U)
#define DMA_ISR_DMEIF0          (0x1U << DMA_ISR_DMEIF0_Pos)                 /*!< 0x00000004 */
#define DMA_ISR_FEIF0_Pos       (0U)
#define DMA_ISR_FEIF0           (0x1U << DMA_ISR_FEIF0_Pos)                  /*!< 0x00000001 */
#define DMA_ISR_TCIF1_Pos       (11U)
#define DMA_ISR_TCIF1           (0x1U << DMA_ISR_TCIF1_Pos)                  /*!< 0x00000800 */
#define DMA_ISR_HTIF1_Pos       (10U)
#define DMA_ISR_HTIF1           (0x1U << DMA_ISR_HTIF1_Pos)                  /*!< 0x00000400 */
#define DMA_ISR_TEIF1_Pos       (9U)
#define DMA_ISR_TEIF1           (0x1U << DMA_ISR_TEIF1_Pos)                  /*!< 0x00000200 */
#define DMA_ISR_DMEIF1_Pos      (8U)
#define DMA_ISR_DMEIF1          (0x1U << DMA_ISR_DMEIF1_Pos)                 /*!< 0x00000100 */
#define DMA_ISR_FEIF1_Pos       (6U)
#define DMA_ISR_FEIF1           (0x1U << DMA_ISR_FEIF1_Pos)                  /*!< 0x00000040 */
#define DMA_ISR_TCIF2_Pos       (21U)
#define DMA_ISR_TCIF2           (0x1U << DMA_ISR_TCIF2_Pos)                  /*!< 0x00200000 */
#define DMA_ISR_HTIF2_Pos       (20U)
#define DMA_ISR_HTIF2           (0x1U << DMA_ISR_HTIF2_Pos)                  /*!< 0x00100000 */
#define DMA_ISR_TEIF2_Pos       (19U)
#define DMA_ISR_TEIF2           (0x1U << DMA_ISR_TEIF2_Pos)                  /*!< 0x00080000 */
#define DMA_ISR_DMEIF2_Pos      (18U)
#define DMA_ISR_DMEIF2          (0x1U << DMA_ISR_DMEIF2_Pos)                 /*!< 0x00040000 */
#define DMA_ISR_FEIF2_Pos       (16U)
#define DMA_ISR_FEIF2           (0x1U << DMA_ISR_FEIF2_Pos)                  /*!< 0x00010000 */
#define DMA_ISR_TCIF3_Pos       (27U)
#define DMA_ISR_TCIF3           (0x1U << DMA_ISR_TCIF3_Pos)                  /*!< 0x08000000 */
#define DMA_ISR_HTIF3_Pos       (26U)
#define DMA_ISR_HTIF3           (0x1U << DMA_ISR_HTIF3_Pos)                  /*!< 0x04000000 */
#define DMA_ISR_TEIF3_Pos       (25U)
#define DMA_ISR_TEIF3           (0x1U << DMA_ISR_TEIF3_Pos)                  /*!< 0x02000000 */
#define DMA_ISR_DMEIF3_Pos      (24U)
#define DMA_ISR_DMEIF3          (0x1U << DMA_ISR_DMEIF3_Pos)                 /*!< 0x01000000 */
#define DMA_ISR_FEIF3_Pos       (22U)
#define DMA_ISR_FEIF3           (0x1U << DMA_ISR_FEIF3_Pos)                  /*!< 0x00400000 */
#define DMA_ISR_TCIF4_Pos       (5U)
#define DMA_ISR_TCIF4           (0x1U << DMA_ISR_TCIF4_Pos)                  /*!< 0x00000020 */
#define DMA_ISR_HTIF4_Pos       (4U)
#define DMA_ISR_HTIF4           (0x1U << DMA_ISR_HTIF4_Pos)                  /*!< 0x00000010 */
#define DMA_ISR_TEIF4_Pos       (3U)
#define DMA_ISR_TEIF4           (0x1U << DMA_ISR_TEIF4_Pos)                  /*!< 0x00000008 */
#define DMA_ISR_DMEIF4_Pos      (2U)
#define DMA_ISR_DMEIF4          (0x1U << DMA_ISR_DMEIF4_Pos)                 /*!< 0x00000004 */
#define DMA_ISR_FEIF4_Pos       (0U)
#define DMA_ISR_FEIF4           (0x1U << DMA_ISR_FEIF4_Pos)                  /*!< 0x00000001 */
#define DMA_ISR_TCIF5_Pos       (11U)
#define DMA_ISR_TCIF5           (0x1U << DMA_ISR_TCIF5_Pos)                  /*!< 0x00000800 */
#define DMA_ISR_HTIF5_Pos       (10U)
#define DMA_ISR_HTIF5           (0x1U << DMA_ISR_HTIF5_Pos)                  /*!< 0x00000400 */
#define DMA_ISR_TEIF5_Pos       (9U)
#define DMA_ISR_TEIF5           (0x1U << DMA_ISR_TEIF5_Pos)                  /*!< 0x00000200 */
#define DMA_ISR_DMEIF5_Pos      (8U)
#define DMA_ISR_DMEIF5          (0x1U << DMA_ISR_DMEIF5_Pos)                 /*!< 0x00000100 */
#define DMA_ISR_FEIF5_Pos       (6U)
#define DMA_ISR_FEIF5           (0x1U << DMA_ISR_FEIF5_Pos)                  /*!< 0x00000040 */
#define DMA_ISR_TCIF6_Pos       (21U)
#define DMA_ISR_TCIF6           (0x1U << DMA_ISR_TCIF6_Pos)                  /*!< 0x00200000 */
#define DMA_ISR_HTIF6_Pos       (20U)
#define DMA_ISR_HTIF6           (0x1U << DMA_ISR_HTIF6_Pos)                  /*!< 0x00100000 */
#define DMA_ISR_TEIF6_Pos       (19U)
#define DMA_ISR_TEIF6           (0x1U << DMA_ISR_TEIF6_Pos)                  /*!< 0x00080000 */
#define DMA_ISR_DMEIF6_Pos      (18U)
#define DMA_ISR_DMEIF6          (0x1U << DMA_ISR_DMEIF6_Pos)                 /*!< 0x00040000 */
#define DMA_ISR_FEIF6_Pos       (16U)
#define DMA_ISR_FEIF6           (0x1U << DMA_ISR_FEIF6_Pos)                  /*!< 0x00010000 */
#define DMA_ISR_TCIF7_Pos       (27U)
#define DMA_ISR_TCIF7           (0x1U << DMA_ISR_TCIF7_Pos)                  /*!< 0x08000000 */
#define DMA_ISR_HTIF7_Pos       (26U)
#define DMA_ISR_HTIF7           (0x1U << DMA_ISR_HTIF7_Pos)                  /*!< 0x04000000 */
#define DMA_ISR_TEIF7_Pos       (25U)
#define DMA_ISR_TEIF7           (0x1U << DMA_ISR_TEIF7_Pos)                  /*!< 0x02000000 */
#define DMA_ISR_DMEIF7_Pos      (24U)
#define DMA_ISR_DMEIF7          (0x1U << DMA_ISR_DMEIF7_Pos)                 /*!< 0x01000000 */
#define DMA_ISR_FEIF7_Pos       (22U)
#define DMA_ISR_FEIF7           (0x1U << DMA_ISR_FEIF7_Pos)                  /*!< 0x00400000 */

#define DMA_IFCR_CTCIF0_Pos     (5U)
#define DMA_IFCR_CTCIF0         (0x1U << DMA_IFCR_CTCIF0_Pos)                /*!< 0x00000020 */
#define DMA_IFCR_CHTIF0_Pos     (4U)
#define DMA_IFCR_CHTIF0         (0x1U << DMA_IFCR_CHTIF0_Pos)                /*!< 0x00000010 */
#define DMA_IFCR_CTEIF0_Pos     (3U)
#define DMA_IFCR_CTEIF0         (0x1U << DMA_IFCR_CTEIF0_Pos)                /*!< 0x00000008 */
#define DMA_IFCR_CDMEIF0_Pos    (2U)
#define DMA_IFCR_CDMEIF0        (0x1U << DMA_IFCR_CDMEIF0_Pos)               /*!< 0x00000004 */
#define DMA_IFCR_CFEIF0_Pos     (0U)
#define DMA_IFCR_CFEIF0         (0x1U << DMA_IFCR_CFEIF0_Pos)                /*!< 0x00000001 */
#define DMA_IFCR_CTCIF1_Pos     (11U)
#define DMA_IFCR_CTCIF1         (0x1U << DMA_IFCR_CTCIF1_Pos)                /*!< 0x00000800 */
#define DMA_IFCR_CHTIF1_Pos     (10U)
#define DMA_IFCR_CHTIF1         (0x1U << DMA_IFCR_CHTIF1_Pos)                /*!< 0x00000400 */
#define DMA_IFCR_CTEIF1_Pos     (9U)
#define DMA_IFCR_CTEIF1         (0x1U << DMA_IFCR_CTEIF1_Pos)                /*!< 0x00000200 */
#define DMA_IFCR_CDMEIF1_Pos    (8U)
#define DMA_IFCR_CDMEIF1        (0x1U << DMA_IFCR_CDMEIF1_Pos)               /*!< 0x00000100 */
#define DMA_IFCR_CFEIF1_Pos     (6U)
#define DMA_IFCR_CFEIF1         (0x1U << DMA_IFCR_CFEIF1_Pos)                /*!< 0x00000040 */
#define DMA_IFCR_CTCIF2_Pos     (21U)
#define DMA_IFCR_CTCIF2         (0x1U << DMA_IFCR_CTCIF2_Pos)                /*!< 0x00200000 */
#define DMA_IFCR_CHTIF2_Pos     (20U)
#define DMA_IFCR_CHTIF2         (0x1U << DMA_IFCR_CHTIF2_Pos)                /*!< 0x00100000 */
#define DMA_IFCR_CTEIF2_Pos     (19U)
#define DMA_IFCR_CTEIF2         (0x1U << DMA_IFCR_CTEIF2_Pos)                /*!< 0x00080000 */
#define DMA_IFCR_CDMEIF2_Pos    (18U)
#define DMA_IFCR_CDMEIF2        (0x1U << DMA_IFCR_CDMEIF2_Pos)               /*!< 0x00040000 */
#define DMA_IFCR_CFEIF2_Pos     (16U)
#define DMA_IFCR_CFEIF2         (0x1U << DMA_IFCR_CFEIF2_Pos)                /*!< 0x00010000 */
#define DMA_IFCR_CTCIF3_Pos     (27U)
#define DMA_IFCR_CTCIF3         (0x1U << DMA_IFCR_CTCIF3_Pos)                /*!< 0x08000000 */
#define DMA_IFCR_CHTIF3_Pos     (26U)
#define DMA_IFCR_CHTIF3         (0x1U << DMA_IFCR_CHTIF3_Pos)                /*!< 0x04000000 */
#define DMA_IFCR_CTEIF3_Pos     (25U)
#define DMA_IFCR_CTEIF3         (0x1U << DMA_IFCR_CTEIF3_Pos)                /*!< 0x02000000 */
#define DMA_IFCR_CDMEIF3_Pos    (24U)
#define DMA_IFCR_CDMEIF3        (0x1U << DMA_IFCR_CDMEIF3_Pos)               /*!< 0x01000000 */
#define DMA_IFCR_CFEIF3_Pos     (22U)
#define DMA_IFCR_CFEIF3         (0x1U << DMA_IFCR_CFEIF3_Pos)                /*!< 0x00400000 */
#define DMA_IFCR_CTCIF4_Pos     (5U)
#define DMA_IFCR_CTCIF4         (0x1U << DMA_IFCR_CTCIF4_Pos)                /*!< 0x00000020 */
#define DMA_IFCR_CHTIF4_Pos     (4U)
#define DMA_IFCR_CHTIF4         (0x1U << DMA_IFCR_CHTIF4_Pos)                /*!< 0x00000010 */
#define DMA_IFCR_CTEIF4_Pos     (3U)
#define DMA_IFCR_CTEIF4         (0x1U << DMA_IFCR_CTEIF4_Pos)                /*!< 0x00000008 */
#define DMA_IFCR_CDMEIF4_Pos    (2U)
#define DMA_IFCR_CDMEIF4        (0x1U << DMA_IFCR_CDMEIF4_Pos)               /*!< 0x00000004 */
#define DMA_IFCR_CFEIF4_Pos     (0U)
#define DMA_IFCR_CFEIF4         (0x1U << DMA_IFCR_CFEIF4_Pos)                /*!< 0x00000001 */
#define DMA_IFCR_CTCIF5_Pos     (11U)
#define DMA_IFCR_CTCIF5         (0x1U << DMA_IFCR_CTCIF5_Pos)                /*!< 0x00000800 */
#define DMA_IFCR_CHTIF5_Pos     (10U)
#define DMA_IFCR_CHTIF5         (0x1U << DMA_IFCR_CHTIF5_Pos)                /*!< 0x00000400 */
#define DMA_IFCR_CTEIF5_Pos     (9U)
#define DMA_IFCR_CTEIF5         (0x1U << DMA_IFCR_CTEIF5_Pos)                /*!< 0x00000200 */
#define DMA_IFCR_CDMEIF5_Pos    (8U)
#define DMA_IFCR_CDMEIF5        (0x1U << DMA_IFCR_CDMEIF5_Pos)               /*!< 0x00000100 */
#define DMA_IFCR_CFEIF5_Pos     (6U)
#define DMA_IFCR_CFEIF5         (0x1U << DMA_IFCR_CFEIF5_Pos)                /*!< 0x00000040 */
#define DMA_IFCR_CTCIF6_Pos     (21U)
#define DMA_IFCR_CTCIF6         (0x1U << DMA_IFCR_CTCIF6_Pos)                /*!< 0x00200000 */
#define DMA_IFCR_CHTIF6_Pos     (20U)
#define DMA_IFCR_CHTIF6         (0x1U << DMA_IFCR_CHTIF6_Pos)                /*!< 0x00100000 */
#define DMA_IFCR_CTEIF6_Pos     (19U)
#define DMA_IFCR_CTEIF6         (0x1U << DMA_IFCR_CTEIF6_Pos)                /*!< 0x00080000 */
#define DMA_IFCR_CDMEIF6_Pos    (18U)
#define DMA_IFCR_CDMEIF6        (0x1U << DMA_IFCR_CDMEIF6_Pos)               /*!< 0x00040000 */
#define DMA_IFCR_CFEIF6_Pos     (16U)
#define DMA_IFCR_CFEIF6         (0x1U << DMA_IFCR_CFEIF6_Pos)                /*!< 0x00010000 */
#define DMA_IFCR_CTCIF7_Pos     (27U)
#define DMA_IFCR_CTCIF7         (0x1U << DMA_IFCR_CTCIF7_Pos)                /*!< 0x08000000 */
#define DMA_IFCR_CHTIF7_Pos     (26U)
#define DMA_IFCR_CHTIF7         (0x1U << DMA_IFCR_CHTIF7_Pos)                /*!< 0x04000000 */
#define DMA_IFCR_CTEIF7_Pos     (25U)
#define DMA_IFCR_CTEIF7         (0x1U << DMA_IFCR_CTEIF7_Pos)                /*!< 0x02000000 */
#define DMA_IFCR_CDMEIF7_Pos    (24U)
#define DMA_IFCR_CDMEIF7        (0x1U << DMA_IFCR_CDMEIF7_Pos)               /*!< 0x01000000 */
#define DMA_IFCR_CFEIF7_Pos     (22U)
#define DMA_IFCR_CFEIF7         (0x1U << DMA_IFCR_CFEIF7_Pos)                /*!< 0x00400000 */

typedef struct
{
  __IO uint32_t ISR[2];  /*!< DMA interrupt status register,      Address offset: 0x00 */
  __IO uint32_t IFCR[2]; /*!< DMA interrupt flag clear register,  Address offset: 0x08 */
} DMA_TypeDef_Array;

#define DMANUM_(a, b, c, d)             a
#define DMANUM(a)                       DMANUM_(a)

#define DMACHN_(a, b, c, d)             b
#define DMACHN(a)                       DMACHN_(a)

#define DMASTREAM_(a, b, c, d)          c
#define DMASTREAM(a)                    DMASTREAM_(a)

#define DMAPRIORITY_(a, b, c, d)        d
#define DMAPRIORITY(a)                  DMAPRIORITY_(a)

#define DMAX_STREAMX_(a, b, c, d)       DMA ## a ## _Stream ## c
#define DMAX_STREAMX(a)                 DMAX_STREAMX_(a)

#define DMAX_STREAMX_IRQ_(a, b, c, d)   DMA ## a ## _Stream ## c ## _IRQn
#define DMAX_STREAMX_IRQ(a)             DMAX_STREAMX_IRQ_(a)

#define DMAX_STREAMX_IRQHANDLER_(a, b, c, d) DMA ## a ## _Stream ## c ## _IRQHandler
#define DMAX_STREAMX_IRQHANDLER(a)      DMAX_STREAMX_IRQHANDLER_(a)

/* Interrupt event pl: if(DMAX_ISR(LCD_DMA_TX) & DMAX_ISR_TCIF(LCD_DMA_TX))... */
#define DMAX_ISR_(a, b, c, d)           ((DMA_TypeDef_Array*) + DMA ## a ## _BASE)->ISR[c >> 2]
#define DMAX_ISR(a)                     DMAX_ISR_(a)

#define DMAX_ISR_TCIF_(a, b, c, d)      DMA_ISR_TCIF ## c
#define DMAX_ISR_TCIF(a)                DMAX_ISR_TCIF_(a)

#define DMAX_ISR_HTIF_(a, b, c, d)      DMA_ISR_HTIF ## c
#define DMAX_ISR_HTIF(a)                DMAX_ISR_HTIF_(a)

#define DMAX_ISR_TEIF_(a, b, c, d)      DMA_ISR_TEIF ## c
#define DMAX_ISR_TEIF(a)                DMAX_ISR_TEIF_(a)

#define DMAX_ISR_DMEIF_(a, b, c, d)     DMA_ISR_DMEIF ## c
#define DMAX_ISR_DMEIF(a)               DMAX_ISR_DMEIF_(a)

#define DMAX_ISR_FEIF_(a, b, c, d)      DMA_ISR_FEIF ## c
#define DMAX_ISR_FEIF(a)                DMAX_ISR_FEIF_(a)

/* Interrupt clear pl: DMAX_IFCR(LCD_DMA_TX) = DMAX_IFCR_CTCIF(LCD_DMA_TX) | DMAX_IFCR_CFEIF(LCD_DMA_TX); */
#define DMAX_IFCR_(a, b, c, d)          ((DMA_TypeDef_Array*) + DMA ## a ## _BASE)->IFCR[c >> 2]
#define DMAX_IFCR(a)                    DMAX_IFCR_(a)

#define DMAX_IFCR_CTCIF_(a, b, c, d)    DMA_IFCR_CTCIF ## c
#define DMAX_IFCR_CTCIF(a)              DMAX_IFCR_CTCIF_(a)

#define DMAX_IFCR_CHTIF_(a, b, c, d)    DMA_IFCR_CHTIF ## c
#define DMAX_IFCR_CHTIF(a)              DMAX_IFCR_CHTIF_(a)

#define DMAX_IFCR_CTEIF_(a, b, c, d)    DMA_IFCR_CTEIF ## c
#define DMAX_IFCR_CTEIF(a)              DMAX_IFCR_CTEIF_(a)

#define DMAX_IFCR_CDMEIF_(a, b, c, d)   DMA_IFCR_CDMEIF ## c
#define DMAX_IFCR_CDMEIF(a)             DMAX_IFCR_CDMEIF_(a)

#define DMAX_IFCR_CFEIF_(a, b, c, d)    DMA_IFCR_CFEIF ## c
#define DMAX_IFCR_CFEIF(a)              DMAX_IFCR_CFEIF_(a)

//-----------------------------------------------------------------------------
/* freertos vs HAL */
#ifdef  osCMSIS
#define DelayMs(t)            osDelay(t)
#define GetTime()             osKernelSysTick()
#else
#define DelayMs(t)            HAL_Delay(t)
#define GetTime()             HAL_GetTick()
#endif

//-----------------------------------------------------------------------------
#if DMANUM(LCD_DMA) > 0
/* DMA transfer end check */

/* DMA status
   - 0: all DMA transfers are completed
   - 1: last DMA transfer in progress
   - 2: DMA transfer in progress */
volatile uint32_t LCD_IO_DmaTransferStatus = 0;

//-----------------------------------------------------------------------------
/* Waiting for all DMA processes to complete */
#ifndef osFeature_Semaphore
/* no FreeRtos */

extern inline void WaitForDmaEnd(void);
inline void WaitForDmaEnd(void)
{
  while(LCD_IO_DmaTransferStatus);
}

//-----------------------------------------------------------------------------
#else   /* osFeature_Semaphore */
/* FreeRtos */
osSemaphoreId spiDmaBinSemHandle;

extern inline void WaitForDmaEnd(void);
inline void WaitForDmaEnd(void)
{
  if(LCD_IO_DmaTransferStatus)
  {
    osSemaphoreWait(spiDmaBinSemHandle, 500);
    if(LCD_IO_DmaTransferStatus == 1)
      LCD_IO_DmaTransferStatus = 0;
  }
}
#endif  /* #else osFeature_Semaphore */

//-----------------------------------------------------------------------------
/* DMA interrupt handler */
void DMAX_STREAMX_IRQHANDLER(LCD_DMA)(void)
{
  if(DMAX_ISR(LCD_DMA) & DMAX_ISR_TCIF(LCD_DMA))
  {
    DMAX_IFCR(LCD_DMA) = DMAX_IFCR_CTCIF(LCD_DMA);
    DMAX_STREAMX(LCD_DMA)->CR = 0;
    while(DMAX_STREAMX(LCD_DMA)->CR & DMA_SxCR_EN);
    #ifndef osFeature_Semaphore
    /* no FreeRtos */
    LCD_IO_DmaTransferStatus = 0;
    #else
    /* FreeRtos */
    osSemaphoreRelease(spiDmaBinSemHandle);
    #endif /* #else osFeature_Semaphore */
  }
  else
    DMAX_IFCR(LCD_DMA) = DMAX_IFCR_CTCIF(LCD_DMA)  | DMAX_IFCR_CHTIF(LCD_DMA) | DMAX_IFCR_CTEIF(LCD_DMA) |
                         DMAX_IFCR_CDMEIF(LCD_DMA) | DMAX_IFCR_CFEIF(LCD_DMA);
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteMultiData(void * pData, uint32_t Size, uint32_t dmacr)
{
  DMAX_IFCR(LCD_DMA) = DMAX_IFCR_CTCIF(LCD_DMA)  | DMAX_IFCR_CHTIF(LCD_DMA) | DMAX_IFCR_CTEIF(LCD_DMA) |
                       DMAX_IFCR_CDMEIF(LCD_DMA) | DMAX_IFCR_CFEIF(LCD_DMA);
  DMAX_STREAMX(LCD_DMA)->CR = 0;        /* DMA stop */
  DMAX_STREAMX(LCD_DMA)->M0AR = LCD_ADDR_DATA;
  DMAX_STREAMX(LCD_DMA)->PAR = (uint32_t)pData;
  DMAX_STREAMX(LCD_DMA)->NDTR = Size;
  DMAX_STREAMX(LCD_DMA)->CR = dmacr;
  DMAX_STREAMX(LCD_DMA)->CR |= DMA_SxCR_EN;
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteMultiData8(uint8_t * pData, uint32_t Size, uint32_t dinc)
{
  uint32_t dmacr;
  #if LCD_DMA_TXWAIT != 2
  static uint8_t d8s;
  #endif
  if(!dinc)
  {
    #if LCD_DMA_TXWAIT != 2
    d8s = *pData;
    pData = &d8s;
    #endif
    dmacr = DMA_SxCR_TCIE |
            (0 << DMA_SxCR_MSIZE_Pos) | (0 << DMA_SxCR_PSIZE_Pos) |
            (0 << DMA_SxCR_PINC_Pos) | (2 << DMA_SxCR_DIR_Pos) |
            (DMACHN(LCD_DMA) << DMA_SxCR_CHSEL_Pos) |
            (DMAPRIORITY(LCD_DMA) << DMA_SxCR_PL_Pos);
  }
  else
    dmacr = DMA_SxCR_TCIE |
            (0 << DMA_SxCR_MSIZE_Pos) | (0 << DMA_SxCR_PSIZE_Pos) |
            (1 << DMA_SxCR_PINC_Pos) | (2 << DMA_SxCR_DIR_Pos) |
            (DMACHN(LCD_DMA) << DMA_SxCR_CHSEL_Pos) |
            (DMAPRIORITY(LCD_DMA) << DMA_SxCR_PL_Pos);

  #ifdef LCD_DMA_UNABLE
  if(LCD_DMA_UNABLE((uint32_t)(pData)))
  {
    while(Size--)
    {
      *(volatile uint16_t *)LCD_ADDR_DATA = (uint16_t)*pData;
      if(dinc)
        pData++;
    }
  }
  else
  #endif
  {
    while(Size)
    {
      if(Size <= DMA_MAXSIZE)
      {
        LCD_IO_DmaTransferStatus = 1;     /* last transfer */
        LCD_IO_WriteMultiData((void *)pData, Size, dmacr);
        Size = 0;
        #if LCD_DMA_TXWAIT == 1
        if(dinc)
          WaitForDmaEnd();
        #endif
      }
      else
      {
        LCD_IO_DmaTransferStatus = 2;     /* no last transfer */
        LCD_IO_WriteMultiData((void *)pData, DMA_MAXSIZE, dmacr);
        if(dinc)
          pData+= DMA_MAXSIZE;
        Size-= DMA_MAXSIZE;
        #if LCD_DMA_TXWAIT != 2
        WaitForDmaEnd();
        #endif
      }
      #if LCD_DMA_TXWAIT == 2
      WaitForDmaEnd();
      #endif
    }
  }
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteMultiData16(uint16_t * pData, uint32_t Size, uint32_t dinc)
{
  uint32_t dmacr;
  #if LCD_DMA_TXWAIT != 2
  static uint16_t d16s;
  #endif
  if(!dinc)
  {
    #if LCD_DMA_TXWAIT != 2
    d16s = *pData;
    pData = &d16s;
    #endif
    dmacr = DMA_SxCR_TCIE |
            (1 << DMA_SxCR_MSIZE_Pos) | (1 << DMA_SxCR_PSIZE_Pos) |
            (0 << DMA_SxCR_PINC_Pos) | (2 << DMA_SxCR_DIR_Pos) |
            (DMACHN(LCD_DMA) << DMA_SxCR_CHSEL_Pos) |
            (DMAPRIORITY(LCD_DMA) << DMA_SxCR_PL_Pos);
  }
  else
    dmacr = DMA_SxCR_TCIE |
            (1 << DMA_SxCR_MSIZE_Pos) | (1 << DMA_SxCR_PSIZE_Pos) |
            (1 << DMA_SxCR_PINC_Pos) | (2 << DMA_SxCR_DIR_Pos) |
            (DMACHN(LCD_DMA) << DMA_SxCR_CHSEL_Pos) |
            (DMAPRIORITY(LCD_DMA) << DMA_SxCR_PL_Pos);

  #ifdef LCD_DMA_UNABLE
  if(LCD_DMA_UNABLE((uint32_t)(pData)))
  {
    while(Size--)
    {
      *(volatile uint16_t *)LCD_ADDR_DATA = *pData;
      if(dinc)
        pData++;
    }
  }
  else
  #endif
  {
    while(Size)
    {
      if(Size <= DMA_MAXSIZE)
      {
        LCD_IO_DmaTransferStatus = 1;     /* last transfer */
        LCD_IO_WriteMultiData((void *)pData, Size, dmacr);
        Size = 0;
        #if LCD_DMA_TXWAIT == 1
        if(dinc)
          WaitForDmaEnd();
        #endif
      }
      else if(Size < 2 * DMA_MAXSIZE)
      {
        LCD_IO_DmaTransferStatus = 2;     /* no last transfer */
        LCD_IO_WriteMultiData((void *)pData, Size - DMA_MAXSIZE, dmacr);
        if(dinc)
          pData+= Size - DMA_MAXSIZE;
        Size = DMA_MAXSIZE;
        #if LCD_DMA_TXWAIT != 2
        WaitForDmaEnd();
        #endif
      }
      else
      {
        LCD_IO_DmaTransferStatus = 2;     /* no last transfer */
        LCD_IO_WriteMultiData((void *)pData, DMA_MAXSIZE, dmacr);
        if(dinc)
          pData+= DMA_MAXSIZE;
        Size-= DMA_MAXSIZE;
        #if LCD_DMA_TXWAIT != 2
        WaitForDmaEnd();
        #endif
      }
      #if LCD_DMA_TXWAIT == 2
      WaitForDmaEnd();
      #endif
    }
  }
}

//-----------------------------------------------------------------------------
void LCD_IO_ReadMultiData(void * pData, uint32_t Size, uint32_t dmacr)
{
  DMAX_IFCR(LCD_DMA) = DMAX_IFCR_CTCIF(LCD_DMA)  | DMAX_IFCR_CHTIF(LCD_DMA) | DMAX_IFCR_CTEIF(LCD_DMA) |
                       DMAX_IFCR_CDMEIF(LCD_DMA) | DMAX_IFCR_CFEIF(LCD_DMA);
  DMAX_STREAMX(LCD_DMA)->CR = 0;        /* DMA stop */
  DMAX_STREAMX(LCD_DMA)->M0AR = (uint32_t)pData;
  DMAX_STREAMX(LCD_DMA)->PAR = LCD_ADDR_DATA;
  DMAX_STREAMX(LCD_DMA)->NDTR = Size;
  DMAX_STREAMX(LCD_DMA)->CR = dmacr;
  DMAX_STREAMX(LCD_DMA)->CR |= DMA_SxCR_EN;
}

//-----------------------------------------------------------------------------
void LCD_IO_ReadMultiData8(uint8_t * pData, uint32_t Size)
{
  uint32_t dmacr;
  dmacr = DMA_SxCR_TCIE |
          (0 << DMA_SxCR_MSIZE_Pos) | (0 << DMA_SxCR_PSIZE_Pos) |
          (1 << DMA_SxCR_MINC_Pos) | (2 << DMA_SxCR_DIR_Pos) |
          (DMACHN(LCD_DMA) << DMA_SxCR_CHSEL_Pos) |
          (DMAPRIORITY(LCD_DMA) << DMA_SxCR_PL_Pos);
  #ifdef LCD_DMA_UNABLE
  if(LCD_DMA_UNABLE((uint32_t)(pData)))
  {
    while(Size--)
    {
      *pData = (uint8_t)(*(volatile uint16_t *)LCD_ADDR_DATA);
      pData++;
    }
  }
  else
  #endif
  {
    while(Size)
    {
      if(Size > DMA_MAXSIZE)
      {
        LCD_IO_DmaTransferStatus = 2;     /* no last transfer */
        LCD_IO_ReadMultiData((void *)pData, DMA_MAXSIZE, dmacr);
        Size-= DMA_MAXSIZE;
        pData+= DMA_MAXSIZE;
      }
      else
      {
        LCD_IO_DmaTransferStatus = 1;     /* last transfer */
        LCD_IO_ReadMultiData((void *)pData, Size, dmacr);
        Size = 0;
      }
      WaitForDmaEnd();
    }
  }
}

//-----------------------------------------------------------------------------
void LCD_IO_ReadMultiData16(uint16_t * pData, uint32_t Size)
{
  uint32_t dmacr;
  dmacr = DMA_SxCR_TCIE |
          (1 << DMA_SxCR_MSIZE_Pos) | (1 << DMA_SxCR_PSIZE_Pos) |
          (1 << DMA_SxCR_MINC_Pos) | (2 << DMA_SxCR_DIR_Pos) |
          (DMACHN(LCD_DMA) << DMA_SxCR_CHSEL_Pos) |
          (DMAPRIORITY(LCD_DMA) << DMA_SxCR_PL_Pos);
  #ifdef LCD_DMA_UNABLE
  if(LCD_DMA_UNABLE((uint32_t)(pData)))
  {
    while(Size--)
    {
      *pData = *(volatile uint16_t *)LCD_ADDR_DATA;
      pData++;
    }
  }
  else
  #endif
  {
    while(Size)
    {
      if(Size > DMA_MAXSIZE)
      {
        LCD_IO_DmaTransferStatus = 2;     /* no last transfer */
        LCD_IO_ReadMultiData((void *)pData, DMA_MAXSIZE, dmacr);
        Size-= DMA_MAXSIZE;
        pData+= DMA_MAXSIZE;
      }
      else
      {
        LCD_IO_DmaTransferStatus = 1;     /* last transfer */
        LCD_IO_ReadMultiData((void *)pData, Size, dmacr);
        Size = 0;
      }
      WaitForDmaEnd();
    }
  }
}

#else   /* #if DMANUM(LCD_DMA) > 0 */

#define  WaitForDmaEnd()                /* if DMA is not used -> no need to wait */

#endif  /* #else DMANUM(LCD_DMA) > 0 */

//-----------------------------------------------------------------------------
/* reset pin setting */
#define LCD_RST_ON            GPIOX_ODR(LCD_RST) = 0
#define LCD_RST_OFF           GPIOX_ODR(LCD_RST) = 1

//-----------------------------------------------------------------------------
void LCD_Delay(uint32_t Delay)
{
  DelayMs(Delay);
}

//-----------------------------------------------------------------------------
void LCD_IO_Bl_OnOff(uint8_t Bl)
{
  #if GPIOX_PORTNUM(LCD_BL) >= GPIOX_PORTNUM_A
  if(Bl)
    GPIOX_ODR(LCD_BL) = LCD_BLON;
  else
    GPIOX_ODR(LCD_BL) = 1 - LCD_BLON;
  #endif
}

//-----------------------------------------------------------------------------
void LCD_IO_Init(void)
{
  /* Reset pin clock */
  #if GPIOX_PORTNUM(LCD_RST) >= GPIOX_PORTNUM_A
  #define GPIOX_CLOCK_LCD_RST   GPIOX_CLOCK(LCD_RST)
  #else
  #define GPIOX_CLOCK_LCD_RST   0
  #endif

  /* Backlight pin clock */
  #if GPIOX_PORTNUM(LCD_BL) >=  GPIOX_PORTNUM_A
  #define GPIOX_CLOCK_LCD_BL    GPIOX_CLOCK(LCD_BL)
  #else
  #define GPIOX_CLOCK_LCD_BL    0
  #endif

  /* DMA clock */
  #if DMANUM(LCD_DMA) == 1
  #define DMA_CLOCK             RCC_AHB1ENR_DMA1EN
  #elif DMANUM(LCD_DMA) == 2
  #define DMA_CLOCK             RCC_AHB1ENR_DMA2EN
  #else
  #define DMA_CLOCK             0
  #endif

  /* GPIO, DMA Clocks */
  #if GPIOX_PORTNUM(LCD_RST) >= GPIOX_PORTNUM_A || GPIOX_PORTNUM(LCD_BL) >= GPIOX_PORTNUM_A || DMANUM(LCD_DMA) > 0
  RCC->AHB1ENR |= GPIOX_CLOCK_LCD_RST | GPIOX_CLOCK_LCD_BL | DMA_CLOCK;
  #endif

  #if GPIOX_PORTNUM(LCD_RST) >= GPIOX_PORTNUM_A
  GPIOX_MODER(MODE_OUT, LCD_RST);       /* RST = GPIO OUT */
  GPIOX_ODR(LCD_RST) = 1;               /* RST = 1 */
  #endif

  #if GPIOX_PORTNUM(LCD_BL) >= GPIOX_PORTNUM_A
  GPIOX_ODR(LCD_BL) = LCD_BLON;
  GPIOX_MODER(MODE_OUT, LCD_BL);
  #endif

  /* Reset the LCD */
  #if GPIOX_PORTNUM(LCD_RST) >= GPIOX_PORTNUM_A
  LCD_Delay(1);
  LCD_RST_ON;
  LCD_Delay(1);
  LCD_RST_OFF;
  #endif
  LCD_Delay(1);

  #if DMANUM(LCD_DMA) > 0
  NVIC_SetPriority(DMAX_STREAMX_IRQ(LCD_DMA), LCD_DMA_IRQ_PR);
  NVIC_EnableIRQ(DMAX_STREAMX_IRQ(LCD_DMA));
  #ifdef osFeature_Semaphore
  osSemaphoreDef(spiDmaBinSem);
  spiDmaBinSemHandle = osSemaphoreCreate(osSemaphore(spiDmaBinSem), 1);
  osSemaphoreWait(spiDmaBinSemHandle, 1);
  #endif
  #endif  /* #if DMANUM(LCD_DMA) > 0 */
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteCmd8(uint8_t Cmd)
{
  WaitForDmaEnd();
  *(volatile uint16_t *)LCD_ADDR_BASE = (uint16_t)Cmd;
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteCmd16(uint16_t Cmd)
{
  WaitForDmaEnd();
  *(volatile uint16_t *)LCD_ADDR_BASE = Cmd;
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteData8(uint8_t Data)
{
  WaitForDmaEnd();
  *(volatile uint16_t *)LCD_ADDR_DATA = (uint16_t)Data;
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteData16(uint16_t Data)
{
  WaitForDmaEnd();
  *(volatile uint16_t *)LCD_ADDR_DATA = Data;
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteCmd8DataFill16(uint8_t Cmd, uint16_t Data, uint32_t Size)
{
  WaitForDmaEnd();
  *(volatile uint16_t *)LCD_ADDR_BASE = (uint16_t)Cmd;

  #if DMANUM(LCD_DMA) == 0
  while(Size--)
    *(volatile uint16_t *)LCD_ADDR_DATA = Data;

  #else
  LCD_IO_WriteMultiData16(&Data, Size, 0);
  #endif
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteCmd8MultipleData8(uint8_t Cmd, uint8_t *pData, uint32_t Size)
{
  WaitForDmaEnd();
  *(volatile uint16_t *)LCD_ADDR_BASE = (uint16_t)Cmd;

  #if DMANUM(LCD_DMA) == 0
  while(Size--)
  {
    *(volatile uint16_t *)LCD_ADDR_DATA = (uint16_t)*pData;
    pData++;
  }

  #else
  LCD_IO_WriteMultiData8(pData, Size, 1);
  #endif
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteCmd8MultipleData16(uint8_t Cmd, uint16_t *pData, uint32_t Size)
{
  WaitForDmaEnd();
  *(volatile uint16_t *)LCD_ADDR_BASE = (uint16_t)Cmd;

  #if DMANUM(LCD_DMA) == 0
  while(Size--)
  {
    *(volatile uint16_t *)LCD_ADDR_DATA = *pData;
    pData++;
  }

  #else
  LCD_IO_WriteMultiData16(pData, Size, 1);
  #endif
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteCmd16DataFill16(uint16_t Cmd, uint16_t Data, uint32_t Size)
{
  WaitForDmaEnd();
  *(volatile uint16_t *)LCD_ADDR_BASE = Cmd;

  #if DMANUM(LCD_DMA) == 0
  while(Size--)
    *(volatile uint16_t *)LCD_ADDR_DATA = Data;

  #else
  LCD_IO_WriteMultiData16(&Data, Size, 0);
  #endif
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteCmd16MultipleData8(uint16_t Cmd, uint8_t *pData, uint32_t Size)
{
  WaitForDmaEnd();
  *(volatile uint16_t *)LCD_ADDR_BASE = Cmd;

  #if DMANUM(LCD_DMA) == 0
  while(Size--)
  {
    *(volatile uint16_t *)LCD_ADDR_DATA = (uint16_t)*pData;
    pData++;
  }

  #else
  LCD_IO_WriteMultiData8(pData, Size, 1);
  #endif
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteCmd16MultipleData16(uint16_t Cmd, uint16_t *pData, uint32_t Size)
{
  WaitForDmaEnd();
  *(volatile uint16_t *)LCD_ADDR_BASE = Cmd;

  #if DMANUM(LCD_DMA) == 0
  while(Size--)
  {
    *(volatile uint16_t *)LCD_ADDR_DATA = *pData;
    pData++;
  }

  #else
  LCD_IO_WriteMultiData16(pData, Size, 1);
  #endif
}

//-----------------------------------------------------------------------------
void LCD_IO_ReadCmd8MultipleData8(uint8_t Cmd, uint8_t *pData, uint32_t Size, uint32_t DummySize)
{
  #ifdef  __GNUC__
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wunused-but-set-variable"
  uint16_t DummyData;
  #pragma GCC diagnostic pop
  #elif   defined(__CC_ARM)
  uint16_t DummyData;
  #endif

  WaitForDmaEnd();
  *(volatile uint16_t *)LCD_ADDR_BASE = (uint16_t)Cmd;

  while(DummySize--)
    DummyData = *(volatile uint16_t *)LCD_ADDR_DATA;

  #if DMANUM(LCD_DMA) == 0
  while(Size--)
  {
    *pData = (uint8_t)(*(volatile uint16_t *)LCD_ADDR_DATA);
    pData++;
  }

  #else
  LCD_IO_ReadMultiData8(pData, Size);
  #endif
}

//-----------------------------------------------------------------------------
void LCD_IO_ReadCmd8MultipleData16(uint8_t Cmd, uint16_t *pData, uint32_t Size, uint32_t DummySize)
{
  #ifdef  __GNUC__
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wunused-but-set-variable"
  uint16_t DummyData;
  #pragma GCC diagnostic pop
  #elif   defined(__CC_ARM)
  uint16_t DummyData;
  #endif

  WaitForDmaEnd();
  *(volatile uint16_t *)LCD_ADDR_BASE = (uint16_t)Cmd;

  while(DummySize--)
    DummyData = *(volatile uint16_t *)LCD_ADDR_DATA;

  #if DMANUM(LCD_DMA) == 0
  while(Size--)
  {
    *pData = *(volatile uint16_t *)LCD_ADDR_DATA;
    pData++;
  }

  #else
  LCD_IO_ReadMultiData16(pData, Size);
  #endif
}

//-----------------------------------------------------------------------------
void LCD_IO_ReadCmd8MultipleData24to16(uint8_t Cmd, uint16_t *pData, uint32_t Size, uint32_t DummySize)
{
  union packed
  {
    uint8_t  rgb888[6];
    uint16_t rgb888_16[3];
  }u;

  #ifdef  __GNUC__
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wunused-but-set-variable"
  uint16_t DummyData;
  #pragma GCC diagnostic pop
  #elif   defined(__CC_ARM)
  uint16_t DummyData;
  #endif

  WaitForDmaEnd();
  *(volatile uint16_t *)LCD_ADDR_BASE = (uint16_t)Cmd;

  while(DummySize--)
    DummyData = *(volatile uint16_t *)LCD_ADDR_DATA;

  while(Size--)
  {
    u.rgb888_16[0] = *(volatile uint16_t*)LCD_ADDR_DATA;
    u.rgb888_16[1] = *(volatile uint16_t*)LCD_ADDR_DATA;
    u.rgb888_16[2] = *(volatile uint16_t*)LCD_ADDR_DATA;

    *pData = ((u.rgb888[1] & 0xF8) << 8 | (u.rgb888[0] & 0xFC) << 3 | u.rgb888[3] >> 3);
    pData++;
    if(Size)
    {
      *pData = ((u.rgb888[2] & 0xF8) << 8 | (u.rgb888[5] & 0xFC) << 3 | u.rgb888[4] >> 3);
      pData++;
      Size--;
    }
  }
}

//-----------------------------------------------------------------------------
void LCD_IO_ReadCmd16MultipleData8(uint16_t Cmd, uint8_t *pData, uint32_t Size, uint32_t DummySize)
{
  #ifdef  __GNUC__
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wunused-but-set-variable"
  uint16_t DummyData;
  #pragma GCC diagnostic pop
  #elif   defined(__CC_ARM)
  uint16_t DummyData;
  #endif

  WaitForDmaEnd();
  *(volatile uint16_t *)LCD_ADDR_BASE = Cmd;

  while(DummySize--)
    DummyData = *(volatile uint16_t *)LCD_ADDR_DATA;

  #if DMANUM(LCD_DMA) == 0
  while(Size--)
  {
    *pData = (uint8_t)(*(volatile uint16_t *)LCD_ADDR_DATA);
    pData++;
  }

  #else
  LCD_IO_ReadMultiData8(pData, Size);
  #endif
}

//-----------------------------------------------------------------------------
void LCD_IO_ReadCmd16MultipleData16(uint16_t Cmd, uint16_t *pData, uint32_t Size, uint32_t DummySize)
{
  #ifdef  __GNUC__
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wunused-but-set-variable"
  uint16_t DummyData;
  #pragma GCC diagnostic pop
  #elif   defined(__CC_ARM)
  uint16_t DummyData;
  #endif

  WaitForDmaEnd();
  *(volatile uint16_t *)LCD_ADDR_BASE = Cmd;

  while(DummySize--)
    DummyData = *(volatile uint16_t *)LCD_ADDR_DATA;

  #if DMANUM(LCD_DMA) == 0
  while(Size--)
  {
    *pData = *(volatile uint16_t *)LCD_ADDR_DATA;
    pData++;
  }

  #else
  LCD_IO_ReadMultiData16(pData, Size);
  #endif
}

//-----------------------------------------------------------------------------
void LCD_IO_ReadCmd16MultipleData24to16(uint16_t Cmd, uint16_t *pData, uint32_t Size, uint32_t DummySize)
{
  union packed
  {
    uint8_t  rgb888[6];
    uint16_t rgb888_16[3];
  }u;

  #ifdef  __GNUC__
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wunused-but-set-variable"
  uint16_t DummyData;
  #pragma GCC diagnostic pop
  #elif   defined(__CC_ARM)
  uint16_t DummyData;
  #endif

  WaitForDmaEnd();
  *(volatile uint16_t *)LCD_ADDR_BASE = Cmd;

  while(DummySize--)
    DummyData = *(volatile uint16_t *)LCD_ADDR_DATA;

  while(Size--)
  {
    u.rgb888_16[0] = *(volatile uint16_t*)LCD_ADDR_DATA;
    u.rgb888_16[1] = *(volatile uint16_t*)LCD_ADDR_DATA;
    u.rgb888_16[2] = *(volatile uint16_t*)LCD_ADDR_DATA;

    *pData = ((u.rgb888[1] & 0xF8) << 8 | (u.rgb888[0] & 0xFC) << 3 | u.rgb888[3] >> 3);
    pData++;
    if(Size)
    {
      *pData = ((u.rgb888[2] & 0xF8) << 8 | (u.rgb888[5] & 0xFC) << 3 | u.rgb888[4] >> 3);
      pData++;
      Size--;
    }
  }
}
