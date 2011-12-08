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

// Implementation of the drag source COM object

#include "../BB.h"
#include "../Pidl.h"
#include <shlobj.h>

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

	//IDataObject members
	STDMETHOD(GiveFeedback)(DWORD dwEffect);
	STDMETHOD(QueryContinueDrag)(BOOL fEscapePressed,DWORD grfKeyState);

private:
	long m_cRefCount;
};

// -----------------------------------------

CImpIDropSource::CImpIDropSource()
{
	m_cRefCount = 1;
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
	return ++m_cRefCount;
}

STDMETHODIMP_(ULONG) CImpIDropSource::Release(void)
{
	long tempCount;
	tempCount = --m_cRefCount;
	if(tempCount==0) delete this;
	return tempCount; 
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
void drag_pidl(const _ITEMIDLIST *pidl)
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
	if (pDataObject)    pDataObject ->Release();
	if (psfFolder)      psfFolder   ->Release();
	if (pidlItem)       m_free(pidlItem);
	if (pidlFull)       m_free(pidlFull);
}

//===========================================================================
