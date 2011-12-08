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

struct bbTooltipInfo
{
    StyleItem       *pStyle;
    StyleItem       *pStyleFont;
    int             delay;
    int             distance;
    int             max_width;
    int             activation_rect_pad;
    int             alpha;
    bool            bTransparency;
    bool            bAbove;
    bool            bSetLabel;
    bool            bCenterTip;
    bool            bDocked;
	int				Tooltips_Shadow;
};

class bbTooltip
{
public:
    bbTooltip() {};
    bbTooltip(HINSTANCE hModuleInstance, HWND hModuleWindow, bbTooltipInfo* pTipInfo) { Start(hModuleInstance, hModuleWindow, pTipInfo); };
    ~bbTooltip() { End(); };

    void Start(HINSTANCE, HWND, bbTooltipInfo*);
    void End();

    void MouseEvent(POINT, UINT);
    void Set(RECT*, char*, bool);
    void UpdateSettings();
    void ClearList();

    HWND m_TipHwnd;

private:
    enum constants {
        _MAX_LINE_LENGTH = 320,
        _MAX_WIDTH = 600,
        _MAX_HEIGHT = 500,
        _INITIAL_TIMER = 1,
        _UPDATE_TIMER
    };

    struct TipStruct {
        RECT            rect;
        TipStruct       *next;
        char            used_flg,
                        shown,
                        clicked,
                        text[256];
        bool            showTip;
    };

    static LRESULT CALLBACK _WndProc(HWND, UINT, WPARAM, LPARAM);

    inline bool         _Dispatch(UINT, WPARAM, LPARAM);
    inline void         _Timer(WPARAM);
    inline void         _Paint();
    void                _Activate(RECT*, char*);
    void                _Show();
    void                _Hide();

    bbTooltipInfo*      m_pInfo;            // pointer to info struct
    unsigned int        m_Width;            // the tooltip's width
    unsigned int        m_Height;           // the tooltip's height

    unsigned int        m_ScreenHeight;
    unsigned int        m_ScreenWidth;

    BITMAPINFOHEADER    m_bmpInfo;

    PAINTSTRUCT         m_PaintStruct;
    RECT*               m_pPaintRect;

    HINSTANCE           m_hostInstance;
    HWND                m_hHost;
    HWND                m_hCore;

    HDC                 m_hPrimaryBuf;      // place to put return value of BeginPaint
    HDC                 m_hSecondaryBuf;    // the double-buffer

    HBITMAP             m_hBitmapNull;      // null bitmap object stored in dc when it's first created
    HFONT               m_hFontNull;        // null font object stored in dc when it's first created

    char*               m_WindowName;
    char                m_Text[_MAX_LINE_LENGTH];

    RECT                m_ActivationRect;

    COLORREF            m_BorderColor;
    int                 m_BorderWidth;
    int                 m_tipBorder;

    RECT                m_timer_rect;
    POINT               m_timer_point;

    TipStruct*          m_TipList;

    bool                m_bInitial;

    RECT                m_rect;
};

//#define _CopyRect(lprcDst, lprcSrc) (*lprcDst) = (*lprcSrc)
#define _OffsetRect(lprc, dx, dy) (*lprc).left += (dx), (*lprc).right += (dx), (*lprc).top += (dy), (*lprc).bottom += (dy)
#define _CopyOffsetRect(lprcDst,lprcSrc,dx,dy) (*(lprcDst)).left = (*(lprcSrc)).left + (dx), (*(lprcDst)).right = (*(lprcSrc)).right + (dx), (*(lprcDst)).top = (*(lprcSrc)).top + (dy), (*(lprcDst)).bottom = (*(lprcSrc)).bottom + (dy)
//void DrawTextWithShadow(HDC hdc, LPSTR text, RECT r, unsigned int format, COLORREF textColor, COLORREF shadowColor, bool shadow);
//COLORREF CreateShadowColor(StyleItem* styleItem, COLORREF color, COLORREF colorTo, COLORREF textColor);
//---------------------------------------------
