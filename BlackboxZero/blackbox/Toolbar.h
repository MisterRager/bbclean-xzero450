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

#ifndef _BBTOOLBAR_H_
#define _BBTOOLBAR_H_

int beginToolbar(HINSTANCE);
void endToolbar(HINSTANCE);
void Toolbar_UpdatePosition(void);
void Toolbar_ShowMenu(bool pop);

extern ToolbarInfo TBInfo;

//===========================================================================

#endif
