//===========================================================================
// some utilities

char *strcpy_max(char *dest, const char *src, int maxlen)
{
	int n = strlen(src);
	if (n >= maxlen) n = maxlen-1;
	memcpy(dest, src, n);
	dest[n] = 0;
	return dest;
}

//===========================================================================
// Function: get_fontheight
//===========================================================================
int get_fontheight(HFONT hFont)
{
	TEXTMETRIC TXM;
	HDC hdc = CreateCompatibleDC(NULL);
	HGDIOBJ other = SelectObject(hdc, hFont);
	GetTextMetrics(hdc, &TXM);
	SelectObject(hdc, other);
	DeleteDC(hdc);
	return TXM.tmHeight;// - TXM.tmInternalLeading - TXM.tmExternalLeading;
}

//===========================================================================

#define _OffsetRect(lprc, dx, dy) (*lprc).left += (dx), (*lprc).right += (dx), (*lprc).top += (dy), (*lprc).bottom += (dy)

int BBDrawText(HDC hDC, LPCTSTR lpString, int nCount, LPRECT lpRect, UINT uFormat, StyleItem * pSI)
{

	/*if (pSI->validated & VALID_OUTLINECOLOR) {	
		COLORREF cr0;
		RECT rcOutline;
        //_CopyRect(&rcOutline, r);
		rcOutline.bottom = lpRect->bottom;
		rcOutline.top = lpRect->top;
		rcOutline.left = lpRect->left;
		rcOutline.right = lpRect->right;
		cr0 = SetTextColor(hDC, pSI->OutlineColor);
        //SetTextColor(m_hSecondaryBuf, cr0);
        rcOutline.left--, rcOutline.right--, rcOutline.top--, rcOutline.bottom--;
        for (int i = 0; i < 3; i++){
            for (int j = 0; j < 3; j++){
                DrawText(hDC, lpString, -1, &rcOutline,  uFormat);
                _OffsetRect(&rcOutline, 0, 1);
            }
            _OffsetRect(&rcOutline, 1, -3);
        }
	}*/
	
	if (pSI->validated & VALID_OUTLINECOLOR){ // draw outline
        RECT rcOutline;
        SetTextColor(hDC, pSI->OutlineColor);
        //_CopyRect(&rcOutline, r);
		rcOutline.bottom = lpRect->bottom;
		rcOutline.top = lpRect->top;
		rcOutline.left = lpRect->left+1;
		rcOutline.right = lpRect->right+1;
        DrawText(hDC, lpString, nCount, &rcOutline, uFormat); _OffsetRect(&rcOutline,   0,  1);
        DrawText(hDC, lpString, nCount, &rcOutline, uFormat); _OffsetRect(&rcOutline,  -1,  0);
        DrawText(hDC, lpString, nCount, &rcOutline, uFormat); _OffsetRect(&rcOutline,  -1,  0);
        DrawText(hDC, lpString, nCount, &rcOutline, uFormat); _OffsetRect(&rcOutline,   0, -1);
        DrawText(hDC, lpString, nCount, &rcOutline, uFormat); _OffsetRect(&rcOutline,   0, -1);
        DrawText(hDC, lpString, nCount, &rcOutline, uFormat); _OffsetRect(&rcOutline,   1,  0);
        DrawText(hDC, lpString, nCount, &rcOutline, uFormat); _OffsetRect(&rcOutline,   1,  0);
        DrawText(hDC, lpString, nCount, &rcOutline, uFormat);
    }
	
	if (pSI->ShadowXY)
	{
		RECT Rs;
		int i;
		COLORREF cr0;
		i = pSI->ShadowY;
		Rs.top = lpRect->top + i;
		Rs.bottom = lpRect->bottom + i;
		i = pSI->ShadowX;
		Rs.left = lpRect->left + i;
		Rs.right = lpRect->right + i;
		cr0 = SetTextColor(hDC, pSI->ShadowColor);
		DrawText(hDC, lpString, nCount, &Rs, uFormat);
		//SetTextColor(hDC, cr0);
	}
    // draw text
    SetTextColor(hDC, pSI->TextColor);
	return DrawText(hDC, lpString, nCount, lpRect, uFormat);
}

//===========================================================================

void dbg_printf (const char *fmt, ...)
{
	char buffer[256]; va_list arg;
	va_start(arg, fmt);
	vsprintf (buffer, fmt, arg);
	strcat(buffer, "\n");
	OutputDebugString(buffer);
}

//===========================================================================

#if 1
void EnumTasks (TASKENUMPROC lpEnumFunc, LPARAM lParam)
{
	struct tasklist *tl;
	dolist (tl, GetTaskListPtr())
		if (FALSE == lpEnumFunc(tl, lParam))
			break;
}
#endif

#if 0
void EnumTray (TRAYENUMPROC lpEnumFunc, LPARAM lParam)
{
	for (int i = 0, s = GetTraySize(); i < s; i++)
		if (FALSE == lpEnumFunc(GetTrayIcon(i), lParam))
			break;
}

void EnumDesks (DESKENUMPROC lpEnumFunc, LPARAM lParam)
{
	DesktopInfo info;
	GetDesktopInfo(&info);
	string_node *p = info.deskNames;
	for (int n = 0; n < info.ScreensX; n++)
	{
		DesktopInfo DI;
		DI.number = n;
		DI.deskNames = info.deskNames;
		DI.isCurrent = n == info.number;
		DI.name[0] = 0;
		if (p) strcpy(DI.name, p->str), p = p->next;

		if (FALSE == lpEnumFunc(&DI, lParam))
			break;
	}
}

#endif

//===========================================================================
