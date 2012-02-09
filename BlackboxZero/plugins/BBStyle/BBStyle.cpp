/*
 ============================================================================
 Blackbox for Windows: BBStyle plugin
 ============================================================================
 Copyright © 2002-2003 nc-17@ratednc-17.com
 http://desktopian.org/bb/ - #bb4win on irc.freenode.net
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

#include "BBStyle.h"

// This is a must have!  The is how BBSlit knows how big it needs to be!
#define SLIT_ADD		11001
#define SLIT_REMOVE		11002
#define SLIT_UPDATE		11003

LPSTR szAppName = "BBStyle";			// The name of our window class, etc.
LPSTR szVersion = "BBStyle 0.3b4";		// Used in MessageBox titlebars

LPSTR szInfoVersion = "0.3b4";
LPSTR szInfoAuthor = "NC-17";
LPSTR szInfoRelDate = "2005-11-20";
LPSTR szInfoLink = "http://www.ratednc-17.com/";
LPSTR szInfoEmail = "nc-17@ratednc-17.com";

//===========================================================================

int beginSlitPlugin(HINSTANCE hMainInstance, HWND hBBSlit)
{
	/* Since we were loaded in the slit we need to remember the Slit
	 * HWND and make sure we remember that we are in the slit ;)
	 */
	inSlit = true;
	hSlit = hBBSlit;

	// Start the plugin like normal now..
	return beginPlugin(hMainInstance);
}

int beginPluginEx(HINSTANCE hPluginInstance, HWND hwndBBSlit) 
{  
	return beginSlitPlugin(hPluginInstance, hwndBBSlit); 
} 

//===========================================================================

int beginPlugin(HINSTANCE hPluginInstance)
{
	WNDCLASS wc;
	hwndBlackbox = GetBBWnd();
	hInstance = hPluginInstance;

	// Register the window class...
	ZeroMemory(&wc,sizeof(wc));
	wc.lpfnWndProc = WndProc;			// our window procedure
	wc.hInstance = hPluginInstance;		// hInstance of .dll
	wc.lpszClassName = szAppName;		// our window class name
	if (!RegisterClass(&wc)) 
	{
		MessageBox(hwndBlackbox, "Error registering window class", szVersion, MB_OK | MB_ICONERROR | MB_TOPMOST);
		return 1;
	}

	// Get plugin and style settings...

	InitRC();
	ReadRCSettings();
	GetStyleSettings();

	inSlit = useSlit;

	if (inheritToolbar)
	{
		//tbInfo.height = height = width = button.fontSize + 2 + (2 * styleBevelWidth) + (2 * frame.borderWidth);
		GetClientRect(FindWindow("Toolbar", 0), &tRect);
		tbInfo.height = height = width = tRect.bottom - tRect.top;
		frame.bevelWidth = styleBevelWidth;
		GetClientRect(FindWindow("Toolbar", 0), &tRect);
		tbInfo.height = height = width = tRect.bottom - tRect.top;
		frame.bevelWidth = styleBevelWidth;
		//GetStyleSettings();
	}

	// Create the window...
	hwndPlugin = CreateWindowEx(
						WS_EX_TOOLWINDOW,								// window style
						szAppName,										// our window class name
						NULL,											// NULL -> does not show up in task manager!
						WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,	// window parameters
						xpos,											// x position
						ypos,											// y position
						width,											// window width
						height,											// window height
						NULL,											// parent window
						NULL,											// no menu
						hPluginInstance,								// hInstance of .dll
						NULL);
	if (!hwndPlugin)
	{						   
		MessageBox(0, "Error creating window", szVersion, MB_OK | MB_ICONERROR | MB_TOPMOST);
		return 1;
	}

	if (!hSlit)
		hSlit = FindWindow("BBSlit", "");

	if (inSlit && hSlit)		// Are we in the Slit?
	{
		if (snapWindow) snapWindowOld = true;
		else snapWindowOld = false;
		snapWindow = false;
		SendMessage(hSlit, SLIT_ADD, NULL, (LPARAM)hwndPlugin);
		
		/* A window can not be a WS_POPUP and WS_CHILD so remove POPUP and add CHILD
		 * IF you decide to allow yourself to be unloaded from the slit, then you would
		 * do the oppisite, remove CHILD and add POPUP
		 */
		SetWindowLong(hwndPlugin, GWL_STYLE, (GetWindowLong(hwndPlugin, GWL_STYLE) & ~WS_POPUP) | WS_CHILD);

		// Make your parent window BBSlit
		SetParent(hwndPlugin, hSlit);
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

	if (osInfo.dwPlatformId == VER_PLATFORM_WIN32_NT && osInfo.dwMajorVersion == 5) 
		usingWin2kXP = true;
	else 
		usingWin2kXP = false;

	if (usingWin2kXP)
	{
		if (transparency && !inSlit)
		{
			SetWindowLong(hwndPlugin, GWL_EXSTYLE, WS_EX_TOOLWINDOW | WS_EX_LAYERED);
			BBSetLayeredWindowAttributes(hwndPlugin, NULL, (unsigned char)transparencyAlpha, LWA_ALPHA);
			//BBSetLayeredWindowAttributes(hwndPlugin, NULL, (unsigned char)"160", LWA_ALPHA);
		}
	}

	// Register to receive Blackbox messages...
	SendMessage(hwndBlackbox, BB_REGISTERMESSAGE, (WPARAM)hwndPlugin, (LPARAM)msgs);

/**
BlackboxZero 1.14.2012
**/
#ifndef GWL_USERDATA
#define GWL_USERDATA (-21)
#endif

	// Set magicDWord to make the window sticky (same magicDWord that is used by LiteStep)...
	SetWindowLong(hwndPlugin, GWL_USERDATA, magicDWord);
	// Make the window AlwaysOnTop?
	if (alwaysOnTop) SetWindowPos(hwndPlugin, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOMOVE);
	// Show the window and force it to update...
	
	if (hideWindow)
	{
		ShowWindow(hwndPlugin, SW_HIDE);
	}
	else
	{
		ShowWindow(hwndPlugin, SW_SHOW);
		InvalidateRect(hwndPlugin, NULL, true);
	}

	if (badPath)
	{
		GetBlackboxPath(listPath, sizeof(listPath));
		strcat(listPath, "styles\\*");
		char temp[4096];
		strcpy(temp, "The original style path in the bbstyle.rc file was invalid.\n\nThe default path will now be used...\n\nPath: ");
		strcat(temp, listPath);
		MessageBox(hwndBlackbox, temp, szVersion, MB_OK | MB_ICONINFORMATION | MB_TOPMOST);
		WriteRCSettings();
		badPath = false;
		//endPlugin(hInstance);
		//return 0;
	}

	// populate style list
	InitList(listPath, true);	

	// Time in milliseconds to redraw window (or whatever defined in WM_TIMER)
	//int UpdateInterval = changeTime;
	if (timer)
	{
		if (!SetTimer(hwndPlugin, 1, changeTime, (TIMERPROC)NULL))
		{
			MessageBox(0, "Error creating update timer", szVersion, MB_OK | MB_ICONERROR | MB_TOPMOST);
			//Log("Could not create Toolbar update timer", NULL);
			timer = false;
			return 1;
		}
	}

	if (changeOnStart)	// if random style on startup, grab a style and set it
	{
		NewStyle(true);
		SetStyle();
	}

	/*oldDesk = currentDesktop = 254;
	SendMessage(hwndBlackbox, BB_LISTDESKTOPS, (WPARAM)hwndPlugin, 0);*/

	return 0;
}

//===========================================================================

void endPlugin(HINSTANCE hPluginInstance)
{
	//if (timer) 
	KillTimer(hwndPlugin, 1);
	// Write the current plugin settings to the config file...
	//WriteRCSettings();

	// Delete used StyleItems...
	if (frame.style) delete frame.style;
	if (button.style) delete button.style;
	if (buttonPressed.style) delete buttonPressed.style;

	// Delete the main plugin menu if it exists (PLEASE NOTE that this takes care of submenus as well!)
	if (BBStyleMenu) DelMenu(BBStyleMenu);

	// remove from slit
	if (inSlit)
		SendMessage(hSlit, SLIT_REMOVE, NULL, (LPARAM)hwndPlugin);

	// Unregister Blackbox messages...
	SendMessage(hwndBlackbox, BB_UNREGISTERMESSAGE, (WPARAM)hwndPlugin, (LPARAM)msgs);

	//workspaceRootCommand.clear();
	styleList.clear();

	// Destroy our window...
	DestroyWindow(hwndPlugin);

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
			// Create buffer hdc's, bitmaps etc.
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hwnd, &ps);
			HDC buf = CreateCompatibleDC(NULL);
			HBITMAP bufbmp = CreateCompatibleBitmap(hdc, width, height);
			//HBITMAP oldbuf = (HBITMAP)SelectObject(buf, bufbmp);
			HGDIOBJ oldbmp = SelectObject(buf, bufbmp);
			RECT r;

			GetClientRect(hwnd, &r);

			// Paint background according to the current style...
			MakeGradient(buf, r, frame.style->type, frame.color, frame.colorTo, frame.style->interlaced, frame.style->bevelstyle, frame.style->bevelposition, frame.bevelWidth, frame.borderColor, frame.borderWidth);

			int mod;
			
			if (inheritToolbar) mod = 1;
			else mod = 0;

			r.top = r.top + (frame.bevelWidth + frame.borderWidth) + mod;
			r.left = r.left + (frame.bevelWidth + frame.borderWidth) + mod;
			r.right = r.right - (frame.bevelWidth + frame.borderWidth) - mod;
			r.bottom = r.bottom - (frame.bevelWidth + frame.borderWidth) - mod;

			buttonRect = r;

			HFONT font = CreateFont(button.fontSize, 0, 0, 0, button.fontWeight, false, false, false, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, button.fontFace);
			//SelectObject(buf, font);
			HGDIOBJ oldfont = SelectObject(buf, font);
			SetBkMode(buf, TRANSPARENT);			

			if (leftButtonDown)
			{
				if (!buttonPressed.style->parentRelative)
				{				
					MakeGradient(buf, r, buttonPressed.style->type, buttonPressed.color, buttonPressed.colorTo, buttonPressed.style->interlaced, buttonPressed.style->bevelstyle, buttonPressed.style->bevelposition, frame.bevelWidth, frame.borderColor, 0);
				}

				SetTextColor(buf, buttonPressed.fontColor);
			}
			else
			{
				if (!button.style->parentRelative)
				{						
					MakeGradient(buf, r, button.style->type, button.color, button.colorTo, button.style->interlaced, button.style->bevelstyle, button.style->bevelposition, frame.bevelWidth, frame.borderColor, 0);
				}

				SetTextColor(buf, button.fontColor);
			}

			// Draw some text...
			DrawText(buf, "S", -1, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_WORD_ELLIPSIS);

			//DeleteObject(font);
			DeleteObject(SelectObject(buf, oldfont));

			// Finally, copy from the paint buffer to the window...
			BitBlt(hdc, 0, 0, width, height, buf, 0, 0, SRCCOPY);

			// Remember to delete all objects!
/*			SelectObject(buf, oldbuf);
			DeleteDC(buf);
			DeleteObject(bufbmp);
			DeleteObject(oldbuf);
			EndPaint(hwnd, &ps);
*/
            //restore the first previous whatever to the dc,
            //get in exchange back our bitmap, and delete it.
            DeleteObject(SelectObject(buf, oldbmp));

            //delete the memory - 'device context'
            DeleteDC(buf);

            //done
            EndPaint(hwnd, &ps);

			return 0;
		}
		break;

		// ==========

		// Broadcast messages (bro@m -> the bang killah! :D <vbg>)
		case BB_BROADCAST:
		{
			char temp[MAX_LINE_LENGTH];
			strcpy(temp, (LPCSTR)lParam);

			// ==========

			// First we check for the two "global" bro@ms, @BBShowPlugins and @BBHidePlugins...
			if (!_stricmp(temp, "@BBShowPlugins")) // @BBShowPlugins: Show window and force update (global bro@m)
			{
				if (!hideWindow)
				{
					ShowWindow(hwndPlugin, SW_SHOW);
					InvalidateRect(hwndPlugin, NULL, true);
				}

				return 0;
			}
			else if (!_stricmp(temp, "@BBHidePlugins")) // @BBHidePlugins: Hide window (global bro@m)
			{
				if (!hideWindow && !inSlit)
				{
					ShowWindow(hwndPlugin, SW_HIDE);
				}

				return 0;
			}

			// ==========

			// ...then we check for the plugin bro@ms that are available to the end-user (ie. listed in the documentation! <g>)...
			else if (!_stricmp(temp, "@BBStyleRandom")) // @myBroam is "our own" bro@m to set the temporary toolbar display
			{
				NewStyle(true);
				SetStyle();				
				return 0;
			}

			// ==========

			// Finally, we check for our "internal" bro@m, which is used for
			// all plugin internal commands, for example the menu commands
			// (this "masking" technique saves a *lot* of _stricmp's in more
			// advanced plugins or when having many menu commands!)...
			else if (IsInString(temp, "@BBStyleInternal"))
			{
				static char msg[MAX_LINE_LENGTH];
				static char status[9];
				char token1[4096], token2[4096], extra[4096];
				LPSTR tokens[2];
				tokens[0] = token1;
				tokens[1] = token2;

				token1[0] = token2[0] = extra[0] = '\0';
				BBTokenize (temp, tokens, 2, extra);

				// open current style file with default editor
				if (!_stricmp(token2, "EditStyle")) 
				{
					SendMessage(hwndBlackbox, BB_EDITFILE, 0, 0);
					SendMessage(hwndBlackbox, BB_SETTOOLBARLABEL, 0, (LPARAM)"BBStyle -> Edit Style");
					return 0;
				}
				else if (!_stricmp(token2, "AboutStyle")) 
				{
					char stylepath[MAX_LINE_LENGTH], temp[MAX_LINE_LENGTH], a1[MAX_LINE_LENGTH], a2[MAX_LINE_LENGTH], a3[MAX_LINE_LENGTH], a4[MAX_LINE_LENGTH], a5[MAX_LINE_LENGTH];
					strcpy(stylepath, stylePath());
					strcpy(a1, ReadString(stylepath, "style.name:", "[Style name not specified]"));
					strcpy(a2, ReadString(stylepath, "style.author:", "[Author not specified]"));
					strcpy(a3, ReadString(stylepath, "style.date:", ""));
					strcpy(a4, ReadString(stylepath, "style.credits:", ""));
					strcpy(a5, ReadString(stylepath, "style.comments:", ""));
					sprintf(temp, "%s\nby %s\n%s\n%s\n%s", a1, a2, a3, a4, a5);
					MessageBox(GetBBWnd(), temp, "BBStyle: Style information", MB_OK | MB_SETFOREGROUND | MB_ICONINFORMATION);
					return 0;
				}
				else if (!_stricmp(token2, "ToggleTimer"))
				{
					if (timer)
					{
						KillTimer(hwndPlugin, 1);
						timer = false;
						SendMessage(hwndBlackbox, BB_SETTOOLBARLABEL, 0, (LPARAM)"BBStyle -> Style Change On Timer disabled");
					}
					else
					{
						if (!SetTimer(hwndPlugin, 1, changeTime, (TIMERPROC)NULL))
						{
							MessageBox(0, "Error creating update timer", szVersion, MB_OK | MB_ICONERROR | MB_TOPMOST);
							//Log("Could not create Toolbar update timer", NULL);
							timer = false;
							return 0;
						}
						timer = true;
						SendMessage(hwndBlackbox, BB_SETTOOLBARLABEL, 0, (LPARAM)"BBStyle -> Style Change On Timer enabled");
					}
				}
				else if (!_stricmp(token2, "RefreshList"))
				{
					InitList(listPath, true);
					NewStyle(true);
					//MessageBox(0, styleToSet, "Before NewStyle()...", MB_OK);
					//NewStyle();
					SendMessage(hwndBlackbox, BB_SETTOOLBARLABEL, 0, (LPARAM)"BBStyle -> Refresh Style List");
					return 0;
				}
				else if (!_stricmp(token2, "AlwaysOnTop"))
				{
					if (!inSlit)
					{
						if (alwaysOnTop)
						{
							alwaysOnTop = false;
							SetWindowPos(hwndPlugin, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOMOVE);
						}
						else
						{
							alwaysOnTop = true;
							SetWindowPos(hwndPlugin, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOMOVE);
						}

						(alwaysOnTop) ? strcpy(status, "enabled") : strcpy(status, "disabled");
						sprintf(msg, "BBStyle -> Always On Top %s", status);
						SendMessage(hwndBlackbox, BB_SETTOOLBARLABEL, 0, (LPARAM)msg);
					}
				}
				else if (!_stricmp(token2, "ToggleStartChange"))
				{
					if (changeOnStart) changeOnStart = false;
					else changeOnStart = true;

					(changeOnStart) ? strcpy(status, "enabled") : strcpy(status, "disabled");
					sprintf(msg, "BBStyle -> Style Change On Start %s", status);
					SendMessage(hwndBlackbox, BB_SETTOOLBARLABEL, 0, (LPARAM)msg);
				}
				else if (!_stricmp(token2, "ToggleStyleTwice"))
				{
					noStyleTwice = !noStyleTwice;

					if (noStyleTwice)
					{
						strcpy(status, "enabled");
					}
					else // if turning off no style twice, repopulate list
					{
						strcpy(status, "disabled");
						InitList(listPath, true);
						NewStyle(true);
					}

					sprintf(msg, "BBStyle -> No Style Twice %s", status);
					SendMessage(hwndBlackbox, BB_SETTOOLBARLABEL, 0, (LPARAM)msg);
				}
				/*else if (!_stricmp(token2, "ToggleRootCommands"))
				{
					if (rootCommands)
					{
						rootCommands = false;

						// Execute the style's rootCommand...
						char command[MAX_LINE_LENGTH], arguments[MAX_LINE_LENGTH];
						LPSTR tokens[1];
						tokens[0] = command;
						command[0] = arguments[0] = '\0';

						BBTokenize (styleRootCommand, tokens, 1, arguments);

						// Since it's a rootCommand, we use the Blackbox dir as working directory...
						char bbpath[MAX_LINE_LENGTH];
						GetBlackboxPath(bbpath, sizeof(bbpath));

						BBExecute(GetDesktopWindow(), NULL, command, arguments, bbpath, SW_SHOWNORMAL, true);
					}
					else 
					{
						rootCommands = true;
						oldDesk = currentDesktop;
						SendMessage(hwndBlackbox, BB_LISTDESKTOPS, (WPARAM)hwndPlugin, 0);
						ChangeRoot();
					}

					(rootCommands) ? strcpy(status, "enabled") : strcpy(status, "disabled");
					sprintf(msg, "BBStyle -> Workspace RootCommands %s", status);
					SendMessage(hwndBlackbox, BB_SETTOOLBARLABEL, 0, (LPARAM)msg);
				}*/
				else if (!_stricmp(token2, "ToggleWindow"))
				{
					if (!inSlit)
					{
						if (hideWindow)
						{
							hideWindow = false;
							ShowWindow(hwndPlugin, SW_SHOW);
							InvalidateRect(hwndPlugin, NULL, true);
						}
						else 
						{
							hideWindow = true;
							ShowWindow(hwndPlugin, SW_HIDE);
						}

						(hideWindow) ? strcpy(status, "enabled") : strcpy(status, "disabled");
						sprintf(msg, "BBStyle -> Hide Window %s", status);
						SendMessage(hwndBlackbox, BB_SETTOOLBARLABEL, 0, (LPARAM)msg);
					}
				}
				else if (!_stricmp(token2, "SnapWindowToEdge"))
				{
					if (!inSlit)
					{
						if (snapWindow) snapWindow = false;
						else snapWindow = true;

						(snapWindow) ? strcpy(status, "enabled") : strcpy(status, "disabled");
						sprintf(msg, "BBStyle -> Snap Window To Edge %s", status);
						SendMessage(hwndBlackbox, BB_SETTOOLBARLABEL, 0, (LPARAM)msg);
					}
				}
				else if (!_stricmp(token2, "ToggleSlit"))
				{
					ToggleSlit();
					return 0;
				}
				else if (!_stricmp(token2, "ToggleBorder"))
				{
					if (frame.drawBorder)
					{
						frame.drawBorder = false;
					}
					else
					{
						frame.drawBorder = true;
					}

					GetStyleSettings();
					InvalidateRect(hwndPlugin, NULL, true);
					if (hSlit && inSlit)
						SendMessage(hSlit, SLIT_UPDATE, 0, 0);					

					(frame.drawBorder) ? strcpy(status, "enabled") : strcpy(status, "disabled");
					sprintf(msg, "BBStyle -> Draw Border %s", status);
					SendMessage(hwndBlackbox, BB_SETTOOLBARLABEL, 0, (LPARAM)msg);					
				}
				else if (!_stricmp(token2, "ToggleToolbar"))
				{
					if (inheritToolbar) 
					{
						inheritToolbar = false;
						/*height = width = 21 + (2 * frame.borderWidth);
						frame.bevelWidth = 3;*/
						GetStyleSettings();
						MoveWindow(hwndPlugin, xpos, ypos, width, height, true);
						InvalidateRect(hwndPlugin, NULL, true);
						if (hSlit && inSlit)
							SendMessage(hSlit, SLIT_UPDATE, 0, 0);
					}
					else 
					{
						inheritToolbar = true;
						//height = width = tbInfo.height;
						if (GetClientRect(FindWindow("Toolbar", 0), &tRect))
						{
							//GetClientRect(FindWindow("Toolbar", 0), &tRect);
							tbInfo.height = height = width = tRect.bottom - tRect.top;
							frame.bevelWidth = styleBevelWidth;
							GetClientRect(FindWindow("Toolbar", 0), &tRect);
							tbInfo.height = height = width = tRect.bottom - tRect.top;
							frame.bevelWidth = styleBevelWidth;
							//GetStyleSettings();
							MoveWindow(hwndPlugin, xpos, ypos, width, height, true);
							InvalidateRect(hwndPlugin, NULL, true);
							if (hSlit && inSlit)
								SendMessage(hSlit, SLIT_UPDATE, 0, 0);
						}
						else if (GetClientRect(FindWindow("BBToolbar", 0), &tRect))
						{
							//GetClientRect(FindWindow("Toolbar", 0), &tRect);
							tbInfo.height = height = width = tRect.bottom - tRect.top;
							frame.bevelWidth = styleBevelWidth;
							GetClientRect(FindWindow("BBToolbar", 0), &tRect);
							tbInfo.height = height = width = tRect.bottom - tRect.top;
							frame.bevelWidth = styleBevelWidth;
							//GetStyleSettings();
							MoveWindow(hwndPlugin, xpos, ypos, width, height, true);
							InvalidateRect(hwndPlugin, NULL, true);
							if (hSlit && inSlit)
								SendMessage(hSlit, SLIT_UPDATE, 0, 0);
						}
						else if (GetClientRect(FindWindow("bbLeanBar", 0), &tRect))
						{
							//GetClientRect(FindWindow("Toolbar", 0), &tRect);
							tbInfo.height = height = width = tRect.bottom - tRect.top;
							frame.bevelWidth = styleBevelWidth;
							GetClientRect(FindWindow("bbLeanBar", 0), &tRect);
							tbInfo.height = height = width = tRect.bottom - tRect.top;
							frame.bevelWidth = styleBevelWidth;
							//GetStyleSettings();
							MoveWindow(hwndPlugin, xpos, ypos, width, height, true);
							InvalidateRect(hwndPlugin, NULL, true);
							if (hSlit && inSlit)
								SendMessage(hSlit, SLIT_UPDATE, 0, 0);
						}
						else
						{
							inheritToolbar = false;
						}
					}

					(inheritToolbar) ? strcpy(status, "enabled") : strcpy(status, "disabled");
					sprintf(msg, "BBStyle -> Inherit Toolbar Height %s", status);
					SendMessage(hwndBlackbox, BB_SETTOOLBARLABEL, 0, (LPARAM)msg);
				}
				else if (!_stricmp(token2, "Transparency"))
				{
					if (usingWin2kXP)
					{
						if (transparency)
						{
							transparency = false;
							SetWindowLong(hwndPlugin, GWL_EXSTYLE, WS_EX_TOOLWINDOW);
						}
						else
						{
							transparency = true;
							SetWindowLong(hwndPlugin, GWL_EXSTYLE, WS_EX_TOOLWINDOW | WS_EX_LAYERED);
							BBSetLayeredWindowAttributes(hwndPlugin, NULL, (unsigned char)transparencyAlpha, LWA_ALPHA);
						}

						(transparency) ? strcpy(status, "enabled") : strcpy(status, "disabled");
						sprintf(msg, "BBStyle -> Transparency %s", status);
						SendMessage(hwndBlackbox, BB_SETTOOLBARLABEL, 0, (LPARAM)msg);
					}
				}
				else if (!_stricmp(token2, "About"))
				{
					char aboutmsg[MAX_LINE_LENGTH];

					strcpy(aboutmsg, szVersion);
					strcat(aboutmsg, "\n\n© 2005 nc-17@ratednc-17.com   \n\nhttp://www.ratednc-17.com/\n#bb4win on irc.freenode.net");

					/*SendMessage(hwndBlackbox, BB_SETTOOLBARLABEL, 0, (LPARAM)"BBStyle -> About BBStyle");
					MessageBox(0, aboutmsg, "About BBStyle...", MB_OK | MB_TOPMOST | MB_SETFOREGROUND | MB_ICONINFORMATION);
					return 0;*/

					MSGBOXPARAMS msgbox;
					ZeroMemory(&msgbox, sizeof(msgbox));
					msgbox.cbSize = sizeof(msgbox);
					msgbox.hwndOwner = 0; //GetBBWnd();
					msgbox.hInstance = hInstance;
					msgbox.lpszText = aboutmsg;
					msgbox.lpszCaption = "About BBStyle...";
					msgbox.dwStyle = MB_OK | MB_SETFOREGROUND | MB_TOPMOST | MB_USERICON;
					msgbox.lpszIcon = MAKEINTRESOURCE(IDI_ICON1);
					msgbox.dwContextHelpId = 0;
					msgbox.lpfnMsgBoxCallback = NULL;
					msgbox.dwLanguageId = LANG_NEUTRAL;
					MessageBoxIndirect(&msgbox);
					return 0;
				}
			}

			WriteRCSettings();
			return 0;
		}
		break;

		// ==========

		// If Blackbox sends a reconfigure message, load the new style settings and update the window...
		case BB_RECONFIGURE:
		{
			ReadRCSettings();
			GetStyleSettings();
			leftButtonDown = false;

			UpdateMonitorInfo();
			if (!inSlit)
			{
				// snap to edge on style change
				// stops diff. bevel/border widths moving pager from screen edge
				int x; int y; int z; int dx, dy, dz;
				int nDist = 20;

				// top/bottom edge
				dy = y = ypos - screenTop;
				dz = z = ypos + height - screenBottom;
				if (dy<0) dy=-dy;
				if (dz<0) dz=-dz;
				if (dz < dy) y = z, dy = dz;

				// left/right edge
				dx = x = xpos - screenLeft;
				dz = z = xpos + width - screenRight;
				if (dx<0) dx=-dx;
				if (dz<0) dz=-dz;
				if (dz < dx) x = z, dx = dz;

				if (dy < nDist)
				{
					ypos -= y;
					// top/bottom center
					dz = z = xpos + (width - screenRight - screenLeft)/2;
					if (dz<0) dz=-dz;
					if (dz < dx) x = z, dx = dz;
				}

				if (dx < nDist)
					xpos -= x;

				SetWindowPos(hwndPlugin, HWND_TOP, xpos, ypos, width, height, SWP_NOZORDER);
			}

			InvalidateRect(hwndPlugin, NULL, true);
			//currentDesktop = 254;

			if (hSlit && inSlit)
				SendMessage(hSlit, SLIT_UPDATE, 0, 0);

			if (badPath)
			{
				GetBlackboxPath(listPath, sizeof(listPath));
				strcat(listPath, "styles\\*");
				char temp[4096];
				strcpy(temp, "The original style path in the bbstyle.rc file was invalid.\n\nThe default path will now be used...\n\nPath: ");
				strcat(temp, listPath);
				MessageBox(hwndBlackbox, temp, szVersion, MB_OK | MB_ICONINFORMATION | MB_TOPMOST);
				WriteRCSettings();
				badPath = false;
				//endPlugin(hInstance);
				//return 0;
			}

			//SendMessage(hwndBlackbox, BB_LISTDESKTOPS, (WPARAM)hwndPlugin, 0);
			//MessageBox(0, "Reconfigure...?", szVersion, MB_OK | MB_ICONERROR | MB_TOPMOST);
		}
		break;

		// ==========

		case WM_WINDOWPOSCHANGING:
		{
			// Is SnapWindowToEdge enabled?
			if (!inSlit)
			{
				if (snapWindow)
				{
					// Snap window to screen edges (if the last bool is false it uses the current DesktopArea)
					if(IsWindowVisible(hwnd)) SnapWindowToEdge((WINDOWPOS*)lParam, 20, true);
				}
			}
		}
		break;

		// ==========
		// Save window position if it changes...

		case WM_WINDOWPOSCHANGED:
		{
			if (!inSlit)
			{
				WINDOWPOS* windowpos = (WINDOWPOS*)lParam;
				xpos = windowpos->x;
				ypos = windowpos->y;

				WriteInt(rcpath, "bbstyle.position.x:", xpos);
				WriteInt(rcpath, "bbstyle.position.y:", ypos);
			}
			
			return 0;
		}
		break;

		case WM_MOVING:
		{
			RECT *r = (RECT *)lParam;

			// Makes sure BBStyle stays on screen when moving.
			if (r->left < vScreenLeft)
			{
				r->left = vScreenLeft;
				r->right = r->left + width;
			}
			if ((r->left + width) > vScreenWidth)
			{
				r->left = vScreenWidth - width;
				r->right = r->left + width;
			}

			if (r->top < vScreenTop) 
			{
				r->top = vScreenTop;
				r->bottom = r->top + height;
			}
			if ((r->top + height) > vScreenHeight)
			{
				r->top = vScreenHeight - height;
				r->bottom = r->top + height;
			}

			return TRUE;
		}
		break;

		// ==========

		case WM_DISPLAYCHANGE:  
		{ 
			// IntelliMove(tm) by qwilk... <g> 
			int relx, rely;  
			int oldscreenwidth = screenWidth; 
			int oldscreenheight = screenHeight;

			/*screenLeft = GetSystemMetrics(SM_YVIRTUALSCREEN);
			screenTop = GetSystemMetrics(SM_XVIRTUALSCREEN);
			
			screenWidth = screenLeft + GetSystemMetrics(SM_CXVIRTUALSCREEN);
			screenHeight = screenTop + GetSystemMetrics(SM_CYVIRTUALSCREEN);*/
			UpdateMonitorInfo();

			if (xpos > oldscreenwidth / 2)  
			{ 
				relx = oldscreenwidth - xpos;  
				xpos = screenWidth - relx; 
			} 

			if (ypos > oldscreenheight / 2)  
			{
				rely = oldscreenheight - ypos;  
				ypos = screenHeight - rely; 
			} 
			
			MoveWindow(hwndPlugin, xpos, ypos, width, height, true);  
		}
		break; 

		//====================
		// Timer message:

		case WM_TIMER:
		{
			// Change to a new style!
			NewStyle(true);
			SetStyle();						
			return 0;
		}

		// ====================
		// Grabs the number of desktops and sets the names of the desktops

		/*case BB_DESKTOPINFO:
		{
			DesktopInfo* info = (DesktopInfo*)lParam;

			if (info->isCurrent)
			{
				oldDesk = currentDesktop;
				currentDesktop = info->number;
				if (oldDesk != currentDesktop)
					ChangeRoot();
			}
		}
		break;*/

		// ====================
		// Get toolbar height and use it if inherit option on

		case BB_TOOLBARUPDATE:
		{
			//GetToolbarInfo(&tbInfo);

			if (inheritToolbar)
			{
				GetClientRect(FindWindow("Toolbar", 0), &tRect);
				tbInfo.height = height = width = tRect.bottom - tRect.top;
				frame.bevelWidth = styleBevelWidth;
				GetClientRect(FindWindow("Toolbar", 0), &tRect);
				tbInfo.height = height = width = tRect.bottom - tRect.top;
				frame.bevelWidth = styleBevelWidth;
				//GetStyleSettings();
				MoveWindow(hwndPlugin, xpos, ypos, width, height, true);
				InvalidateRect(hwndPlugin, NULL, true);
				if (hSlit && inSlit)
					SendMessage(hSlit, SLIT_UPDATE, 0, 0);
			}

			//SendMessage(hwndBlackbox, BB_LISTDESKTOPS, (WPARAM)hwndPlugin, 0);
		}
		break;

		// ====================

		/*case BB_WORKSPACE:
		case BB_BRINGTOFRONT:
		case BB_MINIMIZE:
		{
			PostMessage(hwndBlackbox, BB_LISTDESKTOPS, (WPARAM)hwndPlugin, 0);
		}
		break;*/

		//===========
		// Allow window to move...

		case WM_NCHITTEST:
		{
			//ClickMouse();

			//if (!inButton || GetAsyncKeyState(VK_CONTROL) & 0x8000)
			if (!ClickMouse() || GetAsyncKeyState(VK_CONTROL) & 0x8000)
				return HTCAPTION;
			else
				return HTCLIENT;
		}
		break;

		// ==========

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
			if (!inSlit)// && (GetAsyncKeyState(VK_CONTROL) & 0x8000))
				return DefWindowProc(hwnd, message, wParam, lParam);
		}
		break;

		// ==========

		case WM_LBUTTONDOWN:
		//case WM_NCLBUTTONDOWN:
		{
				leftButtonDown = true;
				InvalidateRect(hwndPlugin, NULL, true);
		}
		break;

		case WM_LBUTTONUP:
		//case WM_NCLBUTTONUP:
		{
			if (leftButtonDown)
			{
				leftButtonDown = false;
				InvalidateRect(hwndPlugin, NULL, true);
				NewStyle(true);
				SetStyle();									
			}
		}

		// ===========

		case WM_NCMBUTTONDOWN:
		{
			middleButtonDownNC = true;
		}
		break;

		case WM_MBUTTONDOWN: 
		{
			middleButtonDown = true;
		}
		break;

		case WM_NCMBUTTONUP:
		{
			if (middleButtonDownNC)
			{
				SendMessage(hwndBlackbox, BB_EDITFILE, 0, 0);
				SendMessage(hwndBlackbox, BB_SETTOOLBARLABEL, 0, (LPARAM)"BBStyle -> Edit Style");
				middleButtonDownNC = false;
			}			
		}
		break;

		case WM_MBUTTONUP:
		{	
			if (middleButtonDown)
			{
				SendMessage(hwndBlackbox, BB_EDITFILE, 0, 0);
				SendMessage(hwndBlackbox, BB_SETTOOLBARLABEL, 0, (LPARAM)"BBStyle -> Edit Style");
				middleButtonDown = false;
			}			
		}
		break;

		// ===========
		// Right mouse button clicked?

		case WM_NCRBUTTONDOWN:
		{
			rightButtonDownNC = true;
		}
		break;

		case WM_RBUTTONDOWN: 
		{
			rightButtonDown = true;
		}
		break;

		case WM_NCRBUTTONUP:
		{
			if (rightButtonDownNC)
			{
				DisplayMenu();
				rightButtonDownNC = false;
			}			
		}
		break;

		case WM_RBUTTONUP:
		{	
			if (rightButtonDown)
			{
				DisplayMenu();
				rightButtonDown = false;
			}			
		}
		break;

		// ===================
		// reset pressed mouse button statuses if mouse leaves the BBPager window

		case WM_MOUSELEAVE:
		{
			//if (!inSlit)
			//{
				leftButtonDown = middleButtonDown = rightButtonDown = false;
				InvalidateRect(hwndPlugin, NULL, true);
			//}
		}
		break;


		case WM_NCMOUSELEAVE:
		{
			//if (!inSlit)
			//{
				middleButtonDownNC = rightButtonDownNC = false;
				InvalidateRect(hwndPlugin, NULL, true);
			//}
		}
		break;

		// ==========

		case WM_MOUSEMOVE:
		case WM_NCMOUSEMOVE:
		{	
			TrackMouse();
		}
		break;

		// ==========

		case WM_CLOSE:
			return 0;
		break;

		default:
			return DefWindowProc(hwnd, message, wParam, lParam);
	}
	return 0;
}

//===========================================================================

void ToggleSlit()
{
	static char msg[MAX_LINE_LENGTH];
	static char status[9];

	hSlit = FindWindow("BBSlit", "");

	if (inSlit)
	{
		// We are in the slit, so lets unload and get out..
		if (snapWindowOld) snapWindow = true;
		//SetParent(hwndPlugin, NULL);
		//SetWindowLong(hwndPlugin, GWL_STYLE, (GetWindowLong(hwndPlugin, GWL_STYLE) & ~WS_CHILD) | WS_POPUP);
		SendMessage(hSlit, SLIT_REMOVE, NULL, (LPARAM)hwndPlugin);
		inSlit = false;

		//turn trans back on?
		if (transparency && usingWin2kXP)
		{
			SetWindowLong(hwndPlugin, GWL_EXSTYLE, WS_EX_TOOLWINDOW | WS_EX_LAYERED);
			BBSetLayeredWindowAttributes(hwndPlugin, NULL, (unsigned char)transparencyAlpha, LWA_ALPHA);
		}

		// Here you can move to where ever you want ;)
		SetWindowPos(hwndPlugin, NULL, xpos2, ypos2, 0, 0, SWP_NOZORDER | SWP_NOSIZE);		
	}
	/* Make sure before you try and load into the slit that you have
	 * the HWND of the slit ;)
	 */
	else if (hSlit)
	{
		// (Back) into the slit..
		if (snapWindow) snapWindowOld = true;
		else snapWindowOld = false;
		snapWindow = false;
		xpos2 = xpos; ypos2 = ypos;

		//turn trans off
		SetWindowLong(hwndPlugin, GWL_EXSTYLE, WS_EX_TOOLWINDOW);

		//SetParent(hwndPlugin, hSlit);
		//SetWindowLong(hwndPlugin, GWL_STYLE, (GetWindowLong(hwndPlugin, GWL_STYLE) & ~WS_POPUP) | WS_CHILD);
		SendMessage(hSlit, SLIT_ADD, NULL, (LPARAM)hwndPlugin);
		inSlit = true;
	}

	if (hSlit)
	{
		(inSlit) ? strcpy(status, "enabled") : strcpy(status, "disabled");
		sprintf(msg, "BBStyle -> Use Slit %s", status);
		SendMessage(hwndBlackbox, BB_SETTOOLBARLABEL, 0, (LPARAM)msg);
		WriteBool(rcpath, "bbstyle.useSlit:", inSlit);
	}
}

//===========================================================================

bool ClickMouse()
{
	RECT r;
	POINT mousepos;
	GetCursorPos(&mousepos);
	GetWindowRect(hwndPlugin, &r);
	mousepos.x = mousepos.x - r.left;
	mousepos.y = mousepos.y - r.top;

	// check if mouse cursor is within button RECT
	if (PtInRect(&buttonRect, mousepos) != 0)
		inButton = true; // if so, set inButton bool
	else
		inButton = false;
	
	return inButton;
}

void TrackMouse()
{
	TRACKMOUSEEVENT track;
	ZeroMemory(&track,sizeof(track));
	track.cbSize = sizeof(track);
	track.dwFlags = TME_LEAVE;
	track.dwHoverTime = HOVER_DEFAULT;
	track.hwndTrack = hwndPlugin;
	TrackMouseEvent(&track);

	TRACKMOUSEEVENT track2;
	ZeroMemory(&track2,sizeof(track2));
	track2.cbSize = sizeof(track2);
	track2.dwFlags = TME_LEAVE | TME_NONCLIENT;
	track2.dwHoverTime = HOVER_DEFAULT;
	track2.hwndTrack = hwndPlugin;
	TrackMouseEvent(&track2);
}

//===========================================================================
/*
void ChangeRoot()
{
	if (rootCommands)
	{
		char bgcmd[MAX_LINE_LENGTH];
		
		vector <string>::size_type i;
		i = workspaceRootCommand.size();

		if ((int)i >= currentDesktop + 1)
			strcpy(bgcmd, workspaceRootCommand[currentDesktop].c_str());
		else
			strcpy(bgcmd, styleRootCommand);

		// Execute the style's rootCommand...
		char command[MAX_LINE_LENGTH], arguments[MAX_LINE_LENGTH];
		LPSTR tokens[1];
		tokens[0] = command;
		command[0] = arguments[0] = '\0';

		BBTokenize (bgcmd, tokens, 1, arguments);

		// Since it's a rootCommand, we use the Blackbox dir as working directory...
		char bbpath[MAX_LINE_LENGTH];
		GetBlackboxPath(bbpath, sizeof(bbpath));
		
		BBExecute(GetDesktopWindow(), NULL, command, arguments, bbpath, SW_SHOWNORMAL, true);
	}
}
*/
//===========================================================================

void DisplayMenu()
{
	// First we delete the main plugin menu if it exists (PLEASE NOTE that this takes care of submenus as well!)
	if (BBStyleMenu) DelMenu(BBStyleMenu);

	// Then we create the main plugin menu with a few commands...
	BBStyleMenu = MakeMenu("BBStyle");

	// Next, we create the first submenu (with a few applications)...
	BBStyleWindowSubMenu = MakeMenu("Window");

		if (!inSlit)
		{
			MakeMenuItem(BBStyleWindowSubMenu, "Always On Top", "@BBStyleInternal AlwaysOnTop", alwaysOnTop);
			MakeMenuItem(BBStyleWindowSubMenu, "Snap Window To Edge", "@BBStyleInternal SnapWindowToEdge", snapWindow);
		}
		MakeMenuItem(BBStyleWindowSubMenu, "Inherit Toolbar Height", "@BBStyleInternal ToggleToolbar", inheritToolbar);
		MakeMenuItem(BBStyleWindowSubMenu, "Draw Border", "@BBStyleInternal ToggleBorder", frame.drawBorder);
		if (usingWin2kXP && !inSlit) MakeMenuItem(BBStyleWindowSubMenu, "Transparency", "@BBStyleInternal Transparency", transparency);
		if (!inSlit) MakeMenuItem(BBStyleWindowSubMenu, "Hide Window", "@BBStyleInternal ToggleWindow", hideWindow);
		if ((hSlit = FindWindow("BBSlit", ""))) 
			MakeMenuItem(BBStyleWindowSubMenu, "Use Slit", "@BBStyleInternal ToggleSlit", inSlit);

	MakeSubmenu(BBStyleMenu, BBStyleWindowSubMenu, "Window");

	// ...the second submenu (with configuration options, please note the "internal" bro@m!)...
	BBStyleStyleSubMenu = MakeMenu("Style");
		MakeMenuItem(BBStyleStyleSubMenu, "About Current Style", "@BBStyleInternal AboutStyle", false);
		MakeMenuItem(BBStyleStyleSubMenu, "Edit Current Style", "@BBStyleInternal EditStyle", false);
		MakeMenuItem(BBStyleStyleSubMenu, "Apply Random Style", "@BBStyleRandom", false);
	MakeSubmenu(BBStyleMenu, BBStyleStyleSubMenu, "Style");
	
	BBStyleConfigSubMenu = MakeMenu("Configuration");
		MakeMenuItem(BBStyleConfigSubMenu, "Style Change On Timer", "@BBStyleInternal ToggleTimer", timer);
		MakeMenuItem(BBStyleConfigSubMenu, "Style Change On Startup", "@BBStyleInternal ToggleStartChange", changeOnStart);
		MakeMenuItem(BBStyleConfigSubMenu, "No Style Twice", "@BBStyleInternal ToggleStyleTwice", noStyleTwice);
		//MakeMenuItem(BBStyleConfigSubMenu, "Workspace RootCommands", "@BBStyleInternal ToggleRootCommands", rootCommands);
	MakeSubmenu(BBStyleMenu, BBStyleConfigSubMenu, "Configuration");

	MakeMenuItem(BBStyleMenu, "Refresh Style List", "@BBStyleInternal RefreshList", false);

	// ...an "About" box is always nice, even though we now have the regular plugin info function...
	MakeMenuItem(BBStyleMenu, "About BBStyle...", "@BBStyleInternal About", false);

	// Finally, we show the menu...
	ShowMenu(BBStyleMenu);
}

//===========================================================================

void InitList(char *path, bool init)
{
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind;
	char buf[4096];

	if (init)
	{
		styleList.clear();

		if (listPath[strlen(listPath)-1] != '*')
		{
			if (listPath[strlen(listPath)-1] == '\\')
				strcat(listPath, "*");
			else
				strcat(listPath, "\\*");
		}
	}

	hFind = FindFirstFile(path, &FindFileData);

	while (FindNextFile(hFind, &FindFileData) != 0)
	{
		// Skip hidden files... and 3dc stuff
		if (   strcmp(FindFileData.cFileName, ".") == 0 || strcmp(FindFileData.cFileName, "..") == 0 
			|| (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) 
			|| IsInString(FindFileData.cFileName, ".3dc")  )
				continue;
		
		// if directory, recurse...
		if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			strcpy(buf, path);
			buf[strlen(buf)-1] = '\0';
			strcat(buf, FindFileData.cFileName);
			strcat(buf, "\\*");
			InitList(buf, false);
		}
		else
		{
			strcpy(buf, path);
			buf[strlen(buf)-1] = '\0';
			strcat(buf, FindFileData.cFileName);
			styleList.push_back(string(buf));
		}
	}

	FindClose(hFind);
}

void NewStyle(bool seed)
{
	char oldStyle[4096];
	
	unsigned int num;
	num = styleList.size();
	if (num < 1)
	{
		char temp[4096];
		strcpy(temp, "No styles in list.\n\nPath: ");
		strcat(temp, listPath);
		MessageBox(0, temp, szVersion, MB_OK);
		//MessageBox(0, listPath, szVersion, MB_OK);
		return;
	}

	if (seed)
	{
		SYSTEMTIME time;
		GetLocalTime(&time);
		srand((unsigned)time.wMilliseconds);
	}
	
	int random = rand();

	random = random % num;

	if (random == num)
		random--;

	// save old style's path
	strcpy(oldStyle, styleToSet);

	// get new style's path
	strcpy(styleToSet, styleList[random].c_str());

	// if 'no styles twice', remove style path from list
	if (noStyleTwice)
	{
		styleList.erase(styleList.begin() + random);
		if (styleList.size() < 1)	// if list empty, repopulate
			InitList(listPath, true);
	}
	// otherwise, if same style picked, try again
	else if (!strcmp(oldStyle, styleToSet) && num > 1)
		NewStyle(false);
}

void SetStyle()
{
	if (styleList.size() < 1)	// dont try to set a style if list empty
		return;

	// Look for a line that should "always" exist in a style file...
	if (strlen(ReadString(styleToSet, "menu.frame:", "")) || strlen(ReadString(styleToSet, "menu.frame.appearance:", "")))
	{
		// Set new style...
		SendMessage(hwndBlackbox, BB_SETSTYLE, 0, (LPARAM)styleToSet);
		SendMessage(hwndBlackbox, BB_SETTOOLBARLABEL, 0, (LPARAM)"BBStyle -> Apply random style");
		if (timer)
			SetTimer(hwndPlugin, 1, changeTime, (TIMERPROC)NULL);
	}
	else
	{
		char temp[4096];

		sprintf(temp, "BBStyle couldn't set the following file as a style:\n\n%s\n\nIt didn't contain a \"menu.frame:\" or \"menu.frame.appearance:\"definition. Please make sure your styles path contains only style files.", styleToSet);

		MessageBox(0, temp, szVersion, MB_OK | MB_TOPMOST | MB_ICONINFORMATION);
	}
}

//===========================================================================

void GetStyleSettings()
{
	char tempstyle[MAX_LINE_LENGTH];
	
	// Get the path to the current style file from Blackbox...
	strcpy(stylepath, stylePath());

	// FRAME - COLOURS
	frame.color = ReadColor(stylepath, "toolbar.color:", "#000000");
	frame.colorTo = ReadColor(stylepath, "toolbar.colorTo:", "#FFFFFF");
	frame.borderColor = ReadColor(stylepath, "borderColor:", "#000000");

	// FRAME - STYLE
	strcpy(tempstyle, ReadString(stylepath, "toolbar:", "Flat Gradient Vertical"));
	if (frame.style) delete frame.style;
	frame.style = new StyleItem;
	ParseItem(tempstyle, frame.style);

	// ...and some additional parameters
	styleBevelWidth = ReadInt(stylepath, "bevelWidth:", 2);

	if (!inheritToolbar)
		frame.bevelWidth = 3; //ReadInt(stylepath, "bevelWidth:", 1); // try fixed now...
	else
		frame.bevelWidth = styleBevelWidth;

	if (frame.drawBorder)
		frame.borderWidth = ReadInt(stylepath, "borderWidth:", 1);
	else
		frame.borderWidth = 0;
	
	// BUTTON - STYLE
	strcpy(tempstyle, ReadString(stylepath, "toolbar.button:", "Flat Gradient Vertical"));
	if (button.style) delete button.style;
	button.style = new StyleItem;
	ParseItem(tempstyle, button.style);
	
	// BUTTON - COLOURS
	button.color = ReadColor(stylepath, "toolbar.button.color:", "#000000");
	button.colorTo = ReadColor(stylepath, "toolbar.button.colorTo:", "#FFFFFF");

	// BUTTON - FONT
	strcpy(button.fontFace, ReadString(stylepath, "toolbar.font:", ""));
	if (!_stricmp(button.fontFace, "")) strcpy(button.fontFace, ReadString(stylepath, "*font:", "Tahoma"));
	button.fontSize = ReadInt(stylepath, "toolbar.fontHeight:", 666);
	if (button.fontSize == 666)
		button.fontSize = ReadInt(stylepath, "*fontHeight:", 12);

	char temp[32];
	strcpy(temp, ReadString(stylepath, "toolbar.fontWeight:", ""));

	if (!_stricmp(temp, "")) 
		strcpy(temp, ReadString(stylepath, "*fontWeight:", "normal"));

	if (!_stricmp(temp, "bold"))
		button.fontWeight = FW_BOLD;
	else
		button.fontWeight = FW_NORMAL;

	strcpy(tempstyle, ReadString(stylepath, "toolbar.button.picColor:", "doesntexist"));
	if (strcmp(tempstyle, "doesntexist") == 0)
		button.fontColor = ReadColor(stylepath, "toolbar.textColor:", "#FFFFFF");
	else
		button.fontColor = ReadColor(stylepath, "toolbar.button.picColor:", "#FFFFFF");

	// BUTTON PRESSED - STYLE
	strcpy(tempstyle, ReadString(stylepath, "toolbar.button.pressed:", "Flat Gradient Vertical"));
	if (buttonPressed.style) delete buttonPressed.style;
	buttonPressed.style = new StyleItem;
	ParseItem(tempstyle, buttonPressed.style);

	// BUTTON PRESSED - COLOURS
	buttonPressed.color = ReadColor(stylepath, "toolbar.button.pressed.color:", "#000000");
	buttonPressed.colorTo = ReadColor(stylepath, "toolbar.button.pressed.colorTo:", "#FFFFFF");

	strcpy(tempstyle, ReadString(stylepath, "toolbar.button.pressed.picColor:", "doesntexist"));
	if (strcmp(tempstyle, "doesntexist") == 0)
	{
		strcpy(tempstyle, ReadString(stylepath, "toolbar.button.picColor:", "doesntexist"));
		if (strcmp(tempstyle, "doesntexist") == 0)
			buttonPressed.fontColor = ReadColor(stylepath, "toolbar.textColor:", "#FFFFFF");
		else
			buttonPressed.fontColor = ReadColor(stylepath, "toolbar.button.picColor:", "#FFFFFF");
	}
	else
		buttonPressed.fontColor = ReadColor(stylepath, "toolbar.button.pressed.picColor:", "#FFFFFF");

	// Detect screen edges
	/*if (xpos == 0) 
		bleft = true;
	else 
		bleft = false;

	if (xpos == screenWidth - width) 
		bright = true;
	else 
		bright = false;

	if (ypos == 0) 
		btop = true;
	else 
		btop = false;

	if (ypos == screenHeight - height) 
		bbottom = true;
	else 
		bbottom = false;*/

	if (!inheritToolbar)
		height = width = 21 + (2 * frame.borderWidth);
	else
	{
		//height = width = tbInfo.height;
		GetClientRect(FindWindow("Toolbar", 0), &tRect);
		tbInfo.height = height = width = tRect.bottom - tRect.top;
	}

	// Makes sure BBStyle stays on screen after style change.
	if (xpos < vScreenLeft /*|| bleft == true*/) 
		xpos = vScreenLeft;
	if ((xpos + width) > vScreenWidth /*|| bright == true*/)
		xpos = vScreenWidth - width;

	if (ypos < vScreenTop /*|| btop == true*/) 
		ypos = vScreenTop;
	if ((ypos + height) > vScreenHeight /*|| bbottom == true*/)
		ypos = vScreenHeight - height;

	// Different wallpapers on different workspaces
	/*if (rootCommands)
	{
		char temp[MAX_LINE_LENGTH], command[MAX_LINE_LENGTH];

		if (strcmp("", ReadString(stylepath, "bbstyle.workspace1.rootCommand:", "")))
		{
			workspaceRootCommand.clear();

			for (int n = 0; n < 128; n++)
			{		
				sprintf(temp, "bbstyle.workspace%d.rootCommand:", n + 1);
				strcpy(command, ReadString(stylepath, temp, ""));
				//MessageBox(0, command, "stuff", MB_OK);

				if (!strcmp(command,""))
					break;
				else
				{
					workspaceRootCommand.push_back(string(command));	
					//MessageBox(0, command, "stuff", MB_OK);
				}
			}
		}
		else
			GetRCRootCommands();
	}

	strcpy(styleRootCommand, ReadString(stylepath, "rootCommand:", ""));*/

	MoveWindow(hwndPlugin, xpos, ypos, width, height, true);

	if (inSlit)
		SendMessage(hSlit, SLIT_UPDATE, 0, 0);

	//MessageBox(0, "style settings read", szVersion, MB_OK | MB_TOPMOST);
}

//===========================================================================
/*
void GetRCRootCommands()
{
	// Different wallpapers on different workspaces
	char temp[MAX_LINE_LENGTH], command[MAX_LINE_LENGTH];

	workspaceRootCommand.clear();

	for (int n = 0; n < 128; n++)
	{		
		sprintf(temp, "bbstyle.workspace%d.rootCommand:", n + 1);
		strcpy(command, ReadString(rcpath, temp, ""));
		//MessageBox(0, command, "stuff", MB_OK);

		if (!strcmp(command,""))
			break;
		else
		{
			workspaceRootCommand.push_back(string(command));	
			//MessageBox(0, command, "stuff", MB_OK);
		}
	}
}
*/
//===========================================================================

void ReadRCSettings()
{
	//char temp[MAX_LINE_LENGTH];
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind;

	if (!FileExists(rcpath)) 
	{
		InitRC();
		return;
	}

	// If a config file was found we read the plugin settings from the file...
	xpos2 = xpos = ReadInt(rcpath, "bbstyle.position.x:", 10);
	ypos2 = ypos = ReadInt(rcpath, "bbstyle.position.y:", 10);

	UpdateMonitorInfo();

	// dont in GetStyleSettings() now
	//if (xpos >= screenWidth) xpos = 0;
	//if (ypos >= screenHeight) ypos = 0;

	alwaysOnTop = ReadBool(rcpath, "bbstyle.alwaysOnTop:", true);
	snapWindow = ReadBool(rcpath, "bbstyle.snapWindow:", true);
	if (snapWindow) snapWindowOld = true;

	inheritToolbar = ReadBool(rcpath, "bbstyle.inheritToolbarHeight:", false);

	strcpy(listPath,ReadString(rcpath, "bbstyle.stylePath:", "C:\\Blackbox\\styles\\"));

	if (listPath[strlen(listPath)-1] == '\\')
		strcat(listPath, "*");
	//if (listPath[strlen(listPath)-1] != '*')
	else
		strcat(listPath, "\\*");

	hFind = FindFirstFile(listPath, &FindFileData);

	if (hFind == INVALID_HANDLE_VALUE)
		badPath = true;

	// new...
	FindClose(hFind);

	changeTime = ReadInt(rcpath, "bbstyle.changeTime:", 5);
	if (changeTime < 1) changeTime = 1;
	changeTime = changeTime * 60 * 1000;

	changeOnStart = ReadBool(rcpath, "bbstyle.changeOnStart:", false);

	timer = ReadBool(rcpath, "bbstyle.timerOn:", false);

	hideWindow = ReadBool(rcpath, "bbstyle.hideWindow:", false);

	noStyleTwice = ReadBool(rcpath, "bbstyle.noStyleTwice:", false);

	/*rootCommands = ReadBool(rcpath, "bbstyle.rootCommands:", false);

	GetRCRootCommands();*/

	transparency = ReadBool(rcpath, "bbstyle.transparency:", false);
	transparencyAlpha = ReadInt(rcpath, "bbstyle.transparency.alpha:", 200);

	frame.drawBorder = ReadBool(rcpath, "bbstyle.drawBorder:", true);

	useSlit = ReadBool(rcpath, "bbstyle.useSlit:", false);

	//MessageBox(0, "rc settings read", szVersion, MB_OK | MB_TOPMOST);
}

//===========================================================================

void WriteRCSettings()
{
	static char szTemp[MAX_LINE_LENGTH];
	static char temp[8];
	DWORD retLength = 0;

	// Write plugin settings to config file, using path found in ReadRCSettings()...
	HANDLE file = CreateFile(rcpath, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (file)
	{
		//if (inSlit) { xpos = xpos2; ypos = ypos2; }
		sprintf(szTemp, "bbstyle.position.x: %d\r\n", xpos);
		WriteFile(file, szTemp, strlen(szTemp), &retLength, NULL);
		sprintf(szTemp, "bbstyle.position.y: %d\r\n", ypos);
		WriteFile(file, szTemp, strlen(szTemp), &retLength, NULL);

		(alwaysOnTop) ? strcpy(temp, "true") : strcpy(temp, "false");
		sprintf(szTemp, "bbstyle.alwaysOnTop: %s\r\n", temp);
 		WriteFile(file, szTemp, strlen(szTemp), &retLength, NULL);

		(snapWindow || snapWindowOld) ? strcpy(temp, "true") : strcpy(temp, "false");
		sprintf(szTemp, "bbstyle.snapWindow: %s\r\n", temp);
 		WriteFile(file, szTemp, strlen(szTemp), &retLength, NULL);

		(inheritToolbar) ? strcpy(temp, "true") : strcpy(temp, "false");
		sprintf(szTemp, "bbstyle.inheritToolbarHeight: %s\r\n", temp);
 		WriteFile(file, szTemp, strlen(szTemp), &retLength, NULL);

		if (usingWin2kXP)
		{
			(transparency) ? strcpy(temp, "true") : strcpy(temp, "false");
			sprintf(szTemp, "bbstyle.transparency: %s\r\n", temp);
 			WriteFile(file, szTemp, strlen(szTemp), &retLength, NULL);

			sprintf(szTemp, "bbstyle.transparency.alpha: %d\r\n", transparencyAlpha);
			WriteFile(file, szTemp, strlen(szTemp), &retLength, NULL);
		}

		(frame.drawBorder) ? strcpy(temp, "true") : strcpy(temp, "false");
		sprintf(szTemp, "bbstyle.drawBorder: %s\r\n", temp);
 		WriteFile(file, szTemp, strlen(szTemp), &retLength, NULL);

		(timer) ? strcpy(temp, "true") : strcpy(temp, "false");
		sprintf(szTemp, "bbstyle.timerOn: %s\r\n", temp);
 		WriteFile(file, szTemp, strlen(szTemp), &retLength, NULL);

		changeTime = changeTime / (60 * 1000);
		if (changeTime < 1) changeTime = 1;
		sprintf(szTemp, "bbstyle.changeTime: %d\r\n", changeTime);
		WriteFile(file, szTemp, strlen(szTemp), &retLength, NULL);

		(changeOnStart) ? strcpy(temp, "true") : strcpy(temp, "false");
		sprintf(szTemp, "bbstyle.changeOnStart: %s\r\n", temp);
 		WriteFile(file, szTemp, strlen(szTemp), &retLength, NULL);

		if (listPath[strlen(listPath)-1] == '*') listPath[strlen(listPath)-1] = '\0';
		sprintf(szTemp, "bbstyle.stylePath: %s\r\n", listPath);
		WriteFile(file, szTemp, strlen(szTemp), &retLength, NULL);

		(hideWindow) ? strcpy(temp, "true") : strcpy(temp, "false");
		sprintf(szTemp, "bbstyle.hideWindow: %s\r\n", temp);
 		WriteFile(file, szTemp, strlen(szTemp), &retLength, NULL);

		(noStyleTwice) ? strcpy(temp, "true") : strcpy(temp, "false");
		sprintf(szTemp, "bbstyle.noStyleTwice: %s\r\n", temp);
 		WriteFile(file, szTemp, strlen(szTemp), &retLength, NULL);

		/*(rootCommands) ? strcpy(temp, "true") : strcpy(temp, "false");
		sprintf(szTemp, "bbstyle.rootCommands: %s\r\n", temp);
 		WriteFile(file, szTemp, strlen(szTemp), &retLength, NULL);

		vector <string>::size_type i;
		i = workspaceRootCommand.size();
		
		for (int n = 0; n < (int)i; n++)
		{
			sprintf(szTemp, "bbstyle.workspace%d.rootCommand: %s\r\n", n + 1, workspaceRootCommand[n].c_str());
			WriteFile(file, szTemp, strlen(szTemp), &retLength, NULL);
			//MessageBox(0, szTemp, "stuff2", MB_OK);
		}*/

		// Are we in the slit?
		(useSlit) ? strcpy(temp, "true") : strcpy(temp, "false");
		sprintf(szTemp, "bbstyle.useSlit: %s\r\n", temp);
 		WriteFile(file, szTemp, strlen(szTemp), &retLength, NULL);
 	}
	CloseHandle(file);
}

//===========================================================================

void InitRC()
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

	strcat(temp, "bbstyle.rc");
	strcat(path, "bbstylerc");
	// ...checking the two possible filenames example.rc and examplerc ...
	if (FileExists(temp)) strcpy(rcpath, temp);
	else if (FileExists(path)) strcpy(rcpath, path);
	// ...if not found, we try the Blackbox directory...
	else
	{
		// ...but first we save the default path (example.rc in the same
		// folder as the plugin) just in case we need it later (see below)...
		strcpy(defaultpath, temp);
		GetBlackboxPath(rcpath, sizeof(rcpath));
		strcpy(temp, rcpath);
		strcpy(path, rcpath);
		strcat(temp, "bbstyle.rc");
		strcat(path, "bbstylerc");
		if (FileExists(temp)) strcpy(rcpath, temp);
		else if (FileExists(path)) strcpy(rcpath, path);
		else // If no config file was found, we use the default path and settings, and return
		{
			strcpy(rcpath, defaultpath);
			xpos = ypos = 10;
			alwaysOnTop = true;
			snapWindow = true;
			inheritToolbar = false;
			transparency = false;
			transparencyAlpha = 200;
			frame.drawBorder = true;

			changeTime = 5 * 60 * 1000;
			changeOnStart = false;
			timer = false;
			hideWindow = false;
			noStyleTwice = false;
			//rootCommands = false;
			useSlit = false;

			GetBlackboxPath(listPath, sizeof(listPath));
			strcat(listPath, "styles\\*");
			//strcpy(windowText, "Welcome to Blackbox for Windows... [Plugin SDK example]");
			//return;

			WriteRCSettings();
		}
	}
}

//===========================================================================

BOOL WINAPI BBSetLayeredWindowAttributes(HWND hwnd, COLORREF crKey, BYTE bAlpha, DWORD dwFlags)
{
    static BOOL (WINAPI *pSLWA)(HWND, COLORREF, BYTE, DWORD);
    static char f=0;
    for (;;) {
    if (2==f)   return pSLWA(hwnd, crKey, bAlpha, dwFlags);
    // if it's not there, just do nothing and report success
    if (f)      return TRUE;
    *(FARPROC*)&pSLWA = GetProcAddress(
        GetModuleHandle("USER32"), "SetLayeredWindowAttributes");
    f = pSLWA ? 2 : 1;
    }
}

void UpdateMonitorInfo()
{
	// multimonitor
	vScreenLeft = GetSystemMetrics(SM_XVIRTUALSCREEN);
	vScreenTop = GetSystemMetrics(SM_YVIRTUALSCREEN);

	vScreenWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
	vScreenHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);

	vScreenRight = vScreenLeft + vScreenWidth;
	vScreenBottom = vScreenTop + vScreenHeight;

	// current monitor
	HMONITOR hMon = MonitorFromWindow(hwndPlugin, MONITOR_DEFAULTTONEAREST);
	MONITORINFO mInfo;
	mInfo.cbSize = sizeof(MONITORINFO);
	GetMonitorInfo(hMon, &mInfo);

	screenLeft = mInfo.rcMonitor.left;
	screenRight = mInfo.rcMonitor.right;
	screenTop = mInfo.rcMonitor.top;
	screenBottom = mInfo.rcMonitor.bottom;
	screenWidth = screenRight - screenLeft;
	screenHeight = screenBottom - screenTop;
}