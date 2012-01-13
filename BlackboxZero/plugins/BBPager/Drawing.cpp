#include "BBPager.h"

//using namespace std;

//===========================================================================

// Desktop information
int currentDesktop;
//RECT desktopRect[64];
vector<RECT> desktopRect;

char desktopNumber[4];

int col, row, currentCol, currentRow;

//===========================================================================

void DrawBBPager(HWND hwnd)
{
	// Create buffer hdc's, bitmaps etc.
	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(hwnd, &ps);
	HDC buf = CreateCompatibleDC(NULL);
	HBITMAP bufbmp = CreateCompatibleBitmap(hdc, frame.width, frame.height);
	HGDIOBJ oldbmp = SelectObject(buf, bufbmp);
	RECT r;
	char toolText[256];

	GetClientRect(hwnd, &r);

	// Paint background and border according to the current style...
	MakeGradient(buf, r, frame.style->type, frame.color, frame.colorTo, frame.style->interlaced, frame.style->bevelstyle, frame.style->bevelposition, frame.bevelWidth, frame.borderColor, frame.borderWidth);

	HFONT font = CreateFont(desktop.fontSize, 0, 0, 0, desktop.fontWeight, false, false, false, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH|FF_DONTCARE, desktop.fontFace);
	HGDIOBJ oldfont = SelectObject(buf, font);
	SetBkMode(buf, TRANSPARENT);
	SetTextColor(buf, desktop.fontColor);

	desktopRect.clear();

	// Paint desktops :D
	if (position.horizontal) 
	{
		// Do loop to draw desktops other than current selected desktop
		int i = 0;

		do 
		{
			col = i / frame.rows;
			row = i % frame.rows + 1;

			if (currentDesktop == i) 
			{
				currentCol = col;
				currentRow = row;
			}
			else 
			{
				r.left = frame.borderWidth + frame.bevelWidth + ((col) * (desktop.width + frame.bevelWidth));
				r.right = r.left + desktop.width;
				r.top = frame.borderWidth + frame.bevelWidth + ((row - 1) * (desktop.height + frame.bevelWidth));
				r.bottom = r.top + desktop.height;

				//desktopRect[i] = r; // set RECT item for this desktop
				//desktopRect.insert(desktopRect.begin() + i - 1, r);
				desktopRect.push_back(r);

				if (!desktop.style->parentRelative)
					MakeGradient(buf, r, desktop.style->type, desktop.color, desktop.colorTo, desktop.style->interlaced, desktop.style->bevelstyle, desktop.style->bevelposition, frame.bevelWidth, frame.borderColor, 0);						
							
				if (desktop.numbers) 
				{
					sprintf(desktopNumber, "%d", (i + 1));
					DrawText(buf, desktopNumber, -1, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_WORD_ELLIPSIS | DT_NOPREFIX);
					//DrawText(buf, desktopNumber, strlen(desktopNumber), &r, DT_CALCRECT|DT_NOPREFIX);
				}
			}
			i++;
		}
		while (i < desktops);

		// Do this now so bordered desktop is drawn last
		i = currentDesktop;

		r.left = frame.borderWidth + frame.bevelWidth + ((currentCol) * (desktop.width + frame.bevelWidth));
		r.right = r.left + desktop.width;
		r.top = frame.borderWidth + frame.bevelWidth + ((currentRow - 1) * (desktop.height + frame.bevelWidth));
		r.bottom = r.top + desktop.height;

		//desktopRect[i] = r; // set RECT item for this desktop

		DrawActiveDesktop(buf, r, i);
		
		if (desktop.numbers) 
		{
			sprintf(desktopNumber, "%d", (i + 1));
			SetTextColor(buf, activeDesktop.borderColor);
			DrawText(buf, desktopNumber, -1, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_WORD_ELLIPSIS | DT_NOPREFIX);
			//DrawText(buf, desktopNumber, strlen(desktopNumber), &r, DT_CALCRECT|DT_NOPREFIX);
			//SetTextColor(buf, desktop.fontColor);
		}
	}
	else if (position.vertical) 
	{
		// Do loop to draw desktops other than current selected desktop
		int i = 0;

		do 
		{					
			row = i / frame.columns;
			col = i % frame.columns + 1;

			if (currentDesktop == i) 
			{
				currentCol = col;
				currentRow = row;
			}
			else 
			{
				r.left = frame.borderWidth + frame.bevelWidth + ((col - 1) * (desktop.width + frame.bevelWidth));
				r.right = r.left + desktop.width;
				r.top = frame.borderWidth + frame.bevelWidth + ((row) * (desktop.height + frame.bevelWidth));
				r.bottom = r.top + desktop.height;

				//desktopRect[i] = r; // set RECT item for this desktop
				//desktopRect.insert(desktopRect.begin() + i - 1, r);
				desktopRect.push_back(r);

				if (!desktop.style->parentRelative)
					MakeGradient(buf, r, desktop.style->type, desktop.color, desktop.colorTo, desktop.style->interlaced, desktop.style->bevelstyle, desktop.style->bevelposition, frame.bevelWidth, frame.borderColor, 0);						
				
				if (desktop.numbers) 
				{
					sprintf(desktopNumber, "%d", (i + 1));
					DrawText(buf, desktopNumber, -1, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_WORD_ELLIPSIS | DT_NOPREFIX);
					//DrawText(buf, desktopNumber, strlen(desktopNumber), &r, DT_CALCRECT|DT_NOPREFIX);
				}
			}
			i++;
		}
		while (i < desktops);

		// Do this now so bordered desktop is drawn last
		i = currentDesktop;

		r.left = frame.borderWidth + frame.bevelWidth + ((currentCol - 1) * (desktop.width + frame.bevelWidth));
		r.right = r.left + desktop.width;
		r.top = frame.borderWidth + frame.bevelWidth + ((currentRow) * (desktop.height + frame.bevelWidth));
		r.bottom = r.top + desktop.height;

		//desktopRect[i] = r; // set RECT item for this desktop

		DrawActiveDesktop(buf, r, i);

		if (desktop.numbers) 
		{
			sprintf(desktopNumber, "%d", (i + 1));
			SetTextColor(buf, activeDesktop.borderColor);
			DrawText(buf, desktopNumber, -1, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_WORD_ELLIPSIS | DT_NOPREFIX);
			//DrawText(buf, desktopNumber, strlen(desktopNumber), &r, DT_CALCRECT|DT_NOPREFIX);
			//SetTextColor(buf, desktop.fontColor);
		}
	}

	//DeleteObject(font);
	DeleteObject(SelectObject(buf, oldfont));

	// Draw windows on workspaces if wanted
	if (desktop.windows)
	{
		winCount = 0; // Reset number of windows to 0 on each paint to be counted by...
		winList.clear();

		// ... this function which passes HWNDs to CheckTaskEnumProc callback procedure
		if (usingBBLean && usingAltMethod) 
			EnumWindows(CheckTaskEnumProc_AltMethod, 0);
		else
			EnumWindows(CheckTaskEnumProc, 0); 

		//struct tasklist *tlist;
		/*tl = GetTaskListPtr();
		while (tl)
		{
			AddBBWindow(tl);
			tl = tl->next;
		}*/

		// Only paint windows if there are any!
		if (winCount > 0)
		{
			// Start at end of list (bottom of zorder)
			for (int i = (winCount - 1);i > -1;i--)
			{
				RECT win = winList[i].r;
				RECT desk = desktopRect[winList[i].desk];

				if (win.right - win.left <= 1 && win.bottom - win.top <= 1)
					continue;
				
				// This is done so that windows only show within the applicable desktop RECT
				if (win.top < desk.top) 
					win.top = desk.top; // + 1;

				if (win.right > desk.right) 
					win.right = desk.right; // - 1;

				if (win.bottom > desk.bottom) 
					win.bottom = desk.bottom; // - 1;

				if (win.left < desk.left) 
					win.left = desk.left; // + 1;

				if (winList[i].sticky)
				{
					RECT sWin;
					RECT sDesk;
					win.bottom = win.bottom - desk.top;
					win.top = win.top - desk.top;
					win.left = win.left - desk.left;
					win.right = win.right - desk.left;

					for (int j = 0; j < desktops; j++)
					{
						sDesk = desktopRect[j];
						sWin.bottom = sDesk.top + win.bottom;
						sWin.top = sDesk.top + win.top;
						sWin.left = sDesk.left + win.left;
						sWin.right = sDesk.left + win.right;

						if (winList[i].active) // draw active window style
						{
							DrawActiveWindow(buf, sWin);
							RemoveFlash(winList[i].window, true);
						}
						else if (IsFlashOn(winList[i].window))
						{
							DrawActiveWindow(buf, sWin);
						}
						else // draw inactive window style
						{
							DrawInactiveWindow(buf, sWin);
							RemoveFlash(winList[i].window, true);
						}

						// Create a tooltip...
						if (desktop.tooltips)
						{
							GetWindowText(winList[i].window, toolText, 255);
							SetToolTip(&sWin, toolText);
						}
					}
				}
				else
				{
					if (winList[i].active) // draw active window style
					{
						DrawActiveWindow(buf, win);
						RemoveFlash(winList[i].window, true);
					}
					else if (IsFlashOn(winList[i].window))
					{
						DrawActiveWindow(buf, win);
					}
					else // draw inactive window style
					{
						DrawInactiveWindow(buf, win);
						RemoveFlash(winList[i].window, true);
					}

					// Create a tooltip...
					if (desktop.tooltips)
					{
						GetWindowText(winList[i].window, toolText, 255);
						SetToolTip(&win, toolText);
					}
				}
			}
		}

		if (winMoving)
		{
			RECT win = moveWin.r;
			RECT client;

			GetClientRect(hwndBBPager, &client);
		
			// This is done so that the window only shows within the pager
			if (win.top < client.top) 
				win.top = client.top; // + 1;

			if (win.right > client.right) 
				win.right = client.right; // - 1;

			if (win.bottom > client.bottom) 
				win.bottom = client.bottom; // - 1;

			if (win.left < client.left) 
				win.left = client.left; // + 1;

			if (moveWin.active) // draw active window style
				DrawActiveWindow(buf, win);
			else // draw inactive window style
				DrawInactiveWindow(buf, win);
		}

	}

	ClearToolTips();

	// Finally, copy from the paint buffer to the window...
	BitBlt(hdc, 0, 0, frame.width, frame.height, buf, 0, 0, SRCCOPY);

	// Remember to delete #all# objects!
/*	SelectObject(buf, oldbuf);
	DeleteDC(buf);
	DeleteObject(bufbmp);
	DeleteObject(oldbuf);
	EndPaint(hwnd, &ps);
*/
    //restore the first previous whatever to the dc,
    //get in exchange back our bitmap, and delete it.
    DeleteObject(SelectObject(buf, oldbmp));

    //delete the memory - 'device context'
    DeleteDC(buf);

    //done
    EndPaint(hwnd, &ps);
}

//===========================================================================

void DrawActiveWindow(HDC buf, RECT r)
{
	COLORREF bColor;

	// Checks for windows just showing on the edges of the screen
	if (r.bottom - r.top < 2)
	{
		if (!_stricmp(focusedWindow.styleType, "border"))
			bColor = focusedWindow.borderColor;
		else
			bColor = window.borderColor;

		HPEN borderPen = CreatePen(PS_SOLID, 1, bColor);
		HPEN oldPen = (HPEN) SelectObject(buf, borderPen);

		MoveToEx(buf, r.left, r.top, NULL);
		LineTo(buf, r.right, r.top);

		SelectObject(buf,oldPen);
		DeleteObject(borderPen);

		//MessageBox(0, "Warning sir!\n\nCan't draw this window as a RECT dude!", "DrawActiveWindow", MB_OK | MB_TOPMOST | MB_SETFOREGROUND);

		return;
	}

	if (r.right - r.left < 2)
	{
		if (!_stricmp(focusedWindow.styleType, "border"))
			bColor = focusedWindow.borderColor;
		else
			bColor = window.borderColor;

		HPEN borderPen = CreatePen(PS_SOLID, 1, bColor);
		HPEN oldPen = (HPEN) SelectObject(buf, borderPen);

		MoveToEx(buf, r.left, r.top, NULL);
		LineTo(buf, r.left, r.bottom);

		SelectObject(buf,oldPen);
		DeleteObject(borderPen);

		return;
	}

	if (!_stricmp(focusedWindow.styleType, "texture"))
		MakeGradient(buf, r, focusedWindow.style->type, focusedWindow.color, focusedWindow.colorTo, focusedWindow.style->interlaced, focusedWindow.style->bevelstyle, focusedWindow.style->bevelposition, frame.bevelWidth, window.borderColor, 1);
	else if (!_stricmp(focusedWindow.styleType, "border"))
		MakeGradient(buf, r, window.style->type, window.color, window.colorTo, window.style->interlaced, window.style->bevelstyle, window.style->bevelposition, frame.bevelWidth, focusedWindow.borderColor, 1);
	else
		MakeGradient(buf, r, window.style->type, window.color, window.colorTo, window.style->interlaced, window.style->bevelstyle, window.style->bevelposition, frame.bevelWidth, window.borderColor, 1);
}

void DrawInactiveWindow(HDC buf, RECT r)
{
	if (r.bottom - r.top < 2)
	{
		HPEN borderPen = CreatePen(PS_SOLID, 1, window.borderColor);
		HPEN oldPen = (HPEN) SelectObject(buf, borderPen);

		MoveToEx(buf, r.left, r.top, NULL);
		LineTo(buf, r.right, r.top);

		SelectObject(buf,oldPen);
		DeleteObject(borderPen);

		return;
	}

	if (r.right - r.left < 2)
	{
		HPEN borderPen = CreatePen(PS_SOLID, 1, window.borderColor);
		HPEN oldPen = (HPEN) SelectObject(buf, borderPen);

		MoveToEx(buf, r.left, r.top, NULL);
		LineTo(buf, r.left, r.bottom);

		SelectObject(buf,oldPen);
		DeleteObject(borderPen);

		return;
	}

	MakeGradient(buf, r, window.style->type, window.color, window.colorTo, window.style->interlaced, window.style->bevelstyle, window.style->bevelposition, frame.bevelWidth, window.borderColor, 1);
}

//===========================================================================

void DrawActiveDesktop(HDC buf, RECT r, int i)
{
	if (!_stricmp(activeDesktop.styleType, "border")) 
	{
		r.right = r.right + 2;
		r.bottom = r.bottom + 2;

		/*ratioX = screenWidth / (desktop.width + 2);
		ratioY = screenHeight / (desktop.height + 2);*/
		ratioX = vScreenWidth / (desktop.width + 2);
		ratioY = vScreenHeight / (desktop.height + 2);

		if (desktop.style->parentRelative)
			DrawBorder(buf, r, activeDesktop.borderColor, 1);
		else
			MakeGradient(buf, r, desktop.style->type, desktop.color, desktop.colorTo, desktop.style->interlaced, desktop.style->bevelstyle, desktop.style->bevelposition, frame.bevelWidth, activeDesktop.borderColor, 1);
	}
	else if (!_stricmp(activeDesktop.styleType, "border2")) 
	{
		r.left = r.left - 1;
		r.top = r.top - 1;
		r.right = r.right + 1;
		r.bottom = r.bottom + 1;

		ratioX = vScreenWidth / (desktop.width + 2);
		ratioY = vScreenHeight / (desktop.height + 2);

		if (desktop.style->parentRelative)
			DrawBorder(buf, r, activeDesktop.borderColor, 1);
		else
			MakeGradient(buf, r, desktop.style->type, desktop.color, desktop.colorTo, desktop.style->interlaced, desktop.style->bevelstyle, desktop.style->bevelposition, frame.bevelWidth, activeDesktop.borderColor, 1);
	}
	else if (!_stricmp(activeDesktop.styleType, "border3")) 
	{
		if (desktop.style->parentRelative)
			DrawBorder(buf, r, activeDesktop.borderColor, 1);
		else
			MakeGradient(buf, r, desktop.style->type, desktop.color, desktop.colorTo, desktop.style->interlaced, desktop.style->bevelstyle, desktop.style->bevelposition, frame.bevelWidth, activeDesktop.borderColor, 1);
	}
	else if (!_stricmp(activeDesktop.styleType, "texture")) 
	{
		if (activeDesktop.useDesktopStyle)
		{
			if (!desktop.style->parentRelative)
				MakeGradient(buf, r, desktop.style->type, desktop.color, desktop.colorTo, desktop.style->interlaced, desktop.style->bevelstyle, desktop.style->bevelposition, frame.bevelWidth, activeDesktop.borderColor, 0);
		}
		else if (!activeDesktop.style->parentRelative)
			MakeGradient(buf, r, activeDesktop.style->type, activeDesktop.color, activeDesktop.colorTo, activeDesktop.style->interlaced, activeDesktop.style->bevelstyle, activeDesktop.style->bevelposition, frame.bevelWidth, activeDesktop.borderColor, 0);
	}
	else if (!_stricmp(activeDesktop.styleType, "none"))
	{
		if (!desktop.style->parentRelative)
			MakeGradient(buf, r, desktop.style->type, desktop.color, desktop.colorTo, desktop.style->interlaced, desktop.style->bevelstyle, desktop.style->bevelposition, frame.bevelWidth, activeDesktop.borderColor, 0);
	}

	//desktopRect[i] = r;
	desktopRect.insert(desktopRect.begin() + i, r);
}

//===========================================================================
// CreateBorder takes a HDC and draws a border at the edges...

void DrawBorder(HDC hdc, RECT rect, COLORREF borderColour, int borderWidth)
{
	HPEN borderPen = CreatePen(PS_SOLID, 1, borderColour);
	HPEN oldPen = (HPEN) SelectObject(hdc, borderPen);

	for (int i = 0; i < borderWidth; i++)
	{
		int right = rect.right - i - 1;
		int bottom = rect.bottom - i - 1;

		MoveToEx(hdc, rect.left + i, rect.top + i, NULL);
		LineTo(hdc, right, rect.top + i);
		LineTo(hdc, right, bottom);
		LineTo(hdc, rect.left + i, bottom);
		LineTo(hdc, rect.left + i, rect.top + i);
	}

	SelectObject(hdc,oldPen);
	DeleteObject(borderPen);
}