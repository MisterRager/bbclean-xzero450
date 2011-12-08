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

#ifndef __BBDESK_H_
#define __BBDESK_H_

//===========================================================================

void Desk_Exit(void);
void Desk_Init(void);
void ShowExplorer(void);
void HideExplorer(void);
void set_focus_model(const char *fm_string);

void Desk_new_background(const char *rootCommand = NULL);
const char * Desk_extended_rootCommand(const char *p);
bool Desk_mousebutton_event(LPARAM button);
void Desk_reset_rootCommand(void);

//===========================================================================

#endif /* __BBDESK_H_ */
