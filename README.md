# STM32Fxxx graphics display drivers

Layer chart:
- Lcd_drv.pdf

Example:
- LcdSpeedtest stm32f103c8t (cubemx, truestudio)
- LcdSpeedtest stm32f407vet (cubemx, truestudio)
- LcdSpeedtest stm32f407vet fsmc (cubemx, truestudio)

LCD I/O driver:
- stm32f1: spi_io
- stm32f1: gpio_io (8bit paralell)
- stm32f4: gpio_io (8bit paralell)
- stm32f4: fsmc_io (8bit paralell)

LCD driver:
- st7735
- st7783
- ili9325
- ili9328
- ili9488
- hx8347g

How to use?
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
   - copy 5 files from Drivers to Lcd (lcd.h, ts.h, bmp.h, stm32_adafruit_lcd.h, stm32_adafruit_lcd.c)
   - copy Fonts folder to Lcd folder
   - copy io driver to Lcd folder (lcd_io_spi.h / c or lcdts_io8p_gpio.h / c or lcdts_io8p_fsmc.h / c)
   - copy lcd driver to Lcd folder (st7735.h / c or ili9325.h /c or...)
   - setting the confifuration the io driver header file (pin settings)
   - add include path : Src/Lcd
   - comple ...
   
   