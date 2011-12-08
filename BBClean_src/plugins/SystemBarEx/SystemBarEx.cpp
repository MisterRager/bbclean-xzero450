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
#include "sbex.h"
#include "SystemBarEx.h"

//bool multipleInstances = false;
//int	numberInstances = 0;
bool dragging = false;
int	hoverTime = 0;
int	hoverSecond = 0;
int mousex = 0;
int mousey = 0;

//exclusionsList_t	sysTrayExList[24];

//char appName[MINI_MAX_LINE_LENGTH];
TaskbarMenuIndex     TBMenuInfo;

class TinyDropTarget *m_TinyDropTarget;
class TinyDropTarget *init_drop_targ(HWND hwnd);
void exit_drop_targ(class TinyDropTarget *m_TinyDropTarget);
//void handle_task_timer(class TinyDropTarget *m_TinyDropTarget);
//HWND task_over_hwnd;

void ShowSysmenu(HWND Window, HWND hSystembarExWnd);

//-----------------------------------------------------------------------------
int beginSlitPlugin(HINSTANCE hPluginInstance, HWND hBBSlit) {
    bInSlit = true;
    hSlitWnd = hBBSlit;
    return beginPlugin(hPluginInstance);
}

//-----------------------------------------------------------------------------

int beginPlugin (HINSTANCE hPluginInstance) {
    BYTE		i = 0;
    WNDCLASS	wc;
	FILE* fp;

	//int		k = 0;
	//int		instances = 0;

	//if ( FindWindow(ModInfo.Get(ModInfo.APPNAME), 0) ) {
	//	instances++;
	//}
	/* loop to find all instances */
	//for ( k = 0; k < 10; k++ ) {
	//	char temp[MINI_MAX_LINE_LENGTH];
	//	memset(&temp, 0, sizeof(temp));
	//	sprintf(temp, "%s%i", ModInfo.Get(ModInfo.APPNAME), k);
	//	if ( FindWindow(temp, 0) ) {
	//		instances++;
	//	}
	//}

    //------------------------------------------------------
    //isLean = IsInString(GetBBVersion(), "bbLean");
	isLean = false;

	//sprintf(appName, "%s%i", ModInfo.Get(ModInfo.APPNAME), InstanceCount);
	//if ( FindWindow(ModInfo.Get(ModInfo.APPNAME), 0) ) {
	//	sprintf(sbx, "sbx%i", instances);
		//sprintf(sbx2, "@sbx%i.", instances);
		//sprintf(appName, "%s%i", ModInfo.Get(ModInfo.APPNAME), instances);
	//	multipleInstances = true;
	//} else {
		//sprintf(sbx, "%s", "sbx");
		//sprintf(sbx2, "%s", "@sbx.");
		//sprintf(appName, "%s", ModInfo.Get(ModInfo.APPNAME));
	//}
	//memset(&appName, 0, sizeof(appName));
	//sprintf(appName, "%s%i", ModInfo.Get(ModInfo.APPNAME), InstanceCount);
	
    hSystemBarExInstance                = hPluginInstance;
    hDesktopWnd                         = GetDesktopWindow();
    hBlackboxWnd                        = GetBBWnd();

    pSetLayeredWindowAttributes         = NULL;
    pTaskEventSubmenuItem               = NULL;
    pTaskbarItemStored_Menu             = NULL;
    bTaskbarListsUpdated                = false;
    CurrentWorkspace                    = NULL;
    TooltipActivationRect.top           = 0;
    MenuArray[MENU_ID_PLUGIN_MENU]      = MenuArray[MENU_ID_TASKBAR_MENU] = 0;

    ZeroMemory((void*)&menuInfo, sizeof(menuInfo));
    menuInfo.cbSize = sizeof(menuInfo);
    menuInfo.fMask = MIIM_ID | MIIM_SUBMENU | MIIM_STRING | MIIM_STATE;

    ActiveTaskHwnd = 0;
    ActiveTaskHwndOld = (HWND)1;

    GetBlackboxEditor(editor_path);
    CreatePath(docpath, "html"/*, false*/);
    CreatePath(rcpath, "rc"/*, false*/);//

	//CreatePath(exclusionpath, "exclusions.rc", true);

    //------------------------------------------------------

    TBMenuInfo.reset();
    do sprintf(TBMenuInfo.getBroam(), "@sbx.Window %d", TBMenuInfo.n);
    while (++TBMenuInfo < MI_NUMBER);

    ToggleInfo.reset();
    do sprintf(ToggleInfo.getBroam(), "@sbx.Toggle.%d", ToggleInfo.ti_n);
    while (++ToggleInfo < TI_NUMBER);

	EnableInfo.reset();
    do sprintf(EnableInfo.getBroam(), "@sbx.Enable.%d", EnableInfo.ei_n);
    while (++EnableInfo < EI_NUMBER);

    //------------------------------------------------------

    i = 0;
    do hdcArray[i] = CreateCompatibleDC(0),
        hBitmapNullArray[i] = (HBITMAP)SelectObject(hdcArray[i], CreateCompatibleBitmap(bufDC, 2, 2));  // grab 0 bitmap objects from DC
    while (++i < NUMBER_HDC);

    hFontNull = (HFONT)SelectObject(bufDC, CreateStyleFont((StyleItem*)GetSettingPtr(SN_TOOLBAR)));  // grab 0 font object from DC

    SetBkMode(bufDC, TRANSPARENT);

    //------------------------------------------------------

    i = 0;
    do FunctionItemArray[i].click =
        (i < TE_BITS_PER_FXN) ? WM_RBUTTONDOWN:
        (i < (2 * TE_BITS_PER_FXN)) ? WM_RBUTTONUP:
        (i < (3 * TE_BITS_PER_FXN)) ? WM_MBUTTONDOWN:
        (i < (4 * TE_BITS_PER_FXN)) ? WM_MBUTTONUP:
        (i < (5 * TE_BITS_PER_FXN)) ? WM_LBUTTONDOWN:
        WM_LBUTTONUP;
    while (++i < (TE_BUTTON_LENGTH * TE_UPDOWN_LENGTH * TE_BITS_PER_FXN));

	i = 0;
    do FunctionItemArrayDBL[i].click =
        (i == 0) ? WM_RBUTTONDBLCLK:
        (i == 1) ? WM_MBUTTONDBLCLK:
        WM_LBUTTONDBLCLK;
    while (++i < TE_BUTTON_LENGTH);

    //------------------------------------------------------

    TaskIconItem.SetHDC(bufIconTask);
    TrayIconItem.SetHDC(bufIconTray);

    //------------------------------------------------------

    IM_Array[INT_CUSTOM_X].Set("Custom X", INT_CUSTOM_X, &SystemBarExX, -2000, 2000);
    IM_Array[INT_CUSTOM_Y].Set("Custom Y", INT_CUSTOM_Y, &SystemBarExY, -2000, 2000);
    IM_Array[INT_WIDTH_PERCENT].Set("Width Percent", INT_WIDTH_PERCENT, &SystemBarExWidthPercent, 5, 100);
    IM_Array[INT_TOOLTIP_ALPHA].Set("Alpha", INT_TOOLTIP_ALPHA, &TipSettings.alpha, 0, 255);
    IM_Array[INT_WINDOW_ALPHA].Set("Alpha", INT_WINDOW_ALPHA, &WindowAlpha, 0, 255);
    IM_Array[INT_TASK_FONTSIZE].Set("Font Size", INT_TASK_FONTSIZE, &TaskFontSize, 2, 50);
    IM_Array[INT_TASK_ICON_SIZE].Set("Icon Size", INT_TASK_ICON_SIZE, &TaskIconItem.m_size, 4, 32);
    IM_Array[INT_TASK_ICON_HUE].Set("Icon Hue", INT_TASK_ICON_HUE, &TaskIconItem.m_hue, 0, 255);
    IM_Array[INT_TASK_ICON_SAT].Set("Icon Sat", INT_TASK_ICON_SAT, &TaskIconItem.m_sat, 0, 255);
    IM_Array[INT_USER_HEIGHT].Set("Custom", INT_USER_HEIGHT, &CustomHeightSizing, 8, 128);
	IM_Array[INT_TASK_MAXWIDTH].Set("Task Max Width", INT_TASK_MAXWIDTH, &TaskMaxWidth, 0, 600);
    IM_Array[INT_TRAY_ICON_SIZE].Set("Icon Size", INT_TRAY_ICON_SIZE, &TrayIconItem.m_size, 4, 32);
    IM_Array[INT_TRAY_ICON_HUE].Set("Icon Hue", INT_TRAY_ICON_HUE, &TrayIconItem.m_hue, 0, 255);
    IM_Array[INT_TRAY_ICON_SAT].Set("Icon Sat", INT_TRAY_ICON_SAT, &TrayIconItem.m_sat, 0, 255);
    IM_Array[INT_TOOLTIP_DELAY].Set("Delay", INT_TOOLTIP_DELAY, &TipSettings.delay, 0, 5000);
    IM_Array[INT_TOOLTIP_DISTANCE].Set("Distance", INT_TOOLTIP_DISTANCE, &TipSettings.distance, -10, 100);
    IM_Array[INT_TOOLTIP_MAXWIDTH].Set("Max Width", INT_TOOLTIP_MAXWIDTH, &TipSettings.max_width, 100, 600);

    //------------------------------------------------------

    SystrayList = new LeanList<SystrayItem*>();
    TaskbarList = new LeanList<TaskbarItem*>();
    TaskbarListIconized = new LeanList<TaskbarItem*>();

    //------------------------------------------------------

    BGItemArray[BG_BB_BUTTON] = new BackgroundItem(
        STYLE_MENU_BBBUTTON,
        EI_SHOW_BBBUTTON,
        (toggle_index)0,
        bufBBButtonPressed,
        bufBBButtonNotPressed,
        BBButtonText,
        (unsigned*)&BBButtonTextSize.cx,
        0,
        &BackgroundItem::GenericBG,
        BBButtonMouseEvent);

    BGItemArray[BG_WORKSPACE] = new BackgroundItem(
        STYLE_MENU_WORKSPACE,
        EI_SHOW_WORKSPACE,
        (toggle_index)0,
        bufWorkspacePressed,
        bufWorkspaceNotPressed,
        WorkspaceText,
        (unsigned*)&WorkspaceTextSize.cx,
        0,
        &BackgroundItem::GenericBG,
        WorkspaceMouseEvent);

    BGItemArray[BG_CLOCK] = new BackgroundItem(
        STYLE_MENU_CLOCK,
        EI_SHOW_CLOCK,
        TI_TOOLTIPS_CLOCK,
        bufClockPressed,
        bufClockNotPressed,
        ClockTime,
        (unsigned*)&ClockTextSize.cx,
        ClockTimeTooltip,
        &BackgroundItem::GenericBG,
        ClockMouseEvent);

    BGItemArray[BG_TRAY] = new BackgroundItem(
        STYLE_MENU_SYSTRAY,
        EI_SHOW_TRAY,
        TI_TOOLTIPS_TRAY,
        0,
        bufTray,
        0,
        0,
        0,
        &BackgroundItem::PaintTray,
        TrayMouseEvent);
	//
	//
	BGItemArray[BG_WORKSPACEMOVEL] = new BackgroundItem(
        STYLE_MENU_WORKSPACEBUTTONS,
        EI_SHOW_WORKMOVELEFT1,
        (toggle_index)0,
        bufWSLButtonNotPressed,
        bufWSLButtonPressed,
        BBButtonText,
        (unsigned*)&BBButtonTextSize.cx,
        0,
		&BackgroundItem::PaintMoveLeft,//&BackgroundItem::GenericBG,
        WorkspaceMoveLMouseEvent);
	BGItemArray[BG_WORKSPACEMOVER] = new BackgroundItem(
        STYLE_MENU_WORKSPACEBUTTONS,
        EI_SHOW_WORKMOVERIGHT1,
        (toggle_index)0,
        bufWSRButtonNotPressed,
        bufWSRButtonPressed,
        BBButtonText,
        (unsigned*)&BBButtonTextSize.cx,
        0,
		&BackgroundItem::PaintMoveRight,//&BackgroundItem::GenericBG,
        WorkspaceMoveRMouseEvent);

    pWindowLabelItem = new BackgroundItem(
        STYLE_MENU_WINDOWLABEL,
        EI_SHOW_WINDOWLABEL,
        (toggle_index)0,
        bufWindowLabelPressed,
        bufWindowLabelNotPressed,
        0,
        (unsigned*)&WindowLabelWidth,
        0,
        &BackgroundItem::PaintWindowLabel,
        WindowLabelMouseEvent);

    pWindowLabelItem->align = ELEM_ALIGN_RIGHT;

    //------------------------------------------------------

    ZeroMemory(&bmpInfo, sizeof(bmpInfo));
    bmpInfo.biSize = sizeof(bmpInfo);
    bmpInfo.biPlanes = 1;
    bmpInfo.biCompression = BI_RGB;

    //------------------------------------------------------

    TheBorder = 10;
    RCSettings(READ);
	//ReadExclusions();

    if (bInSlit)
        ShowPluginMenu(false);

    Placement_Old = SystemBarExPlacement;

    //------------------------------------------------------

//	if ( instances <= 1 ) {
		ZeroMemory(&wc,sizeof(wc));
		wc.lpfnWndProc = SystemBarExWndProc;
		wc.hInstance = hSystemBarExInstance;
		wc.lpszClassName = (ModInfo.Get(ModInfo.APPNAME));
		wc.hCursor  = LoadCursor(0, IDC_ARROW);
		wc.style = CS_DBLCLKS;
		RegisterClass(&wc);
//	}

    //------------------------------------------------------

    hSystemBarExWnd = CreateWindowEx(
        WS_EX_TOOLWINDOW|WS_EX_ACCEPTFILES,
        (ModInfo.Get(ModInfo.APPNAME)),
		0,
        WS_POPUP|WS_VISIBLE,
        SystemBarExX,
        SystemBarExY,
        SystemBarExWidth,
        SystemBarExHeight,
        0,
        0,
        hSystemBarExInstance,
        0);

	DragAcceptFiles(hSystemBarExWnd, true);
	//------------------------------------------------------
	
    //------------------------------------------------------

    pTbInfo = GetToolbarInfo();
    NotifyToolbar();

    //------------------------------------------------------

    *(FARPROC*)&pSetLayeredWindowAttributes = GetProcAddress(GetModuleHandle("user32.dll"), "SetLayeredWindowAttributes");

	if ( WindowAlpha < 255 ) {
        SetTransTrue(hSystemBarExWnd, WindowAlpha);
	}
    if (!bInSlit && (SystemBarExPlacement != PLACEMENT_LINK_TO_TOOLBAR) && ToggleInfo.test(TI_ALWAYS_ON_TOP))
        SetWindowPos(hSystemBarExWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE);

    //------------------------------------------------------

    SendMessage(hBlackboxWnd, BB_REGISTERMESSAGE, (WPARAM)hSystemBarExWnd, (LPARAM)bb_messages);
	ShowWindow(hSystemBarExWnd, SW_SHOW);
    MakeSticky(hSystemBarExWnd);

    //------------------------------------------------------

    SetTimer(hSystemBarExWnd, CLOCK_UPDATE_TIMER, 250, 0);

    //------------------------------------------------------
    TipSettings.activation_rect_pad = OUTER_SPACING;
    TipSettings.pStyle =
        TipSettings.pStyleFont = (StyleItem*)GetSettingPtr(SN_TOOLBAR);

    bbTip.Start(hSystemBarExInstance, hSystemBarExWnd, &TipSettings);

    if (bbTip.m_TipHwnd && TipSettings.alpha < 255)
        SetTransTrue(bbTip.m_TipHwnd, TipSettings.alpha);

    //------------------------------------------------------
    Reconfig(true);

    WindowLabelText[0] = 0;
    GestureObject.m_x = -1;
    p_bbAbout = 0;

	if (ToggleInfo.test(TI_AUTOHIDE))
        HideBar(true);

    bStart = true;
	if (bInSlit && hSlitWnd) 
        SendMessage(hSlitWnd, SLIT_ADD, 0, (LPARAM)hSystemBarExWnd);

    bCaptureInv = true;
	m_TinyDropTarget = init_drop_targ(hSystemBarExWnd);

    return 0;
}

//-----------------------------------------------------------------------------

void endPlugin(HINSTANCE hPluginInstance) {
	//int		k = 0;
	//int		instances = 0;

	//if ( FindWindow(ModInfo.Get(ModInfo.APPNAME), 0) ) {
	//	instances++;
	//}
	/* loop to find all instances */
	//for ( k = 0; k < 10; k++ ) {
	//	char temp[MINI_MAX_LINE_LENGTH];
	//	memset(&temp, 0, sizeof(temp));
	//	sprintf(temp, "%s%i", ModInfo.Get(ModInfo.APPNAME), k);
	//	if ( FindWindow(temp, 0) ) {
	//		instances++;
	//	}
	//}

	//if ( instances <= 1 ) {
	exit_drop_targ(m_TinyDropTarget);

		if (hMenuFrameFont) DeleteObject(hMenuFrameFont);  // fonts for system menus
		if (hMenuTitleFont) DeleteObject(hMenuTitleFont);

		BYTE i = 0;
		KillTimer(hSystemBarExWnd, CLOCK_UPDATE_TIMER);
		RemoveSticky(hSystemBarExWnd);

		bbTip.End();

		if (p_bbAbout)
			delete p_bbAbout;

		DeleteObject((HFONT)SelectObject(bufDC, hFontNull));

		do DeleteObject((HBITMAP)SelectObject(hdcArray[i], hBitmapNullArray[i])),
			DeleteDC(hdcArray[i]);
		while (++i < NUMBER_HDC);

		if (MenuArray[MENU_ID_TASKBAR_MENU])
			DelMenu(MenuArray[MENU_ID_TASKBAR_MENU]);

		delete pTaskEventSubmenuItem;

		deleteSystrayList();
		delete SystrayList;
		deleteTaskbarList(TaskbarList);
		delete TaskbarList;
		deleteTaskbarList(TaskbarListIconized);
		delete TaskbarListIconized;

		i = 0;
		do ReleaseButton(&BGItemArray[i]->mouseRect),
			delete BGItemArray[i];
		while (++i < NUMBER_BACKGROUNDITEMS);

		if(bInSlit && hSlitWnd)
			SendMessage(hSlitWnd, SLIT_REMOVE, 0, (LPARAM)hSystemBarExWnd);

		UnregisterClass((ModInfo.Get(ModInfo.APPNAME)), hSystemBarExInstance);
	//}
	SendMessage(hBlackboxWnd, BB_UNREGISTERMESSAGE, (WPARAM)hSystemBarExWnd, (LPARAM)bb_messages);
	DestroyWindow(hSystemBarExWnd);
}
bool SameRect( RECT *newRect, RECT oldRect ) {
	return true;
}

void HoverRaiseTask( int MouseX, int MouseY) {
	bool found = false;
	HWND temp = NULL;
	//tasklist *t1 = GetTaskListPtr();

	pTaskTmp = TaskbarList->next;
	do {
		if ( !pTaskTmp->v->TaskExists ) {
			break;
		}
		//Make sure we're in the same rect, or reset the timer.
		if ( MouseX <= pTaskTmp->v->TaskRect.right && MouseX >= pTaskTmp->v->TaskRect.left ) {
			found = true;
			temp = pTaskTmp->v->pTaskList->hwnd;
			break;
		}
		
	} while (pTaskTmp = pTaskTmp->next);
	//raise window for task
	if ( found ) {
		PostMessage(hBlackboxWnd, BB_BRINGTOFRONT, 0, (LPARAM)temp);
	}
}

//-----------------------------------------------------------------------------
LRESULT CALLBACK SystemBarExWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
	if ( dragging && hoverTime ) {
		if ( systemTimeNow.wMilliseconds - hoverTime >= 500 || systemTimeNow.wSecond > hoverSecond) {
			//if we've waited about 1/2 a second, raise the window and clear the dragging information for next time.
			HoverRaiseTask(mousex, mousey);
			dragging = false;
			mousex = mousey = 0;
			hoverTime = 0;
			hoverSecond = 0;
		}
	}

    switch (message) {
        case WM_PAINT:
            MainDisplayDC = BeginPaint(hwnd, &paintStructure);

            TaskbarRect = MainRect;             // reset TaskbarRect;

			if (TaskMaxWidth && (unsigned)TaskMaxWidth != MaxTaskWidth) {
				MaxTaskWidth = TaskMaxWidth;
				bTaskBackgroundsExist = false;
			}


            if (!bBackgroundExists) {
                RECT rectnow;
                GetClientRect(hwnd, &rectnow);

                bmpInfo.biWidth = SystemBarExWidth;
                bmpInfo.biHeight = SystemBarExHeight;
                DeleteObject((HBITMAP)SelectObject(bufBackground, CreateDIBSection(
                    0, (BITMAPINFO*)&bmpInfo, DIB_RGB_COLORS, 0, 0, 0)));

				MakeStyleGradient(bufBackground, &rectnow, &StyleItemArray[getSC(STYLE_MENU_TOOLBAR)], bInSlit?false:StyleItemArray[getSC(STYLE_MENU_TOOLBAR)].bordered);
                bBackgroundExists = true;
            }

            BitBlt(bufDC, 0, 0, SystemBarExWidth, SystemBarExHeight, bufBackground, 0, 0, SRCCOPY);

            Global_Just = DT_CENTER;

            {
                BYTE i = 0;
                do if (EnableInfo.test(BGItemOrderArray[i]->ocIndex) && (BGItemOrderArray[i]->*BGItemOrderArray[i]->pPaint)()) {
					if (BGItemOrderArray[i]->align == ELEM_ALIGN_LEFT) {
                        TaskbarRect.left += BGItemOrderArray[i]->width + OUTER_SPACING;
					} else {
                        TaskbarRect.right -= BGItemOrderArray[i]->width + OUTER_SPACING;
					}
                } while (++i < NUMBER_BACKGROUNDITEMS);
            }

            TaskbarRect.right += 1;             // adjust
            InscriptionLeft = 0;                // reset

            Global_Just = TB_Just;

            if (EnableInfo.test(EI_SHOW_TASKS)) {
                PaintTasks();                   // paint tasks
                InscriptionLeft += OUTER_SPACING;
            }

            WindowLabelWidth = TaskbarRect.right - max(TaskbarRect.left, (int)InscriptionLeft) - 4 * INNER_SPACING - 1;

            if (EnableInfo.test(EI_SHOW_INSCRIPTION))
                PaintInscription();             // paint inscription
            else if (EnableInfo.test(pWindowLabelItem->ocIndex)) {
                TaskbarRect.right -= 1;
                pWindowLabelItem->PaintWindowLabel();
                TaskbarRect.right += 1;
            }

            bbTip.ClearList();
            BitBltRect(MainDisplayDC, bufDC, &paintStructure.rcPaint, 1);

            EndPaint(hwnd, &paintStructure);
            break;

        //----------------------

        case BB_SETTOOLBARLABEL:
            //strncpy_s(WindowLabelText, sizeof(WindowLabelText), (char*)lParam, MINI_MAX_LINE_LENGTH);
			strncpy(WindowLabelText, (char*)lParam, MINI_MAX_LINE_LENGTH);
            SetTimer(hwnd, WINDOWLABEL_TIMER, (isLean ? 1000: 2000), 0);
            InvalidateRect(hwnd, &TaskbarRect, false);
            break;

        //----------------------

        case BB_TASKSUPDATE:
			if (!EnableInfo.test(EI_SHOW_TASKS) && !EnableInfo.test(EI_SHOW_WINDOWLABEL)) {
				break;
			}

			if ( ToggleInfo.test(TI_AUTOHIDE) && bSystemBarExHidden && lParam == TASKITEM_FLASHED ) {
				ShowBar(false);
			}

			if ( ToggleInfo.test(TI_AUTOHIDE) && !bSystemBarExHidden && lParam == TASKITEM_ACTIVATED ) {
				HideBar(false);
			}
            if (isLean) {
                switch (lParam) {
                    case TASKITEM_ACTIVATED:
                        if ((ActiveTaskHwndOld = ActiveTaskHwnd) != (ActiveTaskHwnd = (HWND)wParam))
                            InvalidateRect(hwnd, &TaskbarRect, false);
						
                        break;

                    case TASKITEM_MODIFIED:
                        ActiveTaskHwnd = GetTask(GetActiveTask());
                        goto ico_inv1;

                    case TASKITEM_ADDED:
                    case TASKITEM_REMOVED:
                        bTaskbarListsUpdated = pWindowLabelItem->bgPainted = false;
                    case TASKITEM_REFRESH:
                        bTaskBackgroundsExist = false;
                    case TASKITEM_FLASHED:
                        InvalidateRect(hwnd, &TaskbarRect, false);
						 
                }
            } else {
                ActiveTaskHwndOld = ActiveTaskHwnd;
                ActiveTaskHwnd = GetTask(GetActiveTask());
                if (ToggleInfo.test(TI_FLASH_TASKS)) {
                    HWND h = (HWND)wParam;
                    if (pTaskTmp = TaskbarList->next)
                        do if (h == pTaskTmp->v->pTaskList->hwnd) goto start_flash;
                        while (pTaskTmp = pTaskTmp->next);

                    if (pTaskTmp = TaskbarListIconized->next)
                        do if (h == pTaskTmp->v->pTaskList->hwnd) {
start_flash:                pTaskTmp->v->bFlashed = (lParam == TASKITEM_FLASHED);
                            break;
                        } while (pTaskTmp = pTaskTmp->next);
                }

                switch (lParam) {
                    case TASKITEM_ACTIVATED:
                        if (ActiveTaskHwnd != ActiveTaskHwndOld)
                    case TASKITEM_MODIFIED:
ico_inv1:
                            {
                                HWND h = (HWND)wParam;
                                if (pTaskTmp = TaskbarList->next)
                                    do if (pTaskTmp->v->pTaskList->hwnd == h) goto ico_inv2;
                                    while (pTaskTmp = pTaskTmp->next);
                                if (pTaskTmp = TaskbarListIconized->next)
                                    do if (pTaskTmp->v->pTaskList->hwnd == h) {
ico_inv2:
                                        if (pTaskTmp->v->hIcon_local_b)
                                            DestroyIcon(pTaskTmp->v->hIcon_local_b);
                                        if (pTaskTmp->v->hIcon_local_s)
                                            DestroyIcon(pTaskTmp->v->hIcon_local_s);

                                        pTaskTmp->v->hIcon_local_s =
                                            pTaskTmp->v->hIcon_local_b = 0;

                                        break;
                                    } while (pTaskTmp = pTaskTmp->next);
                            }
                            InvalidateRect(hwnd, &TaskbarRect, false);
                        break;

                    case TASKITEM_ADDED:
                    case TASKITEM_REMOVED:
                        bTaskbarListsUpdated =
                            pWindowLabelItem->bgPainted = false;
                    case TASKITEM_REFRESH:
                        bTaskBackgroundsExist = false;
                    case TASKITEM_FLASHED:
                        InvalidateRect(hwnd, &TaskbarRect, false);
                }
            }
            return 0;

        //----------------------

        case BB_TRAYUPDATE:
            if (EnableInfo.test(EI_SHOW_TRAY)) {
                switch (lParam) {
                    case TRAYICON_MODIFIED:
                        InvalidateRect(hwnd, &BGItemArray[BG_TRAY]->placementRect, false);
                        break;

                    case TRAYICON_REFRESH:
                        bTaskBackgroundsExist = BGItemArray[BG_TRAY]->bgPainted = false;
                        InvalidateRect(hwnd, 0, false); 
					
					//case TRAYICON_ADDED:
					//case TRAYICON_REMOVED:
					default:
                        bTrayListsUpdated = false;
                        RefreshAll();
                        break;
                }
            }
            break;

        //----------------------

        case BB_TOOLBARUPDATE:
            if (!bInSlit && (SystemBarExPlacement == PLACEMENT_LINK_TO_TOOLBAR)) {
                SetWindowPos(hSystemBarExWnd,
                    ((pTbInfo->autoHide || pTbInfo->onTop) ? HWND_TOPMOST: HWND_NOTOPMOST),
                    0, 0, 0, 0, SWP_NOACTIVATE|SWP_NOSENDCHANGING|SWP_NOSIZE|SWP_NOMOVE);
                Reconfig(false);
                SetScreenMargin(false);
            }
            break;

        //----------------------

        case BB_REDRAWGUI:
            if ((wParam & BBRG_TOOLBAR) || (p_bbAbout && (wParam & BBRG_MENU))) {
                GetStyleSettings(true);

                //---------------------  ..pasted from Reconfig()
                if (!bInSlit)
                    SetWindowPos(hSystemBarExWnd, 0,
                        SystemBarExX, SystemBarExY,
                        SystemBarExWidth, SystemBarExHeight,
                        SWP_NOSENDCHANGING|SWP_NOACTIVATE|SWP_NOZORDER);

                UpdateClientRect();
                UpdateClock(true);
                //---------------------

                RefreshAll();
            } else {
                TipSettings.pStyle = (StyleItem*)GetSettingPtr(SN_MENUTITLE);
                bbTip.UpdateSettings();
            }
            break;

        //----------------------
		//SBX will attempt to load a style file if one is dragged and dropped on top of it.

		//For our purpose we need to know if the user is dragging a file, and is hovering over a task item
        //Thus, this is seemingly useless for us to reference.
		case WM_DROPFILES:
            DropStyle((HDROP)wParam);
            return 0;

        //----------------------

        case BB_RECONFIGURE:
            Reconfig(true);
            break;

        //----------------------

        case BB_BROADCAST:
            broam_temp = (char*)lParam;
            //-------------------------------------------------------------------
            if (ToggleInfo.test(TI_TOGGLE_WITH_PLUGINS)) {
                if (!_stricmp(broam_temp, "@BBShowPlugins"))
                    ShowWindow(hwnd, SW_SHOW);
                else if (!_stricmp(broam_temp, "@BBHidePlugins") && !bInSlit)
                    ShowWindow(hwnd, SW_HIDE);
            }
            //-------------------------------------------------------------------
			
			if (_strnicmp(broam_temp, "@sbx.", 5 )) {
                return 0;
			}
            broam_temp += 5;
            //-------------------------------------------------------------------
            if (!_stricmp(broam_temp, "About")) {
                if (!p_bbAbout) {
                    PostMessage(hBlackboxWnd, BB_HIDEMENU, 0, 0);
                    p_bbAbout = new bbAbout(hSystemBarExInstance, &p_bbAbout);
                }
            }
            //-------------------------------------------------------------------
            else if (!_strnicmp(broam_temp, "SysMenu", 7)) {
                X_BBTokenize(broam_temp + 8, 2);
                PostMessage((HWND)BBTokenizeIndexArray[0], WM_SYSCOMMAND, (WPARAM)BBTokenizeIndexArray[1], 0);
                goto end_task_menu;
            }
            //-------------------------------------------------------------------
            else if (!_strnicmp(broam_temp, "Window", 6)) {
                BBTokenizeIndexArray[0] =
                    BBTokenizeIndexArray[1] = 0;
                X_BBTokenize(broam_temp + 7, 2);
                buttonHwnd = BBTokenizeIndexArray[1] ? (HWND)BBTokenizeIndexArray[1]: ActiveTaskHwnd;

                switch(BBTokenizeIndexArray[0]) {
					case MI_CLOSE_ALL:
                        windowAll(SC_CLOSE);
                        break;

					case MI_RESTORE_DOWN_ALL:
                        windowAll(SC_RESTORE);
                        break;

					case MI_MAXIMIZE_ALL:
                        windowAll(SC_MAXIMIZE);
                        break;

					case MI_RESTORE_ALL:
                        windowAll(SW_RESTORE);
                        break;

					case MI_MINIMIZE_ALL:
                        windowAll(SC_MINIMIZE);
                        break;

					case MI_MOVE_LEFT:
                        taskEventFxn7();
                        break;

					case MI_MOVE_RIGHT:
                        taskEventFxn8();
                        break;

					case MI_ICONIZE:
                        if (taskIcons == ICONIZED_ONLY) break;

                        if (pTaskTmp = TaskbarList->next)
                            do if (pTaskTmp->v->pTaskList->hwnd == buttonHwnd) goto toggle_t;
                            while (pTaskTmp = pTaskTmp->next);

                        if (pTaskTmp = TaskbarListIconized->next)
                            do if (pTaskTmp->v->pTaskList->hwnd == buttonHwnd) goto toggle_t;
                            while (pTaskTmp = pTaskTmp->next);

                        break;
toggle_t:               pTaskTmp->v->IsIconized = !pTaskTmp->v->IsIconized;
                        bTaskbarListsUpdated = false;
                        RefreshAll();
                        break;

					case MI_FULL_NORMAL:
                        SetFullNormal(false);
                        break;

					case MI_FULL_NORMAL_ALL:
                        SetFullNormal(true);
                        break;

					case MI_RESTORE_DOWN:
                        PostMessage(buttonHwnd, WM_SYSCOMMAND, SC_RESTORE, 0);
                        break;

					case MI_MAXIMIZE:
                        //PostMessage(buttonHwnd, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
						taskEventFxn4();
                        break;

					case MI_MINIMIZE:
                        taskEventFxn3();
                        break;

					case MI_CLOSE:
                        taskEventFxn5();
                        break;

					case MI_TILE_HORIZONTAL:
                        {
                            RECT r;
                            SystemParametersInfo(SPI_GETWORKAREA, 0, (PVOID)&r, 0);
                            TileWindows(0, MDITILE_HORIZONTAL, &r, 0, 0);
                            break;
                        }

					case MI_TILE_VERTICAL:
                        {
                            RECT r;
                            SystemParametersInfo(SPI_GETWORKAREA, 0, (PVOID)&r, 0);
                            TileWindows(0, MDITILE_VERTICAL, &r, 0, 0);
                            break;
                        }

					case MI_CASCADE:
                        {
                            RECT r;
                            SystemParametersInfo(SPI_GETWORKAREA, 0, (PVOID)&r, 0);
                            //CascadeWindows(0, MDITILE_ZORDER, &r, 0, 0);
							CascadeWindows(0, 0x0004, &r, 0, 0);
                        }
                }

end_task_menu:  bSystemBarExHidden = false;
                PostMessage(hBlackboxWnd, BB_HIDEMENU, 0, 0);
                if (ToggleInfo.test(TI_AUTOHIDE))
                    HideBar(false);
                return 0;
            }
            //-------------------------------------------------------------------
			else if (!_strnicmp(broam_temp, "TaskEventDBL", 12)) {
                broam_temp += 13;
                X_BBTokenize(broam_temp, TE_FACTOR_LENGTH);
				setDBLTEC(BBTokenizeIndexArray[0], BBTokenizeIndexArray[1]);
                //UpdateFunctionItemArrayDBL();
            }
            else if (!_strnicmp(broam_temp, "TaskEvent", 9)) {
                broam_temp += 10;
                X_BBTokenize(broam_temp, TE_FACTOR_LENGTH);
				setTEC(BBTokenizeIndexArray[0], BBTokenizeIndexArray[1], BBTokenizeIndexArray[2], BBTokenizeIndexArray[3]);
                UpdateFunctionItemArray();
            }

			
            //-------------------------------------------------------------------
            else if (!_strnicmp(broam_temp, "TaskMenu.", 9)) {
                TBMenuInfo.n = (menu_index)atoi(broam_temp + 9);
                if (TBMenuInfo.test())
                    TBMenuInfo.setFalse();
                else
                    TBMenuInfo.setTrue();
            }
            //-------------------------------------------------------------------
            else if (!_stricmp(broam_temp, "DefaultME")) {
                TaskEventIntArray[0] = DEFAULT_TE_RIGHT;
                TaskEventIntArray[1] = DEFAULT_TE_MIDDLE;
                TaskEventIntArray[2] = DEFAULT_TE_LEFT;
                UpdateFunctionItemArray();
            }

			else if (!_stricmp(broam_temp, "DefaultDME")) {
                TaskDBLEventIntArray[0] = 0;
                TaskDBLEventIntArray[1] = 0;
                TaskDBLEventIntArray[2] = 0;
				//UpdateFunctionItemArrayDBL();
			}
            //-------------------------------------------------------------------
            else if (!_stricmp(broam_temp, "EditRC"))
                BBExecute(hDesktopWnd, 0, editor_path, rcpath, 0, SW_SHOWNORMAL, false);
            //-------------------------------------------------------------------
            else if (!_stricmp(broam_temp, "Doc"))
                ShellExecute(hDesktopWnd, "open", docpath, 0, 0, SW_SHOWMAXIMIZED);
            //-------------------------------------------------------------------
            else if (!_stricmp(broam_temp, "RefreshMargin")) {
                SetScreenMargin(true);
                pF_TV_RB f = ToggleInfo.test(TI_TASKS_IN_CURRENT_WORKSPACE) ? retW: retT;
                if (!pTbInfo->autoHide && (p_TaskList = GetTaskListPtr()))
                    do if (f() && IsZoomed(p_TaskList->hwnd)) {
                        PostMessage(p_TaskList->hwnd, WM_SYSCOMMAND, SC_RESTORE, 0);
                        PostMessage(p_TaskList->hwnd, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
                    }
                    while (p_TaskList = p_TaskList->next);
            }
            //-------------------------------------------------------------------
            else if (!_stricmp(broam_temp, "Style.Default")) {
                StyleConfig = 2147261058;
                goto rfr_sty;
            }
            //-------------------------------------------------------------------
            else if (!_stricmp(broam_temp, "Style.Auto")) {
                StyleConfig = 2147483647;
                goto rfr_sty;
            }
            //-------------------------------------------------------------------
            else if (!_stricmp(broam_temp, "Style.PR")) {
                BYTE i = 0;
                do if ((i != STYLE_MENU_PRESSED) && (i != STYLE_MENU_TOOLTIPS))
                    setSC(i, STYLE_PARENTRELATIVE);
                while (++i < NUMBER_STYLE_MENUS);

                goto rfr_sty;
            }
            //-------------------------------------------------------------------
            else if (!_strnicmp(broam_temp, "Style", 5)) {
                broam_temp += 6;
                X_BBTokenize(broam_temp, 2);
                setSC(BBTokenizeIndexArray[0], BBTokenizeIndexArray[1]);
rfr_sty:
                GetStyleSettings(true);
                goto rfr_all;
            }
            //-------------------------------------------------------------------
            else if (!_stricmp(broam_temp, "ReadRC")) {
                RCSettings(READ);
                //UpdateTransparency(TI_TOOLTIPS_TRANS);  // ........WINDOW_TRANS is updated in Reconfig()
				UpdateToolTipsTransparency();
                Reconfig(true);
            }
            //-------------------------------------------------------------------
            else if (!_strnicmp(broam_temp, "Placement.", 10)) {
                Placement_Old = SystemBarExPlacement;
                SystemBarExPlacement = atoi(broam_temp + 10);
                Reconfig(false);
                SetScreenMargin(true);
                if (ToggleInfo.test(TI_AUTOHIDE) && (SystemBarExPlacement != PLACEMENT_LINK_TO_TOOLBAR))
                    HideBar(true);
            }
            //-------------------------------------------------------------------
            else if (!_stricmp(broam_temp, "AllIcons"))
            {
                deleteSystrayList();
                SystrayList->clear();
                bTrayListsUpdated = false;
                goto rfr_all;
            }
            //-------------------------------------------------------------------
            else if (!_stricmp(broam_temp, "NoIconized"))
            {
                if (pTaskTmp = TaskbarListIconized->next)
                    do pTaskTmp->v->IsIconized = false;
                    while (pTaskTmp = pTaskTmp->next);
                bTaskbarListsUpdated = false;
                goto rfr_all;
            }
            //-------------------------------------------------------------------
            else if (!_strnicmp(broam_temp, "String.ClockTip", 15))
                ProcessStringBroam(ClockTooltipFormat, broam_temp + 16);
            //-------------------------------------------------------------------
            else if (!_stricmp(broam_temp, "WorkspaceName"))
            {
                PostMessage(hBlackboxWnd, BB_WORKSPACE, 13, 0);
                break;
            }
            //-------------------------------------------------------------------
            else if (!_strnicmp(broam_temp, "Order.", 6)) {
				BYTE    currentSpot = atoi(broam_temp + 6),
                        nextSpot = currentSpot + 1,
                        currentValue = PlacementStruct[currentSpot],
                        nextValue = PlacementStruct[nextSpot];

				if ( currentSpot == NUMBER_ELEMENTS - 1 ) {
					nextSpot = 0;
				}

               // if ( currentSpot < NUMBER_ELEMENTS - 1) {
                    do {//Find the next enabled item to switch with
						
						if ( nextSpot == NUMBER_ELEMENTS ) {
							//Start over from 0 if we've hit the end
							nextSpot = 0;
						}
						nextValue = PlacementStruct[nextSpot];//Reset the swap value

                        if (EnableInfo.test(nextValue == BG_TASKS ? EI_SHOW_TASKS : BGItemArray[nextValue]->ocIndex)) {
							//If it's enabled, swap and end the loop
                            PlacementStruct[currentSpot] = nextValue;
                            PlacementStruct[nextSpot] = currentValue;
                            break;
                        }
                    } while (++nextSpot != currentSpot);

				//Rebuild PlacementConfigString
				memset(&PlacementConfigString, 0, sizeof(PlacementConfigString));
				sprintf(PlacementConfigString, "%i%i%i%i%i%i%i", PlacementStruct[0], PlacementStruct[1], PlacementStruct[2], PlacementStruct[3], PlacementStruct[4], PlacementStruct[5], PlacementStruct[6] );

                UpdatePlacementOrder();
                goto rfr_all;
            }
            //-------------------------------------------------------------------
            else if (!_strnicmp(broam_temp, "Tray", 4))
            {
                pSysTmp = SystrayList;
                SystrayItem *pSystrayNow = (SystrayItem*)atoi(broam_temp + 5);
                while (pSysTmp->next)
                {
                    if (pSysTmp->next->v == pSystrayNow)
                    {
                        delete pSystrayNow;
                        pSysTmp->erase();
                        break;
                    }
                    pSysTmp = pSysTmp->next;
                }
                bTrayListsUpdated = false;
rfr_all:        RefreshAll();
            }
            //-------------------------------------------------------------------
            else {
                //-------------------------------------------------------------------
                if (!_strnicmp(broam_temp, "IntMenu.", 8)) {
                    broam_temp += 8;
                    X_BBTokenize(broam_temp, 2);
                    *IM_Array[BBTokenizeIndexArray[0]].m_value = BBTokenizeIndexArray[1];

                    switch (BBTokenizeIndexArray[0]) {
                        case INT_CUSTOM_X:
                        case INT_CUSTOM_Y:
                            ShowPluginMenu(false);  // needed before reconfig for correct updating?
                            Reconfig(true);
                            return 0;

                        case INT_TOOLTIP_ALPHA:
                            //UpdateTransparency(TI_TOOLTIPS_TRANS);
							UpdateToolTipsTransparency();
                            break;

                        case INT_WINDOW_ALPHA:
                            //UpdateTransparency(TI_WINDOW_TRANS);
							UpdateBarTransparency();
                            break;
                    }
                }
                //-------------------------------------------------------------------
                else if (!_strnicmp(broam_temp, "String.Clock", 12))
                    ProcessStringBroam(ClockFormat, broam_temp + 13);
                //-------------------------------------------------------------------
                else if (!_strnicmp(broam_temp, "String.Inscription", 18))
                    ProcessStringBroam(InscriptionText, broam_temp + 19);
                //-------------------------------------------------------------------
                else if (!_strnicmp(broam_temp, "String.BBButton", 15))
                    ProcessStringBroam(BBButtonText, broam_temp + 16);

				else if (!_strnicmp(broam_temp, "String.BBButtonRCommand", 23))
                    ProcessStringBroam(BBButtonRCommand, broam_temp + 24);
				else if (!_strnicmp(broam_temp, "String.BBButtonLCommand", 23))
                    ProcessStringBroam(BBButtonLCommand, broam_temp + 24);
				else if (!_strnicmp(broam_temp, "String.BBButtonMCommand", 23))
                    ProcessStringBroam(BBButtonMCommand, broam_temp + 24);
				//
				else if (!_strnicmp(broam_temp, "String.WinLabelRCommand", 23))
                    ProcessStringBroam(WindowLabelRCommand, broam_temp + 24);
				else if (!_strnicmp(broam_temp, "String.WinLabelLCommand", 23))
                    ProcessStringBroam(WindowLabelLCommand, broam_temp + 24);
				else if (!_strnicmp(broam_temp, "String.WinLabelMCommand", 23))
                    ProcessStringBroam(WindowLabelMCommand, broam_temp + 24);
                //-------------------------------------------------------------------
                else if (!_strnicmp(broam_temp, "TaskDisp.", 9)) {
                    taskIcons = atoi(broam_temp + 9);
                    Reconfig(true);
                }
                //-------------------------------------------------------------------
                else if (!_strnicmp(broam_temp, "Height.", 7))
                    HeightSizing = atoi(broam_temp + 7);
				
                //-------------------------------------------------------------------
				else if (!_strnicmp(broam_temp, "Enable.", 7)) {
					EnableInfo.ei_n = enable_index(atoi(broam_temp + 7));
					EnableInfo.toggle();
					switch ( EnableInfo.ei_n ) {
						case EI_SHOW_WINDOWLABEL:
								EnableInfo.setFalse(EI_SHOW_INSCRIPTION);
								break;

						case EI_SHOW_INSCRIPTION:
								EnableInfo.setFalse(EI_SHOW_WINDOWLABEL);
								break;
						default:
							break;
					}
				}
                //-------------------------------------------------------------------
                else if (!_strnicmp(broam_temp, "Toggle.", 7)) {
                    ToggleInfo.ti_n = toggle_index(atoi(broam_temp + 7));
                    ToggleInfo.toggle();
                    switch (ToggleInfo.ti_n) {
						case TI_REVERSE_TASKS:
						case TI_TASKS_IN_CURRENT_WORKSPACE:
								NotifyToolbar();
								RefreshAll();
								goto rc_mn;

						case TI_TOOLTIPS_TRANS:
							TipSettings.bTransparency = TipSettings.alpha < 255 ? true : false;
								goto upd_trans;

						case TI_WINDOW_TRANS:
								if (bInSlit)
									goto tg_over;
	upd_trans:                  UpdateToolTipsTransparency();//UpdateTransparency(ToggleInfo.ti_n);
								break;

						case TI_COMPRESS_ICONIZED:
								goto rfr_sty;
								break;

						case TI_TOOLTIPS_DOCKED:
								TipSettings.bDocked = ToggleInfo.test();
								break;

						case TI_TOOLTIPS_ABOVE_BELOW:
								TipSettings.bAbove = ToggleInfo.test();
								break;

						case TI_TOOLTIPS_CENTER:
								TipSettings.bCenterTip = ToggleInfo.test();
								break;

						case TI_SET_WINDOWLABEL:
								TipSettings.bSetLabel = ToggleInfo.test();
								break;

						/*case TI_SHOW_WINDOWLABEL:
								EnableInfo.setFalse(EI_SHOW_INSCRIPTION);
								break;

						case TI_SHOW_INSCRIPTION:
								EnableInfo.setFalse(EI_SHOW_WINDOWLABEL);
								break;*/

						case TI_ALWAYS_ON_TOP:
								if (bInSlit)
									goto tg_over;
								if (!ToggleInfo.test(TI_AUTOHIDE))
									SetWindowPos(hwnd,
									(ToggleInfo.test() ? HWND_TOPMOST: HWND_NOTOPMOST),
									0, 0, 0, 0, SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE);
								break;

						case TI_AUTOHIDE:
								if (bInSlit) {
	tg_over:                        ToggleInfo.toggle();
									return 0;
								}
								if (SystemBarExPlacement != PLACEMENT_LINK_TO_TOOLBAR) {
									if (ToggleInfo.test())
										HideBar(true);
									else {
										ShowBar(true);
										if (!ToggleInfo.test(TI_ALWAYS_ON_TOP))
											SetWindowPos(hwnd, HWND_NOTOPMOST,
											0, 0, 0, 0, SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE|SWP_NOSENDCHANGING);
									}
								}
								SetScreenMargin(true);
						default:
							break;
                    }
                }
                //-------------------------------------------------------------------
                Reconfig(false);
            }

rc_mn:      RCSettings(WRITE);
            ShowPluginMenu(false);
            break;

        //----------------------

        case WM_MOUSEACTIVATE:
            return MA_NOACTIVATE;

        //----------------------

        case WM_RBUTTONUP:
            if (TestAsyncKeyState(VK_CONTROL)) {
		case WM_NCRBUTTONUP:
                ShowPluginMenu(true);
                break;
            }
        case WM_MOUSEMOVE:
            if (!hover_count) {
                hover_count = 1;
                SetTimer(hwnd, CHECK_RECT_TIMER, 50, 0);
                if (ToggleInfo.test(TI_AUTOHIDE) && (SystemBarExPlacement != PLACEMENT_LINK_TO_TOOLBAR))
                    ShowBar(false);
            }

        case WM_MBUTTONUP:
        case WM_LBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_LBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_RBUTTONDBLCLK:
        case WM_LBUTTONDBLCLK:
        case WM_MBUTTONDBLCLK:


            MouseEventPoint.x = (short)LOWORD(lParam);
            MouseEventPoint.y = (short)HIWORD(lParam);
            bbTip.MouseEvent(MouseEventPoint, message);

			if (TestAsyncKeyState(VK_CONTROL)) {
                break;
			}

			if (GestureObject.Process(message)) {
                break;
			}

			if (HandleCapture(message, MouseEventPoint)) {
                break;
			}

            if (message != WM_MOUSEMOVE) {
                if ((MouseEventPoint.x >= TasksMouseSize.cx) && (MouseEventPoint.x <= TasksMouseSize.cy)) {
                    if (!TasksMouseEvent(message)) {
						if ((message == WM_LBUTTONDOWN) || (message == WM_RBUTTONDOWN)) {
                            WindowLabelX = MouseEventPoint.x;
						}

                        if (EnableInfo.test(pWindowLabelItem->ocIndex)) {
                            CheckButton(message, MouseEventPoint, &pWindowLabelItem->mouseRect,
                                &pWindowLabelItem->bPressed, pWindowLabelItem->pMouseEvent, true, true);
                        } else if (MouseEventPoint.x > (TasksMouseSize.cy - WindowLabelWidth))
                            WindowLabelMouseEvent(message);  // ...outside the tasks, but _not_ in the window-label
                    }
                    break;
                }

                BYTE i = 0;
                do if ((i != BG_TRAY) && EnableInfo.test(BGItemArray[i]->ocIndex) &&
                    CheckButton(message, MouseEventPoint, &BGItemArray[i]->mouseRect,
					&BGItemArray[i]->bPressed, BGItemArray[i]->pMouseEvent, true, true)) {
                    return 0;
				} while (++i < NUMBER_BACKGROUNDITEMS);
            }

            if (EnableInfo.test(BGItemArray[BG_TRAY]->ocIndex) && (MouseEventPoint.x > BGItemArray[BG_TRAY]->mouseRect.left) &&
				(MouseEventPoint.x < BGItemArray[BG_TRAY]->mouseRect.right)) {
                BGItemArray[BG_TRAY]->pMouseEvent(message);
			}

            break;

        //----------------------

        case BB_DESKTOPINFO:
            DeskInfo = (DesktopInfo*)lParam;

            if (DeskInfo->isCurrent) {
                SIZE tmp;
                //strlist *list = DeskInfo->deskNames;
				string_node *list = DeskInfo->deskNames;
                UINT old_width = WorkspaceTextSize.cx;
                WorkspaceTextSize.cx = 0;
                do {
                    GetTextExtentPoint32(bufDC, list->str, strlen(list->str), &tmp);
                    if (WorkspaceTextSize.cx < tmp.cx)
                        WorkspaceTextSize.cx = tmp.cx;
                } while (list = list->next);

                CurrentWorkspace = DeskInfo->number;
                if (strcmp(WorkspaceText, DeskInfo->name) || (old_width != WorkspaceTextSize.cx)) {
                    strcpy(WorkspaceText, DeskInfo->name);
                    BGItemArray[BG_WORKSPACE]->bgPainted = false;
                    bTaskbarListsUpdated = false;
                    Reconfig(false);
                }
            }

            break;

        //----------------------

        case BB_LISTDESKTOPS:
            if ((EnableInfo.test(EI_SHOW_WORKSPACE)) && (pTbInfo->hwnd == (HWND)wParam))
        case WM_CREATE:
                PostMessage(hBlackboxWnd, BB_LISTDESKTOPS, (WPARAM)hwnd, 0);
            break;

        //----------------------

        case WM_DESTROY:
            SystemBarExHeight = 0;
            SetScreenMargin(true);
            pTbInfo->bbsb_linkedToToolbar = false;
            pTbInfo->bbsb_hwnd = 0;
            break;

        //----------------------
		case BB_DRAGOVER:
			//task_over_hwnd = NULL;
			if (bSystemBarExHidden) {
				ShowBar(true);
			}// else {
			//	HoverRaiseTask((short)LOWORD(lParam), (short)HIWORD(lParam));
			//}
			mousex = LOWORD(lParam);
			mousey = HIWORD(lParam);
			hoverTime = systemTimeNow.wMilliseconds;
			hoverSecond = systemTimeNow.wSecond;
			dragging = true;
			//return (LRESULT)task_over_hwnd;

        case WM_TIMER:
            switch (wParam) {
                case CLOCK_UPDATE_TIMER:
                    GetLocalTime(&systemTimeNow);
                    SetTimer(hwnd, CLOCK_UPDATE_TIMER, 1100 - systemTimeNow.wMilliseconds, 0);
                    UpdateClock(false);
                    break;

                case CHECK_RECT_TIMER:
                    GetCursorPos(&MouseEventPoint);
                    if (hwnd != WindowFromPoint(MouseEventPoint)) {
                        KillTimer(hwnd, CHECK_RECT_TIMER);
                        hover_count = 0;
                        if (ToggleInfo.test(TI_AUTOHIDE))
                            HideBar(false);
                    }
                    else if ((hover_count < 2) && (++hover_count == 2) && ToggleInfo.test(TI_HIDE_MENU_ON_HOVER))
                        PostMessage(hBlackboxWnd, BB_HIDEMENU, 0, 0);
                    break;

                case WINDOWLABEL_TIMER:
                    KillTimer(hwnd, WINDOWLABEL_TIMER);
                    *WindowLabelText = 0;
                    InvalidateRect(hwnd, &TaskbarRect, false);
                    break;
				/*case TASK_RISE_TIMER:
					dragging = false;
					handle_task_timer(m_TinyDropTarget);
					break;*/
            }
            break;

        //----------------------

        case WM_NCHITTEST:          // allow window to move if <ctrl> is held down
            return (!bInSlit && !ToggleInfo.test(TI_LOCK_POSITION) && TestAsyncKeyState(VK_CONTROL)) ? HTCAPTION: HTCLIENT;

        //----------------------

        case WM_WINDOWPOSCHANGING:  // if moved, snap window to screen edges...
            {
                WINDOWPOS *p = (WINDOWPOS*)lParam;
                if (ToggleInfo.test(TI_SNAP_TO_EDGE) && !bInSlit)
                    SnapWindowToEdge(p, 10, true);

                if (!(p->flags & SWP_NOMOVE)) {
                    SystemBarExX = p->x;
                    SystemBarExY = p->y;
                }

                if (SystemBarExPlacement == PLACEMENT_CUSTOM)
                    RCSettings(WRITE);
            }
            break;

        //----------------------

        case WM_NCLBUTTONDOWN:
            SetCursor(LoadCursor(0, IDC_SIZEALL));

        //----------------------
		
        default:
            return DefWindowProc(hwnd,message,wParam,lParam);

    //----------------------
    }
    return 0;
}

//---------------------------------------------------------------------------
// Function: ProcessStringBroam
//---------------------------------------------------------------------------

void ProcessStringBroam(char *dest_char, char *src_char) {
    if (src_char[0] == '"') ++src_char;  // peel off leading quotation mark for 0.90
    //strncpy_s(dest_char, sizeof(dest_char), src_char, MINI_MAX_LINE_LENGTH);     // chop off buffer overruns
	strncpy(dest_char, src_char, MINI_MAX_LINE_LENGTH);
    dest_char[MINI_MAX_LINE_LENGTH - 1] = 0;                // make last character 0, just in case
    char *p = dest_char;
    while (p[0]) {
        if (p[0] == '"') {
            p[0] = 0;       // peel off trailing quotation mark for 0.90
            break;
        }
        ++p;
    }
}

//---------------------------------------------------------------------------
// Function: deleteTaskbarList
// Purpose:  deletes all TaskbarItems in a TaskbarItem list
//---------------------------------------------------------------------------

void deleteTaskbarList(LeanList<TaskbarItem*> *n) {
    while (n = n->next) delete n->v;
}

//---------------------------------------------------------------------------
// Function: deleteSystrayList
// Purpose:  deletes all SystrayItems in the SystrayList
//---------------------------------------------------------------------------

void deleteSystrayList() {
    if (pSysTmp = SystrayList->next)
        do delete pSysTmp->v;
        while(pSysTmp = pSysTmp->next);
}

//---------------------------------------------------------------------------
// Function: X_BBTokenize
//---------------------------------------------------------------------------

void X_BBTokenize(char *broam_temp, BYTE sz) {
    BYTE i = 0;
    char char_array[MAX_TOKENS][MAX_LINE_LENGTH];
    LPSTR tokens[MAX_TOKENS];

	do {
		tokens[i] = char_array[i], tokens[i][0] = '\0';
	} while (++i < sz);

    BBTokenize(broam_temp, tokens, sz, 0);

    i = 0;
	do {
		BBTokenizeIndexArray[i] = atoi(tokens[i]);
	} while (++i < sz);
}

//---------------------------------------------------------------------------
// Function: ShowBar
//---------------------------------------------------------------------------

void ShowBar(bool force) {
    if (force || (bSystemBarExHidden && !TestAsyncKeyState(VK_CONTROL))) {
        bSystemBarExHidden = false;
        Autohide(ScreenHeight - SystemBarExHeight, 0);
		if ( WindowAlpha < 255 ) {
            SetTransTrue(hSystemBarExWnd, WindowAlpha);
		} else {
            SetWindowLongPtr(hSystemBarExWnd, GWL_EXSTYLE, WS_EX_TOOLWINDOW|WS_EX_ACCEPTFILES);
		}
    }
}

//---------------------------------------------------------------------------
// Function: HideBar
//---------------------------------------------------------------------------

void HideBar(bool force) {
    if ((SystemBarExPlacement != PLACEMENT_LINK_TO_TOOLBAR)
        && (force || (!bSystemBarExHidden && !TestAsyncKeyState(VK_CONTROL)))) {
        bSystemBarExHidden = true;
        Autohide(ScreenHeight - 1, 1 - SystemBarExHeight);
        SetTransTrue(hSystemBarExWnd, 1);
    }
}

//---------------------------------------------------------------------------
// Function: Autohide
//---------------------------------------------------------------------------

void Autohide(unsigned low, unsigned high) {
    if (!bInSlit)
    SetWindowPos(hSystemBarExWnd, HWND_TOPMOST, SystemBarExX,
        (((2 * SystemBarExY) > ScreenHeight) ? low: high),
        0, 0, SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOSENDCHANGING);
}

//---------------------------------------------------------------------------
// Function: SetTransTrue
//---------------------------------------------------------------------------

void SetTransTrue(HWND hwnd, BYTE alpha) {
    if (pSetLayeredWindowAttributes && !(bInSlit && hwnd == hSystemBarExWnd)) {
        SetWindowLongPtr(hwnd, GWL_EXSTYLE, WS_EX_TOOLWINDOW|WS_EX_ACCEPTFILES|WS_EX_LAYERED);
		if (hwnd == hSystemBarExWnd && alpha == 0) {
			pSetLayeredWindowAttributes(hwnd, 0x00000000, 255, LWA_COLORKEY);
		} else {
			pSetLayeredWindowAttributes(hwnd, 0x00000000, alpha, LWA_ALPHA);
		}
    }
}

//---------------------------------------------------------------------------
// Function: windowAll
//---------------------------------------------------------------------------

void winAll_1(HWND h, unsigned d) {
    if (IsIconic(h))
        ShowWindow(h, SW_RESTORE);
}
//------------------------------
void winAll_2(HWND h, unsigned d) {
    if (!IsIconic(h))
        PostMessage(h, WM_SYSCOMMAND, d, 0);
}
//------------------------------
typedef void (*pF_WINALL)(HWND, unsigned);
//------------------------------
void windowAll(unsigned dNow) {
	pF_WINALL f1 = (dNow == SW_RESTORE) ? winAll_1: winAll_2;
	pF_TV_RB f = ToggleInfo.test(TI_TASKS_IN_CURRENT_WORKSPACE) ? retW: retT;
	
	p_TaskList = GetTaskListPtr();  // check this, maybe?
	do if (f()) f1(p_TaskList->hwnd, dNow);
	while (p_TaskList = p_TaskList->next);
}

//---------------------------------------------------------------------------
// Function: UpdateClientRect
//---------------------------------------------------------------------------

void UpdateClientRect() {
    GetClientRect(hSystemBarExWnd, &MainRect);
    MainRect.bottom -= TheBorder;
    MainRect.top += TheBorder;
    MainRect.right -= TheBorder;
    MainRect.left += TheBorder;
}

//---------------------------------------------------------------------------
// Function: RCSettings
//---------------------------------------------------------------------------

void RCSettings(bool read_write) {
    BYTE i = 0, j = 0, k = 0;
	char temp;

#define NUMBER_RCV                  47//33      // total number
#define NUMBER_COLORREF				2
#define NUMBER_DEFAULT_STRINGS      11       // ____strings____ at the front

    struct RCstructVoid {
		char *label;
		unsigned resort;
		void *pV;
	}

	rcV[] = {
        {"BB.Button.Text:", (unsigned)DEFAULT_BBBUTTON_TEXT, BBButtonText},
		{"BB.Button.R.Command:", (unsigned)DEFAULT_BBBUTTON_RCOMMAND, BBButtonRCommand},
		{"BB.Button.L.Command:", (unsigned)DEFAULT_BBBUTTON_LCOMMAND, BBButtonLCommand},
		{"BB.Button.M.Command:", (unsigned)DEFAULT_BBBUTTON_MCOMMAND, BBButtonMCommand},
		{"WindowLabel.R.Command:", (unsigned)DEFAULT_WINDOWLABEL_COMMAND, WindowLabelRCommand},
		{"WindowLabel.L.Command:", (unsigned)DEFAULT_WINDOWLABEL_COMMAND, WindowLabelLCommand},
		{"WindowLabel.M.Command:", (unsigned)DEFAULT_WINDOWLABEL_COMMAND, WindowLabelMCommand},
        {"Inscription.Text:", (unsigned)(isLean ? "BBLEAN": "BLACKBOX"), InscriptionText},
        {"Clock.Tooltip:", (unsigned)DEFAULT_CLOCK_TOOLTIP_FORMAT, ClockTooltipFormat},
        {"Clock.Format:", (unsigned)DEFAULT_CLOCK_FORMAT, ClockFormat},
		{"Placement.Config:", (unsigned)DEFAULT_PLACEMENT_CONFIG_STRING, PlacementConfigString},
		//----------------------------------------------------------------------
		{"toolbar.button.shadowColor:", DEFAULT_SHADOW_COLOR, &ShadowColors[0]},
		{"toolbar.button.pressed.shadowColor:", DEFAULT_SHADOW_COLOR, &ShadowColors[1]},
		//----------------------------------------------------------------------
		{"toolbar.button.shadowX:", DEFAULT_SHADOW_XY, &BShadowX},
		{"toolbar.button.shadowY:", DEFAULT_SHADOW_XY, &BShadowY},
		{"toolbar.button.pressed.shadowX:", DEFAULT_SHADOW_XY, &BPShadowX},
		{"toolbar.button.pressed.shadowY:", DEFAULT_SHADOW_XY, &BPShadowY},
        {"Width.Percent:", DEFAULT_WIDTH_PERCENT, &SystemBarExWidthPercent},
        {"Custom.Height:", DEFAULT_USER_HEIGHT, &CustomHeightSizing},
        {"Window.Alpha:", DEFAULT_WINDOW_ALPHA, &WindowAlpha},
        {"Position.X:", DEFAULT_POSITION_X, &SystemBarExX},
        {"Position.Y:", DEFAULT_POSITION_Y, &SystemBarExY},
        {"Font.Size:", DEFAULT_TASK_FONT_SIZE, &TaskFontSize},
        {"Task.Icon.Size:", DEFAULT_TASK_ICON_SIZE, &TaskIconItem.m_size},
        {"Task.Icon.Sat:", DEFAULT_TASK_ICON_SATURATION, &TaskIconItem.m_sat},
		{"Task.MaxWidth:", DEFAULT_TASK_MAXWIDTH, &TaskMaxWidth},
        {"Task.Icon.Hue:", DEFAULT_TASK_ICON_HUE, &TaskIconItem.m_hue},
        {"Tray.Icon.Size:", DEFAULT_TRAY_ICON_SIZE, &TrayIconItem.m_size},
        {"Tray.Icon.Sat:", DEFAULT_TRAY_ICON_SATURATION, &TrayIconItem.m_sat},
        {"Tray.Icon.Hue:", DEFAULT_TRAY_ICON_HUE, &TrayIconItem.m_hue},
        {"Tooltip.MaxWidth:", DEFAULT_BBTOOLTIP_MAXWIDTH, &TipSettings.max_width},
        {"Tooltip.Distance:", DEFAULT_BBTOOLTIP_DISTANCE, &TipSettings.distance},
        {"Tooltip.Delay:", DEFAULT_BBTOOLTIP_POPUP_DELAY, &TipSettings.delay},
        {"Tooltip.Alpha:", DEFAULT_TOOLTIP_ALPHA, &TipSettings.alpha},
        {".DT:", DEFAULT_TASK_ICONS, &taskIcons},
        {".HT:", DEFAULT_HEIGHT_TYPE, &HeightSizing},
        {".PL:", DEFAULT_PLACEMENT, &SystemBarExPlacement},
        {".SM:", DEFAULT_SHOW_MENU_ITEMS, &TBMenuInfo.bitlist},
        {".SC:", DEFAULT_STYLE_CONFIG, &StyleConfig},
        {".OC:", DEFAULT_OBJECT_CONFIG, &ToggleInfo.bitlist},
		{".EC:", DEFAULT_ENBALE_CONFIG, &EnableInfo.enabled},
        {".R:", DEFAULT_TE_RIGHT, &TaskEventIntArray[0]},
        {".M:", DEFAULT_TE_MIDDLE, &TaskEventIntArray[1]},
        {".L:", DEFAULT_TE_LEFT, &TaskEventIntArray[2]},
		{".RD:", DEFAULT_TE_DBL_RIGHT, &TaskDBLEventIntArray[0]},
        {".MD:", DEFAULT_TE_DBL_MIDDLE, &TaskDBLEventIntArray[1]},
        {".LD:", DEFAULT_TE_DBL_LEFT, &TaskDBLEventIntArray[2]},
    };

    if (read_write == READ) {
		do if (i < NUMBER_DEFAULT_STRINGS) {
			strcpy((char*)(rcV[i].pV), ReadString(rcpath, rcV[i].label, (char*)(rcV[i].resort)));
		} else if ( i >= NUMBER_DEFAULT_STRINGS && i < (NUMBER_DEFAULT_STRINGS + NUMBER_COLORREF) ) {
			ShadowColors[k] = ReadColor(rcpath, rcV[i].label, (char*)rcV[i].resort);
			k++;
		} else {
			*((int*)(rcV[i].pV)) = ReadInt(rcpath, rcV[i].label, rcV[i].resort);
		} while (++i < NUMBER_RCV);

        ToggleInfo.update();
        TBMenuInfo.update();
		EnableInfo.update();

		//We'll have to update the string with the menu changes.
		do {
			temp = PlacementConfigString[j];//temp is needed for atoi to work properly.
			PlacementStruct[j] = atoi(&temp);
			++j;
		} while ( j < NUMBER_ELEMENTS * 2 );

        TipSettings.bTransparency = ToggleInfo.test(TI_TOOLTIPS_TRANS);
        TipSettings.bAbove = ToggleInfo.test(TI_TOOLTIPS_ABOVE_BELOW);
        TipSettings.bSetLabel = ToggleInfo.test(TI_SET_WINDOWLABEL);
        TipSettings.bCenterTip = ToggleInfo.test(TI_TOOLTIPS_CENTER);
        TipSettings.bDocked = ToggleInfo.test(TI_TOOLTIPS_DOCKED);

        UpdatePlacementOrder();
        UpdateFunctionItemArray();

        if (TaskIconItem.m_size < 4) TaskIconItem.m_size = 4;
        if (TaskIconItem.m_size > 255) TaskIconItem.m_size = 255;
        if (TrayIconItem.m_size < 4) TrayIconItem.m_size = 4;
        if (TrayIconItem.m_size > 255) TrayIconItem.m_size = 255;

        if (FileExists(rcpath))  // if there isn't a file, make one
            return;
    }

    i = 0;
	k = 0;
	do if (i < NUMBER_DEFAULT_STRINGS) {
        WriteString(rcpath, rcV[i].label, (char*)(rcV[i].pV));
	} else if ( i >= NUMBER_DEFAULT_STRINGS && i < (NUMBER_DEFAULT_STRINGS + NUMBER_COLORREF) ) {
			WriteColor(rcpath, rcV[i].label, ShadowColors[k]);
			k++;
	} else {
        WriteInt(rcpath, rcV[i].label, *((int*)(rcV[i].pV)));
	} while (++i < NUMBER_RCV);
}

/*void ReadExclusions() {
	//Default file is blank

	//Read each line and store it
	FILE *fp = FileOpen(exclusionpath);
	if ( fp ) {
		for (;ttlExclusions < 24;) {
			char *line, line_buffer[256];
			if ( false == ReadNextCommand(fp, line_buffer, sizeof line_buffer) ) {
				FileClose(fp);
				break;
			}
			strlwr(line = line_buffer);

			int line_len = strlen(line);
			memcpy(sysTrayExList[ttlExclusions].excluded, line, ++line_len);
			ttlExclusions++;
		}
	}
	//The stored lines will be used to filter tray icons

	if (FileExists(exclusionpath)) // if there isn't a file, make one
            return;

}*/

inline int BBDrawTextSBX(HDC hDC, LPCTSTR lpString, int nCount, LPRECT lpRect, UINT uFormat, StyleItem* pSI){
	
	if ((pSI->validated & VALID_SHADOWCOLOR) && (pSI->ShadowColor != ((COLORREF)-1))){ // draw shadow
        RECT rcShadow;
		rcShadow.top = lpRect->top + pSI->ShadowY;
		rcShadow.bottom = lpRect->bottom + pSI->ShadowY;
		rcShadow.left = lpRect->left + pSI->ShadowX;
		rcShadow.right = lpRect->right + pSI->ShadowX;

        SetTextColor(hDC, pSI->ShadowColor);
        DrawText(hDC, lpString, nCount, &rcShadow, uFormat);
    }
	
	if ((pSI->validated & VALID_OUTLINECOLOR) && (pSI->OutlineColor != ((COLORREF)-1))){ // draw outline
		RECT rcOutline;
		_CopyOffsetRect(&rcOutline, lpRect, 1, 0);//Was just CopyRect
		SetTextColor(hDC, pSI->OutlineColor);
			
		DrawText(hDC, lpString, nCount, &rcOutline, uFormat);
		_OffsetRect(&rcOutline,   0,  1);
		DrawText(hDC, lpString, nCount, &rcOutline, uFormat);
		_OffsetRect(&rcOutline,  -1,  0);
		DrawText(hDC, lpString, nCount, &rcOutline, uFormat);
		_OffsetRect(&rcOutline,  -1,  0);
		DrawText(hDC, lpString, nCount, &rcOutline, uFormat);
		_OffsetRect(&rcOutline,   0, -1);
		DrawText(hDC, lpString, nCount, &rcOutline, uFormat);
		_OffsetRect(&rcOutline,   0, -1);
		DrawText(hDC, lpString, nCount, &rcOutline, uFormat);
		_OffsetRect(&rcOutline,   1,  0);
		DrawText(hDC, lpString, nCount, &rcOutline, uFormat);
		_OffsetRect(&rcOutline,   1,  0);
		DrawText(hDC, lpString, nCount, &rcOutline, uFormat);
    }
    // draw text
    SetTextColor(hDC, pSI->TextColor);
    return DrawText(hDC, lpString, nCount, lpRect, uFormat);
}

//---------------------------------------------------------------------------
// Function: UpdateFunctionItemArray
// Purpose:  updates the functions bound to mouse events for the taskbar
//---------------------------------------------------------------------------

void UpdateFunctionItemArray() {
    BYTE i, x, j;
	j = 0;

	for(; j < TE_BUTTON_LENGTH; j++) { // cycle through buttons
		i = x = 0;
		for (; i < (TE_UPDOWN_LENGTH * TE_MODIFIER_LENGTH); i++, x += TE_BITS_PER_FXN) { // cycle through events in each TaskEventConfigX integer
            FunctionItemArray[j * (TE_UPDOWN_LENGTH * TE_MODIFIER_LENGTH) + i].p = pFxnArray[(TaskEventIntArray[j] & (0xF << x)) >> x];
		}
	}
}

//---------------------------------------------------------------------------
// Function: GetStyleSettings
//---------------------------------------------------------------------------

void GetStyleSettings(bool getStyle) {
    BYTE i = 0;

    //----------------------------------------

    ScreenWidth = GetSystemMetrics(SM_CXSCREEN);
    ScreenHeight = GetSystemMetrics(SM_CYSCREEN);

    //----------------------------------------

    if (getStyle) {
		do {
			StyleItemArray[i] = *(StyleItem*)GetSettingPtr(StyleIDList[i]);
		} while (++i < NUMBER_STYLE_ITEMS);

        styleBorderWidth = *(int*)GetSettingPtr(SN_BORDERWIDTH);
        styleBorderColor = *(COLORREF*)GetSettingPtr(SN_BORDERCOLOR);

        InactiveTaskTextColor = StyleItemArray[
            (StyleItemArray[STYLE_TOOLBAR].validated & F_CO3a) ? STYLE_TOOLBAR :
            StyleItemArray[STYLE_TOOLBARWINDOWLABEL].parentRelative ? STYLE_TOOLBARWINDOWLABEL :
            StyleItemArray[STYLE_TOOLBARLABEL].parentRelative ? STYLE_TOOLBARLABEL :
			!StyleItemArray[STYLE_TOOLBARBUTTON_P].parentRelative ? STYLE_TOOLBARBUTTON_P :
            StyleItemArray[STYLE_TOOLBARBUTTON].parentRelative ? STYLE_TOOLBARBUTTON :
            STYLE_TOOLBARWINDOWLABEL
            ].TextColor;

		//ActiveStyleIndex
		//If it's parent relative, set it to the next one on the list until we hit the toolbar.
		ActiveStyleIndex =  !StyleItemArray[STYLE_TOOLBARWINDOWLABEL].parentRelative ? STYLE_TOOLBARWINDOWLABEL:
                            !StyleItemArray[STYLE_TOOLBARLABEL].parentRelative ? STYLE_TOOLBARLABEL:
                            !StyleItemArray[STYLE_TOOLBARCLOCK].parentRelative ? STYLE_TOOLBARCLOCK:
                            !StyleItemArray[STYLE_TOOLBARBUTTON_P].parentRelative ? STYLE_TOOLBARBUTTON_P:
							!StyleItemArray[STYLE_TOOLBARBUTTON].parentRelative ? STYLE_TOOLBARBUTTON:
                            STYLE_TOOLBAR;

        ActiveTaskTextColor = StyleItemArray[ActiveStyleIndex].TextColor;
        ToolbarBevelRaised = (StyleItemArray[STYLE_TOOLBAR].bevelstyle == BEVEL_RAISED);
        TheBorder = bInSlit ? 0: (styleBorderWidth + *(int*)GetSettingPtr(SN_BEVELWIDTH));

        TB_Just = (StyleItemArray[STYLE_TOOLBAR].validated & VALID_JUSTIFY) ? StyleItemArray[STYLE_TOOLBAR].Justify: DT_CENTER;
        UpdatePlacementOrder();

        BYTE s = getSC(STYLE_MENU_TASKS);
        bTaskPR = ((s == STYLE_PARENTRELATIVE) || ((s != STYLE_DEFAULT) && StyleItemArray[s].parentRelative));
    }

    if (ToggleInfo.test(TI_RC_TASK_FONT_SIZE))
        StyleItemArray[STYLE_TOOLBAR].FontHeight = TaskFontSize;

    //----------------------------------------

    DeleteObject((HFONT)SelectObject(bufDC, CreateStyleFont(&StyleItemArray[STYLE_TOOLBAR])));

    if (hMenuFrameFont) DeleteObject(hMenuFrameFont);  // fonts for system menus
    if (hMenuTitleFont) DeleteObject(hMenuTitleFont);

    hMenuFrameFont = CreateStyleFont((StyleItem*)GetSettingPtr(SN_MENUFRAME));
    hMenuTitleFont = CreateStyleFont((StyleItem*)GetSettingPtr(SN_MENUTITLE));

    BYTE s = getSC(STYLE_MENU_TOOLTIPS);
	if (s == STYLE_DEFAULT) {
        TipSettings.pStyle = &StyleItemArray[ActiveStyleIndex];
	} else if (s == STYLE_PARENTRELATIVE) {
        TipSettings.pStyle = &StyleItemArray[STYLE_TOOLBAR];
	} else if (StyleItemArray[s].parentRelative) {
        TooltipPRStyle = StyleItemArray[STYLE_TOOLBAR];
        TooltipPRStyle.TextColor = StyleItemArray[s].TextColor;
        TipSettings.pStyle = &TooltipPRStyle;
	} else {
        TipSettings.pStyle = &StyleItemArray[s];
	}

    TipSettings.pStyleFont = (StyleItem*)GetSettingPtr(SN_TOOLBAR);
    bbTip.UpdateSettings();

	if (p_bbAbout) {
        p_bbAbout->Refresh();
	}

    //----------------------------------------
    GetTextExtentPoint32(bufDC, InscriptionText, strlen(InscriptionText), &InscriptionTextSize);
    TextHeight = InscriptionTextSize.cy;

    //----------------------------------------
    GetTextExtentPoint32(bufDC, BBButtonText, strlen(BBButtonText), &BBButtonTextSize);

    //----------------------------------------
    SetBarHeightAndCoord();  // determine SystemBarExHeight and window coordinates
}


//---------------------------------------------------------------------------
// Function: UpdateClock
//---------------------------------------------------------------------------

void UpdateClock(bool force) {
    bool b;
    time(&SystemTime);                      // get system time
    if (LocalTime = localtime(&SystemTime)) {// adjust time for local time zone
        strftime(CurrentTime, MINI_MAX_LINE_LENGTH, ClockTooltipFormat, LocalTime);

        if (b = (strcmp(CurrentTime, ClockTimeTooltip) != 0))
            strcpy(ClockTimeTooltip, CurrentTime);

        strftime(CurrentTime, MINI_MAX_LINE_LENGTH, ClockFormat, LocalTime);

        if (force) {
            ClockTextWidthOld = 0;
            goto fc;
        }

        if (b || (strcmp(CurrentTime, ClockTime) != 0)) {
fc:         strcpy(ClockTime, CurrentTime);
            GetTextExtentPoint32(bufDC, ClockTime, strlen(ClockTime), &ClockTextSize);

			if (force) {
upd_clw:        ClockTextWidthOld = ClockTextSize.cx;
			} else {
                if (ClockTextWidthOld < ClockTextSize.cx) {
                    RefreshAll();
                    goto upd_clw;
				} else {
                    InvalidateRect(hSystemBarExWnd, &BGItemArray[BG_CLOCK]->placementRect, false);
				}
            }
        }
    }
}

//---------------------------------------------------------------------------
// Function: TrayMouseEvent
//---------------------------------------------------------------------------

inline void TrayMouseEvent(unsigned message) {
    if ((message == WM_RBUTTONDOWN) && TestAsyncKeyState(VK_SHIFT))
        return;

    RECT r;
    systemTray *icon = 0;
    int i = 0, k = 0, j = -1, traysize = GetTraySize(), diff = traysize - SystrayList->size();
    r.top = 0;
    r.bottom = SystemBarExHeight;

    // "k" is the number of hidden icons between the ScreenWidth and i
    // "j" is the int-pointer of the icon that was clicked on

    for (; i < diff; ++i) {
        r.left = BGItemArray[BG_TRAY]->mouseRect.right - INNER_SPACING - (i + 1) * (TrayIconItem.m_size + INNER_SPACING);
        r.right = r.left + TrayIconItem.m_size;
        if (PtInRect(&r, MouseEventPoint))
            break;
    }

    while ((j - k) < i) {
        ++j;
        if (pSysTmp = SystrayList->next)
            do if (j == pSysTmp->v->pseudopointer) {
                ++k;
                break;
            } while (pSysTmp = pSysTmp->next);
    }

    if (j != traysize) {
        icon = GetTrayIcon(j);
        if (!IsWindow(icon->hWnd)) {
            PostMessage(hBlackboxWnd, BB_CLEANTRAY, 0, 0);
            return;
        }

        if (message == WM_MOUSEMOVE)
            goto post_msg;

        if ((message == WM_MBUTTONUP) || ((message == WM_RBUTTONUP) && TestAsyncKeyState(VK_SHIFT))) {
            SystrayList->push_back(new SystrayItem(j, icon));
            bTrayListsUpdated = false;
            RefreshAll();
            ShowPluginMenu(false);
            FocusActiveTask();
        } else {
            if ((message == WM_LBUTTONDOWN) || (message == WM_RBUTTONDOWN))
                SetForegroundWindow(icon->hWnd);
post_msg:   PostMessage(icon->hWnd, icon->uCallbackMessage, WPARAM(icon->uID), LPARAM(message));
        }
    }
}

//---------------------------------------------------------------------------
// Function: WorkspaceMouseEvent
//---------------------------------------------------------------------------

inline void WorkspaceMouseEvent(unsigned message) {
    switch (message) {
        case WM_RBUTTONUP:
            if (TestAsyncKeyState(VK_SHIFT)) {                   // if shift is held down...
                bSystemBarExHidden = true;
                PostMessage(hBlackboxWnd, BB_MENU, 1, 0);       // show the workspace menu
            } else if (!(GetKeyState(VK_MENU) < 0)) {              // else if alt is not held down...
            
                bTaskbarListsUpdated = false;
				if (ToggleInfo.test(TI_INVERT_WORK_SHIFT)) {
					PostMessage(hBlackboxWnd, BB_WORKSPACE, 0, 0);
				} else {
					PostMessage(hBlackboxWnd, BB_WORKSPACE, 1, 0);  // move to next workspace
				}
            }
            break;

        case WM_LBUTTONUP:
            if (!TestAsyncKeyState(VK_SHIFT) && !(GetKeyState(VK_MENU) < 0)) {// if shift and alt are not held down...
                bTaskbarListsUpdated = false;
				if (ToggleInfo.test(TI_INVERT_WORK_SHIFT)) {
					PostMessage(hBlackboxWnd, BB_WORKSPACE, 1, 0);
				} else {
					PostMessage(hBlackboxWnd, BB_WORKSPACE, 0, 0);  // move to previous workspace
				}
            }
            break;

        case WM_MBUTTONUP:
            bTaskbarListsUpdated =
                bIconBuffersUpdated =
                pWindowLabelItem->bgPainted = false;
            PostMessage(hBlackboxWnd, BB_WORKSPACE, 5, 0);      // gather tasks in current workspace
    }
}

//---------------------------------------------------------------------------
// Function: WindowLabelMouseEvent
//---------------------------------------------------------------------------

void WindowLabelMouseEvent(unsigned message) {
#define D_THRES 20
	char		RButtonCommand[MINI_MAX_LINE_LENGTH];//Need it static..
	char		LButtonCommand[MINI_MAX_LINE_LENGTH];//Need it static..
	char		MButtonCommand[MINI_MAX_LINE_LENGTH];//Need it static..

	ProcessStringBroam(RButtonCommand, WindowLabelRCommand); //Make it "safe"
	ProcessStringBroam(LButtonCommand, WindowLabelLCommand); //Make it "safe"
	ProcessStringBroam(MButtonCommand, WindowLabelMCommand); //Make it "safe"

	if ( message == WM_RBUTTONUP && (_stricmp( DEFAULT_WINDOWLABEL_COMMAND, RButtonCommand ) == 0) ||
		 message == WM_LBUTTONUP && (_stricmp( DEFAULT_WINDOWLABEL_COMMAND, LButtonCommand ) == 0) ||
		 message == WM_MBUTTONUP && (_stricmp( DEFAULT_WINDOWLABEL_COMMAND, MButtonCommand ) == 0) ) {
	switch (message) {
		case WM_LBUTTONUP:
			{
				if (MouseEventPoint.x < (WindowLabelX - D_THRES)) {
					PostMessage(hBlackboxWnd, BB_WORKSPACE, 6, (LPARAM)ActiveTaskHwnd);
				} else if (MouseEventPoint.x > (WindowLabelX + D_THRES)) {
					PostMessage(hBlackboxWnd, BB_WORKSPACE, 7, (LPARAM)ActiveTaskHwnd);
				} else {
					bSystemBarExHidden = true;
					PostMessage(hBlackboxWnd, BB_MENU, 1, 0);		// show the workspace menu
				}
			}
			break;

		case WM_RBUTTONUP:
			if (TestAsyncKeyState(VK_SHIFT)) {		// if shift is held down...
		case WM_MBUTTONUP:
				bTaskbarListsUpdated = bIconBuffersUpdated = pWindowLabelItem->bgPainted = false;
				PostMessage(hBlackboxWnd, BB_WORKSPACE, 5, 0);		// gather tasks in current workspace
			} else {
				if (MouseEventPoint.x < (WindowLabelX - D_THRES)) {
					PostMessage(hBlackboxWnd, BB_WORKSPACE, 0, 0);
				} else if (MouseEventPoint.x > (WindowLabelX + D_THRES)) {
					PostMessage(hBlackboxWnd, BB_WORKSPACE, 1, 0);
				} else {
					bSystemBarExHidden = true;
					ShowRootMenu();     // show the desktop menu
				}
			}
		}
	} else {
		if ( message == WM_RBUTTONUP && strlen(WindowLabelRCommand) ) {
			bSystemBarExHidden = true, PostMessage(hBlackboxWnd, BB_EXECUTE, 0, (LPARAM)WindowLabelRCommand);
		} else if ( message == WM_LBUTTONUP && strlen(WindowLabelLCommand) ) {
			bSystemBarExHidden = true, PostMessage(hBlackboxWnd, BB_EXECUTE, 0, (LPARAM)WindowLabelLCommand);
		} else if ( message == WM_MBUTTONUP && strlen(WindowLabelMCommand) ) {
			bSystemBarExHidden = true, PostMessage(hBlackboxWnd, BB_EXECUTE, 0, (LPARAM)WindowLabelMCommand);
		}	 	 
	}
#undef D_THRES
}

//---------------------------------------------------------------------------
// Function: StartButtonMouseEvent
//---------------------------------------------------------------------------
inline void BBButtonMouseEvent(unsigned message) {
	char		ButtonCommand[MINI_MAX_LINE_LENGTH];//Need it static..

	if (message == WM_LBUTTONUP) {
		ProcessStringBroam(ButtonCommand, BBButtonLCommand); //Make it "safe"
		//We want to check the value of the bbbutton.command and exec the broam, or show the menu.
		if ( (_stricmp( "ShowRootMenu", ButtonCommand ) == 0)  || (_stricmp( "BBCore.ShowMenu root", ButtonCommand ) == 0) ) {
			bSystemBarExHidden = true, ShowRootMenu();      // show the desktop menu
		} else if ( (_stricmp( "ShowSBXMenu", ButtonCommand ) == 0) ) {
			ShowPluginMenu(true);  
		} else {
			bSystemBarExHidden = true, PostMessage(hBlackboxWnd, BB_EXECUTE, 0, (LPARAM)BBButtonLCommand);
		}
	} else if (message == WM_RBUTTONUP) {
		ProcessStringBroam(ButtonCommand, BBButtonRCommand); //Make it "safe"
		//We want to check the value of the bbbutton.command and exec the broam, or show the menu.
		if ( _stricmp( "ShowSBXMenu", ButtonCommand ) == 0 ) {
			ShowPluginMenu(true);                           // show the plugin menu
		} else if ( (_stricmp( "ShowRootMenu", ButtonCommand ) == 0)  || (_stricmp( "BBCore.ShowMenu root", ButtonCommand ) == 0) ) {
			bSystemBarExHidden = true, ShowRootMenu();      // show the desktop menu
		} else {
			bSystemBarExHidden = true, PostMessage(hBlackboxWnd, BB_EXECUTE, 0, (LPARAM)BBButtonRCommand);
		}
	} else if (message == WM_MBUTTONUP ) {
		ProcessStringBroam(ButtonCommand, BBButtonRCommand); //Make it "safe"
		//We want to check the value of the bbbutton.command and exec the broam, or show the menu.
		if ( _stricmp( "ShowSBXMenu", ButtonCommand ) == 0 ) {
			ShowPluginMenu(true);                           // show the plugin menu
		} else if ( (_stricmp( "ShowRootMenu", ButtonCommand ) == 0)  || (_stricmp( "BBCore.ShowMenu root", ButtonCommand ) == 0) ) {
			bSystemBarExHidden = true, ShowRootMenu();      // show the desktop menu
		} else {
			bSystemBarExHidden = true, PostMessage(hBlackboxWnd, BB_EXECUTE, 0, (LPARAM)BBButtonMCommand);
		}
	}
}

//
//
//
inline void WorkspaceMoveLMouseEvent(unsigned message) {
	if (message == WM_LBUTTONUP) {
		bTaskbarListsUpdated = false;
		PostMessage(hBlackboxWnd, BB_WORKSPACE, 0, 0);
	}
}
inline void WorkspaceMoveRMouseEvent(unsigned message) {
	if (message == WM_LBUTTONUP) {
		bTaskbarListsUpdated = false;
		PostMessage(hBlackboxWnd, BB_WORKSPACE, 1, 0);
	}
}
//---------------------------------------------------------------------------
// Function: ClockMouseEvent
//---------------------------------------------------------------------------

inline void ClockMouseEvent(unsigned message) {
    switch (message) {
        case WM_LBUTTONUP:
            if (!TestAsyncKeyState(VK_SHIFT) && !(GetKeyState(VK_MENU) < 0)) // if shift and alt are not held down...
                BBExecute(hDesktopWnd, 0, "control.exe", "timedate.cpl", 0, SW_SHOWNORMAL, false);
            break;

        case WM_RBUTTONUP:
            bSystemBarExHidden = true, ShowRootMenu();  // show the desktop menu
    }
}

//---------------------------------------------------------------------------
// Function: TasksMouseEvent
//---------------------------------------------------------------------------

inline bool TasksMouseEvent(unsigned message) {
    bool alt_pressed = (GetKeyState(VK_MENU) < 0);
    pF_TV_RB f = ToggleInfo.test(TI_TASKS_IN_CURRENT_WORKSPACE) ? retW_TI: retT;

    if (alt_pressed) {
        switch (message) {
            case WM_RBUTTONUP:
                if (TaskCount) {
                    if (taskIcons == ICONIZED_ONLY)  // switch between text, icons/text, and iconized
                        taskIcons = TEXT_ONLY;
                    else
                        ++taskIcons;

                    Reconfig(false);
                    RCSettings(WRITE);
                    ShowPluginMenu(false);
                    FocusActiveTask();
                }
            case WM_RBUTTONDOWN:
            case WM_RBUTTONDBLCLK:
                return true;
        }
    }

    if (EnableInfo.test(EI_SHOW_TASKS)) {
        bool bInv = !((message == WM_LBUTTONDOWN) && testTEC(2, 1, TE_WITH_NONE, TE_ACTIVATE_BRINGFRONT));  // ...make a special case...don't invalidate the task when left-button-up will activate the task
        bool bCap = !(
            ToggleInfo.test(TI_ALLOW_GESTURES) &&
            ((message == WM_LBUTTONDOWN) || (message == WM_RBUTTONDOWN)) &&
            !(GetAsyncKeyState(VK_SHIFT) & 0x8000) && !alt_pressed);

        if (pTaskTmp = TaskbarList->next)
            do if (
				CheckButton( message, MouseEventPoint, &pTaskTmp->v->TaskRect, &pTaskTmp->v->bButtonPressed,
				Single_TaskMouseEvent, bCap, bInv )) {
					goto continue_on;
			} while (pTaskTmp = pTaskTmp->next);

        if (pTaskTmp = TaskbarListIconized->next)
            do if ( f() &&
				CheckButton(message, MouseEventPoint, &pTaskTmp->v->TaskRect, &pTaskTmp->v->bButtonPressed,
				Single_TaskMouseEvent, bCap, bInv)) {
					goto continue_on;
			} while (pTaskTmp = pTaskTmp->next);
    }

    return false;

continue_on:
    pTaskbarItemStored = pTaskTmp->v;
    buttonHwnd = pTaskTmp->v->pTaskList->hwnd;
    Single_TaskMouseEvent(message);
    return true;
}

//---------------------------------------------------------------------------
// Function: Single_TaskMouseEvent
//---------------------------------------------------------------------------

void Single_TaskMouseEvent(unsigned message)
{
    unsigned i = 0;
    if (message == WM_MOUSEMOVE)
        return;
    /*else if (message == WM_LBUTTONDBLCLK)
    {
        if (!ToggleInfo.test(TI_SHOW_THEN_MINIMIZE) &&
            !(GetKeyState(VK_MENU) < 0) &&
            !TestAsyncKeyState(VK_SHIFT))
        {
            PostMessage(buttonHwnd, WM_SYSCOMMAND, (IsZoomed(buttonHwnd) ? SC_RESTORE: SC_MAXIMIZE), 0);
            return;
        }
    }*/
	//----------------------------------------------
	else if ( message == WM_RBUTTONDBLCLK ) {
		(*pFxnArray[ (TaskDBLEventIntArray[0]) ])();
		return;
	} else if ( message == WM_MBUTTONDBLCLK ) {
		(*pFxnArray[ (TaskDBLEventIntArray[1]) ])();
		return;
	} else if ( message == WM_LBUTTONDBLCLK ) {
		(*pFxnArray[ (TaskDBLEventIntArray[2]) ])();
		return;
	}

    if (ToggleInfo.test(TI_ALLOW_GESTURES))
        GestureObject.Set(message, buttonHwnd);

    TaskMessage = message;  // used mostly for taskEventFxn1()

    for (; i < (TE_BUTTON_LENGTH * TE_UPDOWN_LENGTH * TE_BITS_PER_FXN); i += TE_BITS_PER_FXN)  // find which event
        if (message == FunctionItemArray[i].click) {
            (*FunctionItemArray[i + ( (GetKeyState(VK_MENU) < 0) ? 
				((GetAsyncKeyState(VK_SHIFT) & 0x8000) ? TE_WITH_ALT_AND_SHIFT: TE_WITH_ALT): ((GetAsyncKeyState(VK_SHIFT) & 0x8000) ? TE_WITH_SHIFT: TE_WITH_NONE)
                )].p)();
            break;
        }
}

//----------------------------------------------
void taskEventFxn0() {}  // do nothing
//----------------------------------------------
void taskEventFxn1() { // activate and bring to foreground
    if (ToggleInfo.test(TI_SHOW_THEN_MINIMIZE) &&
        ((TaskMessage == WM_LBUTTONDOWN) || (TaskMessage == WM_LBUTTONUP)) &&
        (buttonHwnd == ActiveTaskHwnd))
        PostMessage(hBlackboxWnd, BB_WINDOWMINIMIZE, 0, (LPARAM)buttonHwnd);
    else
        ActivateTask(buttonHwnd);

    ActiveTaskHwnd = buttonHwnd;
    InvalidateRect(hSystemBarExWnd, &TaskbarRect, false);
    bCaptureInv = false;
}
//----------------------------------------------
void taskEventFxn2() { // show the task menu
    ShowTaskMenu(buttonHwnd);
}
//----------------------------------------------
void taskEventFxn3() { // minimize task
    PostMessage(hBlackboxWnd, BB_WINDOWMINIMIZE, 0, (LPARAM)buttonHwnd);
    bCaptureInv = false;
}
//----------------------------------------------
void taskEventFxn4() { // maximize task
    PostMessage(hBlackboxWnd, BB_WINDOWMAXIMIZE, 0, (LPARAM)buttonHwnd);
    bCaptureInv = false;
}
//----------------------------------------------
void taskEventFxn5() { // close task
    PostMessage(hBlackboxWnd, BB_WINDOWCLOSE, 0, (LPARAM)buttonHwnd);
    bCaptureInv = false;
}
//----------------------------------------------
void taskEventFxn6() { // toggle iconized
    if ((taskIcons == TEXT_ONLY) || (taskIcons == ICON_AND_TEXT)) {
        pTaskbarItemStored->IsIconized = !pTaskbarItemStored->IsIconized;
        bTaskbarListsUpdated = false;
        RefreshAll();
        FocusActiveTask();
    }
}
//----------------------------------------------
void taskEventFxn7() { // move task to previous workspace
    bTaskbarListsUpdated = false;
    PostMessage(hBlackboxWnd, BB_WORKSPACE, 6, (LPARAM)buttonHwnd);
    bCaptureInv = false;
}
//----------------------------------------------
void taskEventFxn8() { // move task to next workspace
    bTaskbarListsUpdated = false;
    PostMessage(hBlackboxWnd, BB_WORKSPACE, 7, (LPARAM)buttonHwnd);
    bCaptureInv = false;
}
//----------------------------------------------
#define aot		"AOT"
void taskEventFxn9() {
	void	*info = GetProp(hBlackboxWnd, aot);
	
	if ( info ) {
		RemoveProp( hBlackboxWnd, aot );
		SetWindowPos( hBlackboxWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE );
	} else {
		SetProp( hBlackboxWnd, aot, (HANDLE)(TRUE) );
		SetWindowPos( hBlackboxWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE );
	}
	
	PostMessage(hBlackboxWnd, BB_REDRAWGUI, 0, 0);

}
//----------------------------------------------
void taskEventFxn10() {
	PostMessage(hBlackboxWnd, BB_WINDOWSHADE, 0, (LPARAM)buttonHwnd);
}
//----------------------------------------------
void taskEventFxn11() {
	PostMessage(hBlackboxWnd, BB_WORKSPACE, BBWS_TOGGLESTICKY, (LPARAM)buttonHwnd);
	
}

//---------------------------------------------------------------------------
// Function: ShowPluginMenu
//---------------------------------------------------------------------------

void ShowPluginMenu(bool isPopup) {
    int i = 0, j;
    char menuChar1[MINI_MAX_LINE_LENGTH];
	char menuChar2[MINI_MAX_LINE_LENGTH];
	//char menuId[MINI_MAX_LINE_LENGTH];
    popup = isPopup;
    menu_id = 1;

    //---------------------------------------
    if (pTaskEventSubmenuItem) delete pTaskEventSubmenuItem;
    pTaskEventSubmenuItem = new TaskEventItem();
    TaskEventItem **p_array = new TaskEventItem*[TE_FACTOR_LENGTH + 1];
    p_array[0] = pTaskEventSubmenuItem;
    p_array[0]->SetStuff("Mouse", BLANK_TE, 0);
    tmeCycle(p_array, 0, menuChar1);
    delete[] p_array;

    MakeMenuNOP(pTaskEventSubmenuItem->theMenu, 0);
    ToggleInfo.makeMenuItem(pTaskEventSubmenuItem->theMenu, TI_SHOW_THEN_MINIMIZE);
    ToggleInfo.makeMenuItem(pTaskEventSubmenuItem->theMenu, TI_ALLOW_GESTURES);
    MakeMenuNOP(pTaskEventSubmenuItem->theMenu, 0);

	MakeMenuItem(pTaskEventSubmenuItem->theMenu, "<Reset>", "@sbx.DefaultME", false);

	if (pTaskEventSubmenuItemDBL) delete pTaskEventSubmenuItemDBL;
    pTaskEventSubmenuItemDBL = new TaskEventItemDBL();
    TaskEventItemDBL **p_dbl_array = new TaskEventItemDBL*[TE_FACTOR_LENGTH + 1];
    p_dbl_array[0] = pTaskEventSubmenuItemDBL;
    p_dbl_array[0]->SetStuff("Double Click", BLANK_TE, 0);
    tmeDBLCycle(p_dbl_array, 0, menuChar2);
    delete[] p_dbl_array;

	MakeMenuNOP(pTaskEventSubmenuItemDBL->theMenu, 0);

    MakeMenuItem(pTaskEventSubmenuItemDBL->theMenu, "<Reset>", "@sbx.DefaultDME", false);
    //---------------------------------------

    MenuArray[MENU_ID_MISC] = XMakeMenu(PluginMenuText[MENU_ID_MISC]);

    MakeMenuItem(MenuArray[MENU_ID_MISC], "Edit Settings", "@sbx.EditRC", false);
    MakeMenuItem(MenuArray[MENU_ID_MISC], "Reload Settings", "@sbx.ReadRC", false);
    MakeMenuNOP(MenuArray[MENU_ID_MISC], 0);
    MakeMenuItem(MenuArray[MENU_ID_MISC], "Refresh Margin", "@sbx.RefreshMargin", false);

    if (isLean) {
        MakeMenuNOP(MenuArray[MENU_ID_MISC], 0);
        MakeMenuItem(MenuArray[MENU_ID_MISC], "Edit Workspace Name", "@sbx.WorkspaceName", false);
    }
	ToggleInfo.makeMenuItem(MenuArray[MENU_ID_MISC], TI_INVERT_WORK_SHIFT);
	MakeMenuNOP(MenuArray[MENU_ID_MISC], 0);

    MakeMenuNOP(MenuArray[MENU_ID_MISC], 0);
    ToggleInfo.makeMenuItem(MenuArray[MENU_ID_MISC], TI_TOGGLE_WITH_PLUGINS);
    ToggleInfo.makeMenuItem(MenuArray[MENU_ID_MISC], TI_HIDE_MENU_ON_HOVER);

    //---------------------------------------

    MenuArray[MENU_ID_EDIT_STRING] = XMakeMenu("Edit");
    MakeMenuItemString(MenuArray[MENU_ID_EDIT_STRING], "Clock", "@sbx.String.Clock", ClockFormat);
    MakeMenuItemString(MenuArray[MENU_ID_EDIT_STRING], "Clock Tip", "@sbx.String.ClockTip", ClockTooltipFormat);
    MakeMenuItemString(MenuArray[MENU_ID_EDIT_STRING], "Inscription", "@sbx.String.Inscription", InscriptionText);
	MakeMenuNOP(MenuArray[MENU_ID_EDIT_STRING], 0);
    MakeMenuItemString(MenuArray[MENU_ID_EDIT_STRING], "Button Text", "@sbx.String.BBButton", BBButtonText);
	MakeMenuItemString(MenuArray[MENU_ID_EDIT_STRING], "Right Command", "@sbx.String.BBButtonRCommand", BBButtonRCommand);
	MakeMenuItemString(MenuArray[MENU_ID_EDIT_STRING], "Left Command", "@sbx.String.BBButtonLCommand", BBButtonLCommand);
	MakeMenuItemString(MenuArray[MENU_ID_EDIT_STRING], "Middle Command", "@sbx.String.BBButtonMCommand", BBButtonMCommand);
	MakeMenuNOP(MenuArray[MENU_ID_EDIT_STRING], 0);
	MakeMenuItemString(MenuArray[MENU_ID_EDIT_STRING], "WinLabel Right", "@sbx.String.WinLabelRCommand", WindowLabelRCommand);
	MakeMenuItemString(MenuArray[MENU_ID_EDIT_STRING], "WinLabel Left", "@sbx.String.WinLabelLCommand", WindowLabelLCommand);
	MakeMenuItemString(MenuArray[MENU_ID_EDIT_STRING], "WinLabel Middle", "@sbx.String.WinLabelMCommand", WindowLabelMCommand);

    //---------------------------------------

    MenuArray[MENU_ID_TASKMENUITEMS] = XMakeMenu("Menu");

    TBMenuInfo.reset();
    do sprintf(menuChar1, "@sbx.TaskMenu.%d", TBMenuInfo.n),
        MakeMenuItem(MenuArray[MENU_ID_TASKMENUITEMS], TBMenuInfo.getText(), menuChar1, TBMenuInfo.test());
    while (++TBMenuInfo < MI_NUMBER);

    //---------------------------------------

    MenuArray[MENU_ID_TOOLTIP_PLACEMENT] = XMakeMenu("Placement");

    MakeMenuItem(MenuArray[MENU_ID_TOOLTIP_PLACEMENT], "Above", ToggleInfo.getBroam(TI_TOOLTIPS_ABOVE_BELOW), ToggleInfo.test(TI_TOOLTIPS_ABOVE_BELOW));
    MakeMenuItem(MenuArray[MENU_ID_TOOLTIP_PLACEMENT], "Below", ToggleInfo.getBroam(TI_TOOLTIPS_ABOVE_BELOW), !ToggleInfo.test(TI_TOOLTIPS_ABOVE_BELOW));
    MakeMenuNOP(MenuArray[MENU_ID_TOOLTIP_PLACEMENT], 0);
    MakeMenuItem(MenuArray[MENU_ID_TOOLTIP_PLACEMENT], "Left", ToggleInfo.getBroam(TI_TOOLTIPS_CENTER), !ToggleInfo.test(TI_TOOLTIPS_CENTER));
    MakeMenuItem(MenuArray[MENU_ID_TOOLTIP_PLACEMENT], "Center", ToggleInfo.getBroam(TI_TOOLTIPS_CENTER), ToggleInfo.test(TI_TOOLTIPS_CENTER));
    MakeMenuNOP(MenuArray[MENU_ID_TOOLTIP_PLACEMENT], 0);
    ToggleInfo.makeMenuItem(MenuArray[MENU_ID_TOOLTIP_PLACEMENT], TI_TOOLTIPS_DOCKED);
    if (isLean) {
        MakeMenuNOP(MenuArray[MENU_ID_TOOLTIP_PLACEMENT], 0);
        IM_Array[INT_TOOLTIP_DISTANCE].CreateIntMenu(MenuArray[MENU_ID_TOOLTIP_PLACEMENT]);
    }

    //---------------------------------------

    MenuArray[MENU_ID_TOOLTIPS] = XMakeMenu(PluginMenuText[MENU_ID_TOOLTIPS]);

    ToggleInfo.makeMenuItem(MenuArray[MENU_ID_TOOLTIPS], TI_TOOLTIPS_TRAY);
    ToggleInfo.makeMenuItem(MenuArray[MENU_ID_TOOLTIPS], TI_TOOLTIPS_CLOCK);
    ToggleInfo.makeMenuItem(MenuArray[MENU_ID_TOOLTIPS], TI_TOOLTIPS_TASKS);

    MakeMenuNOP(MenuArray[MENU_ID_TOOLTIPS], 0);

    if (isLean) {
        //ToggleInfo.makeMenuItem(MenuArray[MENU_ID_TOOLTIPS], TI_TOOLTIPS_TRANS);
        IM_Array[INT_TOOLTIP_ALPHA].CreateIntMenu(MenuArray[MENU_ID_TOOLTIPS]);

        MakeMenuNOP(MenuArray[MENU_ID_TOOLTIPS], 0);
        ToggleInfo.makeMenuItem(MenuArray[MENU_ID_TOOLTIPS], TI_SET_WINDOWLABEL);

        MakeMenuNOP(MenuArray[MENU_ID_TOOLTIPS], 0);
        MakeSubmenu(MenuArray[MENU_ID_TOOLTIPS], MenuArray[MENU_ID_TOOLTIP_PLACEMENT], "Placement");
        IM_Array[INT_TOOLTIP_MAXWIDTH].CreateIntMenu(MenuArray[MENU_ID_TOOLTIPS]);
        IM_Array[INT_TOOLTIP_DELAY].CreateIntMenu(MenuArray[MENU_ID_TOOLTIPS]);
    } else {
       // ToggleInfo.makeMenuItem(MenuArray[MENU_ID_TOOLTIPS], TI_TOOLTIPS_TRANS);
        ToggleInfo.makeMenuItem(MenuArray[MENU_ID_TOOLTIPS], TI_SET_WINDOWLABEL);
        MakeMenuNOP(MenuArray[MENU_ID_TOOLTIPS], 0);
        MakeSubmenu(MenuArray[MENU_ID_TOOLTIPS], MenuArray[MENU_ID_TOOLTIP_PLACEMENT], "Placement");
    }

    //---------------------------------------

    MenuArray[MENU_ID_WINDOW] = XMakeMenu(PluginMenuText[MENU_ID_WINDOW]);

    if (isLean) {
        //ToggleInfo.makeMenuItem(MenuArray[MENU_ID_WINDOW], TI_WINDOW_TRANS);
        IM_Array[INT_WINDOW_ALPHA].CreateIntMenu(MenuArray[MENU_ID_WINDOW]);

        MakeMenuNOP(MenuArray[MENU_ID_WINDOW], 0);
        ToggleInfo.makeMenuItem(MenuArray[MENU_ID_WINDOW], TI_RC_TASK_FONT_SIZE);
        IM_Array[INT_TASK_FONTSIZE].CreateIntMenu(MenuArray[MENU_ID_WINDOW]);
    } else {
        //ToggleInfo.makeMenuItem(MenuArray[MENU_ID_WINDOW], TI_WINDOW_TRANS);
        ToggleInfo.makeMenuItem(MenuArray[MENU_ID_WINDOW], TI_RC_TASK_FONT_SIZE);
    }

    MakeMenuNOP(MenuArray[MENU_ID_WINDOW], 0);

    //---------------------------------------

    if (isLean)
    {
        MenuArray[MENU_ID_TRAY] = XMakeMenu("Tray");
        IM_Array[INT_TRAY_ICON_SAT].CreateIntMenu(MenuArray[MENU_ID_TRAY]);
        IM_Array[INT_TRAY_ICON_HUE].CreateIntMenu(MenuArray[MENU_ID_TRAY]);
        IM_Array[INT_TRAY_ICON_SIZE].CreateIntMenu(MenuArray[MENU_ID_TRAY]);
    }

    //---------------------------------------

    MenuArray[MENU_ID_TASKS] = XMakeMenu("Tasks");

    i = 0;
    do sprintf(menuChar1, "@sbx.TaskDisp.%d", i),
        MakeMenuItem(MenuArray[MENU_ID_TASKS], TaskDisplayNames[i], menuChar1, (taskIcons == i));
    while (++i < NUMBER_TASK_TYPE);

    MakeMenuNOP(MenuArray[MENU_ID_TASKS], 0);
	IM_Array[INT_TASK_MAXWIDTH].CreateIntMenu(MenuArray[MENU_ID_TASKS]);
	MakeMenuNOP(MenuArray[MENU_ID_TASKS], 0);

    ToggleInfo.makeMenuItem(MenuArray[MENU_ID_TASKS], TI_FLASH_TASKS);

    ToggleInfo.makeMenuItem(MenuArray[MENU_ID_TASKS], TI_COMPRESS_ICONIZED);
    ToggleInfo.makeMenuItem(MenuArray[MENU_ID_TASKS], TI_REVERSE_TASKS);
    ToggleInfo.makeMenuItem(MenuArray[MENU_ID_TASKS], TI_SAT_HUE_ON_ACTIVE_TASK);
    ToggleInfo.makeMenuItem(MenuArray[MENU_ID_TASKS], TI_TASKS_IN_CURRENT_WORKSPACE);

    MakeMenuNOP(MenuArray[MENU_ID_TASKS], 0);
    MakeSubmenu(MenuArray[MENU_ID_TASKS], MenuArray[MENU_ID_TASKMENUITEMS], "Menu");
    MakeSubmenu(MenuArray[MENU_ID_TASKS], pTaskEventSubmenuItem->theMenu, pTaskEventSubmenuItem->theText);
	MakeSubmenu(MenuArray[MENU_ID_TASKS], pTaskEventSubmenuItemDBL->theMenu, pTaskEventSubmenuItemDBL->theText);
    if (isLean) {
        IM_Array[INT_TASK_ICON_SAT].CreateIntMenu(MenuArray[MENU_ID_TASKS]);
        IM_Array[INT_TASK_ICON_HUE].CreateIntMenu(MenuArray[MENU_ID_TASKS]);
        IM_Array[INT_TASK_ICON_SIZE].CreateIntMenu(MenuArray[MENU_ID_TASKS]);
    }

    //---------------------------------------

    MenuArray[MENU_ID_ORDER] = XMakeMenu("Order");

    i = 0;
    do {
       // j = getPC(i);
        if (EnableInfo.test(PlacementStruct[i] == BG_TASKS ? EI_SHOW_TASKS : BGItemArray[PlacementStruct[i]]->ocIndex)) {
            sprintf(menuChar1, "@sbx.Order.%d", i);
            MakeMenuItem(MenuArray[MENU_ID_ORDER], ElementNames[PlacementStruct[i]], menuChar1, false);
        }
    } while (++i < NUMBER_ELEMENTS);

    //---------------------------------------

    MenuArray[MENU_ID_HEIGHT] = XMakeMenu("Height");

    i = 0;
    do sprintf(menuChar1, "@sbx.Height.%d", i),
        MakeMenuItem(MenuArray[MENU_ID_HEIGHT], HeightSettingNames[i], menuChar1, HeightSizing == i);
    while (++i < NUMBER_HEIGHT);

    if (isLean) {
        MakeMenuNOP(MenuArray[MENU_ID_HEIGHT], 0);
        IM_Array[INT_USER_HEIGHT].CreateIntMenu(MenuArray[MENU_ID_HEIGHT]);
    }

    //---------------------------------------

    MenuArray[MENU_ID_STYLE] = XMakeMenu("Styles");

    int k = -1;
    j = NUMBER_BEGIN_STYLE_MENUS;
    do {
        i = NUMBER_STYLE_NAMES - 1;
        MenuArray[j] = XMakeMenu(StyleMenuNames[++k]);
        do {
            sprintf(menuChar1, "@sbx.Style %d %d", k, i);
            MakeMenuItem(MenuArray[j], StyleDisplayNames[i], menuChar1, testSC(k, i));
        }
        while (--i != -1);
        MakeSubmenu(MenuArray[MENU_ID_STYLE], MenuArray[j], StyleMenuNames[k]);
    }
    while (++j < NUMBER_MENU_OBJECTS);

	MakeMenuNOP(MenuArray[MENU_ID_STYLE], 0);
	ToggleInfo.makeMenuItem(MenuArray[MENU_ID_STYLE], TI_PLUGINS_SHADOWS);
    MakeMenuNOP(MenuArray[MENU_ID_STYLE], 0);

    MakeMenuItem(MenuArray[MENU_ID_STYLE], "<Default>", "@sbx.Style.Default", false);

    MakeMenuItem(MenuArray[MENU_ID_STYLE], "<Auto>", "@sbx.Style.Auto", false);

    MakeMenuItem(MenuArray[MENU_ID_STYLE], "<PR>", "@sbx.Style.PR", false);

    //---------------------------------------

    MenuArray[MENU_ID_PLACEMENT] = XMakeMenu("Placement");

    ToggleInfo.makeMenuItem(MenuArray[MENU_ID_PLACEMENT], TI_AUTOHIDE);
    ToggleInfo.makeMenuItem(MenuArray[MENU_ID_PLACEMENT], TI_LOCK_POSITION);
    ToggleInfo.makeMenuItem(MenuArray[MENU_ID_PLACEMENT], TI_SNAP_TO_EDGE);
    ToggleInfo.makeMenuItem(MenuArray[MENU_ID_PLACEMENT], TI_ALWAYS_ON_TOP);
    MakeMenuNOP(MenuArray[MENU_ID_PLACEMENT], 0);
    i = 0;
    do sprintf(menuChar1, "@sbx.Placement.%d", i),
        MakeMenuItem(MenuArray[MENU_ID_PLACEMENT], PlacementNames[i], menuChar1, (SystemBarExPlacement == i));
    while (++i < NUMBER_PLACEMENT);

    if (isLean) {
        MakeMenuNOP(MenuArray[MENU_ID_PLACEMENT], 0);
        IM_Array[INT_CUSTOM_X].CreateIntMenu(MenuArray[MENU_ID_PLACEMENT]);
        IM_Array[INT_CUSTOM_Y].CreateIntMenu(MenuArray[MENU_ID_PLACEMENT]);
        IM_Array[INT_WIDTH_PERCENT].CreateIntMenu(MenuArray[MENU_ID_PLACEMENT]);
    }

    //---------------------------------------

    MenuArray[MENU_ID_ENABLED] = XMakeMenu("Enabled");

	if (GetTraySize() == SystrayList->size()) {
        MakeMenuItem(MenuArray[MENU_ID_ENABLED], RestoreAllChar, "@sbx.AllIcons", false);
	} else {
        EnableInfo.makeMenuItem(MenuArray[MENU_ID_ENABLED], EI_SHOW_TRAY);
	}
    EnableInfo.makeMenuItem(MenuArray[MENU_ID_ENABLED], EI_SHOW_CLOCK);
    EnableInfo.makeMenuItem(MenuArray[MENU_ID_ENABLED], EI_SHOW_TASKS);
	EnableInfo.makeMenuItem(MenuArray[MENU_ID_ENABLED], EI_SHOW_WORKMOVELEFT1);
	EnableInfo.makeMenuItem(MenuArray[MENU_ID_ENABLED], EI_SHOW_WORKMOVERIGHT1);
    EnableInfo.makeMenuItem(MenuArray[MENU_ID_ENABLED], EI_SHOW_BBBUTTON);
    EnableInfo.makeMenuItem(MenuArray[MENU_ID_ENABLED], EI_SHOW_WORKSPACE);
	//EnableInfo.makeMenuItem(MenuArray[MENU_ID_ENABLED], EI_SHOW_WORKMOVELEFT2);
	//EnableInfo.makeMenuItem(MenuArray[MENU_ID_ENABLED], EI_SHOW_WORKMOVERIGHT2);
    MakeMenuNOP(MenuArray[MENU_ID_ENABLED], 0);
    EnableInfo.makeMenuItem(MenuArray[MENU_ID_ENABLED], EI_SHOW_WINDOWLABEL);
    EnableInfo.makeMenuItem(MenuArray[MENU_ID_ENABLED], EI_SHOW_INSCRIPTION);

    //---------------------------------------

	if (isLean) {
        MakeSubmenu(MenuArray[MENU_ID_WINDOW], MenuArray[MENU_ID_TRAY], "Tray");
	}
	MakeSubmenu(MenuArray[MENU_ID_WINDOW], MenuArray[MENU_ID_TASKS], "Tasks");
	MakeSubmenu(MenuArray[MENU_ID_WINDOW], MenuArray[MENU_ID_ENABLED], "Enabled");
	MakeSubmenu(MenuArray[MENU_ID_WINDOW], MenuArray[MENU_ID_PLACEMENT], "Placement");
	MakeSubmenu(MenuArray[MENU_ID_WINDOW], MenuArray[MENU_ID_HEIGHT], "Height");
	MakeSubmenu(MenuArray[MENU_ID_WINDOW], MenuArray[MENU_ID_ORDER], "Order");
	MakeSubmenu(MenuArray[MENU_ID_WINDOW], MenuArray[MENU_ID_EDIT_STRING], "Edit");

    //---------------------------------------

    MenuArray[MENU_ID_PLUGIN_MENU] = XMakeMenu(ModInfo.Get(ModInfo.NAME_VERSION));

    MakeMenuItem(MenuArray[MENU_ID_PLUGIN_MENU], "About", "@sbx.About", false);

    MakeMenuItem(MenuArray[MENU_ID_PLUGIN_MENU], "Documentation", "@sbx.Doc", false);
    MakeMenuNOP(MenuArray[MENU_ID_PLUGIN_MENU], 0);

    i = 0;
    do MakeSubmenu(MenuArray[MENU_ID_PLUGIN_MENU], MenuArray[i], PluginMenuText[i]);
    while (++i < NUMBER_PLUGIN_MENU_LIST);

    if (EnableInfo.test(EI_SHOW_TRAY) && (pSysTmp = SystrayList->next)) {
        MakeMenuNOP(MenuArray[MENU_ID_PLUGIN_MENU], 0);
        MakeMenuItem(MenuArray[MENU_ID_PLUGIN_MENU], RestoreAllChar, "@sbx.AllIcons", false);
        do sprintf(menuChar1, "@sbx.Tray %d", (unsigned)pSysTmp->v),
            MakeMenuItem(MenuArray[MENU_ID_PLUGIN_MENU], pSysTmp->v->pSystrayStructure->szTip, menuChar1, false),
            strcpy(pSysTmp->v->tip, pSysTmp->v->pSystrayStructure->szTip);
        while(pSysTmp = pSysTmp->next);
    }

    //---------------------------------------

    bSystemBarExHidden = true;
    ShowMenu(MenuArray[MENU_ID_PLUGIN_MENU]);
}

//---------------------------------------------------------------------------
// Function: tmeCycle
// Purpose: used in creating taskbar-mouse-events submenu in ShowPluginMenu()
//---------------------------------------------------------------------------

inline void tmeCycle(TaskEventItem **p, unsigned index, char *char1) {
    if (index < TE_FACTOR_LENGTH) {
        p[index]->CreateMenuArray(index + 2);
        p[index + 1] = p[index]->pArray;
        do tmeCycle(p, (index + 1), char1);
        while ((++p[index + 1])->ID);
        if (index)
            MakeSubmenu(p[index - 1]->theMenu, p[index]->theMenu, p[index]->theText);
    } else {
        BYTE i = 1;
		sprintf(char1, "@%s.TaskEvent ", "sbx");
        do sprintf(char1, "%s%d ", char1, p[i]->theIndex);
        //do sprintf(char1, "@sbx.TaskEvent %d ", p[i]->theIndex);
        while (++i < (TE_FACTOR_LENGTH + 1));
        MakeMenuItem(p[3]->theMenu, p[4]->theText, char1, testTEC(p[1]->theIndex, p[2]->theIndex, p[3]->theIndex, p[4]->ID));
    }
}

inline void tmeDBLCycle(TaskEventItemDBL **p, unsigned index, char *char1) {
    if (index < TE_FACTOR_LENGTH - 2) {
        p[index]->CreateMenuArray(index + 2);
        p[index + 1] = p[index]->pArray;
		do {
			tmeDBLCycle(p, (index + 1), char1);
		} while ((++p[index + 1])->ID);
		if (index) {
            MakeSubmenu(p[index - 1]->theMenu, p[index]->theMenu, p[index]->theText);
		}
    } else {
        BYTE i = 1;
		sprintf(char1, "@%s.TaskEventDBL ", "sbx");
        do sprintf(char1, "%s%d ", char1, p[i]->theIndex);
        //do sprintf(char1, "@sbx.TaskEventDBL %d ", p[i]->theIndex);
        while (++i < TE_FACTOR_LENGTH - 1 );
        MakeMenuItem(p[1]->theMenu, p[2]->theText, char1, testDBLTEC(p[1]->theIndex, p[2]->ID));
    }
}

//---------------------------------------------------------------------------
// Function: ShowTaskMenu
//---------------------------------------------------------------------------

inline void ShowTaskMenu(HWND hwnd) {
    SIZE sz;
    bool bNOP;
    HMENU hMenu;
    TaskMenuLine *tm;
    int count, i = 0;
    char *tx = pTaskbarItemStored->pTaskList->caption;

    // get the largest text-width in the menu, plus the width of an ellipsis, plus margin

    HFONT hFontTemp = (HFONT)SelectObject(bufDC, hMenuFrameFont);  // dump out toolbar font temporarily

    int pix_max = 16 + GetTaskMenuInfo(hwnd, &hMenu, &tm, &count, &bNOP);
    if (MenuArray[MENU_ID_TASKBAR_MENU])
        DelMenu(MenuArray[MENU_ID_TASKBAR_MENU]);

    int caption_len = strlen(tx);
    int *len_array = new int[caption_len];
    if (!caption_len)
        goto make_menu;

    SelectObject(bufDC, hMenuTitleFont);

    GetTextExtentExPoint(bufDC, tx, caption_len, pix_max, &i, len_array, &sz);
    if (i < caption_len) {
        int *g = len_array + i;
        pix_max -= GetItemLength("...", &sz);
        while (*(--g) > pix_max) --i;
        char buf[200];
        strncpy(buf, tx, i);
        buf[i] =
            buf[i + 1] =
            buf[i + 2] = '.';
        buf[i + 3] = 0;
        MenuArray[MENU_ID_TASKBAR_MENU] = MakeMenu(buf);  // create menu object
    }
    else
make_menu:
        MenuArray[MENU_ID_TASKBAR_MENU] = MakeMenu(tx);  // create menu object

    SelectObject(bufDC, hFontTemp);  // select the toolbar font back in

    delete[] len_array;

    TBMenuInfo.reset();
	if (taskmenu_scaffold[TBMenuInfo.n]) {
        MakeMenuItem(MenuArray[MENU_ID_TASKBAR_MENU], TBMenuInfo.getText(), "@sbx.AllIcons", false);
	}

	if (taskmenu_scaffold[++TBMenuInfo]) {
        MakeMenuItem(MenuArray[MENU_ID_TASKBAR_MENU], TBMenuInfo.getText(), "@sbx.NoIconized", false);
	}

    do if (taskmenu_scaffold[++TBMenuInfo])
        MakeMenuItem(
            MenuArray[MENU_ID_TASKBAR_MENU],
            TBMenuInfo.getText(),
            (TBMenuInfo.n > (MI_BEGIN_SINGLE_TASK - 1)) ? BuildSingleTaskBroam(hwnd, tm_buf_data, (TB_MAX + TB_OVERFLOW)): TBMenuInfo.getBroam(),
            false);
    while (TBMenuInfo.n < MI_BEGIN_SHELL_ITEMS);

    for (i = 0; i < count; ++i)
    {
        if (!tm[i].m_enabled) continue;
        if (tm[i].m_buf[0])
            PickTaskMenuItem(hwnd, MenuArray[MENU_ID_TASKBAR_MENU], tm[i].m_hsubmenu, i, tm[i].m_state, tm[i].m_id, tm[i].m_buf, &bNOP);
        else
            bNOP = true;
    }

    delete[] tm;

    bSystemBarExHidden = true;
    ShowMenu(MenuArray[MENU_ID_TASKBAR_MENU]);
}

//---------------------------------------------------------------------------
// Function: GetItemLength
//---------------------------------------------------------------------------

int GetItemLength(char *txt, SIZE *sz) {
	GetTextExtentPoint32(bufDC, txt, strlen(txt), sz);
	return sz->cx;
}

//---------------------------------------------------------------------------
// Function: GetTaskMenuInfo  ...used in ShowTaskMenu()
//---------------------------------------------------------------------------

inline int GetTaskMenuInfo(HWND hwnd, HMENU *p_hMenu, TaskMenuLine **tm2, int *p_count, bool *p_is_sbx) {
    int len;
    BYTE b = 0;
    TaskMenuLine *tm;
    UINT max_len = 0, i = 0;
    bool is_system = false;
    SIZE sz;

    ZeroMemory((void*)&taskmenu_scaffold, sizeof(taskmenu_scaffold));

    if (TaskbarList->next)
        b += TaskbarList->next->next ? 2: 1;

    if (TaskbarListIconized->next)
        b += TaskbarListIconized->next->next ? 2: 1;

    if (!b && !SystrayList->next) return 0;

    //---------------------------------------

    TBMenuInfo.reset();
    if (TBMenuInfo.test() && EnableInfo.test(EI_SHOW_TRAY) && SystrayList->next)
        max_len = GetItemLength(TBMenuInfo.getText(), &sz),
        taskmenu_scaffold[TBMenuInfo.n] = true;

    ++TBMenuInfo;
    if (TBMenuInfo.test() && TaskbarListIconized->next)
        max_len = max(max_len, GetItemLength(TBMenuInfo.getText(), &sz)),
        taskmenu_scaffold[TBMenuInfo.n] = true;

    //---------------------------------------

    if (b > 1) {
        while (++TBMenuInfo < MI_BEGIN_SINGLE_TASK)
            if (TBMenuInfo.test())
                max_len = max(max_len, GetItemLength(TBMenuInfo.getText(), &sz)),
                taskmenu_scaffold[TBMenuInfo.n] = true;
    }
    else ++TBMenuInfo;

    //---------------------------------------

    do if (TBMenuInfo.test())
        max_len = max(max_len, GetItemLength(TBMenuInfo.getText(), &sz)),
        taskmenu_scaffold[TBMenuInfo.n] = true;
    while (++TBMenuInfo < MI_BEGIN_SHELL_ITEMS);

    *p_is_sbx = max_len > 0;
    *p_hMenu = GetSystemMenu(hwnd, false);
    *p_count = GetMenuItemCount(*p_hMenu);

    if (*p_count < 0) {
        GetSystemMenu(hwnd, true);                      // if the menu's empty, reset the default
        *p_hMenu = GetSystemMenu(hwnd, false);          // try again to get a handle
        *p_count = GetMenuItemCount(*p_hMenu);          // get the new menu-item count
    }

    *tm2 = new TaskMenuLine[*p_count];

    SendMessageTimeout(hwnd, WM_INITMENU, (WPARAM)*p_hMenu, 0, SMTO_ABORTIFHUNG|SMTO_BLOCK, 100, 0);

    for (; i < *p_count; ++i) {
        tm = &(*tm2)[i];

        menuInfo.wID = 0;
        menuInfo.dwTypeData = 0;
        GetMenuItemInfo(*p_hMenu, i, true, &menuInfo);  // get the length of the text
        len = ++menuInfo.cch;
        menuInfo.dwTypeData = tm->m_buf;
        GetMenuItemInfo(*p_hMenu, i, true, &menuInfo);  // get text into buffer and get id
        tm->m_buf[min(TB_MAX, len)] = 0; // just use a big buffer and 0-terminate it

        if (tm->m_buf[0]) {
            CleanMenuItem(tm->m_buf, len);                      // clean up the menu-item

            if (!strcmp(tm->m_buf, "Restore"))          { if (!TBMenuInfo.test(MI_SHELL_RESTORE))   continue; }
            else if (!strcmp(tm->m_buf, "Move"))        { if (!TBMenuInfo.test(MI_SHELL_MOVE))      continue; }
            else if (!strcmp(tm->m_buf, "Size"))        { if (!TBMenuInfo.test(MI_SHELL_SIZE))      continue; }
            else if (!strcmp(tm->m_buf, "Minimize"))    { if (!TBMenuInfo.test(MI_SHELL_MINIMIZE))  continue; }
            else if (!strcmp(tm->m_buf, "Maximize"))    { if (!TBMenuInfo.test(MI_SHELL_MAXIMIZE))  continue; }
            else if (!strcmp(tm->m_buf, "Close"))       { if (!TBMenuInfo.test(MI_SHELL_CLOSE))     continue; }
            else if (!TBMenuInfo.test(MI_SHELL_SPECIAL))                                            continue;

            max_len = max(max_len, GetItemLength(tm->m_buf, &sz));
        }
        else if (!is_system) continue;

        tm->m_hsubmenu = menuInfo.hSubMenu;
        tm->m_state = menuInfo.fState;
        tm->m_id = menuInfo.wID;

        tm->m_enabled = true;
        is_system = true;
    }

    return max_len;
}

//---------------------------------------------------------------------------
// Function: BuildSingleTaskBroam  ...used in ShowTaskMenu(), above
//---------------------------------------------------------------------------

inline char *BuildSingleTaskBroam(HWND hwnd, char *buf, int len) {
    sprintf(buf, "%s %d", TBMenuInfo.getBroam(), hwnd);
    buf[len - 1] = 0;
    return buf;
}

//---------------------------------------------------------------------------
// Function: MakeSystemMenu
//---------------------------------------------------------------------------

Menu *MakeSystemSubmenu(HWND target_hwnd, char *name, HMENU hMenu) {
    bool bNOP = false;
    char buf_local[TB_MAX];
    Menu *parent_menu = MakeMenu(name);
    int len, i = 0, count = GetMenuItemCount(hMenu);

	if (!count) {
        MakeMenuNOP(parent_menu, "[empty]");
	} else do {
        menuInfo.wID = 0;
        menuInfo.dwTypeData = 0;
        GetMenuItemInfo(hMenu, i, true, &menuInfo);  // get the length of the text
        len = ++menuInfo.cch;
        menuInfo.dwTypeData = buf_local;
        GetMenuItemInfo(hMenu, i, true, &menuInfo);  // get text into buffer and get id
        buf_local[min(TB_MAX, len)] = 0; // just use a big buffer and 0-terminate it

        if (buf_local[0]) {
            CleanMenuItem(buf_local, len);
            PickTaskMenuItem(target_hwnd, parent_menu, menuInfo.hSubMenu, i, menuInfo.fState, menuInfo.wID, buf_local, &bNOP);
		} else {
            bNOP = true;
		}
    } while (++i < count);

    return parent_menu;
}

//---------------------------------------------------------------------------
// Function: MakeSystemMenu  ...inner block of loop for making a system menu
//---------------------------------------------------------------------------

void PickTaskMenuItem(HWND target_hwnd, Menu *parent_menu, HMENU hsubmenu, int index, int state, int id, char *buf, bool *nop) {
	if (*nop) {
        *nop = false, MakeMenuNOP(parent_menu, 0);
	}

    if (hsubmenu) {
        SendMessageTimeout(target_hwnd, WM_INITMENUPOPUP, (WPARAM)hsubmenu, MAKELPARAM(index, true), SMTO_ABORTIFHUNG|SMTO_BLOCK, 100, 0);
        MakeSubmenu(parent_menu, MakeSystemSubmenu(target_hwnd, buf, hsubmenu), buf);
    } else {
        sprintf(tm_buf_data, "@sbx.SysMenu %d %d", target_hwnd, id);  // build broam
        MakeMenuItem(parent_menu, buf, tm_buf_data, ((state & MFS_CHECKED) > 0));  // make menu-item
    }
}

//---------------------------------------------------------------------------
// Function: CleanMenuItem   ... cleans menu-items for system menus
//---------------------------------------------------------------------------

void CleanMenuItem(char *p, UINT len) {
    bool is_tab = false;
    char *p2 = p + len - 2;

    while (1) {
        if (p2[0] == 9) is_tab = true; // peel off stuff aligned by tabs, like "ctrl+n"
        else if (is_tab && (p2[0] > 32)) { p2[1] = 0; break; }
        if (p2 == p) break;
        --p2;
    }

    do if (p[0] == '&') strcpy(p, p + 1);  // remove underlined letters for 0.90
    while ((++p)[0]);
}

//---------------------------------------------------------------------------
// Function: PaintTasks
//---------------------------------------------------------------------------

inline void PaintTasks() {
    unsigned k = 0;
    pF_TV_RB f = ToggleInfo.test(TI_TASKS_IN_CURRENT_WORKSPACE) ? retW_TI: retT;
    bTaskTooltips = ToggleInfo.test(TI_TOOLTIPS_TASKS);

    TasksMouseSize.cx = TaskbarRect.left;
    TasksMouseSize.cy = TaskbarRect.right;

    if (!bTaskbarListsUpdated)
        UpdateTaskbarLists();

    if (!bIconizedTaskBackgroundsExist) {
        IconizedTaskWidth = TaskIconItem.m_size + ((ToggleInfo.test(TI_COMPRESS_ICONIZED) && bTaskPR) ? 
			(2 * OUTER_SPACING) : (4 * INNER_SPACING));

        PaintBackgrounds(bufIconizedActiveTask, bufIconizedInactiveTask, getSC(STYLE_MENU_TASKS), IconizedTaskWidth,
			(SystemBarExHeight - 2 * TheBorder));
        bIconizedTaskBackgroundsExist = true;
    }

	IconizedRight = TaskbarRect.left;

	if (pTaskTmp = TaskbarListIconized->next) {
		if (ToggleInfo.test(TI_REVERSE_TASKS)) {
            k += NumberNotIconized + NumberIconized;
			
            do if ( f() ) {
                pTaskTmp->v->ViewPosition = --k;
                pTaskTmp->v->Paint();
            } while (pTaskTmp = pTaskTmp->next);
        } else {
			do if ( f() ) {
				pTaskTmp->v->ViewPosition = k;
				pTaskTmp->v->Paint();
				++k;
			} while (pTaskTmp = pTaskTmp->next);
		}
	}

    if (!bTaskBackgroundsExist) {
        if (NumberNotIconized) {
            TaskRegionWidth = TaskbarRect.right - TaskbarRect.left;
            IconizedShift = NumberIconized ? (IconizedRight - TaskbarRect.left +
                ((ToggleInfo.test(TI_COMPRESS_ICONIZED) && bTaskPR) ? (2 * OUTER_SPACING): 0)): 0;

            TaskWidth = (TaskRegionWidth - IconizedShift - NumberNotIconized * OUTER_SPACING) / NumberNotIconized;
			if ( TaskMaxWidth && TaskWidth > TaskMaxWidth ) {
				TaskWidth = (unsigned)TaskMaxWidth;
			}
            ExtraTaskWidth = TaskWidth + 1;

            if (TaskWidth > 2) {
                PaintBackgrounds(bufActiveTask, bufInactiveTask, getSC(STYLE_MENU_TASKS), TaskWidth, (SystemBarExHeight - 2 * TheBorder));
                PaintBackgrounds(bufActiveTaskExtra, bufInactiveTaskExtra, getSC(STYLE_MENU_TASKS), ExtraTaskWidth, (SystemBarExHeight - 2 * TheBorder));
            }
        }
        bTaskBackgroundsExist = true;
    }

    if (pTaskTmp = TaskbarList->next) {
        if (ToggleInfo.test(TI_REVERSE_TASKS)) {
            k += NumberNotIconized;
            do if ( f() ) {
                pTaskTmp->v->ViewPosition = --k;
                pTaskTmp->v->Paint();
            } while (pTaskTmp = pTaskTmp->next);
        } else {
            do if ( f() ) {
                pTaskTmp->v->ViewPosition = k;
                pTaskTmp->v->Paint();
                ++k;
            } while (pTaskTmp = pTaskTmp->next);
        }
    }
    bIconBuffersUpdated = true;
}

//---------------------------------------------------------------------------
// Function: retT
// Purpose:  use with retW
//---------------------------------------------------------------------------

bool retT() {
	return true;
}

//---------------------------------------------------------------------------
// Function: retW
// Purpose:  use with retT
//---------------------------------------------------------------------------

bool retW() {
	return p_TaskList->wkspc == CurrentWorkspace;
}

//---------------------------------------------------------------------------
// Function:    retW_TI
//---------------------------------------------------------------------------

bool retW_TI() {
	return pTaskTmp->v->pTaskList->wkspc == CurrentWorkspace;
}

//---------------------------------------------------------------------------
// Function:    retW_TIn
//---------------------------------------------------------------------------

bool retW_TIn() {
	return pTaskTmp->next->v->pTaskList->wkspc == CurrentWorkspace;
}

//---------------------------------------------------------------------------
// Function: UpdateTaskbarLists
// Purpose:  populates the TaskbarLists with pointers to TaskbarItems and removes old pointers
//---------------------------------------------------------------------------

inline void UpdateTaskbarLists() {
    pF_TV_RB f, f2, f3;
    tasklist *t1 = GetTaskListPtr();
    if (ToggleInfo.test(TI_TASKS_IN_CURRENT_WORKSPACE))
        f = retW, f2 = retW_TI, f3 = retW_TIn;
    else
        f = f2 = f3 = retT;

    if (pTaskTmp = TaskbarList->next)
        do pTaskTmp->v->TaskExists = false;
        while (pTaskTmp = pTaskTmp->next);

    NumberIconized = 0;

    pTaskTmp = TaskbarListIconized;
    while (pTaskTmp->next) {
        if (p_TaskList = t1)
            do if (p_TaskList == pTaskTmp->next->v->pTaskList) {
                pTaskTmp = pTaskTmp->next;
                if (f2()) ++NumberIconized;
                goto utv_cont_1;
            } while (p_TaskList = p_TaskList->next);
        delete pTaskTmp->next->v;
        if (pTaskTmp = pTaskTmp->erase())
			do if (f2()) {
				++NumberIconized;
			} while (pTaskTmp = pTaskTmp->next);
        goto set_taskcount;
utv_cont_1:;
    }
    pTaskTmpI = pTaskTmp;

    TaskbarListNew = new LeanList<TaskbarItem*>(), pTaskTmpNew = TaskbarListNew;

    NumberNotIconized = 0;
    if (p_TaskList = t1) {
        do if (f()) {
            if (pTaskTmp = TaskbarList->next)
                do if (p_TaskList == pTaskTmp->v->pTaskList) {
                    pTaskTmp->v->TaskExists = true;
                    if (pTaskTmp->v->IsIconized) {
                        if (f2()) ++NumberIconized;
                        pTaskTmpI = pTaskTmpI->append(pTaskTmp->v);
                    } else
                        pTaskTmpNew = pTaskTmpNew->append(pTaskTmp->v), ++NumberNotIconized;
                    goto utv_cont_2;
                } while (pTaskTmp = pTaskTmp->next);

            pTaskTmp = TaskbarListIconized;
            while (pTaskTmp->next) {
                if (p_TaskList == pTaskTmp->next->v->pTaskList) {
                    if (!pTaskTmp->next->v->IsIconized) {
                        if (f3()) --NumberIconized;
                        pTaskTmpNew = pTaskTmpNew->append(pTaskTmp->next->v), ++NumberNotIconized;
                        if (!pTaskTmp->next->next)
                            pTaskTmpI = pTaskTmp;
                        pTaskTmp->erase();
                    }
                    goto utv_cont_2;
                }
                pTaskTmp = pTaskTmp->next;
            }

            pTaskTmpNew = pTaskTmpNew->append(new TaskbarItem(p_TaskList)), ++NumberNotIconized;
utv_cont_2:;
        }
        while (p_TaskList = p_TaskList->next);
    }

    if (pTaskTmp = TaskbarList->next)
        do if (!pTaskTmp->v->TaskExists) delete pTaskTmp->v;
        while (pTaskTmp = pTaskTmp->next);

    delete TaskbarList;
    TaskbarList = TaskbarListNew;
set_taskcount:
    TaskCount = NumberNotIconized + NumberIconized;
    bTaskbarListsUpdated = true;
}

//---------------------------------------------------------------------------
// Function: PaintBackgrounds
// Purpose:  paint background to buffer
//---------------------------------------------------------------------------

void PaintBackgrounds(HDC buf1, HDC buf2, BYTE StyleInactive, unsigned width, unsigned height) {
    RECT    r1 = {0, 0, width, height},
            r2 = {1, 1, width - 1, height - 1};
    BYTE    s = getSC(STYLE_MENU_PRESSED);
    bool    b = !ToolbarBevelRaised && (s == STYLE_DEFAULT);

    bmpInfo.biWidth = width;
    bmpInfo.biHeight = height;

	if (buf1) {
		DeleteObject((HBITMAP)SelectObject(buf1, CreateDIBSection(
			0, (BITMAPINFO*)&bmpInfo, DIB_RGB_COLORS, 0, 0, 0)));
		if (!StyleItemArray[s].parentRelative) {
			MakeStyleGradient(buf1, &(b ? r2: r1), &StyleItemArray[(s == STYLE_DEFAULT) ? ActiveStyleIndex: s], StyleItemArray[(s == STYLE_DEFAULT) ? ActiveStyleIndex : s].bordered);
		}
	}

	if (buf2) {
			b = !ToolbarBevelRaised;
			DeleteObject((HBITMAP)SelectObject(buf2, CreateDIBSection(
				0, (BITMAPINFO*)&bmpInfo, DIB_RGB_COLORS, 0, 0, 0)));
		if (StyleInactive == STYLE_DEFAULT) {
			MakeStyleGradient(buf2, &(b ? r2: r1), &StyleItemArray[STYLE_TOOLBAR], StyleItemArray[STYLE_TOOLBAR].bordered);
		} else {
			MakeStyleGradient(buf2, &r1, &StyleItemArray[StyleInactive], StyleItemArray[StyleInactive].bordered);
		}
    }
}

//---------------------------------------------------------------------------
// Function: PaintInscription
//---------------------------------------------------------------------------

inline void PaintInscription() {
    char *p;
    int margin,
        spacing,
        letter_size,
        left = max(TaskbarRect.left, InscriptionLeft),
        width = TaskbarRect.right - left,
        len = strlen(InscriptionText);

    if ((width < InscriptionTextSize.cx) || (len < 2)) return;

    letter_size = InscriptionTextSize.cx / len;
    margin = (width ^ 2) / 2 - width / 3 + letter_size;
    RECT rectnow = {left + margin, TaskbarRect.top, TaskbarRect.right - margin, TaskbarRect.bottom};
    spacing = letter_size + (rectnow.right - rectnow.left - InscriptionTextSize.cx) / (len - 1);

    if (spacing < (letter_size + 3)) return;

	p = InscriptionText;
	
	do  {
		
		//if (StyleItemArray[STYLE_TOOLBARLABEL].ShadowXY && !StyleItemArray[STYLE_TOOLBARLABEL].parentRelative) {
		/*if (StyleItemArray[STYLE_TOOLBARLABEL].validated & VALID_SHADOWCOLOR) {
			RECT Rs;
			int i;
			COLORREF cr0;
			i = StyleItemArray[STYLE_TOOLBARLABEL].ShadowY;
			Rs.top = rectnow.top + i;
			Rs.bottom = rectnow.bottom + i;
			i = StyleItemArray[STYLE_TOOLBARLABEL].ShadowX;
			Rs.left = rectnow.left + i;
			Rs.right = rectnow.right + i;
			cr0 = SetTextColor(bufDC, StyleItemArray[STYLE_TOOLBARLABEL].ShadowColor);
			DrawText(bufDC, p, 1, &Rs, DT_LEFT|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE|DT_NOCLIP);
		}

		if (StyleItemArray[STYLE_TOOLBARLABEL].validated & VALID_OUTLINECOLOR && !StyleItemArray[STYLE_TOOLBARLABEL].parentRelative) {	
	COLORREF cr0;
			RECT rcOutline;
			//_CopyRect(&rcOutline, r);
			rcOutline.bottom = rectnow.bottom;
			rcOutline.top = rectnow.top;
			rcOutline.left = rectnow.left+1;
			rcOutline.right = rectnow.right+1;
			//cr0 = SetTextColor(bufDC, StyleItemArray[STYLE_TOOLBARLABEL].OutlineColor);
			SetTextColor(bufDC, StyleItemArray[STYLE_TOOLBARLABEL].OutlineColor);
			//_CopyOffsetRect(&rcOutline, lpRect, 1, 0);
			DrawText(bufDC, p, 1, &rcOutline, DT_LEFT|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE|DT_NOCLIP);
			_OffsetRect(&rcOutline,   0,  1);
			DrawText(bufDC, p, 1, &rcOutline, DT_LEFT|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE|DT_NOCLIP);
			_OffsetRect(&rcOutline,  -1,  0);
			DrawText(bufDC, p, 1, &rcOutline, DT_LEFT|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE|DT_NOCLIP);
			_OffsetRect(&rcOutline,  -1,  0);
			DrawText(bufDC, p, 1, &rcOutline, DT_LEFT|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE|DT_NOCLIP);
			_OffsetRect(&rcOutline,   0, -1);
			DrawText(bufDC, p, 1, &rcOutline, DT_LEFT|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE|DT_NOCLIP);
			_OffsetRect(&rcOutline,   0, -1);
			DrawText(bufDC, p, 1, &rcOutline, DT_LEFT|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE|DT_NOCLIP);
			_OffsetRect(&rcOutline,   1,  0);
			DrawText(bufDC, p, 1, &rcOutline, DT_LEFT|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE|DT_NOCLIP);
			_OffsetRect(&rcOutline,   1,  0);
			DrawText(bufDC, p, 1, &rcOutline, DT_LEFT|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE|DT_NOCLIP);
		}

		SetTextColor(bufDC, StyleItemArray[STYLE_TOOLBARLABEL].TextColor);
		DrawText(bufDC, p, 1, &rectnow, DT_LEFT|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE|DT_NOCLIP);*/

		BBDrawTextSBX(bufDC, p, 1, &rectnow, DT_LEFT|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE|DT_NOCLIP, &StyleItemArray[STYLE_TOOLBARLABEL]);
		rectnow.left += spacing;
	} while ((++p)[0]);
}

//---------------------------------------------------------------------------
// Function: DropStyle  .........grischka
//---------------------------------------------------------------------------

void DropStyle(HDROP hdrop) {
    char filename[MAX_PATH];
    filename[0] = 0;
    DragQueryFile(hdrop, 0, filename, sizeof(filename));
    DragFinish(hdrop);
    SendMessage(hBlackboxWnd, BB_SETSTYLE, 1, (LPARAM)filename);
}

//---------------------------------------------------------------------------
// Function: SetBarHeightAndCoord
// Purpose:  sets SystemBarEx coordinates and height
//---------------------------------------------------------------------------

inline void SetBarHeightAndCoord() {
    if (bStart && bInSlit)
        return;

    unsigned r = 0, margin = 2 * TheBorder, detArray[] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
    SystemBarExHeight = 2;  // reset to something small

    if (HeightSizing == HEIGHT_AUTO) {
        if (EnableInfo.test(EI_SHOW_TASKS)) {
#define TXT_HT_SPC 2
#define ICO_HT_SPC 4

			if (taskIcons == ICONIZED_ONLY) {
				goto icon_ht;
			} else {
                detArray[r] = TextHeight + TXT_HT_SPC;
                if ((taskIcons == ICON_AND_TEXT) || (NumberIconized > 0))
icon_ht:            detArray[++r] = TaskIconItem.m_size + ICO_HT_SPC;
            }
        }

        if (EnableInfo.test(EI_SHOW_WORKSPACE) ||
            EnableInfo.test(EI_SHOW_BBBUTTON) ||
            EnableInfo.test(EI_SHOW_INSCRIPTION) ||
            EnableInfo.test(EI_SHOW_WINDOWLABEL))
            detArray[++r] = TextHeight + TXT_HT_SPC;

        if (EnableInfo.test(EI_SHOW_TRAY))
            detArray[++r] = TrayIconItem.m_size + ICO_HT_SPC;

        if (EnableInfo.test(EI_SHOW_CLOCK))
            detArray[++r] = TextHeight + TXT_HT_SPC;

#undef TXT_HT_SPC
#undef ICO_HT_SPC

        SystemBarExHeight = detArray[0];
        r = 1;
        do SystemBarExHeight = max(SystemBarExHeight, detArray[r]);
        while (++r < 5);
        SystemBarExHeight += margin;
    } else if (HeightSizing == HEIGHT_TOOLBAR) {
        SystemBarExHeight = (pTbInfo->height < (TextHeight + 4)) ? (TextHeight + 4):
                            (pTbInfo->height < 12) ? 12: pTbInfo->height;

        if (TrayIconItem.m_size > SystemBarExHeight)
            TrayIconItem.m_size = ((SystemBarExHeight - margin) < 4) ? 4: (SystemBarExHeight - margin - 2 * OUTER_SPACING);
        if (TaskIconItem.m_size > SystemBarExHeight)
            TaskIconItem.m_size = ((SystemBarExHeight - margin) < 4) ? 4: (SystemBarExHeight - margin);
    } else {
        SystemBarExHeight = (CustomHeightSizing < (TextHeight + 4)) ? (TextHeight + 4):
                            (CustomHeightSizing < 12) ? 12: CustomHeightSizing;

        if (TrayIconItem.m_size > SystemBarExHeight)
            TrayIconItem.m_size = ((SystemBarExHeight - 2 * styleBorderWidth) < 4) ? 4: (SystemBarExHeight - 2 * styleBorderWidth);
        if (TaskIconItem.m_size > SystemBarExHeight)
            TaskIconItem.m_size = ((SystemBarExHeight - 2 * styleBorderWidth) < 4) ? 4: (SystemBarExHeight - 2 * styleBorderWidth);
    }

    if (SystemBarExPlacement == PLACEMENT_LINK_TO_TOOLBAR) {
        if (!pTbInfo->hidden && (pTbInfo->width > 10)) { // if the toolbar's big enough to link to
            SystemBarExWidth = pTbInfo->width;
            SystemBarExX = pTbInfo->xpos;

            if (Placement_Old != PLACEMENT_LINK_TO_TOOLBAR) {
                NotifyToolbar();
                if (ToggleInfo.test(TI_AUTOHIDE))
                    ShowBar(true);
            }

            SystemBarExY = (pTbInfo->autoHide && pTbInfo->autohidden) ? ScreenHeight:
                (pTbInfo->placement < 3) ? (pTbInfo->height - styleBorderWidth):
                (pTbInfo->ypos - SystemBarExHeight + styleBorderWidth);

            goto set_plc_old;
        }

        SystemBarExPlacement =  (Placement_Old != PLACEMENT_LINK_TO_TOOLBAR) ? Placement_Old:
                                (pTbInfo->ypos < 3) ? PLACEMENT_TOP_CENTER: PLACEMENT_BOTTOM_CENTER;
        ShowPluginMenu(false);
    }

    SystemBarExWidth = (ScreenWidth * SystemBarExWidthPercent) / 100;

    if (SystemBarExPlacement < PLACEMENT_BOTTOM_LEFT) { // if placement is ON TOP, but not linked to toolbar
        SystemBarExY = 0;
        if (SystemBarExPlacement == PLACEMENT_TOP_LEFT)
            goto p_left;
        else if (SystemBarExPlacement == PLACEMENT_TOP_RIGHT)
            goto p_right;
        else //...if (SystemBarExPlacement == PLACEMENT_TOP_CENTER)
            goto p_center;
    } else if (SystemBarExPlacement < PLACEMENT_LINK_TO_TOOLBAR) { // if placement is ON BOTTOM, but not linked to toolbar
        SystemBarExY = ScreenHeight - SystemBarExHeight;
        if (SystemBarExPlacement == PLACEMENT_BOTTOM_LEFT)
p_left:     SystemBarExX = 0;
        else if (SystemBarExPlacement == PLACEMENT_BOTTOM_RIGHT)
p_right:    SystemBarExX = (ScreenWidth - SystemBarExWidth) / 2;
		//SystemBarExX = ScreenWidth - SystemBarExWidth;
        else //...if (SystemBarExPlacement == PLACEMENT_BOTTOM_CENTER)
p_center:   SystemBarExX = ScreenWidth - SystemBarExWidth;
		//SystemBarExX = (ScreenWidth - SystemBarExWidth) / 2;
    }

set_plc_old:
    TooltipActivationRect.bottom = SystemBarExHeight;
    Placement_Old = SystemBarExPlacement;

    HDC hdc = GetDC(0);
    bmpInfo.biBitCount = GetDeviceCaps(hdc, BITSPIXEL);
    ReleaseDC(0, hdc);

    bmpInfo.biWidth = SystemBarExWidth;
    bmpInfo.biHeight = SystemBarExHeight;
    DeleteObject((HBITMAP)SelectObject(bufDC, CreateDIBSection(
        0, (BITMAPINFO*)&bmpInfo, DIB_RGB_COLORS, 0, 0, 0)));
}

//---------------------------------------------------------------------------
// Function: Reconfig
//---------------------------------------------------------------------------

void Reconfig(bool getStyle) {
    GetStyleSettings(getStyle);

    if (getStyle) {
        SetScreenMargin(false);
        //UpdateTransparency(TI_WINDOW_TRANS);
		UpdateBarTransparency();
    }

    UINT flags = SWP_NOSENDCHANGING|SWP_NOACTIVATE|SWP_NOZORDER;
    if (bInSlit && bStart)
        flags |= SWP_NOMOVE|SWP_NOSIZE;
    else if ((SystemBarExPlacement == PLACEMENT_CUSTOM) && !getStyle)
        flags |= SWP_NOMOVE;

    SetWindowPos(hSystemBarExWnd, 0,
        SystemBarExX, SystemBarExY,
        SystemBarExWidth, SystemBarExHeight,
        flags);

    UpdateClientRect();
    UpdateClock(true);
    RefreshAll();
}

//---------------------------------------------------------------------------
// Function:    RefreshAll
//---------------------------------------------------------------------------

void RefreshAll() {
    BYTE i = 0;

	do {
		if ( i != BG_TASKS ) {
			BGItemArray[i]->bgPainted = false;
		}
	} while (++i < NUMBER_BACKGROUNDITEMS);

    ClockTextWidthOld = ClockTextSize.cx;
    pWindowLabelItem->bgPainted = false;

    bBackgroundExists =
        bTextArrayExists =
        bTaskBackgroundsExist =
        TrayIconItem.m_updated =
        TaskIconItem.m_updated =
        bIconBuffersUpdated =
        bIconizedTaskBackgroundsExist = false;

    InvalidateRect(hSystemBarExWnd, 0, false);
}

//---------------------------------------------------------------------------
// Function:    XMakeMenu
// Purpose:     make an updatable menu
//---------------------------------------------------------------------------

Menu* XMakeMenu(LPSTR header) {
    char buf[14];
    sprintf(buf, "sbx_%d", ++menu_id);
    return MakeNamedMenu(header, buf, popup);
}

//---------------------------------------------------------------------------
// Function:    SetFullNormal
// In:          bool isAll (true: applies to all non-minimized windows; false: applies only to the selected window)
//---------------------------------------------------------------------------

void fullNorm(HWND hwnd, RECT *r, unsigned w, unsigned h) {
    ShowWindow(hwnd, SW_SHOWNORMAL);
    SetWindowPos(hwnd, 0, (*r).left, (*r).top, w, h, SWP_NOZORDER);
}
//-----------------------------------
void SetFullNormal(bool isAll) {
    RECT r;
    unsigned width, height;

    SystemParametersInfo(SPI_GETWORKAREA, 0, (PVOID)&r, 0);
    width = r.right - r.left;
    height = r.bottom - r.top;
    if (isAll) {
        p_TaskList = GetTaskListPtr();
        pF_TV_RB f = ToggleInfo.test(TI_TASKS_IN_CURRENT_WORKSPACE) ? retW: retT;
        do if (f() && !IsIconic(p_TaskList->hwnd))
            fullNorm(p_TaskList->hwnd, &r, width, height);
        while (p_TaskList = p_TaskList->next);
	} else {
        fullNorm(buttonHwnd, &r, width, height);
	}
}

//---------------------------------------------------------------------------
// Function:    SetScreenMargin
// Purpose:     set desktop margin for SystemBarEx
//---------------------------------------------------------------------------

void SetScreenMargin(bool force) {
    UINT h = 0;
    BYTE edge = BB_DM_BOTTOM;

    if (!bInSlit && ((SystemBarExPlacement == PLACEMENT_LINK_TO_TOOLBAR) ? !pTbInfo->autoHide: !ToggleInfo.test(TI_AUTOHIDE))) {
        h = SystemBarExHeight;
        if (SystemBarExPlacement > PLACEMENT_BOTTOM_CENTER) { // ...if PLACEMENT_LINK_TO_TOOLBAR or PLACEMENT_CUSTOM
			if (SystemBarExY < (ScreenHeight / 2)) {
                edge = BB_DM_TOP, h += SystemBarExY;
			} else {
                h = ScreenHeight - SystemBarExY;
			}
        } else if (SystemBarExPlacement < PLACEMENT_BOTTOM_LEFT) { // if placement is __TOP__
            edge = BB_DM_TOP;
		}
    }
    SetDesktopMargin(hSystemBarExWnd, edge, h);
}

//---------------------------------------------------------------------------
// Function:    FocusActiveTask
//---------------------------------------------------------------------------

void FocusActiveTask() {
	if (!IsIconic(ActiveTaskHwnd)) {
        ActivateTask(ActiveTaskHwnd);
	}
}

//---------------------------------------------------------------------------
// Function:    ActivateTask
//---------------------------------------------------------------------------

inline void ActivateTask(HWND hwnd) {
    PostMessage(hBlackboxWnd, BB_BRINGTOFRONT, 0, (LPARAM)hwnd);
}

//---------------------------------------------------------------------------
// Function:    CheckButton   ...grischka
//---------------------------------------------------------------------------

bool CheckButton(UINT message, POINT MousePoint, RECT *pRect, bool *pState, void (*handler)(UINT), bool bCap, bool bInv) {
    if (PtInRect(pRect, MousePoint)) {
        switch (message) {
            case WM_LBUTTONDOWN:
            case WM_RBUTTONDOWN:
            case WM_MBUTTONDOWN:
            case WM_LBUTTONDBLCLK:
            case WM_MBUTTONDBLCLK:
            case WM_RBUTTONDBLCLK:
				if (bCap) {
                    SetCapture(hSystemBarExWnd);
				}

                capture_rect = pRect;
                pressed_state = pState;
                MouseButtonEvent = handler;
                if (pressed_state) {
                    *pressed_state = true;
					if (bInv) {
                        InvalidateRect(hSystemBarExWnd, capture_rect, false);
					}
                }
        }
        return true;
    }
    return false;
}

//---------------------------------------------------------------------------
// Function:    ReleaseButton   ...grischka
//---------------------------------------------------------------------------

void ReleaseButton(RECT *r) {
	if (capture_rect == r) {
		capture_rect = 0;
		ReleaseCapture();
	}
}

//---------------------------------------------------------------------------
// Function:    HandleCapture  ...grischka
//---------------------------------------------------------------------------

inline bool HandleCapture(UINT message, POINT MousePoint) {
    if (!capture_rect) return false;

    bool release = false;
	bool hilite = (PtInRect(capture_rect, MousePoint) != 0);
	bool inside = hilite;
    bool bTest = (GetCapture() == hSystemBarExWnd);

    if (message == WM_LBUTTONUP || message == WM_MBUTTONUP || message == WM_RBUTTONUP || GetCapture() != hSystemBarExWnd) {
        release = true;
        hilite  = false;
    }

    if (pressed_state && (hilite != *pressed_state)) {
        *pressed_state = hilite;
		if (bCaptureInv) {
            InvalidateRect(hSystemBarExWnd, capture_rect, false);
		} else {
            bCaptureInv = true;
		}
    }

	if (release) {
        ReleaseButton(capture_rect);
	}

	if (inside) {
        MouseButtonEvent(message);
	}

    return true;
}

//---------------------------------------------------------------------------
// Function:    BitBltRect  ...grischka
//---------------------------------------------------------------------------

void BitBltRect(HDC hdc_to, HDC hdc_from, RECT *r, int use_xy) {
	BitBlt(hdc_to, r->left, r->top, (r->right - r->left), (r->bottom - r->top), hdc_from, (use_xy * r->left), (use_xy * r->top), SRCCOPY);
}

//---------------------------------------------------------------------------
// Function:    ShowRootMenu
// Purpose:     shows the desktop menu
//---------------------------------------------------------------------------

void ShowRootMenu() {
	if (!TestAsyncKeyState(VK_SHIFT)) {
		PostMessage(hBlackboxWnd, BB_MENU, 0, 0);
	}
}

//---------------------------------------------------------------------------
// Function:    TestAsyncKeyState
//---------------------------------------------------------------------------

bool TestAsyncKeyState(unsigned kc) {
	return (GetAsyncKeyState(kc) & 0x8000) != 0;
}

//---------------------------------------------------------------------------
// Function:    CreatePath
//---------------------------------------------------------------------------

void CreatePath(char *p, char *ex/*, bool exclusions*/) {
	//if ( !exclusions ) {
		GetModuleFileName(hSystemBarExInstance, p, MAX_LINE_LENGTH);
		sprintf(&p[strlen(p) - 3], "%s", ex);
	/*} else {
		GetModuleFileName(hSystemBarExInstance, p, MAX_LINE_LENGTH);
		sprintf(&p[strlen(p) - 15], "%s", ex);
		p = exclusionpath2;
		GetModuleFileName(hSystemBarExInstance, p, MAX_LINE_LENGTH);
		ex = "";
		sprintf(&p[strlen(p) - 15], "%s", ex);
	}*/
}

//---------------------------------------------------------------------------
// Function:    NotifyToolbar
//---------------------------------------------------------------------------

void NotifyToolbar() {
	if (pTbInfo) {
		pTbInfo->bbsb_hwnd = hSystemBarExWnd;
		pTbInfo->bbsb_reverseTasks = ToggleInfo.test(TI_REVERSE_TASKS);
		pTbInfo->bbsb_linkedToToolbar = (SystemBarExPlacement == PLACEMENT_LINK_TO_TOOLBAR);
		pTbInfo->bbsb_currentOnly = ToggleInfo.test(TI_TASKS_IN_CURRENT_WORKSPACE);
	}
}

//---------------------------------------------------------------------------
// Function:    UpdatePlacementOrder
//---------------------------------------------------------------------------

void UpdatePlacementOrder() {
	BYTE i = 0, j, t;// k,
	pWindowLabelItem->overflow = TheBorder;

	//Parse up to BG_TASKS, then build placement/bar right to left.
	//This allows BG_TASKS to have as much space as can be spared.
	do {
		if ( PlacementStruct[i] == BG_TASKS ) {
			if ( i == (NUMBER_ELEMENTS - 1) ) {
				return;
			}
			break;
		} else if ( PlacementStruct[i] == (NUMBER_ELEMENTS - 1) ) {
			return;
		}

		BGItemArray[PlacementStruct[i]]->align = ELEM_ALIGN_LEFT;
		BGItemArray[PlacementStruct[i]]->overflow = (i == 0) ? TheBorder: 0;
		BGItemOrderArray[i] = BGItemArray[PlacementStruct[i]];
		++i;
	} while ( true );

	t = i;
	j = NUMBER_ELEMENTS - 1;
	do {
		BGItemArray[PlacementStruct[j]]->align = ELEM_ALIGN_RIGHT;
		BGItemArray[PlacementStruct[j]]->overflow = (j == NUMBER_ELEMENTS - 1) ? TheBorder: 0;
		BGItemOrderArray[t] = BGItemArray[PlacementStruct[j]];
		++t;
	} while (--j != i);
}

//---------------------------------------------------------------------------
// Function:    UpdateTransparency
//---------------------------------------------------------------------------

/*void UpdateTransparency(toggle_index which) {
    HWND hNow = (which == TI_WINDOW_TRANS ? hSystemBarExWnd : bbTip.m_TipHwnd);
    if (ToggleInfo.test(which))
        SetTransTrue(hNow, ((which == TI_WINDOW_TRANS) ? WindowAlpha : TipSettings.alpha));
    else
        SetWindowLongPtr(hNow, GWL_EXSTYLE, WS_EX_TOOLWINDOW|WS_EX_ACCEPTFILES);
};*/

void UpdateBarTransparency() {
	//|WS_EX_LAYERED
	SetWindowLongPtr(hSystemBarExWnd, GWL_EXSTYLE, WS_EX_TOOLWINDOW|WS_EX_ACCEPTFILES);
	if ( WindowAlpha == 0) {
		pSetLayeredWindowAttributes(hSystemBarExWnd, 0x00000000, WindowAlpha, LWA_COLORKEY);
	} else if ( WindowAlpha != 255) {
		pSetLayeredWindowAttributes(hSystemBarExWnd, 0x00000000, WindowAlpha, LWA_ALPHA);
	}
}

void UpdateToolTipsTransparency() {
	SetWindowLongPtr(bbTip.m_TipHwnd, GWL_EXSTYLE, WS_EX_TOOLWINDOW|WS_EX_ACCEPTFILES|WS_EX_LAYERED);
	pSetLayeredWindowAttributes(bbTip.m_TipHwnd, 0x00000000, TipSettings.alpha, LWA_ALPHA);
}

//-----------------------------------------------------------------------------



// *************************  class TaskEventItem  ****************************

TaskEventItem::TaskEventItem() {
	theMenu = 0;
	pArray = 0;
}
TaskEventItemDBL::TaskEventItemDBL() {
	theMenu = 0;
	pArray = 0;
}

//-----------------------------------------------------------------------------

TaskEventItem::~TaskEventItem() {
	delete[] pArray;
}
TaskEventItemDBL::~TaskEventItemDBL() {
	delete[] pArray;
}

//-----------------------------------------------------------------------------
void TaskEventItem::SetStuff(char *textNow, BYTE id, BYTE index) {
	ID = id;
	strcpy(theText, textNow);
	theIndex = index;
}

void TaskEventItemDBL::SetStuff(char *textNow, BYTE id, BYTE index) {
	ID = id;
	strcpy(theText, textNow);
	theIndex = index;
}
//-----------------------------------------------------------------------------
void TaskEventItem::CreateMenuArray(BYTE arrayType) {
	BYTE i = 0, k = TE_data_array[arrayType - 2].length + 1;
	theMenu = XMakeMenu(theText);
	pArray = new TaskEventItem[tmeLengthArray[arrayType - 2] + 1];
	TaskEventItem *p = pArray;
    do {
		p->SetStuff( (*(TE_data_array[arrayType - 2].here + i)).txt, (*(TE_data_array[arrayType - 2].here + i)).spot, i);
		if ( i < (k - 1) ) {
			++p;
		}
	} while (++i < k);
}

void TaskEventItemDBL::CreateMenuArray(BYTE arrayType) {
	BYTE i = 0, k = TE_DBL_data_array[arrayType - 2].length + 1;
	theMenu = XMakeMenu(theText);
	pArray = new TaskEventItemDBL[tmeDBLLengthArray[arrayType - 2] + 1];
	TaskEventItemDBL *p = pArray;
    do {
		p->SetStuff( (*(TE_DBL_data_array[arrayType - 2].here + i)).txt, (*(TE_DBL_data_array[arrayType - 2].here + i)).spot, i);
		if ( i < (k - 1) ) {
			++p;
		}
	} while (++i < k);
}

//-----------------------------------------------------------------------------



// **************************  class BackgroundItem  *****************************


BackgroundItem::BackgroundItem( BYTE style_index, enable_index enableOC_index, toggle_index tipOC_index,
    HDC hdcPressed, HDC hdcNotPressed, char *p_text, unsigned *p_text_width, char *p_tip_text, bool (BackgroundItem::*pF)(),
    void (*pME)(unsigned)) {

    styleIndex = style_index;
    ocIndex = enableOC_index;
    ocTip = tipOC_index;
    hdcP = hdcPressed;
    hdcNP = hdcNotPressed;
    pText = p_text;
    pTextWidth = p_text_width;
    pTip = p_tip_text;
    pPaint = pF;
    pMouseEvent = pME;
    bgPainted = bPressed = false;
}

//-----------------------------------------------------------------------------

void BackgroundItem::PaintBG( unsigned content_width ) {
    mouseRect.top = 0;
    mouseRect.bottom = SystemBarExHeight;
    placementRect = TaskbarRect;
    width = content_width + 4 * INNER_SPACING;

    if (align == ELEM_ALIGN_LEFT) {
        placementRect.right = placementRect.left + width;
        mouseRect.left = placementRect.left - overflow;
        mouseRect.right = placementRect.right;
    } else {
        placementRect.left = placementRect.right - width;
        mouseRect.right = placementRect.right + overflow;
        mouseRect.left = placementRect.left;
    }

    PaintBackgrounds(hdcP, hdcNP, getSC(styleIndex), width, (placementRect.bottom - placementRect.top));
    bgPainted = true;
}

//-----------------------------------------------------------------------------

bool BackgroundItem::GenericBG() {
    HDC hdc;
    BYTE style;
	BYTE orgStyle;
    COLORREF color;

    if (!bgPainted)
        PaintBG(*pTextWidth);

    if (bPressed) {
        style = getSC(STYLE_MENU_PRESSED);
        color = ( (style != STYLE_DEFAULT) && (style != STYLE_PARENTRELATIVE) ) ?
            StyleItemArray[style].TextColor: ActiveTaskTextColor;
        hdc = (
			(style == STYLE_PARENTRELATIVE) || ( (style != STYLE_DEFAULT) && StyleItemArray[style].parentRelative )
			) ? 0: hdcP;
    } else {
        style = getSC(styleIndex);
        color = ((style != STYLE_DEFAULT) && (style != STYLE_PARENTRELATIVE)) ?
            StyleItemArray[style].TextColor: InactiveTaskTextColor;
        hdc = ((style == STYLE_PARENTRELATIVE) ||
            ((style != STYLE_DEFAULT) && StyleItemArray[style].parentRelative)) ? 0: hdcNP;
    }

    BitBltRect(bufDC, hdc, &placementRect, 0);

	//----> HACK!
	orgStyle = style;
	if ( style == STYLE_DEFAULT ) {
		style = STYLE_TOOLBAR;
	}
	//____> HACK!

    if (pText) {
        RECT r = {
            placementRect.left + 2 * INNER_SPACING,
            placementRect.top,
            placementRect.right - 2 * INNER_SPACING,
            placementRect.bottom
        };
	
        //if (StyleItemArray[style].ShadowXY && !StyleItemArray[style].parentRelative) {
		/*if (StyleItemArray[style].validated & VALID_SHADOWCOLOR ) {
			RECT Rs;

			Rs.top = r.top + StyleItemArray[style].ShadowY;
			Rs.bottom = r.bottom + StyleItemArray[style].ShadowY;

			Rs.left = r.left + StyleItemArray[style].ShadowX;
			Rs.right = r.right + StyleItemArray[style].ShadowX;

			SetTextColor(bufDC, StyleItemArray[style].ShadowColor);
			DrawText(bufDC, pText, -1, &Rs, DT_CENTER|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);
			
		}

		if (StyleItemArray[style].validated & VALID_OUTLINECOLOR && !StyleItemArray[style].parentRelative) {
		//if ( StyleItemArray[style].OutlineColor ) {
			COLORREF cr0;
			RECT rcOutline;
			//_CopyRect(&rcOutline, r);
			rcOutline.bottom = r.bottom;
			rcOutline.top = r.top;
			rcOutline.left = r.left+1;
			rcOutline.right = r.right+1;
			//cr0 = SetTextColor(bufDC, StyleItemArray[STYLE_TOOLBARLABEL].OutlineColor);
			SetTextColor(bufDC, StyleItemArray[style].OutlineColor);
			//_CopyOffsetRect(&rcOutline, lpRect, 1, 0);
			DrawText(bufDC, pText, -1, &rcOutline, DT_CENTER|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);
			_OffsetRect(&rcOutline,   0,  1);
			DrawText(bufDC, pText, -1, &rcOutline, DT_CENTER|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);
			_OffsetRect(&rcOutline,  -1,  0);
			DrawText(bufDC, pText, -1, &rcOutline, DT_CENTER|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);
			_OffsetRect(&rcOutline,  -1,  0);
			DrawText(bufDC, pText, -1, &rcOutline, DT_CENTER|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);
			_OffsetRect(&rcOutline,   0, -1);
			DrawText(bufDC, pText, -1, &rcOutline, DT_CENTER|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);
			_OffsetRect(&rcOutline,   0, -1);
			DrawText(bufDC, pText, -1, &rcOutline, DT_CENTER|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);
			_OffsetRect(&rcOutline,   1,  0);
			DrawText(bufDC, pText, -1, &rcOutline, DT_CENTER|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);
			_OffsetRect(&rcOutline,   1,  0);
			DrawText(bufDC, pText, -1, &rcOutline, DT_CENTER|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);
		}

		SetTextColor(bufDC, color);
		DrawText(bufDC, pText, -1, &r, Global_Just|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);*/

		BBDrawTextSBX(bufDC, pText, -1, &r, Global_Just|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS, &StyleItemArray[style]);
    }
	style = orgStyle;//HACK

    if (pTip) {
        RECT r = mouseRect;
        if (align == ELEM_ALIGN_LEFT)
            r.left += overflow;
        else
            r.right -= overflow;

        bbTip.Set(&r, pTip, ToggleInfo.test(ocTip));
    }

    return true;
}

//-----------------------------------------------------------------------------

bool BackgroundItem::PaintWindowLabel() {
	//BYTE style = getSC(STYLE_MENU_WINDOWLABEL);
    if (WindowLabelWidth < 2)
        return false;

    pText = 0;
    if (*WindowLabelText)
        pText = WindowLabelText;
    else if (p_TaskList = GetTaskListPtr())
        do if (p_TaskList->hwnd == ActiveTaskHwnd) {
            pText = p_TaskList->caption;
            break;
        } while (p_TaskList = p_TaskList->next);

    GenericBG();

    if (mouseRect.right == SystemBarExWidth)
        TasksMouseSize.cy = SystemBarExWidth;

    return true;
}

//-----------------------------------------------------------------------------

bool BackgroundItem::PaintTray() {
    RECT iconRect;
    systemTray *icon = 0;
    unsigned i = 0, k = 0, right_now, TrayCount = GetTraySize();
    bool tooltips_tray = ToggleInfo.test(ocTip);
    BYTE style = getSC(styleIndex);

    iconRect.top = TaskbarRect.top + (TaskbarRect.bottom - TaskbarRect.top - TrayIconItem.m_size) / 2;
    iconRect.bottom = iconRect.top + TrayIconItem.m_size;
    SystrayIconsPainted = 0;

    //--  update the SystrayList  ---------------------------------------------------------
	if (!bTrayListsUpdated) {
		TrayIconCount = 0;
		pSysTmp = SystrayList->next;
		SystrayListNew = new LeanList<SystrayItem*>(), pSysTmpNew = SystrayListNew;
		while (pSysTmp) {
			for (i = 0; i < TrayCount; ++i)
				if (GetTrayIcon(i) == pSysTmp->v->pSystrayStructure) {
					pSysTmp->v->pseudopointer = i;
					pSysTmpNew = pSysTmpNew->append(pSysTmp->v);
					++TrayIconCount;
					goto stv_end_1;
                }
            delete pSysTmp->v;
stv_end_1:  pSysTmp = pSysTmp->next;
        }
        delete SystrayList;
        SystrayList = SystrayListNew;
        TrayIconCount = TrayCount - TrayIconCount;
        bTrayListsUpdated = true;
    }
    //------------------------------------------------------------------------------------

    if (!TrayIconCount) return false;

    if (!bgPainted)
        PaintBG(TrayIconCount * (TrayIconItem.m_size + INNER_SPACING) - INNER_SPACING);

    right_now = placementRect.right - INNER_SPACING;

    if ((style != STYLE_PARENTRELATIVE) && ((style == STYLE_DEFAULT) || !StyleItemArray[style].parentRelative))
        BitBltRect(bufDC, hdcNP, &placementRect, 0);

    for (i = 0; i < TrayCount; ++i) {
        if (SystrayIconsPainted > TrayIconCount)
            break;  // quit if all of the visible icons have been painted

        if (pSysTmp = SystrayList->next)
            do if (i == pSysTmp->v->pseudopointer)
            {
                ++k;
                goto stv_end_2;
            }
            while (pSysTmp = pSysTmp->next);

        icon = GetTrayIcon(i);
        iconRect.right = right_now;
        TooltipActivationRect.right = right_now - INNER_SPACING;
        TooltipActivationRect.left = iconRect.right - TrayIconItem.m_size - INNER_SPACING;

        right_now =
            iconRect.left = iconRect.right - (TrayIconItem.m_size + INNER_SPACING);

        TrayIconItem.PaintIcon(bufDC, iconRect.left, iconRect.top, icon->hIcon, false);

        ++SystrayIconsPainted;
        bbTip.Set(&TooltipActivationRect, icon->szTip, tooltips_tray);
stv_end_2:;
    }
    return true;
}

bool BackgroundItem::PaintMoveLeft() {
	HDC hdc;
	BYTE style;
	COLORREF color;
	
	if (!bgPainted)
        PaintBG(WORKSPACE_MOVE_BTN_WIDTH);

	if (bPressed) {
        style = getSC(STYLE_MENU_PRESSED);
        color = ( (style != STYLE_DEFAULT) && (style != STYLE_PARENTRELATIVE) ) ?
            StyleItemArray[style].TextColor: ActiveTaskTextColor;
        hdc = (
			(style == STYLE_PARENTRELATIVE) || ( (style != STYLE_DEFAULT) && StyleItemArray[style].parentRelative )
			) ? 0: hdcP;
    } else {
        style = getSC(styleIndex);
        color = ((style != STYLE_DEFAULT) && (style != STYLE_PARENTRELATIVE)) ?
            StyleItemArray[style].TextColor: InactiveTaskTextColor;
        hdc = ((style == STYLE_PARENTRELATIVE) ||
            ((style != STYLE_DEFAULT) && StyleItemArray[style].parentRelative)) ? 0: hdcNP;
    }

	if ((style != STYLE_PARENTRELATIVE) && ((style == STYLE_DEFAULT) || !StyleItemArray[style].parentRelative))
        BitBltRect(bufDC, hdc, &placementRect, 0);

	{
		HPEN Pen = CreatePen(PS_SOLID, 1, StyleItemArray[style].TextColor);
		HGDIOBJ other = SelectObject(bufDC, Pen);
		int w = (int)(WORKSPACE_MOVE_BTN_WIDTH * .45);
		int x = (int)(placementRect.left + (WORKSPACE_MOVE_BTN_WIDTH * .9));
		int y = (int)(placementRect.top + (SystemBarExHeight * .4));
		int start, end, v = (w/2), z = y - v, z_end = y + v;;

		start = x + w, end = x - w + 1;

			MoveToEx ( bufDC, start, z, 0 );
			LineTo ( bufDC, start, z_end + 1 );
			for(; z <= z_end; z++)
			{
				MoveToEx ( bufDC, end, y, 0 );
				LineTo ( bufDC, start, z );
			}
	}

	return true;
}
bool BackgroundItem::PaintMoveRight() {
	HDC hdc;
	BYTE style;
	COLORREF color;

	if (!bgPainted)
        PaintBG(WORKSPACE_MOVE_BTN_WIDTH);

	if (bPressed) {
        style = getSC(STYLE_MENU_PRESSED);
        color = ( (style != STYLE_DEFAULT) && (style != STYLE_PARENTRELATIVE) ) ?
            StyleItemArray[style].TextColor: ActiveTaskTextColor;
        hdc = (
			(style == STYLE_PARENTRELATIVE) || ( (style != STYLE_DEFAULT) && StyleItemArray[style].parentRelative )
			) ? 0: hdcP;
    } else {
        style = getSC(styleIndex);
        color = ((style != STYLE_DEFAULT) && (style != STYLE_PARENTRELATIVE)) ?
            StyleItemArray[style].TextColor: InactiveTaskTextColor;
        hdc = ((style == STYLE_PARENTRELATIVE) ||
            ((style != STYLE_DEFAULT) && StyleItemArray[style].parentRelative)) ? 0: hdcNP;
    }

	if ((style != STYLE_PARENTRELATIVE) && ((style == STYLE_DEFAULT) || !StyleItemArray[style].parentRelative))
        BitBltRect(bufDC, hdc, &placementRect, 0);

	{
		HPEN Pen = CreatePen(PS_SOLID, 1, StyleItemArray[style].TextColor);
		HGDIOBJ other = SelectObject(bufDC, Pen);//needed, even if unused.
		int w = (int)(WORKSPACE_MOVE_BTN_WIDTH * .45);
		int x = (int)(placementRect.left + (WORKSPACE_MOVE_BTN_WIDTH * .9));
		int y = (int)(placementRect.top + (SystemBarExHeight * .4));
		int start, end, v = (w/2), z = y - v, z_end = y + v;;

		start = x - w + 1, end = x + w;

			MoveToEx ( bufDC, start, z, 0 );
			LineTo ( bufDC, start, z_end + 1 );
			for(; z <= z_end; z++)
			{
				MoveToEx ( bufDC, end, y, 0 );
				LineTo ( bufDC, start, z );
			}
	}

	return true;
}

//-----------------------------------------------------------------------------


// **************************  class TaskbarItem  *****************************

TaskbarItem::TaskbarItem(tasklist *t) {
    pTaskList = t;

    hIcon_local_s = 0;
    hIcon_local_b = 0;

    IsIconized = false;
    TaskExists = true;
    bButtonPressed = false;
    bFlashed = false;
    bBufferA = bBufferI = false;

    bufIcon = CreateCompatibleDC(0);  // create icon-buffer
    bmpIcon = (HBITMAP)SelectObject(bufIcon, CreateCompatibleBitmap(bufDC, 2, 2));  // grab 0 bitmap objects from DC
    hIcon_old = 0;
    task_width_old = -1;
}

//-----------------------------------------------------------------------------

TaskbarItem::~TaskbarItem() {
    if (hIcon_local_b) DestroyIcon(hIcon_local_b);
    if (hIcon_local_s) DestroyIcon(hIcon_local_s);
    hIcon_local_b = hIcon_local_s = 0;
    ReleaseButton(&TaskRect);
    DeleteObject((HBITMAP)SelectObject(bufIcon, bmpIcon));  // delete last bitmap, and select 0 bmp back into DC
    DeleteDC(bufIcon);  // delete icon-buffer
}

//-----------------------------------------------------------------------------

bool TaskbarItem::IsActive(HDC bufActive, HDC bufInactive) {
    if ((ToggleInfo.test(TI_FLASH_TASKS) && (isLean ? pTaskList->flashing: bFlashed)) ||
        (pTaskList->hwnd == ActiveTaskHwnd) || bButtonPressed) {

        BYTE s = getSC(STYLE_MENU_PRESSED);
        BufBacking = ( 
		(s == STYLE_PARENTRELATIVE) || ( (s != STYLE_DEFAULT) && StyleItemArray[s].parentRelative )
			) ? 0: bufActive;
		//
        TaskTextColor = ( (s == STYLE_DEFAULT) || (s == STYLE_PARENTRELATIVE) ) ? ActiveTaskTextColor: StyleItemArray[s].TextColor;
    } else {
        BYTE s = getSC(STYLE_MENU_TASKS);
        BufBacking = ( 
			(s == STYLE_PARENTRELATIVE) || ( (s != STYLE_DEFAULT) && StyleItemArray[s].parentRelative ) 
			) ? 0: bufInactive;
		//
        TaskTextColor = ( 
			(s == STYLE_DEFAULT) || (s == STYLE_PARENTRELATIVE) 
			) ? InactiveTaskTextColor: StyleItemArray[s].TextColor;
        return false;
    }
    return true;
}

//-----------------------------------------------------------------------------

inline void TaskbarItem::Paint() {
    HICON hIcon;
    RECT rectnow;
	BYTE ShdwX = 0, ShdwY = 0;
	COLORREF ShdwClr;
	//BYTE style = getSC(STYLE_MENU_WINDOWLABEL);

    TaskRect.top = TaskbarRect.top;
    TaskRect.bottom = TaskbarRect.bottom;

	if ((taskIcons == ICONIZED_ONLY) || IsIconized) {//.......paint iconized
		bPressed = IsActive(bufIconizedActiveTask, bufIconizedInactiveTask);

		if ( ToggleInfo.test(TI_REVERSE_TASKS) ) {
			InscriptionLeft = TaskbarRect.left + (NumberNotIconized + NumberIconized) * (IconizedTaskWidth + ((ToggleInfo.test(TI_COMPRESS_ICONIZED) || bTaskPR)?0:1)) - 1;
			IconizedRight = TaskbarRect.left + (ViewPosition + 1) * (IconizedTaskWidth + ((ToggleInfo.test(TI_COMPRESS_ICONIZED) || bTaskPR)?0:1)) - 1;
			TaskRect.left = IconizedRight - 16;

			if (ToggleInfo.test(TI_COMPRESS_ICONIZED) || bTaskPR) {
				TaskRect.right = TaskRect.left + IconizedTaskWidth;
				IconizedRight = TaskRect.right;
			} else {
				TaskRect.right = TaskRect.left + IconizedTaskWidth;
				IconizedRight = TaskRect.right + OUTER_SPACING;
			}
		} else {
			InscriptionLeft = TaskbarRect.left + (NumberNotIconized + NumberIconized) * (IconizedTaskWidth + ((ToggleInfo.test(TI_COMPRESS_ICONIZED) || bTaskPR)?0:1)) - 1;
			TaskRect.left = IconizedRight;
			
			if (ToggleInfo.test(TI_COMPRESS_ICONIZED) || bTaskPR) {
				TaskRect.right = TaskRect.left + IconizedTaskWidth;
				IconizedRight = TaskRect.right;
			} else {
				TaskRect.right = TaskRect.left + IconizedTaskWidth;
				IconizedRight = TaskRect.right + OUTER_SPACING;
			}
		}


		if (TaskRect.right > TaskbarRect.right) {
            return;
		}

		if (BufBacking) {
            BitBltRect(bufDC, BufBacking, &TaskRect, 0);
		}

        if (!(hIcon = GetIconHandle(TaskIconItem.m_size))) {
			//FIXME?: Shadows and Outlines?
            SetTextColor(bufDC, TaskTextColor);
            DrawText(bufDC, "!", 1, &TaskRect, DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_NOPREFIX|DT_NOCLIP);
            goto set_tt;
        }

        rectnow.left = TaskRect.left + (TaskRect.right - TaskRect.left - TaskIconItem.m_size) / 2;
    } else {
		BYTE style;
        //.....don't try to shrink calc of TaskRect here
        TaskRect.left = TaskbarRect.left + IconizedShift + (ViewPosition - NumberIconized) * (TaskRegionWidth - IconizedShift) / NumberNotIconized;
        TaskRect.right = TaskbarRect.left + IconizedShift + (ViewPosition - NumberIconized + 1) * (TaskRegionWidth - IconizedShift) / NumberNotIconized - OUTER_SPACING;
		InscriptionLeft = TaskbarRect.right;
		
		if ( TaskMaxWidth && TaskMaxWidth < (TaskRect.right - TaskRect.left) ) {
			TaskRect.left = TaskbarRect.left + IconizedShift + (ViewPosition - NumberIconized) * (TaskMaxWidth + OUTER_SPACING);
			TaskRect.right = TaskbarRect.left + IconizedShift + ( (ViewPosition + 1) - NumberIconized) * (TaskMaxWidth + OUTER_SPACING) - OUTER_SPACING;
			InscriptionLeft = TaskbarRect.left + IconizedShift + ( NumberNotIconized - NumberIconized) * (TaskMaxWidth + OUTER_SPACING) - OUTER_SPACING;
		}
		
	
        bPressed = ((TaskRect.right - TaskRect.left) == TaskWidth) ?
            IsActive(bufActiveTask, bufInactiveTask): IsActive(bufActiveTaskExtra, bufInactiveTaskExtra);

		if ( bPressed ) {
			style = getSC(STYLE_MENU_PRESSED);
			if (ToggleInfo.test(TI_PLUGINS_SHADOWS)) {
				ShdwX = BPShadowX;
				ShdwY = BPShadowY;
				ShdwClr = ShadowColors[1];
			} else {
				ShdwX = StyleItemArray[style].ShadowX;
				ShdwY = StyleItemArray[style].ShadowY;
				ShdwClr = StyleItemArray[style].ShadowColor;
			}
		} else {
			style = getSC(STYLE_MENU_TASKS);
			if (ToggleInfo.test(TI_PLUGINS_SHADOWS)) {
				ShdwX = BShadowX;
				ShdwY = BShadowY;
				ShdwClr = ShadowColors[0];
			} else {
				ShdwX = StyleItemArray[style].ShadowX;
				ShdwY = StyleItemArray[style].ShadowY;
				ShdwClr = StyleItemArray[style].ShadowColor;
			}
		}

		if (BufBacking) {
            BitBltRect(bufDC, BufBacking, &TaskRect, 0);
		}

        SetTextColor(bufDC, TaskTextColor);
        GetTextExtentPoint32(bufDC, pTaskList->caption, strlen(pTaskList->caption), &TextSize);

        rectnow = TaskRect;
        rectnow.right -= 2 * INNER_SPACING;

        if ((taskIcons == ICON_AND_TEXT) &&                   //......paint icon/text
            (hIcon = GetIconHandle(TaskIconItem.m_size)) &&  // if there's an icon
            ((TaskRect.right - TaskRect.left) > (TaskIconItem.m_size + 4 * INNER_SPACING))) { // if there's an icon to paint and if there's room for an icon
        
			switch ((TextSize.cx >= (rectnow.right - rectnow.left - TaskIconItem.m_size - 4 * INNER_SPACING)) ? DT_LEFT: TB_Just) {
				case DT_CENTER:
					rectnow.left += INNER_SPACING + TaskIconItem.m_size;
					
					//if (StyleItemArray[style].validated & VALID_OUTLINECOLOR && style != STYLE_PARENTRELATIVE) {
					/*if ( StyleItemArray[style].OutlineColor ) {
						COLORREF cr0;
						RECT rcOutline;
						rcOutline.bottom = rectnow.bottom;
						rcOutline.top = rectnow.top;
						rcOutline.left = rectnow.left+1;
						rcOutline.right = rectnow.right+1;
						SetTextColor(bufDC, StyleItemArray[STYLE_TOOLBARLABEL].OutlineColor);
						DrawText(bufDC, pTaskList->caption, -1, &rcOutline, DT_CENTER|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);
						_OffsetRect(&rcOutline,   0,  1);
						DrawText(bufDC, pTaskList->caption, -1, &rcOutline, DT_CENTER|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);
						_OffsetRect(&rcOutline,  -1,  0);
						DrawText(bufDC, pTaskList->caption, -1, &rcOutline, DT_CENTER|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);
						_OffsetRect(&rcOutline,  -1,  0);
						DrawText(bufDC, pTaskList->caption, -1, &rcOutline, DT_CENTER|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);
						_OffsetRect(&rcOutline,   0, -1);
						DrawText(bufDC, pTaskList->caption, -1, &rcOutline, DT_CENTER|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);
						_OffsetRect(&rcOutline,   0, -1);
						DrawText(bufDC, pTaskList->caption, -1, &rcOutline, DT_CENTER|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);
						_OffsetRect(&rcOutline,   1,  0);
						DrawText(bufDC, pTaskList->caption, -1, &rcOutline, DT_CENTER|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);
						_OffsetRect(&rcOutline,   1,  0);
						DrawText(bufDC, pTaskList->caption, -1, &rcOutline, DT_CENTER|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);
					}

					if (StyleItemArray[style].ShadowXY && style != STYLE_PARENTRELATIVE) {
						RECT Rs;
						
						Rs.top = rectnow.top + ShdwY;
						Rs.bottom = rectnow.bottom + ShdwY;

						Rs.left = rectnow.left + ShdwX;
						Rs.right = rectnow.right + ShdwX;

						SetTextColor(bufDC, ShdwClr);
						DrawText(bufDC, pTaskList->caption, -1, &Rs, DT_CENTER|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);
					}

					SetTextColor(bufDC, TaskTextColor);
                    DrawText(bufDC, pTaskList->caption, -1, &rectnow, DT_CENTER|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);*/
					BBDrawTextSBX(bufDC, pTaskList->caption, -1, &rectnow, DT_CENTER|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS, &StyleItemArray[style]);
                    rectnow.left = TaskRect.left + ((TaskRect.right - TaskRect.left - TextSize.cx - TaskIconItem.m_size - 3 * INNER_SPACING) / 2);
                    break;

                case DT_LEFT:
                    rectnow.left += (3 * INNER_SPACING + TaskIconItem.m_size);
					
					//if (StyleItemArray[style].validated & VALID_OUTLINECOLOR && style != STYLE_PARENTRELATIVE) {
					/*if ( StyleItemArray[style].OutlineColor ) {
						COLORREF cr0;
						RECT rcOutline;
						//_CopyRect(&rcOutline, r);
						rcOutline.bottom = rectnow.bottom;
						rcOutline.top = rectnow.top;
						rcOutline.left = rectnow.left+1;
						rcOutline.right = rectnow.right+1;
						//cr0 = SetTextColor(bufDC, StyleItemArray[STYLE_TOOLBARLABEL].OutlineColor);
						SetTextColor(bufDC, StyleItemArray[STYLE_TOOLBARLABEL].OutlineColor);
						//_CopyOffsetRect(&rcOutline, lpRect, 1, 0);
						DrawText(bufDC, pTaskList->caption, -1, &rcOutline, DT_CENTER|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);
						_OffsetRect(&rcOutline,   0,  1);
						DrawText(bufDC, pTaskList->caption, -1, &rcOutline, DT_CENTER|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);
						_OffsetRect(&rcOutline,  -1,  0);
						DrawText(bufDC, pTaskList->caption, -1, &rcOutline, DT_CENTER|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);
						_OffsetRect(&rcOutline,  -1,  0);
						DrawText(bufDC, pTaskList->caption, -1, &rcOutline, DT_CENTER|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);
						_OffsetRect(&rcOutline,   0, -1);
						DrawText(bufDC, pTaskList->caption, -1, &rcOutline, DT_CENTER|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);
						_OffsetRect(&rcOutline,   0, -1);
						DrawText(bufDC, pTaskList->caption, -1, &rcOutline, DT_CENTER|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);
						_OffsetRect(&rcOutline,   1,  0);
						DrawText(bufDC, pTaskList->caption, -1, &rcOutline, DT_CENTER|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);
						_OffsetRect(&rcOutline,   1,  0);
						DrawText(bufDC, pTaskList->caption, -1, &rcOutline, DT_CENTER|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);
					}

					if (StyleItemArray[style].ShadowXY && style != STYLE_PARENTRELATIVE) {
						RECT Rs; 
						
						Rs.top = rectnow.top + ShdwY;
						Rs.bottom = rectnow.bottom + ShdwY;

						Rs.left = rectnow.left + ShdwX;
						Rs.right = rectnow.right + ShdwX;
						
						SetTextColor(bufDC, ShdwClr);
						DrawText(bufDC, pTaskList->caption, -1, &Rs, DT_LEFT|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);
					}

					SetTextColor(bufDC, TaskTextColor);
                    DrawText(bufDC, pTaskList->caption, -1, &rectnow, DT_LEFT|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);*/
					BBDrawTextSBX(bufDC, pTaskList->caption, -1, &rectnow, DT_LEFT|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS, &StyleItemArray[style]);
                    rectnow.left = TaskRect.left + 2 * INNER_SPACING;
                    break;

                case DT_RIGHT:
                    {
                        SIZE sz;
						//if (StyleItemArray[style].validated & VALID_OUTLINECOLOR && style != STYLE_PARENTRELATIVE) {
						/*if ( StyleItemArray[style].OutlineColor ) {
							COLORREF cr0;
							RECT rcOutline;
							//_CopyRect(&rcOutline, r);
							rcOutline.bottom = rectnow.bottom;
							rcOutline.top = rectnow.top;
							rcOutline.left = rectnow.left+1;
							rcOutline.right = rectnow.right+1;
							//cr0 = SetTextColor(bufDC, StyleItemArray[STYLE_TOOLBARLABEL].OutlineColor);
							SetTextColor(bufDC, StyleItemArray[STYLE_TOOLBARLABEL].OutlineColor);
							//_CopyOffsetRect(&rcOutline, lpRect, 1, 0);
							DrawText(bufDC, pTaskList->caption, -1, &rcOutline, DT_CENTER|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);
							_OffsetRect(&rcOutline,   0,  1);
							DrawText(bufDC, pTaskList->caption, -1, &rcOutline, DT_CENTER|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);
							_OffsetRect(&rcOutline,  -1,  0);
							DrawText(bufDC, pTaskList->caption, -1, &rcOutline, DT_CENTER|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);
							_OffsetRect(&rcOutline,  -1,  0);
							DrawText(bufDC, pTaskList->caption, -1, &rcOutline, DT_CENTER|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);
							_OffsetRect(&rcOutline,   0, -1);
							DrawText(bufDC, pTaskList->caption, -1, &rcOutline, DT_CENTER|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);
							_OffsetRect(&rcOutline,   0, -1);
							DrawText(bufDC, pTaskList->caption, -1, &rcOutline, DT_CENTER|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);
							_OffsetRect(&rcOutline,   1,  0);
							DrawText(bufDC, pTaskList->caption, -1, &rcOutline, DT_CENTER|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);
							_OffsetRect(&rcOutline,   1,  0);
							DrawText(bufDC, pTaskList->caption, -1, &rcOutline, DT_CENTER|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);
						}

						if (StyleItemArray[style].ShadowXY && style != STYLE_PARENTRELATIVE) {
							RECT Rs;
							
							Rs.top = rectnow.top + ShdwY;
							Rs.bottom = rectnow.bottom + ShdwY;

							Rs.left = rectnow.left + ShdwX;
							Rs.right = rectnow.right + ShdwX;

							SetTextColor(bufDC, ShdwClr);
							DrawText(bufDC, pTaskList->caption, -1, &Rs, DT_RIGHT|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);
						}

						SetTextColor(bufDC, TaskTextColor);
                        DrawText(bufDC, pTaskList->caption, -1, &rectnow, DT_RIGHT|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);*/
						BBDrawTextSBX(bufDC, pTaskList->caption, -1, &rectnow, DT_RIGHT|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS, &StyleItemArray[style]);
                        GetTextExtentPoint32(bufDC, pTaskList->caption, strlen(pTaskList->caption), &sz);
                        rectnow.left = rectnow.right - sz.cx - TaskIconItem.m_size - 2 * INNER_SPACING;
                    }
            }
        } else {
			//BYTE s = getSC(STYLE_MENU_TASKS);
            //......paint text-only
            rectnow.left += 2 * INNER_SPACING;
			//if (StyleItemArray[style].validated & VALID_OUTLINECOLOR && style != STYLE_PARENTRELATIVE) {
			/*if ( StyleItemArray[style].OutlineColor ) {
				COLORREF cr0;
				RECT rcOutline;
				rcOutline.bottom = rectnow.bottom;
						rcOutline.top = rectnow.top;
						rcOutline.left = rectnow.left+1;
						rcOutline.right = rectnow.right+1;
						//cr0 = SetTextColor(bufDC, StyleItemArray[STYLE_TOOLBARLABEL].OutlineColor);
						SetTextColor(bufDC, StyleItemArray[STYLE_TOOLBARLABEL].OutlineColor);
						//_CopyOffsetRect(&rcOutline, lpRect, 1, 0);
						DrawText(bufDC, pTaskList->caption, -1, &rcOutline, DT_CENTER|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);
						_OffsetRect(&rcOutline,   0,  1);
						DrawText(bufDC, pTaskList->caption, -1, &rcOutline, DT_CENTER|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);
						_OffsetRect(&rcOutline,  -1,  0);
						DrawText(bufDC, pTaskList->caption, -1, &rcOutline, DT_CENTER|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);
						_OffsetRect(&rcOutline,  -1,  0);
						DrawText(bufDC, pTaskList->caption, -1, &rcOutline, DT_CENTER|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);
						_OffsetRect(&rcOutline,   0, -1);
						DrawText(bufDC, pTaskList->caption, -1, &rcOutline, DT_CENTER|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);
						_OffsetRect(&rcOutline,   0, -1);
						DrawText(bufDC, pTaskList->caption, -1, &rcOutline, DT_CENTER|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);
						_OffsetRect(&rcOutline,   1,  0);
						DrawText(bufDC, pTaskList->caption, -1, &rcOutline, DT_CENTER|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);
						_OffsetRect(&rcOutline,   1,  0);
						DrawText(bufDC, pTaskList->caption, -1, &rcOutline, DT_CENTER|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);
			}
			//Shadow
				if (StyleItemArray[style].ShadowXY && style != STYLE_PARENTRELATIVE) {
					RECT Rs;
					
					Rs.top = rectnow.top + ShdwY;
					Rs.bottom = rectnow.bottom + ShdwY;

					Rs.left = rectnow.left + ShdwX;
					Rs.right = rectnow.right + ShdwX;

					SetTextColor(bufDC, ShdwClr);
					DrawText(bufDC, pTaskList->caption, -1, &Rs,
						(TextSize.cx >= (rectnow.right - rectnow.left)) ?
						DT_LEFT|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS:
						TB_Just|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);
				}

			SetTextColor(bufDC, TaskTextColor);
            DrawText(bufDC, pTaskList->caption, -1, &rectnow,
                ((TextSize.cx >= (rectnow.right - rectnow.left)) ?
                DT_LEFT|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS:
                TB_Just|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS)
                );*/
			BBDrawTextSBX(bufDC, pTaskList->caption, -1, &rectnow, 
                ((TextSize.cx >= (rectnow.right - rectnow.left)) ?
                DT_LEFT|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS:
                TB_Just|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS), &StyleItemArray[style]);
            goto set_tt;
        }
    }

    rectnow.top = TaskRect.top + (TaskRect.bottom - TaskRect.top - TaskIconItem.m_size) / 2;

    if (!bIconBuffersUpdated || (task_width_old < 0)) {
        bmpInfo.biWidth = 2 * TaskIconItem.m_size;
        bmpInfo.biHeight = TaskIconItem.m_size;
        DeleteObject((HBITMAP)SelectObject(bufIcon, CreateDIBSection(
            0, (BITMAPINFO*)&bmpInfo, DIB_RGB_COLORS, 0, 0, 0)));
        goto full_ico_rf;
    }

    if (task_width_old != TextSize.cx) {
full_ico_rf:
        bBufferA = bBufferI = false;
        goto refresh_buf;
    }

    if (bPressed) {
        if (!bBufferA)
            goto refresh_buf;
    } else if (!bBufferI)
        goto refresh_buf;

    if (hIcon != hIcon_old) {
refresh_buf:
        BitBlt(bufIcon, (bPressed ? 0 : TaskIconItem.m_size), 0, TaskIconItem.m_size, TaskIconItem.m_size, bufDC, rectnow.left, rectnow.top, SRCCOPY);

        TaskIconItem.PaintIcon( bufIcon, (bPressed ? 0 : TaskIconItem.m_size), 0, hIcon, 
			(bPressed && !ToggleInfo.test(TI_SAT_HUE_ON_ACTIVE_TASK)) );

        if (bPressed)
            bBufferA = true;
        else
            bBufferI = true;

        task_width_old = TextSize.cx;
        hIcon_old = hIcon;
    }
    BitBlt(bufDC, rectnow.left, rectnow.top, TaskIconItem.m_size, TaskIconItem.m_size, bufIcon, (bPressed ? 0: TaskIconItem.m_size), 0, SRCCOPY);

set_tt:
    TooltipActivationRect.left = TaskRect.left;
    TooltipActivationRect.right = TaskRect.right;
    bbTip.Set(&TooltipActivationRect, pTaskList->caption, bTaskTooltips);

    //InscriptionLeft = TaskRect.right;
    TaskRect.bottom = SystemBarExHeight;
    TaskRect.top = 0;

    if (TaskRect.left == MainRect.left)
        TaskRect.left = TasksMouseSize.cx = 0;
    if (TaskRect.right == MainRect.right)
        TaskRect.right = TasksMouseSize.cy = SystemBarExWidth;
}

//-----------------------------------------------------------------------------

HICON TaskbarItem::GetIconHandle ( int size ) {
#define ICO_FLAGS       SMTO_BLOCK|SMTO_ABORTIFHUNG
#define ICO_TIMEOUT     200
    if (size > 16) {
        if (hIcon_local_b)
            return hIcon_local_b;
        
#ifdef _WIN64
		SendMessageTimeout(pTaskList->hwnd, WM_GETICON, ICON_BIG, 0, ICO_FLAGS, ICO_TIMEOUT, (PDWORD_PTR)&hIcon_local_b);
		if (hIcon_local_b || (hIcon_local_b = (HICON)GetClassLongPtr(pTaskList->hwnd, GCLP_HICON)))
#else
		SendMessageTimeout(pTaskList->hwnd, WM_GETICON, ICON_BIG, 0, ICO_FLAGS, ICO_TIMEOUT, (LPDWORD)&hIcon_local_b);
        if (hIcon_local_b || (hIcon_local_b = (HICON)GetClassLongPtr(pTaskList->hwnd, GCL_HICON)))
#endif
            return (hIcon_local_b = CopyIcon(hIcon_local_b));

        if (hIcon_local_s)
            return hIcon_local_s;
        
#ifdef _WIN64
		SendMessageTimeout(pTaskList->hwnd, WM_GETICON, ICON_SMALL, 0, ICO_FLAGS, ICO_TIMEOUT, (PDWORD_PTR)&hIcon_local_s);
		if (hIcon_local_s || (hIcon_local_s = (HICON)GetClassLongPtr(pTaskList->hwnd, GCLP_HICONSM)))
#else
		SendMessageTimeout(pTaskList->hwnd, WM_GETICON, ICON_SMALL, 0, ICO_FLAGS, ICO_TIMEOUT, (LPDWORD)&hIcon_local_s);
        if (hIcon_local_s || (hIcon_local_s = (HICON)GetClassLongPtr(pTaskList->hwnd, GCL_HICONSM)))
#endif
            return (hIcon_local_s = CopyIcon(hIcon_local_s));
    } else {
        if (hIcon_local_s)
            return hIcon_local_s;
        
#ifdef _WIN64
		SendMessageTimeout(pTaskList->hwnd, WM_GETICON, ICON_SMALL, 0, ICO_FLAGS, ICO_TIMEOUT, (PDWORD_PTR)&hIcon_local_s);
        if (hIcon_local_s || (hIcon_local_s = (HICON)GetClassLongPtr(pTaskList->hwnd, GCLP_HICONSM)))
#else
		SendMessageTimeout(pTaskList->hwnd, WM_GETICON, ICON_SMALL, 0, ICO_FLAGS, ICO_TIMEOUT, (LPDWORD)&hIcon_local_s);
        if (hIcon_local_s || (hIcon_local_s = (HICON)GetClassLongPtr(pTaskList->hwnd, GCL_HICONSM)))
#endif
            return (hIcon_local_s = CopyIcon(hIcon_local_s));

        if (hIcon_local_b)
            return hIcon_local_b;
        //SendMessageTimeout(pTaskList->hwnd, WM_GETICON, ICON_BIG, 0, ICO_FLAGS, ICO_TIMEOUT, (LPDWORD)&hIcon_local_b);
#ifdef _WIN64
		SendMessageTimeout(pTaskList->hwnd, WM_GETICON, ICON_BIG, 0, ICO_FLAGS, ICO_TIMEOUT, (PDWORD_PTR)&hIcon_local_b);
        if (hIcon_local_b || (hIcon_local_b = (HICON)GetClassLongPtr(pTaskList->hwnd, GCLP_HICON)))
#else
		SendMessageTimeout(pTaskList->hwnd, WM_GETICON, ICON_BIG, 0, ICO_FLAGS, ICO_TIMEOUT, (LPDWORD)&hIcon_local_b);
        if (hIcon_local_b || (hIcon_local_b = (HICON)GetClassLongPtr(pTaskList->hwnd, GCL_HICON)))
#endif
            return (hIcon_local_b = CopyIcon(hIcon_local_b));
    }
    return 0;

#undef ICO_FLAGS
#undef ICO_TIMEOUT
}


//-----------------------------------------------------------------------------


// **************************  class GestureItem  *****************************

void GestureItem::Set ( UINT msg, HWND hwnd ) {
    if ((msg == WM_LBUTTONDOWN) || (msg == WM_RBUTTONDOWN)) {
        m_x = MouseEventPoint.x;
        m_hwnd = hwnd;
	} else {
        m_hwnd = 0;
	}
}

//-----------------------------------------------------------------------------

bool GestureItem::Process(UINT msg) {
    if ((msg != WM_MOUSEMOVE) && m_hwnd) {
        WPARAM wp1 = 6, wp2 = 7;
        LPARAM lp = (LPARAM)m_hwnd;
        m_threshold = min(50, min((m_x / 2), ((SystemBarExWidth - m_x) / 2)));
        m_hwnd = 0;
        switch (msg) {
            case WM_RBUTTONUP:
                wp1 = 0, wp2 = 1, lp = 0;
            case WM_LBUTTONUP:
                if (MouseEventPoint.x < (m_x - m_threshold)) {
                    PostMessage(hBlackboxWnd, BB_WORKSPACE, wp1, lp);
                    return true;
                } else if (MouseEventPoint.x > (m_x + m_threshold)) {
                    PostMessage(hBlackboxWnd, BB_WORKSPACE, wp2, lp);
                    return true;
                }
        }
    }
    return false;
}
