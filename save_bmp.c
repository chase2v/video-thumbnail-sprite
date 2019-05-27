#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>

#include "save_bmp.h"

int saveBMP(char *outputFilename, SpriteImage *spriteImg)
{
    int numBytes = avpicture_get_size(AV_PIX_FMT_RGB24, spriteImg->width, spriteImg->height);

    BITMAPINFOHEADER header;
    header.biSize = sizeof(BITMAPINFOHEADER);

    header.biWidth = spriteImg->width;
    header.biHeight = spriteImg->height * (-1);
    header.biBitCount = 24;
    header.biCompression = 0;
    header.biSizeImage = 0;
    header.biClrImportant = 0;
    header.biClrUsed = 0;
    header.biXPelsPerMeter = 0;
    header.biYPelsPerMeter = 0;
    header.biPlanes = 1;

    BITMAPFILEHEADER bmpFileHeader = {0,};
    //HANDLE hFile = NULL;
    DWORD dwTotalWriten = 0;
    DWORD dwWriten;

    bmpFileHeader.bfType = 0x4d42; //'BM';
    bmpFileHeader.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER)+ numBytes;
    bmpFileHeader.bfOffBits=sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER);

    FILE* pf = fopen(outputFilename, "wb");
    fwrite(&bmpFileHeader, sizeof(BITMAPFILEHEADER), 1, pf);
    fwrite(&header, sizeof(BITMAPINFOHEADER), 1, pf);
    fwrite(spriteImg->data, 1, numBytes, pf);
    fclose(pf);

    return 0;
}
