/*
 ============================================================================

  This file is part of the bbLean source code
  Copyright © 2001-2003 The Blackbox for Windows Development Team
  Copyright © 2004 grischka

  http://bb4win.sourceforge.net/bblean
  http://sourceforge.net/projects/bb4win

 ============================================================================

  bbLean and bb4win are free software, released under the GNU General
  Public License (GPL version 2 or later), with an extension that allows
  linking of proprietary modules under a controlled interface. This means
  that plugins etc. are allowed to be released under any license the author
  wishes. For details see:

  http://www.fsf.org/licenses/gpl.html
  http://www.fsf.org/licenses/gpl-faq.html#LinkingOverControlledInterface

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
  for more details.

 ============================================================================
*/

#include "BB.h"
#include "Desk.h"
#include "Settings.h"
#include "Workspaces.h"
#include "MessageManager.h"
#include <shlobj.h>
#include <shellapi.h>
#include <process.h>

#define ST static

// changed for compatibility with ZMatrix
// ST const char szDesktopName[] = "BBLeanDesktop";
ST const char szDesktopName[] = "DesktopBackgroundClass";

ST HWND hDesktopWnd;
ST int focusmodel;

ST struct {
    HBITMAP bmp;
    char command[MAX_PATH];
} Root;

ST LRESULT CALLBACK Desk_WndProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
ST class DeskDropTarget *m_DeskDropTarget;
ST void Desk_Clear(void);

bool dont_hide_explorer = false;
bool dont_hide_tray = false;

// in bbroot.cpp
extern HBITMAP load_desk_bitmap(LPCSTR command);

static void init_DeskDropTarget(HWND hwnd);
static void exit_DeskDropTarget(HWND hwnd);
static bool get_drop_command(const char *filename, int flags);

//===========================================================================
ST struct hwnd_list *basebarlist;

ST void hidewnd (HWND hwnd, int f)
{
    //dbg_printf("hw = %08x", hw);
    if (hwnd && (ShowWindow(hwnd, SW_HIDE) || f))
        cons_node(&basebarlist, new_node(hwnd));
}

ST BOOL CALLBACK EnumExplorerWindowsProc(HWND hwnd, LPARAM lParam)
{
    char StringTmp[256];
    if (GetClassName(hwnd, StringTmp, 256) && 0==strcmp(StringTmp, "BaseBar"))
        hidewnd(hwnd, 0);
    return TRUE;
}

void HideExplorer()
{
    HWND hw;
    hw = FindWindow("Progman", "Program Manager");
    if (hw)
    {
        if (false == dont_hide_explorer)
        {
            //hw = FindWindowEx(hw, NULL, "SHELLDLL_DefView", NULL);
            //hw = FindWindowEx(hw, NULL, "SysListView32", NULL);
            hidewnd(hw, 0);
        }
        else
            MakeSticky(hw);
    }
    hw = FindWindow(ShellTrayClass, NULL);
    if (hw)
    {
        if (false == dont_hide_tray)
        {
            hidewnd(hw, 1);
            EnumWindows((WNDENUMPROC)EnumExplorerWindowsProc, 0);
        }
        else
            MakeSticky(hw);
    }
}

void ShowExplorer()
{
    struct hwnd_list *p;
    dolist (p, basebarlist) ShowWindow(p->hwnd, SW_SHOW);
    freeall(&basebarlist);
}

//===========================================================================
void set_focus_model(const char *fm_string)
{
    int fm = 0;
    if (0==stricmp(fm_string, "SloppyFocus"))
        fm = 1;
    if (0==stricmp(fm_string, "AutoRaise"))
        fm = 3;

    if (fm==focusmodel) return;
    focusmodel = fm;

    if (fm)
    SystemParametersInfo(SPI_SETACTIVEWNDTRKTIMEOUT,  0, (PVOID)Settings_autoRaiseDelay, SPIF_SENDCHANGE);
    SystemParametersInfo(SPI_SETACTIVEWINDOWTRACKING, 0, (PVOID)(0!=(fm & 1)), SPIF_SENDCHANGE);
    SystemParametersInfo(SPI_SETACTIVEWNDTRKZORDER,   0, (PVOID)(0!=(fm & 2)), SPIF_SENDCHANGE);
}

//===========================================================================
static void (*pSetDesktopMouseHook)(HWND BlackboxWnd, bool withExplorer);
static void (*pUnsetDesktopMouseHook)();
static HMODULE DestopHookInstance;

// --------------------------------------------
void Desk_Init(void)
{
    set_focus_model(Settings_focusModel);

    if (Settings_desktopHook || dont_hide_explorer)
    {
        DestopHookInstance = LoadLibrary("DesktopHook");
        *(FARPROC*)&pSetDesktopMouseHook = GetProcAddress(DestopHookInstance, "SetDesktopMouseHook" );
        *(FARPROC*)&pUnsetDesktopMouseHook = GetProcAddress(DestopHookInstance, "UnsetDesktopMouseHook" );
        if (pSetDesktopMouseHook)
            pSetDesktopMouseHook(BBhwnd, underExplorer);
        else
            BBMessageBox(MB_OK, NLS2("$BBError_DesktopHook$",
                "Error: DesktopHook.dll not found!"));
    }
    else
    {
        WNDCLASS wc;
        ZeroMemory(&wc, sizeof(wc));
        wc.hInstance = hMainInstance;
        wc.lpfnWndProc = Desk_WndProc;
        wc.lpszClassName = szDesktopName;
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.style = CS_DBLCLKS;

        BBRegisterClass(&wc);
        CreateWindowEx(
            WS_EX_TOOLWINDOW,
            szDesktopName,
            NULL,
            WS_CHILD|WS_VISIBLE|WS_CLIPCHILDREN|WS_CLIPSIBLINGS,
            0,0,0,0,
            GetDesktopWindow(),
            NULL,
            hMainInstance,
            NULL
            );
    }
    Desk_new_background();
}

// --------------------------------------------
void Desk_Exit()
{
    if (pUnsetDesktopMouseHook)
    {
        pUnsetDesktopMouseHook();
        FreeLibrary(DestopHookInstance);
    }
    if (hDesktopWnd)
    {
        DestroyWindow(hDesktopWnd);
        UnregisterClass(szDesktopName, hMainInstance);
        hDesktopWnd = NULL;
    }
    Desk_Clear();
    set_focus_model("");
}

//===========================================================================
ST void Desk_SetPosition()
{
    SetWindowPos(
        hDesktopWnd,
        HWND_BOTTOM,
        VScreenX, VScreenY, VScreenWidth, VScreenHeight,
        SWP_NOACTIVATE
        );
}

//===========================================================================
ST void Desk_Clear(void)
{
    if (Root.bmp)
        DeleteObject(Root.bmp), Root.bmp = NULL;

    if (hDesktopWnd)
        InvalidateRect(hDesktopWnd, NULL, FALSE);
}

void Desk_reset_rootCommand(void)
{
    Root.command [0] = 0;
}

//===========================================================================
const char * Desk_extended_rootCommand(const char *p)
{
    const char rc_key [] = "blackbox.background.rootCommand:";
    const char *extrc = extensionsrcPath();
    if (p) WriteString(extrc, rc_key, p);
    else p = ReadString(extrc, rc_key, NULL);
    return p;
}

//===========================================================================
ST HANDLE hDTThread;

ST void load_root_thread(void *pv)
{
    HBITMAP bmp = NULL;
    if (Root.command[0]) bmp = load_desk_bitmap(Root.command);
    PostMessage(hDesktopWnd, WM_USER, 0, (LPARAM)bmp);
    CloseHandle(hDTThread);
    hDTThread = NULL;
}

void Desk_new_background(const char *p)
{
    p = Desk_extended_rootCommand(p);
    if (p)
    {
        if (0 == stricmp(p, "none")) p = "";
        if (0 == stricmp(p, "style")) p = NULL;
    }

    if (false == Settings_background_enabled)
        p = "";
    else
    if (NULL == p)
        p = mStyle.rootCommand;

    if (0 == strcmp(Root.command, p))
        return;

    strcpy(Root.command, p);

    if (NULL == hDesktopWnd || false == Settings_smartWallpaper)
    {
        // use Windows Wallpaper?
        Desk_Clear();
        if (Root.command[0]) BBExecute_string(Root.command, true);
    }
    else
    {
        if (hDTThread) WaitForSingleObject(hDTThread, INFINITE);
        hDTThread = (HANDLE)_beginthread(load_root_thread, 0, NULL);
    }
}

//===========================================================================

ST LRESULT CALLBACK Desk_WndProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static UINT msgs [] = { BB_DRAGTODESKTOP, 0 };
    static bool button_down;
    int n = 0;
    int nDelta;
    switch (uMsg)
    {
        //====================
        case WM_CREATE:
            hDesktopWnd = hwnd;
            MakeSticky(hwnd);
            MessageManager_AddMessages(hwnd, msgs);
            Desk_SetPosition();
            init_DeskDropTarget(hwnd);
            break;

        //====================
        case WM_DESTROY:
            exit_DeskDropTarget(hwnd);
            MessageManager_RemoveMessages(hwnd, msgs);
            RemoveSticky(hwnd);
            break;

        case WM_USER:
            Desk_Clear();
            Root.bmp = (HBITMAP)lParam;
        case WM_NCPAINT:
            Desk_SetPosition();
            break;

        //====================
        case WM_CLOSE:
            break;

        //====================
        case WM_MOUSEACTIVATE:
            return MA_NOACTIVATE;

        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_XBUTTONDOWN:
            button_down = true;
            break;

        //====================
        case WM_LBUTTONUP: n = 0; goto post_click;
        case WM_RBUTTONUP: n = 1; goto post_click;
        case WM_MBUTTONUP: n = 2; goto post_click;
        case WM_XBUTTONUP:
            switch (HIWORD(wParam)) {
            case XBUTTON1: n = 3; goto post_click;
            case XBUTTON2: n = 4; goto post_click;
            case XBUTTON3: n = 5; goto post_click;
            } break;

        case WM_LBUTTONDBLCLK:
        case WM_RBUTTONDBLCLK:
        case WM_MBUTTONDBLCLK: n = 6;
            button_down = true;
            goto post_click;

        case WM_MOUSEWHEEL:
            nDelta = (short)HIWORD(wParam);
            if (nDelta > 0) n =  7; // Wheel UP
            if (nDelta < 0) n =  8; // Wheel Down
            button_down = true;
            goto post_click;

        post_click:
            if (button_down) PostMessage(BBhwnd, BB_DESKCLICK, 0, n);
            button_down = false;
//            PostMessage(BBhwnd, BB_DESKCLICK, 0, n);
            break;

        //====================
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc_scrn = BeginPaint(hwnd, &ps);
            if (Root.bmp)
            {
                HDC hdc_bmp = CreateCompatibleDC(hdc_scrn);
                HGDIOBJ other = SelectObject(hdc_bmp, Root.bmp);
                BitBltRect(hdc_scrn, hdc_bmp, &ps.rcPaint);
                SelectObject(hdc_bmp, other);
                DeleteDC(hdc_bmp);
            }
            else
            {
                PaintDesktop(hdc_scrn);
            }
            EndPaint(hwnd, &ps);
            break;
        }

        //====================
        case WM_ERASEBKGND:
            return TRUE;

        //====================
        case BB_DRAGTODESKTOP:
            return get_drop_command((const char *)lParam, wParam);

        //====================
        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);

    }
    return 0;
}

//===========================================================================
static const short vk_codes[] = { VK_MENU, VK_SHIFT, VK_CONTROL, VK_LBUTTON, VK_RBUTTON, VK_MBUTTON };
static const char mk_mods[] = { MK_ALT, MK_SHIFT, MK_CONTROL, MK_LBUTTON, MK_RBUTTON, MK_MBUTTON };
static const char modkey_strings_r[][6] = { "Alt", "Shift", "Ctrl", "Left", "Right", "Mid" };
static const char modkey_strings_l[][6] = { "Alt", "Shift", "Ctrl", "Right", "Left", "Mid" };
static const char button_strings[][10] = { "Left", "Right", "Mid", "X1", "X2", "X3", "Double", "WheelUp", "WheelDown"};
//===========================================================================
unsigned get_modkeys(void)
{
    unsigned modkey = 0;
    for (int i = 0; i < (int)(sizeof(vk_codes)/sizeof(vk_codes[0])); i++){
        if (0x8000 & GetAsyncKeyState(vk_codes[i])) modkey |= mk_mods[i];
    }
    return modkey;
}

bool Desk_mousebutton_event(LPARAM button)
{
    char rc_key[80] = "blackbox.desktop.";

    const char (*modkey_strings)[6] = GetSystemMetrics(SM_SWAPBUTTON) ? &modkey_strings_l[0] : &modkey_strings_r[0];

    unsigned modkey = get_modkeys();
    for (int i = 0; i < (int)(sizeof(mk_mods)/sizeof(mk_mods[0])); i++){
        if (mk_mods[i] & modkey) strcat(rc_key, modkey_strings[i]);
    }

    if (button >= (int)(sizeof(button_strings)/sizeof(button_strings[0]))) return false;
    if (button >= 7){
        sprintf(strchr(rc_key, 0), "%s:", button_strings[button]); // WheelUp/WheelDown
    }
    else{
        sprintf(strchr(rc_key, 0), "%sClick:", button_strings[button]);
    }
    const char *broam = ReadString(extensionsrcPath(), rc_key, NULL);

    if (broam)
        post_command(broam);
    else
    if (1 == button && 0 == modkey)
        PostMessage(BBhwnd, BB_MENU, BB_MENU_ROOT, 0);
    else
    if ((2 == button && 0 == modkey) || (1 == button && MK_SHIFT == modkey))
        PostMessage(BBhwnd, BB_MENU, BB_MENU_TASKS, 0);
    else
        return false;

    return true;
}

static bool get_drop_command(const char *filename, int flags)
{
    char buffer[MAX_PATH + 100];

    const char *e = strrchr(filename, '.');
    if (e && stristr(".bmp.gif.png.jpg.jpeg", e))
    {
        unsigned modkey = get_modkeys();
        const char *mode;

        if (MK_SHIFT == modkey) mode = "center";
        else
        if (MK_CONTROL == modkey) mode = "tile";
        else mode = "full";

        if (0 == (flags & 1))
        {
            sprintf(buffer, "@BBCore.rootCommand bsetroot -%s \"%s\"", mode, filename);
            post_command(buffer);
        }
        return true;
    }

    if (flags & 2)
    {
        if (0 == (flags & 1))
        {
            sprintf(buffer, "@BBCore.style %s", filename);
            post_command(buffer);
        }
        return true;
    }

    return false;
}

//===========================================================================

class DeskDropTarget : public IDropTarget
{
public:
    DeskDropTarget()
    {
        m_dwRef = 1;
    };

    virtual ~DeskDropTarget()
    {
    };

    STDMETHOD(QueryInterface)(REFIID iid, void** ppvObject)
    {
        if (IsEqualIID(iid, IID_IUnknown) || IsEqualIID(iid, IID_IDropTarget))
        {
            *ppvObject = this;
            AddRef();
            return S_OK;
        }
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }

    STDMETHOD_(ULONG, AddRef)()
    {
        return ++m_dwRef;
    }

    STDMETHOD_(ULONG, Release)()
    { 
        int tempCount = --m_dwRef;
        if (0 == tempCount) delete this;
        return tempCount; 
    }

    STDMETHOD (DragEnter) (LPDATAOBJECT pDataObject, DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect)
    {
        AddRef();
        m_filename[0] = 0;
        if (pDataObject)
        {
            FORMATETC fmte;
            fmte.cfFormat   = CF_HDROP;
            fmte.ptd        = NULL;
            fmte.dwAspect   = DVASPECT_CONTENT;  
            fmte.lindex     = -1;
            fmte.tymed      = TYMED_HGLOBAL;

            STGMEDIUM medium;
            if (SUCCEEDED(pDataObject->GetData(&fmte, &medium)))
            {
                HDROP hDrop = (HDROP)medium.hGlobal;
                m_filename[0] = 0;
                DragQueryFile(hDrop, 0, m_filename, sizeof(m_filename));
                DragFinish(hDrop);
                m_flags = is_stylefile(m_filename) ? 2 : 0;
            }
        }
        *pdwEffect = DROPEFFECT_NONE;
        return S_OK;
    }

    STDMETHOD(DragOver)(DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect)
    {
        *pdwEffect = DROPEFFECT_NONE;
        if (m_filename[0] && SendMessage(BBhwnd, BB_DRAGTODESKTOP, m_flags | 1, (LPARAM)m_filename))
            *pdwEffect = DROPEFFECT_LINK;
        return S_OK;
    }

    STDMETHOD(Drop)(LPDATAOBJECT pDataObject, DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect)
    {
        *pdwEffect = DROPEFFECT_NONE;
        if (m_filename[0] && SendMessage(BBhwnd, BB_DRAGTODESKTOP, m_flags, (LPARAM)m_filename))
            *pdwEffect = DROPEFFECT_LINK;
        Release();
        return S_OK;
    }

    STDMETHOD(DragLeave)()
    {
        Release();
        return S_OK;
    }

private:
    DWORD m_dwRef;
    char m_filename[MAX_PATH];
    int m_flags;
};

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// interface

static void init_DeskDropTarget(HWND hwnd)
{
    m_DeskDropTarget = new DeskDropTarget();
    RegisterDragDrop(hwnd, m_DeskDropTarget);
}

static void exit_DeskDropTarget (HWND hwnd)
{
    RevokeDragDrop(hwnd);
    m_DeskDropTarget->Release();
}

//===========================================================================
