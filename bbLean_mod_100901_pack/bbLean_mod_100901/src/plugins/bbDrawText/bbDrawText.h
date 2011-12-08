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

#include "BBApi.h"
#include <stdlib.h>
#include <commdlg.h>

// ----------------------------------
// Global vars

HINSTANCE g_hInstance;
HWND g_BBhwnd;

// receives the path to "bbDrawText.rc"
char rcpath[MAX_PATH];

// ----------------------------------
// Plugin window properties

struct plugin_properties
{
	// settings
	int xpos, ypos;
	int width, height;

	bool snapWindow;
	bool pluginToggle;

	// our plugin window
	HWND hwnd;

	// current state variables
	bool is_ontop;
	bool is_moving;
	bool is_sizing;
	bool is_hidden;

	// GDI objects
	HFONT hFont;

	// the text
	char filePath[MAX_PATH];
	char *window_text;
	char strFont[128];
	int nFontHeight;
	COLORREF textColor;
	COLORREF shadowColor;
	COLORREF outlineColor;

	// others
	char exeCmd[MAX_PATH];

} my;

// ----------------------------------
// some function prototypes

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

int my_substr_icmp(const char *a, const char *b);
void dbg_printf (const char *fmt, ...);
int BBDrawTextEx(HDC hDC, LPSTR lpString, int nCount, LPRECT lpRect, UINT uFormat, LPDRAWTEXTPARAMS lpDTParams, plugin_properties* lppiprop);
HFONT CreateMyFont(LPCSTR strFont, int nHeight);

// ----------------------------------
// plugin info

#define szVersion         "bbDrawText 0.0.3"
#define szAppName         "bbDrawText"
#define szInfoVersion     "0.0.3"
#define szInfoAuthor      "nocd5"
#define szInfoRelDate     "2009-04-07"

// ----------------------------------
// Interface declaration

extern "C"
{
	DLL_EXPORT int beginPlugin(HINSTANCE hPluginInstance);
	DLL_EXPORT void endPlugin(HINSTANCE hPluginInstance);
	DLL_EXPORT LPCSTR pluginInfo(int field);
};

#define _CopyRect(lprcDst,lprcSrc) (*lprcDst) = (*lprcSrc)
#define _InflateRect(lprc,dx,dy) (*(lprc)).left -= (dx), (*(lprc)).right += (dx), (*(lprc)).top -= (dy), (*(lprc)).bottom += (dy)
#define _OffsetRect(lprc,dx,dy) (*(lprc)).left += (dx), (*(lprc)).right += (dx), (*(lprc)).top += (dy), (*(lprc)).bottom += (dy)
#define _SetRect(lprc,xLeft,yTop,xRight,yBottom) (*(lprc)).left = (xLeft), (*(lprc)).right = (xRight), (*(lprc)).top = (yTop), (*(lprc)).bottom = (yBottom)
#define _CopyOffsetRect(lprcDst,lprcSrc,dx,dy) (*(lprcDst)).left = (*(lprcSrc)).left + (dx), (*(lprcDst)).right = (*(lprcSrc)).right + (dx), (*(lprcDst)).top = (*(lprcSrc)).top + (dy), (*(lprcDst)).bottom = (*(lprcSrc)).bottom + (dy)

