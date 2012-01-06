/*
 ============================================================================

  This file is part of bbIconBox source code.
  bbIconBox is a plugin for Blackbox for Windows

  Copyright © 2004-2009 grischka
  http://bb4win.sf.net/bblean

  bbIconBox is free software, released under the GNU General Public License
  (GPL version 2).

  http://www.fsf.org/licenses/gpl.html

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

 ============================================================================
*/

// DropTarget.cpp: implementation of the CDropTarget class.
// (most parts here are from an article at codeproject of a kind unknown author)

#include "bbIconBox.h"

//////////////////////////////////////////////////////////////////////
// DropTarget.h: interface for the CDropTarget class.

class CDropTarget : public IDropTarget
{
public:
    CDropTarget(HWND hwnd);
    virtual ~CDropTarget();

    // IUnknown methods
    STDMETHOD(QueryInterface)(REFIID iid, void** ppvObject);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();
    // IDropTarget methods
    STDMETHOD(DragEnter)(LPDATAOBJECT pDataObject, DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect);
    STDMETHOD(DragOver)(DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect);
    STDMETHOD(DragLeave)();
    STDMETHOD(Drop)(LPDATAOBJECT pDataObject, DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect);
public:
    HWND m_hwnd;
private:
    LPCITEMIDLIST m_pidl;
    LPDROPTARGET m_pIDTFolder;
    LPDATAOBJECT m_pDataObject;
    DWORD m_dwRef; // "COM" object reference counter
    HRESULT cleanup(bool leave);
};

//////////////////////////////////////////////////////////////////////
// IDropTarget implementation


CDropTarget::CDropTarget(HWND hwnd)
{
    m_dwRef = 1;
    m_hwnd = hwnd;
    m_pidl = NULL;
    m_pIDTFolder = NULL;
    //dbg_printf("CDropTarget created %d", ++obj_count);
};

CDropTarget::~CDropTarget()
{
};

// IUnknown methods
STDMETHODIMP CDropTarget::QueryInterface(REFIID iid, void** ppvObject)
{
    *ppvObject = NULL;
    if( IsEqualIID(iid, IID_IUnknown) || IsEqualIID(iid, IID_IDropTarget))
    {
        *ppvObject = this;
        AddRef();
        return S_OK;
    }
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CDropTarget::AddRef()
{
    return ++m_dwRef;
}

STDMETHODIMP_(ULONG) CDropTarget::Release()
{ 
    int tempCount = --m_dwRef;
    if(tempCount==0)
    {
        //dbg_printf("CDropTarget deleted %d", --obj_count);
        delete this;
    }
    return tempCount; 
}

LPDROPTARGET get_drop_target(LPCITEMIDLIST pidl)
{
    LPDROPTARGET m_pDropTarget = NULL;
    if (pidl)
    {
        bool is_desk = false;
        if (0 == pidl->mkid.cb)
        {
            SHGetSpecialFolderLocation(NULL, CSIDL_DESKTOPDIRECTORY, (LPITEMIDLIST*)&pidl);
            is_desk = true;
        }

        LPSHELLFOLDER psfFolder;
        LPITEMIDLIST pidlItem;
        LPITEMIDLIST pidlFull;
        sh_get_uiobject(pidl, &pidlFull, &pidlItem, &psfFolder, IID_IDropTarget, (void**)&m_pDropTarget);
        freeIDList(pidlItem);
        freeIDList(pidlFull);
        if (psfFolder) psfFolder->Release();
        if (is_desk) SHMalloc_Free((LPITEMIDLIST)pidl);
    }
    return m_pDropTarget;
}

// first called when the mouse enters our target window space
STDMETHODIMP CDropTarget::DragEnter(LPDATAOBJECT pDataObject, DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect)
{
    AddRef();
    m_pDataObject = pDataObject;
    m_pidl = NULL;
    *pdwEffect = DROPEFFECT_NONE; // we check out that later
    return S_OK;
}

//-----------------------------------------------------------------------------
// from now on things get really simple

STDMETHODIMP CDropTarget::DragOver(DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect)
{
    // don't set "*pdwEffect = DROPEFFECT_NONE"; that would adversely affect the drop target
    LPCITEMIDLIST pidl = indrag_get_pidl(m_hwnd, (POINT *)&pt);
    if (pidl != m_pidl)
    {
        cleanup(true);
        m_pidl = pidl;
        m_pIDTFolder = get_drop_target(m_pidl);
        if (m_pIDTFolder)
        {
            HRESULT hr = m_pIDTFolder->DragEnter(m_pDataObject, grfKeyState, pt, pdwEffect);
            if(!SUCCEEDED(hr))
                cleanup(false);
        }
    }

    if (m_pIDTFolder)
    {
        HRESULT hr = m_pIDTFolder->DragOver(grfKeyState, pt, pdwEffect);
        if(SUCCEEDED(hr)) 
              return hr;
    }

    *pdwEffect = DROPEFFECT_NONE; // can't accept now
    return S_OK;
}

HRESULT CDropTarget::cleanup(bool leave)
{
    HRESULT hr = S_OK;
    if (m_pIDTFolder)
    {
        if (leave) hr = m_pIDTFolder->DragLeave();
        m_pIDTFolder->Release();
        m_pIDTFolder = NULL;
    }
    return hr;
}

//-----------------------------------------------------------------------------
// Either DragLeave or Drop will be called for finishing the operation, but NOT both
// DragLeave is for when the mouse leaves the target, or the drag is somehow cancelled

STDMETHODIMP CDropTarget::DragLeave()
{
    HRESULT hr = cleanup(true);
    Release();
    return hr;
}

//-----------------------------------------------------------------------------
// exit point when something is actually dropped
STDMETHODIMP CDropTarget::Drop(LPDATAOBJECT pDataObject, DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect)
{
    HRESULT hr = S_OK;
    if (m_pIDTFolder) {
        hr = m_pIDTFolder->Drop(pDataObject, grfKeyState, pt, pdwEffect);
    }
        cleanup(false);
    Release();
    return hr;
}

//-----------------------------------------------------------------------------
class CDropTarget *init_drop_targ(HWND hwnd)
{
    CDropTarget *m_dropTarget = new CDropTarget(hwnd);
    RegisterDragDrop(hwnd, m_dropTarget);
    return m_dropTarget;
}

void exit_drop_targ (class CDropTarget *m_dropTarget)
{
    RevokeDragDrop(m_dropTarget->m_hwnd);
    m_dropTarget->m_hwnd = NULL;
    m_dropTarget->Release();
}

//-----------------------------------------------------------------------------
