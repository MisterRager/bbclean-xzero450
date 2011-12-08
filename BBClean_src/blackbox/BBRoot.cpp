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

// execute a bsetroot command and load the generated <tempfile.bmp>
// file into a HBITMAP, for Desk.cpp to paint the wallpaper.

#include "BB.H"
#include "Settings.h"

//#define EXTERN_WALLPAPER
#define INTERN_GRADIENT

//===========================================================================
#ifdef EXTERN_WALLPAPER
HBITMAP load_desk_bitmap(LPCSTR command)
{
		BBExecute_string(command, false);
		return NULL;
}

//===========================================================================
#else
//===========================================================================
#ifdef INTERN_GRADIENT

static const char *cmds[] =
{
	"-tile",        "-t",
	"-full",        "-f",
	"-center",      "-c",
	"-bitmap",      "-hue",     "-sat",
	"tile",         "center",   "stretch",

	"-solid",
	"-gradient",    "-from",    "-to",
	"-mod",         "-fg",      "-bg",

	"interlaced",
	NULL
};
enum
{
	E_tile      , E_t         ,
	E_full      , E_f         ,
	E_center    , E_c         ,
	E_bitmap    , E_hue       , E_sat       ,
	Etile       , Ecenter     , Estretch    ,

	E_solid     ,
	E_gradient  , E_from      , E_to        ,
	E_mod       , E_fg        , E_bg        ,
	E_interlaced
};

class root_bitmap
{

	const char *cptr;
	char token[MAX_PATH];

// -bitmap
	char bmp;
	char wpstyle;
	char wpfile[MAX_PATH];
	int sat;
	int hue;

#define WP_NONE 0
#define WP_TILE 1
#define WP_CENTER 2
#define WP_FULL 3

	// -solid
	char solid;
	COLORREF solidcolor;

	// -gradient
	char gradient;
	COLORREF from;
	COLORREF to;
	bool interlaced;
	int type;

	// -mod
	char mod;
	int modx;
	int mody;
	COLORREF modfg;
	COLORREF modbg;

	void init_root(void)
	{
		bmp     =
		solid   =
		gradient =
		mod     =
		wpstyle =
		*wpfile = 0;
		sat     = 255;
		hue     = 0;
		from    =
		modfg   = 0xffffff;
		to      =
		modbg   = 0x000000;
		interlaced = false;
		type    = B_HORIZONTAL;
		modx    =
		mody    = 4;
	}

	//===========================================================================
	int next_token(void)
	{
	   strlwr(NextToken(token, &cptr));
	   return get_string_index(token, cmds);
	}

	int read_int(int *dst)
	{
		next_token();
		if (0==*token) return 0;
		*dst = atoi(token);
		return 1;
	}

	int read_color(COLORREF *dst)
	{
		next_token();
		if (0==*token) return 0;
		*dst = ReadColorFromString(token);
		return 1;
	}


	//===========================================================================
	int parse_command(void)
	{
		while (*cptr)
		{
			int s = next_token();
	cont_1:
			if ('-' != token[0] && 0==bmp) goto img_name;
			switch (s) {
			case E_bitmap:
				while(*cptr)
				{
					s = next_token();
					if (s == Etile)
			case E_tile:
			case E_t:
						wpstyle = WP_TILE;
					else
					if (s == Ecenter)
			case E_center:
			case E_c:
						wpstyle = WP_CENTER;
					else
					if (s == Estretch)
			case E_full:
			case E_f:
						wpstyle = WP_FULL;
					else
					if (s==E_hue)
					{
						if (0==read_int(&hue)) goto command_error;
					}
					else
					if (s==E_sat)
					{
						if (0==read_int(&sat)) goto command_error;
					}
					else
					if (0==bmp)
					{
	img_name:
						unquote(wpfile, token);
						bmp = 1;
					}
					else goto cont_1;
				}
				continue;

			case E_solid:
				while(*cptr)
				{
					s = next_token();
					if (s == E_interlaced)
					{
						interlaced = true;
					}
					else
					if (0 == solid)
					{
						solidcolor = ReadColorFromString(token);
						solid = 1;
					}
					else goto cont_1;
				}
				continue;

			case E_gradient:
				while(*cptr)
				{
					s = next_token();
					int t; int ParseType(char *);
					if (-1 != (t = ParseType(token)))
					{
						gradient = 1;
						type = t;
						if (strstr(token, cmds[E_interlaced]))
							interlaced = true;
					}
					else
					if (s == E_from)
					{
						if (0==read_color(&from)) goto command_error;
					}
					else
					if (s == E_to)
					{
						if (0==read_color(&to)) goto command_error;
					}
					else
					if (s == E_interlaced)
					{
						interlaced = true;
					}
					else goto cont_1;
				}
				continue;

			case E_mod:
				mod = 1;
				while (*cptr)
				{
					s = next_token();
					if (s == E_interlaced)
					{
						interlaced = true;
					}
					else
					if (s == E_fg)
					{
						if (0==read_color(&modfg)) goto command_error;
					}
					else
					if (s == E_bg)
					{
						if (0==read_color(&modbg)) goto command_error;
					}
					else
					if (*token>='0' && *token <='9' && mod<3)
					{
						mody = atoi(token);
						if (++mod==2) modx = mody;
					}
					else goto cont_1;
				}
				continue;

			default:
			command_error:
				return 0;
			}
		}
		return 1;
	}

	//===========================================================================
	void Modula(HDC hdc, int width, int height, int mx, int my, COLORREF fg)
	{
		int x, y;

		if (mx<2) mx = 4;
		if (my<2) my = 4;

		HGDIOBJ P0 = SelectObject(hdc, CreatePen(PS_SOLID, 1, fg));

		for (y = height-mx; y >= 0; y-=my)
			MoveToEx(hdc, 0, y, NULL), LineTo(hdc, width, y);

		for (x = mx-1; x < width;  x+=mx)
			MoveToEx(hdc, x, 0, NULL), LineTo(hdc, x, height);

		DeleteObject(SelectObject(hdc, P0));
	}

	//===========================================================================
public:
	HBITMAP make_root_bmp(const char *command)
	{
		cptr = command;
		next_token(); // skip 'bsetroot.exe'
		init_root();

		if (0==parse_command() || bmp) return NULL;

		if (gradient)
			;
		else
		if (solid)
		   type = B_SOLID, from = to = solidcolor;
		else
		if (mod)
		   type = B_SOLID, from = to = modbg;
		else
			return NULL;

		int width = VScreenWidth;
		int height = VScreenHeight;

		HWND hwnd_desk = GetDesktopWindow();
		HDC hdc_desk = GetDC(hwnd_desk);

		HDC buf = CreateCompatibleDC(hdc_desk);
		HBITMAP b = CreateCompatibleBitmap(hdc_desk, width, height);
		HGDIOBJ B0 = SelectObject(buf, b);

		RECT rect = { 0, 0, width, height };
		MakeGradient(buf, rect, type, from, to, interlaced, 0, 0, 0, 0, 0);
		if (mod) Modula (buf, width, height, modx, mody, modfg);

		SelectObject(buf, B0);
		DeleteDC(buf);

		ReleaseDC(hwnd_desk, hdc_desk);
		return b;
	}
}; // class root_bitmap

#endif
//===========================================================================

//===========================================================================
static HBITMAP read_bitmap(LPCSTR path, bool delete_after)
{
	//return (HBITMAP)LoadImage(NULL, path, IMAGE_BITMAP, 0,0, LR_LOADFROMFILE);

	FILE *fp;
	BITMAPFILEHEADER hdr;
	HBITMAP bmp;

	if (NULL==(fp=fopen(path, "rb")))
		return NULL;

	fread(&hdr, 1, sizeof(hdr), fp);
	if (0x4D42 != hdr.bfType)
	{
		fclose (fp);
		return NULL;
	}

	BITMAPINFOHEADER bih, *pbih; int CU; void *lpBits;
	fread(&bih, 1, sizeof(bih), fp);
	CU = bih.biClrUsed * sizeof(RGBQUAD);
	pbih = (PBITMAPINFOHEADER)m_alloc(bih.biSize + CU);
	memmove(pbih, &bih, bih.biSize);
	fread(&((BITMAPINFO*)pbih)->bmiColors, 1, CU, fp);

	lpBits = m_alloc(pbih->biSizeImage);
	fseek(fp, hdr.bfOffBits, SEEK_SET);
	fread(lpBits, 1, pbih->biSizeImage, fp);
	fclose(fp);

	HWND hwnd_desk = GetDesktopWindow();
	HDC hdc_desk = GetDC(hwnd_desk);
	bmp = CreateDIBitmap(hdc_desk, pbih, CBM_INIT, lpBits, (LPBITMAPINFO)pbih, DIB_RGB_COLORS);
	m_free(lpBits);
	m_free(pbih);
	if (bmp)
	{
		// convert in any case (20ms), bc if it's compatible, it's faster to paint.
		HDC hdc_old = CreateCompatibleDC(hdc_desk);
		HGDIOBJ old_bmp = SelectObject(hdc_old, bmp);
		HDC hdc_new = CreateCompatibleDC(hdc_desk);
		HBITMAP bmp_new = CreateCompatibleBitmap(hdc_desk, VScreenWidth, VScreenHeight);

		SelectObject(hdc_new, bmp_new);
		StretchBlt(hdc_new, 0, 0, VScreenWidth, VScreenHeight, hdc_old, 0, 0, bih.biWidth, bih.biHeight, SRCCOPY);
		DeleteDC(hdc_new);

		DeleteObject(SelectObject(hdc_old, old_bmp));
		DeleteDC(hdc_old);
		bmp = bmp_new;
	}
	ReleaseDC(hwnd_desk, hdc_desk);

	if (delete_after) DeleteFile(path);
	return bmp;
}

//===========================================================================

//===========================================================================
HBITMAP load_desk_bitmap(LPCSTR command)
{
	char token[MAX_PATH];
	const char *cptr = command;

	strlwr(NextToken(token, &cptr));

	if (NULL==strstr(token, "bsetroot") && NULL==strstr(token, "bsetbg"))
	{
		BBExecute_string(command, true);
		return NULL;
	}

#ifdef INTERN_GRADIENT
	{
		class root_bitmap RB;
		HBITMAP bmp = RB.make_root_bmp(command);
		if (NULL != bmp) return bmp;
	}
#endif

	char exe_path[MAX_PATH];
	make_bb_path(exe_path, "bsetroot.exe");

	unsigned long bsrt_vernum = getfileversion(exe_path, NULL);
	if (bsrt_vernum < 0x02000000)
	{
		BBExecute_string(command, true);
		return NULL;
	}

	char bsetroot_bmp[MAX_PATH];
	make_bb_path(bsetroot_bmp, "$bsroot$.bmp");

	char spawn_cmd[1024];
	sprintf(spawn_cmd, "\"%s\" %s -save \"%s\"",
		exe_path, cptr, bsetroot_bmp);

	char bb_path[MAX_PATH];
	GetBlackboxPath(bb_path, MAX_PATH);

	//dbg_printf("command <%s>", spawn_cmd);
	if (false == ShellCommand(spawn_cmd, bb_path, true))
	{
		BBMessageBox(MB_OK, NLS2("$BBError_rootCommand$",
			"Error: Could not execute rootCommand:\n%s"), spawn_cmd);
		return NULL;
	}

	return read_bitmap(bsetroot_bmp, true);
}

//===========================================================================
#endif
