# STM32Fxxx graphics display drivers

Layer chart:
- Lcd_drv.pdf

Example (please unzip the app you like):
- stm32f103c8t (HAL applications, cubemx, truestudio)
- stm32f103c8t (HAL-FreeRtos applications, cubemx, truestudio)
- stm32f407vet (HAL-applications, cubemx, truestudio)
- stm32f407vet (HAL-FreeRtos applications, cubemx, truestudio)
- stm32f407vet (HAL applications, FSMC, cubemx, truestudio)
- stm32f407vet (HAL-FreeRtos applications, FSMC, cubemx, truestudio)

LCD I/O driver:
- stm32f1: spi_io (software SPI, hardware SPI, hardware SPI with DMA)
- stm32f1: gpio_io (8bit paralell with analog resistive touchscreen)
- stm32f4: spi_io (software SPI, hardware SPI, hardware SPI with DMA)
- stm32f4: gpio_io (8bit paralell with analog resistive touchscreen)
- stm32f4: fsmc_io (8bit paralell with analog resistive touchscreen + FSMC or FSMC with DMA)

LCD driver:
- st7735  (SPI mode tested)
- st7783  (8bit paralell mode tested)
- ili9325 (8bit paralell mode tested)
- ili9328 (8bit paralell mode tested)
- ili9488 (8bit paralell mode tested)
- hx8347g (8bit paralell mode tested)

App:
- LcdSpeedTest: Lcd speed test 
  (printf: the result, i use the SWO pin for ST-LINK Serial Wire Viewer (SWV))
- TouchCalib: Touchscreen calibration program 
  (printf: the result, i use the SWO pin for ST-LINK Serial Wire Viewer (SWV))
- Paint: Arduino paint clone

How to use starting from zero?
1. Create project for Cubemex
   - setting the RCC (Crystal/ceramic resonator)
   - setting the debug (SYS / serial wire or trace assyn sw)
   - setting the timebase source (i like the basic timer for this)
   - if FSMC : setting the FSMC (chip select, memory type = LCD, Lcd reg select, data = 8 bits, timing)
   - project settings: toolchain = truestudio, stack size = 0x800
   - generate source code
2. Truestudio
   - open projects from file system
   - open main.c
   - add USER CODE BEGIN PFP: void mainApp(void);
   - add USER CODE BEGIN WHILE: mainApp();
   - open main.h
   - add USER CODE BEGIN Includes: #include "stm32f1xx_hal.h" or #include "stm32f4xx_hal.h"
   - add 2 new folder for Src folder (App, Lcd)
   - copy files from App/... to App
   - copy 7 files from Drivers to Lcd (lcd.h, ts.h, bmp.h, stm32_adafruit_lcd.h / c, stm32_adafruit_ts.h / c)
   - copy Fonts folder to Lcd folder
   - copy io driver to Lcd folder (lcd_io_spi.h / c or lcdts_io8p_gpio.h / c or lcdts_io8p_fsmc.h / c)
   - copy lcd driver to Lcd folder (st7735.h / c or ili9325.h /c or...)
   - setting the configuration the io driver header file (pin settings)
   - add include path : Src/Lcd
   - comple ...
   
   