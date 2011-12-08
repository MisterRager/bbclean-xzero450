/*
 ============================================================================
 This file is part of the Blackbox for Windows source code
 Copyright © 2001-2002 The Blackbox for Windows Development Team
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

#ifndef __BBKEYS_H
#define __BBKEYS_H

#define WIN32_LEAN_AND_MEAN

#pragma	warning(disable: 4786) // STL naming warnings

#include <windows.h>
#include <string>
#include <vector>
#include "../../blackbox/BBApi.h"

using namespace std;

//===========================================================================

struct VKTable
{
	char* key;
	int vKey;
};

//====================

struct HotkeyType
{
	string szCommand;
	string szAction;
	int sub;
	char ch;
};

//====================

class BBKeys
{
public:
	BBKeys(HINSTANCE);
	~BBKeys();
	friend LRESULT CALLBACK HotkeyProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
private:
	HWND hKeysWnd;
	void LoadHotkeys();
	void FreeHotkeys();
	HWND m_hMainWnd;
	HINSTANCE m_hMainInstance;
	typedef vector<HotkeyType> HotkeyVec;
	HotkeyVec hotKeys;
	int getDesktop(HWND h);
	int	getDesktopByRect(RECT r);
};

//===========================================================================

extern "C"
{
	__declspec(dllexport) int beginPlugin(HINSTANCE hPluginInstance);
	__declspec(dllexport) void endPlugin(HINSTANCE hPluginInstance);
	__declspec(dllexport) LPCSTR pluginInfo(int field);
}

//===========================================================================

#endif
