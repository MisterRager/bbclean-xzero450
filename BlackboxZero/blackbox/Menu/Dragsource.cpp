/* ==========================================================================

  This file is part of the bbLean source code
  Copyright © 2001-2003 The Blackbox for Windows Development Team
  Copyright © 2004-2009 grischka

  http://bb4win.sourceforge.net/bblean
  http://developer.berlios.de/projects/bblean

  bbLean is free software, released under the GNU General Public License
  (GPL version 2). For details see:

  http://www.fsf.org/licenses/gpl.html

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
  for more details.

  ========================================================================== */

// Implementation of the drag source COM object

#include "../BB.h"
#include "bbshell.h"

//===========================================================================
class CImpIDropSource: public IDropSource
{
public:
    CImpIDropSource();
    virtual ~CImpIDropSource();
    //IUnknown members
    STDMETHOD(QueryInterface)(REFIID,  void **);
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release)(void);
private:
    //IDataObject members
    STDMETHOD(GiveFeedback)(DWORD dwEffect);
    STDMETHOD(QueryContinueDrag)(BOOL fEscapePressed,DWORD grfKeyState);
    long m_dwRef;
};

// -----------------------------------------

CImpIDropSource::CImpIDropSource()
{
    m_dwRef = 1;
    //dbg_printf("CImpIDropSource Created");
}

CImpIDropSource::~CImpIDropSource()
{
    //dbg_printf("CImpIDropSource Deleted");
}

STDMETHODIMP CImpIDropSource::QueryInterface(REFIID iid, void ** ppv)
{
    if(IsEqualIID(iid, IID_IUnknown) || IsEqualIID(iid, IID_IDropSource))
    {
        *ppv=this;
        AddRef();
        return S_OK;
    }
    *ppv = NULL;
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CImpIDropSource::AddRef(void)
{
    return ++m_dwRef;
}

STDMETHODIMP_(ULONG) CImpIDropSource::Release(void)
{ 
    int r;
    if (0 == (r = --m_dwRef))
        delete this;
    return r; 
}

STDMETHODIMP CImpIDropSource::GiveFeedback(DWORD dwEffect)
{
    return DRAGDROP_S_USEDEFAULTCURSORS;
}

STDMETHODIMP CImpIDropSource::QueryContinueDrag(BOOL fEscapePressed, DWORD grfKeyState)
{
    if (fEscapePressed)
        return DRAGDROP_S_CANCEL;

    if (0 == (grfKeyState & (MK_LBUTTON|MK_RBUTTON)))
    {
        SetCursor(LoadCursor(NULL, IDC_ARROW));
        return DRAGDROP_S_DROP;
    }
    return S_OK;
}

//===========================================================================
void drag_pidl(LPCITEMIDLIST pidl)
{
    LPSHELLFOLDER psfFolder;
    LPITEMIDLIST pidlItem;
    LPITEMIDLIST pidlFull;
    LPDATAOBJECT pDataObject;
    if (sh_get_uiobject(pidl, &pidlFull, &pidlItem, &psfFolder, IID_IDataObject, (void**)&pDataObject))
    {
        LPDROPSOURCE pDropSource = new CImpIDropSource;
        DWORD dwEffect = 0;
        DoDragDrop(pDataObject, pDropSource, DROPEFFECT_COPY | DROPEFFECT_MOVE | DROPEFFECT_LINK ,&dwEffect);
        pDropSource ->Release();
    }
    if (pDataObject) pDataObject->Release();
    if (psfFolder)  psfFolder->Release();
    freeIDList(pidlItem);
    freeIDList(pidlFull);
}

//===========================================================================
