//===========================================================================
// TinyDropTarg.cpp

#include <shlobj.h>
#include <shellapi.h>

class TinyDropTarget : public IDropTarget
{
private:
	DWORD m_dwRef;
	bool valid_data;

public:
	HWND m_hwnd;
	HWND task_over;

	TinyDropTarget(HWND hwnd) {
		m_dwRef = 1;
		m_hwnd = hwnd;
		task_over = 0;
		valid_data = false;
		//dbg_printf("created");
	};

	virtual ~TinyDropTarget() {
		//dbg_printf("deleted");
	};

	STDMETHOD(QueryInterface)(REFIID iid, void** ppvObject) {
		*ppvObject = NULL;
		if(IsEqualIID(iid, IID_IUnknown) || IsEqualIID(iid, IID_IDropTarget)) {
			*ppvObject = this;
			AddRef();
			return S_OK;
		}
		return E_NOINTERFACE;
	}

	STDMETHOD_(ULONG, AddRef)() {
		//dbg_printf("addref %d", 1+m_dwRef);
		return ++m_dwRef;
	}

	STDMETHOD_(ULONG, Release)()
	{ 
		int tempCount = --m_dwRef;
		//dbg_printf("decref %d", tempCount);
		if(tempCount==0)
		{
			delete this;
		}
		return tempCount; 
	}

	STDMETHOD (DragEnter) (LPDATAOBJECT pDataObject, DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect)
	{
		AddRef();
	/*
		FORMATETC fmte;
		fmte.cfFormat   = CF_HDROP;
		fmte.ptd        = NULL;
		fmte.dwAspect   = DVASPECT_CONTENT;  
		fmte.lindex     = -1;
		fmte.tymed      = TYMED_HGLOBAL;       
		valid_data      = S_OK == pDataObject->QueryGetData(&fmte);
	*/
		*pdwEffect  = DROPEFFECT_NONE;
		task_over = NULL;
		return S_OK;
	}

	STDMETHOD(DragOver)(DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect) {
		ScreenToClient(m_hwnd, (POINT*)&pt);
		HWND task = (HWND)SendMessage(m_hwnd, BB_DRAGOVER, 0, MAKELPARAM(pt.x, pt.y));
		if (task_over != task) {
			task_over = task;
			if (task) SetTimer(m_hwnd, TASK_RISE_TIMER, 500, 0);

		}
		*pdwEffect = DROPEFFECT_NONE;
		if (NULL == task && valid_data)
			*pdwEffect = DROPEFFECT_COPY;
		return S_OK;
	}

	STDMETHOD(DragLeave)() {
		task_over = NULL;
		Release();
		return S_OK;
	}

	STDMETHOD(Drop)(LPDATAOBJECT pDataObject, DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect) {
		*pdwEffect = DROPEFFECT_NONE;
/*
		if (valid_data)
		{
			*pdwEffect = DROPEFFECT_COPY;
			if (pDataObject)
			{
				FORMATETC fmte;
				STGMEDIUM medium;
				fmte.cfFormat   = CF_HDROP;
				fmte.ptd        = NULL;
				fmte.dwAspect   = DVASPECT_CONTENT;  
				fmte.lindex     = -1;
				fmte.tymed      = TYMED_HGLOBAL;
				if (SUCCEEDED(pDataObject->GetData(&fmte, &medium)))
				{
					HDROP hdrop = (HDROP)medium.hGlobal;

					char filename[MAX_PATH]; filename[0]=0;
					DragQueryFile(hdrop, 0, filename, sizeof(filename));
					ReleaseStgMedium(&medium);
					SendMessage(GetBBWnd(), BB_SETSTYLE, 1, (LPARAM)filename);
				}
			}
		}
*/
		Release();
		return S_OK;
	}

   void handle_task_timer(void) {
	   KillTimer(m_hwnd, TASK_RISE_TIMER);
	   if (NULL == task_over) return;

	   DWORD ThreadID1 = GetWindowThreadProcessId(GetForegroundWindow(), NULL);
	   DWORD ThreadID2 = GetCurrentThreadId();
	   if (ThreadID1 != ThreadID2) {
		   AttachThreadInput(ThreadID1, ThreadID2, TRUE);
		   SetForegroundWindow(m_hwnd);
		   AttachThreadInput(ThreadID1, ThreadID2, FALSE);
	   }
	   PostMessage(GetBBWnd(), BB_BRINGTOFRONT, 0, (LPARAM)task_over);
   }

};

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// the interface

// call after the bar window is created with it's hwnd
class TinyDropTarget *init_drop_targ(HWND hwnd)
{
	class TinyDropTarget *m_TinyDropTarget = new TinyDropTarget(hwnd);
	RegisterDragDrop(hwnd, m_TinyDropTarget);
	return m_TinyDropTarget;
}

// call before the bar window is destroyed
void exit_drop_targ (class TinyDropTarget *m_TinyDropTarget)
{
	RevokeDragDrop(m_TinyDropTarget->m_hwnd);
	m_TinyDropTarget->Release();
}

// call on WM_TIMER / TASK_RISE_TIMER
void handle_task_timer(class TinyDropTarget *m_TinyDropTarget)
{
	m_TinyDropTarget->handle_task_timer();
}

//-----------------------------------------------------------------------------
