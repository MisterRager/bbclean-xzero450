/*
 ============================================================================

  This program is free software, released under the GNU General Public License
  (GPL version 2 or later).

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
  for more details.

  http://www.fsf.org/licenses/gpl.html

 ============================================================================
*/

#include "bbrecyclebin.h"

//#define TTM_SETMAXTIPWIDTH	 (WM_USER+24)
#define StrFormatByte64 StrFormatByte64A

char *plugin_info[] =
{
	"BBRecycleBin 0.0.6",
	"BBRecycleBin",
	"0.0.6",
	"ysuke",
	"2005-12-15",
	"http://zzbb.hp.infoseek.co.jp/",
	"zb2_460@yahoo.co.jp"
};

#define szVersion       plugin_info[0]
#define szAppName       plugin_info[1]
#define szInfoVersion	plugin_info[2]
#define szInfoAuthor	plugin_info[3]
#define szInfoRelDate	plugin_info[4]
#define szInfoLink	plugin_info[5]
#define szInfoEmail	plugin_info[6]

DWORD JustifyStyle[] =
{
	DT_LEFT,
	DT_CENTER,
	DT_RIGHT
};

enum
{
	LEFT,
	CENTER,
	RIGHT
};

BOOL ( WINAPI *pSetLayeredWindowAttributes ) ( HWND, DWORD, BYTE, DWORD );
BOOL ( WINAPI *pUpdateLayeredWindow ) ( HWND, HDC, POINT*, SIZE*, HDC, POINT*, COLORREF, BLENDFUNCTION*, DWORD );

//===========================================================================

int beginSlitPlugin(HINSTANCE hMainInstance, HWND hBBSlit)
{
	inSlit = true;
	hSlit = hBBSlit;

	// Start the plugin like normal now..
	int res = beginPlugin(hMainInstance);

	// Are we to be in the Slit?
	if (inSlit && hSlit){
		// Yes, so Let's let BBSlit know.
		SendMessage(hSlit, SLIT_ADD, 0, (LPARAM)hwndPlugin);
	}

	return res;
}

//===========================================================================

int beginPluginEx(HINSTANCE hMainInstance, HWND hBBSlit)
{
	return beginSlitPlugin(hMainInstance, hBBSlit);
}

//===========================================================================

int beginPlugin(HINSTANCE hPluginInstance) {
	ZeroMemory(&osvinfo, sizeof(osvinfo));
	osvinfo.dwOSVersionInfoSize = sizeof (osvinfo);
	GetVersionEx(&osvinfo);
	usingWin2kXP = (osvinfo.dwPlatformId == VER_PLATFORM_WIN32_NT && osvinfo.dwMajorVersion > 4);
	usingWin2k = (osvinfo.dwMajorVersion == 5 && osvinfo.dwMinorVersion == 0);

	//InitCommonControls();

	if(usingWin2kXP){
	HMODULE hDll = LoadLibrary("user32");
	pSetLayeredWindowAttributes = (BOOL (WINAPI *)(HWND, DWORD, BYTE, DWORD))
			GetProcAddress(hDll, "SetLayeredWindowAttributes");

	pUpdateLayeredWindow = (BOOL (WINAPI *)(HWND, HDC, POINT*, SIZE*, HDC, POINT*, COLORREF, BLENDFUNCTION*, DWORD))
			GetProcAddress(hDll, "UpdateLayeredWindow");
	FreeLibrary(hDll);
        }

	WNDCLASS wc;
	hwndBlackbox = GetBBWnd();
	hInstance = hPluginInstance;

	// Register the window class...
	ZeroMemory(&wc,sizeof(wc));
	wc.lpfnWndProc = WndProc;			// our window procedure
	wc.hInstance = hPluginInstance;		// hInstance of .dll
	wc.lpszClassName = szAppName;		// our window class name
	if (!RegisterClass(&wc)) {
		MessageBox(hwndBlackbox, "Error registering window class", szAppName, MB_OK | MB_ICONERROR | MB_TOPMOST);
		return 1;
	}

	// Get plugin and style settings...
	ReadRCSettings();
	GetStyleSettings();
	int gap = bevelWidth + borderWidth;
	iconSize = height - gap * 2;

	ScreenWidth  = GetSystemMetrics(SM_CXVIRTUALSCREEN);
	ScreenHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);

	// Create the window...
	hwndPlugin = CreateWindowEx(
			WS_EX_TOOLWINDOW, // window style
			szAppName, // our window class name
			NULL, // NULL -> does not show up in task manager!
			WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, // window parameters
			xpos, // x position
			ypos, // y position
			width, // window width
			height, // window height
			NULL, // parent window
			NULL, // no menu
			hPluginInstance, // hInstance of .dll
			NULL);

	if (!hwndPlugin){						   
		MessageBox(0, "Error creating window", szAppName, MB_OK | MB_ICONERROR | MB_TOPMOST);
		// Unregister Blackbox messages...
		SendMessage(hwndBlackbox, BB_UNREGISTERMESSAGE, (WPARAM)hwndPlugin, (LPARAM)msgs);
		return 1;
	}

	// Register to receive Blackbox messages...
	SendMessage(hwndBlackbox, BB_REGISTERMESSAGE, (WPARAM)hwndPlugin, (LPARAM)msgs);
	// Make the window sticky
	MakeSticky(hwndPlugin);
	// Make the window AlwaysOnTop?
	if (alwaysOnTop) SetWindowPos(hwndPlugin, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOMOVE);

	SetTimer(hwndPlugin, 0, checkInterval * 1000, NULL); // Start timer.

	// Show the window and force it to update...
	ShowWindow(hwndPlugin, SW_SHOW);
	setAttr = true;
	InvalidateRect(hwndPlugin, NULL, true);

  	//====================
	
	INITCOMMONCONTROLSEX ic;
	ic.dwSize = sizeof(INITCOMMONCONTROLSEX);
	ic.dwICC = ICC_BAR_CLASSES;

        if (InitCommonControlsEx(&ic))
        { // Load "tab" controls, including tooltips.
	        hToolTips = CreateWindowEx(
		        WS_EX_TOPMOST,
		        TOOLTIPS_CLASS, // "tooltips_class32"
		        NULL, //"BBSBTT",
		        WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX,
		        CW_USEDEFAULT,
		        CW_USEDEFAULT,
		        CW_USEDEFAULT,
		        CW_USEDEFAULT,
		        NULL,
		        NULL,
		        hPluginInstance,
		        NULL);

	        SendMessage(hToolTips, TTM_SETMAXTIPWIDTH, 0, 300);

	        //SendMessage(hToolTips, TTM_SETDELAYTIME, TTDT_AUTOMATIC, 200);

	        SendMessage(hToolTips, TTM_SETDELAYTIME, TTDT_AUTOPOP, 4000);
	        SendMessage(hToolTips, TTM_SETDELAYTIME, TTDT_INITIAL, 120);
	        SendMessage(hToolTips, TTM_SETDELAYTIME, TTDT_RESHOW,   60);

                if (NULL != hToolTips)
                      SetAllowTip(allowtip);
                else
                      SetAllowTip(false);
	} 
	else
        SetAllowTip(false);
  
  return 0;
}

//===========================================================================

void endPlugin(HINSTANCE hPluginInstance)
{
	// Write the current plugin settings to the config file...
	WriteRCSettings();
	// Delete the main plugin menu if it exists (PLEASE NOTE that this takes care of submenus as well!)
	if (myMenu){ DelMenu(myMenu); myMenu = NULL;}

	// Release our timer resources
	KillTimer(hwndPlugin, 0);
	// Make the window unsticky
	RemoveSticky(hwndPlugin);
	// Unregister Blackbox messages...
	SendMessage(hwndBlackbox, BB_UNREGISTERMESSAGE, (WPARAM)hwndPlugin, (LPARAM)msgs);

	if(inSlit && hSlit)
		SendMessage(hSlit, SLIT_REMOVE, 0, (LPARAM)hwndPlugin);

	ClearToolTips();
  	DestroyWindow(hToolTips);
	// Destroy our window...
	DestroyWindow(hwndPlugin);
	// Unregister window class...
	UnregisterClass(szAppName, hPluginInstance);
}

//===========================================================================

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

//===========================================================================

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int gap = bevelWidth + borderWidth;

	switch (message)
	{
		case WM_PAINT:
		{
			SetRecyclebin();
			OnPaint(hwnd);
		}
		break;

		case WM_DROPFILES:
		{
			OnDropFiles((HDROP)wParam);
			InvalidateRect(hwndPlugin, NULL, false);
		}
		break;

		// ==========

		case BB_RECONFIGURE:
		{
			if (myMenu){ DelMenu(myMenu); myMenu = NULL;}
			GetStyleSettings();
			InvalidateRect(hwndPlugin, NULL, true);
		}
		break;

		// for bbStylemaker
		case BB_REDRAWGUI:
		{
			GetStyleSettings();
			InvalidateRect(hwndPlugin, NULL, true);
		}
		break;

		case WM_TIMER:
			InvalidateRect(hwndPlugin, NULL, false);
		break;

		case WM_DISPLAYCHANGE:
		{
			if(!inSlit)
			{
				// IntelliMove(tm)... <g>
				// (c) 2003 qwilk
				//should make this a function so it can be used on startup in case resolution changed since
				//the last time blackbox was used.
				int relx, rely;
				int oldscreenwidth = ScreenWidth;
				int oldscreenheight = ScreenHeight;
				ScreenWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
				ScreenHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);
				if (xpos > oldscreenwidth / 2)
				{
					relx = oldscreenwidth - xpos;
					xpos = ScreenWidth - relx;
				}
				if (ypos > oldscreenheight / 2)
				{
					rely = oldscreenheight - ypos;
					ypos = ScreenHeight - rely;
				}
				MoveWindow(hwndPlugin, xpos, ypos, width, height, true);
			}
		}
		break;

		// ==========

		case BB_BROADCAST:
		{
			szTemp = (char*)lParam;

			if (!_stricmp(szTemp, "@BBShowPlugins") &&  pluginToggle && !inSlit)
			{
				// Show window and force update...
				ShowWindow( hwndPlugin, SW_SHOW);
				InvalidateRect( hwndPlugin, NULL, true);
			}
			else if (!_stricmp(szTemp, "@BBHidePlugins") && pluginToggle && !inSlit)
			{
				// Hide window...
				ShowWindow( hwndPlugin, SW_HIDE);
			}

			//===================

			if (strnicmp(szTemp, "@BBRecycleBin", 13))
				return 0;
			szTemp += 13;

			if (!_stricmp(szTemp, "About"))
			{
				char tmp_str[MAX_LINE_LENGTH];
				SendMessage(hwndBlackbox, BB_HIDEMENU, 0, 0);
				sprintf(tmp_str, "%s", szVersion);
				MessageBox(hwndBlackbox, tmp_str, szAppName, MB_OK | MB_TOPMOST);
			}

			//===================

			else if (!_strnicmp(szTemp, "StyleType", 9))
			{
				styleType = atoi(szTemp + 10);
				GetStyleSettings();
				InvalidateRect(hwndPlugin, NULL, true);
			}

			//===================

			else if (!_strnicmp(szTemp, "JustifyType", 11))
			{
				justify = atoi(szTemp + 12);
				InvalidateRect(hwndPlugin, NULL, true);
				ShowMyMenu(false);
			}

			//===================

			else if (!_strnicmp(szTemp, "WidthSize", 9))
			{
				newWidth = atoi(szTemp + 10);
				if(ResizeMyWindow(newWidth, height))
					InvalidateRect(hwndPlugin, NULL, true);
			}
			else if (!_strnicmp(szTemp, "HeightSize", 10))
			{
				newHeight = atoi(szTemp + 11);
				iconSize = newHeight - gap * 2;
				if(ResizeMyWindow(width, newHeight))
					InvalidateRect(hwndPlugin, NULL, true);
			}

			else if (!_strnicmp(szTemp, "FontSize", 8))
			{
				fontHeight = atoi(szTemp + 9);
				InvalidateRect(hwndPlugin, NULL, true);
			}
			else if (!_strnicmp(szTemp, "FontFace", 8))
			{
				strcpy(fontFace, szTemp + 9);
				InvalidateRect(hwndPlugin, NULL, true);
				ShowMyMenu(false);
			}
			else if (!_stricmp(szTemp, "UseRefFont"))
			{
				useRefFont = (useRefFont ? false : true);
				InvalidateRect(hwndPlugin, NULL, false);
				ShowMyMenu(false);
			}
			else if (!_stricmp(szTemp, "UseTwoLine"))
			{
				useTwoLine = (useTwoLine ? false : true);
				InvalidateRect(hwndPlugin, NULL, false);
				ShowMyMenu(false);
			}
			else if (!_stricmp(szTemp, "InsertSpace"))
			{
				insertSpace = (insertSpace ? false : true);
				InvalidateRect(hwndPlugin, NULL, false);
				ShowMyMenu(false);
			}
			else if (!_strnicmp(szTemp, "CheckInterval", 13))
			{
				checkInterval = atoi(szTemp + 14);
				KillTimer(hwndPlugin, 0); 
				SetTimer(hwndPlugin, 0, checkInterval * 1000, NULL);
			}

			//===================

			else if (!_stricmp(szTemp, "PluginToggle"))
			{
				pluginToggle = (pluginToggle ? false : true);
				ShowMyMenu(false);
			}
			else if (!_stricmp(szTemp, "OnTop"))
			{
				alwaysOnTop = (alwaysOnTop ? false : true);
				if(!inSlit)SetWindowPos( hwndPlugin,
                                                         alwaysOnTop ? HWND_TOPMOST : HWND_NOTOPMOST,
                                                         0, 0, 0, 0,
                                                         SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOMOVE);

				ShowMyMenu(false);
			}

			else if (!_stricmp(szTemp, "InSlit"))
			{
				if(inSlit && hSlit){
					// We are in the slit, so lets unload and get out..
					SendMessage(hSlit, SLIT_REMOVE, 0, (LPARAM)hwndPlugin);

					// Here you can move to where ever you want ;)
					SetWindowPos(hwndPlugin, NULL, xpos, ypos, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
					inSlit = false;
					
				}
				/* Make sure before you try and load into the slit that you have
				* the HWND of the slit ;)
				*/
				else if(hSlit){
					// (Back) into the slit..
					inSlit = true;
					SetWindowLong(hwndPlugin, GWL_EXSTYLE,
                                                      GetWindowLong(hwndPlugin, GWL_EXSTYLE) & ~WS_EX_LAYERED);
					SendMessage(hSlit, SLIT_ADD, 0, (LPARAM)hwndPlugin);
				}
				setAttr = true;
				InvalidateRect(hwndPlugin, NULL, true);
				ShowMyMenu(false);
			}

			else if (!_stricmp(szTemp, "Transparent"))
			{
				transparency = (transparency ? false : true);
				setAttr = true;
				InvalidateRect(hwndPlugin, NULL, true);
				ShowMyMenu(false);
			}
			else if (!_stricmp(szTemp, "TransBack"))
			{
				transBack = (transBack ? false : true);
				setAttr = true;
				InvalidateRect(hwndPlugin, NULL, true);
				ShowMyMenu(false);
			}
			else if (!_strnicmp(szTemp, "AlphaValue", 10))
			{
				alpha = atoi(szTemp + 11);
				if (transBack || transparency){
					InvalidateRect(hwndPlugin, NULL, true);
                                }
			}
			else if (!_stricmp(szTemp, "SnapToEdge"))
			{
				snapWindow = (snapWindow ? false : true);
				ShowMyMenu(false);
			}
			else if (!_stricmp(szTemp, "ShowBorder"))
			{
				showBorder = (showBorder ? false : true);
				InvalidateRect(hwndPlugin, NULL, false);
				ShowMyMenu(false);
			}
			else if (!_stricmp(szTemp, "AllowTip"))
			{
				allowtip = (allowtip ? false : true);
				SetAllowTip(allowtip);
				ShowMyMenu(false);
			}
			else if (!_stricmp(szTemp, "LockPosition"))
			{
				lockPos = (lockPos ? false : true);
				ShowMyMenu(false);
                        }
			//===================

			else if (!_stricmp(szTemp, "Empty"))
			{
				SHEmptyRecycleBin(NULL, NULL, 0);
				InvalidateRect(hwndPlugin, NULL, false);
				ShowMyMenu(false);
			}

			else if (!_stricmp(szTemp, "SetEmptyIcon"))
			{
				OPENFILENAME ofn;
				memset( &ofn, 0, sizeof(OPENFILENAME) );
				ofn.lStructSize= sizeof(OPENFILENAME);
				ofn.lpstrFilter= "ICON(*.ico)\0*.ico\0\0";
				ofn.lpstrFile= emptyicon;
				ofn.nMaxFile= sizeof(emptyicon);
				ofn.Flags= OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

				if( GetOpenFileName(&ofn) == TRUE)
					    InvalidateRect(hwndPlugin, NULL, true);
			}
			else if (!_stricmp(szTemp, "RemoveEmptyIcon"))
			{
				strcpy(emptyicon, "");
				InvalidateRect(hwndPlugin, NULL, true);
			}
			else if (!_stricmp(szTemp, "SetFullIcon"))
			{
				OPENFILENAME ofn;
				memset( &ofn, 0, sizeof(OPENFILENAME) );
				ofn.lStructSize= sizeof(OPENFILENAME);
				ofn.lpstrFilter= "ICON(*.ico)\0*.ico\0\0";
				ofn.lpstrFile= fullicon;
				ofn.nMaxFile= sizeof(fullicon);
				ofn.Flags= OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

				if( GetOpenFileName(&ofn) == TRUE)
					    InvalidateRect(hwndPlugin, NULL, true);
			}
			else if (!_stricmp(szTemp, "RemoveFullIcon"))
			{
				strcpy(fullicon, "");
				InvalidateRect(hwndPlugin, NULL, true);
			}
			else if (!_stricmp(szTemp, "FontColor"))
			{
				CHOOSECOLOR cc;
				COLORREF custCol[16];
				ZeroMemory(&cc, sizeof(CHOOSECOLOR));
				cc.lStructSize = sizeof(CHOOSECOLOR);
				cc.hwndOwner = NULL;
				cc.lpCustColors = (LPDWORD) custCol;
				cc.rgbResult = fontColor;
				cc.Flags = CC_RGBINIT;

				if (ChooseColor(&cc)){
					    fontColor = cc.rgbResult;
					    InvalidateRect(hwndPlugin, NULL, true);
				}
			}

			else if (!_stricmp(szTemp, "SetFont"))
			{
				LOGFONT lf;
				hFont = CreateFont(fontHeight,
                                                       0, 0, 0, FW_NORMAL,
                                                       false, false, false,
                                                       DEFAULT_CHARSET,
                                                       OUT_DEFAULT_PRECIS,
                                                       CLIP_DEFAULT_PRECIS,
                                                       DEFAULT_QUALITY,
                                                       DEFAULT_PITCH, fontFace);
				GetObject(hFont, sizeof(lf), &lf);
				DeleteObject(hFont);

				CHOOSEFONT cf;
				memset(&cf, 0, sizeof(CHOOSEFONT));
				cf.lStructSize = sizeof(CHOOSEFONT);
				cf.hwndOwner = hwndPlugin;
				cf.lpLogFont = &lf;
				cf.nFontType = SCREEN_FONTTYPE;
				cf.Flags = CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT;

				if (ChooseFont(&cf)){
					    strcpy(fontFace, lf.lfFaceName);
					    fontHeight = -lf.lfHeight;
					    InvalidateRect(hwndPlugin, NULL, true);
					    ShowMyMenu(false);
				}
			}

			//===================

			else if (!_stricmp(szTemp, "EditRC"))
			{
				BBExecute(GetDesktopWindow(), NULL, rcpath, NULL, NULL, SW_SHOWNORMAL, false);
			}
			else if (!_stricmp(szTemp, "ReloadSettings"))
			{
				ReadRCSettings();
				GetStyleSettings();

				if(inSlit && hSlit)
					SendMessage(hSlit, SLIT_UPDATE, 0, 0);
				else if(!inSlit || !hSlit)
					SendMessage(hSlit, SLIT_REMOVE, 0, (LPARAM)hwndPlugin);

				else inSlit = false;
			}
			else if (!_stricmp(szTemp, "SaveSettings"))
			{
				WriteRCSettings();
			}
		}
		break;

		// ==========

		case WM_WINDOWPOSCHANGING:
		{
			// Is SnapWindowToEdge enabled?
			if (!inSlit && snapWindow)
			{
				// Snap window to screen edges (if the last bool is false it uses the current DesktopArea)
				if(IsWindowVisible(hwnd)) SnapWindowToEdge((WINDOWPOS*)lParam, 10, true);
			}
		}
		break;

		// ==========

		// Save window position if it changes...
		case WM_WINDOWPOSCHANGED:
		{
			if(!inSlit)
			{
				WINDOWPOS* windowpos = (WINDOWPOS*)lParam;
				xpos = windowpos->x;
				ypos = windowpos->y;

				if(ResizeMyWindow(windowpos->cx, windowpos->cy))
				{
                                	iconSize = height - gap * 2;
					InvalidateRect(hwndPlugin, NULL, true);
				}
			}
		}
		break;

		// ==========

		case WM_NCHITTEST:
		{
			if (GetKeyState(VK_MENU) & 0xF0 && !lockPos)
				return HTBOTTOMRIGHT;
			else
				return HTCAPTION;
		}
		break;

		case WM_NCLBUTTONDOWN:
		{
			if(!inSlit && !lockPos)
				return DefWindowProc(hwnd,message,wParam,lParam);
		}
		break;

		case WM_NCLBUTTONDBLCLK: 
		{
			SHELLEXECUTEINFO sei;
			ZeroMemory(&sei, sizeof(sei));
			sei.cbSize = sizeof(sei);
			sei.hwnd = NULL;
			sei.lpVerb = NULL;
			sei.lpFile = GUID_RECYCLEBIN;
			sei.lpParameters = _T("");
			sei.lpDirectory = _T("");
			sei.nShow = SW_SHOWNORMAL;
			ShellExecuteEx(&sei);
		}
		break;

		// ==========

		// Right mouse button clicked?
		case WM_NCRBUTTONUP:
		{
			ShowMyMenu(true);
		}
		break;

		case WM_NCRBUTTONDOWN: {} break;


		case WM_CLOSE:
		{
			return 0;
		}
		break;
 		// ==========
		default:
			return DefWindowProc(hwnd,message,wParam,lParam);
	}
	return 0;
}

//===========================================================================

void ShowMyMenu(bool popup)
{
	if (myMenu){ DelMenu(myMenu); myMenu = NULL;}

	configSubmenu = MakeNamedMenu("Configuration", "BBRB_Config", popup);
	MakeMenuItemInt(configSubmenu, "Width Size", "@BBRecycleBinWidthSize", width, height, ScreenWidth);
	MakeMenuItemInt(configSubmenu, "Height Size", "@BBRecycleBinHeightSize", height, 10, ScreenHeight);

	fontSubmenu = MakeNamedMenu("Font", "BBRB_Font", popup);
 	MakeMenuItem(fontSubmenu, "Use Ref Font", "@BBRecycleBinUseRefFont", useRefFont);
	if(!useRefFont){
	MakeMenuItemInt(fontSubmenu, "Font Size", "@BBRecycleBinFontSize", fontHeight, 1, 64);
	MakeMenuItemString(fontSubmenu, "Font Face", "@BBRecycleBinFontFace", fontFace);
	MakeMenuItem(fontSubmenu, "Font Color",  "@BBRecycleBinFontColor", false);
	MakeMenuItem(fontSubmenu, "Browse...",  "@BBRecycleBinSetFont", false);
	}
	MakeSubmenu(configSubmenu, fontSubmenu, "Font");

	iconSubmenu = MakeNamedMenu("Icon", "BBRB_Icon", popup);
	emptySubmenu = MakeNamedMenu("Empty Icon", "BBRB_Empty", popup);
	MakeMenuItem(emptySubmenu, "Browse...", "@BBRecycleBinSetEmptyIcon", false);
	MakeMenuItem(emptySubmenu, "Nothing", "@BBRecycleBinRemoveEmptyIcon", false);
	MakeSubmenu(iconSubmenu, emptySubmenu, "Empty Icon");
	fullSubmenu = MakeNamedMenu("Full Icon", "BBRB_Full", popup);
	MakeMenuItem(fullSubmenu, "Browse...", "@BBRecycleBinSetFullIcon", false);
	MakeMenuItem(fullSubmenu, "Nothing", "@BBRecycleBinRemoveFullIcon", false);
	MakeSubmenu(iconSubmenu, fullSubmenu, "Full Icon");
	MakeSubmenu(configSubmenu, iconSubmenu, "Icon");

 	MakeMenuItem(configSubmenu, "Use Two Line", "@BBRecycleBinUseTwoLine", useTwoLine);
 	MakeMenuItem(configSubmenu, "Insert Space", "@BBRecycleBinInsertSpace", insertSpace);
	MakeMenuNOP(configSubmenu, NULL);
	if(hSlit) MakeMenuItem(configSubmenu, "In Slit", "@BBRecycleBinInSlit", inSlit);
	MakeMenuItem(configSubmenu, "Plugin Toggle", "@BBRecycleBinPluginToggle", pluginToggle);
	MakeMenuItem(configSubmenu, "Always on Top", "@BBRecycleBinOnTop", alwaysOnTop);
	MakeMenuItem(configSubmenu, "Snap To Edge", "@BBRecycleBinSnapToEdge", snapWindow);
	MakeMenuItem(configSubmenu, "Show Border", "@BBRecycleBinShowBorder", showBorder);
	MakeMenuItem(configSubmenu, "Allow Tooltip", "@BBRecycleBinAllowTip", allowtip);
	MakeMenuItem(configSubmenu, "Lock Position", "@BBRecycleBinLockPosition", lockPos);
	if(!inSlit){
	MakeMenuItem(configSubmenu, "Transparency", "@BBRecycleBinTransparent", transparency);
	MakeMenuItem(configSubmenu, "Trans Back", "@BBRecycleBinTransBack", transBack);
	MakeMenuItemInt(configSubmenu, "Alpha Value", "@BBRecycleBinAlphaValue", alpha, 0, 255);
	}

	styleSubmenu = MakeNamedMenu("Style Type", "BBRB_StyleType", popup);
	MakeMenuItem(styleSubmenu, "Toolbar",  "@BBRecycleBinStyleType 1", (styleType == 1));
	MakeMenuItem(styleSubmenu, "Button",   "@BBRecycleBinStyleType 2", (styleType == 2));
	MakeMenuItem(styleSubmenu, "ButtonP",  "@BBRecycleBinStyleType 3", (styleType == 3));
	MakeMenuItem(styleSubmenu, "Label",    "@BBRecycleBinStyleType 4", (styleType == 4));
	MakeMenuItem(styleSubmenu, "WinLabel", "@BBRecycleBinStyleType 5", (styleType == 5));
	MakeMenuItem(styleSubmenu, "Clock",    "@BBRecycleBinStyleType 6", (styleType == 6));

	justifySubmenu = MakeNamedMenu("Justification", "BBRB_JustifyType", popup);
	MakeMenuItem(justifySubmenu, "Left",     "@BBRecycleBinJustifyType 0", (justify == 0));
	MakeMenuItem(justifySubmenu, "Center",   "@BBRecycleBinJustifyType 1", (justify == 1));
	MakeMenuItem(justifySubmenu, "Right",    "@BBRecycleBinJustifyType 2", (justify == 2));

	settingsSubmenu = MakeNamedMenu("Settings", "BBRB_Settings", popup);
	MakeMenuItem(settingsSubmenu, "Edit Settings",   "@BBRecycleBinEditRC", false);
	MakeMenuItem(settingsSubmenu, "Reload Settings", "@BBRecycleBinReloadSettings", false);
	MakeMenuItem(settingsSubmenu, "Save Settings",   "@BBRecycleBinSaveSettings", false);
	MakeMenuItemInt(settingsSubmenu, "Check Interval (sec)", "@BBRecycleBinCheckInterval", checkInterval, 1, 60);

	myMenu = MakeNamedMenu("BBRecycleBin", "BBRB_Main", popup);
	if (!g_isEmpty) MakeMenuItem(myMenu, "EmptyRecycleBin", "@BBRecycleBinEmpty", false);
	MakeSubmenu(myMenu, configSubmenu, "Configuration");
	MakeSubmenu(myMenu, styleSubmenu, "Style Type");
	MakeSubmenu(myMenu, justifySubmenu, "Justification");
	MakeSubmenu(myMenu, settingsSubmenu, "Settings");
  	MakeMenuItem(myMenu, "About", "@BBRecycleBinAbout", false);
	ShowMenu(myMenu);
}

//===========================================================================

void GetStyleSettings()
{
	bevelWidth  = *(int*)GetSettingPtr(SN_BEVELWIDTH);
	borderWidth = *(int*)GetSettingPtr(SN_BORDERWIDTH);
	borderColor = *(COLORREF*)GetSettingPtr(SN_BORDERCOLOR);

	myStyleItem = *(StyleItem*)GetSettingPtr(styleType);

	ShowMyMenu(false);
}

//===========================================================================

void ReadRCSettings()
{
	char temp[MAX_LINE_LENGTH], path[MAX_LINE_LENGTH], defaultpath[MAX_LINE_LENGTH];
	int nLen;

	// First we look for the config file in the same folder as the plugin...
	GetModuleFileName(hInstance, rcpath, sizeof(rcpath));
	nLen = strlen(rcpath) - 1;
	while (nLen >0 && rcpath[nLen] != '\\') nLen--;
	rcpath[nLen + 1] = 0;
	strcpy(temp, rcpath);
	strcpy(path, rcpath);
	strcat(temp, "bbrecyclebin.rc");
	strcat(path, "bbrecyclebinrc");
	// ...checking the two possible filenames bbrecyclebin.rc and bbrecyclebinrc ...
	if (FileExists(temp)) strcpy(rcpath, temp);
	else if (FileExists(path)) strcpy(rcpath, path);
	// ...if not found, we try the Blackbox directory...
	else
	{
		// ...but first we save the default path (bbrecyclebin.rc in the same
		// folder as the plugin) just in case we need it later (see below)...
		strcpy(defaultpath, temp);
		GetBlackboxPath(rcpath, sizeof(rcpath));
		strcpy(temp, rcpath);
		strcpy(path, rcpath);
		strcat(temp, "bbrecyclebin.rc");
		strcat(path, "bbrecyclebinrc");
		if (FileExists(temp)) strcpy(rcpath, temp);
		else if (FileExists(path)) strcpy(rcpath, path);
		else // If no config file was found, we use the default path and settings, and return
		{
			strcpy(rcpath, defaultpath);
			xpos = ypos = 10;
			width = 100;
			height = 34;
			fontHeight = 12;
			alpha = 160;
			styleType = 1;
			justify = 1;
			checkInterval = 5;
			alwaysOnTop = false;
			transparency = false;
			transBack = false;
			snapWindow = true;
			pluginToggle = false;
			inSlit = false;
			showBorder = true;
			allowtip = false;
			lockPos = false;
			useRefFont = true;
			useTwoLine = false;
			insertSpace = false;
			strcpy(fontFace, "Tahoma");
			WriteRCSettings(); //write a file for editing later
			return;
		}
	}

	// If a config file was found we read the plugin settings from the file...
	xpos = ReadInt(rcpath, "bbrecyclebin.x:", 10);
	ypos = ReadInt(rcpath, "bbrecyclebin.y:", 10);
	if (xpos >= GetSystemMetrics(SM_CXVIRTUALSCREEN)) xpos = 10;
	if (ypos >= GetSystemMetrics(SM_CYVIRTUALSCREEN)) ypos = 10;
	width = ReadInt(rcpath, "bbrecyclebin.width:", 100);
  	height = ReadInt(rcpath, "bbrecyclebin.height:", 34);
	fontHeight = ReadInt(rcpath, "bbrecyclebin.fontHeight:", 12);
	alpha = ReadInt(rcpath, "bbrecyclebin.alpha:", 160);
	styleType = ReadInt(rcpath, "bbrecyclebin.styleType:", 1);
	justify = ReadInt(rcpath, "bbrecyclebin.justification:", 1);
	checkInterval = ReadInt(rcpath, "bbrecyclebin.checkInterval(sec):", 5);
	alwaysOnTop = ReadBool(rcpath, "bbrecyclebin.alwaysontop:", false);
	transparency = ReadBool(rcpath, "bbrecyclebin.transparency:", false);
	transBack = ReadBool(rcpath, "bbrecyclebin.transBack:", false);
	snapWindow = ReadBool(rcpath, "bbrecyclebin.snapwindow:", true);
	pluginToggle = ReadBool(rcpath, "bbrecyclebin.pluginToggle:", false);
	inSlit = ReadBool(rcpath, "bbrecyclebin.inSlit:", false);
	showBorder = ReadBool(rcpath, "bbrecyclebin.showBorder:", true);
	allowtip = ReadBool(rcpath, "bbrecyclebin.allowtooltip:", false);
	lockPos = ReadBool(rcpath, "bbrecyclebin.lockPos:", false);
	useRefFont = ReadBool(rcpath, "bbrecyclebin.useRefFont:", true);
 	useTwoLine = ReadBool(rcpath, "bbrecyclebin.useTwoLine:", false);
 	insertSpace = ReadBool(rcpath, "bbrecyclebin.insertSpace:", false);
	fontColor = ReadColor(rcpath, "bbrecyclebin.fontColor:", "#000000");
	strcpy(fontFace, ReadString(rcpath, "bbrecyclebin.font:", "Tahoma"));
  	strcpy(emptyicon, ReadString(rcpath, "bbrecyclebin.emptyicon:", ""));
  	strcpy(fullicon, ReadString(rcpath, "bbrecyclebin.fullicon:", ""));
}

//===========================================================================

void WriteRCSettings()
{
	WriteInt(rcpath, "bbrecyclebin.x:", xpos);
	WriteInt(rcpath, "bbrecyclebin.y:", ypos);
	WriteInt(rcpath, "bbrecyclebin.width:", width);
	WriteInt(rcpath, "bbrecyclebin.height:", height);
	WriteInt(rcpath, "bbrecyclebin.fontHeight:", fontHeight);
	WriteInt(rcpath, "bbrecyclebin.alpha:", alpha);
	WriteInt(rcpath, "bbrecyclebin.styleType:", styleType);
	WriteInt(rcpath, "bbrecyclebin.justification:", justify);
	WriteInt(rcpath, "bbrecyclebin.checkInterval(sec):", checkInterval);
	WriteBool(rcpath, "bbrecyclebin.alwaysontop:", alwaysOnTop);
	WriteBool(rcpath, "bbrecyclebin.transparency:", transparency);
	WriteBool(rcpath, "bbrecyclebin.transBack:", transBack);
	WriteBool(rcpath, "bbrecyclebin.snapWindow:", snapWindow);
	WriteBool(rcpath, "bbrecyclebin.pluginToggle:", pluginToggle);
	WriteBool(rcpath, "bbrecyclebin.inSlit:", inSlit);
	WriteBool(rcpath, "bbrecyclebin.showBorder:", showBorder);
	WriteBool(rcpath, "bbrecyclebin.allowtooltip:", allowtip);
	WriteBool(rcpath, "bbrecyclebin.lockPos:", lockPos);
	WriteBool(rcpath, "bbrecyclebin.useRefFont:", useRefFont);
	WriteBool(rcpath, "bbrecyclebin.useTwoLine:", useTwoLine);
	WriteBool(rcpath, "bbrecyclebin.insertSpace:", insertSpace);
	WriteColor(rcpath, "bbrecyclebin.fontColor:", fontColor);
	WriteString(rcpath, "bbrecyclebin.font:", fontFace);
	WriteString(rcpath, "bbrecyclebin.emptyicon:", emptyicon);
  	WriteString(rcpath, "bbrecyclebin.fullicon:", fullicon);
}

//===========================================================================

void OnPaint(HWND hwnd)
{
	// Create buffer hdc's, bitmaps etc.
	PAINTSTRUCT ps;  RECT r, textrc;  HICON hIcon;
	int gap = bevelWidth + borderWidth;

	//get screen buffer
	HDC hdc_scrn = BeginPaint(hwnd, &ps);

	//get window rectangle.
	GetClientRect(hwnd, &r);

	//first get a new 'device context'
	HDC hdc = CreateCompatibleDC(NULL);

	//then create a buffer in memory with the window size
	HBITMAP bufbmp = CreateCompatibleBitmap(hdc_scrn, r.right, r.bottom);

	//select the bitmap into the DC
	HGDIOBJ otherbmp = SelectObject(hdc, bufbmp);


	if(inSlit || !transBack) MakeStyleGradient(hdc, &r, &myStyleItem, showBorder);

  	r.left = r.left + gap;
	r.top = r.top + gap;
	r.bottom = r.bottom - gap;
	r.right = r.right - gap;

	if(useRefFont){
		tbStyleItem = *(StyleItem*)GetSettingPtr(SN_TOOLBAR);
		otherfont = SelectObject(hdc,CreateStyleFont(&tbStyleItem));
		SetTextColor(hdc, tbStyleItem.TextColor);
	}else{
		otherfont = SelectObject(hdc,
					CreateFont(fontHeight,
					0, 0, 0, FW_NORMAL,
					false, false, false,
					DEFAULT_CHARSET,
					OUT_DEFAULT_PRECIS,
					CLIP_DEFAULT_PRECIS,
					DEFAULT_QUALITY,
					DEFAULT_PITCH, fontFace));

		SetTextColor(hdc, fontColor);
	}
	SetBkMode(hdc, TRANSPARENT);

	SetToolTip(&r, displayString);

	if (!g_isEmpty)
        	hIcon = (HICON)LoadImage( hInstance, fullicon,IMAGE_ICON, iconSize, iconSize, LR_LOADFROMFILE);
        else
        	hIcon = (HICON)LoadImage( hInstance, emptyicon,IMAGE_ICON, iconSize, iconSize, LR_LOADFROMFILE);

	if(0 == hIcon){
		if(useTwoLine){
			DrawText(hdc, displayString, strlen(displayString), &textrc, DT_CALCRECT);
			r.top = (r.bottom - r.top - (textrc.bottom - textrc.top)) / 2 + gap;
        		DrawText(hdc, displayString, strlen(displayString), &r, justify);
		}else{
			DrawText(hdc, displayString, strlen(displayString), &r, justify | DT_VCENTER | DT_SINGLELINE | DT_WORD_ELLIPSIS);
		}
	}else{
        	DrawIconEx(hdc, gap, gap, hIcon, iconSize, iconSize, 0, NULL, DI_NORMAL);

        	r.left = r.left + iconSize + gap;

		if(useTwoLine){
			DrawText(hdc, displayString, strlen(displayString), &textrc, DT_CALCRECT);
			r.top = (r.bottom - r.top - (textrc.bottom - textrc.top)) / 2 + gap;
        		DrawText(hdc, displayString, strlen(displayString), &r, justify);
		}else{
			DrawText(hdc, displayString, strlen(displayString), &r, justify | DT_VCENTER | DT_SINGLELINE | DT_WORD_ELLIPSIS);
		}
	}

	if(usingWin2kXP){
	// Set an attribute of the window
	if(setAttr){
		if(!inSlit){
			SetWindowLong(hwnd, GWL_EXSTYLE,
                                      GetWindowLong(hwnd, GWL_EXSTYLE) & ~WS_EX_LAYERED);

			SetWindowLong(hwnd, GWL_EXSTYLE ,
                                      GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED);
		}
		DragAcceptFiles(hwnd, true);
		if(!inSlit) SetWindowPos( hwnd,
                                          alwaysOnTop ? HWND_TOPMOST : HWND_NOTOPMOST,
                                          0, 0, 0, 0,
                                          SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOMOVE);
		setAttr = false;
	}

	// Set the transparency mode
	if(!inSlit && transBack)
	{
		RECT rt;
		GetWindowRect(hwnd, &rt);

		BLENDFUNCTION bf;
		bf.BlendOp = AC_SRC_OVER;
		bf.BlendFlags = 0;
		bf.AlphaFormat = AC_SRC_ALPHA;
		bf.SourceConstantAlpha = alpha;

		POINT ptWnd = {rt.left, rt.top};
		POINT ptSrc = {0, 0};
		SIZE  szWnd = {width, height};

		pUpdateLayeredWindow(hwnd, NULL, NULL, &szWnd,
                                                     hdc, &ptSrc, 0, &bf, ULW_ALPHA);
	}
	else if(!inSlit)
	{
		pSetLayeredWindowAttributes(hwnd, NULL,
                                            (transparency) ? (unsigned char)alpha : 255, LWA_ALPHA);
	}

	}

	// Finally, copy from the paint buffer to the window...
	BitBlt(hdc_scrn, 0, 0, width, height, hdc, 0, 0, SRCCOPY);

	// Remember to delete all objects!
	DestroyIcon(hIcon);
	DeleteObject(SelectObject(hdc, otherfont));
	DeleteObject(SelectObject(hdc, otherbmp));
	DeleteDC(hdc);

	EndPaint(hwnd, &ps);
}

//=========================================================================

void OnDropFiles(HDROP hDrop)
{
	TCHAR szFile[MAX_PATH];
	size_t len = 0;
	TCHAR *pc;
	BOOL bNoRecycle = (GetKeyState(VK_SHIFT) < 0);

	int num = DragQueryFile(hDrop, -1, NULL, 0);
	LPTSTR lpszFiles = (LPTSTR)GlobalAlloc(GMEM_FIXED, num * (MAX_PATH + 1));
	if (lpszFiles){
		lpszFiles[0] = 0;
		for (int ii = 0; ii < num; ii ++){
			DragQueryFile(hDrop, ii, szFile, sizeof(szFile));
			if (ii == 0){
				len = _tcslen(szFile);
				_tcscpy(lpszFiles, szFile);
			}else{
				pc = lpszFiles + len;
				memcpy(pc, szFile, _tcslen(szFile));
				len = len + _tcslen(szFile);
			}
			pc = lpszFiles + len;
			*pc = 0;
			len ++;

		}
		pc = lpszFiles + len;
		*pc = 0;

		SHFILEOPSTRUCT FileOp;
		FileOp.hwnd = hwndPlugin;
		FileOp.wFunc = FO_DELETE;
		FileOp.pFrom = lpszFiles;
		FileOp.pTo = NULL;
		FileOp.fFlags = ((bNoRecycle) ? 0 : FOF_ALLOWUNDO) | FOF_NOCONFIRMATION;
		FileOp.fAnyOperationsAborted = FALSE;
		FileOp.hNameMappings = NULL;
		FileOp.lpszProgressTitle = NULL;
		SHFileOperation(&FileOp);
		GlobalFree(lpszFiles);
	}
}

//===========================================================================

void SetRecyclebin()
{
	g_nItems = 0;
	g_nBytes = 0;
	g_isEmpty = true;
	SHQUERYRBINFO rbi;
	rbi.cbSize = sizeof(rbi);

	if(usingWin2k){
		char szDrives[256];
		GetLogicalDriveStrings(255, szDrives);
		const char *pStr = szDrives;
		while(*pStr != '\0'){
			SHQueryRecycleBin(pStr, &rbi);
			if (rbi.i64NumItems > 0) {
				g_isEmpty = false;
				g_nItems += (DWORD)rbi.i64NumItems;
				g_nBytes += (DWORD)rbi.i64Size;
			}
			pStr = strchr(pStr, '\0');
			pStr += 1;
		}
        }else{
		SHQueryRecycleBin("", &rbi);
		if (rbi.i64NumItems > 0) {
			g_isEmpty = false;
			g_nItems += (DWORD)rbi.i64NumItems;
			g_nBytes += (DWORD)rbi.i64Size;
		}
        }
	TCHAR szSize[64];
	StrFormatByteSize64(g_nBytes, szSize, sizeof(szSize));
	if(rbCntrPtrPos != -1){
		if (g_isEmpty){
			sprintf(displayString, "Empty");
                }else{
                	if(useTwoLine){
                		if(insertSpace) sprintf(displayString, "%d%s\n%s", g_nItems, (g_nItems > 1) ? " items" : " item", szSize);
                		else sprintf(displayString, "%d%s\n%s", g_nItems, (g_nItems > 1) ? "items" : "item", szSize);
                	}else{
                		if(insertSpace) sprintf(displayString, "%d%s/%s", g_nItems, (g_nItems > 1) ? " items" : " item", szSize);
                		else sprintf(displayString, "%d%s/%s", g_nItems, (g_nItems > 1) ? "items" : "item", szSize);
                	}
                }
	}
}

//=========================================================================

struct tt
{
	struct tt *next;
	char used_flg;
	char text[256];
	TOOLINFO ti;
} *tt0;

void SetToolTip(RECT *tipRect, char *tipText)
{
	if (NULL==hToolTips) return;

	struct tt **tp, *t; unsigned n=0;
	for (tp=&tt0; NULL!=(t=*tp); tp=&t->next){
		if (0==memcmp(&t->ti.rect, tipRect, sizeof(RECT))){
			t->used_flg = 1;
			if (0!=strcmp(t->ti.lpszText, tipText)){
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
	t->ti.hwnd     = hwndPlugin;
	t->ti.uId      = n+1;
	//t->ti.hinst    = NULL;
	t->ti.lpszText = t->text;
	t->ti.rect     = *tipRect;

	SendMessage(hToolTips, TTM_ADDTOOL, 0, (LPARAM)&t->ti);
}

void ClearToolTips(void)
{
	struct tt **tp, *t;
	tp=&tt0; while (NULL!=(t=*tp)){
		if (0==t->used_flg){
			SendMessage(hToolTips, TTM_DELTOOL, 0, (LPARAM)&t->ti);
			*tp=t->next;
			m_free(t);
		}else{
			t->used_flg = 0;
			tp=&t->next;
		}
	}
}

void SetAllowTip(bool allowtip)
{
	if (NULL != hToolTips){
		SendMessage(hToolTips, TTM_ACTIVATE, (WPARAM)allowtip, 0);
	}else{
		SendMessage(hToolTips, TTM_ACTIVATE, (WPARAM)false, 0);
	}
}

//===========================================================================

bool ResizeMyWindow(int newWidth, int newHeight)
{
	if(newWidth != width || newHeight != height){
		width = newWidth;
		height = newHeight;
		if (width <= 10)  width  = 10;
		if (height <= 10) height = 10;
		if (width < height)  width = height;
	}
	else
		return false;

	if(inSlit){
		SetWindowPos(hwndPlugin, 0, 0, 0, width, height, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOMOVE);
		SendMessage(hSlit, SLIT_UPDATE, 0, 0);
		return true;
	}else{
		SetWindowPos(hwndPlugin, 0, xpos, ypos, width, height, SWP_NOACTIVATE | SWP_NOZORDER);
		return true;
	}
}

//===========================================================================
