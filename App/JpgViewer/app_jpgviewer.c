#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "main.h"
#include "fatfs.h"
#include "jpeglib.h"
#include "jdata_conf.h"
#include "stm32_adafruit_lcd.h"

/* flash drive type (1=SD card, 2=USB pendrive */
#define FLASHTYPE             1

#define JPG_FILENAMEEXT ".jpg"
/* If search start folder not is the root folder */
// #define STARTFOLDER     "320x240_jpg/"

#if     FLASHTYPE == 1
#define FLASHPATH             SDPath
#define FLASHFATS             SDFatFS
#define FLASHPROCESS()
#define FLASHREADY            1
#endif

#if     FLASHTYPE == 2
#include "usb_host.h"
extern  ApplicationTypeDef    Appli_state;
#define FLASHPATH             USBHPath
#define FLASHFATS             USBHFatFS
#define FLASHPROCESS()        MX_USB_HOST_Process()
#define FLASHREADY            Appli_state == APPLICATION_READY
#endif

// ----------------------------------------------------------------------------
#ifdef  osCMSIS
#define Delay(t)              osDelay(t)
#define GetTime()             osKernelSysTick()
#else
#define Delay(t)              HAL_Delay(t)
#define GetTime()             HAL_GetTick()
#endif

typedef struct RGB
{
  uint8_t B;
  uint8_t G;
  uint8_t R;
}RGB_typedef;

RGB_typedef *RGB_matrix;
uint32_t line_counter = 0;

/* This struct contains the JPEG decompression parameters */
struct jpeg_decompress_struct cinfo;
/* This struct represents a JPEG error handler */
struct jpeg_error_mgr jerr;

uint16_t *RGB16Buffer;

// ----------------------------------------------------------------------------
static uint8_t Jpeg_CallbackFunction(uint8_t* Row, uint32_t DataLength)
{
  uint32_t i = 0;
  RGB_matrix =  (RGB_typedef*)Row;

  for(i = 0; i < cinfo.image_width; i++)
  {
    RGB16Buffer[i] = (uint16_t)
    (
     ((RGB_matrix[i].R & 0x00F8) >> 3)|
     ((RGB_matrix[i].G & 0x00FC) << 3)|
     ((RGB_matrix[i].B & 0x00F8) << 8)
    );
  }
  BSP_LCD_DrawRGB16Image(0, line_counter, cinfo.image_width, 1, RGB16Buffer);
  line_counter++;
  return 0;
}

// ----------------------------------------------------------------------------
uint32_t jpeg_decode(JFILE *file, uint32_t width, uint8_t * buff, uint8_t (*callback)(uint8_t*, uint32_t))
{
  /* Decode JPEG Image */
  uint32_t ret = 0;
  JSAMPROW buffer[2] = {0}; /* Output row buffer */
  uint32_t row_stride = 0; /* physical row width in image buffer */

  buffer[0] = buff;

  /* Step 1: allocate and initialize JPEG decompression object */
  cinfo.err = jpeg_std_error(&jerr);

  /* Initialize the JPEG decompression object */
  jpeg_create_decompress(&cinfo);

  jpeg_stdio_src (&cinfo, file);

  /* Step 3: read image parameters with jpeg_read_header() */
  jpeg_read_header(&cinfo, TRUE);

  /* Step 4: set parameters for decompression */
  cinfo.dct_method = JDCT_FLOAT;

  if((cinfo.image_width <= BSP_LCD_GetXSize()) && (cinfo.image_height <= BSP_LCD_GetYSize()))
  {
    BSP_LCD_Clear(LCD_COLOR_BLACK);
    RGB16Buffer = JMALLOC(width * 2);
    /* Step 5: start decompressor */
    jpeg_start_decompress(&cinfo);

    row_stride = width * 3;
    while (cinfo.output_scanline < cinfo.output_height)
    {
      (void) jpeg_read_scanlines(&cinfo, buffer, 1);

      if (callback(buffer[0], row_stride) != 0)
      {
        break;
      }
    }
    /* Step 6: Finish decompression */
    jpeg_finish_decompress(&cinfo);

    JFREE(RGB16Buffer);
    ret = 1;
  }

  /* Step 7: Release JPEG decompression object */
  jpeg_destroy_decompress(&cinfo);
  return ret;
}

// ----------------------------------------------------------------------------
void jpg_view(char* pth, char* fn)
{
  FIL sfile;
  static uint8_t _aucLine[2048];

  char f[256];
  sprintf(f, "%s/%s", pth, fn);
  if(f_open(&sfile, f, FA_READ) == FR_OK)
  {
    line_counter = 0;
    if(jpeg_decode(&sfile, BSP_LCD_GetXSize(), _aucLine, Jpeg_CallbackFunction))
    {
      printf("Jpg view: '%s'\n", f);
      Delay(5000);
    }
    f_close(&sfile);
  }
}

// ----------------------------------------------------------------------------
// végigkeresi a 'path'-ban megadott helytõl kezdve az 'ext'-ben megadott kiterjesztésü fájlokat
// és meghívja az 'fp'-ben megadott függvényt
FRESULT scan_files(char* path, char* ext, void (*fp)(char* pth, char* fn))
{
  FRESULT res;
  DIR dir;
  UINT i;
  static FILINFO fno;

  UINT el = strlen(ext);                // kiterjesztés hossza
  res = f_opendir(&dir, path);          // kutatás kezdete
  if (res == FR_OK)
  {
    for (;;)
    {
      res = f_readdir(&dir, &fno);
      if (res != FR_OK || fno.fname[0] == 0) break;  // vége?
      if (fno.fattrib & AM_DIR)         // DIR ?
      {
        i = strlen(path);
        sprintf(&path[i], "/%s", fno.fname);
        res = scan_files(path, ext, fp);// alkönyvtár tartalmának felkutatása
        /* if (res != FR_OK) break; */
        path[i] = 0;                    // kilépünk az alkönyvtárbol
      }
      else
      {                                 // file ?
        if(!strcmp(ext, (char *)fno.fname + strlen(fno.fname) - el))
          fp(path, fno.fname);
      }
    }
    f_closedir(&dir);
  }
  return res;
}

//-----------------------------------------------------------------------------
void mainApp(void)
{
  char buff[256];

  BSP_LCD_Init();
  Delay(500);
  printf("Start\n");
  printf("Flash Path:%s\n", FLASHPATH);
  while(1)
  {
    FLASHPROCESS();
    while(FLASHREADY)
    {
      if(f_mount(&FLASHFATS, FLASHPATH, 1) == FR_OK)
      {
        printf("FatFs ok\n");

        while(FLASHREADY)
        {
          FLASHPROCESS();
          strcpy(buff, FLASHPATH);
          #ifdef STARTFOLDER
          strcat(buff, STARTFOLDER);
          #endif
          if(buff[strlen(buff) - 1] == '/')
            buff[strlen(buff) - 1] = 0; // utolso / jelet lecsapjuk, mert egyébként duplázva lesz
          scan_files(buff, JPG_FILENAMEEXT, jpg_view); // jpg view
        }
      }
      else
        printf("FatFs error !!!\n");
    }
  }
}
