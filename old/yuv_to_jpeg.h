#ifndef YUC_TO_JPEG_H
#define YUC_TO_JPEG_H
#include <stdio.h>
#include <jpeglib.h>
#include <stdlib.h>
#include <string.h>
 
int yuv_to_jpeg(int Width,int Height,int size,unsigned char *yuv_buffer, unsigned char *jpg_buffer, int quality);
#endif
