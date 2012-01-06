/*
 ============================================================================

  This file is part of the bbLeanSkin source code.
  Copyright © 2003-2009 grischka (grischka@users.sourceforge.net)

  bbLeanSkin is a plugin for Blackbox for Windows

  http://bb4win.sourceforge.net/bblean
  http://bb4win.sourceforge.net/


  bbLeanSkin is free software, released under the GNU General Public License
  (GPL version 2) For details see:

    http://www.fsf.org/licenses/gpl.html

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
  for more details.

 ============================================================================
*/

#include "BBApi.h"
#include "engine/hookinfo.h"
#include "bblib.h"
#include "bbversion.h"

// info
const char szVersion     [] = "bbLeanSkin "BBLEAN_VERSION;
const char szAppName     [] = "bbLeanSkin";
const char szInfoVersion [] = BBLEAN_VERSION;
const char szInfoAuthor  [] = "grischka";
const char szInfoRelDate [] = BBLEAN_RELDATE;
const char szInfoLink    [] = "http://bb4win.sourceforge.net/bblean";
const char szInfoEmail   [] = "grischka@users.sourceforge.net";
const char szCopyright   [] = "2003-2009";

void about_box(void)
{
    char buff[256];
    sprintf(buff, "%s - © %s %s\n", szVersion, szCopyright, szInfoEmail);
    MessageBox(NULL, buff, szAppName, MB_OK|MB_TOPMOST|MB_SETFOREGROUND);
}

//============================================================================

// blackbox and logwindow stuff
    HINSTANCE hInstance;
    HWND BBhwnd;
    int BBVersion;
    int bbLeanVersion;
    HWND m_hwnd;
    HWND hwndLog;
    bool enableLog;

// settings
    bool adjustCaptionHeight;
    char rcpath[MAX_PATH];
    char buttonpath[MAX_PATH];

// additional windows options
    char windows_menu_fontFace[120];
    int windows_menu_fontHeight;
    int ScrollbarSize;
    int MenuHeight;
    bool setTTColor;
    bool sendToSwitchTo;

// skin info passed via shared mem
    UINT bbSkinMsg;
    SkinStruct mSkin;

// for the shared memory
    HANDLE hMapObject;
    SkinStruct *lpvMem;

// skinner dll
    bool engine_running;
    char bbsm_option;
    bool sysmenu_timerset;

// forward declaration
    void refreshStyle(void);
    void startEngine();
    void stopEngine();
    void reconfigureEngine(void);
    const char* engineInfo(int field);

    bool init_app(void);
    void exit_app(void);
    bool load_dll(void);
    void free_dll(void);

//============================================================================
// plugin interface

DLL_EXPORT int beginPlugin(HINSTANCE hMainInstance)
{
    hInstance = hMainInstance;
    if (false == init_app())
        return 1;
    startEngine();
    return 0;
}

DLL_EXPORT void endPlugin(HINSTANCE hMainInstance)
{
    stopEngine();
    exit_app();
}

DLL_EXPORT LPCSTR pluginInfo(int field)
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

//===========================================================================

bool ShowSysmenu(HWND TaskWnd, HWND BarWnd, RECT *pRect, const char *plugin_broam);
bool exec_sysmenu_command(const char *temp, bool sendToSwitchTo);
bool sysmenu_exists();

#if 0
bool exec_sysmenu_command(const char *temp, bool sendToSwitchTo)
{ return false; }
bool ShowSysmenu(HWND Window, HWND Owner, RECT *pRect, const char *broam)
{ return false; }
bool sysmenu_exists()
{ return false; }
#endif

//============================================================================
// utilities

char *find_config_file(char *rcpath, const char *file)
{
    bool (*pFindRCFile)(LPSTR rcpath, LPCSTR rcfile, HINSTANCE plugin_instance);
    *(FARPROC*)&pFindRCFile = GetProcAddress(GetModuleHandle(NULL), "FindRCFile");
    if (pFindRCFile)
        pFindRCFile(rcpath, file, hInstance);
    else
        set_my_path(hInstance, rcpath, file);
    return rcpath;
}

void send_stickyinfo_to_tasks(void)
{
    const struct tasklist *tl;
    for (tl = GetTaskListPtr(); tl; tl = tl->next)
        PostMessage(m_hwnd, BB_TASKSUPDATE, (WPARAM)tl->hwnd, TASKITEM_ADDED);
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
        (HANDLE)-1,             // use paging file
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
            memset(lpvMem, 0, size);
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
    FILE *fp = FileOpen(find_config_file(exclusionspath, "exclusions.rc"));
    if (fp) for (;;)
    {
        char *line, line_buffer[256], *cp;
        int option, line_len;
        if (false == ReadNextCommand(fp, line_buffer, sizeof line_buffer))
        {
            FileClose(fp);
            break;
        }
        strlwr(line = line_buffer);
        option = 0;
        if (0 == memicmp (line, "hook-early:", 11))
        {
            option = 1;
            line += 10;
            while (' ' == *++line);
        }

        line_len = strlen(line);
        cp = strchr(line, ':');
        if (cp)
            *cp++ = 0;
        else
            *(cp = line + line_len++) = 0;
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
        ei->flen = (unsigned char)(1+strlen(strcpy(cp, p->buff)));
        ei->clen = (unsigned char)(1+strlen(strcpy(cp+ei->flen, p->buff+p->flen)));
        ei->option = (unsigned char)p->option;
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

//===========================================================================
// test function, writes skin info into binary file
#if 0
void write_bin(void)
{
    char path[MAX_PATH];
    FILE *fp = fopen(set_my_path(hInstance, path, "skin_struct.bin"), "wb");
    int size = offset_exInfo + lpvMem->exInfo.size;
    fwrite(lpvMem, size, 1, fp);
    fclose(fp);
}
#else
#define write_bin()
#endif

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

void write_log(const char *s)
{
    char buffer[4000]; int l1;
    if (NULL == hwndLog)
        return;
    sprintf(buffer, "%s\r\n", s);
    l1 = GetWindowTextLength(hwndLog);
    SendMessage(hwndLog, EM_SETSEL, l1, l1);
    SendMessage(hwndLog, EM_REPLACESEL, false, (LPARAM)buffer);
}

bool get_param(const char **msg, const char *key)
{
    int l = strlen(key);
    if (0 != memicmp(*msg, key, l) || (unsigned char)(*msg)[l] > ' ')
        return false;
    *msg += l;
    while (' ' ==  **msg)
        ++*msg;
    return true;
}

int set_param(WPARAM wParam, LPARAM lParam, const char *p, const char *arg)
{
    int l = p - (LPCSTR)lParam;
    memcpy((char *)wParam, (LPCSTR)lParam, l);
    strcpy((char *)wParam + l, arg);
    return 0;
}

//============================================================================
// the log window, usually hidden, but when shown it contains the edit control

LRESULT CALLBACK WndProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static UINT msgs[] = {
        BB_RECONFIGURE, BB_REDRAWGUI, BB_BROADCAST,
        BB_GETBOOL, BB_TASKSUPDATE, 0};

    const char *msg;

    switch (message)
    {
    default:
        return DefWindowProc (hwnd, message, wParam, lParam);

    case WM_CREATE:
        m_hwnd = hwnd;
        SendMessage(BBhwnd, BB_REGISTERMESSAGE, (WPARAM)hwnd, (LPARAM)msgs);
        MakeSticky(hwnd);
        break;

    case WM_DESTROY:
        SendMessage(BBhwnd, BB_UNREGISTERMESSAGE, (WPARAM)hwnd, (LPARAM)msgs);
        RemoveSticky(hwnd);
        break;

    case BB_GETBOOL:
        if (0 == memicmp((LPCSTR)lParam, "@BBLeanSkin.", 12))
        {
            const char *msg = (LPCSTR)lParam + 12;
            if (0 == stricmp(msg, "toggleSkin")) {
                *(int*)wParam = engine_running;
            }
            else if (0 == stricmp(msg, "toggleLog")) {
                *(int*)wParam = NULL != hwndLog;
            }
            else if (get_param(&msg, "buttonOrder")) {
                set_param(wParam, lParam, msg, mSkin.button_string);
            }
            else if (get_param(&msg, "buttonGlyphs")) {
                set_param(wParam, lParam, msg, buttonpath);
            }
            return 1;
        }
        break;

    case BB_BROADCAST:
        if (0 != memicmp((LPCSTR)lParam, "@BBLeanSkin.", 12))
            break;

        msg = (LPCSTR)lParam + 12;

        if (0 == stricmp(msg, "About"))
        {
            about_box();
            break;
        }

        if (0 == stricmp(msg, "Reconfigure"))
        {
            reconfigureEngine();
            break;
        }

        if (0 == stricmp(msg, "toggleLog"))
        {
toggle_log:
            WriteBool(rcpath, "bbleanskin.option.enableLog:", false == enableLog);
            reconfigureEngine();
            goto menu_update;
        }

        if (0 == stricmp(msg, "toggleSkin"))
        {
            if (engine_running)
            {
                write_log("\r\n\t---- stopping engine ----\r\n");
                PostMessage(hwnd, bbSkinMsg, BBLS_UNLOAD, 0);
                PostMessage(hwnd, BB_QUIT, 0, 0);
                break;
            }
            else
            {
                write_log("\r\n\t---- starting engine ----\r\n");
                startEngine();
                goto menu_update;
            }
        }

        if (get_param(&msg, "buttonGlyphs"))
        {
            WriteString(rcpath,
                "bbleanskin.titlebar.glyphs:", get_relative_path(hInstance, msg));
            reconfigureEngine();
            goto menu_update;
        }

        if (get_param(&msg, "buttonOrder"))
        {
            WriteString(rcpath, "bbleanskin.titlebar.buttons:", msg);
            reconfigureEngine();
            goto menu_update;
        }

        if (exec_sysmenu_command(msg, sendToSwitchTo))
        {
            break;
        }

        break;

    case BB_QUIT:
        stopEngine();
menu_update:
        PostMessage(BBhwnd, BB_MENU, BB_MENU_UPDATE, 0);
        break;

    case BB_RECONFIGURE:
        reconfigureEngine();
        break;

    //-------------------------------------------------------------
    // used in combination with bbstylemaker to update the skin info
    // and optionally force active or button pressed state.

    case BB_REDRAWGUI:
        if (BBRG_WINDOW & wParam)
        {
            char opt = 0;
            if (wParam & BBRG_FOCUS) {
                opt |= 1;
                if (wParam & BBRG_PRESSED)
                    opt |= 2;
            }
            bbsm_option = opt;
            refreshStyle();
        }
        break;

    //-------------------------------------------------------------
    // Log string sent by the engine dll

    case WM_COPYDATA:
    {
        int d = ((PCOPYDATASTRUCT)lParam)->dwData;
        void *p = ((COPYDATASTRUCT*)lParam)->lpData;

        if (201 == d)
        {
            write_log((const char*)p);
            return TRUE;
        }

        if (202 == d)
        {
            struct sysmenu_info *s = (struct sysmenu_info*)p;
            if (sysmenu_timerset) {
                PostMessage(s->hwnd, WM_SYSCOMMAND, SC_CLOSE, 0);
            } else {
                if (sysmenu_exists()) {
                    PostMessage(BBhwnd, BB_HIDEMENU, 0, 0);
                } else {
                    ShowSysmenu(s->hwnd, hwnd, &s->rect, "@bbLeanSkin");
                    SetTimer(hwnd, 2, GetDoubleClickTime(), NULL);
                    sysmenu_timerset = true;
                }
            }
            return TRUE;
        }

        break;
    }

    case WM_TIMER:
        KillTimer(hwnd, wParam);
        if (2 == wParam)
            sysmenu_timerset = false;
        break;

    //-------------------------------------------------------------
    case BB_TASKSUPDATE:
        if (TASKITEM_ADDED == lParam)
        {
            HWND hwnd = (HWND)wParam;
            //bool is_sticky = 0 != SendMessage(BBhwnd, BB_WORKSPACE, BBWS_ISSTICKY, (LPARAM)hwnd);
            bool is_sticky = CheckSticky(hwnd);
            PostMessage(hwnd, bbSkinMsg, BBLS_SETSTICKY, (LPARAM)is_sticky);
        }
        break;

    //-------------------------------------------------------------
    // things for the Log EDIT control

    case WM_SETFOCUS:
        if (hwndLog)
            SetFocus(hwndLog);
        break;

    case WM_SIZE:
        if (hwndLog)
            MoveWindow(hwndLog, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
        break;

    case WM_CLOSE:
        if (hwndLog)
            goto toggle_log;
        break;
    }
    return 0 ;
}

//============================================================================
#if 0
extern "C" DLL_EXPORT Menu *pluginMenu(bool popup)
{
    char temp[MAX_PATH + 100]; int x;
    Menu *m = MakeNamedMenu(szAppName, "bbLeanSkin_Main", popup);
    MakeMenuItem(m, "Show Log", "@BBLeanSkin.ToggleLog", NULL != hwndLog);
    MakeMenuItem(m, "Enable Skin", "@BBLeanSkin.ToggleSkin", engine_running);
    MakeMenuNOP(m, NULL);

    x = sprintf(temp, "@BBCore.edit ");
    strcpy(temp+x, rcpath);
    MakeMenuItem(m, "Edit Settings", temp, false);
    find_config_file(temp+x, "exclusions.rc");
    MakeMenuItem(m, "Edit Exclusions", temp, false);
    set_my_path(hInstance, temp+x, "readme.txt");

    MakeMenuNOP(m, NULL);
    MakeMenuItem(m, "Readme", temp, false);
    MakeMenuItem(m, "About", "@BBLeanSkin.About", false);
    return m;
}
#endif

//============================================================================
bool init_app(void)
{
    WNDCLASS wc;
    RECT dt;
    const char *bbv;
    int a, b, c;

    bbv = GetBBVersion();
    if (0 == memicmp(bbv, "bblean", 6))
        BBVersion = 2;
    else
    if (0 == memicmp(bbv, "bb", 2))
        BBVersion = 1;
    else
        BBVersion = 0;

    c = 0;
    bbLeanVersion = sscanf(bbv, "bbLean %d.%d.%d", &a, &b, &c)
        >= 2 ? a*1000+b*10+c : 0;

    if (BBhwnd) {
        MessageBox(BBhwnd, "Dont load me twice!", szAppName, MB_OK|MB_SETFOREGROUND);
        return false;
    }

    BBhwnd = GetBBWnd();
    find_config_file(rcpath, "bbLeanSkin.rc");

    //dbg_printf("offset_exInfo: %x", offset_exInfo);
    //char xxx[0x480 - offset_exInfo];

    memset(&wc, 0, sizeof(wc));
    wc.lpszClassName = szAppName;
    wc.hInstance = hInstance;     
    wc.lpfnWndProc = WndProc;

    if (FindWindow(wc.lpszClassName, NULL) || FALSE == RegisterClass(&wc))
        return false;

    // center the window
    SystemParametersInfo(SPI_GETWORKAREA, 0, &dt, 0);
    int width = 480;
    int height = 300;
    int xleft = (dt.left+dt.right-width)/2;
    int ytop = (dt.top+dt.bottom-height)/2;

    CreateWindow(
        wc.lpszClassName,
        "bbLeanSkin Log",
        WS_POPUP|WS_CAPTION|WS_SIZEBOX|WS_SYSMENU|WS_MAXIMIZEBOX|WS_MINIMIZEBOX,
        xleft, ytop, width, height,
        NULL,
        NULL,
        (HINSTANCE)wc.hInstance,
        NULL
        );

    return true;
}

void exit_app(void)
{
    DestroyWindow(m_hwnd);
    UnregisterClass(szAppName, hInstance);
    BBhwnd = NULL;
}

//===========================================================================

//===========================================================================
#if 0
void write_buttons(void)
{
    int x, n, gs;
    char buffer[MAX_PATH];
    unsigned char *up;
    FILE *fp;

    up = mSkin.glyphmap;
    gs = ONE_GLYPH(up[0], up[1]);

    fp = fopen(hInstance, set_my_path(hInstance, buffer, "defbutton.txt"), "wt");
    fprintf(fp, "\t0x%02X,0x%02X,\n", up[0], up[1]);
    up += 2;

    for (x = 12; --x >= 0; ) {
        for (n = 0; n < gs; ++n, ++up)
            fprintf(fp, "%s0x%02X", n?",":"\t", *up);
        fprintf(fp, x?",\n":"\n");
    }
    fclose(fp);
}
#else
#define write_buttons()
#endif

//===========================================================================

void read_buttons(const char *buttonfile)
{
    HBITMAP hbmp = NULL;

    if (buttonfile && buttonfile[0] && 0 != stricmp(buttonfile, "default"))
    {
        hbmp = (HBITMAP)LoadImage(
            NULL,
            set_my_path(hInstance, buttonpath, buttonfile),
            IMAGE_BITMAP,
            0,
            0,
            LR_LOADFROMFILE
            );
    }

    if (hbmp)
    {
        BITMAP bm;
        HDC hdc;
        HGDIOBJ other;
        int btn, state, y, x, n, tx, ty;
        unsigned char *up;

        tx = ty = 9;
        if (GetObject(hbmp, sizeof bm, &bm))
            tx = ty = iminmax((bm.bmHeight-3)/2, 9, GLYPH_MAX_SIZE);


        hdc = CreateCompatibleDC(NULL);
        other = SelectObject(hdc, hbmp);
        up = mSkin.glyphmap;
        up[0] = (char)tx;
        up[1] = (char)ty;
        up += 2;

        for (btn = 0; btn < 6; ++btn)
            for (state = 0; state < 2; ++state) {
                for (n = y = 0; y < ty; ++y)
                    for (x = 0; x < tx; ++x, ++n) {
                        int px = btn*(tx+1)+1+x;
                        int py = state*(ty+1)+1+y;
                        COLORREF c = GetPixel(hdc, px, py);
                        if (0 == c)
                            up[n/8] |= 1 << n%8;
                    }
                up += ONE_GLYPH(tx,ty);
            }
        DeleteObject(SelectObject(hdc, other));
        DeleteDC(hdc);

        write_buttons();

    } else {

        static unsigned char default_buttons[2+6*2*ONE_GLYPH(9,9)] = {
        0x09,0x09,
        0x00,0x8C,0xB9,0xE3,0x83,0x83,0x8F,0x3B,0x63,0x00,0x00,
        0x00,0x8C,0xB9,0xE3,0x83,0x83,0x8F,0x3B,0x63,0x00,0x00,
        0x00,0xFC,0xF9,0x13,0x24,0x48,0x90,0x20,0x7F,0x00,0x00,
        0xF8,0xF1,0x23,0xFC,0xF9,0x33,0x7C,0x88,0x10,0x3F,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x80,0x3F,0x7F,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x80,0x3F,0x7F,0x00,0x00,
        0x00,0x1C,0x38,0x70,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x1C,0x38,0x70,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x20,0xE0,0xE0,0xE3,0x0F,0x07,0x0E,0x1C,0x00,0x00,
        0x00,0x70,0xE0,0xC0,0xE1,0x8F,0x0F,0x0E,0x08,0x00,0x00,
        0x00,0x0C,0x18,0x30,0xE0,0xC7,0x80,0x01,0x03,0x00,0x00,
        0x00,0xFC,0xF9,0xF3,0x07,0x01,0x02,0x04,0x08,0x00,0x00,
        };

        memcpy(mSkin.glyphmap, default_buttons, sizeof default_buttons);
        strcpy(buttonpath, "default");
    }
}

//===========================================================================
// get settings

void readSettings(void)
{
    int i;
    const char *p;

    memset(&mSkin, 0, sizeof mSkin);

    // titlebar click actions
    for (i = 0; i < 16; i++)
    {
        static const char *modifiers[4] = { "", "Shift", "Ctrl", "Alt" };
        static const char *buttons[4]  = { "Dbl", "Right", "Mid", "Left" };
        static const char *button_ids[] = {
            /* same order as subclass.h: enum button_types */
            "Close", "Maximize", "Minimize", "Rollup", "AlwaysOnTop", "Pin",
            "Menu", "Lower", "MinimizeToTray", NULL
        };
        char buf[80];
        sprintf(buf, "bbleanskin.titlebar.%s%sClick:", modifiers[i%4], buttons[i/4]);
        const char *p = ReadString(rcpath, buf, "");
        mSkin.captionClicks.Dbl[i] = (char)(1 + get_string_index(p, button_ids));
    }

    p = ReadString(rcpath, "bbleanskin.titlebar.buttons:", "4-321");
    if (strchr(p, '-')) {
        sprintf(mSkin.button_string, "%.*s", (int)sizeof mSkin.button_string - 1, p);
    } else {
        sprintf(mSkin.button_string, "%.3s-%.3s", p, p + imin(3, strlen(p)));
    }

    // button glyphs
    read_buttons(ReadString(rcpath, "bbleanskin.titlebar.glyphs:", NULL));

    // other settings
    mSkin.nixShadeStyle = ReadBool(rcpath, "bbleanskin.option.nixShadeStyle:", true);
    mSkin.snapWindows = ReadBool(rcpath, "bbleanskin.option.snapWindows:", false);
    mSkin.drawLocked = ReadBool(rcpath, "bbleanskin.option.drawLocked:", false);
    mSkin.imageDither = ReadBool(bbrcPath(), "session.imageDither:", false);
    mSkin.iconSat = ReadInt(rcpath, "bbleanskin.titlebar.iconSat", 255);
    mSkin.iconHue = ReadInt(rcpath, "bbleanskin.titlebar.iconHue", 0);

    // options
    adjustCaptionHeight = ReadBool(rcpath, "bbleanskin.option.adjustCaptionHeight:", false);
    enableLog = ReadBool(rcpath, "bbleanskin.option.enableLog:", false);
    sendToSwitchTo = ReadBool(rcpath, "bbleanskin.option.sendToSwitchTo:", false);

    // windows appearance settings
    MenuHeight = ReadInt(rcpath, "bbleanskin.windows.menu.height:", -1);
    strcpy(windows_menu_fontFace, ReadString(rcpath, "bbleanskin.windows.menu.Font:", ""));
    windows_menu_fontHeight = ReadInt(rcpath, "bbleanskin.windows.menu.fontHeight:", 8);
    ScrollbarSize = ReadInt(rcpath, "bbleanskin.windows.scrollbarsize:", -1);
    setTTColor = ReadBool(rcpath, "bbleanskin.windows.setTooltipColor:", false);

    // pass to skin structure
    mSkin.BBhwnd = BBhwnd;
    mSkin.BBVersion = BBVersion;
    mSkin.loghwnd = m_hwnd;
    mSkin.enableLog = enableLog;
}

//===========================================================================
// copy style into Skin

#define CANBEPR   0x200
#define DEFBORDER 0x400
#define DEFMARGIN 0x800
#define HASFONT   0x1000
#define HASMARGIN 0x2000
#define ISBORDER  0x4000

#define VALID_MARGIN 0x200

void readStyle(void)
{
    static const short settings_id_array[] =
    {
        SN_WINUNFOCUS_TITLE  |DEFBORDER ,
        SN_WINUNFOCUS_LABEL  |CANBEPR   ,
        SN_WINUNFOCUS_HANDLE |DEFBORDER ,
        SN_WINUNFOCUS_GRIP   |CANBEPR|DEFBORDER ,
        SN_WINUNFOCUS_BUTTON |CANBEPR   ,
        SN_WINFOCUS_BUTTONP  |CANBEPR ,
        SN_WINUNFOCUS_FRAME_COLOR |ISBORDER,

        SN_WINFOCUS_TITLE    |DEFBORDER|DEFMARGIN|HASMARGIN ,
        SN_WINFOCUS_LABEL    |CANBEPR|HASFONT|HASMARGIN   ,
        SN_WINFOCUS_HANDLE   |DEFBORDER         ,
        SN_WINFOCUS_GRIP     |CANBEPR|DEFBORDER ,
        SN_WINFOCUS_BUTTON   |CANBEPR|HASMARGIN ,
        SN_WINFOCUS_BUTTONP  |CANBEPR ,
        SN_WINFOCUS_FRAME_COLOR |ISBORDER,
        0
    };

    void *p;
    bool is_style070;
    const short *s;
    short id, flags;
    int tfh, fontheight;
    GradientItem *pG;
    windowGradients *wG;

    // init style reader (incase its built-in here)
    GetSettingPtr(0);

    p = GetSettingPtr(SN_ISSTYLE070);
    is_style070 = HIWORD(p) ? *(bool*)p : NULL!=p;

    mSkin.is_style070 = is_style070;
    mSkin.frameWidth = *(int*)GetSettingPtr(SN_FRAMEWIDTH);
    mSkin.handleHeight = *(int*)GetSettingPtr(SN_HANDLEHEIGHT);

    fontheight = tfh = 12;

    pG = &(wG = &mSkin.U)->Title;
    for (s = settings_id_array; 0 != (flags = *s); ++s)
    {
        id = flags & 255;
        p = GetSettingPtr(id);
        if (NULL == p)
            continue;

        if (flags & ISBORDER)
        {
            wG->FrameColor = *(COLORREF*)p;
            pG = &(++wG)->Title;
            continue;
        }

        StyleItem *pSI = (StyleItem*)p;
        copy_GradientItem(pG, pSI);

        if (0 == (flags & CANBEPR))
            pG->parentRelative = false;

        if (flags & HASFONT)
        {
            HFONT hf = CreateStyleFont(pSI);
            tfh = fontheight = get_fontheight(hf);
            GetObject(hf, sizeof mSkin.Font, &mSkin.Font);
            DeleteObject(hf);
            // damned, GetObject fills in random crap after the fontname,
            // which again makes the SystemParametersInfo below trigger
            strncpy(
                mSkin.Font.lfFaceName,
                mSkin.Font.lfFaceName,
                sizeof mSkin.Font.lfFaceName
                );
            mSkin.Justify = pSI->Justify;
        }

        if ((flags & HASMARGIN)
            && 0 == (pSI->validated & VALID_MARGIN)
            && false == is_style070)
        {
            if (flags & HASFONT)
                fontheight = pSI->FontHeight-2;
            if (bbLeanVersion < 1170) {
                if (id == SN_WINFOCUS_LABEL)
                    pG->marginWidth = 2;
                if (id == SN_WINFOCUS_BUTTON)
                    pG->marginWidth = mSkin.F.Label.marginWidth-1;
            }
        }

        ++pG;
    }

    //----------------------------------------------------------------
    // pre-calculate titlebar metrics (formula similar to the toolbar)

    int labelH, buttonH, tbheight;

    int top_border = imax(mSkin.F.Title.borderWidth, mSkin.U.Title.borderWidth);
    int lbl_border = imax(mSkin.F.Label.borderWidth, mSkin.U.Label.borderWidth);
    int bottom_border = imax(mSkin.F.Handle.borderWidth, mSkin.U.Handle.borderWidth);
    int button_border = imax(mSkin.F.Button.borderWidth, mSkin.U.Button.borderWidth);

    labelH  = (fontheight|1) +
        2*(mSkin.F.Label.marginWidth/*+lbl_border*/);
    buttonH = labelH +
        2*(mSkin.F.Button.marginWidth-mSkin.F.Label.marginWidth);
    tbheight = imax(labelH, buttonH) +
        2*(top_border + mSkin.F.Title.marginWidth);

    mSkin.buttonSize = buttonH;
    mSkin.labelHeight = labelH;
    mSkin.labelIndent = imax(2+lbl_border,(labelH-tfh)/2);

    mSkin.ncTop = tbheight;
    mSkin.ncBottom =
        mSkin.handleHeight
        ? mSkin.handleHeight + 2 * bottom_border
        : mSkin.frameWidth
        ;

    mSkin.rollupHeight = mSkin.ncTop;
    if (false == mSkin.nixShadeStyle && mSkin.handleHeight)
        mSkin.rollupHeight += mSkin.ncBottom - imin(top_border, bottom_border);

    mSkin.gripWidth = 2 * mSkin.buttonSize + mSkin.frameWidth;

    mSkin.labelMargin = (mSkin.ncTop - mSkin.labelHeight) / 2;
    mSkin.buttonMargin = (mSkin.ncTop - mSkin.buttonSize) / 2;
    mSkin.buttonInnerMargin = mSkin.buttonMargin - top_border;
    mSkin.buttonSpace = mSkin.buttonMargin - top_border;
    if (mSkin.buttonSpace == 0)
        mSkin.buttonSpace -= button_border;
    mSkin.bbsm_option = bbsm_option;
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
    // set the border to at least the style border.
    ncm_now.iBorderWidth = imax(1, mSkin.frameWidth - 3);

    //---------------------------
    // set the caption heights. If this is more than "mSkin.ncTop",
    // it will be hidden by the region setting.

    if (adjustCaptionHeight)
    {
        int n = mSkin.ncTop - ncm_now.iBorderWidth - 3;
        ncm_now.iCaptionHeight = ncm_now.iSmCaptionHeight = n;
        ncm_now.lfCaptionFont = mSkin.Font;
        ncm_now.lfSmCaptionFont = mSkin.Font;
    }

    //---------------------------
    // other settings
    if (-1 != MenuHeight)
        ncm_now.iMenuHeight = MenuHeight;

    if (-1 != ScrollbarSize)
        ncm_now.iScrollWidth = ncm_now.iScrollHeight = ScrollbarSize;

    if (windows_menu_fontFace[0])
    {
        //---------------------------
        // set menu/status/message font

        LOGFONT F;
        memset(&F, 0, sizeof F);
        //F.lfWidth = 0;
        //F.lfEscapement = 0;
        //F.lfOrientation = 0;
        //F.lfItalic = 0;
        //F.lfUnderline = 0;
        //F.lfStrikeOut = 0;
        F.lfCharSet = DEFAULT_CHARSET;
        F.lfOutPrecision = OUT_DEFAULT_PRECIS;
        F.lfClipPrecision = CLIP_DEFAULT_PRECIS;
        F.lfQuality = DEFAULT_QUALITY;
        F.lfPitchAndFamily = DEFAULT_PITCH|FF_DONTCARE;

        F.lfWeight = FW_NORMAL;
        F.lfHeight = -windows_menu_fontHeight;
        strcpy(F.lfFaceName, windows_menu_fontFace);

        ncm_now.lfMenuFont = F;
        ncm_now.lfStatusFont = F;
        ncm_now.lfMessageFont = F;
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
    }

    mSkin.cxSizeFrame = GetSystemMetrics(SM_CYSIZEFRAME);
    mSkin.cxFixedFrame = GetSystemMetrics(SM_CYFIXEDFRAME);
    mSkin.cyCaption = GetSystemMetrics(SM_CYCAPTION);
    mSkin.cySmCaption = GetSystemMetrics(SM_CYSMCAPTION);

    //-----------------------------
    // copy skin into shared memory
    memcpy(lpvMem, &mSkin, offset_exInfo);
}

//===========================================================================
// SysColor Stuff

enum ACTION_3DC { SAVE_3DC, RESTORE_3DC, APPLY_3DC };

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
        S1 = S2 = &mSkin.F.Label;

        if (S1->parentRelative)
            S1 = &mSkin.F.Title;

        if (B_SOLID == S1->type)
            C_CR[0] = S1->Color;
        else
            C_CR[0] = mixcolors(S1->Color, S1->ColorTo, 128);

        C_CR[1] = S2->TextColor;

        SetSysColors(NCOLORS, C_ID, C_CR);
        changed = true;
    }
}

//===========================================================================

//===========================================================================
// pass messages to windows

static BOOL CALLBACK SkinEnumProc(HWND hwnd, LPARAM lParam)
{
    void *pInfo = GetProp(hwnd, BBLEANSKIN_INFOPROP);
    if (NULL == pInfo)
    {
        if (BBLS_LOAD == lParam
            && WS_CAPTION == (WS_CAPTION & GetWindowLong(hwnd, GWL_STYLE)))
        {
            //dbg_printf("post %08x", hwnd);
            PostMessage(hwnd, bbSkinMsg, lParam, 0);
        }
    } else {
        if (BBLS_LOAD == lParam)
        {
            // This is likely a window that is still skinned after a
            // blackbox crash. Let it update it's skin.
            lParam = BBLS_REFRESH;
        }
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
// load the engine

// #define SIMU_WIN64

#ifdef SIMU_WIN64

#define pEntryFunc(option)
#define load_dll() true
#define free_dll()
#define _WIN64

#else

HINSTANCE hEngineInst;
int (*pEntryFunc)(int option);

void free_dll(void)
{
    if (hEngineInst)
        FreeLibrary(hEngineInst);
    hEngineInst = NULL;
}

bool load_dll(void)
{
    if (NULL == hEngineInst)
    {
        char engine_path[MAX_PATH];
        const char *error = NULL;
        hEngineInst = LoadLibrary(set_my_path(hInstance, engine_path, BBLEANSKIN_ENGINEDLL));
        *(FARPROC*)&pEntryFunc = GetProcAddress(hEngineInst, "EntryFunc");
        if (NULL == pEntryFunc)
            error = "Could not load: " BBLEANSKIN_ENGINEDLL;
        else
        if (ENGINE_THISVERSION != pEntryFunc(ENGINE_GETVERSION))
            error = "Version mismatch: " BBLEANSKIN_ENGINEDLL;

        if (error) {
            free_dll();
            MessageBox(NULL, error, szAppName,
                MB_OK | MB_ICONERROR | MB_SETFOREGROUND | MB_TOPMOST);
            return false;
        }
    }
    return true;
}

#endif //ndef SIMU_WIN64

//===========================================================================
// under _WIN64: startup 32-bit version of skinner from a separate process

#ifdef _WIN64

HANDLE hProcess32, hAck32;

void setHooks32(void)
{
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    BOOL ret;

    char cmdline[MAX_PATH];
    char path[MAX_PATH];

    hAck32 = CreateEvent(NULL, FALSE, FALSE, BBLEANSKIN_RUN32EVENT);
    if (NULL == hAck32)
        return;
    if (GetLastError() == ERROR_ALREADY_EXISTS)
        return;

    sprintf(cmdline, "\"%s\" %d",
        set_my_path(hInstance, path, "bbLeanSkinRun32.exe"),
        ENGINE_THISVERSION);

    memset(&si, 0, sizeof si);
    si.cb = sizeof(si);
    ret = CreateProcess(NULL, cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    if (FALSE == ret)
        return;

    CloseHandle (pi.hThread);
    hProcess32 = pi.hProcess;
    WaitForSingleObject(hAck32, 5000);
}

void unsetHooks32(void)
{
    if (hAck32) {
        PulseEvent(hAck32);
        CloseHandle(hAck32);
    }
    if (hProcess32) {
        WaitForSingleObject(hProcess32, 5000);
        CloseHandle(hProcess32);
    }
    hAck32 = hProcess32 = NULL;
}

#endif

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

    if (false == load_dll()) {
        free_exclusion_list();
        return;
    }

    bbSkinMsg = RegisterWindowMessage(BBLEANSKIN_MSG);
    // save sys-colors
    setTTC(SAVE_3DC);

    // save the normal SystemParameter settings, window metrics, etc.
    ncm_save.cbSize = sizeof(NONCLIENTMETRICS);
    SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof ncm_save, &ncm_save, 0);
    memset(&ncm_prev, 0, sizeof(NONCLIENTMETRICS));

    // read styles for skin
    readStyle();
    // possibly set sys-colors
    setTTC(APPLY_3DC);
    // set the system wide window metrics according to style metrics
    setmetrics();

    // set the hook
#ifdef _WIN64
    setHooks32();
#endif
    pEntryFunc(ENGINE_SETHOOKS);
    engine_running = true;

    // now apply to open windows
    sendToAll(BBLS_LOAD);
    send_stickyinfo_to_tasks();
    write_bin();
}

void refreshStyle(void)
{
    if (false == engine_running)
        return;
    readStyle();
    setmetrics();
    sendToAll(BBLS_REFRESH);
}

void reconfigureEngine(void)
{
    //dbg_printf("Reconfigured skinner engine");
    readSettings();
    set_log_window();
    if (false == engine_running)
        return;
    free_exclusion_list();
    make_exclusion_list();
    refreshStyle();
    setTTC(APPLY_3DC);
}

void stopEngine(void)
{
    //dbg_printf("Stopped skinner engine");
    if (false == engine_running)
        return;

    sendToAll(BBLS_UNLOAD);

#ifdef _WIN64
    unsetHooks32();
#endif
    pEntryFunc(ENGINE_UNSETHOOKS);

    free_exclusion_list();
    free_dll();
    // restore the normal SystemParameter settings, window metrics, etc.
    SystemParametersInfo(
        SPI_SETNONCLIENTMETRICS,
        sizeof(NONCLIENTMETRICS),
        &ncm_save,
        SPIF_SENDCHANGE
        );
    // restore sys-colors
    setTTC(RESTORE_3DC);
    engine_running = false;
}

//===========================================================================

