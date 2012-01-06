// xImaSel.cpp : Selection functions
/* 07/08/2001 v1.00 - ing.davide.pizzolato@libero.it
 * CxImage version 5.50 07/Jan/2003
 */

#include "ximage.h"

#if CXIMAGE_SUPPORT_SELECTION

////////////////////////////////////////////////////////////////////////////////
bool CxImage::SelectionClear()
{
	if (pSelection){
		memset(pSelection,0,head.biWidth * head.biHeight);
		info.rSelectionBox.left = head.biWidth;
		info.rSelectionBox.bottom = head.biHeight;
		info.rSelectionBox.right = info.rSelectionBox.top = 0;
		return true;
	}
	return false;
}
////////////////////////////////////////////////////////////////////////////////
bool CxImage::SelectionCreate()
{
	SelectionDelete();
	pSelection = (BYTE*)calloc(head.biWidth * head.biHeight, 1);
	return (pSelection!=0);
}
////////////////////////////////////////////////////////////////////////////////
bool CxImage::SelectionDelete()
{
	if (pSelection){ free(pSelection); pSelection=NULL; }
	info.rSelectionBox.left = head.biWidth;
	info.rSelectionBox.bottom = head.biHeight;
	info.rSelectionBox.right = info.rSelectionBox.top = 0;
	return true;
}
////////////////////////////////////////////////////////////////////////////////
bool CxImage::SelectionIsInside(long x, long y)
{
	if (IsInside(x,y)){
		if (pSelection==NULL) return true;
		return pSelection[x+y*head.biWidth]!=0;
	}
	return false;
}
////////////////////////////////////////////////////////////////////////////////
bool CxImage::SelectionAddRect(RECT r)
{
	if (pSelection==NULL) SelectionCreate();

	RECT r2;
	if (r.left<r.right) {r2.left=r.left; r2.right=r.right; } else {r2.left=r.right ; r2.right=r.left; }
	if (r.bottom<r.top) {r2.bottom=r.bottom; r2.top=r.top; } else {r2.bottom=r.top ; r2.top=r.bottom; }

	if (info.rSelectionBox.top < r2.left) info.rSelectionBox.top = max(0L,min(head.biHeight,r2.top));
	if (info.rSelectionBox.left > r2.left) info.rSelectionBox.left = max(0L,min(head.biWidth,r2.left));
	if (info.rSelectionBox.right < r2.left) info.rSelectionBox.right = max(0L,min(head.biWidth,r2.right));
	if (info.rSelectionBox.bottom > r2.bottom) info.rSelectionBox.bottom = max(0L,min(head.biHeight,r2.bottom));

	long ymin = max(0L,min(head.biHeight,r2.bottom));
	long ymax = max(0L,min(head.biHeight,r2.top));
	long xmin = max(0L,min(head.biWidth,r2.left));
	long xmax = max(0L,min(head.biWidth,r2.right));

	for (long y=ymin; y<ymax; y++)
		memset(pSelection + xmin + y * head.biWidth, 255, xmax-xmin);

	return true;
}
////////////////////////////////////////////////////////////////////////////////
bool CxImage::SelectionAddEllipse(RECT r)
{
	if (pSelection==NULL) SelectionCreate();

	long xradius = abs(r.right - r.left)/2;
	long yradius = abs(r.top - r.bottom)/2;
	if (xradius==0 || yradius==0) return false;

	long xcenter = (r.right + r.left)/2;
	long ycenter = (r.top + r.bottom)/2;

	if (info.rSelectionBox.left > (xcenter - xradius)) info.rSelectionBox.left = max(0L,min(head.biWidth,(xcenter - xradius)));
	if (info.rSelectionBox.right < (xcenter + xradius)) info.rSelectionBox.right = max(0L,min(head.biWidth,(xcenter + xradius)));
	if (info.rSelectionBox.bottom > (ycenter - yradius)) info.rSelectionBox.bottom = max(0L,min(head.biHeight,(ycenter - yradius)));
	if (info.rSelectionBox.top < (ycenter + yradius)) info.rSelectionBox.top = max(0L,min(head.biHeight,(ycenter + yradius)));

	long xmin = max(0L,min(head.biWidth,xcenter - xradius));
	long xmax = max(0L,min(head.biWidth,xcenter + xradius));
	long ymin = max(0L,min(head.biHeight,ycenter - yradius));
	long ymax = max(0L,min(head.biHeight,ycenter + yradius));

	long y,yo;
	for (y=ymin; y<ycenter; y++){
		for (long x=xmin; x<xmax; x++){
			yo = (long)(ycenter - yradius * sqrt(1-pow((float)(x - xcenter)/(float)xradius,2)));
			if (yo<y) pSelection[x + y * head.biWidth] = 255;
		}
	}
	for (y=ycenter; y<ymax; y++){
		for (long x=xmin; x<xmax; x++){
			yo = (long)(ycenter + yradius * sqrt(1-pow((float)(x - xcenter)/(float)xradius,2)));
			if (yo>y) pSelection[x + y * head.biWidth] = 255;
		}
	}
	return true;
}
////////////////////////////////////////////////////////////////////////////////
bool CxImage::SelectionInvert()
{
	if (pSelection) {
		BYTE *iSrc=pSelection;
		long n=head.biHeight*head.biWidth;
		for(long i=0; i < n; i++){
			*iSrc=(BYTE)~(*(iSrc));
			iSrc++;
		}
		return true;
	}
	return false;
}
////////////////////////////////////////////////////////////////////////////////
bool CxImage::SelectionCopy(CxImage &from)
{
	if (from.pSelection == NULL || head.biWidth != from.head.biWidth || head.biWidth != from.head.biWidth) return false;
	if (pSelection==NULL) pSelection = (BYTE*)malloc(head.biWidth * head.biHeight);
	memcpy(pSelection,from.pSelection,head.biWidth * head.biHeight);
	memcpy(&info.rSelectionBox,&from.info.rSelectionBox,sizeof(RECT));
	return true;
}
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool CxImage::SelectionAddPolygon(POINT *points, long npoints)
{
	if (points==NULL || npoints<3) return false;

	if (pSelection==NULL) SelectionCreate();
	BYTE* plocal = (BYTE*)calloc(head.biWidth*head.biHeight, 1);
	RECT localbox = {head.biWidth,0,0,head.biHeight};

	long i=0;
	POINT *current,*next,*start;
	//trace contour
	while (i < npoints){
		current = &points[i];
		if (current->x!=-1){
			if (i==0 || (i>0 && points[i-1].x==-1)) start = &points[i];

			if ((i+1)==npoints || points[i+1].x==-1)
				next = start;
			else
				next = &points[i+1];

			long x,y;
			float beta;
			if (current->x != next->x){
				beta = (float)(next->y - current->y)/(float)(next->x - current->x);
				if (current->x < next->x){
					for (x=current->x; x<=next->x; x++){
						y = (long)(current->y + (x - current->x) * beta);
						if (IsInside(x,y)) plocal[x + y * head.biWidth] = 255;
					}
				} else {
					for (x=current->x; x>=next->x; x--){
						y = (long)(current->y + (x - current->x) * beta);
						if (IsInside(x,y)) plocal[x + y * head.biWidth] = 255;
					}
				}
			}
			if (current->y != next->y){
				beta = (float)(next->x - current->x)/(float)(next->y - current->y);
				if (current->y < next->y){
					for (y=current->y; y<=next->y; y++){
						x = (long)(current->x + (y - current->y) * beta);
						if (IsInside(x,y)) plocal[x + y * head.biWidth] = 255;
					}
				} else {
					for (y=current->y; y>=next->y; y--){
						x = (long)(current->x + (y - current->y) * beta);
						if (IsInside(x,y)) plocal[x + y * head.biWidth] = 255;
					}
				}
			}
		}

		RECT r2;
		if (current->x < next->x) {r2.left=current->x; r2.right=next->x; } else {r2.left=next->x ; r2.right=current->x; }
		if (current->y < next->y) {r2.bottom=current->y; r2.top=next->y; } else {r2.bottom=next->y ; r2.top=current->y; }
		if (localbox.top < r2.top) localbox.top = max(0L,min(head.biHeight-1,r2.top+1));
		if (localbox.left > r2.left) localbox.left = max(0L,min(head.biWidth-1,r2.left-1));
		if (localbox.right < r2.right) localbox.right = max(0L,min(head.biWidth-1,r2.right+1));
		if (localbox.bottom > r2.bottom) localbox.bottom = max(0L,min(head.biHeight-1,r2.bottom-1));

		i++;
	}
	
	//fill the outer region
	long npix=(localbox.right - localbox.left)*(localbox.top - localbox.bottom);
	POINT* pix = (POINT*)calloc(npix,sizeof(POINT));
	BYTE back=0, mark=1;
	long fx, fy, first, last,x,y,xmin,xmax,ymin,ymax;

	for (int side=0; side<4; side++){
		switch(side){
		case 0:
			xmin=localbox.left; xmax=localbox.right; ymin=localbox.bottom; ymax=localbox.bottom+1;
			break;
		case 1:
			xmin=localbox.right; xmax=localbox.right+1; ymin=localbox.bottom; ymax=localbox.top;
			break;
		case 2:
			xmin=localbox.left; xmax=localbox.right; ymin=localbox.top; ymax=localbox.top+1;
			break;
		case 3:
			xmin=localbox.left; xmax=localbox.left+1; ymin=localbox.bottom; ymax=localbox.top;
			break;
		}
		//fill from the border points
		for(y=ymin;y<ymax;y++){
			for(x=xmin;x<xmax;x++){
				if (plocal[x+y*head.biWidth]==0){
					// Subject: FLOOD FILL ROUTINE              Date: 12-23-97 (00:57)       
					// Author:  Petter Holmberg                 Code: QB, QBasic, PDS        
					// Origin:  petter.holmberg@usa.net         Packet: GRAPHICS.ABC
					first=0;
					last=1;
					while(first!=last){
						fx = pix[first].x;
						fy = pix[first].y;
						do {
							if ((plocal[x + fx + (y + fy)*head.biWidth] == back) &&
								(x + fx)>=localbox.left && (x + fx)<localbox.right && (y + fy)>=localbox.bottom && (y + fy)<localbox.top )
							{
								plocal[x + fx + (y + fy)*head.biWidth] = mark;
								if (plocal[x + fx + (y + fy - 1)*head.biWidth] == back){
									pix[last].x = fx;
									pix[last].y = fy - 1;
									last++;
									if (last == npix) last = 0;
								}
								if (plocal[x + fx + (y + fy + 1)*head.biWidth] == back){
									pix[last].x = fx;
									pix[last].y = fy + 1;
									last++;
									if (last == npix) last = 0;
								}
							} else {
								break;
							}
							fx++;
						} while(1);

						fx = pix[first].x - 1;
						fy = pix[first].y;

						do {
							if ((plocal[x + fx + (y + fy)*head.biWidth] == back) &&
								(x + fx)>=localbox.left && (x + fx)<localbox.right && (y + fy)>=localbox.bottom && (y + fy)<localbox.top )
							{
								plocal[x + fx + (y + fy)*head.biWidth] = mark;
								if (plocal[x + fx + (y + fy - 1)*head.biWidth] == back){
									pix[last].x = fx;
									pix[last].y = fy - 1;
									last++;
									if (last == npix) last = 0;
								}
								if (plocal[x + fx + (y + fy + 1)*head.biWidth] == back){
									pix[last].x = fx;
									pix[last].y = fy + 1;
									last++;
									if (last == npix) last = 0;
								}
							} else {
								break;
							}
							fx--;
						} while(1);
						
						first++;
						if (first == npix) first = 0;
					}
				}
			}
		}
	}

	//transfer the region
	long yoffset;
	for (y=localbox.bottom; y<localbox.top; y++){
		yoffset = y * head.biWidth;
		for (x=localbox.left; x<localbox.right; x++)
			if (plocal[x + yoffset]!=mark) pSelection[x + yoffset]=255;
	}
	if (info.rSelectionBox.top < localbox.left) info.rSelectionBox.top = localbox.top;
	if (info.rSelectionBox.left > localbox.left) info.rSelectionBox.left = localbox.left;
	if (info.rSelectionBox.right < localbox.left) info.rSelectionBox.right = localbox.right;
	if (info.rSelectionBox.bottom > localbox.bottom) info.rSelectionBox.bottom = localbox.bottom;

	free(plocal);
	free(pix);

	return true;
}
////////////////////////////////////////////////////////////////////////////////
bool CxImage::SelectionAddColor(RGBQUAD c)
{
    if (pSelection==NULL) SelectionCreate();

    for (long y = 0; y < head.biHeight; y++){
        for (long x = 0; x < head.biWidth; x++){
            RGBQUAD color = GetPixelColor(x, y);
            if (color.rgbRed   == c.rgbBlue  ||
				color.rgbGreen == c.rgbGreen ||
                color.rgbBlue  == c.rgbBlue)
            {
                pSelection[x + y * head.biWidth] = 255; // set the correct mask bit
            }
        }
    }

	return true;
}
////////////////////////////////////////////////////////////////////////////////
#if CXIMAGE_SUPPORT_WINDOWS
bool CxImage::SelectionToHRGN(HRGN& region)
{
	if (pSelection && region){           
        for(int y = 0; y < head.biHeight; y++){
            HRGN hTemp = NULL;
            int iStart = -1;
            int x = 0;
			for(; x < head.biWidth; x++){
                if (pSelection[x + y * head.biWidth] == 255){
					if (iStart == -1) iStart = x;
					continue;
                }else{
                    if (iStart >= 0){
                        hTemp = CreateRectRgn(iStart, y, x, y + 1);
                        CombineRgn(region, hTemp, region, RGN_OR);
                        DeleteObject(hTemp);
                        iStart = -1;
                    }
                }
            }
            if (iStart >= 0){
                hTemp = CreateRectRgn(iStart, y, x, y + 1);
                CombineRgn(region, hTemp, region, RGN_OR);
                DeleteObject(hTemp);
                iStart = -1;
            }
        }
		return true;
    }
	return false;
}
#endif //CXIMAGE_SUPPORT_WINDOWS
////////////////////////////////////////////////////////////////////////////////
#endif //CXIMAGE_SUPPORT_SELECTION
