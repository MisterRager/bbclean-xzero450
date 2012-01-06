//===========================================================================
// cx_image.cpp

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include "CxImage/CxImage/ximage.h"

extern "C" void dbg_printf (const char *fmt, ...);

char m_error[200];
typedef void *HIMG;

static void set_pixels(CxImage *Img, void *pixels);

HIMG image_create_fromfile(const char *path)
{
    CxImage *Img = new CxImage;
    m_error[0] = 0;
    if (Img->Load(path))
        return (HIMG)Img;
    strcpy(m_error, Img->GetLastError());
    delete Img;
    return NULL;
}

HIMG image_create_fromraw(int width, int height, void *pixels)
{
    CxImage *Img = new CxImage(width, height, 24);
    set_pixels(Img, pixels);
    return (HIMG)Img;
}

void image_destroy(HIMG Img)
{
    if (Img) delete ((CxImage*)Img);
}

const char *image_getversion(void)
{
    CxImage Img;
    return Img.GetVersion();
}

const char *image_getlasterror(void)
{
    return m_error;
}

int image_getwidth(HIMG Img)
{
    return ((CxImage*)Img)->GetWidth();
}

int image_getheight(HIMG Img)
{
    return ((CxImage*)Img)->GetHeight();
}

int image_setpixel(HIMG Img, int x, int y, RGBQUAD c)
{
    ((CxImage*)Img)->SetPixelColor(x, y, c);
    return 1;
}

RGBQUAD image_getpixel(HIMG Img, int x, int y)
{
    return ((CxImage*)Img)->GetPixelColor(x, y);
}

int image_save(HIMG Img, const char *path)
{
    return ((CxImage*)Img)->Save(path, CXIMAGE_FORMAT_BMP);
}

int image_resample(HIMG *Img, int w, int h)
{
    return ((CxImage*)*Img)->Resample(w, h, 0);
}

//======================================================

static void set_pixels(CxImage *Img, void *pixels)
{
    int width = Img->GetWidth();
    int height = Img->GetHeight();
    BYTE *d = Img->GetBits();
    BYTE *s = (BYTE*)pixels;
    if (NULL == s || NULL == d)
        return;
    for (int y = 0; y < height; ++y) {
        BYTE *p = d + y * width * 3;
        for(int x = 0; x< width;++x){
            p[0] = s[0];
            p[1] = s[1];
            p[2] = s[2];
            p+=3;
            s+=4;
        }
    }
}

//===========================================================================
