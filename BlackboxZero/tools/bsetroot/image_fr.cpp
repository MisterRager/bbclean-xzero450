//===========================================================================
// free_image.cpp

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include "FreeImage.h"

int error;
typedef void * HIMG;

HIMG image_create(FIBITMAP *Img)
{
    if (NULL == Img)
    {
        error = 2;
        FreeImage_DeInitialise();
        return NULL;
    }
    error = 0;
    return (HIMG)Img;
}

HIMG image_create_fromfile(const char *path)
{
    FIBITMAP *Img = NULL;
    FreeImage_Initialise(FALSE);
    FREE_IMAGE_FORMAT fif = FreeImage_GetFIFFromFilename(path);
    if (fif != FIF_UNKNOWN)
        Img = FreeImage_Load(fif, path, 0);
    return image_create(Img);
}

HIMG image_create_fromraw(int width, int height, void *pixels)
{
    FreeImage_Initialise(FALSE);
    FIBITMAP *Img = FreeImage_ConvertFromRawBits(
        (BYTE*)pixels, width, height, 4*width, 32, 0, 0, 0, 1);
    return image_create(Img);
}

void image_destroy(HIMG hImg)
{
    FIBITMAP *Img = (FIBITMAP*)hImg;
    if (Img)
    {
        FreeImage_Unload(Img);
        FreeImage_DeInitialise();
    }
}

int image_save(HIMG hImg, const char *path)
{
    FIBITMAP *Img = (FIBITMAP*)hImg;
    BOOL r = FreeImage_Save(FIF_BMP, Img, path, 0);
    return 0 != r;

}

const char *image_getversion(void)
{
    static char buf[40];
    sprintf(buf, "FreeImage %s", FreeImage_GetVersion());
    return buf;
}

const char *image_getlasterror(void)
{
    if (error == 2)
        return "unknown image format";
    if (error == 1)
        return "file not found";
    return "";
}

int image_getwidth(HIMG hImg)
{
    FIBITMAP *Img = (FIBITMAP*)hImg;
    return FreeImage_GetWidth(Img);
}

int image_getheight(HIMG hImg)
{
    FIBITMAP *Img = (FIBITMAP*)hImg;
    return FreeImage_GetHeight(Img);
}

int image_setpixel(HIMG hImg, int x, int y, RGBQUAD c)
{
    FIBITMAP *Img = (FIBITMAP*)hImg;
    FreeImage_SetPixelColor(Img, x, y, &c);
    return 1;
}

RGBQUAD image_getpixel(HIMG hImg, int x, int y)
{
    FIBITMAP *Img = (FIBITMAP*)hImg;
    RGBQUAD c;
    FreeImage_GetPixelColor(Img, x, y, &c);
    return c;
}

/*
    Resample filters:
    -----------------
    FILTER_BOX        = 0,  // Box, pulse, Fourier window, 1st order (constant) b-spline
    FILTER_BICUBIC    = 1,  // Mitchell & Netravali's two-param cubic filter
    FILTER_BILINEAR   = 2,  // Bilinear filter
    FILTER_BSPLINE    = 3,  // 4th order (cubic) b-spline
    FILTER_CATMULLROM = 4,  // Catmull-Rom spline, Overhauser spline
    FILTER_LANCZOS3   = 5   // Lanczos3 filter
*/

int image_resample(HIMG *hImg, int new_width, int new_height)
{
    FIBITMAP *Img = *(FIBITMAP**)hImg;
    FIBITMAP *Img2 = FreeImage_Rescale(
        Img,
        new_width,
        new_height,
        FILTER_BSPLINE);

    if (Img2)
    {
        FreeImage_Unload(Img);
        *(FIBITMAP**)hImg = Img2;
        return 1;
    }
    return 0;
}

// -----------------------------------------------
