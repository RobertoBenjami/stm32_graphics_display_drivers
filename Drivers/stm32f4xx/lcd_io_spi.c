/*
 * SPI LCD driver STM32F4
 * készitö: Roberto Benjami
 * verzio:  2019.05
 *
 * - hardver és szoftver SPI
 * - 3 féle üzemmód (csak TX, half duplex, full duplex)
*/

//-----------------------------------------------------------------------------
#include "main.h"
#include "lcd.h"
#include "lcd_io_spi.h"

#if LCD_REVERSE16 == 0
#define RD(a)                 a
#endif

#if LCD_REVERSE16 == 1
#define RD(a)                 __REVSH(a)
#endif

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

//-----------------------------------------------------------------------------
// portláb mádok (PP: push-pull, OD: open drain, FF: input floating)
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

#define BITBAND_ACCESS(v, b)  *(volatile uint32_t*)(((uint32_t)&v & 0xF0000000) + 0x2000000 + (((uint32_t)&v & 0x000FFFFF) << 5) + (b << 2))

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

#define GPIOX_PORTTONUM_A     1
#define GPIOX_PORTTONUM_B     2
#define GPIOX_PORTTONUM_C     3
#define GPIOX_PORTTONUM_D     4
#define GPIOX_PORTTONUM_E     5
#define GPIOX_PORTTONUM_F     6
#define GPIOX_PORTTONUM_G     7
#define GPIOX_PORTTONUM_H     8
#define GPIOX_PORTTONUM_J     9
#define GPIOX_PORTTONUM_K     10
#define GPIOX_PORTTONUM_L     11
#define GPIOX_PORTTONUM_M     12
#define GPIOX_PORTNUM_(p, m)  GPIOX_PORTTONUM_ ## p
#define GPIOX_PORTNAME_(p, m) p
#define GPIOX_PORTNUM(x)      GPIOX_PORTNUM_(x)
#define GPIOX_PORTNAME(x)     GPIOX_PORTNAME_(x)

//-----------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------
/* Parancs/adat láb üzemmod */
#define LCD_RS_CMD            GPIOX_ODR(LCD_RS) = 0
#define LCD_RS_DATA           GPIOX_ODR(LCD_RS) = 1

/* Reset láb aktiv/passziv */
#define LCD_RST_ON            GPIOX_ODR(LCD_RST) = 0
#define LCD_RST_OFF           GPIOX_ODR(LCD_RST) = 1

/* Chip select láb */
#define LCD_CS_ON             GPIOX_ODR(LCD_CS) = 0
#define LCD_CS_OFF            GPIOX_ODR(LCD_CS) = 1

/* Ha az olvasási sebesség ninc smegadva akkor megegyezik az irási sebességgel */
#ifndef LCD_SPI_SPD_READ
#define LCD_SPI_SPD_READ      LCD_SPI_SPD
#endif

//-----------------------------------------------------------------------------
#if LCD_SPI == 0
/* Szoftver SPI */
volatile uint16_t tmp16;

#define LCD_DUMMY_READ        {GPIOX_ODR(LCD_SCK) = 0; LCD_READ_DELAY; GPIOX_ODR(LCD_SCK) = 1;}

#if     LCD_SPI_MODE == 1
/* Kétirányu (halfduplex) SPI esetén az adatirányt váltogatni kell, és MISO láb = MOSI láb */
#undef  LCD_MISO
#define LCD_MISO              LCD_MOSI

#define LCD_DIRREAD(d)        {                 \
  GPIOX_MODER(MODE_DIGITAL_INPUT, LCD_MOSI);    \
  GPIOX_ODR(LCD_MOSI) = 0;                      \
  LCD_READ_DELAY;                               \
  while(d--)                                    \
  {                                             \
    GPIOX_ODR(LCD_SCK) = 0;                     \
    LCD_READ_DELAY;                             \
    GPIOX_ODR(LCD_SCK) = 1;                     \
  }                                             }

#define LCD_DIRWRITE(d)        GPIOX_MODER(MODE_OUT, LCD_MOSI)

#else
/* TX és fullduplex SPI esetén az adatirány fix */
#define LCD_DIRREAD(d)
#define LCD_DIRWRITE(d)
#endif

/* Write SPI sebesség */
#if     LCD_SPI_SPD == 0
#define LCD_WRITE_DELAY
#elif   LCD_SPI_SPD == 1
#define LCD_WRITE_DELAY       GPIOX_ODR(LCD_SCK) = 0
#else
#define LCD_WRITE_DELAY       LCD_IO_Delay(LCD_SPI_SPD - 2)
#endif

/* Read SPI sebesség */
#if     LCD_SPI_SPD_READ == 0
#define LCD_READ_DELAY
#elif   LCD_SPI_SPD_READ == 1
#define LCD_READ_DELAY        GPIOX_ODR(LCD_SCK) = 0
#else
#define LCD_READ_DELAY        LCD_IO_Delay(LCD_SPI_SPD_READ - 2)
#endif

#define LCD_WRITE_CLK         GPIOX_ODR(LCD_SCK) = 0; LCD_WRITE_DELAY; GPIOX_ODR(LCD_SCK) = 1;
#define LCD_READ_CLK          GPIOX_ODR(LCD_SCK) = 1; GPIOX_ODR(LCD_SCK) = 0; LCD_READ_DELAY;

#define LCD_WRITE8(d8) {                         \
  GPIOX_ODR(LCD_MOSI) = BITBAND_ACCESS(d8, 7);   \
  LCD_WRITE_CLK;                                 \
  GPIOX_ODR(LCD_MOSI) = BITBAND_ACCESS(d8, 6);   \
  LCD_WRITE_CLK;                                 \
  GPIOX_ODR(LCD_MOSI) = BITBAND_ACCESS(d8, 5);   \
  LCD_WRITE_CLK;                                 \
  GPIOX_ODR(LCD_MOSI) = BITBAND_ACCESS(d8, 4);   \
  LCD_WRITE_CLK;                                 \
  GPIOX_ODR(LCD_MOSI) = BITBAND_ACCESS(d8, 3);   \
  LCD_WRITE_CLK;                                 \
  GPIOX_ODR(LCD_MOSI) = BITBAND_ACCESS(d8, 2);   \
  LCD_WRITE_CLK;                                 \
  GPIOX_ODR(LCD_MOSI) = BITBAND_ACCESS(d8, 1);   \
  LCD_WRITE_CLK;                                 \
  GPIOX_ODR(LCD_MOSI) = BITBAND_ACCESS(d8, 0);   \
  LCD_WRITE_CLK;                                 }

#define LCD_READ8(d8) {                          \
  GPIOX_ODR(LCD_SCK) = 0;                        \
  LCD_READ_DELAY;                                \
  BITBAND_ACCESS(d8, 7) = GPIOX_IDR(LCD_MISO);   \
  LCD_READ_CLK;                                  \
  BITBAND_ACCESS(d8, 6) = GPIOX_IDR(LCD_MISO);   \
  LCD_READ_CLK;                                  \
  BITBAND_ACCESS(d8, 5) = GPIOX_IDR(LCD_MISO);   \
  LCD_READ_CLK;                                  \
  BITBAND_ACCESS(d8, 4) = GPIOX_IDR(LCD_MISO);   \
  LCD_READ_CLK;                                  \
  BITBAND_ACCESS(d8, 3) = GPIOX_IDR(LCD_MISO);   \
  LCD_READ_CLK;                                  \
  BITBAND_ACCESS(d8, 2) = GPIOX_IDR(LCD_MISO);   \
  LCD_READ_CLK;                                  \
  BITBAND_ACCESS(d8, 1) = GPIOX_IDR(LCD_MISO);   \
  LCD_READ_CLK;                                  \
  BITBAND_ACCESS(d8, 0) = GPIOX_IDR(LCD_MISO);   \
  GPIOX_ODR(LCD_SCK) = 1;                        }

#if LCD_REVERSE16 == 0
#define LCD_WRITE16(d16) {                       \
  GPIOX_ODR(LCD_MOSI) = BITBAND_ACCESS(d16, 15); \
  LCD_WRITE_CLK;                                 \
  GPIOX_ODR(LCD_MOSI) = BITBAND_ACCESS(d16, 14); \
  LCD_WRITE_CLK;                                 \
  GPIOX_ODR(LCD_MOSI) = BITBAND_ACCESS(d16, 13); \
  LCD_WRITE_CLK;                                 \
  GPIOX_ODR(LCD_MOSI) = BITBAND_ACCESS(d16, 12); \
  LCD_WRITE_CLK;                                 \
  GPIOX_ODR(LCD_MOSI) = BITBAND_ACCESS(d16, 11); \
  LCD_WRITE_CLK;                                 \
  GPIOX_ODR(LCD_MOSI) = BITBAND_ACCESS(d16, 10); \
  LCD_WRITE_CLK;                                 \
  GPIOX_ODR(LCD_MOSI) = BITBAND_ACCESS(d16, 9);  \
  LCD_WRITE_CLK;                                 \
  GPIOX_ODR(LCD_MOSI) = BITBAND_ACCESS(d16, 8);  \
  LCD_WRITE_CLK;                                 \
  GPIOX_ODR(LCD_MOSI) = BITBAND_ACCESS(d16, 7);  \
  LCD_WRITE_CLK;                                 \
  GPIOX_ODR(LCD_MOSI) = BITBAND_ACCESS(d16, 6);  \
  LCD_WRITE_CLK;                                 \
  GPIOX_ODR(LCD_MOSI) = BITBAND_ACCESS(d16, 5);  \
  LCD_WRITE_CLK;                                 \
  GPIOX_ODR(LCD_MOSI) = BITBAND_ACCESS(d16, 4);  \
  LCD_WRITE_CLK;                                 \
  GPIOX_ODR(LCD_MOSI) = BITBAND_ACCESS(d16, 3);  \
  LCD_WRITE_CLK;                                 \
  GPIOX_ODR(LCD_MOSI) = BITBAND_ACCESS(d16, 2);  \
  LCD_WRITE_CLK;                                 \
  GPIOX_ODR(LCD_MOSI) = BITBAND_ACCESS(d16, 1);  \
  LCD_WRITE_CLK;                                 \
  GPIOX_ODR(LCD_MOSI) = BITBAND_ACCESS(d16, 0);  \
  LCD_WRITE_CLK;                                 }

#define LCD_READ16(d16) {                        \
  GPIOX_ODR(LCD_SCK) = 0;                        \
  LCD_READ_DELAY;                                \
  BITBAND_ACCESS(d16, 15) = GPIOX_IDR(LCD_MISO); \
  LCD_READ_CLK;                                  \
  BITBAND_ACCESS(d16, 14) = GPIOX_IDR(LCD_MISO); \
  LCD_READ_CLK;                                  \
  BITBAND_ACCESS(d16, 13) = GPIOX_IDR(LCD_MISO); \
  LCD_READ_CLK;                                  \
  BITBAND_ACCESS(d16, 12) = GPIOX_IDR(LCD_MISO); \
  LCD_READ_CLK;                                  \
  BITBAND_ACCESS(d16, 11) = GPIOX_IDR(LCD_MISO); \
  LCD_READ_CLK;                                  \
  BITBAND_ACCESS(d16, 10) = GPIOX_IDR(LCD_MISO); \
  LCD_READ_CLK;                                  \
  BITBAND_ACCESS(d16, 9) = GPIOX_IDR(LCD_MISO);  \
  LCD_READ_CLK;                                  \
  BITBAND_ACCESS(d16, 8) = GPIOX_IDR(LCD_MISO);  \
  LCD_READ_CLK;                                  \
  BITBAND_ACCESS(d16, 7) = GPIOX_IDR(LCD_MISO);  \
  LCD_READ_CLK;                                  \
  BITBAND_ACCESS(d16, 6) = GPIOX_IDR(LCD_MISO);  \
  LCD_READ_CLK;                                  \
  BITBAND_ACCESS(d16, 5) = GPIOX_IDR(LCD_MISO);  \
  LCD_READ_CLK;                                  \
  BITBAND_ACCESS(d16, 4) = GPIOX_IDR(LCD_MISO);  \
  LCD_READ_CLK;                                  \
  BITBAND_ACCESS(d16, 3) = GPIOX_IDR(LCD_MISO);  \
  LCD_READ_CLK;                                  \
  BITBAND_ACCESS(d16, 2) = GPIOX_IDR(LCD_MISO);  \
  LCD_READ_CLK;                                  \
  BITBAND_ACCESS(d16, 1) = GPIOX_IDR(LCD_MISO);  \
  LCD_READ_CLK;                                  \
  BITBAND_ACCESS(d16, 0) = GPIOX_IDR(LCD_MISO);  \
  GPIOX_ODR(LCD_SCK) = 1;                        }
#endif

#if LCD_REVERSE16 == 1
#define LCD_WRITE16(d16) {                       \
  GPIOX_ODR(LCD_MOSI) = BITBAND_ACCESS(d16, 7);  \
  LCD_WRITE_CLK;                                 \
  GPIOX_ODR(LCD_MOSI) = BITBAND_ACCESS(d16, 6);  \
  LCD_WRITE_CLK;                                 \
  GPIOX_ODR(LCD_MOSI) = BITBAND_ACCESS(d16, 5);  \
  LCD_WRITE_CLK;                                 \
  GPIOX_ODR(LCD_MOSI) = BITBAND_ACCESS(d16, 4);  \
  LCD_WRITE_CLK;                                 \
  GPIOX_ODR(LCD_MOSI) = BITBAND_ACCESS(d16, 3);  \
  LCD_WRITE_CLK;                                 \
  GPIOX_ODR(LCD_MOSI) = BITBAND_ACCESS(d16, 2);  \
  LCD_WRITE_CLK;                                 \
  GPIOX_ODR(LCD_MOSI) = BITBAND_ACCESS(d16, 1);  \
  LCD_WRITE_CLK;                                 \
  GPIOX_ODR(LCD_MOSI) = BITBAND_ACCESS(d16, 0);  \
  LCD_WRITE_CLK;                                 \
  GPIOX_ODR(LCD_MOSI) = BITBAND_ACCESS(d16, 15); \
  LCD_WRITE_CLK;                                 \
  GPIOX_ODR(LCD_MOSI) = BITBAND_ACCESS(d16, 14); \
  LCD_WRITE_CLK;                                 \
  GPIOX_ODR(LCD_MOSI) = BITBAND_ACCESS(d16, 13); \
  LCD_WRITE_CLK;                                 \
  GPIOX_ODR(LCD_MOSI) = BITBAND_ACCESS(d16, 12); \
  LCD_WRITE_CLK;                                 \
  GPIOX_ODR(LCD_MOSI) = BITBAND_ACCESS(d16, 11); \
  LCD_WRITE_CLK;                                 \
  GPIOX_ODR(LCD_MOSI) = BITBAND_ACCESS(d16, 10); \
  LCD_WRITE_CLK;                                 \
  GPIOX_ODR(LCD_MOSI) = BITBAND_ACCESS(d16, 9);  \
  LCD_WRITE_CLK;                                 \
  GPIOX_ODR(LCD_MOSI) = BITBAND_ACCESS(d16, 8);  \
  LCD_WRITE_CLK;                                 \

#define LCD_READ16(d16) {                        \
  GPIOX_ODR(LCD_SCK) = 0;                        \
  LCD_READ_DELAY;                                \
  BITBAND_ACCESS(d16, 7) = GPIOX_IDR(LCD_MISO);  \
  LCD_READ_CLK;                                  \
  BITBAND_ACCESS(d16, 6) = GPIOX_IDR(LCD_MISO);  \
  LCD_READ_CLK;                                  \
  BITBAND_ACCESS(d16, 5) = GPIOX_IDR(LCD_MISO);  \
  LCD_READ_CLK;                                  \
  BITBAND_ACCESS(d16, 4) = GPIOX_IDR(LCD_MISO);  \
  LCD_READ_CLK;                                  \
  BITBAND_ACCESS(d16, 3) = GPIOX_IDR(LCD_MISO);  \
  LCD_READ_CLK;                                  \
  BITBAND_ACCESS(d16, 2) = GPIOX_IDR(LCD_MISO);  \
  LCD_READ_CLK;                                  \
  BITBAND_ACCESS(d16, 1) = GPIOX_IDR(LCD_MISO);  \
  LCD_READ_CLK;                                  \
  BITBAND_ACCESS(d16, 0) = GPIOX_IDR(LCD_MISO);  \
  LCD_READ_CLK;                                  \
  BITBAND_ACCESS(d16, 15) = GPIOX_IDR(LCD_MISO); \
  LCD_READ_CLK;                                  \
  BITBAND_ACCESS(d16, 14) = GPIOX_IDR(LCD_MISO); \
  LCD_READ_CLK;                                  \
  BITBAND_ACCESS(d16, 13) = GPIOX_IDR(LCD_MISO); \
  LCD_READ_CLK;                                  \
  BITBAND_ACCESS(d16, 12) = GPIOX_IDR(LCD_MISO); \
  LCD_READ_CLK;                                  \
  BITBAND_ACCESS(d16, 11) = GPIOX_IDR(LCD_MISO); \
  LCD_READ_CLK;                                  \
  BITBAND_ACCESS(d16, 10) = GPIOX_IDR(LCD_MISO); \
  LCD_READ_CLK;                                  \
  BITBAND_ACCESS(d16, 9) = GPIOX_IDR(LCD_MISO);  \
  LCD_READ_CLK;                                  \
  BITBAND_ACCESS(d16, 8) = GPIOX_IDR(LCD_MISO);  \
  GPIOX_ODR(LCD_SCK) = 1;                        }
#endif

#define LCD_DATA8_WRITE(d8)     {tmp8 = d8; LCD_WRITE8(tmp8);}
#define LCD_DATA8_READ(d8)      LCD_READ8(tmp8); d8 = tmp8;
#define LCD_CMD8_WRITE(cmd8)    {LCD_RS_CMD; LCD_WRITE8(cmd8); LCD_RS_DATA;}
#define LCD_DATA16_WRITE(d16)   {tmp16 = d16; LCD_WRITE16(tmp16);}
#define LCD_DATA16_READ(d16)    LCD_READ16(tmp16); d16 = tmp16;
#define LCD_CMD16_WRITE(cmd16)  {LCD_RS_CMD; LCD_WRITE16(cmd16); LCD_RS_DATA;}
#define LCD_SPI_MODE8
#define LCD_SPI_MODE16

#else    // #if LCD_SPI == 0
/* Hardver SPI */

#if LCD_SPI == 1
#define SPIX                  SPI1
#define LCD_SPI_RCC_EN        BITBAND_ACCESS(RCC->APB2ENR, RCC_APB2ENR_SPI1EN_Pos) = 1
#elif LCD_SPI == 2
#define SPIX                  SPI2
#define LCD_SPI_RCC_EN        BITBAND_ACCESS(RCC->APB1ENR, RCC_APB1ENR_SPI2EN_Pos) = 1
#elif LCD_SPI == 3
#define SPIX                  SPI3
#define LCD_SPI_RCC_EN        BITBAND_ACCESS(RCC->APB1ENR, RCC_APB1ENR_SPI3EN_Pos) = 1
#elif LCD_SPI == 4
#define SPIX                  SPI4
#define LCD_SPI_RCC_EN        BITBAND_ACCESS(RCC->APB2ENR, RCC_APB2ENR_SPI4EN_Pos) = 1
#elif LCD_SPI == 5
#define SPIX                  SPI5
#define LCD_SPI_RCC_EN        BITBAND_ACCESS(RCC->APB2ENR, RCC_APB2ENR_SPI5EN_Pos) = 1
#elif LCD_SPI == 6
#define SPIX                  SPI6
#define LCD_SPI_RCC_EN        BITBAND_ACCESS(RCC->APB2ENR, RCC_APB2ENR_SPI6EN_Pos) = 1
#endif

/* Read SPI sebesség */
#define LCD_READ_DELAY        LCD_IO_Delay(LCD_SPI_SPD_READ * 4)

#define LCD_DUMMY_READ(d)     {GPIOX_MODER(MODE_OUT, LCD_SCK);          \
                               while(d--)                               \
                               {                                        \
                                 GPIOX_ODR(LCD_SCK) = 0;                \
                                 LCD_READ_DELAY;                        \
                                 GPIOX_ODR(LCD_SCK) = 1;                \
                               }                                        \
                               GPIOX_MODER(MODE_ALTER, LCD_SCK);        }

#define LCD_SPI_MODE8         BITBAND_ACCESS(SPIX->CR1, SPI_CR1_DFF_Pos) = 0
#define LCD_SPI_MODE16        BITBAND_ACCESS(SPIX->CR1, SPI_CR1_DFF_Pos) = 1

#if     LCD_SPI_MODE == 1
/* Kétirányu (halfduplex) SPI esetén az adatirányt váltogatni kell */
#if (defined(LCD_SPI_SPD_READ) && (LCD_SPI_SPD != LCD_SPI_SPD_READ)) // Eltérö olvasási/irási sebesség
#define LCD_DIRREAD(d)  {                            \
  LCD_DUMMY_READ(d);                                 \
  SPIX->CR1 =                                        \
    (SPIX->CR1 & ~(SPI_CR1_BR | SPI_CR1_BIDIOE)) |   \
    (LCD_SPI_SPD_READ << SPI_CR1_BR_Pos);            }
#define LCD_DIRWRITE(d8) {                           \
  while(BITBAND_ACCESS(SPIX->SR, SPI_SR_RXNE_Pos))   \
    d8 = SPIX->DR;                                   \
  SPIX->CR1 &= ~SPI_CR1_SPE;                         \
  SPIX->CR1 = (SPIX->CR1 & ~SPI_CR1_BR) |            \
    ((LCD_SPI_SPD << SPI_CR1_BR_Pos) |               \
    SPI_CR1_BIDIOE);                                 \
  LCD_IO_Delay(2 ^ LCD_SPI_SPD_READ);                \
  while(BITBAND_ACCESS(SPIX->SR, SPI_SR_RXNE_Pos))   \
    d8 = SPIX->DR;                                   \
  SPIX->CR1 |= SPI_CR1_SPE;                          }

#else   // azonos sebesség, csak LCD_MOSI irányváltás
#define LCD_DIRREAD(d) {                             \
  LCD_DUMMY_READ(d);                                 \
  BITBAND_ACCESS(SPIX->CR1, SPI_CR1_BIDIOE_Pos) = 0; }
#define LCD_DIRWRITE(d8) {                           \
  while(BITBAND_ACCESS(SPIX->SR, SPI_SR_RXNE_Pos))   \
    d8 = SPIX->DR;                                   \
  SPIX->CR1 &= ~SPI_CR1_SPE;                         \
  SPIX->CR1 |= SPI_CR1_BIDIOE;                       \
  LCD_IO_Delay(2 ^ LCD_SPI_SPD_READ);                \
  while(BITBAND_ACCESS(SPIX->SR, SPI_SR_RXNE_Pos))   \
    d8 = SPIX->DR;                                   \
  SPIX->CR1 |= SPI_CR1_SPE;                          }
#endif

#else   // LCD_SPI_MODE == 1
// TX és fullduplex SPI esetén az adatirány fix
#if (defined(LCD_SPI_SPD_READ) && (LCD_SPI_SPD != LCD_SPI_SPD_READ)) // Eltérö olvasási/irási sebesség
#define LCD_DIRREAD(d) { \
  LCD_DUMMY_READ(d);     \
  SPIX->CR1 = (SPIX->CR1 & ~SPI_CR1_BR) | (LCD_SPI_SPD_READ << SPI_CR1_BR_Pos);}
#define LCD_DIRWRITE(d8)  { \
  SPIX->CR1 = (SPIX->CR1 & ~SPI_CR1_BR) | (LCD_SPI_SPD << SPI_CR1_BR_Pos);}
#else   // nincs irányváltás
#define LCD_DIRREAD(d)   LCD_DUMMY_READ(d)
#define LCD_DIRWRITE
#endif
#endif  // #else LCD_SPI_MODE == 1

#define LCD_DATA8_WRITE(d8)   {                   \
  SPIX->DR = d8;                                  \
  LCD_IO_Delay(2);                                \
  while(BITBAND_ACCESS(SPIX->SR, SPI_SR_BSY_Pos));}

#define LCD_DATA8_READ(d8)    {                      \
  while(!BITBAND_ACCESS(SPIX->SR, SPI_SR_RXNE_Pos)); \
  d8 = (uint8_t)SPIX->DR;                            }

#define LCD_CMD8_WRITE(cmd8)   {                  \
  LCD_RS_CMD;                                     \
  LCD_DATA8_WRITE(cmd8);                          \
  LCD_RS_DATA;                                    }

#define LCD_DATA16_WRITE(d16)   {                 \
  SPIX->DR = RD(d16);                             \
  LCD_IO_Delay(1);                                \
  while(BITBAND_ACCESS(SPIX->SR, SPI_SR_BSY_Pos));}
#define LCD_DATA16_READ(d16)    {                    \
  while(!BITBAND_ACCESS(SPIX->SR, SPI_SR_RXNE_Pos)); \
  d16 = RD(SPIX->DR);                                }

#define LCD_CMD16_WRITE(cmd16)  {                 \
  LCD_RS_CMD;                                     \
  LCD_DATA16_WRITE(cmd16);                        \
  LCD_RS_DATA;                                    }

#if DMANUM(LCD_DMA_TX) > 0 || DMANUM(LCD_DMA_RX) > 0
#ifdef  osFeature_Semaphore
#if DMANUM(LCD_DMA_TX) > 0
#define WAIT_FOR_DMA_END_TX   osSemaphoreWait(BinarySemDmaHandle, osWaitForever)
#endif
#if DMANUM(LCD_DMA_RX) > 0
#define WAIT_FOR_DMA_END_RX   osSemaphoreWait(BinarySemDmaHandle, osWaitForever)
#endif
#define TCIE                  DMA_SxCR_TCIE
#define LCD_DMA_IRQ
#else  // #ifdef  osFeature_Semaphore
#if DMANUM(LCD_DMA_TX) > 0
#define WAIT_FOR_DMA_END_TX   while(!(DMAX_ISR(LCD_DMA_TX) & DMAX_ISR_TCIF(LCD_DMA_TX)));
#endif
#if DMANUM(LCD_DMA_RX) > 0
#define WAIT_FOR_DMA_END_RX   while(!(DMAX_ISR(LCD_DMA_RX) & DMAX_ISR_TCIF(LCD_DMA_RX)));
#endif
#define TCIE                  0
#endif // #else  osFeature_Semaphore
#endif // #if DMANUM(LCD_DMA_TX) > 0 || DMANUM(LCD_DMA_RX) > 0

#if DMANUM(LCD_DMA_TX) > 0
/* SPI DMA WRITE(a: data pointer, b: number of data, c: 0=8 bit, 1=16bit, d: 0:MINC=off, 1:MINC=on */
#define LCD_SPI_DMA_WRITE(a, b, c, d) {                                       \
  DMAX_IFCR(LCD_DMA_TX) = DMAX_IFCR_CTCIF(LCD_DMA_TX) |  /* DMA IRQ clear */  \
    DMAX_IFCR_CHTIF(LCD_DMA_TX) | DMAX_IFCR_CTEIF(LCD_DMA_TX) |               \
    DMAX_IFCR_CDMEIF(LCD_DMA_TX) | DMAX_IFCR_CFEIF(LCD_DMA_TX);               \
  SPIX->CR1 &= ~SPI_CR1_SPE;           /* SPI stop */                         \
  DMAX_STREAMX(LCD_DMA_TX)->CR = 0;    /* DMA stop */                         \
  while(DMAX_STREAMX(LCD_DMA_TX)->CR & DMA_SxCR_EN);                          \
  DMAX_STREAMX(LCD_DMA_TX)->M0AR = (uint32_t)a;                               \
  DMAX_STREAMX(LCD_DMA_TX)->PAR = (uint32_t)&SPIX->DR;                        \
  DMAX_STREAMX(LCD_DMA_TX)->NDTR = b;                                         \
  DMAX_STREAMX(LCD_DMA_TX)->CR = TCIE |                                       \
    (c << DMA_SxCR_MSIZE_Pos) | (c << DMA_SxCR_PSIZE_Pos) |                   \
    (d << DMA_SxCR_MINC_Pos) | (0b01 << DMA_SxCR_DIR_Pos) |                   \
    (DMACHN(LCD_DMA_TX) << DMA_SxCR_CHSEL_Pos) |                              \
    (DMAPRIORITY(LCD_DMA_TX) << DMA_SxCR_PL_Pos);                             \
  DMAX_STREAMX(LCD_DMA_TX)->CR |= DMA_SxCR_EN;                                \
  BITBAND_ACCESS(SPIX->CR2, SPI_CR2_TXDMAEN_Pos) = 1;                         \
  SPIX->CR1 |= SPI_CR1_SPE;                                                   \
  WAIT_FOR_DMA_END_TX;                                                        \
  DMAX_STREAMX(LCD_DMA_TX)->CR = 0;                                           \
  while(DMAX_STREAMX(LCD_DMA_TX)->CR & DMA_SxCR_EN);                          \
  BITBAND_ACCESS(SPIX->CR2, SPI_CR2_TXDMAEN_Pos) = 0;                         \
  while(BITBAND_ACCESS(SPIX->SR, SPI_SR_BSY_Pos));                            \
  SPIX->CR1 &= ~SPI_CR1_SPE;                                                  \
  LCD_IO_Delay(2 ^ LCD_SPI_SPD);                                              \
  SPIX->CR1 |= SPI_CR1_SPE;                                                   }
#endif     // #if DMANUM(LCD_DMA_TX) > 0

#if DMANUM(LCD_DMA_RX) > 0
/* SPI DMA READ(a: data pointer, b: number of data, c: 0=8 bit, 1=16bit */
#define LCD_SPI_DMA_READ(a, b, c) {                                             \
  DMAX_IFCR(LCD_DMA_RX) = DMAX_IFCR_CTCIF(LCD_DMA_RX) |  /* DMA IRQ clear */    \
    DMAX_IFCR_CHTIF(LCD_DMA_RX) | DMAX_IFCR_CTEIF(LCD_DMA_RX) |                 \
    DMAX_IFCR_CDMEIF(LCD_DMA_RX) | DMAX_IFCR_CFEIF(LCD_DMA_RX);                 \
  DMAX_STREAMX(LCD_DMA_RX)->CR = 0;  /* DMA stop */                             \
  while(DMAX_STREAMX(LCD_DMA_RX)->CR & DMA_SxCR_EN);                            \
  DMAX_STREAMX(LCD_DMA_RX)->M0AR = (uint32_t)a;  /* memory addr */              \
  DMAX_STREAMX(LCD_DMA_RX)->PAR = (uint32_t)&SPIX->DR; /* periph addr */        \
  DMAX_STREAMX(LCD_DMA_RX)->NDTR = b;           /* number of data */            \
  DMAX_STREAMX(LCD_DMA_RX)->CR = TCIE | (c << DMA_SxCR_MSIZE_Pos) |             \
    (c << DMA_SxCR_PSIZE_Pos) | DMA_SxCR_MINC |    /* bitdepht */               \
    (DMACHN(LCD_DMA_RX) << DMA_SxCR_CHSEL_Pos) |                                \
    (DMAPRIORITY(LCD_DMA_RX) << DMA_SxCR_PL_Pos | (0 << DMA_SxCR_MBURST_Pos));  \
  DMAX_STREAMX(LCD_DMA_RX)->CR |= DMA_SxCR_EN;  /* DMA start */                 \
  BITBAND_ACCESS(SPIX->CR2, SPI_CR2_RXDMAEN_Pos) = 1; /* SPI DMA eng */         \
  WAIT_FOR_DMA_END_RX;                                                          \
  BITBAND_ACCESS(SPIX->CR2, SPI_CR2_RXDMAEN_Pos) = 0; /* SPI DMA tilt */        \
  while(BITBAND_ACCESS(SPIX->SR, SPI_SR_RXNE_Pos)) tmp8 = SPIX->DR;             \
  SPIX->CR1 &= ~SPI_CR1_SPE;                                                    \
  SPIX->CR1 = (SPIX->CR1 & ~SPI_CR1_BR) |                                       \
    ((LCD_SPI_SPD << SPI_CR1_BR_Pos) | SPI_CR1_BIDIOE);                         \
  LCD_IO_Delay(2 ^ LCD_SPI_SPD_READ);                                           \
  while(BITBAND_ACCESS(SPIX->SR, SPI_SR_RXNE_Pos)) tmp8 = SPIX->DR;             \
  SPIX->CR1 |= SPI_CR1_SPE;                                                     \
  DMAX_STREAMX(LCD_DMA_RX)->CR = 0;                                             \
  while(DMAX_STREAMX(LCD_DMA_RX)->CR & DMA_SxCR_EN);                            }

/* SPI DMA READ(a: data pointer, b: number of data, c: tempbuffer pointer, d: tempbuffer size */
#define LCD_SPI_DMA_READ24TO16(a, b, c, d) {                                    \
  uint32_t rp = 0, rgbcnt = 0;        /* buffer olv pointer */                  \
  uint32_t nd;                                                                  \
  DMAX_IFCR(LCD_DMA_RX) = DMAX_IFCR_CTCIF(LCD_DMA_RX) |  /* DMA IRQ clear */    \
    DMAX_IFCR_CHTIF(LCD_DMA_RX) | DMAX_IFCR_CTEIF(LCD_DMA_RX) |                 \
    DMAX_IFCR_CDMEIF(LCD_DMA_RX) | DMAX_IFCR_CFEIF(LCD_DMA_RX);                 \
  DMAX_STREAMX(LCD_DMA_RX)->CR = 0;  /* DMA stop */                             \
  while(DMAX_STREAMX(LCD_DMA_RX)->CR & DMA_SxCR_EN);                            \
  DMAX_STREAMX(LCD_DMA_RX)->M0AR = (uint32_t)c;                                 \
  DMAX_STREAMX(LCD_DMA_RX)->PAR = (uint32_t)&SPIX->DR;                          \
  DMAX_STREAMX(LCD_DMA_RX)->NDTR = d;                                           \
  nd = d;                                                                       \
  DMAX_STREAMX(LCD_DMA_RX)->CR = (0b00 << DMA_SxCR_MSIZE_Pos) |                 \
    (0b00 << DMA_SxCR_PSIZE_Pos) | DMA_SxCR_MINC | (0 << DMA_SxCR_MBURST_Pos) | \
    (DMACHN(LCD_DMA_RX) << DMA_SxCR_CHSEL_Pos) |                                \
    (DMAPRIORITY(LCD_DMA_RX) << DMA_SxCR_PL_Pos) | DMA_SxCR_CIRC;               \
  DMAX_STREAMX(LCD_DMA_RX)->CR |= DMA_SxCR_EN;                                  \
  BITBAND_ACCESS(SPIX->CR2, SPI_CR2_RXDMAEN_Pos) = 1;                           \
  while(b)                                                                      \
  {                                                                             \
    if(nd != DMAX_STREAMX(LCD_DMA_RX)->NDTR)                                    \
    {                                                                           \
      if(!--nd)                                                                 \
        nd = LCD_DMA_RX_BUFSIZE;                                                \
      rgb888[rgbcnt++] = da[rp++];                                              \
      rp &= (LCD_DMA_RX_BUFSIZE - 1);                                           \
      if(rgbcnt == 3)                                                           \
      {                                                                         \
        rgbcnt = 0;                                                             \
        b--;                                                                    \
        *a++ = RD((rgb888[0] & 0b11111000) << 8 |                               \
                  (rgb888[1] & 0b11111100) << 3 | rgb888[2] >> 3);              \
      }                                                                         \
    }                                                                           \
  }                                                                             \
  BITBAND_ACCESS(SPIX->CR2, SPI_CR2_RXDMAEN_Pos) = 0;                           \
  while(BITBAND_ACCESS(SPIX->SR, SPI_SR_RXNE_Pos)) tmp8 = SPIX->DR;             \
  SPIX->CR1 = (SPIX->CR1 & ~SPI_CR1_BR) | ((LCD_SPI_SPD << SPI_CR1_BR_Pos) |    \
    SPI_CR1_BIDIOE) | SPI_CR1_SPE; LCD_IO_Delay(2 ^ LCD_SPI_SPD_READ);          \
  while(BITBAND_ACCESS(SPIX->SR, SPI_SR_RXNE_Pos)) tmp8 = SPIX->DR;             \
  DMAX_STREAMX(LCD_DMA_RX)->CR = 0;                                             \
  while(DMAX_STREAMX(LCD_DMA_RX)->CR & DMA_SxCR_EN);                            }
#endif   // #if DMANUM(LCD_DMA_RX) > 0

#if DMANUM(LCD_DMA_RX) > 0 && LCD_DMA_RX_BUFMODE == 1 && LCD_SPI_MODE > 0
uint8_t da[LCD_DMA_RX_BUFSIZE] __attribute__((aligned));
#endif

#endif   // #else LCD_SPI == 0

uint8_t   tmp8;

//-----------------------------------------------------------------------------
#ifdef LCD_DMA_IRQ
osSemaphoreId BinarySemDmaHandle;
#if DMANUM(LCD_DMA_TX) > 0
void DMAX_STREAMX_IRQHANDLER(LCD_DMA_TX)(void)
{
  if(DMAX_ISR(LCD_DMA_TX) & DMAX_ISR_TCIF(LCD_DMA_TX))
  {
    DMAX_IFCR(LCD_DMA_TX) = DMAX_IFCR_CTCIF(LCD_DMA_TX);
    osSemaphoreRelease(BinarySemDmaHandle);
  }
}
#endif

#if DMANUM(LCD_DMA_RX) > 0
void DMAX_STREAMX_IRQHANDLER(LCD_DMA_RX)(void)
{
  if(DMAX_ISR(LCD_DMA_RX) & DMAX_ISR_TCIF(LCD_DMA_RX))
  {
    DMAX_IFCR(LCD_DMA_RX) = DMAX_IFCR_CTCIF(LCD_DMA_RX);
    osSemaphoreRelease(BinarySemDmaHandle);
  }
  if(DMAX_ISR(LCD_DMA_RX) & DMAX_ISR_TEIF(LCD_DMA_RX))
  {
    while(1);
  }
  if(DMAX_ISR(LCD_DMA_RX) & DMAX_ISR_DMEIF(LCD_DMA_RX))
  {
    while(1);
  }
}
#endif

#endif

//-----------------------------------------------------------------------------
#pragma GCC push_options
#pragma GCC optimize("O0")
void LCD_IO_Delay(volatile uint32_t c)
{
  while(c--);
}
#pragma GCC pop_options

//-----------------------------------------------------------------------------
void LCD_Delay(uint32_t Delay)
{
  HAL_Delay(Delay);
}

//-----------------------------------------------------------------------------
void LCD_IO_Bl_OnOff(uint8_t Bl)
{
  #if (GPIOX_PORTNUM(LCD_BL) >= 1) && (GPIOX_PORTNUM(LCD_BL) <= 12)
  if(Bl)
    GPIOX_ODR(LCD_BL) = LCD_BLON;
  else
    GPIOX_ODR(LCD_BL) = 1 - LCD_BLON;
  #endif
}

//-----------------------------------------------------------------------------
void LCD_IO_Init(void)
{
  #if LCD_SPI_MODE == 2                 // Full duplex
  RCC->AHB1ENR |= (GPIOX_CLOCK(LCD_RS) | GPIOX_CLOCK(LCD_CS) | GPIOX_CLOCK(LCD_SCK) | GPIOX_CLOCK(LCD_MOSI) | GPIOX_CLOCK(LCD_MISO));
  GPIOX_MODER(MODE_DIGITAL_INPUT, LCD_MISO);
  #else                                 // TX vagy half duplex
  RCC->AHB1ENR |= GPIOX_CLOCK(LCD_RS) | GPIOX_CLOCK(LCD_CS) | GPIOX_CLOCK(LCD_SCK) | GPIOX_CLOCK(LCD_MOSI);
  #endif

  #if (GPIOX_PORTNUM(LCD_BL) >= 1) && (GPIOX_PORTNUM(LCD_BL) <= 12) // háttérvilágitás
  RCC->AHB1ENR |= GPIOX_CLOCK(LCD_BL);
  GPIOX_MODER(MODE_OUT, LCD_BL);
  LCD_IO_Bl_OnOff(1);
  #endif

  #if (GPIOX_PORTNUM(LCD_RST) >= 1) && (GPIOX_PORTNUM(LCD_RST) <= 12) // reset
  RCC->APB2ENR |= GPIOX_CLOCK(LCD_RST);
  GPIOX_MODER(MODE_OUT, LCD_RST);
  GPIOX_OSPEEDR(MODE_SPD_LOW, LCD_RST);
  LCD_RST_OFF;
  #endif

  LCD_RS_DATA;
  LCD_CS_OFF;
  GPIOX_MODER(MODE_OUT, LCD_RS);
  GPIOX_MODER(MODE_OUT, LCD_CS);

  GPIOX_OSPEEDR(MODE_SPD_VHIGH, LCD_RS);
  GPIOX_OSPEEDR(MODE_SPD_VHIGH, LCD_CS);
  GPIOX_OSPEEDR(MODE_SPD_VHIGH, LCD_SCK);
  GPIOX_OSPEEDR(MODE_SPD_VHIGH, LCD_MOSI);
  GPIOX_ODR(LCD_SCK) = 1;               // SCK = 1

  #if LCD_SPI == 0                      // Szoftver SPI
  GPIOX_MODER(MODE_OUT, LCD_SCK);
  GPIOX_MODER(MODE_OUT, LCD_MOSI);

  #else                                 // Hardver SPI
  LCD_SPI_RCC_EN;

  GPIOX_AFR(LCD_SPI_AFR, LCD_SCK);
  GPIOX_MODER(MODE_ALTER, LCD_SCK);
  GPIOX_AFR(LCD_SPI_AFR, LCD_MOSI);
  GPIOX_MODER(MODE_ALTER, LCD_MOSI);

  #if LCD_SPI_MODE == 1
  // Half duplex (adatirány váltogatás)
  SPIX->CR1 = SPI_CR1_CPHA | SPI_CR1_CPOL | SPI_CR1_MSTR/* | SPI_CR1_SPE*/ | SPI_CR1_SSM | SPI_CR1_SSI | (LCD_SPI_SPD << SPI_CR1_BR_Pos) | SPI_CR1_BIDIMODE | SPI_CR1_BIDIOE;
  #else // #if LCD_SPI_MODE == 1
  // TX vagy full duplex mod
  SPIX->CR1 = SPI_CR1_CPHA | SPI_CR1_CPOL | SPI_CR1_MSTR/* | SPI_CR1_SPE*/ | SPI_CR1_SSM | SPI_CR1_SSI | (LCD_SPI_SPD << SPI_CR1_BR_Pos);
  #endif // #else LCD_SPI_MODE == 1
  SPIX->CR1 |= SPI_CR1_SPE;

  #if DMANUM(LCD_DMA_TX) > 0 && DMANUM(LCD_DMA_RX) > 0 && DMANUM(LCD_DMA_TX) != DMANUM(LCD_DMA_RX)
  RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN; | RCC_AHB1ENR_DMA2EN;   // DMA1, DMA2 orajel be
  #elif DMANUM(LCD_DMA_TX) == 1 || DMANUM(LCD_DMA_RX) == 1
  RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;   // DMA1 orajel be
  #elif DMANUM(LCD_DMA_TX) == 2 || DMANUM(LCD_DMA_RX) == 2
  RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN;   // DMA2 orajel be
  #endif

  #endif // #else LCD_SPI == 0

  /* Set or Reset the control line */
  #if (GPIOX_PORTNUM(LCD_RST) >= 1) && (GPIOX_PORTNUM(LCD_RST) <= 12) // reset
  LCD_Delay(10);
  LCD_RST_ON;
  LCD_Delay(10);
  LCD_RST_OFF;
  #endif
  LCD_Delay(10);

  #ifdef LCD_DMA_IRQ
  osSemaphoreDef(myBinarySem01);
  BinarySemDmaHandle = osSemaphoreCreate(osSemaphore(myBinarySem01), 1);
  #if DMANUM(LCD_DMA_TX) > 0
  HAL_NVIC_SetPriority(DMAX_STREAMX_IRQ(LCD_DMA_TX), configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY, 0);
  HAL_NVIC_EnableIRQ(DMAX_STREAMX_IRQ(LCD_DMA_TX));
  #endif
  #if DMANUM(LCD_DMA_RX) > 0
  HAL_NVIC_SetPriority(DMAX_STREAMX_IRQ(LCD_DMA_RX), configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY, 0);
  HAL_NVIC_EnableIRQ(DMAX_STREAMX_IRQ(LCD_DMA_RX));
  #endif
  osSemaphoreWait(BinarySemDmaHandle, 1);
  #endif
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteCmd8(uint8_t Cmd)
{
  LCD_SPI_MODE8;
  LCD_CS_ON;
  LCD_CMD8_WRITE(Cmd);
  LCD_CS_OFF;
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteCmd16(uint16_t Cmd)
{
  LCD_SPI_MODE16;
  LCD_CS_ON;
  LCD_CMD16_WRITE(Cmd);
  LCD_CS_OFF;
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteData8(uint8_t Data)
{
  LCD_SPI_MODE8;
  LCD_CS_ON;
  LCD_DATA8_WRITE(Data);
  LCD_CS_OFF;
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteData16(uint16_t Data)
{
  LCD_SPI_MODE16;
  LCD_CS_ON;
  LCD_DATA16_WRITE(Data);
  LCD_CS_OFF;
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteCmd8DataFill16(uint8_t Cmd, uint16_t Data, uint32_t Size)
{
  LCD_SPI_MODE8;
  LCD_CS_ON;
  LCD_CMD8_WRITE(Cmd);
  LCD_SPI_MODE16;
  #if DMANUM(LCD_DMA_TX) == 0 || LCD_SPI == 0
  while(Size--)
  {
    LCD_DATA16_WRITE(Data);
  }

  #else
  uint16_t d = RD(Data);
  uint32_t DmaSize;
  while(Size)
  {
    if(Size > DMA_MAXSIZE)
      DmaSize = DMA_MAXSIZE;
    else
      DmaSize = Size;
    Size -= DmaSize;
    LCD_SPI_DMA_WRITE(&d, DmaSize, 1, 0);
  }
  #endif

  LCD_CS_OFF;
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteCmd8MultipleData8(uint8_t Cmd, uint8_t *pData, uint32_t Size)
{
  LCD_SPI_MODE8;
  LCD_CS_ON;
  LCD_CMD8_WRITE(Cmd);

  #if DMANUM(LCD_DMA_TX) == 0 || LCD_SPI == 0
  while(Size--)
  {
    LCD_DATA8_WRITE(*pData);
    pData ++;
  }

  #else
  uint32_t DmaSize;
  while(Size)
  {
    if(Size > DMA_MAXSIZE)
      DmaSize = DMA_MAXSIZE;
    else
      DmaSize = Size;
    Size -= DmaSize;
    LCD_SPI_DMA_WRITE(pData, DmaSize, 0, 1);
    pData += DmaSize;
  }
  #endif

  LCD_CS_OFF;
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteCmd8MultipleData16(uint8_t Cmd, uint16_t *pData, uint32_t Size)
{
  LCD_SPI_MODE8;
  LCD_CS_ON;
  LCD_CMD8_WRITE(Cmd);
  LCD_SPI_MODE16;

  #if DMANUM(LCD_DMA_TX) == 0 || LCD_SPI == 0 || LCD_REVERSE16 == 1
  while(Size--)
  {
    LCD_DATA16_WRITE(*pData);
    pData ++;
  }

  #else
  uint32_t DmaSize;
  while(Size)
  {
    if(Size > DMA_MAXSIZE)
      DmaSize = DMA_MAXSIZE;
    else
      DmaSize = Size;
    Size -= DmaSize;
    LCD_SPI_DMA_WRITE(pData, DmaSize, 1, 1);
    pData += DmaSize;
  }
  #endif

  LCD_CS_OFF;
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteCmd16DataFill16(uint16_t Cmd, uint16_t Data, uint32_t Size)
{
  LCD_SPI_MODE16;
  LCD_CS_ON;
  LCD_CMD16_WRITE(Cmd);

  #if DMANUM(LCD_DMA_TX) == 0 || LCD_SPI == 0
  while(Size--)
  {
    LCD_DATA16_WRITE(Data);
  }

  #else
  uint16_t d = RD(Data);

  uint32_t DmaSize;
  while(Size)
  {
    if(Size > DMA_MAXSIZE)
      DmaSize = DMA_MAXSIZE;
    else
      DmaSize = Size;
    Size -= DmaSize;
    LCD_SPI_DMA_WRITE(&d, DmaSize, 1, 0);
  }
  #endif

  LCD_CS_OFF;
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteCmd16MultipleData8(uint16_t Cmd, uint8_t *pData, uint32_t Size)
{
  LCD_SPI_MODE16;
  LCD_CS_ON;
  LCD_CMD16_WRITE(Cmd);
  LCD_SPI_MODE8;

  #if DMANUM(LCD_DMA_TX) == 0 || LCD_SPI == 0
  while(Size--)
  {
    LCD_DATA8_WRITE(*pData);
    pData ++;
  }

  #else
  uint32_t DmaSize;
  while(Size)
  {
    if(Size > DMA_MAXSIZE)
      DmaSize = DMA_MAXSIZE;
    else
      DmaSize = Size;
    Size -= DmaSize;
    LCD_SPI_DMA_WRITE(pData, DmaSize, 0, 1);
    pData += DmaSize;
  }
  #endif

  LCD_CS_OFF;
}

//-----------------------------------------------------------------------------
void LCD_IO_WriteCmd16MultipleData16(uint16_t Cmd, uint16_t *pData, uint32_t Size)
{
  LCD_SPI_MODE16;
  LCD_CS_ON;
  LCD_CMD16_WRITE(Cmd);

  #if DMANUM(LCD_DMA_TX) == 0 || LCD_SPI == 0 || LCD_REVERSE16 == 1
  while(Size--)
  {
    LCD_DATA16_WRITE(*pData);
    pData ++;
  }

  #else
  uint32_t DmaSize;
  while(Size)
  {
    if(Size > DMA_MAXSIZE)
      DmaSize = DMA_MAXSIZE;
    else
      DmaSize = Size;
    Size -= DmaSize;
    LCD_SPI_DMA_WRITE(pData, DmaSize, 1, 1);
    pData += DmaSize;
  }
  #endif

  LCD_CS_OFF;
}

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
  LCD_SPI_MODE8;
  LCD_CS_ON;
  LCD_CMD8_WRITE(Cmd);

  DummySize = (DummySize << 3) + LCD_SCK_EXTRACLK;
  LCD_DIRREAD(DummySize);

  #if DMANUM(LCD_DMA_RX) == 0 || LCD_SPI == 0
  while(Size--)
  {
    LCD_DATA8_READ(*pData);
    pData++;
  }
  LCD_CS_OFF;
  LCD_DIRWRITE(tmp8);

  #else
  uint32_t DmaSize;
  while(Size)
  {
    if(Size > DMA_MAXSIZE)
      DmaSize = DMA_MAXSIZE;
    else
      DmaSize = Size;
    Size -= DmaSize;
    LCD_SPI_DMA_READ(pData, DmaSize, 0);
    pData += DmaSize;
  }

  // LCD_SPI_DMA_READ(pData, Size, 0);
  LCD_CS_OFF;
  #endif
}

//-----------------------------------------------------------------------------
void LCD_IO_ReadCmd8MultipleData16(uint8_t Cmd, uint16_t *pData, uint32_t Size, uint32_t DummySize)
{
  LCD_SPI_MODE8;
  LCD_CS_ON;
  LCD_CMD8_WRITE(Cmd);
  LCD_SPI_MODE16;

  DummySize = (DummySize << 3) + LCD_SCK_EXTRACLK;
  LCD_DIRREAD(DummySize);

  #if DMANUM(LCD_DMA_RX) == 0 || LCD_SPI == 0 || LCD_REVERSE16 == 1
  while(Size--)
  {
    LCD_DATA16_READ(*pData);
    pData++;
  }
  LCD_CS_OFF;
  LCD_DIRWRITE(tmp8);

  #else
  LCD_SPI_DMA_READ(pData, Size, 1);
  LCD_CS_OFF;
  #endif
}

//-----------------------------------------------------------------------------
void LCD_IO_ReadCmd8MultipleData24to16(uint8_t Cmd, uint16_t *pData, uint32_t Size, uint32_t DummySize)
{
  uint8_t rgb888[3];
  LCD_SPI_MODE8;
  LCD_CS_ON;
  LCD_CMD8_WRITE(Cmd);

  DummySize = (DummySize << 3) + LCD_SCK_EXTRACLK;
  LCD_DIRREAD(DummySize);

  #if DMANUM(LCD_DMA_RX) == 0 || LCD_SPI == 0
  while(Size--)
  {
    LCD_DATA8_READ(rgb888[0]);
    LCD_DATA8_READ(rgb888[1]);
    LCD_DATA8_READ(rgb888[2]);
    *pData = RD((rgb888[0] & 0b11111000) << 8 | (rgb888[1] & 0b11111100) << 3 | rgb888[2] >> 3);
    pData++;
  }
  LCD_CS_OFF;
  LCD_DIRWRITE(tmp8);

  #else
  #if LCD_DMA_RX_BUFMODE == 0
  uint8_t da[LCD_DMA_RX_BUFSIZE];
  #endif

  LCD_SPI_DMA_READ24TO16(pData, Size, &da, LCD_DMA_RX_BUFSIZE);
  LCD_CS_OFF;
  #endif
}

//-----------------------------------------------------------------------------
void LCD_IO_ReadCmd16MultipleData8(uint16_t Cmd, uint8_t *pData, uint32_t Size, uint32_t DummySize)
{
  LCD_SPI_MODE16;
  LCD_CS_ON;
  LCD_CMD16_WRITE(Cmd);
  LCD_SPI_MODE8;

  DummySize = (DummySize << 3) + LCD_SCK_EXTRACLK;
  LCD_DIRREAD(DummySize);

  #if DMANUM(LCD_DMA_RX) == 0 || LCD_SPI == 0
  uint8_t  d;

  while(Size--)
  {
    LCD_DATA8_READ(d);
    *pData = d;
    pData++;
  }
  LCD_CS_OFF;
  LCD_DIRWRITE(tmp8);

  #else
  LCD_SPI_DMA_READ(pData, Size, 0);
  LCD_CS_OFF;
  #endif
}

//-----------------------------------------------------------------------------
void LCD_IO_ReadCmd16MultipleData16(uint16_t Cmd, uint16_t *pData, uint32_t Size, uint32_t DummySize)
{
  LCD_SPI_MODE16;
  LCD_CS_ON;
  LCD_CMD16_WRITE(Cmd);

  DummySize = (DummySize << 3) + LCD_SCK_EXTRACLK;
  LCD_DIRREAD(DummySize);

  #if DMANUM(LCD_DMA_RX) == 0 || LCD_SPI == 0 || LCD_REVERSE16 == 1

  LCD_SPI_MODE16;
  while(Size--)
  {
    LCD_DATA16_READ(*pData);
    pData++;
  }
  LCD_CS_OFF;
  LCD_DIRWRITE(tmp8);

  #else
  LCD_SPI_DMA_READ(pData, Size, 1);
  LCD_CS_OFF;
  #endif
}

//-----------------------------------------------------------------------------
void LCD_IO_ReadCmd16MultipleData24to16(uint16_t Cmd, uint16_t *pData, uint32_t Size, uint32_t DummySize)
{
  uint8_t  rgb888[3];
  LCD_SPI_MODE16;
  LCD_CS_ON;
  LCD_CMD16_WRITE(Cmd);
  LCD_SPI_MODE8;

  DummySize = (DummySize << 3) + LCD_SCK_EXTRACLK;
  LCD_DIRREAD(DummySize);

  #if DMANUM(LCD_DMA_RX) == 0 || LCD_SPI == 0
  while(Size--)
  {
    LCD_DATA8_READ(rgb888[0]);
    LCD_DATA8_READ(rgb888[1]);
    LCD_DATA8_READ(rgb888[2]);
    #if LCD_REVERSE16 == 0
    *pData = ((rgb888[0] & 0b11111000) << 8 | (rgb888[1] & 0b11111100) << 3 | rgb888[2] >> 3);
    #endif
    #if LCD_REVERSE16 == 1
    *pData = __REVSH((rgb888[0] & 0b11111000) << 8 | (rgb888[1] & 0b11111100) << 3 | rgb888[2] >> 3);
    #endif
    pData++;
  }
  LCD_CS_OFF;
  LCD_DIRWRITE(tmp8);

  #else
  #if LCD_DMA_RX_BUFMODE == 0
  uint8_t da[LCD_DMA_RX_BUFSIZE];
  #endif

  LCD_SPI_DMA_READ24TO16(pData, Size, &da, LCD_DMA_RX_BUFSIZE);
  LCD_CS_OFF;
  #endif
}

#endif // #else LCD_SPI_MODE == 0
