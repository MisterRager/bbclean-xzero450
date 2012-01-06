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

int bsetroot_parse(NStyleStruct *pss, const char *command)
{
    char token[MAX_PATH];
    const char *cptr = command;

    struct rootinfo *r = &pss->rootInfo;
    struct NStyleItem *pSI = &pss->rootStyle;
    int f;

    memset(r, 0, sizeof *r);
    memset(pSI, 0, sizeof *pSI);

    strlwr(NextToken(token, &cptr, NULL));
    //f = strstr(token, "bsetroot") || strstr(token, "bsetbg");

    init_root(r);
    f = parse_root(r, cptr) && (r->gradient | r->solid | r->mod);

    pSI->nVersion       = STYLEITEM_VERSION;
    pSI->type           = r->type;
    pSI->Color          = r->color1;
    pSI->ColorTo        = r->color2;
    pSI->interlaced     = r->interlaced;
    pSI->bevelstyle     = r->bevelstyle;
    pSI->bevelposition  = r->bevelposition;
    pSI->TextColor      = r->modfg;

    if (0 == f)
        pSI->parentRelative = true;

    return 1;
}

void make_bsetroot_string(NStyleStruct *pss, char *out, int all)
{
    COLORREF c1, c2;
    int t, x, i, bp, bs;
    char b1[40], b2[40];
    extern int style_version;

    struct rootinfo *r = &pss->rootInfo;
    struct NStyleItem *pSI = &pss->rootStyle;

    memset(out, 0, sizeof pss->rootCommand);

    r->type           = pSI->type           ;
    r->color1         = pSI->Color          ;
    r->color2         = pSI->ColorTo        ;
    r->interlaced     = pSI->interlaced     ;
    r->bevelstyle     = pSI->bevelstyle     ;
    r->bevelposition  = pSI->bevelposition  ;
    r->modfg          = pSI->TextColor      ;
    r->bmp            = 0 != r->wpfile[0];

    c1 = r->color1;
    c2 = r->color2;
    t =  r->type;
    i =  r->interlaced;
    bp = r->bevelposition;
    bs = r->bevelstyle;
    if (pSI->parentRelative)
        t = -1;

    x = 0;

    if (t == -1 && (0 == all || 0 == r->bmp)) {
        out[x] = 0;
        if (all) return;
    }

    x += sprintf(out+x, "bsetroot");
    if (B_SOLID == t && (false == i || write_070)) {
        if (r->mod) {
            x += sprintf(out+x, " -mod %d %d -bg %s -fg %s",
                r->modx,
                r->mody,
                rgb_string(b1, c1),
                rgb_string(b2, pSI->TextColor)
                );
        } else if (i) {
            if (style_version < 4)
                x += sprintf(out+x, " -solid interlaced %s",
                    rgb_string(b1, c1)
                    );
            else
                x += sprintf(out+x, " -solid interlaced -bg %s -fg %s",
                    rgb_string(b1, c1),
                    rgb_string(b2, c2)
                    );
        } else {
            x += sprintf(out+x, " -solid %s", rgb_string(b1, c1));
        }

    } else if (t >= 0) {

        if (B_SOLID == t)
            c2 = c1, t = B_HORIZONTAL;

        x += sprintf(out+x, " -gradient %s%s%s%sgradient -from %s -to %s",
            i ? "interlaced":"",
            bs ? get_styleprop(2)[bs].key : "",
            bs && bp > BEVEL1 ? "bevel2" : "",
            get_styleprop(1)[1+get_styleprop(1)[1+t].val].key,
            rgb_string(b1, c1),
            rgb_string(b2, c2)
            );

        if (r->mod)
            x += sprintf(out+x, " -mod %d %d -fg %s",
                r->modx,
                r->mody,
                rgb_string(b1, pSI->TextColor)
                );
    }

    if (all && r->bmp) {
        t = r->wpstyle;

        if (WP_NONE != t)
            x += sprintf(out+x, " %s", get_root_switch(E_tile + 2*(t-WP_TILE)));

        if (strchr(r->wpfile, ' '))
            x += sprintf(out+x, " \"%s\"", r->wpfile);
        else
            x += sprintf(out+x, " %s", r->wpfile);

        if (r->scale && r->scale != 100)
            x += sprintf(out+x, " -scale %d%%", r->scale);
        if (r->sat < 255)
            x += sprintf(out+x, " -sat %d", r->sat);
        if (r->hue > 0)
            sprintf(out+x, " -hue %d", r->hue);
    }

    //dbg_printf("%s", out);
}

//===========================================================================
