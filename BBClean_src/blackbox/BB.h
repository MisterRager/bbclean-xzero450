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

/* BB.h - global defines aside of what is needed for the sdk-api */

#ifndef _BB_H
#define _BB_H

// ==============================================================
/* optional defines */

/* experimental nationalized language support */
//#define BBOPT_SUPPORT_NLS

/* misc. developement options */
//#define BBOPT_DEVEL

/* memory allocation tracking */
//#define BBOPT_MEMCHECK

/* core dump to file */
//#define BBOPT_STACKDUMP

//#define NDEBUG

// ==============================================================
/* compiler specifics */

#define WINVER 0x0500
#define _WIN32_WINNT 0x0500
#define _WIN32_IE 0x0500
#define NO_INTSHCUT_GUIDS
#define NO_SHDOCVW_GUIDS

#ifdef __BORLANDC__
  #ifdef CRTSTRING
	#include "crt-string.h"
	#define TRY if (1)
	#define EXCEPT if(0)
  #else
	#define TRY __try
	#define EXCEPT __except(1)
  #endif
#endif

#ifdef __GNUC__
  #define TRY if (1)
  #define EXCEPT if(0)
#endif

#ifdef _MSC_VER
  #ifdef BBOPT_STACKDUMP
	#define TRY if (1)
	#define EXCEPT if(0)
  #else
	#define TRY _try
	#define EXCEPT _except(1)
  #endif
  #define stricmp _stricmp
  #define strnicmp _strnicmp
  #define memicmp _memicmp
  #define strlwr _strlwr
  #define strupr _strupr
#else
  #undef BBOPT_STACKDUMP
#endif

// ==============================================================

// ==============================================================
/* Always includes */

#define __BBCORE__ // enable exports in BBApi.h
#include "BBApi.h"
#include "win0x500.h"
#include "m_alloc.h"
#include "Tinylist.h"
#include <assert.h>

// ==============================================================
/* Blackbox icon and menu drag-cursor */

#define IDI_BLACKBOX 101
#define IDC_MOVEMENU 102

/* Convenience defs */
#define IS_SPC(c) ((unsigned char)(c) <= 32)
#define IS_SLASH(c) ((c) == '\\' || (c) == '/')

#if defined(__GNUC__) && (__GNUC__ == 3)
  #define imax(a,b) ((a) >? (b))
  #define imin(a,b) ((a) <? (b))
#else
  #define imax(a,b) ((a) > (b) ? (a) : (b))
  #define imin(a,b) ((a) < (b) ? (a) : (b))
#endif
#define iminmax(a,b,c) (imin(imax((a),(b)),(c)))
#define _CopyRect(lprcDst,lprcSrc) (*lprcDst) = (*lprcSrc)
#define _InflateRect(lprc,dx,dy) (*(lprc)).left -= (dx), (*(lprc)).right += (dx), (*(lprc)).top -= (dy), (*(lprc)).bottom += (dy)
#define _OffsetRect(lprc,dx,dy) (*(lprc)).left += (dx), (*(lprc)).right += (dx), (*(lprc)).top += (dy), (*(lprc)).bottom += (dy)
#define _SetRect(lprc,xLeft,yTop,xRight,yBottom) (*(lprc)).left = (xLeft), (*(lprc)).right = (xRight), (*(lprc)).top = (yTop), (*(lprc)).bottom = (yBottom)
#define _CopyOffsetRect(lprcDst,lprcSrc,dx,dy) (*(lprcDst)).left = (*(lprcSrc)).left + (dx), (*(lprcDst)).right = (*(lprcSrc)).right + (dx), (*(lprcDst)).top = (*(lprcSrc)).top + (dy), (*(lprcDst)).bottom = (*(lprcSrc)).bottom + (dy)
#define GetRectWidth(lprc) ((*(lprc)).right - (*(lprc)).left)
#define GetRectHeight(lprc) ((*(lprc)).bottom - (*(lprc)).top)
// ==============================================================
/* global variables */

extern OSVERSIONINFO osInfo;
extern bool         usingNT;
extern bool         usingWin2kXP;

extern HINSTANCE    hMainInstance;
extern HWND         BBhwnd;
extern int          VScreenX, VScreenY;
extern int          VScreenWidth, VScreenHeight;
extern int          ScreenWidth, ScreenHeight;
extern bool         underExplorer;
extern bool         multimon;
extern bool         dont_hide_explorer;
extern bool         dont_hide_tray;

extern const char   bb_exename[];

extern const char   ShellTrayClass[];
extern bool         bbactive;

extern void (WINAPI* pSwitchToThisWindow)(HWND, int);
extern BOOL (WINAPI* pTrackMouseEvent)(LPTRACKMOUSEEVENT lpEventTrack);

// ==============================================================
/* Blackbox window timers */
#define BB_CHECKWINDOWS_TIMER   2 /* refresh the VWM window list */
#define BB_WRITERC_TIMER        3
#define BB_RUNSTARTUP_TIMER     4

/* SetDesktopMargin internal flags */
#define BB_DM_REFRESH -1
#define BB_DM_RESET -2

// ==============================================================
/* utils.cpp */

int BBMessageBox(int flg, const char *fmt, ...);
BOOL BBRegisterClass (WNDCLASS *wcp);

char *strcpy_max(char *dest, const char *src, int maxlen);
const char* stristr(const char *a, const char *b);
int get_string_index (const char *key, const char **string_list);
int get_substring_index (const char *key, const char **string_list);
int substr_icmp(const char *a, const char *b);
int get_false_true(const char *arg);
void set_bool(bool *v, const char *arg);
const char *false_true_string(bool f);
const char *string_empty_or_null(const char *s);
char *extract_string(char *dest, const char *src, int length);
int bb_sscanf(const char **src, const char *fmt, ...);

// int imax(int a, int b);
// int imin(int a, int b);
// int iminmax(int i, int minval, int maxval);
bool is_alpha(int c);
bool is_num(int c);
bool is_alnum(int c);

char *replace_argument1(char *out, const char *format, const char *arg);

/* color utilities */
COLORREF rgb (unsigned r,unsigned g,unsigned b);
COLORREF switch_rgb (COLORREF c);
COLORREF mixcolors(COLORREF c1, COLORREF c2, int mixfac);
COLORREF shadecolor(COLORREF c, int f);
unsigned greyvalue(COLORREF c);
void draw_line_h(HDC hDC, int x1, int x2, int y, int w, COLORREF C);

/* filenames */
const char *get_ext(const char *e);
const char *get_file(const char *path);
char *get_directory(char *buffer, const char *path);
char* unquote(char *d, const char *s);
char *add_slash(char *d, const char *s);
const char *get_relative_path(const char *p);
char *replace_slashes(char *buffer, const char *path);
bool is_relative_path(const char *path);
char *make_bb_path(char *dest, const char *src);

/* folder changed notification register */
UINT add_change_notify_entry(HWND hwnd, const struct _ITEMIDLIST *pidl);
void remove_change_notify_entry(UINT id_notify);

/* filetime */
int get_filetime(const char *fn, FILETIME *ft);
int check_filetime(const char *fn, FILETIME *ft0);

/* drawing */
void arrow_bullet (HDC buf, int x, int y, int d);
void BitBltRect(HDC hdc_to, HDC hdc_from, RECT *r);
int get_fontheight(HFONT hFont);

/* other */
int GetAppByWindow(HWND Window, LPSTR processName);
int EditBox(const char *caption, const char *message, const char *initvalue, char *buffer);
HWND GetRootWindow(HWND hwnd);
void SetOnTop (HWND hwnd);

/* Logging */
extern "C" void log_printf(int flag, const char *fmt, ...);
void reset_logging(void);

void init_log(void);
void exit_log(void);
void show_log(bool fShow);
void write_log(const char *szString);

// ==============================================================
/* stackdump.cpp - stack trace */
DWORD except_filter( EXCEPTION_POINTERS *ep );

// ==============================================================
/* pidl.cpp - shellfolders */
bool sh_getfolderpath(LPSTR szPath, UINT csidl);
char* replace_shellfolders(char *buffer, const char *path, bool search_path);

// ==============================================================
/* drag and drop - dragsource.cpp / droptarget.cpp */
void init_drop(HWND hwnd);
void exit_drop(HWND hwnd);
void drag_pidl(const struct _ITEMIDLIST *pidl);
class CDropTarget *init_drop_targ(HWND hwnd, const struct _ITEMIDLIST *pidl);
void exit_drop_targ(class CDropTarget  *);
bool in_drop(class CDropTarget *dt);

// ==============================================================
/* shell context menus */
Menu *GetContextMenu(const struct _ITEMIDLIST *pidl);

// ==============================================================
/* workspaces and tasks */
bool focus_top_window(void);
void ForceForegroundWindow(HWND theWin);
struct hwnd_list { struct hwnd_list *next; HWND hwnd; };

// ==============================================================
/* BBApi.cpp - some (non api) utils */

unsigned long getfileversion(const char *path, char *buffer);

BOOL BBExecute_command(const char *command, const char *arguments, bool no_errors);
BOOL BBExecute_string(const char *s, bool no_errors);
bool ShellCommand(const char *cmd, const char *work_dir, bool wait);

/* tokenizer */
LPSTR NextToken(LPSTR buf, LPCSTR *string, const char *delims = NULL);

/* rc-reader */
void reset_reader(void);
void write_rcfiles(void);
FILE *create_rcfile(const char *path);
char *read_file_into_buffer(const char *path, int max_len);
char scan_line(char **pp, char **ss, int *ll);
int match(const char *str, const char *pat);
bool read_next_line(FILE *fp, LPSTR szBuffer, DWORD dwLength);
bool is_stylefile(const char *path);
bool is_newstyle(LPCSTR path);

/* color parsing */
COLORREF ReadColorFromString(LPCSTR string);

/* window */
void ClearSticky();
void dbg_window(HWND window, const char *fmt, ...);

/* generic hash function */
unsigned calc_hash(char *p, const char *s, int *pLen);

// ==============================================================
/* Blackbox.cpp */

void post_command(const char *cmd);
int exec_pidl(const _ITEMIDLIST *pidl, LPCSTR verb, LPCSTR arguments);
int get_workspace_number(const char *s);
void set_opaquemove(void);

/* Menu */
bool IsMenu(HWND hwnd);

// ==============================================================
/* Experimental Nationalized Language support */

#ifdef BBOPT_SUPPORT_NLS
const char *nls1(const char *p);
const char *nls2a(const char *i, const char *p);
const char *nls2b(const char *p);
void free_nls(void);
#define NLS0(S) S
#define NLS1(S) nls1(S)
#define NLS2(I,S) nls2b(I S)
#else
#define free_nls()
#define NLS0(S) S
#define NLS1(S) (S)
#define NLS2(I,S) (S)
#endif

// ==============================================================
#endif
