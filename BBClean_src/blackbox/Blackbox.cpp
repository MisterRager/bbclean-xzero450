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
#include "Settings.h"
#include "MessageManager.h"
#include "PluginManager.h"
#include "Workspaces.h"
#include "Desk.h"
#include "Tray.h"
#include "Toolbar.h"
#include "Menu/MenuMaker.h"
#include "Resource.h"
#include "BBSendData.h"
#include "VolumeControl.h"

#include <process.h>
#include <shellapi.h>
#include <shlobj.h>
#include <commdlg.h>
#include <tlhelp32.h>
#include <ole2.h>
#include <locale.h>

#include "blackbox.h"

//====================

//====================

const char bb_exename       [] = "Blackbox.exe";
const char szBlackboxName   [] = "Blackbox";
const char szBlackboxClass  [] = "BlackboxClass";
const char ShellTrayClass   [] = "Shell_TrayWnd";

OSVERSIONINFO osInfo;
bool usingWin2kXP;
bool usingNT;
bool usingx64;

//====================

void shutdown_blackbox();
void start_plugins(void);
void kill_plugins(void);

bool installBlackbox(bool forceInstall);
bool uninstallBlackbox();
void RunStartupStuff(void);
WPARAM message_loop(void);

void about_style(void);
void edit_file(int id, const char* path);
bool exec_broam(const char *command);
void exec_command(const char *cmd);
void ShutdownWindows(int state, int no_msg);

//====================

HINSTANCE hMainInstance;
HWND BBhwnd;
bool bRunStartup = true;
bool underExplorer;
bool PluginsHidden;
unsigned WM_ShellHook;
bool multimon;
RECT OldDT;
bool bbactive = true;
BOOL save_opaquemove;

LRESULT CALLBACK MainWndProc(HWND, UINT, WPARAM, LPARAM);

//===========================================================================
// delay-load some functions on runtime to keep compatibility with 9x - nt

// undocumented function calls
void (WINAPI *pMSWinShutdown)(HWND);
void (WINAPI *pRunDlg)(HWND, HICON, LPCSTR, LPCSTR, LPCSTR, int);
void (WINAPI *pRegisterShellHook)(HWND, DWORD);
void (WINAPI *pSwitchToThisWindow)(HWND, int);
void (WINAPI *pShellDDEInit)(BOOL bInit);

//BOOL (WINAPI *pSetShellWindow)(HWND);
//HWND (WINAPI *pGetShellWindow)(void);

/* shell folder change notification (undocumented) */
UINT   (WINAPI *pSHChangeNotifyRegister)(HWND, DWORD, LONG, UINT, DWORD, struct _SHChangeNotifyEntry*);
BOOL   (WINAPI *pSHChangeNotifyDeregister)(UINT);

/* ToolHelp Function Pointers. (not in NT4) */
extern HANDLE (WINAPI *pCreateToolhelp32Snapshot)(DWORD,DWORD);
extern BOOL   (WINAPI *pModule32First)(HANDLE, LPMODULEENTRY32);
extern BOOL   (WINAPI *pModule32Next)(HANDLE, LPMODULEENTRY32);
/* psapi (NT based versions only) */
extern DWORD  (WINAPI *pGetModuleBaseName)(HANDLE, HMODULE, LPTSTR, DWORD);
extern BOOL   (WINAPI *pEnumProcessModules)(HANDLE, HMODULE *, DWORD, LPDWORD);

/* transparency (win2k+ only) */
extern BOOL (WINAPI *pSetLayeredWindowAttributes)(HWND, COLORREF, BYTE, DWORD);

/* multimonitor api (not in win95) */
extern HMONITOR (WINAPI *pMonitorFromPoint)(POINT, DWORD);
extern HMONITOR (WINAPI *pMonitorFromWindow)(HWND, DWORD);
extern BOOL (WINAPI *pGetMonitorInfoA)(HMONITOR, LPMONITORINFO);
extern BOOL (WINAPI* pEnumDisplayMonitors)(HDC, LPCRECT, MONITORENUMPROC, LPARAM);

/* NT based versions only: */
//BOOL (WINAPI* pSetProcessShutdownParameters)(DWORD dwLevel, DWORD dwFlags);

/* not in win95 */
BOOL (WINAPI* pTrackMouseEvent)(LPTRACKMOUSEEVENT lpEventTrack);

////////////////////////////////////////////////////////
static const char shell_lib   [] =  "SHELL32"   ;
static const char user_lib    [] =  "USER32"    ;
static const char kernel_lib  [] =  "KERNEL32"  ;
static const char psapi_lib   [] =  "PSAPI"     ;
static const char shdocvw_lib [] =  "SHDOCVW"   ;
static const char *rtl_libs   [] =
{
    shell_lib,
    user_lib,
    kernel_lib,
    psapi_lib,
    shdocvw_lib,
    NULL
};

////////////////////////////////////////////////////////
void init_runtime_libs(void)
{
    struct proc_info { const char *lib; char *procname; void *procadr; };

    static struct proc_info rtl_list [] =
    {
        { shdocvw_lib, (char*)0x76, &pShellDDEInit },
        //{ shell_lib, (char*)0xBC, &pShellDDEInit },
        //{ user_lib ,"SetShellWindow", &pSetShellWindow },
        //{ user_lib ,"GetShellWindow", &pGetShellWindow },

        { user_lib, "SwitchToThisWindow", &pSwitchToThisWindow },
        { user_lib, "SetLayeredWindowAttributes", &pSetLayeredWindowAttributes },
        { user_lib, "MonitorFromWindow", &pMonitorFromWindow },
        { user_lib, "MonitorFromPoint", &pMonitorFromPoint },
        { user_lib, "GetMonitorInfoA", &pGetMonitorInfoA },
        { user_lib, "EnumDisplayMonitors", &pEnumDisplayMonitors },
        { user_lib, "TrackMouseEvent", &pTrackMouseEvent },

        { shell_lib, (char*)0x02, &pSHChangeNotifyRegister },
        { shell_lib, (char*)0x04, &pSHChangeNotifyDeregister },
        { shell_lib, (char*)0x3C, &pMSWinShutdown },
        { shell_lib, (char*)0x3D, &pRunDlg },
        { shell_lib, (char*)0xB5, &pRegisterShellHook },

        { kernel_lib, "CreateToolhelp32Snapshot", &pCreateToolhelp32Snapshot },
        { kernel_lib, "Module32First", &pModule32First },
        { kernel_lib, "Module32Next", &pModule32Next },

        //{ kernel_lib, "SetProcessShutdownParameters", &pSetProcessShutdownParameters },

        { psapi_lib, "EnumProcessModules", &pEnumProcessModules },
        { psapi_lib, "GetModuleBaseNameA", &pGetModuleBaseName },
        { NULL }
    };

    struct proc_info *rtl_ptr = rtl_list;

    if (underExplorer
        || FindWindow("Progman", NULL)
        || false == ReadBool(extensionsrcPath(), "blackbox.options.enableDDE:", true))
        ++rtl_ptr; // skip 'ShellDDEInit'

    do {
        *(FARPROC*)rtl_ptr->procadr = GetProcAddress(LoadLibrary(rtl_ptr->lib), rtl_ptr->procname);
    } while ((++rtl_ptr)->lib);

    if (pShellDDEInit) pShellDDEInit(TRUE);
}

void exit_runtime_libs(void)
{
    if (pShellDDEInit) pShellDDEInit(FALSE);
    const char **p = rtl_libs;
    do FreeLibrary(GetModuleHandle(*p)); while (*++p);
}

void register_shellhook(HWND hwnd)
{
    if (pRegisterShellHook)
    {
        pRegisterShellHook(NULL, TRUE);
        pRegisterShellHook(hwnd, usingNT ? 3 : 1);
        WM_ShellHook = RegisterWindowMessage("SHELLHOOK");
    }
}

void unregister_shellhook(HWND hwnd)
{
    if (pRegisterShellHook)
    {
        pRegisterShellHook(hwnd, 0);
    }
}

void set_opaquemove(void)
{
    SystemParametersInfo(SPI_SETDRAGFULLWINDOWS, Settings_opaqueMove, NULL, SPIF_SENDCHANGE);
}

//===========================================================================

void bb_about(void)
{
    BBMessageBox(MB_OK,
    "%s - © 2003-2005 grischka"
    "\n%s",
    GetBBVersion(),
    NLS2("$BBAbout$",
		"Based stylistically on the Blackbox window manager for Linux by Brad Hughes\n\n"
		"Built at " __DATE__ " " __TIME__ "\n"
        "\n"
        "\nSwitches:"
        "\n-help  \t\tShow this text"
        "\n-install       \tInstall Blackbox as default shell"
        "\n-uninstall     \tReset Explorer as default shell"
        "\n-nostartup     \tDo not run startup programs"
        "\n-rc <path>     \tSpecify alternate blackbox.rc path"
		"\n-exec <broam>  \tSend broadcast message to running shell"
        "\n"
        "\nMore information about bbLean and Blackbox for Windows can be found here:"
        "\nhttp://bb4win.sourceforge.net/bblean/"
        "\nhttp://bb4win.org/"
        ));

}

//===========================================================================
/* parse commandline options - returns true to exit immediately */

bool check_options (LPCSTR lpCmdLine)
{
    for (;*lpCmdLine;)
    {
        static const char * options[] =
        {
            "help", "install", "uninstall", "exec",
            "rc", "nostartup", "dticons", "dttray",
			"toggle", "forceinstall",
            NULL
        };

        char option[MAX_PATH]; NextToken(option, &lpCmdLine);

        if (option[0] == '-') switch (get_string_index(&option[1], options))
        {
            case 0:
                bb_about();
                return true;
            case 1:
				installBlackbox(false);
                return true;
            case 2:
                uninstallBlackbox();
                return true;
            case 3:
            {
                const char *msg = "Blackbox not running.";
                if (BBhwnd)
                {
                    MSG m; PeekMessage(&m, NULL, 0, 0, PM_NOREMOVE);
                    if (BBSendData(BBhwnd, BB_EXECUTE, 0, lpCmdLine, -1))
                        return true;
                    msg = "No response from Blackbox.";
                }
                BBMessageBox(MB_OK, "%s (%s)\t", msg, option);
                return true;
            }
            case 4:
                bbrcPath(NextToken(option, &lpCmdLine));
                continue;
            case 5:
                bRunStartup = false;
                continue;
            case 6:
                dont_hide_explorer = true;
                continue;
            case 7:
                dont_hide_tray = true;
                continue;
            case 8:
                if (BBhwnd)
                {
                    PostMessage(BBhwnd, BB_QUIT, 0, 1);
                    return true;
                }
                bRunStartup = false;
                continue;
			case 9:
				installBlackbox(true);
				return true;
        }
        BBMessageBox(MB_OK, "Unknown commandline option: %s\t", option);
        return true;
    }
    return false;
}

//===========================================================================

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    hMainInstance = hInstance;

	ZeroMemory(&osInfo, sizeof(osInfo));
	osInfo.dwOSVersionInfoSize = sizeof(osInfo);
	GetVersionEx(&osInfo);
	usingNT         = osInfo.dwPlatformId == VER_PLATFORM_WIN32_NT;
	usingWin2kXP    = usingNT && osInfo.dwMajorVersion >= 5;

	//64-bit OS test, when running as 32-bit under WoW
	BOOL bIs64BitOS= FALSE;
	typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS)(HANDLE, PBOOL);
	LPFN_ISWOW64PROCESS fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(GetModuleHandle("kernel32"),"IsWow64Process");
	if (NULL != fnIsWow64Process)
		fnIsWow64Process(GetCurrentProcess(), &bIs64BitOS);
	usingx64=bIs64BitOS;
	//64-bit OS test, if compiled as native 64-bit. In case we ever need it.
	if (!usingx64)
		usingx64=(sizeof(int)!=sizeof(void*));

	setlocale(LC_TIME, "");

    BBhwnd = FindWindow(szBlackboxClass, szBlackboxName);

    if (check_options (lpCmdLine))
		return 0;

    /* Give the user a chance to get rid of a broken installation */
    if (0x8000 & GetAsyncKeyState(VK_CONTROL))
    {
        if (uninstallBlackbox())
        {
            ExitWindowsEx(EWX_LOGOFF, 0);
			return 0;
        }
    }

    /* Check if Blackbox is already running... */
    while (BBhwnd)
    {
        if (IDCANCEL == BBMessageBox(MB_RETRYCANCEL,
                NLS2("$BBError_StartedTwice$",
                    "***  Blackbox already running  ***")))
			return 0;
        BBhwnd = FindWindow(szBlackboxClass, szBlackboxName);
    }

    WNDCLASS wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.hInstance = hMainInstance;
    wc.lpfnWndProc = MainWndProc;
    wc.lpszClassName = szBlackboxClass;
    wc.hbrBackground = (HBRUSH)(COLOR_APPWORKSPACE + 1);
    wc.hIcon = LoadIcon (hMainInstance, (LPCSTR)IDI_BLACKBOX);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    if (0 == BBRegisterClass(&wc))
		return 0;

    // ------------------------------------------
	/* Are we running on top of Explorer? */
	underExplorer = NULL != FindWindow(ShellTrayClass, NULL);
	if (underExplorer) bRunStartup = false;

	// ------------------------------------------
    init_runtime_libs();
    multimon = NULL != pGetMonitorInfoA;
    OleInitialize(0);
    //SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);

    // ------------------------------------------
    /* WinNT based systems only: */
/*
    if (false == underExplorer && pSetProcessShutdownParameters)
        pSetProcessShutdownParameters(2, 0);
*/
    // ------------------------------------------
    /* get things going... */
    HideExplorer();
    SystemParametersInfo(SPI_GETWORKAREA, 0, (PVOID)&OldDT, 0);
    SystemParametersInfo(SPI_GETDRAGFULLWINDOWS, 0, &save_opaquemove, 0);

    /* Set "Hide minimized windows" system option... */
    MINIMIZEDMETRICS mm;
    ZeroMemory(&mm, sizeof(mm));
    mm.cbSize = sizeof(mm);
    SystemParametersInfo(SPI_GETMINIMIZEDMETRICS, sizeof(mm), &mm, 0);
    mm.iArrange |= ARW_HIDE; /* ARW_HIDE = 8 */
    /* the shell-hook notification will not work unless this is done: */
    SystemParametersInfo(SPI_SETMINIMIZEDMETRICS, sizeof(mm), &mm, 0);

    WPARAM RetCode;

#ifdef BBOPT_STACKDUMP
    __try {
#endif

    /* reset desktop margins */
    SetDesktopMargin(NULL, BB_DM_RESET, 0);
    MessageManager_Init();

    // ------------------------------------------
    /* create the main message window... */
    CreateWindowEx(
        WS_EX_TOOLWINDOW,
        szBlackboxClass,
        szBlackboxName,
        // visible since used as focus fallback for empty workspaces
        WS_POPUP|WS_VISIBLE,
        // this centers the mouse cursor with the autoraise focus-model
        (OldDT.left+OldDT.right)/2, (OldDT.top+OldDT.bottom)/2, 0, 0,
        NULL,
        NULL,
        hMainInstance,
        NULL
        );

    // ------------------------------------------
    /* This seems to inizialize some things and free some memory at startup. */
    SendMessage(GetDesktopWindow(), 0x400, 0, 0); /* 0x400 = WM_USER */

    /* Hack to terminate the XP welcome screen... */
    HANDLE hSRE;
    hSRE = OpenEvent(EVENT_MODIFY_STATE, FALSE, "Global\\msgina: ShellReadyEvent");
    if (hSRE) { SetEvent(hSRE), CloseHandle(hSRE); }
    hSRE = OpenEvent(EVENT_MODIFY_STATE, FALSE, "msgina: ShellReadyEvent");
    if (hSRE) { SetEvent(hSRE), CloseHandle(hSRE); }
    /*hSRE = CreateEvent(NULL, 0, 0, "ShellReadyEvent");
    if (hSRE) { SetEvent(hSRE); }*/

    // ------------------------------------------
    /* get things running */

	Settings_ReadRCSettings();
	/* Eliteforce: set process priority */
	SetProcessPriority();
	/* end */
	Settings_ReadStyleSettings();
    set_opaquemove();
    MenuMaker_Init();
    MenuMaker_Configure();
    Workspaces_Init();
    Desk_Init();
    Tray_Init();

    start_plugins();

    if (bRunStartup) SetTimer(BBhwnd, BB_RUNSTARTUP_TIMER, 500, NULL);

	Session_UpdateSession();

	/* Message Loop */
    RetCode = message_loop();

    // ------------------------------------------
    /* clean up */

    DestroyWindow(BBhwnd);
    UnregisterClass(szBlackboxClass, hMainInstance);

    MessageManager_Exit();
    free_nls();
    ClearSticky();

	// Noccy: New setting, blackbox.tweaks.noOleUnInit to bypass ole uninit
	if (Settings_noOleUninit == false) {
		OleUninitialize();
	}
	//exit_runtime_libs();

    SystemParametersInfo(SPI_SETWORKAREA, 0, (PVOID)&OldDT, SPIF_SENDCHANGE);
    Settings_opaqueMove = save_opaquemove;
    set_opaquemove();
    ShowExplorer();

    m_alloc_check_leaks("Blackbox Core");

#ifdef BBOPT_STACKDUMP
    }
    __except( except_filter( GetExceptionInformation() ) )
    {
        RetCode = -1;
    }
#endif

	return RetCode;
}

//===========================================================================
WPARAM message_loop(void)
{
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0)
    {
/*
        char buffer[200]; GetClassName(msg.hwnd, buffer, sizeof buffer);
        dbg_printf("hwnd %04x <%s>  msg %x  wp %08x  lp %08x", msg.hwnd, buffer, msg.message, msg.wParam, msg.lParam);
*/
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return msg.wParam;
}

//=====================================================
void start_plugins(void)
{
    beginToolbar(hMainInstance);
    PluginManager_Init();
}

//=====================================================
void kill_plugins(void)
{
    PluginManager_Exit();
    endToolbar(hMainInstance);
    SetDesktopMargin(NULL, BB_DM_RESET, 0);
    reset_reader();
}

//=====================================================
void shutdown_blackbox()
{
    WM_ShellHook = (unsigned)-1; // dont accept shell messages anymore
    Menu_All_Delete();
    kill_plugins();
    Tray_Exit();
    Desk_Exit();
    Workspaces_Exit();
    MenuMaker_Exit();
}

//===========================================================================
// Load a new_style

void reset_extended_rootCommand(void)
{
    if (0 == (GetAsyncKeyState(VK_CONTROL) & 0x8000) || (GetAsyncKeyState(VK_LWIN) & 0x8000))
    {
        Desk_extended_rootCommand("style");
    }
    else
    {
        const char *p = Desk_extended_rootCommand(NULL);
        if (0 == stricmp(p, "style"))
            Desk_extended_rootCommand(mStyle.rootCommand);
    }
}

void set_style(const char* filename, UINT flags)
{
    char fullpath[MAX_PATH];
    if (false == FindConfigFile(fullpath, filename, NULL))
    {
        MBoxErrorFile(fullpath);
    }
    else
    if (is_stylefile(fullpath))
    {
        WriteString(bbrcPath(), "session.styleFile:", get_relative_path(fullpath));
        reset_extended_rootCommand();
    }
    else
    if (flags & 1)
    {
        BBMessageBox(MB_OK,
            NLS2("$BBError_SetStyle$",
                "Trying to fool me, eh? ...drag'n drop a style file instead!"));
    }
    else
    {
        BBExecute_command(filename, NULL, false);
    }
}

void adjust_style(void) // temporary fix for bbStyleMaker 1.2
{
    mStyle.MenuTitle.marginWidth =
    mStyle.Toolbar.marginWidth =
    mStyle.windowTitleFocus.marginWidth =
        mStyle.bevelWidth;

    mStyle.MenuFrame.borderWidth =
    mStyle.MenuTitle.borderWidth =
    mStyle.Toolbar.borderWidth =
    mStyle.windowTitleFocus.borderWidth =
    mStyle.windowTitleUnfocus.borderWidth =
    mStyle.windowHandleFocus.borderWidth =
    mStyle.windowHandleUnfocus.borderWidth =
    mStyle.windowGripFocus.borderWidth =
    mStyle.windowGripUnfocus.borderWidth =
    mStyle.frameWidth =
        mStyle.borderWidth;

    mStyle.MenuFrame.borderColor =
    mStyle.MenuTitle.borderColor =
    mStyle.Toolbar.borderColor =
    mStyle.windowTitleFocus.borderColor =
    mStyle.windowTitleUnfocus.borderColor =
    mStyle.windowHandleFocus.borderColor =
    mStyle.windowHandleUnfocus.borderColor =
    mStyle.windowGripFocus.borderColor =
    mStyle.windowGripUnfocus.borderColor =
    mStyle.windowFrameFocusColor =
    mStyle.windowFrameUnfocusColor =
        mStyle.borderColor;

    mStyle.MenuFrame.foregroundColor =
    mStyle.MenuFrame.disabledColor =
        mStyle.MenuFrame.TextColor;

    mStyle.MenuHilite.foregroundColor =
    mStyle.MenuHilite.disabledColor =
        mStyle.MenuHilite.TextColor;

}

//===========================================================================
//#include "other/BBMessages.cpp"
//===========================================================================

LRESULT CALLBACK MainWndProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	const WPARAM ID_HOTKEY = 3;

    //if (uMsg >= BB_MSGFIRST && uMsg < BB_MSGLAST) log_BBMessage(uMsg, wParam, lParam);

    switch (uMsg)
    {
        // first the BB_... internal messages
        //===================================

        case BB_QUIT:
            // dont quit, if started as the main shell
            if (false == underExplorer && 0 == lParam && IDOK!=BBMessageBox(MB_OKCANCEL,
                    NLS2("$BBQuitQuery$",
                        "Are you sure you want to terminate the main shell?")))
                break;
bb_quit:
            SendMessage(hwnd, BB_EXITTYPE, 0, 0);
            Workspaces_GatherWindows();
            shutdown_blackbox();
            PostQuitMessage(0);
            break;

        //====================
        case BB_SHUTDOWN:
            Menu_All_Hide();
            ShutdownWindows(wParam, lParam);
            break;

        //====================
        case BB_SETSTYLE:
            if (lParam) set_style((const char*)lParam, wParam);
            PostMessage(hwnd, BB_RECONFIGURE, 0, 0);
            break;

        //====================
        case BB_ABOUTPLUGINS:
            PluginManager_aboutPlugins();
            break;

        case BB_ABOUTSTYLE:
            about_style();
            break;

        //====================
        case BB_EDITFILE:
            edit_file(wParam, (const char*)lParam);
            break;

        //====================
        case BB_RUN:
            pRunDlg(NULL, NULL, NULL, NULL, NULL, 0 );
            break;

        //====================
        // Execute a string (shellcommand or broam)
        case BB_EXECUTE:
            if (lParam) exec_command((const char*)lParam);
            break;

        case BB_EXECUTEASYNC: // for bbkeys etc.
            post_command((const char*)lParam);
            break;

        case BB_POSTSTRING: // posted command-string, from menu click
            exec_command((const char*)lParam);
            m_free((char*)lParam);
            break;

        //====================
        case BB_BROADCAST:
            if (false == exec_broam((LPCSTR)lParam))
                goto dispatch_bb_message;
            break;

        //====================
        case BB_TOGGLEPLUGINS:
            SendMessage(BBhwnd, BB_BROADCAST, 0, (LPARAM)
                (PluginsHidden ? "@BBShowPlugins" : "@BBHidePlugins"));
            goto dispatch_bb_message;

        //====================
        case BB_DESKCLICK:
            if (0 == lParam)
            {
                Menu_All_Hide();
                Menu_All_BringOnTop();
                if (false == Menu_Activate_Last())
                    SetActiveWindow(hwnd);
            }
			Desk_mousebutton_event(lParam,wParam);
            goto dispatch_bb_message;

        //====================
        // Menu

        case BB_MENU:
            if (PluginsHidden)
                Menu_All_Toggle(false);

            if (MenuMaker_ShowMenu(wParam, lParam))
                goto dispatch_bb_message;

            break;

        case BB_HIDEMENU:
            Menu_All_Hide();
            goto dispatch_bb_message;


        //======================================================
        case BB_RESTART:
        case_bb_restart:
            kill_plugins();
            if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
                BBMessageBox(MB_OK,
                    NLS2("$BBRestartPaused$",
                        "Restart paused, press OK to continue..."));
            SendMessage(BBhwnd, BB_RECONFIGURE, 0, 1);
            start_plugins();
			//RedrawConfigMenu();
			//Because of the dropShadow we need to destroy and rebuild the menu's
			//Destroy Menu's
			Menu_All_Delete();
			MenuMaker_Exit();
			//Rebuild menu's
			MenuMaker_Init();
			MenuMaker_Configure();
            break;

        //======================================================
        case BB_RECONFIGURE:
            if (check_filetime(plugrcPath(), &PluginManager_FT) && 0 == lParam)
                goto case_bb_restart;

            free_nls();
			Settings_ReadRCSettings();
			/* Elitefore: set process priority */
			SetProcessPriority();
			/* end */
			Settings_ReadStyleSettings();
			Session_UpdateSession();
            set_opaquemove();
            Workspaces_Reconfigure();
            Desk_new_background();
            MenuMaker_Configure();
            Menu_All_Redraw(0);
            if (false == Settings_toolbarEnabled) Toolbar_UpdatePosition();
            goto dispatch_bb_message;

        //====================
        case BB_REDRAWGUI:
            if (wParam & BBRG_MENU)
            {
                MenuMaker_Configure();
                Menu_All_Redraw(wParam);
            }
            goto dispatch_bb_message;

        //======================================================
        case BB_WINDOWLOWER:
        case BB_WINDOWRAISE:
        case BB_WINDOWSHADE:
        case BB_WINDOWGROWHEIGHT:
        case BB_WINDOWGROWWIDTH:
		case BB_WINDOWAOT:
            if (0 == lParam && IsWindow((HWND)wParam))
                lParam = wParam;

        case BB_WORKSPACE:
        case BB_SWITCHTON:
        case BB_LISTDESKTOPS:

        case BB_BRINGTOFRONT:
        case BB_WINDOWMINIMIZE:
        case BB_WINDOWMAXIMIZE:
        case BB_WINDOWRESTORE:
        case BB_WINDOWCLOSE:
        case BB_WINDOWMOVE:
        case BB_WINDOWSIZE:
        case BB_WINDOWMINIMIZETOTRAY:
            Workspaces_Command(uMsg, wParam, lParam);
            goto dispatch_bb_message;

        //====================
        case BB_DESKTOPINFO:
        case BB_TASKSUPDATE:
            Menu_All_Update(MENU_IS_TASKS);
            goto dispatch_bb_message;

        //====================
        // sent from the systembar on mouse over, if on mouseover
        // a tray app's window turned out to be not valid anymore
        case BB_CLEANTRAY:
            CleanTray();
            break;

        //====================
        case BB_REGISTERMESSAGE:
            MessageManager_AddMessages((HWND)wParam, (UINT*)lParam);
            break;

        case BB_UNREGISTERMESSAGE:
            MessageManager_RemoveMessages((HWND)wParam, (UINT*)lParam);
            break;

        // This one is registered as a workaround (see MenuMaker.cpp)
        case BB_FOLDERCHANGED:
            //dbg_printf("BB folder changed");
            break;

        //==============================================================
        // COPYDATA stuff, for passing information from/to other processes
        // (i.e. bbStyleMaker, BBNote)

        case BB_GETSTYLE:
            return BBSendData((HWND)lParam, BB_SENDDATA, wParam, stylePath(), -1);

        case BB_GETSTYLESTRUCT:
            return BBSendData((HWND)lParam, BB_SENDDATA, wParam, &mStyle, sizeof mStyle);

        case BB_SETSTYLESTRUCT:
            memcpy(&mStyle, (const void*)lParam, sizeof mStyle);
            break;

        // ------------------------
        // previous implementation:

        case BB_GETSTYLE_OLD:
        {
            COPYDATASTRUCT cds;
            cds.dwData = 1;
            cds.lpData = (void*)stylePath();
            cds.cbData = 1+strlen((char*)cds.lpData);
            return SendMessage ((HWND)lParam, WM_COPYDATA, (WPARAM)hwnd, (LPARAM)&cds);
        }

        case BB_GETSTYLESTRUCT_OLD:
        {
            COPYDATASTRUCT cds;
            if (SN_STYLESTRUCT != wParam) break;
            cds.dwData = SN_STYLESTRUCT+100;
            cds.cbData = sizeof(StyleStruct);
            cds.lpData = GetSettingPtr(SN_STYLESTRUCT);
            return SendMessage ((HWND)lParam, WM_COPYDATA, (WPARAM)hwnd, (LPARAM)&cds);
        }

        case BB_SETSTYLESTRUCT_OLD:
        {
            if (SN_STYLESTRUCT != wParam) break;
            memcpy(&mStyle, (const void*)lParam, sizeof mStyle);
            adjust_style();
            break;
        }

        // done with BB_messages,
        //==============================================================

        //==============================================================
        // now for the WM_... messages

        case WM_CREATE:
            BBhwnd = hwnd;
            MakeSticky(hwnd);
            register_shellhook(hwnd);
			RegisterHotKey(hwnd, ID_HOTKEY, MOD_CONTROL|MOD_ALT, VK_F1);
            break;

        //====================
        case WM_DESTROY:
            unregister_shellhook(hwnd);
            RemoveSticky(hwnd);
            break;

        //====================
        case WM_ENDSESSION:
            if (wParam) shutdown_blackbox();
            break;

        case WM_QUERYENDSESSION:
            Workspaces_GatherWindows();
            return TRUE;

        case WM_CLOSE:
            break;

        //====================
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            PaintDesktop(hdc);
            EndPaint(hwnd, &ps);
            break;
        }

        //====================
		case WM_HOTKEY:
//             //dbg_printf("HK = %d", wParam);
			if (ID_HOTKEY == wParam)
				goto bb_quit;
			break;

        //====================
        case WM_MOUSEACTIVATE:
            return MA_NOACTIVATE;

        //====================
        case WM_ACTIVATEAPP:
        {
            bool new_bbactive = 0 != wParam;
            if (new_bbactive != bbactive)
            {
                bbactive = new_bbactive;
                if (false == new_bbactive) Menu_All_Hide();
            }
            break;
        }

        //======================================================
        case WM_DISPLAYCHANGE:
            // disable this during shutdown
            if ((unsigned)-1 == WM_ShellHook) break;
            Desk_reset_rootCommand();
            PostMessage(hwnd, BB_RECONFIGURE, 0,0);
            break;

        //====================
        case WM_TIMER:
            if (BB_CHECKWINDOWS_TIMER == wParam)
            {
                Workspaces_handletimer();
                break;
            }
            KillTimer(hwnd, wParam);
            if (BB_WRITERC_TIMER == wParam)
            {
                write_rcfiles();
                break;
            }
            if (BB_RUNSTARTUP_TIMER == wParam)
            {
                RunStartupStuff();
                break;
            }
            break;

        //====================
        case WM_KEYDOWN:
            if (VK_TAB == wParam)
                Menu_Tab_Next(NULL);
            break;

        //====================
        case WM_COPYDATA:
            return BBReceiveData(hwnd, lParam);

        case WM_SETTEXT:
			SendMessage(hwnd, BB_BROADCAST, 0, lParam);
			return 1;

		//====================
        default:
            if (uMsg == WM_ShellHook)
            {
                LPARAM extended = 0 != (wParam & 0x8000);
                switch (wParam & 0x7fff)
                {
                    case HSHELL_TASKMAN:
                        uMsg = BB_WINKEY;
                        lParam = (GetAsyncKeyState(VK_RWIN) & 1) ? VK_RWIN : VK_LWIN;
                        goto p2;

                    case HSHELL_WINDOWCREATED:          uMsg = BB_ADDTASK;              break;
                    case HSHELL_WINDOWDESTROYED:        uMsg = BB_REMOVETASK;           break;
                    case HSHELL_ACTIVATESHELLWINDOW:    uMsg = BB_ACTIVATESHELLWINDOW;  break;
                    case HSHELL_WINDOWACTIVATED:        uMsg = BB_ACTIVETASK;           break;
                    case HSHELL_GETMINRECT:             uMsg = BB_MINMAXTASK;           break;
                    case HSHELL_REDRAW:                 uMsg = BB_REDRAWTASK;           break;
                    default: return 0;
                }
                TaskWndProc(wParam, (HWND)lParam);
            p2:
                PostMessage(hwnd, uMsg, lParam, extended);
                break;
            }

            if (uMsg >= BB_MSGFIRST && uMsg < BB_MSGLAST)
            {
                dispatch_bb_message:
                return MessageManager_SendMessage(uMsg, wParam, lParam);
            }

            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

//===========================================================================

//===========================================================================
void about_style(void)
{
    const char *cp = stylePath();
    BBMessageBox(MB_OK,
        "#bbLean - %s#"
		//"Style: %s\t\t\t\t"
		"Style:\t%s\n"
		"Author:\t%s %s\n"
		"Date:\t%s\n"
		"Credits:\t%s\n"
		"Comments: %s\n",
        NLS2("$BBAboutStyle_Title$","Style Information"),
        ReadString(cp, "style.name:", NLS2("$BBAboutStyle_no_name$", "[Style name not specified]")),
        NLS2("$BBAboutStyle_by$", "by"),
        ReadString(cp, "style.author:", NLS2("$BBAboutStyle_no_author$", "[Author not specified]")),
        ReadString(cp, "style.date:", ""),
        ReadString(cp, "style.credits:", ""),
        ReadString(cp, "style.comments:", "")
        );
}

//=====================================================
void edit_file(int id, const char* path)
{
    char fullpath[MAX_PATH];
    char editor[MAX_PATH];
    switch (id)
    {
        case 0:  path=stylePath();        goto edit;
        case 1:  path=menuPath();         goto edit;
        case 2:  path=plugrcPath();       goto edit;
        case 3:  path=extensionsrcPath(); goto edit;
        case 4:  path=bbrcPath();         goto edit;
        case -1: FindConfigFile(fullpath, path, NULL), path = fullpath;
    edit:
        GetBlackboxEditor(editor);
        BBExecute_command(editor, path, false);
    }
}

//===========================================================================
void ShowAppnames(void)
{
    char buffer[4096];
    char appname[MAX_PATH];
    char caption[256];
    int i, x, ts;
    buffer[0] = 0;
    for (i = x = 0, ts = GetTaskListSize(); i<ts; i++)
    {
        HWND hw = GetTask(i);
        GetAppByWindow(hw, appname);
        GetWindowText(hw, caption, 256);
        x+=sprintf(buffer+x, "\n%-20s \"%s\"", appname, caption);
    }
    BBMessageBox(0, NLS2("$BBShowApps$",
            "Current Applications:"
            "\n(Use the name from first column for StickyWindows.ini)"
            "\n%s"
            ), buffer);
}

//===========================================================================
void exec_command(const char *cmd)
{
    if ('@' == cmd[0])
        SendMessage(BBhwnd, BB_BROADCAST, 0, (LPARAM)cmd);
    else
        BBExecute_string(cmd, false);
}

void post_command(const char *p)
{
    PostMessage(BBhwnd, BB_POSTSTRING, 0, (LPARAM)new_str(p));
}

//===========================================================================
enum { // Shutdown modes;
    BBSD_SHUTDOWN   = 0,
    BBSD_REBOOT     = 1,
    BBSD_LOGOFF     = 2,
    BBSD_HIBERNATE  = 3,
    BBSD_SUSPEND    = 4,
    BBSD_LOCKWS     = 5,
    BBSD_EXITWIN    = 6
};
enum {
    e_rootCommand = 1,
    e_Message,
    e_ShowAppnames,
    e_About,
    e_Nop,
    e_Crash,
    e_Test,

    e_alt   = 0x80,
    e_lparg = 0x8000
};

//===========================================================================
static const struct corebroam_table
{
    char *str; unsigned short msg; short wParam;
}
    corebroam_table [] =
{
    { "Raise",                      BB_WINDOWRAISE,          0 },
    { "RaiseWindow",                BB_WINDOWRAISE,          0 },
    { "Lower",                      BB_WINDOWLOWER,          0 },
    { "LowerWindow",                BB_WINDOWLOWER,          0 },
    { "ShadeWindow",                BB_WINDOWSHADE,          0 },
    { "Close",                      BB_WINDOWCLOSE,          0 },
    { "CloseWindow",                BB_WINDOWCLOSE,          0 },
    { "Minimize",                   BB_WINDOWMINIMIZE,       0 },
    { "MinimizeWindow",             BB_WINDOWMINIMIZE,       0 },
    { "Maximize",                   BB_WINDOWMAXIMIZE,       0 },
    { "MaximizeWindow",             BB_WINDOWMAXIMIZE,       0 },
    { "MaximizeVertical",           BB_WINDOWGROWHEIGHT,     0 },
    { "MaximizeHorizontal",         BB_WINDOWGROWWIDTH,      0 },
    { "Restore",                    BB_WINDOWRESTORE,        0 },
    { "RestoreWindow",              BB_WINDOWRESTORE,        0 },
    { "Resize",                     BB_WINDOWSIZE,           0 },
    { "ResizeWindow",               BB_WINDOWSIZE,           0 },
    { "Move",                       BB_WINDOWMOVE,           0 },
    { "MoveWindow",                 BB_WINDOWMOVE,           0 },
    { "MinimizeToTray",             BB_WINDOWMINIMIZETOTRAY, 0 },
	{ "AlwaysOnTop",				BB_WINDOWAOT,		0 },

    { "PrevWindow",                 BB_WORKSPACE,       BBWS_PREVWINDOW },
    { "NextWindow",                 BB_WORKSPACE,       BBWS_NEXTWINDOW },
    { "PrevWindowAllWorkspaces",    BB_WORKSPACE,       BBWS_PREVWINDOW|e_alt },
    { "NextWindowAllWorkspaces",    BB_WORKSPACE,       BBWS_NEXTWINDOW|e_alt },
    { "StickWindow",                BB_WORKSPACE,       BBWS_TOGGLESTICKY },

    // all windows
    { "MinimizeAll",                BB_WORKSPACE,       BBWS_MINIMIZEALL },
    { "RestoreAll",                 BB_WORKSPACE,       BBWS_RESTOREALL },
    { "Cascade",                    BB_WORKSPACE,       BBWS_CASCADE },
    { "TileVertical",               BB_WORKSPACE,       BBWS_TILEVERTICAL },
    { "TileHorizontal",             BB_WORKSPACE,       BBWS_TILEHORIZONTAL },

    // workspaces
    { "LeftWorkspace",              BB_WORKSPACE,       BBWS_DESKLEFT },
    { "PrevWorkspace",              BB_WORKSPACE,       BBWS_DESKLEFT },
    { "RightWorkspace",             BB_WORKSPACE,       BBWS_DESKRIGHT },
    { "NextWorkspace",              BB_WORKSPACE,       BBWS_DESKRIGHT },
    { "SwitchToWorkspace",          BB_WORKSPACE,       BBWS_SWITCHTODESK },

    { "MoveWindowLeft",             BB_WORKSPACE,       BBWS_MOVEWINDOWLEFT },
    { "MoveWindowRight",            BB_WORKSPACE,       BBWS_MOVEWINDOWRIGHT },
    { "MoveWindowToWS",             BB_WORKSPACE,       BBWS_MOVEWINDOWTOWS },
    { "Gather",                     BB_WORKSPACE,       BBWS_GATHERWINDOWS },
    { "GatherWindows",              BB_WORKSPACE,       BBWS_GATHERWINDOWS },
    { "AddWorkspace",               BB_WORKSPACE,       BBWS_ADDDESKTOP },
    { "DelWorkspace",               BB_WORKSPACE,       BBWS_DELDESKTOP },
    { "EditWorkspaceNames",         BB_WORKSPACE,       BBWS_EDITNAME },

    { "ActivateTask",               BB_BRINGTOFRONT,    0},

    // edit
    { "Edit",                       BB_EDITFILE |e_lparg, -1 },
    { "EditStyle",                  BB_EDITFILE,        0 },
    { "EditMenu",                   BB_EDITFILE,        1 },
    { "EditPlugins",                BB_EDITFILE,        2 },
    { "EditExtensions",             BB_EDITFILE,        3 },
    { "EditBlackbox",               BB_EDITFILE,        4 },

    // menu
    { "ShowMenuKBD",                BB_MENU |e_lparg,   BB_MENU_BYBROAM_KBD  },
    { "ShowMenu",                   BB_MENU,            BB_MENU_ROOT  },
    { "ShowWorkspaceMenu",          BB_MENU,            BB_MENU_TASKS },
    { "ShowIconMenu",               BB_MENU,            BB_MENU_ICONS },
    { "HideMenu",                   BB_HIDEMENU,        0 },

    // blackbox
    { "AboutStyle",                 BB_ABOUTSTYLE,      0 },
    { "AboutPlugins",               BB_ABOUTPLUGINS,    0 },
    { "TogglePlugins",              BB_TOGGLEPLUGINS,   0 },
    { "Restart",                    BB_RESTART,         0 },
    { "Reconfig",                   BB_RECONFIGURE,     0 },
    { "Reconfigure",                BB_RECONFIGURE,     0 },
    { "Exit",                       BB_QUIT,            0 },
    { "Quit",                       BB_QUIT,            0 },
    { "Run",                        BB_RUN,             0 },

    // shutdown
    { "Shutdown",                   BB_SHUTDOWN,        BBSD_SHUTDOWN   },
    { "Reboot",                     BB_SHUTDOWN,        BBSD_REBOOT     },
    { "Logoff",                     BB_SHUTDOWN,        BBSD_LOGOFF     },
    { "Hibernate",                  BB_SHUTDOWN,        BBSD_HIBERNATE  },
    { "Suspend",                    BB_SHUTDOWN,        BBSD_SUSPEND    },
    { "LockWorkstation",            BB_SHUTDOWN,        BBSD_LOCKWS     },
    { "ExitWindows",                BB_SHUTDOWN,        BBSD_EXITWIN    },

    // miscellaneous
    { "Style",                      BB_SETSTYLE |e_lparg, 0 },
    { "Exec",                       BB_EXECUTE  |e_lparg, 0 },

    { "rootCommand",                0, e_rootCommand    },
    { "Message",                    0, e_Message        },
    { "ShowAppnames",               0, e_ShowAppnames   },
    { "About",                      0, e_About          },
    { "Nop",                        0, e_Nop            },
    { "Crash",                      0, e_Crash          },
    { "Test",                       0, e_Test           },

    { NULL /*"Workspace#"*/,        BB_WORKSPACE,       BBWS_SWITCHTODESK },
};

//===========================================================================

int get_workspace_number(const char *s)
{
    int n;
    if (0 == memicmp(s, "workspace", 9) && s[9] >= '1' && s[9] <= '9')
        if ((n = atoi(s + 9)) > 0) return n - 1;
    return -1;
}

//===========================================================================

void exec_core_broam(const char *broam)
{
    char buffer[1024]; int n;
    const char *core_args = broam + 8; // skip "BBCore."
    char *core_cmd = NextToken(buffer, &core_args);

    const struct corebroam_table *action = corebroam_table;
    do if (0==stricmp(action->str, core_cmd)) break;
    while ((++action)->str);

    WPARAM wParam = action->wParam;
    LPARAM lParam = 0;
    UINT msg = action->msg;

    if (NULL == action->str)
    {
        if (-1 == (n = get_workspace_number(core_cmd)))
        {
            BBMessageBox(MB_OK, NLS2("$BBError_UnknownCoreBroam$",
                "Error: Unknown Command:\n<%s>"), broam);
            return;
        }
        lParam = n;
    }
    else
    if (msg == BB_SHUTDOWN || msg == BB_QUIT)
    {
        // check for 'no confirmation' option
        lParam = 0 == memicmp(core_args, "-quiet", 2);
    }
    else
    if (msg & e_lparg) // needs argument
    {
        msg &= ~e_lparg; lParam = (LPARAM)core_args;
    }
    else
    if (msg == BB_MENU && wParam == BB_MENU_ROOT && core_args[0])
    {
        // "@BBCore.ShowMenu <argument>"
        wParam = BB_MENU_BYBROAM;
        lParam = (LPARAM)core_args;
    }
    else
    if (msg == BB_WORKSPACE)
    {
        if (wParam == BBWS_SWITCHTODESK)
            lParam = atoi(core_args)-1;
        else
        if (wParam == BBWS_MOVEWINDOWTOWS) 
            lParam = (LPARAM)core_args;
        else
        if (wParam & e_alt) // ...AllWorkspaces option
            wParam &= ~e_alt, lParam = 1;
    }
    else
    if (msg == BB_BRINGTOFRONT)
    {
        lParam = (LPARAM)GetTask(atoi(core_args)-1);
    }

    if (msg)
    {
        SendMessage(BBhwnd, msg, wParam, lParam);
        return;
    }

    switch(wParam)
    {
        case e_rootCommand:
            Desk_new_background(core_args);
            Menu_All_Redraw(0);
            break;

        case e_Message:
            BBMessageBox(MB_OK, "%s", core_args);
            break;

        case e_ShowAppnames:
            ShowAppnames();
            break;

        case e_About:
            bb_about();
            break;

        case e_Nop:
            break;

        case e_Crash:
            *(DWORD *)0x33333333 = 0x11111111;
            break;

        case e_Test:
            break;
    }
}

//===========================================================================

bool exec_script(const char *broam)
{
    char buffer[1024];
    if ('[' == *broam)
    {
        char *s = new_str(broam);
        StrRemoveEncap(s);
        const char *a = s;
        while (*NextToken(buffer, &a, "|")) exec_command(buffer);
        free_str(&s);
        return true;
    }
    return false;
}

//===========================================================================

bool exec_broam(const char *broam) // 'true' for done, 'false' for broadcast
{
    //dbg_printf("broam [%s]", broam);

    if (0 == memicmp(broam, "@BBCfg.", 7))
    {
        // broams from configuration menu
        exec_cfg_command(broam+7);
        return true;
    }

    if (0 == memicmp(broam, "@BBCore.", 8))
    {
        // broams from bbkeys or menu commands
        exec_core_broam(broam);
        return true;
    }

    if (0==memicmp(broam, "@Script", 7))
    {
        broam += 7 - 1; while (' ' == *++broam);
        exec_script(broam);
        return true;
    }

    if (0==stricmp(broam, "@BBHidePlugins"))
        Menu_All_Toggle(PluginsHidden = true);
    else
    if (0==stricmp(broam, "@BBShowPlugins"))
        Menu_All_Toggle(PluginsHidden = false);


	// Noccy: Broams to control screen saver

    if (0==stricmp(broam, "@DisableScreensaver"))
    {
		SetScreenSaverActive(false);
		return(true);
    }

    if (0==stricmp(broam, "@EnableScreensaver"))
    {
		SetScreenSaverActive(true);
		return(true);
    }

    if (0==stricmp(broam, "@ToggleScreensaver"))
    {
		Session_screensaverEnabled = (!GetScreenSaverActive());
		SetScreenSaverActive(Session_screensaverEnabled);
		return(true);
    }

    if (broam[0] == '@'){
        if (char *p = strrchr(broam, '.')){
            char DllName[256]; DllName[0] = '\0';
            strcpy_max(DllName, broam + 1, p - broam);
            if (0 == memicmp(p, ".VolumeAdd ", 11)){
                VolumeControl *pVolCtrl = new VolumeControl(DllName);
                if (pVolCtrl){
                    pVolCtrl->BBSetVolume(iminmax(pVolCtrl->BBGetVolume() + atoi(p+11), 0, 100));
                    delete pVolCtrl;
                    return true;
                }
            }
            if (0==stricmp(p, ".VolumeMute")){
                VolumeControl *pVolCtrl = new VolumeControl(DllName);
                if (pVolCtrl){
                    pVolCtrl->BBToggleMute();
                    delete pVolCtrl;
                    return true;
                }
            }
        }
    }

	return false;
}

//============================================================================
//
// Session Control (including screensaver)
//
//============================================================================

void Session_UpdateSession()
{
	Session_screensaverEnabled = GetScreenSaverActive();
}

void SetScreenSaverActive(bool state)
{
	SystemParametersInfo(SPI_SETSCREENSAVEACTIVE, state, 0, SPIF_SENDWININICHANGE);
}

bool GetScreenSaverActive()
{
	bool *state;
	SystemParametersInfo(SPI_GETSCREENSAVEACTIVE, 0, &state, 0);
	return(state);
}

void ToggleScreenSaverActive()
{
	SetScreenSaverActive(!(GetScreenSaverActive()));
}

//============================================================================
//
// Process Priority (by Eliteforce)
//
//============================================================================

void SetProcessPriority() {
	DWORD dwPriorityClass;

	// 0 = idle, 1 = below normal, 2 = normal, 3 = above normal, 4 = high, everything else = disabled
	switch (Settings_processPriority) {
		case 0:
			dwPriorityClass = IDLE_PRIORITY_CLASS;
			break;
		case 1:
			dwPriorityClass = BELOW_NORMAL_PRIORITY_CLASS;
			break;
		case 2:
			dwPriorityClass = NORMAL_PRIORITY_CLASS;
			break;
		case 3:
			dwPriorityClass = ABOVE_NORMAL_PRIORITY_CLASS;
			break;
		case 4:
			dwPriorityClass = HIGH_PRIORITY_CLASS;
			break;
		default:
			return;
	}

	SetPriorityClass(GetCurrentProcess(), dwPriorityClass);
}

//===========================================================================
//
// ShutdownWindows stuff....
//
//===========================================================================

static const char *shutdn_cmds_display[] =
{
    NLS0("shut down"),
    NLS0("reboot"),
    NLS0("log off"),
    NLS0("hibernate"),
    NLS0("suspend"),
    NLS0("lock workstation"),
    NLS0("exit windows")
};

void ShutdownWindows(int mode, int no_msg)
{
    switch (mode)
    {
        case BBSD_LOCKWS:
            BBExecute_string("rundll32.exe user32.dll,LockWorkStation", false);
            return;

        case BBSD_EXITWIN: // Standard Windows shutdown menu
            pMSWinShutdown(BBhwnd);
            return;

        case BBSD_SHUTDOWN  :
        case BBSD_REBOOT    :
        case BBSD_LOGOFF    :
        case BBSD_HIBERNATE :
        case BBSD_SUSPEND   :
        {
            if (0 == no_msg && IDYES != BBMessageBox(MB_YESNO ,
                NLS2("$BBShutdownQuery$", "Are you sure you want to %s?"),
                NLS1(shutdn_cmds_display[mode])
                )) return;
            break;
        }

        default:
            return;
    }

	//When shutting down, rebooting, or logging off completely unload blackbox and all plugins before..
	if ( mode == BBSD_SHUTDOWN || mode == BBSD_REBOOT || mode == BBSD_LOGOFF ) {
		/* kill_plugins(); */
		//FindWindowEx(NULL, hSlit, "BBSlit", NULL);
		SendMessage(BBhwnd, BB_EXITTYPE, 0, 0);
		Workspaces_GatherWindows();
		shutdown_blackbox();
		//PostQuitMessage(0);
	}

	DWORD tid;
	DWORD WINAPI ShutdownThread(void *pv);
    CloseHandle(CreateThread(NULL, 0, ShutdownThread, (LPVOID)mode, 0, &tid));
}

// ------------------------------------------------------------

DWORD WINAPI ShutdownThread(void *mode)
{
    if (usingNT && BBSD_LOGOFF != (int)mode)
    {
        // Under WinNT/2k/XP we need to adjust privileges to be able to
        // shutdown/reboot/hibernate/suspend...

        HANDLE hToken;
        bool success = false;

        // Get a token for this process...
        if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
        {
            TOKEN_PRIVILEGES tkp;

            // Get the LUID for the shutdown privilege...
            LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid); 
            tkp.PrivilegeCount = 1;  // one privilege to set    
            tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED; 

            // Get the shutdown privileges for this process...
            AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0); 
            success = GetLastError() == ERROR_SUCCESS;

            CloseHandle(hToken);
        }

        if (false == success)
        {
            BBMessageBox(MB_OK, NLS2("$BBError_AdjustPrivileges$",
                "Error: Failed to adjust shutdown privileges."));
            goto leave;
        }

    }

    switch ((int)mode)
    {
        case BBSD_SHUTDOWN:
				if (ExitWindowsEx(EWX_SHUTDOWN|EWX_POWEROFF, 0)) {
                goto leave;
				}
            break;

        case BBSD_REBOOT:
				if (ExitWindowsEx(EWX_REBOOT, 0)) {
                goto leave;
				}
            break;

        case BBSD_LOGOFF:
				if (ExitWindowsEx(EWX_LOGOFF, 0)) {
                PostMessage(BBhwnd, WM_QUIT, 1, 0);
                // exicode 1 lets bbrestart terminate too
                goto leave;
            }
            break;

        case BBSD_HIBERNATE:
            if (SetSystemPowerState(FALSE, FALSE))
                goto leave;
            break;

        case BBSD_SUSPEND:
            if (SetSystemPowerState(TRUE, FALSE))
                goto leave;
            break;
    }

    BBMessageBox(MB_OK, NLS2("$BBError_Shutdown$",
        "Error: Failed to %s."), NLS1(shutdn_cmds_display[(int)mode]));
leave:
    return 0;
}

//===========================================================================
//
// RunStartupStuff stuff....
//
//===========================================================================
#define RS_ONCE 1
#define RS_WAIT 2
#define RS_CHCK 4

bool RunEntriesIn (HKEY root_key, LPCSTR subpath, UINT flags)
{
	int index; HKEY hKey; bool ret = false; char path[100];
	typedef BOOL (WINAPI *LPFN_WOW64ENABLEREDIR)(BOOL);
	LPFN_WOW64ENABLEREDIR fnEnableRedir;

	sprintf(path, "Software\\Microsoft\\Windows\\CurrentVersion\\%s", subpath);
	for(int i=0;i<(usingx64?2:1);++i)
	{
		if (ERROR_SUCCESS != RegOpenKeyEx(root_key, path, 0, KEY_ALL_ACCESS|(i?KEY_WOW64_64KEY:KEY_WOW64_32KEY), &hKey))
			return ret;

		//if busy with x64 section, disable the filesystem redirection
		if (i==1)
		{
			fnEnableRedir = (LPFN_WOW64ENABLEREDIR)GetProcAddress(GetModuleHandle("kernel32"),"Wow64EnableWow64FsRedirection");
			fnEnableRedir(FALSE);
		}

		//log_printf(2, "\In Registry: %s\\%s", HKEY_CURRENT_USER == root_key ? "HKCU": "HKLM", path);
		for (index=0;;++index)
		{
			char szNameBuffer[200]; char szValueBuffer[1000];
			DWORD dwNameSize = sizeof szNameBuffer;
			DWORD dwValueSize = sizeof szValueBuffer;
			DWORD dwType;

			if (ERROR_SUCCESS != RegEnumValue(
					hKey, index,
					szNameBuffer, &dwNameSize,
					NULL, &dwType,
					(BYTE*)szValueBuffer, &dwValueSize
					)) break;

			ret = true;
			if (flags & RS_CHCK) break;

			//log_printf(2, "\t\tRunning: %s", szValueBuffer);
			WinExec(szValueBuffer, SW_SHOWNORMAL);

			if (flags & RS_ONCE)
				if (ERROR_SUCCESS == RegDeleteValue(hKey, szNameBuffer))
					--index;
		}
		RegCloseKey (hKey);

		//Enable filesystem redirection, if we disabled it;
		if (i==1)
			fnEnableRedir(TRUE);
	}
    return ret;
}

// ------------------------------------------

void RunFolderContents(LPCSTR szParams)
{
    char szPath[MAX_PATH];
    int x = strlen(add_slash(szPath, szParams));
    strcpy(szPath+x, "*.*");
    WIN32_FIND_DATA findData;
    HANDLE hSearch = FindFirstFile(szPath, &findData);
    if (hSearch == INVALID_HANDLE_VALUE) return;

    //log_printf(2, "\In startup folder: %s", szParams);
    do if (0 == (findData.dwFileAttributes & (
        FILE_ATTRIBUTE_SYSTEM
        |FILE_ATTRIBUTE_DIRECTORY
        |FILE_ATTRIBUTE_HIDDEN
        )))
    {
        strcpy(szPath+x, findData.cFileName);
        //log_printf(2, "\t\tRunning: %s", szPath);
        ShellExecute(NULL, NULL, szPath, NULL, NULL, SW_SHOWNORMAL);
    }
    while (FindNextFile(hSearch, &findData));
    FindClose(hSearch);
}

// ------------------------------------------

DWORD WINAPI RunStartupThread (void *pv)
{
    static const char szRun[] = "Run";
    static const char szRunOnce[] = "RunOnce";

	//log_printf(2, "Now Running Startup Items.");

	// RunOnceEx, any?
	static const char szRunOnceEx[] = "RunOnceEx";
	if (RunEntriesIn (HKEY_LOCAL_MACHINE, szRunOnceEx, RS_CHCK))
		ShellCommand("RunDLL32.EXE iernonce.dll,RunOnceExProcess", NULL, true);


    RunEntriesIn (HKEY_LOCAL_MACHINE, szRunOnce, RS_ONCE|RS_WAIT);
    RunEntriesIn (HKEY_LOCAL_MACHINE, szRun, 0);
    RunEntriesIn (HKEY_CURRENT_USER, szRun, 0);

    // Run startup items
    static const short startuptable[4] = {
        CSIDL_COMMON_STARTUP,       //0x0018,
        CSIDL_COMMON_ALTSTARTUP,    //0x001e,
        CSIDL_STARTUP,              //0x0007,
        CSIDL_ALTSTARTUP            //0x001d
    };

    char szPath[MAX_PATH]; int i;
    for (i = 0; i < 4; ++i)
        if (sh_getfolderpath(szPath, startuptable[i]))
            RunFolderContents(szPath);

    RunEntriesIn (HKEY_CURRENT_USER, szRunOnce, RS_ONCE);
    //log_printf(2, "Startup Finished.");
    return 0;
}

// ------------------------------------------

void RunStartupStuff(void)
{
    DWORD threadId;
    CloseHandle(CreateThread(NULL, 0, RunStartupThread, NULL, 0, &threadId));
}

//===========================================================================

