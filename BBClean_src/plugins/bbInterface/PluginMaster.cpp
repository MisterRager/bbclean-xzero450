/*===================================================

	PLUGIN MASTER CODE

===================================================*/
// Global Include
#include "BBApi.h"

//Define the ALPHA SOFTWARE flag
//This will cause an annoying message box to pop up and confirm
//that the user wants to load it.  Comment in or out as desired.
//#define BBINTERFACE_ALPHA_SOFTWARE

//Parent Include
#include "PluginMaster.h"

//Includes
#include "Definitions.h"
#include "StyleMaster.h"
#include "WindowMaster.h"
#include "ControlMaster.h"
#include "AgentMaster.h"
#include "MessageMaster.h"
#include "ConfigMaster.h"
#include "DialogMaster.h"
#include "MenuMaster.h"
#include "Tooltip.h"
#include "ModuleMaster.h"

#include "ControlType_Label.h"
#include "ControlType_Button.h"
#include "ControlType_Slider.h"

#include "AgentType_Broam.h"
#include "AgentType_Winamp.h"
#include "AgentType_Mixer.h"
#include "AgentType_StaticText.h"
#include "AgentType_CompoundText.h"
#include "AgentType_Run.h"
#include "AgentType_Bitmap.h"
#include "AgentType_TGA.h"
#include "AgentType_System.h"
#include "AgentType_SystemMonitor.h"
#include "AgentType_SwitchedState.h"
#include "AgentType_Graph.h"

#ifdef _DEBUG
//Memory leak tracking - not used in production version
#define CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

/*=================================================*/
//Plugin information

const char szVersion      [] = "BBInterface 0.9.9";   // Used in MessageBox titlebars
const char szAppName      [] = "BBInterface";          // The name of our window class, etc.
const char szInfoVersion  [] = "0.9.9";
const char szInfoAuthor   [] = "psyci - additions by grischka/kana/ysuke/pkt-zer0/jancuk";
const char szInfoRelDate  [] = "2006-08-12";
const char szInfoLink     [] = "";
const char szInfoEmail    [] = "";

//Local variables
const char szPluginAbout  [] =
"BBInterface - Written Feb 2004 by psyci in many long, caffeine filled hours."
"\n"
"\nThanks to:"
"\n    The BlackBox for Windows Development Team,"
"\n    the folks from Freenode #bb4win,"
"\n    \"jglatt\", for loads of information on Windows Mixers,"
"\n    and a very sexy young lady who somehow put up with me coding until 5 AM\t"
"\n    while she tried to sleep five feet away."
"\n"
"\nBBInterface is licensed under the GPL."
"\n"
"\nHistory:"
"\nFeb 2004 - Original release by psyci."
"\nNov 2004 - Snap controls and plugin management added by grischka."
"\nMay 2006 - Added new agents and window properties by psyci."
;

const char szPluginAboutLastControl [] =
"This control has been created because there are no more\n"
"BBInterface controls.\n\n"
"To create a new one, Control+Right Click this button, and\n"
"use the menus to create new controls.\n\n"
"If you don't want any more controls, please just unload\n"
"the plugin."
;

const char szPluginAboutQuickRef [] =
"Control + Right Click = Show Menu.\n"
"Control + Drag = Move\n"
"Control + Shift + Drag = Move Parent\n"
"Alt + Drag = Resize\n"
;

//Strings used frequently
const char szBBroam                     []  = "@BBInterface";
const int szBBroamLength = sizeof szBBroam-1;

const char szBEntityControl             [] = "Control";
const char szBEntityAgent               [] = "Agent";
const char szBEntityPlugin              [] = "Plugin";
const char szBEntityVarset              [] = "Set";
const char szBEntityWindow              [] = "Window";
const char szBEntityModule              [] = "Module";

const char szBActionCreate              [] = "Create";
const char szBActionCreateChild         [] = "CreateChild";
const char szBActionDelete              [] = "Delete";

const char szBActionSetAgent            [] = "SetAgent";
const char szBActionRemoveAgent         [] = "RemoveAgent";

const char szBActionSetAgentProperty    [] = "SetAgentProperty";
const char szBActionSetControlProperty  [] = "SetControlProperty";  
const char szBActionSetWindowProperty   [] = "SetWindowProperty";
const char szBActionSetPluginProperty   [] = "SetPluginProperty";
const char szBActionSetModuleProperty   [] = "SetModuleProperty";

//const char szBActionAddModule           [] = "AddModule";
const char szBActionToggle				[] = "Toggle";
const char szBActionSetDefault		    [] = "SetDefault";
const char szBActionAssignToModule      [] = "AssignToModule";
const char szBActionDetachFromModule    [] = "DetachFromModule";

const char szBActionOnLoad				[] = "OnLoad";
const char szBActionOnUnload			[] = "OnUnload";

const char szBActionRename              [] = "Rename"; 
const char szBActionLoad                [] = "Load";
const char szBActionEdit                [] = "Edit";
const char szBActionSave                [] = "Save";
const char szBActionSaveAs              [] = "SaveAs"; 
const char szBActionRevert              [] = "Revert"; 
const char szBActionAbout               [] = "About";

const char szTrue                       [] = "true";
const char szFalse                      [] = "false";

const char szFilterProgram  [] = "Programs\0*.exe;*.bat;*.com\0Screen Savers\0*.scr\0All Files\0*.*\0";
const char szFilterScript   [] = "Script Files\0*.rc;\0All Files\0*.*\0";
const char szFilterAll      [] = "All Files\0*.*\0";

//Convenient arrays of strings
const char *szBoolArray[2] = {  szFalse, szTrue };

/*=================================================*/
//Special Functions

extern "C"
{
	DLL_EXPORT int beginPlugin(HINSTANCE hMainInstance);
	DLL_EXPORT void endPlugin(HINSTANCE hMainInstance);
	DLL_EXPORT LPCSTR pluginInfo(int field);
	DLL_EXPORT int beginSlitPlugin(HINSTANCE hMainInstance, HWND hSlit);
	DLL_EXPORT int beginPluginEx(HINSTANCE hMainInstance, HWND hSlit);
}

/*=================================================*/
//Global variables

HINSTANCE plugin_instance_plugin = NULL;
HWND plugin_hwnd_blackbox = NULL;
HWND plugin_hwnd_slit = NULL;

/*=================================================*/
// plugin configuration

bool plugin_using_modern_os = false;
bool plugin_zerocontrolsallowed = false;    
bool plugin_suppresserrors = true;
bool plugin_enableshadows = false;
bool plugin_click_raise = false;
bool plugin_snapwindows = true;
// bool plugin_backup_script = true; // A global variable didn't seem to be the most compatible option.
char *plugin_desktop_drop_command = NULL;
bool plugin_clear_configuration_on_load = false;
bool plugin_save_modules_on_unload = true;

int plugin_snap_padding     = 2;
int plugin_snap_dist        = 7;
bool plugin_load = false;

int plugin_icon_sat = 255;
int plugin_icon_hue = 0;

int BBVersion;

#define M_BOL 1
#define M_INT 2
#define M_STR 3

struct plugin_properties { const char *key; const char *menutext; void *data; unsigned char type:3, update:1; short minval, maxval; } plugin_properties[] =
{
	{ "SnapWindows",            "Snap To Edges",        &plugin_snapwindows,    M_BOL, 0 },
	{ "SnapDistance",           "Snap Trigger Distance", &plugin_snap_dist,     M_INT, 0,   0,  20 },
	{ "SnapPadding",            "Snap Padding",         &plugin_snap_padding,   M_INT, 0, -10,  40 },
	{ "" },
	{ "IconSaturation",         "Icon Saturation",      &plugin_icon_sat,       M_INT, 1, 0, 255 },
	{ "IconHue",                "Icon Hue",             &plugin_icon_hue,       M_INT, 1, 0, 255 },
	{ "" },
	{ "DeskDropCommand",        "Desktop OnDrop Command", &plugin_desktop_drop_command, M_STR, 0 },
	{ "GlobalOnload",        "Global OnLoad", &globalmodule.actions[MODULE_ACTION_ONLOAD], M_STR, 0 },
	{ "GlobalOnunload",        "Global OnUnload", &globalmodule.actions[MODULE_ACTION_ONUNLOAD], M_STR, 0 },
	{ "" },
	{ "ClickRaise",             "Click Raise",          &plugin_click_raise,    M_BOL, 0 },
	{ "SuppressErrors",         "Suppress Errors",      &plugin_suppresserrors, M_BOL, 0 },
	{ "ZeroControlsAllowed",    "Allow Zero Controls",  &plugin_zerocontrolsallowed, M_BOL, 0 },
	{ "EnableShadows",			"Enable Shadows",		&plugin_enableshadows, M_BOL, 1 },
	{ "UseTooltip",             "Use Tooltip",          &tooltip_enabled, M_BOL, 0 },
	{ "ClearConfigurationOnLoad", "Clear Configuration On Load", &plugin_clear_configuration_on_load, M_BOL, 0 },
	{ "ModuleAutoSave", "Save Modules On Unload", &plugin_save_modules_on_unload, M_BOL, 0 },
	{ NULL, NULL, 0 }
};

/*=================================================*/
//Internal functions
void plugin_getosinfo();
void plugin_controls_startup();
void plugin_controls_shutdown();
void plugin_agents_startup();
void plugin_agents_shutdown();

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//beginPlugin
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int beginPlugin(HINSTANCE hMainInstance)
{
	if (plugin_hwnd_blackbox)
	{
		MessageBox(plugin_hwnd_blackbox, (LPCWSTR)"Dont load me twice!", (LPCWSTR)szAppName, MB_OK|MB_SETFOREGROUND);
		return 1;
	}

	//Deal with instances
	plugin_instance_plugin = hMainInstance;
	plugin_hwnd_blackbox = GetBBWnd();

	const char *bbv = GetBBVersion();
	if (0 == memicmp(bbv, "bblean", 6)) BBVersion = BBVERSION_LEAN;
	else
	if (0 == memicmp(bbv, "bb", 2)) BBVersion = BBVERSION_XOB;
	else BBVersion = BBVERSION_09X;

	//Deal with os info
	plugin_getosinfo();

#ifdef BBINTERFACE_ALPHA_SOFTWARE
	int result = BBMessageBox(plugin_hwnd_blackbox,
		"WARNING!\n\n"
		"This is ALPHA software! Use at your own risk!\n\n"
		"The authors are not responsible in the event that:\n - your configuration is lost,\n - your computer blows up,\n - you are hit by a truck, or\n - anything else at all.\n\n"
		"Do you wish to continue loading this ALPHA software?",
		"BBInterface ALPHA Warning",
		MB_ICONWARNING|MB_DEFBUTTON2|MB_YESNO);
	if (result != IDYES) return 0;
#endif

	//Startup
	plugin_load = true;
	plugin_startup();

	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//beginPlugin
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int beginSlitPlugin(HINSTANCE hMainInstance, HWND hSlit)
{
	plugin_hwnd_slit = hSlit;
	return beginPlugin(hMainInstance);
}

int beginPluginEx(HINSTANCE hMainInstance, HWND hSlit)
{
	return beginSlitPlugin(hMainInstance, hSlit);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//endPlugin
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void endPlugin(HINSTANCE hMainInstance)
{
	if (plugin_load) plugin_shutdown(true);
#ifdef _DEBUG
	//Memory leak tracking - not used in production version
	_CrtDumpMemoryLeaks();
#endif
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//pluginInfo
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
LPCSTR pluginInfo(int field)
{
	switch (field)
	{
		default:
		case 0: return szVersion;
		case 1: return szAppName;
		case 2: return szInfoVersion;
		case 3: return szInfoAuthor;
		case 4: return szInfoRelDate;
		case 5: return szInfoLink;
		case 6: return szInfoEmail;
	}
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//plugin_startup
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void plugin_startup()
{
	//Startup the configuration system
	config_startup();

	//Startup style system
	style_startup();

	//Startup window system
	window_startup();

	menu_startup();

	//Startup controls and agents
	variables_startup();
	control_startup();
	agent_startup();

	//Startup message & dialog system
	message_startup();
	dialog_startup();

	//Startup control types
	plugin_controls_startup();

	//Startup agent types
	plugin_agents_startup();

	tooltip_startup();
	module_startup();

	//Load the config file
	if (0 == config_load(config_path_mainscript, &globalmodule))
	{
		check_mainscript_filetime();
		if (!check_mainscript_version())
			SetTimer(message_window, 1, 1000, NULL);
	}
	control_checklast();

	message_interpret(globalmodule.actions[MODULE_ACTION_ONLOAD], false, &globalmodule);

}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//plugin_shutdown
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void plugin_shutdown(bool save)
{
	message_interpret(globalmodule.actions[MODULE_ACTION_ONUNLOAD], false, &globalmodule);

	//Save config settings
	if (save) config_save(config_path_mainscript);

	//Shutdown the message & dialog system
	message_shutdown();
	dialog_shutdown();

	//Shutdown agents and controls
	variables_shutdown();
	module_shutdown();
	control_shutdown();
	agent_shutdown();   

	//Shutdown the windowing system
	window_shutdown();
	
	//Shutdown the style system.
	style_shutdown();

	//Shutdown control types
	plugin_controls_shutdown();

	//Shutdown agents
	plugin_agents_shutdown();

	tooltip_shutdown();

	//Shutdown the configuration system
	config_shutdown();

	menu_shutdown();

}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//plugin_reconfigure
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
static void _restart_specific_systems(void)
{
	//Shutdown specific systems
	module_shutdown();
	variables_shutdown();
	control_shutdown();
	agent_shutdown();
	style_shutdown();
	plugin_controls_shutdown();
	plugin_agents_shutdown();

	//Restart agent and control masters
	style_startup();
	control_startup();
	agent_startup();
	plugin_controls_startup();
	plugin_agents_startup();
	module_startup();
	variables_startup();

}

void plugin_reconfigure(bool force)
{
	if (force || (FileExists(config_path_mainscript) && check_mainscript_filetime()))
	{
		_restart_specific_systems();

		//Reload config file
		config_load(config_path_mainscript, &globalmodule);
		control_checklast();
	}
	else
	{
		style_shutdown();
		style_startup();
		control_invalidate();
	}
}


static int config_load2(char *filename, module* caller)
{
	if (plugin_clear_configuration_on_load)
		_restart_specific_systems();
	return config_load(filename, caller);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//plugin_message
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int plugin_message(int tokencount, char *tokens[], bool from_core, module* caller)
{
	char *filename;

	if (tokencount == 3 && !stricmp(tokens[2], szBActionSave))
	{
		config_save(config_path_mainscript);
		return 0;
	}
	else if (!stricmp(tokens[2], szBActionSaveAs))
	{
		if (tokencount == 4)
		{
			config_save(tokens[3]);
		}
		else
		{       
			if ((filename = dialog_file(szFilterScript, "Save Configuration Script", ".rc", config_path_plugin, true)))
			{
				config_save(filename);
			}           
		}       
		return 0;
	}
	else if (tokencount == 3 && !stricmp(tokens[2], szBActionRevert))
	{
		plugin_reconfigure(true);
		return 0;
	}
	else if (!stricmp(tokens[2], szBActionLoad))
	{
		if (tokencount == 4)
		{
			config_load2(tokens[3], caller);
			return 0;
		}
		else if (tokencount == 3)
		{
			if ((filename = dialog_file(szFilterScript, "Load Configuration Script", ".rc", config_path_plugin, false)))
			{
				config_load2(filename, caller);
			}
			return 0;
		}
		else if (tokencount == 6)
		{
			if (0 == stricmp(tokens[4], "from"))
			{
				config_load(tokens[5], caller, tokens[3]);
				return 0;
			} else if (0 == stricmp(tokens[4], "into"))
			{
				config_load(tokens[3], module_get(tokens[5]));
				return 0;
			}
		}
		else if (tokencount == 8 && 0 == stricmp(tokens[4], "from") && 0 == stricmp(tokens[6], "into"))
		{
			config_load(tokens[5], module_get(tokens[7]), tokens[3]);
			return 0;
		}
	}
	else if (!stricmp(tokens[2], szBActionAbout))
	{
		if (tokencount == 3)
		{
			BBMessageBox(NULL, szPluginAbout, szVersion, MB_OK|MB_SYSTEMMODAL);
			return 0;
		}
		else if (tokencount == 4 && !stricmp(tokens[3], "LastControl"))
		{
			BBMessageBox(NULL, szPluginAboutLastControl, szAppName, MB_OK|MB_SYSTEMMODAL);
			return 0;
		}
		else if (tokencount == 4 && !stricmp(tokens[3], "QuickHelp"))
		{
			BBMessageBox(NULL, szPluginAboutQuickRef, szAppName, MB_OK|MB_SYSTEMMODAL);
			return 0;
		}
	}
	else if (!stricmp(tokens[2], szBActionEdit))
	{
		//SendMessage(plugin_hwnd_blackbox, BB_EDITFILE, (WPARAM)-1, (LPARAM) config_path_mainscript);
		//return 0;
		char temp[MAX_PATH]; GetBlackboxEditor(temp);
		BBExecute(NULL, "",temp , config_path_mainscript, NULL, SW_SHOWNORMAL, false);
		return 0;
	}
	else if (tokencount == 5 && !stricmp(tokens[2], szBActionSetPluginProperty))
	{
		for (struct plugin_properties *p = plugin_properties; p->key; p++)
			if (p->data && 0 == stricmp(tokens[3], p->key)) {
				switch (p->type) {
					case M_BOL:
						if (config_set_bool(tokens[4], (bool*)p->data)) break; return 1;
					case M_INT:
						if (config_set_int(tokens[4], (int*)p->data)) break; return 1;
					case M_STR:
						if (config_set_str(tokens[4], (char**)p->data)) break; return 1;
					default: return 1;
				}
				if (p->update) control_invalidate();
				if (from_core) menu_update_global();
				return 0;
			}
	}
	else if (tokencount == 4 && !stricmp(tokens[2], szBActionOnLoad) )
	{
		config_set_str(tokens[3],&(globalmodule.actions[MODULE_ACTION_ONLOAD]));
		return 0;
	}
	else if (tokencount == 4 && !stricmp(tokens[2], szBActionOnUnload) )
	{
		config_set_str(tokens[3],&(globalmodule.actions[MODULE_ACTION_ONUNLOAD]));
		return 0;
	}
	return 1;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//plugin_save
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void plugin_save()
{
	config_printf("!---- %s ----", szVersion);
	for (struct plugin_properties *p = plugin_properties; p->key; p++)
		if (p->data) switch (p->type) {
			case M_BOL: config_write(config_get_plugin_setpluginprop_b(p->key, (bool*)p->data)); break;
			case M_INT: config_write(config_get_plugin_setpluginprop_i(p->key, (const int*)p->data)); break;
			case M_STR: config_write(config_get_plugin_setpluginprop_c(p->key, *(const char**)p->data)); break;
		}
	//Save OnLoad/OnUnload actions
	config_write(config_get_plugin_onload());
	config_write(config_get_plugin_onunload());

	variables_save();

}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Menu *plugin_menu_settings(void)
{
	Menu *m = make_menu("Global Options");
	bool tmp;
	for (struct plugin_properties *p = plugin_properties; p->key; p++)
		switch (p->type)
		{
			case M_BOL:
				tmp = false == *(bool*)p->data;
				make_menuitem_bol(m, p->menutext, config_get_plugin_setpluginprop_b(p->key, &tmp), !tmp);
				break;
			case M_INT:
				make_menuitem_int(m, p->menutext, config_get_plugin_setpluginprop_s(p->key), *(const int*)p->data, p->minval, p->maxval);
				break;
			case M_STR:
				make_menuitem_str(m, p->menutext, config_get_plugin_setpluginprop_s(p->key), *(const char **)p->data ? *(const char**)p->data : "");
				break;
			default:
				make_menuitem_nop(m, NULL);
				break;
		}
	return m;
}

//##################################################
//plugin_getosinfo
//##################################################
void plugin_getosinfo()
{
	//This code stolen from the bb4win SDK example
	OSVERSIONINFO osvinfo;
	ZeroMemory(&osvinfo, sizeof(osvinfo));
	osvinfo.dwOSVersionInfoSize = sizeof (osvinfo);
	GetVersionEx(&osvinfo);
	plugin_using_modern_os =
		osvinfo.dwPlatformId == VER_PLATFORM_WIN32_NT
		&& osvinfo.dwMajorVersion > 4;
}

//##################################################
//plugin_controls_startup
//##################################################
void plugin_controls_startup()
{
	controltype_label_startup();
	controltype_button_startup();
	controltype_slider_startup();
}

//##################################################
//plugin_controls_shutdown
//##################################################
void plugin_controls_shutdown()
{
	controltype_label_shutdown();
	controltype_button_shutdown();
	controltype_slider_shutdown();
}

//##################################################
//plugin_agents_startup
//##################################################
void plugin_agents_startup()
{
	agenttype_run_startup();
	agenttype_broam_startup();
	agenttype_system_startup();
	agenttype_winamp_startup();
	agenttype_mixer_startup();
	agenttype_statictext_startup();
	agenttype_compoundtext_startup();
	agenttype_bitmap_startup();
	agenttype_tga_startup();
	agenttype_systemmonitor_startup();
	agenttype_switchedstate_startup();
	agenttype_graph_startup();
}

//##################################################
//plugin_agents_shutdown
//##################################################
void plugin_agents_shutdown()
{
	agenttype_run_shutdown();
	agenttype_broam_shutdown();
	agenttype_system_shutdown();
	agenttype_winamp_shutdown();
	agenttype_mixer_shutdown();
	agenttype_statictext_shutdown();
	agenttype_compoundtext_shutdown();
	agenttype_bitmap_shutdown();
	agenttype_tga_shutdown();
	agenttype_systemmonitor_shutdown();
	agenttype_switchedstate_shutdown();
	agenttype_graph_shutdown();
}

//##################################################
