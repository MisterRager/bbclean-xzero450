/* ------------------------------------------------------------------------- */
/*
  This file is part of the bbLean source code
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
*/
/* ------------------------------------------------------------------------- */
/* bool functions */

#include "bblib.h"

int get_false_true(const char *arg)
{
    if (arg) {
        if (0==stricmp(arg, "true"))
            return 1;
        if (0==stricmp(arg, "false"))
            return 0;
    }
    return -1;
}

const char *false_true_string(int f)
{
    return f ? "true" : "false";
}

void set_bool(void *v, const char *arg)
{
    char *p = (char *)v;
    int f = get_false_true(arg);
    *p = -1 == f ? 0 == *p : 0 != f;
}

/* ------------------------------------------------------------------------- */
