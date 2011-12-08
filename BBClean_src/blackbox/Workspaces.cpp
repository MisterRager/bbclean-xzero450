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
#include "Workspaces.h"
#include "Desk.h"
#include "BBVWM.h"

#define ST static


//====================
// public variables

int VScreenX;
int VScreenY;
int VScreenHeight;
int VScreenWidth;
int ScreenHeight;
int ScreenWidth;

int currentScreen;


//====================
// private variables

struct toptask {
    struct toptask *next;
    struct tasklist* task;
};

// workspace names
ST struct string_node *deskNames;

// application listed in StickyWindows.ini
ST struct string_node * stickyNamesList;

// the list of tasks, in order as they were added
ST struct tasklist  * taskList;

// the list of tasks, in order as they were recently active
ST struct toptask   * pTopTask;

//====================
// local functions

ST void WS_LoadStickyNamesList(void);
ST bool WS_IsSticky(HWND hwnd);
ST void WS_MakeItSticky(HWND hwnd);
ST void WS_ClearStickyness(HWND hwnd);

ST void DeskLeft(void);
ST void DeskRight(void);
ST void DeskSwitch(int);

ST void MoveWindowToWkspc(HWND, int);
ST void NextWindow(bool, int);

ST void switchToDesktop(int desk);
ST void setDesktop(HWND hwnd, int desk, bool switchto);
ST void MoveWindowToDirectWS(HWND hwnd, int ws_number, bool follow);

ST void EditWorkspaceNames(void);
ST void AddDesktop(int);
ST void SetNames(void);

ST void exit_tasks(void);
ST void init_tasks(void);

ST int Workspaces_getDesktop(HWND h);
ST bool is_valid_task(HWND hwnd);
ST int FindTask(HWND hwnd);
ST void SetTopTask(struct tasklist *, int f);
ST HWND search_top_window(int scrn, HWND hwnd_exclude);
ST bool focus_top_window_beside(HWND hwnd_exclude);
ST void post_desk_refresh(void);
ST void post_task_refresh(void);

ST void WS_ShadeWindow(HWND hwnd);
ST void WS_GrowWindowHeight(HWND hwnd);
ST void WS_GrowWindowWidth(HWND hwnd);
ST void WS_LowerWindow(HWND hwnd);
ST void WS_RaiseWindow(HWND hwnd);
ST void WS_MaximizeWindow(HWND hwnd);
ST void WS_RestoreWindow(HWND hwnd);
ST void WS_BringToFront(HWND hwnd, bool to_current);
ST void WS_AlwaysOnTopWindow(HWND hwnd);
ST void WS_SendStickyInfo(HWND hwnd);

ST void WS_Config(void);
ST void CleanTasks(void);

ST void MinimizeAllWindows(void);
ST void RestoreAllWindows(void);

//===========================================================================

void Workspaces_Init(void)
{
    currentScreen   = 0;
    deskNames       = NULL;
    stickyNamesList = NULL;
    WS_Config();
    vwm_init(Settings_altMethod, Settings_workspacesPCo);
    init_tasks();
    //SetTimer(BBhwnd, BB_CHECKWINDOWS_TIMER, 4000, NULL);
}

void Workspaces_Exit(void)
{
    KillTimer(BBhwnd, BB_CHECKWINDOWS_TIMER);
    exit_tasks();
    vwm_exit();
    freeall(&deskNames);
    freeall(&stickyNamesList);
}

void Workspaces_Reconfigure(void)
{
    WS_Config();
    vwm_reconfig(Settings_altMethod, Settings_workspacesPCo);
}

void Workspaces_handletimer(void)
{
    vwm_update_winlist();
    CleanTasks();
}

ST void WS_Config(void)
{
    ScreenWidth  = GetSystemMetrics(SM_CXSCREEN);
    ScreenHeight = GetSystemMetrics(SM_CYSCREEN);
    if (multimon)
    {
        VScreenWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
        VScreenHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);
        VScreenX = GetSystemMetrics(SM_XVIRTUALSCREEN);
        VScreenY = GetSystemMetrics(SM_YVIRTUALSCREEN);
    }
    else
    {
        VScreenWidth  = ScreenWidth;
        VScreenHeight = ScreenHeight;
        VScreenX = VScreenY = 0;
    }
    SetNames();
    WS_LoadStickyNamesList();
}

//===========================================================================

ST void EditWorkspaceNames(void)
{
    SwitchToBBWnd();
    if (IDOK == EditBox(
        "bbLean",
        NLS2("$BBWorkspace_EditNames$", "Workspace Names:"),
        Settings_workspaceNames,
        Settings_workspaceNames
        ))
    {
        Settings_WriteRCSetting(&Settings_workspaceNames);
        SetNames();
        post_desk_refresh();
    }
}

ST void SetNames(void)
{
    freeall(&deskNames);
    const char *names = Settings_workspaceNames; int i;
    for (i = 0; i < Settings_workspaces; ++i)
    {
        char wkspc_name[80];
        if (0 == *NextToken(wkspc_name, &names, ","))
            sprintf(wkspc_name, NLS2("$BBWorkspace_DefaultName$", "Workspace %d"), i+1);
        append_string_node(&deskNames, wkspc_name);
    }
}

//===========================================================================
ST HWND get_default_window(HWND hwnd)
{
    if (NULL == hwnd)
    {
        hwnd = GetForegroundWindow();
        if (NULL == hwnd || GetCurrentThreadId() == GetWindowThreadProcessId(hwnd, NULL))
        {
            hwnd = search_top_window(currentScreen, NULL);
            if (NULL == hwnd) return NULL;
			hwnd = GetLastActivePopup(GetRootWindow(hwnd));
            SwitchToWindow(hwnd);
        }
    }
    int n = Workspaces_getDesktop(hwnd);
    if (currentScreen != n && -1 != n) return NULL;
    return hwnd;
}

ST BOOL list_desktops_func(DesktopInfo * DI, LPARAM lParam)
{
    SendMessage((HWND)lParam, BB_DESKTOPINFO, 0, (LPARAM)DI);
    return TRUE;
}

/*
ST LRESULT send_syscommand(HWND hwnd, UINT SC_XXX)
{
	DWORD_PTR dwResult = 0;
	SendMessageTimeout(hwnd, WM_SYSCOMMAND, SC_XXX, 0, SMTO_NORMAL, 1000, &dwResult);
	return dwResult;
}
*/

// CyberShadow 2007.05.25: the result isn't required in most cases; post the message instead of sending it, to prevent temporary lock-ups
ST void post_syscommand(HWND hwnd, UINT SC_XXX)
{
    PostMessage(hwnd, WM_SYSCOMMAND, SC_XXX, 0);
}

void Workspaces_Command(UINT msg, WPARAM wParam, LPARAM lParam)
{
    HWND hwnd = (HWND)lParam;
    switch (msg)
    {
        case BB_SWITCHTON:
            switchToDesktop((int)lParam);
            break;

        case BB_LISTDESKTOPS:
            if (wParam) EnumDesks(list_desktops_func, wParam);
            break;

        //====================
        case BB_WORKSPACE:
            //dbg_printf("BB_WORKSPACE %d %x", wParam, hwnd);
            switch (wParam)
            {
                // ---------------------------------
                case BBWS_DESKLEFT: DeskLeft();
                    break;
                case BBWS_DESKRIGHT: DeskRight();
                    break;

                // ---------------------------------
                case BBWS_ADDDESKTOP: AddDesktop(1);
                    break;
                case BBWS_DELDESKTOP: AddDesktop(-1);
                    break;

                // ---------------------------------
                case BBWS_SWITCHTODESK:
                    DeskSwitch((int)lParam);
                    break;

                case BBWS_LASTDESK:
                    DeskSwitch(Settings_workspaces-1);
                    break;

                case BBWS_GATHERWINDOWS:
                    Workspaces_GatherWindows();
                    break;

                // ---------------------------------
                case BBWS_MOVEWINDOWLEFT:
                    MoveWindowToWkspc(get_default_window(hwnd), -1);
                    break;

                case BBWS_MOVEWINDOWRIGHT:
                    MoveWindowToWkspc(get_default_window(hwnd), 1);
                    break;

                case BBWS_MOVEWINDOWTOWS:
                    const char *args;
                    bool follow;
                    int ws_number;
                    char ws[10];

                    hwnd = get_default_window(NULL);
                    if (NULL==hwnd) break;
                    
//                     dbg_printf("BBWS_MOVEWINDOWTOWS hwnd=%x, lParam=%s", hwnd, lParam); 
                    
                    args = (const char *)lParam;
                    NextToken(ws, &args);
                    ws_number = atoi(ws)-1;
                    
					if (*args)
						follow = !stricmp(args, "true");
					else
						follow = Settings_followMoved;

					MoveWindowToDirectWS(hwnd, ws_number, follow);
                    break;

                // ---------------------------------
                case BBWS_PREVWINDOW:
                    NextWindow((bool)lParam, -1);
                    break;

                case BBWS_NEXTWINDOW:
                    NextWindow((bool)lParam, 1);
                    break;

                // ---------------------------------
                case BBWS_TOGGLESTICKY:
                    hwnd = get_default_window(hwnd);
                    if (NULL==hwnd) break;
                    if (WS_IsSticky(hwnd))
                        goto case_BBWS_CLEARSTICKY;

                case BBWS_MAKESTICKY:
                    hwnd = get_default_window(hwnd);
                    if (hwnd) WS_MakeItSticky(hwnd);
                    break;

                case_BBWS_CLEARSTICKY:
                case BBWS_CLEARSTICKY:
                    hwnd = get_default_window(hwnd);
                    if (hwnd) WS_ClearStickyness(hwnd);
                    break;

                // ---------------------------------
                case BBWS_EDITNAME:
                    EditWorkspaceNames();
                    break;

                case BBWS_MINIMIZEALL:
                    MinimizeAllWindows();
                    break;

                case BBWS_RESTOREALL:
                    RestoreAllWindows();
                    break;

                case BBWS_CASCADE:
                    CascadeWindows(NULL, 0, NULL, 0, NULL);
                    break;

                case BBWS_TILEVERTICAL:
                    TileWindows(NULL, MDITILE_VERTICAL, NULL, 0, NULL);
                    break;

                case BBWS_TILEHORIZONTAL:
                    TileWindows(NULL, MDITILE_HORIZONTAL, NULL, 0, NULL);
                    break;

				default:
					break;
            }
            break;

        //====================
        case BB_BRINGTOFRONT:
            WS_BringToFront(hwnd, wParam & BBBTF_CURRENT);
            break;

        //====================
        default:
        {
            hwnd = get_default_window(hwnd);
            if (NULL == hwnd) break;
            LONG style = GetWindowLong(hwnd, GWL_STYLE);
            switch (msg)
            {
                case BB_WINDOWMINIMIZE:
                    if (0 == (WS_MINIMIZEBOX & style)) break;
					post_syscommand(hwnd, SC_MINIMIZE);
                    focus_top_window_beside(hwnd);
                    break;

                case BB_WINDOWRESTORE:
                    WS_RestoreWindow(hwnd);
                    break;

                case BB_WINDOWCLOSE:
					post_syscommand(hwnd, SC_CLOSE);
                    focus_top_window_beside(hwnd);
                    break;

                case BB_WINDOWMAXIMIZE:
                    if (0 == (WS_MAXIMIZEBOX & style)) break;
                    WS_MaximizeWindow(hwnd);
                    break;

                case BB_WINDOWGROWHEIGHT:
                    if (0 == (WS_MAXIMIZEBOX & style)) break;
                    WS_GrowWindowHeight(hwnd);
                    break;

                case BB_WINDOWGROWWIDTH:
                    if (0 == (WS_MAXIMIZEBOX & style)) break;
                    WS_GrowWindowWidth(hwnd);
                    break;

                case BB_WINDOWLOWER:
                    WS_LowerWindow(hwnd);
                    break;

                case BB_WINDOWRAISE:
                    WS_RaiseWindow(hwnd);
                    break;

                case BB_WINDOWSHADE:
                    if (0 == (WS_SIZEBOX & style)) break;
                    WS_ShadeWindow(hwnd);
                    break;

                case BB_WINDOWSIZE:
                    if (0 == (WS_SIZEBOX & style)) break;
					post_syscommand(hwnd, SC_SIZE);
                    break;

                case BB_WINDOWMOVE:
					post_syscommand(hwnd, SC_MOVE);
				case BB_WINDOWAOT:
					WS_AlwaysOnTopWindow(hwnd);
				//This forces leanSkin to update the window/buttons
					WS_SendStickyInfo(hwnd);
					break;
			}
        }
    }
}

//===========================================================================

ST void WS_BringToFront(HWND hwnd, bool to_current)
{
    if (false == is_valid_task(hwnd))
    {
        CleanTasks();
        return;
    }

    int n = vwm_get_desk(hwnd);
    if (n != currentScreen)
    {
        if (false == to_current
            && (FALSE == Settings_restoreToCurrent || FALSE == IsIconic(hwnd)))
            switchToDesktop(n);
        else
            setDesktop(hwnd, currentScreen, false);
    }
    SwitchToWindow(hwnd);
}

//===========================================================================
static const char BBSHADE_PROP[] = "BBNormalHeight";
#define BBLS_GETSHADEHEIGHT 1

ST void get_rect(HWND hwnd, RECT *rp)
{
    GetWindowRect(hwnd, rp);
    if (WS_CHILD & GetWindowLong(hwnd, GWL_STYLE))
    {
        HWND pw = GetParent(hwnd);
        ScreenToClient(pw, (LPPOINT)&rp->left);
        ScreenToClient(pw, (LPPOINT)&rp->right);
    }
}

ST void window_set_pos(HWND hwnd, RECT rc)
{
    int width = rc.right - rc.left;
    int height = rc.bottom - rc.top;
    SetWindowPos(hwnd, NULL,
        rc.left, rc.top, width, height,
        SWP_NOZORDER|SWP_NOACTIVATE);
}

ST int get_shade_height(HWND hwnd)
{
    int shade = SendMessage(hwnd,
        RegisterWindowMessage("BBLEANSKIN_MSG"),
        BBLS_GETSHADEHEIGHT, 0);

    if (shade) return shade;

    int border = GetSystemMetrics(
        (WS_SIZEBOX & GetWindowLong(hwnd, GWL_STYLE))
        ? SM_CYFRAME
        : SM_CYFIXEDFRAME);

    int caption = GetSystemMetrics(
        (WS_EX_TOOLWINDOW & GetWindowLong(hwnd, GWL_EXSTYLE))
        ? SM_CYSMCAPTION
        : SM_CYCAPTION);

    //dbg_printf("caption %d  border %d", caption, border);
    return 2*border + caption;
}

ST void WS_ShadeWindow(HWND hwnd)
{
    RECT rc; get_rect(hwnd, &rc);
    int height = rc.bottom - rc.top;
    DWORD prop = (DWORD)GetProp(hwnd, BBSHADE_PROP);

    int h1 = LOWORD(prop);
    int h2 = HIWORD(prop);
    if (IsZoomed(hwnd))
    {
        if (h2) height = h2, h2 = 0;
        else h2 = height, height = get_shade_height(hwnd);
    }
    else
    {
        if (h1) height = h1, h1 = 0;
        else h1 = height, height = get_shade_height(hwnd);
        h2 = 0;
    }

    prop = MAKELPARAM(h1, h2);
    if (0 == prop) RemoveProp(hwnd, BBSHADE_PROP);
    else SetProp(hwnd, BBSHADE_PROP, (PVOID)prop);

    rc.bottom = rc.top + height;
    window_set_pos(hwnd, rc);
}

#define aot		"AOT"
ST void WS_AlwaysOnTopWindow ( HWND hwnd ) {
	void	*info = GetProp(hwnd, aot);
	
	if ( info ) {
		RemoveProp( hwnd, aot );

		SetWindowPos( hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE );
	} else {
		SetProp( hwnd, aot, (HANDLE)(TRUE) );

		SetWindowPos( hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE );
	}

}
ST bool check_for_restore(HWND hwnd)
{
    if (FALSE == IsZoomed(hwnd))
    {
		return false;
	}
	post_syscommand(hwnd, SC_RESTORE);

	// restore the default maxPos (necessary when it was V-max'd or H-max'd)
	WINDOWPLACEMENT wp;
	wp.length = sizeof wp;
	GetWindowPlacement(hwnd, &wp);
	wp.ptMaxPosition.x =
	wp.ptMaxPosition.y = -1;
	SetWindowPlacement(hwnd, &wp);
	return true;
}

ST void grow_window(HWND hwnd, bool v)
{
    if (check_for_restore(hwnd)) return;
    RECT r; get_rect(hwnd, &r);
    LockWindowUpdate(hwnd);
	post_syscommand(hwnd, SC_MAXIMIZE);
    RECT r2; get_rect(hwnd, &r2);
    if (v) r.top = r2.top, r.bottom = r2.bottom;
    else r.left = r2.left, r.right = r2.right;
    window_set_pos(hwnd, r);
    LockWindowUpdate(NULL);
}

ST void WS_GrowWindowHeight(HWND hwnd)
{
    grow_window(hwnd, true);
}

ST void WS_GrowWindowWidth(HWND hwnd)
{
    grow_window(hwnd, false);
}

ST void WS_MaximizeWindow(HWND hwnd)
{
    if (check_for_restore(hwnd)) return;
	post_syscommand(hwnd, SC_MAXIMIZE);
}

ST void WS_RestoreWindow(HWND hwnd)
{
    if (check_for_restore(hwnd)) return;
	post_syscommand(hwnd, SC_RESTORE);
}

//===========================================================================

ST void WS_LowerWindow(HWND hwnd)
{
    struct tasklist *tl = (struct tasklist *)assoc(taskList, hwnd);
    if (NULL == tl) return;
    SetTopTask(tl, 2); // append
    SwitchToBBWnd();
    SetWindowPos(hwnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE|SWP_NOSENDCHANGING);
    focus_top_window();
}

//====================

ST void WS_RaiseWindow(HWND hwnd_notused)
{
    struct tasklist *tl = NULL;
    struct toptask *lp;
    dolist (lp, pTopTask)
        if (currentScreen == Workspaces_getDesktop(lp->task->hwnd))
            tl = lp->task;
    if (tl) WS_BringToFront(tl->hwnd, false);
}

//================================================================
// Sticky functions - add a window and its popups to the internal
// sticky list, which is different to the API sticky list for the
// BB-(plugin-)windows

ST void WS_SendStickyInfo(HWND hwnd)
{
    PostMessage(BBhwnd, BB_REDRAWGUI, WS_IsSticky(hwnd)
            ? BBRG_WINDOW|BBRG_STICKY|BBRG_FOCUS
            : BBRG_WINDOW|BBRG_STICKY,
            (LPARAM)hwnd
            );
}

ST void WS_MakeItSticky(HWND hwnd)
{
    vwm_make_sticky(hwnd, true);
    WS_SendStickyInfo(hwnd);
}

ST void WS_ClearStickyness(HWND hwnd)
{
    vwm_make_sticky(hwnd, false);
    WS_SendStickyInfo(hwnd);
}

ST bool WS_IsSticky(HWND hwnd)
{
    return VWM_STICKY & vwm_get_status(hwnd);
}

//===========================================================================
// For application windows listed in 'StickyWindows.ini'

// export to BBVWM.cpp
bool check_sticky_name(HWND hwnd)
{
    char appName[MAX_PATH];
    if (NULL == stickyNamesList || 0 == GetAppByWindow(hwnd, appName))
        return false;

    //check appnames from "StickyWindows.ini"
    struct string_node * sl;
    dolist (sl, stickyNamesList)
        if (0==stricmp(appName, sl->str))
        {
            //dbg_printf("sticky: %s", appName);
            return true;
        }
    return false;
}

ST void WS_LoadStickyNamesList()
{
    char path[MAX_PATH]; char buffer[256];
    freeall(&stickyNamesList);
    FindConfigFile(path, "StickyWindows.ini", NULL);
    FILE* fp = FileOpen(path);
    if (fp)
    {
        while (ReadNextCommand(fp, buffer, sizeof (buffer)))
            append_string_node(&stickyNamesList, buffer);
        FileClose(fp);
    }
}

//===========================================================================
// Functions: Minimize/Restore All Windows
//===========================================================================

ST BOOL CALLBACK enumproc(HWND hwnd, LPARAM lParam)
{
    struct tasklist *tl = (struct tasklist *)assoc(taskList, hwnd);
    if (tl && currentScreen == tl->wkspc)
        cons_node ((void**)lParam, new_node(hwnd));
    return TRUE;
}

ST void min_rest_helper (LPARAM cmd)
{
    list_node *p, *lp = NULL;
    EnumWindows(enumproc, (LPARAM)&lp);
    if (SC_MINIMIZE == cmd) reverse_list(&lp);
    dolist (p, lp)
    {
        HWND hwnd = (HWND)p->v;
        if ((cmd == SC_MINIMIZE) == (FALSE == IsIconic(hwnd)))
        {
			post_syscommand(hwnd, cmd);
            if (SC_RESTORE == cmd) SwitchToWindow(hwnd);
        }
    }
    freeall(&lp);
}

ST void MinimizeAllWindows(void)
{
    min_rest_helper(SC_MINIMIZE);
    SwitchToBBWnd();
}

ST void RestoreAllWindows(void)
{
    min_rest_helper(SC_RESTORE);
}

//===========================================================================
// Switch the focus, even it another window has the focus.

void ForceForegroundWindow(HWND theWin)
{
    DWORD ThreadID1, ThreadID2;
    HWND fw = GetForegroundWindow();
    if(theWin == fw) return; // Nothing to do if already in foreground
    ThreadID1 = GetWindowThreadProcessId(fw, NULL);
    ThreadID2 = GetCurrentThreadId();
    if(ThreadID1 != ThreadID2) AttachThreadInput(ThreadID1, ThreadID2, TRUE);
    SetForegroundWindow(theWin);
    if(ThreadID1 != ThreadID2) AttachThreadInput(ThreadID1, ThreadID2, FALSE);
}

//===========================================================================

void SwitchToWindow (HWND hwnd)
{
	HWND hwndLAP = GetLastActivePopup(GetRootWindow(hwnd));
    if (IsWindowVisible(hwndLAP)) hwnd = hwndLAP;

    if (pSwitchToThisWindow)
    {
        // this one also automatically restores the window, if it's iconic:
        pSwitchToThisWindow(hwnd, 1);
    }
    else
    {
		if (IsIconic(hwnd)) post_syscommand(hwnd, SC_RESTORE);
        SetForegroundWindow(hwnd);
    }

}

void SwitchToBBWnd (void)
{
    ForceForegroundWindow(BBhwnd);
}

//================================================================
// retrieve the topwindow in the z-order of the current workspace,
// but not 'hw0'

ST HWND search_top_window(int scrn, HWND hwnd_exclude)
{
    struct toptask *lp;
    dolist (lp, pTopTask)
    {
        HWND hw = lp->task->hwnd;
        if (hwnd_exclude != hw && scrn == Workspaces_getDesktop(hw))
            return hw;
    }
    return NULL;
}

//================================================================
// activate the topwindow in the z-order of the current workspace
ST bool focus_top_window_beside(HWND hwnd_exclude)
{
    HWND hw = search_top_window(currentScreen, hwnd_exclude);
    if (hw) { SwitchToWindow(hw); return true; }
    SwitchToBBWnd(); return false;
}

bool focus_top_window(void)
{
    return focus_top_window_beside(NULL);
}

//================================================================
// is this window a valid task

ST bool is_valid_task(HWND hwnd)
{
    if (FALSE == IsWindow(hwnd)) return false;
    if (FALSE == IsWindowVisible(hwnd))
    {
        if (false == Settings_altMethod || false == (vwm_get_status(hwnd) & VWM_MOVED))
            return false;
    }
    return true;
}

//===========================================================================
void get_desktop_info(DesktopInfo *deskInfo, int i)
{
    deskInfo->isCurrent = i == currentScreen;
    deskInfo->number = i;
    deskInfo->name[0] = 0;
    deskInfo->ScreensX = Settings_workspaces;
    deskInfo->deskNames = deskNames;
    struct string_node *sp = (struct string_node*)nth_node(deskNames, i);
    if (sp) strcpy(deskInfo->name, sp->str);
}

//===========================================================================

ST void post_desk_refresh(void)
{
    static DesktopInfo DI;
    get_desktop_info(&DI, currentScreen);
    PostMessage(BBhwnd, BB_DESKTOPINFO, 0, (LPARAM)&DI);
    PostMessage(BBhwnd, BB_REDRAWTASK, 0, 0); // for bbpager
}

ST void post_task_refresh(void)
{
    PostMessage(BBhwnd, BB_TASKSUPDATE, 0, TASKITEM_REFRESH);
}

//===========================================================================
// gather windows,
// n = 0: gather in current WS,
// n =-1: gather from removed last WS
// n = 1: adjust workspaces for newly added WS

void Workspaces_GatherWindows(void)
{   
    vwm_gather();
    focus_top_window();
    post_task_refresh();
}

//===========================================================================
// get the desktop number for hwnd, or -1 if not found or valid
ST int Workspaces_getDesktop(HWND hwnd)
{
    if (IsIconic(hwnd)) return -1;
    return vwm_get_desk(hwnd);
}

//===========================================================================
// the internal switchToDesktop

ST void switchToDesktop(int n)
{
    int c = currentScreen;
    if (n < 0 || n >= Settings_workspaces) return;
    vwm_switch_desk(n);
    if (c != currentScreen) post_desk_refresh();
}

ST void setDesktop(HWND hwnd, int n, bool switchto)
{
    int c = currentScreen;
    if (n < 0 || n >= Settings_workspaces) return;
    vwm_set_desk (hwnd, n, switchto);
    if (switchto && c != currentScreen) post_desk_refresh();
}

//===========================================================================
ST int next_desk (int d)
{
    int n = currentScreen + d;
    int m = Settings_workspaces-1;
    if (n > m) return 0;
    if (n < 0) return m;
    return n;
}

ST void AddDesktop(int d)
{
    int s = Settings_workspaces + d;
    if (s < 1) return;
    Settings_workspaces = s;
    Settings_WriteRCSetting(&Settings_workspaces);
    Workspaces_Reconfigure();
    post_desk_refresh();
}

//====================

ST void DeskSwitch(int i)
{   
    HWND hwnd = search_top_window(i, NULL);
    switchToDesktop(i);
    if (NULL == hwnd)
        SwitchToBBWnd();
    else
        SwitchToWindow(hwnd);
}

//====================
ST void DeskLeft(void)
{
    DeskSwitch(next_desk(-1));
}

//====================

ST void DeskRight(void)
{
    DeskSwitch(next_desk(1));
}

//====================

ST void MoveWindowToWkspc(HWND hwnd, int where)
{
    if (NULL == hwnd) return;
    RemoveSticky(hwnd); // BBEdgeFlip hack
    setDesktop(hwnd, next_desk(where), true);
    SwitchToWindow(hwnd);
}

ST void MoveWindowToDirectWS(HWND hwnd, int ws_number, bool follow)
{
	RemoveSticky(hwnd); // BBEdgeFlip hack
	setDesktop(hwnd, ws_number, follow);

	if (follow)
		SwitchToWindow(hwnd);
	else
	{
		post_task_refresh();
		post_desk_refresh();
		//hack to make SystemBarEx behave correctly
		PostMessage(BBhwnd, BB_TASKSUPDATE, 0, TASKITEM_REMOVED);
	}
}

//====================

ST void NextWindow(bool allDesktops, int dir)
{
    HWND hw; int s,i,j, d;
    s = GetTaskListSize();  if (0==s) return;
    i = FindTask(GetTopTask());  if (-1==i) i=0;
    j = i;
    for (;;)
    {
        if (dir>0) {
            i++;
            if (s==i) i=0;
        } else {
            if (0==i) i=s;
            i--;
        }
        d = Workspaces_getDesktop(hw = GetTask(i));
        if (-1 != d && (allDesktops || currentScreen == d))
            break;

        if (j==i) return;
    }
    WS_BringToFront(hw, false);
}

//===========================================================================

//===========================================================================
// Task - Support

ST void del_from_toptasks(struct tasklist *tl)
{
    struct toptask **lpp, *lp;
    lpp = (struct toptask**)assoc_ptr(&pTopTask, tl);
    if (lpp) *lpp=(lp=*lpp)->next, m_free(lp);
}

ST void SetTopTask(struct tasklist *tl, int set_where)
{
    if (NULL==tl) return;
    del_from_toptasks(tl);

    //if (0==set_where) ;// delete_only

    if (1==set_where)   // push at front
        cons_node(&pTopTask, new_node(tl));

    if (2==set_where)   // push at end
        append_node(&pTopTask, new_node(tl));
}

HWND GetTopTask(void)
{
    return pTopTask ? pTopTask->task->hwnd : NULL;
}

//==================================

ST void get_ico(struct tasklist *tl)
{
	HWND hwnd = tl->hwnd;
	HICON hIco = NULL, hIco_big = NULL;
	// Noccy 2007-06-06: Changed waiting time from 500ms to 2s to hopefully avoid icon garbling. This may
	// cause issues with hung applications.
	SendMessageTimeout(hwnd, WM_GETICON, ICON_SMALL, 0, SMTO_ABORTIFHUNG|SMTO_NORMAL, 2000, (DWORD_PTR*)&hIco);
	if (NULL==hIco) hIco = (HICON)GetClassLong(hwnd, GCLP_HICONSM);
	if (tl->orig_icon != hIco && NULL != hIco)
	{
		if (tl->icon && tl->orig_icon_big != tl->orig_icon) DestroyIcon(tl->icon);
		tl->icon = CopyIcon(tl->orig_icon = hIco);
	}

	hIco_big = NULL;
	SendMessageTimeout(hwnd, WM_GETICON, ICON_BIG, 0, SMTO_ABORTIFHUNG|SMTO_NORMAL, 500, (DWORD_PTR*)&hIco_big);
	if (NULL==hIco_big) hIco_big = (HICON)GetClassLong(hwnd, GCLP_HICON);
	if (tl->orig_icon_big != hIco_big && NULL != hIco_big)
	{
		if (tl->icon_big && tl->orig_icon_big != tl->orig_icon) DestroyIcon(tl->icon_big);
		tl->icon_big = CopyIcon(tl->orig_icon_big = hIco_big);
	}
	if (tl->orig_icon == NULL)
	{
		if (tl->orig_icon_big != NULL)
			tl->icon = tl->icon_big;
	}
	else
	{
		if (tl->orig_icon_big == NULL)
			tl->icon_big = tl->icon;
	}
}

ST void get_text(struct tasklist *tl)
{
    GetWindowText(tl->hwnd, tl->caption, sizeof(tl->caption));
}

//==================================

ST struct tasklist *AddTask(HWND hwnd)
{
    struct tasklist *tl = (struct tasklist *)c_alloc(sizeof (struct tasklist));
    tl->hwnd = hwnd;
    tl->wkspc = currentScreen;
    append_node(&taskList, tl);
    get_text(tl);
    get_ico(tl);
    PostMessage(BBhwnd, BB_TASKSUPDATE, (WPARAM)hwnd, TASKITEM_ADDED);
    return tl;
}

ST void RemoveTask(struct tasklist *tl)
{
    if (tl->icon) DestroyIcon(tl->icon);
	if (tl->icon_big) DestroyIcon(tl->icon_big);
    del_from_toptasks(tl);
    HWND hwnd = tl->hwnd;
    remove_item(&taskList, tl);
    PostMessage(BBhwnd, BB_TASKSUPDATE, (WPARAM)hwnd, TASKITEM_REMOVED);
}

ST int FindTask(HWND hwnd)
{
    struct tasklist *tl; int i = 0;
    dolist(tl, taskList) { if (tl->hwnd == hwnd) return i; i++; }
    return -1;
}

// run through the tasklist and remove invalid tasks
ST void CleanTasks(void)
{
    struct tasklist **tl = &taskList;
    while (*tl)
    {
        if (is_valid_task((*tl)->hwnd))
            tl = &(*tl)->next;
        else
            RemoveTask(*tl);
    }
}

//==================================
ST BOOL CALLBACK TaskProc(HWND hwnd, LPARAM lParam)
{
    if(IsAppWindow(hwnd))
        SetTopTask(AddTask(hwnd), 2);
    return TRUE;
}

ST void init_tasks(void)
{
    EnumWindows((WNDENUMPROC)TaskProc, 0);
    focus_top_window();
}

ST void exit_tasks(void)
{
    while (taskList) RemoveTask(taskList);
}

//===========================================================================
#if 0
ST void debug_tasks(WPARAM wParam, HWND hwnd)
{
    int n = wParam;
    int task = FindTask(hwnd);
    char buffer[256];
    GetWindowModuleFileName(hwnd, buffer, 256);
    const char *p = get_file(buffer);
    bool BBA = false;
    if (NULL == hwnd)
    {
        DWORD tid = GetWindowThreadProcessId(GetForegroundWindow(), NULL);
        if (GetCurrentThreadId() == tid)
            BBA = true;
    }

    static char *msg[8] = { "null", "add", "remove", "activateshell", "activate", "minmax", "redraw", "taskman" };

    dbg_printf("[msg %d %s] hwnd=%x task=%d app=%s BBActive=%d", n, n<8 ? msg[n] : "xxx", hwnd, task, p, BBA);
}
#define DEBUG_TASKS
#endif

//===========================================================================
// called from the main windowproc on the registered WM_ShellHook message

void TaskWndProc(WPARAM wParam, HWND hwnd)
{
    bool extra = wParam & 0x8000;
    int code = wParam & 0x7FFF;
    struct tasklist *tl = NULL;

    if (hwnd)
    {
        if (BBhwnd == hwnd)
        {
            hwnd = NULL;
        }
        else
        {
            tl = (struct tasklist *)assoc(taskList, hwnd);
            if (tl) tl->flashing = false;
        }
    }

#ifdef DEBUG_TASKS
    debug_tasks(wParam, hwnd);
#endif

    switch (code)
    {
        //====================
        case HSHELL_WINDOWCREATED:
            // windows reshown by the vwm also trigger the HSHELL_WINDOWCREATED
            if (hwnd && NULL == tl)
            {
                AddTask(hwnd);
            }
            break;

        //====================
        case HSHELL_WINDOWDESTROYED:
            // windows hidden by the vwm also trigger the HSHELL_WINDOWDESTROYED
            if (tl && false == is_valid_task(hwnd))
            {
                RemoveTask(tl);
            }
            break;

        //====================
        case HSHELL_WINDOWACTIVATED:
        hshell_windowactivated:
        {
            //if (extra) ...; // on fullscreen windows, which are not maximized

            // the current active taskwindow or NULL
            static HWND activeWindow;

            HWND prev_act_window = activeWindow;
            activeWindow = hwnd;

            struct tasklist *tl2;
            dolist (tl2, taskList) tl2->active = false;

            if (tl)
            {
                tl->active = true;
                bool was_iconic = VWM_ICONIC & vwm_get_status(hwnd);
                int n = vwm_get_desk(hwnd);

                if (currentScreen != n)
                {
                    if (was_iconic && Settings_restoreToCurrent)
                    {
                        setDesktop(hwnd, currentScreen, false);
                    }
                    else
                    if (Settings_followActive
                        && -1 != FindTask(prev_act_window)
                        && FALSE == IsIconic(prev_act_window)
                        )
                    {
                        setDesktop(hwnd, n, true);
                    }
                    else
                    if (currentScreen == vwm_get_desk(GetLastActivePopup(hwnd)))
                    {
                        // app on other workspaces has popup'd something spontaneously,
                        // we put the popup on the other WS and switch to
                        setDesktop(hwnd, n, true);
                    }
                    else
                    {
                        setDesktop(hwnd, n, false);
                        SwitchToBBWnd();
                        break;
                    }
                }

                // ----------------------------------------
                // insert at head of the 'activated windows' list

                SetTopTask(tl, 1);

                // ----------------------------------------
                // try to grab title & icon, if still missing

                if (0 == tl->caption[0])
                    get_text(tl);

				if (NULL == tl->icon && NULL == tl->icon_big)
                    get_ico(tl);
            }

            PostMessage(BBhwnd, BB_TASKSUPDATE, (WPARAM)hwnd, TASKITEM_ACTIVATED);
            break;
        }

        //====================
        case HSHELL_REDRAW:
            if (tl)
            {
                get_text(tl);
                get_ico(tl); // disable for foobar delay issue ?

                UINT tbmsg = TASKITEM_MODIFIED;
                if (extra)
                {
                    tbmsg = TASKITEM_FLASHED;
                    tl->flashing = true;
                }

                if (false == tl->active && GetForegroundWindow() == hwnd)
                    goto hshell_windowactivated;

                PostMessage(BBhwnd, BB_TASKSUPDATE, (WPARAM)hwnd, tbmsg);
            }
            break;
    /*
        //====================
        case HSHELL_ACTIVATESHELLWINDOW: // never seen
            break;

        case HSHELL_GETMINRECT:          // never seen
            break;
    */
    }
}

//===========================================================================

//===========================================================================
// API: GetTaskListSize - returns the number of currently registered tasks

int GetTaskListSize(void)
{
    return listlen(taskList);
}

//===========================================================================
// API: GetTaskListPtr - returns the raw task-list

struct tasklist *GetTaskListPtr(void)
{
    struct tasklist *tl;
    dolist (tl, taskList) tl->wkspc = vwm_get_desk(tl->hwnd);
    return taskList;
}

//===========================================================================
// API: GetTask - returns the HWND of the task by index

HWND GetTask(int index)
{
    if (index >= 0)
    {
        struct tasklist *tl = (struct tasklist *)nth_node(taskList, index);
        if (tl) return tl->hwnd;
    }
    return NULL;
}

//===========================================================================
// API: GetActiveTask - returns index of current active task or -1, if none or BB

int GetActiveTask(void)
{
    struct tasklist *tl; int i = 0;
    dolist (tl, taskList) { if (tl->active) return i; i++; }
    return -1;
}

//===========================================================================
// API: GetTaskWorkspace - returns the workspace of the task by HWND

int GetTaskWorkspace(HWND hwnd)
{
    return assoc(taskList, hwnd) ? vwm_get_desk(hwnd) : -1;
}

//===========================================================================
// API: SetTaskWorkspace - set the workspace of the task by HWND

void SetTaskWorkspace(HWND hwnd, int wkspc)
{
    vwm_set_workspace(hwnd, wkspc);
}

//===========================================================================
// API: GetTaskLocation - retrieve the desktop and the original coords for a window

bool GetTaskLocation(HWND hwnd, struct taskinfo *t)
{
    return vwm_get_location(hwnd, t);
}

//===========================================================================
// API: SetTaskLocation - move a window and it's popups to another desktop and/or position

bool SetTaskLocation(HWND hwnd, struct taskinfo *t, UINT flags)
{
    int c = currentScreen;
    bool is_top = pTopTask && hwnd == pTopTask->task->hwnd;
    if (false == vwm_set_location(hwnd, t, flags))
        return false;

    if (is_top && vwm_get_desk(hwnd) != currentScreen)
        focus_top_window();

    if (c != currentScreen) post_desk_refresh();
    post_task_refresh();
    return true;
}

//===========================================================================
// API: EnumTasks

void EnumTasks (TASKENUMPROC lpEnumFunc, LPARAM lParam)
{
    struct tasklist *tl;
    dolist (tl, taskList)
    {
        tl->wkspc = vwm_get_desk(tl->hwnd);
        if (FALSE == lpEnumFunc(tl, lParam))
            break;
    }
}

//===========================================================================
// API: GetDesktopInfo

void GetDesktopInfo(DesktopInfo *deskInfo)
{
    get_desktop_info(deskInfo, currentScreen);
}

//===========================================================================
// API: EnumDesks

void EnumDesks (DESKENUMPROC lpEnumFunc, LPARAM lParam)
{
    for (int n = 0; n < Settings_workspaces; n++)
    {
		DesktopInfo DI;
		get_desktop_info(&DI, n);
        if (FALSE == lpEnumFunc(&DI, lParam))
            break;
    }
}


//===========================================================================


