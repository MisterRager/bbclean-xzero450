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
// EDFILES.C - functions to handle the edited file, buffers, open/save dlgs

#include "edstruct.h"
#include <malloc.h>
#include <commdlg.h>

char backup=0;
char bakdir=0;
char unix_eol=0;

#define BLS     4096
#define EDSZ    (BLS*3)
#define QFLSH   (BLS*2)
#define BLN     2

struct bufpage {
    struct bufpage *next;
    struct bufpage *prev;
    int size;
    char cont[BLS];
};

#define inbuff (buf_e-buf_a)

/*----------------------------------------------------------------------------*/
ST void ins_pg(void) {
    struct bufpage *p;
    p=(struct bufpage *)c_alloc(sizeof(struct bufpage));
    p->next=next_pg;
    p->prev=prev_pg;
    if (NULL!=prev_pg) prev_pg->next=p;
    if (NULL!=next_pg) next_pg->prev=p;
    next_pg=p;
}

ST void del_pg(void) {
    struct bufpage *p;
    p=next_pg;
    next_pg=p->next;
    prev_pg=p->prev;
    if (NULL!=prev_pg) prev_pg->next=next_pg;
    if (NULL!=next_pg) next_pg->prev=prev_pg;
    m_free(p);
}

ST int flush_out(int m) {
    int l;
    l=imin(inbuff,BLS);
    ins_pg();
    memmove(next_pg->cont,buffer+m,l);
    next_pg->size=l;
    return l;
}

ST void flush_last(void){
    buf_e-=flush_out(inbuff-BLS);
}

ST void flush_first(void){
    int o;
    buf_a+=o=flush_out(0);
    memmove(buffer,buffer+o,inbuff);
    prev_pg=next_pg;
    next_pg=next_pg->next;
}

ST void insert_first(void){
    int s;
    next_pg=prev_pg;
    prev_pg=next_pg->prev;
    s=next_pg->size;
    memmove(buffer+s,buffer,inbuff);
    memmove(buffer,next_pg->cont,s);
    buf_a-=s;
    del_pg();
}

ST void insert_last(void){
    int s;
    if (next_pg==NULL) return;
    s=next_pg->size;
    memmove(buffer+inbuff,next_pg->cont,s);
    buf_e+=s;
    del_pg();
}

ST void flush_all(void) {
    for (;inbuff;) {
        if (inbuff<BLS) insert_last();
        flush_first();
    }
}

ST struct bufpage *getstart(void) {
    struct bufpage *p,*n;
    n=next_pg; p=prev_pg;
    for (;NULL!=p;p=(n=p)->prev);
    return n;
}

ST void fill_buffer(int o){
    struct bufpage *p,*n;
    int i;
    i=o/BLS; buf_a=buf_e=i*BLS;
    p=NULL; n=getstart();
    for (;i--;) n=(p=n)->next;
    prev_pg=p; next_pg=n; i=BLN;
    for (;i--;) insert_last();
}

ST void shift_dn(void) {
    if (inbuff > EDSZ-BLS) flush_last();
    insert_first();
}

ST void shift_up(void) {
    if (inbuff > EDSZ-BLS) flush_first();
    insert_last();
}

/*----------------------------------------------------------------------------*/
ST char *getmem(int o) {

    for (;o<buf_a;) {
        if (buf_a-o>QFLSH) goto p2;
        shift_dn();
    }

    for (;o+BLS>buf_e && flen>buf_e;) {
        if (o+BLS-buf_e>QFLSH) goto p2;
        shift_up();
    }

    goto p1;
p2:
    flush_all();
    fill_buffer(o);
p1:
    return buffer+(o-buf_a);
}

/*----------------------------------------------------------------------------*/
void insdelmem (int o, int len) {
    int a,b,d,f; char *at,*to,*p;

    f = len>0; if (!f) len=-len;

    p=getmem(o);

    for (;len;)
    {
        b=imin(BLS,len);
        if (f)
        {
            if (inbuff+b>EDSZ)
            {
                if (o>=buf_a+BLS) flush_first(), p-=BLS;
                else flush_last();
            }
            to=(at=p)+b,a=o,d=b;
        }
        else
        {
            at=(to=p)+b,a=o+b,d=-b;
            if (a>buf_e) insert_last();
        }

        memmove(to,at,buf_e-a);

        buf_e+=d, flen+=d, len-=b;
    }
}

/*----------------------------------------------------------------------------*/
void clear_buffer(void) {
    struct bufpage *n=getstart();
    freelist(&n);
    next_pg = prev_pg = n;
    buf_a   = buf_e   = 0;
    u_reset();
}

struct edvars *new_buffer(void) {
    return (struct edvars *)c_alloc(sizeof(struct edvars)-1+EDSZ);
}

/*----------------------------------------------------------------------------*/
// buffer access

unsigned char getchr(int o) {
    if (o<buf_a || o>=buf_e) {
        if (o<0 || o>=flen) return 0; 
        return *getmem(o);
    }
    return *(buffer+(o-buf_a));
}

#if 0
void setchr(int o, char c) {
    if (o<0 || o>=flen) return;
    *getmem(o)=c;
    upd=1;
}
#endif

void copyto(int dst, const char *src, int l) {
    int b; char *p;
    for (;l;) {
        b=imin(l,BLS);
        p=getmem(dst);
        memmove(p,src,b);
        l-=b; src+=b; dst+=b;
    }
    upd=1;
}

void copyfrom(char *dst, int src, int l) {
    int b; char *p;
    for (;l;) {
        b=imin(l,BLS);
        p=getmem(src);
        memmove(dst,p,b);
        l-=b; src+=b; dst+=b;
    }
}

void clearchr(int dst, char c, int l) {
    int b; char *p;
    for (;l;) {
        b=imin(l,BLS);
        p=getmem(dst);
        memset(p,c,b);
        l-=b; dst+=b;
    }
    upd=1;
}

/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
void revlist (void *d) {
    void **a,**b,**c;
    for (a=*(void***)d, b=NULL; a!=NULL;) c=*(void***)a, *(void***)a=b, b=a, a=c;
    *(void**)d=b;
}

void appendlist (void *a,void *e) {
    for (;*(void**)a!=NULL;a=*(void**)a);
    *(void**)a=e;
}

void conslist (void *a,void *e) {
    *(void**)e=*(void**)a;
    *(void**)a=e;
}

void dellist (void *a,void *e) {
    for (;a!=NULL;a=*(void**)a)
        if (*(void**)a==e) { *(void**)a=**(void***)a; break; }
}

void delitem(void *a,void *e) {
    dellist(a,e);
    if (NULL!=e) m_free(e);
}

void inslist (void *a, void *e, void *i) {
    void *x;
    if (e==i) return;
    dellist(a,i);
    if (NULL==e)
        appendlist(a,i);
    else
        x=*(void**)e, *(void**)e=i, *(void**)i=x;
}

struct strl *newstr(const char *s) {
    struct strl *b=(struct strl *)c_alloc(sizeof(struct strl)+strlen(s)+1);
    strcpy(b->str,s);
    return b;
};

void freelist(void *p) {
    void *s,*q;
    for (q=*(void**)p;NULL!=q;)
        q=*(void**)(s=q), m_free(s);
    *(void**)p=q;
}

void appendstr(struct strl **p, const char *s) {
    appendlist(p, newstr(s));
}

/*----------------------------------------------------------------------------*/
int fileexist(char *name) {
    DWORD a=GetFileAttributes(name);
    return (a==(DWORD)-1) ? 0 : a;
}

HANDLE hopenfile(char *name, DWORD access, DWORD creation) {
    HANDLE hf;
    hf=CreateFile(name,access,
        access==GENERIC_READ?FILE_SHARE_READ|FILE_SHARE_WRITE:0,
        NULL,creation,0,NULL);
    if (hf==INVALID_HANDLE_VALUE) return NULL;
    return hf;
}


int getftime_0(char *fn, FILETIME *ft) {
    HANDLE hf; int r;
    hf=hopenfile(fn, 0, OPEN_EXISTING);
    if (hf==NULL) return 0;
    r=GetFileTime(hf, NULL, NULL, ft);
    CloseHandle(hf);
    return r!=0;
}

int getftime(void) {
    fileflg&=~4;
    return getftime_0(filename, &filetime);
}

int setftime(void) {
    HANDLE hf;
    hf=hopenfile(filename,GENERIC_WRITE,OPEN_EXISTING);
    if (hf==NULL) return 0;
    SetFileTime(hf, &filetime, &filetime, &filetime);
    CloseHandle(hf);
    return 1;
}


void f_reload(int f) {
    const char *p; int i;

    if (fileflg&4) return;

    if (f==0)  goto r0;

    if (!changed())// && fileflg==0)
        goto r1;
    fileflg &= ~3;
    p="Discard changes ?\n%s";
    i=2+4;
    goto r2;

r1:
    clear_buffer();
    chg=0;
    if (loadfile()) {
        ed_cmd(EK_INIT,plin,cury,curx);
        settitle();
        unmark();
        InfoMsg("Loaded");
    } else CloseFile();
    return;


r0:
    if (fileflg & 1) goto r1;
    if (fileflg & 2) goto r3;

    p=changed()
    ? "File changed on disk. Discard changes and reload it ?\n%s"
    : "File changed on disk. Reload it ?\n%s"
    ;
    i=2+4+16+32;
r2:
    fileflg|=4;
    i=oyncan_msgbox(p, filename, i);
    fileflg&=~4;
    if (i==IDYES)       { goto r1; }
    if (i==IDALWAYS)    { fileflg|=1; goto r1; }
    if (i==IDNO)        { goto r3; }
    if (i==IDNEVER)     { fileflg|=2; }
r3:
    if (f==0) {
        getftime();
        u_setchg(1);
        setwtext();
    }
    return;

}


void checkftime(HWND hwnd)  {
    FILETIME t1; struct edvars *v;
    for (v=ed0; NULL!=v; v=v->next)
        if (getftime_0(v->sfilename, &t1)
        && CompareFileTime(&v->sfiletime, &t1) != 0)
            PostMessage((HWND)hwnd, WM_COMMAND, CMD_FILECHG, (LPARAM)v);
}

/*
Yeah sure. This is what I've been looking at so far on MSDN:

  NT: ReadDirectoryChangesW w/ FILE_NOTIFY_CHANGE_LAST_WRITE and a
callback.

  9x: FindFirstChangeNotification w/ FILE_NOTIFY_CHANGE_LAST_WRITE
      then (I think) FindNextChangeNotification
      then WaitForSingleObject in a thread

Something like that.
*/

HANDLE openf(char *name) {
    return hopenfile(name,GENERIC_READ,OPEN_EXISTING);
}

HANDLE creatf(char *name) {
    return hopenfile(name,GENERIC_WRITE,CREATE_ALWAYS);
}

DWORD readf(HANDLE hf, void *buf, DWORD l) {
    DWORD r=0;
    ReadFile(hf, buf, l, &r, NULL);
    return r;
}

DWORD writf(HANDLE hf, void *buf, DWORD l) {
    DWORD r=0;
    WriteFile(hf, buf, l, &r, NULL);
    return r;
}

int closef(HANDLE hf) {
    return CloseHandle(hf);
}

/*----------------------------------------------------------------------------*/
int backupfile (char *org) {
    char bak[128], *q, d, e;

    if (0==backup || 0==fileexist(org)) return 1;

    if (bakdir) {
        makepath(bak, projectdir, "bak");
        if (0==(fileexist(bak) & FILE_ATTRIBUTE_DIRECTORY))
            CreateDirectory (bak, NULL);
        makepath(bak,bak,fname(org));
    } else {
        q=fname(strcpy(bak,org));
        for (d='~';e=*q,0 != (*q=d);d=e,q++);
    }

    if (0!=fileexist(bak)) DeleteFile(bak);
    if (0!=MoveFile(org,bak)) return 1;
    oyncan_msgbox("Rename failed:\n%s",bak,1);
    return 0;
}

/*----------------------------------------------------------------------------*/
int loadfile(void) {

    HANDLE fp;
    char d,*r=NULL;
    char *buf_i;
    char *buf_o;
    int a,b,n,k,e;
    int p,tl;

    flen=tl=a=b=e=0; k=tabs;

    if (NULL==(buf_i = (char *)m_alloc (BLS))) goto end_0;
    if (NULL==(buf_o = (char *)m_alloc (BLS))) goto end_1;
    if ((fp = openf(filename))==NULL)  goto end_2;

p0:
    n=0;
    for (;;) {
        if (0==a && 0==(a=readf(fp, r=buf_i, BLS)))
            break;

        d=*r++, a--, b++;
        switch (d) {
        case 10: b=0; tl++; break;
        case  9: d=(char)TABC; if (b%k) r--,a++; break;
        case 13: b--; continue;
        }
        buf_o[n++]=d;
        if (n>=BLS) {
p2:
            insdelmem(p=flen,n);
            copyto(p, buf_o, n);
            goto p0;
        }
    }
    if (n) goto p2;
    closef(fp);
    getftime();
    e=1;
end_2:
    m_free(buf_o);
end_1:
    m_free(buf_i);
end_0:
    tlin=tl;
    return e;
}

/*----------------------------------------------------------------------------*/
int is_makefile(const char *name)
{
    char path[MAX_PATH];
    return !!strstr(strlwr(strcpy(path, name)), "makefile");
}

/*----------------------------------------------------------------------------*/
int savefile(char *name) {

    HANDLE fp;
    unsigned char d,e,f,g,l,*q,*p=NULL;
    int a,b,c,i,k,t,s;
    char *buf_i;
    char *buf_o;

    e=l=g=d=a=b=t=s=0;

    k = tabs;
    if (!tuse && !is_makefile(name))
        k = 0;

    if (0==backupfile(name)) goto end_0;

    if (NULL==(buf_o = (char *)m_alloc (BLS))) goto end_0;
    if (NULL==(buf_i = (char *)m_alloc (BLS))) goto end_1;
    if (NULL==(fp = creatf(name))) goto end_2;
p0:
    i=0; q=(unsigned char*)buf_o;
    for (;;) {
        if (d) goto p4;
        if (b==0) {
            if (0==(b=imin(BLS,flen-a)))
                break;
            copyfrom(buf_i, a, b);
            p = (unsigned char*)buf_i;
            a+=b;
        }
        d=*p++; t++; b--;
        if (d==TABC || (d==32 && g==0)) {
            s++;
            d = 0;
            if (k<2 || t%k)
                continue;
            d = 9;
            s = 0;
            goto p5;
        }

        g=1;
p4:
        if (s) {
            f=32; s--;
            goto p3;
        }
        if (d==10) {
            if (l==0 && unix_eol==0) {
                f=13; l=1; goto p3;
            }
            t=g=l=0;
        }
p5:
        f=d; d=0;
p3:
        *q++=f; i++;
        if (i>=BLS) {
p1:
            c=writf(fp, buf_o, i);
            if (c<i) goto end_3;
            goto p0;
        }
    }
    if (i) goto p1;
    e=1;
end_3:
    closef(fp);
end_2:
    m_free(buf_i);
end_1:
    m_free(buf_o);
end_0:
    return e;
}

/*----------------------------------------------------------------------------*/
void addfile(void) {
    void *p;
    inslist(&ed0, edp, p=new_buffer());
    edp=(edvars*)p;
}

void nextfile(void) {
    struct edvars *p;
    if (NULL!=(p=edp) && NULL!=(p=p->next))
        edp=p, settitle();
}

struct edvars *get_prev(void) {
    struct edvars *p,*q;
    if (NULL!=(p=ed0) && edp!=p)
        for (;NULL!=(q=p->next); p=q)
            if (q==edp) return p;
    return NULL;
}


void prevfile(void) {
    struct edvars *p;
    if (NULL!=(p=get_prev()))
        edp=p, settitle();
}

void insfile(struct edvars *p) {
    inslist(&ed0,edp,p);
    edp=p;
    settitle();
}

void NewFile(void) {
    static int filn=0;
    addfile();
    sprintf(filename,"new file %d",++filn);
    settitle();
}

void delfile(void) {
    struct edvars *p;
    if (NULL==(p=get_prev())) p=edp->next;
    clear_buffer();
    delitem(&ed0,edp);
    edp=p;
}

void CloseFile(void) {
    delfile();
    settitle();
}

int LoadFile(LPSTR pszFileName) {
    struct edvars *p;
    char tmp[256];

    GetFullPathName(pszFileName,256,tmp,NULL);

    for (p=ed0;p!=NULL;p=p->next) {
        if (0==stricmp(tmp,p->sfilename)) {
            insfile(p);
            return 1;
        }}
    addfile();
    strcpy(filename,tmp);
    if (0==loadfile()) {
        CloseFile();
        return 0;
    }
    fnameflg=1;
    settitle();
    return 1;
}


char savedly = 2; //sec

int SaveFile(LPSTR pszFileName) {
    unsigned long d;

    if (0==savefile(pszFileName)) return 0;
    strcpy(filename,pszFileName);
    fnameflg=1;
    getftime();
    u_setchg(0);

    if (savedly>0) {
        d=savedly;
        d*=10000000UL;
        if (d>filetime.dwLowDateTime)
            filetime.dwHighDateTime--;
        filetime.dwLowDateTime-=d;
        setftime();
    }

    InfoMsg("Saved");
    settitle();
    return 1;
}

/*----------------------------------------------------------------------------*/
void clean_up(void) {
    void freehash(void);
    void clrcfg(void);

    while(NULL!=edp)
        delfile();
    clrcfg();
    freehash();

#ifdef BBOPT_MEMCHECK
    int n = m_alloc_size() - clip_s;
    if (ownd)
        n-=sizeof(struct edvars)+EDSZ;
    if (0!=n) {
        char buf[40];
        sprintf(buf,"alloc = %d", n);
        MessageBox(NULL,buf,"",MB_OK|MB_TOPMOST|MB_SETFOREGROUND);
    }
#endif
}

/*----------------------------------------------------------------------------*/

#ifndef OPENFILENAME_SIZE_VERSION_400
#ifndef CDSIZEOF_STRUCT
#define CDSIZEOF_STRUCT(s, member) (((LPBYTE)&((s*)0)->member - (LPBYTE)(s*)NULL) + sizeof (((s*)0)->member) )
#endif
#define OPENFILENAME_SIZE_VERSION_400 CDSIZEOF_STRUCT(OPENFILENAMEA, lpTemplateName)
#endif
/*----------------------------------------------------------------------------*/

int DoFileOpenSave(HWND hwnd, int mode) {

    OPENFILENAME ofn;
    char *szFileName, *fn, buf[128];
    int o,l,n,r=0;
    const char *err;
    const int FNMAX  = 32768;
    szFileName=(char*)c_alloc(FNMAX);

    memset(&ofn, 0, sizeof ofn);
    ofn.lStructSize = sizeof(OPENFILENAME);
    if (LOBYTE(GetVersion()) < 5) // win9x/me
        ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;
    ofn.hwndOwner   = hwnd;
    ofn.lpstrInitialDir = currentdir;
    ofn.lpstrFile   = szFileName;
    ofn.nMaxFile    = FNMAX;
    ofn.nFilterIndex = 1;
    ofn.lpstrFilter =
        "All Files (*.*)\0*.*\0"
        "Text Files (*.txt)\0*.txt\0"
        "Styles (*.style)\0*.style\0"
        "Resources (*.rc)\0*.rc\0"
        "C-Files (*.c *.cpp *.h)\0*.c;*.cpp;*.h\0"
        ;

    switch (mode) {
    case 0:
        ofn.Flags = OFN_EXPLORER | OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT;
        if(GetOpenFileName(&ofn)==0)
            goto end;
        o=ofn.nFileOffset;
        l=strlen(strcpy(buf,szFileName));   // path only, if multi
        for (;l;) {
            if (o<l) {  l=0; goto p1; }     // single
            if (buf[l-1]!='\\') buf[l++]='\\';
            if (0==(n=strlen(strcpy(buf+l,szFileName+o)))) break;
            o+=1+n;
        p1:
            if(FALSE==LoadFile(fn=buf)) {
                err = "Load failed:\n%s"; goto p_err;
            }
        }
        settitle();
        prjflg|=2;
    succ:
        r=1;
        goto end;

    case 1:
    save_as:
        ofn.Flags = OFN_EXPLORER | OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_ENABLEHOOK;
        if (fnameflg)
            strcpy(szFileName,filename);
        else
            sprintf(buf, "Save '%s' as ...",filename),ofn.lpstrTitle=buf;

        if (GetSaveFileName(&ofn)==0)
            goto end;

        fn = szFileName;
        goto save1;

    case 2:
        if (fnameflg==0)
            goto save_as;
        fn = filename;
    save1:
        if (SaveFile(fn)) goto succ;
        err = "Saving failed:\n%s";
    p_err:
        oyncan_msgbox(err, fn, 1);
        r=-1;
    default:
    end:
        m_free(szFileName);
        GetCurrentDirectory(256,currentdir);
        return r;
}}

/*----------------------------------------------------------------------------*/
int QueryDiscard_1(HWND hwnd, int f) {
    int r;

    if (0 == (f&4))
    {
        if (!changed() || ((f&2) && (fileflg&2))) return 1;
    }
    if (f&1)
    {
        r=oyncan_msgbox("Save changes ?\n%s", filename, 2+4+8);
        if (r==IDCANCEL) return 0;
        if (r==IDNO)     return 1;
    }
    return DoFileOpenSave(hwnd, 2);
}


int QueryDiscard(HWND hwnd, int f) {
    int r = 1;
    struct edvars *p=edp;
    for (edp=ed0;edp!=NULL;edp=edp->next) {
        if (1 != (r=QueryDiscard_1(hwnd, f)))
            break;
    }
    edp=p;
    settitle();
    return r;
}

/*----------------------------------------------------------------------------*/
HWND seaDlg;
char seabuf[80];
char rplbuf[80];
char seamodeflg;
char * ed_make_rplc(char *, char *);

/*----------------------------------------------------------------------------*/
int do_search (int msg, DWORD param)
{
    static HWND hwnd;
    static char s_dir, s_cont;
    struct mark_s m;
    int i; char c,tmpbuf[80],*p,rpltmp[80];

    switch(msg) {
        case 0:

        hwnd = (HWND)param;
        s_dir=s_cont=0;

        if (getmark(&m) && m.y==0 && m.l<40) {
            copyfrom(p=tmpbuf, m.a, m.l);
            tmpbuf[m.l]=0;
            s_cont=s_dir=1;
            strcpy(seabuf,p);
            return 2;

            //updbox(hDlg, SEA_LINE, &seabox, seabuf);
        }
        return 1;

        case IDRPL:
            if ((c=s_dir) ==0)  goto s1;
            if ((s_cont&1)==0)  goto s2;
            s_cont=4;
    r1:
            p=(seamodeflg&64) ? ed_make_rplc(rpltmp,rplbuf) : rplbuf;
            if (s_cont!=4)
            {
                sprintf(tmpbuf,"\001rplace by '%s'", p);
                InfoMsg(tmpbuf);
                return 1;
            }
            SendMessage(hwnd, CMD_NSEARCH, 8|(int)(s_dir==1), (LPARAM)p);
            s_cont=0;
            return 1;

        case IDCANCEL:
            SendMessage(hwnd,CMD_NSEARCH,0,0);
            return 1;

        case IDUP: c=2;   goto s2;
    s1:
        case IDOK: c=1;
    s2:
            if (seabuf[0]==0) return 1;
            i = (s_cont && (c==s_dir || (s_cont & 1)));
            s_dir=c;
            if (i) c|=4;
            {
              struct sea sea;
              sea.from=fpos;
              sea.str=strcpy(tmpbuf,seabuf);
              sea.sf=c|seamodeflg;
              i=SendMessage(hwnd,CMD_NSEARCH,sea.sf,(LPARAM)&sea);
              if (i>0) {
                s_cont=1;
                if (seamodeflg&64) goto r1;
                return 1;
              }
            }
            s_cont=2;
            sprintf(tmpbuf,i==0 ? "no more '%s'": "error in pattern '%s'", seabuf);
            InfoMsg(tmpbuf);
            return 1;


        case IDGREP: {
            char *buf, *p, *s, c, d, e;
            const char *q;
            p=buf=(char*)m_alloc(1000), s=grep_cmd, d=16;
            do {
                q="(:)";
                if (*q==(c=*s++))
                    for (e=d&seamodeflg, d<<=1; *++q; e=!e)
                      for (;(c=*s++)!=*q;) {
                        if (0==c) goto se;
                        if (e) *p++=c;
                      }
                else
                if (c=='%') {
                    if ((c=*s++)=='%') goto se;
                    p+=strlen(strcpy(p,c=='2'?(rplbuf[0]?rplbuf:"*"):seabuf));
                }
                else
                se: *p++=c;
            } while(c);

            SendMessage(hwnd,WM_COMMAND,CMD_RUNTOOL,(LPARAM)buf);

            m_free(buf);
            }
            return 1;

        default:
            break;
        }

    return 0;
}

/*----------------------------------------------------------------------------*/
