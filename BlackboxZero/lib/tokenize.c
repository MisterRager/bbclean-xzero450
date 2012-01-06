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
/* tokenizing */

#include "bblib.h"

int nexttoken(const char **p_out, const char **p_in, const char *delims)
{
    const char *s, *a, *e;
    char c, q;
    int delim_spc;

    delim_spc = NULL == delims || strchr(delims, ' ');

    for (a = e = s = *p_in, q = 0; 0 != (c = *s);) {
        ++s;
        if (0==q) {
            if ('\"'==c || '\''==c)
                q = c;
            else
            if (IS_SPC(c)) {
                if (e == a) {
                    a = e = s;
                    continue;
                }
                if (delim_spc)
                    break;
            }
            if (delims && strchr(delims, c))
                break;
        } else if (c==q) {
            q=0;
        }
        e = s;
    }
    while (e > a && IS_SPC(e[-1]))
        --e;
    skip_spc(&s);
    *p_out = a, *p_in = s;
    return e - a;
}

/* ------------------------------------------------------------------------- */
/* Function: NextToken */
/* Purpose: Copy the first token of 'string' seperated by the delim to 'buf', */
/*  update input position to start of next token */

char* NextToken(char* buf, const char** string, const char *delims)
{
    const char *a; int n;
    n = nexttoken(&a, string, delims);
    return extract_string(buf, a, imin(n, MAX_PATH - 1));
}

/* ------------------------------------------------------------------------- */
/* Purpose:  extract a token from a string between two delimiters */

int get_string_within (char *dest, int size, const char **p_src, const char *delims)
{
    const char *a, *b, *p;
    char c, n, d, e;
    n = 0; /* balance-counter for nested pairs of delims */
    a = NULL;
    d = delims[0], e = delims[1];
    for (p = *p_src; 0 != (c = *p++); ) {
        if (c == d && 0 == n++) {
            a = p;
        } else if (c == e && 0 == --n) {
            b = (*p_src = p) - 1;
            while (a < b && IS_SPC(*a))
                ++a;
            while (b > a && IS_SPC(b[-1]))
                --b;
            extract_string(dest, a, imin(b-a, size-1));
            return 1;
        }
    }
    *dest = 0;
    return 0;
}

/* ------------------------------------------------------------------------- */
/* separate the command from the path spec. in a string like: */
/*  "c:\bblean\backgrounds >> @BBCore.rootCommand bsetroot -full "%1"" */

const char *get_special_command(const char **p_path, char *buffer, int size)
{
    const char *in, *a, *b;
    a = strstr(in = *p_path, ">>");
    if (NULL == a)
        return NULL;
    b = a + 2;
    skip_spc(&b);
    while (a > in && IS_SPC(a[-1]))
        --a;
    *p_path = extract_string(buffer, in, imin(a-in, size-1));
    return b;
}

int skip_spc(const char **pp)
{
    int c;
    while (c = **pp, IS_SPC(c) && c)
        ++*pp;
    return c;
}

/* ------------------------------------------------------------------------- */
