/*---------------------------------------------------------------------------------
 bbDrawText 0.0.1 (© 2007 nocd5)
 ----------------------------------------------------------------------------------
 bbMemLimiter is a plugin for bbLean
 ----------------------------------------------------------------------------------
 bbMemLimiter is free software, released under the GNU General Public License,
 version 2, as published by the Free Software Foundation.  It is distributed
 WITHOUT ANY WARRANTY--without even the implied warranty of MERCHANTABILITY
 or FITNESS FOR A PARTICULAR PURPOSE.  Please see the GNU General Public
 License for more details:  [http://www.fsf.org/licenses/gpl.html].
 --------------------------------------------------------------------------------*/

#include "bbDrawText.h"

//===========================================================================
// The startup interface

int beginPlugin(HINSTANCE hPluginInstance)
{
	// ---------------------------------------------------
	// grab some global information

	g_BBhwnd          = GetBBWnd();
	g_hInstance       = hPluginInstance;

	// ---------------------------------------------------
	// register the window class

	WNDCLASS wc;
	ZeroMemory(&wc, sizeof wc);

	wc.lpfnWndProc      = WndProc;      // window procedure
	wc.hInstance        = g_hInstance;    // hInstance of .dll
	wc.lpszClassName    = szAppName;    // window class name
	wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
	wc.style            = CS_DBLCLKS;

	if (!RegisterClass(&wc)){
		MessageBox(g_BBhwnd,
			"Error registering window class", szVersion,
				MB_OK | MB_ICONERROR | MB_TOPMOST);
		return 1;
	}

	// ---------------------------------------------------
	// Zero out variables, read configuration and style

	ZeroMemory(&my, sizeof my);

	ReadRCSettings();

	// ---------------------------------------------------
	// create the window

	my.hwnd = CreateWindowEx(
		WS_EX_TOOLWINDOW,   // window ex-style
		szAppName,          // window class name
		NULL,               // window caption text
		WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, // window style
		my.xpos,            // x position
		my.ypos,            // y position
		my.width,           // window width
		my.height,          // window height
		NULL,               // parent window
		NULL,               // window menu
		g_hInstance,          // hInstance of .dll
		NULL                // creation data
		);

	// ---------------------------------------------------
	// set window location and properties

	set_window_modes();
	ShowWindow(my.hwnd, SW_SHOWNA);
	SetWindowPos(my.hwnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
	return 0;
}

//===========================================================================
// on unload...

void endPlugin(HINSTANCE hPluginInstance)
{
	// Destroy the window...
	DestroyWindow(my.hwnd);

	// clean up HFONT object
	if (my.hFont) DeleteObject(my.hFont);

	// free my.window_text
	if (my.window_text) free(my.window_text);

	// Unregister window class...
	UnregisterClass(szAppName, hPluginInstance);
}

//===========================================================================

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static int msgs[] = { BB_RECONFIGURE, BB_BROADCAST, 0};

	switch (message)
	{
		case WM_CREATE:
			// Register to reveive these message
			SendMessage(g_BBhwnd, BB_REGISTERMESSAGE, (WPARAM)hwnd, (LPARAM)msgs);
			// Make the window appear on all workspaces
			MakeSticky(hwnd);
			break;

		case WM_DESTROY:
			RemoveSticky(hwnd);
			SendMessage(g_BBhwnd, BB_UNREGISTERMESSAGE, (WPARAM)hwnd, (LPARAM)msgs);
			break;

		// ----------------------------------------------------------
		// Blackbox sends a "BB_RECONFIGURE" message on style changes etc.

		case BB_RECONFIGURE:
			set_window_modes();
			break;

		// ----------------------------------------------------------
		// Painting with a cached double-buffer.

		case WM_PAINT:
		{
			RECT r = {0, 0, my.width, my.height};
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hwnd, &ps);

			HGDIOBJ otherfont = SelectObject(hdc, my.hFont);

			SetBkMode(hdc, TRANSPARENT);

			// set parameters
			DRAWTEXTPARAMS dtpParam;
			dtpParam.cbSize       = sizeof(DRAWTEXTPARAMS);
			dtpParam.iTabLength   = 4;
			dtpParam.iLeftMargin  = 1;
			dtpParam.iRightMargin = 1;
			DWORD dwStyle = DT_TABSTOP|DT_EXPANDTABS|DT_END_ELLIPSIS;

			// draw desktop bmp
			PaintDesktop(hdc);
			// draw the text
			BBDrawTextEx(hdc, my.window_text, -1, &r, dwStyle, &dtpParam, &my);

			// Put back the previous default font.
			SelectObject(hdc, otherfont);

			// Done.
			EndPaint(hwnd, &ps);
			break;
		}

		// ----------------------------------------------------------
		// Manually moving/sizing has been started

		case WM_ENTERSIZEMOVE:
			my.is_moving = true;
			break;

		case WM_EXITSIZEMOVE:
			if (my.is_moving){
				// record new location
				WriteInt(rcpath, "bbDrawText.xpos:", my.xpos);
				WriteInt(rcpath, "bbDrawText.ypos:", my.ypos);

				if (my.is_sizing){
					// record new size
					WriteInt(rcpath, "bbDrawText.width:", my.width);
					WriteInt(rcpath, "bbDrawText.height:", my.height);
				}
			}
			my.is_moving = my.is_sizing = false;
			break;

		// ---------------------------------------------------
		// snap to edges on moving

		case WM_WINDOWPOSCHANGING:
			if (my.is_moving){
				WINDOWPOS* wp = (WINDOWPOS*)lParam;
				if (my.snapWindow)
					SnapWindowToEdge(wp, 10, my.is_sizing ? SNAP_FULLSCREEN|SNAP_SIZING : SNAP_FULLSCREEN);

				// set a minimum size
				if (wp->cx < 32) wp->cx = 32;
				if (wp->cy < 16) wp->cy = 16;
			}
			break;

		// ---------------------------------------------------
		// store new location and size. when not in slit

		case WM_WINDOWPOSCHANGED:
			if (my.is_moving){
				WINDOWPOS* wp = (WINDOWPOS*)lParam;
				if (my.is_sizing){
					// record sizes
					my.width = wp->cx;
					my.height = wp->cy;

					// redraw window
				}
				// record position
				my.xpos = wp->x;
				my.ypos = wp->y;
			}
			InvalidateRect(my.hwnd, NULL, FALSE);
			break;

		// ----------------------------------------------------------
		// start moving or sizing accordingly to keys held down

		case WM_LBUTTONDOWN:
			UpdateWindow(hwnd);
			if (GetAsyncKeyState(VK_MENU) & 0x8000)
			{
				// start sizing, when alt-key is held down
				PostMessage(hwnd, WM_SYSCOMMAND, 0xf008, 0);
				my.is_sizing = true;
				SetWindowPos(hwnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
				break;
			}
			if (GetAsyncKeyState(VK_CONTROL) & 0x8000){
				// start moving, when control-key is held down
				PostMessage(hwnd, WM_SYSCOMMAND, 0xf012, 0);
				SetWindowPos(hwnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
				break;
			}
			break;

		// ----------------------------------------------------------
		// Show the user menu

		case WM_RBUTTONUP:
			if (GetAsyncKeyState(VK_CONTROL) & 0x8000){
				ShowMyMenu(true);
				break;
			}
			PostMessage(g_BBhwnd, BB_EXECUTE, 0, (LPARAM)my.exeCmd);
			break;

		// ----------------------------------------------------------
		// Show the user menu
		case WM_LBUTTONDBLCLK:
			SendMessage(g_BBhwnd, BB_EDITFILE, (WPARAM)-1, (LPARAM)my.filePath);
			break;

		// ----------------------------------------------------------
		// Blackbox sends Broams to all windows...

		case BB_BROADCAST:
		{
			const char *msg_string = (LPCSTR)lParam;

			// check general broams
			if (!stricmp(msg_string, "@BBShowPlugins")){
				if (my.is_hidden){
					my.is_hidden = false;
					ShowWindow(hwnd, SW_SHOWNA);
				}
				break;
			}

			if (!stricmp(msg_string, "@BBHidePlugins")){
				if (my.pluginToggle){
					my.is_hidden = true;
					ShowWindow(hwnd, SW_HIDE);
				}
				break;
			}

			// Our broadcast message prefix:
			const char broam_prefix[] = "@bbDrawText.";
			const int broam_prefix_len = sizeof broam_prefix - 1; // minus terminating \0

			// check broams sent from our own menu
			if (!memicmp(msg_string, broam_prefix, broam_prefix_len)){
				msg_string += broam_prefix_len;

				if (!stricmp(msg_string, "Reconfigure")){
					set_window_modes();
					break;
				}

				if (!stricmp(msg_string, "snapWindow")){
					eval_menu_cmd(M_BOL, &my.snapWindow, msg_string);
					break;
				}

				if (!stricmp(msg_string, "pluginToggle")){
					eval_menu_cmd(M_BOL, &my.pluginToggle, msg_string);
					break;
				}

				if (!stricmp(msg_string, "editRC")){
					SendMessage(g_BBhwnd, BB_EDITFILE, (WPARAM)-1, (LPARAM)rcpath);
					break;
				}

				if (!my_substr_icmp(msg_string, "fontHeight")){
					eval_menu_cmd(M_INT, &my.nFontHeight, msg_string);
					break;
				}

				if (!my_substr_icmp(msg_string, "Font")){
					eval_menu_cmd(M_STR, &my.strFont, msg_string);
					break;
				}

				if (!my_substr_icmp(msg_string, "TextColor")){
					char *p = strchr(msg_string, ' ');
					my.textColor = ReadColor("", "", p+1);
					WriteString(rcpath, "bbDrawText.textColor:", p+1);
					set_window_modes();
					break;
				}
				if (!my_substr_icmp(msg_string, "shadowColor")){
					char *p = strchr(msg_string, ' ');
					my.shadowColor = ReadColor("", "", p+1);
					WriteString(rcpath, "bbDrawText.shadowColor:", p+1);
					set_window_modes();
					break;
				}
				if (!my_substr_icmp(msg_string, "outlineColor")){
					char *p = strchr(msg_string, ' ');
					my.outlineColor = ReadColor("", "", p+1);
					WriteString(rcpath, "bbDrawText.outlineColor:", p+1);
					set_window_modes();
					break;
				}

				if (!my_substr_icmp(msg_string, "filePath")){
					eval_menu_cmd(M_STR, &my.filePath, msg_string);
				}

				if (!my_substr_icmp(msg_string, "openPath")){
					OPENFILENAME ofn;
					ZeroMemory(&ofn, sizeof(ofn));
					ofn.lStructSize = sizeof(OPENFILENAME);
					ofn.lpstrFilter =
						TEXT("Text Files(*.txt)\0*.txt\0")
						TEXT("All Files(*.*)\0*.*\0\0");
					ofn.lpstrFile = my.filePath;
					ofn.nMaxFile = MAX_PATH;
					ofn.Flags = OFN_FILEMUSTEXIST;

					GetOpenFileName(&ofn);

					WriteString(rcpath, "bbDrawText.filePath:", my.filePath);
					set_window_modes();
					break;
				}

				if (!stricmp(msg_string, "editFile")){
					SendMessage(g_BBhwnd, BB_EDITFILE, (WPARAM)-1, (LPARAM)my.filePath);
					break;
				}
			}
			break;
		}

		// ----------------------------------------------------------
		// always bottommost
		case WM_MOUSEACTIVATE:
			return MA_NOACTIVATE;

		case WM_ACTIVATE:
			SetWindowPos(hwnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
			break;

			// ----------------------------------------------------------
		// prevent the user from closing the plugin with alt-F4
		case WM_CLOSE:
			break;

		// ----------------------------------------------------------
		// let windows handle any other message
		default:
			return DefWindowProc(hwnd,message,wParam,lParam);
	}
	return 0;
}

//===========================================================================

//===========================================================================
// Update position and size, as well as onTop, transparency and inSlit states.

void set_window_modes(void)
{
	ReadRCSettings();
	SetWindowPos(my.hwnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
	// window needs drawing
	InvalidateRect(my.hwnd, NULL, FALSE);
}

//===========================================================================

void ReadRCSettings(void)
{
	int i = 0;
	do
	{
		// First and third, we look for the config file
		// in the same folder as the plugin...
		HINSTANCE hInst = g_hInstance;
		// second we check the blackbox directory
		if (1 == i) hInst = NULL;

		GetModuleFileName(hInst, rcpath, sizeof(rcpath));
		char *file_name_start = strrchr(rcpath, '\\');
		if (file_name_start) ++file_name_start;
		else file_name_start = strchr(rcpath, 0);
		strcpy(file_name_start, "bbDrawText.rc");

	} while (++i < 3 && false == FileExists(rcpath));

	// If a config file was found we read the plugin settings from it...
	// ...if not, the ReadXXX functions give us just the defaults.

	my.xpos     = ReadInt(rcpath, "bbDrawText.xpos:", 10);
	my.ypos     = ReadInt(rcpath, "bbDrawText.ypos:", 10);
	my.width    = ReadInt(rcpath, "bbDrawText.width:", 80);
	my.height   = ReadInt(rcpath, "bbDrawText.height:", 40);

	my.snapWindow       = ReadBool(rcpath, "bbDrawText.snapWindow:", true);
	my.pluginToggle     = ReadBool(rcpath, "bbDrawText.pluginToggle:", true);

	strcpy(my.strFont, ReadString(rcpath, "bbDrawText.Font:", "Arial"));
	my.nFontHeight = ReadInt(rcpath, "bbDrawText.FontHeight:", 16);
	my.textColor = ReadColor(rcpath, "bbDrawText.textColor:", 0);
	my.shadowColor = ReadColor(rcpath, "bbDrawText.shadowColor:", "-1");
	my.outlineColor = ReadColor(rcpath, "bbDrawText.outlineColor:", "-1");
	strcpy(my.filePath, ReadString(rcpath, "bbDrawText.filePath:", szAppName));
	strcpy(my.exeCmd, ReadString(rcpath, "bbDrawText.RightClick:", "@BBCore"));

	if (my.hFont) DeleteObject(my.hFont);
	my.hFont = CreateMyFont(my.strFont, my.nFontHeight);

	FILE *fp;
	long begin, end;
	int filesize = 0;
	if (my.window_text) free(my.window_text);
	if (fp = fopen(my.filePath, "a+")){
		fseek(fp, 0, SEEK_SET); begin = ftell(fp);
		fseek(fp, 0, SEEK_END); end = ftell(fp);
		fseek(fp, 0, SEEK_SET); filesize = (int)(end - begin);
		my.window_text = (char*)malloc(filesize);
		strcpy(my.window_text, "");
		char *buf = (char*)malloc(filesize);
		for(;;) {
			if (fgets(buf, filesize, fp) == NULL) break;
			strcat(my.window_text, buf);
		}
		free(buf);
		fclose(fp);
	}
}

//===========================================================================

void ShowMyMenu(bool popup)
{
	Menu *pMenu, *pSub;

	// Create the main menu, with a title and an unique IDString
	pMenu = MakeNamedMenu("bbDrawText", "bbDrawText_IDMain", popup);
	pSub = MakeNamedMenu("Configuration", "bbSDK_IDConfig", popup);

	MakeMenuItem(pMenu, "Reconfigure", "@bbDrawText.Reconfigure", false);
	MakeMenuSEP(pMenu);
	MakeMenuItemString(pMenu, "Font", "@bbDrawText.Font", my.strFont);
	MakeMenuItemInt(pMenu, "Font Height", "@bbDrawText.fontHeight", my.nFontHeight, 0, 100);

	char tc_buf[8];
	char tmp;
	sprintf(tc_buf, "#%.6lX", my.textColor);
	for (int i = 0; i < (int)strlen(tc_buf)/2; i++){
		tmp = tc_buf[i];
		tc_buf[i] = tc_buf[strlen(tc_buf)-i];
		tc_buf[strlen(tc_buf)-i] = tmp;
	}
	MakeMenuItemString(pMenu, "Text Color", "@bbDrawText.textColor", tc_buf);
	sprintf(tc_buf, "#%.6lX", my.shadowColor);
	for (int i = 0; i < (int)strlen(tc_buf)/2; i++){
		tmp = tc_buf[i];
		tc_buf[i] = tc_buf[strlen(tc_buf)-i];
		tc_buf[strlen(tc_buf)-i] = tmp;
	}
	MakeMenuItemString(pMenu, "Shadow Color", "@bbDrawText.shadowColor", tc_buf);
	sprintf(tc_buf, "#%.6lX", my.outlineColor);
	for (int i = 0; i < (int)strlen(tc_buf)/2; i++){
		tmp = tc_buf[i];
		tc_buf[i] = tc_buf[strlen(tc_buf)-i];
		tc_buf[strlen(tc_buf)-i] = tmp;
	}
	MakeMenuItemString(pMenu, "Outline Color", "@bbDrawText.outlineColor", tc_buf);

	MakeMenuNOP(pMenu);
	MakeMenuItem(pMenu, "Edit File", "@bbDrawText.editFile", false);
	MakeMenuItem(pMenu, "Open File", "@bbDrawText.openPath", false);
	MakeSubmenu(pMenu, pSub, "Others");
		MakeMenuItem(pSub, "Edit Settings", "@bbDrawText.editRC", false);
		MakeMenuItem(pSub, "Snap To Edges", "@bbDrawText.snapWindow", my.snapWindow);

	// ----------------------------------
	// Finally, show the menu...
	ShowMenu(pMenu);
}

//===========================================================================
// helper to handle commands  from the menu

void eval_menu_cmd(int mode, void *pValue, const char *msg_string)
{
	// Our rc_key prefix:
	const char rc_prefix[] = "bbDrawText.";
	const int rc_prefix_len = sizeof rc_prefix - 1; // minus terminating \0

	char rc_string[80];

	// scan for a second argument after a space, like in "AlphaValue 200"
	const char *p = strchr(msg_string, ' ');
	int msg_len = p ? p++ - msg_string : strlen(msg_string);

	// Build the full rc_key. i.e. "bbDrawText.<subkey>:"
	strcpy(rc_string, rc_prefix);
	memcpy(rc_string + rc_prefix_len, msg_string, msg_len);
	strcpy(rc_string + rc_prefix_len + msg_len, ":");

	switch (mode)
	{
		case M_BOL: // --- toggle boolean variable ----------------
			*(bool*)pValue = false == *(bool*)pValue;

			// write the new setting to the rc - file
			WriteBool(rcpath, rc_string, *(bool*)pValue);
			break;

		case M_INT: // --- set integer variable -------------------
			if (p){
				*(int*)pValue = atoi(p);

				// write the new setting to the rc - file
				WriteInt(rcpath, rc_string, *(int*)pValue);
			}
			break;

		case M_STR: // --- set string variable -------------------
			if (p){
				strcpy((char*)pValue, p);

				// write the new setting to the rc - file
				WriteString(rcpath, rc_string, (char*)pValue);
			}
			break;
	}

	// apply new settings
	set_window_modes();

	// and update the menu checkmarks
	ShowMyMenu(false);
}

//===========================================================================
// pluginInfo is used by Blackbox for Windows to fetch information about
// a particular plugin.

LPCSTR pluginInfo (int field){
	switch (field){
		case 0: return szVersion;
		case 1: return szAppName;
		case 2: return szInfoVersion;
		case 3: return szInfoAuthor;
		case 4: return szInfoRelDate;
	}
	return "";
}

//*****************************************************************************
// utilities

// case insensitive string compare, up to lenght of second string
int my_substr_icmp(const char *a, const char *b)
{
	return memicmp(a, b, strlen(b));
}

// debugging (checkout "DBGVIEW" from "http://www.sysinternals.com/")
void dbg_printf (const char *fmt, ...)
{
	char buffer[4096]; va_list arg;
	va_start(arg, fmt);
	vsprintf (buffer, fmt, arg);
	OutputDebugString(buffer);
}

HFONT CreateMyFont(LPCSTR strFont, int nHeight)
{
    return CreateFont(
        nHeight,
        0, 0, 0,
        0,
        false, false, false,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY,
        DEFAULT_PITCH|FF_DONTCARE,
        strFont
        );
}

//===========================================================================
// Function: BBDrawTextEx
// Purpose: draw text with shadow and/or outline
// In:
// Out:
//===========================================================================
int BBDrawTextEx(HDC hDC, LPSTR lpString, int nCount, LPRECT lpRect, UINT uFormat, LPDRAWTEXTPARAMS lpDTParams, plugin_properties* lppiprop){

    if (lppiprop->shadowColor != (COLORREF)-1){ // draw shadow
        RECT rcShadow;
        SetTextColor(hDC, lppiprop->shadowColor);
        if (lppiprop->outlineColor != (COLORREF)-1){ // draw shadow with outline
            _CopyOffsetRect(&rcShadow, lpRect, 2, 0);
            DrawTextEx(hDC, lpString, nCount, &rcShadow, uFormat, lpDTParams); _OffsetRect(&rcShadow,  0, 1);
            DrawTextEx(hDC, lpString, nCount, &rcShadow, uFormat, lpDTParams); _OffsetRect(&rcShadow,  0, 1);
            DrawTextEx(hDC, lpString, nCount, &rcShadow, uFormat, lpDTParams); _OffsetRect(&rcShadow, -1, 0);
            DrawTextEx(hDC, lpString, nCount, &rcShadow, uFormat, lpDTParams); _OffsetRect(&rcShadow, -1, 0);
            DrawTextEx(hDC, lpString, nCount, &rcShadow, uFormat, lpDTParams);
        }
        else{
            _CopyOffsetRect(&rcShadow, lpRect, 1, 1);
            DrawTextEx(hDC, lpString, nCount, &rcShadow, uFormat, lpDTParams);
        }
    }
    if (lppiprop->outlineColor != (COLORREF)-1){ // draw outline
        RECT rcOutline;
        SetTextColor(hDC, lppiprop->outlineColor);
        _CopyOffsetRect(&rcOutline, lpRect, 1, 0);
        DrawTextEx(hDC, lpString, nCount, &rcOutline, uFormat, lpDTParams); _OffsetRect(&rcOutline,   0,  1);
        DrawTextEx(hDC, lpString, nCount, &rcOutline, uFormat, lpDTParams); _OffsetRect(&rcOutline,  -1,  0);
        DrawTextEx(hDC, lpString, nCount, &rcOutline, uFormat, lpDTParams); _OffsetRect(&rcOutline,  -1,  0);
        DrawTextEx(hDC, lpString, nCount, &rcOutline, uFormat, lpDTParams); _OffsetRect(&rcOutline,   0, -1);
        DrawTextEx(hDC, lpString, nCount, &rcOutline, uFormat, lpDTParams); _OffsetRect(&rcOutline,   0, -1);
        DrawTextEx(hDC, lpString, nCount, &rcOutline, uFormat, lpDTParams); _OffsetRect(&rcOutline,   1,  0);
        DrawTextEx(hDC, lpString, nCount, &rcOutline, uFormat, lpDTParams); _OffsetRect(&rcOutline,   1,  0);
        DrawTextEx(hDC, lpString, nCount, &rcOutline, uFormat, lpDTParams);
    }
    // draw text
    SetTextColor(hDC, lppiprop->textColor);
    return DrawTextEx(hDC, lpString, nCount, lpRect, uFormat, lpDTParams);
}

