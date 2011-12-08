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

#include "BB.h"
#include "Settings.h"

#include <shellapi.h>
#include <time.h>

#define ST static

ST char tempBuf[1024];

ST char blackboxrc_path[MAX_PATH];
ST char extensionsrc_path[MAX_PATH];
ST char menurc_path[MAX_PATH];
ST char pluginrc_path[MAX_PATH];
ST char stylerc_path[MAX_PATH];

//===========================================================================

unsigned long getfileversion(const char *path, char *buffer)
{
    char *info_buffer; void *value; UINT bytes; DWORD result; DWORD dwHandle;

    if (buffer) buffer[0] = 0;

    dwHandle = 0;
    result = GetFileVersionInfoSize((LPTSTR)path, &dwHandle);
    if (0 == result)
        return 0;

    if (FALSE == GetFileVersionInfo((LPTSTR)path, 0, result, info_buffer=(char*)m_alloc(result)))
        return 0;

    result = 0;

    if (buffer)
    {
        if (VerQueryValue(info_buffer,
            // change to whatever version encoding used (currently language neutral)
            "\\StringFileInfo\\000004b0\\FileVersion",
            &value, &bytes))
        {
            strcpy(buffer, (const char*)value);
            result = 1;
            //dbg_printf("version of %s <%s>", path, buffer);
        }
    }
    else
    if (VerQueryValue(info_buffer, "\\", &value, &bytes))
    {
        VS_FIXEDFILEINFO *vs = (VS_FIXEDFILEINFO*)value;
        result = MAKELPARAM(
            MAKEWORD(LOWORD(vs->dwFileVersionLS), HIWORD(vs->dwFileVersionLS)),
            MAKEWORD(LOWORD(vs->dwFileVersionMS), HIWORD(vs->dwFileVersionMS)));
        //dbg_printf("version number of %s %08x", path, result);
    }
    m_free(info_buffer);
    return result;
}

//===========================================================================
// API: GetBBVersion
// Purpose: Returns the current version
// In: None
// Out: LPCSTR = Formatted Version String
//===========================================================================

LPCSTR GetBBVersion(void)
{
    static char bb_version [40];
    if (0 == *bb_version) getfileversion(bb_exename, bb_version);
    return bb_version;
}

//===========================================================================
// API: GetBBWnd
// Purpose: Returns the handle to the main Blackbox window
// In: None
// Out: HWND = Handle to the Blackbox window
//===========================================================================

HWND GetBBWnd()
{
    return BBhwnd;
}

//===========================================================================
// API: GetOSInfo
// Purpose: Retrieves info about the current OS
// In: None
// Out: LPCSTR = Returns a readable string containing the OS name
//===========================================================================

LPCSTR GetOSInfo(void)
{
    if (osInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
    {
        if (osInfo.dwMinorVersion >= 90)
            return "Windows ME";
        if (osInfo.dwMinorVersion >= 10)
            return "Windows 98";
        return "Windows 95";
    }
    if (osInfo.dwMajorVersion == 5)
    {
        if (osInfo.dwMinorVersion >= 1)
            return "Windows XP";
        return "Windows 2000";
    }
    static char osinfo_buf[40];
    sprintf(osinfo_buf, "Windows NT %d.%d", (int)osInfo.dwMajorVersion, (int)osInfo.dwMinorVersion);
    return osinfo_buf;
}

//===========================================================================
// API: GetBlackboxPath
// Purpose: Copies the path of the Blackbox executable to the specified buffer
// In: LPSTR pszPath = Location of the buffer for the path
// In: int nMaxLen = Maximum length for the buffer
// Out: bool = Returns status of completion
//===========================================================================

LPSTR WINAPI GetBlackboxPath(LPSTR pszPath, int nMaxLen)
{
    GetModuleFileName(NULL, pszPath, nMaxLen);
    *(char*)get_file(pszPath) = 0;
    return pszPath;
}

//===========================================================================
// Function: NextToken
// Purpose: Copy the first token of 'string' seperated by the delim to 'buf'
//          and move the rest of 'string' down.
// returns: 'buf'
//===========================================================================

LPSTR NextToken(LPSTR buf, LPCSTR *string, const char *delims)
{
    char c, q=0;
    char* bufptr = buf;
    const char* s = *string;
    bool delim_spc = NULL == delims || strchr(delims, ' ');

    while (0 != (c=*s))
    {
        s++;
        if (0==q)
        {
            // User-specified delimiter
            if ((delims && strchr(delims, c)) || (delim_spc && IS_SPC(c)))
                break;

            if ('\"'==c || '\''==c) q=c;
            else
            if (IS_SPC(c) && bufptr==buf)
                continue;
        }
        else
        if (c==q)
        {
            q=0;
        }
        *bufptr++ = c;
    }
    while (bufptr > buf && IS_SPC(bufptr[-1])) bufptr--;
    *bufptr = '\0';
    while (IS_SPC(*s) && *s) s++;
    *string = s;
    return buf;
}

//===========================================================================
// API: Tokenize
// Purpose: Retrieve the first string seperated by the delim and return a pointer to the rest of the string
// In: LPCSTR string = String to be parsed
// In: LPSTR buf = String where the tokenized string will be placed
// In: LPSTR delims = The delimeter that signifies the seperation of the tokens in the string
// Out: LPSTR = Returns a pointer to the entire remaining string
//===========================================================================

LPSTR Tokenize(LPCSTR string, LPSTR buf, LPCSTR delims)
{
    NextToken(buf, &string, delims);
    return strcpy(tempBuf, string);
}

//===========================================================================
// API: BBTokenize
// Purpose: Assigns a specified number of string variables and outputs the remaining to a variable
// In: LPCSTR szString = The string to be parsed
// In: LPSTR lpszBuffers[] = A pointer to the location for the tokenized strings
// In: DWORD dwNumBuffers = The amount of tokens to be parsed
// In: LPSTR szExtraParameters = A pointer to the location for the remaining string
// Out: int = Number of tokens parsed
//===========================================================================

int BBTokenize (LPCSTR srcString, char **lpszBuffers, DWORD dwNumBuffers, LPSTR szExtraParameters)
{
    int   ol, stored; DWORD dwBufferCount;
    char  quoteChar, c, *output;
    quoteChar = 0; dwBufferCount = stored = 0;
    while (dwBufferCount < dwNumBuffers)
    {
        output = lpszBuffers[dwBufferCount]; ol = 0;
        while (0 != (c = *srcString))
        {
            srcString++;
            switch (c) {
                case ' ':
                case '\t':
                    if (quoteChar) goto _default;
                    if (ol) goto next;
                    continue;

                case '"':
                case '\'':
                    if (0==quoteChar) { quoteChar = c; continue; }
                    if (c==quoteChar) { quoteChar = 0; goto next; }

                _default:
                default:
                    output[ol]=c; ol++; continue;
            }
        }
        if (ol) next: stored++;
        output[ol]=0; dwBufferCount++;
    }
    if (szExtraParameters)
    {
        while (0 != (c = *srcString) && IS_SPC(c)) srcString++;
        strcpy(szExtraParameters, srcString);
    }
    return stored;
}

//===========================================================================
// API: StrRemoveEncap
// Purpose: Removes the first and last characters of a string
// In: LPSTR string = The string to be altered
// Out: LPSTR = A pointer to the altered string
//===========================================================================

LPSTR StrRemoveEncap(LPSTR string)
{
    int l = strlen(string);
    if (l>2) memmove(string, string+1, l-=2);
    else l=0;
    *(string+l) = '\0';
    return string;
}

//===========================================================================
// API: IsInString
// Purpose: Checks a given string to an occurance of the second string
// In: LPCSTR = string to search
// In: LPCSTR = string to search for
// Out: bool = found or not
//===========================================================================

bool IsInString(LPCSTR inputString, LPCSTR searchString)
{
    // xoblite-flavour plugins bad version test workaround
    if (0 == strcmp(searchString, "bb") && 0 == strcmp(inputString, GetBBVersion()))
        return false;

    return NULL != stristr(inputString, searchString);
}

//===========================================================================
// tiny cache reader: first checks, if the file had already been read, if not,
// reads the file into a malloc'ed buffer, then for each line separates
// keyword from value, cuts off leading and trailing spaces, strlwr's the
// keyword and adds both to a list of below defined structures, where k is
// the offset to the start of the value-string. Comments or other non-keyword
// lines have a "" keyword and the line as value.

// added checking for external updates by the user.

// added *wild*card* processing, it first looks for an exact match, if it
// cant find any, returns the first wildcard value, that matches, or null,
// if none...

//===========================================================================
// structures

struct lin_list
{
    struct lin_list *next;
    unsigned hash, k, o, v;
    bool is_wild;
    char str[2];
};

ST struct fil_list
{
    struct fil_list *next;
    struct lin_list *lines;
    struct list_node *wild;
    unsigned hash, k;
    FILETIME ft;
    DWORD tickcount;
    bool dirty;
    bool new_style;
    char path[1];
} *rc_files = NULL;

//===========================================================================

FILE *create_rcfile(const char *path)
{
    FILE *fp;
    //dbg_printf("writing to %s", path);
    if (NULL==(fp=fopen(path, "wt")))
        BBMessageBox(MB_OK, NLS2("$BBError_WriteFile$", "Error: Could not open \"%s\" for writing."), string_empty_or_null(path));
    return fp;
}

ST void write_rcfile(struct fil_list *fl)
{
    FILE *fp;
    if (NULL==(fp=create_rcfile(fl->path)))
        return;

    unsigned ml = 0;
    struct lin_list *tl;

    // calculate the max. keyword length
    dolist (tl, fl->lines) if (tl->k > ml) ml = tl->k;
    ml |= 3; // round to the next tabstop

    dolist (tl, fl->lines)
    {
        char *s = tl->str + tl->k;
        if (0 == *tl->str) fprintf (fp, "%s\n", s); //comment
        else fprintf (fp, "%-*s %s\n", ml, tl->str+tl->o, s);
    }
    fclose(fp);
    get_filetime(fl->path, &fl->ft);
    fl->dirty = false;
}

ST void mark_rc_dirty(struct fil_list *fl)
{
#ifdef BBOPT_DEVEL
    #pragma message("\n"__FILE__ "(370) : warning X0: Delayed Writing to rc-files enabled.\n")
    fl->dirty = true;
    SetTimer(BBhwnd, BB_WRITERC_TIMER, 2, NULL);
#else
    write_rcfile(fl);
#endif
}

//===========================================================================

ST void delete_from_list(struct fil_list *fl)
{
    freeall(&fl->lines);
    freeall(&fl->wild);
    remove_item(&rc_files, fl);
}

ST void flush_file(struct fil_list *fl)
{
    if (fl->dirty) write_rcfile(fl);
    delete_from_list(fl);
}

void write_rcfiles(void)
{
    struct fil_list *fl;
    dolist (fl, rc_files) if (fl->dirty) write_rcfile(fl);
}

void reset_reader(void)
{
    while (rc_files) flush_file(rc_files);
}

//===========================================================================
// helpers

unsigned calc_hash(char *p, const char *s, int *pLen)
{
    unsigned h, c; char *d = p;
    for (h = 0; 0 != (c = *s); ++s, ++d)
    {
       if (c <= 'Z' && c >= 'A') c += 32;
       *d = c;
       if ((h ^= c) & 1) h^=0xedb88320;
       h>>=1;
    }
    *d = 0;
    *pLen = d - p;
    return h;
}

// Xrm database-like wildcard pattern matcher
// ------------------------------------------
// returns: 0 for no match, else a number that is somehow a measure for
// 'how much' the items match. Btw 'toolbar*color' means just the
// same as 'toolbar.*.color' and both match e.g. 'toolbar.color'
// as well as 'toolbar.label.color', 'toolbar.button.pressed.color', ...

ST int xrm_match (const char *str, const char *pat)
{
    int c = 256, m = 0; const char *pp, *ss; char s, p; int l;
    for (;;)
    {
        s = *str, p = *pat;
        if (0 == s || ':' == s) return s == p ? m : 0;
        // scan component in the string
        for (l=1, ss=str+1; 0!=*ss && ':'!=*ss && '.'!=*ss++;++l);
        if ('*' == p)
        {
            // ignore dots with '*'
            for (pp = pat+1; '.'==*pp; ++pp);
            int n = xrm_match(ss, pat);
            if (n) return m + c + n*c/(256*2);
            pat = pp;
            continue;
        }
        // scan component in the pattern
        for (pp=pat+1; 0!=*pp && ':'!=*pp && '*'!=*pp && '.'!=*pp++;);
        if ('?' == p)
            m += c; // one point with matching wildcard
        else
        if (memcmp(str, pat, l))
            return 0; // zero for no match
        else
            m += 2*c; // two points for exact match
        str = ss; pat = pp; c /= 2;
    }
}

ST struct lin_list *make_line (struct fil_list *fl, const char *key, int k, const char *val)
{
    struct lin_list *tl; int v = strlen(val);
    tl=(struct lin_list*)m_alloc(sizeof(*tl) + k*2 + v + 1);
    tl->v = v++;
    tl->o = (tl->k = k+1) + v;
    memcpy(tl->str + tl->k, val, v);
    *((char*)memcpy(tl->str + tl->o, key, k) + k) = 0;
    tl->hash = calc_hash(tl->str, tl->str + tl->o, &k);
    tl->next = NULL;
    tl->is_wild = false;
    //if the key contains a wildcard
    if (k && memchr(key, '*', k) || memchr(key, '?', k))
    {
        // add it to the wildcard - list
        append_node(&fl->wild, new_node(tl));
        tl->is_wild = true;
    }
    return tl;
}

ST void free_line(struct fil_list *fl, struct lin_list *tl)
{
    if (tl->is_wild) delete_assoc(&fl->wild, tl);
    m_free(tl);
}

ST struct lin_list **search_line(struct lin_list **tlp, const char *key, struct list_node *wild)
{
    int n, m, k; char buff[256]; unsigned h = calc_hash(buff, key, &k);
    struct lin_list **wlp = NULL, *tl;
    while (NULL != (tl = *tlp))
    {
        if (tl->hash==h && 0==memcmp(tl->str, buff, k) && k)
            return tlp;
        tlp = &tl->next;
    }
    m = 0;
    while (NULL != wild)
    {
        tl = *(wlp = (struct lin_list **)&wild->v);
        n = xrm_match(buff, tl->str);
        if (n > m) tlp = wlp, m = n;
        wild = wild->next;
    }
    return tlp;
}

char * read_file_into_buffer (const char *path, int max_len)
{
    FILE *fp; char *buf; int k;
    if (NULL == (fp = fopen(path,"rb")))
        return NULL;

    fseek(fp,0,SEEK_END);
    k = ftell (fp);
    fseek (fp,0,SEEK_SET);
    if (max_len && k >= max_len) k = max_len-1;

    buf=(char*)m_alloc(k+1);
    fread (buf, 1, k, fp);
    fclose(fp);

    buf[k]=0;
    return buf;
}

bool is_stylefile(const char *path)
{
    char *temp = read_file_into_buffer(path, 10000);
    bool r = false;
    if (temp)
    {
        r = NULL != strstr(strlwr(temp), "menu.frame");
        m_free(temp);
    }
    return r;
}

char scan_line(char **pp, char **ss, int *ll)
{
    char c, e, *d, *s, *p = *pp;
    for (; 0!=(c=*p) && 10!=c && IS_SPC(c); p++);
    //find end of line
    for (s=p; 0!=(e=*p) && 10!=e; p++);
    //cut off trailing spaces
    for (d = p; d>s && IS_SPC(d[-1]); d--); *d=0;
    //ready for next line
    if (e) ++p;
    *pp = p, *ss = s, *ll = d-s;
    return c;
}

//===========================================================================
// BB070

void translate_items070(char *buffer, int bufsize, char **pkey, int *pklen)
{
    static const char *pairs [] =
    {
        // from         -->   to
        ".appearance"       , ""                ,
        "alignment"         , "justify"         ,
        "color1"            , "color"           ,
        "color2"            , "colorTo"         ,
        "backgroundColor"   , "color"           ,
        "foregroundColor"   , "picColor"        ,
        "disabledColor"     , "disableColor"    ,
        "menu.active"       , "menu.hilite"     ,
        NULL
    };

    int k = *pklen;
    if (k >= bufsize) return;
    *pkey = (char *)memcpy(buffer, *pkey, k);
    buffer[k] = 0;
    const char **p = pairs;
    do
    {
        char *q = (char*)stristr(buffer, *p);
        if (q)
        {
            int lp = strlen(p[0]);
            int lq = strlen(q);
            int lr = strlen(p[1]);
            int k0 = k + lr - lp;
            if (k0 >= bufsize) break;
            memmove(q + lr, q + lp, lq - lp + 1);
            memmove(q, p[1], lr);
            k = k0;
        }
    } while ((p += 2)[0]);
    *pklen = k;
}

//===========================================================================
// searches for the filename and, if not found, builds a _new line-list

ST struct fil_list *read_file(const char *filename)
{
    struct lin_list **slp, *sl;
    struct fil_list **flp, *fl;
    char *buf, *p, *d, *s, *t, c; int k;

    DWORD ticknow = GetTickCount();
    char path[MAX_PATH];
    unsigned h = calc_hash(path, filename, &k);

    // ----------------------------------------------
    // first check, if the file has already been read
    for (flp = &rc_files; NULL!=(fl=*flp); flp = &fl->next)
    {
        if (fl->hash==h && 0==memcmp(path, fl->path, 1+k))
        {
            // re-check the timestamp after 20 ms
            if (fl->tickcount + 20 < ticknow)
            {
                //dbg_printf("check time: %s", path);
                if (check_filetime(path, &fl->ft))
                {
                    // file was externally updated
                    delete_from_list(fl);
                    break;
                }
                fl->tickcount = ticknow;
            }
            return fl; //... return cached line list.
        }
    }

    // ----------------------------------------------

    // limit to 8 cached files
    fl = (struct fil_list *)nth_node(rc_files, 7);
    if (fl) while (fl->next) flush_file(fl->next);

    //dbg_printf("read file %s", path);

    // allocate a _new file structure
    fl = (struct fil_list*)c_alloc(sizeof(*fl) + k);
    cons_node(&rc_files, fl);

    memcpy(fl->path, path, k+1);
    fl->hash = h;
    fl->tickcount = ticknow;

    buf = read_file_into_buffer(path, 0);
    if (NULL == buf) return fl;

    //set timestamp
    get_filetime(fl->path, &fl->ft);

    slp = &fl->lines; p = buf;

    fl->new_style =
        0 == stricmp(fl->path, stylePath())
        && stristr(buf, "appearance:");

    for (;;)
    {
        c = scan_line(&p, &s, &k);
        if (0 == c) break;

        if (c == '#' || c == '!' || NULL == (d = (char*)memchr(s, ':', k)))
        {
            k=0, d = s;
        }
        else
        {   //skip spaces between key and value, replace tabs etc
            for (t = s; 0 != (c=*t); t++) if (IS_SPC(c)) *t = ' ';
            k = d - s + 1; while (*++d == ' ');
        }

        char buffer[100];
        if (fl->new_style && k)
            translate_items070(buffer, sizeof buffer, &s, &k);

        //put it into the line structure
        sl = make_line(fl, s, k, d);
        //append it to the list
        slp = &(*slp=sl)->next;
    }
    m_free(buf);
    return fl;
}

//===========================================================================
// API: FoundLastValue
// Purpose: Was the last read value actually in the rc-file?
// returns: 0=no 1=yes 2=wildcard matched
//===========================================================================

ST int found_last_value;

int FoundLastValue(void)
{
    return found_last_value;
}

//===========================================================================
// API: ReadValue
// Purpose: Searches the given file for the supplied keyword and returns a
// pointer to the value - string
// In: LPCSTR path = String containing the name of the file to be opened
// In: LPCSTR szKey = String containing the keyword to be looked for
// In: LONG ptr: optional: an index into the file to start search.
// Out: LPSTR = Pointer to the value string of the keyword
//===========================================================================

LPCSTR ReadValue(LPCSTR path, LPCSTR szKey, LONG *ptr)
{
    //static int rcc; dbg_printf("read %d %s", ++rcc, szKey);
    struct fil_list *fl = read_file(path);
    struct list_node *wild = fl->wild;
    struct lin_list *tl, *tl_start = fl->lines;
    if (ptr)
    {
        // originally meant as offset to seek into the file,
        // implemented as line index
        tl_start = (struct lin_list *)nth_node(tl_start, (*ptr)++);
        wild = NULL;
    }

    tl = tl_start;
    if (szKey[0])
    {
        tl = *search_line(&tl, szKey, wild);
        if (ptr) while (tl_start != tl) (*ptr)++, tl_start = tl_start->next;
    }

    if (NULL == tl)
    {
        found_last_value = 0;
        return NULL;
    }

    if (tl->is_wild)
        found_last_value = 2;
    else
        found_last_value = 1;

    return tl->str + tl->k;
}

void read_extension_style(LPCSTR path, LPCSTR szKey)
{
    if (szKey[0]){
        int l = strlen(szKey) - 1;
        struct fil_list *fl = read_file(path);
        struct lin_list *tl;
        dolist (tl, fl->lines){
            if (0 == strnicmp(tl->str, szKey, l)){
                if (char *p = strstr(tl->str, ".ext.")){
                    p = strrchr(tl->str, '.');

                    // check duplicate itme
                    bool bExist = false;
                    struct ext_list *el0;
                    dolist(el0, Settings_menuExtStyle){
                        if(!strnicmp(tl->str, el0->key, p - tl->str)){
                            bExist = true;
                            break;
                        }
                    }

                    if(!bExist){
                        struct ext_list *el = (struct ext_list*)c_alloc(sizeof(ext_list));
                        strncpy(el->key, tl->str, p - tl->str);
                        strcpy(el->ext, strrchr(el->key, '.'));
                        append_node(&Settings_menuExtStyle, el);
                    }
                }
            }
        }
    }
}

void free_extension_style(){
    freeall(&Settings_menuExtStyle);
}

bool is_newstyle(LPCSTR path)
{
    struct fil_list *fl = read_file(path);
    return fl && fl->new_style;
}

//===========================================================================
// Search for the szKey in the file_list, replace, if found and the value
// did change, or append, if not found. Write to file on changes.

int stri_eq_len(const char *a, const char *b)
{
    char c, d; int n = 0;
    for (;;++a, ++b)
    {
        if (0 == (c = *b)) return n;
        if (0 == (d = *a^c))
        {
            if ('.' == c || ':' == c) ++n;
            continue;
        }
        if (d != 32 || (c |= 32) < 'a' || c > 'z')
            return n;
    }
}

struct lin_list **longest_match(struct fil_list *fl, const char *key)
{
    struct lin_list **tlp = NULL, **slp, *sl;
    int n, m = 0;
    for (slp = &fl->lines; NULL!=(sl=*slp); slp = &sl->next)
    {
        n = stri_eq_len(sl->str, key);
        if (n >= m) m = n, tlp = &sl->next;
    }
    return tlp;
}

void WriteValue(LPCSTR path, LPCSTR szKey, LPCSTR value)
{
    //dbg_printf("WriteValue <%s> <%s> <%s>", path, szKey, value);

    struct fil_list *fl = read_file(path);
    struct lin_list *tl, **tlp;
    tl = *(tlp = search_line(&fl->lines, szKey, NULL));
    if (tl)
    {
        if (value && 0==strcmp(tl->str + tl->k, value))
            return; //if it didn't change, quit

        *tlp = tl->next; free_line(fl, tl);
    }
#if 1
    else
    {
        struct lin_list **slp = longest_match(fl, szKey);
        if (slp) tlp = slp;
    }
#endif
    if (value)
    {
        tl = make_line(fl, szKey, strlen(szKey), value);
        tl->next = *tlp;
        *tlp = tl;
    }
    mark_rc_dirty(fl);
}

//===========================================================================
// API: DeleteSetting

bool DeleteSetting(LPCSTR path, LPCSTR szKey)
{
    char buff[256]; int k;
    strlwr((char*)memcpy(buff, szKey, 1 + (k = strlen(szKey))));
    struct fil_list *fl = read_file(path);

    struct lin_list **slp, *sl; int dirty = 0;
    for (slp = &fl->lines; NULL!=(sl=*slp); )
    {
        //if (sl->k==1+k && 0==memcmp(sl->str, buff, k))
        if (('*' == *buff && 1==k) || xrm_match(sl->str, buff))
        {
            *slp = sl->next; free_line(fl, sl);
            ++dirty;
        }
        else
        {
            slp = &(*slp)->next;
        }
    }
    if (dirty) mark_rc_dirty(fl);
    return 0 != dirty;
}

//===========================================================================
// API: RenameSetting

bool RenameSetting(LPCSTR path, LPCSTR szKey, LPCSTR new_keyword)
{
    char buff[256]; unsigned k;
    strlwr((char*)memcpy(buff, szKey, 1 + (k = strlen(szKey))));
    struct fil_list *fl = read_file(path);

    struct lin_list **slp, *sl, *tl; int dirty = 0;
    for (slp = &fl->lines; NULL!=(sl=*slp); slp = &(*slp)->next)
    {
        if (sl->k==1+k && 0==memcmp(sl->str, buff, k))
        {
            tl = make_line(fl, new_keyword, strlen(new_keyword), sl->str+sl->k);
            tl->next = sl->next;
            *slp = tl; free_line(fl, sl);
            ++dirty;
        }
    }
    if (dirty) mark_rc_dirty(fl);
    return 0 != dirty;
}

//===========================================================================

// API: WriteString
void WriteString(LPCSTR fileName, LPCSTR szKey, LPCSTR value)
{
    WriteValue(fileName, szKey, value);
}

// API: WriteBool
void WriteBool(LPCSTR fileName, LPCSTR szKey, bool value)
{
    WriteValue(fileName, szKey, value ? "true" : "false");
}

// API: WriteInt
void WriteInt(LPCSTR fileName, LPCSTR szKey, int value)
{
    char buff[32];
    WriteValue(fileName, szKey, itoa(value, buff, 10));
}

// API: WriteColor
void WriteColor(LPCSTR fileName, LPCSTR szKey, COLORREF value)
{
    char buff[32];
    sprintf(buff, "#%06lx", (unsigned long)switch_rgb (value));
    WriteValue(fileName, szKey, buff);
}


//===========================================================================

// API: ReadBool
bool ReadBool(LPCSTR fileName, LPCSTR szKey, bool bDefault)
{
    LPCSTR szValue = ReadValue(fileName, szKey);
    if (szValue)
    {
        if (!stricmp(szValue, "true"))  return true;
        if (!stricmp(szValue, "false")) return false;
    }
    return bDefault;
}

// API: ReadInt
int ReadInt(LPCSTR fileName, LPCSTR szKey, int nDefault)
{
    LPCSTR szValue = ReadValue(fileName, szKey);
    return szValue ? atoi(szValue) : nDefault;
}

// API: ReadString
LPCSTR ReadString(LPCSTR fileName, LPCSTR szKey, LPCSTR szDefault)
{
    LPCSTR szValue = ReadValue(fileName, szKey);
    return szValue ? szValue : szDefault;
}

// API: ReadColor
COLORREF ReadColor(LPCSTR fileName, LPCSTR szKey, LPCSTR defaultString)
{
    LPCSTR szValue = szKey[0] ? ReadValue(fileName, szKey) : NULL;
    return ReadColorFromString(szValue ? szValue : defaultString);
}

//===========================================================================
// API: FileExists
// Purpose: Checks for a files existance
// In: LPCSTR = file to set as check for
// Out: bool = whether found or not
//===========================================================================

bool FileExists(LPCSTR szFileName)
{
    DWORD a = GetFileAttributes(szFileName);
    return (DWORD)-1 != a && 0 == (a & FILE_ATTRIBUTE_DIRECTORY);
}

//===========================================================================
// API: FileOpen
// Purpose: Opens file for parsing
// In: LPCSTR = file to open
// Out: FILE* = the file itself
//===========================================================================

FILE *FileOpen(LPCSTR szPath)
{
    // hack to prevent BBSlit from loading plugins, since they are
    // loaded by the built-in PluginManager.
    return strcmp(szPath, pluginrc_path) ? fopen(szPath, "rt") : NULL;
}

//===========================================================================
// API: FileClose
// Purpose: Close selected file
// In: FILE *fp = The file you wish to close
// Out: bool = Returns the status of completion
//===========================================================================

bool FileClose(FILE *fp)
{
    return fp && 0==fclose(fp);
}

//===========================================================================
// API: FileRead
// Purpose: Read's a line from given FILE and returns boolean on status
// In: FILE *stream = FILE to parse
// In: LPSTR string = Pointer to returned line from the file
// Out: bool = Status (true if read, false if not)
//===========================================================================

bool FileRead(FILE *fp, LPSTR buffer)
{
    return read_next_line(fp, buffer, 1024);
}

//===========================================================================
// API: ReadNextCommand
// Purpose: Reads the next line of the file
// In: FILE* = File to be parsed
// In: LPSTR = buffer to assign next line to
// DWORD = maximum size of line to be outputted
// Out: bool = whether there was a next line to parse
//===========================================================================

bool ReadNextCommand(FILE *fp, LPSTR szBuffer, DWORD dwLength)
{
    while (read_next_line(fp, szBuffer, dwLength))
    {
        char c = szBuffer[0];
        if (c && '#' != c && '!' != c) return true;
    }
    return false;
}

//===========================================================================

bool read_next_line(FILE *fp, LPSTR szBuffer, DWORD dwLength)
{
    if (fp && fgets(szBuffer, dwLength, fp))
    {
        char *p, *q, c; p = q = szBuffer;
        while (0 != (c = *p) && IS_SPC(c)) p++;
        while (0 != (c = *p)) *q++ = IS_SPC(c) ? ' ' : c, p++;
        while (q > szBuffer && IS_SPC(q[-1])) q--;
        *q = 0;
        return true;
    }
    szBuffer[0] = 0;
    return false;
}

//===========================================================================
// API: ReplaceShellFolders
// Purpose: replace shell folders in a string path, like DESKTOP\...
//===========================================================================

LPSTR ReplaceShellFolders(LPSTR string)
{
    return replace_shellfolders(string, string, false);
}

//===========================================================================
// API: ReplaceEnvVars
// Purpose: parses a given string and replaces all %VAR% with the environment
//          variable value if such a value exists
// In: LPSTR = string to replace env vars in
// Out: void = None
//===========================================================================

void ReplaceEnvVars(LPSTR string)
{
    char buffer[4096];
    DWORD r = ExpandEnvironmentStrings(string, buffer, sizeof buffer);
    if (r && r <= sizeof buffer) memcpy(string, buffer, r);
}

//===========================================================================
// API: GetBlackboxEditor
//===========================================================================

void GetBlackboxEditor(LPSTR editor)
{
    replace_shellfolders(editor, Settings_preferredEditor, true);
}

//===========================================================================
// API: FindConfigFile
// Purpose: Look for a config file in the Blackbox, UserAppData and
//          (optionally) plugin directories
// In:  LPSTR = pszOut = the location where to put the result
// In:  LPCSTR = filename to look for, LPCSTR = plugin directory or NULL
// Out: bool = true of found, FALSE otherwise
//===========================================================================

bool FindConfigFile(LPSTR pszOut, LPCSTR filename, LPCSTR pluginDir)
{
    char defaultPath[MAX_PATH]; defaultPath[0] = 0;

    if (is_relative_path(filename))
    {
/*
        // Look for the file in the $UserAppData$\Blackbox directory,
        char temp[MAX_PATH];
        sprintf(temp, "APPDATA\\Blackbox\\%s", filename);
        replace_shellfolders(pszOut, temp, false);
        if (FileExists(pszOut)) return true;
*/
        // If pluginDir is specified, we look for the file in this directory...
        if (pluginDir)
        {
            strcpy(pszOut, pluginDir);
            // skip back to the last slash and add filename
            strcpy((char*)get_file(pszOut), filename);
            if (FileExists(pszOut)) return true;

            // save default path
            strcpy(defaultPath, pszOut);
        }
    }

    // Look for the file in the Blackbox directory...
    replace_shellfolders(pszOut, filename, false);
    if (FileExists(pszOut)) return true;

    // If the plugin path has been set, copy it as default,
    // otherwise keep the Blackbox directory
    if (defaultPath[0]) strcpy(pszOut, defaultPath);

    // does not exist
    return false;
}

//===========================================================================
// API: ConfigFileExists
//===========================================================================

LPCSTR ConfigFileExists(LPCSTR filename, LPCSTR pluginDir)
{
    if (false == FindConfigFile(tempBuf, filename, pluginDir))
        tempBuf[0] = 0;
    return tempBuf;
}

//===========================================================================

ST LPCSTR bbPath(LPCSTR other_name, LPSTR path, LPCSTR name)
{
    if (other_name)
    {
        path[0] = 0;
        if (other_name[0])
            FindConfigFile(path, other_name, NULL);
    }
    else
    if (0 == path[0])
    {
        char file_dot_rc[MAX_PATH];
        sprintf(file_dot_rc, "%s.rc", name);
        if (false == FindConfigFile(path, file_dot_rc, NULL))
        {
            char file_rc[MAX_PATH];
            sprintf(file_rc, "%src", name);
            if (false == FindConfigFile(path, file_rc, NULL))
            {
                FindConfigFile(path, file_dot_rc, NULL);
            }
        }
    }
    //dbg_printf("other <%s>  path <%s>  name <%s>", other_name, path, name);
    return path;
}

//===========================================================================
// API: bbrcPath
// Purpose: Returns the handle to the blackboxrc file that is being used
// In: LPCSTR = file to set as default bbrc file (defaults to NULL if not specified)
// Out: LPCSTR = full path of file
//===========================================================================

LPCSTR bbrcPath(LPCSTR other)
{
    return bbPath (other, blackboxrc_path, "blackbox");
}

//===========================================================================
// API: extensionsrcPath
// Purpose: Returns the handle to the extensionsrc file that is being used
// In: LPCSTR = file to set as default extensionsrc file (defaults to NULL if not specified)
// Out: LPCSTR = full path of file
//===========================================================================

LPCSTR extensionsrcPath(LPCSTR other)
{
    return bbPath (other, extensionsrc_path, "extensions");
}

//===========================================================================
// API: plugrcPath
// In: LPCSTR = file to set as default plugins rc file (defaults to NULL if not specified)
// Purpose: Returns the handle to the plugins rc file that is being used
// Out: LPCSTR = full path of file
//===========================================================================

LPCSTR plugrcPath(LPCSTR other)
{
    return bbPath (other, pluginrc_path, "plugins");
}

//===========================================================================
// API: menuPath
// Purpose: Returns the handle to the menu file that is being used
// In: LPCSTR = file to set as default menu file (defaults to NULL if not specified)
// Out: LPCSTR = full path of file
//===========================================================================

LPCSTR menuPath(LPCSTR other)
{
    return bbPath (other, menurc_path, "menu");
}

//===========================================================================
// API: stylePath
// Purpose: Returns the handle to the style file that is being used
// In: LPCSTR = file to set as default style file (defaults to NULL if not specified)
// Out: LPCSTR = full path of file
//===========================================================================

LPCSTR stylePath(LPCSTR other)
{
    return bbPath (other, stylerc_path, "style");
}

//===========================================================================
// Function: ParseLiteralColor
// Purpose: Parses a given literal colour and returns the hex value
// In: LPCSTR = color to parse (eg. "black", "white")
// Out: COLORREF (DWORD) of rgb value
// (old)Out: LPCSTR = literal hex value
//===========================================================================

ST struct litcolor1 { char *cname; COLORREF cref; } litcolor1_ary[] = {

    { "ghostwhite", RGB(248,248,255) },
    { "whitesmoke", RGB(245,245,245) },
    { "gainsboro", RGB(220,220,220) },
    { "floralwhite", RGB(255,250,240) },
    { "oldlace", RGB(253,245,230) },
    { "linen", RGB(250,240,230) },
    { "antiquewhite", RGB(250,235,215) },
    { "papayawhip", RGB(255,239,213) },
    { "blanchedalmond", RGB(255,235,205) },
    { "bisque", RGB(255,228,196) },
    { "peachpuff", RGB(255,218,185) },
    { "navajowhite", RGB(255,222,173) },
    { "moccasin", RGB(255,228,181) },
    { "cornsilk", RGB(255,248,220) },
    { "ivory", RGB(255,255,240) },
    { "lemonchiffon", RGB(255,250,205) },
    { "seashell", RGB(255,245,238) },
    { "honeydew", RGB(240,255,240) },
    { "mintcream", RGB(245,255,250) },
    { "azure", RGB(240,255,255) },
    { "aliceblue", RGB(240,248,255) },
    { "lavender", RGB(230,230,250) },
    { "lavenderblush", RGB(255,240,245) },
    { "mistyrose", RGB(255,228,225) },
    { "white", RGB(255,255,255) },
    { "black", RGB(0,0,0) },
    { "darkslategray", RGB(47,79,79) },
    { "dimgray", RGB(105,105,105) },
    { "slategray", RGB(112,128,144) },
    { "lightslategray", RGB(119,136,153) },
    { "gray", RGB(190,190,190) },
    { "lightgray", RGB(211,211,211) },
    { "midnightblue", RGB(25,25,112) },
    { "navy", RGB(0,0,128) },
    { "navyblue", RGB(0,0,128) },
    { "cornflowerblue", RGB(100,149,237) },
    { "darkslateblue", RGB(72,61,139) },
    { "slateblue", RGB(106,90,205) },
    { "mediumslateblue", RGB(123,104,238) },
    { "lightslateblue", RGB(132,112,255) },
    { "mediumblue", RGB(0,0,205) },
    { "royalblue", RGB(65,105,225) },
    { "blue", RGB(0,0,255) },
    { "dodgerblue", RGB(30,144,255) },
    { "deepskyblue", RGB(0,191,255) },
    { "skyblue", RGB(135,206,235) },
    { "lightskyblue", RGB(135,206,250) },
    { "steelblue", RGB(70,130,180) },
    { "lightsteelblue", RGB(176,196,222) },
    { "lightblue", RGB(173,216,230) },
    { "powderblue", RGB(176,224,230) },
    { "paleturquoise", RGB(175,238,238) },
    { "darkturquoise", RGB(0,206,209) },
    { "mediumturquoise", RGB(72,209,204) },
    { "turquoise", RGB(64,224,208) },
    { "cyan", RGB(0,255,255) },
    { "lightcyan", RGB(224,255,255) },
    { "cadetblue", RGB(95,158,160) },
    { "mediumaquamarine", RGB(102,205,170) },
    { "aquamarine", RGB(127,255,212) },
    { "darkgreen", RGB(0,100,0) },
    { "darkolivegreen", RGB(85,107,47) },
    { "darkseagreen", RGB(143,188,143) },
    { "seagreen", RGB(46,139,87) },
    { "mediumseagreen", RGB(60,179,113) },
    { "lightseagreen", RGB(32,178,170) },
    { "palegreen", RGB(152,251,152) },
    { "springgreen", RGB(0,255,127) },
    { "lawngreen", RGB(124,252,0) },
    { "green", RGB(0,255,0) },
    { "chartreuse", RGB(127,255,0) },
    { "mediumspringgreen", RGB(0,250,154) },
    { "greenyellow", RGB(173,255,47) },
    { "limegreen", RGB(50,205,50) },
    { "yellowgreen", RGB(154,205,50) },
    { "forestgreen", RGB(34,139,34) },
    { "olivedrab", RGB(107,142,35) },
    { "darkkhaki", RGB(189,183,107) },
    { "khaki", RGB(240,230,140) },
    { "palegoldenrod", RGB(238,232,170) },
    { "lightgoldenrodyellow", RGB(250,250,210) },
    { "lightyellow", RGB(255,255,224) },
    { "yellow", RGB(255,255,0) },
    { "gold", RGB(255,215,0) },
    { "lightgoldenrod", RGB(238,221,130) },
    { "goldenrod", RGB(218,165,32) },
    { "darkgoldenrod", RGB(184,134,11) },
    { "rosybrown", RGB(188,143,143) },
    { "indianred", RGB(205,92,92) },
    { "saddlebrown", RGB(139,69,19) },
    { "sienna", RGB(160,82,45) },
    { "peru", RGB(205,133,63) },
    { "burlywood", RGB(222,184,135) },
    { "beige", RGB(245,245,220) },
    { "wheat", RGB(245,222,179) },
    { "sandybrown", RGB(244,164,96) },
    { "tan", RGB(210,180,140) },
    { "chocolate", RGB(210,105,30) },
    { "firebrick", RGB(178,34,34) },
    { "brown", RGB(165,42,42) },
    { "darksalmon", RGB(233,150,122) },
    { "salmon", RGB(250,128,114) },
    { "lightsalmon", RGB(255,160,122) },
    { "orange", RGB(255,165,0) },
    { "darkorange", RGB(255,140,0) },
    { "coral", RGB(255,127,80) },
    { "lightcoral", RGB(240,128,128) },
    { "tomato", RGB(255,99,71) },
    { "orangered", RGB(255,69,0) },
    { "red", RGB(255,0,0) },
    { "hotpink", RGB(255,105,180) },
    { "deeppink", RGB(255,20,147) },
    { "pink", RGB(255,192,203) },
    { "lightpink", RGB(255,182,193) },
    { "palevioletred", RGB(219,112,147) },
    { "maroon", RGB(176,48,96) },
    { "mediumvioletred", RGB(199,21,133) },
    { "violetred", RGB(208,32,144) },
    { "magenta", RGB(255,0,255) },
    { "violet", RGB(238,130,238) },
    { "plum", RGB(221,160,221) },
    { "orchid", RGB(218,112,214) },
    { "mediumorchid", RGB(186,85,211) },
    { "darkorchid", RGB(153,50,204) },
    { "darkviolet", RGB(148,0,211) },
    { "blueviolet", RGB(138,43,226) },
    { "purple", RGB(160,32,240) },
    { "mediumpurple", RGB(147,112,219) },
    { "thistle", RGB(216,191,216) },

    { "darkgray", RGB(169,169,169) },
    { "darkblue", RGB(0,0,139) },
    { "darkcyan", RGB(0,139,139) },
    { "darkmagenta", RGB(139,0,139) },
    { "darkred", RGB(139,0,0) },
    { "lightgreen", RGB(144,238,144) },
    { NULL, CLR_INVALID }
    };

ST struct litcolor4 { char *cname; COLORREF cref[4]; } litcolor4_ary[] = {

    { "snow", { RGB(255,250,250), RGB(238,233,233), RGB(205,201,201), RGB(139,137,137) }},
    { "seashell", { RGB(255,245,238), RGB(238,229,222), RGB(205,197,191), RGB(139,134,130) }},
    { "antiquewhite", { RGB(255,239,219), RGB(238,223,204), RGB(205,192,176), RGB(139,131,120) }},
    { "bisque", { RGB(255,228,196), RGB(238,213,183), RGB(205,183,158), RGB(139,125,107) }},
    { "peachpuff", { RGB(255,218,185), RGB(238,203,173), RGB(205,175,149), RGB(139,119,101) }},
    { "navajowhite", { RGB(255,222,173), RGB(238,207,161), RGB(205,179,139), RGB(139,121,94) }},
    { "lemonchiffon", { RGB(255,250,205), RGB(238,233,191), RGB(205,201,165), RGB(139,137,112) }},
    { "cornsilk", { RGB(255,248,220), RGB(238,232,205), RGB(205,200,177), RGB(139,136,120) }},
    { "ivory", { RGB(255,255,240), RGB(238,238,224), RGB(205,205,193), RGB(139,139,131) }},
    { "honeydew", { RGB(240,255,240), RGB(224,238,224), RGB(193,205,193), RGB(131,139,131) }},
    { "lavenderblush", { RGB(255,240,245), RGB(238,224,229), RGB(205,193,197), RGB(139,131,134) }},
    { "mistyrose", { RGB(255,228,225), RGB(238,213,210), RGB(205,183,181), RGB(139,125,123) }},
    { "azure", { RGB(240,255,255), RGB(224,238,238), RGB(193,205,205), RGB(131,139,139) }},
    { "slateblue", { RGB(131,111,255), RGB(122,103,238), RGB(105,89,205), RGB(71,60,139) }},
    { "royalblue", { RGB(72,118,255), RGB(67,110,238), RGB(58,95,205), RGB(39,64,139) }},
    { "blue", { RGB(0,0,255), RGB(0,0,238), RGB(0,0,205), RGB(0,0,139) }},
    { "dodgerblue", { RGB(30,144,255), RGB(28,134,238), RGB(24,116,205), RGB(16,78,139) }},
    { "steelblue", { RGB(99,184,255), RGB(92,172,238), RGB(79,148,205), RGB(54,100,139) }},
    { "deepskyblue", { RGB(0,191,255), RGB(0,178,238), RGB(0,154,205), RGB(0,104,139) }},
    { "skyblue", { RGB(135,206,255), RGB(126,192,238), RGB(108,166,205), RGB(74,112,139) }},
    { "lightskyblue", { RGB(176,226,255), RGB(164,211,238), RGB(141,182,205), RGB(96,123,139) }},
    { "slategray", { RGB(198,226,255), RGB(185,211,238), RGB(159,182,205), RGB(108,123,139) }},
    { "lightsteelblue", { RGB(202,225,255), RGB(188,210,238), RGB(162,181,205), RGB(110,123,139) }},
    { "lightblue", { RGB(191,239,255), RGB(178,223,238), RGB(154,192,205), RGB(104,131,139) }},
    { "lightcyan", { RGB(224,255,255), RGB(209,238,238), RGB(180,205,205), RGB(122,139,139) }},
    { "paleturquoise", { RGB(187,255,255), RGB(174,238,238), RGB(150,205,205), RGB(102,139,139) }},
    { "cadetblue", { RGB(152,245,255), RGB(142,229,238), RGB(122,197,205), RGB(83,134,139) }},
    { "turquoise", { RGB(0,245,255), RGB(0,229,238), RGB(0,197,205), RGB(0,134,139) }},
    { "cyan", { RGB(0,255,255), RGB(0,238,238), RGB(0,205,205), RGB(0,139,139) }},
    { "darkslategray", { RGB(151,255,255), RGB(141,238,238), RGB(121,205,205), RGB(82,139,139) }},
    { "aquamarine", { RGB(127,255,212), RGB(118,238,198), RGB(102,205,170), RGB(69,139,116) }},
    { "darkseagreen", { RGB(193,255,193), RGB(180,238,180), RGB(155,205,155), RGB(105,139,105) }},
    { "seagreen", { RGB(84,255,159), RGB(78,238,148), RGB(67,205,128), RGB(46,139,87) }},
    { "palegreen", { RGB(154,255,154), RGB(144,238,144), RGB(124,205,124), RGB(84,139,84) }},
    { "springgreen", { RGB(0,255,127), RGB(0,238,118), RGB(0,205,102), RGB(0,139,69) }},
    { "green", { RGB(0,255,0), RGB(0,238,0), RGB(0,205,0), RGB(0,139,0) }},
    { "chartreuse", { RGB(127,255,0), RGB(118,238,0), RGB(102,205,0), RGB(69,139,0) }},
    { "olivedrab", { RGB(192,255,62), RGB(179,238,58), RGB(154,205,50), RGB(105,139,34) }},
    { "darkolivegreen", { RGB(202,255,112), RGB(188,238,104), RGB(162,205,90), RGB(110,139,61) }},
    { "khaki", { RGB(255,246,143), RGB(238,230,133), RGB(205,198,115), RGB(139,134,78) }},
    { "lightgoldenrod", { RGB(255,236,139), RGB(238,220,130), RGB(205,190,112), RGB(139,129,76) }},
    { "lightyellow", { RGB(255,255,224), RGB(238,238,209), RGB(205,205,180), RGB(139,139,122) }},
    { "yellow", { RGB(255,255,0), RGB(238,238,0), RGB(205,205,0), RGB(139,139,0) }},
    { "gold", { RGB(255,215,0), RGB(238,201,0), RGB(205,173,0), RGB(139,117,0) }},
    { "goldenrod", { RGB(255,193,37), RGB(238,180,34), RGB(205,155,29), RGB(139,105,20) }},
    { "darkgoldenrod", { RGB(255,185,15), RGB(238,173,14), RGB(205,149,12), RGB(139,101,8) }},
    { "rosybrown", { RGB(255,193,193), RGB(238,180,180), RGB(205,155,155), RGB(139,105,105) }},
    { "indianred", { RGB(255,106,106), RGB(238,99,99), RGB(205,85,85), RGB(139,58,58) }},
    { "sienna", { RGB(255,130,71), RGB(238,121,66), RGB(205,104,57), RGB(139,71,38) }},
    { "burlywood", { RGB(255,211,155), RGB(238,197,145), RGB(205,170,125), RGB(139,115,85) }},
    { "wheat", { RGB(255,231,186), RGB(238,216,174), RGB(205,186,150), RGB(139,126,102) }},
    { "tan", { RGB(255,165,79), RGB(238,154,73), RGB(205,133,63), RGB(139,90,43) }},
    { "chocolate", { RGB(255,127,36), RGB(238,118,33), RGB(205,102,29), RGB(139,69,19) }},
    { "firebrick", { RGB(255,48,48), RGB(238,44,44), RGB(205,38,38), RGB(139,26,26) }},
    { "brown", { RGB(255,64,64), RGB(238,59,59), RGB(205,51,51), RGB(139,35,35) }},
    { "salmon", { RGB(255,140,105), RGB(238,130,98), RGB(205,112,84), RGB(139,76,57) }},
    { "lightsalmon", { RGB(255,160,122), RGB(238,149,114), RGB(205,129,98), RGB(139,87,66) }},
    { "orange", { RGB(255,165,0), RGB(238,154,0), RGB(205,133,0), RGB(139,90,0) }},
    { "darkorange", { RGB(255,127,0), RGB(238,118,0), RGB(205,102,0), RGB(139,69,0) }},
    { "coral", { RGB(255,114,86), RGB(238,106,80), RGB(205,91,69), RGB(139,62,47) }},
    { "tomato", { RGB(255,99,71), RGB(238,92,66), RGB(205,79,57), RGB(139,54,38) }},
    { "orangered", { RGB(255,69,0), RGB(238,64,0), RGB(205,55,0), RGB(139,37,0) }},
    { "red", { RGB(255,0,0), RGB(238,0,0), RGB(205,0,0), RGB(139,0,0) }},
    { "deeppink", { RGB(255,20,147), RGB(238,18,137), RGB(205,16,118), RGB(139,10,80) }},
    { "hotpink", { RGB(255,110,180), RGB(238,106,167), RGB(205,96,144), RGB(139,58,98) }},
    { "pink", { RGB(255,181,197), RGB(238,169,184), RGB(205,145,158), RGB(139,99,108) }},
    { "lightpink", { RGB(255,174,185), RGB(238,162,173), RGB(205,140,149), RGB(139,95,101) }},
    { "palevioletred", { RGB(255,130,171), RGB(238,121,159), RGB(205,104,137), RGB(139,71,93) }},
    { "maroon", { RGB(255,52,179), RGB(238,48,167), RGB(205,41,144), RGB(139,28,98) }},
    { "violetred", { RGB(255,62,150), RGB(238,58,140), RGB(205,50,120), RGB(139,34,82) }},
    { "magenta", { RGB(255,0,255), RGB(238,0,238), RGB(205,0,205), RGB(139,0,139) }},
    { "orchid", { RGB(255,131,250), RGB(238,122,233), RGB(205,105,201), RGB(139,71,137) }},
    { "plum", { RGB(255,187,255), RGB(238,174,238), RGB(205,150,205), RGB(139,102,139) }},
    { "mediumorchid", { RGB(224,102,255), RGB(209,95,238), RGB(180,82,205), RGB(122,55,139) }},
    { "darkorchid", { RGB(191,62,255), RGB(178,58,238), RGB(154,50,205), RGB(104,34,139) }},
    { "purple", { RGB(155,48,255), RGB(145,44,238), RGB(125,38,205), RGB(85,26,139) }},
    { "mediumpurple", { RGB(171,130,255), RGB(159,121,238), RGB(137,104,205), RGB(93,71,139) }},
    { "thistle", { RGB(255,225,255), RGB(238,210,238), RGB(205,181,205), RGB(139,123,139) }},
    { NULL, { CLR_INVALID, CLR_INVALID, CLR_INVALID, CLR_INVALID }}
    };

COLORREF ParseLiteralColor(LPCSTR colour)
{
    int i; unsigned l; char *p, c, buf[32];
    l = strlen(colour) + 1;
    if (l > 2 && l < sizeof buf)
    {
        memcpy(buf, colour, l); //strlwr(buf);
        while (NULL!=(p=strchr(buf,' '))) strcpy(p, p+1);
        if (NULL!=(p=strstr(buf,"grey"))) p[2]='a';
        if (0==memcmp(buf,"gray", 4) && (c=buf[4]) >= '0' && c <= '9')
        {
            i = atoi(buf+4);
            if (i >= 0 && i <= 100)
            {
                i = (i * 255 + 50) / 100;
                return rgb(i,i,i);
            }
        }
        i = *(p = &buf[l-2]) - '1';
        if (i>=0 && i<4)
        {
            *p=0; --l;
            struct litcolor4 *cp4=litcolor4_ary;
            do if (0==memcmp(buf, cp4->cname, l)) return cp4->cref[i];
            while ((++cp4)->cname);
        }
        else
        {
            struct litcolor1 *cp1=litcolor1_ary;
            do if (0==memcmp(buf, cp1->cname, l)) return cp1->cref;
            while ((++cp1)->cname);
        }
    }
    return CLR_INVALID;
}

//===========================================================================
// Function: ReadColorFromString
// Purpose: parse a literal or hexadezimal color string


COLORREF ReadColorFromString(LPCSTR string)
{
    if (NULL == string) return CLR_INVALID;
    char stub[256], rgbstr[7];
    char *s = strlwr(unquote(stub, string));
    if ('#'==*s) s++;
    for (;;)
    {
        COLORREF cr = 0; char *d, c;
        // check if its a valid hex number
        for (d = s; (c = *d) != 0; ++d)
        {
            cr <<= 4;
            if (c >= '0' && c <= '9') cr |= c - '0';
            else
            if (c >= 'a' && c <= 'f') cr |= c - ('a'-10);
            else goto check_rgb;
        }

        if (d - s == 3) // #AB4 short type colors
            cr = (cr&0xF00)<<12 | (cr&0xFF0)<<8 | (cr&0x0FF)<<4 | cr&0x00F;

        return switch_rgb(cr);

check_rgb:
        // check if its an "rgb:12/ee/4c" type string
        s = stub;
        if (0 == memcmp(s, "rgb:", 4))
        {
            int j=3; s+=4; d = rgbstr;
            do {
                d[0] = *s && '/'!=*s ? *s++ : '0';
                d[1] = *s && '/'!=*s ? *s++ : d[0];
                d+=2; if ('/'==*s) ++s;
            } while (--j);
            *d=0; s = rgbstr;
            continue;
        }

        // must be one of the literal color names (or is invalid)
        return ParseLiteralColor(s);
    }
}

//===========================================================================
// API: Log
// Purpose: Appends given line to Blackbox.log file
//===========================================================================

void Log(LPCSTR Title, LPCSTR Line)
{
    //log_printf(1, "%s: %s", Title, Line);
}

//===========================================================================
// API: MBoxErrorFile
// Purpose: Gives a message box proclaming missing file
// In: LPCSTR = missing file
// Out: int = return value of messagebox
//===========================================================================

int MBoxErrorFile(LPCSTR szFile)
{
    return BBMessageBox(MB_OK, NLS2("$BBError_ReadFile$",
        "Error: Unable to open file \"%s\"."
        "\nPlease check location and try again."
        ), szFile);
}

//===========================================================================
// API: MBoxErrorValue
// Purpose: Gives a message box proclaming a given value
// In: LPCSTR = value
// Out: int = return value of messagebox
//===========================================================================

int MBoxErrorValue(LPCSTR szValue)
{
    return BBMessageBox(MB_OK, NLS2("$BBError_MsgBox$",
        "Error: %s"), szValue);
}

//===========================================================================
// API: BBExecute
// Purpose: A safe execute routine
// In: HWND = owner
// In: LPCSTR = operation (eg. "open")
// In: LPCSTR = command to run
// In: LPCSTR = arguments
// In: LPCSTR = directory to run from
// In: int = show status
// In: bool = suppress error messages
// //Out: HINSTANCE = instance of file running
//===========================================================================

BOOL BBExecute(
    HWND Owner,
    LPCSTR szOperation,
    LPCSTR szCommand, LPCSTR szArgs, LPCSTR szDirectory,
    int nShowCmd, bool noErrorMsgs)
{
    if (szCommand[0])
    {
        SHELLEXECUTEINFO sei; ZeroMemory(&sei, sizeof(sei));
        sei.cbSize       = sizeof(sei);
        sei.hwnd         = Owner;
        sei.lpVerb       = szOperation;
        sei.lpFile       = szCommand;
        sei.lpParameters = szArgs;
        sei.lpDirectory  = szDirectory;
        sei.nShow        = nShowCmd;
        sei.fMask        = SEE_MASK_DOENVSUBST | SEE_MASK_FLAG_NO_UI;

        if (ShellExecuteEx(&sei))
            return TRUE;
    }
    if (!noErrorMsgs)
        BBMessageBox(MB_OK, NLS2("$BBError_Execute$",
            "Error: Could not execute:"
            "\nCommand:  \t%s"
            "\nOperation:\t%s"
            "\nArguments:\t%s"
            "\nWorking Directory:\t%s"),
            string_empty_or_null(szCommand),
            string_empty_or_null(szOperation),
            string_empty_or_null(szArgs),
            string_empty_or_null(szDirectory)
            );

    return FALSE;
}

//===========================================================================

BOOL BBExecute_command(const char *command, const char *arguments, bool no_errors)
{
    char parsed_command[MAX_PATH];
    replace_shellfolders(parsed_command, command, true);
    char workdir[MAX_PATH];
    get_directory(workdir, parsed_command);
    return BBExecute(NULL, NULL, parsed_command, arguments, workdir, SW_SHOWNORMAL, no_errors);
}

BOOL BBExecute_string(const char *command, bool no_errors)
{
    char path[MAX_PATH];
    NextToken(path, &command);
    return BBExecute_command(path, command, no_errors);
}

bool ShellCommand(const char *command, const char *work_dir, bool wait)
{
    SHELLEXECUTEINFO sei; char path[MAX_PATH]; BOOL r;
    NextToken(path, &command);
    ZeroMemory(&sei, sizeof sei);
    sei.cbSize = sizeof sei;
    sei.fMask = SEE_MASK_DOENVSUBST;
    if (wait) sei.fMask |= SEE_MASK_NOCLOSEPROCESS;
    sei.lpFile = path;
    sei.lpParameters = command;
    sei.lpDirectory = work_dir;
    sei.nShow = SW_SHOWNORMAL;
    r = ShellExecuteEx(&sei);
    if (r && wait)
    {
        WaitForSingleObject(sei.hProcess, INFINITE);
        CloseHandle(sei.hProcess);
    }
    return r;
}

//===========================================================================
// API: IsAppWindow
// Purpose: checks given hwnd to see if it's an app
// In: HWND = hwnd to check
// Out: bool = if app or not
//===========================================================================
// This is used to populate the task list in case bb is started manually.

bool IsAppWindow(HWND hwnd)
{
    LONG nStyle, nExStyle; HWND hOwner; HWND hParent;

    if (!IsWindow(hwnd))
        return false;
    
    if (CheckSticky(hwnd))
        return false;

    nStyle = GetWindowLong(hwnd, GWL_STYLE);    

    // if it is a WS_CHILD or not WS_VISIBLE, fail it
    if((nStyle & WS_CHILD) || !(nStyle & WS_VISIBLE) || (nStyle & WS_DISABLED))
        return false;

    nExStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
    // if the window is a WS_EX_TOOLWINDOW fail it
    if((nExStyle & WS_EX_TOOLWINDOW) && !(nExStyle & WS_EX_APPWINDOW))
        return false;

    // If this has a parent, then only accept this window
    // if the parent is not accepted
    hParent = GetParent(hwnd);
    if (hParent && IsAppWindow(hParent))
        return false;
    
    // If this has an owner, then only accept this window
    // if the parent is not accepted
    hOwner = GetWindow(hwnd, GW_OWNER);
    if(hOwner && IsAppWindow(hOwner))
        return false;
    
    return true;
}

//===========================================================================
// API: MakeStyleGradient
// Purpose:  Make a gradient from style Item
//===========================================================================

void MakeStyleGradient(HDC hdc, RECT *rp, StyleItem * pSI, bool withBorder)
{
    COLORREF borderColor; int borderWidth;
    if (withBorder)
    {
        if (pSI->bordered)
        {
            borderColor = pSI->borderColor;
            borderWidth = pSI->borderWidth;
        }
        else
        {
            borderColor = mStyle.borderColor;
            borderWidth = mStyle.borderWidth;
        }
    }
    else
    {
        borderColor = 0;
        borderWidth = 0;
    }

    MakeGradientEx(hdc, *rp,
        pSI->parentRelative ? -1 : pSI->type,
        pSI->Color,
        pSI->ColorTo,
        pSI->ColorSplitTo,
        pSI->ColorToSplitTo,
        pSI->interlaced,
        pSI->bevelstyle,
        pSI->bevelposition,
        0,
        borderColor,
        borderWidth
        );
}

//===========================================================================
// API: ParseItem
// Purpose: parses a given string and assigns settings to a StyleItem class
// In: LPCSTR = item to parse out
// In: StyleItem* = class to assign values to
// Out: void = None
//===========================================================================

struct styleprop { char *key; int  val; };

ST struct styleprop styleprop_1[] = {
    {"flat"        ,BEVEL_FLAT           },
    {"raised"      ,BEVEL_RAISED         },
    {"sunken"      ,BEVEL_SUNKEN         },
    {NULL          ,BEVEL_RAISED         }
    };

ST struct styleprop styleprop_2[] = {
    {"bevel1"      ,BEVEL1         },
    {"bevel2"      ,BEVEL2         },
    {"bevel3"      ,3              },
    {NULL          ,BEVEL1         }
    };

ST struct styleprop styleprop_3[] = {
    {"solid"           ,B_SOLID            },
    {"splithorizontal" ,B_SPLITHORIZONTAL  }, // "horizontal" is match .*horizontal
    {"mirrorhorizontal",B_MIRRORHORIZONTAL },
    {"horizontal"      ,B_HORIZONTAL       },
    {"splitvertical"   ,B_SPLITVERTICAL    }, // "vertical" is match .*vertical
    {"mirrorvertical"  ,B_MIRRORVERTICAL   },
    {"vertical"        ,B_VERTICAL         },
    {"crossdiagonal"   ,B_CROSSDIAGONAL    },
    {"diagonal"        ,B_DIAGONAL         },
    {"pipecross"       ,B_PIPECROSS        },
    {"elliptic"        ,B_ELLIPTIC         },
    {"rectangle"       ,B_RECTANGLE        },
    {"pyramid"         ,B_PYRAMID          },
    {NULL              ,-1                 }
    };

ST int check_item(const char *p, struct styleprop *s)
{
    do if (strstr(p, s->key)) break; while ((++s)->key);
    return s->val;
}

int ParseType(char *buf)
{
    return check_item(buf, styleprop_3);
}

void ParseItem(LPCSTR szItem, StyleItem *item)
{
    char buf[256]; int t;
    strlwr(strcpy(buf, szItem));
    item->bevelstyle = check_item(buf, styleprop_1);
    item->bevelposition = BEVEL_FLAT == item->bevelstyle ? 0 : check_item(buf, styleprop_2);
    t = check_item(buf, styleprop_3);
    item->type = (-1 != t) ? t : B_SOLID;
    item->interlaced = NULL!=strstr(buf, "interlaced");
    item->parentRelative = NULL!=strstr(buf, "parentrelative");
    //if (item->nVersion >= 2) item->bordered = NULL!=strstr(buf, "border");
}

//===========================================================================
// API: MakeSticky
// API: RemoveSticky
// API: CheckSticky
// Purpose:  make a plugin-window appear on all workspaces
//===========================================================================

ST struct hwnd_list * stickyhwl;

void MakeSticky(HWND window)
{
    if (window && NULL == assoc(stickyhwl, window))
        cons_node(&stickyhwl, new_node(window));
    //dbg_window(window, "makesticky");
}

void RemoveSticky(HWND window)
{
    delete_assoc(&stickyhwl, window);
    //dbg_window(window, "removesticky");
}

bool CheckSticky(HWND window)
{
    if (assoc(stickyhwl, window))
        return true;

    // for bbPager
    if (GetWindowLongPtr(window, GWLP_USERDATA) == 0x49474541 /*magicDWord*/)
        return true;
    
    return false;
}

//================================
// non api
void ClearSticky()
{
    freeall(&stickyhwl);
}

//===========================================================================
// API: GetUnderExplorer
//===========================================================================

bool GetUnderExplorer()
{
    return underExplorer;
}

//===========================================================================
// API: SetTransparency
// Purpose: Wrapper, win9x conpatible
// In:      HWND, alpha
// Out:     bool
//===========================================================================

BOOL (WINAPI *pSetLayeredWindowAttributes)(HWND, COLORREF, BYTE, DWORD);

bool SetTransparency(HWND hwnd, BYTE alpha)
{
    //dbg_window(hwnd, "alpha %d", alpha);
    if (NULL == pSetLayeredWindowAttributes) return false;

    LONG wStyle1, wStyle2;
    wStyle1 = wStyle2 = GetWindowLong(hwnd, GWL_EXSTYLE);

    if (alpha < 255) wStyle2 |= WS_EX_LAYERED;
    else wStyle2 &= ~WS_EX_LAYERED;

    if (wStyle2 != wStyle1)
        SetWindowLong(hwnd, GWL_EXSTYLE, wStyle2);

    if (wStyle2 & WS_EX_LAYERED)
        return 0 != pSetLayeredWindowAttributes(hwnd, 0, alpha, LWA_ALPHA);

    return true;
}

//*****************************************************************************

//*****************************************************************************
// multimon api, win 9x compatible

HMONITOR (WINAPI *pMonitorFromWindow)(HWND hwnd, DWORD dwFlags);
HMONITOR (WINAPI *pMonitorFromPoint)(POINT pt, DWORD dwFlags);
BOOL     (WINAPI *pGetMonitorInfoA)(HMONITOR hMonitor, LPMONITORINFO lpmi);
BOOL     (WINAPI* pEnumDisplayMonitors)(HDC, LPCRECT, MONITORENUMPROC, LPARAM);

ST void get_mon_rect(HMONITOR hMon, RECT *s, RECT *w)
{
    if (hMon)
    {
        MONITORINFO mi;
        mi.cbSize = sizeof(mi);
        if (pGetMonitorInfoA(hMon, &mi))
        {
            if (w) *w = mi.rcWork;
            if (s) *s = mi.rcMonitor;
            return;
        }
    }

    if (w)
    {
        SystemParametersInfo(SPI_GETWORKAREA, 0, w, 0);
    }

    if (s)
    {
        s->top = s->left = 0;
        s->right = GetSystemMetrics(SM_CXSCREEN);
        s->bottom = GetSystemMetrics(SM_CYSCREEN);
    }
}

//===========================================================================
// API: GetMonitorRect
//===========================================================================

HMONITOR GetMonitorRect(void *from, RECT *r, int flags)
{
    HMONITOR hMon = NULL;
    if (multimon && from)
    {
        if (flags & GETMON_FROM_WINDOW)
            hMon = pMonitorFromWindow((HWND)from, MONITOR_DEFAULTTONEAREST);
        else
        if (flags & GETMON_FROM_POINT)
            hMon = pMonitorFromPoint(*(POINT*)from, MONITOR_DEFAULTTONEAREST);
        else
        if (flags & GETMON_FROM_MONITOR)
            hMon = (HMONITOR)from;
    }

    if (flags & GETMON_WORKAREA)
        get_mon_rect(hMon, NULL, r);
    else
        get_mon_rect(hMon, r, NULL);

    return hMon;
}

//===========================================================================
// API: SetDesktopMargin
// Purpose:  Set a margin for e.g. toolbar, bbsystembar, etc
// In:       hwnd to associate the margin with, location, margin-width
// Out:      void
//===========================================================================

struct dt_margins
{
    struct dt_margins *next;
    HWND hwnd;
    HMONITOR hmon;
    int location;
    int margin;
};

void update_screen_areas(struct dt_margins *dt_margins);

void SetDesktopMargin(HWND hwnd, int location, int margin)
{
    //dbg_printf("SDTM: %08x %d %d", hwnd, location, margin);

    static struct dt_margins *margin_list;

    if (BB_DM_RESET == location)
        freeall(&margin_list); // reset everything
    else
    if (BB_DM_REFRESH == location)
        ; // do nothing
    else
    if (hwnd)
    {
        // search for hwnd:
        struct dt_margins *p = (struct dt_margins *)assoc(margin_list, hwnd);
        if (margin)
        {
            if (NULL == p) // insert a _new structure
            {
                p = (struct dt_margins *)c_alloc(sizeof(struct dt_margins));
                cons_node (&margin_list, p);
                p->hwnd = hwnd;
            }
            p->location = location;
            p->margin = margin;
            if (multimon) p->hmon = pMonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
        }
        else
        if (p)
        {
            remove_item(&margin_list, p);
        }
    }
    update_screen_areas(margin_list);
}

//===========================================================================

struct _screen
{
    struct _screen *next;
    HMONITOR hMon;
    int index;
    RECT screen_rect;
    RECT work_rect;
    RECT new_rect;
    RECT custom_margins;
};

struct _screen_list
{
    struct _screen *pScr;
    struct _screen **ppScr;
    int index;
};

void get_custom_margin(RECT *pcm, int screen)
{
    char key[80]; int i = 0, x;
    static const char *edges[4] = { "Left:", "Top:", "Right:", "Bottom:" };
    if (0 == screen) x = sprintf(key, "blackbox.desktop.margin");
    else x = sprintf(key, "blackbox.desktop.%d.margin", 1+screen);
    do strcpy(key+x, edges[i]), (&pcm->left)[i] = ReadInt(extensionsrcPath(), key, -1);
    while (++i < 4);
}

ST BOOL CALLBACK fnEnumMonProc(HMONITOR hMon, HDC hdcOptional, RECT *prcLimit, LPARAM dwData)
{
    //dbg_printf("EnumProc %08x", hMon);
    struct _screen * s = (struct _screen *)c_alloc (sizeof *s);
    s->hMon = hMon;
    get_mon_rect(hMon, &s->screen_rect, &s->work_rect);
    s->new_rect = s->screen_rect;

    struct _screen_list *i = (struct _screen_list *)dwData;
    *i->ppScr = s;
    i->ppScr = &s->next;

    int screen = s->index = i->index++;
    if (0 == screen) s->custom_margins = Settings_desktopMargin;
    else get_custom_margin(&s->custom_margins, screen);

    return TRUE;
}

void update_screen_areas(struct dt_margins *dt_margins)
{
    struct _screen_list si = { NULL, &si.pScr, 0 };

    if (multimon)
        pEnumDisplayMonitors(NULL, NULL, fnEnumMonProc, (LPARAM)&si);
    else
        fnEnumMonProc(NULL, NULL, NULL, (LPARAM)&si);

    //dbg_printf("list: %d %d", listlen(si.pScr), si.index);

    struct _screen *pS;

    if (false == Settings_fullMaximization)
    {
        struct dt_margins *p;
        dolist (p, dt_margins) // loop through margins
        {
            pS = (struct _screen *)assoc(si.pScr, p->hmon); // get screen for this window
            //dbg_printf("assoc: %x", ppScr);
            if (pS)
            {
                RECT *n = &pS->new_rect;
                RECT *s = &pS->screen_rect;
                switch (p->location)
                {
                    case BB_DM_LEFT   : n->left     = imax(n->left  , s->left   + p->margin); break;
                    case BB_DM_TOP    : n->top      = imax(n->top   , s->top    + p->margin); break;
                    case BB_DM_RIGHT  : n->right    = imin(n->right , s->right  - p->margin); break;
                    case BB_DM_BOTTOM : n->bottom   = imin(n->bottom, s->bottom - p->margin); break;
                }
            }
        }

        dolist (pS, si.pScr)
        {
            RECT *n = &pS->new_rect;
            RECT *s = &pS->screen_rect;
            RECT *m = &pS->custom_margins;
            if (-1 != m->left)     n->left     = s->left     + m->left    ;
            if (-1 != m->top)      n->top      = s->top      + m->top     ;
            if (-1 != m->right)    n->right    = s->right    - m->right   ;
            if (-1 != m->bottom)   n->bottom   = s->bottom   - m->bottom  ;
        }
    }

    dolist (pS, si.pScr)
    {
        RECT *n = &pS->new_rect;
        if (0 != memcmp(&pS->work_rect, n, sizeof(RECT)))
        {
            //dbg_printf("HW = %08x  WA = %d %d %d %d", hwnd, n->left, n->top, n->right, n->bottom);
            SystemParametersInfo(SPI_SETWORKAREA, 0, (PVOID)n, SPIF_SENDCHANGE);
        }
    }

    freeall(&si.pScr);
}

//===========================================================================

//===========================================================================
// API: SnapWindowToEdge
// Purpose:Snaps a given windowpos at a specified distance
// In: WINDOWPOS* = WINDOWPOS recieved from WM_WINDOWPOSCHANGING
// In: int = distance to snap to
// In: bool = use screensize of workspace
// Out: void = none
//===========================================================================

// Structures
struct edges { int from1, from2, to1, to2, dmin, omin, d, o, def; };

struct snap_info { struct edges *h; struct edges *v;
    bool sizing; bool same_level; int pad; HWND self; HWND parent; };

// Local fuctions
//ST void snap_to_grid(struct edges *h, struct edges *v, bool sizing, int grid, int pad);
ST void snap_to_edge(struct edges *h, struct edges *v, bool sizing, bool same_level, int pad);
ST BOOL CALLBACK SnapEnumProc(HWND hwnd, LPARAM lParam);


void SnapWindowToEdge(WINDOWPOS* wp, LPARAM nDist_or_pContent, UINT Flags)
{
    int snapdist    = Settings_edgeSnapThreshold;
    int padding     = Settings_edgeSnapPadding;
    //int grid        = 0;

    if (snapdist < 1) return;

    HWND self = wp->hwnd;

    // well, why is this here? Because some plugins call this even if
    // they reposition themselves rather than being moved by the user.
    static DWORD snap_tick; DWORD ticknow = GetTickCount();
    if (GetCapture() != self)
    {
        if (snap_tick < ticknow) return;
    }
    else
    {
        snap_tick = ticknow + 100;
    }

    HWND parent = (WS_CHILD & GetWindowLong(self, GWL_STYLE)) ? GetParent(self) : NULL;

    //RECT r; GetWindowRect(self, &r);
    //bool sizing         = wp->cx != r.right-r.left || wp->cy != r.bottom-r.top;

    bool sizing         = 0 != (Flags & SNAP_SIZING);
    bool snap_workarea  = 0 == (Flags & SNAP_FULLSCREEN);
    bool snap_plugins   = 0 == (Flags & SNAP_NOPLUGINS) && Settings_edgeSnapPlugins;
    //dbg_printf("%x %x %d %d %d %d Flags: %x siz %d", self, parent, wp->x, wp->y, wp->cx, wp->cy, Flags, sizing);

    // ------------------------------------------------------
    struct edges h;
    struct edges v;
    struct snap_info si = { &h, &v, sizing, true, padding, self, parent };

    h.dmin = v.dmin = h.def = v.def = snapdist;

    h.from1 = wp->x;
    h.from2 = h.from1 + wp->cx;
    v.from1 = wp->y;
    v.from2 = v.from1 + wp->cy;

    // ------------------------------------------------------
    // snap to grid

    /*if (grid > 1 && (parent || sizing))
    {
        snap_to_grid(&h, &v, sizing, grid, padding);
    }*/
    //else
    {
        // -----------------------------------------
        if (parent) // snap to siblings
        {
            EnumChildWindows(parent, SnapEnumProc, (LPARAM)&si);
            if (0 == (Flags&SNAP_NOPARENT))
            {
                // snap to frame edges
                RECT r; GetClientRect(parent, &r);
                h.to1 = r.left;
                h.to2 = r.right;
                v.to1 = r.top;
                v.to2 = r.bottom;
                snap_to_edge(&h, &v, sizing, false, padding);
            }
        }
        else // snap to top level windows
        {
            if (snap_plugins)
                EnumThreadWindows(GetCurrentThreadId(), SnapEnumProc, (LPARAM)&si);

            // snap to screen edges
            RECT r; GetMonitorRect(self, &r, snap_workarea ?  GETMON_WORKAREA|GETMON_FROM_WINDOW : GETMON_FROM_WINDOW);
            h.to1 = r.left;
            h.to2 = r.right;
            v.to1 = r.top;
            v.to2 = r.bottom;
            snap_to_edge(&h, &v, sizing, false, 0);
        }
        // -----------------------------------------
        if (sizing)
        {
            if ((Flags & SNAP_CONTENT) && nDist_or_pContent)// snap to button icons
            {
                // images have to be double padded, since they are centered
                h.to2 = (h.to1 = h.from1) + ((SIZE*)nDist_or_pContent)->cx;
                v.to2 = (v.to1 = v.from1) + ((SIZE*)nDist_or_pContent)->cy;
                snap_to_edge(&h, &v, sizing, false, 0);
            }
            // snap frame to childs
            si.same_level = false;
            si.pad = -padding;
            si.self = NULL;
            si.parent = self;
            EnumChildWindows(self, SnapEnumProc, (LPARAM)&si);
        }
    }

    // -----------------------------------------
    // adjust the window-pos

    if (h.dmin < snapdist)
        if (sizing) wp->cx += h.omin; else wp->x += h.omin;

    if (v.dmin < snapdist)
        if (sizing) wp->cy += v.omin; else wp->y += v.omin;
}

//*****************************************************************************

ST BOOL CALLBACK SnapEnumProc(HWND hwnd, LPARAM lParam)
{
    struct snap_info *si = (struct snap_info *)lParam;
    LONG style = GetWindowLong(hwnd, GWL_STYLE);

    if (hwnd != si->self && (style & WS_VISIBLE))
    {
        HWND pw = (style & WS_CHILD) ? GetParent(hwnd) : NULL;
        if (pw == si->parent && false == IsMenu(hwnd))
        {
            RECT r; GetWindowRect(hwnd, &r);
            r.right -= r.left;
            r.bottom -= r.top;
            if (pw) ScreenToClient(pw, (POINT*)&r.left);
            if (false == si->same_level)
            {
                r.left += si->h->from1;
                r.top  += si->v->from1;
            }
            si->h->to2 = (si->h->to1 = r.left) + r.right;
            si->v->to2 = (si->v->to1 = r.top)  + r.bottom;
            snap_to_edge(si->h, si->v, si->sizing, si->same_level, si->pad);
        }
    }
    return TRUE;
}   

//*****************************************************************************
/*
ST void snap_to_grid(struct edges *h, struct edges *v, bool sizing, int grid, int pad)
{
    for (struct edges *g = h;;g = v)
    {
        int o, d;
        if (sizing) o = g->from2 - g->from1 + pad; // relative to topleft
        else        o = g->from1 - pad; // absolute coords

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
*/
//*****************************************************************************
ST void snap_to_edge(struct edges *h, struct edges *v, bool sizing, bool same_level, int pad)
{
    int o, d, n; struct edges *t;
    h->d = h->def; v->d = v->def;
    for (n = 2;;) // v- and h-edge
    {
        // see if there is any common edge, i.e if the lower top is above the upper bottom.
        if ((v->to2 < v->from2 ? v->to2 : v->from2) >= (v->to1 > v->from1 ? v->to1 : v->from1))
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

//===========================================================================
