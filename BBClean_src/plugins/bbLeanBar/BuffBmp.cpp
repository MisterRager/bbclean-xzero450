//===========================================================================
// Auto - buffered wrapper for MakeStyleGradient

// call instead:
// void BuffBmp::MakeStyleGradient(HDC hdc, RECT *rc, StyleItem *pSI, bool bordered);

// call after everything is drawn, and once at end of program to cleanup
// void BuffBmp::ClearBitmaps(void);

class BuffBmp
{
private:
	struct Bmp
	{
		struct Bmp *next;
		RECT r;
		StyleItem s_SI;
		HBITMAP bmp;
		bool borderMode;
		bool in_use;
	};
	struct Bmp *g_Buffers;

public:
	BuffBmp()
	{
		g_Buffers = NULL;
	}

	~BuffBmp()
	{
		ClearBitmaps();
	}

	void MakeStyleGradient(HDC hdc, RECT *rc, StyleItem *pSI, bool borderMode)
	{
		if (is_bblean && pSI->nVersion >= 2) borderMode |= pSI->bordered;

		if (pSI->parentRelative)
		{
			COLORREF borderColor; int borderWidth;
			if (borderMode)
			{
				if (is_bblean && pSI->nVersion >= 2 && pSI->borderWidth)
				{
					borderColor = pSI->borderColor;
					borderWidth = pSI->borderWidth;
				}
				else
				{
					borderColor = styleBorderColor;
					borderWidth = styleBorderWidth;
				}
				CreateBorder(hdc, rc, borderColor, borderWidth);
			}
			return;
		}

		int width   = rc->right - rc->left;
		int height  = rc->bottom - rc->top;

		struct Bmp * B;
		dolist (B, g_Buffers)
			if (B->r.right == width
			 && B->r.bottom == height
			 && B->borderMode == borderMode
			 && 0 == memcmp(pSI, &B->s_SI, sizeof B->s_SI)
			 ) break;

		HDC buf = CreateCompatibleDC(NULL);
		HGDIOBJ other;

		if (NULL == B)
		{
			B = new struct Bmp;
			B->r.left =
			B->r.top = 0;
			B->r.right = width;
			B->r.bottom = height;
			B->borderMode = borderMode;
			B->s_SI = *pSI;
			B->bmp = CreateCompatibleBitmap(hdc, width, height);
			B->next = g_Buffers;
			g_Buffers = B;

			other = SelectObject(buf, B->bmp);
			::MakeStyleGradient(buf, &B->r, pSI, borderMode);

			//dbg_printf("new bitmap %d %d", width, height);
		}
		else
		{
			other = SelectObject(buf, B->bmp);
		}

		B->in_use = true;

		BitBlt(hdc, rc->left, rc->top, width, height, buf, 0, 0, SRCCOPY);
		SelectObject(buf, other);
		DeleteDC(buf);
	}

	void ClearBitmaps(void)
	{
		struct Bmp *B, **pB = &g_Buffers;
		while (NULL != (B=*pB))
		{
			if (false == B->in_use)
			{
				*pB = B->next;
				DeleteObject(B->bmp);
				delete B;
			}
			else
			{
				B->in_use = false;
				pB = &B->next;
			}
		}
	}

};

//===========================================================================
