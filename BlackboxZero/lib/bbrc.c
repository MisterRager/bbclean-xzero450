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

/*
   tiny cache reader: first checks, if the file had already been read, if not,
   reads the file into a malloc'ed buffer, then for each line separates
   keyword from value, cuts off leading and trailing spaces, strlwr's the
   keyword and adds both to a list of below defined structures, where k is
   the offset to the start of the value-string. Comments or other non-keyword
   lines have a "" keyword and the line as value.

   added checking for external updates by the user.

   added *wild*card* processing, it first looks for an exact match, if it
   cant find any, returns the first wildcard value, that matches, or null,
   if none...
*/

#include "BBApi.h"
#include "win0x500.h"
#include "bbrc.h"

#define MAX_KEYWORD_LENGTH 200
#define RCFILE_HTS 40 // hash table size
// #define DEBUG_READER

#define ST static
#define true 1
#define false 0

ST struct rcreader_init *g_rc;

void init_rcreader(struct rcreader_init *init)
{
    g_rc = init;
}

int found_last_value(void)
{
    return g_rc->found_last_value;
}

int set_translate_065(int f)
{
    int r = g_rc->translate_065;
    g_rc->translate_065 = f;
    return r;
}

ST struct lin_list *search_line(
    struct fil_list *fl, const char *key, int fwild, LONG *p_seekpos);

/* ------------------------------------------------------------------------- */

ST int translate_key070(char *key)
{
    static const char * const checklist [] = {
        "^toolbar"       ,
        "^slit"          ,
        // toolbar.*:
        "windowlabel"   ,
        "clock"         ,
        "label"         ,
        "button"        ,
        "pressed"       ,
        // menu.*:
        "frame"         ,
        "title"         ,
        "active"        ,
        // window.*:
        "focus"         ,
        "unfocus"       ,
        NULL,
        // from         -->   to
        "color"         , "color1"              ,
        "colorto"       , "color2"              ,
        "piccolor"      , "foregroundColor"     ,
        "bulletcolor"   , "foregroundColor"     ,
        "disablecolor"  , "disabledColor"       ,
        "justify"       , "alignment"           ,

        "^handlewidth"  , "window.handleHeight" ,
        "^borderwidth"  , "toolbar.borderWidth" ,
        "^bordercolor"  , "toolbar.borderColor" ,
        "^bevelwidth"   , "toolbar.marginWidth" ,
        "^frameWidth"   , "window.frame.borderWidth",
        "focusColor"    , "focus.borderColor"     ,
        "unfocusColor"  , "unfocus.borderColor"   ,
    /*
        "titleJustify"  , "menu.title.alignment"   ,
        "menuJustify"   , "menu.frame.alignment"   ,
    */
        NULL
    };

    const char * const * pp, *p;
    char *d;
    int l, n, x, k, f, r = 0;

    l = strlen(key);
    if (0 == l)
        return 0;

    if (key[l-1] == ':')
        key[--l] = 0;

    strlwr(key);
    if (NULL != (d = strstr(key, "hilite")))
        memcpy(d, "active", 6), r = 1;

    for (pp = checklist, x = 0; x++ < 2; ++pp)
    {
        for (n = 0; 0 != (p = *pp); ++n, pp += x) {
            f = p[0] == '^';
            k = strlen(p += f);
            d = key+l-k;
            if (!(f ? d == key : d > key && d[-1] == '.'))
                continue;
            if (0 != memcmp(d, p, k))
                continue;
            if (x == 1)
                strcpy(key+l, ".appearance");
            else
                strcpy(d, pp[1]);
            return 1 + (x==2 && n==0);
        }
    }
    return r;
}

// This one converts all keys in a style from 065 to 070 style conventions
void make_style070(struct fil_list *fl)
{
    struct lin_list *tl, **tlp, *sl, *ol;
    char buffer[MAX_KEYWORD_LENGTH], *p;
    int f;
    for (tlp = &fl->lines; NULL != (tl = *tlp); tlp = &tl->next)
    {
        if (0 == tl->str[0])
            continue;
        memcpy(buffer, tl->str+tl->o, tl->k);
        f = translate_key070(buffer);
        if (f) {
            for (ol = tl;;) {
                sl = make_line(fl, buffer, tl->str+tl->k);
                //dbg_printf("%s -> %s", tl->str, sl->str);
                sl->next = tl->next;
                tl->next = sl;
                tl = sl;
                if (0 == --f)
                    break;
                // since I dont know (and dont want to check here)
                // whether its solid or gradient, I just translate
                // 'color' to both 'color1' and 'backgroundColor'
                p = strchr(buffer, 0) - (sizeof "color1" - 1);
                strcpy(p, "backgroundColor");
            }
            *tlp = ol->next;
            free_line(fl, ol);
        }
    }
}

/* ------------------------------------------------------------------------- */
ST bool translate_key065(char *key)
{
    static const char *pairs [] =
    {
        // from         -->   to
        ".appearance"       , ""                ,
        "alignment"         , "justify"         ,
        "color1"            , "color"           ,
        "color2"            , "colorTo"         ,
        "backgroundColor"   , "color"           ,
        "foregroundColor"   , "picColor"        ,
        "disabledColor"     , "disableColor"    ,
        "menu.active"       , "menu.hilite"     ,
        "window.handleHeight","handleWidth"   ,
        NULL
    };
    const char **p = pairs;
    bool ret = false;
    int k = 0;
    do
    {
        char *q = (char*)stristr(key, *p);
        if (q)
        {
            int lp = strlen(p[0]);
            int lq = strlen(q);
            int lr = strlen(p[1]);
            int k0 = k + lr - lp;
            memmove(q + lr, q + lp, lq - lp + 1);
            memmove(q, p[1], lr);
            k = k0;
            ret = true;
        }
    } while ((p += 2)[0]);
    return ret;
}

// This one converts all keys in a style from 070 to 065 style conventions
void make_style065(struct fil_list *fl)
{
    struct lin_list *tl, **tlp, *ol;
    char buffer[1000]; int f;
    for (tlp = &fl->lines; NULL != (tl = *tlp); tlp = &tl->next)
    {
        if (0 == tl->str[0])
            continue;
        memcpy(buffer, tl->str+tl->o, tl->k);
        f = translate_key065(buffer);
        if (f) {
            ol = tl;
            *tlp = tl = make_line(fl, buffer, tl->str+tl->k);
            tl->next = ol->next;
            free_line(fl, ol);
        }
    }
}

/* ------------------------------------------------------------------------- */
// this one tries to satisfy a plugin that queries an 0.65 item
// from a style that has 0.70 syntax

ST struct lin_list *search_line_065(struct fil_list *fl, const char *key)
{
    char buff[MAX_KEYWORD_LENGTH];
    int f;
    struct lin_list *tl;
    char *d;

    strcpy_max(buff, key, sizeof buff);
    f = translate_key070(buff);
    if (0 == f)
        return NULL;
    if (2 == f) {
        // we need to translate ".color" to "color1" for gradients
        // but to "backgroundColor" for solid (unless interlaced)
        d = strchr(buff, 0) - (sizeof ".color1" - 1);
        strcpy(d, ".appearance");
        tl = search_line(fl, buff, true, NULL);
        if (tl && (stristr(tl->str+tl->k, "gradient")
                || stristr(tl->str+tl->k, "interlaced")))
            strcpy(d, ".color1");
        else
            strcpy(d, ".backgroundColor");
    }
    // dbg_printf("%s -> %s", key, buff);
    return search_line(fl, buff, true, NULL);
}

/* ------------------------------------------------------------------------- */
// check whether a style uses 0.70 conventions

int get_070(const char* path)
{
    int ret;
    ret = read_file(path)->is_070;
    return ret;
}

void check_070(struct fil_list *fl)
{
    struct lin_list *tl;
    dolist (tl, fl->lines)
        if (tl->k > 11 && 0 == memcmp(tl->str+tl->k-11, "appearance", 10))
            break;
    fl->is_070 = NULL != tl;
    dolist (tl, fl->lines)
        if (stristr(tl->str+tl->k, "gradient")
         || stristr(tl->str+tl->k, "solid"))
            break;
    fl->is_style = NULL != tl;
}

int is_stylefile(const char *path)
{
    char *temp = read_file_into_buffer(path, 10000);
    int r = false;
    if (temp) {
        r = NULL != strstr(strlwr(temp), "menu.frame");
        m_free(temp);
    }
    return r;
}

/* ------------------------------------------------------------------------- */
FILE *create_rcfile(const char *path)
{
    FILE *fp;
    //dbg_printf("writing to %s", path);
    if (NULL == (fp = fopen(path, g_rc->dos_eol ? "wt" : "wb"))) {
        if (g_rc->write_error)
            g_rc->write_error(path);
    }
    return fp;
}

ST void write_rcfile(struct fil_list *fl)
{
    FILE *fp;
    unsigned ml = 0;
    struct lin_list *tl;

#ifdef DEBUG_READER
    dbg_printf("writing file %s", fl->path);
#endif
    if (NULL == (fp = create_rcfile(fl->path)))
        return;

    if (fl->tabify) {
        // calculate the max. keyword length
        dolist (tl, fl->lines)
            if (tl->k > ml) 
                ml = tl->k;
        ml = (ml+4) & ~3; // round up to the next tabstop
    }

    dolist (tl, fl->lines) {
        const char *s = tl->str + tl->k;
        if (0 == *tl->str)
            fprintf (fp, "%s\n", s); //comment
        else
            fprintf(fp, "%s:%*s%s\n", tl->str+tl->o, imax(1, ml - tl->k), "", s);
    }

    fclose(fp);
    fl->dirty = false;
}

ST void mark_rc_dirty(struct fil_list *fl)
{
    fl->dirty = true;
}

/* ------------------------------------------------------------------------- */
ST void delete_lin_list(struct fil_list *fl)
{
    freeall(&fl->lines);
    memset(fl->ht, 0, sizeof fl->ht);
    fl->wild = NULL;
}

ST void delete_fil_list(struct fil_list *fl)
{
    if (fl->dirty)
        write_rcfile(fl);
    delete_lin_list(fl);
    remove_item(&g_rc->rc_files, fl);
}

void reset_rcreader(void)
{
    while (g_rc->rc_files)
        delete_fil_list(g_rc->rc_files);
#ifdef DEBUG_READER
    dbg_printf("RESET READER");
#endif
}

/* ------------------------------------------------------------------------- */

ST VOID CALLBACK reset_reader_proc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
    if (g_rc) {
        if (g_rc->used) {
            g_rc->used = 0;
            return;
        }
        reset_rcreader();
        g_rc->timer_set = 0;
    }
    // dbg_printf("reset_reader %x %x %x %d", hwnd, uMsg, idEvent, dwTime);
    KillTimer(hwnd, idEvent);
}

ST void set_reader_timer(void)
{
    if (g_rc->timer_set)
        return;
    // dbg_printf("set_reader_timer");
    SetTimer(NULL, 0, 10, reset_reader_proc);
    g_rc->timer_set = 1;
}

/* ------------------------------------------------------------------------- */
// helpers

char *read_file_into_buffer (const char *path, int max_len)
{
    FILE *fp; char *buf; int len;
    if (NULL == (fp = fopen(path,"rb")))
        return NULL;

    fseek(fp,0,SEEK_END);
    len = ftell (fp);
    fseek (fp,0,SEEK_SET);
    if (max_len && len >= max_len)
        len = max_len-1;

    buf=(char*)m_alloc(len+1);
    fread (buf, 1, len, fp);
    fclose(fp);

    buf[len]=0;
    return buf;
}

// Scan one line in a buffer, advance read pointer, set start pointer
// and length for the caller. Returns first non-space char.
char scan_line(char **pp, char **ss, int *ll)
{
    char c, e, *d, *s, *p;
    for (p = *pp; c=*p, IS_SPC(c) && 10 != c && c; p++);
    //find end of line, replace tabs with spaces
    for (s = p; 0!=(e=*p) && 10!=e; p++)
        if (e == 9) *p = ' ';
    //cut off trailing spaces
    for (d = p; d>s && IS_SPC(d[-1]); d--);
    //ready for next line
    *d=0, *pp = p + (10 == e), *ss = s, *ll = d-s;
    return c;
}

/* ------------------------------------------------------------------------- */
// XrmResource fake wildcard pattern matcher
// -----------------------------------------
// returns: 0 for no match, else a number that is somehow a measure for
// 'how much' the item matches. Btw 'toolbar*color' means just the
// same as 'toolbar.*.color' and both match e.g. 'toolbar.color'
// as well as 'toolbar.label.color', 'toolbar.button.pressed.color', ...

// this scans one group in a keyword, that is the portion between
// dots, may be literal or '*' or '?'

int scan_component(const char **p)
{
    const char *s; char c; int n;
    for (s=*p, n=0; 0 != (c = *s); ++s, ++n)
    {
        if (c == '*' || c == '?') {
            if (n)
                break;
            do {
                c = *++s;
            } while (c == '.' || c == '*' || c == '?');
            n = 1;
            break;
        }
        if (c == '.') {
            do {
                c = *++s;
            } while (c == '.');
            break;
        }
    }
    //dbg_printf("scan_component: %d %.*s", n, n, *p);
    *p = s;
    return n;
}

int xrm_match (const char *key, const char *pat)
{
    const char *pp, *kk; int c, m, n, k, p;
    for (c = 256, m = 0; ; key = kk, c /= 2)
    {
        kk = key, k = scan_component(&kk);
        pp = pat, p = scan_component(&pp);
        if (0==k)
            return 0==p ? m : 0;
        if (0==p)
            return 0;
        if ('*' == *pat) {
            n = xrm_match(key, pp);
            if (n)
                return m + n * c/384;
            continue;
        }
        if ('?' != *pat) {
            if (k != p || 0 != memcmp(key, pat, k))
                return 0;
            m+=c;
        }
        pat=pp;
    }
}

/* ------------------------------------------------------------------------- */
struct lin_list *make_line (struct fil_list *fl, const char *key, const char *val)
{
    char buffer[MAX_KEYWORD_LENGTH];
    struct lin_list *tl, **tlp;
    int k, v;
    unsigned h;

    v = strlen(val);
    h = k = 0;
    if (key)
        h = calc_hash(buffer, key, &k, ':');

    tl=(struct lin_list*)c_alloc(sizeof (struct lin_list) + k*2 + v);
    tl->hash = h;
    tl->k = k+1;
    tl->o = k+v+2;
    if (k) {
        memcpy(tl->str, buffer, k);
        memcpy(tl->str + tl->o, key, k);
    }
    memcpy(tl->str+tl->k, val, v);

    //if the key contains a wildcard
    if (k && (memchr(key, '*', k) || memchr(key, '?', k))) {
        // add it to the wildcard - list
        tl->wnext = fl->wild;
        fl->wild = tl;
        tl->is_wild = true;
    } else {
        // link it in the hash bucket
        tlp = &fl->ht[tl->hash%RCFILE_HTS];
        tl->hnext = *tlp;
        *tlp = tl;
    }
    return tl;
}

ST void del_from_list(void *tlp, void *tl, void *n)
{
    void *v; int o = (char*)n - (char*)tl;
    while (NULL != (v = *(void**)tlp)) {
        void **np = (void **)((char *)v+o);
        if (v == tl) {
            *(void**)tlp = *np;
            break;
        }
        tlp = np;
    }
}

void free_line(struct fil_list *fl, struct lin_list *tl)
{
    if (tl->is_wild)
        del_from_list(&fl->wild, tl, &tl->wnext);
    else
        del_from_list(&fl->ht[tl->hash%RCFILE_HTS], tl, &tl->hnext);
    m_free(tl);
}

ST struct lin_list *search_line(
    struct fil_list *fl, const char *key, int fwild, LONG *p_seekpos)
{
    int key_len, n;
    char buff[MAX_KEYWORD_LENGTH];
    unsigned h;
    struct lin_list *tl;

    h = calc_hash(buff, key, &key_len, ':');
    if (0 == key_len)
        return NULL;

    ++key_len; // check terminating \0 too

    if (p_seekpos) {
        long seekpos = *p_seekpos;
        n = 0;
        dolist (tl, fl->lines)
            if (++n > seekpos && tl->hash == h && 0==memcmp(tl->str, buff, key_len)) {
                *p_seekpos = n;
                break;
            }
        return tl;
    }

    // search hashbucket
    for (tl = fl->ht[h % RCFILE_HTS]; tl; tl = tl->hnext)
        if (0==memcmp(tl->str, buff, key_len))
            return tl;

    if (fwild) {
        // search wildcards
        struct lin_list *wl;
        int best_match = 0;

        for (wl = fl->wild; wl; wl = wl->wnext) {
            n = xrm_match(buff, wl->str);
            //dbg_printf("match:%d <%s> <%s>", n, buff, sl->str);
            if (n > best_match)
                tl = wl, best_match = n;
        }
    }
    return tl;
}

void translate_new(char *buffer, int bufsize, char **pkey, int *pklen, int syntax)
{
    static const char *pairs [] =
    {
		// from         -->   to [OB -> 0.65 fork] 
        "padding.width"					, "bevelWidth"     ,
        "menu.border.width"				, "menu.*.borderWidth"     ,
        "border.width"					, "borderWidth"     ,
        "handle.width"					, "handleWidth"     ,
        "menu.border.color"				, "menu.*.borderColor"     ,
        "border.color"					, "borderColor"     ,
        "label.text.justify"			, "justify"                ,
        ".bg"							, ""                ,
        "title.text.font"				, "title.font"                ,
        "items.font"					, "frame.font"                ,
        "items.active"					, "hilite"                ,
        "items"							, "frame"                ,
        "disabled.text.color"			, "disableColor"                ,
        "active.label.text.font"        , "font"                ,
        ".active.title"					, ".title.focus"                ,
        ".active.label"					, ".label.focus"                ,
        ".active.handle"				, ".handle.focus"                ,
        ".active.grip"					, ".grip.focus"                ,
        "window.active.button.*"        , "window.button.focus"        ,
       	"window.inactive.button.*"      , "window.button.unfocus"     ,
        ".active.button.unpressed"      , ".button.focus"                ,
        ".active.button.pressed"        , ".button.pressed"            ,
        "inactive.title"				, "title.unfocus"                ,
        "inactive.label"				, "label.unfocus"                ,
        "inactive.handle"				, "handle.unfocus"                ,
        "inactive.grip"					, "grip.unfocus"                ,
        "inactive.button.unpressed"     , "button.unfocus"                ,
        "text.justify"					, "justify"                ,
        "text.color"					, "textColor"                ,
      	"image.color"					, "picColor"             ,
        "osd"							, "toolbar"     ,
        "unhighlight"					, "button"     ,
        "highlight"						, "button.pressed"     ,
		// frame values
        "window.client.padding.width"   , "window.frame.borderWidth"     ,
        "window.active.border.color"    , "window.*.focus.borderColor"  ,
        "window.inactive.border.color"  , "window.*.unfocus.borderColor" ,
        "active.client.color"			, "frame.focus.borderColor"     ,
        "inactive.client.color"			, "frame.unfocus.borderColor"   ,
        "frameColor"					, "frame.focus.borderColor"     ,
       NULL
    };

    static const char *FBpairs [] =    // older items
    {
        "window.frame.focusColor"		, "window.frame.focus.borderColor" ,
        "window.frame.unfocusColor"		, "window.frame.unfocus.borderColor" ,
        "window.frame.focus.color"		, "window.frame.focus.borderColor" ,
        "window.frame.unfocus.color"	, "window.frame.unfocus.borderColor" ,
        "frameWidth"					, "window.frame.borderWidth"     ,
        "window.frameWidth"				, "window.frame.borderWidth" ,
      NULL
    };

    const char **p = pairs;
    int k = *pklen;
    if (k >= bufsize) return;
    *pkey = (char *)memcpy(buffer, *pkey, k);
    buffer[k] = 0;
	if (syntax == 1)
		p = FBpairs;
    do
    {
        char *q = (char*)stristr(buffer, *p);
        if (q)
        {
            int lp = strlen(p[0]);
            int lq = strlen(q);
            int lr = strlen(p[1]);
            int k0 = k + lr - lp;
            if (k0 >= bufsize) break;
            memmove(q + lr, q + lp, lq - lp + 1);
            memmove(q, p[1], lr);
            k = k0;
        }
    } while ((p += 2)[0]);
    *pklen = k;
}

/* ------------------------------------------------------------------------- */
// searches for the filename and, if not found, builds a _new line-list

struct fil_list *read_file(const char *filename)
{
    struct lin_list **slp, *sl;
    struct fil_list **flp, *fl;
    char *buf, *p, *d, *s, *t, c, hashname[MAX_PATH], buff[MAX_KEYWORD_LENGTH];
    unsigned h;
    int k, is_OB, is_070;

    // ----------------------------------------------
    // first check, if the file has already been read
    h = calc_hash(hashname, filename, &k, 0);
    k = k + 1;
    for (flp = &g_rc->rc_files; NULL!=(fl=*flp); flp = &fl->next)
        if (fl->hash==h && 0==memcmp(hashname, fl->path+fl->k, k)) {
            ++g_rc->used;
            return fl; //... return cached line list.
    }

    // allocate a _new file structure, the filename is
    // stored twice, as is and strlwr'd for compare.
    fl = (struct fil_list*)c_alloc(sizeof(*fl) + k*2);
    memcpy(fl->path, filename, k);
    memcpy(fl->path+k, hashname, k);
    fl->k = k;
    fl->hash = h;
    cons_node(&g_rc->rc_files, fl);

#ifdef DEBUG_READER
    dbg_printf("reading file %s", fl->path);
#endif
    set_reader_timer();

    buf = read_file_into_buffer(fl->path, 0);
    if (NULL == buf) {
        fl->newfile = true;
        return fl;
    }

    is_OB = is_070 = false;
	if(stristr(buf, "bg:"))
	    is_OB = true;
	else
    if(stristr(buf, "appearance:"))
	    is_070 = true;

	for (slp = &fl->lines, p = buf;;)
    {
        c = scan_line(&p, &s, &k);
        if (0 == c)
            break;
        if (0 == k || c == '#' || c == '!') {
comment:
            // empty line or comment
            sl = make_line(fl, NULL, s);

        } else {
            d = (char*)memchr(s, ':', k);
            if (NULL == d)
                goto comment;
            for (t = d; t > s && IS_SPC(t[-1]); --t)
                ;
            *t = 0;
            if (t - s >= MAX_KEYWORD_LENGTH)
                goto comment;
            // skip spaces between key and value
            while (*++d == ' ')
                ;

			if (k && is_OB)
				translate_new(buff, sizeof buff, &s, &k, 0);
			else 
			if (k && false == is_070)
				translate_new(buff, sizeof buff, &s, &k, 1);

            sl = make_line(fl, s, d);
        }
        //append it to the list
        slp = &(*slp=sl)->next;
    }
    m_free(buf);
    check_070(fl);
    return fl;
}

/* ------------------------------------------------------------------------- */
// Purpose: Searches the given file for the supplied keyword and returns a
// pointer to the value - string
// In: const char* path = String containing the name of the file to be opened
// In: const char* szKey = String containing the keyword to be looked for
// In: LONG ptr: optional: an index into the file to start search.
// Out: const char* = Pointer to the value string of the keyword

const char* read_value(const char* path, const char* szKey, long *ptr)
{
    struct fil_list *fl;
    struct lin_list *tl;
    const char *r = NULL;

    fl = read_file(path);
    tl = search_line(fl, szKey, true, ptr);

    if (NULL == tl && fl->is_style && g_rc->translate_065)
        tl = search_line_065(fl, szKey);
    if (tl)
        r = tl->str + tl->k;

    g_rc->found_last_value = tl ? (tl->is_wild ? 2 : 1) : 0;

#ifdef DEBUG_READER
    { static int rcc; dbg_printf("read_value %d %s:%s <%s>", ++rcc, path, szKey, r); }
#endif
    return r;
}

/* ------------------------------------------------------------------------- */
// Find out a good place where to put an item if it was not yet found in the
// rc-file previously.

ST int simkey(const char *a0, const char *b0)
{
    const char *a = a0, *b = b0, *aa, *bb;
    int ca, cb, na, nb, m, f, e;
    for (f = e = m = na = nb = 0; ;) {
        aa = a, ca = scan_component(&a);
        bb = b, cb = scan_component(&b);
        if (0 == ca && 0 == cb)
            break;
        if (ca) ++na;
        if (cb) ++nb;
        if (0 == ca || 0 == cb || f)
            continue;
        if (ca == cb && 0 == memcmp(aa, bb, ca)) {
            m ++;
            e = 0;
        } else {
            f = 1;
            e = 0 == memcmp(aa, bb, 4);
        }
    }
    f = 2*m + e * (m && na == nb);
    //dbg_printf("sim %d <%s> <%s>", f, a0, b0);
    return f;
}

struct lin_list **get_simkey(struct lin_list **slp, const char *key)
{
    struct lin_list **tlp = NULL, *sl;
    int n, m, i, k;
    m = 1;
    i = k = 0;
    for (; NULL!=(sl=*slp); slp = &sl->next) {
        if (0 == sl->str[0])
            continue;
        n = simkey(sl->str, key);
        if (n != m)
            i = 0;
        if (n < m)
            continue;
        ++i;
        if (n > m || i > k) {
            m = n;
            k = i;
            tlp = &sl->next;
        }
    }
    return tlp;
}

/* ------------------------------------------------------------------------- */
// Search for the szKey in the file_list, replace, if found and the value
// did change, or append, if not found. Write to file on changes.

void write_value(const char* path, const char* szKey, const char* value)
{
    struct fil_list *fl;
    struct lin_list *tl, **tlp, *sl, **slp;

#ifdef DEBUG_READER
    dbg_printf("write_value <%s> <%s> <%s>", path, szKey, value);
#endif

    fl = read_file(path);
    tl = search_line(fl, szKey, false, NULL);

    if (tl && value && 0 == strcmp(tl->str + tl->k, value)) {
        // nothing changed
        if (memcmp(tl->str + tl->o, szKey, tl->k-1)) {
            // make shure that keyword has correct letter case
            memcpy(tl->str + tl->o, szKey, tl->k-1);
            mark_rc_dirty(fl);
        }
        tl->dirty = 1;
    } else {
        for (tlp = &fl->lines; *tlp != tl; tlp = &(*tlp)->next)
            ;
        if (tl) {
            *tlp = tl->next;
            free_line(fl, tl);
        }
        if (value) {
            sl = make_line(fl, szKey, value);
            sl->dirty = true;
            if (NULL == tl && false == fl->newfile) {
                // insert a new item below a similar one
                slp = get_simkey(&fl->lines, sl->str);
                if (slp) tlp = slp;
            }
            sl->next = *tlp;
            *tlp = sl;
        }
        mark_rc_dirty(fl);
    }
}

/* ------------------------------------------------------------------------- */

int rename_setting(const char* path, const char* szKey, const char* new_keyword)
{
    char buff[MAX_KEYWORD_LENGTH];
    struct fil_list *fl;
    struct lin_list **slp, *sl, *tl;
    int k, dirty = 0;

    calc_hash(buff, szKey, &k, ':');
    if (0 == k)
        return false;

    fl = read_file(path);
    for (slp = &fl->lines; NULL!=(sl=*slp); ) {
        if (new_keyword) {
            if ((int)sl->k == 1+k && 0==memcmp(sl->str, buff, k)) {
                tl = make_line(fl, new_keyword, sl->str+sl->k);
                tl->next = sl->next;
                *slp = tl;
                slp = &tl->next;
                free_line(fl, sl);
                ++dirty;
                continue;
            }
        } else {
            if ((1==k && '*' == buff[0]) || xrm_match(sl->str, buff)) {
                *slp = sl->next;
                free_line(fl, sl);
                ++dirty;
                continue;
            }
        }
        slp = &sl->next;
    }
    if (dirty)
        mark_rc_dirty(fl);
    return 0 != dirty;
}

/* ------------------------------------------------------------------------- */

int delete_setting(LPCSTR path, LPCSTR szKey)
{
#ifdef DEBUG_READER
    dbg_printf("delete_setting <%s> <%s>", path, szKey);
#endif
    return rename_setting(path, szKey, NULL);
}

/* ------------------------------------------------------------------------- */

int read_next_line(FILE *fp, char* szBuffer, unsigned dwLength)
{
    if (fp && fgets(szBuffer, dwLength, fp))
    {
        const char *p; char *q, c;
        p = q = szBuffer;
        skip_spc(&p);
        while (0 != (c = *p))
            *q++ = IS_SPC(c) ? ' ' : c, p++;
        while (q > szBuffer && IS_SPC(q[-1]))
            q--;
        *q = 0;
        return true;
    }
    szBuffer[0] = 0;
    return false;
}

/* ------------------------------------------------------------------------- */
// parse a given string and assigns settings to a StyleItem class

ST const struct styleprop styleprop_1[] = {
 {"splithorizontal" ,B_SPLITHORIZONTAL  }, // "horizontal" is match .*horizontal
 {"blockhorizontal",B_BLOCKHORIZONTAL },
 {"mirrorhorizontal",B_MIRRORHORIZONTAL },
 {"wavehorizontal",B_WAVEHORIZONTAL },
 {"splitvertical"   ,B_SPLITVERTICAL    }, // "vertical" is match .*vertical
 {"blockvertical",B_BLOCKVERTICAL   },
 {"mirrorvertical"  ,B_MIRRORVERTICAL   },
 {"solid"        ,B_SOLID           },
 {"wavevertical" ,B_WAVEVERTICAL    },
 {"vertical"     ,B_VERTICAL        },
 {"crossdiagonal",B_CROSSDIAGONAL   },
 {"diagonal"     ,B_DIAGONAL        },
 {"pipecross"    ,B_PIPECROSS       },
 {"elliptic"     ,B_ELLIPTIC        },
 {"rectangle"    ,B_RECTANGLE       },
 {"pyramid"      ,B_PYRAMID         },
 {NULL           ,-1                }
 };

ST const struct styleprop styleprop_2[] = {
 {"flat"        ,BEVEL_FLAT     },
 {"raised"      ,BEVEL_RAISED   },
 {"sunken"      ,BEVEL_SUNKEN   },
 {NULL          ,-1             }
 };

ST const struct styleprop styleprop_3[] = {
 {"bevel1"      ,BEVEL1 },
 {"bevel2"      ,BEVEL2 },
 {"bevel3"      ,BEVEL2+1 },
 {NULL          ,-1     }
 };


const struct styleprop *get_styleprop(int n)
{
    switch (n) {
        case 1: return styleprop_1;
        case 2: return styleprop_2;
        case 3: return styleprop_3;
        default : return NULL;
    }
}

int findtex(const char *p, int prop)
{
    const struct styleprop *s = get_styleprop(prop);
    do
        if (strstr(p, s->key))
            break;
    while ((++s)->key);
    return s->val;
}

void parse_item(LPCSTR szItem, StyleItem *item)
{
    char buf[256]; int t;
    strlwr(strcpy(buf, szItem));
    t = item->parentRelative = NULL != strstr(buf, "parentrelative");
    if (t) {
        item->type = item->bevelstyle = item->bevelposition = item->interlaced = 0;
        return;
    }
    t = findtex(buf, 1);
    item->type = (-1 != t) ? t : strstr(buf, "gradient") ? B_DIAGONAL : B_SOLID;

    t = findtex(buf, 2);
    item->bevelstyle = (-1 != t) ? t : BEVEL_RAISED;

    t = BEVEL_FLAT == item->bevelstyle ? 0 : findtex(buf, 3);
    item->bevelposition = (-1 != t) ? t : BEVEL1;

    item->interlaced = NULL!=strstr(buf, "interlaced");
}   

/* ------------------------------------------------------------------------- */
