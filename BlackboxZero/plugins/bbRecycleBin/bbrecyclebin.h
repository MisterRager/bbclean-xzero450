/*
 ============================================================================

  This program is free software, released under the GNU General Public License
  (GPL version 2 or later). See for details:

  http://www.fsf.org/licenses/gpl.html

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
  for more details.

 ============================================================================
*/

#ifdef _MSC_VER
  #ifdef BBOPT_STACKDUMP
    #define TRY if (1)
    #define EXCEPT if(0)
  #else
    #define TRY _try
    #define EXCEPT _except(1)
  #endif
  #define stricmp _stricmp
  #define strnicmp _strnicmp
  #define memicmp _memicmp
  #define strlwr _strlwr
  #define strupr _strupr
#else
  #undef BBOPT_STACKDUMP
#endif

#ifndef __BBRECYCLEBIN_H
#define __BBRECYCLEBIN_H

#ifndef magicDWord
#define magicDWord		0x49474541
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif


#include <windows.h>
#include <shlwapi.h>
#include <commctrl.h>
#include <tchar.h>
#include "BBApi.h"

#ifndef SLIT_ADD
#define SLIT_ADD 11001
#endif

#ifndef SLIT_REMOVE
#define SLIT_REMOVE 11002
#endif

#ifndef SLIT_UPDATE
#define SLIT_UPDATE 11003
#endif

#ifndef WS_EX_LAYERED
#define WS_EX_LAYERED	0x00080000
#define LWA_COLORKEY	0x00000001
#define LWA_ALPHA	0x00000002
#endif

#define MAXPATH 80
#define IDT_RECYCLEBIN_TIMER 4500

#define MAXRECYCLEBINCNTSIZE 10
#define GUID_RECYCLEBIN	_T("::{645FF040-5081-101B-9F08-00AA002F954E}")
#define IDR_MENU 101

//===========================================================================

HINSTANCE hInstance;
HWND hwndPlugin, hwndBlackbox;

bool inSlit = false;
HWND hSlit = NULL;

int msgs[] = {BB_RECONFIGURE, BB_REDRAWGUI, BB_BROADCAST, 0};

char rcpath[MAX_PATH];
char stylepath[MAX_PATH];

int xpos, ypos;
int width, height;
int alpha;
int fontHeight;
int styleType;
int textPos;
bool alwaysOnTop;
bool snapWindow;
bool transparency;
bool transBack;
bool pluginToggle;
bool showBorder;
bool allowtip;
bool lockPos;

DWORD justify;
bool setAttr;
bool usingWin2kXP;
bool usingWin2k;
OSVERSIONINFO  osvinfo;

HDC hdc;
HGDIOBJ otherfont;
HBITMAP bufbmp;
HFONT hFont;

char fontFace[256];
int fontSize;
COLORREF fontColor;

void ShowMyMenu(bool popup);
void SetWindowModes(void);
void OnPaint(HWND hwnd);
StyleItem myStyleItem, tbStyleItem;
int bevelWidth;
int borderWidth;
COLORREF borderColor;

Menu *myMenu, *configSubmenu, *settingsSubmenu,  *styleSubmenu, *justifySubmenu,
*fontSubmenu, *iconSubmenu, *emptySubmenu, *fullSubmenu, *textPosSubmenu, *hMenu;

char displayString[MAX_LINE_LENGTH];
char *szTemp;

int ScreenWidth;
int ScreenHeight;

int newWidth;
int newHeight;

//===========================================================================

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL WINAPI BBSetLayeredWindowAttributes(HWND hwnd, COLORREF crKey, BYTE bAlpha, DWORD dwFlags);

void GetStyleSettings();
void ReadRCSettings();
void WriteRCSettings();

//===========================================================================

void SetRecyclebin();
void OnDropFiles(HDROP);

HICON hIcon;

bool g_isEmpty = true;
bool bFull = true;
char  emptyicon[MAX_LINE_LENGTH];
char  fullicon[MAX_LINE_LENGTH];
int iconSize;

HDROP hDrop;
DWORD g_nBytes;
DWORD g_nItems;
int rbCntrPtrPos;

#define m_alloc(n) malloc(n)
#define c_alloc(n) calloc(1,n)
#define m_free(v) free(v)

HWND hToolTips;
void SetToolTip(RECT *tipRect, char *tipText);
void ClearToolTips(void);
void SetAllowTip(bool);

bool ResizeMyWindow(int newWidth, int newHeight);
bool useRefFont;
bool useTwoLine;
bool insertSpace;

int checkInterval;

//===========================================================================

extern "C"
{
	__declspec(dllexport) LPCSTR pluginInfo(int field);
	__declspec(dllexport) void endPlugin(HINSTANCE hMainInstance);
	__declspec(dllexport) int beginPlugin(HINSTANCE hMainInstance);
	__declspec(dllexport) int beginSlitPlugin(HINSTANCE hMainInstance, HWND hBBSlit);
	__declspec(dllexport) int beginPluginEx(HINSTANCE hMainInstance, HWND hBBSlit);
}

//===========================================================================

#endif
