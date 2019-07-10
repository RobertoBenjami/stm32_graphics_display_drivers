# ST7783 driver

st7783.h / c
  Options for built in analog resistiv touchscreen version 

FSMC, no DMA time config (168MHz M4)
  Timing.AddressSetupTime = 5;
  Timing.DataSetupTime = 8;
  Timing.BusTurnAroundDuration = 1;

FSMC, DMA time config (168MHz M4)
  Timing.AddressSetupTime = 5;
  Timing.DataSetupTime = 8;
  Timing.BusTurnAroundDuration = 14;
  