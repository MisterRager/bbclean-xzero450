/*
 ============================================================================

  This file is part of the bbLean source code
  Copyright © 2001-2003 The Blackbox for Windows Development Team
  Copyright © 2004 grischka

  http://bb4win.sourceforge.net/bblean
  http://sourceforge.net/projects/bb4win

 ============================================================================

  bbLean and bb4win are free software, released under the GNU General
  Public License (GPL version 2 or later), with an extension that allows
  linking of proprietary modules under a controlled interface. This means
  that plugins etc. are allowed to be released under any license the author
  wishes. For details see:

  http://www.fsf.org/licenses/gpl.html
  http://www.fsf.org/licenses/gpl-faq.html#LinkingOverControlledInterface

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
  for more details.

 ============================================================================
*/

/* m_alloc.h - memory block tracker */

#ifdef BBOPT_MEMCHECK

void * _m_alloc(unsigned n, const char *file, int line);
void * _c_alloc (unsigned n, const char *file, int line);
void   _m_free(void *v);
void * _m_realloc (void *v, unsigned s, const char *file, int line);
unsigned m_alloc_usable_size(void *v);
void m_alloc_dump_memory(void);
void m_alloc_check_memory(void);
void   _m_setinfo(const char *file, int line);

extern unsigned alloc_size;
extern unsigned alloc_size_max;
extern unsigned alloc_count;
extern unsigned alloc_count_max;

#define m_alloc(s)      _m_alloc(s,__FILE__,__LINE__)
#define c_alloc(s)      _c_alloc(s,__FILE__,__LINE__)
#define m_realloc(p,s)  _m_realloc(p,s,__FILE__,__LINE__)
#define m_free(p)       _m_free(p)
#define new             (_m_setinfo(__FILE__,__LINE__), false) ? NULL : new

#define m_alloc_check_leaks(title) \
    if (alloc_size) { \
        BBMessageBox(MB_OK, "#" title " - Memory Check#Memory leak detected: %d bytes", alloc_size); \
        m_alloc_dump_memory(); \
    }

#else

#define m_alloc(n) ((void*)new char[n])
#define c_alloc(n) memset(new char[n], 0, n)
#define m_free(v)  (delete[] (char*)v)
#define m_alloc_check_leaks(title)
#define m_alloc_check_memory()

#endif

