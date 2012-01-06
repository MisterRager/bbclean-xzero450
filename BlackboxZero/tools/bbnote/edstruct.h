/*---------------------------------------------------------------------------*

  This file is part of the BBNote source code

  Copyright 2003-2009 grischka@users.sourceforge.net

  BBNote is free software, released under the GNU General Public License
  (GPL version 2). For details see:

  http://www.fsf.org/licenses/gpl.html

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
  for more details.

 *---------------------------------------------------------------------------*/
// EDSTRUCT.H - main include file for the editor engine

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "bblib.h"
#include "eddef.h"

#define ST static

#ifdef _MSC_VER
#pragma warning(disable: 4244) // convert int to short
#endif

/*----------------------------------------------------------------------------*/

struct edvars {
    struct edvars *next;

    struct bufpage *sprev_pg;
    struct bufpage *snext_pg;

    int  sbuf_a,sbuf_e;

    int sflen;
    int sfpga, slpos, sfpos;
    int splin, stlin;

    int scurx,scury,sclft,slmax;

    struct undo_s *sundo_l,*sredo_l;

    int  *sma,      *sme;
    int  *smxa,     *smxe;
    int  smark1,    smark2;
    int  smark1x,   smark2x;
    char smarkf1,   smarkf2;

    //char svmark;

    char schg;
    char supd;
    char sflg;

    FILETIME sfiletime;
    char sfnameflg;
    char sfilename[128];

    char sbuffer[1];
};

#define changed()  (chg!=0)


extern struct edvars *ed0,*edp,*new_buffer(void);

#define buffer  (edp->sbuffer)
#define next_pg (edp->snext_pg)
#define prev_pg (edp->sprev_pg)
#define buf_a   (edp->sbuf_a)
#define buf_e   (edp->sbuf_e)

#define fpga    (edp->sfpga)
#define lpos    (edp->slpos)
#define fpos    (edp->sfpos)
#define flen    (edp->sflen)

#define plin    (edp->splin)
#define tlin    (edp->stlin)
#define curx    (edp->scurx)
#define cury    (edp->scury)
#define clft    (edp->sclft)
#define lmax    (edp->slmax)
#define undo_l  (edp->sundo_l)
#define redo_l  (edp->sredo_l)
#define usave   (edp->susave)

#define ma      (edp->sma)
#define me      (edp->sme)
#define mxa     (edp->smxa)
#define mxe     (edp->smxe)
#define mark1   (edp->smark1)
#define mark2   (edp->smark2)
#define mark1x  (edp->smark1x)
#define mark2x  (edp->smark2x)
#define markf1  (edp->smarkf1)
#define markf2  (edp->smarkf2)
//#define vmark   (edp->svmark)

#define chg      (edp->schg)
#define upd      (edp->supd)
#define fileflg  (edp->sflg)

#define filetime (edp->sfiletime)
#define fnameflg (edp->sfnameflg)
#define filename (edp->sfilename)

void clear_buffer(void);
void insdelmem(int at, int len);
void copyfrom(char*,int,int);
void copyto(int,const char*,int);
void clearchr(int,char,int);
unsigned char getchr(int o);

void u_reset(void);
void u_setchg(int);

void revlist  (void *d);
void appendlist (void *a,void *e);
void conslist (void *a,void *e);
void dellist  (void *a,void *e);
void delitem  (void *a,void *e);
void inslist  (void *a,void *e, void* i);

void nextfile(void);
void prevfile(void);
void insfile(struct edvars*);

void NewFile(void);
void CloseFile(void);
int LoadFile(char *);
int DoFileOpenSave(HWND hwnd, int mode);
int QueryDiscard(HWND, int f);
int QueryDiscard_1(HWND, int f);
int loadfile(void);
//int savefile(char *);

void InfoMsg(const char*);
void make_dialog(const char*);
extern char make_f,fsearch;
extern char owscroll;
void kill_prog(void);

int  send_to_prog(char *);
void send_brk_to_prog(void);
void search_dialog(HWND);
void run_dialog(void);
void debug_dialog(void);
void project_dialog(void);
void conf_dialog(void);
int  init_ownd(char*);
void ow_move(void);
int  line_dialog(HWND);
int run_cmd(char *cmd, char *dir, int f, int d);
int oyncan_msgbox(const char *t, const char *s, int f);
void spawn_exit(void);
extern char *spawncmd;
//extern HWND mwnd;

int fixdir(char *p);
void setide(char*);
void settitle(void);
void setwtext(void);
void sethistory(void);
int  getftime (void);
void checkftime(HWND);
void f_reload(int);

char *fname(char*);
char *checkhome(char *p);
char *setpath(char *dst, char *src);
extern char linebuf[];
extern char tmpbuff[];

int  nextline_v(int o, int n, int *v);
int  prevline_v(int o, int n, int *v);
int  nextline(int o, int n);
int  prevline(int o, int n);
int  linelen(int o);
void inschr(int,int, const char*);
void delchr(int,int);
int getkword(int o,char *p);
int  inslf(int);
//void setchr(int o,char c);

int  cntlf(int,int);
int  fixline(int);
char upc(char);
char lwc(char);
int  setmark(void);

struct mark_s {
    int a,e,l,y;
    int xa,xe,x;
    char lf;
};
int getmark(struct mark_s *m);

void ed_cmd(int cmd,...);

struct sea { int from; char *str; int sf; int a; int e; };
int  ed_search(struct sea *);

int  movpage(int);
void domarking(int);
void unmark(void);
int  entab(char*,char*,int,int);
int  detab(char*,char*,int,int);

#define EK_INIT     1000
#define EK_SETVAR   1001
#define EK_SIZE     1002
#define EK_GOTO     1003
#define EK_GOTOLINE 1004
#define EK_REPLACE  1005
#define EK_INSERT   1006
#define EK_INSBLK   1007
#define EK_CHAR     1008
#define EK_MARK     1009
#define EK_RETAB    1010
#define EK_DRAG_COPY   1011
#define EK_DRAG_MOVE   1012

#define TABC 0x09

int printpage(HDC hdc, int x, int y1, int y2);
void set_lang(const char*);
void printcfg(void);
void readcfg_1(void);
void readcfg_2(int);

extern char prjflg;
extern char ide_flg;

extern char cfgfilename[];
extern char prjfilename[];
extern char winhelppath[];
extern char moduledir[];
extern char currentdir[];
extern char projectdir[];

char *makepath(char *d, char *p, const char *s);


struct winvars {
    int _wx0,_wy0;
    int _wxl,_wyl;
    int _winzoomed;

    int _cwx,_cwy;
    int _pgx,_pgy;
    int _zx0;
    int _zy0;
    int _zx;
    int _zy;
    char _langflg;
    HFONT _hfnt_n;
    HFONT _hfnt_b;
    int _sbx,_sby;
    char _winflg;
    char _winid;
};

extern struct winvars editw,ow1,*winp;

#define cwx (winp->_cwx)
#define cwy (winp->_cwy)
#define pgx (winp->_pgx)
#define pgy (winp->_pgy)
#define wx0 (winp->_wx0)
#define wy0 (winp->_wy0)
#define wxl (winp->_wxl)
#define wyl (winp->_wyl)

#define hfnt_n (winp->_hfnt_n)
#define hfnt_b (winp->_hfnt_b)
#define zx0    (winp->_zx0  )
#define zy0    (winp->_zy0  )
#define zx     (winp->_zx   )
#define zy     (winp->_zy   )
#define sbx    (winp->_sbx   )
#define sby    (winp->_sby   )
#define langflg (winp->_langflg)
#define winflg (winp->_winflg)
#define winid  (winp->_winid)
#define winzoomed (winp->_winzoomed)

#define COLORN 17

extern DWORD My_Colors  [COLORN];
extern char  My_Weights [COLORN];

#define Cbgd  0
#define Ctxt  1
#define CIbgd 2
#define CItxt 3
#define Ccmt  4
#define Ckey  5
#define Copr  6
#define Cstr  7
#define Cnum  8
#define Cmac  9
#define CbgdS 10
#define CtxtS 11
#define CbgdL 12
#define CtxtL 13
#define CIbgdL 14
#define CItxtL 15
#define CIerrL 16
extern char colortrans[3][11];


extern int My_FontSize_n;
extern int My_FontSize_s;
extern int My_FontWeight_n;
extern int My_FontWeight_b;
extern char  My_Font[64];
extern char  My_Font_s[64];
extern int My_CaretSize;

int rcomp (unsigned char *in, unsigned char *out, int omax, int cf);
struct rmres { int p; int w; };
int rmatch(int s, int a, int e, unsigned char *m, struct rmres *m_ptr, int (*get)(int));

#define OW_CLEAR 1001
#define OW_PRINT 1002
#define OW_INSERT 1003

extern HWND ownd;
extern HWND hwnd ;
extern HWND hwndTV0 ;
extern HINSTANCE hInst;

void vscroll(DWORD wP);
void hscroll(DWORD wP);
void setsb (HWND hwnd);

extern struct proj {
    char name [128];
    char home [128];
    char make [128];
    char dir  [128];
    char env  [128];
    char run  [128];
    char rdir [128];
    char dbug [128];
    char help [128];
} proj;


void init_drop(HWND);
void exit_drop(void);
int  do_drag(char *);
void do_drop(char *, int);

void createcaret(HWND hwnd);
void destroycaret(void);
int  showcaret(int,int);
void set_update(HWND);

extern HWND seaDlg;

extern struct efile {
    struct efile *next;
    int i[4];
    char name[256];

} *efp;

void load_all_files(void);

extern int
    clip_n, clip_s
    ;

extern char
    smart, tuse, backup, bakdir, savedly, unix_eol,
    winhelp_1[], winhelp_2[], winhelp_3[],
    grep_cmd[], seabuf[], rplbuf[],
    seamodeflg, moumrk, linmrk, ltup, vmark,
    hsbar,
    wrdsep[]
    ;

extern int tabs;

#ifndef STRL
struct strl { struct strl *next; char str[1]; };
#define STRL
#endif

extern struct strl
    *artool,
    *errstr,
    *seabox,
    *rplbox,
    *lastproj
    ;

struct strl *newstr(const char *s);
void freelist(void *p);
void appendstr(struct strl **p, const char *s);
void clean_up(void);
void openproject(char *);
void manage_prj(void);

char* projstrpath(struct strl *p);

void setsize(HWND);

void centerdlg(HWND);

void savetree(void);
int  inittree(void);

int movedlg (HWND bwnd, UINT message, WPARAM wParam, LPARAM lParam);
