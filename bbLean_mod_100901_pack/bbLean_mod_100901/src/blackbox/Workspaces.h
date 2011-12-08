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

#ifndef __BBWORKSPACES_H
#define __BBWORKSPACES_H

//===========================================================================
void Workspaces_Init(void);
void Workspaces_Exit(void);
void Workspaces_Reconfigure(void);
void Workspaces_Command(UINT msg, WPARAM wParam, LPARAM lParam);
void Workspaces_GatherWindows(void);

void TaskWndProc(WPARAM, HWND);
void Workspaces_handletimer(void);

bool focus_top_window(void);
void ForceForegroundWindow(HWND theWin);
void SwitchToWindow(HWND hwnd);
void SwitchToBBWnd(void);
HWND GetTopTask(void);
void get_desktop_info(DesktopInfo *deskInfo, int i);

extern int currentScreen;
extern int Settings_workspaces;

//===========================================================================
#endif
