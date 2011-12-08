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

// DropTarget.cpp: implementation of the CDropTarget class.
// (parts from an article at codeproject of a kind unknown author)

#include "../BB.h"
#include "../Pidl.h"
#include <ole2.h>
#include <shlobj.h>
#include <shlwapi.h>

//static int objcount;

//////////////////////////////////////////////////////////////////////
// DropTarget.h: interface for the CDropTarget class.

class CDropTarget : public IDropTarget
{
public:
	CDropTarget(HWND hwnd, LPCITEMIDLIST pidl)
	{
		m_dwRef = 1;
		m_pDropTarget = NULL;
		m_hwnd = hwnd;
		m_pidl = duplicateIDlist(pidl);
		m_pDataObject = NULL;
		//dbg_printf("CDropTarget created %d", ++objcount);
	};

	virtual ~CDropTarget()
	{
		//dbg_printf("CDropTarget deleted %d", --objcount);
		if (m_pidl) m_free(m_pidl);
	};

	// IUnknown methods
	STDMETHOD(QueryInterface)(REFIID iid, void** ppvObject)
	{
		*ppvObject = NULL;
		if( IsEqualIID(iid, IID_IUnknown) || IsEqualIID(iid, IID_IDropTarget))
		{
			*ppvObject = this; // direct inheritance
			AddRef(); // implicitly assumed for each successful query
			return S_OK;
		}
		return E_NOINTERFACE;
	}

	STDMETHOD_(ULONG, AddRef)()
	{
		return ++m_dwRef;
	}

	STDMETHOD_(ULONG, Release)()
	{ 
		int tempCount = --m_dwRef;
		if(tempCount==0)
		{
			delete this;
		}
		return tempCount; 
	}

private:
	// drop target methods
	STDMETHOD(DragEnter)(LPDATAOBJECT pDataObject, DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect);
	STDMETHOD(DragOver)(DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect);
	STDMETHOD(DragLeave)();
	STDMETHOD(Drop)(LPDATAOBJECT pDataObject, DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect);

	DWORD m_dwRef; // "COM" object reference counter
	HWND m_hwnd;
	LPITEMIDLIST m_pidl;
	LPDROPTARGET m_pDropTarget; // the actual shell folder that will receive dropped items
	LPDATAOBJECT m_pDataObject;

	friend class CDropTarget *init_drop_targ(HWND hwnd, LPCITEMIDLIST pidl);
	friend void exit_drop_targ (class CDropTarget *dt);
	friend bool in_drop(class CDropTarget *dt);
};

//////////////////////////////////////////////////////////////////////
// IDropTarget implementation

// This implementation is tightly linked with the sample dialog. A small child window with a
// target on it is the drop "target", but this in turn relegates the drop to whatever shell
// object is specified in the "Target path" box in the dialog. When our DragEnter() is called,
// we try to obtain the IDropTarget of the shell item, and from that point on we redirect all
// our methods to it. So when the OLE subsystem inquires whether we can accept the data, it
// is the target shell item that decides what should be done, NOT our minimal implementation.

// first called when the mouse enters our target window space
STDMETHODIMP CDropTarget::DragEnter(LPDATAOBJECT pDataObject, DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect)
{
	// don't set "*pdwEffect = DROPEFFECT_NONE"; that would adversely affect the drop target
	BOOL ok = FALSE;
	AddRef();
	if (m_pidl)
	{
		LPSHELLFOLDER psfFolder;
		LPITEMIDLIST pidlItem;
		LPITEMIDLIST pidlFull;
		if (sh_get_uiobject(m_pidl, &pidlFull, &pidlItem, &psfFolder, IID_IDropTarget, (void**)&m_pDropTarget))
		{
			HRESULT hr = m_pDropTarget->DragEnter(pDataObject, grfKeyState, pt, pdwEffect);
			if(SUCCEEDED(hr)) ok = TRUE;
		}
		if (psfFolder)      psfFolder   ->Release();
		if (pidlItem)       m_free(pidlItem);
		if (pidlFull)       m_free(pidlFull);
	}
	if(FALSE == ok) *pdwEffect = DROPEFFECT_NONE; // we can't understand this thing
	m_pDataObject = pDataObject;
	return S_OK;
}

//-----------------------------------------------------------------------------
// from now on things get really simple

STDMETHODIMP CDropTarget::DragOver(DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect)
{
#if 0
	LPITEMIDLIST pidl = (LPITEMIDLIST)SendMessage(m_hwnd, BB_DRAGOVER, grfKeyState, (LPARAM)&pt);
	// -----------------------------------------------------------
	// if this is enabled, things are dropped to menu items
	// rather than to the menu's folder itself
	int s1 = pidl ? GetIDListSize(pidl) : 0;
	int s2 = m_pidl ? GetIDListSize(m_pidl) : 0;
	if (s1 != s2 || (s1 && memcmp(pidl, m_pidl, s1)))
	{
		AddRef();
		DragLeave();
		if (m_pidl) m_free(m_pidl);
		m_pidl = duplicateIDlist(pidl);
		DWORD dwEffect = *pdwEffect;
		DragEnter(m_pDataObject, grfKeyState, pt, &dwEffect);
		Release();
	}
	// -----------------------------------------------------------
#else
	SendMessage(m_hwnd, BB_DRAGOVER, grfKeyState, (LPARAM)&pt);
#endif

	// if we have a valid target object, relay the call
	if(m_pDropTarget)
	{
		// mouse pointer information is not really relevant to the shell item...
		return m_pDropTarget->DragOver(grfKeyState, pt, pdwEffect);
	}
	else
	{
		*pdwEffect = DROPEFFECT_NONE; // can't accept now
		return S_OK;
	}
}

//-----------------------------------------------------------------------------
// Either DragLeave or Drop will be called for finishing the operation, but NOT both
// DragLeave is for when the mouse leaves the target, or the drag is somehow cancelled

STDMETHODIMP CDropTarget::DragLeave()
{
	HRESULT hr = S_OK;
	if(m_pDropTarget)
	{
		// next call should release the shell item's grip on the transferred data object
		hr = m_pDropTarget->DragLeave();
		// and we are done with the drop target interface, too
		m_pDropTarget->Release();
		m_pDropTarget = NULL;
	}
	Release();
	return hr;
}

//-----------------------------------------------------------------------------
// exit point when something is actually dropped
STDMETHODIMP CDropTarget::Drop(LPDATAOBJECT pDataObject, DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect)
{
	HRESULT hr = S_OK;
	if(m_pDropTarget)
	{
		// this call will actually perform the file operation in pdwEffect
		hr = m_pDropTarget->Drop(pDataObject, grfKeyState, pt, pdwEffect);
		// all is said & done
		// perform cleanup similar to DragLeave() above
		m_pDropTarget->Release();
		m_pDropTarget = NULL;
	}
	Release();
	return hr;
}

//-----------------------------------------------------------------------------
class CDropTarget *init_drop_targ(HWND hwnd, LPCITEMIDLIST pidl)
{
	CDropTarget *dt = new CDropTarget(hwnd, pidl);
	RegisterDragDrop(hwnd, dt);
	return dt;
}

void exit_drop_targ (class CDropTarget *dt)
{
	RevokeDragDrop(dt->m_hwnd);
	dt->m_hwnd = NULL;
	dt->Release();
}

bool in_drop(class CDropTarget *dt)
{
	return dt && NULL != dt->m_pDropTarget;
}

//-----------------------------------------------------------------------------
