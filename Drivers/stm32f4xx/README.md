# stm32f4 i/o drivers

- lcd_io_spi.h / c
  SPI driver (software SPI, hardware SPI, hardware SPI with DMA)

- lcd_io_gpio8.h / c
  paralell 8 bits GPIO driver

- lcdts_io_gpio8.h / c
  paralell 8 bits GPIO driver with analog resistiv touchscreen

- lcd_io_gpio16.h / c
  paralell 16 bits GPIO driver

- lcd_io_fsmc8.h / c
  paralell 8 bits FMSC driver, optional DMA 
  (not included FSMC config, please config with cubemx)

- lcdts_io_fsmc8.h / c
  paralell 8 bits FMSC driver with analog resistiv touchscreen, optional DMA 
  (not included FSMC config, please config with cubemx)

- lcd_io_fsmc16.h / c
  paralell 16 bits FMSC driver, optional DMA
  (not included FSMC config, please config with cubemx)
