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

class IconItem
{
public:
    IconItem() {};
    IconItem(HDC hdc) { SetHDC(hdc); };
    ~IconItem() {};

    void SetHDC(HDC);
    void PaintIcon(HDC, UINT, UINT, HICON, bool);

    int                 m_size,
                        m_sat,
                        m_hue;
    bool                m_updated;

private:
    BITMAPINFOHEADER    m_bv4info;

    HDC                 m_buf;

    UINT                m_grey,
                        m_h,
                        m_s,
                        m_inc;

    BYTE                *m_bit,
                        *m_mask,
                        *m_bg,
                        *m_ico,
                        *m_end;
};

//-----------------------------------------------------------------------------
