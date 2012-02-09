/*
 ============================================================================
  This file is part of the bbLeanBar+ source code.

  bbLeanBar+ is a plugin for BlackBox for Windows
  Copyright © 2003-2009 grischka
  Copyright © 2008-2009 The Blackbox for Windows Development Team

  http://bb4win.sourceforge.net/bblean/

  bbLeanBar+ is free software, released under the GNU General Public License
  (GPL version 2). See for details:

  http://www.fsf.org/licenses/gpl.html

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
  for more details.

 ============================================================================
*/

// Auto-buffered wrapper for MakeStyleGradient

// call instead:
// void BuffBmp::MakeStyleGradient(HDC hdc, RECT *rc, StyleItem *pSI, bool bordered);

// call after everything is drawn, and once at end of program to cleanup
// void BuffBmp::ClearBitmaps(void);

class BuffBmp
{
private:
    struct Bmp
    {
        struct Bmp *next;

        /* 0.0.80 */
        int bevelstyle;
        int bevelposition;
        int type;
        bool parentRelative;
        bool interlaced;

        /* 0.0.90 */
        COLORREF Color;
        COLORREF ColorTo;

        int borderWidth;
        COLORREF borderColor;
		COLORREF ColorSplitTo;
		COLORREF ColorToSplitTo;

        RECT r;
        HBITMAP bmp;
        bool in_use;
    };

    struct Bmp *g_Buffers;

public:
    BuffBmp()
    {
        g_Buffers = NULL;
    }

    ~BuffBmp()
    {
        ClearBitmaps(true);
    }

    void MakeStyleGradient(HDC hdc, RECT *rc, StyleItem *pSI, bool withBorder)
    {
#if 0
        ::MakeStyleGradient(hdc, rc, pSI, withBorder);
#else
        COLORREF    borderColor = 0;
        int         borderWidth = 0;

        if (withBorder)
        {
            if (pSI->bordered)
            {
                borderColor = pSI->borderColor;
                borderWidth = pSI->borderWidth;
            }
            else
            {
                borderColor = styleBorderColor;
                borderWidth = styleBorderWidth;
            }
        }

        if (pSI->parentRelative)
        {
            if (borderWidth)
                CreateBorder(hdc, rc, borderColor, borderWidth);
            return;
        }

        int width   = rc->right - rc->left;
        int height  = rc->bottom - rc->top;

        struct Bmp *B;

        dolist (B, g_Buffers)
            if (B->r.right      == width
             && B->r.bottom     == height
             && B->type         == pSI->type
             && B->Color        == pSI->Color
             && B->ColorTo      == pSI->ColorTo
             && B->interlaced   == pSI->interlaced
             && B->bevelstyle   == pSI->bevelstyle
             && B->bevelposition== pSI->bevelposition
             && B->borderColor  == borderColor
             && B->borderWidth  == borderWidth
             && B->ColorSplitTo        == pSI->ColorSplitTo
             && B->ColorToSplitTo      == pSI->ColorToSplitTo
             ) break;

        HDC buf = CreateCompatibleDC(NULL);
        HGDIOBJ other;

        if (NULL == B)
        {
            B = new struct Bmp;

            B->r.left       =
            B->r.top        = 0;

            B->r.right      = width;
            B->r.bottom     = height;

            B->type         = pSI->type         ,
            B->Color        = pSI->Color        ,
            B->ColorTo      = pSI->ColorTo      ,
            B->interlaced   = pSI->interlaced   ,
            B->bevelstyle   = pSI->bevelstyle   ,
            B->bevelposition= pSI->bevelposition,
            B->borderColor  = borderColor       ,
            B->borderWidth  = borderWidth       ,
            B->ColorSplitTo        = pSI->ColorSplitTo,
            B->ColorToSplitTo      = pSI->ColorToSplitTo;

            B->bmp = CreateCompatibleBitmap(hdc, width, height);
            B->next = g_Buffers;
            g_Buffers = B;

            other = SelectObject(buf, B->bmp);

            MakeGradientEx(
                buf,
                B->r,
                B->type,
                B->Color,
                B->ColorTo,
				B->ColorSplitTo,
				B->ColorToSplitTo,
                B->interlaced,
                B->bevelstyle,
                B->bevelposition,
                0,
                B->borderColor,
                B->borderWidth
                );

            //dbg_printf("new bitmap %d %d", width, height);
        }
        else
        {
            other = SelectObject(buf, B->bmp);
        }

        B->in_use = true;

        BitBlt(hdc, rc->left, rc->top, width, height, buf, 0, 0, SRCCOPY);
        SelectObject(buf, other);
        DeleteDC(buf);
#endif
    }

    void ClearBitmaps(bool force = false)
    {
        struct Bmp *B, **pB = &g_Buffers;
        while (NULL != (B=*pB))
        {
            if (false == B->in_use || force)
            {
                *pB = B->next;
                DeleteObject(B->bmp);
                delete B;
            }
            else
            {
                B->in_use = false;
                pB = &B->next;
            }
        }
    }

};

//===========================================================================
