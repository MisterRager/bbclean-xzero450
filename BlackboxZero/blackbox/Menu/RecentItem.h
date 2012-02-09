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

#ifndef __RECENTITEM_H
#define __RECENTITEM_H

#include "..\BB.h"
#include "..\Settings.h"
//#include "..\Tinylist.h"
#include <shlobj.h>
#include <limits.h>

typedef struct ItemList{
	struct ItemList *next;
	char szItem[MAX_PATH];
	UINT nFrequency;
} ItemList;

#define _strcat(dest, src) strncat(dest, src, sizeof(dest) - strlen(dest) - 1)
#define _strcpy(dest, src) strncpy(dest, src, sizeof(dest) - 1)
#define _sprintf(str, format, ...) snprintf(str, sizeof(str) - 1, format, __VA_ARGS__)

int CreateRecentItemMenu(char *pszFileName, char *pszCommand, char *pszTitle, char *pszIcon, int nKeep, int nSort, bool bBeginEnd);
ItemList *sortlist(ItemList *il);

#endif /* __RECENTITEM_H */
