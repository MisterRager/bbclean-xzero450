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
/* string functions */

#include "bblib.h"

int replace_string(char *out, int bufsize, int offset, int len, const char *in)
{
    int len2, restlen, newlen;
    len2 = strlen(in);
    restlen = strlen(out+offset+len) + 1;
    newlen = offset+len2+restlen;
    if (newlen > bufsize)
        return 0;
    memmove(out+offset+len2, out+offset+len, restlen);
    memmove(out+offset, in, len2);
    return newlen;
}

char *extract_string(char *dest, const char *src, int n)
{
    memcpy(dest, src, n);
    dest[n] = 0;
    return dest;
}

char *strcpy_max(char *dest, const char *src, int maxlen)
{
    int l = strlen(src);
    return extract_string(dest, src, l < maxlen ? l : maxlen-1);
}

char* stristr(const char *aa, const char *bb)
{
    const char *a, *b; int c, d;
    do {
        for (a = aa, b = bb;;++a, ++b) {
            if (0 == (c = *b))
                return (char*)aa;
            if (0 != (d = *a^c)
                && (d != 32 || (c |= 32) < 'a' || c > 'z'))
                break;
        }
    } while (*aa++);
    return NULL;
}

int get_string_index (const char *key, const char * const * string_array)
{
    int i; const char *s;
    for (i=0; NULL != (s = *string_array); i++, string_array++)
        if (0==stricmp(key, s))
            return i;
    return -1;
}

/* ------------------------------------------------------------------------- */

char *new_str_n(const char *s, int n)
{
    char *d;
    if (NULL == s) return NULL;
    d = (char*)m_alloc(n+1);
    memcpy(d, s, n);
    d[n] = 0;
    return d;
}

char *new_str(const char *s)
{
    return s ? new_str_n(s, strlen(s)) : NULL;
}

void free_str(char **s)
{
    if (*s) m_free(*s), *s=NULL;
}

void replace_str(char **s, const char *n)
{
    if (*s) m_free(*s);
    *s = new_str(n);
}

char *concat_str(const char *s1, const char *s2)
{
    int l1 = strlen(s1);
    int l2 = strlen(s2);
    char *p = (char*)m_alloc(l1 + l2 + 1);
    memcpy(p, s1, l1);
    memcpy(p + l1, s2, l2);
    p[l1+l2] = 0;
    return p;
}

/* ------------------------------------------------------------------------- */
/* string format & alloc */

static int cstr_cpy(char *out, const char *in)
{
    const char *s = in;
    char *d = out, c;
    if (out) *d = '"';
    ++d;
    while (0 != (c = *s++)) {
        if (c == '\\' || c == '\"') {
            if (out) *d = '\\';
            ++d;
        }
        if (out) *d = (char)c;
        ++d;
    }
    if (out) *d = '"';
    ++d;
    return d - out;
}

char *m_formatv(const char *fmt, va_list arg_list)
{
    va_list arg;
    char *out, *ptr, c;
    const char *f, *cp;
    char buff[100], tmp[10];
    int n, len;

    cp = NULL;
    out = NULL;

_restart:
    arg = arg_list, ptr = out, len = 0;
    for (f = fmt;;++f) {
        switch (c = *f) {
        case '\0':
_quit:
            if (ptr) {
                *ptr = 0;
                /*dbg_printf("%s (%d): <%s>", fmt, strlen(out), out); */
                return out;
            }
            out = (char*)m_alloc(len + 1);
            goto _restart;

        default:
_char:
            if (ptr)
                *ptr++ = c;
            ++len;
            continue;

        case '%':
            switch (c = *++f) {
            case '\0':
                goto _quit;
            default:
                goto _char;

            case 'x':
            case 'X':
            case 'd':
            case 'u':
            case 'c':
                tmp[0] = '%', tmp[1] = c, tmp[2] = 0;
                sprintf(buff, tmp, va_arg(arg, int));
                cp = buff;
                goto put;

            case 'b':
                cp = false_true_string(0 != (char)va_arg(arg, int));
                goto put;

            case '1':
            case 's':
            case 'q':
                cp = va_arg(arg, const char*);
                if (NULL == cp)
                    cp = "(null)";
                if ('q' == c) {
                    n = cstr_cpy(ptr, cp);
                } else {
            put:
                    n = strlen(cp);
                    if (ptr)
                        memcpy(ptr, cp, n), ptr += n;
                }
                len += n;
                continue;
            }
        }
    }
}

char *m_format(const char *fmt, ...)
{
    va_list arg_list;
    va_start(arg_list, fmt);
    return m_formatv(fmt, arg_list);
}

/* ------------------------------------------------------------------------- */

// strlwr a keyword, calculate hash value and length
unsigned calc_hash(char *p, const char *s, int *pLen, int delim)
{
    unsigned h; int c; char *d = p;
    for (h = 0; 0 != (c = *s) && delim != c; ++s, ++d)
    {
        if (c >= 'A' && c <= 'Z')
            c += 32;
        *d = (char)c;
        if ((h ^= c) & 1)
            h^=0xedb88320;
        h>>=1;
    }
    *d = 0;
    *pLen = d - p;
    return h;
}

/* ------------------------------------------------------------------------- */
