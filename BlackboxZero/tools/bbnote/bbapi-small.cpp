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
// bbapi-small.cpp - rc-file reader for blackbox styles

#include "BBApi.h"
#include "BImage.h"
#include "bbrc.h"

#define ST static

ST struct rcreader_init g_rc =
{
    NULL,   // struct fil_list *rc_files;
    NULL,   // void (*write_error)(const char *filename);
    true,   // char dos_eol;
    true,   // char translate_065;
    0,      // char found_last_value;
};

void bb_rcreader_init(void)
{
    init_rcreader(&g_rc);
}

// API: ReadValue
const char* ReadValue(const char* path, const char* szKey, long *ptr)
{
    return read_value(path, szKey, ptr);
}

// API: ReadBool
bool ReadBool(const char* fileName, const char* szKey, bool bDefault)
{
    const char* szValue = read_value(fileName, szKey, NULL);
    if (szValue) {
        if (!stricmp(szValue, "true"))
            return true;
        if (!stricmp(szValue, "false"))
            return false;
    }
    return bDefault;
}

// API: ReadInt
int ReadInt(const char* fileName, const char* szKey, int nDefault)
{
    const char* szValue = read_value(fileName, szKey, NULL);
    return szValue ? atoi(szValue) : nDefault;
}

// API: ReadString
const char* ReadString(const char* fileName, const char* szKey, const char* szDefault)
{
    const char* szValue = read_value(fileName, szKey, NULL);
    return szValue ? szValue : szDefault;
}

// API: ReadColor
COLORREF ReadColor(const char* fileName, const char* szKey, const char* defaultColor)
{
    const char* szValue = szKey[0] ? read_value(fileName, szKey, NULL) : NULL;
    return ReadColorFromString(szValue ? szValue : defaultColor);
}

// API: ParseItem
void ParseItem(const char* szItem, StyleItem *item)
{
    parse_item(szItem, item);
}

