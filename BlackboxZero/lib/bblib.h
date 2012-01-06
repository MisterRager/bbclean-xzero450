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

#ifndef _BBLIB_H_
#define _BBLIB_H_

#ifdef BBLIB_COMPILING
# ifndef WINVER
#  define WINVER 0x0500
#  define _WIN32_WINNT 0x0500
#  define _WIN32_IE 0x0501
# endif
# define WIN32_LEAN_AND_MEAN
# include <windows.h>
# include <stdlib.h>
# include <stdio.h>
# ifndef BBLIB_STATIC
#  define BBLIB_EXPORT __declspec(dllexport)
# endif
#endif

#ifndef BBLIB_EXPORT
# define BBLIB_EXPORT
#endif

#ifndef __BBCORE__
# define dbg_printf _dbg_printf
#endif

// #define BBOPT_MEMCHECK

/* ------------------------------------------------------------------------- */

/* ------------------------------------------------------------------------- */

/* Convenience defs */
#define IS_SPC(c) ((unsigned char)(c) <= 32)
#define IS_SLASH(c) ((c) == '\\' || (c) == '/')

#ifndef offsetof
# define offsetof(s,m) ((size_t)&(((s*)0)->m))
#endif

#ifndef array_count
# define array_count(s) ((int)(sizeof (s) / sizeof (s)[0]))
#endif

#define c_new(t) (t*)c_alloc(sizeof(t))
#define c_del(v) m_free(v)

/* ------------------------------------------------------------------------- */

/* ------------------------------------------------------------------------- */

#ifdef BBOPT_MEMCHECK
# define m_alloc(s) _m_alloc(s,__FILE__,__LINE__)
# define c_alloc(s) _c_alloc(s,__FILE__,__LINE__)
# define m_realloc(p,s) _m_realloc(p,s,__FILE__,__LINE__)
# define m_free(p) _m_free(p,__FILE__,__LINE__)
#else
# define m_alloc(n) malloc(n)
# define c_alloc(n) calloc(1,n)
# define m_free(v) free(v)
# define m_realloc(p,s) realloc(p,s)
# define m_alloc_check_leaks(title)
# define m_alloc_check_memory()
# define m_alloc_size() 0
#endif /* BBOPT_MEMCHECK */

/* ------------------------------------------------------------------------- */

/* ------------------------------------------------------------------------- */
#ifdef __cplusplus
extern "C" {
#endif

/* numbers.c */

BBLIB_EXPORT int imin(int a, int b);
BBLIB_EXPORT int imax(int a, int b);
BBLIB_EXPORT int iminmax(int a, int b, int c);
BBLIB_EXPORT int iabs(int a);
BBLIB_EXPORT int is_alpha(int c);
BBLIB_EXPORT int is_digit(int c);
BBLIB_EXPORT int is_alnum(int c);

/* colors.c */

BBLIB_EXPORT COLORREF rgb (unsigned r, unsigned g, unsigned b);
BBLIB_EXPORT COLORREF switch_rgb (COLORREF c);
BBLIB_EXPORT COLORREF mixcolors(COLORREF c1, COLORREF c2, int f);
BBLIB_EXPORT COLORREF shadecolor(COLORREF c, int f);
BBLIB_EXPORT unsigned greyvalue(COLORREF c);
BBLIB_EXPORT COLORREF ParseLiteralColor(LPCSTR color);
BBLIB_EXPORT COLORREF ReadColorFromString(const char* string);

/* bools.c */

BBLIB_EXPORT int get_false_true(const char *arg);
BBLIB_EXPORT const char *false_true_string(int f);
BBLIB_EXPORT void set_bool(void *v, const char *arg);

/* strings.c */

BBLIB_EXPORT int replace_string(char *out, int bufsize, int offset, int len, const char *in);
BBLIB_EXPORT char *extract_string(char *dest, const char *src, int n);
BBLIB_EXPORT char *strcpy_max(char *dest, const char *src, int maxlen);
BBLIB_EXPORT char* stristr(const char *aa, const char *bb);
BBLIB_EXPORT int get_string_index (const char *key, const char * const * string_array);
BBLIB_EXPORT unsigned calc_hash(char *p, const char *s, int *pLen, int delim);

BBLIB_EXPORT char *new_str_n(const char *s, int n);
BBLIB_EXPORT char *new_str(const char *s);
BBLIB_EXPORT void free_str(char **s);
BBLIB_EXPORT void replace_str(char **s, const char *n);
BBLIB_EXPORT char *concat_str(const char *s1, const char *s2);

BBLIB_EXPORT char *m_formatv(const char *fmt, va_list arg_list);
BBLIB_EXPORT char *m_format(const char *fmt, ...);

/* tokenize.c */

BBLIB_EXPORT int nexttoken(const char **p_out, const char **p_in, const char *delims);
BBLIB_EXPORT char* NextToken(char* buf, const char** string, const char *delims);
BBLIB_EXPORT int get_string_within (char *dest, int size, const char **p_src, const char *delims);
BBLIB_EXPORT const char *get_special_command(const char **p_path, char *buffer, int size);
BBLIB_EXPORT int skip_spc(const char **pp);

/* paths.c */

BBLIB_EXPORT char* unquote(char *src);
BBLIB_EXPORT char *quote_path(char *path);
BBLIB_EXPORT const char *file_basename(const char *path);
BBLIB_EXPORT const char *file_extension(const char *path);
BBLIB_EXPORT char *file_directory(char *buffer, const char *path);
BBLIB_EXPORT char *fix_path(char *path);
BBLIB_EXPORT int is_absolute_path(const char *path);
BBLIB_EXPORT char *join_path(char *buffer, const char *dir, const char *filename);
BBLIB_EXPORT char *replace_slashes(char *buffer, const char *path);

BBLIB_EXPORT void bbshell_set_utf8(int f);
BBLIB_EXPORT void bbshell_set_defaultrc_path(const char *s);

/* winutils.c */

BBLIB_EXPORT void BitBltRect(HDC hdc_to, HDC hdc_from, RECT *r);
BBLIB_EXPORT HWND GetRootWindow(HWND hwnd);
BBLIB_EXPORT int is_bbwindow(HWND hwnd);
BBLIB_EXPORT int get_fontheight(HFONT hFont);
BBLIB_EXPORT int get_filetime(const char *fn, FILETIME *ft);
BBLIB_EXPORT int diff_filetime(const char *fn, FILETIME *ft0);
BBLIB_EXPORT unsigned long getfileversion(const char *path);
BBLIB_EXPORT const char *replace_environment_strings_alloc(char **out, const char *src);
BBLIB_EXPORT char* replace_environment_strings(char* src, int max_size);
BBLIB_EXPORT void dbg_printf (const char *fmt, ...);
BBLIB_EXPORT void dbg_window(HWND hwnd, const char *fmt, ...);
BBLIB_EXPORT char* win_error(char *msg, int msgsize);
BBLIB_EXPORT void ForceForegroundWindow(HWND theWin);
BBLIB_EXPORT void SetOnTop (HWND hwnd);
BBLIB_EXPORT int is_frozen(HWND hwnd);
BBLIB_EXPORT HWND window_under_mouse(void);
BBLIB_EXPORT int load_imp(void *pp, const char *dll, const char *proc);
BBLIB_EXPORT int _load_imp(void *pp, const char *dll, const char *proc);
#define have_imp(pp) ((DWORD_PTR)pp > 1)

BBLIB_EXPORT char* get_exe_path(HINSTANCE h, char* pszPath, int nMaxLen);
BBLIB_EXPORT char *set_my_path(HINSTANCE h, char *dest, const char *fname);
BBLIB_EXPORT const char *get_relative_path(HINSTANCE h, const char *path);

BBLIB_EXPORT int BBWait(int delay, unsigned nObj, HANDLE *pObj);
BBLIB_EXPORT void BBSleep(unsigned millisec);
BBLIB_EXPORT int run_process(const char *cmd, const char *dir, int flags);
#define RUN_SHOWERRORS  0
#define RUN_NOERRORS    1
#define RUN_WAIT        2
#define RUN_HIDDEN      4
#define RUN_NOARGS      8
#define RUN_NOSUBST    16
#define RUN_ISPIDL     32
#define RUN_WINDIR     64

/* tinylist.c */

#ifndef LIST_NODE_DEFINED
typedef struct list_node { struct list_node *next; void *v; } list_node;
#endif
#ifndef STRING_NODE_DEFINED
typedef struct string_node { struct string_node *next; char str[1]; } string_node;
#endif

#define dolist(_e,_l) for (_e=(_l);_e;_e=_e->next)

BBLIB_EXPORT void *member(void *a0, void *e0);
BBLIB_EXPORT void *member_ptr(void *a0, void *e0);
BBLIB_EXPORT void *assoc(void *a0, void *e0);
BBLIB_EXPORT void *assoc_ptr(void *a0, void *e0);
BBLIB_EXPORT int remove_assoc(void *a, void *e);
BBLIB_EXPORT int remove_node (void *a, void *e);
BBLIB_EXPORT int remove_item(void *a, void *e);
BBLIB_EXPORT void reverse_list (void *d);
BBLIB_EXPORT void append_node (void *a0, void *e0);
BBLIB_EXPORT void cons_node (void *a0, void *e0);
BBLIB_EXPORT void *copy_list (void *l0);
BBLIB_EXPORT void *nth_node (void *v0, int n);
BBLIB_EXPORT void *new_node(void *p);
BBLIB_EXPORT int listlen(void *v0);
BBLIB_EXPORT void freeall(void *p);

BBLIB_EXPORT struct string_node *new_string_node(const char *s);
BBLIB_EXPORT void append_string_node(struct string_node **p, const char *s);

/* m_alloc.c */

#ifdef BBOPT_MEMCHECK
BBLIB_EXPORT void * _m_alloc(unsigned n, const char *file, int line);
BBLIB_EXPORT void * _c_alloc (unsigned n, const char *file, int line);
BBLIB_EXPORT void   _m_free(void *v, const char *file, int line);
BBLIB_EXPORT void * _m_realloc (void *v, unsigned s, const char *file, int line);
BBLIB_EXPORT void   _m_setinfo(const char *file, int line);

BBLIB_EXPORT unsigned m_alloc_usable_size(void *v);
BBLIB_EXPORT void m_alloc_dump_memory(void);
BBLIB_EXPORT void m_alloc_check_memory(void);
BBLIB_EXPORT void m_alloc_check_leaks(const char *title);
BBLIB_EXPORT unsigned m_alloc_size(void);
#endif /* BBOPT_MEMCHECK */

/* ------------------------------------------------------------------------- */

/* ------------------------------------------------------------------------- */
#ifdef __cplusplus
}
#endif

/* ------------------------------------------------------------------------- */

/* ------------------------------------------------------------------------- */
#if defined __cplusplus && defined BBOPT_MEMCHECK
#undef new
#undef delete
inline void *operator new (size_t n)
{
    return _m_alloc(n, NULL, -1);
}

inline void operator delete (void *v)
{
     _m_free(v, NULL, -1);
}

inline void * operator new[] (size_t n)
{
    return _m_alloc(n, NULL, -1);
}

inline void operator delete[] (void *v)
{
     _m_free(v, NULL, -1);
}
#define new (_m_setinfo(__FILE__,__LINE__),false)?NULL:new
#define delete _m_setinfo(__FILE__,__LINE__),delete
#endif /* BBOPT_MEMCHECK */
/* ------------------------------------------------------------------------- */

/* ------------------------------------------------------------------------- */
#endif /* ndef _BBLIB_H_ */
