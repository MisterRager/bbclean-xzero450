/*
 ============================================================================

  This file is part of the bbStyleMaker source code
  Copyright 2003-2009 grischka@users.sourceforge.net

  http://bb4win.sourceforge.net/bblean
  http://developer.berlios.de/projects/bblean

  bbStyleMaker is free software, released under the GNU General Public
  License (GPL version 2). For details see:

  http://www.fsf.org/licenses/gpl.html

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
  for more details.

 ============================================================================
*/

// bbStyleMaker::writestyle.cpp

#ifdef _MSC_VER
  #pragma warning(disable: 4800) // 'force int to bool'
#endif

#include "BBApi.h"
#include <commdlg.h>
#include "win0x500.h"
#include "bblib.h"
#include "StyleStruct.h"
#include "bbstylemaker.h"

#define ST static
#define NLS2(a,b) b

ST char stylerc_path[MAX_PATH];

LPCSTR extensionsrcPath(LPCSTR extensionsrcFileName)
{
    return "";
}

LPCSTR stylePath(LPCSTR styleFileName)
{
    return stylerc_path;
}

char *rgb_string(char *buffer, COLORREF value)
{
    sprintf(buffer, "rgb:%02x/%02x/%02x",
        GetRValue(value),
        GetGValue(value),
        GetBValue(value));
    return buffer;
}

#include "utils.cpp"
#include "rcfile.cpp"
#include "readroot.cpp"
#define BBSETTINGS_INTERNAL
#include "Settings.h"

ST void fn_write_error(const char *filename)
{
    BBMessageBox(MB_OK, NLS2("$Error_WriteFile$",
        "Error: Could not open \"%s\" for writing."), filename);
}

ST struct rcreader_init g_rc =
{
    NULL,               // struct fil_list *rc_files;
    fn_write_error,     // void (*write_error)(const char *filename);
    true,               // char dos_eol;
    false,              // char translate_065;
    0                   // char found_last_value;
};

void bb_rcreader_init(void)
{
    init_rcreader(&g_rc);
}

// -------------------------------------------------------------------------

const char *style_info_keys[] = {
    "style.name"        ,
    "style.author"      ,
    "style.date"        ,
    "style.credits"     ,
    "style.comments"    ,
    NULL
};

int check_key(const char *dd);

char *style_info[5];

void check_style(const char *style)
{
    struct fil_list *fl = read_file(style);
    struct lin_list *tl;
    dolist (tl, fl->lines)
        if (tl->str[0] && !check_key(tl->str))
            dbg_printf("check failed: %s", tl->str);
}

int readstyle(const char *fname, StyleStruct* s, int root)
{
    if (root)
        strcpy(stylerc_path, fname);

    ReadStyle(fname, s);

    if (root) {
        int i;
        for (i = 0; i < 5; ++i) {
            const char *p = ReadString(fname, style_info_keys[i], "");
            replace_str(&style_info[i], p);
        }
    }

    //check_style(fname);
    return 1;
}

//===========================================================================
// This one checks whether an item belongs to usual blackbox style-syntax

struct ck {
    const char *p;
    const struct ck *k;
};

const struct ck cp_font [] = {
    { "font"                , NULL },
    { "fontHeight"          , NULL },
    { "fontWeight"          , NULL },
    { "justify"             , NULL },
    { "alignment"           , NULL },
    { NULL, NULL}
};

const struct ck cp_border [] = {
    { "borderColor"         , NULL },
    { "borderWidth"         , NULL },
    { NULL, NULL}
};

const struct ck cp_texture [] = {
    { ""                    , NULL },
    { "color"               , NULL },
    { "colorTo"             , NULL },

    { "appearance"          , NULL },
    { "color1"              , NULL },
    { "color2"              , NULL },
    { "backgroundColor"     , NULL },
    { ">"                   , cp_border },
    { NULL, NULL}
};

const struct ck cp_ee_label [] = {
    { ">"                   , cp_texture },
    { "textColor"           , NULL },
    { NULL, NULL}
};

const struct ck cp_ee_label_mar [] = {
    { "marginWidth"         , NULL },
    { NULL                  , cp_ee_label },
};


const struct ck cp_ee_button [] = {
    { ">"                   , cp_texture },
    { "picColor"            , NULL },
    { "foregroundColor"     , NULL },
    { NULL, NULL}
};

const struct ck cp_button [] = {
    { "pressed"         , cp_ee_button },
    { "marginWidth"     , NULL },
    { NULL              , cp_ee_button },
};

const struct ck cp_toolbar [] = {
    { ">"               , cp_ee_label_mar },
    { "label"           , cp_ee_label_mar },
    { "clock"           , cp_ee_label },
    { "windowlabel"     , cp_ee_label },
    { "button"          , cp_button },
    { NULL              , cp_font }
};

const struct ck cp_ee_mframe [] = {
    { ">"                   , cp_texture },
    { "textColor"           , NULL },
    { "disableColor"        , NULL },
    { "bulletColor"         , NULL },
    { "picColor"            , NULL },
    { "foregroundColor"     , NULL },
    { "disabledColor"       , NULL },
    { "marginWidth"         , NULL },
    { NULL, NULL}
};

const struct ck cp_menu_title [] = {
    { ">"               , cp_ee_label_mar },
    { NULL              , cp_font },
};

const struct ck cp_menu_frame [] = {
    { ">"               , cp_ee_mframe },
    { NULL              , cp_font },
};


const struct ck cp_menu_bullet [] = {
    { "position"        , NULL },
    { ""                , NULL },
    { NULL, NULL}
};

const struct ck cp_menu [] = {
    { "title"           , cp_menu_title },
    { "frame"           , cp_menu_frame },
    { "active"          , cp_ee_mframe },
    { "hilite"          , cp_ee_mframe },
    { "bullet"          , cp_menu_bullet },
    { NULL, NULL}
};

// ------------------------------------------

const struct ck cp_win_han [] = {
    { "focus"           , cp_texture },
    { "unfocus"         , cp_texture },
    { NULL, NULL}
};

const struct ck cp_win_tit [] = {
    { "focus"           , cp_texture },
    { "unfocus"         , cp_texture },
    { "marginWidth"     , NULL },
    { NULL, NULL}
};

const struct ck cp_win_lbl [] = {
    { "focus"           , cp_ee_label },
    { "unfocus"         , cp_ee_label },
    { "marginWidth"     , NULL },
    { NULL, NULL}
};

const struct ck cp_win_btn [] = {
    { "pressed"         , cp_ee_button },
    { "focus"           , cp_ee_button },
    { "unfocus"         , cp_ee_button },
    { "marginWidth"     , NULL },
    { NULL, NULL}
};

// ------------------------------------------
const struct ck cp_win_frm [] = {
    { "focusColor"      , NULL },
    { "unfocusColor"    , NULL },
    { "borderWidth"     , NULL },
    { "focus"           , cp_border },
    { "unfocus"         , cp_border },
    { NULL, NULL}
};

// ------------------------------------------
const struct ck cp_window [] = {
    { "title"           , cp_win_tit },
    { "label"           , cp_win_lbl },
    { "button"          , cp_win_btn },
    { "handle"          , cp_win_han },
    { "grip"            , cp_win_han },
    { "frame"           , cp_win_frm },
    { "handleHeight"    , NULL },
    { NULL              , cp_font }
};

const struct ck cp_slit [] = {
    { ">"                   , cp_texture },
    { "marginWidth"         , NULL },
    { NULL, NULL}
};

const struct ck cp_info [] = {
    { "name",       NULL },
    { "author",     NULL },
    { "date",       NULL },
    { "credits",    NULL },
    { "comments",   NULL },
    { NULL, NULL}
};

const struct ck cp_misc [] = {
    { "borderWidth"         , NULL },
    { "borderColor"         , NULL },
    { "bevelWidth"          , NULL },
    { "handleWidth"         , NULL },
    { "frameWidth"          , NULL },
    { "titleFont"           , NULL },
    { "menuFont"            , NULL },
    { "titleJustify"        , NULL },
    { "menuJustify"         , NULL },
    { "rootCommand"         , NULL },
    { NULL, NULL}
};

const struct ck cp_all [] = {
    { "toolbar"     , cp_toolbar },
    { "menu"        , cp_menu },
    { "window"      , cp_window },
    { "slit"        , cp_slit },
    { "style"       , cp_info },
    { NULL          , cp_misc },
};

int check_item(const char *dd, const struct ck *ck)
{
    const char *cc; int c;

    if (NULL == ck)
        return 0 == *dd;

    cc = dd;
    c = scan_component(&dd);
    //dbg_printf("scan: <%.*s>", c, cc);

    if (c == 1 && cc[0] == '*') {
        if (0 == *dd) // no wildcard at the end allowed
            return 0;
        if (check_item(dd, ck))
            return 1;
    }

    for (;;) {
        if (ck->p == NULL) {
            if (ck->k)
                return check_item(cc, ck->k);
            return 0;
        } else if (ck->p[0] == '>') {
            if (check_item(cc, ck->k))
                return 1;
        } else if (c == 1 && cc[0] == '*') {
            if (check_item(dd, ck->k))
                return 1;
            if (check_item(cc, ck->k))
                return 1;
        } else if (c == (int)strlen(ck->p) && 0 == memicmp(ck->p, cc, c)) {
            return check_item(dd, ck->k);
        }
        ++ck;
    }
}

int check_key(const char *dd)
{
    return check_item(dd, cp_all);
}

// check for the presence of a specific comment string in the style
struct lin_list *FindRCComment(struct fil_list *fl, LPCSTR keyword)
{
    struct lin_list *tl;
    dolist (tl, fl->lines)
        if (tl->k==1 && 0==stricmp(tl->str+tl->k, keyword))
            return tl;
    return NULL;
}

// add one line to a style
void add_line(struct lin_list ***tlp, struct lin_list *sl)
{
    sl->next = **tlp;
    **tlp = sl;
    *tlp = &sl->next;
    sl->dirty = true;
}

// add more lines to a style
void add_lines(struct lin_list **tlp, struct lin_list *sl)
{
    struct lin_list *tl;
    while (sl)
    {
        tl = sl->next;
        add_line(&tlp, sl);
        sl = tl;
    }
}

// search a line in a style
struct lin_list **get_line(struct fil_list *fl, struct lin_list *tl)
{
    struct lin_list **tlp;
    for (tlp = &fl->lines; *tlp != tl; tlp = &(*tlp)->next);
    return tlp;
}

// search best place to put a line, i.e. after some similar line
struct lin_list **get_line_after(struct fil_list *fl, const char *key)
{
    struct lin_list **tlp, **slp;
    tlp = &fl->lines;
    slp = get_simkey(tlp, key);
    if (slp) {
        for (;;) {
            tlp = slp;
            slp = get_simkey(tlp, key);
            if (NULL == slp)
                break;
        }
        return tlp;
    } else {
        return get_line(fl, NULL);
    }
}

// check if this is the 3dc start comment
bool is_3dc_start(struct lin_list *tl)
{
    const char *s;
    return tl && (s = tl->str + tl->k,
        (s[0] == '!' || s[0] == '#')
        && stristr(s, "3dc")
        && stristr(s, "start"));
}

// pull out the 3dc info from a style
struct lin_list *get_3dc(struct fil_list *fl)
{
    struct lin_list **tlp, *tl, *sl = NULL, **slp = &sl;
    for (tlp = &fl->lines; NULL != (tl = *tlp); )
    {
        if (tl->str[0]) {
            if (sl) break;
        } else {
            const char *s = tl->str + tl->k;
            if (sl) {
                if ((s[0] >= '0' && s[0] <= '9') || s[0] == '!' || s[0] == '#') {
            append:
                    *tlp = tl->next;
                    tl->next = NULL;
                    add_line(&slp, tl);
                    continue;
                }
                break;
            } else if (0 == s[0] && is_3dc_start(tl->next)) {
                goto append;
            } else if (is_3dc_start(tl)) {
                goto append;
            }
        }
        tlp = &tl->next;
    }
    return sl;
}

//===========================================================================

// Remove all unknown items from style, optionally append them at the end
// under a comment string !-- other --
struct lin_list *ClearRCFile(struct fil_list *fl, bool append, bool is_070)
{
    struct lin_list *tl, **tlp = &fl->lines, *sl = NULL, **slp = &sl;

    while (NULL != (tl = *tlp)) {
        if (tl->str[0] && false == tl->dirty) {
            int f = check_key(tl->str);
            if (f) {
                *tlp = tl->next;
                fl->dirty = true;
                free_line(fl, tl);
            } else if (append) {
                *tlp = tl->next;
                fl->dirty = true;
                *slp = tl;
                slp = &tl->next;
                tl->next = NULL;
            } else {
                tlp = &tl->next;
            }
        } else {
            tlp = &tl->next;
        }
    }
    return sl;
}

//===========================================================================

// Move multiple properties to wildcards
// It reads wildcard-suggestions from the bbstylemaker.rc file and then
// checks whether it would make sense to apply them

struct lin_list* make_wildcard(
    struct fil_list *fl,
    const char *wc,
    struct lin_list *ol,
    struct lin_list **dl
    )
{
    struct wc_list {
        struct wc_list *next;
        int n;
        const char *str;
    };

    struct lin_list *sl, **slp, *rl, *ql;
    struct wc_list *wl = NULL, *w, *w2;

    char wbuf[200];
    int m, n;
    strlwr(strcpy(wbuf, wc));

    // first build a list of all items that match the suggested wildcard
    dolist (sl, fl->lines) {
        sl->flags = 0;
        if (sl->str[0]
         && false == sl->is_wild
         && 0 != (m = xrm_match(sl->str, wbuf))) {
            //dbg_printf("match: %s %s", wc, sl->str);
            dolist (w, wl)
                if (0 == strcmp(sl->str + sl->k, w->str))
                    break;

            if (NULL == w) {
                w = (struct wc_list *)m_alloc(sizeof *w);
                cons_node(&wl, w);
                w->n = 0;
                w->str = new_str(sl->str + sl->k);
            }

            // here we test whether this line matches more with the current
            // wildcard than with an already processed one.
            dolist (rl, ol) {
                n = xrm_match(sl->str, rl->str);
                if (n >= m)
                    break;
            }

            if (NULL == rl) {
                w->n ++;
                sl->flags |= 1;
            }
        }
    }

    // then see which value was most often used from them
    m = 0, w2 = NULL;
    dolist (w, wl) {
        //dbg_printf("%d:%s %s", w->n, wc, w->str);
        if (w->n > m) {
            int d, o = 0;

            // first we test whether this would override a wildcard
            // that already has been set.

            dolist (rl, *dl) { // deleted lines list
                d = xrm_match(rl->str, wbuf);
                if (d) {
                    dolist (ql, ol) { // already applied wc's
                        o = xrm_match(rl->str, ql->str);
                        if (o && 0 == strcmp(ql->str + ql->k, rl->str + rl->k))
                            break;
                    }
                    if (ql && o <= d) // would override
                        break;
                }
            }
            if (NULL == rl)
                m = w->n, w2 = w;
        }
    }

    // if a value was used more than one time
    if (m > 1) {
        // backup value
        char *s2 = new_str(w2->str);
        // remove the explicit items
        for (slp = &fl->lines; NULL!=(sl=*slp); ) {
            if ((sl->flags & 1) && 0 == strcmp(sl->str + sl->k, s2)) {
                *slp = sl->next;
                sl->next = *dl;
                *dl = sl;
                //dbg_printf("delete: %s %s | %s %s", wc, s2, sl->str, sl->str + sl->k);
                continue;
            }
            slp = &sl->next;
        }
        // make wildcard line
        sl = make_line(fl, wc, s2);
        free_str(&s2);
    }
    freeall(&wl);
    return sl;
}

struct lin_list *make_wildcards(struct fil_list *fl)
{
    LONG l = 0;
    struct lin_list *tl, *sl = NULL, **slp = &sl, *dl = NULL;

    for (;;) {
        const char *p = ReadValue(rcpath, "bbstylemaker.wildcards.try", &l);
        if (NULL == p)
            break;
        tl = make_wildcard(fl, p, sl, &dl);
        if (tl)
            slp = &(*slp=tl)->next;
    }

    while (dl) {
        tl = dl->next;
        free_line(fl, dl);
        dl = tl;
    }

    return sl;
}

//===========================================================================

struct bulletstyles { const char *s; int b; } bulletstyles[]= {
    { "empty"    , BS_EMPTY },
    { "triangle" , BS_TRIANGLE },
    { "square"   , BS_SQUARE },
    { "diamond"  , BS_DIAMOND },
    { "circle"   , BS_CIRCLE },
    { NULL       , BS_TRIANGLE }
};

int get_bulletstyle (const char *tmp)
{
    struct bulletstyles *bp;
    for (bp = bulletstyles; bp->s && stricmp(tmp,bp->s);bp++);
    return bp->b;
}

int get_bulletpos (const char *tmp)
{
    static const struct { const char *s; int b; } *bp,bs[]= {
        { "right" , DT_RIGHT },
        { "default" , -1 },
        { NULL  , DT_LEFT }
        };
    for (bp = bs;bp->s && NULL == stristr(tmp, bp->s); bp++);
    return bp->b;
}

const char * get_bullet_string (int s)
{
    if (s > BS_CIRCLE || s < BS_EMPTY)
        s = BS_TRIANGLE;
    return bulletstyles[s].s;
}


static const char *getweight_string (int w)
{
    static const char *fontweightstrings[] = {
    "thin",
    "extralight",
    "light",
    "normal",
    "medium",
    "demibold",
    "bold",
    "extrabold",
    "heavy"
    };
    if (w == 0)
        w = FW_NORMAL;
    return fontweightstrings[iminmax(w,100,900)/100-1];
}

char *addstr(char **pp, const char *s, int spc)
{
    int l = strlen(s);
    if (spc) **pp = ' ', ++*pp;
    memcpy(*pp, s, l+1);
    *pp += l;
    return *pp;
}

static int fontMode;
static int colorMode;
static int useWildcards;

void WriteColorEx(LPCSTR fileName, LPCSTR szKey, COLORREF value)
{
    char buffer[40];
#if 0
    const char *p = ReadString(fileName, szKey, NULL);
    if (p && value == ReadColorFromString(p))
        WriteString(fileName, szKey, p);
    else
#endif
    if (colorMode == 1)
        WriteString(fileName, szKey, rgb_string(buffer, value));
    else
        WriteColor(fileName, szKey, value);
}

//===========================================================================

int make_fontstring(StyleItem *si, char *out)
{
    int h, w;
    const char *p;
    char *tp = out;

    p = si->Font;
    h = si->FontHeight;
    w = si->FontWeight;

    *tp = 0;
    if (0 == p[0] && 1 != fontMode)
        return 0;

    if (4 == fontMode) {
        // X-Windows type font description
        const char *sw = "*";
        const char *sh = "*";
        char bh[10];
        const char *s;
        const char *c = "-";
        //-*-lucidatypewriter-medium-r-normal-*-10-100-75-75-m-60-iso8859-1
        if (w && iabs(w - FW_NORMAL) > 100)
            sw = getweight_string(w);
        if (h) {
            sprintf(bh, "%d", h);
            sh = bh;
        }
        s = strchr(p, '-');
        if (s) sw = c = "";
        sprintf(out, "-*-%s%s%s-r-*-*-%s-*-*-*-*-*-*-*", p, c, sw, sh);
        return 2;

    } else if (3 == fontMode || 2 == fontMode) {
        // font/12/bold or font-12-bold
        addstr(&tp, p, 0);
        if ((h && h != 12) || (w && w != FW_NORMAL)) {
            char sep = 2 == fontMode ? '/' : '-';
            tp += sprintf(tp, "%c%d", sep, h);
            if ((w && w != FW_NORMAL)) {
                *tp++ = sep;
                if (w && w != FW_NORMAL)
                    addstr(&tp, getweight_string(w), 0);
            }
        }
        return 2;

    } else {

        sprintf(tp, "%s", p);
        return 1;
    }
}


//===========================================================================
static void write_style_item (const char * style, StyleStruct *pStyle, StyleItem *si, const char *key, int v, int sn)
{
    struct s_prop { const char *k; short mode; short v; };

    static const struct s_prop s_prop_065[]= {
    // texture type
    { "",                  C_TEX , V_TEX  },
    // colors, from, to, text, pics
    { ".color",            C_CO1 , V_CO1 },
    { ".colorTo",          C_CO2 , V_CO2 },
    { ".picColor",         C_PIC , V_PIC },
    { ".textColor",        C_TXT , V_TXT },
    { ".disableColor",     C_DIS , V_DIS },
    // font settings
    { ".font",             C_FON , V_FON },
    { ".justify",          C_JUS , V_JUS },
    { NULL, 0, 0}
    };

    static const struct s_prop s_prop_070[]= {
    // texture type
    { ".appearance",       C_TEX , V_TEX  },

    // colors, from, to, text, pics
    { ".color1",           C_CO1 , V_CO1 },
    { ".color2",           C_CO2 , V_CO2 },
    { ".foregroundColor",  C_PIC , V_PIC },
    { ".textColor",        C_TXT , V_TXT },
    { ".disabledColor",    C_DIS , V_DIS },

    // borders & margins - _new in BBNix 0.70
    { ".borderColor",      C_BOC , V_BOC },
    { ".borderWidth",      C_BOW , V_BOW },
    { ".marginWidth",      C_MAR , V_MAR },

    // font settings
    { ".font",             C_FON , V_FON },
    { ".alignment",        C_JUS , V_JUS },
    { NULL, 0, 0}
    };

    const struct s_prop *cp = s_prop_070;
    char fullkey[80];
    char tex[100];
    int l, t, u;
    char *tp;

    if (false == pStyle->is_070) {
        cp = s_prop_065;
        if (v & I_ACT)
            key = "menu.hilite";
    }

    l = strlen(key);
    memcpy(fullkey, key, l);
    do {
        if (cp->v & v)
        {
            strcpy(fullkey + l, cp->k);
            switch (cp->mode) {

            // --- textture ---
            case C_TEX:
                tp = tex;
                if (sn == SN_MENUTITLE && pStyle->menuNoTitle) {
                    addstr(&tp, "hidden", 0);
                } else if (si->parentRelative) {
                    addstr(&tp, "parentrelative", 0);
                } else {
                    // bevelstyle
                    t = si->bevelstyle;
                    u = si->bevelposition;
                    if (u == 0 || t > 2)
                        t = 0;
                    addstr(&tp, get_styleprop(2)[t].key, 0);
                    if (t && u >= BEVEL2 && u <= BEVEL2+1) {
                        addstr(&tp, get_styleprop(3)[u-1].key, 1);
                    }

                    // texture
                    t = si->type;
                    if (t > 7)
                        t = B_SOLID;


                    for (u = 0; u < 2; ++u) {

                        if (u == (int)pStyle->is_070) {
                        // ---------------------------
                        if (t == B_SOLID)
                            addstr(&tp, "solid", 1);
                        else
                            addstr(&tp, "gradient", 1);
                        // ---------------------------
                        } else {
                        // ---------------------------
                        if (t != B_SOLID)
                            addstr(&tp, get_styleprop(1)[1+get_styleprop(1)[1+t].val].key, 1);

                        if (si->interlaced)
                            addstr(&tp, "interlaced", 1);
                        // ---------------------------
                        }
                    }
                }

                if (sn == SN_MENUTITLE) {
                    if (pStyle->menuTitleLabel)
                        addstr(&tp, "label", 1);
                }

                if ((v & V_BOW) && 0 != si->borderWidth && pStyle->is_070)
                    addstr(&tp, "border", 1);

                WriteString(style, fullkey, tex);
                break;

            // --- colors ---
            case C_CO1:
                if (false == si->parentRelative) {
                    if (si->type == B_SOLID && false == si->interlaced && pStyle->is_070)
                        strcpy(fullkey + l, ".backgroundColor");
                    WriteColorEx(style, fullkey, si->Color);
                }
                break;

            case C_CO2:
                if (false == si->parentRelative && !(si->type == B_SOLID && false == si->interlaced)) {
                    WriteColorEx(style, fullkey, si->ColorTo);
                }
                break;

            case C_TXT:
                WriteColorEx(style, fullkey, si->TextColor);
                break;

            case C_DIS:
                WriteColorEx(style, fullkey, si->disabledColor);
                break;

            case C_PIC:
                if (0 == (v & V_TXT))
                    WriteColorEx(style, fullkey, si->picColor);
                else
                if (pStyle->is_070 || useWildcards || si->foregroundColor != si->TextColor)
                    WriteColorEx(style, fullkey, si->foregroundColor);
                break;

            case C_BOC:
                if (si->borderWidth)
                    WriteColorEx(style, fullkey, si->borderColor);
                break;

            case C_BOW:
                if (si->borderWidth)
                    WriteInt(style, fullkey, si->borderWidth);
                break;

            case C_MAR:
                if (si->validated & V_MAR)
                    WriteInt(style, fullkey, si->marginWidth);
                break;

            // --- Font ---
            case C_FON:
                if (0 == useWildcards)
                {
                    const char *p = ReadString(style, fullkey, "");
                    StyleItem SI;
                    memset(&SI, 0, sizeof SI);
                    SI.FontHeight = 12;
                    SI.FontWeight = FW_NORMAL;
                    parse_font(&SI, p);
                    if (SI.FontHeight == si->FontHeight
                        && SI.FontWeight == si->FontWeight
                        && 0 == strcmp(SI.Font, si->Font)) {
                        WriteString(style, fullkey, p);
                        break;
                    }
                }

                t = make_fontstring(si, tex);
                WriteString(style, fullkey, tex);

                if (1 == t) {
                    int h = si->FontHeight;
                    int w = si->FontWeight;
                    if (useWildcards || (h && h != 12)) {
                        strcpy(fullkey + l + 5, "Height");
                        WriteInt(style, fullkey, h);
                    }
                    if (useWildcards || (w && iabs(w - FW_NORMAL) > 100)) {
                        strcpy(fullkey + l + 5, "Weight");
                        WriteString(style, fullkey, getweight_string(w));
                    }
                }
                break;

            // --- Alignment ---
            case C_JUS:
                WriteString(style, fullkey,
                    si->Justify == DT_CENTER?"center":
                    si->Justify == DT_RIGHT?"right":
                    "left");
                break;
            }
        }
    }
    while ((++cp)->k);
}

//===========================================================================
const char auto_string1[] = "! Automagically generated by bbstylemaker";
const char auto_string2[] = "# Converted to blackbox 0.70 style format by bbstylemaker";
const char header_string[] = "! Stylefile for blackbox";

int is_style_old(const char *style)
{
    bool is_070;
    struct fil_list *fl;

    is_070 = get_070(style);
    //dbg_printf("070: %d (%s)", is_070, style);

    if (write_070 != is_070) {
        //dbg_printf("writing in %d (%s)", write_070, style);
        return true;
    }

    fl = read_file(style);
    if (NULL == FindRCComment(fl, auto_string1))
        return true;

    return false;
}

#define STYLECOMMENT(s) "!-- "s" --"

int writestyle(
    const char *style,
    StyleStruct *pStyle,
    char **style_info,
    int flags
    )
{
    struct fil_list *fl;
    struct lin_list *sl, *tl, **tlp, *tl_3dc;
    const char *auto_string;
    bool newfile;
    bool tabify;
    bool has_auto;
    const char *p, *q;
    const struct items *s;
    int i, r;


    // get some options
    tabify = ReadBool(rcpath, "bbstylemaker.tabify", false);
    g_rc.dos_eol = 0 == stricmp(ReadString(rcpath, "bbstylemaker.eolMode", ""), "dos");
    colorMode = 0 == stricmp(ReadString(rcpath, "bbstylemaker.colorMode", ""), "rgb");
    fontMode = ReadInt(rcpath, "bbstylemaker.fontMode", 1);
    useWildcards = ReadBool(rcpath, "bbstylemaker.wildcards.use", false);

    tlp = NULL;

    reset_rcreader();
    auto_string = (flags & 1) ? auto_string2 : auto_string1;

    fl = read_file(style);

    // translate 065 to 070 items
    if (pStyle->is_070)
        make_style070(fl);
    else
        make_style065(fl);

    fl->tabify = tabify;
    newfile = fl->newfile;
    has_auto = NULL != FindRCComment(fl, auto_string);

    tl_3dc = get_3dc(fl);

    // put some header comments
    if (newfile)
    {
        WriteString(style, "", header_string);
        WriteString(style, "", auto_string);
        WriteString(style, "", "");
        WriteString(style, "", STYLECOMMENT("info"));
    }
    else
    if (false == has_auto)
    {
        tl = FindRCComment(fl, header_string);
        sl = make_line(fl, NULL, auto_string);
        if (tl)
            tlp = &tl->next;
        else
            tlp = &fl->lines;
        add_line(&tlp, sl);
        if (NULL == tl)
            add_line(&tlp, make_line(fl, NULL, ""));
    }

    // put the style infos
    for (i = 0; i < 5; ++i) {
        const char *key = style_info_keys[i];
        if (style_info) {
            p = style_info[i];
            if (p[0] || (newfile && 0 == i))
                WriteString(style, key, p);
        } else {
            const char *ref = ReadString(style, key, NULL);
            if (ref)
                WriteString(style, key, ref);
        }
    }

    // skip old 065 global items
    s = GetStyleItems();
    while (s->sn != SN_ROOTCOMMAND)
        ++s;

    do
    {
        int sn = s->sn;
        void *v = StyleStructPtr(sn, pStyle);
        const char *t = NULL;

        // skip these if in 065 mode
        if (false == pStyle->is_070 &&
            (sn == SN_WINFOCUS_FRAME_COLOR
            || sn == SN_WINUNFOCUS_FRAME_COLOR
            || sn == SN_FRAMEWIDTH
            || sn == SN_HANDLEHEIGHT
            ))
            continue;

        if (newfile) {
            // put in appropriate comment headers once in a while
            if (sn == SN_ROOTCOMMAND)
                t = STYLECOMMENT("background");
            else
            if (sn == SN_TOOLBAR)
                t = STYLECOMMENT("toolbar");
            else
            if (sn == SN_SLIT)
                t = STYLECOMMENT("slit");
            else
            if (sn == SN_MENUTITLE)
                t = STYLECOMMENT("menu");
            else
            if (sn == SN_WINFOCUS_TITLE && (s->flags & V_TEX))
                t = STYLECOMMENT("window");

            if (t) {
                WriteString(style, "", "");
                WriteString(style, "", t);
            }
        }

        // output the items
        switch (s->type)
        {
            case C_STY:
                write_style_item (style, pStyle, (StyleItem*)v, s->rc_string, s->flags, s->sn);
                break;

            case C_INT:
                WriteInt(style, s->rc_string, *(int*)v);
                break;

            case C_BOL:
                WriteBool(style, s->rc_string, *(bool*)v);
                break;

            case C_COL:
                WriteColorEx(style, s->rc_string, *(COLORREF*)v);
                break;

            case C_STR:
                p = (const char*)v;
                if (0 == *p && sn != SN_ROOTCOMMAND)
                    continue;
                WriteString(style, s->rc_string, p);
                break;
        }
    } while ((++s)->sn);

    // append 065 global items
    if (false == pStyle->is_070) {
        const char *misc_comment = STYLECOMMENT("misc.");
        if (newfile) {
            WriteString(style, "", "");
            WriteString(style, "", misc_comment);
        } else {
            tl = FindRCComment(fl, misc_comment);
            if (NULL == tl)
            {
                if (NULL == tlp)
                    tlp = get_line(fl, NULL);
                add_line(&tlp, make_line(fl, NULL, ""));
                add_line(&tlp, make_line(fl, NULL, misc_comment));
            }
        }

        WriteColorEx(style, "borderColor", pStyle->borderColor);
        WriteInt(style, "borderWidth", pStyle->borderWidth);
        WriteInt(style, "bevelWidth", pStyle->bevelWidth);
        //WriteInt(style, "frameWidth", pStyle->frameWidth);
        WriteInt(style, "handleWidth", pStyle->handleHeight);

        if (false == newfile) {
        if (NULL != (p = ReadString(style, q = "window.frame.focusColor", NULL)))
            WriteString(style, q, p);
        if (NULL != (p = ReadString(style, q = "window.frame.unfocusColor", NULL)))
            WriteString(style, q, p);
        if (NULL != (p = ReadString(style, q = "frameWidth", NULL)))
            WriteString(style, q, p);
        }

    }

    if (useWildcards)
    {
        // do the wildcard trick
        sl = make_wildcards(fl);
        if (sl) {
            const char *wc_comment = STYLECOMMENT("wildcards");
            tl = FindRCComment(fl, wc_comment);
            if (NULL == tl)
            {
                tlp = get_simkey(&fl->lines, "rootCommand");
                if (NULL == tlp)
                    tlp = get_line(fl, NULL);

                if (newfile) {
                    add_line(&tlp, make_line(fl, NULL, ""));
                    add_line(&tlp, make_line(fl, NULL, wc_comment));
                }
            }
            else
                tlp = &tl->next;

            add_lines(tlp, sl);
        }
    }

    if (false == newfile)
    {
        // remove redundant items and append unknown ones
        sl = ClearRCFile(fl, 0 != (flags & 2), pStyle->is_070);
        if (sl) {
            const char *lf_comment = STYLECOMMENT("lost & found");
            tl = FindRCComment(fl, lf_comment);
            if (NULL == tl)
            {
                tlp = get_line_after(fl, "window");
                add_line(&tlp, make_line(fl, NULL, ""));
                add_line(&tlp, make_line(fl, NULL, lf_comment));
            }
            else
                tlp = &tl->next;

            add_lines(tlp, sl);
        }
    }

    if (tl_3dc) {
        tlp = get_line(fl, NULL);
        add_lines(tlp, tl_3dc);

    }

    // flush file to disk
    r = fl->dirty;
    reset_rcreader();
    return r;
}

/*----------------------------------------------------------------------------*/
