/*---------------------------------------------------------------------------------
 SystemBarEx (© 2004 Slade Taylor [bladestaylor@yahoo.com])
 ----------------------------------------------------------------------------------
 based on BBSystemBar 1.2 (© 2002 Chris Sutcliffe (ironhead) [ironhead@rogers.com])
 ----------------------------------------------------------------------------------
 SystemBarEx is a plugin for Blackbox for Windows.  For more information,
 please visit [http://bb4win.org] or [http://sourceforge.net/projects/bb4win].
 ----------------------------------------------------------------------------------
 SystemBarEx is free software, released under the GNU General Public License,
 version 2, as published by the Free Software Foundation.  It is distributed
 WITHOUT ANY WARRANTY--without even the implied warranty of MERCHANTABILITY
 or FITNESS FOR A PARTICULAR PURPOSE.  Please see the GNU General Public
 License for more details:  [http://www.fsf.org/licenses/gpl.html].
 --------------------------------------------------------------------------------*/

#include "sbex.h"
#include "IconItem.h"

//-----------------------------------------------------------------------------

void IconItem::SetHDC(HDC hdc)
{
    m_buf = hdc;

    ZeroMemory(&m_bv4info, sizeof(m_bv4info));
    m_bv4info.biSize = sizeof(m_bv4info);
    m_bv4info.biPlanes = 1;
    m_bv4info.biBitCount = 32;
    m_bv4info.biCompression = BI_RGB;
}

//-----------------------------------------------------------------------------

void IconItem::PaintIcon(HDC hdc, UINT left, UINT top, HICON hIcon, bool control)
{
    if (control || (!m_hue && m_sat == 255))
    {
        DrawIconEx(hdc, left, top, hIcon, m_size, m_size, 0, NULL, DI_NORMAL);
        return;
    }

    if (!m_updated)
    {
        m_bv4info.biWidth = m_size;
        m_bv4info.biHeight = 3 * m_size;

        DeleteObject((HBITMAP)SelectObject(m_buf, CreateDIBSection(
            NULL, (BITMAPINFO*)&m_bv4info, DIB_RGB_COLORS, (void**)&m_bit, NULL, 0)));

        m_inc = 4 * m_size * m_size;
        m_updated = true;
    }

    m_s = 255 - m_sat;
    m_h = 255 - m_hue;

    m_mask = m_bit;                 // pointer to mask bits
    m_end =                         // end condition for loop
        m_ico = m_bit + m_inc;      // pointer to normal icon bits
    m_bg = m_ico + m_inc;           // pointer to background buffer bits

    BitBlt(m_buf, 0, 0, m_size, m_size, hdc, left, top, SRCCOPY);    // a copy of the bg for hue
    BitBlt(m_buf, 0, m_size, m_size, m_size, m_buf, 0, 0, SRCCOPY);  // a copy of the bg to paint the normal icon onto
    DrawIconEx(m_buf, 0, m_size, hIcon, m_size, m_size, 0, NULL, DI_NORMAL);        // paint normal icon
    DrawIconEx(m_buf, 0, (2 * m_size), hIcon, m_size, m_size, 0, NULL, DI_MASK);    // paint mask

    /* what's below could be in a single loop, */
    /* but it's split it up for speed          */

    if (m_s)
    {
        if (m_hue)  // both sat and hue
        {
            do
            {
                if (!*((COLORREF*)m_mask))
                {
                    m_grey = 255 + (((m_ico[2] * 79 + m_ico[1] * 156 + m_ico[0] * 21) * m_s) >> 8);
                    m_ico[2] = (((m_ico[2] * m_sat + m_grey) >> 8) * m_h + m_bg[2] * m_hue + 255) >> 8;
                    m_ico[1] = (((m_ico[1] * m_sat + m_grey) >> 8) * m_h + m_bg[1] * m_hue + 255) >> 8;
                    m_ico[0] = (((m_ico[0] * m_sat + m_grey) >> 8) * m_h + m_bg[0] * m_hue + 255) >> 8;
                }
                m_ico += 4;
                m_bg += 4;
            }
            while ((m_mask += 4) < m_end);
        }
        else do  // only sat
        {
            if (!*((COLORREF*)m_mask))
            {
                m_grey = 255 + (((m_ico[2] * 79 + m_ico[1] * 156 + m_ico[0] * 21) * m_s) >> 8);
                m_ico[2] = (m_ico[2] * m_sat + m_grey) >> 8;
                m_ico[1] = (m_ico[1] * m_sat + m_grey) >> 8;
                m_ico[0] = (m_ico[0] * m_sat + m_grey) >> 8;
            }
            m_ico += 4;
        }
        while ((m_mask += 4) < m_end);
    }
    else do  // only hue
    {
        if (!*((COLORREF*)m_mask))
        {
            m_ico[2] = (m_ico[2] * m_h + m_bg[2] * m_hue + 255) >> 8;
            m_ico[1] = (m_ico[1] * m_h + m_bg[1] * m_hue + 255) >> 8;
            m_ico[0] = (m_ico[0] * m_h + m_bg[0] * m_hue + 255) >> 8;
        }
        m_ico += 4;
        m_bg += 4;
    }
    while ((m_mask += 4) < m_end);

    BitBlt(hdc, left, top, m_size, m_size, m_buf, 0, m_size, SRCCOPY);
}
