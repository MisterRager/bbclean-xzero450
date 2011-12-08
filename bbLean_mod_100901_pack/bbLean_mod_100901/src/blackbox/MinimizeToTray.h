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

#ifndef __MINIMIZETOTRAY_H
#define __MINIMIZETOTRAY_H

//===========================================================================

#include <shlobj.h>

#ifndef WM_TRAYICONMESSAGE
#define WM_TRAYICONMESSAGE (WM_USER + 1)
#endif

typedef struct nid_list{
	struct nid_list *next;
	NOTIFYICONDATA NID;
	HWND hWnd;
} nid_list;

bool MinimizeToTray(HWND hWnd);
HWND RestoreFromTray(UINT uID, bool bShow);
bool RemoveFromTray(HWND hWnd);
void FreeNIDList(void);

#endif
