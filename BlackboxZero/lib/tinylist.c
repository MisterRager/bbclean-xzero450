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
/* list functions */

#include "bblib.h"

void *member(void *a0, void *e0)
{
    list_node *l;
    dolist (l, (list_node *)a0) if (l == e0) break;
    return l;
}

void *assoc(void *a0, void *e0)
{
    list_node *l;
    dolist (l, (list_node *)a0) if (l->v == e0) break;
    return l;
}

void *member_ptr(void *a0, void *e0)
{
    list_node *l, **ll;
    for (ll = (list_node**)a0 ;NULL != (l = *ll); ll = &l->next)
        if (l == e0) return ll;
    return NULL;
}

void *assoc_ptr(void *a0, void *e0)
{
    list_node *l, **ll;
    for (ll = (list_node**)a0 ;NULL != (l = *ll); ll = &l->next)
        if (l->v == e0) return ll;
    return NULL;
}

int remove_assoc(void *a, void *e)
{
    list_node *q, **pp = (list_node**)assoc_ptr(a, e);
    if (NULL == pp) return 0;
    q = *pp, *pp = q->next, m_free(q);
    return 1;
}

int remove_node (void *a, void *e)
{
    list_node *q, **pp = (list_node**)member_ptr(a, e);
    if (NULL == pp) return 0;
    q = *pp, *pp = q->next;
    return 1;
}

int remove_item(void *a, void *e)
{
    int r = remove_node(a, e);
    m_free(e);
    return r;
}

void reverse_list (void *d)
{
    list_node *a, *b, *c;
    a = *(list_node**)d;
    for (b=NULL; a; c=a->next, a->next=b, b=a, a=c);
    *(list_node**)d = b;
}

void append_node (void *a0, void *e0)
{
    list_node *l, **pp = (list_node**)a0, *e = (list_node*)e0;
    for ( ;NULL != (l = *pp) ; pp = &l->next);
    *pp = e, e->next=NULL;
}

void cons_node (void *a0, void *e0)
{
    list_node **pp = (list_node**)a0, *e = (list_node*)e0;
    e->next = *pp, *pp = e;
}

void *copy_list (void *l0)
{
    list_node *p = NULL, **pp = &p, *l;
    dolist(l, (list_node*)l0)
        *pp = (list_node*)new_node(l->v), pp = &(*pp)->next;
    return p;
}

void *nth_node (void *v0, int n)
{
    list_node *v = (list_node*)v0;
    while (n && v)  n--, v = v->next;
    return v;
}

void *new_node(void *p)
{
    list_node *n = (list_node*)m_alloc(sizeof *n);
    n->v = p;
    n->next = NULL;
    return (void*)n;
}

int listlen(void *v0)
{
    list_node *v = (list_node*)v0; int i = 0;
    while(v) i++, v=v->next;
    return i;
}

void freeall(void *p)
{
    list_node *s, *q;
    q = *(list_node**)p;
    while (q) q = (s = q)->next, m_free(s);
    *(list_node**)p = q;
}

/* ------------------------------------------------------------------------- */

struct string_node *new_string_node(const char *s)
{
    int l = strlen(s);
    struct string_node *b = (struct string_node *)m_alloc(sizeof *b + l);
    memcpy(b->str, s, l+1);
    b->next = NULL;
    return b;
}

void append_string_node(struct string_node **p, const char *s)
{
    append_node(p, new_string_node(s));
}

/* ------------------------------------------------------------------------- */
