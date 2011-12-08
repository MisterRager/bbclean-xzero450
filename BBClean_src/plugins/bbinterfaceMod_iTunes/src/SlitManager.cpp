/*===================================================

	SLIT MANAGER CODE - Copyright 2004 grischka

	- grischka@users.sourceforge.net -

===================================================*/

// Global Include
#include "BBApi.h"

#include "ControlType_Label.h"
#include "SlitManager.h"
#include "SnapWindow.h"
#include "StyleMaster.h"

//=============================================================================

void dbg_window (HWND window, char *msg)
{
	char buffer[256];
	GetClassName(window, buffer, 256);
	dbg_printf("%s %s", msg, buffer);
}

#define dolist(_e,_l) for (_e=_l;_e;_e=_e->next)

//=============================================================================

void set_plugin_position(PluginInfo *p)
{
	 SetWindowPos(p->hwnd, NULL,
		 p->xpos, p->ypos, p->width, p->height,
		 SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSENDCHANGING);
}

//=============================================================================
PluginInfo *get_plugin(PluginInfo *PI, char *module_name)
{
	PluginInfo *p;
	int l = strlen(module_name);
	dolist (p, PI)
		if (0 == memicmp(module_name, p->module_name, l)
			&& (0 == p->module_name[l] || '.' == p->module_name[l]))
			break;
	return p;
}

//=============================================================================
bool plugin_setpos(PluginInfo *PI, char *module_name, int x, int y)
{
	PluginInfo *p = get_plugin(PI, module_name);
	if (NULL == p) return false;
	p->xpos = x;
	p->ypos = y;
	set_plugin_position(p);
	return true;
}

//=============================================================================
// well, for instance BBIcons has more than one window,
// so we hide/show all of them

bool plugin_getset_show_state(PluginInfo *PI, char *module_name, int state)
{
	PluginInfo *p = PI;
	bool result = false;
	bool show = state;
	for (;;)
	{
		p = get_plugin(p, module_name);
		if (NULL == p) return 3 == state;

		if (false == result && state >= 2)
			show = !(WS_VISIBLE & GetWindowLong(p->hwnd, GWL_STYLE));

		if (3 == state) return !show;

		ShowWindow(p->hwnd, show ? SW_SHOWNOACTIVATE : SW_HIDE);
		result = true;
		p = p->next;
	}
}

//=============================================================================

void get_sizes(PluginInfo **pp, HWND check)
{
	PluginInfo *p;
	while (NULL != (p = *pp))
	{
		HWND hwnd = p->hwnd;
		if (NULL == check || hwnd == check)
		{
			if (FALSE == IsWindow(hwnd))
			{
				*pp = p->next;
				delete p;
				continue;
			}
			RECT r; GetWindowRect(hwnd, &r);
			p->width  = r.right - r.left;
			p->height = r.bottom - r.top;
		}
		pp = &p->next;
	}
}

//=============================================================================

LRESULT CALLBACK subclass_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	bool is_locked_frame(control *c);
	extern bool plugin_snapwindows;

	HWND hSlit = GetParent(hwnd);
	control *c = (control *)GetWindowLong(hSlit, 0);
	controltype_label_details *details = (controltype_label_details *) c->controldetails;
	PluginInfo *PI = details->plugin_info;

	PluginInfo *p;
	dolist (p, PI)
		if (p->hwnd == hwnd) goto found;

	return DefWindowProc(hwnd, uMsg, wParam, lParam);

found:
	switch (uMsg)
	{
		case WM_ENTERSIZEMOVE:
			p->is_moving = true;
			break;

		case WM_EXITSIZEMOVE:
			p->is_moving = false;
			break;

		case WM_WINDOWPOSCHANGING:
			if (p->is_moving && false == is_locked_frame(c))
			{
				if (plugin_snapwindows && false == (GetAsyncKeyState(VK_SHIFT) & 0x8000))
					snap_windows((WINDOWPOS*)lParam, false, NULL);
			}
			else
			{
				WINDOWPOS* wp = (WINDOWPOS*)lParam;
				wp->x = p->xpos, wp->y = p->ypos;
			}
			return 0;

		case WM_WINDOWPOSCHANGED:
			{
				WINDOWPOS* wp = (WINDOWPOS*)lParam;
				if (0 == (wp->flags & SWP_NOMOVE))
				{
					p->xpos = wp->x;
					p->ypos = wp->y;
				}
				if (0 == (wp->flags & SWP_NOSIZE))
				{
					p->width = wp->cx;
					p->height = wp->cy;
				}
				style_check_transparency_workaround(hwnd);
			}
			break;

		case WM_NCLBUTTONDOWN:
			if (0x8000 & GetAsyncKeyState(VK_CONTROL))
			{
				SetWindowPos(hwnd, HWND_TOP, 0,0,0,0, SWP_NOSIZE|SWP_NOMOVE|SWP_NOSENDCHANGING);
				UpdateWindow(hwnd);
				return DefWindowProc(hwnd, uMsg, wParam, lParam);
			}
			break;

		case WM_NCHITTEST:
			if (0x8000 & GetAsyncKeyState(VK_SHIFT))
				return HTTRANSPARENT;

			if (0x8000 & GetAsyncKeyState(VK_CONTROL))
				return HTCAPTION;
			break;

		case WM_NCDESTROY:
			SendMessage(hSlit, SLIT_REMOVE, 0, (LPARAM)hwnd);
			break;
	}
	return CallWindowProc(p->wp, hwnd, uMsg, wParam, lParam);
}

//=============================================================================
int plugin_get_displayname(const char *src, char *buff)
{
	const char *s = src;
	const char *e = s + strlen(s);
	int len = 0;
	// start after the last slash
	while (e>s && e[-1]!= '\\' && e[-1]!= '/') e--, len++;
	strcpy(buff, e);

	// cut off ".dll", if present
	while (len) if (buff[--len]== '.') { buff[len] = 0; break; }
	return len;
}

//=============================================================================
void get_unique_modulename(PluginInfo *PI, HMODULE hMO, char *buffer)
{
	char temp[200]; *buffer = 0;
	char module_name[200];
	GetModuleFileName(hMO, module_name, sizeof module_name);
	int len = plugin_get_displayname(module_name, temp);

	PluginInfo *p; int n = 1;
	dolist (p, PI)
	{
		if (0 == memicmp(temp, p->module_name, len)
			&& (0 == p->module_name[len] || '.' == p->module_name[len]))
		{
			if (1 == n)
				sprintf(p->module_name, "%s.%d", temp, n);
			n++;
		}
	}
	if (1 == n)
		strcpy(buffer, temp);
	else
		sprintf(buffer, "%s.%d", temp, n);

	//dbg_printf("loaded: <%s> <%s>", temp, buffer);
}

//=============================================================================

int SlitWndProc(control *c, HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	controltype_label_details *details = (controltype_label_details *) c->controldetails;
	PluginInfo **pp = &details->plugin_info;

	switch(uMsg)
	{
		case SLIT_ADD:
			//dbg_window ((HWND) lParam, "add");
			{
				PluginInfo *p = new PluginInfo;
				ZeroMemory(p, sizeof *p);

				p->hwnd = (HWND)lParam;
				p->hMO = (HMODULE)GetClassLong(p->hwnd, GCL_HMODULE);

				if (wParam >= 0x400)
					strcpy(p->module_name, (const char *)wParam);
				else
					get_unique_modulename(*pp, p->hMO, p->module_name);

				//dbg_printf("module: %x %s",  p->hMO, p->module_name);

				while (*pp) pp = &(*pp)->next;
				*pp = p, p->next = NULL;

				SetParent(p->hwnd, hwnd);
				SetWindowLong(p->hwnd, GWL_STYLE, (GetWindowLong(p->hwnd, GWL_STYLE) & ~WS_POPUP) | WS_CHILD);
				p->wp = (WNDPROC)SetWindowLong(p->hwnd, GWL_WNDPROC, (LONG)subclass_WndProc);

				get_sizes(pp, (HWND)lParam);
				set_plugin_position(p);
				break;
			}
			break;

		case SLIT_REMOVE:
			//dbg_window ((HWND) lParam, "remove");
			while (*pp)
			{
				if ((*pp)->hwnd == (HWND)lParam)
				{
					PluginInfo *p = *pp;
					if (IsWindow(p->hwnd))
					{
						SetWindowLong(p->hwnd, GWL_WNDPROC, (LONG)p->wp);
						SetParent(p->hwnd, NULL);
						SetWindowLong(p->hwnd, GWL_STYLE, (GetWindowLong(p->hwnd, GWL_STYLE) & ~WS_CHILD) | WS_POPUP);
					}
					*pp = p->next;
					delete p;
					break;
				}
				pp = &(*pp)->next;
			}
			break;

		case SLIT_UPDATE:
			//dbg_window ((HWND) lParam, "update");
			//though, some plugins dont pass their hwnd with SLIT_UPDATE
			get_sizes(pp, (HWND)lParam);
			break;

		//=============================================
		default:
			break;
	}
	return 0;
}

//============================================================================
// load/unloadPlugins

ModuleInfo * m_loadPlugin(HWND hSlit, const char *file_name)
{
	const char *errormsg = NULL;

	HMODULE hMO = LoadLibrary(file_name);
	if (NULL == hMO)
	{
		errormsg = "The plugin you are trying to load does not exist or cannot be loaded.";
	}
	else
	{
		try
		{
			int result = 0;

			int (*beginPlugin)(HINSTANCE hMainInstance);
			int (*beginSlitPlugin)(HINSTANCE hMainInstance, HWND hBBSlit);
			int (*beginPluginEx)(HINSTANCE hMainInstance, HWND hBBSlit);
			int (*endPlugin)(HINSTANCE hMainInstance);

			*(FARPROC*)&beginPlugin     = GetProcAddress(hMO, "beginPlugin");
			*(FARPROC*)&beginSlitPlugin = GetProcAddress(hMO, "beginSlitPlugin");
			*(FARPROC*)&beginPluginEx   = GetProcAddress(hMO, "beginPluginEx");
			*(FARPROC*)&endPlugin       = GetProcAddress(hMO, "endPlugin");

			if (NULL == endPlugin)
				errormsg = "This plugin doesn't have a 'endPlugin'."
					"\nProbably it is not a plugin designed for bb4win.";
			else
			if (beginSlitPlugin)
				result = beginSlitPlugin(hMO, hSlit);
			else
			if (beginPluginEx)
				result = beginPluginEx(hMO, hSlit);
			else
			if (beginPlugin)
				result = beginPlugin(hMO);
			else
				errormsg = "This plugin doesn't have an 'beginPlugin'.";

			if (NULL == errormsg)
			{
				if (0 == result)
				{
					ModuleInfo *m = new ModuleInfo;
					m->hMO = hMO;
					m->endPlugin = endPlugin;
					*(FARPROC*)&m->pluginInfo = GetProcAddress(hMO, "pluginInfo");
					strcpy(m->file_name, file_name);
					plugin_get_displayname(file_name, m->module_name);
					return m;
				}
				errormsg = "This plugin signaled an error on loading.";
			}
		}
		catch(...)
		{
			errormsg = "This plugin crashed on loading.";
		}

		FreeLibrary(hMO);
	}

	char message[MAX_PATH + 512];
	sprintf (message, "%s\n%s", file_name, errormsg);
	MBoxErrorValue(message);
	return NULL;
}

//============================================================================
void m_unloadPlugin(ModuleInfo *m)
{
	m->endPlugin(m->hMO);
	FreeLibrary(m->hMO);
}

//============================================================================
const char * check_relative_path(const char *filename)
{
	// get relative path to blackbox process
	char bb_path[MAX_PATH];
	GetBlackboxPath(bb_path, MAX_PATH);
	int len = strlen(bb_path);

	if (0 == memicmp(bb_path, filename, len))
		return filename + len;
	return filename;
}

//============================================================================
ModuleInfo *loadPlugin(ModuleInfo **pm, HWND hSlit, const char *file_name)
{
	ModuleInfo *m = m_loadPlugin(hSlit, check_relative_path(file_name));
	if (m)
	{
		for (; *pm; pm = &(*pm)->next);
		*pm = m, m->next = NULL;
	}
	return m;
}

//============================================================================
bool unloadPlugin(ModuleInfo **pm, const char *module_name)
{
	ModuleInfo *m; bool result = false;
	for (; NULL != (m = *pm);)
	{
		if (NULL == module_name || 0 == stricmp(module_name, m->module_name))
		{
			m_unloadPlugin(m);
			*pm = m->next;
			delete m;
			result = true;
		}
		else
		{
			pm = &m->next;
		}
	}
	return result;
}

//=============================================================================

void aboutPlugins(ModuleInfo *m0, const char *ctrl)
{
	char buff[5000]; LPCSTR (*pluginInfo)(int);
	int x = 0; ModuleInfo *m;
	for (m = m0; m; m= m->next)
	{
		pluginInfo = m->pluginInfo;
		x += pluginInfo
			? sprintf(buff+x, "%s %s by %s (%s)\t\n",
				pluginInfo(PLUGIN_NAME),
				pluginInfo(PLUGIN_VERSION),
				pluginInfo(PLUGIN_AUTHOR),
				pluginInfo(PLUGIN_RELEASE)
				)
			: sprintf(buff+x, "%s\t\n", m->file_name)
			;
	}
	char caption[256];
	sprintf(caption, "%s - About Plugins", ctrl);
	MessageBox(NULL, buff, caption, MB_OK|MB_SYSTEMMODAL);
}

//=============================================================================
