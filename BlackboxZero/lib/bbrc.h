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
/* read7write rc files with cache */

#ifndef _BBRC_H_
#define _BBRC_H_

#include "bblib.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RCFILE_HTS 40 // hash table size

struct lin_list
{
    struct lin_list *next;
    struct lin_list *hnext;
    struct lin_list *wnext;
    unsigned hash, k, o;
    int i;
    char is_wild;
    char dirty;
    char flags;
    char str[3];
};

struct fil_list
{
    struct fil_list *next;
    struct lin_list *lines;
    struct lin_list *wild;
    struct lin_list *ht[RCFILE_HTS];
    unsigned hash;

    char dirty;
    char newfile;
    char tabify;
    char write_error;
    char is_style;
    char is_070;

    int k;
    char path[1];
};

struct rcreader_init
{
    struct fil_list *rc_files;
    void (*write_error)(const char *filename);
    char dos_eol;
    char translate_065;
    char found_last_value;
    char used, timer_set;
};

BBLIB_EXPORT void init_rcreader(struct rcreader_init *init);
BBLIB_EXPORT void reset_rcreader(void);

BBLIB_EXPORT int set_translate_065(int f);
BBLIB_EXPORT int get_070(const char* path);
BBLIB_EXPORT void check_070(struct fil_list *fl);
BBLIB_EXPORT int is_stylefile(const char *path);

BBLIB_EXPORT FILE *create_rcfile(const char *path);
BBLIB_EXPORT char *read_file_into_buffer(const char *path, int max_len);
BBLIB_EXPORT char scan_line(char **pp, char **ss, int *ll);
BBLIB_EXPORT int read_next_line(FILE *fp, char* szBuffer, unsigned dwLength);

BBLIB_EXPORT const char* read_value(const char* path, const char* szKey, long *ptr);
BBLIB_EXPORT int found_last_value(void);
BBLIB_EXPORT void write_value(const char* path, const char* szKey, const char* value);
BBLIB_EXPORT int rename_setting(const char* path, const char* szKey, const char* new_keyword);
BBLIB_EXPORT int delete_setting(LPCSTR path, LPCSTR szKey);

/* ------------------------------------------------------------------------- */
/* parse a StyleItem */

struct StyleItem;

BBLIB_EXPORT void parse_item(LPCSTR szItem, struct StyleItem *item);
BBLIB_EXPORT int findtex(const char *p, int prop);
BBLIB_EXPORT struct styleprop{ const char *key; int val; };
BBLIB_EXPORT const struct styleprop *get_styleprop(int prop);

/* ------------------------------------------------------------------------- */
/* only used in bbstylemaker */

BBLIB_EXPORT int scan_component(const char **p);
BBLIB_EXPORT int xrm_match (const char *key, const char *pat);

BBLIB_EXPORT struct fil_list *read_file(const char *filename);
BBLIB_EXPORT struct lin_list *make_line(struct fil_list *fl, const char *key, const char *val);
BBLIB_EXPORT void free_line(struct fil_list *fl, struct lin_list *tl);
BBLIB_EXPORT struct lin_list **get_simkey(struct lin_list **slp, const char *key);
BBLIB_EXPORT void make_style070(struct fil_list *fl);
BBLIB_EXPORT void make_style065(struct fil_list *fl);

/* ------------------------------------------------------------------------- */
#ifdef __cplusplus
}
#endif

#endif
