/*
 ============================================================================
 Blackbox for Windows: BBPager
 ============================================================================
 Copyright © 2003 nc-17@ratednc-17.com
 http://www.ratednc-17.com
 ============================================================================

  Blackbox for Windows is free software, released under the
  GNU General Public License (GPL version 2 or later), with an extension
  that allows linking of proprietary modules under a controlled interface.
  What this means is that plugins etc. are allowed to be released
  under any license the author wishes. Please note, however, that the
  original Blackbox gradient math code used in Blackbox for Windows
  is available under the BSD license.

  http://www.fsf.org/licenses/gpl.html
  http://www.fsf.org/licenses/gpl-faq.html#LinkingOverControlledInterface
  http://www.xfree86.org/3.3.6/COPYRIGHT2.html#5

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  For additional license information, please read the included license.html

 ============================================================================
*/

#define REFRESH_TIME 1000	// Automatic repaint time in milliseconds

#include "BBPager.h"

// This is a must have!  This is how BBSlit knows how big it needs to be!
#define SLIT_ADD		11001
#define SLIT_REMOVE		11002
#define SLIT_UPDATE		11003

//using namespace std;

LPSTR szAppName = "BBPager";  // The name of our window class, etc.
LPSTR szVersion = "BBPager 1.3b5";

LPSTR szInfoVersion = "1.3b5";
LPSTR szInfoAuthor = "NC-17";
LPSTR szInfoRelDate = "2006-02-05";
LPSTR szInfoLink = "http://www.ratednc-17.com/";
LPSTR szInfoEmail = "nc-17@ratednc-17.com";

//=====================================================================

HINSTANCE hInstance;
HWND hwndBBPager, hwndBlackbox;

// Blackbox messages we want to "subscribe" to:
// BB_RECONFIGURE -> Sent when changing style and on reconfigure
// BB_BROADCAST -> Broadcast message (bro@m)
int msgs[] = {BB_RECONFIGURE, BB_BROADCAST, BB_DESKTOPINFO, BB_WORKSPACE, BB_BRINGTOFRONT,
	BB_ADDTASK,	BB_REMOVETASK, BB_ACTIVETASK, BB_MINMAXTASK, BB_WINDOWSHADE, BB_WINDOWGROWHEIGHT,
	BB_WINDOWGROWWIDTH,	BB_WINDOWLOWER,	BB_REDRAWTASK, /*BB_MINIMIZE,*/ BB_WINDOWMINIMIZE, BB_WINDOWRAISE,
	BB_WINDOWMAXIMIZE, BB_WINDOWRESTORE, BB_WINDOWCLOSE, 
	BB_TASKSUPDATE, // IMPORTANT for flashing.
	0};

// Structure definitions
struct FRAME frame;
struct DESKTOP desktop;
struct ACTIVEDESKTOP activeDesktop;
struct WINDOW window;
struct FOCUSEDWINDOW focusedWindow;
struct POSITION position;

// Desktop information
int desktops;
int desktopPressed = -1;
//char desktopName[64][MAX_LINE_LENGTH];
vector<string> desktopName;

// Window information
//winStruct winList[255];
vector<winStruct> winList;
int winCount = 0;
int winPressed;
bool winMoving;
winStruct moveWin;
int mx, my, oldx, oldy, grabDesktop;
HWND lastActive;
//bool debug = false;

// flashing tasks
vector<flashTask> flashList;

// tooltips
HWND hToolTips;
bool tempTool;

// Mouse button settings
bool leftButtonDown = false, middleButtonDown = false, 
rightButtonDown = false, rightButtonDownNC = false;

// Menus
Menu *BBPagerMenu, *BBPagerWindowSubMenu, *BBPagerDisplaySubMenu;
Menu *BBPagerPositionSubMenu, *BBPagerAlignSubMenu;
Menu *BBPagerSettingsSubMenu, *BBPagerDebugSubMenu;

// Transparency
bool usingWin2kXP;
//int transparencyAlpha;
//bool transparency;

// Slit items
int heightOld = 0, widthOld = 0, posXOld = 0, posYOld = 0;
int xpos, ypos;
bool inSlit = false;	//Are we loaded in the slit? (default of no)
HWND hSlit;				//The Window Handle to the Slit (for if we are loaded)

// Compatibility
bool (*BBPager_STL)(HWND, struct taskinfo *, UINT) = NULL;
tasklist* (*BBPager_GTLP)(void) = NULL;
//static char blackbox_exe [] = "\0BLACKBOX.EXE";
struct tasklist *tl;
bool usingBBLean, usingAltMethod;

//bool loggerOn = true;

//===========================================================================

int beginSlitPlugin(HINSTANCE hMainInstance, HWND hBBSlit)
{
	/* Since we were loaded in the slit we need to remember the Slit
	 * HWND and make sure we remember that we are in the slit ;)
	 */

	//inSlit = true; // now uses RC Setting
	hSlit = hBBSlit;

	// Start the plugin like normal now..
	return beginPlugin(hMainInstance);
}

int beginPluginEx(HINSTANCE hPluginInstance, HWND hwndBBSlit) 
{  
	// start just like beginSlitPlugin
	hSlit = hwndBBSlit;
	return beginPlugin(hPluginInstance);
} 

//===========================================================================

int beginPlugin(HINSTANCE hPluginInstance)
{
	WNDCLASS wc;
	hwndBlackbox = GetBBWnd();
	hInstance = hPluginInstance;

	/*static struct { char *module; char *procname; void *procadr; }
	rtl_list [] =
	{
		{ blackbox_exe, "GetTaskListPtr",   &BBPager_GTLP },
		{ blackbox_exe, "SetTaskLocation",   &BBPager_STL },
		{ NULL }
	},
	*rtl_ptr = rtl_list;

	do 
	{
		HMODULE hLib;
		hLib = GetModuleHandle(rtl_ptr->module+1);
		if (NULL == hLib)
		hLib = LoadLibrary(rtl_ptr->module+1), rtl_ptr->module[0]=1;
		// if either lib or proc is not available,
		// the proc-ptr is set to NULL, so check before calling it
		*(FARPROC*)rtl_ptr->procadr = GetProcAddress(hLib, rtl_ptr->procname);
	} 
	while ((++rtl_ptr)->module);*/

	*(FARPROC*)&BBPager_STL = GetProcAddress(GetModuleHandle(NULL), "SetTaskLocation");
	*(FARPROC*)&BBPager_GTLP = GetProcAddress(GetModuleHandle(NULL), "GetTaskListPtr");

	// Register the window class...
	ZeroMemory(&wc,sizeof(wc));
	wc.lpfnWndProc = WndProc;			// our window procedure
	wc.hInstance = hPluginInstance;		// hInstance of .dll
	wc.lpszClassName = szAppName;		// our window class name
	if (!RegisterClass(&wc)) 
	{
		MessageBox(hwndBlackbox, "Error registering window class", szAppName, MB_OK | MB_ICONERROR | MB_TOPMOST);
		return 1;
	}

	// Get plugin and style settings...
	usingBBLean = IsInString(GetBBVersion(), "bbLean");
	InitRC();
	ReadRCSettings();
	GetStyleSettings();

	inSlit = useSlit;
	tempTool = desktop.tooltips;

	// Set window size and position
	//UpdatePosition();

	desktopName.clear();
	winList.clear();
	desktopRect.clear();

	/*if (position.autohide && !inSlit)
	{
		position.hidden = false;
		HidePager();
	}
	else*/
	{
		GetPos(false);
		SetPos(position.side);
		UpdatePosition();
	}

	// Create the window...
	hwndBBPager = CreateWindowEx(
						WS_EX_TOOLWINDOW,								// window style
						szAppName,										// our window class name
						NULL,											// NULL -> does not show up in task manager!
						WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,	// window parameters
						position.x,										// x position
						position.y,										// y position
						0,											// window width
						0,											// window height
						NULL,											// parent window
						NULL,											// no menu
						hPluginInstance,								// hInstance of .dll
						NULL);

	if (!hwndBBPager)
	{						   
		MessageBox(0, "Error creating window", szAppName, MB_OK | MB_ICONERROR | MB_TOPMOST);
		return 1;
	}

	if (!hSlit)
		hSlit = FindWindow("BBSlit", "");

	if (inSlit && hSlit)		// Are we in the Slit?
	{
		if (position.snapWindow) position.snapWindowOld = true;
		else position.snapWindowOld = false;
		position.snapWindow = false;
		if (position.autohide) position.autohideOld = true;
		else position.autohideOld = false;
		position.autohide = false;
		SendMessage(hSlit, SLIT_ADD, NULL, (LPARAM)hwndBBPager);
		
		/* A window can not be a WS_POPUP and WS_CHILD so remove POPUP and add CHILD
		 * IF you decide to allow yourself to be unloaded from the slit, then you would
		 * do the oppisite, remove CHILD and add POPUP
		 */
		SetWindowLong(hwndBBPager, GWL_STYLE, (GetWindowLong(hwndBBPager, GWL_STYLE) & ~WS_POPUP) | WS_CHILD);

		// Make your parent window BBSlit
		SetParent(hwndBBPager, hSlit);
	}
	else
	{
		useSlit = inSlit = false;
	}

	// Transparency is only supported under Windows 2000/XP...
	OSVERSIONINFO osInfo;
	ZeroMemory(&osInfo, sizeof(osInfo));
	osInfo.dwOSVersionInfoSize = sizeof(osInfo);
	GetVersionEx(&osInfo);

	if (osInfo.dwPlatformId == VER_PLATFORM_WIN32_NT && osInfo.dwMajorVersion >= 5) 
		usingWin2kXP = true;
	else 
		usingWin2kXP = false;

	if (usingWin2kXP)
	{
		if (transparency && !inSlit)
		{
			//SetWindowLong(hwndBBPager, GWL_EXSTYLE, WS_EX_TOOLWINDOW | WS_EX_LAYERED);
			//BBSetLayeredWindowAttributes(hwndBBPager, NULL, (unsigned char)transparencyAlpha, LWA_ALPHA);
			SetTransparency(hwndBBPager, (unsigned char)transparencyAlpha);
		}
	}

	// Register to receive Blackbox messages...
	SendMessage(hwndBlackbox, BB_REGISTERMESSAGE, (WPARAM)hwndBBPager, (LPARAM)msgs);
	// Set magicDWord to make the window sticky (same magicDWord that is used by LiteStep)...
	//SetWindowLong(hwndBBPager, GWL_USERDATA, magicDWord);
	MakeSticky(hwndBBPager);
	// Make the window AlwaysOnTop?
	if (position.raised) SetWindowPos(hwndBBPager, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOMOVE);

	// Set window size again, etc.
	UpdateMonitorInfo();
	UpdatePosition();

	if (hSlit && inSlit)
	{
		SetWindowPos(hwndBBPager, HWND_TOP, 0, 0, frame.width, frame.height, SWP_NOMOVE | SWP_NOZORDER);
		SendMessage(hSlit, SLIT_UPDATE, 0, 0);
	}

	if (usingBBLean)
		tl = BBPager_GetTaskListPtr();

	// Show the window and force it to update...
	ShowWindow(hwndBBPager, SW_SHOW);
	InvalidateRect(hwndBBPager, NULL, true);

    hToolTips = CreateWindowEx(	// Create tooltips window
        WS_EX_TOPMOST,
        TOOLTIPS_CLASS,
        NULL,
        WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        NULL,
        NULL,
        hPluginInstance,
        NULL);

    if (!hToolTips)
    {
        MessageBox(0, "Error creating tooltips window", szAppName, MB_OK | MB_ICONERROR | MB_TOPMOST);
        //Log("SysTray: Error creating tooltips window", NULL);
    }
    else	// set tooltip delay times based on MS Windows setting
    {
        SendMessage(hToolTips, TTM_SETMAXTIPWIDTH, 0, 300);
		SendMessage(hToolTips, TTM_SETDELAYTIME, TTDT_AUTOPOP, GetDoubleClickTime()*10);
		SendMessage(hToolTips, TTM_SETDELAYTIME, TTDT_INITIAL, GetDoubleClickTime());
		SendMessage(hToolTips, TTM_SETDELAYTIME, TTDT_RESHOW,  GetDoubleClickTime()/5);
    }

	// Timer for auto redraw, to catch window movement
	if (!SetTimer(hwndBBPager, 1, REFRESH_TIME, (TIMERPROC)NULL))
	{
		MessageBox(0, "Error creating update timer", szAppName, MB_OK | MB_ICONERROR | MB_TOPMOST);
		//return 0;
	}

	return 0;
}

//===========================================================================

void endPlugin(HINSTANCE hPluginInstance)
{
	KillTimer(hwndBBPager, 1);

	// 030703 - commented out, maybe people want to set and reload bb4win.
	// Write the current plugin position only to the config file...
	//WriteInt(rcpath, "bbpager.position.x:", position.x);
	//WriteInt(rcpath, "bbpager.position.y:", position.y);

	// Delete used StyleItems...
	if (frame.style) delete frame.style;
	if (desktop.style) delete desktop.style;
	if (activeDesktop.style) delete activeDesktop.style;
	if (window.style) delete window.style;
	if (focusedWindow.style) delete focusedWindow.style;

	// Delete the main plugin menu if it exists (PLEASE NOTE that this takes care of submenus as well!)
	if (BBPagerMenu) DelMenu(BBPagerMenu);

	// remove from slit
	if (inSlit)
		SendMessage(hSlit, SLIT_REMOVE, NULL, (LPARAM)hwndBBPager);

	// Unregister Blackbox messages...
	SendMessage(hwndBlackbox, BB_UNREGISTERMESSAGE, (WPARAM)hwndBBPager, (LPARAM)msgs);

	// Delete vectors
	desktopName.clear();
	winList.clear();
	desktopRect.clear();
	flashList.clear();

	/*static char *libnames[] =
	{
		blackbox_exe,
		NULL
	};

	char **p = libnames;
	do 
		if (**p) FreeLibrary(GetModuleHandle(*p+1));
	while (*++p);*/
	
	// Destroy our window...
	ClearToolTips();
    DestroyWindow(hToolTips);
	DestroyWindow(hwndBBPager);

	// Unregister window class...
	UnregisterClass(szAppName, hPluginInstance);
}

//===========================================================================

LPCSTR pluginInfo(int field)
{
	// pluginInfo is used by Blackbox for Windows to fetch information about
	// a particular plugin. At the moment this information is simply displayed
	// in an "About loaded plugins" MessageBox, but later on this will be
	// expanded into a more advanced plugin handling system. Stay tuned! :)

	switch (field)
	{
		case 1:
			return szAppName; // Plugin name
		case 2:
			return szInfoVersion; // Plugin version
		case 3:
			return szInfoAuthor; // Author
		case 4:
			return szInfoRelDate; // Release date, preferably in yyyy-mm-dd format
		case 5:
			return szInfoLink; // Link to author's website
		case 6:
			return szInfoEmail; // Author's email

		// ==========

		default:
			return szVersion; // Fallback: Plugin name + version, e.g. "MyPlugin 1.0"
	}
}

//===========================================================================

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{		
		// Window update process...

		case WM_PAINT:
		{
			UpdatePosition(); // get number of desktops and sets size/position of window
	
			DrawBBPager(hwnd);

			//ClearToolTips();

			return 0;
		}
		break;

        //====================

        case BB_TASKSUPDATE:
        {
			if (desktop.windows)
            {
				HWND taskHwnd = (HWND)wParam;

                if ((int)lParam == TASKITEM_MODIFIED)
                {
                }
                else if((int)lParam == TASKITEM_ACTIVATED)
                {
					RemoveFlash(taskHwnd, false);
					InvalidateRect(hwndBBPager, NULL, true);
                }
				else if ((int)lParam == TASKITEM_ADDED)
				{
					InvalidateRect(hwndBBPager, NULL, true);
				}
				else if ((int)lParam == TASKITEM_FLASHED)
				{
					if (IsFlashOn(taskHwnd))
					{
						RemoveFlash(taskHwnd, true);
					}
					else if (taskHwnd != GetForegroundWindow())
					{
						AddFlash(taskHwnd);
						InvalidateRect(hwndBBPager, NULL, true);
					}
				}
                else if ((int)lParam == TASKITEM_REMOVED)
                {
					RemoveFlash(taskHwnd, false);
					InvalidateRect(hwndBBPager, NULL, true);
                }
				//InvalidateRect(hwndBBPager, NULL, true);
            }
        }
        break;

		// =================
		// Broadcast messages (bro@m -> the bang killah! :D <vbg>)

		case BB_BROADCAST:
		{
			char temp[MAX_LINE_LENGTH];
			strcpy(temp, (LPCSTR)lParam);
			
			// @BBShowPlugins: Show window and force update (global bro@m)
			if (!_stricmp(temp, "@BBShowPlugins")) 
			{
				ShowWindow(hwndBBPager, SW_SHOW);
				InvalidateRect(hwndBBPager, NULL, true);
				return 0;
			}
			// @BBHidePlugins: Hide window (global bro@m)
			else if (!_stricmp(temp, "@BBHidePlugins")) 
			{
				if (!inSlit)
					ShowWindow(hwndBBPager, SW_HIDE);
				return 0;
			}
			// @BBPagerReload rereads RC/style settings and updates window
			// Nice to do without reconfiguring all of blackbox
			else if (!_stricmp(temp, "@BBPagerReload")) 
			{
				ReadRCSettings();
				GetStyleSettings();
				//UpdatePosition();
				InvalidateRect(hwndBBPager, NULL, true);
				SendMessage(hwndBlackbox, BB_SETTOOLBARLABEL, 0, (LPARAM)"BBPager -> Reload Settings");

				return 0;
			}
		/*	else if (!_stricmp(temp, "@BBPagerSave")) 
			{
				WriteRCSettings();
				SendMessage(hwndBlackbox, BB_SETTOOLBARLABEL, 0, (LPARAM)"BBPager -> Save Settings");
			}*/
			// start positioning bro@ms for BBPager
			else if (IsInString(temp, "@BBPagerPosition"))
			{
				if (!inSlit)
				{
					static char msg[MAX_LINE_LENGTH];
					static char status[9];
					char token1[4096], token2[4096], extra[4096], /*tpos[16], */tmsg[4096];
					LPSTR tokens[2];
					tokens[0] = token1;
					tokens[1] = token2;

					token1[0] = token2[0] = extra[0] = '\0';
					BBTokenize (temp, tokens, 2, extra);

					if (!_stricmp(token2, "TopLeft")) 
					{
						position.side = 3;
						/*SetPos(position.side);
						sprintf(tpos, "TopLeft");*/
					}
					else if (!_stricmp(token2, "TopCenter")) 
					{
						position.side = 2;
						/*SetPos(position.side);
						sprintf(tpos, "TopCenter");*/
					}
					else if (!_stricmp(token2, "TopRight")) 
					{
						position.side = 6;
						/*SetPos(position.side);
						sprintf(tpos, "TopRight");*/
					}
					else if (!_stricmp(token2, "CenterLeft")) 
					{
						position.side = 1;
						/*SetPos(position.side);
						sprintf(tpos, "CenterLeft");*/
					}
					else if (!_stricmp(token2, "CenterRight")) 
					{
						position.side = 4;
						/*SetPos(position.side);
						sprintf(tpos, "CenterRight");*/
					}
					else if (!_stricmp(token2, "BottomLeft")) 
					{
						position.side = 9;
						/*SetPos(position.side);
						sprintf(tpos, "BottomLeft");*/
					}
					else if (!_stricmp(token2, "BottomCenter")) 
					{
						position.side = 8;
						/*SetPos(position.side);					
						sprintf(tpos, "BottomCenter");*/
					}
					else if (!_stricmp(token2, "BottomRight")) 
					{
						position.side = 12;
						/*SetPos(position.side);					
						sprintf(tpos, "BottomRight");*/
					}
					else
					{
						return 0;
					}

					SetPos(position.side);
					//sprintf(tpos, "token2");

					SetWindowPos(hwndBBPager, HWND_TOP, position.x, position.y, frame.width, frame.height, SWP_NOZORDER);
					InvalidateRect(hwndBBPager, NULL, true);

					sprintf(tmsg, "BBPager -> Set Position: %s", token2);
					SendMessage(hwndBlackbox, BB_SETTOOLBARLABEL, 0, (LPARAM)tmsg);
				}

				WriteRCSettings();
			}
			// start internal bro@ms for BBPager only
			else if (IsInString(temp, "@BBPagerInternal"))
			{
				//static char msg[MAX_LINE_LENGTH];
				//static char status[9];
				char token1[4096], token2[4096], extra[4096];
				LPSTR tokens[2];
				tokens[0] = token1;
				tokens[1] = token2;

				token1[0] = token2[0] = extra[0] = '\0';
				BBTokenize (temp, tokens, 2, extra);

				//open bbpager.rc file with default editor
				if (!_stricmp(token2, "OpenRC")) 
				{
					BBExecute(GetDesktopWindow(), NULL, editor, rcpath, NULL, SW_SHOWNORMAL, false);
					SendMessage(hwndBlackbox, BB_SETTOOLBARLABEL, 0, (LPARAM)"BBPager -> Edit Settings");
					return 0;
				}
				// open bbpager.bb file with default editor
				else if (!_stricmp(token2, "OpenStyle")) 
				{
					BBExecute(GetDesktopWindow(), NULL, editor, bspath, NULL, SW_SHOWNORMAL, false);
					SendMessage(hwndBlackbox, BB_SETTOOLBARLABEL, 0, (LPARAM)"BBPager -> Edit Style");
					return 0;
				}
				// @BBPagerToggleNumbers toggles drawing of numbers on desktops
				else if (!_stricmp(token2, "ToggleNumbers")) 
				{
					if (desktop.numbers) 
					{
						desktop.numbers = false;
						SendMessage(hwndBlackbox, BB_SETTOOLBARLABEL, 0, (LPARAM)"BBPager -> Desktop Numbers disabled");
					}
					else 
					{
						desktop.numbers = true;						
						SendMessage(hwndBlackbox, BB_SETTOOLBARLABEL, 0, (LPARAM)"BBPager -> Desktop Numbers enabled");
					}

					InvalidateRect(hwndBBPager, NULL, true);
				}
				// @BBPagerVertical / Horizontal toggles vertical/horizontal alignment
				// also forces window to update
				else if (!_stricmp(token2, "Vertical")) 
				{
					if (position.horizontal) // set drawing to use columns/vertical
					{
						position.horizontal = false; 
						position.vertical = true;
						SendMessage(hwndBlackbox, BB_SETTOOLBARLABEL, 0, (LPARAM)"BBPager -> Alignment set to Vertical");
					}
					else
					{
						position.horizontal = true; 
						position.vertical = false;
						SendMessage(hwndBlackbox, BB_SETTOOLBARLABEL, 0, (LPARAM)"BBPager -> Alignment set to Horizontal");
					}
					InvalidateRect(hwndBBPager, NULL, true);					
				}
				else if (!_stricmp(token2, "Horizontal"))
				{
					if (position.vertical) // set drawing to use rows/horizontal
					{
						position.horizontal = true; 
						position.vertical = false;
						SendMessage(hwndBlackbox, BB_SETTOOLBARLABEL, 0, (LPARAM)"BBPager -> Alignment set to Horizontal");
					}
					else
					{
						position.horizontal = false; 
						position.vertical = true;
						SendMessage(hwndBlackbox, BB_SETTOOLBARLABEL, 0, (LPARAM)"BBPager -> Alignment set to Vertical");
					}
					InvalidateRect(hwndBBPager, NULL, true);
				}
				 // @BBPagerToggleSnap toggles snap to windows and sets toolbar msg
				else if (!_stricmp(token2, "ToggleSnap")) 
				{
					if (!inSlit)
					{
						if (position.snapWindow) 
						{
							position.snapWindow = false;
							SendMessage(hwndBlackbox, BB_SETTOOLBARLABEL, 0, (LPARAM)"BBPager -> Snap Window To Edge disabled");
						}
						else 
						{
							position.snapWindow = true;
							SendMessage(hwndBlackbox, BB_SETTOOLBARLABEL, 0, (LPARAM)"BBPager -> Snap Window To Edge enabled");
						}
					}
				}
				// @BBPagerToggleRaised toggles always on top and sets toolbar msg
				else if (!_stricmp(token2, "ToggleRaised")) 
				{
					if (position.raised) 
					{
						position.raised = false;
						SetWindowPos(hwndBBPager, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOMOVE);
						SendMessage(hwndBlackbox, BB_SETTOOLBARLABEL, 0, (LPARAM)"BBPager -> Always On Top disabled");
					}
					else 
					{
						position.raised = true;
						SetWindowPos(hwndBBPager, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOMOVE);
						SendMessage(hwndBlackbox, BB_SETTOOLBARLABEL, 0, (LPARAM)"BBPager -> Always On Top enabled");
					}
				}
				// @BBPagerInternal ToggleHide toggles autohiding
				else if (!_stricmp(token2, "ToggleHide")) 
				{
					if (!inSlit)
					{
						if (position.autohide) 
						{
							position.autohide = false;
							position.hidden = false;
							//SetWindowPos(hwndBBPager, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOMOVE);
							SendMessage(hwndBlackbox, BB_SETTOOLBARLABEL, 0, (LPARAM)"BBPager -> Autohide disabled");
						}
						else 
						{
							position.autohide = true;
							GetPos(true);
							SetPos(position.side);
							//SetWindowPos(hwndBBPager, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOMOVE);
							SendMessage(hwndBlackbox, BB_SETTOOLBARLABEL, 0, (LPARAM)"BBPager -> Autohide enabled");
							HidePager();
						}
					}
				}
				// @BBPagerToggleWindows toggles window painting
				else if (!_stricmp(token2, "ToggleWindows")) 
				{
					if (desktop.windows) 
					{
						desktop.windows = false;
						SendMessage(hwndBlackbox, BB_SETTOOLBARLABEL, 0, (LPARAM)"BBPager -> Desktop Windows disabled");
					}
					else 
					{
						desktop.windows = true;
						SendMessage(hwndBlackbox, BB_SETTOOLBARLABEL, 0, (LPARAM)"BBPager -> Desktop Windows enabled");
					}

					InvalidateRect(hwndBBPager, NULL, true);
				}
				else if (!_stricmp(token2, "ToggleToolTips")) 
				{
					if (desktop.tooltips) 
					{
						desktop.tooltips = false;
						ClearToolTips();
						SendMessage(hwndBlackbox, BB_SETTOOLBARLABEL, 0, (LPARAM)"BBPager -> Window ToolTips disabled");
					}
					else 
					{
						desktop.tooltips = true;
						SendMessage(hwndBlackbox, BB_SETTOOLBARLABEL, 0, (LPARAM)"BBPager -> Window Tooltips enabled");
					}
					tempTool = desktop.tooltips;

					InvalidateRect(hwndBBPager, NULL, true);
				}
				else if (!_stricmp(token2, "ToggleAltMethod") && usingBBLean) 
				{
					if (usingAltMethod) 
					{
						usingAltMethod = false;
						SendMessage(hwndBlackbox, BB_SETTOOLBARLABEL, 0, (LPARAM)"BBPager -> Alternate Method disabled");
					}
					else 
					{
						usingAltMethod = true;
						SendMessage(hwndBlackbox, BB_SETTOOLBARLABEL, 0, (LPARAM)"BBPager -> Alternate Method enabled");
					}

					InvalidateRect(hwndBBPager, NULL, true);
				}
				else if (!_stricmp(token2, "ToggleBorder")) 
				{
					if (drawBorder) 
					{
						drawBorder = false;
						SendMessage(hwndBlackbox, BB_SETTOOLBARLABEL, 0, (LPARAM)"BBPager -> Draw Border disabled");
					}
					else 
					{
						drawBorder = true;
						SendMessage(hwndBlackbox, BB_SETTOOLBARLABEL, 0, (LPARAM)"BBPager -> Draw Border enabled");
					}

					GetStyleSettings();
					InvalidateRect(hwndBBPager, NULL, true);
				}
				// @BBPagerInternal Transparency toggles transparency in w2k/xp
				else if (!_stricmp(token2, "ToggleTrans"))
				{
					if (usingWin2kXP)
					{
						if (transparency)
						{
							transparency = false;
							//SetWindowLong(hwndBBPager, GWL_EXSTYLE, WS_EX_TOOLWINDOW);
							SetTransparency(hwndBBPager, 255);
							SendMessage(hwndBlackbox, BB_SETTOOLBARLABEL, 0, (LPARAM)"BBPager -> Transparency disabled");
						}
						else if (!inSlit)
						{
							transparency = true;
							//SetWindowLong(hwndBBPager, GWL_EXSTYLE, WS_EX_TOOLWINDOW | WS_EX_LAYERED);
							//BBSetLayeredWindowAttributes(hwndBBPager, NULL, (unsigned char)transparencyAlpha, LWA_ALPHA);
							SetTransparency(hwndBBPager, (unsigned char)transparencyAlpha);
							SendMessage(hwndBlackbox, BB_SETTOOLBARLABEL, 0, (LPARAM)"BBPager -> Transparency enabled");
						}
					}
				}
				else if (!_stricmp(token2, "ToggleSlit"))
				{
					ToggleSlit();
					WriteRCSettings();
					return 0;
				}
				else if (!_stricmp(token2, "ListWindows"))
				{
					char msg[MAX_LINE_LENGTH], temp[MAX_LINE_LENGTH], title[32];
					int i = 0;
					RECT rect;
					msg[0] = '\0';

					//if (CheckSticky(hwndBBPager))
					//	MessageBox(0, "Yes", "Pager Sticky", MB_OK);

					do
					{
						if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
							GetWindowRect(winList[i].window, &rect);
						else
							rect = winList[i].r;

						GetWindowText(winList[i].window, title, 20);
#ifdef _WIN64
						if (CheckSticky(winList[i].window) || GetWindowLong(winList[i].window, GWLP_USERDATA) == magicDWord)
#else
						if (CheckSticky(winList[i].window) || GetWindowLong(winList[i].window, GWL_USERDATA) == magicDWord)
#endif
							strcat(title, "*");	// add an * to windows who are sticky
						else strcat(title, " ");

						sprintf(temp, "%s\t|\tDesktop %d\t|\tCoords %d %d %d %d\n", title, winList[i].desk, rect.left, rect.top, rect.right, rect.bottom);
						strcat(msg, temp);
						i++;
					}
					while (i < winCount);
					MessageBox(0, msg, "BBPager: Window List", MB_OK | MB_SETFOREGROUND);
					return 0;
				}
				else if (!_stricmp(token2, "ListDesktops"))
				{
					char msg[MAX_LINE_LENGTH], temp[MAX_LINE_LENGTH];
					int i = 0;
					RECT rect;
					msg[0] = '\0';
					do
					{
						rect = desktopRect[i];
						sprintf(temp, "Desktop %d\t|\tCoords %d %d %d %d\n", i, rect.left, rect.top, rect.right, rect.bottom);
						strcat(msg, temp);
						i++;
					}
					while (i < desktops);

					sprintf(temp, "\nRatioX: %f, RatioY: %f\n\n", ratioX, ratioY);
					strcat(msg, temp);
					sprintf(temp, "Screen: %d %d %d %d %d %d\n", screenLeft, screenTop, screenRight, screenBottom, screenWidth, screenHeight);
					strcat(msg, temp);
					sprintf(temp, "VScreen: %d %d %d %d %d %d\n(L T R B W H)\n", vScreenLeft, vScreenTop, vScreenRight, vScreenBottom, vScreenWidth, vScreenHeight);
					strcat(msg, temp);

					MessageBox(0, msg, "BBPager: Desktop List", MB_OK | MB_SETFOREGROUND);
					return 0;
				}
				else if (!_stricmp(token2, "ListStuff"))
				{
					char msg[MAX_LINE_LENGTH], temp[MAX_LINE_LENGTH], temp2[32];
					//int i = 0;
					msg[0] = '\0';
					sprintf(temp, "desktop.font: %d height, %d weight, %s\n\n", desktop.fontSize, desktop.fontWeight, desktop.fontFace);
					strcat(msg, temp);
					(usingBBLean) ? strcpy(temp2, "true") : strcpy(temp2, "false");
					sprintf(temp, "bbpager.usingBBLean: %s\n", temp2);	
					strcat(msg, temp);
					(usingAltMethod) ? strcpy(temp2, "true") : strcpy(temp2, "false");
					sprintf(temp, "bbpager.desktopAltMethod: %s\n", temp2);
					strcat(msg, temp);

					MessageBox(0, msg, "BBPager: Stuff List", MB_OK | MB_SETFOREGROUND);
					return 0;
				}
				// @BBPagerAbout pops up info box about the plugin
				else if (!_stricmp(token2, "About")) 
				{
					char tempaboutmsg[MAX_LINE_LENGTH];

					sprintf(tempaboutmsg, "%s\n\n"
					"© 2003 nc-17@ratednc-17.com\n\n"
					"http://www.ratednc-17.com/\n"
					"#bb4win on irc.freenode.net", szVersion);

				/*	SendMessage(hwndBlackbox, BB_SETTOOLBARLABEL, 0, (LPARAM)"BBPager -> About BBPager");
					MessageBox(0, tempaboutmsg, "About BBPager...", MB_OK | MB_TOPMOST | MB_SETFOREGROUND | MB_ICONINFORMATION);
					return 0;*/

					MSGBOXPARAMS msgbox;
					ZeroMemory(&msgbox, sizeof(msgbox));
					msgbox.cbSize = sizeof(msgbox);
					msgbox.hwndOwner = 0; //GetBBWnd();
					msgbox.hInstance = hInstance;
					msgbox.lpszText = tempaboutmsg;
					msgbox.lpszCaption = "About BBPager...";
					msgbox.dwStyle = MB_OK | MB_SETFOREGROUND | MB_TOPMOST | MB_USERICON;
					msgbox.lpszIcon = MAKEINTRESOURCE(IDI_ICON1);
					msgbox.dwContextHelpId = 0;
					msgbox.lpfnMsgBoxCallback = NULL;
					msgbox.dwLanguageId = LANG_NEUTRAL;
					MessageBoxIndirect(&msgbox);
					return 0;
				}

				WriteRCSettings();
			}

			return 0;
		}
		break;

		// =================
		// If Blackbox sends a reconfigure message, load the new style settings and update the window...

		case BB_RECONFIGURE:
		{
			ReadRCSettings();
			GetStyleSettings();
			UpdatePosition();

			UpdateMonitorInfo();
			if (!inSlit)
			{
				// snap to edge on style change
				// stops diff. bevel/border widths moving pager from screen edge
				int x; int y; int z; int dx, dy, dz;
				int nDist = 20;

				// top/bottom edge
				dy = y = position.y - screenTop;
				dz = z = position.y + frame.height - screenBottom;
				if (dy<0) dy=-dy;
				if (dz<0) dz=-dz;
				if (dz < dy) y = z, dy = dz;

				// left/right edge
				dx = x = position.x - screenLeft;
				dz = z = position.x + frame.width - screenRight;
				if (dx<0) dx=-dx;
				if (dz<0) dz=-dz;
				if (dz < dx) x = z, dx = dz;

				if (dy < nDist)
				{
					position.y -= y;
					// top/bottom center
					dz = z = position.x + (frame.width - screenRight - screenLeft)/2;
					if (dz<0) dz=-dz;
					if (dz < dx) x = z, dx = dz;
				}

				if (dx < nDist)
					position.x -= x;

				SetWindowPos(hwndBBPager, HWND_TOP, position.x, position.y, frame.width, frame.height, SWP_NOZORDER);
			}

			//UpdatePosition();

			InvalidateRect(hwndBBPager, NULL, true);
		}
		break;

		// ===================
		// Grabs the number of desktops and sets the names of the desktops
	
		case BB_DESKTOPINFO:
		{
			DesktopInfo* info = (DesktopInfo*)lParam;
			if (info->isCurrent) 
			{
				currentDesktop = info->number;
			}
			//strcpy(desktopName[desktops], info->name); // Grab the name of each desktop as it comes
			desktopName.push_back(string(info->name));
			desktops++; // Increase desktop count by one for each
		}
		break;

		//====================
		// This is done for workspace switching with the toolbar
		// as a result of focusing an app that is on another workspace

		case BB_BRINGTOFRONT:

		// Force window to update when workspaces are changed or added, etc.
		// Or when apps moved by middle click on taskbar/desktop

		case BB_WORKSPACE:
		case BB_LISTDESKTOPS:

		// Here's all the Hook msgs, makes the pager a lot more responsive to window changes
		case BB_ADDTASK:		case BB_REMOVETASK:		case BB_ACTIVETASK:
		case BB_MINMAXTASK:		case BB_WINDOWSHADE:	case BB_WINDOWGROWHEIGHT:
		case BB_WINDOWGROWWIDTH:case BB_WINDOWLOWER:	case BB_REDRAWTASK:			
		/*case BB_MINIMIZE:*/	case BB_WINDOWMINIMIZE:	case BB_WINDOWRAISE:
		case BB_WINDOWMAXIMIZE:	case BB_WINDOWRESTORE:	case BB_WINDOWCLOSE:
		{
			//UpdatePosition();
			InvalidateRect(hwndBBPager, NULL, true);
		}
		break;

		// ==================		

		case WM_WINDOWPOSCHANGING:
		{
			if (!inSlit)
			{
				// Is SnapWindowToEdge enabled?
				if (position.snapWindow)
				{
					// Snap window to screen edges (if the last bool is false it uses the current DesktopArea)
					if(IsWindowVisible(hwnd)) SnapWindowToEdge((WINDOWPOS*)lParam, 20, true);
				}
			}
			
			return 0;
		}
		break;

		// ==================
		// Save window position if it changes...
		case WM_WINDOWPOSCHANGED:
		{
			if (!inSlit)
			{
				WINDOWPOS* windowpos = (WINDOWPOS*)lParam;

				position.x = windowpos->x;
				position.y = windowpos->y;

				WriteInt(rcpath, "bbpager.position.x:", position.x);
				WriteInt(rcpath, "bbpager.position.y:", position.y);
				//WriteRCSettings();
			}

			return 0;
		}
		break;

		// ==================

		case WM_MOVING:
		{
			RECT *r = (RECT *)lParam;

			if (!position.autohide)
			{
			    //RECT *r = (RECT *)lParam;

				// Makes sure BBPager stays on screen when moving.
				if (r->left < vScreenLeft)
				{
					r->left = vScreenLeft;
					r->right = r->left + frame.width;
				}
				if ((r->left + frame.width) > vScreenRight)
				{
					r->left = vScreenRight - frame.width;
					r->right = r->left + frame.width;
				}

				if (r->top < vScreenTop) 
				{
					r->top = vScreenTop;
					r->bottom = r->top + frame.height;
				}
				if ((r->top + frame.height) > vScreenBottom)
				{
					r->top = vScreenBottom - frame.height;
					r->bottom = r->top + frame.height;
				}
			}
			
			return TRUE;
		}
		break;

		// ==================

		case WM_DISPLAYCHANGE:  
		{ 
			// IntelliMove(tm) by qwilk... <g> 
			int relx, rely;  
			int oldscreenwidth = screenWidth; 
			int oldscreenheight = screenHeight;

			/*screenLeft = GetSystemMetrics(SM_YVIRTUALSCREEN);
			screenTop = GetSystemMetrics(SM_XVIRTUALSCREEN);
			
			screenWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
			screenHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);

			screenRight = screenLeft + screenWidth;
			screenBottom = screenTop + screenHeight;*/
			UpdateMonitorInfo();

			if (position.x > oldscreenwidth / 2)  
			{ 
				relx = oldscreenwidth - position.x;  
				position.x = screenRight - relx; 
			} 

			if (position.y > oldscreenheight / 2)  
			{
				rely = oldscreenheight - position.y;  
				position.y = screenBottom - rely; 
			} 

			/*ratioX = screenWidth / desktop.width;
			ratioY = screenHeight / desktop.height;*/

			if (!inSlit)
				UpdatePosition();
				//MoveWindow(hwndBBPager, position.x, position.y, frame.width, frame.height, true);
		}
		break; 

		// ==================
		// Allow window to move...

		case WM_NCHITTEST:
		{
			ClickMouse();

			if (/*(desktopPressed == -1) || */(GetAsyncKeyState(VK_CONTROL) & 0x8000))
			{
				if (!position.autohide)
					return HTCAPTION;
				else
					return HTCLIENT;
			}
			else
			{
				return HTCLIENT;
			}
		}
		break;

		// ===================
		// Left mouse button msgs

		case WM_NCLBUTTONDBLCLK:
		//case WM_LBUTTONDBLCLK:
		{
			ToggleSlit();
		}
		break;

		case WM_NCLBUTTONDOWN:
		{
			/* Please do not allow plugins to be moved in the slit.
			 * That's not a request..  Okay, so it is.. :-P
			 * I don't want to hear about people losing their plugins
			 * because they loaded it into the slit and then moved it to
			 * the very edge of the slit window and can't get it back..
			 */
			if (!inSlit || !hSlit)// && (GetAsyncKeyState(VK_CONTROL) & 0x8000))
				return DefWindowProc(hwnd, message, wParam, lParam);
		}
		break;

		case WM_LBUTTONDOWN:
		{
			leftButtonDown = true;

			if (moveButton == 1)
				if (!winMoving)
					GrabWindow();

			return 0;
		}
		break;

		case WM_LBUTTONUP:
		{
			if (leftButtonDown) 
			{
				if (focusButton == 1 && !winMoving) // if window focus button is RMB
				{
					FocusWindow();
					//return 0;
				}

				if (moveButton == 1 && winMoving)
				{
					DropWindow();
					winMoving = false;					
					desktop.tooltips = tempTool;
					InvalidateRect(hwndBBPager, NULL, true);
				}
				
				if (desktopChangeButton == 1 && !winMoving) 
				{
					if (desktopPressed != currentDesktop)
						DeskSwitch();
				}
			}
			leftButtonDown = false;

			return 0;
		}
		break;

		// ===================
		// Middle mouse button msgs

		case WM_MBUTTONDOWN:
		{
			middleButtonDown = true;

			if (moveButton == 3)
				if (!winMoving)
					GrabWindow();

			return 0;
		}
		break;

		case WM_MBUTTONUP:
		{
			if (middleButtonDown) 
			{
				if (focusButton == 3 && !winMoving) // if window focus button is RMB
				{
					FocusWindow();
					//return 0;
				}

				if (moveButton == 3 && winMoving)
				{
					DropWindow();
					winMoving = false;
					desktop.tooltips = tempTool;
					InvalidateRect(hwndBBPager, NULL, true);
				}

				if (desktopChangeButton == 3 && !winMoving) 
				{
					if (desktopPressed != currentDesktop)
						DeskSwitch();
					//return 0;
				}
			}
			middleButtonDown = false;

			return 0;
		}
		break;
		
		case WM_NCMBUTTONUP:
		{
			if (winMoving && middleButtonDown)
			{
				DropWindow();
				winMoving = false;
				desktop.tooltips = tempTool;
				InvalidateRect(hwndBBPager, NULL, true);
			}
			middleButtonDown = false;
		}
		break;

		// ====================
		// Right mouse button msgs

		case WM_NCRBUTTONDOWN:
		{
			rightButtonDownNC = true; // right button clicked on BBPager

			return 0;
		}
		break;

		case WM_NCRBUTTONUP:
		{
			if (rightButtonDownNC) // was right button clicked on BBPager?
			{
				DisplayMenu(); // Just show menu if right clicked on Non-Client Area (not desktops)
				rightButtonDownNC = false;
			}

			if (winMoving)
			{
				winMoving = false;
				desktop.tooltips = tempTool;
				InvalidateRect(hwndBBPager, NULL, true);
			}

			return 0;
		}
		break;
		
		case WM_RBUTTONDOWN:
		{
			rightButtonDown = true; // right button clicked on BBPager

			if (moveButton == 2)
				if (!winMoving)
					GrabWindow();

			return 0;
		}
		break;		
		
		case WM_RBUTTONUP:
		{			
			if (rightButtonDown) // was right button clicked on BBPager?
			{
				if (/*desktopPressed == -1 || */(GetAsyncKeyState(VK_CONTROL) & 0x8000)) 
				{	// if mouse was not inside a desktop, show the menu
					DisplayMenu();
					return 0;
				}

				bool menu = true;
	
				if (focusButton == 2 && !winMoving) // if window focus button is RMB
				{
					FocusWindow();
					menu = false;
				}

				if (moveButton == 2 && winMoving)
				{
					DropWindow();
					winMoving = false;
					desktop.tooltips = tempTool;
					InvalidateRect(hwndBBPager, NULL, true);

					menu = false;
				}

				if (desktopChangeButton == 2 && !winMoving) // if desktop switch button is RMB
				{
					if (desktopPressed != currentDesktop)
					{
						DeskSwitch();							
						//return 0;
					}

					menu = false;
				}
				
				// if desktop switch button is not RMB, just show menu
				if (menu)
					DisplayMenu();

				rightButtonDown = false;
			}

			return 0;
		}
		break;

		// ===================

		case WM_MOUSEMOVE:
		case WM_NCMOUSEMOVE:
		{				
			if (position.hidden && position.autohide)
			{
				position.hidden = false;
				InvalidateRect(hwndBBPager, NULL, true);
				//Sleep(250);
				position.x = position.ox;
				position.y = position.oy;
				MoveWindow(hwndBBPager, position.x, position.y, frame.width, frame.height, true);
				//MoveWindow(hwndBBPager, position.ox, position.oy, frame.width, frame.height, true);
				InvalidateRect(hwndBBPager, NULL, true);
			}

			if (winMoving)
			{
				int nx, ny, dx, dy;
				RECT r;
				POINT mousepos;
				GetCursorPos(&mousepos);
				GetWindowRect(hwndBBPager, &r);

				//mousepos.x -= position.x;
				//mousepos.y -= position.y;
				mousepos.x -= r.left;
				mousepos.y -= r.top;

				nx = mousepos.x - moveWin.r.left;
				ny = mousepos.y - moveWin.r.top;

				dx = nx - mx;
				dy = ny - my;

				moveWin.r.left += dx;
				moveWin.r.right += dx;
				moveWin.r.top += dy;
				moveWin.r.bottom += dy;

				InvalidateRect(hwndBBPager, NULL, true);
			}
			else
				lastActive = GetForegroundWindow();

			TrackMouse();

			return 0;
		}
		break;

		// ===================
		// reset pressed mouse button statuses if mouse leaves the BBPager window

		case WM_MOUSELEAVE:
		{
			if (winMoving)
			{
				if (moveButton != 1)
					leftButtonDown = false;
				if (moveButton != 2)
					rightButtonDown = false;
				if (moveButton != 3)
					middleButtonDown = false;
			}
			else
				leftButtonDown = middleButtonDown = rightButtonDown = false;
			
			if (position.autohide && !inSlit)
				HidePager();

			if (CursorOutside() && winMoving)
			{
				winMoving = leftButtonDown = middleButtonDown = rightButtonDown = false;
				InvalidateRect(hwndBBPager, NULL, true);
			}

			return 0;
		}
		break;

		case WM_NCMOUSELEAVE:
		{
			rightButtonDownNC = false;

			if (position.autohide && !inSlit)
				HidePager();

			if (CursorOutside() && winMoving)
			{
				winMoving = leftButtonDown = middleButtonDown = rightButtonDown = false;
				InvalidateRect(hwndBBPager, NULL, true);
			}

			return 0;
		}
		break;

		//====================
		// Mouse wheel desktop changing:

		case WM_MOUSEWHEEL:
		{
			int delta = (int)wParam;

			if (delta < 0)
			{
				SendMessage(hwndBlackbox, BB_WORKSPACE, 1, 0);
				SendMessage(hwndBlackbox, BB_SETTOOLBARLABEL, 0, (LPARAM)"BBPager -> Right Workspace");
			}
			else
			{
				SendMessage(hwndBlackbox, BB_WORKSPACE, 0, 0);
				SendMessage(hwndBlackbox, BB_SETTOOLBARLABEL, 0, (LPARAM)"BBPager -> Left Workspace");
			}

			SetForegroundWindow(hwndBBPager);

			return 0;
		}
		break;

		//====================
		// Timer message:

		case WM_TIMER:
		{
			if (CursorOutside() && winMoving)
			{
				winMoving = false;
				desktop.tooltips = tempTool;
			}

			InvalidateRect(hwndBBPager, NULL, true);

			return 0;
		}

		// ===================

		case WM_KEYDOWN:
		{
			if (wParam == VK_ESCAPE && winMoving)
			{
				winMoving = false;
				desktop.tooltips = tempTool;
				SendMessage(hwndBlackbox, BB_SETTOOLBARLABEL, 0, (LPARAM)"BBPager -> Cancel Move Window");
			}
			
			InvalidateRect(hwndBBPager, NULL, true);			

			return 0;
		}
		break;

		case WM_KEYUP:
		{
			if (wParam == VK_NUMPAD7) 
				SendMessage(hwndBBPager, BB_BROADCAST, 0, (LPARAM)"@BBPagerPosition TopLeft");
			else if (wParam == VK_NUMPAD8) 
				SendMessage(hwndBBPager, BB_BROADCAST, 0, (LPARAM)"@BBPagerPosition TopCenter");
			else if (wParam == VK_NUMPAD9) 
				SendMessage(hwndBBPager, BB_BROADCAST, 0, (LPARAM)"@BBPagerPosition TopRight");
			else if (wParam == VK_NUMPAD4) 
				SendMessage(hwndBBPager, BB_BROADCAST, 0, (LPARAM)"@BBPagerPosition CenterLeft");
			else if (wParam == VK_NUMPAD6) 
				SendMessage(hwndBBPager, BB_BROADCAST, 0, (LPARAM)"@BBPagerPosition CenterRight");
			else if (wParam == VK_NUMPAD1) 
				SendMessage(hwndBBPager, BB_BROADCAST, 0, (LPARAM)"@BBPagerPosition BottomLeft");
			else if (wParam == VK_NUMPAD2) 
				SendMessage(hwndBBPager, BB_BROADCAST, 0, (LPARAM)"@BBPagerPosition BottomCenter");
			else if (wParam == VK_NUMPAD3) 
				SendMessage(hwndBBPager, BB_BROADCAST, 0, (LPARAM)"@BBPagerPosition BottomRight");
			else if (wParam == VK_BACK || wParam == VK_RETURN)
				SendMessage(hwndBBPager, BB_BROADCAST, 0, (LPARAM)"@BBPagerInternal ToggleHide");
			else if (wParam == VK_SUBTRACT)
				SendMessage(hwndBBPager, BB_BROADCAST, 0, (LPARAM)"@BBPagerReload");
			//else if (wParam == VK_ADD)
			//	SendMessage(hwndBBPager, BB_BROADCAST, 0, (LPARAM)"@BBPagerSave");
			else if (wParam == VK_ESCAPE && winMoving)
			{
				winMoving = false;
				desktop.tooltips = tempTool;
				SendMessage(hwndBlackbox, BB_SETTOOLBARLABEL, 0, (LPARAM)"BBPager -> Cancel Move Window");
			}
			
			InvalidateRect(hwndBBPager, NULL, true);			

			return 0;
		}
		break;

		case WM_CLOSE:
			return 0;
		break;

		default:
			return DefWindowProc(hwnd,message,wParam,lParam);
		break;
	}
	return 0;
}

//===========================================================================

void ToggleSlit()
{
	static char msg[MAX_LINE_LENGTH];
	static char status[9];

	if (!hSlit) hSlit = FindWindow("BBSlit", "");

	if (inSlit)
	{
		// We are in the slit, so lets unload and get out..
		if (position.snapWindowOld) position.snapWindow = true;
		if (position.autohideOld) position.autohide = true;
		//SetParent(hwndBBPager, NULL);
		//SetWindowLong(hwndBBPager, GWL_STYLE, (GetWindowLong(hwndBBPager, GWL_STYLE) & ~WS_CHILD) | WS_POPUP);
		SendMessage(hSlit, SLIT_REMOVE, NULL, (LPARAM)hwndBBPager);
		inSlit = false;

		//turn trans back on?
		if (transparency && usingWin2kXP)
		{
			SetWindowLong(hwndBBPager, GWL_EXSTYLE, WS_EX_TOOLWINDOW | WS_EX_LAYERED);
			//BBSetLayeredWindowAttributes(hwndBBPager, NULL, (unsigned char)transparencyAlpha, LWA_ALPHA);
			SetTransparency(hwndBBPager, (unsigned char)transparencyAlpha);
		}

		position.x = xpos; position.y = ypos;

		// Here you can move to where ever you want ;)				
		if (position.raised) 
			SetWindowPos(hwndBBPager, HWND_TOPMOST, xpos, ypos, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOMOVE);
		else
			SetWindowPos(hwndBBPager, NULL, xpos, ypos, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
	}
	/* Make sure before you try and load into the slit that you have
	 * the HWND of the slit ;)
	 */
	else if (hSlit)
	{
		// (Back) into the slit..
		if (position.snapWindow) position.snapWindowOld = true;
		else position.snapWindowOld = false;
		position.snapWindow = false;
		if (position.autohide) position.autohideOld = true;
		else position.autohideOld = false;
		position.autohide = false;
		xpos = position.x; ypos = position.y;

		//turn trans off
		SetWindowLong(hwndBBPager, GWL_EXSTYLE, WS_EX_TOOLWINDOW);

		//SetParent(hwndBBPager, hSlit);
		//SetWindowLong(hwndBBPager, GWL_STYLE, (GetWindowLong(hwndBBPager, GWL_STYLE) & ~WS_POPUP) | WS_CHILD);
		SendMessage(hSlit, SLIT_ADD, NULL, (LPARAM)hwndBBPager);
		inSlit = true;
	}

	if (hSlit)
	{
		(inSlit) ? strcpy(status, "enabled") : strcpy(status, "disabled");
		sprintf(msg, "BBPager -> Use Slit %s", status);
		SendMessage(hwndBlackbox, BB_SETTOOLBARLABEL, 0, (LPARAM)msg);
		WriteBool(rcpath, "bbpager.useSlit:", inSlit);
	}
}

//===========================================================================
// Grabs handles to windows, checks them, and saves their info in an array of structs

BOOL CALLBACK CheckTaskEnumProc(HWND window, LPARAM lParam)
{
	int d, height, width, offsetX;
	RECT desk;
	winStruct winListTemp;

	// Only allow 128 windows, just for speed reasons...
	if (winCount > 128)
		return 0;

	// Make sure the window meets criteria to be drawn
	// (GetWindowLong(window, GWL_USERDATA) != magicDWord) &&
	// above not needed - checked with IsAppWindow
	// IsAppWindow(window)
	//((GetWindowLong(window, GWL_USERDATA) != magicDWord) && !IsIconic(window) && IsWindowVisible(window))
	if (IsValidWindow(window))		
	{
		// Get the virtual workspace the window is located.
		d = getDesktop(window);

		if (d < 0 || d >= desktops)	// This stops a crash trying to draw windows that don't exist on a desktop
			return 1;				// such as when bb crashes with windows on numerous workspaces

		// ... and save this information
		winListTemp.desk = d;

		if (CheckSticky(window))
			winListTemp.sticky = true;
#ifdef _WIN64
		else if (GetWindowLong(window, GWLP_USERDATA) == magicDWord)
#else
		else if (GetWindowLong(window, GWL_USERDATA) == magicDWord)
#endif
			winListTemp.sticky = true;
		else
			winListTemp.sticky = false;

		/*char temptext[32];
		GetWindowText(window, temptext, 32);
		if (IsInString(temptext, "girl")) 
		{
			winListTemp.sticky = true;		
		}*/

		if (position.horizontal)
			desk = desktopRect[d];
		else if (position.vertical)
			desk = desktopRect[d];

		// Check whether the window is currently active/focused
		HWND temp = GetForegroundWindow();
		if (temp == window)
			winListTemp.active = true;
		else
			winListTemp.active = false;

		// Set window handle
		winListTemp.window = window;

		// Get window coordinates/RECT
		GetWindowRect(window, &winListTemp.r);

		// offset in desktops away from current desktop
		offsetX = currentDesktop - winListTemp.desk;

		// Height and width of window as displayed in the pager
		height = int( 1 + (winListTemp.r.bottom - winListTemp.r.top) /  ratioY );
		width = int( 1 + (winListTemp.r.right - winListTemp.r.left) /  ratioX );
		
		// Set RECT of window using width/height and top/left values
		winListTemp.r.top = long( desk.top + winListTemp.r.top / ratioY );

		//winList[winCount].r.bottom = long( winList[winCount].r.top + (winList[winCount].r.bottom - winList[winCount].r.top) /  ratioY );
		winListTemp.r.bottom = winListTemp.r.top + height;

		while (winListTemp.r.left < -10 + vScreenLeft) 
		{
			winListTemp.r.left += vScreenWidth + 10;
		}

		while (winListTemp.r.left >= vScreenWidth + vScreenLeft)
		{
			winListTemp.r.left -= vScreenWidth + 10;
		}

		winListTemp.r.left -= vScreenLeft;

		winListTemp.r.left = long( desk.left + (winListTemp.r.left / ratioX) );

		//winList[winCount].r.right = long( winList[winCount].r.left + (winList[winCount].r.right - winList[winCount].r.left) /  ratioX );
		winListTemp.r.right = winListTemp.r.left + width;

		winList.push_back(winListTemp);
		//winList[winCount] = winListTemp;
		
		// Increase number of windows by one if everything successful
		winCount++;
	}

	UNREFERENCED_PARAMETER( lParam );

	return 1;
}

BOOL CALLBACK CheckTaskEnumProc_AltMethod(HWND window, LPARAM lParam)
{
	tl = BBPager_GetTaskListPtr();

	while (tl)
	{
		if (tl->hwnd == window)
		{
			AddBBWindow(tl);
			return 1;
		}
		tl = tl->next;
	}

	/*tasklist tl2;
	tl2.hwnd = window;

	if (DoWindow(&tl2))
	{
		AddBBWindow(&tl2);
	}*/

	return 1;
}

bool AddBBWindow(tasklist *tl)
{
	int height, width, bbdesk;
	RECT desk;
	winStruct winListTemp;
	HWND window;

	window = tl->hwnd;

	// Only allow 128 windows, just for speed reasons...
	if (winCount > 128)
		return 0;

	// Make sure the window meets criteria to be drawn
	if (1)//IsValidWindow(window))		
	{
		// Get the virtual workspace the window is located.
		bbdesk = tl->wkspc;

		if (bbdesk < 0 || bbdesk >= desktops)	// This stops a crash trying to draw windows that don't exist on a desktop
			return 1;							// such as when bb crashes with windows on numerous workspaces

		// ... and save this information
		winListTemp.desk = bbdesk;

		desk = desktopRect[bbdesk];
#ifdef _WIN64
		if (CheckSticky(window) || GetWindowLong(window, GWLP_USERDATA) == magicDWord)
#else
		if (CheckSticky(window) || GetWindowLong(window, GWL_USERDATA) == magicDWord)
#endif
			winListTemp.sticky = true;
		else
			winListTemp.sticky = false;

		winListTemp.active = tl->active;

		// Set window handle
		winListTemp.window = window;

		// Get window coordinates/RECT
		GetWindowRect(window, &winListTemp.r);

		// Height and width of window as displayed in the pager
		height = int( 1 + (winListTemp.r.bottom - winListTemp.r.top) /  ratioY );
		width = int( 1 + (winListTemp.r.right - winListTemp.r.left) /  ratioX );
		
		// Set RECT of window using width/height and top/left values
		winListTemp.r.top = long( desk.top + winListTemp.r.top / ratioY );

		winListTemp.r.bottom = winListTemp.r.top + height;
		
		winListTemp.r.left = long( desk.left + (winListTemp.r.left / ratioX) );

		winListTemp.r.right = winListTemp.r.left + width;

		//if (winListTemp.r.left < desk.left) winListTemp.r.left = desk.left;

		//if (winListTemp.r.right > desk.right) winListTemp.r.right = desk.right;

		winList.push_back(winListTemp);
		
		// Increase number of windows by one if everything successful
		winCount++;
	}

	return 1;
}

//===========================================================================
/*
bool DoWindow(tasklist *tl)
{
	HWND hWnd = tl->hwnd;

	if (GetForegroundWindow() == hWnd || GetActiveWindow() == hWnd)
		tl->active = true;
	else
		tl->active = false;

	// sticky window checks
	if (CheckSticky(hWnd))
		return false;

	if (GetWindowLong(hWnd, GWL_USERDATA) == magicDWord)
		return false;

	// if window is minimised, fail it
	// (no point in displaying something not there =)
	if (IsIconic(hWnd))
		return false;
	
	// *** below is commented out because trillian is a toolwindow
	// *** and perhaps miranda (and other systray things?)
	// if the window is a WS_EX_TOOLWINDOW fail it
	LONG nStyle;
	nStyle = GetWindowLong(hWnd, GWL_EXSTYLE);
	if ((nStyle & WS_EX_TOOLWINDOW) && (GetWindowLong(hWnd, GWL_STYLE) & WS_POPUP))
		return false;

	// This fails the check if its a tooltip or window menu
//	nStyle = GetWindowLong(hWnd, GWL_STYLE);
//	if(nStyle & WS_POPUP)
//		bResult = false;
	
	// *** Below is commented out cause it stops delphi apps working
	// If this has an owner, then only accept this window
	// if the partent is not accepted
//	HWND hOwner;
//	hOwner = GetWindow(hWnd, GW_OWNER);
//	if(hOwner && bResult)
//		bResult = !IsAppWindow(hOwner);
	

	// if it has a parent, fail
	if (GetParent(hWnd))
		return false;

	// if it is a WS_CHILD or not WS_VISIBLE, fail it
	nStyle = GetWindowLong(hWnd, GWL_STYLE);	

	if (nStyle & WS_CHILD)
		return false;

	if (!(nStyle & WS_VISIBLE))
	{
		HWND hOwner;
		hOwner = GetWindow(hWnd, GW_OWNER);
		if (!hOwner)
			return false;

		struct tasklist *tlCheck;
		tlCheck = BBPager_GetTaskListPtr();
		while (tlCheck)
		{
			if (tlCheck->hwnd == hOwner)
			{
				tl->wkspc = tlCheck->wkspc;
				return true;
			}

			tlCheck = tlCheck->next;
		}
		return false;
	}
	else
	{
		tl->wkspc = currentDesktop;
		return true;
	}
	
	return true;
}
*/

bool IsValidWindow(HWND hWnd)
{
	// sticky window checks
	/*if (CheckSticky(hWnd))
		return false;

	if (GetWindowLong(hWnd, GWL_USERDATA) == magicDWord)
		return false;*/

	// if window is minimised, fail it
	// (no point in displaying something not there =)
	if (IsIconic(hWnd))
		return false;

	// if it is a WS_CHILD or not WS_VISIBLE, fail it
	LONG nStyle;
	nStyle = GetWindowLong(hWnd, GWL_STYLE);	
	if ((nStyle & WS_CHILD) || !(nStyle & WS_VISIBLE))
		return false;
	
	// *** below is commented out because trillian is a toolwindow
	// *** and perhaps miranda (and other systray things?)
	// if the window is a WS_EX_TOOLWINDOW fail it
	nStyle = GetWindowLong(hWnd, GWL_EXSTYLE);
	if ((nStyle & WS_EX_TOOLWINDOW) && (GetWindowLong(hWnd, GWL_STYLE) & WS_POPUP))
		return false;

	// This fails the check if its a tooltip or window menu
/*	nStyle = GetWindowLong(hWnd, GWL_STYLE);
	if(nStyle & WS_POPUP)
		bResult = false;
*/	
	// *** Below is commented out cause it stops delphi apps working
	// If this has an owner, then only accept this window
	// if the partent is not accepted
//	HWND hOwner;
/*	hOwner = GetWindow(hWnd, GW_OWNER);
	if(hOwner && bResult)
		bResult = !IsAppWindow(hOwner);
*/	
	// if it has a parent, fail
	if (GetParent(hWnd))
		return false;
	
	return true;
}

//===========================================================================

int getDesktop(HWND h)
{
	if (desktops < 1)
		return -1;

	RECT r;
	GetWindowRect(h, &r);

	/*int offsetx = currentDesktop % desktops;
	int offsety = currentDesktop / desktops;
	int desk = 0;
	
	/*r.left += offsetx * (screenWidth + 10);
	r.top += offsety * (screenHeight + 10);
	
	offsetx = ((r.left + 10) / (screenWidth + 10));
	offsety = ((r.top + 10) / (screenHeight + 10));*/

	/*r.left += offsetx * (screenWidth + 10) + screenLeft;
	r.top += offsety * (screenHeight + 10) + screenTop;
	
	offsetx = ((r.left + 10) / (screenWidth + 10 + screenLeft));
	offsety = ((r.top + 10) / (screenHeight + 10 + screenTop));
	
	desk = (offsety * desktops) + offsetx;*/

	int desktopWidth = vScreenWidth + 10;
	int offset = 0;
	int winLeft = r.left;

	while (winLeft < -10 + vScreenLeft) 
	{
		winLeft += desktopWidth;
		offset--;
	}

	while (winLeft >= (vScreenWidth + vScreenLeft)) 
	{
		winLeft -= desktopWidth;
		offset++;
	}

	//*pLeft = l;
	return offset + currentDesktop;
	
	//if (desk < 0 || desk > desktops) desk = 0;
	//return desk;
}

//===========================================================================
// The actual building of the menu

void DisplayMenu()
{
	// First we delete the main plugin menu if it exists (PLEASE NOTE that this takes care of submenus as well!)
	if (BBPagerMenu) DelMenu(BBPagerMenu);

	// Then we create the main plugin menu with a few commands...
	BBPagerMenu = MakeMenu("BBPager");

		// Window Submenu
		BBPagerWindowSubMenu = MakeMenu("Window");
			if (!inSlit)
			{
				MakeMenuItem(BBPagerWindowSubMenu, "Always On Top", "@BBPagerInternal ToggleRaised", position.raised);
				MakeMenuItem(BBPagerWindowSubMenu, "Autohide", "@BBPagerInternal ToggleHide", position.autohide);
				MakeMenuItem(BBPagerWindowSubMenu, "Snap Window To Edge", "@BBPagerInternal ToggleSnap", position.snapWindow);
				if (usingWin2kXP)
				{
					MakeMenuItem(BBPagerWindowSubMenu, "Transparency", "@BBPagerInternal ToggleTrans", transparency);
				}
			}
			MakeMenuItem(BBPagerWindowSubMenu, "Draw Border", "@BBPagerInternal ToggleBorder", drawBorder);
			if (!hSlit) hSlit = FindWindow("BBSlit", "");
			if (hSlit) MakeMenuItem(BBPagerWindowSubMenu, "Use Slit", "@BBPagerInternal ToggleSlit", inSlit);
		MakeSubmenu(BBPagerMenu, BBPagerWindowSubMenu, "Window");

		// Display Submenu
		BBPagerDisplaySubMenu = MakeMenu("Display");
			MakeMenuItem(BBPagerDisplaySubMenu, "Desktop Numbers", "@BBPagerInternal ToggleNumbers", desktop.numbers);
			MakeMenuItem(BBPagerDisplaySubMenu, "Desktop Windows", "@BBPagerInternal ToggleWindows", desktop.windows);
			MakeMenuItem(BBPagerDisplaySubMenu, "Window ToolTips", "@BBPagerInternal ToggleToolTips", desktop.tooltips);
			if (usingBBLean) 
				MakeMenuItem(BBPagerDisplaySubMenu, "Use Alt. Method", "@BBPagerInternal ToggleAltMethod", usingAltMethod);
			//MakeMenuItem(BBPagerDisplaySubMenu, "Debugging", "@BBPagerInternal ToggleDebug", debug);
		MakeSubmenu(BBPagerMenu, BBPagerDisplaySubMenu, "Display");

		// Alignment Submenu
		BBPagerAlignSubMenu = MakeMenu("Alignment");
			MakeMenuItem(BBPagerAlignSubMenu, "Horizontal", "@BBPagerInternal Horizontal", position.horizontal);
			MakeMenuItem(BBPagerAlignSubMenu, "Vertical", "@BBPagerInternal Vertical", position.vertical);
		MakeSubmenu(BBPagerMenu, BBPagerAlignSubMenu, "Alignment");		
		
		// Position Submenu
		if (!inSlit)
		{
			BBPagerPositionSubMenu = MakeMenu("Placement");
				MakeMenuItem(BBPagerPositionSubMenu, "Top Left", "@BBPagerPosition TopLeft", (position.side == 3));
				MakeMenuItem(BBPagerPositionSubMenu, "Top Center", "@BBPagerPosition TopCenter", (position.side == 2));
				MakeMenuItem(BBPagerPositionSubMenu, "Top Right", "@BBPagerPosition TopRight", (position.side == 6));
				MakeMenuItem(BBPagerPositionSubMenu, "Center Left", "@BBPagerPosition CenterLeft", (position.side == 1));
				MakeMenuItem(BBPagerPositionSubMenu, "Center Right", "@BBPagerPosition CenterRight", (position.side == 4));
				MakeMenuItem(BBPagerPositionSubMenu, "Bottom Left", "@BBPagerPosition BottomLeft", (position.side == 9));
				MakeMenuItem(BBPagerPositionSubMenu, "Bottom Center", "@BBPagerPosition BottomCenter", (position.side == 8));
				MakeMenuItem(BBPagerPositionSubMenu, "Bottom Right", "@BBPagerPosition BottomRight", (position.side == 12));
			MakeSubmenu(BBPagerMenu, BBPagerPositionSubMenu, "Placement");		
		}

		// Settings Submenu
		BBPagerSettingsSubMenu = MakeMenu("Settings");
			MakeMenuItem(BBPagerSettingsSubMenu, "Reload Settings", "@BBPagerReload", false);
			//MakeMenuItem(BBPagerSettingsSubMenu, "Save Settings", "@BBPagerSave", false);
			MakeMenuItem(BBPagerSettingsSubMenu, "Edit Settings", "@BBPagerInternal OpenRC", false);
			MakeMenuItem(BBPagerSettingsSubMenu, "Edit Style", "@BBPagerInternal OpenStyle", false);
		MakeSubmenu(BBPagerMenu, BBPagerSettingsSubMenu, "Settings");

		
		if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
		{
			BBPagerDebugSubMenu = MakeMenu("Debug");
				MakeMenuItem(BBPagerDebugSubMenu, "List Windows", "@BBPagerInternal ListWindows", false);
				MakeMenuItem(BBPagerDebugSubMenu, "List Desktops", "@BBPagerInternal ListDesktops", false);
				MakeMenuItem(BBPagerDebugSubMenu, "List Stuff", "@BBPagerInternal ListStuff", false);
			MakeSubmenu(BBPagerMenu, BBPagerDebugSubMenu, "Debug");
		}

	MakeMenuItem(BBPagerMenu, "About BBPager...", "@BBPagerInternal About", false);
	
	// Finally, we show the menu...
	ShowMenu(BBPagerMenu);
}

//===========================================================================
// Checks to see if the cursor is inside the pager window

bool CursorOutside()
{
	RECT r;
	POINT pt;
	GetCursorPos(&pt);
	GetWindowRect(hwndBBPager, &r);

	//RECT r = {position.x, position.y, position.x + frame.width, position.y + frame.height};

	if (PtInRect(&r, pt))
		return false;
	else
		return true;
}

//===========================================================================
// This checks the coords of the click to set the appropriate desktop to pressed...

void ClickMouse()
{
	RECT r;
	int inDesk = 0;
	POINT mousepos;
	GetCursorPos(&mousepos);
	GetWindowRect(hwndBBPager, &r);
	//mousepos.x = mousepos.x - position.x;
	//mousepos.y = mousepos.y - position.y;
	mousepos.x = mousepos.x - r.left;
	mousepos.y = mousepos.y - r.top;

	desktopPressed = -1;

	for (int i = 0; i < desktops; i++) 
	{
		inDesk = PtInRect(&desktopRect[i], mousepos); // check if mouse cursor is within a desktop RECT
		if (inDesk != 0)
		{
			desktopPressed = i; // if so, set desktop pressed
			return;
		}
	}
}

// This checks the pressed desktop and switches to that, as well as displaying a nice msg :D
void DeskSwitch()
{
	if (desktopPressed > -1 && desktopPressed != (currentDesktop)) 
	{
		char desktopmsg[MAX_LINE_LENGTH];
		sprintf(desktopmsg, "BBPager -> Change to Workspace %d (%s)", (desktopPressed + 1), desktopName[desktopPressed].c_str());
		SendMessage(hwndBlackbox, BB_SETTOOLBARLABEL, 0, (LPARAM)desktopmsg);
		SendMessage(hwndBlackbox, BB_WORKSPACE, 4, (LPARAM)desktopPressed);
	}

	desktopPressed = -1;
}

//===========================================================================
// Focuses window under cursor

void FocusWindow()
{
	RECT r;
	int inWin = 0;
	POINT mousepos;
	GetCursorPos(&mousepos);
	GetWindowRect(hwndBBPager, &r);
	//mousepos.x = mousepos.x - position.x;
	//mousepos.y = mousepos.y - position.y;
	mousepos.x = mousepos.x - r.left;
	mousepos.y = mousepos.y - r.top;

	winPressed = -1;

	for (int i = 0;i <= (winCount - 1);i++)
	{
		inWin = PtInRect(&winList[i].r, mousepos); // check if mouse cursor is within a window RECT
		if (inWin != 0)
		{
			winPressed = i; // if so, set desktop pressed
			HWND hwnd = winList[winPressed].window;

			char desktopmsg[MAX_LINE_LENGTH], temp[40];
			GetWindowText(hwnd, temp, 32);
			strcat(temp, "...");
			sprintf(desktopmsg, "BBPager -> Focus Window: ");
			strcat(desktopmsg, temp);
			SendMessage(hwndBlackbox, BB_SETTOOLBARLABEL, 0, (LPARAM)desktopmsg);

			//int winDesk = getDesktop(hwnd);
			//int winDesk = winList[winPressed].desk;
			
			/*if (winDesk != currentDesktop)
				SendMessage(hwndBlackbox, BB_WORKSPACE, 4, (LPARAM)winDesk);

			SetForegroundWindow(hwnd);*/

			//ShowWindow(winList[winPressed].window, SW_SHOW);
			//BringWindowToTop(winList[winPressed].window);
			//SetWindowPos(winList[winPressed].window, HWND_TOP, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
			SendMessage(hwndBlackbox, BB_BRINGTOFRONT, 0, (LPARAM)winList[winPressed].window);

			InvalidateRect(hwndBBPager, NULL, true);
			return;
		}
	}

	return;
}

//===========================================================================
// Picks up window under cursor and drops it wherever 
// (according to top left corner of window)

void GrabWindow()
{
	if (!desktop.windows)
		return;

	//lastActive = GetActiveWindow();

	RECT r;
	int inWin = 0;
	POINT mousepos;

	GetCursorPos(&mousepos);
	GetWindowRect(hwndBBPager, &r);
	//mousepos.x -= position.x;
	//mousepos.y -= position.y;
	mousepos.x -= r.left;
	mousepos.y -= r.top;

	for (int i = (winCount - 1);i > -1;i--)
	{
		inWin = PtInRect(&winList[i].r, mousepos); // check if mouse cursor is within a window RECT
		if (inWin != 0)
		{
			moveWin = winList[i];
			//grabDesktop = getDesktop(moveWin.window);
			grabDesktop = moveWin.desk;
			oldx = moveWin.r.left;
			oldy = moveWin.r.top;
			mx = mousepos.x - moveWin.r.left;
			my = mousepos.y - moveWin.r.top;
			winMoving = true;
			tempTool = desktop.tooltips;
			desktop.tooltips = false;
			//MessageBox(0,"Hello","BOOYAH",MB_OK);
		}
	}
}

void DropWindow()
{
	if (!desktop.windows)
		return;

	int inDesk = 0, newx, newy;
	POINT pos;

	newx = moveWin.r.left;
	newy = moveWin.r.top;

	if (newx == oldx && newy == oldy)
	{
		SetActiveWindow(lastActive);
		SetForegroundWindow(lastActive);
		return;
	}

	pos.x = moveWin.r.left + ((moveWin.r.right - moveWin.r.left) / 2);
	pos.y = moveWin.r.top + ((moveWin.r.bottom - moveWin.r.top) / 2);

	int dropDesktop = -1;

	for (int i = 0; i < desktops; i++) 
	{
		inDesk = PtInRect(&desktopRect[i], pos); // check if middle of moving window is within a desktop RECT
		if (inDesk != 0)
		{
			dropDesktop = i; // if so, set desktop pressed

			/*if (dropDesktop == grabDesktop)
			{
				SetActiveWindow(lastActive);
				SetForegroundWindow(lastActive);
				return;
			}*/

			if (usingBBLean)
			{
				taskinfo ti;
				int stl_flags;

				ti.desk = dropDesktop;
				//ti.width = ti.height = 0;
				RECT wRect; GetWindowRect(moveWin.window, &wRect);
				ti.width = wRect.right - wRect.left;
				ti.height = wRect.bottom - wRect.top;

				if (!IsZoomed(moveWin.window))
				{
					int offX = moveWin.r.left - desktopRect[dropDesktop].left;
					int offY = moveWin.r.top - desktopRect[dropDesktop].top;

					int movX = int(offX * ratioX);
					int movY = int(offY * ratioY);

					if (movX < 0) movX = 0;
					if (movY < 0) movY = 0;
					if (movX + ti.width > vScreenWidth) movX = vScreenWidth - ti.width;
					if (movY + ti.height > vScreenHeight) movY = vScreenHeight - ti.height;

					ti.xpos = movX;
					ti.ypos = movY;

					stl_flags = BBTI_SETDESK | BBTI_SETPOS;
				}
				else
				{
					ti.xpos = ti.ypos = 0;

					stl_flags = BBTI_SETDESK;
				}			

				if (moveWin.active || dropDesktop == currentDesktop)
					stl_flags |= BBTI_SWITCHTO;

				//MessageBox(0, "CHECK", "CHECK", MB_OK);

				BBPager_SetTaskLocation(moveWin.window, &ti, stl_flags);

				//MessageBox(0, "CHECK2", "CHECK2", MB_OK);
			}
			else
			{
				int diff = dropDesktop - currentDesktop;
				int offX = moveWin.r.left - desktopRect[dropDesktop].left;
				int offY = moveWin.r.top - desktopRect[dropDesktop].top;

				int movX = int((diff * (vScreenWidth + 10) + vScreenLeft) + (offX * ratioX));
				int movY = int((offY * ratioX));

				if (IsZoomed(moveWin.window))
				{
					if (dropDesktop == grabDesktop)
						return;
					if (moveWin.active)
						SetWindowPos(moveWin.window, HWND_TOP, (diff * (vScreenWidth + 10) - 4 + vScreenLeft) + leftMargin, -4 + topMargin, 300, 300, SWP_NOSIZE);
					else
						SetWindowPos(moveWin.window, HWND_TOP, (diff * (vScreenWidth + 10) - 4) + leftMargin, -4 + topMargin, 300, 300, SWP_NOSIZE | SWP_NOACTIVATE);
				}
				else
				{
					if (moveWin.active)
						SetWindowPos(moveWin.window, HWND_TOP, movX, movY, 300, 300, SWP_NOSIZE);
					else
						SetWindowPos(moveWin.window, HWND_TOP, movX, movY, 300, 300, SWP_NOSIZE | SWP_NOACTIVATE);
				}

				SetTaskWorkspace(moveWin.window, dropDesktop);

				if (currentDesktop != dropDesktop && moveWin.active)
					SendMessage(hwndBlackbox, BB_WORKSPACE, 4, (LPARAM)dropDesktop);
			}

			char desktopmsg[MAX_LINE_LENGTH], temp[40];
			GetWindowText(moveWin.window, temp, 32);
			strcat(temp, "...");
			if (currentDesktop != dropDesktop)
				sprintf(desktopmsg, "BBPager -> Move Window: %s to Workspace %d (%s)", temp, (dropDesktop + 1), desktopName[dropDesktop].c_str());
			else
				sprintf(desktopmsg, "BBPager -> Move Window: %s", temp);
			SendMessage(hwndBlackbox, BB_SETTOOLBARLABEL, 0, (LPARAM)desktopmsg);

			if (!GetWindowLong(moveWin.window, WS_VISIBLE) && dropDesktop == currentDesktop)
				ShowWindow(moveWin.window, SW_SHOW);

			if (moveWin.active || dropDesktop == currentDesktop)
			{
				SetActiveWindow(moveWin.window);
				SetForegroundWindow(moveWin.window);
			}
			else
			{
				SetActiveWindow(lastActive);
				SetForegroundWindow(lastActive);
			}

			return;
		}
	}

	SetActiveWindow(lastActive);
	SetForegroundWindow(lastActive);
	return;
}

//===========================================================================

void TrackMouse()
{
	TRACKMOUSEEVENT tme0;
	tme0.cbSize = sizeof(TRACKMOUSEEVENT);
	tme0.dwFlags = TME_LEAVE;
	tme0.hwndTrack = hwndBBPager;
	tme0.dwHoverTime = 0;
	TrackMouseEvent(&tme0);

	TRACKMOUSEEVENT tme1;
	tme1.cbSize = sizeof(TRACKMOUSEEVENT);
	tme1.dwFlags = TME_LEAVE | TME_NONCLIENT;
	tme1.hwndTrack = hwndBBPager;
	tme1.dwHoverTime = 0;
	TrackMouseEvent(&tme1);
}

void HidePager()
{
	if (!inSlit)
	{
		if (!position.hidden)
		{
			if (CursorOutside())
			{
				InvalidateRect(hwndBBPager, NULL, true);
				GetPos(true);
				//Sleep(250);
				//MessageBox(0, "Left NON client area", "WM_MOUSELEAVES", MB_OK | MB_TOPMOST);
				position.x = position.hx;
				position.y = position.hy;
				MoveWindow(hwndBBPager, position.x, position.y, frame.width, frame.height, true);
				//MoveWindow(hwndBBPager, position.hx, position.hy, frame.width, frame.height, true);
				InvalidateRect(hwndBBPager, NULL, true);
				position.hidden = true;
			}		
		}
	}
}

void SetPos(int place)
{
	//UpdateMonitorInfo();
	switch (place)
	{
		case 3:
		{
			position.x = position.ox = screenLeft;
			position.y = position.oy = screenTop;

			if (position.vertical)
			{
				position.hx = frame.hideWidth - frame.width + screenLeft;
				position.hy = position.oy;
			}
			else
			{
				position.hy = frame.hideWidth - frame.height + screenTop;
				position.hx = position.ox;
			}
			return;
		}
		break;

		case 6:
		{
			position.x = position.ox = screenWidth - frame.width + screenLeft;
			position.y = position.oy = screenTop;

			if (position.vertical)
			{
				position.hx = screenWidth - frame.hideWidth + screenLeft;
				position.hy = position.oy;
			}
			else
			{
				position.hy = frame.hideWidth - frame.height + screenTop;
				position.hx = position.ox;
			}
			return;
		}
		break;

		case 9:
		{
			position.y = position.oy = screenHeight - frame.height + screenTop;
			position.x = position.ox = screenLeft;

			if (position.vertical)
			{
				position.hx = frame.hideWidth - frame.width + screenLeft;
				position.hy = position.oy;
			}
			else
			{
				position.hy = screenHeight - frame.hideWidth + screenTop;
				position.hx = position.ox;
			}
			return;
		}
		break;

		case 12:
		{		
			position.y = position.oy = screenHeight - frame.height + screenTop;
			position.x = position.ox = screenWidth - frame.width + screenLeft;

			if (position.vertical)
			{
				position.hx = screenWidth - frame.hideWidth + screenLeft;
				position.hy = position.oy;
			}
			else
			{
				position.hy = screenHeight - frame.hideWidth + screenTop;
				position.hx = position.ox;
			}
			return;	
		}
		break;

		case 1:
		{
			position.x = position.ox = screenLeft;
			position.y = position.oy = (screenHeight / 2) - (frame.height / 2) + screenTop;

			position.hx = frame.hideWidth - frame.width + screenLeft;
			position.hy = position.oy;
			return;
		}
		break;

		case 2:
		{
			position.y = position.oy = screenTop;
			position.x = position.ox = (screenWidth / 2) - (frame.width / 2) + screenLeft;

			position.hy = frame.hideWidth - frame.height + screenTop;
			position.hx = position.ox;
			return;
		}
		break;

		case 4:
		{
			position.x = position.ox = screenWidth - frame.width + screenLeft;
			position.y = position.oy = (screenHeight / 2) - (frame.height / 2) + screenTop;

			position.hx = screenWidth - frame.hideWidth + screenLeft;
			position.hy = position.oy;
			return;
		}
		break;
	
		case 8:
		{
			position.y = position.oy = screenHeight - frame.height + screenTop;
			position.x = position.ox = (screenWidth / 2) - (frame.width / 2) + screenLeft;

			position.hy = screenHeight - frame.hideWidth + screenTop;
			position.hx = position.ox;
			return;
		}
		break;

		default:
		{
/*			position.hy = position.oy = position.y;
			position.hx = position.ox = position.x;
			position.hidden = false;
			position.autohide = false;*/
			return;
		}
		break;
	}

	//MoveWindow(hwndBBPager, position.x, position.y, frame.width, frame.height, true);
}

void GetPos(bool snap)
{
	if (!position.hidden)
	{
		position.ox = position.x;
		position.oy = position.y;

		if (position.ox == screenLeft && position.oy == ((screenHeight / 2) - (frame.height / 2)) + screenTop)
		{
			position.side = 1; //CL
			return;
		}
		if (position.ox == screenLeft && position.oy == screenTop) 
		{
			position.side = 3; //TL
			return;
		}
		if (position.oy == screenTop && position.ox == ((screenWidth / 2) - (frame.width / 2)) + screenLeft)
		{
			position.side = 2; //TC
			return;
		}
		if (position.ox + frame.width == screenRight && position.oy == screenTop)
		{
			position.side = 6; //TR
			return;	
		}
		if (position.ox == screenRight - frame.width && position.oy == ((screenHeight / 2) - (frame.height / 2)) + screenTop)
		{
			position.side = 4; //CR
			return;
		}
		if (position.oy + frame.height == screenBottom && position.ox == screenLeft)
		{
			position.side = 9; //BL
			return;
		}
		if (position.oy + frame.height == screenBottom && position.ox + frame.width == screenRight)
		{
			position.side = 12; //BR
			return;
		}
		if (position.oy + frame.height == screenBottom && position.ox == ((screenWidth / 2) - (frame.width / 2)) + screenLeft)
		{
			position.side = 8; //BC
			return;
		}

		position.side = 0;

		if (snap)
		{
			int left, right, top, bottom;

			left = position.ox;
			top = position.oy;
			right = screenWidth - (position.ox + frame.width) + screenLeft;
			bottom = screenHeight - (position.oy + frame.height) + screenTop;
			
			if (left <=	top && left <= right && left <= bottom)
			{
				position.ox = position.x = screenLeft;
				position.oy = position.y;
				position.hx = frame.hideWidth - frame.width;
				position.hy = position.oy;

				if (position.y == ((screenHeight / 2) - (frame.height / 2)) + screenTop)
					position.side = 1;

				return;
			}
			if (top <= bottom && top <= left && top <= right)
			{
				position.ox = position.x;
				position.oy = position.y = screenTop;
				position.hy = frame.hideWidth - frame.height;
				position.hx = position.ox;

				if (position.x == ((screenWidth / 2) - (frame.width / 2)) + screenLeft)
					position.side = 2;

				return;
			}
			if (right <= top && right <= bottom && right <= left)
			{
				position.ox = position.x = screenWidth - frame.width + screenLeft;
				position.oy = position.y;
				position.hx = screenWidth - frame.hideWidth + screenLeft;
				position.hy = position.oy;
				
				if (position.y == ((screenHeight / 2) - (frame.height / 2)) + screenTop)
					position.side = 4;

				return;
			}
			if (bottom <= top && bottom <= left && bottom <= right)
			{
				position.ox = position.x;
				position.oy = position.y = screenHeight - frame.height + screenTop;
				position.hy = screenHeight - frame.hideWidth + screenTop;
				position.hx = position.ox;	
				
				if (position.x == ((screenWidth / 2) - (frame.width / 2)) + screenLeft)
					position.side = 8;

				return;
			}
			 
			position.hy = position.oy = position.y;
			position.hx = position.ox = position.x;
			position.hidden = false;
			position.autohide = false;
			return;
		}
	}
}

//===========================================================================

void UpdatePosition()
{
	// save old dimensions for slit updating
	heightOld = frame.height;
	widthOld = frame.width;
	posXOld = position.x;
	posYOld = position.y;

	// =======================
	// Size of pager
	
	desktops = 0; // reset number of desktops
	desktopName.clear();
	SendMessage(hwndBlackbox, BB_LISTDESKTOPS, (WPARAM)hwndBBPager, 0);
	// currentDesktop gives current desktop number starting at _0_ !

	// Set window width and height based on number of desktops
	// Takes into account of row/column setting
	if (position.horizontal) // row/horizontal
	{
		frame.width = (unsigned int)((desktops - 1)/frame.rows + 1) *
                (desktop.width + frame.bevelWidth) + 
                 frame.bevelWidth + (2 * frame.borderWidth);

		if (desktops < frame.rows)
			frame.height = (unsigned int)(desktops * (desktop.height + frame.bevelWidth)) + frame.bevelWidth 
							+ (2 * frame.borderWidth);
		else
			frame.height = (unsigned int)(frame.rows * (desktop.height + frame.bevelWidth)) + frame.bevelWidth
							+ (2 * frame.borderWidth);
	}
	else // column/vertical
	{
		if (desktops < frame.columns)
			frame.width = (unsigned int)(desktops * (desktop.width + frame.bevelWidth)) + frame.bevelWidth
							+ (2 * frame.borderWidth);
		else
			frame.width = (unsigned int)(frame.columns * (desktop.width + frame.bevelWidth)) + frame.bevelWidth
							+ (2 * frame.borderWidth);

		frame.height = (unsigned int)(((desktops - 1)/frame.columns) + 1) *
                  (desktop.height + frame.bevelWidth) + 
                  frame.bevelWidth + (2 * frame.borderWidth);
	}

	// ========================
	// Keep in screen area if no autohide :)

	//UpdateMonitorInfo();
	if (!position.autohide)
	{
		if (position.x < vScreenLeft) 
			position.x = vScreenLeft;
		if ((position.x + frame.width) > (vScreenWidth + vScreenLeft))
			position.x = vScreenWidth - frame.width + vScreenLeft;

		if (position.y < vScreenTop) 
			position.y = vScreenTop;
		if ((position.y + frame.height) > (vScreenHeight + vScreenTop))
			position.y = vScreenHeight - frame.height + vScreenTop;
	}
	else
	{
		if (position.hx <= vScreenLeft - frame.width) 
			position.hx = vScreenLeft - frame.width + frame.hideWidth;
		if (position.hx >= (vScreenWidth + vScreenLeft))
			position.hx = vScreenWidth - frame.hideWidth + vScreenLeft;

		if (position.hy <= vScreenTop - frame.height) 
			position.hy = vScreenTop - frame.height + frame.hideWidth;
		if (position.hy >= (vScreenHeight + vScreenTop))
			position.hy = vScreenHeight - frame.hideWidth + vScreenTop;
	}

	//===============
	
	if (!position.hidden)
	{
		position.ox = position.x;
		position.oy = position.y;
	}

	if (position.autohide && !inSlit)
	{
		GetPos(true);
		SetPos(position.side);
		position.hidden = false;
		HidePager();
		return;
	}

	GetPos(false);
	SetPos(position.side);

	// Reset BBPager window's position and dimensions
	if (!inSlit)
	{
		if (heightOld != frame.height || widthOld != frame.width || posXOld != position.x || posYOld != position.y)
			MoveWindow(hwndBBPager, position.x, position.y, frame.width, frame.height, true);
	}
	else if (heightOld != frame.height || widthOld != frame.width)
	{
		SetWindowPos(hwndBBPager, HWND_TOP, 0, 0, frame.width, frame.height, SWP_NOMOVE | SWP_NOZORDER);

		if (hSlit && inSlit)
		{
			SendMessage(hSlit, SLIT_UPDATE, 0, 0);
		}
	}
}

//===========================================================================
//Flashing tasks

void AddFlash(HWND task)
{
	for (int i = 0; i < (int)flashList.size(); i++)
	{
		if (flashList[i].task == task)
		{
			flashList[i].on = !flashList[i].on;
			return;
		}
	}

	flashTask flashTemp;

	flashTemp.task = task;
	flashTemp.on = true;
	flashList.push_back(flashTemp);
}

void RemoveFlash(HWND task, bool quick)
{
	for (int i = 0; i < (int)flashList.size(); i++)
	{
		if (flashList[i].task == task)
		{
			flashList.erase(flashList.begin() + i);
			if (quick)
			{
				//RECT taskRect;
				//if (GetTaskItemRect(task, &taskRect))
					//InvalidateRect(hwndBBPager, &taskRect, false);
				InvalidateRect(hwndBBPager, NULL, false);
			}
			return;
		}
	}
}

bool IsFlashOn(HWND task)
{
	for (int i = 0; i < (int)flashList.size(); i++)
	{
		if (flashList[i].task == task && flashList[i].on)
			return true;
	}
	return false;
}

//===========================================================================
//Tooltips

void *m_alloc(unsigned s) { return malloc(s); }
void *c_alloc(unsigned s) { return calloc(1,s); }
void m_free(void *v)      { free(v); }

struct tt
{
	struct tt *next;
	char used_flg;
	char text[256];
	TOOLINFO ti;
} *tt0;

void SetToolTip(RECT *tipRect, char *tipText)
{
	//if (NULL==hToolTips) return;
	if (NULL==hToolTips || !*tipText) return;

	struct tt **tp, *t; unsigned n=0;
	for (tp=&tt0; NULL!=(t=*tp); tp=&t->next)
	{
		if (0==memcmp(&t->ti.rect, tipRect, sizeof(RECT)))
		{
			t->used_flg = 1;
			if (0!=strcmp(t->ti.lpszText, tipText))
			{
				strcpy(t->text, tipText);
				SendMessage(hToolTips, TTM_UPDATETIPTEXT, 0, (LPARAM)&t->ti);
			}
			return;
		}
		if (t->ti.uId > n)
			n = t->ti.uId;
	}

	t = (struct tt*)c_alloc(sizeof (*t));
	t->used_flg  = 1;
	t->next = NULL;
	strcpy(t->text, tipText);
	*tp = t;

	memset(&t->ti, 0, sizeof(TOOLINFO));

	t->ti.cbSize   = sizeof(TOOLINFO);
	t->ti.uFlags   = TTF_SUBCLASS;
	t->ti.hwnd     = hwndBBPager;
	t->ti.uId      = n+1;
	//t->ti.hinst    = NULL;
	t->ti.lpszText = t->text;
	t->ti.rect     = *tipRect;

	SendMessage(hToolTips, TTM_ADDTOOL, 0, (LPARAM)&t->ti);
}

//===========================================================================
// Function: ClearToolTips
// Purpose:  clear all tooltips, which are not longer used
//===========================================================================

void ClearToolTips(void)
{
	struct tt **tp, *t;
	tp=&tt0; while (NULL!=(t=*tp))
	{
		if (0==t->used_flg)
		{
			SendMessage(hToolTips, TTM_DELTOOL, 0, (LPARAM)&t->ti);
			*tp=t->next;
			m_free(t);
		}
		else
		{
			t->used_flg = 0;
			tp=&t->next;
		}
	}
}

//===========================================================================

bool BBPager_SetTaskLocation(HWND hwnd, struct taskinfo *pti, UINT flags)
{
    //extern bool (__stdcall *BBPager_STL)(HWND, struct taskinfo *, UINT);
	if (NULL == BBPager_STL) 
	{
		MessageBox(0,"BBPager_SetTaskLocation NO","BBPager_SetTaskLocation NO",MB_OK);
		return false;
	}

	//MessageBox(0,"BBPager_SetTaskLocation FETCH","BBPager_SetTaskLocation FETCH",MB_OK);

	return ( 1 == BBPager_STL(hwnd, pti, flags) );
}

tasklist* BBPager_GetTaskListPtr(void)
{
    //extern tasklist* (__stdcall *BBPager_GTLP)(void);
	if (NULL == BBPager_GTLP) 
	{
		MessageBox(0,"BBPager_GetTaskListPtr NO","BBPager_GetTaskListPtr NO",MB_OK);
		return NULL;
	}
	
	return BBPager_GTLP();
}
