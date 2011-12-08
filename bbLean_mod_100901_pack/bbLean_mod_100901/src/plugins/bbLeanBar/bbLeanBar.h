#ifndef __BBLEANBAR_H
#define __BBLEANBAR_H

#define BB_WINDOWDEFAULT 10618
#define BB_SYSMENU 10619

#define _CopyRect(lprcDst, lprcSrc) (*lprcDst) = (*lprcSrc)
#define _InflateRect(lprc, dx, dy) (*lprc).left -= (dx), (*lprc).right += (dx), (*lprc).top -= (dy), (*lprc).bottom += (dy)
#define _OffsetRect(lprc, dx, dy) (*lprc).left += (dx), (*lprc).right += (dx), (*lprc).top += (dy), (*lprc).bottom += (dy)
#define _SetRect(lprc, xLeft, yTop, xRight, yBottom) (*lprc).left = (xLeft), (*lprc).right = (xRight), (*lprc).top = (yTop), (*lprc).bottom = (yBottom)
#define _CopyOffsetRect(lprcDst, lprcSrc, dx, dy) (*lprcDst).left = (*lprcSrc).left + (dx), (*lprcDst).right = (*lprcSrc).right + (dx), (*lprcDst).top = (*lprcSrc).top + (dy), (*lprcDst).bottom = (*lprcSrc).bottom + (dy)

int nBalloonTimer;
bool ShowTrayIcon;

typedef struct IconList
{
	struct IconList *next;
	systemTray* icon;
} IconList;

IconList *HideIconList;
IconList *TrayIconList;

bool find_node(void *a0, const void *e0);
BOOL GetFileNameFromHwnd(HWND hWnd, LPTSTR lpszFileName, DWORD nSize);
LPCSTR set_my_path(char *path, char *fname);
void show_tray_menu(bool popup);
string_node *exclusions_list;
LPCSTR ReadMouseAction(LPARAM button);
unsigned get_modkeys(void);
int BBDrawText(HDC hDC, LPCTSTR lpString, int nCount, LPRECT lpRect, UINT uFormat, StyleItem* pSI);

static const struct corebroam_table
{
	char *str; unsigned short msg; short wParam;
}
	corebroam_table [] =
{
	{ "Default",             BB_WINDOWDEFAULT,        0 },
	{ "SystemMenu",          BB_SYSMENU,              0 },
	{ "Raise",               BB_WINDOWRAISE,          0 },
	{ "RaiseWindow",         BB_WINDOWRAISE,          0 },
	{ "Lower",               BB_WINDOWLOWER,          0 },
	{ "LowerWindow",         BB_WINDOWLOWER,          0 },
	{ "Shade",               BB_WINDOWSHADE,          0 },
	{ "ShadeWindow",         BB_WINDOWSHADE,          0 },
	{ "Close",               BB_WINDOWCLOSE,          0 },
	{ "CloseWindow",         BB_WINDOWCLOSE,          0 },
	{ "Minimize",            BB_WINDOWMINIMIZE,       0 },
	{ "MinimizeWindow",      BB_WINDOWMINIMIZE,       0 },
	{ "MinimizeToTray",      BB_WINDOWMINIMIZETOTRAY, 0 },
	{ "Maximize",            BB_WINDOWMAXIMIZE,       0 },
	{ "MaximizeWindow",      BB_WINDOWMAXIMIZE,       0 },
	{ "MaximizeVertical",    BB_WINDOWGROWHEIGHT,     0 },
	{ "MaximizeHorizontal",  BB_WINDOWGROWWIDTH,      0 },
	{ "Restore",             BB_WINDOWRESTORE,        0 },
	{ "RestoreWindow",       BB_WINDOWRESTORE,        0 },

	{ "PrevWindow",          BB_WORKSPACE,            BBWS_PREVWINDOW },
	{ "NextWindow",          BB_WORKSPACE,            BBWS_NEXTWINDOW },
	{ "StickWindow",         BB_WORKSPACE,            BBWS_TOGGLESTICKY },

	// workspaces
	{ "MoveWindowLeft",      BB_WORKSPACE,            BBWS_MOVEWINDOWLEFT },
	{ "MoveWindowRight",     BB_WORKSPACE,            BBWS_MOVEWINDOWRIGHT },

	{ "ActivateTask",        BB_BRINGTOFRONT,         0},
	{ "",                    0,                       0},

	{ NULL /*"Workspace#"*/, BB_WORKSPACE,            BBWS_SWITCHTODESK },
};

#endif /* __BBLEANBAR_H */
