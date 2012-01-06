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
//  EDFUNC.C

#include "edstruct.h"
#include <ctype.h>

void inschr(int o, int n, const char *p);
void delchr(int o, int n);
void ed_char(int cmd);
void ed_goto(void);
void ed_fixup(void);

char  smart = 0;
int   tabs  = 4;
char  tuse  = 1;
char  tcvt  = 0;
char  ltup  = 0;

char upc(char);
char lwc(char);
void domarking(int);
int setmark(void);
int getmark0(struct mark_s *);
int getmark(struct mark_s *);
void allmark(void);
void unmark(void);

void ed_init (int pg, int y, int x);
void ed_insblk_0(char *, char, int);
void ed_insblk(void);
//void ed_delblk(void);
//void ed_movblk(void);
void ed_bcopy(void);
//void ed_bcut(void);
void ed_bpaste(void);
void ed_allmark(void);
void ed_lt(void);
void ed_rt(void);
void ed_clt(void);
void ed_crt(void);
void ed_home(void);
void ed_end(void);
void ed_chome(void);
void ed_cend(void);
void ed_cup(void);
void ed_cdn(void);
void ed_up(void);
void ed_dn(void);
void ed_prior(void);
void ed_next(void);
void ed_cprior(void);
void ed_cnext(void);
void ed_para_up(void);
void ed_para_dn(void);
void ed_ret(void);
void ed_cret(void);
void ed_back(void);
void ed_cback(void);
void ed_sback(void);
void ed_delright(void);
void ed_insright(void);
void ed_btab(void);
void ed_ntab(void);
void ed_goto(void);
void ed_size(void);
void ed_markmode(void);
void ed_unmark(void);
void ed_swapdn(void);
void ed_swapup(void);
void ed_dupline(void);
void ed_char(int);
void ed_rplc(char *);
void ed_insert(char *);
void ed_cmd(int, ...);
void ed_fixup(void);
void ed_mark(int, int);
int  ed_search(struct sea *);
char *ed_make_rplc(char *, char *);

/*----------------------------------------------------------------------------*/
char upc(char c) {
    if (c>='a' && c<='z') return c-32;
    return c;
}

char lwc(char c) {
    if (c>='A' && c<='Z') return c+32;
    return c;
}

/*----------------------------------------------------------------------------*/
// mark helper functions

void domarking(int flg) {
    int x,p;

    if (NULL==edp) return;

    if (0==flg) {
        markf2=markf1;
        return;
    }

    if (markf2) unmark();

    x=curx; p=fpos;

    if (markf1==0 || p!=mark2 || x!=mark2x) upd=1;

    if (markf1==0) markf1=1, mark1=p, mark1x=x;

    mark2=p, mark2x=x;
}

int setmark(void) {
    if (markf1==0)  return 0;

    if (mark1<=mark2) ma=&mark1, me=&mark2;
    else              ma=&mark2, me=&mark1;

    if (mark1x<=mark2x) mxa=&mark1x, mxe=&mark2x;
    else                mxa=&mark2x, mxe=&mark1x;

    if (vmark==0) return (mark1!=mark2);
    return (mark1x!=mark2x);
}

int getmark0(struct mark_s *m) {
    int a,e,f,la,le,ne;

    if (!setmark()) return 0;

    la = fixline(a=*ma);
    le = fixline(e=*me);
    ne = nextline_v(e,1,&f);

    m->y=m->lf=0;
    if (vmark)
    {
        m->a=la;
        m->e=ne;
        m->xa=*mxa;
        m->xe=*mxe;
        m->y=0==f;
    }
    else
    if (linmrk && le>la)
    {
        m->a=la;
        m->e=(me==&mark1) ? ne : le;
        m->xa=m->xe=0;
        m->lf=1;
    }
    else
    {
        m->a=a;
        m->e=e;
        m->xa=a-la;
        m->xe=e-le;
    }
    m->x=m->xe-m->xa;
    m->l=m->e-m->a;
    return 1;
}

int getmark(struct mark_s *m) {
    if (!getmark0(m)) return 0;
    m->y+=cntlf(m->a, m->e);
    return 1;
}


void allmark(void) {
    vmark=0;
    markf1=markf2=1;
    mark1=0;
    mark2=flen;
    mark1x=mark2x=0;
    upd=1;
}

void unmark(void) {
    if (edp==NULL) return;
    if (markf1||markf2) upd=1;
    markf1=markf2=0;
}

/*----------------------------------------------------------------------------*/
// undo / redo

#define UD_INS 1
#define UD_DEL 2
#define UD_POS 3
#define UD_CHG 4

typedef struct undo_s {
    struct undo_s *next;
    int p;
    int l;
    char cmd;
    char buf[1];
} sud;

sud **u_pp;

sud *u_add(int cmd, int p, int l, int b) {
    sud *up;

    if (winflg&1) return NULL;

    if (NULL!=(up=(struct undo_s *)m_alloc(sizeof(sud)+b))) {
        up->next = *u_pp;
        *u_pp    = up;
        up->p    = p;
        up->l    = l;
        up->cmd  = cmd;
    }
    return up;
}

void u_setchg(int c) {
    sud *up; int i;
    for (i=0, up=undo_l; i<2; i++, up=redo_l)
        for (;NULL!=up; up=up->next)
            if (up->cmd==UD_POS)
                up->cmd=UD_CHG;
    chg=c;
}


void u_check(int a,int b) {     // called at exit of 'do-edit-cmd(int cmd)'
    u_add(UD_POS+chg,a,b,0);

}

void u_reset(void) {            // called from 'close-the-file()'
    freelist(&undo_l);
    freelist(&redo_l);
}

void u_ins (int p, int l) {     // called from 'inschr(int pos, int cnt, char *what)'
    u_add(UD_INS,p,l,0);
}

void u_del (int p, int l) {     // called from 'delchr(int pos, int cnt)'
    sud *up=u_add(UD_DEL,p,l,l);
    if (NULL!=up) copyfrom(up->buf, p, l);
}

static char msg_undo[]={'u','n'};
static char msg_redo[]={'r','e'};
static char msg_nour[]="nothing to ..do";

void unredo(sud **u_p1, sud **u_p2, char *m) { // the one and only undo-redo
    sud *up; int p,l,a,e,c;

    if (NULL==(up=*u_p1)) {
        *(short*)(msg_nour+sizeof(msg_nour)-5) = *(short*)m;
        InfoMsg(msg_nour);
        return;
    }

    if ((a=e=up->l)!=fpos) // if not in sight
        goto p1;

    a=up->p;  c=up->cmd-UD_POS;  u_pp=u_p2;

    for (;;) {
        *u_p1=up->next; m_free(up);

        if (NULL==(up=*u_p1) || UD_POS<=up->cmd)  break;

        p=up->p, l=up->l;

        switch (up->cmd) {

        case UD_INS:
            delchr(p, l);
            break;

        case UD_DEL:
            inschr(p, l, up->buf);
            break;
        }
    }
    u_check(e,a);
    chg=c;
p1:
    fpos=a;
    ed_goto();
    unmark();
}

/*----------------------------------------------------------------------------*/
// basic clipboard + convert to/from internal format

int entab(char* q, char *p, int n, int k) {
    unsigned char d,f,g,l;
    int s,t,i;
    d=f=g=l=s=t=i=0;
    for (;;) {
        if (d) goto p4;
        if (n==0) break;
        d=*p++; t++; n--;
        if (d==TABC || (d==32 && g==0)) {
            s++; d=0;
            if (k<2||t%k) continue;
            d=9; s=0;
            goto p5;
        }
        g=1;
p4:
        if (s)  { f=32; s--; goto p3; }
        if (d==10) {
            if (l==0) { f=13; l=1; goto p3; }
            t=g=l=0;
        }
p5:
        f=d; d=0;
p3:
        if (q!=NULL) *q++=f;
        i++;
    }
    return i;
}


int detab(char* p, char *q, int n, int k) {
    int i,r;
    unsigned char c;

    r=i=0;
    for (;n;) {
        c=*q++ ,n--;
        if (c==13) continue;
        if (c==0) break;
        i++;
        if (c==10) i=0;
        if (c==9) {
            c=TABC;
            if (i%k) n++, q--;
            if (k<2) c=32;
        }
        if (NULL!=p) *p++=c;
        r++;
    }
    return r;
}


void ed_retab (int ts1, int ts2) {

    int l,n,pg,o; char *p,*q;

    pg=plin;

    q = (char*)m_alloc(l=flen);

    copyfrom(q, 0, l);

    n=entab(NULL,q,l,ts1);
    entab(p=(char*)m_alloc(n),q,l,ts1);
    m_free(q);

    o=detab(NULL,p,n,ts2);
    detab(q=(char*)m_alloc(o),p,n,ts2);
    m_free(p);

    delchr(0, l); inschr(0,o,q); m_free(q);

    ed_init(pg, cury, curx);
}

/*----------------------------------------------------------------------------*/
int clip_n=-1;
int clip_s;
void *clip_b[3];

void free_clip (int n) {
    void **vp;
    if (NULL!=*(vp=&clip_b[n]))
        clip_s-=strlen((char*)*vp)+1, m_free(*vp), *vp=NULL;
}

void free_all_clip (void) {
    int n;
    n=0; do free_clip(n); while (++n<3);
}

int copytoclip(char *q) {
    char *p;
    int n=0,k;
    HANDLE hMem=NULL;

    k=strlen(q);
    if (k==0) goto end_0;
    n=entab(NULL,q,k,tabs);

    if (-1==clip_n) {

    if (FALSE==OpenClipboard(NULL)) goto end_0;
    if (FALSE==EmptyClipboard())    goto end_1;
    if (NULL==(
           hMem=GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, n+1)
           )) goto end_1;

    if (NULL==(p=(char*)GlobalLock(hMem))) {
        GlobalFree(hMem); goto end_1;
    }

    } else {

    void **vp=&clip_b[clip_n];
    free_clip(clip_n);
    *vp=p=(char*)m_alloc(n+1);
    clip_s+=n+1;

    }

    n=entab(p,q,k,tabs);
    p[n]=0;

    if (-1==clip_n) {
    SetClipboardData(CF_TEXT,hMem);
    GlobalUnlock(hMem);
end_1:
    CloseClipboard();
    }
end_0:
    return n;
}

char* copyfromclip(void) {
    char *p,*q=NULL;
    int n,k;
    HANDLE hMem=NULL;

    if (-1==clip_n) {

    if (FALSE==OpenClipboard(NULL)) goto end_0;
    if (NULL ==(hMem=GetClipboardData(CF_TEXT))) goto end_1;
    if (NULL ==(p=(char*)GlobalLock(hMem))) goto end_1;

    } else {
    if (NULL==(p=(char*)clip_b[clip_n]))
        goto end_0;
    }

    n=strlen(p);
    if (n==0) goto end_2;

    k=detab(NULL,p,n,tabs);
    q=(char*)m_alloc(k+1);
    if (NULL!=q) detab(q,p,n,tabs), q[k]=0;

    if (-1==clip_n) {
end_2:
    GlobalUnlock(hMem);
end_1:
    CloseClipboard();
    }
end_0:
    return q;
}

void ed_setclip (int n) {
    clip_n = n-1;

}

/*----------------------------------------------------------------------------*/
// basic insert / delete


void fixmark(int o1, int o2, int n) {
    if (setmark()) {
        if (*ma>=o1) { *ma+=n; }
        if (*me>=o2) { *me+=n; }
    }}

void inschr(int o, int n, const char *p) {
    int l;

    if (n<=0) return;
    u_ins(o,n);
    insdelmem (o, n);

    if (NULL==p) clearchr(o,' ',n);
    else         copyto(o,p,n);

    tlin+=(l=cntlf(o,o+n));

    if (fpga>o) fpga+=n, plin+=l;
    fixmark(o,o+1,n);
}


void delchr(int a, int n) {
    int l, e;

    if (n<=0) return;
    e = a + n;

    tlin-=(l=cntlf(a, e));
    if (fpga>a) {
        if (fpga>=e) fpga-=n,  plin-=l;
        else         plin-=cntlf(a,fpga),fpga=fixline(a);
    }
    u_del(a, n);
    insdelmem (a, -n);
    upd=1;
    fixmark(e, e, -n);
}

/*----------------------------------------------------------------------------*/
// basic next/prev line

int nextline_v(int o, int n, int *v) {
    int w=0,a;
    for (;n--;) {
        a=o;
        for (;;) {
            if (o==flen) { o=a; goto p1; }
            if (getchr(o++)==10) break;
        }
        w++;
    }
p1:
    if (v!=NULL) *v=w;
    return o;
}

int prevline_v(int o, int n, int *v) {
    int w=0;
    for (;;) {
        for (;;) {
            if (o==0) goto p1;
            if (getchr(--o)==10) break;
        }
        if (n--==0) { o++; break; }
        w++;
    }
p1:
    if (v!=NULL) *v=w;
    return o;
}

int nextline(int o, int n) {
    return nextline_v(o,n,NULL);
}

int prevline(int o, int n) {
    return prevline_v(o,n,NULL);
}

int fixline (int p) {
    return prevline(iminmax(p,0,flen),0);
}

int linelen(int o) {
    int m=o;
    for (;m<flen && getchr(m)!=10;m++);
    return m-o;
}

int movpage(int n) {
    int y=0;
    if (n>0)  fpga=nextline_v(fpga,n,&y),  plin+=y;
    else
    if (n<=0) fpga=prevline_v(fpga,-n,&y), plin-=y;
    if (upd==0) {
        if (y==1)  upd=(n<0)?2:4;
        else
        if (y)     upd=1;
    }
    return y;
}

int delspc(int o) {
    int n,l;
    unsigned char d;
    o+=linelen(o);
    for (n=o;n && ((d=getchr(n-1))==' ' || d==TABC);n--);
    l=o-n;
    if (l) delchr(n, l);
    return l;
}

int inslf(int o) {
    inschr(o,1,"\n");
    o-=delspc(o);
    return o+1;
}

void inschr_tab(int lp, int o, int k, int n, char *q) {
    char *p; int i,c;

    i=k+(c=o-lp);
    i=i-i%tabs-c;
    if (i<0 || smart) i=0;

    p=(char*)m_alloc(n+k);
    memset(p,32,k);
    memset(p,TABC,i);
    memmove(p+k,q,n);
    inschr(o,n+k,p);
    m_free(p);
}

/*----------------------------------------------------------------------------*/
// count lines

int cntlf(int a, int e) {
    int r=0;
    for (;a<e;a++)
        if (getchr(a)==10) r++;
    return r;
}

/*----------------------------------------------------------------------------*/
// dup/swap line helper functions

void swapm(int a, int b, int c) {
    char *tmp;
    int n = c - b;
    tmp=(char*)m_alloc(n);
    copyfrom(tmp,b,n);
    delchr(b, n);
    inschr(a,n,tmp);
    m_free(tmp);
}

int swaplines(int a) {
    int b,c,f;
    b=nextline_v(a,1,&f);
    if (f==0) return f;
    c=nextline_v(b,1,&f);
    //if (f==0) return f;
    if (f==0) swapm(a,a+linelen(a),b), c+=linelen(c);
    swapm(a,b,c);
    return 1;
}

int dupline(int a) {
    int b,f;
    char *tmp;
    b=nextline_v(a,1,&f);
    if (f==0) return f;
    tmp=(char*)m_alloc(f=b-a);
    copyfrom(tmp,a,f);
    inschr(b,f,tmp);
    m_free(tmp);
    return 1;
}


/*----------------------------------------------------------------------------*/
// find keyword for winapi32 help

unsigned char getalnum(int o) {
    unsigned char d;
    return (isalnum(d=getchr(o)) || d=='_') ? d : 0;
}

int getkword(int o, char *p) {
    int m;
    if (getalnum(o)) for (;o && getalnum(o-1);o--);
    m=o;
    for (;o<flen && 0!=(*p=getalnum(o));p++,o++); *p=0;
    return m;
}

/*----------------------------------------------------------------------------*/
// word left/right functions

char wrdsep[48]=";,.:()+*-/%\\";

char tf=0;

int is_blank(int o) {
    unsigned char d;
    return ((d=getchr(o))==' ' || d==TABC);
}

int isspc(int o) {
    return
    is_blank(o) ? 1 : tf ? 0 :
        //0==getalnum(o);
        NULL!=strchr(wrdsep,getchr(o));
}

int wrt(int n,int e) {
    for (;n<e && !isspc(n);n++);
    for (;n<e &&  isspc(n);n++);
    return n;
}

int wlt(int n, int a) {
    for (;n>a &&  isspc(--n););
    for (;n>a;) if (isspc(--n)) {
        n++;
        break;
    }
    return n;
}

/*----------------------------------------------------------------------------*/
// find (smart) tab positions

int nhtab(int x) {
    int k=tabs;
    x+=k;
    return x-x%k;
}

int phtab(int x) {
    int k=tabs;
    x--;
    return x-x%k;
}


int nexttab(int x){
    int y,n,o,p,l;
    if (smart==0) goto hard;
    y=cury; o=lpos; tf=1;
    for (;y--;) {
        o=prevline_v(o,1,&n);
        if (n==0) break;
        l=linelen(o);
        p=imin(l,x);
        n=wrt(o+p,o+l)-o;
        if (n>x && n<l) { tf=0; return n; }
    }
    tf=0;
hard:
    return nhtab(x);
}

int prevtab(int x){
    int y,n,o,p,l,x0;

    o=lpos;
    l=linelen(o);
    x0=imin(l,x);
    for (;x0 && is_blank(o+x0-1); x0--);
    if (x0>0 && x0<x && x0<l) x0++;

    if (smart==0) goto hard;
    y=cury; tf=1;

    for (;y--;) {
        o=prevline_v(o,1,&n);
        if (n==0) break;
        l=linelen(o);
        p=imin(l,x);
        n=wlt(o+p,o)-o;
        if (n<x && n>=x0 && (n>0 || !is_blank(o)))
           { tf=0; return n; }
    }
    tf=0;
hard:
    return imax(x0,phtab(x));
}

/*----------------------------------------------------------------------------*/
char *get_block(void) {
    char *q;
    struct mark_s m;

    if (!getmark(&m)) return NULL;
    if (m.l==0) return NULL;
    q=(char*)m_alloc(m.l+1);
    if (q!=NULL) {
    copyfrom(q,m.a,m.l);
    q[m.l]=0;
    }
    return q;
}

void ins_block(char *q, char lf, int f) {
    int k,o,l,m,p;

    m=lpos;

    if (lf) k=0, o=m;
    else k=imax(0,curx-linelen(m)), o=fpos;

    l=strlen(q);
    inschr_tab(m,o,k,l,q);

    p=o+k+l;
    //if (0==cntlf(m,p)) { curx=p-m; }

    delspc(p);

    if (f==0) return;

    mark1=o+k;
    mark2=p;
    markf1=1;
}

void del_block(void) {
    struct mark_s m;
    if (!getmark(&m)) return;
    delchr(m.a, m.l);
    delspc(m.a);
    unmark();
}

/*----------------------------------------------------------------------------*/
char *get_vblock(void) {
    int x3,x4,o,i,k,l;
    char *p,*q;
    struct mark_s m;

    if (!getmark(&m)) return NULL;

    if (m.y==0 || m.x==0) return NULL;

    i=m.y*(m.x+1);
    q=p=(char*)m_alloc(i+1);

    for (o=m.a;m.y--;o=nextline(o,1)) {
        k=linelen(o);
        x3=imin(k,m.xa);
        x4=imin(k,m.xe);
        l=x4-x3;
        if (l) copyfrom(p,o+x3,l);
        p+=l;
        for (k=m.x-l;k>0;k--) *p++=' ';
        *p++=10;
    }
    *p=0;
    return q;
}

void ins_vblock(char *p) {
    int f,m,i,l,k,x;
    char *q;

    i=curx;
    m=lpos;
    for (;*p;) {
        for (q=p;*q && *q!=10;q++);

        x = q-p;
        l = linelen(m);
        k = i-l;
        if (k<0) k=0, l=i;

        inschr_tab(m,m+l,k,x,p);
        delspc(m);

        //curx = imax(curx,l+k+x);

        m=nextline_v(m,1,&f);
        if (f==0) {
            m=inslf(m+linelen(m));
        }
        if (*q==10) q++;
        p=q;
    }
}

void del_vblock(int *a) {
    int x3,x4,i,k,l,o,z;
    struct mark_s m;

    if (!getmark(&m)) return;

    if (lpos>=m.a && lpos<m.e){
        if (curx >= m.xe)
            curx-=m.x;
        else
        if (curx > m.xa)
            curx=m.xa;
    }
    z=0;
    for (o=m.a; m.y--;o=nextline(o,1)) {
        k=linelen(o);
        x3=imin(k,m.xa);
        x4=imin(k,m.xe);
        l=x4-x3;
        i=o+x3;
        if (0==z) *a=i, z++;
        if (l) {
            delchr(i, l);
            delspc(i);
        }
    }

}

void mark_vblock(int x0, int x,int y) {
    mark1=lpos;
    mark2=nextline(lpos,y-1);
    mark1x=x0;
    mark2x=x0+x;
    markf1=1;
}

int inmark(int x, int y) {  // export for mouse draging
    struct mark_s m;
    int o,p,i;

    if (!getmark0(&m)) return 0;

    o=nextline_v(fpga,y,&i);
    p=o+imin(x,linelen(o));
    if (i<y) p=flen;

    if (p>=m.a && p<m.e) {
        if (vmark==0) return 1;
        if (x>=m.xa && x<m.xe) return 1;
    }
    return 0;
}

/*----------------------------------------------------------------------------*/
// block

void ed_insblk_0(char *q, char lf, int f) {
    if (vmark) ins_vblock(q);
    else       ins_block(q,lf,f);
    m_free(q);
}

void ed_insblk(void) {
    char *q; int c;
    struct mark_s ms;

    if (getmark(&ms)==0) return;

    if (vmark)  q=get_vblock();
    else        q=get_block();

    if (NULL!=q) {
        c=curx;
        ed_insblk_0(q,ms.lf,1);
        if (vmark) {
            mark_vblock(c, ms.x,ms.y);
        }
    }
}

void ed_delblk(int *ap, int f) {
    struct mark_s ms;
    int i,k,a;
    if (getmark(&ms)==0) return;
    if (vmark)
    {
        del_vblock(ap);
     p1:
        unmark();
        return;
    }

    k=ms.l; *ap=a=ms.a;

    if (fpos>ms.a && fpos<nextline(ms.e,1))
    {
        if (curx>=ms.xe) curx-=ms.x;
        else
        if (curx> ms.xa) curx=ms.xa;
    }

    delchr(ms.a, ms.l);
    i=delspc(a), k+=i, a-=i;

    if (f)
    {
        if (fpos>=ms.e) fpos-=k;
        else
        if (fpos> a)    fpos=a;
        lpos=fixline(fpos);
        cury=cntlf(fpga,lpos);
    }
    else
    {
        fpos = a;
        ed_goto();
    }
    goto p1;
}

void ed_movblk(int *a) {
    struct mark_s ms;
    void ed_fixup(void);
    char *q; int c;
    if (getmark(&ms)==0) return;
    if (vmark) {

        c=curx;
        if (lpos==ms.a && c>=ms.xa && c<= ms.xe) return;

        q=get_vblock();
        if (q==NULL) return;
        del_vblock(a);

        if (lpos!=ms.a) curx=c;

        c=curx;

        ed_fixup();
        ins_vblock(q);
        m_free(q);

        mark_vblock(c, ms.x,ms.y);
        return;
    }
    if (fpos>=ms.a && lpos+curx<=ms.e) return;
    q=get_block();
    if (q==NULL) return;
    ed_delblk(a, 1);
    ed_fixup();
    ins_block(q,ms.lf,1);
    m_free(q);
}

/*----------------------------------------------------------------------------*/
void ed_flipcase(void) {
    struct mark_s ms;
    char *q,*p,c,d = 0;

    if (getmark(&ms)==0 || vmark)
        return;

    q=get_block();

    if (q==NULL) return;

    for (p=q; 0!=(c=*p) && (d=lwc(c))==upc(c); p++);

    if (0==c) goto p1;

    d = d==c?0:32;

    for (p=q;0!=(c=upc(*p)); p++) {
        if (lwc(c)!=c) *p=c+d;
    }

    delchr(ms.a, ms.l);
    inschr(ms.a, ms.l, q);
    *ma=ms.a; *me=ms.e;
p1:
    m_free(q);
}



/*----------------------------------------------------------------------------*/
// clipboard

void ed_bcopy(void) {
    char *q;
    struct mark_s m;

    getmark(&m);

    if (vmark)
        q=get_vblock();
    else
        q=get_block();

    if (q==NULL) return;

    copytoclip(q);
    m_free(q);
    unmark();

    if (m.lf) curx=0;
}

void ed_bcut(int *a) {
    ed_bcopy();
    markf1=1;
    ed_delblk(a,0);
}

void ed_bpaste(void) {
    char *q;
    q=copyfromclip();
    if (q==NULL) return;

    if (vmark)
         ins_vblock(q);
    else
         ins_block(q,0,1);

    m_free(q);
}

void ed_allmark(void) {
    allmark();
}

/*----------------------------------------------------------------------------*/
// movement

void ed_lt(void) {
    int i,m,o;
    i=curx;
    if (i==0) {
        if (ltup && 0==vmark && !(winflg&1) && fpos) {
            ed_up();
            ed_fixup();
            ed_end();
        }
        return;
    }
    if (i<=linelen(lpos) && getchr(lpos+i-1)==TABC) {
        o=phtab(m=i);
        for (;--m>o && getchr(lpos+m-1)==TABC;);
        curx=m;
        return;
    }
    curx--;
}

void ed_rt(void) {
    int i,m,o;

    i=curx; m=linelen(lpos);
    if (i>=m && ltup && 0==vmark && !(winflg&1)) {
        ed_home();
        ed_dn();
        return;
    }

    if (i<m && getchr(lpos+i)==TABC) {
        o=nhtab(m=i);
        for (;++m<o && getchr(lpos+m)==TABC;);
        curx=m;
        return;
    }
    curx++;
}

void ed_clt(void) {
    int n;
    if (fpos>lpos) {
        fpos=wlt(fpos,lpos);
        if (fpos>lpos || !isspc(fpos)) {
            curx=fpos-lpos;
            return;
        }
    }
    if (lpos==0) {
        curx=0;
        return;
    }
    cury--;
    lpos=prevline(lpos,1);
    n=linelen(lpos);
    fpos=lpos+n;
    curx=n;
}

void ed_crt(void) {
    int n,l;
    n=linelen(lpos);
    l=lpos+n;
    if (fpos==l) {
        if (l==flen) return;
        cury++;
        curx=0;
        lpos=fpos=nextline(lpos,1);
        n=linelen(lpos);
        if (!isspc(fpos)) return;
    }
    fpos=wrt(fpos,lpos+n);
    curx=fpos-lpos;
}


void ed_home(void) {
    curx=0;
}

void ed_end(void) {
    curx=linelen(lpos);
}

void ed_chome(void) {
    int n;
    n=clft-1;
    if (n<0) return;
    n-=n%4;
    curx=imin(curx, (clft=n) + pgx);
    upd=1;
}

void ed_cend(void) {
    int n;
    n=clft+4; n-=n%4;
    curx=imax(curx, clft=n);
    upd=1;
}

void ed_cup(void) {
    int y=movpage(-1);
    cury=imin(pgy,cury+y);
}

void ed_cdn(void) {
    int y=movpage(1);
    cury=imax(0,cury-y);
}

void ed_up(void) {
    cury--;
}

void fix_list(void) {
    if (winflg&1) {
       cury=imin(cury,tlin-plin-1);
    }
}

void ed_dn(void) {
    if (lpos<flen) cury++;
    fix_list();
}

void ed_prior(void) {
    int i,k;
    prevline_v(fpos,k=pgy+1,&i);
    if (i==0) return;
    if (i==k) movpage(-pgy);
    else fpga=cury=plin=0, upd=1;
}

void ed_next(void) {
    int i;
    nextline_v(fpos,pgy+1,&i);
    if (i==0) return;
    if (i>pgy-cury) movpage(pgy);
    if (i!=pgy+1)   cury=pgy;
    fix_list();
}

void ed_cprior(void) {
    curx=0;
    fpga=cury=plin=0;
    upd=1;
}

void ed_cnext(void) {
    int i;
    curx=0;
    nextline_v(fpga,pgy+1,&i);
    if (i>pgy) fpga=flen,plin=tlin,movpage(-pgy);
    cury=pgy;
}

/*----------------------------------------------------------------------------*/
ST int is_empty_line(int p)
{
    int x, n = linelen(p);
    for (x = 0; x < n; x++)
        if (!is_blank(p+x))
            return 0;
    return 1;
}

ST void para_hlp_1(int f) {
    int o,p,e,i,n;
    o=lpos; e=f?fixline(flen):0;
    for (i=0; i<2; )
    {
        if (e==o) break;
        p = f ? o : prevline(o,1);
        if ((i^f)==(!is_empty_line(p)))
        {
            o=f?nextline(p,1):p;
            continue;
        }
        i++;
    }
    n = linelen(o);
    if (n && is_blank(o))
    {
        tf = 1;
        o=wrt(o, o+n);
        tf = 0;
    }
    fpos=o;
    ed_goto();
}

void ed_para_up(void) {
    para_hlp_1(0);
}

void ed_para_dn(void) {
    para_hlp_1(1);
}

/*----------------------------------------------------------------------------*/
// enter

void ed_ret(void) {
    int i,m,n,l;
    i=curx;
    m=lpos;
    l=linelen(m);
    for (n=0;n<l && is_blank(m+n);n++);
    if (n==l || i<n) n=i;

    i=fpos=inslf(fpos);
    curx=0, cury++;
    if (linelen(i)) {
        inschr_tab(i,i,n,0,NULL);
    }
    curx+=n;
}

void ed_cret(void) {
    inslf(lpos);
}

/*----------------------------------------------------------------------------*/
// backspace etc

void ed_back(void) {
    int i,m,n,k,l,o; char u;
    i=curx;
    m=lpos;
    k=i-linelen(lpos);
    if (i) {
        u=upd;
        o=1;
        if (k<=0 && getchr(lpos+i-1)==TABC) {
            o=phtab(m=i);
            for (;--m>o && getchr(lpos+m-1)==TABC;);
            o=i-m;
        }
        if (k<=0) delchr(fpos-o, o);
        if (k==0) delspc(fpos-o);
        curx-=o;
        if (u==0) upd=8;
        return;
    }
    if (m==0)   return;
    n=prevline(m,1);
    l=linelen(n);
    delchr(n+l, m-(n+l));
    curx=l;
    if (cury==0) fpga=n;
    else cury--;
}

void ed_cback(void) {
    int n;
    n=nextline(lpos,1)-lpos;
    if (n==0) n=linelen(lpos);
    delchr(lpos, n);
}

void ed_sback(void) {
    int m,n,o; unsigned char d,e;
    for (m=lpos;m<fpos;m++)
        if (!is_blank(m)) goto b1;

    o=prevline_v(lpos,1,&n);
    if (n && linelen(o)==0) {
        delchr(o, lpos-o);
        cury--;
        return;
    }
b1:
    m=fpos;
    for (e=0;m && ((d=getchr(m-1))<=32||d==TABC);e=d,m--);

    n=fpos-m;
    if (n==0 || (n==1 && e==32)) return;
    delchr(m, n);

    if (linelen(m)) inschr(m++,1,NULL);
    fpos=m;
    ed_goto();
}

void ed_delright(void) {
    int n,i,m,f;
    n=linelen(lpos);
    i=curx;
    if (i<n) {
       delchr(fpos, 1);
       upd=8;
       return;
    }
    m=nextline_v(lpos,1,&f);
    if (f==0) return;
    if (i>n) {
        inschr(lpos+n,i-n,NULL);
        m+=i-n;
    }
    delchr(lpos+i, m-(lpos+i));
}

void ed_insright(void) {
    int i,n;
    n=linelen(lpos);
    i=curx;
    if (i<n) {
       inschr(lpos+i,1,NULL);
       upd=8;
    }
}

void ed_deleol(void) {
    int n,i,m,f;
    n=linelen(lpos);
    i=curx;

    if (i<n)
    {
       delchr(lpos+i, n-i);
       upd=8;
       return;
    }
    m=nextline_v(lpos,1,&f);
    if (f==0) return;
    if (i>n) {
        inschr(lpos+n,i-n,NULL);
        m+=i-n;
    }

    delchr(lpos+i, m-(lpos+i));
}

/*----------------------------------------------------------------------------*/
// un/indent

int sh_lines (int f) {
    struct mark_s ms; int n,m,l,a,e,a0,s,m0; unsigned char d;

    if (getmark(&ms)==0)            return 0;
    if (ms.a!=fixline(m0=ms.a))     return 1;

    a0=*ma;
    d=smart ? ' ' : TABC;

    for (s=0;s<2;s++)
    for (m=m0,n=0; n<ms.y; n++, m=nextline(m,1)) {

        a=0, e=l=linelen(m);
        if (vmark) {
            a=ms.xa; e=ms.xe;
            if (0==f) {
                if (0==a) return 1;
                a--,e--;
            }
        }
        if (a>=l) continue;

        if (0==f) {
            if (0==s) {
                if (!is_blank(m+a)) return 1;
                continue;
            }
            delchr(m+a,1);
            if (e<l) inschr(m+e,1,(char*)&d);

        } else {

            if (0==s) {
                if (e<l && !is_blank(m+e)) return 1;
                continue;
            }
            if (e<l) delchr(m+e,1);
            inschr(m+a,1,(char*)&d);
        }
    }
    if (vmark) {
        f+=f-1;
        *mxa += f;
        *mxe += f;
    }
    else
    if (f && 0==ms.lf) *ma=a0;
    return 1;
}

/*----------------------------------------------------------------------------*/
// tab
void ed_btab (void) {
    int n,m,k;
    if (sh_lines(0)) return;

    n=lpos+linelen(lpos);
    for (m=fpos;m<n && is_blank(m);m++);
    if (m>fpos) {
        delchr(fpos, m-fpos);
        return;
    }
    n=prevtab(k=curx);
    if (n>=k) return;
    curx=n;
    if (linelen(fpos)==0) return;
    delchr(fpos-(k-n), k-n);
}

void ed_ntab (void) {
    int n,k;
    if (sh_lines(1)) return;

    if (smart==0) {
        ed_char(9);
        return;
    }
    n=nexttab(k=curx);
    if (n<=k) return;
    curx=n;
    if (linelen(fpos)==0) return;
    inschr(fpos,n-k,NULL);
}

/*----------------------------------------------------------------------------*/
// diverse

void ed_goto(void) {
    int m,n,k,o,d,e;

    d=pgy/4;             // +- toleranz
    e=pgy/3;             // new page
    m=imax(0,pgy-2);     // 2, zeile von oben
    n=imin(pgy,2);       // 2. zeile von unten
    lpos=fixline(fpos);

    k=prevline(lpos,n);
    if (k<fpga)  goto p2;

    o=nextline(fpga,m);
    if (o>=lpos) goto p0;

    o=nextline(o,d);
    if (o<lpos)  goto p3;

    fpga=prevline(lpos,m);
    goto p1;

p3: // neue seite
    fpga=prevline(lpos,e);
    goto p1;

p2: // unterhalb
    o=prevline(fpga,d);
    if (o>lpos) goto p3;
    fpga=k;

p1:
    upd = 1;
    plin = cntlf(0,fpga);
p0: // alles klar
    cury = cntlf(fpga,lpos);
    curx = fpos-lpos;
    //clft = 0;
}

void ed_size(void) {
    curx=iminmax(curx,clft,pgx+clft);
    cury=imin(cury,pgy);
}

void ed_markmode(void) {
    vmark=!vmark;
    upd=1;
}

void ed_unmark(void) {
    unmark();
}

void ed_swapdn(void) {
    if (!swaplines(lpos)) return;
    cury++;
}

void ed_swapup(void) {
    int n,m;
    n=prevline_v(lpos,1,&m);
    if (!m) return;
    if (!swaplines(n)) return;
    if (--cury<0) fpga=nextline(n,1);
}

void ed_dupline(void) {
    if (!dupline(lpos)) return;
    cury++;
}

/*----------------------------------------------------------------------------*/
// char

void ed_char(int cmd) {
    int i,m,k,l,o,n; char u;

    if (cmd<32 && cmd!=9 && cmd!=12)
        return;
#if 0
    if (cmd=='}' && linelen(lpos)==0)
        curx=prevtab(curx);
#endif

    i=curx;
    m=lpos;
    k=i-(l=linelen(lpos));
    o=i+1;

    if (cmd==9) { cmd=TABC, o=nhtab(i); }

    if ((cmd!=32 && cmd!=TABC) || k<0) {
        if (k<0) k=0, l=i;
        u=upd;
        n=1;
        if (cmd==TABC) {
            k+=o-i, n=0;
        }

        inschr_tab(m,m+l,k,n,(char *)&cmd);

        if (u==0) upd=8;
    }

    curx=o;
}


/*----------------------------------------------------------------------------*/
// replace

void ed_rplc(char *q) {
    int m,n,i;
    if (setmark()==0) return;
    m=*ma;
    n=*me-m;
    i=strlen(q);
    delchr(m,n);
    inschr(m,i,q);
    ed_mark(m,m+i);
}

void ed_insert(char *e_ptr) {
    int m,n,i,o,u,k,l; char *q;

    n=strlen(q=e_ptr);

    i=curx;
    m=lpos;
    k=i-(l=linelen(lpos));
    o=i+n;

    if (k<0) k=0, l=i;
    u=upd;
    inschr_tab(m,m+l,k,n,q);
    if (u==0 && 0==cntlf(m+l+k,m+l+k+n)) upd=8;

    curx=o;
}

/*----------------------------------------------------------------------------*/
void ed_init (int pg, int y, int x) {
    fpga = nextline (0, plin=pg);
    cury = y;
    curx = x;
    clft = 0;
    upd  = 1;
}

void ed_mark(int a, int b) {
    mark1=fpos=a;
    mark2=b;
    markf1=markf2=1;
    vmark=0;
    upd=1;
}

/*----------------------------------------------------------------------------*/
void ed_cmd (int cmd, ...) {
    int a,b,c,d; void *u; va_list vl;

    if (edp==NULL) return;

    a=fpos;
    u=*(u_pp=&undo_l);

    va_start(vl,cmd);

    switch (cmd) {
    case EK_RETAB:          b = va_arg(vl,int);
                            c = va_arg(vl,int);
                            ed_retab(b,c);
                            break;

    case EK_INIT:           b = va_arg(vl,int);
                            c = va_arg(vl,int);
                            d = va_arg(vl,int);
                            ed_init(b,c,d);
                            break;

    case EK_GOTOLINE:       fpos=nextline(0,va_arg(vl,int)); ed_goto(); break;
    case EK_GOTO:           fpos=va_arg(vl,int);             ed_goto(); break;

    case EK_REPLACE:        ed_rplc     (va_arg(vl,char*));     break;
    case EK_INSERT:         ed_insert   (va_arg(vl,char*));     break;
    case EK_INSBLK:         ed_insblk_0 (va_arg(vl,char*),0,0); break;

    case EK_CHAR:           if (setmark()) ed_delblk(&a, 1);
                            ed_char     (va_arg(vl,int));       break;

    case EK_SIZE:           ed_size();       break;

    case EK_MARK:           b=va_arg(vl,int);
                            c=va_arg(vl,int);
                            ed_mark(b,c);    break;

    case EK_SETVAR:         break;

    case EK_DRAG_COPY:      ed_insblk();     break;
    case EK_DRAG_MOVE:      ed_movblk(&a);   break;


    case KEY_A_DELETE:      ed_swapdn();     break;
    case KEY_A_INSERT:      ed_swapup();     break;


    case KEY_A_RET:         ed_dupline();    break;


    case KEY_LEFT:          ed_lt();         break;
    case KEY_RIGHT:         ed_rt();         break;

    case KEY_C_LEFT:        ed_clt();        break;
    case KEY_C_RIGHT:       ed_crt();        break;

    case KEY_HOME:          ed_home();       break;
    case KEY_END:           ed_end();        break;
    case KEY_C_HOME:        ed_chome();      break;
    case KEY_C_END:         ed_cend();       break;

    case KEY_UP:            ed_up();         break;
    case KEY_DOWN:          ed_dn();         break;
    case KEY_C_UP:          ed_cup();        break;
    case KEY_C_DOWN:        ed_cdn();        break;
    case KEY_A_UP:          ed_para_up();    break;
    case KEY_A_DOWN:        ed_para_dn();    break;

    case KEY_PRIOR:         ed_prior();      break;
    case KEY_NEXT:          ed_next();       break;
    case KEY_C_PRIOR:       ed_cprior();     break;
    case KEY_C_NEXT:        ed_cnext();      break;

    case KEY_RET:           ed_ret();        break;

    case KEY_BACK:          if (setmark()) { ed_delblk(&a, 1); break; }
                            ed_back();       break;

    case KEY_S_BACK:        ed_sback();      break;

    case KEY_C_RET:         ed_cret();       break;
    case KEY_C_BACK:        ed_cback();      break;

    case KEY_TAB:           ed_ntab();       break;
    case KEY_S_TAB:         ed_btab();       break;

    case KEY_C_INSERT:      //ed_insright();   break;
    case KEY_C_C:           ed_bcopy();      break;

    case KEY_S_DELETE:
    case KEY_C_X:           ed_bcut(&a);     break;

    case KEY_INSERT:
    case KEY_S_INSERT:
    case KEY_C_V:           if (setmark()) ed_delblk(&a, 1);
                            ed_bpaste();     break;

                            delrt:  ed_delright();      break;
    case KEY_DELETE:        if (!setmark()) goto delrt;
                            ed_delblk(&a, 0);   break;

    case KEY_C_DELETE:      ed_deleol();     break;

    case KEY_C_A:           ed_allmark();    break;
    case KEY_C_B:           ed_markmode();   break;
    case KEY_C_D:           ed_unmark();     break;
    case KEY_C_U:           ed_flipcase();   break;

    case KEY_C_0:           ed_setclip(0);   break;
    case KEY_C_7:           ed_setclip(1);   break;
    case KEY_C_8:           ed_setclip(2);   break;
    case KEY_C_9:           ed_setclip(3);   break;

    case KEY_C_Z:           unredo(&undo_l, &redo_l, msg_undo); goto u1;
    case KEY_CS_Z:          unredo(&redo_l, &undo_l, msg_redo);
                            u1: ed_fixup(); return;
    }

    ed_fixup();

    if (u!=undo_l) {
        u_check(a,fpos);
        freelist(&redo_l);
        chg=1;
    }
}

void ed_fixup(void) {
    int x,y;
    extern char scroll_lock;

    if (cury<0)   movpage(cury),     cury=0;
    if (cury>pgy) movpage(cury-pgy), cury=pgy;

    lpos=nextline_v(fpga,cury,&y);
    if (cury>y) cury=y;

    //curx=imin(curx,linelen(lpos));

    if (scroll_lock) curx=clft;
    x=imax(0,curx);
    if (x<clft)     clft=x,     upd=1;
    if (x>pgx+clft) clft=x-pgx, upd=1;

    fpos=lpos+imin(linelen(lpos),curx=x);
#if 0
    if (l>x) {
        for (;x && getchr(fpos-1)==TABC && x%tabs; fpos--,x--);
        curx=x;
    }
#endif
}

/*----------------------------------------------------------------------------*/
int checkword(int a, int e) {
    return !((a && getalnum(a-1)) || (e<flen && getalnum(e)));
}

static struct rmres res[16];
static char cres;

int r_getchr(int o) {
    int c=getchr(o);
    if (c==TABC) return ' ';
    return c;
}

int ed_search(struct sea *sea) {
    int m=sea->from; char* q=sea->str; int sf=sea->sf;
    int i,k,n,o,fl;
    char *p,e;
    char bstr[128];
    char cstr[128];
    unsigned char *code;
    static int pmat;

    k=1; i=fl=flen;
    if (sf&2) i=-1,k=-1;        //backwards
    if (sf&4 || m==fl) m+=k;    //continue
    if (m<0  || m>=fl) return 0;
    if (sf&64) goto regsearch;  //regular expr.

    p=strcpy(bstr,q=strcpy(cstr,q));

    if (0==(sf&16))             //ignore case
        strlwr(q), strupr(p);

    for (;m!=i;m+=k)
        if ((e=getchr(m))==q[0]||e==p[0])
            for (o=m+(n=1);;o++,n++) {
                if (q[n]==0) {
                    if ((sf&32) && !checkword(m,o)) break; //words
                    goto s01;
                }
                if (o>=fl || ((e=getchr(o))!=q[n] && e!=p[n])) break;
            }
    return 0;
s01:
    sea->a=m;
    sea->e=o;
    return 1;


regsearch:
    if (sf&4 && k>0 && pmat) m+=pmat-k;
    pmat=0;

    if (m>=fl) return 0;

    for (n=0,code=NULL;;) {
        o=rcomp((unsigned char*)q,code,n,(sf&16)==0); //ignore case
        if (o==0) return -1;
        if (n) break;
        code=(unsigned char*)m_alloc(n=o);
    }
    cres=code[0];
    for (;m!=i;m+=k) {
        for (o=0;;o=n,m--) {
            n=rmatch(m,0,fl,code,res,r_getchr);
            if (n<=o || k>0 || m==0) break;
        }
        if (o>n) n=o, m++;
        if (n==0) continue;
        o=m+n;
        if (sf&32 && !checkword(m,o)) continue; //words
        pmat=n;
        break;
    }
    m_free(code);
    if (pmat) goto s01;
    return 0;
}

char *ed_make_rplc(char *dst, char *src) {
    int i, p, w;
    unsigned char c, d, u;
    char *t = dst;
    do {
        c=*src++;
        if (c=='%') {

            u=0; d=*src++;
            if (d=='l') u=1, d=*src++;
            else
            if (d=='u') u=2, d=*src++;

            if ((i=d-'1')>=0 && i<cres) {
               p=res[i].p; w=res[i].w;
               for (;w--;) {
                    c=r_getchr(p++);
                    if (u==1) c=lwc(c);
                    else
                    if (u==2) c=upc(c);
                    *t++=c;
                }
               continue;
            }
            if (d!='%') src-=u?2:1;
        }
        else
        if (c=='\\') {
            d=*src++;
            if (d=='t') { c=TABC; if ((t-dst+1)%tabs) src-=2; }
            else
            if (d=='n') c=10;
            else c=d;
        }
        *t++=c;
    } while (c);
    return dst;
}

/*----------------------------------------------------------------------------*/
