/* ==========================================================================

  This file is part of the bbLean source code
  Copyright © 2001-2003 The Blackbox for Windows Development Team
  Copyright © 2004-2009 grischka

  http://bb4win.sourceforge.net/bblean
  http://developer.berlios.de/projects/bblean

  bbLean is free software, released under the GNU General Public License
  (GPL version 2). For details see:

  http://www.fsf.org/licenses/gpl.html

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
  for more details.

  ========================================================================== */

#ifndef _BBDESK_H_
#define _BBDESK_H_

//===========================================================================

void Desk_Exit(void);
void Desk_Init(void);
void ShowExplorer(void);
void HideExplorer(void);

void Desk_new_background(const char *rootCommand);
const char * Desk_extended_rootCommand(const char *p);
bool Desk_mousebutton_event(int button);
void Desk_Reset(bool all);
HBITMAP Desk_getbmp(void);

extern HWND hDesktopWnd;

//===========================================================================

#endif /* _BBDESK_H_ */
