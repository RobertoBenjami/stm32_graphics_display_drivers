/*
 * bmp.h
 *
 *  Created on: 2019. febr. 17.
 *      Author: Benjami
 */

#ifndef __BMP_H_
#define __BMP_H_

typedef struct __attribute__((packed)) tagBITMAPFILEHEADER {
  uint16_t   bfType;
  uint32_t   bfSize;
  uint16_t   bfReserved1;
  uint16_t   bfReserved2;
  uint32_t   bfOffBits;
} BITMAPFILEHEADER; // size is 14 bytes

typedef struct __attribute__((packed)) tagBITMAPINFOHEADER {
  uint32_t  biSize;
  uint32_t  biWidth;
  uint32_t  biHeight;
  uint16_t  biPlanes;
  uint16_t  biBitCount;
  uint32_t  biCompression;
  uint32_t  biSizeImage;
  uint32_t  biXPelsPerMeter;
  uint32_t  biYPelsPerMeter;
  uint32_t  biClrUsed;
  uint32_t  biClrImportant;
} BITMAPINFOHEADER; // size is 40 bytes

typedef struct __attribute__((packed)) tagBITMAPSTRUCT {
  // offset 0, size 14
  BITMAPFILEHEADER fileHeader;
  // offset 14, size 40
  BITMAPINFOHEADER infoHeader;
  // offset 54, size X * Y words
  uint16_t data[];
} BITMAPSTRUCT;

#endif /* __BMP_H_ */
