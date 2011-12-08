/*
 ============================================================================

  This file is part of the bbLeanSkin source code.
  Copyright © 2003 grischka (grischka@users.sourceforge.net)

  bbLeanSkin is a plugin for Blackbox for Windows

  http://bb4win.sourceforge.net/bblean
  http://bb4win.sourceforge.net/


  bbLeanSkin is free software, released under the GNU General Public License
  (GPL version 2 or later) For details see:

    http://www.fsf.org/licenses/gpl.html

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
  for more details.

 ============================================================================
*/

#include "BBApi.h"
#include "engine\hookinfo.h"

#ifdef __GNUC__
  #define imax(a,b) ((a) >? (b))
  #define imin(a,b) ((a) <? (b))
#else
  #define imax(a,b) ((a) > (b) ? (a) : (b))
  #define imin(a,b) ((a) < (b) ? (a) : (b))
#endif
//============================================================================
// info

const char szVersion     [] = "bbLeanSkin 1.16";
const char szAppName     [] = "bbLeanSkin";
const char szInfoVersion [] = "1.16";
const char szInfoAuthor  [] = "grischka";
const char szInfoRelDate [] = "2005-05-02";
const char szInfoLink    [] = "http://bb4win.sourceforge.net/bblean";
const char szInfoEmail   [] = "grischka@users.sourceforge.net";

void about_box(void)
{
    char buff[256];
    sprintf(buff, "%s - © 2003-2005 %s\n", szVersion, szInfoEmail);
    MessageBox(NULL, buff, szAppName, MB_OK|MB_TOPMOST|MB_SETFOREGROUND);
}

//============================================================================
// exported names

#if 0
extern "C"
{
    DLL_EXPORT void startEngine();
    DLL_EXPORT void stopEngine();
    DLL_EXPORT void reconfigureEngine(void);
    DLL_EXPORT const char* engineInfo(int field);
    DLL_EXPORT void setEngineOption(UINT id);
};
#endif

//============================================================================

// blackbox and logwindow stuff
    HINSTANCE hInstance;
    HWND BBhwnd;
    int BBVersion;
    HWND m_hwnd;
    HWND hwndLog;
    bool is_plugin;
    bool enableLog;

// settings
    bool applyToOpen;
    bool adjustCaptionHeight;
    char rcpath[MAX_PATH];

// additional windows options
    char windows_menu_fontFace[120];
    int windows_menu_fontHeight;
    int ScrollbarSize;
    int MenuHeight;
    bool setTTColor;

// skin info passed via shared mem
    UINT bbSkinMsg;
    SkinStruct mSkin;

// for the shared memory
    HANDLE hMapObject;
    SkinStruct *lpvMem;

// skinner dll
    bool engine_running;

// forward declaration
    void refreshStyle(void);

    void startEngine();
    void stopEngine();
    void reconfigureEngine(void);
    const char* engineInfo(int field);
    void setEngineOption(UINT id);

    void free_dll(void);
    bool load_dll(void);

//============================================================================
// plugin interface

#ifndef INCLUDE_STYLEREADER
int beginPlugin(HINSTANCE hMainInstance)
{
    if (is_plugin || engine_running)
    {
       MessageBox(NULL, "Dont load me twice!", szAppName, MB_OK|MB_TOPMOST|MB_SETFOREGROUND);
       return 1;
    }
    is_plugin = true;
    startEngine();
    return 0;
}

void endPlugin(HINSTANCE hMainInstance)
{
    stopEngine();
    is_plugin = false;
}

LPCSTR pluginInfo(int field)
{
    switch (field)
    {
        default:
        case 0: return szVersion;
        case 1: return szAppName;
        case 2: return szInfoVersion;
        case 3: return szInfoAuthor;
        case 4: return szInfoRelDate;
        case 5: return szInfoLink;
        case 6: return szInfoEmail;
    }
}
#endif

LPCSTR engineInfo(int field)
{   
    return pluginInfo(field);
}

//============================================================================
// utilities

char *set_my_path(char *path, char *fname)
{
    int nLen = GetModuleFileName(hInstance, path, MAX_PATH);
    while (nLen && path[nLen-1] != '\\') nLen--;
    strcpy(path+nLen, fname);
    return path;
}

int get_string_index (const char *key, const char **string_list)
{
    for (int i=0; *string_list; i++, string_list++)
        if (0==stricmp(key, *string_list)) return i;
    return -1;
}

void write_log(char *s)
{
    char buffer[4096];
    sprintf(buffer, "%s\r\n", s);
    int l1 = GetWindowTextLength(hwndLog);
    SendMessage(hwndLog, EM_SETSEL, l1, l1);
    SendMessage(hwndLog, EM_REPLACESEL, false, (LPARAM)buffer);
}

void dbg_printf (const char *fmt, ...)
{
    char buffer[256]; va_list arg;
    va_start(arg, fmt);
    vsprintf (buffer, fmt, arg);
    OutputDebugString(buffer);
}

// int imax(int a, int b) {
//     return a>b?a:b;
// }
// 
// int imin(int a, int b) {
//     return a<b?a:b;
// }
// 
// int iminmax(int a, int b, int c) {
//     if (a<b) a=b;
//     if (a>c) a=c;
//     return a;
// }

int get_fontheight(HFONT hFont)
{
    TEXTMETRIC TXM;
    HDC hdc = CreateCompatibleDC(NULL);
    HGDIOBJ other = SelectObject(hdc, hFont);
    GetTextMetrics(hdc, &TXM);
    SelectObject(hdc, other);
    DeleteDC(hdc);
    return TXM.tmHeight;// - TXM.tmInternalLeading - TXM.tmExternalLeading;
}

//============================================================================
// shared mem

void DestroySharedMem()
{
    if (hMapObject)
    {
        if (lpvMem)
        {
            // Unmap shared memory from the process's address space.
            UnmapViewOfFile(lpvMem);
            lpvMem = NULL;
        }
        // Close the process's handle to the file-mapping object.
        CloseHandle(hMapObject);
        hMapObject = NULL;
    }
}

BOOL CreateSharedMem(int size)
{
    // handle to file mapping
    hMapObject = CreateFileMapping( 
        (HANDLE)0xFFFFFFFF,     // use paging file
        NULL,                   // no security attributes
        PAGE_READWRITE,         // read/write access
        0,                      // size: high 32-bits
        size,                   // size: low 32-bits
        BBLEANSKIN_SHMEMID      // name of map object
        );

    if (hMapObject && GetLastError() != ERROR_ALREADY_EXISTS)
    {
        // Get a pointer to the file-mapped shared memory.
        lpvMem = (SkinStruct *)MapViewOfFile(
            hMapObject,     // object to map view of
            FILE_MAP_WRITE, // read/write access
            0,              // high offset:  map from
            0,              // low offset:   beginning
            0);             // default: map entire file
        if (lpvMem)
        {
            ZeroMemory(lpvMem, size);
            return TRUE;
        }
    }
    DestroySharedMem();
    return FALSE;
}

//============================================================================

//============================================================================
// exclusion chaos, just assume it works...
// reads in the exclusions.rc, and puts it into the global shared mem, which
// is created here also, because it's size depends on the exclusion strings.

bool make_exclusion_list(void)
{
    struct elist
    {
        struct elist *next;
        int flen;
        int option;
        char buff[1];
    } *p0 = NULL, **pp = &p0, *p;

    int t_len   = 0;
    int t_count = 0;
    char exclusionspath[MAX_PATH];

    // first read all strings into a list and calculate the size...
    FILE *fp = FileOpen(set_my_path(exclusionspath, "exclusions.rc"));
    if (fp) for (;;)
    {
        char *line, line_buffer[256];
        if (false == ReadNextCommand(fp, line_buffer, sizeof line_buffer))
        {
            FileClose(fp);
            break;
        }
        strlwr(line = line_buffer);
        int option = 0;
        if (0 == memicmp (line, "hook-early:", 11))
        {
            option = 1; line += 10; while (' ' == *++line);
        }

        int line_len = strlen(line);

        char *cp = strchr(line, ':');
        if (cp) *cp++ = 0;
        else *(cp = line + line_len++) = 0;
        p = (struct elist *)malloc(line_len + sizeof(struct elist));
        memcpy(p->buff, line, ++line_len);
        p->flen = cp - line;
        p->option = option;
        *(pp = &(*pp = p)->next) = NULL;
        t_len += line_len-2;
        t_count++;
    }

    t_len += sizeof(struct exclusion_info) + (t_count-1) * sizeof(struct exclusion_item);

    // ... then create the shared mem, which is supposed to succeed,...
    if (FALSE == CreateSharedMem(offset_exInfo + t_len))
        return false;

    // ... and finally copy the list into it, free items by the way.
    struct exclusion_info *pExclInfo = &lpvMem->exInfo;
    pExclInfo->size = t_len;
    pExclInfo->count = t_count;
    struct exclusion_item *ei = pExclInfo->ei;
    while (p0)
    {
        p = p0; p0 = p0->next; char *cp = ei->buff;
        ei->flen = 1+strlen(strcpy(cp, p->buff));
        ei->clen = 1+strlen(strcpy(cp+ei->flen, p->buff+p->flen));
        ei->option = p->option;
        //dbg_printf("added \"%s:%s\"", p->buff, p->p_class);
        free(p);
        ei = (struct exclusion_item *)(ei->buff + ei->flen + ei->clen);
    }
    return true;
}

void free_exclusion_list(void)
{
    DestroySharedMem();
}

// test function, writes skin info into binary file
void write_bin(void)
{
    char path[MAX_PATH];
    FILE *fp = fopen(set_my_path(path, "SkinStruct.bin"), "wb");
    int size = offset_exInfo + lpvMem->exInfo.size;
    fwrite(lpvMem, size, 1, fp);
    fclose(fp);
}

//===========================================================================

//===========================================================================
// edit control, readonly
void make_log_window(void)
{
    RECT r; GetClientRect(m_hwnd, &r);
    hwndLog = CreateWindow(
        "EDIT", NULL,
        WS_CHILD
        | WS_HSCROLL
        | WS_VSCROLL
        | WS_VISIBLE
        | ES_MULTILINE
        | ES_READONLY
        | ES_AUTOHSCROLL
        | ES_AUTOVSCROLL
        ,
        0, 0, r.right, r.bottom,
        m_hwnd,
        NULL,
        hInstance,
        NULL
        );

    SendMessage(hwndLog, WM_SETFONT, (WPARAM)GetStockObject(ANSI_FIXED_FONT), 0);
    ShowWindow(m_hwnd, SW_SHOW);
}

void delete_log_window(void)
{
    ShowWindow(m_hwnd, SW_HIDE);
    DestroyWindow(hwndLog);
    hwndLog = NULL;
}

void set_log_window(void)
{
    if (enableLog && NULL == hwndLog)
        make_log_window();

    if (false == enableLog && hwndLog)
        delete_log_window();
}

//============================================================================
// the log window, usually hidden, but when shown it contains the edit control

LRESULT CALLBACK WndProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static UINT msgs[] = { BB_RECONFIGURE, BB_REDRAWGUI, BB_BROADCAST, 0};

    switch (message)
    {

    default:
        return DefWindowProc (hwnd, message, wParam, lParam);

    case WM_CREATE:
        m_hwnd = hwnd;
        SendMessage(BBhwnd, BB_REGISTERMESSAGE, (WPARAM)hwnd, (LPARAM)msgs);
        break;

    case WM_DESTROY:
        SendMessage(BBhwnd, BB_UNREGISTERMESSAGE, (WPARAM)hwnd, (LPARAM)msgs);
        break;

    case BB_BROADCAST:
        if (0 == memicmp((LPCSTR)lParam, "@BBLeanSkin.", 12))
        {
            const char *msg = (LPCSTR)lParam + 12;
            if (0 == stricmp(msg, "About"))
                about_box();
            else
            if (0 == stricmp(msg, "toggleLog"))
                goto toggle_log;
            else
            if (0 == stricmp(msg, "toggleSkin"))
            {
                if (engine_running)
                {
                    write_log("\r\n\t---- stopping engine ----\r\n");
                    PostMessage(hwnd, bbSkinMsg, MSGID_UNLOAD, 0);
                    PostMessage(hwnd, BB_QUIT, 0, 0);
                }
                else
                {
                    write_log("\r\n\t---- starting engine ----\r\n");
                    startEngine();
                }
            }
        }
        break;

    case BB_QUIT:
        stopEngine();
        break;

    case BB_RECONFIGURE:
        if (is_plugin) // i.e. not loaded by BBWinSkin
            reconfigureEngine();
        break;

    toggle_log:
        WriteBool(rcpath, "bbleanskin.option.enableLog:", false == enableLog);
        reconfigureEngine();
        break;

    //-------------------------------------------------------------
    // used in combination with bbstylemaker to update the skin info
    // and optionally force active or button pressed state.

    case BB_REDRAWGUI:
        if (BBRG_WINDOW & wParam)
        {
            if (wParam & BBRG_STICKY)
            {   // and to transfer the is_sticky info from bb.
                PostMessage((HWND)lParam, bbSkinMsg, MSGID_BB_SETSTICKY, 0 != (wParam & BBRG_FOCUS));
                break;
            }

            static bool prev_opt;
            int opt = 0;
            if (prev_opt)               opt = MSGID_BBSM_RESET;
            if (wParam & BBRG_FOCUS)    opt = MSGID_BBSM_SETACTIVE;
            if (wParam & BBRG_PRESSED)  opt = MSGID_BBSM_SETPRESSED;
            prev_opt = opt >= MSGID_BBSM_SETACTIVE;

            if (opt) setEngineOption(opt);
            refreshStyle();
        }
        break;

    //-------------------------------------------------------------
    // Log string sent by the engine dll

    case WM_COPYDATA:
    {
        if (201 == ((PCOPYDATASTRUCT)lParam)->dwData)
        {
            write_log((char*)((COPYDATASTRUCT*)lParam)->lpData);
            return TRUE;
        }
        break;
    }

    //-------------------------------------------------------------
    // things for the Log EDIT control

    case WM_SETFOCUS:
        if (hwndLog) SetFocus(hwndLog);
        break;

    case WM_SIZE:
        if (hwndLog) MoveWindow(hwndLog, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
        break;

    case WM_CLOSE:
        if (hwndLog) goto toggle_log;
        break;
    }
    return 0 ;
}

//============================================================================

extern "C" BOOL WINAPI DllMain(HINSTANCE hi, DWORD reason, LPVOID reserved)
{
    if (reason==DLL_PROCESS_ATTACH)
    {
        hInstance = hi; WNDCLASS wc; RECT dt;

        if (BBhwnd)
        {
            MessageBox(BBhwnd, "Dont load me twice!", szAppName, MB_OK|MB_SETFOREGROUND);
            return FALSE;
        }

        const char *bbv = GetBBVersion();
        if (0 == memicmp(bbv, "bblean", 6)) BBVersion = 2;
        else
        if (0 == memicmp(bbv, "bb", 2)) BBVersion = 1;
        else BBVersion = 0;

        BBhwnd = GetBBWnd();
        set_my_path(rcpath, "bbLeanSkin.rc");

        ZeroMemory(&wc, sizeof(wc));
        wc.lpszClassName = szAppName;
        wc.hInstance = hInstance;     
        wc.lpfnWndProc = WndProc;

        if (NULL == GetSettingPtr(SN_WINUNFOCUS_TITLE))
            return FALSE;

        if (FindWindow(wc.lpszClassName, NULL) || FALSE == RegisterClass(&wc))
            return FALSE;

        // center the window
        SystemParametersInfo(SPI_GETWORKAREA, 0, &dt, 0);
        int width = 480;
        int height = 300;
        int xleft = (dt.left+dt.right-width)/2;
        int ytop = (dt.top+dt.bottom-height)/2;

        CreateWindow(
            wc.lpszClassName,
            "bbLeanSkin Log",
            //WS_OVERLAPPEDWINDOW,
            WS_POPUP|WS_CAPTION|WS_SIZEBOX|WS_SYSMENU|WS_MAXIMIZEBOX|WS_MINIMIZEBOX,
            xleft, ytop, width, height,
            NULL,
            NULL,
            wc.hInstance,
            NULL
            );
    }
    else
    if (reason==DLL_PROCESS_DETACH)
    {
        stopEngine();
        DestroyWindow(m_hwnd);
        UnregisterClass(szAppName, hInstance);
    }
    return TRUE;
}

//===========================================================================

//===========================================================================

void read_buttons(void)
{
    char path[MAX_PATH];
    HBITMAP hbmp = (HBITMAP)LoadImage(
        NULL, set_my_path(path, "buttons.bmp"),
        IMAGE_BITMAP,
        0,
        0,
        LR_LOADFROMFILE// | LR_CREATEDIBSECTION
        );

    if (NULL == hbmp)
    {
        static unsigned char default_buttons[6*2][BUTTON_MAP_SIZE] = {
        { 0x00, 0x8C, 0xB9, 0xE3, 0x83, 0x83, 0x8F, 0x3B, 0x63, 0x00, 0x00 },
        { 0x00, 0x8C, 0xB9, 0xE3, 0x83, 0x83, 0x8F, 0x3B, 0x63, 0x00, 0x00 },
        { 0x00, 0xFC, 0xF9, 0x13, 0x24, 0x48, 0x90, 0x20, 0x7F, 0x00, 0x00 },
        { 0xF8, 0xF1, 0x23, 0xFC, 0xF9, 0x33, 0x7C, 0x88, 0x10, 0x3F, 0x00 },
        { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, 0x7F, 0x00, 0x00 },
        { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, 0x7F, 0x00, 0x00 },
        { 0x00, 0x1C, 0x38, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
        { 0x00, 0x1C, 0x38, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
        { 0x00, 0x20, 0xE0, 0xE0, 0xE3, 0x0F, 0x07, 0x0E, 0x1C, 0x00, 0x00 },
        { 0x00, 0x70, 0xE0, 0xC0, 0xE1, 0x8F, 0x0F, 0x0E, 0x08, 0x00, 0x00 },
        { 0x00, 0x0C, 0x18, 0x30, 0xE0, 0xC7, 0x80, 0x01, 0x03, 0x00, 0x00 },
        { 0x00, 0xFC, 0xF9, 0xF3, 0x07, 0x01, 0x02, 0x04, 0x08, 0x00, 0x00 },
        };
        memcpy(&mSkin.button_bmp, default_buttons, sizeof default_buttons);
        return;
    }

    HDC hdc = CreateCompatibleDC(NULL);
    HGDIOBJ other = SelectObject(hdc, hbmp);
    struct button_bmp *bp = mSkin.button_bmp;
    int b, state, y, x, n;
    for (b = 0; b < 6; ++b, ++bp)
        for (state = 0; state < 2; ++state)
        {
            for (n = y = 0; y < BUTTON_SIZE; ++y)
                for (x = 0; x < BUTTON_SIZE; ++x, ++n)
                {
                    COLORREF c = GetPixel(hdc, b*(1+BUTTON_SIZE)+1+x, state*(1+BUTTON_SIZE)+1+y);
                    if (0 == c) bp->data[state][n/8] |= 1 << n%8;
                }
#if 0
            unsigned char *u = (unsigned char*)bp->data[state];
            dbg_printf("       { "
                "0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X },",
                u[0], u[1], u[2], u[3], u[4], u[5], u[6], u[7], u[8], u[9], u[10]);
#endif
        }
    DeleteObject(SelectObject(hdc, other));
    DeleteDC(hdc);
}

//===========================================================================
// get settings

void readSettings(void)
{
    int i;

    ZeroMemory(&mSkin, offset_hooks);

    //-----------------------
    // titlebar click actions
    for (i = 0; i < 12; i++)
    {
        static const char *modifiers[] = { "", "Shift", "Ctrl" };
        static const char *buttons[]  = { "Dbl", "Right", "Mid", "Left" };
        static const char *button_ids[6+2+1] = {
            "Close", "Maximize", "Minimize",
            "Rollup", "AlwaysOnTop", "Pin",
            "Lower", "MinimizeToTray",
            NULL
        };

        char rcstring[80];
        sprintf(rcstring, "bbleanskin.titlebar.%s%sClick:", modifiers[i%3], buttons[i/3]);
        const char *p = ReadString(rcpath, rcstring, "");
        mSkin.captionClicks.Dbl[i] = get_string_index(p, button_ids);
    }

    strncpy(mSkin.button_string, ReadString(rcpath, "bbleanskin.titlebar.buttons:", "400321"), 6);

    // button pics
    read_buttons();

    //-----------------
    // other settings

    mSkin.nixShadeStyle = ReadBool(rcpath, "bbleanskin.option.nixShadeStyle:", true);
    mSkin.snapWindows = ReadBool(rcpath, "bbleanskin.option.snapWindows:", true);
    applyToOpen = ReadBool(rcpath, "bbleanskin.option.applyToOpen:", true);
    adjustCaptionHeight = ReadBool(rcpath, "bbleanskin.option.adjustCaptionHeight:", false);
    enableLog = ReadBool(rcpath, "bbleanskin.option.enableLog:", false);

    //mSkin.drawLocked = ReadBool(rcpath, "bbleanskin.option.drawLocked:", false);
    mSkin.imageDither = ReadBool(bbrcPath(), "session.imageDither:", false);

    //----------------------------
    // windows appearance settings
    MenuHeight = ReadInt(rcpath, "bbleanskin.windows.menu.height:", -1);
    strcpy(windows_menu_fontFace, ReadString(rcpath, "bbleanskin.windows.menu.Font:", ""));
    windows_menu_fontHeight = ReadInt(rcpath, "bbleanskin.windows.menu.fontHeight:", 8);
    ScrollbarSize = ReadInt(rcpath, "bbleanskin.windows.scrollbarsize:", -1);
    setTTColor = ReadBool(rcpath, "bbleanskin.windows.setToolTipColor:", false);

    mSkin.BBhwnd = BBhwnd;
    mSkin.BBVersion = BBVersion;
    mSkin.loghwnd = m_hwnd;
    mSkin.enableLog = enableLog;
}

//===========================================================================
// copy style into Skin

void readStyle(void)
{
    static const char settings_id_array[] =
    {
        SN_WINFOCUS_TITLE         ,
        SN_WINFOCUS_LABEL         ,
        SN_WINFOCUS_HANDLE        ,
        SN_WINFOCUS_GRIP          ,
        SN_WINFOCUS_BUTTON        ,
        SN_WINFOCUS_BUTTONP       ,
        SN_WINFOCUS_BUTTONC       ,
        SN_WINFOCUS_BUTTONCP      ,

        SN_WINUNFOCUS_TITLE       ,
        SN_WINUNFOCUS_LABEL       ,
        SN_WINUNFOCUS_HANDLE      ,
        SN_WINUNFOCUS_GRIP        ,
        SN_WINUNFOCUS_BUTTON      ,
        SN_WINUNFOCUS_BUTTONC     ,
    };

    GetSettingPtr(0); // init style reader

    void *p = GetSettingPtr(SN_WINFOCUS_LABEL);
    int nVersion = 0;

    if (p)
    {
        memcpy(&mSkin.windowFont, &((StyleItem*)p)->FontHeight, sizeof(mSkin.windowFont));
        nVersion = ((StyleItem*)p)->nVersion;
    }

    if (nVersion < 2)
    {
        mSkin.borderWidth = *(int*)GetSettingPtr(SN_BORDERWIDTH);
        mSkin.focus_borderColor =
        mSkin.unfocus_borderColor = *(COLORREF*)GetSettingPtr(SN_BORDERCOLOR);
    }
    else
    {
        mSkin.borderWidth = *(int*)GetSettingPtr(SN_FRAMEWIDTH);
        mSkin.focus_borderColor = *(COLORREF*)GetSettingPtr(SN_WINFOCUS_FRAME_COLOR);
        mSkin.unfocus_borderColor = *(COLORREF*)GetSettingPtr(SN_WINUNFOCUS_FRAME_COLOR);
    }

    mSkin.handleHeight = *(int*)GetSettingPtr(SN_HANDLEHEIGHT);
    int bevelWidth = *(int*)GetSettingPtr(SN_BEVELWIDTH);

    GradientItem * pG; const char *s = settings_id_array;
    for (pG = &mSkin.windowTitleFocus; pG <= &mSkin.windowButtonCloseUnfocus; ++pG, ++s)
    {
        StyleItem *pSI = (StyleItem*)GetSettingPtr(*s);
        if (NULL == pSI) continue;

        memcpy(pG, pSI, sizeof *pG);
        pG->borderWidth    = pSI->borderWidth    ;
        pG->borderColor    = pSI->borderColor    ;
        pG->marginWidth    = pSI->marginWidth    ;
        pG->validated      = pSI->validated      ;
        pG->ShadowColor    = pSI->ShadowColor    ;
        pG->OutlineColor   = pSI->OutlineColor   ;
        pG->ColorSplitTo   = pSI->ColorSplitTo   ;
        pG->ColorToSplitTo = pSI->ColorToSplitTo ;

        if (nVersion < 2
         && (*s == SN_WINFOCUS_TITLE
          || *s == SN_WINFOCUS_HANDLE
          || *s == SN_WINFOCUS_GRIP
          || *s == SN_WINUNFOCUS_TITLE
          || *s == SN_WINUNFOCUS_HANDLE
          || *s == SN_WINUNFOCUS_GRIP
          ))
        {
            pG->borderWidth = mSkin.borderWidth;
            if (*s == SN_WINFOCUS_TITLE)
                pG->marginWidth = bevelWidth;

        }

        if (false == (pG->validated & VALID_BORDERCOLOR))
        {
            if (*s >= SN_WINUNFOCUS_TITLE)
                pG->borderColor = mSkin.unfocus_borderColor  ;
            else
                pG->borderColor = mSkin.focus_borderColor  ;
        }
    }

    //----------------------------------------------------
    // calculate dependant sizes

    int labelH, buttonH;

    bool newMetrics = (bool)GetSettingPtr(SN_NEWMETRICS);

    if (newMetrics || (mSkin.windowLabelFocus.validated & VALID_MARGIN))
    {
        HFONT hf = CreateStyleFont((StyleItem*)GetSettingPtr(SN_WINFOCUS_LABEL));
        int lfh = get_fontheight(hf);
        DeleteObject(hf);

        if (mSkin.windowLabelFocus.validated & VALID_MARGIN)
            labelH = lfh + 2*mSkin.windowLabelFocus.marginWidth;
        else
            labelH = imax(10, lfh) + 2*2;
    }
    else
    {
        labelH = imax(8, mSkin.windowFont.Height) + 2;
    }

    labelH |= 1;

    if (mSkin.windowButtonFocus.validated & VALID_MARGIN)
    {
        buttonH = 9 + 2 * mSkin.windowButtonFocus.marginWidth;
    }
    else
    {
        buttonH = labelH - 2;
    }

    int tbheight = imax(labelH, buttonH) + 2 *
        (mSkin.windowTitleFocus.borderWidth + mSkin.windowTitleFocus.marginWidth);

    mSkin.buttonSize = buttonH;
    mSkin.labelHeight = labelH;
    mSkin.isNewStyle = newMetrics;

    // titlebar metrics are similar to the toolbar
    mSkin.ncTop = tbheight;

    int bottom_border = imax(mSkin.windowHandleFocus.borderWidth, mSkin.windowGripFocus.borderWidth);
    int top_border = mSkin.windowTitleFocus.borderWidth;

    mSkin.ncBottom =
        mSkin.handleHeight // with no handle, there is only one border
        ? mSkin.handleHeight + 2 * bottom_border
        : mSkin.borderWidth
        ;

    mSkin.rollupHeight = mSkin.ncTop;
    if (false == mSkin.nixShadeStyle && mSkin.handleHeight)
        mSkin.rollupHeight += mSkin.ncBottom - top_border;

    mSkin.gripWidth = 2*mSkin.buttonSize + mSkin.borderWidth;

    mSkin.labelMargin = (mSkin.ncTop - mSkin.labelHeight) / 2;
    mSkin.buttonMargin = (mSkin.ncTop - mSkin.buttonSize) / 2;
    mSkin.buttonSpace = mSkin.buttonMargin - mSkin.borderWidth;
    //mSkin.buttonSpace = imax(1, mSkin.windowTitleFocus.marginWidth);

}

//===========================================================================
// adjust the global window metrics

static NONCLIENTMETRICS ncm_save;
static NONCLIENTMETRICS ncm_prev;

void setmetrics(void)
{
    //---------------------------
    // copy default settings

    NONCLIENTMETRICS ncm_now = ncm_save;

    //---------------------------
    // set the sizing border to at least the style border.
    // ncm_now.iBorderWidth is the sizing_border - SM_CYFIXEDFRAME,
    // but at least 1, while SM_CYFIXEDFRAME is seemingly usually 3.
    // So the sizing border usually is at least 4.

    int cyFF = GetSystemMetrics(SM_CYFIXEDFRAME);
    ncm_now.iBorderWidth = imax(1, mSkin.borderWidth - cyFF);

    //---------------------------
    // set the caption heights. The total height where the caption
    // can be drawn on, is
    //      ncm_now.iCaptionHeight + ncm_now.iBorderWidth + cyFF;
    // If this is more than "mSkin.ncTop", it will be hidden by
    // the region setting.

    if (adjustCaptionHeight)
    {
        int n = mSkin.ncTop - (ncm_now.iBorderWidth + cyFF);
        ncm_now.iCaptionHeight = ncm_now.iSmCaptionHeight = n;
    }

    //---------------------------
    // other settings
    if (-1 != MenuHeight)
        ncm_now.iMenuHeight = MenuHeight;

    if (-1 != ScrollbarSize)
        ncm_now.iScrollWidth = ncm_now.iScrollHeight = ScrollbarSize;

    //---------------------------
    // set menu/status/message font

    if (windows_menu_fontFace[0])
    {
        LOGFONT F;
        ZeroMemory(&F, sizeof F);
        F.lfHeight = -windows_menu_fontHeight;
        //F.lfWidth = 0;
        //F.lfEscapement = 0;
        //F.lfOrientation = 0;
        F.lfWeight = FW_NORMAL;
        //F.lfItalic = 0;
        //F.lfUnderline = 0;
        //F.lfStrikeOut = 0;
        F.lfCharSet = DEFAULT_CHARSET;
        F.lfOutPrecision = OUT_DEFAULT_PRECIS;
        F.lfClipPrecision = CLIP_DEFAULT_PRECIS;
        F.lfQuality = DEFAULT_QUALITY;
        F.lfPitchAndFamily = DEFAULT_PITCH|FF_DONTCARE;

        strcpy(F.lfFaceName, windows_menu_fontFace);

        ncm_now.lfMenuFont = F;
        ncm_now.lfStatusFont = F;
        ncm_now.lfMessageFont = F;

        //ncm_now.lfCaptionFont = ncm_now.lfSmCaptionFont = F;
    }

    //---------------------------
    // set system parameters on changes
    if (memcmp (&ncm_now, &ncm_prev, sizeof(NONCLIENTMETRICS)))
    {
        ncm_prev = ncm_now;
        SystemParametersInfo(
            SPI_SETNONCLIENTMETRICS,
            sizeof(NONCLIENTMETRICS),
            &ncm_now,
            SPIF_SENDCHANGE
            );
        //dbg_printf("set SystemParametersInfo");
    }

    mSkin.cxSizeFrame = GetSystemMetrics(SM_CYSIZEFRAME);
    mSkin.cxFixedFrame = GetSystemMetrics(SM_CYFIXEDFRAME);
    mSkin.cyCaption = GetSystemMetrics(SM_CYCAPTION);
    mSkin.cySmCaption = GetSystemMetrics(SM_CYSMCAPTION);

    mSkin.gripWidth = 2*ncm_now.iScrollWidth + mSkin.borderWidth;


    //---------------------------
    // set skin info in shared memory
    memcpy(lpvMem, &mSkin, offset_hooks);
}

//===========================================================================
// SysColor Stuff

enum { SAVE_3DC, RESTORE_3DC, APPLY_3DC };

COLORREF mixcolors(COLORREF c1, COLORREF c2)
{
    return RGB(
        (GetRValue(c1)+GetRValue(c2))/2,
        (GetGValue(c1)+GetGValue(c2))/2,
        (GetBValue(c1)+GetBValue(c2))/2);
}

void setTTC(int f)
{
    const int NCOLORS = 2;
    static int C_ID[NCOLORS] =
    {
        // tooltips bk + txt
        COLOR_INFOBK,
        COLOR_INFOTEXT
    };
    static COLORREF C_SAVE[NCOLORS];
    static bool changed;

    if (SAVE_3DC == f)
    {
        int n = 0;
        do C_SAVE[n] = GetSysColor(C_ID[n]); while (++n<NCOLORS);
        changed = false;
        return;
    }

    if (RESTORE_3DC == f || false == setTTColor)
    {
        if (changed)
        {
            SetSysColors(NCOLORS, C_ID, C_SAVE);
            changed = false;
        }
        return;
    }

    if (APPLY_3DC == f)
    {
        COLORREF C_CR[NCOLORS];
        GradientItem *S1, *S2;
        S1 = S2 = &mSkin.windowLabelFocus;

        if (S1->parentRelative)
            S1 = &mSkin.windowTitleFocus;

        if (B_SOLID == S1->type)
            C_CR[0] = S1->Color;
        else
            C_CR[0] = mixcolors(S1->Color, S1->ColorTo);

        C_CR[1] = S2->TextColor;

        SetSysColors(NCOLORS, C_ID, C_CR);
        changed = true;
    }
}

//===========================================================================

//===========================================================================
// load the engine

HINSTANCE hEngineInst;
int (*EntryFunc)(int mode, SkinStruct *pS);

void free_dll(void)
{
    if (hEngineInst)
        FreeLibrary(hEngineInst), hEngineInst = NULL;
}

bool load_dll(void)
{
    if (NULL == hEngineInst)
    {
        char engine_path[MAX_PATH];
        const char *error = NULL;

        hEngineInst = LoadLibrary(set_my_path(engine_path, BBLEANSKIN_ENGINEDLL));
        *(FARPROC*)&EntryFunc = GetProcAddress(hEngineInst, "EntryFunc");

        if (NULL == EntryFunc)
            error = "Could not load: " BBLEANSKIN_ENGINEDLL;
        else
        if (ENGINE_THISVERSION != EntryFunc(ENGINE_GETVERSION, NULL))
            error = "Wrong version: " BBLEANSKIN_ENGINEDLL;

        if (error)
        {
            free_dll();
            MessageBox(NULL, error, szAppName,
                MB_OK | MB_ICONERROR | MB_SETFOREGROUND | MB_TOPMOST);
            return false;
        }
    }
    return true;
}

//===========================================================================
// pass messages to windows

static BOOL CALLBACK SkinEnumProc(HWND hwnd, LPARAM lParam)
{
    void *pInfo = GetProp(hwnd, BBLEANSKIN_INFOPROP);
    if (MSGID_LOAD == lParam)
    {
        if (NULL == pInfo
            && WS_CAPTION == (WS_CAPTION & GetWindowLong(hwnd, GWL_STYLE)))
        {
            //dbg_printf("post %08x", hwnd);
            PostMessage(hwnd, bbSkinMsg, lParam, 0);
        }
    }
    else
    if (pInfo)
    {
        SendMessage(hwnd, bbSkinMsg, lParam, 0);
    }
    return TRUE;
}

static BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
    EnumChildWindows(hwnd, (WNDENUMPROC)SkinEnumProc, lParam);
    SkinEnumProc(hwnd, lParam);
    return TRUE;
}

static void sendToAll(UINT msg_id)
{
    EnumWindows((WNDENUMPROC)EnumWindowsProc, msg_id);
}

//===========================================================================
// engine interface

void startEngine(void)
{
    //dbg_printf("Started skinner engine");
    readSettings();
    set_log_window();

    if (engine_running)
        return;
    if (false == make_exclusion_list())
        return;

    if (false == load_dll())
    {
        free_exclusion_list();
        return;
    }

    bbSkinMsg = RegisterWindowMessage(BBLEANSKIN_WINDOWMSG);

    // save sys-colors
    setTTC(SAVE_3DC);

    // save the normal SystemParameter settings, window metrics, etc.
    ncm_save.cbSize = sizeof(NONCLIENTMETRICS);
    SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof ncm_save, &ncm_save, 0);
    ZeroMemory(&ncm_prev, sizeof(NONCLIENTMETRICS));

    // read styles for skin
    readStyle();
    // possibly set sys-colors
    setTTC(APPLY_3DC);
    // set the system wide window metrics according to style metrics
    setmetrics();
    // set the hook
    EntryFunc(ENGINE_SETHOOKS, lpvMem);
    engine_running = true;

    // now apply to open windows
    if (applyToOpen) sendToAll(MSGID_LOAD);

    //write_bin();
}

void refreshStyle(void)
{
    if (false == engine_running) return;
    readStyle();
    setmetrics();
    sendToAll(MSGID_REFRESH);
}

void reconfigureEngine(void)
{
    //dbg_printf("Reconfigured skinner engine");
    readSettings();
    set_log_window();
    if (false == engine_running) return;
    refreshStyle();
    setTTC(APPLY_3DC);
}

void stopEngine(void)
{
    //dbg_printf("Stopped skinner engine");
    if (false == engine_running) return;
    sendToAll(MSGID_UNLOAD);
    EntryFunc(ENGINE_UNSETHOOKS, lpvMem);
    free_exclusion_list();
    free_dll();
    // restore the normal SystemParameter settings, window metrics, etc.
    SystemParametersInfo(SPI_SETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &ncm_save, SPIF_SENDCHANGE);
    // restore sys-colors
    setTTC(RESTORE_3DC);
    engine_running = false;
}

void setEngineOption(UINT id)
{
    //dbg_printf("Set Option (%d) for skinner engine", id);
    if (engine_running) sendToAll(id);
}

//===========================================================================
