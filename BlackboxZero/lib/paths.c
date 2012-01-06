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
/* path functions */

#include "bblib.h"
#include "win0x500.h"

char* unquote(char *src)
{
    int l = strlen(src);
    if (l >= 2 && (src[0] == '\"' || src[0] == '\'') && src[l-1] == src[0])
        return extract_string(src, src+1, l-2);
    return src;
}

char *quote_path(char *path)
{
    int l = strlen(path);
    if (l >= 2
     && (path[0] == '\"' || path[0] == '\'')
     && path[l-1] == path[0])
        return path;

    memmove(path+1, path, l);
    path[0] = path[l+1] = '\"';
    path[l+2] = 0;
    return path;
}

/* ------------------------------------------------------------------------- */

const char *file_basename(const char *path)
{
    int nLen = strlen(path);
    while (nLen && !IS_SLASH(path[nLen-1])) nLen--;
    return path + nLen;
}

const char *file_extension(const char *path)
{
    const char *p, *e;
    p = e = strchr(path, 0);
    while (--p >= path) {
        if ('.' == *p)
            return p;
        if (IS_SLASH(*p))
            break;
    }
    return e;
}

char *file_directory(char *buffer, const char *path)
{
    const char *f;
    f = file_basename(path);
    if (f >= path+2 && f[-2] != ':')
        --f;
    return extract_string(buffer, path, f-path);
}

/* remove trailing slashes but keep/add for root (C:\) */
char *fix_path(char *path)
{
    int l = strlen(path);
    if (l>=2 && IS_SLASH(path[l-1]) && ':' != path[l-2])
        path[l-1] = 0;
    else
    if (l && path[l-1] == ':')
        path[l] = '\\', path[l+1] = 0;
    return path;
}

int is_absolute_path(const char *path)
{
    const char *p, *q; char c;
    for (p = q = path; 0 != (c = *p);) {
        if (IS_SLASH(c))
            return p == q;
        ++p;
        if (':' == c) {
            q = p;
            if (':' == *p)
                return 1;
        }
    }
    return 0;
}

/* concatenate directory / file */
char *join_path(char *buffer, const char *dir, const char *filename)
{
    int l = strlen(dir);
    if (l) {
        memcpy(buffer, dir, l);
        if (!IS_SLASH(buffer[l-1]))
            buffer[l++] = '\\';
    }
    if (filename) {
        while (IS_SLASH(filename[0]))
            ++filename;
        strcpy(buffer + l, filename);
    } else {
        buffer[l] = 0;
        fix_path(buffer);
    }
    return buffer;
}

/* convert to backslashes */
char *replace_slashes(char *buffer, const char *path)
{
    const char *p = path;
    char *b = buffer;
    int c;
    do
        *b++ = '/' == (c = *p++) ? '\\' : c;
    while (c);
    return buffer;
}

/* ------------------------------------------------------------------------- */

static DWORD (WINAPI *pGetLongPathName)(
    LPCTSTR lpszShortPath,
    LPTSTR lpszLongPath,
    DWORD cchBuffer);

char* get_exe_path(HINSTANCE h, char* pszPath, int nMaxLen)
{
    GetModuleFileName(h, pszPath, nMaxLen);
    if (load_imp(&pGetLongPathName, "KERNEL32.DLL", "GetLongPathNameA"))
        pGetLongPathName(pszPath, pszPath, nMaxLen);
    *(char*)file_basename(pszPath) = 0;
    return pszPath;
}

/* ------------------------------------------------------------------------- */
/* Function: get_relative_path */
/* get the sub-path, if the path is in the HINSTANCE folder, */
/* In:       path to check */
/* Out:      pointer to subpath or full path otherwise. */
/* ------------------------------------------------------------------------- */

const char *get_relative_path(HINSTANCE h, const char *path)
{
    char basedir[MAX_PATH];
    int l;
    get_exe_path(h, basedir, sizeof basedir);
    l = strlen(basedir);
    if (l && 0 == memicmp(path, basedir, l))
        return path + l;
    return path;
}

/* ------------------------------------------------------------------------- */
/* Function: set_my_path */
/* Purpose:  add the HINSTANCE path as default */
/* In: */
/* Out: */
/* ------------------------------------------------------------------------- */

char *set_my_path(HINSTANCE h, char *dest, const char *fname)
{
    dest[0] = 0;
    if (0 == is_absolute_path(fname))
        get_exe_path(h, dest, MAX_PATH);
    return strcat(dest, fname);
}

/* ------------------------------------------------------------------------- */
