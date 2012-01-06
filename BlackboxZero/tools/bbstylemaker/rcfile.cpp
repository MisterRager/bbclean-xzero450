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

// API: ReadValue
const char* ReadValue(const char* path, const char* szKey, long *ptr)
{
    return read_value(path, szKey, ptr);
}

// API: WriteValue
void WriteValue(const char* path, const char* szKey, const char* value)
{
    write_value(path, szKey, value);
}

// API: RenameSetting
bool RenameSetting(const char* path, const char* szKey, const char* new_keyword)
{
    return 0 != rename_setting(path, szKey, new_keyword);
}

// API: DeleteSetting
bool DeleteSetting(LPCSTR path, LPCSTR szKey)
{
    return 0 != delete_setting(path, szKey);
}

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

// API: ReadBool
bool ReadBool(LPCSTR fileName, LPCSTR szKey, bool bDefault)
{
    LPCSTR szValue = read_value(fileName, szKey, NULL);
    if (szValue) {
        if (!stricmp(szValue, "true"))
            return true;
        if (!stricmp(szValue, "false"))
            return false;
    }
    return bDefault;
}

// API: ReadInt
int ReadInt(LPCSTR fileName, LPCSTR szKey, int nDefault)
{
    LPCSTR szValue = read_value(fileName, szKey, NULL);
    return szValue ? atoi(szValue) : nDefault;
}

// API: ReadString
LPCSTR ReadString(LPCSTR fileName, LPCSTR szKey, LPCSTR szDefault)
{
    LPCSTR szValue = read_value(fileName, szKey, NULL);
    return szValue ? szValue : szDefault;
}

// API: ReadColor
COLORREF ReadColor(LPCSTR fileName, LPCSTR szKey, LPCSTR defaultColor)
{
    LPCSTR szValue = szKey[0] ? read_value(fileName, szKey, NULL) : NULL;
    return ReadColorFromString(szValue ? szValue : defaultColor);
}

// API: ParseItem
void ParseItem(const char* szItem, StyleItem *item)
{
    parse_item(szItem, item);
}

// API: FileExists
bool FileExists(LPCSTR szFileName)
{
    DWORD a = GetFileAttributes(szFileName);
    return (DWORD)-1 != a && 0 == (a & FILE_ATTRIBUTE_DIRECTORY);
}

//===========================================================================
