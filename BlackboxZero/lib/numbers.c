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
/* number functions */

#include "bblib.h"

int imax(int a, int b)
{
    return a>b?a:b;
}

int imin(int a, int b)
{
    return a<b?a:b;
}

int iminmax(int a, int b, int c)
{
    if (a>c) a=c;
    if (a<b) a=b;
    return a;
}

int iabs(int a)
{
    return a < 0 ? -a : a;
}

int is_alpha(int c)
{
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

int is_digit(int c)
{
    return c >= '0' && c <= '9';
}

int is_alnum(int c)
{
    return is_alpha(c) || is_digit(c);
}

/* ------------------------------------------------------------------------- */
