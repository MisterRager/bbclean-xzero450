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

#include <windows.h>
#include "VolumeControl.h"
//===========================================================================
// constructor
VolumeControl::VolumeControl(const char *pszDllName){
    LoadVolCtrlDLL(pszDllName);
}
// destructor
VolumeControl::~VolumeControl(){
    if (m_hVolCtrlDLL){
        FreeLibrary(m_hVolCtrlDLL);
        m_hVolCtrlDLL = NULL;
    }
}

//===========================================================================
void VolumeControl::LoadVolCtrlDLL(const char *pszDllName){
    // Load Library & get function pointer
    if (m_hVolCtrlDLL){
        FreeLibrary(m_hVolCtrlDLL);
        m_hVolCtrlDLL = NULL;
    }
    m_hVolCtrlDLL = LoadLibrary(pszDllName);
    if (m_hVolCtrlDLL){
        *(FARPROC*)&m_pfnSetVolume  = GetProcAddress(m_hVolCtrlDLL, "SetVolume");
        *(FARPROC*)&m_pfnGetVolume  = GetProcAddress(m_hVolCtrlDLL, "GetVolume");
        *(FARPROC*)&m_pfnToggleMute = GetProcAddress(m_hVolCtrlDLL, "ToggleMute");
        *(FARPROC*)&m_pfnSetMute    = GetProcAddress(m_hVolCtrlDLL, "SetMute");
        *(FARPROC*)&m_pfnGetMute    = GetProcAddress(m_hVolCtrlDLL, "GetMute");
    }
    else{
        m_pfnSetVolume  = NULL;
        m_pfnGetVolume  = NULL;
        m_pfnToggleMute = NULL;
        m_pfnSetMute    = NULL;
        m_pfnGetMute    = NULL;
    }
}

//===========================================================================
// ::BBSetVolume
// Volume in percents (0-100[%])
//===========================================================================
void VolumeControl::BBSetVolume(int nVol){
    if (m_pfnSetVolume) m_pfnSetVolume(nVol);
    return;
}

//===========================================================================
// ::BBGetVolume
// Volume in percents (0-100[%])
//===========================================================================
int  VolumeControl::BBGetVolume(){
    return m_pfnGetVolume ? m_pfnGetVolume() : -1;
}

//===========================================================================
// ::BBToggleMute
// Toggle Mute state
//===========================================================================
void VolumeControl::BBToggleMute(){
    if (m_pfnToggleMute)
        m_pfnToggleMute();
    else if (m_pfnGetMute && m_pfnSetMute)
        m_pfnSetMute(!m_pfnGetMute());
    return;
}

//===========================================================================
// ::BBSetMute
// bMute : true  => set mute
//         false => unset mute
//===========================================================================
void VolumeControl::BBSetMute(bool bMute){
    if (m_pfnSetMute) m_pfnSetMute(bMute);
    return;
}

//===========================================================================
// ::BBGetMute
// Return : true  => volume is mute
//          false => volume is not mute
//===========================================================================
bool VolumeControl::BBGetMute(){
    return m_pfnGetMute ? m_pfnGetMute() : false;
}

