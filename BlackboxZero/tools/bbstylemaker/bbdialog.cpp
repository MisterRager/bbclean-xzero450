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

// bbStyleMaker bbdialog.cpp

#include "snap.cpp"

WNDPROC prvlineproc;

void dlg_save_config(struct dlg *dlg);
void dlg_load_config(struct dlg *dlg);

#define DLG_SCALE 100

/*----------------------------------------------------------------------------*/
void do_sound(int f)
{
    MessageBeep(MB_OK);
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
/* button class */

void get_item_rect(struct button* bp, RECT *r)
{
    r->right = (r->left = bp->x) + bp->w;
    r->bottom = (r->top = bp->y) + bp->h;
}

void get_slider_rect(struct button* bp, RECT *r)
{
    int dx = bp->w;
    int k = bp->h - dx;
    r->top = bp->y + k - bp->data * k / SB_DIV;
    r->bottom = r->top + dx;
    r->right = (r->left = bp->x) + bp->w;
}

void set_slider(struct button* bp, int my)
{
    int dx  = bp->w;
    int k = bp->h - dx;
    bp->data = iminmax((bp->y + k - my) * SB_DIV / k, 0, SB_DIV-1);
}

int inside_item(struct button* bp, int mx, int my)
{
    if (0 == (bp->f & BN_HID)) {
       switch (bp->typ) {
        case BN_RECT:
        case BN_STR:
        case BN_EDT:
        case BN_SLD:
        case BN_BTN:
        case BN_CHK:
        case BN_COLOR:
        case BN_ITM:
        case BN_UPDN:
            return mx>=bp->x
                && mx<bp->x+bp->w
                && my>=bp->y
                && my<bp->y+bp->h;
    }}
    return 0;
}

/*----------------------------------------------------------------------------*/
/* class dlg */

void invalidate_button(struct button *bp)
{
    if (bp) {
        RECT r;
        get_item_rect(bp, &r);
        InvalidateRect(bp->dlg->hwnd, &r, FALSE);
    }
}

/*----------------------------------------------------------------------------*/

void ed_setfont(HWND edw, HFONT fnt1)
{
    TEXTMETRIC TXM;
    int realfontheight;
    RECT r;
    int extraspace;

    HDC hdc = CreateCompatibleDC(NULL);
    SelectObject(hdc, fnt1);
    GetTextMetrics(hdc, &TXM);
    DeleteDC(hdc);

    SendMessage (edw, WM_SETFONT, (WPARAM)fnt1, MAKELPARAM (TRUE, 0));

    realfontheight = TXM.tmHeight; //pixel
    GetClientRect(edw, &r);
    extraspace = (r.bottom - realfontheight) / 2;

    if (extraspace<0)
        extraspace=0;
    r.top       += extraspace;
    r.bottom    -= extraspace;
    if (extraspace<2)
        extraspace=2;
    r.left      += extraspace;
    r.right     -= extraspace;

    SendMessage(edw, EM_SETRECT, 0, (LPARAM)&r);
    SendMessage (edw, EM_SETSEL, 0, -1);
}

void set_button_place(struct button *bp)
{
    if (bp->typ == BN_EDT && 0 == (bp->f & BN_HID))
    {
        if (0 == bp->data)
            create_editline(bp);

        SetWindowPos((HWND)bp->data, NULL,
            bp->x,
            bp->y,
            bp->w,
            bp->h,
            SWP_NOACTIVATE|SWP_NOZORDER);

        ed_setfont((HWND)bp->data, bp->dlg->fnt1);
    }
}

void fix_button (struct button *bp)
{
    int dx = bp->dlg->dx;
    int dy = bp->dlg->dy;
    int title_h = bp->dlg->title_h;

    bp->x = bp->x0 * dx / DLG_SCALE;
    bp->y = bp->y0  * dy / DLG_SCALE + title_h;
#if 0
    bp->w = (bp->x0 + bp->w0) * dx / DLG_SCALE - bp->x;
    bp->h = (bp->y0 + bp->h0) * dy / DLG_SCALE + title_h - bp->y;
#else
    bp->w = (bp->w0 * dx + 50) / DLG_SCALE;
    bp->h = (bp->h0 * dy + 50) / DLG_SCALE;
#endif
    if ((bp->typ == BN_CHK
            || (bp->typ == BN_BTN && (bp->f & BN_EXT)))
        && 0 == (bp->f & BN_CEN) && 0 == bp->dlg->config)
    {
        RECT r = {0,0,0,0};
        HDC hdc = CreateCompatibleDC(NULL);
        HGDIOBJ of = SelectObject(hdc, bp->dlg->fnt1);
        DrawText(hdc, bp->str, -1, &r, DT_CALCRECT);
        bp->w = r.right + 8;
        SelectObject(hdc, of);
        DeleteDC(hdc);
    }
    set_button_place(bp);
}

void set_dlg_windowpos(struct dlg* dlg)
{
    int ax, ay;
    ax = ay = 0;
    if (dlg->captionbar) {
        RECT r;
        GetWindowRect(dlg->hwnd, &r);
        ax = r.right - r.left;
        ay = r.bottom - r.top;
        GetClientRect(dlg->hwnd, &r);
        ax -= r.right - r.left;
        ay -= r.bottom - r.top;
    }
    SetWindowPos(dlg->hwnd, NULL,
        dlg->x, dlg->y, dlg->w + ax, dlg->h + ay,
        SWP_NOACTIVATE|SWP_NOZORDER);

}

void invalidate_dlg (struct dlg *dlg)
{
    delete_bitmaps(dlg);
    InvalidateRect(dlg->hwnd, NULL, FALSE);
}


void fix_dlg (struct dlg *dlg)
{
    struct button *bp;

    if (dlg->fnt1)
        DeleteObject(dlg->fnt1);
    dlg->fnt1=CreateStyleFont(&gui_style.MenuFrame);

    if (dlg->fnt2)
        DeleteObject(dlg->fnt2);
    dlg->fnt2=CreateStyleFont(&gui_style.MenuTitle);

    if (dlg->captionbar)
        dlg->title_h = 0;
    else
        dlg->title_h = get_fontheight(dlg->fnt2)
            + 2*(gui_style.MenuTitle.marginWidth
                    + gui_style.MenuTitle.borderWidth);

    dlg->w = dlg->w_orig * dlg->dx / DLG_SCALE;
    dlg->h = dlg->h_orig * dlg->dy / DLG_SCALE + dlg->title_h;

    for (bp = dlg->bn_ptr; bp; bp = bp->next) {
        fix_button(bp);
    }

    if (dlg->typ == D_BOX)
        fix_box(dlg);

    invalidate_dlg(dlg);
}

struct dlg *make_dlg (const struct button *bp0, int w, int h)
{
    struct dlg *dlg;
    struct button **p, *bp;
    dlg = (struct dlg*)c_alloc(sizeof(*dlg));
    dlg->w_orig = w;
    dlg->h_orig = h;
    p = &dlg->bn_ptr;
    for (; bp0->str; ++bp0) {
        bp = (struct button *)c_alloc(sizeof *bp);
        *bp = *bp0;
        bp->dlg = dlg;
        *p = bp;
        p = &bp->next;
    }
    return dlg;
}

void delete_dlg(struct dlg *dlg)
{
    struct button *np, *bp;
    delete_bitmaps(dlg);
    if (dlg->fnt1)
        DeleteObject(dlg->fnt1);
    if (dlg->fnt2)
        DeleteObject(dlg->fnt2);

    for (bp = dlg->bn_ptr; bp; bp = np) {
        np = bp->next;
        m_free(bp);
    }
    m_free(dlg);
}

/*----------------------------------------------------------------------------*/
int get_accel_msg(struct dlg* dlg, int key)
{
    struct button *bp;
    const char *p;
    for (bp = dlg->bn_ptr; bp; bp = bp->next) {
        if (0 == (bp->f & BN_DIS)
             && (bp->typ==BN_BTN || bp->typ==BN_CHK || bp->typ==BN_ITM)
             && NULL!=(p = strchr(bp->str,'&'))
             && (p[1]|32) == ((char)key|32)
             )
        {
            if (bp->typ==BN_CHK) {
                bp->f^=BN_ON;
                invalidate_button(bp);
            }
            if (bp->msg)
                return bp->msg;
            return -1;
        }
    }
    return 0;
}

/*----------------------------------------------------------------------------*/
LRESULT CALLBACK LineProc (HWND hText, UINT msg, WPARAM wParam, LPARAM lParam)
{
    struct button *bp = (struct button *)GetWindowLongPtr(hText, GWLP_USERDATA);
    switch (msg) {
    case WM_CHAR:
        switch (LOWORD(wParam)) {
        case 13:
        case 27:
        case 9:
            return 0;
        case 6:
            return 0;
        default:
            break;
        }
        break;

    case WM_SYSKEYDOWN:
        PostMessage(bp->dlg->hwnd, msg, wParam, lParam);
        break;

    case WM_KILLFOCUS:
        PostMessage(bp->dlg->hwnd, WM_COMMAND, bp->msg, 0);
        break;

    case WM_KEYDOWN:
        switch (LOWORD(wParam)) {
        case VK_RETURN:
            PostMessage(bp->dlg->hwnd, WM_COMMAND, bp->msg, 1);
            break;

        case VK_ESCAPE:
            PostMessage(bp->dlg->hwnd, WM_COMMAND, bp->msg, 0);
            PostMessage(bp->dlg->hwnd, WM_COMMAND, CMD_QUI, 0);
            break;

        case VK_F3:
            return 0;

        case VK_UP:
            PostMessage(bp->dlg->hwnd, WM_COMMAND, bp->msg, 4);
            return 0;

        case VK_DOWN:
            PostMessage(bp->dlg->hwnd, WM_COMMAND, bp->msg, 3);
            return 0;

        case VK_TAB:
            PostMessage(bp->dlg->hwnd, WM_COMMAND, bp->msg,
                0x8000 & GetAsyncKeyState(VK_SHIFT) ? 2 : 1
                );
            return 0;
        }
        break;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc;
        RECT r;
        HDC buf;
        HGDIOBJ oldbuf;
        StyleItem SI;

        hdc = BeginPaint(hText, &ps);
        GetClientRect(hText, &r);
        buf = CreateCompatibleDC(hdc);
        oldbuf = SelectObject(buf, CreateCompatibleBitmap(hdc, r.right, r.bottom));

        SI = gui_style.MenuFrame;
        SI.bevelstyle = BEVEL_SUNKEN;
        SI.bevelposition = BEVEL1;
        SI.borderWidth = 0;

        put_bitmap(bp->dlg, buf, &r, &SI, 0, BMP_NULL, NULL);
        CallWindowProc(prvlineproc, hText, msg, (WPARAM)buf, lParam);

        BitBltRect(hdc, buf, &ps.rcPaint);
        DeleteObject(SelectObject(buf, oldbuf));
        DeleteDC(buf);
        EndPaint(hText, &ps);
        return 0;
    }

    case WM_NCHITTEST:
        if (bp->dlg->config) {
            return HTTRANSPARENT;
        }
        break;

    case WM_ERASEBKGND:
        return TRUE;

    default:
        break;
    }
    return CallWindowProc (prvlineproc, hText, msg, wParam, lParam);
}

void create_editline (struct button *bp)
{
    HWND edw;
    edw = CreateWindow(
      "EDIT",
      bp->str,
      WS_CHILD|WS_VISIBLE|ES_MULTILINE|ES_AUTOHSCROLL,
      0,0,0,0,
      bp->dlg->hwnd,
      (HMENU)EDT_ID,
      g_hInstance,
      NULL
      );
    bp->data = (DWORD_PTR)edw;
    SetWindowLongPtr(edw, GWLP_USERDATA, (LONG_PTR)bp);
    prvlineproc = (WNDPROC)SetWindowLongPtr (edw, GWLP_WNDPROC, (LONG_PTR)LineProc);
    set_button_place(bp);
}

void resize_dlg(struct dlg *dlg)
{
    RECT r;
    GetWindowRect(dlg->hwnd, &r);
    dlg->x = r.left;
    dlg->y = r.top;
    GetClientRect(dlg->hwnd, &r);
    dlg->dx = r.right * DLG_SCALE / dlg->w_orig;
    dlg->dy = r.bottom * DLG_SCALE / dlg->h_orig;
    fix_dlg(dlg);
}

/*----------------------------------------------------------------------------*/

LRESULT CALLBACK dlg_dlg_proc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    struct dlg *dlg = (struct dlg *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if (NULL == dlg)
    {
        if (msg == WM_NCCREATE)
        {
            dlg = (struct dlg *)((CREATESTRUCT*)lParam)->lpCreateParams;
            dlg->hwnd = hwnd;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)dlg);
        }
        return DefWindowProc (hwnd, msg, wParam, lParam);
    }
    switch (msg)
    {
    case WM_DESTROY:
        SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
        break;
        
    case WM_ERASEBKGND:
        if (dlg->captionbar)
            return DefWindowProc (hwnd, msg, wParam, lParam);
        return TRUE;

    case WM_PAINT:
        paint_box(dlg);
        return 0;

    case WM_TIMER:
        if (UPDOWN_TIMER == wParam) { // up-down control
            dlg_timer(dlg, wParam);
            return 0;
        }
        break;

    case WM_CAPTURECHANGED:
        KillTimer(hwnd, 2);
        return 0;

    case WM_EXITSIZEMOVE:
        if (0 == dlg->config)
            resize_dlg(dlg);
        return 0;

    case WM_WINDOWPOSCHANGED:
    {
        WINDOWPOS *wp = (WINDOWPOS*)lParam;
        if (dlg->config)
        {
            dlg->w_orig = wp->cx * DLG_SCALE / dlg->dx;
            dlg->h_orig = (wp->cy - dlg->title_h) * DLG_SCALE / dlg->dy;
            dlg->w = wp->cx;
            dlg->h = wp->cy;
            invalidate_dlg(dlg);
        } else {
            dlg->x = wp->x;
            dlg->y = wp->y;
        }
        break;
    }


    case WM_KEYDOWN:
        switch (wParam) {
        case VK_INSERT:
            dlg->config = 0 == dlg->config;
            fix_dlg(dlg);
            return 0;

        default:
            if (dlg->config) switch (wParam) {
                case '0':
                case '2':
                case '4':
                case '6':
                case '8':
                    g_snap_gridsize = wParam - '0';
                    return 0;

                case VK_F4:
                    dlg_save_config(dlg);
                    return 0;

                case VK_F2:
                    dlg_load_config(dlg);
                    invalidate_dlg(dlg);
                    return 0;
            }
            break;
        }
        break;

    case WM_CTLCOLOREDIT:
        SetBkMode ((HDC)wParam, TRANSPARENT);
        SetTextColor ((HDC)wParam, gui_style.MenuFrame.TextColor);
        return (LRESULT)GetStockObject(NULL_BRUSH);

    case WM_COMMAND:
        if (EDT_ID == LOWORD(wParam))
        {
            if (EN_UPDATE == HIWORD(wParam)
            ||  EN_HSCROLL == HIWORD(wParam))
            {
                InvalidateRect((HWND)lParam, NULL, FALSE);
            }
            else
            if (EN_KILLFOCUS == HIWORD(wParam))
            {
                ;
            }
            return 0;
        }
        break;
    }
    return dlg->proc(dlg, hwnd, msg, wParam, lParam);
}

/*----------------------------------------------------------------------------*/
int make_dlg_wnd(struct dlg* dlg, HWND parent_hwnd, int x, int y, const char *title, dlg_proc *proc)
{
    static bool reg;
    WNDCLASS wc;
    UINT wstyle;

    memset(&wc, 0, sizeof (WNDCLASS));
    wc.style         = CS_DBLCLKS;//|CS_HREDRAW|CS_VREDRAW;
    wc.lpfnWndProc   = dlg_dlg_proc;
    wc.lpszClassName = APPNAME;
    wc.hInstance     = g_hInstance;
    wc.hIcon         = LoadIcon(g_hInstance, MAKEINTRESOURCE(100));
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(LTGRAY_BRUSH);
    if (0 == reg && FALSE == RegisterClass (&wc))
        return 0;

    reg = 1;

    dlg->proc  = proc;
    strcpy(dlg->title, title);

    if (dlg->captionbar)
        wstyle = parent_hwnd
            ? WS_POPUPWINDOW|WS_CLIPSIBLINGS|WS_CLIPCHILDREN|WS_CAPTION
            : WS_OVERLAPPEDWINDOW|WS_CLIPSIBLINGS|WS_CLIPCHILDREN
            ;
    else
        wstyle = WS_POPUP|WS_CLIPSIBLINGS|WS_CLIPCHILDREN|WS_MINIMIZEBOX;

    CreateWindowEx(
        NULL == parent_hwnd ? WS_EX_ACCEPTFILES : WS_EX_TOOLWINDOW,
        wc.lpszClassName,
        dlg->title,
        wstyle,
        0, 0, 200, 200,
        parent_hwnd,    // parent window handle
        NULL,           // window menu handle
        g_hInstance,    // program instance handle
        dlg             // creation parameters
        );

    fix_dlg(dlg);

    if (x == -1 || y == -1) {
        //center dialog
        RECT r;
        GetWindowRect(parent_hwnd ? parent_hwnd : GetDesktopWindow(), &r);
        x = r.left + imax(20, (r.right - r.left - dlg->w)/2);
        y = r.top  + imax(20, (r.bottom - r.top - dlg->h)/2);
    }
    dlg->x = x;
    dlg->y = y;

    set_dlg_windowpos(dlg);
    ShowWindow(dlg->hwnd, SW_SHOW);
    return 1;
}

/*----------------------------------------------------------------------------*/
void delete_bitmaps(struct dlg* dlg)
{
    int n = BMP_ALL;
    HGDIOBJ *pBmp = dlg->my_bmps;
    do {
        if (*pBmp) DeleteObject(*pBmp), *pBmp = NULL;
        pBmp++;
    } while (--n);
}

void put_bitmap(struct dlg* dlg, HDC hdc, RECT *rc, StyleItem *pSI, int borderWidth, int index, RECT *rPaint)
{
    HBITMAP *pBmp = (HBITMAP *)dlg->my_bmps + index;

    HDC buf = CreateCompatibleDC(hdc);
    HGDIOBJ other;

    int w = rc->right - rc->left;
    int h = rc->bottom - rc->top;

    if (NULL == *pBmp)
    {
        RECT rect = {0, 0, w, h };
        COLORREF borderColor = gui_style.borderColor;
        if (borderWidth < 0) {
            borderColor = pSI->borderColor;
            borderWidth = pSI->borderWidth;
        }

        if (pSI->parentRelative)
        {
            CreateBorder(hdc, rc, borderColor, borderWidth);
            return;
        }

        *pBmp = CreateCompatibleBitmap(hdc, w, h);
        other = SelectObject(buf, *pBmp);
        MakeGradient(buf, rect,
            pSI->type,
            pSI->Color,
            pSI->ColorTo,
            pSI->interlaced,
            pSI->bevelstyle,
            pSI->bevelposition,
            0,
            borderColor,
            borderWidth
            );
    }
    else
    {
        other = SelectObject(buf, *pBmp);
    }

    if (rPaint)
    {
        int dx = imax(0, rPaint->left - rc->left);
        int dy = imax(0, rPaint->top - rc->top);
        w = iminmax(w-dx, 0, rPaint->right - rPaint->left);
        h = iminmax(h-dy, 0, rPaint->bottom - rPaint->top);
        BitBlt(hdc, rc->left+dx, rc->top+dy, w, h, buf, dx, dy, SRCCOPY);
        //dbg_printf("p: %d %d - %d %d - %d %d", rc->left+dx, rc->top+dy, w, h, dx, dy);
    }
    else
        BitBlt(hdc, rc->left, rc->top, w, h, buf, 0, 0, SRCCOPY);

    SelectObject(buf, other);
    DeleteDC(buf);

    if (BMP_NULL == index)
        DeleteObject(*pBmp), *pBmp = NULL;
    else
    if (dlg->config) {
        if (index > BMP_FRAME)
            DeleteObject(*pBmp), *pBmp = NULL;
    }
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

void draw_menu_title(struct dlg *dlg, HDC hdc, RECT rw, const char *title, int index)
{
    StyleItem *s_tit = &gui_style.MenuTitle;
    int frm = s_tit->marginWidth + s_tit->borderWidth + 2;
    HGDIOBJ hf0;

    if (s_tit->parentRelative)
    {
        COLORREF bc = s_tit->borderColor;
        int bw = s_tit->borderWidth;
        int d = gui_style.MenuFrame.borderWidth + 3;
        draw_line(hdc, rw.left + d, rw.right - d, rw.bottom - bw, bw, bc);
        if (dlg->config)
            CreateBorder (hdc, &rw, bc, 1);

    } else {
        put_bitmap(dlg, hdc, &rw, s_tit, -1, index, NULL);
    }

    hf0 = SelectObject (hdc, dlg->fnt2);
    SetTextColor (hdc, s_tit->TextColor);
    rw.left  +=frm;
    rw.right -=frm;
    DrawText(hdc, title, strlen(title), &rw,
        BMP_TITLE == index
            ? DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_PATH_ELLIPSIS
            : s_tit->Justify|DT_VCENTER|DT_SINGLELINE|DT_NOCLIP
            );
    SelectObject (hdc, hf0);
}

/*----------------------------------------------------------------------------*/
void paint_box(struct dlg* dlg)
{

    PAINTSTRUCT ps ;
    HDC         hdc;
    HGDIOBJ hf0;
    RECT rw,r,r2;

    struct button *bp;
    struct button *ap[200], **app;

    StyleItem *sty, *s_frm, *s_tit, *s_hil;
    char bstr[256];
    COLORREF c;
    COLORREF bc;
    const char *p;
    int x, y, f;
    RECT tempRect;
    int act, on, dis;

#define MEMDC
#ifdef MEMDC
    HBITMAP     bm;
    HDC         fhdc ;
    HGDIOBJ     bm0;

    fhdc = BeginPaint (dlg->hwnd, &ps);
    hdc  = CreateCompatibleDC(fhdc);
    bm   = CreateCompatibleBitmap(fhdc, dlg->w, dlg->h);
    bm0  = SelectObject(hdc, bm);
#else
    hdc  = BeginPaint(dlg->hwnd, &ps);
#endif

    SetBkMode (hdc,TRANSPARENT);
    hf0 = SelectObject (hdc, dlg->fnt1);

    s_frm = &gui_style.MenuFrame;
    s_tit = &gui_style.MenuTitle;
    s_hil = &gui_style.MenuHilite;
    bc = s_frm->borderColor;

    rw.left = rw.top = 0;
    rw.right = dlg->w;
    rw.bottom = dlg->h;

    //frame
    if (0 == dlg->captionbar && false == s_tit->parentRelative)
        rw.top = dlg->title_h - s_tit->borderWidth;

    put_bitmap(dlg, hdc, &rw, s_frm,
        dlg->captionbar || (s_frm->bevelposition && s_tit->parentRelative) ? 0 : -1,
        BMP_FRAME, &ps.rcPaint);

    // title
    rw.top = 0;
    rw.bottom = rw.top + dlg->title_h;
    if (dlg->title_h && IntersectRect(&tempRect, &ps.rcPaint, &rw))
        draw_menu_title(dlg, hdc, rw, dlg->title, BMP_TITLE);

    // objects - need to paint in reverse order
    for (app = ap, bp = dlg->bn_ptr; bp; bp = bp->next) {
        if (bp->f & BN_HID)
            continue;
        *app++ = bp;
    }

    while (--app >= ap) {
        bp = *app;
        get_item_rect(bp, &rw);

        if (FALSE == IntersectRect(&tempRect, &ps.rcPaint, &rw))
            continue;

        x = strlen(p = bp->str);

        act = 0 != (bp->f & BN_ACT);
        on = 0 != (bp->f & BN_ON);
        dis = 0 != (bp->f & BN_DIS);
        c = gui_style.MenuFrame.TextColor;

        if (dis) {
            c = gui_style.MenuFrame.disabledColor;
            act = on = 0;
        }

        if (dlg->config) {
            CreateBorder (hdc, &rw, bc, 1);
            act = on = 0;
        }

        switch (bp->typ) {

        case BN_BTN:
            f = (bp->f & BN_LFT) ? DT_LEFT : (bp->f & BN_RHT) ?DT_RIGHT : DT_CENTER;
            if (bp->f & BN_EXT) {
                on = act, act = 0;
                goto case_BN_CHK;
            }
            if (act)
                sty = &gui_style.ToolbarButtonPressed;
            else
                sty = &gui_style.ToolbarButton;
            put_bitmap(dlg, hdc, &rw, sty, -1, BMP_NULL, NULL);
            SetTextColor (hdc, sty->TextColor);
            DrawText(hdc, p, x, &rw, f|DT_VCENTER|DT_SINGLELINE);
            break;

        case BN_UPDN:
            if (act) {
                put_bitmap(dlg, hdc, &rw, s_hil, -1, BMP_NULL, NULL);
                c = s_hil->TextColor;
            }
            SetTextColor (hdc, c);
            rw.left += 4;
            rw.right -= 4;

            DrawText(hdc, bp->str, -1, &rw, DT_LEFT|DT_VCENTER|DT_SINGLELINE);
            rw.left += (rw.right - rw.left)/2;
            if (act) {
                sprintf (bstr, "-%d+", (int)bp->data);
            } else {
                sprintf (bstr, "%d", (int)bp->data);
            }
            DrawText(hdc, bstr, -1, &rw, DT_CENTER|DT_VCENTER|DT_SINGLELINE);
            break;

        case BN_CHK:
        case_BN_CHK:
            f = (bp->f & BN_LFT) ? DT_LEFT : (bp->f & BN_RHT) ?DT_RIGHT : DT_CENTER;
            y=6;
            r2 = rw;
            if (f == DT_CENTER)
            {
                r = r2;
                r2.left  +=4;
                r2.right -=2;

                if (on && !act)
                    c = s_hil->TextColor;

                if (0 == (bp->f & BN_CEN))
                    f = DT_LEFT;

            }
            else
            if (f == DT_RIGHT)
            {
                r.right    = r2.right -2;
                r.left     = r.right - y;
                r.bottom   = r2.bottom-3;
                r.top      = r.bottom - y;
                r2.right   -= y*2;
                r2.left    += 4;
            }
            else
            {
                r.left     = r2.left + 2;
                r.right    = r.left + y;
                r.bottom   = r2.bottom-3;
                r.top      = r.bottom - y;
                r2.left    += y*2;
            }

            if (act)
            {
                CreateBorder (hdc, &rw, s_hil->borderColor, 1);
            }
            else
            if (on)
            {
                put_bitmap(dlg, hdc, &r, s_hil, -1, BMP_NULL, NULL);
            }

            SetTextColor (hdc, c);
            DrawText(hdc, p, x, &r2, f|DT_SINGLELINE|DT_NOCLIP|DT_VCENTER);
            break;

        case BN_RECT:
            CreateBorder (hdc, &rw, s_frm->borderColor, s_frm->borderWidth);
            break;

        case BN_STR:
            if (bp->f & BN_EXT)
            {
                draw_menu_title(dlg, hdc, rw, p, BMP_NULL);
                break;
            }
            if (dlg->config) {
                CreateBorder (hdc, &rw, bc, 1);
            }

            f = bp->f&BN_LFT ? DT_LEFT : bp->f&BN_RHT ? DT_RIGHT : DT_CENTER;

            if (NULL == strchr(p, '\n'))
                f |= DT_SINGLELINE|DT_VCENTER;

            SetTextColor (hdc, c);
            DrawText(hdc, p, x, &rw, f);
            break;

        case BN_EDT:
            //CreateBorder (hdc, &rw, s_hil->borderColor, 1);
            break;

        case BN_ITM:
            if (act)
            {
               put_bitmap(dlg, hdc, &rw, s_hil, -1, BMP_NULL, NULL);
               c = s_hil->TextColor;
            }
            rw.left += s_frm->marginWidth + s_frm->borderWidth + 2;
            SetTextColor (hdc, c);
            for (;;) {
                char *s = strchr(strcpy(bstr, p), 9);
                if (s) *s++=0;
                DrawText(hdc, bstr, strlen(bstr), &rw, DT_LEFT|DT_SINGLELINE|DT_VCENTER);
                if (!s) break;
                p = s;
                rw.left += dlg->tab;
            }
            break;

        case BN_SLD:
            x = bp->w/2;
            rw.top    += x;
            rw.bottom -= x;
            rw.left   += (bp->w)/2 - 2;
            rw.right  = rw.left + 4;

            sty = &gui_style.ToolbarButtonPressed;
            put_bitmap(dlg, hdc, &rw, sty, -1, BMP_SLD1, NULL);
            f = 0;

            get_slider_rect(bp, &rw);
            sty = s_tit;
            if (sty->parentRelative)
                sty = &gui_style.ToolbarButton, f = -1;
            if (sty->parentRelative)
                sty = &gui_style.ToolbarButtonPressed;

            put_bitmap(dlg, hdc, &rw, sty, f, BMP_SLD2, NULL);
            break;

        case BN_COLOR:
        {
            COLORREF TC; HGDIOBJ hp0; NStyleItem *pS;

            x = bp->msg - PAL_1;
            pS = x < 0 ? P0 : &Palette[x];
            if (NULL == pS || pS->nVersion == 0)
            {
                CreateBorder (hdc, &rw, bc, 1);
                break;
            }

            TC = P0_dis ? pS->disabledColor : pS->TextColor;
            put_bitmap(dlg, hdc, &rw, (StyleItem*)pS, -1, BMP_NULL, NULL);

            x = (rw.left + rw.right-1) / 2;
            rw.top += 3;
            rw.bottom -= 3;

            hp0 = SelectObject(hdc, CreatePen(PS_SOLID, 1, TC));
            //MoveToEx (hdc, x-1, rw.top, NULL);
            //LineTo   (hdc, x-1, rw.bottom);
            MoveToEx (hdc, x, rw.top, NULL);
            LineTo   (hdc, x, rw.bottom);
            MoveToEx (hdc, x+1, rw.top, NULL);
            LineTo   (hdc, x+1, rw.bottom);
            DeleteObject(SelectObject(hdc, hp0));
            break;
        }
    }}

    SelectObject (hdc, hf0);

#ifdef MEMDC
    BitBltRect(fhdc, hdc, &ps.rcPaint);
    SelectObject (hdc, bm0);
    DeleteDC(hdc);
    DeleteObject (bm);
#endif

    EndPaint (dlg->hwnd, &ps);
}

/*----------------------------------------------------------------------------*/
void add_ud(struct button *bp, int dir)
{
    int mod, value = bp->data;
    if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
    {
        dir *= 10;
        mod = value % dir;
        if (mod>0 && dir<0) mod+=dir;
        value -= mod;
    }
    value = imax(value + dir, 0);
    if (bp->f & BN_16)
        value = imin(value, 16);
    if (bp->f & BN_255)
        value = imin(value, 255);
    bp->data = value;
}

/*----------------------------------------------------------------------------*/
int dlg_mouse(struct dlg* dlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    struct button *bp;
    int mx, my;
    int f, m, s;
    RECT r;

    mx=(short)LOWORD(lParam);
    my=(short)HIWORD(lParam);

    switch (msg)
    {
    // -------------------------------------
    case WM_RBUTTONDBLCLK:
    case WM_RBUTTONDOWN:
        m = 1;
        goto down;
    case WM_LBUTTONDBLCLK:
    case WM_LBUTTONDOWN:
        m = 0;
    down:
        SetFocus(dlg->hwnd);

        dlg->tx = mx;
        dlg->ty = my;
        dlg->bn_act = NULL;

        if (my>0 && my<dlg->title_h)
        {
            dlg->tf = m+1;
            SetCapture(dlg->hwnd);
            return 0;
        }

        s = 0 != (0x8000 & GetAsyncKeyState(VK_MENU));
        bp = mousebutton(dlg, mx, my);

        if (NULL == bp) {
            if (dlg->config && s) {
                PostMessage(dlg->hwnd, WM_SYSCOMMAND, 0xf008, 0);
            }
            return 0;
        }

        SetCapture(dlg->hwnd);

        dlg->bn_act = bp;
        invalidate_button(bp);

        if (dlg->config) {
            dlg->sizing = s;
            if (dlg->sizing) {
                dlg->sx = bp->w0;
                dlg->sy = bp->h0;
            } else{
                dlg->sx = bp->x0;
                dlg->sy = bp->y0;
            }
            insert_first(dlg, bp);
            return -1;
        }

        bp->f|=BN_ACT;

        if (bp->typ==BN_SLD) {
            get_slider_rect(bp, &r);
            if (mx >= r.left && mx<r.right && my>=r.top && my<r.bottom) {
                dlg->ty = my - r.top;
            } else {
                dlg->ty = bp->w/2;
                set_slider(bp, my - dlg->ty);
                return bp->msg;
            }
            return 0;
        }

        if (bp->typ==BN_UPDN) {
            if (mx < bp->x + bp->w*2/3)
                dlg->ud = -1;
            else
                dlg->ud = 1;

            if (dlg->ud) {
                add_ud(bp, dlg->ud);
                SetTimer(dlg->hwnd, UPDOWN_TIMER, 300, NULL);
            }
            return 0;
        }
        return 0;

    // -------------------------------------
    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
        ReleaseCapture();
        bp = dlg->bn_act;
        dlg->bn_act = NULL;
        if (dlg->tf) {
        /*
            if (tf == 2 && my>0 && my<dlg->title_h) {
                PostMessage(dlg->hwnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
            }
        */
            dlg->tf = 0;
            return 0;
        }

        if (dlg->config) {
            return -1;
        }

        if (bp && (bp->f & BN_ACT)) {
            bp->f &= ~BN_ACT;
            invalidate_button(bp);
            switch (bp->typ)
            {
                case BN_CHK:
                    if (bp->f & BN_RAD)
                        check_radio(dlg, bp->msg);
                    else
                        bp->f^=BN_ON;
                case BN_COLOR:
                case BN_UPDN:
                case BN_BTN:
                case BN_SLD:
                case BN_ITM:
                    return bp->msg;
            }
        }
        return 0;

    // -------------------------------------
    case WM_MOUSEMOVE:
        if (dlg->tf==1) {
            dlg->x += mx - dlg->tx;
            dlg->y += my - dlg->ty;
            SetWindowPos(dlg->hwnd, NULL, dlg->x, dlg->y, 0, 0, SWP_NOSIZE|SWP_NOZORDER);
            return 0;
        }

        bp = dlg->bn_act;
        if (bp) {

            if (dlg->config) {
                int dx = (mx - dlg->tx) * DLG_SCALE / dlg->dx;
                int dy = (my - dlg->ty) * DLG_SCALE / dlg->dy;
                invalidate_button(bp);
                if (dlg->sizing) {
                    bp->w0 = imax(4, dlg->sx + dx);
                    bp->h0 = imax(4, dlg->sy + dy);
                    snap_button(dlg, bp, true, NULL);
                } else {
                    bp->x0 = dlg->sx + dx;
                    bp->y0 = dlg->sy + dy;
                    snap_button(dlg, bp, false, NULL);
                }
                fix_button(bp);
                set_button_place(bp);
                invalidate_button(bp);
                return -1;
            }

            if (bp->typ==BN_SLD) {
                set_slider(bp, my - dlg->ty);
                invalidate_button(bp);
                return bp->msg;
            }

            f = bp->f;
            if (inside_item(bp, mx, my))
                bp->f|=BN_ACT;
            else
                bp->f&=~BN_ACT;

            if (f != bp->f) {
                invalidate_button(bp);
            }

            return 0;
        }

// #define SHOW_HOOVER
#ifdef SHOW_HOOVER
        for (bp = dlg->bn_ptr; bp; bp = bp->next) {
            int xx, yy, d;

            if (bp->f & BN_DIS)
                continue;

            if (mx < bp->x)
                xx = bp->x - mx;
            else
            if (mx > bp->x + bp->w)
                xx = mx - bp->x - bp->w;
            else
                xx = 0;

            if (my < bp->y)
                yy = bp->y - my;
            else
            if (my > bp->y + bp->h)
                yy = my - bp->y - bp->h;
            else
                yy = 0;

            //xx = mx - (bp->x + bp->w/2);
            //yy = my - (bp->y + bp->h/2);

            d = (xx*xx + yy*yy);

            if (d < 400) {
            //if (inside_item(bp, mx, my)) {
                if (0==(bp->f&BN_ACT) && bp->typ != BN_BTN) {
                    bp->f|=BN_ACT;
                    invalidate_button(bp);
                }
            } else if (bp->f&BN_ACT) {
                bp->f&=~BN_ACT;
                invalidate_button(bp);
            }
        }
#endif
        return 0;
    // -------------------------------------
    }

    return 0;
}

/*----------------------------------------------------------------------------*/
const struct button msg_btn[] = {
    { ""            , BN_STR, 0        , 10,  8, 48, 12, 0},
    { "ok"          , BN_BTN, IDOK     ,  0, 16, 48, 16, 0},
    { "&yes"        , BN_BTN, IDYES    ,  0, 16, 48, 16, 0},
    { "&no"         , BN_BTN, IDNO     ,  0, 16, 48, 16, 0},
    { "&cancel"     , BN_BTN, IDCANCEL ,  0, 16, 48, 16, 0},
    { "&always"     , BN_BTN, IDALWAYS ,  0, 16, 48, 16, 0},
    { "ne&ver"      , BN_BTN, IDNEVER  ,  0, 16, 48, 16, 0},
    {NULL}
};


void fix_box(struct dlg *dlg)
{
    HDC hdc;
    HGDIOBJ hf0;
    RECT r1;
    int i, x, z, b, f, w, h;
    struct button *bp;
    const char *s;

    bp = dlg->bn_ptr;
    s = bp->str;
    f = dlg->box_flags;

    //get size of msg-text
    hdc = CreateCompatibleDC(NULL);
    hf0 = SelectObject(hdc, dlg->fnt1);
    ZeroMemory(&r1,sizeof(r1));
    DrawText(hdc, s, strlen(s), &r1, DT_CALCRECT);
    SelectObject(hdc, hf0);
    DeleteDC(hdc);

    //how many buttons to display ?
    for (i=0, z=f&255; z; i+=(z&1), z>>=1);

    b = i * bp->w + (i-1)*4;

    bp->w = imax(r1.right, b);
    bp->h = r1.bottom;

    w = bp->w + 2*bp->x;
    x = (w - b) / 2;
    h = r1.bottom;

    dlg->w = w;
    dlg->h += h;

    //enable and fixup buttons
    while (NULL != (bp = bp->next)) {
        if (f&1) {
            bp->x = x;
            bp->y += r1.bottom;
            x += bp->w + 4;
        }
        else
            bp->f |= (BN_DIS|BN_HID);
        f>>=1;
    }
}

/*----------------------------------------------------------------------------*/
int bb_msgbox(HWND hwnd, const char *s, const char *t, int f)
{
    struct dlg *dlg;
    LRESULT CALLBACK msg_proc (struct dlg *dlg, HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    dlg = make_dlg(msg_btn, 20, 42);
    dlg->typ = D_BOX;
    dlg->bn_ptr->str = s;
    dlg->box_flags = f;
    dlg->dx = DLG_SCALE;
    dlg->dy = DLG_SCALE;

    if (0==make_dlg_wnd(dlg, hwnd, -1, -1, t, msg_proc))
        return IDCANCEL;

    do_sound(1);
    if (NULL == hwnd)
        return 0;

    //modal dialog msg-loop
    EnableWindow(hwnd, FALSE);
    f = do_message_loop();
    EnableWindow(hwnd, TRUE);
    return f;
}

/*----------------------------------------------------------------------------*/
LRESULT CALLBACK msg_proc (struct dlg *dlg, HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    int f;
    HWND hwnd_parent;
    switch (msg) {

        case WM_CREATE:
            return 0;

        case WM_DESTROY:
            if (GetParent(hwnd))
                PostQuitMessage(dlg->ud);
            delete_dlg(dlg);
            return 0;

        case WM_CLOSE:
            f = IDCANCEL;
            goto post;

        case WM_MOUSEMOVE:
        case WM_LBUTTONDBLCLK:
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
            f = dlg_mouse(dlg, msg, wParam, lParam);
        post:
            if (f<=0)
                return 0;

            dlg->ud = f;
            hwnd_parent = GetParent(hwnd);
            if (hwnd_parent) {
                EnableWindow(hwnd_parent, TRUE);
                SetFocus(hwnd_parent);
            }
            DestroyWindow(hwnd);
            return 0;

        case WM_KEYDOWN:
            switch (LOWORD(wParam)) {
            case VK_TAB:
                 break;

            case VK_ESCAPE:
                f=IDCANCEL;
                if (0 == (getbutton(dlg, f)->f&BN_DIS))
                    goto post;
                f=IDNO;
                if (0 == (getbutton(dlg, f)->f&BN_DIS))
                    goto post;
                f=IDOK;
                if (0 == (getbutton(dlg, f)->f&BN_DIS))
                    goto post;
                break;

            case VK_RETURN:
                f=IDYES;
                if (0 == (getbutton(dlg, f)->f&BN_DIS))
                    goto post;
                f=IDOK;
                if (0 == (getbutton(dlg, f)->f&BN_DIS))
                    goto post;
                break;
            }
            return 0;

        case WM_CHAR:
            f = get_accel_msg(dlg, LOWORD(wParam));
            goto post;
    }
    return DefWindowProc (hwnd, msg, wParam, lParam);
}

/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
struct button * getbutton(struct dlg* dlg, int msg)
{
    struct button *bp;
    for (bp = dlg->bn_ptr; bp; bp = bp->next) {
        if (msg==bp->msg)
            return bp;
    }
    return NULL;
}

void check_button(struct dlg* dlg, int msg, int f)
{
    struct button *bp = getbutton (dlg, msg);
    if (bp && (0 != f) != (0 != (bp->f & BN_ON))) {
        if (0==f)
            bp->f &=~BN_ON;
        else
            bp->f |=BN_ON;
        invalidate_button(bp);
    }
}

void check_radio(struct dlg* dlg, int msg)
{
    struct button *bp; int d, m, f;
    for (d = -1, m = msg;;) {
        while (NULL != (bp = getbutton(dlg, m))) {
            f = bp->f;
            if (d > 0 && (f & BN_GRP))
                break;
            if (bp->typ == BN_CHK && (f & BN_RAD)) {
                if ((f & BN_ON) && m != msg) {
                    bp->f &=~BN_ON;
                    invalidate_button(bp);
                }
            }
            if (d < 0 && (f & BN_GRP))
                break;
            m += d;
        }
        if ((d = -d) < 0)
            break;
        m = msg + 1;
    }
    check_button(dlg, msg, true);
}

void set_button_state(struct dlg* dlg, int msg, int f)
{
    struct button *bp = getbutton(dlg, msg);
    if (NULL == bp)
        return;

    if (f == (bp->f & (BN_HID|BN_DIS)))
        return;

    bp->f = (bp->f & ~(BN_HID|BN_DIS)) | f;
    invalidate_button(bp);

    if (bp->typ == BN_EDT) {
        f = 0 == (bp->f & BN_HID);
        if (f && 0 == bp->data) {
            create_editline(bp);
        } else if (0 == f && bp->data) {
            DestroyWindow((HWND)bp->data);
            bp->data = 0;
        }
    }
}

void enable_button(struct dlg* dlg, int msg, int f)
{
    set_button_state(dlg, msg, f ? 0 : BN_DIS);
}

void show_button(struct dlg* dlg, int msg, int f)
{
    set_button_state(dlg, msg, f ? 0 : BN_DIS|BN_HID);
}

void enable_section(struct dlg* dlg, int i1, int i2, int en)
{
    int i;
    for (i = i1; i <= i2; i++)
        enable_button(dlg, i, en);
}

void show_section(struct dlg* dlg, int i1, int i2, int en)
{
    int i;
    for (i = i1; i <= i2; i++)
        show_button(dlg, i, en);
}

/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
void invalidate_item(struct dlg* dlg, int id)
{
    struct button *bp = getbutton(dlg, id);
    invalidate_button(bp);
}

void set_button_data(struct dlg* dlg, int msg, int f)
{
    struct button *bp = getbutton (dlg, msg);
    if (bp && f != (int)bp->data)
    {
        bp->data = f;
        invalidate_button(bp);
    }
}

struct button * mousebutton(struct dlg* dlg, int mx, int my)
{
    struct button *bp; int n;
    if (mx < 0 || mx >= dlg->w || my < 0 || my >= dlg->h)
        return NULL;
    for (bp = dlg->bn_ptr; bp; bp = bp->next) {
        if ((bp->typ >= BN_BTN && 0 == (bp->f & BN_DIS)) || dlg->config) {
            n = inside_item(bp, mx, my);
            if (n) return bp;
        }
    }
    return NULL;
}

void insert_first(struct dlg* dlg, struct button *cp)
{
    struct button *bp = dlg->bn_ptr, *np;
    if (bp != cp) for (;;) {
        if ((np = bp->next) == cp) {
            bp->next = np->next;
            cp->next = dlg->bn_ptr;
            dlg->bn_ptr = cp;
            return;
        }
        if (NULL == (bp = np))
            return;
    }
}

void dlg_timer(struct dlg* dlg, int n_timer)
{
    struct button *bp = dlg->bn_act;
    KillTimer(dlg->hwnd, n_timer);
    if (bp && bp->typ == BN_UPDN)
    {
        add_ud(bp, dlg->ud);
        invalidate_button(bp);
        SetTimer(dlg->hwnd, n_timer, 100, NULL);
    }
}

void set_button_text(struct dlg *dlg, int id, const char *text)
{
    struct button *bp = getbutton(dlg, id);
    if (NULL == bp)
        return;

    if (bp->typ == BN_EDT) {
        HWND hText = (HWND)bp->data;
    #if 0
        DWORD a, e;
        a = 0, e = strlen(text);

        SetWindowText(hText, text);
        PostMessage(hText, EM_SETSEL, a, e);
        PostMessage(hText, EM_SCROLLCARET, 0, 0);
    #else
        SetWindowText(hText, text);
    #endif
        return;
    }

    bp->str = text;
    invalidate_button(bp);
    if (bp->typ == BN_CHK && 0 == (bp->f & BN_CEN)) {
        fix_button (bp);
        invalidate_button(bp);
    }
}

int get_button_text(struct dlg *dlg, int id, char *text, int bufsize)
{
    struct button *bp = getbutton(dlg, id);
    if (NULL == bp)
        return 0;

    if (bp->typ == BN_EDT) {
        return GetWindowText((HWND)bp->data, text, bufsize);
    }
    return strlen(strcpy(text, bp->str));
}

void set_button_focus(struct dlg *dlg, int id)
{
    struct button *bp = getbutton(dlg, id);
    if (NULL == bp)
        return;
    if (bp->typ == BN_EDT) {
        HWND hText = (HWND)bp->data;
        SetFocus(hText);
        PostMessage(hText, EM_SETSEL, 0, 0);
        PostMessage(hText, EM_SCROLLCARET, 0, 0);
        return;
    }
}

/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
// write/load dialog resource (devel aid)

static int bpcmp(const void *a, const void *b)
{
    return (*(struct button **)a)->msg - (*(struct button **)b)->msg;
}

void dlg_save_config(struct dlg *dlg)
{
    FILE *fp;
    struct button *bp;
    struct button *ap[200], **app, **epp;

    char path[MAX_PATH];

    fp = fopen(set_my_path(NULL, path, "dlgitems.rc"), "wt");
    if (NULL == fp) return;

    for (app = ap, bp = dlg->bn_ptr; bp; bp = bp->next) {
        *app++ = bp;
    }

    epp = app, app = ap;
    qsort(ap, epp - app, sizeof ap[0], bpcmp);

    while (app < epp) {
        const char *idstr = "UNKNOWN";
        int id, f, i;
        char flags[100];
        char namestr[100];
        const char *typstr;

        bp = *app;
        id = bp->msg;

        if (id > FIRST_ITEM && id < LAST_ITEM)
            idstr = dlg_item_strings[id - FIRST_ITEM - 1];

        typstr = dlg_item_types[bp->typ];
        sprintf(namestr, "\"%s\"", (bp->f & BN_BUF)?"":bp->str);

        f = bp->f>>4, i = 0, flags[0] = 0;
        if (0 == f)
            strcpy(flags, "0");
        else
            while (f) {
                if (f & 1) {
                    if (flags[0]) strcat(flags, "|");
                    strcat(flags, dlg_item_flags[i]);
                }
                ++i; f>>=1;
            }

        if (app != ap && ((bp->f & BN_GRP) || bp->typ == BN_RECT))
            fprintf(fp, "\n");

        fprintf(fp, "{ %-16s, %-9s, %-9s, %3d, %3d, %3d, %3d, %s },\n",
            namestr, typstr, idstr,
                bp->x0,
                bp->y0,
                bp->w0,
                bp->h0,
                flags);
        ++app;
    }
    fclose(fp);
}

#define IS_SPC(c) ((unsigned char)(c) <= 32)
static int tokenize_string(char *buffer, char **pp, const char *src, int n, const char *delims)
{
    const char *s = src; char *d = buffer, *a; int i = 0, r = 0;
    while (i < n) {
        a = d;
        while (*s && (IS_SPC(*s) || strchr(delims, *s)))
            ++s;
        while (*s && NULL == strchr(delims, *s)) {
            if (*s == '\"') {
                ++s;
                while (*s && *s++ != '\"')
                    *d++ = s[-1];
            }
            else
                *d++ = *s++;
        }
        while (d > a && IS_SPC(d[-1]))
            --d;
        *d++ = 0;
        *pp++ = a;
        ++i;
        if (*a) r = i;
        if (*s) ++s;
    }
    return r;
}

void dlg_load_config(struct dlg *dlg)
{
    FILE *fp;
    struct button *bp;
    char path[MAX_PATH];
    char line[200];

    fp = fopen(set_my_path(NULL, path, "dlgitems.rc"), "rt");
    if (NULL == fp) return;

    while (fgets(line, sizeof line, fp))
    {
        char buffer[200], *pp[16];
        int id, r;
        int x, y, w, h;
        char *idstr;

        if (line[0] != '{')
            continue;

        r = tokenize_string(buffer, pp, line, 10, ",{} ");
        if (8 != r) {
            //dbg_printf("%d <%s> <%s> <%s> <%s>", r, pp[0], pp[1], pp[2], pp[3]);
            continue;
        }

        idstr = pp[2];
        x = atoi(pp[3]);
        y = atoi(pp[4]);
        w = atoi(pp[5]);
        h = atoi(pp[6]);

        id = get_string_index(idstr, dlg_item_strings);
        if (-1 == id)
            continue;
        id += FIRST_ITEM+1;
        bp = getbutton(dlg, id);
        if (NULL == bp)
            continue;

        bp->x0=x,
        bp->y0=y,
        bp->w0=w,
        bp->h0=h;

        fix_button(bp);
        set_button_place(bp);
    }
    fclose(fp);
}

/*----------------------------------------------------------------------------*/

