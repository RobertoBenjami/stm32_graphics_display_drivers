/*
 * SPI LCD driver STM32F7
 * author: Roberto Benjami
 * version:  2020.04
 *
 * - hardware, software SPI
 * - 3 modes (only TX, half duplex, full duplex)
*/

//-----------------------------------------------------------------------------
#include "main.h"
#include "lcd.h"
#include "lcd_io_spi.h"

#define DMA_MAXSIZE           0xFFFE

//-----------------------------------------------------------------------------
/* Link function for LCD peripheral */
void  LCD_Delay (uint32_t delay);
void  LCD_IO_Init(void);
void  LCD_IO_Bl_OnOff(uint8_t Bl);

void  LCD_IO_WriteCmd8(uint8_t Cmd);
void  LCD_IO_WriteCmd16(uint16_t Cmd);
void  LCD_IO_WriteData8(uint8_t Data);
void  LCD_IO_WriteData16(uint16_t Data);

void  LCD_IO_WriteCmd8DataFill16(uint8_t Cmd, uint16_t Data, uint32_t Size);
void  LCD_IO_WriteCmd8MultipleData8(uint8_t Cmd, uint8_t *pData, uint32_t Size);
void  LCD_IO_WriteCmd8MultipleData16(uint8_t Cmd, uint16_t *pData, uint32_t Size);
void  LCD_IO_WriteCmd16DataFill16(uint16_t Cmd, uint16_t Data, uint32_t Size);
void  LCD_IO_WriteCmd16MultipleData8(uint16_t Cmd, uint8_t *pData, uint32_t Size);
void  LCD_IO_WriteCmd16MultipleData16(uint16_t Cmd, uint16_t *pData, uint32_t Size);

void  LCD_IO_ReadCmd8MultipleData8(uint8_t Cmd, uint8_t *pData, uint32_t Size, uint32_t DummySize);
void  LCD_IO_ReadCmd8MultipleData16(uint8_t Cmd, uint16_t *pData, uint32_t Size, uint32_t DummySize);
void  LCD_IO_ReadCmd8MultipleData24to16(uint8_t Cmd, uint16_t *pData, uint32_t Size, uint32_t DummySize);
void  LCD_IO_ReadCmd16MultipleData8(uint16_t Cmd, uint8_t *pData, uint32_t Size, uint32_t DummySize);
void  LCD_IO_ReadCmd16MultipleData16(uint16_t Cmd, uint16_t *pData, uint32_t Size, uint32_t DummySize);
void  LCD_IO_ReadCmd16MultipleData24to16(uint16_t Cmd, uint16_t *pData, uint32_t Size, uint32_t DummySize);

void  LCD_IO_Delay(uint32_t c);

//=============================================================================
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

#define GPIOX_SET_(a, b)      GPIO ## a ->BSRR = 1 << b
#define GPIOX_SET(a)          GPIOX_SET_(a)

#define GPIOX_CLR_(a, b)      GPIO ## a ->BSRR = 1 << (b + 16)
#define GPIOX_CLR(a)          GPIOX_CLR_(a)

#define GPIOX_IDR_(a, b)      (GPIO ## a ->IDR & (1 << b))
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

//-----------------------------------------------------------------------------
/* DMA */
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

// Interrupt event pl: if(DMAX_ISR(LCD_DMA_TX) & DMAX_ISR_TCIF(LCD_DMA_TX))...
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

// Interrupt clear pl: DMAX_IFCR(LCD_DMA_TX) = DMAX_IFCR_CTCIF(LCD_DMA_TX) | DMAX_IFCR_CFEIF(LCD_DMA_TX);
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

//=============================================================================
/* Command/data pin set */
#define LCD_RS_CMD            GPIOX_CLR(LCD_RS)
#define LCD_RS_DATA           GPIOX_SET(LCD_RS)

/* Reset pin set */
#define LCD_RST_ON            GPIOX_CLR(LCD_RST)
#define LCD_RST_OFF           GPIOX_SET(LCD_RST)

/* Chip select pin set */
#define LCD_CS_ON             GPIOX_CLR(LCD_CS)
#define LCD_CS_OFF            GPIOX_SET(LCD_CS)

/* If the read speed is undefinied -> is the same as writing speed */
#ifndef LCD_SPI_SPD_READ
#define LCD_SPI_SPD_READ      LCD_SPI_SPD_WRITE
#endif

#if GPIOX_PORTNUM(LCD_RS) < GPIOX_PORTNUM_A
#error  not definied the LCD RS pin
#endif

#if GPIOX_PORTNUM(LCD_CS) < GPIOX_PORTNUM_A
#error  not definied the LCD CS pin
#endif

#if GPIOX_PORTNUM(LCD_SCK) < GPIOX_PORTNUM_A
#error  not definied the LCD SCK pin
#endif

#if GPIOX_PORTNUM(LCD_MOSI) < GPIOX_PORTNUM_A
#error  not definied the LCD MOSI pin
#endif

#if GPIOX_PORTNUM(LCD_MISO) < GPIOX_PORTNUM_A && LCD_SPI_MODE == 2
#error  not definied the LCD MISO pin
#endif

//-----------------------------------------------------------------------------
#if LCD_SPI == 0
/* Software SPI */

/* Write SPI delay (0: none, 1: NOP, 2: clock pin keeping, 3.. LCD_IO_Delay) */
#if     LCD_SPI_SPD_WRITE == 0
#define LCD_WRITE_DELAY
#elif   LCD_SPI_SPD_WRITE == 1
#define LCD_WRITE_DELAY       __NOP()
#elif   LCD_SPI_SPD_WRITE == 2
#define LCD_WRITE_DELAY       GPIOX_CLR(LCD_SCK)
#else
#define LCD_WRITE_DELAY       LCD_IO_Delay(LCD_SPI_SPD_WRITE - 3)
#endif

/* Read SPI delay (0: none, 1: NOP, 2: clock pin keeping, 3.. LCD_IO_Delay) */
#if     LCD_SPI_SPD_READ == 0
#define LCD_READ_DELAY
#elif   LCD_SPI_SPD_READ == 1
#define LCD_READ_DELAY        __NOP()
#elif   LCD_SPI_SPD_READ == 2
#define LCD_READ_DELAY        GPIOX_CLR(LCD_SCK)
#else
#define LCD_READ_DELAY        LCD_IO_Delay(LCD_SPI_SPD_READ - 3)
#endif

#define WaitForDmaEnd()

#define LcdSpiMode8()
#define LcdSpiMode16()

#define LCD_WRITE_CLK         GPIOX_CLR(LCD_SCK); LCD_WRITE_DELAY; GPIOX_SET(LCD_SCK); LCD_WRITE_DELAY;
#define LCD_READ_CLK          GPIOX_SET(LCD_SCK); LCD_READ_DELAY; GPIOX_CLR(LCD_SCK); LCD_READ_DELAY;

void LcdWrite8(uint8_t d8)
{
  if(d8 & 0x80) GPIOX_SET(LCD_MOSI); else GPIOX_CLR(LCD_MOSI);
  LCD_WRITE_CLK;
  if(d8 & 0x40) GPIOX_SET(LCD_MOSI); else GPIOX_CLR(LCD_MOSI);
  LCD_WRITE_CLK;
  if(d8 & 0x20) GPIOX_SET(LCD_MOSI); else GPIOX_CLR(LCD_MOSI);
  LCD_WRITE_CLK;
  if(d8 & 0x10) GPIOX_SET(LCD_MOSI); else GPIOX_CLR(LCD_MOSI);
  LCD_WRITE_CLK;
  if(d8 & 0x08) GPIOX_SET(LCD_MOSI); else GPIOX_CLR(LCD_MOSI);
  LCD_WRITE_CLK;
  if(d8 & 0x04) GPIOX_SET(LCD_MOSI); else GPIOX_CLR(LCD_MOSI);
  LCD_WRITE_CLK;
  if(d8 & 0x02) GPIOX_SET(LCD_MOSI); else GPIOX_CLR(LCD_MOSI);
  LCD_WRITE_CLK;
  if(d8 & 0x01) GPIOX_SET(LCD_MOSI); else GPIOX_CLR(LCD_MOSI);
  LCD_WRITE_CLK;
}

void LcdWrite16(uint16_t d16)
{
  if(d16 & 0x8000) GPIOX_SET(LCD_MOSI); else GPIOX_CLR(LCD_MOSI);
  LCD_WRITE_CLK;
  if(d16 & 0x4000) GPIOX_SET(LCD_MOSI); else GPIOX_CLR(LCD_MOSI);
  LCD_WRITE_CLK;
  if(d16 & 0x2000) GPIOX_SET(LCD_MOSI); else GPIOX_CLR(LCD_MOSI);
  LCD_WRITE_CLK;
  if(d16 & 0x1000) GPIOX_SET(LCD_MOSI); else GPIOX_CLR(LCD_MOSI);
  LCD_WRITE_CLK;
  if(d16 & 0x0800) GPIOX_SET(LCD_MOSI); else GPIOX_CLR(LCD_MOSI);
  LCD_WRITE_CLK;
  if(d16 & 0x0400) GPIOX_SET(LCD_MOSI); else GPIOX_CLR(LCD_MOSI);
  LCD_WRITE_CLK;
  if(d16 & 0x0200) GPIOX_SET(LCD_MOSI); else GPIOX_CLR(LCD_MOSI);
  LCD_WRITE_CLK;
  if(d16 & 0x0100) GPIOX_SET(LCD_MOSI); else GPIOX_CLR(LCD_MOSI);
  LCD_WRITE_CLK;
  if(d16 & 0x0080) GPIOX_SET(LCD_MOSI); else GPIOX_CLR(LCD_MOSI);
  LCD_WRITE_CLK;
  if(d16 & 0x0040) GPIOX_SET(LCD_MOSI); else GPIOX_CLR(LCD_MOSI);
  LCD_WRITE_CLK;
  if(d16 & 0x0020) GPIOX_SET(LCD_MOSI); else GPIOX_CLR(LCD_MOSI);
  LCD_WRITE_CLK;
  if(d16 & 0x0010) GPIOX_SET(LCD_MOSI); else GPIOX_CLR(LCD_MOSI);
  LCD_WRITE_CLK;
  if(d16 & 0x0008) GPIOX_SET(LCD_MOSI); else GPIOX_CLR(LCD_MOSI);
  LCD_WRITE_CLK;
  if(d16 & 0x0004) GPIOX_SET(LCD_MOSI); else GPIOX_CLR(LCD_MOSI);
  LCD_WRITE_CLK;
  if(d16 & 0x0002) GPIOX_SET(LCD_MOSI); else GPIOX_CLR(LCD_MOSI);
  LCD_WRITE_CLK;
  if(d16 & 0x0001) GPIOX_SET(LCD_MOSI); else GPIOX_CLR(LCD_MOSI);
  LCD_WRITE_CLK;
}

#define LcdCmdWrite8(cmd8)      {LCD_RS_CMD; LcdWrite8(cmd8); LCD_RS_DATA;}
#define LcdCmdWrite16(cmd16)    {LCD_RS_CMD; LcdWrite16(cmd16); LCD_RS_DATA;}

#if LCD_SPI_MODE != 0
/* half duplex, full duplex */

#if     LCD_SPI_MODE == 1
/* Halfduplex mode : data direction change and MISO pin = MOSI pin */
#undef  LCD_MISO
#define LCD_MISO              LCD_MOSI
#define LcdDirWrite()         GPIOX_MODER(MODE_OUT, LCD_MOSI)

#elif   LCD_SPI_MODE == 2
/* Fullduplex SPI : data direction is fix, dummy read */
#define LcdDirWrite()
#endif  // #elif   LCD_SPI_MODE == 2

//-----------------------------------------------------------------------------
void LcdDirRead(uint32_t d)
{
  #if LCD_SPI_MODE == 1
  GPIOX_MODER(MODE_DIGITAL_INPUT, LCD_MOSI);
  GPIOX_CLR(LCD_MOSI);
  #endif
  while(d--)
  {
    LCD_READ_DELAY;
    GPIOX_CLR(LCD_SCK);
    LCD_READ_DELAY;
    GPIOX_SET(LCD_SCK);
  }
}

//-----------------------------------------------------------------------------
uint8_t LcdRead8(void)
{
  uint8_t d8;
  GPIOX_CLR(LCD_SCK);
  LCD_READ_DELAY;
  if(GPIOX_IDR(LCD_MISO)) d8 = 0x80; else d8 = 0;
  LCD_READ_CLK;
  if(GPIOX_IDR(LCD_MISO)) d8 |= 0x40;
  LCD_READ_CLK;
  if(GPIOX_IDR(LCD_MISO)) d8 |= 0x20;
  LCD_READ_CLK;
  if(GPIOX_IDR(LCD_MISO)) d8 |= 0x10;
  LCD_READ_CLK;
  if(GPIOX_IDR(LCD_MISO)) d8 |= 0x08;
  LCD_READ_CLK;
  if(GPIOX_IDR(LCD_MISO)) d8 |= 0x04;
  LCD_READ_CLK;
  if(GPIOX_IDR(LCD_MISO)) d8 |= 0x02;
  LCD_READ_CLK;
  if(GPIOX_IDR(LCD_MISO)) d8 |= 0x01;
  GPIOX_SET(LCD_SCK);
  return d8;
}

//-----------------------------------------------------------------------------
uint16_t LcdRead16(void)
{
  uint16_t d16;
  GPIOX_CLR(LCD_SCK);
  LCD_READ_DELAY;
  if(GPIOX_IDR(LCD_MISO)) d16 = 0x8000; else d16 = 0;
  LCD_READ_CLK;
  if(GPIOX_IDR(LCD_MISO)) d16 |= 0x4000;
  LCD_READ_CLK;
  if(GPIOX_IDR(LCD_MISO)) d16 |= 0x2000;
  LCD_READ_CLK;
  if(GPIOX_IDR(LCD_MISO)) d16 |= 0x1000;
  LCD_READ_CLK;
  if(GPIOX_IDR(LCD_MISO)) d16 |= 0x0800;
  LCD_READ_CLK;
  if(GPIOX_IDR(LCD_MISO)) d16 |= 0x0400;
  LCD_READ_CLK;
  if(GPIOX_IDR(LCD_MISO)) d16 |= 0x0200;
  LCD_READ_CLK;
  if(GPIOX_IDR(LCD_MISO)) d16 |= 0x0100;
  LCD_READ_CLK;
  if(GPIOX_IDR(LCD_MISO)) d16 |= 0x0080;
  LCD_READ_CLK;
  if(GPIOX_IDR(LCD_MISO)) d16 |= 0x0040;
  LCD_READ_CLK;
  if(GPIOX_IDR(LCD_MISO)) d16 |= 0x0020;
  LCD_READ_CLK;
  if(GPIOX_IDR(LCD_MISO)) d16 |= 0x0010;
  LCD_READ_CLK;
  if(GPIOX_IDR(LCD_MISO)) d16 |= 0x0008;
  LCD_READ_CLK;
  if(GPIOX_IDR(LCD_MISO)) d16 |= 0x0004;
  LCD_READ_CLK;
  if(GPIOX_IDR(LCD_MISO)) d16 |= 0x0002;
  LCD_READ_CLK;
  if(GPIOX_IDR(LCD_MISO)) d16 |= 0x0001;
  GPIOX_SET(LCD_SCK);
  return d16;
}
#endif  // #if LCD_SPI_MODE != 0

/* not using the DMA -> no need to wait for the end of the previous DMA operation */

//=============================================================================
#else    // #if LCD_SPI == 0
/* Hardware SPI */

#define LCD_WRITE_DELAY       LCD_IO_Delay(2 ^ (LCD_SPI_SPD_WRITE + 4))
#define LCD_READ_DELAY        LCD_IO_Delay(2 ^ (LCD_SPI_SPD_READ + 4))

#if LCD_SPI == 1
#define SPIX                  SPI1
#define LCD_SPI_RCC_EN        RCC->APB2ENR |= RCC_APB2ENR_SPI1EN
#elif LCD_SPI == 2
#define SPIX                  SPI2
#define LCD_SPI_RCC_EN        RCC->APB1ENR |= RCC_APB1ENR_SPI2EN
#elif LCD_SPI == 3
#define SPIX                  SPI3
#define LCD_SPI_RCC_EN        RCC->APB1ENR |= RCC_APB1ENR_SPI3EN
#elif LCD_SPI == 4
#define SPIX                  SPI4
#define LCD_SPI_RCC_EN        RCC->APB2ENR |= RCC_APB2ENR_SPI4EN
#elif LCD_SPI == 5
#define SPIX                  SPI5
#define LCD_SPI_RCC_EN        RCC->APB2ENR |= RCC_APB2ENR_SPI5EN
#elif LCD_SPI == 6
#define SPIX                  SPI6
#define LCD_SPI_RCC_EN        RCC->APB2ENR |= RCC_APB2ENR_SPI6EN
#endif

#define LcdSpiMode8()         SPIX->CR2 = (SPIX->CR2 & ~SPI_CR2_DS) | ((8 - 1) << SPI_CR2_DS_Pos)
#define LcdSpiMode16()        SPIX->CR2 = (SPIX->CR2 & ~SPI_CR2_DS) | ((16 - 1) << SPI_CR2_DS_Pos)

//-----------------------------------------------------------------------------
#if     LCD_SPI_MODE == 1
/* Halfduplex SPI : the direction of the data must be changed */

/* If -O0 optimize: in compiler -> inline functions are error. This is the solution */
extern inline void LcdDirRead(uint32_t d);

/* Data direction from OUT to IN. The parameter: dummy clock number */
inline void LcdDirRead(uint32_t d)
{
  GPIOX_MODER(MODE_DIGITAL_INPUT, LCD_MOSI);
  GPIOX_MODER(MODE_OUT, LCD_SCK);
  SPIX->CR1 &= ~SPI_CR1_SPE;
  while(d--)
  {
    GPIOX_CLR(LCD_SCK);
    LCD_READ_DELAY;
    GPIOX_SET(LCD_SCK);
    LCD_READ_DELAY;
  }
  SPIX->CR1 = (SPIX->CR1 & ~(SPI_CR1_BR | SPI_CR1_BIDIOE)) | (LCD_SPI_SPD_READ << SPI_CR1_BR_Pos);
  SPIX->CR1 |= SPI_CR1_SPE;
  while(!(SPIX->CR1 & SPI_CR1_SPE));
  GPIOX_MODER(MODE_ALTER, LCD_SCK);
  GPIOX_MODER(MODE_ALTER, LCD_MOSI);
}

/* Data direction from IN to OUT */
extern inline void LcdDirWrite(void);
inline void LcdDirWrite(void)
{
  volatile uint16_t d16 __attribute__((unused));
  while(SPIX->SR & SPI_SR_RXNE)
    d16 = *(uint16_t *)&SPIX->DR;
  SPIX->CR1 &= ~SPI_CR1_SPE;
  SPIX->CR1 = (SPIX->CR1 & ~SPI_CR1_BR) | ((LCD_SPI_SPD_WRITE << SPI_CR1_BR_Pos) | SPI_CR1_BIDIOE);
  // LCD_IO_Delay(2 ^ LCD_SPI_SPD_READ);
  while(SPIX->SR & SPI_SR_RXNE)
    d16 = *(uint16_t *)&SPIX->DR;
  SPIX->CR1 |= SPI_CR1_SPE;
}

#elif   LCD_SPI_MODE == 2
/* Fullduplex SPI : the direction is fix */
extern inline void LcdDirRead(uint32_t d);
inline void LcdDirRead(uint32_t d)
{
  GPIOX_MODER(MODE_OUT, LCD_SCK);
  while(d--)
  {
    GPIOX_CLR(LCD_SCK);
    LCD_READ_DELAY;
    GPIOX_SET(LCD_SCK);
  }
  GPIOX_MODER(MODE_ALTER, LCD_SCK);
  SPIX->CR1 = (SPIX->CR1 & ~SPI_CR1_BR) | (LCD_SPI_SPD_READ << SPI_CR1_BR_Pos);
}

extern inline void LcdDirWrite(void);
inline void LcdDirWrite(void)
{
  SPIX->CR1 = (SPIX->CR1 & ~SPI_CR1_BR) | (LCD_SPI_SPD_WRITE << SPI_CR1_BR_Pos);
}

#endif

//-----------------------------------------------------------------------------
extern inline void LcdWrite8(uint8_t d8);
inline void LcdWrite8(uint8_t d8)
{
  *(volatile uint8_t *)&SPIX->DR = d8;
  // LCD_IO_Delay(2);
  while(SPIX->SR & SPI_SR_BSY);
}

//-----------------------------------------------------------------------------
extern inline uint8_t LcdRead8(void);
inline uint8_t LcdRead8(void)
{
  uint8_t d8;
  while(!(SPIX->SR & SPI_SR_RXNE));
  d8 = *(uint8_t *)&SPIX->DR;
  return d8;
}

//-----------------------------------------------------------------------------
extern inline void LcdCmdWrite8(uint8_t cmd8);
inline void LcdCmdWrite8(uint8_t cmd8)
{
  LCD_RS_CMD;
  *(volatile uint8_t *)&SPIX->DR = cmd8;
  // LCD_IO_Delay(2);
  while(SPIX->SR & SPI_SR_BSY);
  LCD_RS_DATA;
}

//-----------------------------------------------------------------------------
extern inline void LcdWrite16(uint16_t d16);
inline void LcdWrite16(uint16_t d16)
{
  SPIX->DR = d16;
  LCD_IO_Delay(1);
  while(SPIX->SR & SPI_SR_BSY);
}

//-----------------------------------------------------------------------------
extern inline uint16_t LcdRead16(void);
inline uint16_t LcdRead16(void)
{
  uint16_t d16;
  while(!(SPIX->SR & SPI_SR_RXNE));
  d16 = SPIX->DR;
  return d16;
}

//-----------------------------------------------------------------------------
extern inline void LcdCmdWrite16(uint16_t cmd16);
inline void LcdCmdWrite16(uint16_t cmd16)
{
  LCD_RS_CMD;
  SPIX->DR = cmd16;
  LCD_IO_Delay(2);
  while(SPIX->SR & SPI_SR_BSY);
  LCD_RS_DATA;
}

#endif   // #else LCD_SPI == 0

//-----------------------------------------------------------------------------
#if (DMANUM(LCD_DMA_TX) > 0 || DMANUM(LCD_DMA_RX) > 0) && LCD_SPI > 0
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
#else   // osFeature_Semaphore
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

#endif  // #else osFeature_Semaphore

#else   // #if (DMANUM(LCD_DMA_TX) > 0 || DMANUM(LCD_DMA_RX) > 0) && LCD_SPI > 0

#define  WaitForDmaEnd()                /* if DMA is not used -> no need to wait */

#endif  // #else (DMANUM(LCD_DMA_TX) > 0 || DMANUM(LCD_DMA_RX) > 0) && LCD_SPI > 0

//-----------------------------------------------------------------------------
#if DMANUM(LCD_DMA_TX) == 0 || LCD_SPI == 0

/* SPI TX no DMA */

void LCD_IO_WriteMultiData8(uint8_t * pData, uint32_t Size, uint32_t dinc)
{
  while(Size--)
  {
    LcdWrite8(*pData);
    if(dinc)
      pData++;
  }
  LCD_CS_OFF;
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteMultiData16(uint16_t * pData, uint32_t Size, uint32_t dinc)
{
  while(Size--)
  {
    LcdWrite16(*pData);
    if(dinc)
      pData++;
  }
  LCD_CS_OFF;
}

#else // #if DMANUM(LCD_DMA_TX) == 0 || LCD_SPI == 0

//-----------------------------------------------------------------------------
/* SPI TX on DMA */

/* All DMA_TX interrupt flag clear */
#define DMAX_IFCRALL_LCD_DMA_TX                        { \
  DMAX_IFCR(LCD_DMA_TX) = DMAX_IFCR_CTCIF(LCD_DMA_TX)  | \
                          DMAX_IFCR_CHTIF(LCD_DMA_TX)  | \
                          DMAX_IFCR_CTEIF(LCD_DMA_TX)  | \
                          DMAX_IFCR_CDMEIF(LCD_DMA_TX) | \
                          DMAX_IFCR_CFEIF(LCD_DMA_TX)  ; }

//-----------------------------------------------------------------------------
void DMAX_STREAMX_IRQHANDLER(LCD_DMA_TX)(void)
{
  if(DMAX_ISR(LCD_DMA_TX) & DMAX_ISR_TCIF(LCD_DMA_TX))
  {
    DMAX_IFCR(LCD_DMA_TX) = DMAX_IFCR_CTCIF(LCD_DMA_TX);
    DMAX_STREAMX(LCD_DMA_TX)->CR = 0;
    while(DMAX_STREAMX(LCD_DMA_TX)->CR & DMA_SxCR_EN);
    SPIX->CR2 &= ~SPI_CR2_TXDMAEN;
    while(SPIX->SR & SPI_SR_BSY);
    SPIX->CR1 &= ~SPI_CR1_SPE;
    LCD_IO_Delay(2 ^ LCD_SPI_SPD_WRITE);
    SPIX->CR1 |= SPI_CR1_SPE;

    if(LCD_IO_DmaTransferStatus == 1) /* last transfer end ? */
      LCD_CS_OFF;

    #ifndef osFeature_Semaphore
    /* no FreeRtos */
    LCD_IO_DmaTransferStatus = 0;
    #else
    /* FreeRtos */
    osSemaphoreRelease(spiDmaBinSemHandle);
    #endif // #else osFeature_Semaphore
  }
  else
    DMAX_IFCRALL_LCD_DMA_TX;
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteMultiData(void * pData, uint32_t Size, uint32_t dmacr)
{
  DMAX_IFCRALL_LCD_DMA_TX;
  SPIX->CR1 &= ~SPI_CR1_SPE;           /* SPI stop */
  DMAX_STREAMX(LCD_DMA_TX)->CR = 0;    /* DMA stop */
  while(DMAX_STREAMX(LCD_DMA_TX)->CR & DMA_SxCR_EN);
  DMAX_STREAMX(LCD_DMA_TX)->M0AR = (uint32_t)pData;
  DMAX_STREAMX(LCD_DMA_TX)->PAR = (uint32_t)&SPIX->DR;
  DMAX_STREAMX(LCD_DMA_TX)->NDTR = Size;
  DMAX_STREAMX(LCD_DMA_TX)->CR = dmacr;
  DMAX_STREAMX(LCD_DMA_TX)->CR |= DMA_SxCR_EN;
  SPIX->CR2 |= SPI_CR2_TXDMAEN;
  SPIX->CR1 |= SPI_CR1_SPE;
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteMultiData8(uint8_t * pData, uint32_t Size, uint32_t dinc)
{
  uint32_t dmacr;
  static uint8_t d8s;
  if(!dinc)
  {
    d8s = *pData;
    pData = &d8s;
    dmacr = DMA_SxCR_TCIE |
            (0 << DMA_SxCR_MSIZE_Pos) | (0 << DMA_SxCR_PSIZE_Pos) |
            (0 << DMA_SxCR_MINC_Pos) | (1 << DMA_SxCR_DIR_Pos) |
            (DMACHN(LCD_DMA_TX) << DMA_SxCR_CHSEL_Pos) |
            (DMAPRIORITY(LCD_DMA_TX) << DMA_SxCR_PL_Pos);
  }
  else
    dmacr = DMA_SxCR_TCIE |
            (0 << DMA_SxCR_MSIZE_Pos) | (0 << DMA_SxCR_PSIZE_Pos) |
            (1 << DMA_SxCR_MINC_Pos) | (1 << DMA_SxCR_DIR_Pos) |
            (DMACHN(LCD_DMA_TX) << DMA_SxCR_CHSEL_Pos) |
            (DMAPRIORITY(LCD_DMA_TX) << DMA_SxCR_PL_Pos);

  #ifdef LCD_DMA_UNABLE
  if(LCD_DMA_UNABLE((uint32_t)(pData)))
  {
    while(Size--)
    {
      LcdWrite8(*pData);
      if(dinc)
        pData++;
    }
    LCD_CS_OFF;
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
  static uint16_t d16s;
  if(!dinc)
  {
    d16s = *pData;
    pData = &d16s;
    dmacr = DMA_SxCR_TCIE |
            (1 << DMA_SxCR_MSIZE_Pos) | (1 << DMA_SxCR_PSIZE_Pos) |
            (0 << DMA_SxCR_MINC_Pos) | (1 << DMA_SxCR_DIR_Pos) |
            (DMACHN(LCD_DMA_TX) << DMA_SxCR_CHSEL_Pos) |
            (DMAPRIORITY(LCD_DMA_TX) << DMA_SxCR_PL_Pos);
  }
  else
    dmacr = DMA_SxCR_TCIE |
            (1 << DMA_SxCR_MSIZE_Pos) | (1 << DMA_SxCR_PSIZE_Pos) |
            (1 << DMA_SxCR_MINC_Pos) | (1 << DMA_SxCR_DIR_Pos) |
            (DMACHN(LCD_DMA_TX) << DMA_SxCR_CHSEL_Pos) |
            (DMAPRIORITY(LCD_DMA_TX) << DMA_SxCR_PL_Pos);

  #ifdef LCD_DMA_UNABLE
  if(LCD_DMA_UNABLE((uint32_t)(pData)))
  {
    while(Size--)
    {
      LcdWrite16(*pData);
      if(dinc)
        pData++;
    }
    LCD_CS_OFF;
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

#endif // #else DMANUM(LCD_DMA_TX) == 0 || LCD_SPI == 0

//-----------------------------------------------------------------------------
#if LCD_SPI_MODE != 0
#if DMANUM(LCD_DMA_RX) == 0 || LCD_SPI == 0

void LCD_IO_ReadMultiData8(uint8_t * pData, uint32_t Size)
{
  uint8_t d8;
  while(Size--)
  {
    d8 = LcdRead8();
    *pData = d8;
    pData++;
  }
  LCD_CS_OFF;
  LcdDirWrite();
}

//-----------------------------------------------------------------------------
void LCD_IO_ReadMultiData16(uint16_t * pData, uint32_t Size)
{
  uint16_t d16;
  while(Size--)
  {
    d16 = LcdRead16();
    *pData = d16;
    pData++;
  }
  LCD_CS_OFF;
  LcdDirWrite();
}

//-----------------------------------------------------------------------------
void LCD_IO_ReadMultiData16to24(uint16_t * pData, uint32_t Size)
{
  uint8_t  rgb888[3];
  while(Size--)
  {
    rgb888[0] = LcdRead8();
    rgb888[1] = LcdRead8();
    rgb888[2] = LcdRead8();
    *pData = (rgb888[0] & 0XF8) << 8 | (rgb888[1] & 0xFC) << 3 | rgb888[2] >> 3;
    pData++;
  }
  LCD_CS_OFF;
  LcdDirWrite();
}

#elif DMANUM(LCD_DMA_RX) > 0 && LCD_SPI > 0

//-----------------------------------------------------------------------------
/* SPI RX on DMA */

/* All DMA_RX interrupt flag clear */
#define DMAX_IFCRALL_LCD_DMA_RX                        { \
  DMAX_IFCR(LCD_DMA_RX) = DMAX_IFCR_CTCIF(LCD_DMA_RX)  | \
                          DMAX_IFCR_CHTIF(LCD_DMA_RX)  | \
                          DMAX_IFCR_CTEIF(LCD_DMA_RX)  | \
                          DMAX_IFCR_CDMEIF(LCD_DMA_RX) | \
                          DMAX_IFCR_CFEIF(LCD_DMA_RX)  ; }

//-----------------------------------------------------------------------------
void DMAX_STREAMX_IRQHANDLER(LCD_DMA_RX)(void)
{
  volatile uint32_t d16 __attribute__((unused));
  if(DMAX_ISR(LCD_DMA_RX) & DMAX_ISR_TCIF(LCD_DMA_RX))
  {
    DMAX_IFCR(LCD_DMA_RX) = DMAX_IFCR_CTCIF(LCD_DMA_RX);
    DMAX_STREAMX(LCD_DMA_RX)->CR = 0;
    while(DMAX_STREAMX(LCD_DMA_RX)->CR & DMA_SxCR_EN);
    SPIX->CR2 &= ~SPI_CR2_RXDMAEN;
    while(SPIX->SR & SPI_SR_RXNE)
      d16 = *(uint16_t *)&SPIX->DR;
    SPIX->CR1 &= ~SPI_CR1_SPE;
    LCD_IO_Delay(2 ^ LCD_SPI_SPD_READ);
    SPIX->CR1 |= SPI_CR1_SPE;

    if(LCD_IO_DmaTransferStatus == 1) /* last transfer end ? */
      LCD_CS_OFF;

    #ifndef osFeature_Semaphore
    /* no FreeRtos */
    LCD_IO_DmaTransferStatus = 0;
    #else
    /* FreeRtos */
    osSemaphoreRelease(spiDmaBinSemHandle);
    #endif // #else osFeature_Semaphore
  }
  else
    DMAX_IFCRALL_LCD_DMA_RX;
}

//-----------------------------------------------------------------------------
void LCD_IO_ReadMultiData(void * pData, uint32_t Size, uint32_t dmacr)
{
  DMAX_IFCRALL_LCD_DMA_RX;
  SPIX->CR1 &= ~SPI_CR1_SPE;           /* SPI stop */
  DMAX_STREAMX(LCD_DMA_RX)->CR = 0;  /* DMA stop */
  while(DMAX_STREAMX(LCD_DMA_RX)->CR & DMA_SxCR_EN);
  DMAX_STREAMX(LCD_DMA_RX)->M0AR = (uint32_t)pData;  /* memory addr */
  DMAX_STREAMX(LCD_DMA_RX)->PAR = (uint32_t)&SPIX->DR; /* periph addr */
  DMAX_STREAMX(LCD_DMA_RX)->NDTR = Size;           /* number of data */
  DMAX_STREAMX(LCD_DMA_RX)->CR = dmacr;
  DMAX_STREAMX(LCD_DMA_RX)->CR |= DMA_SxCR_EN;  /* DMA start */
  SPIX->CR2 |= SPI_CR2_RXDMAEN;         /* SPI DMA eng */
  SPIX->CR1 |= SPI_CR1_SPE;
}

//-----------------------------------------------------------------------------
void LCD_IO_ReadMultiData8(uint8_t * pData, uint32_t Size)
{
  uint32_t dmacr;
  dmacr = DMA_SxCR_TCIE | (0 << DMA_SxCR_MSIZE_Pos) |
          (0 << DMA_SxCR_PSIZE_Pos) | DMA_SxCR_MINC |
          (DMACHN(LCD_DMA_RX) << DMA_SxCR_CHSEL_Pos) |
          (DMAPRIORITY(LCD_DMA_RX) << DMA_SxCR_PL_Pos)/* | (0 << DMA_SxCR_MBURST_Pos)*/;

  #ifdef LCD_DMA_UNABLE
  uint8_t d8;
  if(LCD_DMA_UNABLE((uint32_t)(pData)))
  {
    while(Size--)
    {
      d8 = LcdRead8();
      *pData = d8;
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
  LCD_CS_OFF;
  LcdDirWrite();
}

//-----------------------------------------------------------------------------
void LCD_IO_ReadMultiData16(uint16_t * pData, uint32_t Size)
{
  uint32_t dmacr;
  dmacr = DMA_SxCR_TCIE | (1 << DMA_SxCR_MSIZE_Pos) |
          (1 << DMA_SxCR_PSIZE_Pos) | DMA_SxCR_MINC |
          (DMACHN(LCD_DMA_RX) << DMA_SxCR_CHSEL_Pos) |
          (DMAPRIORITY(LCD_DMA_RX) << DMA_SxCR_PL_Pos) | (0 << DMA_SxCR_MBURST_Pos);

  #ifdef LCD_DMA_UNABLE
  uint16_t d16;
  if(LCD_DMA_UNABLE((uint32_t)(pData)))
  {
    while(Size--)
    {
      d16 = LcdRead16();
      *pData = d16;
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
  LCD_CS_OFF;
  LcdDirWrite();
}

//-----------------------------------------------------------------------------
void LCD_IO_ReadMultiData16to24(uint16_t * pData, uint32_t Size)
{
  volatile uint8_t d8 __attribute__((unused));
  uint32_t dmadata_ri = 0, rgb888cnt = 0; /* DMA tempbuffer index, rgb888 index (0..2) */
  uint32_t ntdr_follower;               /* NTDR register folower */
  uint8_t  rgb888[3];
  #if LCD_DMA_RX_BUFMODE == 0
  uint8_t dmadata[LCD_DMA_RX_BUFSIZE] __attribute__((aligned));
  #elif LCD_DMA_RX_BUFMODE == 1
  static uint8_t dmadata[LCD_DMA_RX_BUFSIZE] __attribute__((aligned));
  #elif LCD_DMA_RX_BUFMODE == 2
  uint8_t * dmadata;
  #endif

  #ifdef LCD_DMA_UNABLE
  if(LCD_DMA_UNABLE((uint32_t)(pData)))
  {
    while(Size--)
    {
      rgb888[0] = LcdRead8();
      rgb888[1] = LcdRead8();
      rgb888[2] = LcdRead8();
      *pData = (rgb888[0] & 0XF8) << 8 | (rgb888[1] & 0xFC) << 3 | rgb888[2] >> 3;
      pData++;
    }
  }
  else
  #endif
  {
    #if LCD_DMA_RX_BUFMODE == 2
    dmadata = LCD_DMA_RX_MALLOC(LCD_DMA_RX_BUFSIZE);
    if(!dmadata)
      return;
    #endif
    DMAX_IFCRALL_LCD_DMA_RX;
    SPIX->CR1 &= ~SPI_CR1_SPE;           /* SPI stop */
    DMAX_STREAMX(LCD_DMA_RX)->CR = 0;    /* DMA stop */
    while(DMAX_STREAMX(LCD_DMA_RX)->CR & DMA_SxCR_EN);
    DMAX_STREAMX(LCD_DMA_RX)->M0AR = (uint32_t)dmadata;
    DMAX_STREAMX(LCD_DMA_RX)->PAR = (uint32_t)&SPIX->DR;
    DMAX_STREAMX(LCD_DMA_RX)->NDTR = LCD_DMA_RX_BUFSIZE;
    ntdr_follower = LCD_DMA_RX_BUFSIZE;
    DMAX_STREAMX(LCD_DMA_RX)->CR = (0b00 << DMA_SxCR_MSIZE_Pos) | (0b00 << DMA_SxCR_PSIZE_Pos) |
      DMA_SxCR_MINC | (0 << DMA_SxCR_MBURST_Pos) | (DMACHN(LCD_DMA_RX) << DMA_SxCR_CHSEL_Pos) |
      (DMAPRIORITY(LCD_DMA_RX) << DMA_SxCR_PL_Pos) | DMA_SxCR_CIRC;
    DMAX_STREAMX(LCD_DMA_RX)->CR |= DMA_SxCR_EN;
    SPIX->CR2 |= SPI_CR2_RXDMAEN;
    SPIX->CR1 |= SPI_CR1_SPE;
    while(Size)
    {
      if(ntdr_follower != DMAX_STREAMX(LCD_DMA_RX)->NDTR)
      {
        if(!--ntdr_follower)
          ntdr_follower = LCD_DMA_RX_BUFSIZE;
        rgb888[rgb888cnt++] = dmadata[dmadata_ri++];
        if(dmadata_ri >= LCD_DMA_RX_BUFSIZE)
          dmadata_ri = 0;
        if(rgb888cnt == 3)
        {
          rgb888cnt = 0;
          Size--;
          *pData++ = (rgb888[0] & 0b11111000) << 8 | (rgb888[1] & 0b11111100) << 3 | rgb888[2] >> 3;
        }
      }
    }
    SPIX->CR2 &= ~SPI_CR2_RXDMAEN;
    while(SPIX->SR & SPI_SR_RXNE)
      d8 = SPIX->DR;
    SPIX->CR1 = (SPIX->CR1 & ~SPI_CR1_BR) | ((LCD_SPI_SPD_WRITE << SPI_CR1_BR_Pos) |
      SPI_CR1_BIDIOE) | SPI_CR1_SPE;
    LCD_IO_Delay(2 ^ LCD_SPI_SPD_READ);
    while(SPIX->SR & SPI_SR_RXNE)
      d8 = SPIX->DR;
    DMAX_STREAMX(LCD_DMA_RX)->CR = 0;
    while(DMAX_STREAMX(LCD_DMA_RX)->CR & DMA_SxCR_EN);
    #if LCD_DMA_RX_BUFMODE == 2
    LCD_DMA_RX_FREE(dmadata);
    #endif
  }

  LCD_CS_OFF;
  LcdDirWrite();
}

#endif // #elif DMANUM(LCD_DMA_RX) > 0 && LCD_SPI > 0
#endif // #if LCD_SPI_MODE != 0

//=============================================================================
// #pragma push, optimize off
#ifdef  __GNUC__
#pragma GCC push_options
#pragma GCC optimize("O0")
#elif   defined(__CC_ARM)
#pragma push
#pragma O0
#endif
void LCD_IO_Delay(uint32_t c)
{
  while(c--);
}
// #pragma pop
#ifdef  __GNUC__
#pragma GCC pop_options
#elif   defined(__CC_ARM)
#pragma pop
#endif

//=============================================================================
/* Public functions */

void LCD_Delay(uint32_t Delay)
{
  LCD_DELAY_MS(Delay);
}

//-----------------------------------------------------------------------------
void LCD_IO_Bl_OnOff(uint8_t Bl)
{
  #if GPIOX_PORTNUM(LCD_BL) >= GPIOX_PORTNUM_A
  if(Bl)
    #if LCD_BLON == 1
    GPIOX_SET(LCD_BL);
    #else
    GPIOX_CLR(LCD_BL);
    #endif
  else
    #if LCD_BLON == 1
    GPIOX_CLR(LCD_BL);
    #else
    GPIOX_SET(LCD_BL);
    #endif
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
  #if GPIOX_PORTNUM(LCD_BL) >= GPIOX_PORTNUM_A
  #define GPIOX_CLOCK_LCD_BL    GPIOX_CLOCK(LCD_BL)
  #else
  #define GPIOX_CLOCK_LCD_BL    0
  #endif

  /* MISO pin clock */
  #if GPIOX_PORTNUM(LCD_MISO) >= GPIOX_PORTNUM_A
  #define GPIOX_CLOCK_LCD_MISO  GPIOX_CLOCK(LCD_MISO)
  #else
  #define GPIOX_CLOCK_LCD_MISO  0
  #endif

  /* DMA clock */
  #if LCD_SPI == 0
  #define DMA1_CLOCK_TX         0
  #define DMA1_CLOCK_RX         0
  #else // #if LCD_SPI == 0
  #if DMANUM(LCD_DMA_TX) == 1
  #define DMA1_CLOCK_TX         RCC_AHB1ENR_DMA1EN
  #elif DMANUM(LCD_DMA_TX) == 2
  #define DMA1_CLOCK_TX         RCC_AHB1ENR_DMA2EN
  #else
  #define DMA1_CLOCK_TX         0
  #endif
  #if DMANUM(LCD_DMA_RX) == 1
  #define DMA1_CLOCK_RX         RCC_AHB1ENR_DMA1EN
  #elif DMANUM(LCD_DMA_RX) == 2
  #define DMA1_CLOCK_RX         RCC_AHB1ENR_DMA2EN
  #else
  #define DMA1_CLOCK_RX         0
  #endif
  #endif  // #else LCD_SPI == 0

  /* GPIO, DMA Clocks */
  RCC->AHB1ENR |= GPIOX_CLOCK(LCD_RS) | GPIOX_CLOCK(LCD_CS) | GPIOX_CLOCK(LCD_SCK) | GPIOX_CLOCK(LCD_MOSI) |
                  GPIOX_CLOCK_LCD_RST | GPIOX_CLOCK_LCD_BL  | GPIOX_CLOCK_LCD_MISO |
                  DMA1_CLOCK_TX | DMA1_CLOCK_RX;

  /* MISO = input in full duplex mode */
  #if LCD_SPI_MODE == 2                 // Full duplex
  GPIOX_MODER(MODE_DIGITAL_INPUT, LCD_MISO);
  #endif

  /* Backlight = output, light on */
  #if GPIOX_PORTNUM(LCD_BL) >= GPIOX_PORTNUM_A
  LCD_IO_Bl_OnOff(1);
  GPIOX_MODER(MODE_OUT, LCD_BL);
  #endif

  /* Reset pin = output, reset off */
  #if GPIOX_PORTNUM(LCD_RST) >= GPIOX_PORTNUM_A
  GPIOX_OSPEEDR(MODE_SPD_LOW, LCD_RST);
  LCD_RST_OFF;
  GPIOX_MODER(MODE_OUT, LCD_RST);
  #endif

  LCD_RS_DATA;
  LCD_CS_OFF;

  GPIOX_OSPEEDR(MODE_SPD_VHIGH, LCD_RS);
  GPIOX_OSPEEDR(MODE_SPD_VHIGH, LCD_CS);
  GPIOX_OSPEEDR(MODE_SPD_VHIGH, LCD_SCK);
  GPIOX_OSPEEDR(MODE_SPD_VHIGH, LCD_MOSI);
  GPIOX_SET(LCD_SCK);                   // SCK = 1

  GPIOX_MODER(MODE_OUT, LCD_RS);
  GPIOX_MODER(MODE_OUT, LCD_CS);

  #if LCD_SPI == 0
  /* Software SPI */
  GPIOX_MODER(MODE_OUT, LCD_SCK);
  GPIOX_MODER(MODE_OUT, LCD_MOSI);

  #else

  /* Hardware SPI */
  LCD_SPI_RCC_EN;

  GPIOX_AFR(LCD_SPI_AFR, LCD_SCK);
  GPIOX_AFR(LCD_SPI_AFR, LCD_MOSI);
  #if LCD_SPI_MODE == 2                 // Full duplex
  GPIOX_AFR(LCD_SPI_AFR, LCD_MISO);
  #endif

  #if LCD_SPI_MODE == 1
  /* Half duplex */
  SPIX->CR1 = SPI_CR1_CPHA | SPI_CR1_CPOL | SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI | (LCD_SPI_SPD_WRITE << SPI_CR1_BR_Pos) | SPI_CR1_BIDIMODE | SPI_CR1_BIDIOE;
  #else // #if LCD_SPI_MODE == 1
  /* TX or full duplex */
  SPIX->CR1 = SPI_CR1_CPHA | SPI_CR1_CPOL | SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI | (LCD_SPI_SPD_WRITE << SPI_CR1_BR_Pos);
  #endif // #else LCD_SPI_MODE == 1
  SPIX->CR2 = (8 - 1) << SPI_CR2_DS_Pos | SPI_CR2_FRXTH;
  SPIX->CR1 |= SPI_CR1_SPE;

  GPIOX_MODER(MODE_ALTER, LCD_SCK);
  GPIOX_MODER(MODE_ALTER, LCD_MOSI);

  #endif // #else LCD_SPI == 0

  /* Set or Reset the control line */
  #if GPIOX_PORTNUM(LCD_RST) >= GPIOX_PORTNUM_A // reset
  LCD_Delay(10);
  LCD_RST_ON;
  LCD_Delay(10);
  LCD_RST_OFF;
  #endif
  LCD_Delay(10);

  #if (DMANUM(LCD_DMA_TX) > 0 || DMANUM(LCD_DMA_RX) > 0) && LCD_SPI > 0
  #ifndef osFeature_Semaphore
  #define DMA_IRQ_PRIORITY    15
  #else
  #define DMA_IRQ_PRIORITY    configLIBRARY_LOWEST_INTERRUPT_PRIORITY
  #endif
  #if DMANUM(LCD_DMA_TX) > 0
  NVIC_SetPriority(DMAX_STREAMX_IRQ(LCD_DMA_TX), DMA_IRQ_PRIORITY);
  NVIC_EnableIRQ(DMAX_STREAMX_IRQ(LCD_DMA_TX));
  #endif
  #if DMANUM(LCD_DMA_RX) > 0
  NVIC_SetPriority(DMAX_STREAMX_IRQ(LCD_DMA_RX), DMA_IRQ_PRIORITY);
  NVIC_EnableIRQ(DMAX_STREAMX_IRQ(LCD_DMA_RX));
  #endif
  #ifdef osFeature_Semaphore
  osSemaphoreDef(spiDmaBinSem);
  spiDmaBinSemHandle = osSemaphoreCreate(osSemaphore(spiDmaBinSem), 1);
  osSemaphoreWait(spiDmaBinSemHandle, 1);
  #endif
  #endif  // #if DMANUM(LCD_DMA_RX) > 0
} // void LCD_IO_Init(void)

//-----------------------------------------------------------------------------
void LCD_IO_WriteCmd8(uint8_t Cmd)
{
  WaitForDmaEnd();
  LcdSpiMode8();
  LCD_CS_ON;
  LcdCmdWrite8(Cmd);
  LCD_CS_OFF;
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteCmd16(uint16_t Cmd)
{
  WaitForDmaEnd();
  LcdSpiMode16();
  LCD_CS_ON;
  LcdCmdWrite16(Cmd);
  LCD_CS_OFF;
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteData8(uint8_t Data)
{
  WaitForDmaEnd();
  LcdSpiMode8();
  LCD_CS_ON;
  LcdWrite8(Data);
  LCD_CS_OFF;
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteData16(uint16_t Data)
{
  WaitForDmaEnd();
  LcdSpiMode16();
  LCD_CS_ON;
  LcdWrite16(Data);
  LCD_CS_OFF;
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteCmd8DataFill16(uint8_t Cmd, uint16_t Data, uint32_t Size)
{
  WaitForDmaEnd();
  LcdSpiMode8();
  LCD_CS_ON;
  LcdCmdWrite8(Cmd);
  LcdSpiMode16();
  LCD_IO_WriteMultiData16(&Data, Size, 0);
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteCmd8MultipleData8(uint8_t Cmd, uint8_t *pData, uint32_t Size)
{
  WaitForDmaEnd();
  LcdSpiMode8();
  LCD_CS_ON;
  LcdCmdWrite8(Cmd);
  LCD_IO_WriteMultiData8(pData, Size, 1);
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteCmd8MultipleData16(uint8_t Cmd, uint16_t *pData, uint32_t Size)
{
  WaitForDmaEnd();
  LcdSpiMode8();
  LCD_CS_ON;
  LcdCmdWrite8(Cmd);
  LcdSpiMode16();
  LCD_IO_WriteMultiData16(pData, Size, 1);
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteCmd16DataFill16(uint16_t Cmd, uint16_t Data, uint32_t Size)
{
  WaitForDmaEnd();
  LcdSpiMode16();
  LCD_CS_ON;
  LcdCmdWrite16(Cmd);
  LCD_IO_WriteMultiData16(&Data, Size, 0);
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteCmd16MultipleData8(uint16_t Cmd, uint8_t *pData, uint32_t Size)
{
  WaitForDmaEnd();
  LcdSpiMode16();
  LCD_CS_ON;
  LcdCmdWrite16(Cmd);
  LcdSpiMode8();
  LCD_IO_WriteMultiData8(pData, Size, 1);
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteCmd16MultipleData16(uint16_t Cmd, uint16_t *pData, uint32_t Size)
{
  WaitForDmaEnd();
  LcdSpiMode16();
  LCD_CS_ON;
  LcdCmdWrite16(Cmd);
  LCD_IO_WriteMultiData16(pData, Size, 1);
}

//-----------------------------------------------------------------------------
#if LCD_SPI_MODE == 0
__weak void LCD_IO_ReadCmd8MultipleData8(uint8_t Cmd, uint8_t *pData, uint32_t Size, uint32_t DummySize) {}
__weak void LCD_IO_ReadCmd8MultipleData16(uint8_t Cmd, uint16_t *pData, uint32_t Size, uint32_t DummySize) {}
__weak void LCD_IO_ReadCmd8MultipleData24to16(uint8_t Cmd, uint16_t *pData, uint32_t Size, uint32_t DummySize) {}
__weak void LCD_IO_ReadCmd16MultipleData8(uint16_t Cmd, uint8_t *pData, uint32_t Size, uint32_t DummySize) {}
__weak void LCD_IO_ReadCmd16MultipleData16(uint16_t Cmd, uint16_t *pData, uint32_t Size, uint32_t DummySize) {}
__weak void LCD_IO_ReadCmd16MultipleData24to16(uint16_t Cmd, uint16_t *pData, uint32_t Size, uint32_t DummySize) {}
#else

//-----------------------------------------------------------------------------
void LCD_IO_ReadCmd8MultipleData8(uint8_t Cmd, uint8_t *pData, uint32_t Size, uint32_t DummySize)
{
  WaitForDmaEnd();
  LcdSpiMode8();
  LCD_CS_ON;
  LcdCmdWrite8(Cmd);

  DummySize = (DummySize << 3) + LCD_SCK_EXTRACLK;
  LcdDirRead(DummySize);
  LCD_IO_ReadMultiData8(pData, Size);
}

//-----------------------------------------------------------------------------
void LCD_IO_ReadCmd8MultipleData16(uint8_t Cmd, uint16_t *pData, uint32_t Size, uint32_t DummySize)
{
  WaitForDmaEnd();
  LcdSpiMode8();
  LCD_CS_ON;
  LcdCmdWrite8(Cmd);
  LcdSpiMode16();

  DummySize = (DummySize << 3) + LCD_SCK_EXTRACLK;
  LcdDirRead(DummySize);
  LCD_IO_ReadMultiData16(pData, Size);
}

//-----------------------------------------------------------------------------
void LCD_IO_ReadCmd8MultipleData24to16(uint8_t Cmd, uint16_t *pData, uint32_t Size, uint32_t DummySize)
{
  WaitForDmaEnd();
  LcdSpiMode8();
  LCD_CS_ON;
  LcdCmdWrite8(Cmd);

  DummySize = (DummySize << 3) + LCD_SCK_EXTRACLK;
  LcdDirRead(DummySize);
  LCD_IO_ReadMultiData16to24(pData, Size);
}

//-----------------------------------------------------------------------------
void LCD_IO_ReadCmd16MultipleData8(uint16_t Cmd, uint8_t *pData, uint32_t Size, uint32_t DummySize)
{
  WaitForDmaEnd();
  LcdSpiMode16();
  LCD_CS_ON;
  LcdCmdWrite16(Cmd);
  LcdSpiMode8();

  DummySize = (DummySize << 3) + LCD_SCK_EXTRACLK;
  LcdDirRead(DummySize);
  LCD_IO_ReadMultiData8(pData, Size);
}

//-----------------------------------------------------------------------------
void LCD_IO_ReadCmd16MultipleData16(uint16_t Cmd, uint16_t *pData, uint32_t Size, uint32_t DummySize)
{
  WaitForDmaEnd();
  LcdSpiMode16();
  LCD_CS_ON;
  LcdCmdWrite16(Cmd);

  DummySize = (DummySize << 3) + LCD_SCK_EXTRACLK;
  LcdDirRead(DummySize);
  LCD_IO_ReadMultiData16(pData, Size);
}

//-----------------------------------------------------------------------------
void LCD_IO_ReadCmd16MultipleData24to16(uint16_t Cmd, uint16_t *pData, uint32_t Size, uint32_t DummySize)
{
  WaitForDmaEnd();
  LcdSpiMode16();
  LCD_CS_ON;
  LcdCmdWrite16(Cmd);
  LcdSpiMode8();

  DummySize = (DummySize << 3) + LCD_SCK_EXTRACLK;
  LcdDirRead(DummySize);
  LCD_IO_ReadMultiData16to24(pData, Size);
}

#endif // #else LCD_SPI_MODE == 0
