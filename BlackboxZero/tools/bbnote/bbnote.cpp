/*---------------------------------------------------------------------------*

  This file is part of the BBNote source code

  Copyright 2003-2009 grischka@users.sourceforge.net

  BBNote is free software, released under the GNU General Public License
  (GPL version 2). For details see:

  http://www.fsf.org/licenses/gpl.html

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
  for more details.

 *---------------------------------------------------------------------------*/
// BBNOTE.C - main entry point, bb-stylized menu, config dialog

#include "bbstyle.h"
#include <commctrl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include "eddef.h"

#ifdef _MSC_VER
#pragma warning(disable: 4244) // convert int to short
#endif


/*------------------------------------------------------------------------------*/
int ewx0=300;
int ewy0=200;
int ewxl=400;
int ewyl=280;

HINSTANCE hInst;
HICON hIco;
#define IDC_HAND MAKEINTRESOURCE(32649)

int dzx, dzy, dzz;

HFONT fnt1, fnt2;

HWND ewnd;
HWND mwnd;
HWND cwnd;

int dlg_title_h;
int title_h;

/*----------------------------------------------------------------------------*/
struct strl *editfiles;

extern char seabuf[80];
extern char rplbuf[80];
extern char seamodeflg;
extern char currentdir[128];

char defstyle[MAX_PATH];
char ininame[MAX_PATH];

extern int My_FontWeight_n;
extern int My_FontSize_n;
extern char  My_Font[];

extern char smart, tuse, ltup, syncolors, unix_eol;
extern int tabs;

void makefonts(void);
void deletefonts(void);

char open_new_win;

RECT DT;
LRESULT CALLBACK EditProc (HWND, UINT, WPARAM, LPARAM) ;

/*----------------------------------------------------------------------------*/
char inisec[] = "bbnote";
char cfg_f;

struct ini {
    const char *key;
    char f;
    void *val;
} ini[] = {
    { "textfont",       16, My_Font },
    { "textfontsize",   2, &My_FontSize_n },
    { "textfontweight", 2, &My_FontWeight_n },

    { "open_new_window",1, &open_new_win },
    { "smarttabs",      1, &smart },
    { "writetabs",      1, &tuse },
    { "tabsize",        1, &tabs },
    { "wrapcursor",     1, &ltup },
    { "colorized",      1, &syncolors },
    { "unix_eol",       1, &unix_eol },
    { "mousewheel",     4, &mousewheelfac },

    { "wx0", 2, &ewx0 },
    { "wy0", 2, &ewy0 },
    { "wxl", 2, &ewxl },
    { "wyl", 2, &ewyl },
    { "searchfor",      16, seabuf },
    { "replaceby",      16, rplbuf },
    { "searchmode",     1, &seamodeflg },
    { "lastdirectory",  16, currentdir },

    { NULL}
};

void rw_colors(int m) {
    char buf[128]; int n,s,i;
    extern DWORD My_Colors_d[];
    extern DWORD My_Colors_l[];
    static COLORREF* crp[] = { My_Colors_l, My_Colors_d };
    static const char *sections[] = { "light", "dark"};
    static const char *color_names[] = {
    "comment"     ,
    "keyword"     ,
    "operator"    ,
    "string"      ,
    "number"      ,
    "praeproc"    ,
    NULL };
    s=0; do { n=4; do {
    const char *section = sections[s];
    const char *color_name = color_names[n-4];
    COLORREF* cp  = &crp[s][n];
    if (m)
    {
        sprintf(buf, "%06x", (unsigned)switch_rgb(*cp));
        WritePrivateProfileString(section, color_name, buf, ininame);
    }
    else
    {
        GetPrivateProfileString(section, color_name, "", buf, 256, ininame);
        if (buf[0])
            sscanf(buf,"%x",&i), *cp = switch_rgb(i);
    }} while (++n<10); } while (++s<2);
}

void readcfg(void) {
    char buf[128]; int i = 0;
    struct ini *ip = ini;
    //return;
    do {
        GetPrivateProfileString(inisec, ip->key, "", buf, 256, ininame);
        if (buf[0]) {
        if (ip->f&7) sscanf(buf,"%d",&i);
        if (ip->f&4) *(int*)ip->val=i;
        if (ip->f&2) *(short*)ip->val=i;
        if (ip->f&1) *(char*)ip->val=i;
        if (ip->f&16) strcpy((char*)ip->val, buf);
        }
    }
    while ((++ip)->key);
    rw_colors(0);
    cfg_f=0;
}

void savecfg(void) {
    char buf[128]; int i = 0;
    struct ini *ip = ini + 0;
    if (cfg_f==0) return;
    do {
        if (ip->f&4)  i=*(int*)ip->val;
        if (ip->f&2)  i=*(short*)ip->val;
        if (ip->f&1)  i=*(unsigned char*)ip->val;
        if (ip->f&7) sprintf(buf, "%d", i);
        if (ip->f&16) strcpy(buf,(char*)ip->val);
        WritePrivateProfileString(inisec, ip->key, buf, ininame);
    }
    while ((++ip)->key);
    rw_colors(1);
    cfg_f=0;
}

/*----------------------------------------------------------------------------*/
char * set_my_path (char *out, const char *in)
{
    int v = GetModuleFileName(NULL, out, MAX_PATH);
    for (;v && out[v-1]!='\\';v--);
    strcpy(out + v, in);
    return out;
}

/*----------------------------------------------------------------------------*/
void unquote_first (char *p, const char **qq)
{
    const char *q = *qq; char d = 0;
    while (*q && (unsigned char)*q <= ' ')
        ++q;
    while (*q && d < 2) {
        if (*q==':')
            d++; q++;
    }
    q = *qq;
    d = (q[0]=='\"') ? *q++ : d == 2 ? ' ' : 0;
    for (;*q && *q!=d; *p++=*q++);
    *p=0; *qq=q;
}


void get_files(const char *s)
{
    char buff[256];
    freelist(&editfiles);
    for (;;)
    {
        while (*s<=' ') {
            if (0 == *s)
                return;
            s++;
        }
        unquote_first(buff, &s);
        if (buff[0])
            appendstr(&editfiles, buff);
    }
}

/*----------------------------------------------------------------------------*/

void makedlgfont(void)
{
    fnt1 = MenuInfo.hFrameFont;
    fnt2 = MenuInfo.hTitleFont;
    title_h = dlg_title_h = MenuInfo.nTitleHeight;

    TEXTMETRIC TXM;

    HDC hdc = CreateCompatibleDC(NULL);
    HGDIOBJ other = SelectObject(hdc, fnt1);
    GetTextMetrics(hdc, &TXM);
    SelectObject(hdc, other);
    DeleteDC(hdc);

    dzx = TXM.tmAveCharWidth;
    dzy = TXM.tmHeight;
    dzz = TXM.tmAscent;
}

WPARAM do_message_loop(void)
{
    MSG msg;
    while (GetMessage (&msg, NULL, 0, 0)) {
        TranslateMessage (&msg);
        DispatchMessage (&msg);
    }
    return msg.wParam ;
}

/*----------------------------------------------------------------------------*/

int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
    static TCHAR szAppName[] = TEXT("BBNoteWindow") ;
    WNDCLASS wndclass ;
    HWND hwnd;
    long w;

    //MessageBox(NULL, szCmdLine, "bbnote", MB_TOPMOST); return 0;

    SystemParametersInfo(SPI_GETWORKAREA,0,&DT,0);
    ewx0 = DT.left + (DT.right-DT.left-ewxl)/2;
    ewy0 = DT.top  + (DT.bottom-DT.top-ewyl)/3;

    set_my_path(ininame, "bbnote.ini");
    set_my_path(defstyle, "bbnote.rc");
    bb_rcreader_init();
    readcfg();

    if (!strncmp(szCmdLine,"-w",2))
    {
        sscanf(szCmdLine+2,"%d,%d,%d,%d", &ewx0, &ewy0, &ewxl, &ewyl);
        while (*szCmdLine>' ')  ++szCmdLine;
        while (*szCmdLine==' ') ++szCmdLine;
    }

    get_files(szCmdLine);
    cfg_f=1;

    if (NULL!=(hwnd = FindWindow(szAppName, NULL)))
    {
        // MessageBox(NULL, szCmdLine, "BBNOTE", MB_TOPMOST);
        if (0==open_new_win)
        {
            if (IsIconic(hwnd))
                ShowWindow(hwnd, SW_RESTORE);

            SetForegroundWindow(hwnd);
            while (editfiles)
            {
                COPYDATASTRUCT cds;
                cds.dwData = 0x4F4E4242;
                cds.cbData = strlen(editfiles->str)+1;
                cds.lpData = editfiles->str;
                SendMessage (hwnd, WM_COPYDATA, 0, (LPARAM)&cds);
                editfiles = editfiles->next;
            }
            return 0;
        }

        ewx0 += 16; ewy0 += 16;
        if (ewx0+ewxl>DT.right || ewy0+ewyl>DT.bottom)
            ewx0 = DT.left + 16, ewy0=DT.top+16;

        savecfg();
    }

    ZeroMemory(&wndclass,sizeof(WNDCLASS));
    wndclass.style         = CS_DBLCLKS;//|CS_HREDRAW;
    //wndclass.lpfnWndProc   = WndProc ;
    wndclass.lpfnWndProc   = EditProc ;
    //wndclass.cbClsExtra    = 0 ;
    //wndclass.cbWndExtra    = 0 ;
    wndclass.hInstance     = hInst = hInstance ;
    wndclass.hIcon         = hIco = LoadIcon(hInstance,MAKEINTRESOURCE(100)) ;
    wndclass.hCursor       = LoadCursor(NULL, IDC_ARROW);
    //wndclass.hbrBackground = GetStockObject(LTGRAY_BRUSH);
    //wndclass.lpszMenuName  = MAKEINTRESOURCE(MNU_MAIN);
    wndclass.lpszClassName = szAppName ;

    if (!RegisterClass (&wndclass)) return 0;

    makedlgfont();

    //w=GetDialogBaseUnits();
    //sprintf(seabuf, "%d:%d  %d:%d", dzx, dzy, LOWORD(w), HIWORD(w));

    CreateWindowEx(
        0
        //|WS_EX_APPWINDOW
        //|WS_EX_TOOLWINDOW
        ,
        szAppName,                  // window class name
        "BBNote",                   // window caption
        WS_POPUP
        |WS_MINIMIZEBOX
        |WS_MAXIMIZEBOX
        |WS_SYSMENU
        |WS_VISIBLE
        ,
        ewx0,                        // initial x position
        ewy0,                        // initial y position
        ewxl,                        // initial x size
        ewyl,                        // initial y size
        NULL,                       // parent window handle
        NULL,                       // window menu handle
        hInstance,                  // program instance handle
        NULL                        // creation parameters
        );

    w = do_message_loop();

    savecfg();
    return w ;

}

/*----------------------------------------------------------------------------*/
#define BN_BTN 1
#define BN_CHK 2
#define BN_EDT 3
#define BN_STR 4
#define BN_ITM 5
#define BN_SLD 6
#define BN_RECT 7
#define BN_UPDN 8

#define BN_DIS 1
#define BN_ON  2
#define BN_ACT 4
#define BN_LIN 8

#define BN_LEFT 16
#define BN_RIGH 32
#define BN_RAD  64
#define BN_GRP 128
#define BN_PRESSED 256

#define D_DLG  1
#define D_BOX  2
#define D_MENU 3

struct button {
    const char *str;
    DWORD data;
    int x, y, xl, yl;
    int f, typ, msg;
};

struct dlg {
    struct button *btp;
    struct button *bpa;
    int  tf, tx, ty;
    int  wx, wy, typ;
    int dlg_title_h, tab;
    HWND   hwnd;
    char flg;
    char title[40];
};

/*----------------------------------------------------------------------------*/
void get_item_rect (struct button *bp, RECT *r)
{
    r->top    = bp->y + dlg_title_h;
    r->left   = bp->x;
    r->bottom = r->top  + bp->yl;
    r->right  = r->left + bp->xl;
}

void get_slider_rect(struct button *bp, RECT *r)
{
    int dx = bp->xl;
    int k  = bp->yl - dx;
    r->top      = dlg_title_h + bp->y + k - bp->data * k / SB_DIV;
    r->bottom   = r->top + dx;
    r->left     = bp->x;
    r->right    = r->left + bp->xl;
}

void set_slider(int my, struct button *bp)
{
    int dx = bp->xl;
    int k  = bp->yl - dx;
    bp->data =
    iminmax((dlg_title_h + bp->y + k - my) * SB_DIV / k, 0, SB_DIV-1);
}


int inside_item(int mx, int my, struct button *bp)
{
    if (bp->f&BN_DIS)
        return 0;

    if (bp->typ==BN_BTN
    || bp->typ==BN_CHK
    || bp->typ==BN_ITM
    || bp->typ==BN_UPDN
    )
        return
            mx >= bp->x &&
            mx < bp->x+bp->xl &&
            my-dlg_title_h >= bp->y &&
            my-dlg_title_h < bp->y+bp->yl
            ;

    if (bp->typ==BN_SLD)
    {
        RECT rw;
        get_slider_rect(bp, &rw);
        return
            mx>=rw.left && mx<rw.right && my>=rw.top && my<rw.bottom;
    }
    return 0;
}

/*----------------------------------------------------------------------------*/
/*
struct button mainfrm[] = {
    { "",   0,  0,  -15, 15, 15, 0, BN_SCR, 1},
    {NULL}
};
*/

int vscr;
int but1;

#define VSCR_SIZE MenuInfo.nScrollerSize
#define VSCR_TOP (MenuInfo.nTitleHeight + MenuInfo.nFrameMargin - imin(mStyle.MenuFrame.borderWidth,mStyle.MenuTitle.borderWidth) + MenuInfo.nScrollerTopOffset)
#define VSCR_SIDE MenuInfo.nScrollerSideOffset

void get_vscr_rect(RECT* rw)
{
    int x = VSCR_SIZE;
    int top = VSCR_TOP;
    int side = VSCR_SIDE;

    rw->bottom -= side;
    rw->right -= side;
    int range = rw->bottom - top - x;
    rw->top    = vscr * range / SB_DIV + top;
    rw->bottom = rw->top + x;
    rw->left = rw->right - x;
}


#define MINX 150
#define MINY  75

/*----------------------------------------------------------------------------*/
void setc (char mof)
{
    char* C;
    switch (mof) {
        case 1: case 2:  C = IDC_SIZEWE; break;
        case 4: case 8:  C = IDC_SIZENS; break;
        case 5: case 10: C = IDC_SIZENWSE; break;
        case 6: case 9:  C = IDC_SIZENESW; break;
        //case 17:         C = IDC_SIZEALL; break;
        default: return;
    }
    SetCursor(LoadCursor(NULL, C));
}

void checkc(HWND hwnd, DWORD wParam, DWORD lParam, unsigned *pmof)
{
    int mx, my, f; RECT rw; unsigned mof;
    int ismarked (void);

    GetClientRect(hwnd,&rw);
    mx=(short)LOWORD(lParam);
    my=(short)HIWORD(lParam);

    f = 4;
m0:
    mof = 0;
    if (mx>=0 && mx<=f) mof|=1;
    if (mx>=rw.right-f && mx<=rw.right) mof|=2;
    if (my>=0 && my<=f && (mx<=16||mx>=rw.right-16)) mof|=4;
    if (my>=rw.bottom-f  && my<=rw.bottom) mof|=8;
    if (mof && f<16) { f=16; goto m0; }

    get_vscr_rect(&rw);
    if (mx>=rw.left && mx<=rw.right && my>=rw.top && my<=rw.bottom)
        mof=32;

    if (mof==0)
    if (my>=0 && my<title_h)
        mof=16;

    *pmof = mof;
}

int domove (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam){

    int wx, wy, dx, dy, mx2, my2, k; RECT rw;
    static int mx1, my1;
    static unsigned mof = 0;
    unsigned mog;

    switch (message) {

    case WM_RBUTTONUP:
        checkc(hwnd, wParam, lParam, &mog);
        if (mog==16)
            PostMessage(hwnd, WM_COMMAND, CMD_EXIT, 0);
        else
        if (GetAsyncKeyState(VK_CONTROL)<0)
    case WM_MBUTTONUP:
            PostMessage(hwnd, WM_COMMAND, CMD_MENU_2, 1);
        else
            PostMessage(hwnd, WM_COMMAND, CMD_MENU_1, 1);

        return 1;

    case WM_LBUTTONDBLCLK:
        return 0;

    case WM_LBUTTONUP:
        if (mof==0) return 0;
    p2:
        ReleaseCapture();
        mof  = 0;
        return 1;

    case WM_LBUTTONDOWN:
        checkc (hwnd, wParam, lParam, &mof);
        if (mof==0) return 0;
        setc(mof+(mof==16));
        mx1=(short)LOWORD(lParam);
        my1=(short)HIWORD(lParam);
        SetCapture(hwnd);
        return 1;

    p1:
        if (wParam & MK_LBUTTON) return 0;
        checkc(hwnd, wParam, lParam, &mog);
        if (mog==0) return 0;
        setc(mog);
        return 1;

    case WM_MOUSEMOVE:
        if (0==mof) goto p1;
        if (0==(wParam & MK_LBUTTON))
            goto p2;

        GetWindowRect(hwnd,&rw);
        mx2=(short)LOWORD(lParam);
        my2=(short)HIWORD(lParam);
        dx = mx2-mx1;
        dy = my2-my1;
        wx = rw.right  - rw.left;
        wy = rw.bottom - rw.top;
        if (mof & 1)
        {
            dx = imin(wx-MINX,dx); rw.left+=dx; wx-=dx;
        }
        if (mof & 2)
        {
            dx = imax(MINX-wx,dx); wx+=dx; mx1+=dx;
        }
        if (mof & 4)
        {
            dy = imin(wy-MINY,dy); rw.top+=dy; wy-=dy;
        }
        if (mof & 8)
        {
            dy = imax(MINY-wy,dy); wy+=dy; my1+=dy;
        }
        if (mof & 16)
        {
            rw.left+=dx; rw.top+=dy;
        }
        if (mof & 32)
        {
            my2  = iminmax(my2-title_h-VSCR_SIZE/2, 0, k=wy-title_h-VSCR_SIZE);
            vscr = my2 * SB_DIV / k;
            PostMessage(hwnd, WM_VSCROLL, MAKELPARAM(SB_THUMBTRACK, vscr), 0);
            return 1;
        }
        SetWindowPos(hwnd, NULL, rw.left, rw.top, wx, wy,  SWP_NOZORDER);
        return 1;

     }
     return 0;
}

/*----------------------------------------------------------------------------*/
WNDPROC prvlineproc;

LRESULT CALLBACK LineProc (HWND hText, UINT msg, WPARAM wParam, LPARAM lParam)
{

    switch (msg) {
    case WM_CHAR:
        switch (LOWORD(wParam)) {
        case 13:
        case 27:
        case 9:
            goto p2;
        case 6:
            goto p1;

        default:
            /*
            char bstr[128];
            sprintf(bstr,"char %08x", wParam);
            MessageBox(NULL, bstr, "",MB_OK);
            */
            break;
        }
        break;

    case WM_SYSKEYDOWN:
        PostMessage(GetParent(hText), msg, wParam, lParam);
        break;

    case WM_KEYDOWN:
        switch (LOWORD(wParam)) {
        case VK_UP:
        case VK_DOWN:
        case VK_F3:
        case VK_TAB:
        case VK_ESCAPE:
        case VK_RETURN:
    p1:
            PostMessage(GetParent(hText), msg, wParam, lParam);
            //SetFocus(GetParent(hText));
    p2:
            return 0;
        }
        break;

    // -------------------------------------

    case WM_PAINT:
    {
        PAINTSTRUCT ps; RECT r; HGDIOBJ oldbuf;
        HDC hdc = BeginPaint(hText, &ps);
        HDC buf = CreateCompatibleDC(hdc);
        GetClientRect(hText, &r);
        oldbuf = SelectObject(buf, CreateCompatibleBitmap(hdc, r.right, r.bottom));

        {
            StyleItem SI = mStyle.MenuFrame;
            SI.bevelstyle = BEVEL_SUNKEN;
            SI.bevelposition = BEVEL1;
            MakeGradient_s(buf, r, &SI, 0);
        }

        CallWindowProc(prvlineproc, hText, msg, (WPARAM)buf, lParam);

        BitBltRect(hdc, buf, &ps.rcPaint);
        DeleteObject(SelectObject(buf, oldbuf));
        DeleteDC(buf);
        EndPaint(hText, &ps);
        return 0;
    }

    case WM_ERASEBKGND:
        return TRUE;

    default:
        break;
    }
    return CallWindowProc (prvlineproc, hText, msg, wParam, lParam);
}


/*----------------------------------------------------------------------------*/
void ed_setfont(HWND edw)
{
    SendMessage (edw, WM_SETFONT, (WPARAM)fnt1, MAKELPARAM (TRUE, 0));
    {
        TEXTMETRIC TXM;
        int realfontheight;
        RECT r;
        int extraspace;

        HDC hdc = CreateCompatibleDC(NULL);
        SelectObject(hdc, fnt1);
        GetTextMetrics(hdc, &TXM);
        DeleteDC(hdc);
        realfontheight = TXM.tmHeight; //pixel

        GetClientRect(edw, &r);

        extraspace = (r.bottom - realfontheight) / 2;
        if (extraspace<0) extraspace=0;

        r.left      = extraspace + 0;
        r.right     -= extraspace + 0;

        r.top       = extraspace + 0;
        r.bottom    -= 0;

        SendMessage(edw, EM_SETRECT, 0, (LPARAM)&r);
    }

    SendMessage (edw, EM_SETSEL, 0, -1);
}

/*----------------------------------------------------------------------------*/
HWND editline (RECT *r, HWND hwnd, const char *txt)
{
    HWND edw=CreateWindow(
      "EDIT",
      txt,
      WS_CHILD|WS_VISIBLE
      |ES_MULTILINE
      //|ES_WANTRETURN
      |ES_AUTOHSCROLL
      //|WS_TABSTOP
      //|WS_HSCROLL
      //|WS_VSCROLL
      //|ES_AUTOVSCROLL
      ,
      r->left,r->top,r->right,r->bottom,
      hwnd,
      (HMENU)1234,
      NULL,
      0
      );

    prvlineproc = (WNDPROC)SetWindowLongPtr (edw, GWLP_WNDPROC, (LONG_PTR)LineProc);
    ed_setfont(edw);
    return edw;
}

/*----------------------------------------------------------------------------*/
#define SEA_LINE    401
#define IDUP        402
#define IDCASE      403
#define RPL_LINE    404
#define IDWORD      405
#define IDRPL       406
#define IDREXP      407
#define IDALLF      408
#define IDGREP      409
#define SEA_INIT    410

struct button bts[] = {
    { "&up",      0,   6, 18,  20, 10,  0, BN_BTN, IDUP},
    { "&down",    0,  30, 18,  30, 10,  0, BN_BTN, IDOK},
    { "close",    0,  64, 18,  30, 10,  0, BN_BTN, IDCANCEL},
    { "&case",    0,  98, 18,  24, 10,  0, BN_CHK, 0},
    { "&word",    0, 126, 18,  24, 10,  0, BN_CHK, 0},
    { "reg&x",    0, 154, 18,  24, 10,  0, BN_CHK, 0},
    { "&files",   0, 182, 18,  24, 10,  0, BN_CHK, 0},
    { "&replace", 0, 210, 18,  40, 10,  0, BN_BTN, IDRPL},
    { seabuf,     0,   6,  5, 119, 10,  0, BN_EDT, 0},
    { rplbuf,     0, 131,  5, 119, 10,  0, BN_EDT, 0},
    {NULL}
};

/*----------------------------------------------------------------------------*/
struct dlg *fix_dlg (struct button *bp0, int wx, int wy)
{
    struct dlg *dlg;
    struct button *bp;
    int x1 = dzx * 10 + 40, y1 = dzy * 8 + 32;
    int n;
    const int x0 = 80, y0 = 80;

    dlg = (struct dlg *)c_alloc(sizeof(*dlg));

    n=0, bp = bp0;
    do n++; while ((bp++)->str);

    dlg->btp = (struct button *)c_alloc(n*sizeof(*bp));
    memmove(dlg->btp, bp0, n*sizeof(*bp));

    bp = dlg->btp;
    do {
        bp->x  = bp->x  * x1 / x0;
        bp->xl = bp->xl * x1 / x0;
        bp->y  = bp->y  * y1 / y0;
        bp->yl = bp->yl * y1 / y0;
    } while ((++bp)->str);

    dlg->dlg_title_h = dlg_title_h;

    dlg->wx = wx * x1 / x0;
    dlg->wy = wy * y1 / y0 + dlg->dlg_title_h;

    return dlg;
}

/*----------------------------------------------------------------------------*/
struct button * check_accel_msg(struct button *bp0, struct button *bp, int key)
{
    const char *p;
    while (bp)
    {
        while (bp->str)
        {
            if (0==(bp->f & BN_DIS)
                 && (bp->typ==BN_BTN || bp->typ==BN_CHK || bp->typ==BN_ITM)
                 && NULL!=(p=strchr(bp->str,'&'))
                 && (p[1]|32)==((char)key|32)
                 )
                return bp;
            bp++;
        }
        bp = bp0;
        bp0 = NULL;
    }
    return NULL;
}

int get_list_accel_msg(struct dlg *dlg, int key)
{
    struct button *bp, *bp1, *bp2;

    for (bp = dlg->btp; bp->str; bp++)
    {
        if (bp->f&BN_ACT)
        {
            bp->f&=~BN_ACT;
            break;
        }
    }

    if (bp->str)
        bp1 = check_accel_msg(dlg->btp, bp+1, key);
    else
        bp1 = check_accel_msg(NULL, dlg->btp, key);

    if (NULL == bp1) return 0;

    bp2 = check_accel_msg(dlg->btp, bp1+1, key);

    if (NULL == bp2 || bp1 == bp2) return bp1->msg;

    bp1->f |= BN_ACT;

    return 0;
}

int get_accel_msg(struct dlg *dlg, int key)
{
    struct button *bp = check_accel_msg(NULL, dlg->btp, key);
    if (bp)
    {
        if (bp->typ==BN_CHK) bp->f^=BN_ON;
        if (bp->msg) return bp->msg;
        return -1;
    }
    return 0;
}

/*----------------------------------------------------------------------------*/
struct dlg_param
{
    struct dlg *dlg;
    WNDPROC wp;
};

LRESULT CALLBACK def_dlgproc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    struct dlg_param *dp = (struct dlg_param *)((CREATESTRUCT*)lParam)->lpCreateParams;
    SetWindowLongPtr(hwnd, GWLP_WNDPROC,  (LONG_PTR)dp->wp);
    SetWindowLongPtr(hwnd, 0, (LONG_PTR)dp->dlg);
    return DefWindowProc (hwnd, msg, wParam, lParam);
}

int make_dlg_wnd (struct dlg *dlg, HWND hwnd, int x, int y, const char *title, WNDPROC DlgProc)
{
    static TCHAR classname[] = TEXT("BBNoteDlg") ;
    WNDCLASS wndclass ;
    static int cn;

    RECT r;
    struct dlg_param dp = { dlg, DlgProc };
    struct button *bp;

    strcpy(dlg->title, title);

    if (0==cn)
    {
        ZeroMemory(&wndclass,sizeof(WNDCLASS));

        wndclass.style         = CS_DBLCLKS;//|CS_HREDRAW;
        wndclass.lpfnWndProc   = def_dlgproc;
        //wndclass.cbClsExtra  = 0 ;
        wndclass.cbWndExtra    = sizeof (struct dlg *);
        wndclass.hInstance     = hInst;
        wndclass.hCursor       = LoadCursor(NULL, IDC_ARROW);
        //wndclass.hbrBackground = GetStockObject(LTGRAY_BRUSH);
        //wndclass.lpszMenuName  = MAKEINTRESOURCE(MNU_MAIN);
        wndclass.lpszClassName = classname ;
        if (!RegisterClass (&wndclass))
            return 0;
        cn++;
    }

    dlg->hwnd = CreateWindowEx(
        WS_EX_TOOLWINDOW,
        classname,                  // window class name
        NULL,
        WS_POPUP|WS_VISIBLE,
        x,
        y,
        dlg->wx,
        dlg->wy,
        hwnd,      // parent window handle
        NULL,      // window menu handle
        hInst,     // program instance handle
        (void*)&dp    // creation parameters
        );

    bp = dlg->btp;
    do if (bp->typ==BN_EDT && 0==(bp->f&BN_DIS)) {
        r.left   = bp->x;
        r.top    = bp->y + dlg->dlg_title_h;
        r.right  = bp->xl;
        r.bottom = bp->yl;
        bp->data = (DWORD_PTR)editline(&r, dlg->hwnd, bp->str);
    } while ((++bp)->str);

    return 1;
}


/*----------------------------------------------------------------------------*/
int bb_search_dialog(HWND hwnd)
{

    LRESULT CALLBACK SearchProc(HWND, UINT, WPARAM, LPARAM) ;

    RECT r; int i,k; struct button *bp;
    struct dlg *dlg;

    if (mwnd) {
        PostMessage(mwnd, WM_COMMAND, 1000, 0);
        return 0;
    }

    dlg = fix_dlg (bts, 256, 32);

    //place dialog to upper right corner
    GetWindowRect (ewnd, &r);
    r.left = imax(r.left, r.right - dlg->wx - title_h);
    r.top = imax(0, r.top - dlg->wy + title_h + 3);

    //set check-buttons
    k=16; i=3;
    bp = dlg->btp;
    do {
        bp[i].f&=~BN_ON;
        if (seamodeflg & k) bp[i].f|=BN_ON;
        k<<=1;
        } while (++i<=6);


    //start dialog
    if (0==make_dlg_wnd (dlg, hwnd, r.left, r.top, "find/replace", SearchProc))
        return 0;

    //focus to edit-control
    SetFocus((HWND)dlg->btp[8].data);
    return 1;
}


/*----------------------------------------------------------------------------*/

void MakeGradient_s (HDC hdc, RECT rw, StyleItem *si, int borderWidth)
{
    MakeGradient(hdc, rw,
        si->parentRelative ? -1 : si->type,
        si->Color,
        si->ColorTo,
        si->interlaced,
        si->bevelstyle,
        si->bevelposition,
        0,
        si->borderColor,
        si->borderWidth
        );
}

void draw_line(HDC hDC, int x1, int x2, int y, int w, COLORREF C)
{
    if (w)
    {
        HGDIOBJ oldPen = SelectObject(hDC, CreatePen(PS_SOLID, 1, C));
        while (w--)
        {
            MoveToEx(hDC, x1, y, NULL);
            LineTo  (hDC, x2, y);
            ++y;
        }
        DeleteObject(SelectObject(hDC, oldPen));
    }
}

void draw_frame(HDC hdc, RECT *prect, int title_h, RECT *ptext)
{
    RECT rw;
    StyleItem si;
    int b, f;

    rw = *prect;
    b = mStyle.MenuTitle.borderWidth;
    f = mStyle.MenuFrame.borderWidth;

    if (hdc) CreateBorder(hdc, &rw, mStyle.MenuFrame.borderColor, f);

    if (mStyle.MenuTitle.parentRelative || mStyle.menuTitleLabel)
        rw.top += f;
    else
        rw.top = title_h;

    rw.left += f;
    rw.right -= f;
    rw.bottom -= f;

    si = mStyle.MenuFrame;
    si.borderWidth = 0;
    if (hdc) MakeGradient_s(hdc, rw, &si, 0);

    rw.bottom = title_h - b;
    rw.top = f;

    if (mStyle.MenuTitle.parentRelative) {
        rw.left += 3;
        rw.right -= 3;
        if (hdc) draw_line(hdc, rw.left, rw.right, rw.bottom, b, mStyle.MenuTitle.borderColor);
    } else {
        si = mStyle.MenuTitle;
        if (mStyle.menuTitleLabel) {
            f = mStyle.MenuFrame.marginWidth;
            rw.top += f;
            rw.left += f;
            rw.right -= f;
            rw.bottom += b;
            if (hdc) MakeGradient_s(hdc, rw, &si, b);
        } else {
            si.borderWidth = 0;
            if (hdc) MakeGradient_s(hdc, rw, &si, 0);
            if (hdc) draw_line(hdc, rw.left, rw.right, rw.bottom, b, mStyle.MenuTitle.borderColor);
        }
        rw.left += 3;
        rw.right -= 3;
    }

    *ptext = rw;
}

/*----------------------------------------------------------------------------*/
void paint_box(HWND hwnd, struct dlg *dlg)
{

    PAINTSTRUCT ps ;
    HDC         hdc;
    HFONT hf0;
    RECT rw, r;
    int cwx, cwy;

    struct button *bp;
    StyleItem *si;
    char bstr[256];
    COLORREF c;
    const char *s;
    char *p;
    int x, y,f;
    //HBRUSH hb0;
    HPEN hp0;

#define MEMDC

#ifdef MEMDC
    HBITMAP     bm0,bm;
    HDC         fhdc ;
#endif

    GetClientRect(hwnd, &rw);
    cwx = rw.right;
    cwy = rw.bottom;

#ifdef MEMDC
    fhdc = BeginPaint (hwnd, &ps);
    hdc  = CreateCompatibleDC(fhdc);
    bm   = CreateCompatibleBitmap(fhdc,cwx,cwy);
    bm0  = (HBITMAP)SelectObject(hdc,bm);
#else
    hdc  = BeginPaint(hwnd,&ps);
#endif

    draw_frame(hdc, &rw, dlg->dlg_title_h, &r);

    SetBkMode    (hdc,TRANSPARENT);
    hf0 = (HFONT)SelectObject (hdc, fnt2);
    si = &mStyle.MenuTitle;
    SetTextColor (hdc, si->TextColor);
    DrawText(hdc, dlg->title, strlen(dlg->title), &r,
        //si->Justify|DT_VCENTER|DT_SINGLELINE
        DT_LEFT|DT_VCENTER|DT_SINGLELINE
        );

    SelectObject (hdc, fnt1);

    //objects
    bp = dlg->btp;
    while (bp->str) {

    if (0==(bp->f&BN_DIS)) {

        get_item_rect(bp, &rw);
        x = strlen(s = bp->str);
        c = mStyle.MenuFrame.TextColor;

        switch (bp->typ) {

        case BN_BTN:
            if ((0!=(bp->f&BN_ACT))^(0!=(bp->f&BN_PRESSED)))
                si = &mStyle.ToolbarButtonPressed;
            else
                si = &mStyle.ToolbarButton;

            MakeGradient_s(hdc, rw, si, 0);
            SetTextColor (hdc, si->TextColor);
            DrawText(hdc, s, x, &rw, DT_CENTER|DT_VCENTER|DT_SINGLELINE);
            break;

        case BN_UPDN:
            sprintf (bstr, "%d -%s+", (int)bp->data, bp->str);
            x=strlen(s = bstr);

            SetTextColor (hdc, c);
            DrawText(hdc, s, x, &rw, DT_LEFT|DT_VCENTER|DT_SINGLELINE);
            break;

        case BN_CHK:
            f=bp->f&BN_LEFT?DT_LEFT:bp->f&BN_RIGH?DT_RIGHT:DT_CENTER;

            y = dzx; if (y>5) y=5;
            if (f==DT_CENTER)
            {
                r = rw;
                rw.left  +=2;
                rw.right -=2;
                rw.bottom --;
            }
            else if (f==DT_LEFT)
            {
                r.left     = rw.left;
                r.right    = r.left + y;
                r.bottom   = rw.top + dzz;
                r.top      = r.bottom - y;
                rw.left    += y*2;
            }
            else if (f==DT_RIGHT)
            {
                r.right    = rw.right;
                r.left     = r.right - y;
                r.bottom   = rw.top + dzz;
                r.top      = r.bottom - y;
                rw.right   -= y*2;
            }
            else break;

            if (bp->f & BN_ON)
            {
                if (f!=DT_CENTER)
                {
                    hp0 = (HPEN)SelectObject(hdc, CreatePen(PS_SOLID, 1, c));
                    Arc(hdc, r.left, r.top+1, r.right, r.bottom+1, r.left, 0 ,r.left,0);
                    DeleteObject(SelectObject(hdc, hp0));
                }
                else
                    CreateBorder(hdc, &r, c, 1);
            }


            SetTextColor (hdc, c);
            DrawText(hdc, s, x, &rw, f|DT_VCENTER|DT_SINGLELINE|DT_NOCLIP);
            break;

        case BN_RECT:
            if (mStyle.borderWidth)
                c = mStyle.borderColor;
            else
                c = mStyle.MenuFrame.ColorTo;

            CreateBorder (hdc, &rw, c, 1);
            break;

        case BN_STR:
            f=bp->f&BN_LEFT?DT_LEFT:bp->f&BN_RIGH?DT_RIGHT:DT_CENTER;
            if (NULL == strchr(s, '\n'))
                f|=DT_SINGLELINE|DT_VCENTER;
            SetTextColor (hdc, c);
            DrawText(hdc, s, x, &rw, f|DT_NOCLIP);
            break;

        case BN_EDT:
        /*
            hp0 = SelectObject(hdc, CreatePen(PS_SOLID, 1, mStyle.MenuFrame.ColorTo));
            MoveToEx (hdc, rw.left-1, rw.bottom, NULL);
            LineTo   (hdc, rw.left-1, rw.top-1);
            LineTo   (hdc, rw.right,  rw.top-1);
            DeleteObject(SelectObject(hdc, hp0));
        */
            break;

        case BN_ITM:
            rw.left  += mStyle.borderWidth;
            rw.right -= mStyle.borderWidth;
            //rw.top--;
            //rw.bottom++;
            if (bp->f & BN_ACT)
            {
                if (false == mStyle.MenuHilite.parentRelative)
                    MakeGradient_s(hdc, rw, &mStyle.MenuHilite, 0);
                c = mStyle.MenuHilite.TextColor;
            }
            rw.left   += FRM*2;
            SetTextColor (hdc, c);
            for (;;) {
                p = strchr(strcpy(bstr, s), 9);
                if (p)
                    *p=0;
                DrawText(hdc, bstr, strlen(bstr), &rw, DT_LEFT|DT_SINGLELINE|DT_VCENTER);
                if (!p)
                    break;
                rw.left += dlg->tab;
                s = p + 1;
            }
            break;

        case BN_SLD:
            x = bp->xl/2;
            rw.top    += x;
            rw.bottom -= x;
            rw.left   += (bp->xl)/2 - 2;
            rw.right  = rw.left + 4;

            //CreateBorder (hdc, rw, mStyle.MenuFrame.textcolor, 1);

            si = &mStyle.MenuFrame;
            MakeGradient(hdc, rw,
                si->type,
                si->Color,
                si->ColorTo,
                false,
                BEVEL_SUNKEN,
                BEVEL1,
                0,0,0
                );

            //MakeGradient_s(hdc, rw, &mStyle.MenuFrame, 0);

            get_slider_rect (bp, &rw);
            MakeGradient_s(hdc, rw, &mStyle.MenuTitle,0);//mStyle.border_width);
            break;

        }}
        bp++;
    }

    SelectObject (hdc, hf0);

#ifdef MEMDC
    BitBltRect(fhdc, hdc, &ps.rcPaint);
    SelectObject (hdc, bm0);
    DeleteDC(hdc);
    DeleteObject (bm);
#endif
    EndPaint (hwnd, &ps);

}

/*----------------------------------------------------------------------------*/
void check_radio (struct dlg *dlg, int msg)
{
    struct button *bp,*ap;
    bp=dlg->btp;
    for (;;) {
        if (msg==bp->msg)  break;
        bp++;
        if (NULL==bp->str) return;
    }
    ap=bp;
    for (;;) {
        if (bp->f&BN_GRP) break;
        if (bp==dlg->btp) break;
        bp--;
    }
    for (;;) {
        if (bp->typ==BN_CHK && (bp->f&BN_RAD))
        {
            if (bp==ap) bp->f|=BN_ON;
            else bp->f&=~BN_ON;
        }
        bp++;
        if (NULL==bp->str)  break;
        if (bp->f&BN_GRP)   break;
    }
}


int dlg_domouse (struct dlg *dlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    struct button *bp,*bpb;
    int mx, my;
    int f;
    RECT r;
    switch (msg) {
    case WM_LBUTTONUP:
        ReleaseCapture();
        InvalidateRect(dlg->hwnd, NULL, FALSE);
        bpb = dlg->bpa;
        dlg->bpa = NULL;
        dlg->tf  = 0;
        if (bpb  && (bpb->f & BN_ACT))
        {
            bpb->f&=~BN_ACT;
            if (bpb->typ==BN_UPDN)
            {
                mx=(short)LOWORD(lParam);
                if (mx < bpb->x + bpb->xl/3)
                {
                    if (bpb->data>0)
                        bpb->data--;
                }
                else bpb->data++;
                return bpb->msg;
            }

            if (bpb->typ==BN_CHK)
            {
                if (0==(bpb->f&BN_RAD))
                {
                    bpb->f^=BN_ON;
                    return bpb->msg;
                }
                check_radio (dlg, bpb->msg);
                return bpb->msg;
            }
            if (bpb->typ==BN_BTN
              ||
                bpb->typ==BN_SLD
              ||
                bpb->typ==BN_ITM
                )
                return bpb->msg;
        }
        return 0;

    case WM_LBUTTONDBLCLK:
    case WM_LBUTTONDOWN:
        SetFocus(dlg->hwnd);
        mx=(short)LOWORD(lParam);
        my=(short)HIWORD(lParam);
        dlg->bpa = NULL;
        if ((my>0 && my<dlg->dlg_title_h) || (wParam & MK_CONTROL))
        {
            dlg->tf = 1;
            dlg->tx = mx;
            dlg->ty = my;
        }
        else
        {
            bp = dlg->btp;
            while (bp->str) {
                if (inside_item(mx, my, bp)) {
                    dlg->bpa=bp;
                    bp->f|=BN_ACT;
                    if (bp->typ==BN_SLD)
                    {
                        get_slider_rect(bp, &r);
                        dlg->ty = my - r.top;
                    }
                    break;
                }
                bp++;
            }
        }
        SetCapture(dlg->hwnd);
    iv:
        InvalidateRect(dlg->hwnd, NULL, FALSE);
        return 0;

    case WM_MOUSEMOVE:
        mx=(short)LOWORD(lParam);
        my=(short)HIWORD(lParam);
        if (dlg->tf) {
            GetWindowRect(dlg->hwnd, &r);
            SetWindowPos(dlg->hwnd, NULL, r.left + mx-dlg->tx, r.top+my-dlg->ty, 0, 0, SWP_NOSIZE|SWP_NOZORDER);
            return 0;
        }
        if (dlg->bpa) {
            bp=dlg->bpa; f=bp->f;
            if (bp->typ==BN_SLD)
            {
                set_slider(my - dlg->ty, bp);
                //set_slider(my - bp->xl/2, bp);
                get_item_rect(bp, &r);
                InvalidateRect(dlg->hwnd, &r, FALSE);
                return bp->msg;
            }
            if (inside_item(mx,my,bp))
                bp->f|=BN_ACT;
            else
                bp->f&=~BN_ACT;

            if (f!=bp->f)
                goto iv;

            return 0;
        }


        bp = dlg->btp;
        f = 0;
        while (bp->str) {
            if (bp->typ==BN_ITM
                && 0==(bp->f & BN_DIS)
                && inside_item(mx,my,bp))
            {
                if (0==(bp->f&BN_ACT)) bp->f|=BN_ACT, f++;
            }
            else
            {
                if (bp->f&BN_ACT) bp->f&=~BN_ACT, f++;
            }
            bp++;
        }

        if (mx<0 || mx>dlg->wx || my<0 || my>dlg->wy)
            ReleaseCapture();
        else
           if (dlg->typ==D_MENU && GetCapture()!=dlg->hwnd)
                SetCapture(dlg->hwnd);

        if (f) goto iv;
        return 0;
    }
    return 0;
}


/*----------------------------------------------------------------------------*/
int do_search (int msg, DWORD param);

void do_search_0 (int msg, struct dlg *dlg)
{
    int i,k;
    struct button *bp;
    SendMessage((HWND)dlg->btp[8].data, WM_GETTEXT, 80, (LPARAM)seabuf);
    SendMessage((HWND)dlg->btp[9].data, WM_GETTEXT, 80, (LPARAM)rplbuf);
    k=16; i=3; seamodeflg = 0;
    bp = dlg->btp;
    do {
        if (bp[i].f&BN_ON) seamodeflg |= k;
        k<<=1;
        } while (++i<=6);
    do_search (msg, 0);
}

/*----------------------------------------------------------------------------*/
LRESULT CALLBACK SearchProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{

    struct dlg *dlg = (struct dlg *)GetWindowLongPtr(hwnd, 0);
    int f;

    switch (msg) {
        case WM_CREATE:
            mwnd = hwnd;
            cfg_f=1;
            do_search(0, (DWORD_PTR)ewnd);
            return 0;

        case WM_DESTROY:
            mwnd = NULL;
            m_free(dlg->btp);
            m_free(dlg);
            return 0;

        // -------------------------------------
        case WM_SYSKEYDOWN:
            f = get_accel_msg(dlg, wParam);
        sea:
            if (f==-1) goto iv;
            if (f==0)  return 0;
            do_search_0(f, dlg);
            if (f==IDCANCEL)
                DestroyWindow(hwnd);
            return 0;

        iv:
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;

        case WM_RBUTTONUP:
            goto quit;

        case WM_MOUSEMOVE:
        case WM_LBUTTONDBLCLK:
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
            f = dlg_domouse (dlg, msg, wParam, lParam);
            goto sea;


        case WM_KEYDOWN:
            switch (LOWORD(wParam)) {

            case VK_TAB:
                    SetFocus(
                        GetFocus()==(HWND)dlg->btp[8].data
                        ? (HWND)dlg->btp[9].data
                        : (HWND)dlg->btp[8].data
                        );
                    break;

            case VK_F3:
            leav:
                    do_search_0(IDCANCEL, dlg);
                    SetFocus(ewnd);
                    break;

            case VK_UP:
                f=IDUP; goto sea;

            case VK_DOWN:
            case VK_RETURN:
                f=IDOK; goto sea;

            case VK_ESCAPE:
            quit:
                f=IDCANCEL; goto sea;

            }
            return 0;

    case WM_CHAR:
        switch (LOWORD(wParam))
        {
        case 6:
            goto leav;
        }
        break;

        case WM_ERASEBKGND:
            return 1;

        case WM_PAINT:
            paint_box(hwnd, dlg);
            return 0;


        // -------------------------------------
        case WM_CTLCOLOREDIT:
            SetBkMode ((HDC)wParam, TRANSPARENT);
            SetTextColor ((HDC)wParam, mStyle.MenuFrame.TextColor);
            return (LRESULT)GetStockObject(NULL_BRUSH);

        case WM_COMMAND:
            if (1234 == LOWORD(wParam))
            {
                if (EN_UPDATE == HIWORD(wParam)
                ||  EN_HSCROLL == HIWORD(wParam))
                {
                    InvalidateRect((HWND)lParam, NULL, FALSE);
                    return 0;
                }
                if (EN_KILLFOCUS == HIWORD(wParam))
                {
                    return 0;
                }
                return 0;
            }

            if (1000 == LOWORD(wParam))
            {
                HWND edw = (HWND)dlg->btp[8].data;
                do_search(0,(DWORD_PTR)ewnd);
                SendMessage (edw, WM_SETTEXT, 0, (LPARAM)seabuf);
                SendMessage (edw, EM_SETSEL, 0,-1);
                SetFocus    (edw);
                return 0;
            }
            return 0;

    }
    return DefWindowProc (hwnd, msg, wParam, lParam);
}

/*----------------------------------------------------------------------------*/
#define IDALWAYS    11
#define IDNEVER     12

struct button btm[] = {
    { "",         0,   6, 6, 40,  0, 0, BN_STR, 0  },
    { "&ok",      0,   0, 6, 36, 10, BN_DIS, BN_BTN, IDOK     },
    { "&yes",     0,  40, 6, 36, 10, BN_DIS, BN_BTN, IDYES    },
    { "&no",      0,  80, 6, 36, 10, BN_DIS, BN_BTN, IDNO     },
    { "&cancel",  0, 120, 6, 36, 10, BN_DIS, BN_BTN, IDCANCEL },
    { "&always",  0, 160, 6, 36, 10, BN_DIS, BN_BTN, IDALWAYS },
    { "ne&ver",   0, 200, 6, 36, 10, BN_DIS, BN_BTN, IDNEVER  },
    {NULL}
};

/*----------------------------------------------------------------------------*/
LRESULT CALLBACK MsgProc (HWND, UINT, WPARAM, LPARAM) ;

int bb_msgbox(HWND hwnd, const char *s, int f)
{

    RECT r;
    HDC hdc; HFONT hf; RECT r1;
    struct dlg *dlg;
    struct button *bp;
    int i,x,y,z, xs, ys;

    //how many buttons to display ?
    for (i=0, z=f&255; z; i+=(z&1), z>>=1);

    dlg = fix_dlg(btm, i*btm[0].xl-4+2*6, 24);

    //get size of msg-text
    hdc = CreateDC("DISPLAY", NULL, NULL, NULL);
    hf=(HFONT)SelectObject(hdc,fnt1);
    ZeroMemory(&r1,sizeof(r1));
    DrawText(hdc, s, strlen(s), &r1, DT_LEFT|DT_CALCRECT);
    SelectObject(hdc,hf);
    DeleteDC(hdc);

    xs = r1.right  +24;
    ys = r1.bottom +12;

    bp=dlg->btp;

    x = bp->x;
    i = bp->xl;

    if (dlg->wx < xs)
        x += (xs - dlg->wx)/2, dlg->wx = xs;

    bp->xl   = dlg->wx - 2*bp->x;
    bp->yl   = ys;
    bp->str  =  s;
    dlg->wy += ys;

    //enable and fixup buttons
    while ((++bp)->str)
    {
        if (f&1)
        {
            bp->f &= ~BN_DIS;
            bp->x  = x;
            bp->y += ys;
            x+=i;
        }
        f>>=1;
    }

    //center dialog
    GetWindowRect (ewnd, &r);
    x = r.left + imax(20, (r.right - r.left - dlg->wx)/2);
    y = r.top  + imax(20, (r.bottom - r.top - dlg->wy)/3);

    bb_sound(1);

    if (0==make_dlg_wnd(dlg, hwnd, x, y, "bbnote", MsgProc))
        return IDCANCEL;

    //modal dialog msg-loop
    EnableWindow(hwnd, FALSE);
    return do_message_loop();
}

/*----------------------------------------------------------------------------*/
LRESULT CALLBACK MsgProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    int f;
    struct dlg *dlg = (struct dlg *)GetWindowLongPtr(hwnd, 0);
    switch (msg) {

        case WM_CREATE:
            return 0;

        case WM_DESTROY:
            m_free(dlg->btp);
            m_free(dlg);
            return 0;

        case WM_SYSKEYDOWN:
            break;

        case WM_MOUSEMOVE:
        case WM_LBUTTONDBLCLK:
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
            f = dlg_domouse (dlg, msg, wParam, lParam);
        post:
            if (f<=0) return 0;
            PostQuitMessage(f);
            EnableWindow(ewnd, TRUE);
            SetFocus(ewnd);
            DestroyWindow(hwnd);
            return 0;

        case WM_KEYDOWN:
            switch (LOWORD(wParam)) {
            case VK_TAB:
                 break;

            case VK_ESCAPE:
                if (0==(dlg->btp[4].f&BN_DIS)) { f=IDCANCEL; goto post; }
                if (0==(dlg->btp[3].f&BN_DIS)) { f=IDNO; goto post; }

            case VK_RETURN:
                if (0==(dlg->btp[1].f&BN_DIS)) { f=IDOK;  goto post; }
                if (0==(dlg->btp[2].f&BN_DIS)) { f=IDYES; goto post; }
            }
            return 0;

    case WM_CHAR:
            f = get_accel_msg(dlg, LOWORD(wParam));
            goto post;

        case WM_ERASEBKGND:
            return 1;

        case WM_PAINT:
            paint_box(hwnd, dlg);
            return 0;

    }
    return DefWindowProc (hwnd, msg, wParam, lParam);
}

/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
struct button menu1[] = {
    { "&list\tctrl-l",      0, 0,0,0,0, 0,      BN_ITM, CMD_LIST  },
    { "&new\tctrl-n",       0, 0,0,0,0, 0,      BN_ITM, CMD_NEW     },
    { "&open\tctrl-o",      0, 0,0,0,0, 0,      BN_ITM, CMD_OPEN    },
    { "&close\tctrl-f4",    0, 0,0,0,0, 0,      BN_ITM, CMD_CLOSE   },

    { "&save\tctrl-s",      0, 0,0,0,0, BN_LIN, BN_ITM, CMD_SAVE    },
    { "save &as",           0, 0,0,0,0, 0,      BN_ITM, CMD_SAVEAS  },
    { "save a&ll",          0, 0,0,0,0, 0,      BN_ITM, CMD_SAVEALL },

    { "&reload",            0, 0,0,0,0, BN_LIN, BN_ITM, CMD_RELOAD  },

    { "&find\tctrl-f",      0, 0,0,0,0, BN_LIN, BN_ITM, CMD_SEARCH  },
    { "&zoom\tf10",         0, 0,0,0,0, 0,      BN_ITM, CMD_ZOOM    },

    { "&settings",          0, 0,0,0,0, BN_LIN, BN_ITM, CMD_OPTIONS },
    { "&help",              0, 0,0,0,0, 0,      BN_ITM, CMD_HELP    },

    { "a&bout",             0, 0,0,0,0, 0,      BN_ITM, CMD_ABOUT   },
    { "e&xit\tesc",         0, 0,0,0,0, BN_LIN, BN_ITM, CMD_EXIT    },
    {NULL}
};


/*----------------------------------------------------------------------------*/
int bb_menu_1(HWND hwnd, int f, struct button *menu, const char *title, int mf)
{

    LRESULT CALLBACK MenuProc (HWND, UINT, WPARAM, LPARAM) ;
    HDC hdc; HFONT hf; RECT r1; POINT pt;

    struct button *bp;
    struct dlg *dlg; char bstr[256], *p;

    int x,z,y,tb;

    // get tab-pos and max x-extent

    hdc = CreateDC("DISPLAY", NULL, NULL, NULL);
    hf = (HFONT)SelectObject(hdc, fnt1);
    x = y = z = 0;
    do {
        tb = x; x = 0; bp = menu;
        while (bp->str) {
            ZeroMemory(&r1,sizeof(r1));
            p=strchr(strcpy(bstr,bp->str),9);
            if (p) *p=0, p++;
            if (0==z) p=bstr;
            if (p)
            {
                DrawText(hdc, p, strlen(p), &r1, DT_LEFT|DT_CALCRECT);
                x = imax(x, r1.right);
                y = imax(y, r1.bottom);
            }
            bp++;
        }
    } while (++z<2);

    SelectObject(hdc,hf);
    DeleteDC(hdc);
    if (x)
        tb+=8, x+=tb;
    else
        x = tb;

    x += 4*FRM;
    z = MenuInfo.nItemHeight;
    int d = MenuInfo.nFrameMargin - mStyle.MenuFrame.borderWidth;
    bp = menu;
    y = d;
    while (bp->str)
    {
        if (bp->f&BN_LIN)
            y+= z*6/10;
        bp->x  = d;
        bp->xl = x;
        bp->y  = y;
        bp->yl = z;
        bp->f&=~BN_ACT;
        y += z;
        bp++;
        if (y>DT.bottom) {
            bp->str=NULL;
            break;
        }
    }

    dlg = (struct dlg *)c_alloc(sizeof(*dlg));
    dlg->dlg_title_h = dlg_title_h;
    dlg->btp = menu;
    dlg->wx  = x + 2*d;
    dlg->wy  = y + dlg->dlg_title_h + MenuInfo.nFrameMargin;
    dlg->tab = tb;
    dlg->typ = D_MENU;
    dlg->flg = mf;

    if (f)
    {
        GetCursorPos(&pt);
        x = imax(pt.x - x/2, 0);
        y = imax(pt.y - (2==f ? dlg->dlg_title_h*3/2 : dlg->dlg_title_h/2), 0);
    }

    else
    {
        GetWindowRect(ewnd, &r1);
        x = r1.left+1, y = r1.top+title_h;
    }

    y = imax(imin(y, DT.bottom - dlg->wy), DT.top);


    bb_sound(0);

    make_dlg_wnd (dlg, hwnd, x, y, title, MenuProc);
    return 0;
}

/*----------------------------------------------------------------------------*/
int bb_menu(HWND hwnd, int f)
{
    return bb_menu_1(hwnd, f, menu1, "file", 0);
}
/*----------------------------------------------------------------------------*/
int bcomp (const void *a, const void *b)
{
    return stricmp(((struct button *)a)->str, ((struct button *)b)->str);
}

void bb_file_menu (HWND hwnd, int f, struct strl *sp)
{
    int l,c;
    static struct strl *s0;
    struct strl *s;
    static struct button *bp0;
    struct button *bp;

    freelist(&s0);
    if (bp0) m_free(bp0);

    s = s0 = sp, l = 0;
    while (s) l++, s=s->next;

    bp0 = bp = (struct button *)c_alloc((l+1) * sizeof(struct button));

    s = sp; c = CMD_FILE;
    while (s)
    {
        bp->str = s->str;
        bp->typ = BN_ITM;
        bp->msg = c;
        s = s->next;
        bp ++; c++;
    }

    qsort(bp0, l, sizeof(struct button), bcomp);

    bb_menu_1(hwnd, f, bp0, "list", 1);
}

/*----------------------------------------------------------------------------*/
LRESULT CALLBACK MenuProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{

    int f;
    struct dlg *dlg = (struct dlg *)GetWindowLongPtr(hwnd, 0);
    switch (msg) {

        case WM_CREATE:
            return 0;

        case WM_DESTROY:
            m_free(dlg);
            return 0;

        case WM_SYSKEYUP:
            f = LOWORD(wParam);
            if (f==VK_MENU) {
                goto quit;
            }
            break;

        case WM_MOUSEMOVE:
        case WM_LBUTTONDBLCLK:
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
            f = dlg_domouse (dlg, msg, wParam, lParam);
        post:
            if (f<=0) return 0;

            SetFocus(ewnd);
            PostMessage(ewnd, WM_COMMAND, f, 0);

            if (f == IDCANCEL) goto quit;

        case WM_KILLFOCUS:
            //if (dlg->flg&1) return 0;

        case WM_RBUTTONUP:
        quit:
            DestroyWindow(hwnd);
            return 0;

        case WM_KEYDOWN:
            switch (LOWORD(wParam)) {

            case VK_ESCAPE:
                f=IDCANCEL; goto post;

            case VK_RETURN:
                f=0; goto k1;

            case VK_UP:
                f=-1; goto k1;

            case VK_DOWN:
                f=1; k1:
                {
                struct button *bp=NULL,*bp1;
                for (bp1=dlg->btp; bp1->str; bp1++)
                    if (bp1->f&BN_ACT) (bp=bp1)->f&=~BN_ACT;

                if (bp==NULL)
                {
                    if (f) goto k2;
                    return 0;
                }
                else
                {
                    if (f==0)
                    {
                        f=bp->msg; goto post;
                    }
                    if (f>0 && (++bp)->str==NULL)
                 k2:
                        bp=dlg->btp;

                    if (f<0 && (bp--)==dlg->btp)
                        for (;(++bp)[1].str;);
                }
                bp->f|=BN_ACT;
                InvalidateRect(hwnd, NULL, FALSE);
                return 0;
                }

            }
            return 0;

        case WM_CHAR:
            f = get_list_accel_msg(dlg, LOWORD(wParam));
            InvalidateRect(hwnd, NULL, FALSE);
            goto post;

        case WM_ERASEBKGND:
            return 1;

        case WM_PAINT:
            paint_box(hwnd, dlg);
            return 0;

    }
    return DefWindowProc (hwnd, msg, wParam, lParam);
}

/*----------------------------------------------------------------------------*/
struct button *getbutton (struct dlg *dlg, int msg)
{
    struct button *bp;
    for (bp=dlg->btp; bp->str; bp++)
        if (msg==bp->msg) return bp;
    return NULL;
}

void enable_button (struct dlg *dlg, int msg, int f)
{
    struct button *bp = getbutton (dlg, msg);
    if (bp) {
        if (f) bp->f &=~BN_DIS;
        else   bp->f |=BN_DIS;
    }
}

void check_button (struct dlg *dlg, int msg, int f)
{
    struct button *bp = getbutton (dlg, msg);
    if (bp) {
        if (0==f) bp->f &=~BN_ON;
        else   bp->f |=BN_ON;
    }
}

/*----------------------------------------------------------------------------*/
static char fontname[64] ="";
static int xp;
static int yp;


struct button bcfg[] = {
    { fontname,         0,  40,  6, 112,  10,   0, BN_EDT,    0},
    { "font",           0,  12,  6,  20,   8,   BN_LEFT, BN_STR,    0},

    { "size",           0,  40, 18,  48,   8,   0, BN_UPDN, 163},
    { "&bold",          0, 100, 18,  24,   9,   BN_LEFT, BN_CHK,  164},

    { "ok",             0, 170,  6,  28, 10,   0, BN_BTN, IDOK},

    { "",               0,  90, 34, 108, 66,    0, BN_RECT,    0},
    { "misc",           0,  96, 37,  40,  8,   BN_LEFT, BN_STR,    0},

    { "cursor &past eol", 0, 96 , 48,  40, 10,   BN_LEFT, BN_CHK, 121},
    { "&open new window", 0, 96 , 58,  40, 10,   BN_LEFT, BN_CHK, 122},
    { "&colorized",     0, 96,  68,  90, 10,   BN_LEFT, BN_CHK, 123},
    { "unix eol",       0, 96,  78,  90, 10,    BN_LEFT, BN_CHK, 124},

    { "mousewheel",     0, 96,  88,  90, 10,   0, BN_UPDN, 136},

    { "",               0,  6,  34,  78, 66,    0, BN_RECT,    0},
    { "tabs",           0, 12,  37,  40,  8,   BN_LEFT, BN_STR,    0},

    { "&smart",         0, 12 , 48,  36, 10,   BN_LEFT|BN_RAD|BN_GRP, BN_CHK, 131},
    { "fi&xed",         0, 12 , 58,  36, 10,   BN_LEFT|BN_RAD, BN_CHK, 132},
    { "write to &file", 0, 12 , 68,  56, 10,   BN_LEFT, BN_CHK, 133},
    { "size",           0, 12 , 78,  48, 10,   0, BN_UPDN, 134},

    {NULL}

};

/*----------------------------------------------------------------------------*/
int bb_config_dialog(HWND hwnd)
{

    LRESULT CALLBACK ConfigProc(HWND, UINT, WPARAM, LPARAM) ;

    RECT r;
    struct dlg *dlg;

    if (cwnd) {
        PostMessage(cwnd, WM_COMMAND, 1000, 0);
        return 0;
    }

    dlg = fix_dlg(bcfg, 204, 106);

    //center dialog
    GetWindowRect (ewnd, &r);
    xp = r.left + imax(20, (r.right  - r.left - dlg->wx)/2);
    yp = r.top  + imax(20, (r.bottom - r.top - dlg->wy)/3);


    if (0==make_dlg_wnd (dlg, hwnd, xp, yp, "settings", ConfigProc))
        return 0;

    EnableWindow(hwnd, FALSE);
    do_message_loop();
    return 1;
}

LRESULT CALLBACK ConfigProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{

    struct dlg *dlg = (struct dlg *)GetWindowLongPtr(hwnd, 0);
    int f;

    switch (msg) {
        case WM_CREATE:
            cwnd = hwnd;

            strcpy(fontname, My_Font);

            SendMessage((HWND)dlg->btp[0].data, WM_SETTEXT, 0, (LPARAM)fontname);

        upd:
            getbutton(dlg, 163)->data = My_FontSize_n;
            check_button(dlg, 164, My_FontWeight_n>FW_NORMAL);

            check_radio(dlg, smart ? 131 : 132);
            check_button(dlg, 133, tuse);
            getbutton(dlg,134)->data = tabs;
            enable_button(dlg, 133, 0==smart);
            enable_button(dlg, 134, 0==smart);

            check_button(dlg, 121, false == ltup);
            check_button(dlg, 122, open_new_win);
            check_button(dlg, 123, syncolors);
            check_button(dlg, 124, unix_eol);

            getbutton(dlg, 136)->data = mousewheelfac;

        iv:
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;


        case WM_DESTROY:
            cwnd = NULL;
            m_free(dlg->btp);
            m_free(dlg);
            return 0;

        case WM_WINDOWPOSCHANGED:
            xp = ((LPWINDOWPOS) lParam)->x;
            yp = ((LPWINDOWPOS) lParam)->y;
            return 0;


        case WM_SYSKEYDOWN:
            f = get_accel_msg(dlg, wParam);
            goto domsg;


        case WM_RBUTTONUP:
            goto quit;

        case WM_MOUSEMOVE:
        case WM_LBUTTONDBLCLK:
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
            f = dlg_domouse (dlg, msg, wParam, lParam);
        domsg:
            if (f==131)
            {
                smart = 1;
                tuse = 0;
                goto upd;
            }

            if (f==132)
            {
                smart = 0;
                goto upd;
            }

            if (f==134)
            {
                tabs = getbutton(dlg, f)->data;
            }
            if (f==136)
            {
                mousewheelfac = imax(1,getbutton(dlg, f)->data);
                goto upd;
            }
            if (f==133)
            {
                tuse = 0!=(getbutton(dlg, f)->f&BN_ON);
                goto upd;
            }

            if (f==121)
            {
                ltup = 0==(getbutton(dlg, f)->f&BN_ON);
                goto upd;
            }

            if (f==122)
            {
                open_new_win = 0!=(getbutton(dlg, f)->f&BN_ON);
                goto upd;
            }

            if (f==123)
            {
                syncolors = 0!=(getbutton(dlg, f)->f&BN_ON);
                goto post_upd;
            }

            if (f==124)
            {
                unix_eol = 0!=(getbutton(dlg, f)->f&BN_ON);
                goto upd;
            }

            if (f==163) goto setfont;
            if (f==164) goto setfont;
            if (f==165) goto setfont;

        post:
            if (f==-1) goto iv;
            if (f==0)  return 0;
            if (f==IDCANCEL)
            {

            }
            if (f==IDYES)
            {
            }
            if (f==IDCANCEL||f==IDOK)
            {
                EnableWindow(ewnd, TRUE);
                SetFocus(ewnd);
                DestroyWindow(hwnd);
                PostQuitMessage(0);
            }
            return 0;



   setfont:
            SendMessage((HWND)dlg->btp[0].data, WM_GETTEXT, 128, (LPARAM)fontname);
            strcpy (My_Font, fontname);
            My_FontSize_n   = getbutton(dlg, 163)->data;
            My_FontWeight_n = getbutton(dlg, 164)->f&BN_ON ? FW_BOLD:FW_NORMAL;

            deletefonts();
            makefonts();
    post_upd:
            PostMessage(ewnd, WM_COMMAND, CMD_UPD, 0);
            goto iv;


        case WM_KEYDOWN:
            switch (LOWORD(wParam)) {

            case VK_TAB:
                    break;

            case VK_RETURN:
                if (GetFocus() == (HWND)dlg->btp[0].data)
                    goto setfont;

                f=IDOK; goto post;

            case VK_ESCAPE:
            quit:
                f=IDCANCEL; goto post;

            }
            return 0;

        case WM_CHAR:
        /*
            switch (LOWORD(wParam)) {
            default:
                break;
            }
        */
            break;

            case WM_ERASEBKGND:
                return 1;

            case WM_PAINT:
                paint_box(hwnd, dlg);
                return 0;

        // -------------------------------------

        case WM_CTLCOLOREDIT:
            SetBkMode ((HDC)wParam, TRANSPARENT);
            SetTextColor ((HDC)wParam, mStyle.MenuFrame.TextColor);
            return (LRESULT)GetStockObject(NULL_BRUSH);

        case WM_COMMAND:
            if (1234 == LOWORD(wParam))
            {
                if (EN_UPDATE == HIWORD(wParam)
                ||  EN_HSCROLL == HIWORD(wParam))
                {
                    InvalidateRect((HWND)lParam, NULL, FALSE);
                    return 0;
                }
                if (EN_KILLFOCUS == HIWORD(wParam))
                {
                    return 0;
                }
                return 0;
            }

            if (1000 == LOWORD(wParam))
            {
                SetFocus (cwnd);
                return 0;
            }

            return 0;

        // -------------------------------------

    }
    return DefWindowProc (hwnd, msg, wParam, lParam);
}

/*----------------------------------------------------------------------------*/
void bb_close_dlg(void)
{
    if (mwnd)
        PostMessage(mwnd, WM_KEYDOWN, VK_ESCAPE, 0);
    if (cwnd)
        PostMessage(cwnd, WM_KEYDOWN, VK_ESCAPE, 0);
}

/*----------------------------------------------------------------------------*/
// communication with the blackbox core comes here:

#include "BBSendData.h"
UINT msgs[] = { BB_RECONFIGURE, 0 };
UINT bb_broadcast_msg;
HWND BBhwnd;

bool BBN_GetBBWnd(void)
{
    BBhwnd = FindWindow("bbNote-Proxy", NULL);
    if (NULL == BBhwnd)
        BBhwnd = FindWindow("BlackBoxClass", "BlackBox");
    return NULL != BBhwnd;
}

void bb_sound(int f)
{
    if (BBN_GetBBWnd()) PostMessage(BBhwnd, BB_SUBMENU, 0, 0);
    else
    if (f) MessageBeep(MB_OK);
}

void bb_reconfig(void)
{
    if (BBN_GetBBWnd()) PostMessage(BBhwnd, BB_SETSTYLE, 0, 0);
}

bool bb_register(HWND hwnd)
{
    BBN_GetBBWnd();
    bb_broadcast_msg = RegisterWindowMessage("TaskbarCreated");
    return 0 != BBSendData(BBhwnd, BB_REGISTERMESSAGE, (WPARAM)hwnd, msgs, sizeof msgs);
}

void bb_unregister(HWND hwnd)
{
    if (BBN_GetBBWnd())
        BBSendData(BBhwnd, BB_UNREGISTERMESSAGE, (WPARAM)hwnd, msgs, sizeof msgs);
}

struct getdata_info {
    int msg;
    char *dest;
};

BOOL bbgetdata(HWND hwnd, unsigned msg, void *dest, struct getdata_info *gdi)
{
    gdi->msg = msg;
    gdi->dest = (char*)dest;
    return SendMessage(BBhwnd, msg, (WPARAM)gdi, (LPARAM)hwnd);
}

#if 1
bool bb_getstyle(HWND hwnd)
{
    struct getdata_info gdi;
    char stylefile[MAX_PATH];

    if (false == BBN_GetBBWnd())
        return false;
    if (false == bbgetdata(hwnd, BB_GETSTYLE, stylefile, &gdi))
        return false;
    return 0 != readstyle(stylefile);
}
#else
bool bb_getstyle(HWND hwnd)
{
    struct getdata_info gdi;
    if (false == BBN_GetBBWnd())
        return false;
    if (false == bbgetdata(hwnd, BB_GETSTYLESTRUCT, &mStyle, &gdi))
        return false;
    return 0 != readstyle(NULL);
}
#endif

int handle_received_data(HWND hwnd, UINT msg, WPARAM wParam, const void *data, unsigned data_size)
{
    struct getdata_info *gdi;
    if (msg != BB_SENDDATA)
        return 0;
    gdi = (struct getdata_info*)wParam;
    switch(gdi->msg) {
    case BB_GETSTYLESTRUCT:
        memcpy(gdi->dest, data, imin(sizeof (StyleStruct), data_size));
        break;
    case BB_GETSTYLE:
        strcpy(gdi->dest, (const char*)data);
        break;
    }
    return 1;
}

int bbn_receive_data(HWND hwnd, LPARAM lParam)
{
    return BBReceiveData(hwnd, lParam, handle_received_data);
}


//===========================================================================
void GetStyle (const char *styleFile);

int readstyle(const char *fname)
{
    GetStyle(fname);
    //return 0 != (mStyle.MenuFrame.validated & VALID_TEXTURE);
    return 1;
}

/*----------------------------------------------------------------------------*/
