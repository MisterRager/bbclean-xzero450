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
#include <mmsystem.h>
#include "MasterVolume.h"

#ifdef NOSTDLIB
//===========================================================================
BOOL WINAPI DllMainCRTStartup(HANDLE hModule, DWORD dwFunction, LPVOID lpNot){
	return TRUE;
}
#endif

//===========================================================================
// SetVolume
// Volume in percents (0-100[%])
//===========================================================================
void SetVolume(int nVol){
	getsetVolume(nVol);
}

//===========================================================================
// GetVolume
// Volume in percents (0-100[%])
//===========================================================================
int GetVolume(){
	return getsetVolume(-1);
}

//===========================================================================
// SetMute
// bMute : true  => set mute
//         false => unset mute
//===========================================================================
void SetMute(bool bMute){
	getsetMute(bMute);
}

//===========================================================================
// GetMute
// Return : true  => volume is mute
//          false => volume is not mute
//===========================================================================
bool GetMute(){
	return getsetMute(-1);
}

//===========================================================================
int getsetVolume(int nVol){
	HMIXER m_hMixer;
	MIXERCAPS m_mxcaps;
	DWORD m_dwMinimum, m_dwMaximum;
	DWORD m_dwVolumeControlID;
	
	m_hMixer = NULL;
	::ZeroMemory(&m_mxcaps, sizeof(MIXERCAPS));
	if (::mixerOpen(&m_hMixer,
					0,
					0,
					0,
					MIXER_OBJECTF_MIXER)
		!= MMSYSERR_NOERROR)
	{
		return 0;
	}

	if (::mixerGetDevCaps(reinterpret_cast<UINT>(m_hMixer),
						  &m_mxcaps, sizeof(MIXERCAPS))
		!= MMSYSERR_NOERROR)
	{
		return 0;
	}
	// get dwLineID
	MIXERLINE mxl;
	mxl.cbStruct = sizeof(MIXERLINE);
	mxl.dwComponentType = MIXERLINE_COMPONENTTYPE_DST_SPEAKERS;
	if (::mixerGetLineInfo(reinterpret_cast<HMIXEROBJ>(m_hMixer),
						   &mxl,
						   MIXER_OBJECTF_HMIXER |
						   MIXER_GETLINEINFOF_COMPONENTTYPE)
		!= MMSYSERR_NOERROR)
	{
		return 0;
	}
	// get dwControlID
	MIXERCONTROL mxc;
	MIXERLINECONTROLS mxlc;
	mxlc.cbStruct = sizeof(MIXERLINECONTROLS);
	mxlc.dwLineID = mxl.dwLineID;
	mxlc.dwControlType = MIXERCONTROL_CONTROLTYPE_VOLUME;
	mxlc.cControls = 1;
	mxlc.cbmxctrl = sizeof(MIXERCONTROL);
	mxlc.pamxctrl = &mxc;
	if (::mixerGetLineControls(reinterpret_cast<HMIXEROBJ>(m_hMixer),
							   &mxlc,
							   MIXER_OBJECTF_HMIXER |
							   MIXER_GETLINECONTROLSF_ONEBYTYPE)
		!= MMSYSERR_NOERROR)
	{
		return 0;
	}
	m_dwMinimum = mxc.Bounds.dwMinimum;
	m_dwMaximum = mxc.Bounds.dwMaximum;
	m_dwVolumeControlID = mxc.dwControlID;
	
	
	DWORD dwVol = (m_dwMinimum + nVol*(m_dwMaximum-m_dwMinimum)+1)/100;
	MIXERCONTROLDETAILS_UNSIGNED mxcdVolume = { dwVol };
	MIXERCONTROLDETAILS mxcd;
	mxcd.cbStruct = sizeof(MIXERCONTROLDETAILS);
	mxcd.dwControlID = m_dwVolumeControlID;
	mxcd.cChannels = 1;
	mxcd.cMultipleItems = 0;
	mxcd.cbDetails = sizeof(MIXERCONTROLDETAILS_UNSIGNED);
	mxcd.paDetails = &mxcdVolume;
	
	if(nVol>=0){ /// set
		if (::mixerSetControlDetails(reinterpret_cast<HMIXEROBJ>(m_hMixer),
									 &mxcd,
									 MIXER_OBJECTF_HMIXER |
									 MIXER_SETCONTROLDETAILSF_VALUE)
			!= MMSYSERR_NOERROR)
		{
			return 0;
		}
	}else{
		if (::mixerGetControlDetails(reinterpret_cast<HMIXEROBJ>(m_hMixer),
									 &mxcd,
									 MIXER_OBJECTF_HMIXER |
									 MIXER_GETCONTROLDETAILSF_VALUE)
			!= MMSYSERR_NOERROR)
		{
			return 0;
		}
		
		nVol = (100*(mxcdVolume.dwValue-m_dwMinimum+1))/(m_dwMaximum-m_dwMinimum);
	}
	mixerClose(m_hMixer);
		
	return nVol;
}

// volum mete
bool getsetMute(int nMute){
	int                          nOut;
	int                          nDev;
	unsigned int                 i, j;
	MIXERCAPS                    mxcaps;
	MIXERCONTROLDETAILS          mxcd;
	MIXERLINE                    mxl;
	MIXERLINECONTROLS            mxlc;
	HMIXER                       hmx;
	MMRESULT                     mmr;
	MIXERCONTROLDETAILS_UNSIGNED pmxcd_u;

	if ((nDev = mixerGetNumDevs()) <= 0) return pmxcd_u.dwValue;
	for (nOut = 0; nOut < nDev; ++nOut) {
		if (mixerGetDevCaps(nOut, &mxcaps, sizeof(MIXERCAPS)) != MMSYSERR_NOERROR)
			return pmxcd_u.dwValue;
		mmr = mixerOpen(&hmx, nOut, 0, 0, MIXER_OBJECTF_MIXER);
		if (mmr != MMSYSERR_NOERROR) continue;
		for (i = 0; i < mxcaps.cDestinations; ++i) {
			mxl.cbStruct = sizeof(MIXERLINE);
			mxl.dwDestination = i;
			mxl.dwSource = 0;
			mxl.dwComponentType = MIXERLINE_COMPONENTTYPE_DST_SPEAKERS;
			mmr = mixerGetLineInfo((HMIXEROBJ)hmx, &mxl, MIXER_GETLINEINFOF_DESTINATION);
			if ( mmr != MMSYSERR_NOERROR || mxl.fdwLine != MIXERLINE_LINEF_ACTIVE) 
				continue;

			mxlc.cbStruct  = sizeof(MIXERLINECONTROLS);
			mxlc.dwLineID  = mxl.dwLineID;
			mxlc.cControls = mxl.cControls;
			mxlc.cbmxctrl  = sizeof(MIXERCONTROL);
			mxlc.pamxctrl  = (MIXERCONTROL*)malloc(sizeof(MIXERCONTROL) * mxl.cControls);
			mmr = mixerGetLineControls((HMIXEROBJ)hmx, &mxlc,MIXER_GETLINECONTROLSF_ALL);
			for (j = 0; j < mxl.cControls; ++j) {
				if (mxlc.pamxctrl[j].dwControlType == MIXERCONTROL_CONTROLTYPE_MUTE)
					break;
			}
			if (j >= mxl.cControls) {
				free(mxlc.pamxctrl);
				continue;
			}

			mxcd.cbStruct       = sizeof(MIXERCONTROLDETAILS);
			mxcd.dwControlID    = mxlc.pamxctrl[j].dwControlID;
			mxcd.cChannels      = 1; // Uniform Control
			mxcd.cMultipleItems = mxlc.pamxctrl[j].cMultipleItems;
			mxcd.cbDetails      = sizeof(MIXERCONTROLDETAILS_UNSIGNED);
			mxcd.paDetails      = &pmxcd_u;
			mixerGetControlDetails((HMIXEROBJ)hmx, &mxcd, MIXER_GETCONTROLDETAILSF_VALUE);

			if (0 <= nMute){
				pmxcd_u.dwValue = nMute;
				mixerSetControlDetails((HMIXEROBJ)hmx, &mxcd, MIXER_GETCONTROLDETAILSF_VALUE);
			}

			free(mxlc.pamxctrl);
		}
		mixerClose(hmx);
	}
	return pmxcd_u.dwValue;
}

