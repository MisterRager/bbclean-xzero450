/*===================================================

	MENU MASTER CODE

===================================================*/

// Global Include
#include "BBApi.h"

//Includes
#include "AgentMaster.h"
#include "ControlMaster.h"
#include "WindowMaster.h"
#include "Definitions.h"
#include "ConfigMaster.h"
#include "PluginMaster.h"
#include "ListMaster.h"
#include "MenuMaster.h"
#include "ModuleMaster.h"

//Variables

//Internally defined functions
void menu_control_interfaceoptions(Menu *m, control *c);
void menu_control_pluginoptions(Menu *m);
void menu_control_controloptions(Menu *m, control *c, char *title);
void menu_control_windowoptions(Menu *m, control *c, char *title);

static bool menu_initialize(control *c, bool pop);

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//menu_startup
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int menu_startup()
{
	//No errors
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//menu_shutdown
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int menu_shutdown()
{
	menu_initialize(NULL, true);
	//No errors
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//menu_control
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void menu_control(control *c, bool pop)
{
	// -----------------------------------
	//Variables
	char temp[120]; Menu *m;

	// menu update support
	if (false == menu_initialize(c, pop))
		return;

	//Make the new menu
	//NOTE: Include module name in menu title? Possible menu name duplication is already resolved in make_menu
	sprintf(temp, "%s (%s)", c->controlname,  c->controltypeptr->controltypename);
	m = make_menu(temp, c);

	//Add the appropriate menus
	menu_control_interfaceoptions(m, c);
	menu_control_controloptions(m, c, "Control Options");
	menu_control_windowoptions(m, c, "Window Options");
/*
	if (c->parentptr)
	{
		menu_control_controloptions(m, c->parentptr, "Parent Control Options");
		menu_control_windowoptions(m, c->parentptr, "Parent Window Options");
	}
*/
	make_submenu_item(m, "Global Options", plugin_menu_settings());
	menu_control_pluginoptions(m);

	//Show the menu
	show_menu(c, m, pop);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//menu_control
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Menu* menu_control_submenu(control *c)
{
	// NOTE: Currently unused, could be in the Edit Module > Control list.
	// -----------------------------------
	//Variables
	char temp[120]; Menu *m;

	//Make the new menu
	sprintf(temp, "%s (%s)", c->controlname,  c->controltypeptr->controltypename);
	m = make_menu(temp, c);

	//Add the appropriate menus
	menu_control_interfaceoptions(m, c);
	menu_control_controloptions(m, c, "Control Options");
	menu_control_windowoptions(m, c, "Window Options");
	return m;
}


void menu_update_global(void)
{
	if (false == menu_initialize(NULL, false))
		return;

	show_menu(NULL, plugin_menu_settings(), false);
}

void menu_update_modules(void)
{
	if (false == menu_initialize(NULL, false))
		return;

	show_menu(NULL, module_menu_modulelist(), false);
}

void menu_update_editmodules(void)
{
	if (false == menu_initialize(NULL, false))
		return;

	show_menu(NULL, module_menu_editmodules(), false);
}

void menu_update_setactivemodule(void)
{
	if (false == menu_initialize(NULL, false))
		return;

	show_menu(NULL, module_menu_setactivemodule(), false);
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//menu_controloptions
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void menu_controloptions(Menu *m, control *c, int count, agent *agents[], char *actionprefix, char *actions[], const int types[])
{
	//Declare variables
	char completeactionname[1024];

	for (int i = 0; i < count; i++)
	{
		//Make the complete action name
		sprintf(completeactionname, "%s%s", actionprefix, actions[i]);

		//Make the submenu for this action
		Menu *submenu = make_menu(actions[i], c);

		//If there is an agent, add its options menu
		if (agents[i])
		{
			make_menuitem_nop(submenu, agents[i]->agenttypeptr->agenttypenamefriendly);
			Menu *submenu2 = make_menu("Options", c);
			agent_menu_context(submenu2, c, agents[i]);
			make_submenu_item(submenu, "Options", submenu2);
			make_menuitem_nop(submenu, NULL);
			make_menuitem_nop(submenu, NULL);
		}


		//Make the agent selection menus for this action
		agent_menu_set(submenu, c, agents[i], types[i], completeactionname);

		//Add the submenu to the parent menu
		make_submenu_item(m, actions[i], submenu);
	}
}

//##################################################
//menu_control_interfaceoptions
//##################################################

void menu_control_interfaceoptions(Menu *m, control *c)
{
	//Variables
	Menu *submenu, *submenu2;
	//Make the operations menu  
	submenu = make_menu("Interface Operations", c);

	//Create new controls
	submenu2 = make_menu("Create New Control", c);
	control_menu_create(submenu2, c, false);
	make_submenu_item(submenu, "Create New Control", submenu2);
	if (c->controltypeptr->can_parent) //If it can be a parent
	{
		submenu2 = make_menu("Create New Child Control", c);
		control_menu_create(submenu2, c, true);
		make_submenu_item(submenu, "Create New Child Control", submenu2);
	}
	make_menuitem_cmd(submenu,"Create New Module",config_get_module_create());
	make_menuitem_nop(submenu, NULL);
	control_menu_settings(submenu, c);
	make_submenu_item(m, "Interface Operations", submenu);
}

//##################################################
//menu_control_pluginoptions
//##################################################
void menu_control_pluginoptions(Menu *m)
{
	Menu *submenu;
	submenu = make_menu("Configuration");
	make_submenu_item(submenu, "Modules", module_menu_modulelist());
	make_submenu_item(submenu, "Edit Modules", module_menu_editmodules());
	make_submenu_item(submenu, "Set Default Module", module_menu_setactivemodule());
	make_menuitem_nop(submenu, NULL);
	make_menuitem_cmd(submenu, "Load Configuration Script...", config_get_plugin_load_dialog());
	make_menuitem_cmd(submenu, "Edit Configuration Script", config_get_plugin_edit());
	make_menuitem_nop(submenu, NULL);
	make_menuitem_cmd(submenu, "Save Configuration", config_get_plugin_save());
	make_menuitem_cmd(submenu, "Save Configuration As...", config_get_plugin_saveas());
	make_menuitem_nop(submenu, NULL);
	make_menuitem_cmd(submenu, "Reload Configuration", config_get_plugin_revert());
	make_submenu_item(m, "Configuration", submenu);

	submenu = make_menu("Help");
	make_menuitem_cmd(submenu, "Quick Reference...", "@BBInterface Plugin About QuickHelp");
	make_menuitem_cmd(submenu, "About...", "@BBInterface Plugin About");
	make_menuitem_cmd(submenu, "Show Welcome Interface", "@BBInterface Plugin Load WelcomeScript\\welcome.rc");
	make_submenu_item(m, "Help", submenu);
}


//##################################################
//controltype_button_startup
//##################################################
void menu_control_controloptions(Menu *m, control *c, char *title)
{
	//Variables
	Menu *submenu;

	//Make the operations menu  
	submenu = make_menu(title, c);

	//Make the name option
	make_menuitem_str(
		submenu,
		"Control Name:",
		config_getfull_control_renamecontrol_s(c),
		c->controlname
		);
	make_menuitem_nop(submenu, "");

	control_menu_context(submenu, c);
	make_submenu_item(m, title, submenu);
}

//##################################################
//controltype_button_startup
//##################################################
void menu_control_windowoptions(Menu *m, control *c, char *title)
{
	//Variables
	Menu *submenu;

	//Make the operations menu  
	submenu = make_menu(title, c);
	window_menu_context(submenu, c);
	make_submenu_item(m, title, submenu);
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// blackbox menu wrapper
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// for convenience the unique menu id's are generated automatically from
// the control-name and a incremented number.

#define COMMON_MENU

#if 1
static int menu_num_global;
static bool menu_pop_global;
static Menu* last_menu_global;

static char *get_menu_id(char *buffer, control *c, module* m, const char *title)
{
	if (c)
	{
		if (title)
			sprintf(buffer, "BBIMenu::Control-%s-%s-%s-%d", c->moduleptr->name, c->controlname, title, ++menu_num_global);
		else
			sprintf(buffer, "BBIMenu::Control-%s-%s", c->moduleptr->name, c->controlname);
	}
	else if (m)
	{
		sprintf(buffer, "BBIMenu::Module-%s-%s", m->name, title ? title : "");
	}
	else
	{
		sprintf(buffer, "BBIMenu::Global-%s", title ? title : "");
	}
	return buffer;
}

static bool defMenuExists(LPCSTR IDString_start)
{
	return true;
}

static bool menu_exists(control *c)
{
	static bool (*pMenuExists)(LPCSTR IDString_start);
	if (NULL == pMenuExists)
	{
		*(FARPROC*)&pMenuExists = GetProcAddress(GetModuleHandle(NULL), "MenuExists");
		if (NULL == pMenuExists) pMenuExists = defMenuExists;
	}

	char menu_id[200];
	get_menu_id(menu_id, c, NULL, NULL);
	return pMenuExists(menu_id);
}

static bool menu_initialize(control *c, bool pop)
{
	menu_pop_global = pop;
	menu_num_global = 0;
#ifdef COMMON_MENU
	return pop || menu_exists(c);
#else
	if (BBVERSION_LEAN == BBVersion)
		return pop || menu_exists(c);

	if (pop && BBVERSION_09X == BBVersion && last_menu_global)
		DelMenu(last_menu_global), last_menu_global = NULL;

	return pop;
#endif
}

Menu *make_menu(const char *title, control* c)
{
#ifndef COMMON_MENU
	if (BBVERSION_LEAN != BBVersion)
		return MakeMenu(title);
#endif
	char menu_id[356];
	get_menu_id(menu_id, c, NULL, title);
	return MakeNamedMenu(title, menu_id, menu_pop_global);
}

Menu *make_menu(const char *title, module* m)
{
#ifndef COMMON_MENU
	if (BBVERSION_LEAN != BBVersion)
		return MakeMenu(title);
#endif
	char menu_id[300];
	get_menu_id(menu_id, NULL, m, title);
	return MakeNamedMenu(title, menu_id, menu_pop_global);
}


void make_menuitem_str(Menu *m, const char *title, const char* cmd, const char * init_string)
{
	char buffer[BBI_MAX_LINE_LENGTH];

	if (BBVERSION_LEAN == BBVersion)
		sprintf(buffer, "%s \"%%s\"", cmd), cmd = buffer;

	if (BBVERSION_09X == BBVersion)
		sprintf(buffer, "\"%s\"", init_string), init_string = buffer;

	MakeMenuItemString(m, title, cmd, init_string);
}

void make_menuitem_int(Menu *m, const char *title, const char* cmd, int val, int minval, int maxval)
{
	if (BBVERSION_09X == BBVersion)
	{
		char buffer[20]; sprintf(buffer, "%d", val);
		MakeMenuItemString(m, title, cmd, buffer);
	}
	else
	{
		MakeMenuItemInt(m, title, cmd, val, minval, maxval);
	}
}

void make_menuitem_bol(Menu *m, const char *title, const char* cmd, bool checked)
{
	MakeMenuItem(m, title, cmd, checked);
}

void make_menuitem_cmd(Menu *m, const char *title, const char* cmd)
{
	MakeMenuItem(m, title, cmd, false);
}

void make_menuitem_nop(Menu *m, const char *title)
{
	MakeMenuNOP(m, title);
}

void make_submenu_item(Menu *m, const char *title, Menu *sub)
{
	MakeSubmenu(m, sub, title);
}

void show_menu(control *c, Menu *m, bool pop)
{
	last_menu_global = m;
	ShowMenu(m);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#else
#define new_str new_string
#define free_str free_string
#include "NewMenu.cpp"
#endif
