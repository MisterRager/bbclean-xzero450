/*
 ============================================================================

  This file is part of the bbStyleMaker source code
  Copyright 2003-2009 grischka@users.sourceforge.net

  http://bb4win.sourceforge.net/bblean
  http://developer.berlios.de/projects/bblean

  bbStyleMaker is free software, released under the GNU General Public
  License (GPL version 2). For details see:

  http://www.fsf.org/licenses/gpl.html

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
  for more details.

 ============================================================================
*/

int BBMessageBox(int flg, const char *fmt, ...)
{
    char buffer[10000], *p;
    const char *msg = buffer, *caption = APPNAME;
    va_list arg;
    va_start(arg, fmt);
    vsprintf (buffer, fmt, arg);
    if ('#' == buffer[0] && NULL != (p = strchr(buffer+1, buffer[0])))
        caption = buffer+1, *p=0, msg = p+1;
    return MessageBox (NULL, msg, caption, flg | MB_SETFOREGROUND | MB_TOPMOST);
}

const char *string_empty_or_null(const char *s)
{
    return NULL==s ? "<null>" : 0==*s ? "<empty>" : s;
}

COLORREF get_bg_color(StyleItem *pSI)
{
    if (B_SOLID == pSI->type) // && false == pSI->interlaced)
        return pSI->Color;
    return mixcolors(pSI->Color, pSI->ColorTo, 128);
}

COLORREF get_mixed_color(StyleItem *pSI)
{
    COLORREF b = get_bg_color(pSI);
    COLORREF t = pSI->TextColor;
    if (greyvalue(b) > greyvalue(t))
        return mixcolors(t, b, 96);
    else
        return mixcolors(t, b, 144);
}

//===========================================================================

//===========================================================================

ST UINT_PTR APIENTRY OFNHookProc(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    if (WM_INITDIALOG == uiMsg) {
        // center dialog on screen
        RECT r, w;
        if (WS_CHILD & GetWindowLong(hdlg, GWL_STYLE))
            hdlg = GetParent(hdlg);
        GetWindowRect(hdlg, &r);
        GetWindowRect(GetParent(hdlg), &w);
        MoveWindow(hdlg,
            w.left + 2, w.top + 20,
            //(w.right+w.left+r.left-r.right) / 2,
            //(w.bottom+w.top+r.top-r.bottom) / 2,
            r.right - r.left,
            r.bottom - r.top,
            FALSE//TRUE
            );
    }
    return 0;
}

#ifndef OPENFILENAME_SIZE_VERSION_400
#define OPENFILENAME_SIZE_VERSION_400  CDSIZEOF_STRUCT(OPENFILENAME,lpTemplateName)
#endif // (_WIN32_WINNT >= 0x0500)

int get_save_filename(HWND hwnd, char *buffer, const char *title, const char *filter)
{
    OPENFILENAME ofCmdFile;

    char dir[MAX_PATH];
    char file[MAX_PATH];
    char *e;

    e = strrchr(buffer, '\\');
    if (NULL == e)
        e = strrchr(buffer, '/');

    if (NULL == e) {
        dir[0] = 0;
        strcpy(file, buffer);
    } else {
        memcpy(dir, buffer, e-buffer);
        dir[e-buffer] = 0;
        strcpy(file, e+1);
    }

    memset(&ofCmdFile, 0, sizeof(OPENFILENAME));
    ofCmdFile.lStructSize = sizeof(OPENFILENAME);
    ofCmdFile.lpstrTitle = title;
    ofCmdFile.lpstrFilter = filter;
    ofCmdFile.nFilterIndex = 0;
    ofCmdFile.hwndOwner = hwnd;
    ofCmdFile.lpstrFile = file;
    ofCmdFile.lpstrInitialDir = dir;
    ofCmdFile.nMaxFile = MAX_PATH;
    ofCmdFile.lpfnHook = OFNHookProc;
    //ofCmdFile.lpstrFileTitle  = NULL;
    //ofCmdFile.nMaxFileTitle = 0;
    ofCmdFile.Flags = 0
        |OFN_EXPLORER
        | OFN_HIDEREADONLY
        | OFN_PATHMUSTEXIST
        | OFN_ENABLEHOOK
        ;

    if (LOBYTE(GetVersion()) < 5) // win9x/me
        ofCmdFile.lStructSize = OPENFILENAME_SIZE_VERSION_400;

    if (FALSE == GetSaveFileName(&ofCmdFile)
        || file[0] == 0)
        return 0;

    strcpy(buffer, file);
    return 1;
}

int choose_font(HWND hwnd, NStyleItem *pSI)
{
    CHOOSEFONT cf;
    LOGFONT lf;

    memset(&cf, 0, sizeof cf);
    memset(&lf, 0, sizeof lf);
    lf.lfHeight = -pSI->FontHeight;
    lf.lfWeight = pSI->FontWeight;
    strcpy(lf.lfFaceName, pSI->Font);

    cf.lStructSize = sizeof cf;
    cf.hwndOwner = hwnd;
    //cf.hDC = NULL;
    cf.lpLogFont = &lf;
    //cf.iPointSize = 0;
    cf.Flags = CF_SCREENFONTS
        |CF_INITTOLOGFONTSTRUCT
        //|CF_NOSIZESEL
        ;
    //cf.rgbColors = 0;
    //cf.lCustData = NULL;
    //cf.lpfnHook = NULL;
    //cf.lpTemplateName = NULL;
    //cf.hInstance = NULL;
    //cf.lpszStyle = NULL;
    cf.nFontType = SCREEN_FONTTYPE|BOLD_FONTTYPE|REGULAR_FONTTYPE|ITALIC_FONTTYPE;
    //cf.___MISSING_ALIGNMENT__ = 0;
    //cf.nSizeMin = 0;
    //cf.nSizeMax = 0;
    if (!ChooseFont(&cf))
        return 0;
    pSI->FontHeight = -lf.lfHeight;
    pSI->FontWeight = lf.lfWeight;
    strcpy(pSI->Font, lf.lfFaceName);
    return 1;
}

////////////////////////////////////////////////////////////////////////////////
// the code below is borrowed from CXImage

// xImaDsp.cpp : DSP functions
/* 07/08/2001 v1.00 - ing.davide.pizzolato@libero.it
 * CxImage version 5.50 07/Jan/2003 */

////////////////////////////////////////////////////////////////////////////////
#define  HSLMAX   255   /* H,L, and S vary over 0-HSLMAX */
#define  RGBMAX   255   /* R,G, and B vary over 0-RGBMAX */
                        /* HSLMAX BEST IF DIVISIBLE BY 6 */
                        /* RGBMAX, HSLMAX must each fit in a BYTE. */
/* Hue is undefined if Saturation is 0 (grey-scale) */
/* This value determines where the Hue scrollbar is */
/* initially set for achromatic colors */
// #define UNDEFINED (HSLMAX*2/3)
#define UNDEFINED 0 //gr
////////////////////////////////////////////////////////////////////////////////
RGBREF _RGBtoHSL(RGBREF lRGBColor)
{
    BYTE R,G,B;                 /* input RGB values */
    BYTE H,L,S;                 /* output HSL values */
    BYTE cMax,cMin;             /* max and min RGB values */
    WORD Rdelta,Gdelta,Bdelta;  /* intermediate value: % of spread from max*/
    RGBREF hsl;

    R = lRGBColor.rgb.Red;   /* get R, G, and B out of DWORD */
    G = lRGBColor.rgb.Green;
    B = lRGBColor.rgb.Blue;

    cMax = imax( imax(R,G), B);   /* calculate lightness */
    cMin = imin( imin(R,G), B);
    L = (BYTE)((((cMax+cMin)*HSLMAX)+RGBMAX)/(2*RGBMAX));

    if (cMax==cMin){            /* r=g=b --> achromatic case */
        S = 0;                  /* saturation */
        H = UNDEFINED;          /* hue */
    } else {                    /* chromatic case */
        if (L <= (HSLMAX/2))    /* saturation */
            S = (BYTE)((((cMax-cMin)*HSLMAX)+((cMax+cMin)/2))/(cMax+cMin));
        else
            S = (BYTE)((((cMax-cMin)*HSLMAX)+((2*RGBMAX-cMax-cMin)/2))/(2*RGBMAX-cMax-cMin));
        /* hue */
        Rdelta = (WORD)((((cMax-R)*(HSLMAX/6)) + ((cMax-cMin)/2) ) / (cMax-cMin));
        Gdelta = (WORD)((((cMax-G)*(HSLMAX/6)) + ((cMax-cMin)/2) ) / (cMax-cMin));
        Bdelta = (WORD)((((cMax-B)*(HSLMAX/6)) + ((cMax-cMin)/2) ) / (cMax-cMin));

        if (R == cMax)
            H = (BYTE)(Bdelta - Gdelta);
        else if (G == cMax)
            H = (BYTE)((HSLMAX/3) + Rdelta - Bdelta);
        else /* B == cMax */
            H = (BYTE)(((2*HSLMAX)/3) + Gdelta - Rdelta);

//      if (H < 0) H += HSLMAX;     //always false
//      if (H > HSLMAX) H -= HSLMAX;
    }

    hsl.rgb.Red     = H;
    hsl.rgb.Green   = S;
    hsl.rgb.Blue    = L;
    hsl.rgb.Alpha   = 0;
    return hsl;
}

////////////////////////////////////////////////////////////////////////////////
static float HueToRGB(float n1, float n2, float hue)
{
    //<F. Livraghi> fixed implementation for HSL2RGB routine
    float rValue;

    if (hue > 360)
        hue = hue - 360;
    else if (hue < 0)
        hue = hue + 360;

    if (hue < 60)
        rValue = n1 + (n2-n1)*hue/60.0f;
    else if (hue < 180)
        rValue = n2;
    else if (hue < 240)
        rValue = n1+(n2-n1)*(240-hue)/60;
    else
        rValue = n1;

    return rValue;
}

////////////////////////////////////////////////////////////////////////////////
RGBREF _HSLtoRGB(RGBREF lHSLColor)
{ 
    //<F. Livraghi> fixed implementation for HSL2RGB routine
    float h,s,l;
    float m1,m2;
    BYTE r,g,b;
    RGBREF ref;

    h = (float)lHSLColor.rgb.Red * 360.0f/255.0f;
    s = (float)lHSLColor.rgb.Green/255.0f;
    l = (float)lHSLColor.rgb.Blue/255.0f;

    if (l <= 0.5)   m2 = l * (1+s);
    else            m2 = l + s - l*s;

    m1 = 2 * l - m2;

    if (s == 0) {
        r=g=b=(BYTE)(l*255.0f);
    } else {
        r = (BYTE)(HueToRGB(m1,m2,h+120) * 255.0f);
        g = (BYTE)(HueToRGB(m1,m2,h) * 255.0f);
        b = (BYTE)(HueToRGB(m1,m2,h-120) * 255.0f);
    }

    ref.rgb.Red     = r;
    ref.rgb.Green   = g;
    ref.rgb.Blue    = b;
    ref.rgb.Alpha   = 0;
    return ref;
}

////////////////////////////////////////////////////////////////////////////////
