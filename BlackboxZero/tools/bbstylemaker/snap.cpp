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

/* SNAP WINDOWS CODE */

int g_snap_dist = 7;
int g_snap_padding = 2;
int g_snap_usegrid = 1;
int g_snap_gridsize = 4;

// Structures
struct edges {
    int from1, from2;
    int to1, to2;
    int dmin, omin;
    int d, o, def;
    };

struct snap_info {
    struct edges *h;
    struct edges *v;
    bool sizing;
    bool same_level;
    int pad;
    void *self;
    void *parent;
};


// Local fuctions
void snap_to_grid(struct edges *h, struct edges *v, bool sizing, int grid, int pad);
void snap_to_edge(struct edges *h, struct edges *v, bool sizing, bool same_level, int pad);

typedef int button_enum_proc_typ(struct button *bp, void *param);

int snap_enum_proc(struct button *bp, void *param)
{
    struct snap_info *si = (struct snap_info *)param;
    if (bp != si->self && 0 == (bp->f & BN_HID))// && bp->typ == BN_BTN)
    {
        int x, y, w, h;
        x = bp->x0;
        y = bp->y0;
        w = bp->w0;
        h = bp->h0;
        if (false == si->same_level) {
            x += si->h->from1;
            y += si->v->from1;
        }
        si->h->to1 = x;
        si->h->to2 = x+w;
        si->v->to1 = y;
        si->v->to2 = y+h;
        snap_to_edge(si->h, si->v, si->sizing, si->same_level, si->pad);
    }
    return TRUE;
}   

void enum_buttons(struct dlg *dlg, button_enum_proc_typ *fn, void *param)
{
    struct button *bp;
    for (bp = dlg->bn_ptr; bp; bp = bp->next) {
        if (0 == fn(bp, param))
            break;
    }
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//snap_windows
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void snap_button(struct dlg *dlg, struct button *bp, bool sizing, int *content)
{
    struct edges h;
    struct edges v;
    void *parent = NULL;

    int snapdist = g_snap_dist;
    int grid     = g_snap_usegrid ? g_snap_gridsize : 0;
    int padding  = g_snap_padding;

    if (snapdist < 1)
        return;

    h.dmin = v.dmin = h.def = v.def = snapdist;
    h.from2 = (h.from1 = bp->x0) + bp->w0;
    v.from2 = (v.from1 = bp->y0) + bp->h0;

    // ------------------------------------------------------
    // snap to grid
    if (grid > 1)// && (parent || sizing))
    {
        snap_to_grid(&h, &v, sizing, grid, padding);
    }
    else
    {
        struct snap_info si;

        si.h = &h;
        si.v = &v;
        si.sizing = sizing;
        si.same_level = true;
        si.pad = padding;
        si.self = bp;
        si.parent = parent;

        enum_buttons(dlg, snap_enum_proc, &si);

        h.to1 = 0;
        h.to2 = dlg->w;
        v.to1 = 0;
        v.to2 = dlg->h;
        snap_to_edge(&h, &v, sizing, false, padding);

        // -----------------------------------------
        if (sizing)
        {
            // snap to button icons
            if (content)
            {
                // images have to be double padded, since they are centered
                h.to2 = (h.to1 = h.from1) + content[0];
                v.to2 = (v.to1 = v.from1) + content[1];
                snap_to_edge(&h, &v, sizing, false, -2*padding);
            }

        /*
            // snap frame to childs
            si.same_level = false;
            si.pad = -padding;
            si.self = NULL;
            si.parent = self;
            enum_buttons(dlg, snap_enum_proc, &si);
        */
        }
    }

    // -----------------------------------------
    // adjust the window-pos

    if (h.dmin < snapdist) {
        if (sizing)
            bp->w0 += h.omin;
        else
            bp->x0 += h.omin;
    }
    if (v.dmin < snapdist) {
        if (sizing)
            bp->h0 += v.omin;
        else
            bp->y0 += v.omin;
    }
}

//*****************************************************************************

void snap_to_grid(struct edges *h, struct edges *v, bool sizing, int grid, int pad)
{
    struct edges *g; int o, d;
    for (g = h;;g = v)
    {
        if (sizing)
            o = g->from2 - g->from1 + pad; // relative to topleft
        else
            o = g->from1 - pad; // absolute coords

        o = o % grid;
        if (o < 0) o += grid;

        if (o >= grid / 2)
            d = o = grid-o;
        else
            d = o, o = -o;

        if (d < g->dmin) g->dmin = d, g->omin = o;

        if (g == v) break;
    }
}

//*****************************************************************************
void snap_to_edge(struct edges *h, struct edges *v, bool sizing, bool same_level, int pad)
{
    int o, d, n; struct edges *t;
    h->d = h->def; v->d = v->def;
    for (n = 2;;) // v- and h-edge
    {
        // see if there is any common edge, i.e if the lower top is above the upper bottom.
        if ((v->to2 < v->from2 ? v->to2 : v->from2) > (v->to1 > v->from1 ? v->to1 : v->from1))
        {
            if (same_level) // child to child
            {
                //snap to the opposite edge, with some padding between
                bool f = false;

                d = o = (h->to2 + pad) - h->from1;  // left edge
                if (d < 0) d = -d;
                if (d <= h->d)
                {
                    if (false == sizing)
                        if (d < h->d) h->d = d, h->o = o;
                    if (d < h->def) f = true;
                }

                d = o = h->to1 - (h->from2 + pad); // right edge
                if (d < 0) d = -d;
                if (d <= h->d)
                {
                    if (d < h->d) h->d = d, h->o = o;
                    if (d < h->def) f = true;
                }

                if (f)
                {
                    // if it's near, snap to the corner
                    if (false == sizing)
                    {
                        d = o = v->to1 - v->from1;  // top corner
                        if (d < 0) d = -d;
                        if (d < v->d) v->d = d, v->o = o;
                    }
                    d = o = v->to2 - v->from2;  // bottom corner
                    if (d < 0) d = -d;
                    if (d < v->d) v->d = d, v->o = o;
                }
            }
            else // child to frame
            {
                //snap to the same edge, with some bevel between
                if (false == sizing)
                {
                    d = o = h->to1 - (h->from1 - pad); // left edge
                    if (d < 0) d = -d;
                    if (d < h->d) h->d = d, h->o = o;
                }
                d = o = h->to2 - (h->from2 + pad); // right edge
                if (d < 0) d = -d;
                if (d < h->d) h->d = d, h->o = o;
            }
        }
        if (0 == --n) break;
        t = h; h = v, v = t;
    }

    if (false == sizing && false == same_level)
    {
        // snap to center
        for (n = 2;;) // v- and h-edge
        {
            if (v->d < v->dmin)
            {
                d = o = (h->to1 + h->to2)/2 - (h->from1 + h->from2)/2;
                if (d < 0) d = -d;
                if (d < h->d) h->d = d, h->o = o;
            }
            if (0 == --n) break;
            t = h; h = v, v = t;
        }
    }

    if (h->d < h->dmin) h->dmin = h->d, h->omin = h->o;
    if (v->d < v->dmin) v->dmin = v->d, v->omin = v->o;
}

//*****************************************************************************
