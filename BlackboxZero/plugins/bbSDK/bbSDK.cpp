/* ------------------------------------------------------------------ *

  bbSDK - Example plugin for Blackbox for Windows.
  Copyright © 2004,2009 grischka

  This program is free software, released under the GNU General Public
  License (GPL version 2). See:

  http://www.fsf.org/licenses/gpl.html

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

 * ------------------------------------------------------------------ */

/* ------------------------------------------------------------------ *

  Description:
  ------------
  This is an example Plugin for Blackbox for Windows. It displays
  a little stylized window with an inscription.

  Left mouse:
    - with the control key held down: moves the plugin
    - with alt key held down: resizes the plugin

  Right mouse click:
    - shows the plugin menu with some standard plugin configuration
      options. Also the inscription text can be set.

  bbSDK is compatible with all current bb4win versions:
    - bblean 1.12 or later
    - xoblite bb2 or later
    - bb4win 0.90 or later

  This file works both as C++ and plain C source.

 * ------------------------------------------------------------------ */

#include "BBApi.h"
#include <stdlib.h>

/* ---------------------------------- */
/* plugin info */

const char szAppName      [] = "bbSDK";
const char szInfoVersion  [] = "0.2";
const char szInfoAuthor   [] = "grischka";
const char szInfoRelDate  [] = "2009-05-20";
const char szInfoLink     [] = "http://bb4win.sourceforge.net/bblean";
const char szInfoEmail    [] = "grischka@users.sourceforge.net";
const char szVersion      [] = "bbSDK 0.2"; /* fallback for pluginInfo() */
const char szCopyright    [] = "2004,2009";

/* ---------------------------------- */
/* The About MessageBox */

void about_box(void)
{
    char szTemp[1000];
    sprintf(szTemp,
        "%s - A plugin example for Blackbox for Windows."
        "\n© %s %s"
        "\n%s"
        , szVersion, szCopyright, szInfoEmail, szInfoLink
        );
    MessageBox(NULL, szTemp, "About", MB_OK|MB_TOPMOST);
}

/* ---------------------------------- */
/* Dependencies on the plugin-name */

/* prefix for our broadcast messages */
#define BROAM_PREFIX "@bbSDK."
#define BROAM(key) (BROAM_PREFIX key) /* concatenation */

/* configuration file */
#define RC_FILE "bbSDK.rc"

/* prefix for items in the configuration file */
#define RC_PREFIX "bbsdk."
#define RC_KEY(key) (RC_PREFIX key ":")

/* prefix for unique menu id's */
#define MENU_ID(key) ("bbSDK_ID" key)

/* ---------------------------------- */
/* Interface declaration */

#ifdef __cplusplus
extern "C" {
#endif
    DLL_EXPORT int beginPlugin(HINSTANCE hPluginInstance);
    DLL_EXPORT int beginSlitPlugin(HINSTANCE hPluginInstance, HWND hSlit);
    DLL_EXPORT int beginPluginEx(HINSTANCE hPluginInstance, HWND hSlit);
    DLL_EXPORT void endPlugin(HINSTANCE hPluginInstance);
    DLL_EXPORT LPCSTR pluginInfo(int field);
#ifdef __cplusplus
};
#endif

/* ---------------------------------- */
/* Global variables */

HINSTANCE g_hInstance;
HWND g_hSlit;
HWND BBhwnd;
bool under_bblean;
bool under_xoblite;

/* full path to configuration file */
char rcpath[MAX_PATH];

/* ---------------------------------- */
/* Plugin window properties */

struct plugin_properties
{
    /* settings */
    int xpos, ypos;
    int width, height;

    bool useSlit;
    bool alwaysOnTop;
    bool snapWindow;
    bool pluginToggle;
    bool drawBorder;
    bool alphaEnabled;
    int  alphaValue;
    char windowText[100];

    /* our plugin window */
    HWND hwnd;

    /* current state variables */
    bool is_ontop;
    bool is_moving;
    bool is_sizing;
    bool is_hidden;
    bool is_inslit;

    /* the Style */
    StyleItem Frame;

    /* GDI objects */
    HBITMAP bufbmp;
    HFONT hFont;

} my;

/* why are these in a struct and wy is it named 'my'?
   Well, because my.xpos is nicely selfdocumenting. That's all :) */

/* ---------------------------------- */
/* some function prototypes */

void GetStyleSettings(void);
void ReadRCSettings(void);
void WriteRCSettings(void);
void ShowMyMenu(bool popup);
void invalidate_window(void);
void set_window_modes(void);

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

/* helper to handle commands from the menu */
struct msg_test {
    const char *msg;
    const char *test;
};

int scan_broam(struct msg_test *msg_test, const char *test);
void eval_broam(struct msg_test *msg_test, int mode, void *pValue);
enum eval_broam_modes
{
    M_BOL = 1,
    M_INT = 2,
    M_STR = 3,
};

/* ------------------------------------------------------------------ */

/* ------------------------------------------------------------------ */
/* The startup interface */

/* slit interface */
int beginPluginEx(HINSTANCE hPluginInstance, HWND hSlit)
{
    WNDCLASS wc;

    /* --------------------------------------------------- */
    /* This plugin can run in one instance only. If BBhwnd
       is set it means we are already loaded. */

    if (BBhwnd)
    {
        MessageBox(BBhwnd, "Do not load me twice!", szVersion,
                MB_OK | MB_ICONERROR | MB_TOPMOST);
        return 1; /* 1 = failure */
    }

    /* --------------------------------------------------- */
    /* grab some global information */

    BBhwnd = GetBBWnd();
    g_hInstance = hPluginInstance;
    g_hSlit = hSlit;

    if (0 == memicmp(GetBBVersion(), "bbLean", 6))
        under_bblean = true;
    else if (0 == memicmp(GetBBVersion(), "bb", 2))
        under_xoblite = true;

    /* --------------------------------------------------- */
    /* register the window class */

    memset(&wc, 0, sizeof wc);
    wc.lpfnWndProc  = WndProc;      /* window procedure */
    wc.hInstance    = g_hInstance;  /* hInstance of .dll */
    wc.lpszClassName = szAppName;    /* window class name */
    wc.hCursor      = LoadCursor(NULL, IDC_ARROW);
    wc.style        = CS_DBLCLKS;

    if (!RegisterClass(&wc))
    {
        MessageBox(BBhwnd,
            "Error registering window class", szVersion,
                MB_OK | MB_ICONERROR | MB_TOPMOST);
        return 1; /* 1 = failure */
    }

    /* --------------------------------------------------- */
    /* Zero out variables, read configuration and style */

    memset(&my, 0, sizeof my);
    ReadRCSettings();
    GetStyleSettings();

    /* --------------------------------------------------- */
    /* create the window */

    my.hwnd = CreateWindowEx(
        WS_EX_TOOLWINDOW,   /* window ex-style */
        szAppName,          /* window class name */
        NULL,               /* window caption text */
        WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, /* window style */
        0,                  /* x position */
        0,                  /* y position */
        0,                  /* window width */
        0,                  /* window height */
        NULL,               /* parent window */
        NULL,               /* window menu */
        g_hInstance,        /* hInstance of .dll */
        NULL                /* creation data */
        );

    /* set window location and properties */
    set_window_modes();

    /* show window (without stealing focus) */
    ShowWindow(my.hwnd, SW_SHOWNA);
    return 0; /* 0 = success */
}

/* no-slit interface */
int beginPlugin(HINSTANCE hPluginInstance)
{
    return beginPluginEx(hPluginInstance, NULL);
}

/* ------------------------------------------------------------------ */
/* on unload... */

void endPlugin(HINSTANCE hPluginInstance)
{
    /* Get out of the Slit, in case we are... */
    if (my.is_inslit)
        SendMessage(g_hSlit, SLIT_REMOVE, 0, (LPARAM)my.hwnd);

    /* Destroy the window... */
    DestroyWindow(my.hwnd);

    /* clean up HBITMAP object */
    if (my.bufbmp)
        DeleteObject(my.bufbmp);

    /* clean up HFONT object */
    if (my.hFont)
        DeleteObject(my.hFont);

    /* Unregister window class... */
    UnregisterClass(szAppName, hPluginInstance);
}

/* ------------------------------------------------------------------ */
/* pluginInfo is used by Blackbox for Windows to fetch information
   about a particular plugin. */

LPCSTR pluginInfo(int index)
{
    switch (index)
    {
        case PLUGIN_NAME:       return szAppName;       /* Plugin name */
        case PLUGIN_VERSION:    return szInfoVersion;   /* Plugin version */
        case PLUGIN_AUTHOR:     return szInfoAuthor;    /* Author */
        case PLUGIN_RELEASE:    return szInfoRelDate;   /* Release date, preferably in yyyy-mm-dd format */
        case PLUGIN_LINK:       return szInfoLink;      /* Link to author's website */
        case PLUGIN_EMAIL:      return szInfoEmail;     /* Author's email */
        default:                return szVersion;       /* Fallback: Plugin name + version, e.g. "MyPlugin 1.0" */
    }
}

/* ------------------------------------------------------------------ */
/* utilities */

/* debugging utility - 'DBGVIEW' from http://www.sysinternals.com/
   may be used to catch the output (or similar tools). */
void dbg_printf (const char *fmt, ...)
{
    char buffer[4000];
    va_list arg;

    va_start(arg, fmt);
    vsprintf(buffer, fmt, arg);
    OutputDebugString(buffer);
}

/* edit a file with the blackbox editor */
void edit_rc(const char *path)
{
    if (under_bblean) {
        SendMessage(BBhwnd, BB_EDITFILE, (WPARAM)-1, (LPARAM)path);
    } else {
        char editor[MAX_PATH];
        GetBlackboxEditor(editor);
        BBExecute(NULL, NULL, editor, path, NULL, SW_SHOWNORMAL, false);
    }
}

/* tweak val to stay between lo and hi */
int iminmax(int val, int lo, int hi)
{
    return val < lo ? lo : val > hi ? hi : val;
}

/* this invalidates the window, and resets the bitmap at the same time. */
void invalidate_window(void)
{
    if (my.bufbmp)
    {
        /* delete the double buffer bitmap (if we have one), so it
           will be drawn again next time with WM_PAINT */
        DeleteObject(my.bufbmp);
        my.bufbmp = NULL;
    }
    /* notify the OS that the window needs painting */
    InvalidateRect(my.hwnd, NULL, FALSE);
}

/* helper to copy paintbuffer on screen */
void BitBltRect(HDC hdc_to, HDC hdc_from, RECT *r)
{
    BitBlt(
        hdc_to,
        r->left, r->top, r->right - r->left, r->bottom - r->top,
        hdc_from,
        r->left, r->top,
        SRCCOPY
        );
}

/* ------------------------------------------------------------------ */
/* paint the window into the buffer HDC */

void paint_window(HDC hdc_buffer, RECT *p_rect)
{
    /* and draw the frame */
    MakeStyleGradient(hdc_buffer, p_rect, &my.Frame, my.drawBorder);

    if (my.windowText[0])
    {
        HGDIOBJ otherfont;
        int margin;
        RECT text_rect;

        /* Set the font, storing the default.. */
        otherfont = SelectObject(hdc_buffer, my.hFont);

        /* adjust the rectangle */
        margin = my.Frame.marginWidth + my.Frame.bevelposition;
        if (my.drawBorder)
            margin += my.Frame.borderWidth;

        text_rect.left  = p_rect->left + margin;
        text_rect.top   = p_rect->top + margin;
        text_rect.right = p_rect->right - margin;
        text_rect.bottom = p_rect->bottom - margin;

        /* draw the text */
        SetTextColor(hdc_buffer, my.Frame.TextColor);
        SetBkMode(hdc_buffer, TRANSPARENT);
        DrawText(hdc_buffer, my.windowText, -1, &text_rect,
            my.Frame.Justify|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);

        /* Put back the previous default font. */
        SelectObject(hdc_buffer, otherfont);
    }
}

/* ------------------------------------------------------------------ */

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static int msgs[] = { BB_RECONFIGURE, BB_BROADCAST, 0};

    switch (message)
    {
        case WM_CREATE:
            /* Register to reveive these message */
            SendMessage(BBhwnd, BB_REGISTERMESSAGE, (WPARAM)hwnd, (LPARAM)msgs);
            /* Make the window appear on all workspaces */
            MakeSticky(hwnd);
            break;

        case WM_DESTROY:
            /* as above, in reverse */
            RemoveSticky(hwnd);
            SendMessage(BBhwnd, BB_UNREGISTERMESSAGE, (WPARAM)hwnd, (LPARAM)msgs);
            break;

        /* ---------------------------------------------------------- */
        /* Blackbox sends a "BB_RECONFIGURE" message on style changes etc. */

        case BB_RECONFIGURE:
            ReadRCSettings();
            GetStyleSettings();
            set_window_modes();
            break;

        /* ---------------------------------------------------------- */
        /* Painting directly on screen. Good enough for static plugins. */
#if 1
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc;
            RECT r;

            /* get screen DC */
            hdc = BeginPaint(hwnd, &ps);

            /* Setup the rectangle */
            r.left = r.top = 0;
            r.right = my.width;
            r.bottom =  my.height;

            /* and paint everything on it*/
            paint_window(hdc, &r);

            /* Done */
            EndPaint(hwnd, &ps);
            break;
        }

        /* ---------------------------------------------------------- */
        /* Painting with a cached double-buffer. If your plugin updates
           frequently, this avoids flicker */
#else
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc, hdc_buffer;
            HGDIOBJ otherbmp;
            RECT r;

            /* get screen DC */
            hdc = BeginPaint(hwnd, &ps);

            /* create a DC for the buffer */
            hdc_buffer = CreateCompatibleDC(hdc);

            if (NULL == my.bufbmp) /* No bitmap yet? */
            {
                /* Make a bitmap ... */
                my.bufbmp = CreateCompatibleBitmap(hdc, my.width, my.height);

                /* ... and select it into the DC, saving the previous default. */
                otherbmp = SelectObject(hdc_buffer, my.bufbmp);

                /* Setup the rectangle */
                r.left = r.top = 0;
                r.right = my.width;
                r.bottom =  my.height;

                /* and paint everything on it*/
                paint_window(hdc_buffer, &r);
            }
            else
            {
                /* Otherwise it has been painted already,
                   so just select it into the DC */
                otherbmp = SelectObject(hdc_buffer, my.bufbmp);
            }

            /* Copy the buffer on the screen, within the invalid rectangle: */
            BitBltRect(hdc, hdc_buffer, &ps.rcPaint);

            /* Put back the previous default bitmap */
            SelectObject(hdc_buffer, otherbmp);
            /* clean up */
            DeleteDC(hdc_buffer);

            /* Done. */
            EndPaint(hwnd, &ps);
            break;
        }
#endif
        /* ---------------------------------------------------------- */
        /* Manually moving/sizing has been started */

        case WM_ENTERSIZEMOVE:
            my.is_moving = true;
            break;

        case WM_EXITSIZEMOVE:
            if (my.is_moving)
            {
                if (my.is_inslit)
                {
                    /* moving in the slit is not really supported but who
                       knows ... */
                    SendMessage(g_hSlit, SLIT_UPDATE, 0, (LPARAM)hwnd);
                }
                else
                {
                    /* if not in slit, record new position */
                    WriteInt(rcpath, RC_KEY("xpos"), my.xpos);
                    WriteInt(rcpath, RC_KEY("ypos"), my.ypos);
                }

                if (my.is_sizing)
                {
                    /* record new size */
                    WriteInt(rcpath, RC_KEY("width"), my.width);
                    WriteInt(rcpath, RC_KEY("height"), my.height);
                }
            }
            my.is_moving = my.is_sizing = false;
            set_window_modes();
            break;

        /* --------------------------------------------------- */
        /* snap to edges on moving */

        case WM_WINDOWPOSCHANGING:
            if (my.is_moving)
            {
                WINDOWPOS* wp = (WINDOWPOS*)lParam;
                if (my.snapWindow && false == my.is_sizing)
                    SnapWindowToEdge(wp, 10, SNAP_FULLSCREEN);

                /* set a minimum size */
                if (wp->cx < 40)
                    wp->cx = 40;

                if (wp->cy < 20)
                    wp->cy = 20;
            }
            break;

        /* --------------------------------------------------- */
        /* record new position or size */

        case WM_WINDOWPOSCHANGED:
            if (my.is_moving)
            {
                WINDOWPOS* wp = (WINDOWPOS*)lParam;
                if (my.is_sizing)
                {
                    /* record sizes */
                    my.width = wp->cx;
                    my.height = wp->cy;

                    /* redraw window */
                    invalidate_window();
                }

                if (false == my.is_inslit)
                {
                    /* record position, if not in slit */
                    my.xpos = wp->x;
                    my.ypos = wp->y;
                }
            }
            break;

        /* ---------------------------------------------------------- */
        /* start moving or sizing accordingly to keys held down */

        case WM_LBUTTONDOWN:
            UpdateWindow(hwnd);
            if (GetAsyncKeyState(VK_MENU) & 0x8000)
            {
                /* start sizing, when alt-key is held down */
                PostMessage(hwnd, WM_SYSCOMMAND, 0xf008, 0);
                my.is_sizing = true;
            }
            else
            if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
            {
                /* start moving, when control-key is held down */
                PostMessage(hwnd, WM_SYSCOMMAND, 0xf012, 0);
            }
            break;

        /* ---------------------------------------------------------- */
        /* normal mouse clicks */

        case WM_LBUTTONUP:
            /* code goes here ... */
            break;

        case WM_RBUTTONUP:
            /* Show the user menu on right-click (might test for control-key
               held down if wanted */
            /* if (wParam & MK_CONTROL) */
            ShowMyMenu(true);
            break;

        case WM_LBUTTONDBLCLK:
            /* Do something here ... */
            about_box();
            break;

        /* ---------------------------------------------------------- */
        /* Blackbox sends Broams to all windows... */

        case BB_BROADCAST:
        {
            const char *msg = (LPCSTR)lParam;
            struct msg_test msg_test;

            /* check general broams */
            if (!stricmp(msg, "@BBShowPlugins"))
            {
                if (my.is_hidden)
                {
                    my.is_hidden = false;
                    ShowWindow(hwnd, SW_SHOWNA);
                }
                break;
            }

            if (!stricmp(msg, "@BBHidePlugins"))
            {
                if (my.pluginToggle && false == my.is_inslit)
                {
                    my.is_hidden = true;
                    ShowWindow(hwnd, SW_HIDE);
                }
                break;
            }

            /* if the broam is not for us, return now */
            if (0 != memicmp(msg, BROAM_PREFIX, sizeof BROAM_PREFIX - 1))
                break;

            msg_test.msg = msg + sizeof BROAM_PREFIX - 1;

            if (scan_broam(&msg_test, "useSlit"))
            {
                eval_broam(&msg_test, M_BOL, &my.useSlit);
                break;
            }

            if (scan_broam(&msg_test, "alwaysOnTop"))
            {
                eval_broam(&msg_test, M_BOL, &my.alwaysOnTop);
                break;
            }

            if (scan_broam(&msg_test, "drawBorder"))
            {
                eval_broam(&msg_test, M_BOL, &my.drawBorder);
                break;
            }

            if (scan_broam(&msg_test, "snapWindow"))
            {
                eval_broam(&msg_test, M_BOL, &my.snapWindow);
                break;
            }

            if (scan_broam(&msg_test, "pluginToggle"))
            {
                eval_broam(&msg_test, M_BOL, &my.pluginToggle);
                break;
            }

            if (scan_broam(&msg_test, "alphaEnabled"))
            {
                eval_broam(&msg_test, M_BOL, &my.alphaEnabled);
                break;
            }

            if (scan_broam(&msg_test, "alphaValue"))
            {
                eval_broam(&msg_test, M_INT, &my.alphaValue);
                break;
            }

            if (scan_broam(&msg_test, "windowText"))
            {
                eval_broam(&msg_test, M_STR, &my.windowText);
                break;
            }

            if (scan_broam(&msg_test, "editRC"))
            {
                edit_rc(rcpath);
                break;
            }

            if (scan_broam(&msg_test, "About"))
            {
                about_box();
                break;
            }

            break;
        }

        /* ---------------------------------------------------------- */
        /* prevent the user from closing the plugin with alt-F4 */

        case WM_CLOSE:
            break;

        /* ---------------------------------------------------------- */
        /* let windows handle any other message */
        default:
            return DefWindowProc(hwnd,message,wParam,lParam);
    }
    return 0;
}

/* ------------------------------------------------------------------ */

/* ------------------------------------------------------------------ */
/* Update position and size, as well as onTop, transparency and
   inSlit states. */

void set_window_modes(void)
{
    HWND hwnd = my.hwnd;

    /* do we want to use the slit and is there a slit at all?  */
    if (my.useSlit && g_hSlit)
    {
        /* if in slit, dont move... */
        SetWindowPos(hwnd, NULL,
            0, 0, my.width, my.height,
            SWP_NOACTIVATE|SWP_NOSENDCHANGING|SWP_NOZORDER|SWP_NOMOVE
            );

        if (my.is_inslit)
        {
            /* we are already in the slit, so send update */
            SendMessage(g_hSlit, SLIT_UPDATE, 0, (LPARAM)hwnd);
        }
        else
        {
            /* transpareny must be off in slit */
            SetTransparency(hwnd, 255);
            /* enter slit now */
            my.is_inslit = true;
            SendMessage(g_hSlit, SLIT_ADD, 0, (LPARAM)hwnd);
        }
    }
    else
    {
        HWND hwnd_after = NULL;
        UINT flags = SWP_NOACTIVATE|SWP_NOSENDCHANGING|SWP_NOZORDER;
        RECT screen_rect;

        if (my.is_inslit)
        {
            /* leave it */
            SendMessage(g_hSlit, SLIT_REMOVE, 0, (LPARAM)hwnd);
            my.is_inslit = false;
        }

        if (my.is_ontop != my.alwaysOnTop)
        {
            my.is_ontop = my.alwaysOnTop;
            hwnd_after = my.is_ontop ? HWND_TOPMOST : HWND_NOTOPMOST;
            flags = SWP_NOACTIVATE|SWP_NOSENDCHANGING;
        }

        // make shure the plugin is on the screen:
        GetWindowRect(GetDesktopWindow(), &screen_rect);
        my.xpos = iminmax(my.xpos, screen_rect.left, screen_rect.right - my.width);
        my.ypos = iminmax(my.ypos, screen_rect.top, screen_rect.bottom - my.height);

        SetWindowPos(hwnd, hwnd_after, my.xpos, my.ypos, my.width, my.height, flags);
        SetTransparency(hwnd, (BYTE)(my.alphaEnabled ? my.alphaValue : 255));
    }

    /* window needs drawing */
    invalidate_window();
}

/* ------------------------------------------------------------------ */
/* Locate the configuration file */

/* this shows how to 'delay-load' an API that is in one branch but maybe
   not in another */

bool FindRCFile(char* pszOut, const char* rcfile, HINSTANCE plugin_instance)
{
    bool (*pFindRCFile)(LPSTR rcpath, LPCSTR rcfile, HINSTANCE plugin_instance);

    /* try to grab the function from blackbox.exe */
    *(FARPROC*)&pFindRCFile = GetProcAddress(GetModuleHandle(NULL), "FindRCFile");
    if (pFindRCFile) {
        /* use if present */
        return pFindRCFile(rcpath, rcfile, plugin_instance);

    } else {
       /* otherwise do something similar */
       int len = GetModuleFileName(plugin_instance, pszOut, MAX_PATH);
       while (len && pszOut[len-1] != '\\')
           --len;
       strcpy(pszOut + len, rcfile);
       return FileExists(pszOut);
    }
}

/* ------------------------------------------------------------------ */
/* Read the configuration file */

void ReadRCSettings(void)
{
    /* Locate configuration file */
    FindRCFile(rcpath, RC_FILE, g_hInstance);

    /* Read our settings. (If the config file does not exist,
       the Read... functions give us just the defaults.) */

    my.xpos   = ReadInt(rcpath, RC_KEY("xpos"), 10);
    my.ypos   = ReadInt(rcpath, RC_KEY("ypos"), 10);
    my.width  = ReadInt(rcpath, RC_KEY("width"), 80);
    my.height = ReadInt(rcpath, RC_KEY("height"), 40);

    my.alphaEnabled   = ReadBool(rcpath, RC_KEY("alphaEnabled"), false);
    my.alphaValue     = ReadInt(rcpath,  RC_KEY("alphaValue"), 192);
    my.alwaysOnTop    = ReadBool(rcpath, RC_KEY("alwaysOntop"), true);
    my.drawBorder     = ReadBool(rcpath, RC_KEY("drawBorder"), true);
    my.snapWindow     = ReadBool(rcpath, RC_KEY("snapWindow"), true);
    my.pluginToggle   = ReadBool(rcpath, RC_KEY("pluginToggle"), true);
    my.useSlit        = ReadBool(rcpath, RC_KEY("useSlit"), true);

    strcpy(my.windowText, ReadString(rcpath, RC_KEY("windowText"), szVersion));
}

/* ------------------------------------------------------------------ */
/* Get some blackbox style */

void GetStyleSettings(void)
{
    my.Frame = *(StyleItem *)GetSettingPtr(SN_TOOLBAR);
    if (my.hFont)
        DeleteObject(my.hFont);
    my.hFont = CreateStyleFont(&my.Frame);
}

/* ------------------------------------------------------------------ */
/* Show or update configuration menu */

void ShowMyMenu(bool popup)
{
    Menu *pMenu, *pSub;

    /* Create the main menu, with a title and an unique IDString */
    pMenu = MakeNamedMenu(szAppName, MENU_ID("Main"), popup);

    /* Create a submenu, also with title and unique IDString */
    pSub = MakeNamedMenu("Configuration", MENU_ID("Config"), popup);

    /* Insert first Item */
    MakeMenuItem(pSub, "Draw Border", BROAM("drawBorder"), my.drawBorder);

    if (g_hSlit)
        MakeMenuItem(pSub, "Use Slit", BROAM("useSlit"), my.useSlit);

    if (false == my.is_inslit)
    {
        /* these are only available if outside the slit */
        MakeMenuItem(pSub, "Always On Top", BROAM("alwaysOnTop"), my.alwaysOnTop);
        MakeMenuItem(pSub, "Snap To Edges", BROAM("snapWindow"), my.snapWindow);
        MakeMenuItem(pSub, "Toggle With Plugins", BROAM("pluginToggle"), my.pluginToggle);
        MakeMenuItem(pSub, "Transparency", BROAM("alphaEnabled"), my.alphaEnabled);
        MakeMenuItemInt(pSub, "Alpha Value", BROAM("alphaValue"), my.alphaValue, 0, 255);
    }

    /* Insert the submenu into the main menu */
    MakeSubmenu(pMenu, pSub, "Configuration");

    /* The configurable text string */
    MakeMenuItemString(pMenu, "Display Text", BROAM("windowText"), my.windowText);

    /* ---------------------------------- */
    /* add an empty line */
    MakeMenuNOP(pMenu, NULL);

    /* add an entry to let the user edit the setting file */
    MakeMenuItem(pMenu, "Edit Settings", BROAM("editRC"), false);

    /* and an about box */
    MakeMenuItem(pMenu, "About", BROAM("About"), false);

    /* ---------------------------------- */
    /* Finally, show the menu... */
    ShowMenu(pMenu);
}

/* ------------------------------------------------------------------ */
/* helper to handle commands from the menu */

int scan_broam(struct msg_test *msg_test, const char *test)
{
    int len;
    const char *msg;

    len = strlen(test);
    msg = msg_test->msg;

    if (strnicmp(msg, test, len) != 0)
        return 0;

    msg += len;
    if (*msg != 0 && *msg != ' ')
        return 0;

    /* store for function below */
    msg_test->msg = msg;
    msg_test->test = test;
    return 1;
}

void eval_broam(struct msg_test *msg_test, int mode, void *pValue)
{
    char rc_key[80];
    const char *msg;

    /* Build the full rc_key. i.e. "@bbSDK.xxx:" */
    sprintf(rc_key, "%s%s:", RC_PREFIX, msg_test->test);

    msg = msg_test->msg;
    /* skip possible whitespace after broam */
    while (*msg == ' ')
        ++msg;

    switch (mode)
    {
        /* --- set boolean variable ---------------- */
        case M_BOL:
            if (0 == stricmp(msg, "true"))
                *(bool*)pValue = true;
            else
            if (0 == stricmp(msg, "false"))
                *(bool*)pValue = false;
            else
                /* just toggle */
                *(bool*)pValue = false == *(bool*)pValue;

            /* write the new setting to the rc - file */
            WriteBool(rcpath, rc_key, *(bool*)pValue);
            break;

        /* --- set integer variable ------------------- */
        case M_INT:
            *(int*)pValue = atoi(msg);

            /* write the new setting to the rc - file */
            WriteInt(rcpath, rc_key, *(int*)pValue);
            break;

        /* --- set string variable ------------------- */
        case M_STR:
            strcpy((char*)pValue, msg);

            /* write the new setting to the rc - file */
            WriteString(rcpath, rc_key, (char*)pValue);
            break;
    }

    /* Apply new settings */
    set_window_modes();

    /* Update the menu checkmarks */
    ShowMyMenu(false);
}

/* ------------------------------------------------------------------ */
