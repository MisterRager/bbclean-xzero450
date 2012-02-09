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
/* parse bsetroot commandline */

#include "BBApi.h"
#include "bbroot.h"
#include "bbrc.h"

#define ST static

void init_root(struct rootinfo *r)
{
    // clear and set some default values
    memset(r, 0, sizeof *r);
    r->modx = r->mody = 4;
    r->type = B_SOLID;
    r->sat = 255;
    r->hue = 0;
    r->scale = 100;
}

void delete_root(struct rootinfo *r)
{
    freeall(&r->paths);
}

const char *get_root_switch(int n)
{
    return n >= E_solid && n < E_last ? switches[n-E_solid] : "";
}

ST int next_token(struct rootinfo *r)
{
    int s = E_eos;
    strlwr(NextToken(r->token, &r->cptr, " "));
    r->flag = 1;
    if (r->token[0]) {
        const char * p = r->token;
        if (0 == strncmp(p, "-no", 3))
            r->flag = 0, p+= 3;
        s = get_string_index(p, switches) + E_other + 1;
    }
    //dbg_printf("token %d: %s", s, r->token);
    return s;
}

ST int read_color(const char *token, COLORREF *pCR)
{
    COLORREF CR = ReadColorFromString(token);
    if ((COLORREF)-1 == CR) return false;
    *pCR = CR;
    return true;
}

ST int read_int(const char *token, int *ip)
{
    const char *p = token;
    if ('-' == *p) ++p;
    if (*p < '0' || *p > '9') return false;
    *ip = atoi(token);
    return true;
}

int parse_root(struct rootinfo *r, const char *command)
{
    r->cptr = command;
    for (;;)
    {
        int s = next_token(r);
cont_1:
        switch (s)
        {
        case E_eos:
            return true;

        case E_help:
            r->help = 1;
            return true;

        default:
            if ('-' == r->token[0])
                return false;
            goto get_img_1;

        case E_tile:
        case E_t:
            goto img_tile;
        case E_full:
        case E_f:
            goto img_full;
        case E_center:
        case E_c:
            goto img_center;

        case E_bitmap:
            s = next_token(r);
            switch (s) {
            case Etile:
            img_tile:
                r->wpstyle = WP_TILE;
                break;
            case Ecenter:
            img_center:
                r->wpstyle = WP_CENTER;
                break;
            case Estretch:
            img_full:
                r->wpstyle = WP_FULL;
                break;
            default:
                goto get_img_1;
            }
        get_img:
            s = next_token(r);
        get_img_1:
            if (E_eos == s) return false;
            if (r->bmp) return false;
            unquote(strcpy(r->wpfile, r->token));
            r->bmp = 1;
            continue;

        case E_solid:
            r->solid = 1;
            s = next_token(r);
            if (s == Einterlaced)
            {
                r->interlaced = true;
                s = next_token(r);
                if (s != E_other)
                    goto cont_1;
            }
            if (false==read_color(r->token, &r->color1)) return false;
            if (r->interlaced) r->color2 = shadecolor(r->color1, -40);
            continue;

        case E_bg:
        case E_background:
        case E_from:
            next_token(r);
            if (false==read_color(r->token, &r->color1)) return false;
            continue;

        case E_to:
            next_token(r);
            if (false==read_color(r->token, &r->color2)) return false;
            continue;

        case E_splitFrom:
            next_token(r);
            if (false==read_color(r->token, &r->color_from)) return false;
            continue;

        case E_splitTo:
            next_token(r);
            if (false==read_color(r->token, &r->color_to)) return false;
            continue;

        case E_fg:
        case E_foreground:
            next_token(r);
            if (false==read_color(r->token, &r->modfg)) return false;
            if (r->solid && r->interlaced)
                r->color2 = r->modfg;
            continue;

        case E_gradient:
            r->gradient = 1;
            r->type = B_HORIZONTAL;
            for (;;) {
                int n, f = 0;
                s = next_token(r);
                if (E_eos == s || '-' == r->token[0]) break;

                n = findtex(r->token, 1);
                if (-1 != n) r->type = n; else ++f;

                n = findtex(r->token, 2);
                if (-1 != n) r->bevelstyle = n; else ++f;

                n = findtex(r->token, 3);
                if (-1 != n) r->bevelposition = n; else ++f;

                n = NULL != strstr(r->token, "interlaced");
                if (0 != n) r->interlaced = true; else ++f;

                if (f==4) break;
            }
            if (r->bevelstyle) {
                if (0 == r->bevelposition)
                    r->bevelposition = BEVEL1;
            } else {
                if (0 != r->bevelposition)
                    r->bevelstyle = BEVEL_RAISED;
            }
            goto cont_1;

        case E_mod:
            r->mod = 1;
            next_token(r);
            if (false == read_int(r->token, &r->modx)) return false;
            next_token(r);
            if (false == read_int(r->token, &r->mody)) return false;
            continue;

        case Einterlaced:
            r->interlaced = true;
            continue;

        case E_hue:
            next_token(r);
            if (!read_int(r->token, &r->hue)) return false;
            continue;

        case E_sat:
            next_token(r);
            if (!read_int(r->token, &r->sat)) return false;
            continue;

        case E_scale:
            next_token(r);
            if (!read_int(r->token, &r->scale)) return false;
            continue;

        case E_vdesk:
            r->vdesk = r->flag;
            continue;

        case E_quiet:
            r->quiet = r->flag;
            continue;

        case E_convert:
            r->convert = r->flag;
            goto get_img;

        case E_save:
            r->save = r->flag;
            if (r->save)
            {
                if (E_eos==next_token(r)) return false;
                unquote(strcpy(r->bsetroot_bmp, r->token));
            }
            continue;

        case E_prefix:
            if (E_eos==next_token(r)) return false;
            unquote(strcpy(r->search_base, r->token));
            continue;

        case E_path:
            if (E_eos==next_token(r)) return false;
            append_string_node(&r->paths, unquote(r->token));
            continue;
        }
    }
}

