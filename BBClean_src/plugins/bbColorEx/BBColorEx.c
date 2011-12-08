/* BBColorEx - enhanced BBColor or bbColor3dc
   Copyright (C) 2004 kana <nicht AT s8 DOT xrea DOT com>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "../../blackbox/BBApi.h"

DLL_EXPORT LPCTSTR stylePath(LPCSTR);

#define NUMBER_OF(a) (sizeof(a) / sizeof((a)[0]))




static const char BBCX_NAME[] = "BBColorEx";
static const char BBCX_VERSION[] = "0.0.2.1";
static const char BBCX_AUTHOR[] = "kana";
static const char BBCX_RELEASE[] = "2004-11-01/2005-01-29";
static const char BBCX_LINK[] = "http://nicht.s8.xrea.com/";
static const char BBCX_EMAIL[] = "nicht AT s8 DOT xrea DOT con";

static UINT BBCX_MESSAGES[] = {BB_RECONFIGURE, 0};


#define COLOR_3DALTFACE 25  /* missing index */

#define CS_3D_DARK_SHADOW				COLOR_3DDKSHADOW
#define CS_3D_SHADOW					COLOR_3DSHADOW
#define CS_3D_FACE						COLOR_3DFACE
#define CS_3D_ALTERNATE_FACE			COLOR_3DALTFACE
#define CS_3D_LIGHT						COLOR_3DLIGHT
#define CS_3D_HILIGHT					COLOR_3DHILIGHT
#define CS_ACTIVE_CAPTION				COLOR_ACTIVECAPTION
#define CS_ACTIVE_CAPTION_GRADIENT		COLOR_GRADIENTACTIVECAPTION
#define CS_ACTIVE_CAPTION_TEXT			COLOR_CAPTIONTEXT
#define CS_ACTIVE_WINDOW_BORDER			COLOR_ACTIVEBORDER
#define CS_APPLICATION_BACKGROUND		COLOR_APPWORKSPACE
#define CS_BUTTON_TEXT					COLOR_BTNTEXT
#define CS_DESKTOP						COLOR_DESKTOP
#define CS_FOCUSED_OBJECT_FRAME			COLOR_WINDOWFRAME
#define CS_GRAY_TEXT					COLOR_GRAYTEXT
#define CS_INACTIVE_CAPTION				COLOR_INACTIVECAPTION
#define CS_INACTIVE_CAPTION_GRADIENT	COLOR_GRADIENTINACTIVECAPTION
#define CS_INACTIVE_CAPTION_TEXT		COLOR_INACTIVECAPTIONTEXT
#define CS_INACTIVE_CAPTION_BORDER		COLOR_INACTIVEBORDER
#define CS_MENU							COLOR_MENU
#define CS_MENU_TEXT					COLOR_MENUTEXT
#define CS_MOUSE_HIGHLIGHT				COLOR_HOTLIGHT
#define CS_SCROLLBAR					COLOR_SCROLLBAR
#define CS_SELECTED_ITEM				COLOR_HIGHLIGHT
#define CS_SELECTED_ITEM_TEXT			COLOR_HIGHLIGHTTEXT
#define CS_TOOLTIP						COLOR_INFOBK
#define CS_TOOLTIP_TEXT					COLOR_INFOTEXT
#define CS_WINDOW						COLOR_WINDOW
#define CS_WINDOW_TEXT					COLOR_WINDOWTEXT

static int CS_KEYS[] = {
	CS_3D_DARK_SHADOW,
	CS_3D_SHADOW,
	CS_3D_FACE,
	CS_3D_ALTERNATE_FACE,
	CS_3D_LIGHT,
	CS_3D_HILIGHT,
	CS_ACTIVE_CAPTION,
	CS_ACTIVE_CAPTION_GRADIENT,
	CS_ACTIVE_CAPTION_TEXT,
	CS_ACTIVE_WINDOW_BORDER,
	CS_APPLICATION_BACKGROUND,
	CS_BUTTON_TEXT,
	CS_DESKTOP,
	CS_FOCUSED_OBJECT_FRAME,
	CS_GRAY_TEXT,
	CS_INACTIVE_CAPTION,
	CS_INACTIVE_CAPTION_GRADIENT,
	CS_INACTIVE_CAPTION_TEXT,
	CS_INACTIVE_CAPTION_BORDER,
	CS_MENU,
	CS_MENU_TEXT,
	CS_MOUSE_HIGHLIGHT,
	CS_SCROLLBAR,
	CS_SELECTED_ITEM,
	CS_SELECTED_ITEM_TEXT,
	CS_TOOLTIP,
	CS_TOOLTIP_TEXT,
	CS_WINDOW,
	CS_WINDOW_TEXT
};

static const char* CS_KEYS_NAME[] = {
	"3D Dark Shadow",
	"3D Shadow",
	"3D Face",
	"3D Alternate Face",
	"3D Light",
	"3D Hi-Light",
	"Active Caption",
	"Active Caption Gradient",
	"Active Caption Text",
	"Active Window Border",
	"Application Background",
	"Button Text",
	"Desktop",
	"Focused Object Frame",
	"Gray Text",
	"Inactive Caption",
	"Inactive Caption Gradient",
	"Inactive Caption Text",
	"Inactive Window Border",
	"Menu",
	"Menu Text",
	"Mouse Highlight",
	"Scrollbar",
	"Selected Item",
	"Selected Item Text",
	"Tooltip",
	"Tooltip Text",
	"Window",
	"Window Text"
};

typedef struct {
	COLORREF color[NUMBER_OF(CS_KEYS)];
} ColorScheme;




struct {
	HWND plugin_window;
	HMODULE plugin_module;
	ColorScheme original_color_scheme;
} G;


static void error(const char* format, ...) {
	char buf[80*25];
	va_list va;

	va_start(va, format);
	vsnprintf(buf, NUMBER_OF(buf), format, va);
	buf[NUMBER_OF(buf) - 1] = '\0';
	va_end(va);

	MessageBox( G.plugin_window, buf, BBCX_NAME,
	            MB_OK | MB_ICONERROR
	              | MB_SETFOREGROUND | MB_TASKMODAL | MB_TOPMOST );
}


static char* STRNCPY(char* dest, const char* src, size_t n) {
	if (0 < n) {
		strncpy(dest, src, n);
		dest[n - 1] = '\0';
	}

	return dest;
}


static void SNPRINTF(char* buf, int size, const char* format, ...) {
	va_list va;

	if (size < 0)
		return;

	va_start(va, format);
	vsnprintf(buf, size, format, va);
	va_end(va);

	buf[size - 1] = '\0';
}




static void ColorScheme_Backup(ColorScheme* self) {
	int i;

	for (i = 0; i < NUMBER_OF(CS_KEYS); i++) {
		self->color[i] = GetSysColor(CS_KEYS[i]);
	}
}


static void ColorScheme_Apply(ColorScheme* self) {
	SetSysColors(NUMBER_OF(CS_KEYS), CS_KEYS, self->color);
}




static BOOL ColorScheme_LoadEmbeddedColorScheme(ColorScheme* self) {
	FILE* style;
	int i;
	char line[256];
	char* p;
	BOOL result;
	char tag_type;

	result = FALSE;

	style = fopen(stylePath(NULL), "rt");
	if (style == NULL)
		return FALSE;

	while (TRUE) {
		if (fgets(line, NUMBER_OF(line), style) == NULL)
			goto END;

		p = line;
		while (*p == ' ' || *p == '\t')
			p++;

		if (*p != '!' && *p != '#')
			continue;

		if (strstr(p, "<3DCC>") != NULL)
			{tag_type = 'X';  break;}
		strlwr(p);
		if (strstr(p, " 3dc") != NULL && strstr(p, " start") != NULL)
			{tag_type = 'A';  break;}
	}

	i = 0;
	while (i < NUMBER_OF(CS_KEYS)) {
		if (fgets(line, NUMBER_OF(line), style) == NULL)
			goto END;

		p = line;
		while (*p == ' ' || *p == '\t')
			p++;

		if (tag_type == 'X') {
			if (*p != '!' && *p != '#')
				continue;

			if (strstr(p, "</3DCC>") != NULL)
				break;

			p++;
		}

		self->color[i] = (COLORREF)atol(p);
		i++;
	}

	result = TRUE;
END:
	fclose(style);

	return result;
}


static BOOL ColorScheme_LoadExternalFile(ColorScheme* self) {
	char buf[MAX_PATH];
	char filename[MAX_PATH];
	char dir[MAX_PATH];
	char* backslash;
	FILE* style;
	char line[256];
	int i;

	STRNCPY(buf, stylePath(NULL), NUMBER_OF(buf));

	backslash = strrchr(buf, '\\');
	if (backslash != NULL) {
		*backslash = '\0';
		strcpy(filename, backslash + 1);
		strcpy(dir, buf);
	} else {
		strcpy(filename, buf);
		strcpy(dir, ".");
	}

	if ( 6 < strlen(filename)
	     && stricmp(filename + (strlen(filename) - 6), ".style") == 0 )
	{
		*strrchr(filename, '.') = '\0';
	}

	SNPRINTF(buf, NUMBER_OF(buf), "%s\\%s.3dc", dir, filename);

	style = fopen(buf, "rt");
	if (style == NULL) {
		SNPRINTF(buf,NUMBER_OF(buf), "%s\\3dc\\%s.3dc", dir, filename);

		style = fopen(buf, "rt");
		if (style == NULL)
			return FALSE;
	}


	for (i = 0; i < NUMBER_OF(CS_KEYS); i ++) {
		if (fgets(line, NUMBER_OF(line), style) == NULL)
			break;

		self->color[i] = (COLORREF)atol(line);
	}

	fclose(style);

	return TRUE;
}


typedef struct {
	int gradient;
	COLORREF color;
	COLORREF color_to;
	COLORREF text_color;
} DemiStyleItem;

#define B_PARENTRELATIVE -1

static COLORREF mixcolor(COLORREF c1, COLORREF c2, int per) {
	if (per < 0 || 100 < per)
		per = 50;

	return RGB( GetRValue(c1) * per/100 + GetRValue(c2) * (100-per)/100,
	            GetGValue(c1) * per/100 + GetGValue(c2) * (100-per)/100,
	            GetBValue(c1) * per/100 + GetBValue(c2) * (100-per)/100 );
}

static int _index(int cs_key) {
	int i;

	for (i = 0; i < NUMBER_OF(CS_KEYS); i++)
		if (CS_KEYS[i] == cs_key)
			return i;

	return CS_3D_ALTERNATE_FACE;
}

static DemiStyleItem ReadItem(const char* file, const char* _key) {
	char key[256];
	char key_without_colon[256];
	char value[256];
	char* colon;
	DemiStyleItem item;

	STRNCPY(key_without_colon, _key, NUMBER_OF(key_without_colon));

	colon = strrchr(key_without_colon, ':');
	if (colon != NULL)
		*colon = '\0';

	SNPRINTF(key, NUMBER_OF(key), "%s:", key_without_colon);
	STRNCPY( value,
	         ReadString(file, key, "Solid Flat Bevel1"),
	         NUMBER_OF(value) );
	value[NUMBER_OF(value) - 1] = '\0';
	strlwr(value);
	if (strstr(value, "solid") != NULL) {
		item.gradient = B_SOLID;
	} else if (strstr(value, "parentrelative") != NULL) {
		item.gradient = B_PARENTRELATIVE;
	} else {
		item.gradient = B_HORIZONTAL;
	}

	SNPRINTF(key, NUMBER_OF(key), "%s.color:", key_without_colon);
	if (ReadString(file, key, "")[0] != '\0')
		item.color = ReadColor(file, key, "#cc0000");
	else
		item.color = ReadColor(file, "*.color", "#cc0000");

	SNPRINTF(key, NUMBER_OF(key), "%s.colorTo:", key_without_colon);
	if (ReadString(file, key, "")[0] != '\0')
		item.color_to = ReadColor(file, key, "#00cc00");
	else
		item.color_to = ReadColor(file, "*.colorTo", "#00cc00");

	SNPRINTF(key, NUMBER_OF(key), "%s.textColor:", key_without_colon);
	if (ReadString(file, key, "")[0] != '\0')
		item.text_color = ReadColor(file, key, "#0000cc");
	else
		item.text_color = ReadColor(file, "*.textColor", "#0000cc");

	return item;
}

static COLORREF ReadColorEx(const char* file, const char* key) {
	char tmp1[256];
	char tmp2[256];
	const char* dot;

	if (key[strlen(key) - 1] != ':') {
		SNPRINTF(tmp1, NUMBER_OF(tmp1), "%s:", key);
		key = tmp1;
	}

	if (ReadString(file, key, "")[0] != '\0')
		return ReadColor(file, key, "#cc0000");

	dot = strrchr(key, '.');
	SNPRINTF(tmp2, NUMBER_OF(tmp2), "*.%s", (dot != NULL ? dot+1 : key));
	return ReadColor(file, tmp2, "#cc0000");
}

static BOOL ColorScheme_LoadFromStyle(ColorScheme* self) {
	const char* style;
	char dir[MAX_PATH];
	char rc[MAX_PATH];
	char* backslash;
	int i;

	GetModuleFileName(G.plugin_module, dir, NUMBER_OF(dir));
	dir[NUMBER_OF(dir) - 1] = '\0';
	backslash = strrchr(dir, '\\');
	if (backslash != NULL)
		*backslash = '\0';
	SNPRINTF(rc, NUMBER_OF(rc), "%s\\BBColorEx.rc", dir);

	style = stylePath(NULL);

	for (i = 0; i < NUMBER_OF(CS_KEYS); i++) {
		int elem;
		char elem_name[256];
		char tmp[256];
		const char* key;

		elem = CS_KEYS[i];

		if ( elem == CS_ACTIVE_CAPTION_GRADIENT
		     || elem == CS_ACTIVE_CAPTION_TEXT
		     || elem == CS_INACTIVE_CAPTION_GRADIENT
		     || elem == CS_INACTIVE_CAPTION_TEXT )
		{
			continue;
		}

		SNPRINTF( elem_name, NUMBER_OF(elem_name),
		          "%s:", CS_KEYS_NAME[i] );
		key = ReadString(rc, elem_name, "");
		if (key[0] == '\0')
			continue;

		if (elem == CS_ACTIVE_CAPTION || elem == CS_INACTIVE_CAPTION) {
			char* space;
			const char* key_base;
			DemiStyleItem i1;
			DemiStyleItem i2;
			DemiStyleItem i3;

			STRNCPY(tmp, key, NUMBER_OF(tmp));
			space = strchr(tmp, ' ');
			if (space != NULL) {
				*space = '\0';
				key_base = space + 1;
			} else {
				key_base = tmp;
			}

			i1 = ReadItem(style, tmp);
			i2 = ReadItem(style, key_base);
			i3 = (i1.gradient != B_PARENTRELATIVE ? i1 : i2);
			if (elem == CS_ACTIVE_CAPTION) {
			  self->color[_index(CS_ACTIVE_CAPTION)]
			    = i3.color;
			  self->color[_index(CS_ACTIVE_CAPTION_GRADIENT)]
			    = (i3.gradient==B_SOLID ? i3.color : i3.color_to);
			  self->color[_index(CS_ACTIVE_CAPTION_TEXT)]
			    = i1.text_color;
			} else {  /* CS_INACTIVE_CAPTION */
			  self->color[_index(CS_INACTIVE_CAPTION)]
			    = i3.color;
			  self->color[_index(CS_INACTIVE_CAPTION_GRADIENT)]
			    = (i3.gradient==B_SOLID ? i3.color : i3.color_to);
			  self->color[_index(CS_INACTIVE_CAPTION_TEXT)]
			    = i1.text_color;
			}
		} else {
			STRNCPY(tmp, key, NUMBER_OF(tmp));
			strlwr(tmp);
			if (strstr(tmp, "color") != NULL) {
				self->color[_index(elem)]
				  = ReadColorEx(style, key);
			} else {
				DemiStyleItem item;

				item = ReadItem(style, key);
				if (item.gradient == B_SOLID) {
				  self->color[_index(elem)] = item.color;
				} else {
				  self->color[_index(elem)]
				    = mixcolor(item.color, item.color_to, 50);
				}
			}
		}
	}

	return TRUE;
}








static void load_and_apply_color_scheme(void) {
	ColorScheme color_scheme;

	color_scheme = G.original_color_scheme;

	ColorScheme_LoadEmbeddedColorScheme(&color_scheme)
	  || ColorScheme_LoadExternalFile(&color_scheme)
	  || ColorScheme_LoadFromStyle(&color_scheme);

	ColorScheme_Apply(&color_scheme);
}

#define array_count(ary) (sizeof(ary) / sizeof(ary[0]))

char* make_full_path(char *buffer, const char *filename)
{
    buffer[0] = 0;
    if (NULL == strchr(filename, ':')) GetBlackboxPath(buffer, MAX_PATH);
    return strcat(buffer, filename);
}

void syscolor_write(const char *filename) {
	
    char path_to_3dc[MAX_PATH];

    FILE *fp = fopen(make_full_path(path_to_3dc, filename), "wt");
    if (fp) {
        int n = 0;
        do {
			int c = GetSysColor(CS_KEYS[n]);
            fprintf(fp, "%d\n", c);
        } while ( ++n < NUMBER_OF(CS_KEYS) );
        fclose(fp);
    }
}


void endPlugin(HINSTANCE hplugininstance) {
	DestroyWindow(G.plugin_window);
	UnregisterClass(BBCX_NAME, hplugininstance);
}




LPCSTR pluginInfo(int field) {
	switch (field) {
	case PLUGIN_NAME:    return BBCX_NAME;
	case PLUGIN_VERSION: return BBCX_VERSION;
	case PLUGIN_AUTHOR:  return BBCX_AUTHOR;
	case PLUGIN_RELEASE: return BBCX_RELEASE;
	case PLUGIN_LINK:    return BBCX_LINK;
	case PLUGIN_EMAIL:   return BBCX_EMAIL;
	default:             return BBCX_NAME;
	}
}

int n_stricmp(const char **pp, const char *s)
{
    int n = strlen (s);
    int i = memicmp(*pp, s, n);
    if (i) return i;
    i = (*pp)[n] - ' ';
    if (i > 0) return i;
    *pp += n;
    while (' '== **pp) ++*pp;
    return 0;
}

static LRESULT CALLBACK window_procedure(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {

	switch (msg) {
	default:
		break;


	case BB_RECONFIGURE:
		load_and_apply_color_scheme();
		break;


	case WM_CREATE:
		ColorScheme_Backup(&(G.original_color_scheme));
		load_and_apply_color_scheme();
		SendMessage( GetBBWnd(), BB_REGISTERMESSAGE,
		             (WPARAM)hwnd, (LPARAM)BBCX_MESSAGES );
		break;

	case WM_DESTROY:
		SendMessage( GetBBWnd(), BB_UNREGISTERMESSAGE,
		             (WPARAM)hwnd, (LPARAM)BBCX_MESSAGES );

		/* Restore the original color scheme*/
		ColorScheme_Apply(&(G.original_color_scheme));
		break;

		case BB_BROADCAST:
            if (0 == memicmp((LPCSTR)lp, "@BBColor3dc.", 12))
            {
                const char *msg_string = (LPCSTR)lp + 12;
				if (0 == n_stricmp(&msg_string, "Read")) {
                    //syscolor_read(msg_string);
                    break;
                }
                if (0 == n_stricmp(&msg_string, "Keep")) {
					//ColorScheme_Backup(&(G.original_color_scheme))
                    break;
                }
                if (0 == n_stricmp(&msg_string, "Write")) {
                    syscolor_write(msg_string);
                    break;
                }
				if (0 == stricmp(msg_string, "Clear")) {
                   // syscolor_restore();
                    break;
                }
                if (0 == n_stricmp(&msg_string, "SetColor")) {
                   // syscolor_setcolor(msg_string);
                    break;
                }
                if (0 == n_stricmp(&msg_string, "GetColor")) {
                   // syscolor_getcolor(msg_string);
                    break;
                }
            }
            break;
	}

	return DefWindowProc(hwnd, msg, wp, lp);
}

int beginPlugin(HINSTANCE hplugininstance) {
	WNDCLASS wc;

	G.plugin_module = hplugininstance;

	ZeroMemory(&wc, sizeof(wc));
	wc.lpszClassName = BBCX_NAME;
	wc.lpfnWndProc = window_procedure;
	wc.hInstance = hplugininstance;
	if (!RegisterClass(&wc)) {
		error("RegisterClass failed");
		goto E_REGISTER_CLASS;
	}

	G.plugin_window = CreateWindowEx( WS_EX_TOOLWINDOW,
	                           BBCX_NAME,
	                           NULL,
	                           WS_DISABLED,
	                           0, 0, 0, 0,
	                           NULL,
	                           NULL,
	                           hplugininstance,
	                           NULL );
	if (G.plugin_window == NULL) {
		error("CreateWindowEx failed");
		goto E_CREATE_WINDOW_EX;
	}

	return 0;


E_CREATE_WINDOW_EX:
	UnregisterClass(BBCX_NAME, hplugininstance);
E_REGISTER_CLASS:
	return !0;
}

/* __END__ */
