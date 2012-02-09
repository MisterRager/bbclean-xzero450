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

#include "BB.h"
#include "bbrc.h"
#include "PluginManager.h"
#define ST static

struct plugins
{
    struct plugins *next;

    char *name;     // display name as in the menu, NULL for comments
    char *path;     // as in plugins.rc, entire line for comments

    bool enabled;   // plugin should be loaded
    bool useslit;   // plugin should be loaded into slit
    bool inslit;    // plugin is in the slit
    bool isslit;    // this is the bbSlit plugin

    HMODULE hmodule; // as returned by LoadLibrary
    int n_instance; // if the same plugin name is used more than once

    int (*beginPlugin)(HINSTANCE);
    int (*beginPluginEx)(HINSTANCE, HWND);
    int (*beginSlitPlugin)(HINSTANCE, HWND);
    int (*endPlugin)(HINSTANCE);
    const char* (*pluginInfo)(int);
};

// same order as function ptrs above please
static const char* const function_names[] = {
    "beginPlugin"       ,
    "beginPluginEx"     ,
    "beginSlitPlugin"   ,
    "endPlugin"         ,
    "pluginInfo"        ,
    NULL
};

// Privat variables
ST struct plugins *bbplugins; // list of plugins
ST HWND hSlit;  // BBSlit window
ST FILETIME rc_filetime; // plugins.rc filetime

// Forward decls
ST void apply_plugin_state(void);
ST int load_plugin(struct plugins *q, HWND hSlit);
ST int unload_plugin(struct plugins *q);
ST void free_plugin(struct plugins **q);
ST struct plugins *append_plugin(const char *rcline);
ST bool write_plugins(void);
ST void plugin_error(struct plugins *q, int error);

enum plugin_errors
{
    error_plugin_is_built_in       = 1,
    error_plugin_dll_not_found     ,
    error_plugin_dll_needs_module  ,
    error_plugin_does_not_load     ,
    error_plugin_missing_entry     ,
    error_plugin_fail_to_load      ,
    error_plugin_crash_on_load     ,
    error_plugin_crash_on_unload
};

//===========================================================================

void PluginManager_Init(void)
{
    const char *path;
    FILE *fp;
    char szBuffer[MAX_PATH];

    bbplugins = NULL;
    path = plugrcPath(NULL);
    get_filetime(path, &rc_filetime);

    // read plugins.rc
    fp = fopen(path,"rb");
    if (fp) {
        while (read_next_line(fp, szBuffer, sizeof szBuffer))
            append_plugin(szBuffer);
        fclose(fp);
    }

    apply_plugin_state();
}

void PluginManager_Exit(void)
{
    struct plugins *q;
    reverse_list(&bbplugins);
    dolist (q, bbplugins)
        q->enabled = false;
    apply_plugin_state();
    while (bbplugins)
        free_plugin(&bbplugins);
}

int PluginManager_RCChanged(void)
{
    return 0 != diff_filetime(plugrcPath(NULL), &rc_filetime);
}

//===========================================================================
// (API:) EnumPlugins

void EnumPlugins (PLUGINENUMPROC lpEnumFunc, LPARAM lParam)
{
    struct plugins *q;
    dolist(q, bbplugins)
        if (FALSE == lpEnumFunc(q, lParam))
            break;
}

//===========================================================================
// parse a line from plugins,rc and put it into the pluginlist struct

ST struct plugins *append_plugin(const char *rcline)
{
    struct plugins *q = c_new(struct plugins);

    if ('\0' == rcline[0] || '#' == rcline[0] || '[' == rcline[0]) {
        q->path = new_str(rcline);
    } else {
        char name[100];
        int n, l;
        struct plugins *q2;

        q->enabled = true;
        q->useslit = false;
        for (;;) {
            n = *rcline;
            if (n == '!')
                q->enabled = false;
            else if (n == '&')
                q->useslit = true;
            else
                break;
            while (' '== *++rcline);
        }

        strcpy(name, file_basename(rcline)); // copy name
        *(char*)file_extension(name) = 0; // strip ".dll"

        // lookup for duplicates
        n = 0, l = strlen(name);
        dolist (q2, bbplugins) {
            char *p = q2->name;
            if (p && 0 == memicmp(p, name, l) && (0 == p[l] || '.' == p[l]))
                n++;
        }

        if (0 == n) {
            name[l] = 0;
        } else {
            sprintf(name+l, ".%d", 1+n);
            q->isslit = false;
        }

        // accept BBSlit and BBSlit?
        q->isslit = 0 == memicmp(name, "BBSlit", 6)
            && (name[6] == 0 || name[6] == '.');

        q->n_instance = n;
        q->name = new_str(name);
        q->path = new_str(rcline);
    }

    append_node(&bbplugins, q);
    //dbg_printf("%d %d <%s> <%s>", q->enabled, q->inslit, q->path, q->name);
    return q;
}

ST void free_plugin(struct plugins **pp)
{
    struct plugins *q = *pp;
    if (q) {
        free_str(&q->name);
        free_str(&q->path);
        *pp = q->next;
        m_free(q);
    }
}

//===========================================================================
// run through plugin list and load/unload changed plugins

ST void check_plugin(struct plugins *q)
{
    int error = 0;
    if (q->hmodule) {
        if (q->enabled && (q->useslit && hSlit) == q->inslit)
            return;
        error = unload_plugin(q);
    }

    if (q->enabled) {
        if (0 == error)
            error = load_plugin(q, hSlit);
        if (NULL == q->hmodule)
            q->enabled = false;
        if (error)
            write_plugins();
    }

    plugin_error(q, error);
}

ST void apply_plugin_state(void)
{
    struct plugins *q;

    hSlit = NULL;
    // load slit first
    dolist(q, bbplugins)
        if (q->isslit && q->enabled) {
            check_plugin(q);
            hSlit = FindWindow("BBSlit", NULL);
        }

    // load/unload other plugins
    dolist(q, bbplugins)
        if (q->name && false == q->isslit)
            check_plugin(q);

    // unload slit last
    dolist(q, bbplugins)
        if (q->isslit && false == q->enabled)
            check_plugin(q);
}

//===========================================================================
// plugin error message

ST void plugin_error(struct plugins *q, int error)
{
    const char *msg;
    switch (error)
    {
        case 0:
            return;
        case error_plugin_is_built_in      :
            msg = NLS2("$Error_Plugin_IsBuiltIn$",
            "Dont load this plugin with "BBAPPNAME". It is built-in."
            ); break;
        case error_plugin_dll_not_found    :
            msg = NLS2("$Error_Plugin_NotFound$",
            "The plugin was not found."
            ); break;
        case error_plugin_dll_needs_module :
            msg = NLS2("$Error_Plugin_MissingModule$",
            "The plugin cannot be loaded. Possible reason:"
            "\n- The plugin requires another dll that is not there."
            ); break;
        case error_plugin_does_not_load    :
            msg = NLS2("$Error_Plugin_DoesNotLoad$",
            "The plugin cannot be loaded. Possible reasons:"
            "\n- The plugin requires another dll that is not there."
            "\n- The plugin is incompatible with the windows version."
            "\n- The plugin is incompatible with this blackbox version."
            ); break;
        case error_plugin_missing_entry    :
            msg = NLS2("$Error_Plugin_MissingEntry$",
            "The plugin misses the begin- and/or endPlugin entry point. Possible reasons:"
            "\n- The dll is not a plugin for Blackbox for Windows."
            ); break;
        case error_plugin_fail_to_load     :
            msg = NLS2("$Error_Plugin_IniFailed$",
            "The plugin could not be initialized."
            ); break;
        case error_plugin_crash_on_load    :
            msg = NLS2("$Error_Plugin_CrashedOnLoad$",
            "The plugin caused a general protection fault on initializing."
            "\nPlease contact the plugin author."
            ); break;
        case error_plugin_crash_on_unload  :
            msg = NLS2("$Error_Plugin_CrashedOnUnload$",
            "The plugin caused a general protection fault on shutdown."
            "\nPlease contact the plugin author."
            ); break;
        default:
            msg = "(Unknown Error)";
    }
    BBMessageBox(
        MB_OK,
        NLS2("$Error_Plugin$", "Error: %s\n%s"),
        q->path,
        msg
        );
}

//===========================================================================
// load/unload one plugin

ST int unload_plugin(struct plugins *q)
{
    int error = 0;
    if (q->hmodule)
    {
        TRY
        {
            q->endPlugin(q->hmodule);
        }
        EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            error = error_plugin_crash_on_unload;
        }

        FreeLibrary(q->hmodule);
        q->hmodule = NULL;
    }

    return error;
}

ST int load_plugin(struct plugins *q, HWND hSlit)
{
    HINSTANCE hModule = NULL;
    int error = 0;
    bool useslit;
    int r, i;
    char plugin_path[MAX_PATH];

    for (;;)
    {
        //---------------------------------------
        // check for compatibility

#ifndef BBTINY
        if (0 == stricmp(q->name, "BBDDE"))
        {
            error = error_plugin_is_built_in;
            break;
        }
#endif

        //---------------------------------------
        // load the dll

        if (0 == FindRCFile(plugin_path, q->path, NULL)) {
            error = error_plugin_dll_not_found;
            break;
        }

        r = SetErrorMode(0); // enable 'missing xxx.dll' system message
        hModule = LoadLibrary(plugin_path);
        SetErrorMode(r);

        if (NULL == hModule)
        {
            r = GetLastError();
            // char buff[200]; win_error(buff, sizeof buff);
            // dbg_printf("LoadLibrary::GetLastError %d: %s", r, buff);
            if (ERROR_MOD_NOT_FOUND == r)
                error = error_plugin_dll_needs_module;
            else
                error = error_plugin_does_not_load;
            break;
        }

        //---------------------------------------
        // grab interface functions

        for (i = 0; function_names[i]; ++i)
            ((FARPROC*)&q->beginPlugin)[i] =
                GetProcAddress(hModule, function_names[i]);

        //---------------------------------------
        // check interface presence

        if (NULL == q->endPlugin) {
            error = error_plugin_missing_entry;
            break;
        }

        if (NULL == q->beginPluginEx && NULL == q->beginSlitPlugin)
            q->useslit = false;

        useslit = hSlit && q->useslit;

        if (false == useslit && NULL == q->beginPluginEx && NULL == q->beginPlugin) {
            error = error_plugin_missing_entry;
            break;
        }

        //---------------------------------------
        // inititalize plugin

        TRY
        {
            if (useslit) {
                if (q->beginPluginEx)
                    r = q->beginPluginEx(hModule, hSlit);
                else
                    r = q->beginSlitPlugin(hModule, hSlit);
            } else {
                if (q->beginPlugin)
                    r = q->beginPlugin(hModule);
                else
                    r = q->beginPluginEx(hModule, NULL);
            }

            if (BEGINPLUGIN_OK == r) {
                q->hmodule = hModule;
                q->inslit = useslit;
            } else if (BEGINPLUGIN_FAILED_QUIET != r) {
                error = error_plugin_fail_to_load;
            }

        }
        EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            error = error_plugin_crash_on_load;
        }
        break;
    }

    // clean up after error
    if (NULL == q->hmodule && hModule)
        FreeLibrary(hModule);

    return error;
}

//===========================================================================
// write back the plugin list to "plugins.rc"

ST bool write_plugins(void)
{
    FILE *fp;
    struct plugins *q;
    const char *rcpath;

    rcpath = plugrcPath(NULL);
    if (NULL == (fp = create_rcfile(rcpath)))
        return false;

    dolist(q, bbplugins) {
        if (q->name) {
            if (false == q->enabled)
                fprintf(fp,"! ");
            if (false != q->useslit)
                fprintf(fp,"& ");
        }
        fprintf(fp,"%s\n", q->path);
    }

    fclose(fp);
    get_filetime(rcpath, &rc_filetime);
    return true;
}

//===========================================================================

void PluginManager_aboutPlugins(void)
{
    int l, x = 0;
    char *msg = (char*)c_alloc(l = 4096);
    const char* (*pluginInfo)(int);
    struct plugins *q;
    dolist(q, bbplugins)
        if (q->hmodule) {
            if (l - x < MAX_PATH + 100)
                msg = (char*)m_realloc(msg, l*=2);
            if (x)
                msg[x++] = '\n';
            pluginInfo = q->pluginInfo;
            if (pluginInfo)
                x += sprintf(msg + x,
                    "%s %s %s %s (%s)\t",
                    pluginInfo(PLUGIN_NAME),
                    pluginInfo(PLUGIN_VERSION),
                    NLS2("$About_Plugins_By$", "by"),
                    pluginInfo(PLUGIN_AUTHOR),
                    pluginInfo(PLUGIN_RELEASE)
                    );
            else
                x += sprintf(msg + x, "%s\t", q->path);
        }

    BBMessageBox(MB_OK,
        "#"BBAPPNAME" - %s#%s\t",
        NLS2("$About_Plugins_Title$", "About loaded plugins"),
        x ? msg : NLS1("No plugins loaded.")
        );

    m_free(msg);
}

//===========================================================================
// OpenFileName Dialog to add plugins

#ifndef BBTINY
#include <commdlg.h>
#ifndef OPENFILENAME_SIZE_VERSION_400
#define OPENFILENAME_SIZE_VERSION_400  CDSIZEOF_STRUCT(OPENFILENAME,lpTemplateName)
#endif // (_WIN32_WINNT >= 0x0500)
ST UINT_PTR APIENTRY OFNHookProc(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    if (WM_INITDIALOG == uiMsg)
    {
        // center 'open file' dialog on screen
        RECT r, w;
        if (WS_CHILD & GetWindowLong(hdlg, GWL_STYLE))
            hdlg = GetParent(hdlg);
        GetWindowRect(hdlg, &r);
        GetMonitorRect(hdlg, &w, GETMON_WORKAREA|GETMON_FROM_WINDOW);
        MoveWindow(hdlg,
            (w.right+w.left+r.left-r.right) / 2,
            (w.bottom+w.top+r.top-r.bottom) / 2,
            r.right - r.left,
            r.bottom - r.top,
            TRUE
            );
    }
    return 0;
}

ST bool browse_file(char *buffer, const char *title, const char *filters)
{
    OPENFILENAME ofCmdFile;
    static BOOL (WINAPI *pGetOpenFileName)(LPOPENFILENAME lpofn);
    bool ret = false;

    memset(&ofCmdFile, 0, sizeof(OPENFILENAME));
    ofCmdFile.lStructSize = sizeof(OPENFILENAME);
    ofCmdFile.lpstrFilter = filters;
    ofCmdFile.nFilterIndex = 1;
    //ofCmdFile.hwndOwner = NULL;
    ofCmdFile.lpstrFile = buffer;
    ofCmdFile.nMaxFile = MAX_PATH;
    //ofCmdFile.lpstrFileTitle = NULL;
    //ofCmdFile.nMaxFileTitle = 0;
    {
        static bool init;
        char plugin_dir[MAX_PATH];
        if (!init) {
            ofCmdFile.lpstrInitialDir = set_my_path(NULL, plugin_dir, "plugins");
            init = true;
        }
    }
    ofCmdFile.lpstrTitle = title;
    ofCmdFile.Flags = OFN_EXPLORER|OFN_FILEMUSTEXIST|OFN_ENABLEHOOK|OFN_NOCHANGEDIR;
    ofCmdFile.lpfnHook = OFNHookProc;
    if (LOBYTE(GetVersion()) < 5) // win9x/me
        ofCmdFile.lStructSize = OPENFILENAME_SIZE_VERSION_400;
    if ((DWORD)-1 == GetFileAttributes(ofCmdFile.lpstrFile))
        ofCmdFile.lpstrFile[0] = 0;

    if (load_imp(&pGetOpenFileName, "comdlg32.dll", "GetOpenFileNameA"))
        ret = FALSE != pGetOpenFileName(&ofCmdFile);
    return ret;
}

#endif //ndef BBTINY

//===========================================================================
// The plugin configuration menu

ST Menu *get_menu(const char *title, char *menu_id, bool pop, struct plugins **qp, bool b_slit)
{
    struct plugins *q;
    char *end_id;
    MenuItem *pItem;
    char command[20], label[80], broam[MAX_PATH+80];
    const char *cp;
    Menu *pMenu, *pSub;

    end_id = strchr(menu_id, 0);
    pMenu = MakeNamedMenu(title, menu_id, pop);

    while (NULL != (q = *qp)) {
        *qp = q->next;
        if (q->name) {
            if (0 == b_slit) {
                sprintf(broam, "@BBCfg.plugin.load %s", q->name);
                pItem = MakeMenuItem(pMenu, q->name, broam, q->enabled);
#if 0
                sprintf(end_id, "_opt_%s", q->name);
                pSub = MakeNamedMenu(q->name, menu_id, pop);
                MenuItemOption(pItem, BBMENUITEM_RMENU, pSub);

                MakeMenuItem(pSub, "Load", broam, q->enabled);
                sprintf(broam, "@BBCfg.plugin.inslit %s", q->name);
                MakeMenuItem(pSub, "In Slit", broam, q->useslit);
#endif
            } else if (q->enabled && (q->beginPluginEx || q->beginSlitPlugin)) {
                sprintf(broam, "@BBCfg.plugin.inslit %s", q->name);
                MakeMenuItem(pMenu, q->name, broam, q->useslit);
            }
        } else if (0 == b_slit) {
            cp = q->path;
            if (false == get_string_within(command, sizeof command, &cp, "[]"))
                continue;
            get_string_within(label, sizeof label, &cp, "()");
            if (0 == stricmp(command, "nop"))
                MakeMenuNOP(pMenu, label);
            else if (0 == stricmp(command, "sep"))
                MakeMenuNOP(pMenu, NULL);
            else if (0 == stricmp(command, "submenu") && *label) {
                sprintf(end_id, "_%s", label);
                pSub = get_menu(label, menu_id, pop, qp, b_slit);
                MakeSubmenu(pMenu, pSub, NULL);
            } else if (0 == stricmp(command, "end")) {
                break;
            }
        }
    }
    return pMenu;
}

Menu* PluginManager_GetMenu(const char *text, char *menu_id, bool pop, int mode)
{
    struct plugins *q = bbplugins;
    if (SUB_PLUGIN_SLIT == mode && NULL == hSlit)
        return NULL;
    return get_menu(text, menu_id, pop, &q, SUB_PLUGIN_SLIT == mode);
}

//===========================================================================
// parse a line from plugins.rc to obtain the pluginRC address

bool parse_pluginRC(const char *rcline, const char *name)
{
    bool is_comment = false;

    if ('#' == *rcline || 0 == *rcline)
        is_comment = true;
    else
    if ('!' == *rcline)
        while (' '== *++rcline);

    if ('&' == *rcline)
    {
        while (' '== *++rcline);
    }

	if (!is_comment && IsInString(rcline, name))
	{
		char editor[MAX_PATH];
		char road[MAX_PATH];
		char buffer[2*MAX_PATH];
		char *s = strcpy((char*)name, rcline);
		*(char*)file_extension(s) = 0; // strip ".dll"
		if (IsInString(s, "+"))
			*(char*)get_delim(s, '+') = 0; // strip "+"
		rcline = (const char*)strcat((char*)s, ".rc");
		GetBlackboxEditor(editor);
        sprintf(buffer, "\"%s\" \"%s\"", editor, set_my_path(hMainInstance, road, rcline));
        BBExecute_string(buffer, RUN_SHOWERRORS);
		return true;
	}
	return false;
}

//===========================================================================
// parse a line from plugins.rc to obtain the Documentation address

bool parse_pluginDocs(const char *rcline, const char *name)
{
    bool is_comment = false;

    if ('#' == *rcline || 0 == *rcline)
        is_comment = true;
    else
    if ('!' == *rcline)
        while (' '== *++rcline);

    if ('&' == *rcline)
    {
        while (' '== *++rcline);
    }

	if (!is_comment && IsInString(rcline, name))
	{
		char road[MAX_PATH];
		char *s = strcpy((char*)name, rcline);
		*(char*)file_extension(s) = 0; // strip ".dll"
		// files could be *.html, *.htm, *.txt, *.xml ...
		if (locate_file(hMainInstance, road, s, "html") || locate_file(hMainInstance, road, s, "htm") || locate_file(hMainInstance, road, s, "txt") || locate_file(hMainInstance, road, s, "xml"))
		{
			BBExecute(NULL, "open", road, NULL, NULL, SW_SHOWNORMAL, 		false);
			return true;
		}
		*(char*)get_delim(s, '\\') = 0; // strip plugin name
		rcline = (const char*)strcat((char*)s, "\\readme");
		// ... also the old standby: readme.txt
		if (locate_file(hMainInstance, road, rcline, "txt"))
		{
			BBExecute(NULL, "open", road, NULL, NULL, SW_SHOWNORMAL, 		false);
			return true;
		}
	}
	return false;
}

//===========================================================================
// handle the "@BBCfg.plugin.xxx" bro@ms from the config->plugins menu

int PluginManager_handleBroam(const char *args)
{
    static const char * const actions[] = {
        "add", "remove", "load", "inslit", "edit", "docs", NULL
    };
    enum {
        e_add, e_remove, e_load, e_inslit, e_edit, e_docs
    };

    char buffer[MAX_PATH];
    struct plugins *q;
    int action;

    NextToken(buffer, &args, NULL);
    action = get_string_index(buffer, actions);
    if (-1 == action)
        return 0;

    NextToken(buffer, &args, NULL);
	if (action > 3)
    {
		//check for multiple loadings
		if (IsInString(buffer, "/"))
		{
			char path[1];
			char *p = strcpy(path, buffer);
			*(char*)get_delim(p, '/') = 0; // strip "/#"
			strcpy(buffer, (const char*)strcat((char*)p, ".dll"));
		}


		char szBuffer[MAX_PATH];
		const char *path=plugrcPath();

		FILE *fp = fopen(path,"rb");
		if (fp)
		{
			if (e_edit == action)
			while (read_next_line(fp, szBuffer, sizeof szBuffer))
				parse_pluginRC(szBuffer, buffer);
			else
			while (read_next_line(fp, szBuffer, sizeof szBuffer))
				parse_pluginDocs(szBuffer, buffer);
			fclose(fp);
		}
	}

    if (e_add == action && 0 == buffer[0])
    {
#ifndef BBTINY
        if (false == browse_file(buffer,
            NLS1("Add Plugin"),
            "Plugins (*.dll)\0*.dll\0All Files (*.*)\0*.*\0"))
#endif
            return 1;
    }

    strcpy(buffer, get_relative_path(NULL, unquote(buffer)));

    if (e_add == action) {
        q = append_plugin(buffer);
        q->useslit = true;

    } else {
        dolist (q, bbplugins)
            if (q->name
                && 0 == stricmp(/*e_remove==action?q->path:*/q->name, buffer))
                break;
    }

    if (q) {

        if (e_remove == action)
            q->enabled = false;
        if (e_load == action)
            set_bool(&q->enabled, args);
        if (e_inslit == action)
            set_bool(&q->useslit, args);

        apply_plugin_state();

        if (e_remove == action || (e_add == action && false == q->enabled))
            free_plugin((struct plugins **)member_ptr(&bbplugins, q));

        write_plugins();
    }

    return 1;
}

//===========================================================================
