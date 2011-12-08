/*
 ============================================================================

  This file is part of the bbLean source code
  Copyright © 2001-2003 The Blackbox for Windows Development Team
  Copyright © 2004 grischka

  http://bb4win.sourceforge.net/bblean
  http://sourceforge.net/projects/bb4win

 ============================================================================

  bbLean and bb4win are free software, released under the GNU General
  Public License (GPL version 2 or later), with an extension that allows
  linking of proprietary modules under a controlled interface. This means
  that plugins etc. are allowed to be released under any license the author
  wishes. For details see:

  http://www.fsf.org/licenses/gpl.html
  http://www.fsf.org/licenses/gpl-faq.html#LinkingOverControlledInterface

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
  for more details.

 ============================================================================
*/

#ifndef __VOLUMECONTROL_H
#define __VOLUMECONTROL_H

#ifndef DLL_EXPORT
  #define DLL_EXPORT __declspec(dllexport)
#endif

extern "C" {
	BOOL WINAPI DllMainCRTStartup(HANDLE hModule, DWORD dwFunction, LPVOID lpNot);
	DLL_EXPORT void SetVolume(int nVol);
	DLL_EXPORT int  GetVolume();
//     DLL_EXPORT void ToggleMute();
	DLL_EXPORT void SetMute(bool bMute);
	DLL_EXPORT bool GetMute();
}

bool getsetMute(int nMute);
int  getsetVolume(int nVol);

#endif
