/*
 ============================================================================
 Blackbox for Windows: Plugin SDK example
 ============================================================================
 Copyright © 2001-2003 The Blackbox for Windows Development Team
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

#ifndef __EXAMPLE_H
#define __EXAMPLE_H

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif

#define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES 1

#include <windows.h> // BlackboxZero 1.14.2012
#include "BBApi.h"
#include "resource.h"
#include <time.h>
#include <vector>
#include <string>
//#include <aggressiveoptimize.h>

using namespace std;

//===========================================================================

const long magicDWord = 0x49474541; // BlackboxZero 1.14.2012

HINSTANCE hInstance;
HWND hwndPlugin, hwndBlackbox;

// Blackbox messages we want to "subscribe" to:
// BB_RECONFIGURE -> Sent when changing style and on reconfigure
// BB_BROADCAST -> Broadcast message (bro@m)

/**
BlackboxZero 1.14.2012
**/
#ifndef BB_MINIMIZE
#define BB_MINIMIZE BB_WINDOWMINIMIZE
#endif
/** **/

int msgs[] = 
{BB_RECONFIGURE, BB_BROADCAST, /*BB_LISTDESKTOPS,*/ BB_WORKSPACE, BB_BRINGTOFRONT, BB_MINIMIZE, 0};

int xpos, ypos, xpos2, ypos2;
int width, height;
int vScreenWidth, vScreenHeight, vScreenLeft, vScreenTop;
int vScreenRight, vScreenBottom;
int screenWidth, screenHeight, screenLeft, screenTop;
int screenRight, screenBottom;
bool alwaysOnTop;
bool snapWindow, snapWindowOld;

// Toolbar inherited styling
bool inheritToolbar;
RECT tRect;
ToolbarInfo tbInfo;
int styleBevelWidth;

bool hideWindow;

//char windowText[MAX_LINE_LENGTH];
bool transparency;
int transparencyAlpha;

bool usingWin2kXP;

struct FRAME
{
	COLORREF color;
	COLORREF colorTo;
	COLORREF borderColor;

	int bevelWidth;
	int borderWidth;

	bool drawBorder;

	StyleItem *style;
};

struct BUTTON
{
	COLORREF color;
	COLORREF colorTo;
	COLORREF fontColor;

	char fontFace[256];
	
	int fontSize;
	int fontWeight;

	StyleItem *style;
};

struct BUTTONDN
{
	COLORREF color;
	COLORREF colorTo;
	COLORREF fontColor;

	StyleItem *style;
};

// Drawing stuff
struct FRAME frame;
struct BUTTON button;
struct BUTTONDN buttonPressed;

// Mouse buttons
bool rightButtonDown = false, rightButtonDownNC = false;
bool middleButtonDown = false, middleButtonDownNC = false;
bool leftButtonDown = false;
bool inButton = false;
RECT buttonRect;

// Styling stuff
//int num = 0, random = 0;
char styleToSet[4096];
bool noStyleTwice;

// Timer
int changeTime = 300000;
bool timer = true;

bool changeOnStart = false;

// Different wallpapers on each workspace
typedef vector<string> StringVector;
//StringVector workspaceRootCommand;

//bool rootCommands;

//int currentDesktop, oldDesk;
//char styleRootCommand[MAX_LINE_LENGTH];

// Positioning
//bool bleft = false, bright = false, btop = false, bbottom = false;

// File paths
char rcpath[MAX_PATH];
char stylepath[MAX_PATH];
char listPath[4096];

StringVector styleList;

bool badPath = false;

// Menus
Menu *BBStyleMenu, *BBStyleWindowSubMenu, *BBStyleStyleSubMenu, *BBStyleConfigSubMenu;

// Slit items
bool inSlit = false;	//Are we loaded in the slit? (default of no)
bool useSlit = false;	//Are we set to load in the slit from the RC?
HWND hSlit;				//The Window Handle to the Slit (for if we are loaded)

//===========================================================================

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

void GetStyleSettings();
void InitRC();
void ReadRCSettings();
void WriteRCSettings();

void UpdateMonitorInfo();

void InitList(char *path, bool init);
void NewStyle(bool seed);
void SetStyle();

void TrackMouse();
bool ClickMouse();

void DisplayMenu();

void ChangeRoot();
void GetRCRootCommands();

void ToggleSlit();

BOOL WINAPI BBSetLayeredWindowAttributes(HWND hwnd, COLORREF crKey, BYTE bAlpha, DWORD dwFlags);

//===========================================================================

extern "C"
{
	__declspec(dllexport) int beginPlugin(HINSTANCE hMainInstance);
	__declspec(dllexport) void endPlugin(HINSTANCE hMainInstance);
	__declspec(dllexport) LPCSTR pluginInfo(int field);

	// This is the function BBSlit uses to load your plugin into the Slit
	__declspec(dllexport) int beginSlitPlugin(HINSTANCE hMainInstance, HWND hBBSlit);
	__declspec(dllexport) int beginPluginEx(HINSTANCE hMainInstance, HWND hBBSlit);  
}

//===========================================================================

#endif
