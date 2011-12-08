/*
 ============================================================================
  SDK example plugin for Blackbox for Windows.
  Copyright © 2004 grischka

  This program is free software, released under the GNU General Public
  License (GPL version 2 or later). See:

  http://www.fsf.org/licenses/gpl.html

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

 ============================================================================

  Description:

  This is an example Slit-Plugin for Blackbox for Windows. It displays
  a little stylized window with an inscription.

  The plugin window can be moved with:
	- left mouse and the control key held down,

  and resized with
	- left mouse and the alt key held down.

  Some standard plugin window properties can be configured with the
  right-click menu, also the inscription text can be set.

 ============================================================================
*/

#include "BBApi.h"
#include <stdlib.h>

// ----------------------------------
// plugin info

LPCSTR szVersion        = "bbSDK 0.0.1";
LPCSTR szAppName        = "bbSDK";
LPCSTR szInfoVersion    = "0.0.1";
LPCSTR szInfoAuthor     = "grischka";
LPCSTR szInfoRelDate    = "2004-05-01";
LPCSTR szInfoLink       = "http://bb4win.sourceforge.net/bblean";
LPCSTR szInfoEmail      = "grischka@users.sourceforge.net";

// ----------------------------------
// The About MessageBox

void about_box(void)
{
	char szTemp[1000];
	sprintf(szTemp,
		"%s - A slittable plugin example for Blackbox for Windows."
		"\n"
		"\n© 2004 %s"
		"\n%s"
		, szVersion, szInfoEmail, szInfoLink
		);
	MessageBox(NULL, szTemp, "About", MB_OK|MB_TOPMOST);
}

// ----------------------------------
// Interface declaration

extern "C"
{
	DLL_EXPORT int beginPlugin(HINSTANCE hPluginInstance);
	DLL_EXPORT int beginSlitPlugin(HINSTANCE hPluginInstance, HWND hSlit);
	DLL_EXPORT int beginPluginEx(HINSTANCE hPluginInstance, HWND hSlit);
	DLL_EXPORT void endPlugin(HINSTANCE hPluginInstance);
	DLL_EXPORT LPCSTR pluginInfo(int field);
};

// ----------------------------------
// Global vars

HINSTANCE hInstance;
HWND BBhwnd;
HWND hSlit_present;
bool is_bblean;

// receives the path to "bbSDK.rc"
char rcpath[MAX_PATH];

// ----------------------------------
// Style info

struct style_info
{
	StyleItem Frame;
	int bevelWidth;
	int borderWidth;
	COLORREF borderColor;
} style_info;

// ----------------------------------
// Plugin window properties

struct plugin_properties
{
	// settings
	int xpos, ypos;
	int width, height;

	bool useSlit;
	bool alwaysOnTop;
	bool snapWindow;
	bool pluginToggle;
	bool alphaEnabled;
	bool drawBorder;
	int  alphaValue;

	// our plugin window
	HWND hwnd;

	// current state variables
	bool is_ontop;
	bool is_moving;
	bool is_sizing;
	bool is_hidden;

	// the Slit window, if we are in it.
	HWND hSlit;

	// GDI objects
	HBITMAP bufbmp;
	HFONT hFont;

	// the text
	char window_text[100];

} my;

// ----------------------------------
// some function prototypes

void GetStyleSettings();
void ReadRCSettings();
void WriteRCSettings();
void ShowMyMenu(bool popup);
void invalidate_window(void);
void set_window_modes(void);

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

// ----------------------------------
// helper to handle commands  from the menu

void eval_menu_cmd(int mode, void *pValue, const char *sub_message);
enum eval_menu_cmd_modes
{
	M_BOL = 1,
	M_INT = 2,
	M_STR = 3,
};

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

//===========================================================================

//===========================================================================
// The startup interface

int beginSlitPlugin(HINSTANCE hPluginInstance, HWND hSlit)
{
	// ---------------------------------------------------
	// grab some global information

	BBhwnd          = GetBBWnd();
	hInstance       = hPluginInstance;
	hSlit_present   = hSlit;
	is_bblean       = 0 == my_substr_icmp(GetBBVersion(), "bblean");

	// ---------------------------------------------------
	// register the window class

	WNDCLASS wc;
	ZeroMemory(&wc, sizeof wc);

	wc.lpfnWndProc      = WndProc;      // window procedure
	wc.hInstance        = hInstance;    // hInstance of .dll
	wc.lpszClassName    = szAppName;    // window class name
	wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
	wc.style            = CS_DBLCLKS;

	if (!RegisterClass(&wc))
	{
		MessageBox(BBhwnd,
			"Error registering window class", szVersion,
				MB_OK | MB_ICONERROR | MB_TOPMOST);
		return 1;
	}

	// ---------------------------------------------------
	// Zero out variables, read configuration and style

	ZeroMemory(&my, sizeof my);

	ReadRCSettings();
	GetStyleSettings();

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
		hInstance,          // hInstance of .dll
		NULL                // creation data
		);

	// ---------------------------------------------------
	// set window location and properties

	set_window_modes();
	ShowWindow(my.hwnd, SW_SHOWNA);
	return 0;
}

//===========================================================================
// xoblite type slit interface

int beginPluginEx(HINSTANCE hPluginInstance, HWND hSlit)
{
	return beginSlitPlugin(hPluginInstance, hSlit);
}

// no-slit interface
int beginPlugin(HINSTANCE hPluginInstance)
{
	return beginSlitPlugin(hPluginInstance, NULL);
}

//===========================================================================
// on unload...

void endPlugin(HINSTANCE hPluginInstance)
{
	// Get out of the Slit, in case we are...
	if (my.hSlit) SendMessage(my.hSlit, SLIT_REMOVE, 0, (LPARAM)my.hwnd);

	// Destroy the window...
	DestroyWindow(my.hwnd);

	// clean up HBITMAP object
	if (my.bufbmp) DeleteObject(my.bufbmp);

	// clean up HFONT object
	if (my.hFont) DeleteObject(my.hFont);

	// Unregister window class...
	UnregisterClass(szAppName, hPluginInstance);
}

//===========================================================================
// pluginInfo is used by Blackbox for Windows to fetch information about
// a particular plugin.

LPCSTR pluginInfo(int field)
{
	switch (field)
	{
		case PLUGIN_NAME:           return szAppName;       // Plugin name
		case PLUGIN_VERSION:        return szInfoVersion;   // Plugin version
		case PLUGIN_AUTHOR:         return szInfoAuthor;    // Author
		case PLUGIN_RELEASE:        return szInfoRelDate;   // Release date, preferably in yyyy-mm-dd format
		case PLUGIN_LINK:           return szInfoLink;      // Link to author's website
		case PLUGIN_EMAIL:          return szInfoEmail;     // Author's email
		default:                    return szVersion;       // Fallback: Plugin name + version, e.g. "MyPlugin 1.0"
	}
}

//===========================================================================
// this invalidates the window, and resets the bitmap at the same time.

void invalidate_window(void)
{
	if (my.bufbmp)
	{
		// delete the bitmap, so it will be drawn again
		// next time with WM_PAINT
		DeleteObject(my.bufbmp);
		my.bufbmp = NULL;
	}
	// notify the os that the window needs painting
	InvalidateRect(my.hwnd, NULL, FALSE);
}

//===========================================================================

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static int msgs[] = { BB_RECONFIGURE, BB_BROADCAST, 0};

	switch (message)
	{
		case WM_CREATE:
			// Register to reveive these message
			SendMessage(BBhwnd, BB_REGISTERMESSAGE, (WPARAM)hwnd, (LPARAM)msgs);
			// Make the window appear on all workspaces
			MakeSticky(hwnd);
			break;

		case WM_DESTROY:
			RemoveSticky(hwnd);
			SendMessage(BBhwnd, BB_UNREGISTERMESSAGE, (WPARAM)hwnd, (LPARAM)msgs);
			break;

		// ----------------------------------------------------------
		// Blackbox sends a "BB_RECONFIGURE" message on style changes etc.

		case BB_RECONFIGURE:
			ReadRCSettings();
			GetStyleSettings();
			set_window_modes();
			break;

		// ----------------------------------------------------------
		// Painting with a cached double-buffer.

		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hwnd, &ps);

			// create a DC for the buffer
			HDC buf = CreateCompatibleDC(hdc);
			HGDIOBJ otherbmp;

			if (NULL == my.bufbmp) // No bitmap yet?
			{
				// Generate it and paint everything
				my.bufbmp = CreateCompatibleBitmap(hdc, my.width, my.height);

				// Select it into the DC, storing the previous default.
				otherbmp = SelectObject(buf, my.bufbmp);

				// Setup the rectangle
				RECT r; r.left = r.top = 0; r.right = my.width; r.bottom =  my.height;

				// and draw the frame
				MakeStyleGradient(buf, &r, &style_info.Frame, my.drawBorder);

				if (my.window_text[0])
				{
					// Set the font, storing the default..
					HGDIOBJ otherfont = SelectObject(buf, my.hFont);

					SetTextColor(buf, style_info.Frame.TextColor);
					SetBkMode(buf, TRANSPARENT);

					// adjust the rectangle
					int margin = style_info.bevelWidth;
					if (my.drawBorder) margin += style_info.borderWidth;
					r.left  += margin;
					r.top   += margin;
					r.right -= margin;
					r.bottom -= margin;

					// draw the text
					DrawText(buf, my.window_text, -1, &r, DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);

					// Put back the previous default font.
					SelectObject(buf, otherfont);
				}
			}
			else
			{
				// Otherwise it has been painted previously,
				// so just select it into the DC
				otherbmp = SelectObject(buf, my.bufbmp);
			}

			// ... and copy the buffer on the screen:
			BitBlt(hdc,
				ps.rcPaint.left,
				ps.rcPaint.top,
				ps.rcPaint.right  - ps.rcPaint.left,
				ps.rcPaint.bottom - ps.rcPaint.top,
				buf,
				ps.rcPaint.left,
				ps.rcPaint.top,
				SRCCOPY
				);

			// Put back the previous default bitmap
			SelectObject(buf, otherbmp);

			// clean up
			DeleteDC(buf);

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
			if (my.is_moving)
			{
				if (my.hSlit) // already we are
				{
					SendMessage(my.hSlit, SLIT_UPDATE, 0, (LPARAM)hwnd);
				}
				else
				{
					// record new location, (if not in slit)
					WriteInt(rcpath, "bbSDK.xpos:", my.xpos);
					WriteInt(rcpath, "bbSDK.ypos:", my.ypos);
				}

				if (my.is_sizing)
				{
					// record new size
					WriteInt(rcpath, "bbSDK.width:", my.width);
					WriteInt(rcpath, "bbSDK.height:", my.height);
				}
			}
			my.is_moving = my.is_sizing = false;
			break;

		// ---------------------------------------------------
		// snap to edges on moving

		case WM_WINDOWPOSCHANGING:
			if (my.is_moving)
			{
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
			if (my.is_moving)
			{
				WINDOWPOS* wp = (WINDOWPOS*)lParam;
				if (my.is_sizing)
				{
					// record sizes
					my.width = wp->cx;
					my.height = wp->cy;

					// redraw window
					invalidate_window();
				}
				if (NULL == my.hSlit)
				{
					// record position
					my.xpos = wp->x;
					my.ypos = wp->y;
				}
			}
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
				break;
			}
			if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
			{
				// start moving, when control-key is held down
				PostMessage(hwnd, WM_SYSCOMMAND, 0xf012, 0);
				break;
			}
			break;

		// ----------------------------------------------------------
		// Show the user menu

		case WM_RBUTTONUP:
			ShowMyMenu(true);
			break;

		// ----------------------------------------------------------
		// Blackbox sends Broams to all windows...

		case BB_BROADCAST:
		{
			const char *msg_string = (LPCSTR)lParam;

			// check general broams
			if (!stricmp(msg_string, "@BBShowPlugins"))
			{
				if (my.is_hidden)
				{
					my.is_hidden = false;
					ShowWindow(hwnd, SW_SHOWNA);
				}
				break;
			}

			if (!stricmp(msg_string, "@BBHidePlugins"))
			{
				if (my.pluginToggle && NULL == my.hSlit)
				{
					my.is_hidden = true;
					ShowWindow(hwnd, SW_HIDE);
				}
				break;
			}

			// Our broadcast message prefix:
			const char broam_prefix[] = "@bbSDK.";
			const int broam_prefix_len = sizeof broam_prefix - 1; // minus terminating \0

			// check broams sent from our own menu
			if (!memicmp(msg_string, broam_prefix, broam_prefix_len))
			{
				msg_string += broam_prefix_len;
				if (!stricmp(msg_string, "useSlit"))
				{
					eval_menu_cmd(M_BOL, &my.useSlit, msg_string);
					break;
				}

				if (!stricmp(msg_string, "alwaysOnTop"))
				{
					eval_menu_cmd(M_BOL, &my.alwaysOnTop, msg_string);
					break;
				}

				if (!stricmp(msg_string, "drawBorder"))
				{
					eval_menu_cmd(M_BOL, &my.drawBorder, msg_string);
					break;
				}

				if (!stricmp(msg_string, "snapWindow"))
				{
					eval_menu_cmd(M_BOL, &my.snapWindow, msg_string);
					break;
				}

				if (!stricmp(msg_string, "alphaEnabled"))
				{
					eval_menu_cmd(M_BOL, &my.alphaEnabled, msg_string);
					break;
				}

				if (!my_substr_icmp(msg_string, "alphaValue"))
				{
					eval_menu_cmd(M_INT, &my.alphaValue, msg_string);
					break;
				}

				if (!stricmp(msg_string, "pluginToggle"))
				{
					eval_menu_cmd(M_BOL, &my.pluginToggle, msg_string);
					break;
				}

				if (!my_substr_icmp(msg_string, "windowText"))
				{
					eval_menu_cmd(M_STR, &my.window_text, msg_string);
					break;
				}

				if (!stricmp(msg_string, "editRC"))
				{
					SendMessage(BBhwnd, BB_EDITFILE, (WPARAM)-1, (LPARAM)rcpath);
					break;
				}

				if (!stricmp(msg_string, "About"))
				{
					about_box();
					break;
				}
			}
			break;
		}

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
	HWND hwnd = my.hwnd;

	if (my.useSlit && hSlit_present)
	{
		// if in slit, dont move...
		SetWindowPos(hwnd, NULL,
			0, 0, my.width, my.height,
			SWP_NOACTIVATE|SWP_NOSENDCHANGING|SWP_NOZORDER|SWP_NOMOVE
			);

		if (my.hSlit) // already we are
		{
			SendMessage(my.hSlit, SLIT_UPDATE, 0, (LPARAM)hwnd);
		}
		else // enter it
		{
			my.hSlit = hSlit_present;
			SendMessage(my.hSlit, SLIT_ADD, 0, (LPARAM)hwnd);
		}
	}
	else
	{
		if (my.hSlit) // leave it
		{
			SendMessage(my.hSlit, SLIT_REMOVE, 0, (LPARAM)hwnd);
			my.hSlit = NULL;
		}

		HWND hwnd_after = NULL;
		UINT flags = SWP_NOACTIVATE|SWP_NOSENDCHANGING|SWP_NOZORDER;

		if (my.is_ontop != my.alwaysOnTop)
		{
			my.is_ontop = my.alwaysOnTop;
			hwnd_after = my.is_ontop ? HWND_TOPMOST : HWND_NOTOPMOST;
			flags = SWP_NOACTIVATE|SWP_NOSENDCHANGING;
		}

		SetWindowPos(hwnd, hwnd_after, my.xpos, my.ypos, my.width, my.height, flags);
		SetTransparency(hwnd, (BYTE)(my.alphaEnabled ? my.alphaValue : 255));
	}

	// window needs drawing
	invalidate_window();
}

//===========================================================================

void ReadRCSettings(void)
{
	int i = 0;
	do
	{
		// First and third, we look for the config file
		// in the same folder as the plugin...
		HINSTANCE hInst = hInstance;
		// second we check the blackbox directory
		if (1 == i) hInst = NULL;

		GetModuleFileName(hInst, rcpath, sizeof(rcpath));
		char *file_name_start = strrchr(rcpath, '\\');
		if (file_name_start) ++file_name_start;
		else file_name_start = strchr(rcpath, 0);
		strcpy(file_name_start, "bbSDK.rc");

	} while (++i < 3 && false == FileExists(rcpath));

	// If a config file was found we read the plugin settings from it...
	// ...if not, the ReadXXX functions give us just the defaults.

	my.xpos     = ReadInt(rcpath, "bbSDK.xpos:", 10);
	my.ypos     = ReadInt(rcpath, "bbSDK.ypos:", 10);
	my.width    = ReadInt(rcpath, "bbSDK.width:", 80);
	my.height   = ReadInt(rcpath, "bbSDK.height:", 40);

	my.alphaEnabled     = ReadBool(rcpath, "bbSDK.alphaEnabled:", false);
	my.alphaValue       = ReadInt(rcpath,  "bbSDK.alphaValue:", 192);
	my.alwaysOnTop      = ReadBool(rcpath, "bbSDK.alwaysOntop:", true);
	my.drawBorder       = ReadBool(rcpath, "bbSDK.drawBorder:", true);
	my.snapWindow       = ReadBool(rcpath, "bbSDK.snapWindow:", true);
	my.pluginToggle     = ReadBool(rcpath, "bbSDK.pluginToggle:", true);
	my.useSlit          = ReadBool(rcpath, "bbSDK.useSlit:", true);

	strcpy(my.window_text, ReadString(rcpath, "bbSDK.windowText:", szAppName));
}

//===========================================================================

void GetStyleSettings(void)
{
	style_info.Frame = *(StyleItem *)GetSettingPtr(SN_TOOLBAR);
	if (false == (style_info.Frame.validated & VALID_TEXTCOLOR))
		style_info.Frame.TextColor = ((StyleItem *)GetSettingPtr(SN_TOOLBARLABEL))->TextColor;

	style_info.bevelWidth   = *(int*)GetSettingPtr(SN_BEVELWIDTH);
	style_info.borderWidth  = *(int*)GetSettingPtr(SN_BORDERWIDTH);
	style_info.borderColor  = *(COLORREF*)GetSettingPtr(SN_BORDERCOLOR);

	if (my.hFont) DeleteObject(my.hFont);
	my.hFont = CreateStyleFont((StyleItem *)GetSettingPtr(SN_TOOLBAR));
}

//===========================================================================
void ShowMyMenu(bool popup)
{
	Menu *pMenu, *pSub;

	// Create the main menu, with a title and an unique IDString
	pMenu = MakeNamedMenu("bbSDK", "bbSDK_IDMain", popup);

	// Create a submenu, also with title and unique IDString
	pSub = MakeNamedMenu("Configuration", "bbSDK_IDConfig", popup);

	// Insert first Item
	MakeMenuItem(pSub,      "Draw Border",  "@bbSDK.drawBorder", my.drawBorder);

	if (hSlit_present)
		MakeMenuItem(pSub,  "Use Slit",     "@bbSDK.useSlit", my.useSlit);

	if (NULL == my.hSlit)
	{
		// these are only available if outside the slit
		MakeMenuItem(pSub,      "Always On Top",        "@bbSDK.alwaysOnTop", my.alwaysOnTop);
		MakeMenuItem(pSub,      "Snap To Edges",        "@bbSDK.snapWindow", my.snapWindow);
		MakeMenuItem(pSub,      "Toggle With Plugins",  "@bbSDK.pluginToggle", my.pluginToggle);
		MakeMenuItem(pSub,      "Transparent",          "@bbSDK.alphaEnabled", my.alphaEnabled);
		MakeMenuItemInt(pSub,   "Alpha Value",          "@bbSDK.alphaValue", my.alphaValue, 0, 255);
	}
	// Insert the submenu into the main menu
	MakeSubmenu(pMenu, pSub, "Configuration");

	// The configurable text string
	MakeMenuItemString(pMenu,   "Display Text",     "@bbSDK.windowText", my.window_text);

	// ----------------------------------
	// add an empty line
	MakeMenuNOP(pMenu, NULL);

	// add an entry to let the user edit the setting file
	MakeMenuItem(pMenu, "Edit Settings", "@bbSDK.editRC", false);

	// and an about box
	MakeMenuItem(pMenu, "About", "@bbSDK.About", false);

	// ----------------------------------
	// Finally, show the menu...
	ShowMenu(pMenu);
}

//===========================================================================
// helper to handle commands  from the menu

void eval_menu_cmd(int mode, void *pValue, const char *msg_string)
{
	// Our rc_key prefix:
	const char rc_prefix[] = "bbSDK.";
	const int rc_prefix_len = sizeof rc_prefix - 1; // minus terminating \0

	char rc_string[80];

	// scan for a second argument after a space, like in "AlphaValue 200"
	const char *p = strchr(msg_string, ' ');
	int msg_len = p ? p++ - msg_string : strlen(msg_string);

	// Build the full rc_key. i.e. "bbSDK.<subkey>:"
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
			if (p)
			{
				*(int*)pValue = atoi(p);

				// write the new setting to the rc - file
				WriteInt(rcpath, rc_string, *(int*)pValue);
			}
			break;

		case M_STR: // --- set string variable -------------------
			if (p)
			{
				strcpy((char*)pValue, p);

				// bb4win 0.0.9x puts it in quotes, remove them...
				if (false == is_bblean) StrRemoveEncap((char*)pValue);

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

//*****************************************************************************
