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
/* match3.c - regular expression search */

#ifdef _MSC_VER
#pragma warning(disable: 4244) // convert int to short
#endif

int rcomp (unsigned char *in, unsigned char *out, int omax, int cf);
struct rmres { int p; int w; };
int rmatch(int s, int a, int e, unsigned char *m, struct rmres *m_ptr, int (*getchr)(int));

enum codes {
    e_succ     = 1  ,
    e_char     = 2  ,
    e_nocase   = 3  ,
    e_class    = 4  ,
    e_dot      = 5  ,
    e_nospc    = 6  ,
    e_bol      = 7  ,
    e_eol      = 8  ,
    e_back     = 9  ,
    e_que      = 10 ,
    e_jmp      = 11 ,
    e_grp_a    = 12 ,
    e_grp_b    = 13
};

/*---------------------------------------------------------------------------*/
#include <string.h>

#define ST static

#define ECP '/'
#define EOS 0
#define CLSZ 32

ST unsigned char *cp,*outp,*outm;
ST unsigned char cflg,inclass,m_grp;

enum {

    p_char  = CLSZ*8-1,

    n_class = 256,
    n_dot,
    n_nospc,
    n_grp,
    n_paren,

    n_eol,
    n_bol,

    n_que,
    n_star,
    n_pls,

    n_or,
    n_eos,
    p_paren,
    p_grp,
    p_class,

    c_inv,
    c_range,

    n_fail = 512
};

ST short nextchar(void) {
    short c;
p0:
    if (EOS!=(c=*cp)) cp++;
    if (inclass) switch (c) {
    case ECP: goto ecp;
    case EOS: return n_eos;
    case ']': return p_class;
    case '^': return c_inv;
    case '-': return c_range;
    default:  return c;
    }
    switch (c) {
    case ECP:
ecp:
    if (EOS!=(c=*cp)) cp++;
    switch (c) {
    case 'n': return 10;
    case 'r': return 13;
    case 't': return 9;
    case 'p': return 12;
    case 'i': cflg=1; goto p0;
    case 'c': cflg=0; goto p0;
    case EOS: return n_eos;
    default:  return c;
    }
    case '[': return n_class;
    case '.': return n_dot;
    case '!': return n_nospc;
    case '{': return n_grp;
    case '(': return n_paren;
    case '$': return n_eol;
    case '^': return n_bol;

    case '?': return n_que;
    case '*': return n_star;
    case '+': return n_pls;

    case '|': return n_or;
    case EOS: return n_eos;
    case ')': return p_paren;
    case '}': return p_grp;
    case ']': return p_class;
    default:  return c;
    }}


ST void storecode(short code) {
    if (outp<outm) *outp=(unsigned char)code;
    outp++;
}

ST void storejmp(unsigned char *qp, short code, short dist) {
    if (qp+3<=outm)
        qp[0]=(unsigned char)code, *(short*)&qp[1]=dist;
}

ST short nextcomp(unsigned char code) {
    storecode(code);
    return nextchar();
}

ST void comp_que (unsigned char *qp) {
    short l=outp-qp;
    if (outp+3<=outm) memmove(qp+3,qp,l);
    storejmp(qp,e_que,l);
    outp+=3;
}

ST void comp_pls (unsigned char *qp) {
    outp+=3;
    storejmp(outp-3,e_back,qp-outp);
}

unsigned char upc_ger(unsigned char c) {
    if (c>='a' && c<='z') return c-32;
    if (c<128) return c;
    if (c==0xE4) c=0xC4;
    if (c==0xF6) c=0xD6;
    if (c==0xFC) c=0xDC;
    return c;
}

unsigned char lwc_ger(unsigned char c) {
    if (c>='A' && c<='Z') return c+32;
    if (c<128) return c;
    if (c==0xC4) c=0xE4;
    if (c==0xD6) c=0xF6;
    if (c==0xDC) c=0xFC;
    return c;
}

ST int ignore(short *s) {
    unsigned char d;
    if (cflg==0) return 0;
    *s=d=lwc_ger((unsigned char)*s);
    return (d!=upc_ger(d));
}


ST short comp_class(void){
    short n,i,a,b,c,d;
    unsigned char buf[CLSZ];

    inclass=1;

    c=nextcomp(e_class);
    n=0;

    if (c==c_inv)
        n=-1,c=nextchar();

    if (c==p_class)
        c = ']';

    for (i=CLSZ;i--;) buf[i]=0xFF;
    do {
        if (c>p_char) return n_fail;
        a=b=c;
        c=nextchar();
        if (c==c_range) {
            b=nextchar();
            if (b>p_char) return n_fail;
            c=nextchar();
        }
        for (;a<=b;a++) {
            d=a; i=ignore(&d);
            for (;;) {
                buf[(d>>3) & (CLSZ-1)] &= ~(1<<(d&7));
                if (i==0) break;
                i=0, d=upc_ger((unsigned char)d);
            }
        }

    } while (c!=p_class);

    if (n) *(short*)&buf[0]&=~0x2401; //0,CR,LF
    for (i=0;i<CLSZ;i++) storecode(buf[i]^n);
    inclass=0;
    return nextchar();
}


ST short comp_expr(void);

ST short comp_seq(short c) {
    unsigned char *qp;

    if (c>=n_que) return n_fail;

    for (;;) {
        qp=outp;
        switch (c) {
        case n_bol:
            c=nextcomp(e_bol);
            break;

        case n_eol:
            c=nextcomp(e_eol);
            break;

        case n_dot:
            c=nextcomp(e_dot);
            break;

        case n_nospc:
            c=nextcomp(e_nospc);
            break;

        case n_grp:
            storecode(e_grp_a);
            storecode(m_grp++);
            if (comp_expr()!=p_grp)   return n_fail;
            c=nextcomp(e_grp_b);
            break;

        case n_paren:
            if (comp_expr()!=p_paren) return n_fail;
            c=nextchar();
            break;

        case n_class:
            c=comp_class();
            break;

        default:
            storecode(ignore(&c)?e_nocase:e_char);
            c=nextcomp((unsigned char)c);
            break;
        }
        switch (c) {
        case n_star:
            comp_que(qp);
            if (qp<outm) *qp=e_jmp;
            comp_pls(qp+3);
            c=nextchar();
            break;
        case n_que:
            comp_que(qp);
            c=nextchar();
            break;
        case n_pls:
            comp_pls(qp);
            c=nextchar();
            break;
        }
        if (c>=n_que)
            return c;
    }
}


ST short comp_expr(void) {
    short c;
    unsigned char *qp;

    qp=outp;
    c=comp_seq(nextchar());
    if (c!=n_or) return c;

    comp_pls(qp);
    comp_que(qp);
    qp=outp;

    c=comp_expr();

    storejmp(qp-3,e_jmp,outp-qp);
    return c;
}


/*---------------------------------------------------------------------------*/
int rcomp (unsigned char *in, unsigned char *out, int omax, int cf) {

    short c;

    inclass=m_grp=cflg=0;
    if (cf) cflg=1;

    outp=out;
    outm=out+omax;
    cp=in;

    storecode(0);

    c=comp_expr();

    if (omax) *out=m_grp;
    storecode(e_succ);

    if (c!=n_eos)   return 0;
    return outp-out;
}


/*---------------------------------------------------------------------------*/
#define SPSIZ 400
#define GPSIZ 16

int rmatch(int s, int a, int e, unsigned char *m, struct rmres *m_ptr, int (*getchr)(int)) {

    unsigned char c,*m1;
    int m_max, m_min;
    short i,m_grp;

    struct _g {
        unsigned char g; int a,e;
    }
    *g,*g0,*g1,gs[GPSIZ];

    struct {
        int s; void *m,*g;
    }
    *sp,stack[SPSIZ];

    m_max=m_min=s;

    m_grp=*m++;
    sp=stack;
    g0=g=gs;

    for (;;)
        switch (*m++) {

        case e_char:
        if (s>=e)   goto e_fail;
        c=getchr(s);
        if (*m!=c) goto e_fail;
        m++,s++;
        continue;

        case e_nocase:
        if (s>=e) goto e_fail;
        c=getchr(s);
        if (*m!=lwc_ger(c)) goto e_fail;
        m++,s++;
        continue;

        case e_class:
        if (s>=e) goto e_fail;
        c=getchr(s);
#if CLSZ<32
        if (c>p_char) goto e_fail;
#endif
        if (m[(c>>3)&(CLSZ-1)] & (1<<(c&7))) goto e_fail;
        m+=CLSZ,s++;
        continue;

        case e_dot:
        if (s>=e) goto e_fail;
        c=getchr(s);
        if (c==10 || c==13) goto e_fail;
        s++;
        continue;

        case e_nospc:
        if (s>=e) goto e_fail;
        c=getchr(s);
        if (c==' '|| c==9) goto e_fail;
        if (c==10 || c==13) goto e_fail;
        s++;
        continue;

        case e_bol:
        if (s==a) continue;
        c=getchr(s-1);
        if (c==10 || c==13) continue;
        goto e_fail;

        case e_eol:
        if (s==e) continue;
        c=getchr(s);
        if (c==10 || c==13) continue;
        goto e_fail;


        case e_back:
        i=*(short*)m; m+=2; m1=m+i;
    j1:
        if (sp==stack+SPSIZ) continue;
        sp->g=g;
        sp->m=m;
        sp->s=s;
        sp++;
        m=m1;
        continue;

        case e_que:
        i=*(short*)m; m+=2; m1=m; m+=i;
        goto j1;

        case e_jmp:
        i=*(short*)m; m+=2;
        m+=i;
        continue;

        case e_grp_a:
        if (g==gs+GPSIZ) g--;
        g->g=*m++;
        g->a=s;
        g++;
        continue;

        case e_grp_b:
        g[-1].e=s;
        continue;

        case e_succ:
        if (s<=m_max) goto e_fail;
        m_max=s;

        for (i=0;i<m_grp;i++)
            m_ptr[i].w=0,
            m_ptr[i].p=0;

        for (g1=g0;g1<g;g1++)
            i=g1->g,
            m_ptr[i].p=g1->a,
            m_ptr[i].w=g1->e-g1->a;

e_fail:
        if (sp>stack) {
            sp--;
            s=sp->s;
            m=(unsigned char *)sp->m;
            g=(struct _g *)sp->g;
            continue;
        }

        return m_max-m_min;
    }
}

/*---------------------------------------------------------------------------*/
#ifdef match_main

char buff[5000];
char line[256];

#include <stdio.h>

int main(int argc,char *argv[]) {

    FILE *fp;

    char *s;
    int l,n;

    for (;;) {
        printf(">>");
        s=gets(line);

        if (s==NULL || s[0]==0) return 0;
        l=rcomp(s,buff,1000,0);

        if (l==0) return 2;

        fp=stdout;

        //  fp=fopen("tmp.blk","wt");
        if (fp==NULL) return 3;

        s=buff;
        for (n=0;l--;) {
            fprintf(fp,"%02X",*s++ & 255);
            if ((++n&15)==0) fprintf(fp,"\n");
            else fprintf(fp," ");
        }
        fprintf(fp,"\n");

        //  fclose(fp);
    }

    //  return 0;
}


/*---------------------------------------------------------------------------*/

#endif

