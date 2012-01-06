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
// EDPROC.C - window procedure and misc utils for the editor

#include "bbstyle.h"
#include "edstruct.h"
#include "vk.h"
#include <shellapi.h>

#ifdef _MSC_VER
#define WM_MOUSEWHEEL 0x020A
#endif

//---------------------------------------------------------------------
struct winvars editw =
{
    0,0,480,300,
    0,
    0,0,0,0,
    FRM,FRM,8,13,
    1,
    NULL,NULL,
    0,0,
    0,0
    };

struct winvars *winp = &editw;
struct edvars *ed0,*edp;

char caret;
char scroll_lock;
char startup;
int My_CaretSize = 2;


#if 0
int My_FontSize_s    = 11;
int My_FontWeight_s  = FW_NORMAL;
int My_FontWeight_sb = FW_BOLD;
char  My_Font_s[64]    = "lucida console";
int fszx,fszy;
HFONT  hfnt_s,hfnt_sb;
#endif

char synhilite;
char darkcolors=0;
int mousewheelfac=10;
int mousewheelaccu;

int My_FontWeight_n = FW_NORMAL;
int My_FontWeight_b = FW_BOLD;
int My_FontSize_n   = 10;
char  My_Font[64]     = "lucida console";

DWORD My_Colors[COLORN];

HWND ownd;
extern HWND ewnd;

void clrcfg(void) {}

char prjflg;
char syncolors = 1;
char grep_cmd[256];


DWORD My_Colors_d[COLORN]= {
    0x000000, //"Background"        0
    0xffffff, //"Text"              1
    0x917300, //"selected"          2
    0xffffff, //"selected text"     3
    0xeec3ac, //"Comment"           4
    0x35ffff, //"Keyword"           5
    0xffff4a, //"Operator"          6
    0x0f7fff, //"String"            7
    0x4dcc83, //"Number"            8
    0x56c8ff, //"Praeprocessor"     9
    0x676767, //"Statusline"        10
    0xffffff, //"StatusText"        11
    0x460f00, //"log"               12
    0xc5f3cf, //"log text"          13
    0x0000ef, //"log selected"      14
    0xffffff, //"log selected text" 15
    0x00ffff  //"log error"         16
};

char My_Weights[COLORN]= { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };

DWORD My_Colors_l[COLORN]= {
    0xffffff, //"Background"        0
    0x000000, //"Text"              1
    0xffcab8, //"selected"          2
    0x000000, //"selected text"     3
    0x009f00, //"Comment"           4
    0xe50000, //"Keyword"           5
    0xff0000, //"Operator"          6
    0x0000ff, //"String"            7
    0x0000ff, //"Number"            8
    0xc000c1, //"Praeprocessor"     9
    0xbfbfbf, //"Statusline"        10
    0x000000, //"StatusText"        11
    0x674f00, //"log"               12
    0xbfffd7, //"log text"          13
    0xedecd5, //"log selected"      14
    0x000000, //"log selected text" 15
    0x9fcfff  //"log error"         16
};


/*----------------------------------------------------------------------------*/
static HFONT makefont(int h, int b, char*f) {

    HFONT hfnt, hfnt0;
    HDC hdc;
    TEXTMETRIC TXM;

    hfnt= CreateFont(h,0,0,0,b,0,0,0,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY,
        //DEFAULT_PITCH
        FIXED_PITCH
        | FF_DONTCARE,
        f
        );

    hdc = CreateCompatibleDC(NULL);
    hfnt0 = (HFONT)SelectObject(hdc,hfnt);
    GetTextMetrics(hdc,&TXM);
    SelectObject(hdc, hfnt0);
    DeleteDC(hdc);

    zx=TXM.tmAveCharWidth;
    zy=TXM.tmHeight;
    return hfnt;
}

void makefonts(void) {
//    hfnt_sb = makefont(My_FontSize_s, My_FontWeight_sb,My_Font_s);
//    hfnt_s  = makefont(My_FontSize_s, My_FontWeight_s, My_Font_s);
//    fszx = zx;
//    fszy = zy;
    editw._hfnt_b = makefont(My_FontSize_n, My_FontWeight_b, My_Font);
    editw._hfnt_n = makefont(My_FontSize_n, My_FontWeight_n, My_Font);
}

void deletefonts(void) {
    DeleteObject (editw._hfnt_n);
    DeleteObject (editw._hfnt_b);
//    DeleteObject (hfnt_s);
//    DeleteObject (hfnt_sb);
}


/*----------------------------------------------------------------------------*/
char * vscr1;
char * vscr2;

void setsize(HWND hwnd) {

    RECT rect; int f;

    if (IsIconic(hwnd)) return;

    GetClientRect (hwnd, &rect);

    cwx=rect.right  - FRM;
    cwy=rect.bottom - FRM;

    pgx=imax(1,(cwx - zx0) / zx - 1);
    pgy=imax(1,(cwy - zy0) / zy - 1);

    f = (pgx+10) * (pgy+10);

    //if (vscr1) m_free(vscr1); vscr1 = c_alloc(f);
    //if (vscr2) m_free(vscr2); vscr2 = c_alloc(f);

    f=IsZoomed(hwnd);
    if (startup==0) winzoomed=f;
    if (f) return;

    GetWindowRect (hwnd, &rect);

    ewx0=wx0=rect.left;
    ewy0=wy0=rect.top;
    ewxl=wxl=rect.right  - wx0;
    ewyl=wyl=rect.bottom - wy0;
}

/*----------------------------------------------------------------------------*/
char * format_status(char *bstr, int f)
{
    static char cb[]="[ ] "; int i;

    if (NULL==edp)
        return strcpy(bstr,"BBNote");

    i=sprintf(bstr,"%s%s", changed()?"*":"", fname(filename));

    if (f)
    {
        sprintf(bstr+i,"  %d:%d",//  %d",
            plin+(scroll_lock?1:cury+1),
            curx+1
            //,allocated
            );

        if (vmark)  strcat(bstr," B");
        if (scroll_lock)  strcat(bstr," S");
        if (clip_n!=-1)
        {
            cb[1]=clip_n+'7';
            strcat(bstr,cb);
        }
#if 0
        {
        char tmp[256];
        struct edvars *p, *n;
        for (p=NULL, n=ed0; n; n=n->next)
            if (n->next==edp) p=n;

        n = edp->next;

        if (p)
        {
            sprintf(tmp,"  <%s", fname(p->sfilename));
            strcat(bstr,tmp);
        }
        if (n)
        {
            sprintf(tmp,"  %s>", fname(n->sfilename));
            strcat(bstr,tmp);
        }
        }
#endif
    }
    return bstr;
}


/*----------------------------------------------------------------------------*/
char currentdir[256];
char projectdir[256];
extern char infomsg[];

void init_tvdispl(void) {}

int ismarked (void) {
    return (NULL!=edp && 0!=markf1);
}

#if 1
char colortrans[3][11] =  {
   { 10,  0, 1, 2, 3, 4, 5, 6, 7, 8, 9 },
   { 2,  10,11, 0, 0, 0, 0, 0, 0, 0, 0 },
   { 5,  12,13,14,15,16, 0, 0, 0, 0, 0 }
};
#endif

void settitle(void) {
    char bstr[128];
    if (edp)
    {
        set_lang(syncolors ? filename : ".0");
        upd=1;
        lmax=0;
    }
    SetWindowText(ewnd,format_status(bstr,0));
}

void setwtext(void) {

}

/*----------------------------------------------------------------------------*/
char *fname(char *p) {
    char *q = p+strlen(p);
    for (; q>p && q[-1]!='\\' && q[-1]!=':'; q--);
    return q;
}

/*----------------------------------------------------------------------------*/
char *makepath(char *d, char *p, const char *f) {
    strcpy(d,p);
    if (d[0] && f[0] && d[strlen(d)-1]!='\\') strcat(d,"\\");
    strcat(d,f);
    return d;
}

/*----------------------------------------------------------------------------*/
void set_update(HWND hwnd) {

    void checkcomment(int oo, int yn);
    int nextcmt(int);

    static int ac; RECT rect;

    if (caret==3)
        HideCaret(hwnd), caret=1;

    if (edp==NULL) {
        InvalidateRect(hwnd, NULL, FALSE);
        return;
    }

    if (upd) checkcomment(fpga,pgy+2);
    setsb(hwnd);

    if (GetFocus() == hwnd)
        SetCaretPos(zx0+(curx-clft)*zx-My_CaretSize/2, zy0+cury*zy);

    //if (chg_0!=changed()) setwtext();

    rect.top   = rect.left=0;
    rect.right = cwx+FRM;

    switch (upd) {
    case 16:                        // color update
    case 4:                         // scroll up
    case 2:                         // scroll dn
    case 1:                         // update all
        rect.bottom = cwy+FRM;
        break;

    case 8:                         // line update
        if (langflg && ac!=nextcmt(fpos))
        {
            rect.bottom = cwy + FRM;
            break;
        }

        rect.top    = zy0+zy*cury;
        rect.bottom = rect.top+zy;
        InvalidateRect(hwnd, &rect, FALSE);
        rect.top    = 0;

    case 0:
    default:
        rect.bottom=title_h;
    }

    InvalidateRect(hwnd, &rect, FALSE);
    if (langflg) ac=nextcmt(fpos);
    upd=0;
}


/*----------------------------------------------------------------------------*/
#if 0
void fillpaint(RECT *rc, int c, HDC hdc) {
    if (rc->top<zy0 || rc->left<zx0) {
        HBRUSH hbr0,hbr; RECT rect;
        hbr   = CreateSolidBrush(My_Colors[c]);
        hbr0  = SelectObject(hdc,hbr);
        rect.top=rect.left=0;
        rect.right  = cwx;
#if 1
        rect.bottom = zy0;
        FillRect(hdc,&rect,hbr);
        rect.right  = zx0;
#endif
        rect.bottom = cwy;
        FillRect(hdc,&rect,hbr);
        SelectObject (hdc, hbr0);
        DeleteObject (hbr);
        //MessageBeep(MB_OK);
    }}
#endif

/*----------------------------------------------------------------------------*/
COLORREF mixcolors(StyleItem *si)
{
    COLORREF c1, c2;
    c1 = si->Color;
    c2 = si->ColorTo;
    if (si->type == B_SOLID)
        return c1;

    return RGB(
        (GetRValue(c1)+GetRValue(c2))/2,
        (GetGValue(c1)+GetGValue(c2))/2,
        (GetBValue(c1)+GetBValue(c2))/2
        );
}

/*----------------------------------------------------------------------------*/
HDC      hdc_back;
HBITMAP  m_hBitMap;
extern HINSTANCE hInst;
extern HICON hIco;

unsigned grey_value (COLORREF c)
{
    unsigned r = GetRValue(c);
    unsigned g = GetGValue(c);
    unsigned b = GetBValue(c);
    return (r*79 + g*156 + b*21) / 256;
}

void paint_back (HDC hdc_scrn, RECT* m_rect, RECT *ptext)
{
    HDC hdc = NULL;
    if (NULL == hdc_back)
    {
        unsigned grey_b = grey_value (mixcolors(&mStyle.MenuFrame));
        unsigned grey_t = grey_value (mStyle.MenuFrame.TextColor);
        //dbg_printf("%d %d - #%06x #%06x", grey_b, grey_t, c1, c3);
        darkcolors = grey_b < grey_t;

        hdc = hdc_back  = CreateCompatibleDC(hdc_scrn);
        m_hBitMap = CreateCompatibleBitmap(hdc_scrn, m_rect->right, m_rect->bottom);
        SelectObject(hdc_back, m_hBitMap);
    }

    draw_frame(hdc, m_rect, title_h, ptext);
}

void new_back(void)
{
    if (hdc_back)
    {
        DeleteDC(hdc_back);
        DeleteObject(m_hBitMap);
        hdc_back=NULL;
    }
}

/*----------------------------------------------------------------------------*/
void paint_window(HWND hwnd) {

    PAINTSTRUCT ps ;
    HFONT       hfnt0;
    HDC         hdc;
    int i,k,n,y,x;
    //int mm,yy;
    char bstr[256];

    RECT rw, r; SIZE sz;

#define MEMDC

#ifdef MEMDC
    HBITMAP     bm0,bm;
    HDC         fhdc ;
#endif

#ifdef MEMDC
    fhdc = BeginPaint (hwnd, &ps);

    hdc  = CreateCompatibleDC(fhdc);
    bm   = CreateCompatibleBitmap(fhdc,cwx+FRM,cwy+FRM);
    bm0  = (HBITMAP)SelectObject(hdc,bm);
#else
    hdc  = BeginPaint(hwnd,&ps);
#endif

    //goto pe;

    //fillpaint(&ps.rcPaint, Cbgd, hdc);

    GetClientRect(hwnd, &rw);

    paint_back(hdc, &rw, &r);
    BitBltRect(hdc, hdc_back, &ps.rcPaint);

    hfnt0 = (HFONT)SelectObject(hdc,hfnt_n);

    if (0!=synhilite)
    {
        memmove (My_Colors, darkcolors ? My_Colors_d : My_Colors_l, sizeof(My_Colors));
    }

    My_Colors[0] = (COLORREF)-1;
    My_Colors[1] = mStyle.MenuFrame.TextColor;
    My_Colors[3] = mStyle.MenuHilite.TextColor;
    if (mStyle.MenuHilite.parentRelative)
        My_Colors[2] = grey_value (My_Colors[3]) > 128 ? 0x333333 : 0xeeeeee;
    else
        My_Colors[2] = mixcolors(&mStyle.MenuHilite);

    if (edp) {
        i=ps.rcPaint.top;
        k=ps.rcPaint.bottom;
        n=ps.rcPaint.right;

        i=(i-zy0)/zy;
        k=(k-zy0+zy-1)/zy;
        n=(n-zx0+zx-1)/zx;

        if (i<0) i=0;
        if (k<0) k=0;
        if (k>i && n>0) printpage(hdc,n,i,k);

        BitBlt(hdc, 0, rw.bottom-FRM,  rw.right,  FRM, hdc_back, 0, rw.bottom-FRM, SRCCOPY);
        BitBlt(hdc, rw.right-FRM,  0,  FRM, rw.bottom, hdc_back, rw.right-FRM,  0, SRCCOPY);
    }

    SetTextColor (hdc, mStyle.MenuTitle.TextColor );
    SetBkMode    (hdc, TRANSPARENT);
    //SelectObject (hdc, hfnt_sb);
    SelectObject (hdc, fnt2);
    format_status(bstr,1);
    DrawText(hdc, bstr, strlen(bstr), &r,
        //mStyle.MenuTitle.justify|DT_VCENTER|DT_SINGLELINE
        DT_LEFT|DT_VCENTER|DT_SINGLELINE
        );

    if (edp==NULL) goto paint_end;

    if (infomsg[0]) {

        SetBkColor   (hdc, My_Colors[2]);
        SetTextColor (hdc, My_Colors[3]);
        SetBkMode    (hdc,OPAQUE);

        SelectObject (hdc, fnt1);

        sprintf(bstr," %s ",infomsg);
        n=strlen(bstr);

        GetTextExtentPoint32 (hdc, bstr, n, &sz);
        y=cwy-sz.cy;
        x=cwx-sz.cx;
        if (x<0) x = 0;
        TextOut(hdc, x, y, bstr, n);
    }

    {
        StyleItem *pSI = &MenuInfo.Scroller;
        get_vscr_rect(&rw);
        MakeGradient_s(hdc, rw, pSI, pSI->bordered ?  pSI->borderWidth : 0);
    }

#if 0
    if (lmax==0
       && yy>=imin(tlin-plin,pgy)
       && mm>(pgx<50?pgx:pgx*6/5)
        ) lmax=mm;
#endif


paint_end:
    SelectObject (hdc, hfnt0);

#ifdef MEMDC
    BitBltRect(fhdc, hdc, &ps.rcPaint);
    //BitBlt(fhdc,0,0,cwx+FRM,cwy+FRM,hdc,0,0,SRCCOPY);
    SelectObject (hdc, bm0);
    DeleteDC(hdc);
    DeleteObject (bm);
#endif
    EndPaint (hwnd, &ps) ;

    if (edp && 0==scroll_lock && caret==1)
        ShowCaret(hwnd), caret=3;
}

/*----------------------------------------------------------------------------*/
int setsi (HWND hwnd, int f, int SB_XXX, int psiz, int pmax, int pos, int v) {
    SCROLLINFO si; int k;

    si.cbSize    = sizeof(SCROLLINFO);
    si.fMask     = SIF_POS|SIF_RANGE|SIF_PAGE;
    si.nMin      =
    si.nTrackPos =
    si.nPos      = 0;
    si.nPage     = k = SB_DIV;
    si.nMax      = SB_DIV-1;
/*
    if (f && pmax>psiz) {
      si.nPage = k = imax(SB_DIV/psiz*3/2, SB_DIV*psiz/(pmax+v));
      si.nPos  = pos * (SB_DIV-k)/(pmax-psiz+v);
    }
    SetScrollInfo(hwnd, SB_XXX, &si, TRUE);
*/
    if (SB_XXX==SB_VERT)
    {
        if (f && pmax+v>psiz && SB_XXX==SB_VERT)
        {
            vscr = pos * SB_DIV / (pmax - psiz + v);
        }
        else
            vscr=0;
        k = 0;
    }
    return k;
}

void setsb (HWND hwnd) {
    int v,x,y,f,xp,yp,xm,ym;

    if (edp==NULL) f=x=y=v=xp=yp=xm=ym=0; else {
        xm=lmax, xp=clft, x=pgx;
        ym=tlin, yp=plin, v=y=pgy;
        if (winflg&1) v=0, y++;
        f=1;
    }
    y = setsi(hwnd, f, SB_VERT, y, ym, yp, v);
    x = setsi(hwnd, f, SB_HORZ, x, xm, xp, 0);
    if (f) sbx=x, sby=y;
}

void vscroll(DWORD wP) {
    int n,k,v,d,p;
    if (edp==NULL) return;
    p=4; v=0; if (winflg&1) v=pgy+1;
    switch (LOWORD(wP)) {
    case SB_LINEUP:      n=-1; goto s1;
    case SB_LINEDOWN:    n=1;  goto s1;
    case SB_PAGEUP:      n=-p; goto s1;
    case SB_PAGEDOWN:    n=p;  goto s1;
    case SB_THUMBPOSITION:
    case SB_THUMBTRACK:
        if (0==(d=SB_DIV-sby)) return;
        k=HIWORD(wP); n=((tlin-v)*k+d-1)/d-plin;
    s1:
        n=imin(n,tlin-plin-v);
        movpage(n);
        cury=iminmax(cury-n,0,pgy);
        ed_cmd(EK_SETVAR);
    }
}

void hscroll(DWORD wP) {
    int n,k,d;
    if (edp==NULL) return;
    switch (LOWORD(wP)) {
    case SB_LINELEFT:  n=-1; goto h1;
    case SB_LINERIGHT: n=1;  goto h1;
    case SB_PAGELEFT:  n=-8; goto h1;
    case SB_PAGERIGHT: n=8;  goto h1;
    case SB_THUMBPOSITION:
    case SB_THUMBTRACK:
        if (0 == (d=SB_DIV-sbx)) return;
        k=HIWORD(wP); n=((lmax-pgx)*k+d-1)/d-clft;
    h1:
        clft=imax(0,clft+n);
        curx=iminmax(curx,clft,pgx+clft);
        upd=1;
        ed_cmd(EK_SETVAR);
    }
}

/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
extern char vmark, linmrk;

char *getfile(char *buff, const char *file, int *pline)
{
    int line_nr = 0;
    char *q;
    strcpy(buff, file);
    q = strrchr(buff, ':');
    if (q > buff+2 && q[1] >= '0' && q[1] <='9')
    {
        *q = 0;
        line_nr = atoi(q+1);
    }
    *pline = line_nr;
    return buff;
}

int set_currentdir(const char *file)
{
    char *p, *q;
    char dir[MAX_PATH];

    //if (FALSE == SearchPath(NULL, buff, NULL, 256, currentdir, NULL))
    if (FALSE == GetFullPathName(file, 256, dir, &q))
        return 0;
    p = dir;
    q = strchr(p, 0);
    while (q > p && q[-1]!='\\' && q[-1]!='/')
        --q;
    if (q >= p+2 && q[-2] != ':')
        --q;
    *q = 0;
    strcpy(currentdir, p);
    return 1;
}

void init_edit(HWND hwnd) {
    struct strl *e;
    int n = 0;

    makefonts();

    e = editfiles;
    if (e)
    {
        do {
            char buff[MAX_PATH];
            int line_nr;
            getfile(buff, e->str, &line_nr);

            if (n == 0)
            {
                if (0 == set_currentdir(buff))
                    GetCurrentDirectory(256, currentdir);
                //dbg_printf("currentdir <%s> <%s>", currentdir, buff);
            }

            if (LoadFile(buff))
            {
                ed_cmd(EK_INIT, 0, 0, 0);
                if (line_nr) {
                    ed_cmd(EK_GOTOLINE,line_nr-1);
                    //ed_cmd(EK_MARK,lpos,imin(lpos+1,nextline(lpos,1)));
                }
            }
            e=e->next;
            ++n;
        } while (e);
        edp = ed0;
    }

    SetCurrentDirectory(strcpy(projectdir, currentdir));

    settitle();
}

void exit_edit(void) {
     deletefonts();
}


/*----------------------------------------------------------------------------*/
char k_alt, k_shft, k_ctrl;
char alt_f;

int trans_keys(int n, int f) {
    int d;

    n=LOWORD(n);
    d=f&1;

    if (n==VK_MENU) {
        if (d)
        {
            if (alt_f==0)
                alt_f=GetAsyncKeyState(VK_LBUTTON)<0 ? 2 : 1;
        }
        else
        {
            if (alt_f==1)
            {
                if (GetAsyncKeyState(VK_CONTROL)<0)
                    PostMessage(ewnd, WM_COMMAND, CMD_MENU_2, 0);
                else
                    PostMessage(ewnd, WM_COMMAND, CMD_MENU_1, 0);
            }
            alt_f=0;
        }
    }
    else alt_f=0;

    switch (n) {
    case VK_SHIFT:   k_shft=d; return 0;
    case VK_CONTROL: k_ctrl=d; return 0;
    case VK_MENU:    k_alt =d; return 0;
    }

    if (0==d) return 0;

    //if (3==f) k_alt = 1;

    if (k_ctrl) switch (n) {

    case  VK_C:      return KEY_C_C;
    case  VK_X:      return KEY_C_X;
    case  VK_V:      return KEY_C_V;
    case  VK_A:      return KEY_C_A;
    case  VK_Z:      return k_shft ? KEY_CS_Z : KEY_C_Z;
    case  VK_Y:      return KEY_C_Y;
    case  VK_S:      return KEY_C_S;
    case  VK_O:      return KEY_C_O;
    case  VK_B:      return KEY_C_B;
    case  VK_N:      return KEY_C_N;
    case  VK_L:      return KEY_C_L;
    case  VK_F:      return KEY_C_F;
    case  VK_U:      return KEY_C_U;

    case  VK_0:      return KEY_C_0;
    case  VK_9:      return KEY_C_9;
    case  VK_8:      return KEY_C_8;
    case  VK_7:      return KEY_C_7;
    }

    d=0;
    if (k_alt)  d=300;
    if (k_ctrl) d=200;
    if (k_shft) d=100;

    switch (n) {

    case  VK_RETURN: return d+KEY_RET;
    case  VK_BACK:   return d+KEY_BACK;
    case  VK_TAB:    return d+KEY_TAB;
    case  VK_ESCAPE: return d+KEY_ESC;

    case  VK_UP:     return d+KEY_UP;
    case  VK_DOWN:   return d+KEY_DOWN;
    case  VK_RIGHT:  return d+KEY_RIGHT;
    case  VK_LEFT:   return d+KEY_LEFT;
    case  VK_NEXT:   return d+KEY_NEXT;
    case  VK_PRIOR:  return d+KEY_PRIOR;
    case  VK_HOME:   return d+KEY_HOME;
    case  VK_END:    return d+KEY_END;
    case  VK_DELETE: return d+KEY_DELETE;
    case  VK_INSERT: return d+KEY_INSERT;

    case  VK_F1:     return d+KEY_F1;
    case  VK_F2:     return d+KEY_F2;
    case  VK_F3:     return d+KEY_F3;
    case  VK_F4:     return d+KEY_F4;
    case  VK_F5:     return d+KEY_F5;
    case  VK_F6:     return d+KEY_F6;
    case  VK_F7:     return d+KEY_F7;
    case  VK_F8:     return d+KEY_F8;
    case  VK_F9:     return d+KEY_F9;
    case  VK_F10:    return d+KEY_F10;
    case  VK_F11:    return d+KEY_F11;
    case  VK_F12:    return d+KEY_F12;
    default:
    //if (d==300 && n>=VK_A && n<=VK_Z) return d + 2050 + n - VK_A;

    return 0;
}}

/*----------------------------------------------------------------------------*/
static int tcmd,ti0;
static UINT Timer=0;

static HCURSOR dragC,dragCp,pointC,hCurs;

HMENU ShortCut;

char infomsg[128];
char tmpbuff[128];

static char lmf;
static UINT infotimer=0;
static char infoflg;
static char clickflag;
static char drag, dragmove;
char  moumrk=0;
char  linmrk=1;
char   vmark=0;
char   hsbar=0;

#define SCROLL_INTERVAL 50

int inmark(int x, int y);

void settimer(HWND hwnd, int cmd, int ti) {
    if (cmd==-1) cmd=0;
    if (Timer && (cmd!=tcmd || ti!=ti0))
        KillTimer(hwnd, 1), Timer=0;

    tcmd=cmd, ti0=ti;
    if (cmd && Timer==0)
        Timer = SetTimer(hwnd, 1, ti, NULL);
}


void InfoMsg(const char *info) {
    SendMessage(ewnd,WM_COMMAND,CMD_INFO,(LPARAM)info);
}

void resetmsg(HWND hwnd) {
    if (infomsg[0] && edp)
        upd=1;

    infomsg[0]=0;

    if (infoflg)
        infoflg=0, KillTimer(hwnd,2);

    if (lmf&&--lmf==0)
        unmark();
}

/*---------------------------------------------------------------*/
#ifdef MV_DandD
int showcaret(HWND hwnd, int x, int y) {
    int o; POINT pt;
    pt.x = x; pt.y = y;
    ScreenToClient(hwnd,&pt);
    o=getmoupos(pt.x,pt.y);
    if (o!=-1) {
        setcaret(hwnd);
        return 0;
    }
    settimer(hwnd,0,SCROLL_INTERVAL);
    return 1;
}

#endif

/*----------------------------------------------------------------------------*/
void getmouxy(int mox, int moy, int* x, int* y) {

        *x = iminmax((mox - zx0 + zx/3)/zx, 0, pgx);
        *y = iminmax((moy - zy0 - zy/2)/zy, 0, pgy);
}

int getmoupos(int mox, int moy) {
        int m,o,x,y;

        getmouxy(mox,moy,&x,&y);

        m=tlin-plin-(winflg&1);
        y=iminmax(y,0,m);

        curx = x+clft;
        cury = y;
        o=0;

        if (mox<zx0)    o=KEY_LEFT ;
        else
        if (mox>cwx)  o=KEY_RIGHT ;

        if (moy<zy0)    o=KEY_UP ;
        else
        if (moy>cwy && m>=pgy)  o=KEY_DOWN ;

#ifdef MV_DandD
        {
            int i=imax(-mox,imax(mox-cwx,imax(-moy,moy-cwy)));
            if (i>20) o=-1;
        }
#endif
        return o;
}

/*---------------------------------------------------------------*/
void set_v_ins(void) {
    vmark=(0!=(GetAsyncKeyState(VK_MENU)&0x8000));
}

void setvmark(int n) {
    if (n && edp && markf2==0
          && (markf1==0 || fpos!=mark2 || curx!=mark2x)) {
          set_v_ins();
    }
    domarking(n);
}

/*---------------------------------------------------------------*/
void setdragc(int k) {
    if (drag!=k)
        drag=k,SetCursor(k==3?dragCp:dragC);
}

/*---------------------------------------------------------------*/
void do_mouse(HWND hwnd, DWORD wParam, DWORD lParam, int msg) {

    int o,x,y,n,i;
    static int mox,moy;
    static char mf2;
    static char lf2;

#ifdef NOCAPT
    static char wmf;
    static int  mux,muy;
#endif

    switch (msg) {
    case WM_MOUSEMOVE: //move
        if ((wParam & MK_RBUTTON)!=0) {
#ifdef NOCAPT
            mox=(short)LOWORD(lParam);
            moy=(short)HIWORD(lParam);
            if (wmf==0) {
                wmf=1,mux=mox,muy=moy;
                return;
            }
            MoveWindow(hwnd,wx0+mox-mux,wy0+moy-muy,wxl,wyl,TRUE);
#endif
            return;
        }

        if (clickflag) return;

        if ((wParam & MK_LBUTTON)==0 || edp==NULL) {
            mox=(short)LOWORD(lParam);
            moy=(short)HIWORD(lParam);
            getmouxy(mox,moy,&x,&y);
            if (edp==NULL) i=1;
            else i=inmark(x+clft,y);
            SetCursor(i?pointC:hCurs);
            return;
        }

        if (lf2==0) return;
#ifndef MV_DandD
mm3:
#endif
        if (drag) {
            setdragc(2+(0!=(GetAsyncKeyState(VK_CONTROL)&0x8000)));
        }
        n=((wParam & MK_SHIFT)!=0 || 0==moumrk);
        if (n==0) { mf2=0; goto mm1; }
        if (mf2) goto mm1;
        mf2=1;
        goto mm2;



    case WM_LBUTTONDBLCLK:
        lf2=0;
        if (edp) {
            o=getkword(fpos, tmpbuff);
            if (tmpbuff[0]) {
                ed_cmd(EK_MARK,o, o+strlen(tmpbuff));
                return;
            }
        }
        goto drag_cancel;

    case WM_LBUTTONDOWN:
        lf2=1;
        resetmsg(hwnd);
        if (clickflag && --clickflag) return;

//#ifndef MV_DandD
        SetCapture(hwnd);
//#endif
        n=2;
mm1:
        mox=(short)LOWORD(lParam);
        moy=(short)HIWORD(lParam);
mm2:
        if (edp==NULL) return;

        o=getmoupos(mox,moy);

        if (o) {
            settimer(hwnd,o,SCROLL_INTERVAL);
            return;
        }

        ed_cmd(EK_SETVAR);
        getmouxy(mox,moy,&x,&y);

        if (n==2 && inmark(x+clft,y)) {
#ifndef MV_DandD
            drag=1;
            goto mm3;
#else
            char *get_block(void);
            char *get_vblock(void);
            char *p,*q; int n;
            p=vmark ? get_vblock() : get_block();
            if (p!=NULL) {
                q = m_alloc(entab (NULL, p, n=strlen(p), tabs)+1);
                q[entab (q, p, n, tabs)]=0;
                i=do_drag(q),
                m_free(q);
                m_free(p);
                if (i==1) ed_cmd(KEY_DELETE);
            }
            return;
#endif
        }

        if (drag && linmrk && vmark==0 && fixline(*ma) != fixline(*me))
            curx=0;

        if (drag && n!=2 && dragmove<2) dragmove++;

        if (n==2)    unmark();
        if (drag==0) setvmark(n==1);
        return;


    case WM_LBUTTONUP:
        lf2=0;
        if (dragmove==2) {
            ed_cmd(drag==3 ? EK_DRAG_COPY : EK_DRAG_MOVE);
            SetCursor(hCurs);
        } else
        if (drag) unmark();
drag_cancel:
        mf2=drag=dragmove=0;
        settimer(hwnd,0,0);

#ifndef MV_DandD
        ReleaseCapture();
#endif
        return;


    case WM_RBUTTONDOWN:
#if 0
        if (NULL!=ShortCut) {
        POINT pt;
        GetCursorPos(&pt);
        TrackPopupMenu(ShortCut,TPM_CENTERALIGN|TPM_RIGHTBUTTON,
                pt.x, pt.y ,0,hwnd,NULL
                );
        return;
        }
#endif

#ifdef NOCAPT
        SetCapture(hwnd);
        mox=(short)LOWORD(lParam);
        moy=(short)HIWORD(lParam);
        wmf=1,mux=mox,muy=moy;
#endif
        return;

    case WM_RBUTTONUP:
#ifdef NOCAPT
        wmf=0;
        if (NULL==ShortCut)
            ReleaseCapture();
#endif
        return;
    }
}




/*----------------------------------------------------------------------------*/
int domove (HWND bwnd, UINT message, WPARAM wParam, LPARAM lParam);
//void SnapWindowToEdge(WINDOWPOS* pwPos, int nDist, BOOL bUseScreenSize);

extern HWND mwnd;
extern UINT bb_broadcast_msg;
int bbn_receive_data(HWND, LPARAM);

LRESULT CALLBACK EditProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam){

    char bstr[MAX_PATH];
    char *p,c;
    int i,k,n,o,r,f;
    POINT pt;
    struct edvars *ev;

    r = 0;

    //if (domove(GetParent(hwnd),message,wParam,lParam)) return 0;

    switch (message) {
    case WM_COPYDATA:
        if (0x4F4E4242 == ((PCOPYDATASTRUCT)lParam)->dwData)
        {
            char buff[MAX_PATH];
            int line_nr;
            const char *file = (const char*)((PCOPYDATASTRUCT)lParam)->lpData;
            getfile(buff, file, &line_nr);
            set_currentdir(buff);
            LoadFile(buff);
            if (line_nr) {
                ed_cmd(EK_GOTOLINE,line_nr-1);
                //ed_cmd(EK_MARK,lpos,imin(lpos+1,nextline(lpos,1)));
            }

            r = TRUE;
            goto p0r;
        }

        return bbn_receive_data(hwnd, lParam);

    default:
        if (bb_broadcast_msg == message && message)
        {
            if (bb_register(hwnd) && bb_getstyle(hwnd))
               goto reconfig1;
            return 0;
        }
        break;

    case BB_RECONFIGURE:
        if (bb_getstyle(hwnd))
            goto reconfig1;
        return 0;


    reconfig1:
        bb_close_dlg();
        makedlgfont();

    reconfig:
        zy0 = title_h + FRM-1;
        setsize(hwnd);
        new_back();
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;

    case BB_SETSTYLESTRUCT:
    {
        StyleStruct *d = (StyleStruct *)wParam;
        StyleStruct *s = (StyleStruct *)lParam;
        memset(d, 0, sizeof *d);
        memcpy(d, s, sizeof *d);
        break;
    }

    case BB_SETSTYLE:
    {
        char *d = (char *)wParam;
        char *s = (char *)lParam;
        strcpy(d, s);
        break;
    }

    case WM_CREATE:
        zy0 = title_h + FRM-1;
        setsize(ewnd=hwnd);
        dragC  = LoadCursor(hInst,MAKEINTRESOURCE(101));
        dragCp = LoadCursor(hInst,MAKEINTRESOURCE(102));
        pointC = hCurs = LoadCursor(NULL,IDC_ARROW);
        init_edit(hwnd);
        DragAcceptFiles(hwnd,TRUE);
        if (false == bb_register(hwnd) || false == bb_getstyle(hwnd))
            readstyle(defstyle);
        goto reconfig1;


    case WM_DROPFILES: {
        POINT pt;
        HDROP hDrop = (HDROP)wParam; int n,f;
        ev=edp;
        f=0;

        GetCursorPos(&pt);
        if (GetAsyncKeyState(VK_CONTROL)<0) f=1;
        if (GetAsyncKeyState(VK_SHIFT)<0)   f=2;
        if (f) ScreenToClient(hwnd,&pt);

        for (i=-1, k=0; i<k; i++)
        {
            n=DragQueryFile (hDrop, i, p=bstr, 255);
            if (i<0)
            {
                k=n;
            }
            else
            if (f==0)
            {
                LoadFile(p);
            }
            else
            if (f==2)
            {
                if (readstyle(p))
                {
                    CopyFile(p, defstyle, FALSE);
                }
                break;
            }
            else
            {
                if (i == 0 && -1 == getmoupos(pt.x ,pt.y))
                    break;

                ed_cmd(EK_SETVAR);
                ed_cmd(EK_INSERT, p);
                ed_cmd(KEY_RET);
            }
        }
        DragFinish(hDrop);
        SetForegroundWindow(hwnd);
        if (f==0) goto showf;
        if (f==2) goto reconfig1;
        goto p0;
        }

    case WM_QUERYENDSESSION:
        return (1 == QueryDiscard(hwnd,1));

    case WM_ENDSESSION:
    {
        //void savecfg(void); savecfg();
        return 0;
    }

    case WM_DESTROY:
        exit_edit();
        DragAcceptFiles(hwnd,FALSE);
        bb_unregister(hwnd);
        PostQuitMessage(0);
        return 0 ;


    case WM_ACTIVATE:
        i=LOWORD(wParam);
        if (i==WA_ACTIVE)
            clickflag=0;

        return 0;

    case WM_ACTIVATEAPP:
        if (wParam) {
            clickflag=2;
        }
        return 0;

    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
        if (alt_f) alt_f=2;

    case WM_MOUSEMOVE:
#if 0
    {
        static int wm;
        i = (short)HIWORD(lParam)/2;
        n = wm;
        wm = i;
        wParam = MAKELPARAM(0,(n-i)*30);
        goto mwhl;

    }
#endif
    case WM_LBUTTONDBLCLK:
    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_MBUTTONUP:
    case WM_MBUTTONDOWN:

        if (drag==0 && domove (hwnd, message, wParam, lParam))
            return 0;

        do_mouse(hwnd, wParam, lParam, message);

        if (message==WM_MOUSEMOVE && 0==(wParam & (MK_LBUTTON | MK_RBUTTON)))
            return 0;

        goto p0;

    case WM_TIMER:

        if (wParam==2) {
            resetmsg(hwnd);
            goto p0;
        }

        if (wParam==4) {
            mousewheelaccu=0;
        t0:
            KillTimer(hwnd, wParam);
            return 0;
        }

        GetCursorPos(&pt);
        ScreenToClient(hwnd,&pt);
        o=getmoupos(pt.x ,pt.y);
        if (o!=tcmd) {
            settimer(hwnd, o, SCROLL_INTERVAL);
            return 0;
        }

        if (tcmd==0) goto t0;
        c=ltup; ltup=0;
        ed_cmd(tcmd);
        ed_cmd(tcmd);
        upd=1;
        ltup=c;
        k=GetAsyncKeyState(VK_SHIFT)&0x8000;
        domarking(k!=0 || (0==moumrk && 0==drag));
        goto p0;


    case WM_SIZE:
        if (SIZE_MINIMIZED == wParam)
            return 0;

        if (edp) upd=1;
        ed_cmd(EK_SIZE);
        new_back();

    case WM_MOVE:
        setsize(hwnd);
        goto p0;

/*
   case WM_WINDOWPOSCHANGED:
        ewx0 = ((LPWINDOWPOS) lParam)->x;
        ewy0 = ((LPWINDOWPOS) lParam)->y;
        ewxl = ((LPWINDOWPOS) lParam)->cx;
        ewyl = ((LPWINDOWPOS) lParam)->cy;
        cfg_f |= 1;
        break; //process wm_move/wm_size

    case WM_WINDOWPOSCHANGING:
        SnapWindowToEdge((WINDOWPOS*)lParam, 10, 0);
        setsize(hwnd);
        return 0;
*/

    case WM_VSCROLL:
        vscroll(wParam);
        goto p0;

    case WM_HSCROLL:
        hscroll(wParam);
        goto p0;

    case WM_SETFOCUS:
        k_alt = 0>GetAsyncKeyState(VK_MENU);
        k_shft= 0>GetAsyncKeyState(VK_SHIFT);
        k_ctrl= 0>GetAsyncKeyState(VK_CONTROL);
        CreateCaret(hwnd,NULL,My_CaretSize,zy);
        caret=1;
        checkftime(hwnd);
        goto f1;

    case WM_KILLFOCUS:
        DestroyCaret();
        caret=0;
    f1:
        if (edp) upd=1;
        goto p0;

p0:
        r=0;
p0r:
        set_update(hwnd);
        return r;


    case WM_ERASEBKGND:
        return 1;

    case WM_PAINT:
        paint_window (hwnd);
        return 0;


    case WM_COMMAND:
        switch (LOWORD(wParam)) {

        case CMD_HELP:
#if 0
            bbnote_help();
            return 0;
#else
            if (fileexist(set_my_path(bstr, "bbnote.txt")))
                LoadFile(bstr);
            else if (fileexist(set_my_path(bstr, "docs/bbnote.txt")))
                LoadFile(bstr);
            goto p0;
#endif


        case CMD_EXIT:
            goto quit;

        case CMD_MENU_2:
        filemenu:
            if (ed0)
            {
                struct edvars *e = ed0;
                struct strl   *s = NULL;
                while (e)
                {
                    char temp[MAX_PATH];
                    sprintf(temp, "&%s", fname(e->sfilename));
                    appendstr(&s, temp);
                    e = e->next;
                }
                bb_file_menu (hwnd, lParam, s);
            }
            return 0;

        case CMD_MENU_1:
            bb_menu(hwnd, lParam);
            return 0;

        case CMD_COLOR:
            goto reconfig;

        case CMD_UPD:
            settitle();
            if (edp) upd=1;
            goto p0;


        case CMD_ZOOM:
    zoom:
            ShowWindow(hwnd, IsZoomed(hwnd) ? SW_SHOWNORMAL : SW_MAXIMIZE);
            return 0;

        case CMD_SEARCH:
        search:
            if (edp) bb_search_dialog(hwnd);
            return 0;

        case CMD_CLOSE:
        closefile:
            if (edp)
            {
                if (1 != QueryDiscard_1(hwnd, 1)) goto p0;
                if (ed0->next==NULL)
                {
                    extern HWND mwnd;
                    SendMessage(mwnd, WM_KEYDOWN, VK_ESCAPE, 0);
                    //DestroyWindow(mwnd);
                }
                CloseFile();
            }
            goto p0;

        case CMD_OPEN:
        openfile:
            ev=edp;
            DoFileOpenSave(hwnd, 0);
showf:
            if (ev==NULL || ev->next)
            {
                edp = ev ? ev->next : ed0;
                settitle();
            }
            goto p0;

        case CMD_RELOAD:
        reload:
            if (edp) f_reload(1);
            goto p0;


        case CMD_LIST:
            lParam = 2;
            goto filemenu;

        case CMD_NEW:
        newfile:
            NewFile();
            goto p0;

        case CMD_SAVE:
        savefile:
            if (edp) DoFileOpenSave(hwnd, 2);
            goto p0;

        case CMD_SAVEAS:
            if (edp!=NULL) DoFileOpenSave(hwnd, 1);
            break;

        case CMD_SAVEALL:
        //saveall:
            return QueryDiscard(hwnd, 0);

        case CMD_UNDO:
            ed_cmd(KEY_C_Z);
            goto p0;

        case CMD_REDO:
            ed_cmd(KEY_CS_Z);
            goto p0;

        case CMD_ABOUT:
            oyncan_msgbox(
              VERSION_STRING
              "\n"
              "\nediting with style"
              "\n04/2003 by grischka"
              "\n"
              "\ngrischka@users.sourceforge.net"
              , NULL, 1);
              return 0;


        case CMD_OPTIONS:
            goto config;

        case CMD_INFO:
            resetmsg(hwnd);
            p=(char*)lParam;
            if (p[0]==1) p++;
            else infoflg=1,infotimer=SetTimer(hwnd,2,666,NULL);
            strcpy(infomsg,p);
            if (edp) upd=1;
            goto p0;


        case CMD_FILECHG:
        {
            struct edvars *p=edp;
            edp=(struct edvars*)lParam;
            settitle();
            set_update(hwnd);
            f_reload(0);
            edp=p;
            settitle();
            goto p0;
        }

        default:
            i = LOWORD(wParam);
            if (i>=CMD_FILE && i< CMD_FILE_M)
            {
                struct edvars *p=ed0;
                i-=CMD_FILE;
                for (;i && p!=NULL; p=p->next,i--);
                if (p) {
                    //edp = p; settitle();
                    insfile(p);
                    goto p0;
                }
                return 0;
            }
            break;

        }
        break;

quit:
    if (1 == QueryDiscard(hwnd, 1))
        DestroyWindow(hwnd);
    return 0;

    case WM_SYSKEYDOWN:
        f=3; goto k1;

    case WM_SYSKEYUP:
        f=2; goto k1;

    case WM_KEYUP:
        if (wParam==VK_CONTROL && drag==3)  setdragc(2);
        f=0; goto k1;

    case WM_KEYDOWN:
        if (wParam==VK_CONTROL && drag==2)  setdragc(3);
        if (wParam==VK_SCROLL) { scroll_lock^=1; goto p0; }

        f=1; k1:

        n=LOWORD(wParam);

#if 0
        sprintf(bstr,"key %d  stat %d",n,f);
        if (n!=VK_MENU) MessageBox(NULL, bstr, "", MB_OK|MB_TOPMOST|MB_SETFOREGROUND);
#endif
        n=trans_keys(n, f);
        //if (0==n) goto p0;
        if (0==n) return 0;

        if (n>=2110 && n<=2117) {
        vmark=k_alt!=0;
        domarking(1);
        ed_cmd(n-100);
        domarking(1);
        goto p0;
        }

        switch (n) {

        case KEY_F8: return 0;
        case KEY_A_RIGHT:
        case KEY_F6:   nextfile(); goto p0;
        case KEY_A_LEFT:
        case KEY_C_F6: prevfile(); goto p0;

        case KEY_F10:  goto zoom;
        case KEY_F3:
        case KEY_C_F:  goto search;
        case KEY_C_F4: goto closefile;
        case KEY_C_O:  goto openfile;
        //case KEY_A_F3: goto reload;
        case KEY_C_N:  goto newfile;
        case KEY_C_L:  lParam = 0; goto filemenu;

        case KEY_F4:
            QueryDiscard(hwnd, 0);
            bb_reconfig();
            return 0;

        case KEY_S_F4:
            if (IDOK == oyncan_msgbox("Do you want to write all files?", "", 1+8))
                QueryDiscard(hwnd, 4);
            return 0;


        case KEY_C_S:  goto savefile;
        case KEY_A_F4: goto quit;

        case KEY_A_F2:
config:
            n = tabs;
            bb_config_dialog(hwnd);
            if (n!=tabs) goto reload;
            return 0;

        case KEY_ESC:
            if (drag==0) goto quit;
            dragmove=0;
            do_mouse(hwnd, 0,0,WM_LBUTTONUP);
            goto p0;

        case KEY_UP:
        case KEY_DOWN:
            if (scroll_lock) n+=200;
            goto defkey;

        case KEY_LEFT:
        case KEY_RIGHT:
            if (scroll_lock) n=(n^1)+204;
            goto defkey;
        }

    defkey:
        domarking(0);
        ed_cmd(n);
        i=n;
        if (i!=KEY_C_A
         && i!=KEY_C_U
         && i!=KEY_C_7
         && i!=KEY_C_8
         && i!=KEY_C_9
         && i!=KEY_C_0
         && i!=KEY_TAB
         && i!=KEY_S_TAB
         )
            unmark();
        goto p0;


    case WM_MOUSEWHEEL:
        i = mousewheelaccu + mousewheelfac * (short)HIWORD(wParam);
        while (i < -300)
            ed_cmd(KEY_C_DOWN), i+=600;
        while (i >  300)
            ed_cmd(KEY_C_UP),   i-=600;

        mousewheelaccu=i;
        unmark();
        SetTimer(hwnd, 4, 200, NULL);
        goto p0;


    case WM_NCPAINT:
        return 0;


    case WM_CHAR:
        n = LOWORD(wParam);
        if (n<32||n==127) return 0;

        resetmsg(hwnd);
        ed_cmd(EK_CHAR, n);
        goto p0;



    case CMD_GOTOLINE:
        ed_cmd(EK_GOTOLINE,wParam-1);
        ed_cmd(EK_MARK,lpos,imin(lpos+1,nextline(lpos,1)));
        lmf=2;
        goto p0;

    case CMD_LOADFILE:
        r=LoadFile((char*)wParam);
        goto p0r;

    case CMD_NSEARCH:
        resetmsg(hwnd);
        if (wParam&8) {
            ed_cmd(EK_REPLACE,(char *)lParam);
            if (wParam&1)
               ed_cmd(EK_GOTO,fpos+strlen((char*)lParam));
            goto p0;
        }
        unmark();
        if (wParam!=0) {
            struct sea *s=(struct sea *)lParam;
            struct edvars *ev0;

            for (ev0=edp;;) {
                r=ed_search(s);
                if (r || 0==(s->sf&128)) break;
                s->sf &= ~4;
                if (s->sf & 1) {
                    if (edp->next==NULL) break;
                    nextfile();
                    s->from=0;
                    continue;
                }
                if (s->sf & 2) {
                    if (edp==ed0) break;
                    prevfile();
                    s->from=flen;
                    continue;
                }}
            if (r<=0) {
                edp=ev0;
                settitle();
            } else {
               ed_cmd(KEY_HOME);
               ed_cmd(EK_MARK,s->a,s->e);
               ed_cmd(EK_GOTO,s->a);
            }
        }
        goto p0r;

    }
    return DefWindowProc (hwnd, message, wParam, lParam) ;
}

/*----------------------------------------------------------------------------*/
int oyncan_msgbox(const char *t, const char *s, int f) {

    char buf[512];
    if (IsIconic(ewnd))
        ShowWindow(ewnd,SW_RESTORE);

    sprintf(buf,t,s);
    s = buf;

    return bb_msgbox(ewnd, s, f);
}

/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/

