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

class bbAbout
{
public:
    bbAbout(HINSTANCE, bbAbout**);
    ~bbAbout();

    void                Refresh();

private:
    static LRESULT CALLBACK _WndProc(HWND, UINT, WPARAM, LPARAM);

    inline bool         _Dispatch(UINT, WPARAM, LPARAM);

    BITMAPINFOHEADER    m_bmpInfo;
    HINSTANCE           m_hostInstance;
    HDC                 m_hSecondaryBuf;    // the double-buffer
    HBITMAP             m_hBitmapNull;      // null bitmap object stored in dc when it's first created
    HFONT               m_hFontNull;        // null font object stored in dc when it's first created
    UINT                m_Width;
    UINT                m_Height;
    char                *m_WindowName;
    char                *m_Text;
    HWND                m_AboutHwnd;
    RECT                m_PosRect;
    bbAbout             **m_pThis;
};
