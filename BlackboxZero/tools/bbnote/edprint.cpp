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
// EDPRINT.C - put text on screen with syntax colorizing

#include "edstruct.h"
#include <ctype.h>

struct lang {
    const char **ext;
    const char **prae;
    const char **keys;
    const char **cmt;
    const char *str;
    const char *cclass;
    char flg;
};

#define NOCASE 1
#define CMT_LINESTART 2
#define HEX_NUM 4

#define SYHL

#ifdef SYHL

const char *c_prae[]={
    "#","if","else","elseif","endif","define","include",
    "ifdef","ifndef","undef",
    NULL
};

const char *c_words[]={
    "auto","break","case","char","const","continue",
    "default","do","double","else","enum","extern","float",
    "for","goto","if","int","long","register","return","short",
    "signed","sizeof","static","struct","switch","typedef","union",
    "unsigned","void","volatile","while",
    "class","virtual","public","private","protected","friend",

    //"cdecl","far","fortran","huge","interrupt","near","pascal",
    NULL
};

const char *c_cmt[] = { "/* */","//", NULL } ;
const char  c_str[] = "\"\\\'\\";

const char *pas_words[]={
    "char","string","integer","double","real","readln","writeln",
    "absolute","and","array","begin","case","const","default","div",
    "do","downto","else","end","external","file","for","forward",
    "function","goto","if","implementation","implements","in",
    "initialization","inline","interface","is","label","mod","nil",
    "not","object","of","on","or","out","packed","pascal","procedure",
    "program","read","record","repeat","set","shl","shr","string",
    "then","to","type","unit","until","uses","var","while","with",
    "write","xor",
    NULL
};
const char *pas_cmt[] = { "(* *)","{ }", NULL } ;
const char  pas_str[] = "\'\0";

const char *lsp_words[]={
    "de","df","dm","setq","push","pop","and","or","if","cond","case",
    "let","closure","do","dolist","dotimes","prog1","progn","prog",
    "goto","return","catch","throw","while","when","unless",
    NULL
};

const char *lsp_cmt[]   = { ";", NULL } ;
const char  lsp_str[]   = "\"\\";
const char  lsp_class[] = "[^ ().,;@'`#\"]";

const char *asm_words[]={
    "al","bl","cl","dl","ah","bh","ch","dh",
    "ax","bx","cx","dx","sp","bp","si","di",
    "eax","ebx","ecx","edx","esp","ebp","esi","edi",
    "es","cs","ss","ds","fs","gs",
    NULL
};

const char *asm_cmt[]   = { ";", NULL } ;
const char  asm_str[]   = "\"\\";
const char  asm_class[] = "[a-z0-9_@$]";

const char *htm_words[]={
    "html","meta","head","body","title","style","script","table","tr",
    "td","tbody","frame","iframe","noframe","span","input","textarea",
    "form","br","p","a","b","h1","h2","h3","h4","ul","il","li",
    NULL
    };

const char *htm_cmt[]   = { "<!-- -->", NULL } ;
const char  htm_str[]   = "\"\\";

const char *bas_cmt[]   = { "'", NULL } ;
const char  bas_str[]   = "\"\\";
const char *bas_words[]={
"AND","BEGIN","CASE","CALL","CONTINUE","DO","EACH","ELSE","ELSEIF","END",
"ERASE","ERROR","EVENT","EXIT","FALSE","FOR","FUNCTION","GET","GOSUB",
"GOTO","IF","IMPLEMENT","IN","LOAD","LOOP","LSET","ME","MID","NEW","NEXT",
"NOT","NOTHING","ON","OR","PROPERTY","RAISEEVENT","REM","RESUME","RETURN",
"RSET","SELECT","SET","STOP","SUB","THEN","TO","TRUE","UNLOAD","UNTIL",
"WEND","WHILE","WITH","WITHEVENTS","ATTRIBUTE","ALIAS","AS","BOOLEAN",
"BYREF","BYTE","BYVAL","CONST","COMPARE","CURRENCY","DATE","DECLARE",
"DIM","DOUBLE","ENUM","EXPLICIT","FRIEND","GLOBAL","INTEGER","LET","LIB",
"LONG","MODULE","OBJECT","OPTION","OPTIONAL","PRESERVE","PRIVATE",
"PROPERTY","PUBLIC","REDIM","SINGLE","STATIC","STRING","TYPE","VARIANT",
NULL
};

const char *no_words[] = { NULL };
const char *no_cmt[]  = { NULL } ;
const char  no_str[]  = "";


//-----------------------------------------------------
const char *style_words[]={
    // blackbox style
    "flat","raised","sunken","bevel1","bevel2",
    "horizontal","vertical","diagonal","crossdiagonal","pipecross",
    "elliptic","rectangle","pyramid","solid","interlaced",
    "parentrelative","gradient","border",
    "light","normal","bold","heavy","center","left","right",
    "triangle","square","diamond","circle","true","false","default",

    // blackbox menu
    "begin","end","submenu","include","nop","sep","path",
    "insertpath","stylesmenu","stylesdir","config","workspaces",
    "tasks","icons","style","toggleplugins","aboutplugins",
    "aboutstyle","reconfig","restart","exit","gather",
    "edit","editstyle","editmenu","editlugins","editextensions",
    "editblackbox","logoff","suspend","reboot","shutdown",
    "lockworkstation","hibernate","exitwindows","exec","run",
    NULL
    };



const char *style_cmt[]   = { "!", "#", NULL };
const char  style_str[]   = "\"\\";


//-----------------------------------------------------
const char *lua_words[]={
    "and","break","do","else","elseif","end","false","for","function",
    "if","in","local","nil","not","or","repeat","return","then","true",
    "until","while",
    NULL
};

const char *lua_cmt[]   = { "--", "[[ ]]", "[..[ ],,]", NULL } ;

//-----------------------------------------------------
const char *tcl_cmt[]   = { "#", NULL };

//-----------------------------------------------------
const char *c_ext[]   = { "c", "cc", "cpp", "h", "hh", "hpp", //"rc",
                    "md", "l", "y", "js", "cxx", "py", "php", NULL };

const char *pas_ext[] = { "pas", "p", "dpr" , "pp", NULL };
const char *lsp_ext[] = { "lsp", NULL };
const char *asm_ext[] = { "asm", "a", "s", NULL };
const char *htm_ext[] = { "htm", "html", "shtml", "css", NULL };
const char *tcl_ext[] = { "tcl", "pl", "pls", NULL };
const char *bas_ext[] = { "bas","frm","cls","vbs","ctl", NULL};
const char *style_ext[] = { "", "rc", "style", NULL };
const char *lua_ext[] = { "lua", NULL };

struct lang   c_lang = { c_ext  , c_prae, c_words,   c_cmt,   c_str   , NULL,       0};
struct lang pas_lang = { pas_ext, NULL,   pas_words, pas_cmt, pas_str , NULL,       0};
struct lang lsp_lang = { lsp_ext, NULL,   lsp_words, lsp_cmt, lsp_str , lsp_class,  0};
struct lang asm_lang = { asm_ext, NULL,   asm_words, asm_cmt, asm_str , asm_class,  0};
struct lang htm_lang = { htm_ext, NULL,   htm_words, htm_cmt, htm_str , NULL,       NOCASE };
struct lang bas_lang = { bas_ext, NULL,   bas_words, bas_cmt, bas_str , NULL,       NOCASE };
struct lang style_lang = { style_ext, NULL, style_words, style_cmt, style_str , NULL, NOCASE|CMT_LINESTART|HEX_NUM };
struct lang  tcl_lang = { tcl_ext, NULL,  c_words,   tcl_cmt,  c_str  , NULL,       0};
struct lang lua_lang = { lua_ext, NULL,   lua_words, lua_cmt,   c_str , NULL,       0};
struct lang  no_lang = { NULL   , NULL,   no_words,  no_cmt,  no_str  , NULL,       0};

struct lang *lasearch[] = {
    &  c_lang ,
    &pas_lang ,
    &lsp_lang ,
    &asm_lang ,
    &htm_lang ,
    &bas_lang ,
    &style_lang ,
    &tcl_lang ,
    &lua_lang ,
    & no_lang
};

#else

const char *no_words[] = { NULL };
const char *no_cmt[]  = { NULL } ;
const char  no_str[]  = "";
struct lang  no_lang = { NULL , NULL, no_words, no_cmt, no_str, NULL, 0};

struct lang *lasearch[] = {
    & no_lang
};


#endif


struct lang *lang = &no_lang;
char langcode[40];

/*----------------------------------------------------------------------------*/
#define HTS 53

struct strl *hashtab[HTS];

int hashstr(const char *s) {
    unsigned int h; char c;
    for (h=0;0!=(c=*s++);h>>=1)
      if ((h^=c)&1) h^=0xa709;
    return h%HTS;
}

void freehash(void) {
    int i=0;
    do freelist(&hashtab[i]); while (++i<HTS);
    lang=NULL;
}

void inshash(const char *q) {
    conslist(&hashtab[hashstr(q)], newstr(q));
}

void set_lang(const char *p) {
    char buf[32]; const char *q, **cp;
    struct lang **lp; int i;
    extern char synhilite;

    buf[0]=0;

    for (q=p+strlen(p), i=30;--i && q>p; q--)
        if ('.'==q[-1])
        {
            strcpy(buf, q);
            break;
        }

    for (lp=lasearch;NULL!=(cp=(*lp)->ext);lp++) {
       for (;NULL!=*cp;cp++) {
            if (!stricmp(buf,*cp)) {
                goto brk;
            }}}
brk:
    synhilite = NULL!=(*lp)->ext;

    if (lang==*lp)
        return;

    freehash();
    lang=*lp;

    for (cp=lang->keys;NULL!=(q=*cp);cp++) {
        inshash(q);
    }

    if (NULL!=(cp=lang->prae)) {
        for (buf[0]=**cp++; NULL!=(q=*cp); cp++) {
            strcpy(buf+1,q);
            inshash(buf);
        }}

    langcode[0]=0;
    if (NULL!=(q=lang->cclass)) {
        rcomp((unsigned char*)q, (unsigned char*)langcode, sizeof(langcode),1);
        langcode[0]=1;
    }

}

/*----------------------------------------------------------------------------*/
// step back for 20 lines and check for comments;

#define CM 100

int cmt[CM]; int cn;

void checkcomment(int oo, int yn) {
    int a,b,o,p,i;
    unsigned char *s,c,d,f,ls;
    const char **lc,*la,*le;

    if (langflg==0) return;

    a=cn=0; s=NULL; la=NULL;
    o=prevline_v(oo, 20, &i); yn+=i;
    f=ls=0;
    lc=lang->cmt;

    for (;yn>0 && o<flen;) {
        c=getchr(o++);
        if (c==10) {
            yn--;
            ls=0;
            if (f==2) f=0;
            continue;
        }
        if (f==0)
        {
            for (s=(unsigned char*)lang->str;*s;s+=2) {
                if (c==*s) { f=2; goto c1; }
            }
            if (0==(lang->flg & CMT_LINESTART) || 0==ls)
            for (i=0;NULL!=(la=lc[i]);i++) {
                if (c!=*la) continue;
                for (p=o; p<flen; ) {
                    d=*++la;
                    if (d<=32) {
                        a=o-1;
                        if (d==32) { la++; f=1; goto c1; }
                        for (;p<flen && getchr(p++)!=10;);
                        yn--;
                        ls=0;
                        goto s1;
                    }
                    if (d!=getchr(p++)) break;
                }
            }
            ls = 1;
        c1:
            continue;
        }
        if (f==2) {
            if (c==*s) f=0;
            else
            if (c==*(s+1)) o++;
            continue;
        }
        if (c==*la) {
            for (le=la,p=o;p<flen;) {
                d=*++le;
                if (d==0) goto s1;
                if (d!=getchr(p++)) break;
            }
        }
        continue;
    s1:
        o=p;
    s2:
        f=0; b=o;
        if (b<oo) continue;
        if (a<oo) a=oo;
        cmt[cn]=a;
        cmt[cn+1]=b;
        cn+=2;
        if (cn==CM) return;
        continue;
    }
   if (f==1) goto s2;
}


/*----------------------------------------------------------------------------*/
int nextcmt(int o) {
    int i,e;
    for (i=0;i<cn;i+=2) {
        if (o<cmt[i]) break;
        if (o<(e=cmt[i+1])) return e-o;
    }
    return 0;
}

int cmpkey(int p, int n, char w, int *ip) {
    int i;
    unsigned char d,*cp; char buf[80];
    struct strl *a;

    i=0; cp=(unsigned char*)buf; if (n>31) n=31;
    if (w) *cp++=w;
    if (langcode[0]) goto p3;

    if (n && (d=getchr(p))<128 && (isalpha(d) || d=='_'))
      do i++,*cp++=d;
        while (i<n && (d=getchr(p+i))<128 && (isalnum(d) || d=='_'));
p2:
    if ((*ip=i)<1) return 0;

    *cp=0;
    if (lang->flg & NOCASE) strlwr(buf);

    for (a=hashtab[hashstr(buf)];NULL!=a;a=a->next)
        if (!strcmp(a->str,buf)) return 1;

    return 0;

p3:
#define CLSZ 32
    for (;i<n;i++,*cp++=d) {
        d=getchr(p+i);
        if (langcode[2+((d>>3)&(CLSZ-1))] & (1<<(d&7)))
            break;
    }
    goto p2;
}

/*----------------------------------------------------------------------------*/
extern char * vscr1;
extern char * vscr2;

void ctext (HDC hdc,int x, int l, int c, int y, int la, char rf, int ra, int re) {
    unsigned char buf[256],*p; int k,i,c1,c2,d;

    x-=clft;
    if (x<0)  l+=x, x=0;
    if (l<=0) return;
    if (l>256) l=256;

    d = colortrans[(int)winid][1];
    SelectObject(hdc, My_Weights[c+d] ? hfnt_b:hfnt_n);

    if (la==-1) memset(buf,' ',l);
    else copyfrom((char*)buf,la+x+clft,l);

    for (p=buf,k=l;k;k--,p++)
        if (*p==TABC)
#if 0
            *p=0xb7;
#else
            *p=32;
#endif

    for (k=0;k<l;k+=i,x+=i)
    {
        i=l-k;

        if (rf==0 || x>=re || x+l<=ra) goto p1;
        if (x<ra)  { i=imin(i,ra-x); goto p1; }

        i=imin(i,re-x);

        c1=My_Colors[CItxt + d];
        c2=My_Colors[CIbgd + d];
        goto p2;
    p1:
        c1=My_Colors[c    +  d];
        c2=My_Colors[Cbgd +  d];
    p2:
        if (c2 == -1)
            SetBkMode(hdc, TRANSPARENT);
        else
            SetBkMode(hdc, OPAQUE), SetBkColor(hdc,c2);

        SetTextColor(hdc,c1);
        TextOut(hdc, zx0+x*zx, y, (char*)buf+k, i);
    }
}

/*----------------------------------------------------------------------------*/
void textout_s(HDC hdc, int yy, int la, int n, int xl, char rf, int ra, int re) {

            unsigned char b,c,d,e,g,m,*s; int x,i,j,l0,x0,p;

            p=la; e=g=c=x=m=0;

            if (NULL!=*lang->keys && 1==langflg) g=1;
            if (NULL!=lang->prae) e=**lang->prae;

            for (;x<xl;) {
                if (n==0) {
                    c=Ctxt; j=xl-x; la=-1;
                    goto p2;
                }
                if (g==0)
                {
                    c=Ctxt;
#if 0
                    if (getchr(p)=='~')
                    {
                        la++, p++;
                        if (--n==0) continue;
                        c=Ccmt;
                    }
#endif
                    j=n;
                    goto p2;
                }

              x0=x; l0=0;
              do {
        //comment
                if (0!=(i=nextcmt(p))) {
                    c=Ccmt; j=imin(i,n);
                    goto p1;
                }

                d=getchr(p);
                if (d==' ' || d==TABC) goto p0;
                if (d>=128) { j=1; c=Ctxt; goto p1; }

        //style color spec
                if (d=='#'
                    && (lang->flg & HEX_NUM)
                    && n>1
                    && isxdigit(b=getchr(p+1)))
                {
                    for (j=2; j<n && isxdigit(b=getchr(p+j)) ;j++);
                    if (j==7 && (j==n || !isalpha(b)))
                    {
                            c=Cnum; goto p1;
                    }
                }
        //number
                if (isdigit(d))
                    // || d=='-' && n>1 && isdigit(getchr(p+1)))
                {
                    for (j=1; j<n && (isxdigit(b=getchr(p+j)) || (j==1 && (b&0xdf)=='X'));j++);
                    c=Cnum; goto p1;
                }
        //string
                for (s=(unsigned char*)lang->str;*s;s+=2)
                  if (d==*s) {
                    for (j=1;j<n && (b=getchr(p+j))!=*s;j++) {
                        if (b==*(s+1) && j+1<n) j++;
                    }
                    if (j<n) j++;
                    c=Cstr; goto p1;
                }

        //praeprocessor
                if (m && cmpkey(p,n,e,&j)) {
                    c=Cmac; m=0; goto p1;
                }

                if (0!=(m=(d==e))) {
                    j=1; c=Cmac; goto p1;
                }

        //keywords
                if (cmpkey(p,n,0,&j)) {
                    c=Ckey; goto p1;
                }

        //identifier
                if (j) {
                    c=Ctxt; goto p1;
                }
        p0:
        //the rest (spaces & operators)
                l0++; p++; x++;

             } while (--n && x<xl); j=0;
        p1:
             if (l0) ctext(hdc, x0, l0,  Copr, yy, la, rf, ra, re);
        p2:
             if (x+j>xl) { j=xl-x; if (j<=0) return; }

             if (j)  ctext(hdc,  x,  j,  c, yy, la, rf, ra, re);
             n-=j;
             p+=j;
             x+=j;
             }
}

/*----------------------------------------------------------------------------*/
int printpage(HDC hdc, int x, int y1, int y2) {
    int i,yy,xx,mm, ra, re, la, le, ll;
    char rf, ef, mf;

    struct mark_s ms;
    int getmark0(struct mark_s *m);

    ef=1; mm=0; xx=x+clft; yy = zy*y1+zy0;

    la=nextline_v(fpga,y1,&i);
    if (i<y1) ef=0, la=flen;

    mf=getmark0(&ms);

    for (i=y1; i<y2; i++, yy+=zy) {

        le=la+(ll=linelen(la));
        if (ll>mm) mm=ll;

        rf=ra=re=0;
        if (mf && ef && la<ms.e && le>=ms.a) {
            if (vmark==0) {
                ra=(la>ms.a) ? 0 : ms.xa-clft;
                re=(le<ms.e) ? x : ms.xe-clft;
            } else {
                ra=ms.xa-clft;
                re=ms.xe-clft;
            }
            ra=iminmax(ra, 0, x);
            re=iminmax(re, 0, x);
            rf=(re>ra);
        }

        textout_s(hdc, yy, la, ll, xx, rf, ra, re);

        if (le==flen) la=le, ef=0;
        else la=nextline(le, 1);
    }
   return mm;
}

/*----------------------------------------------------------------------------*/

