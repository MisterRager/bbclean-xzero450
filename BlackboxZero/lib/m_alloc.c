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
/* memory tracker */

#include "bblib.h"

#ifdef BBOPT_MEMCHECK

#define _STRING(x) #x
#define STRING(x) _STRING(x)
#pragma message(__FILE__ "("STRING(__LINE__)") : warning 0: MEMCHECK enabled.")

#ifndef GWLP_USERDATA
# define DWORD_PTR unsigned long
#endif

struct alloc_block
{
    struct alloc_block *prev;
    struct alloc_block *next;
    int line;
    char file[40];
    unsigned size;
    unsigned check1;
    unsigned check2;
};

#define ALLOC_HASHSIZE 256
#define ALLOC_HASHFN(ab) (((DWORD_PTR)ab>>4) & (ALLOC_HASHSIZE-1))
#define ALLOC_MAG 0x12345678

#define ALLOC_MEM(ab) &ab->check2
#define ALLOC_BLK(v) (struct alloc_block*)\
    ((char*)v-(DWORD_PTR)ALLOC_MEM(((struct alloc_block*)NULL)))
#define ALLOC_CHK1(ab) (ab)->check1
#define ALLOC_CHK2(ab) ((struct alloc_block*)((char*)ab+ab->size))->check2

static struct alloc_block *SA[ALLOC_HASHSIZE];
unsigned alloc_size;
unsigned alloc_size_max;
unsigned alloc_count;
unsigned alloc_count_max;

const char *_m_file;
int _m_line;

/* ------------------------------------------------------------------------- */

/* ------------------------------------------------------------------------- */
static void mem_error_msg(const char *fn, const char *msg,
    struct alloc_block *ab, const char *file, int line)
{
    char buf[1000];
    int x;

    x = sprintf(buf, "%s:%d: Error(%s): %s", file, line, fn, msg);
    if (ab)
        x += sprintf(buf+x, " (%d bytes)", ab->size);
    sprintf(buf+x, "\nPress OK to start debugger");

    x = MessageBox(NULL, buf, "message from mem-check", MB_OKCANCEL|MB_TOPMOST);
    if (IDCANCEL == x)
    {
        /* DebugBreak(); */
        ExitProcess(0);
    }
}

/* ------------------------------------------------------------------------- */
static void link_sa(struct alloc_block *ab)
{
    unsigned int i = ALLOC_HASHFN(ab);
    if (SA[i])
        SA[i]->prev = ab;
    ab->next = SA[i];
    ab->prev = NULL;
    SA[i] = ab;
}

static void free_sa(struct alloc_block *ab)
{
    if (ab->prev) {
        ab->prev->next = ab->next;
    } else {
        unsigned int i = ALLOC_HASHFN(ab);
        SA[i] = ab->next;
    }
    if (ab->next)
        ab->next->prev = ab->prev;
}

static const char *_m_alloc_basename(const char **p_file, int *p_line)
{
    const char *s, *f;

    if (*p_file == NULL && *p_line == -1) {
        *p_file = _m_file;
        *p_line = _m_line;
        _m_file = NULL;
        _m_line = 0;
    }

    f = s = *p_file;
    if (s) {
        s = strchr(s, 0);
        while (s > f && s[-1] != '\\' && s[-1] != '/')
            --s;
    }
    return s;
}

static int check_block(const char *msg, struct alloc_block *ab, const char *file, int line)
{
    unsigned x;

    if (ab) {
        x = ab->size ^ ALLOC_MAG;
        if (x != ALLOC_CHK1(ab))
            ab = NULL;
    } else {
        x = 0;
    }

    if (!file)
        file = ab->file, line = ab->line;

    if (NULL == ab) {
        mem_error_msg(msg, "block is unknown", ab, file, line);
        return -1;
    } else if (x != ALLOC_CHK2(ab)) {
        mem_error_msg(msg, "block is corrupted", ab, file, line);
        return 1;
    }

    return 0;
}

unsigned m_alloc_usable_size(void *v)
{
    struct alloc_block *ab;
    if (v==NULL)
        return 0;
    ab = ALLOC_BLK(v);
    if (check_block("check", ab, NULL, 0) < 0)
        return 0;
    return ab->size;
}

unsigned m_alloc_size(void)
{
    return alloc_size;
}


/* ------------------------------------------------------------------------- */
static FILE *alloc_fp;
static char logfilename[100] = "m_alloc_dump.log";

void m_alloc_printf(const char *fmt, ...)
{
    char buffer[4000];
    va_list arg;
    if (NULL == alloc_fp)
    {
        GetModuleFileName(NULL, buffer, MAX_PATH);
        strcpy(strrchr(buffer, '\\')+1, logfilename);
        alloc_fp = fopen(buffer, "wt");
    }
    va_start(arg, fmt);
    vsprintf (buffer, fmt, arg);
    fputs (buffer, alloc_fp);
    /*OutputDebugString(buffer); */
}

void m_alloc_dump_memory(void)
{
    int i, n;
    unsigned x;

    m_alloc_printf("memory allocation:\n");
    m_alloc_printf("count\t%d\t(max: %d)\n", alloc_count,  alloc_count_max);
    m_alloc_printf("size \t%d\t(max: %d)\n\n", alloc_size, alloc_size_max);
    for (i = 0; i < ALLOC_HASHSIZE; i++)
    {
        struct alloc_block *ab = SA[i];
        while (ab)
        {
            n = ab->size;
            x = n ^ ALLOC_MAG;
            if (x != ALLOC_CHK1(ab) || x != ALLOC_CHK2(ab)) {
                m_alloc_printf("%s:%d: %s (%d bytes)\n\n",
                    ab->file,
                    ab->line,
                    "block corrupted",
                    ab->size
                    );
                break;
            } else {
                m_alloc_printf("%s:%d: %s (%d bytes)\n",
                    ab->file,
                    ab->line,
                    "block is ok",
                    ab->size
                    );
                m_alloc_printf("content: <%s>\n\n", ALLOC_MEM(ab));
            }
            ab = ab->next;
        }
    }
    fclose(alloc_fp);
    alloc_fp = NULL;
}

void m_alloc_check_leaks(const char *title)
{
    if (alloc_size) {
        char text[100];
        char caption[100];
        sprintf(text, "Memory leak detected: %d bytes", alloc_size);
        sprintf(caption, "%s - Memory Check", title);
        MessageBox(MB_OK, text, caption, MB_OK|MB_TOPMOST|MB_SETFOREGROUND);

        sprintf(logfilename, "m_alloc_dump_%s.log", title);
        m_alloc_dump_memory();
    }
}

void m_alloc_check_memory(void)
{
    int i;
    for (i = 0; i < ALLOC_HASHSIZE; i++) {
        struct alloc_block *ab = SA[i];
        while (ab) {
            if (check_block("check", ab, NULL, 0) < 0)
                break;
            ab = ab->next;
        }
    }
}

void *_m_alloc(unsigned n, const char *file, int line)
{
    struct alloc_block *ab;
    const char *f;

    if (n==0)
        return NULL;

    f = _m_alloc_basename(&file, &line);

    ab=(struct alloc_block*)malloc(sizeof(*ab) + n);
    /*ab=(struct alloc_block*)GlobalAlloc(GMEM_FIXED, sizeof(*ab) + n); */

    if (NULL==ab) {
        mem_error_msg("m_alloc", "out of memory", ab, f, line);
        return NULL;
    }

    memset(ab, 0x22, sizeof *ab + n);

    ab->size = n;
    ab->line = line;
    strcpy(ab->file, f ? f : "<no file>");
    ALLOC_CHK2(ab) = ALLOC_CHK1(ab) = n ^ ALLOC_MAG;

    alloc_size+=n;
    ++alloc_count;

    if (alloc_size > alloc_size_max)
        alloc_size_max = alloc_size;
    if (alloc_count > alloc_count_max)
        alloc_count_max = alloc_count;

    link_sa(ab);
    return ALLOC_MEM(ab);
}

void *_c_alloc (unsigned n, const char *file, int line)
{
    void *v = _m_alloc(n, file, line);
    if (v)
        memset(v, 0, n);
    return v;
}

void _m_free(void *v, const char *file, int line)
{
    struct alloc_block *ab;
    const char *f;

    if (v==NULL)
        return;

    ab = ALLOC_BLK(v);
    f = _m_alloc_basename(&file, &line);

    if (check_block("free", ab, f, line) < 0)
        return;

    free_sa(ab);
    alloc_size-=ab->size;
    --alloc_count;
    memset(ab, 0x33, sizeof *ab + ab->size);
    free(ab);
    /*GlobalFree(ab); */
}

void *_m_realloc (void *v, unsigned s, const char *file, int line)
{
    struct alloc_block *ab;
    const char *f;
    void *v2 = NULL;

    f = _m_alloc_basename(&file, &line);

    if (v == NULL)
        return _m_alloc(s, file, line);

    if (s) {
        ab = ALLOC_BLK(v);

        if (check_block("m_realloc", ab, f, line) < 0)
            return NULL;

        v2 = _m_alloc(s, file, line);
        if (v2)
            memcpy(v2, v, s < ab->size ? s : ab->size);
    }

    _m_free(v, file, line);
    return v2;
}

void _m_setinfo(const char *file, int line)
{
    _m_file = file;
    _m_line = line;
}

/* ------------------------------------------------------------------------- */
#endif /*def BBOPT_MEMCHECK */
