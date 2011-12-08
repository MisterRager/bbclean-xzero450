/*===================================================

	MODULE MASTER CODE

===================================================*/
/* Note to self: Merge module code into control handling. Perhaps a separate entity (in broam naming) */

// Global Include
#include "BBApi.h"
#include <string.h>
#include <stdlib.h>

//Parent Include
#include "ModuleMaster.h"

//Includes
#include "Definitions.h"
#include "MenuMaster.h"
#include "ListMaster.h"
#include "ConfigMaster.h"
#include "ControlMaster.h"
#include "MessageMaster.h"
#include "PluginMaster.h"
#include "DialogMaster.h"

//Global variables
module globalmodule; // Create the default module
module *currentmodule = &globalmodule; //NULL means the global module


//Local variables
list *modulelist = NULL;
//External functions
void control_save_control(control *c, struct renamed_control ** p_renamed_list);
bool control_is_valid_name(char *name);
// It seems it'd be better to put this whole code into ControlMaster.cpp, they're so deeply intertwined

//Constant variables

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//module_startup
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int module_startup()
{
	modulelist = list_create();

	//Init global module, or at least the parts we use
	strcpy(globalmodule.name,"");
	globalmodule.controllist = list_create();
	globalmodule.controllist_parentsonly = list_create();
	globalmodule.variables = list_create();
	globalmodule.actions[MODULE_ACTION_ONLOAD] = globalmodule.actions[MODULE_ACTION_ONUNLOAD] = 0;

	//No errors
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//module_shutdown
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int module_shutdown()
{
	listnode *ln, *ln2;

	//Destroy contained controls
	dolist (ln2, globalmodule.controllist_parentsonly)
		control_destroy((control *) ln2->value, false, false);
	dolist (ln, modulelist)
	{
		module *m = (module *) ln->value;
		if (m->enabled)
			dolist (ln2, m->controllist_parentsonly)
				control_destroy((control *) ln2->value, false, false);
	}

	//Destroy all modules
	dolist (ln, modulelist)
		module_destroy((module *) ln->value,false);

	//Destroy the list itself
	list_destroy(modulelist);

	list_destroy(globalmodule.controllist);
	list_destroy(globalmodule.controllist_parentsonly);

	dolist (ln, globalmodule.variables) free_string((char **)&ln->value);
	list_destroy(globalmodule.variables);

	delete[] globalmodule.actions[MODULE_ACTION_ONLOAD];
	delete[] globalmodule.actions[MODULE_ACTION_ONUNLOAD];

	//No errors
	return 0;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//module_set_author
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int module_set_author(module *m, const char *str)
{
	delete[] m->author;
	m->author = new_string(str);
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//module_set_comments
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int module_set_comments(module *m, const char *str)
{
	delete[] m->comments;
	m->comments = new_string(str);
	return 0;
}

int module_rename(module* m, char *newname)
{
	//Check the name to make sure it is valid
	if (!control_is_valid_name(newname)) return 1;

	//Rename the control if possible
	if (list_rename(modulelist, m->name, newname)) return 1;

	//Change the name
	strcpy(m->name, newname);

	return 0;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//module_get_info
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int module_get_info(char *filename, module *m)
{
	//Open the file
	FILE *config_file_in = config_open(filename, "rt");

	char config_line[BBI_MAX_LINE_LENGTH];
	if (config_file_in)
	{
		while (fgets(config_line, sizeof config_line, config_file_in))
		{
			char *p = strchr(config_line, 0);
			while (p > config_line && (unsigned char)p[-1] <= ' ') --p;
			*p = 0;

			if (config_line[0] == '!')
			{
				char *start = NULL;
				if ((start = strstr(config_line,"Module:")) && !m->name[0])
				{
					start += 7; //past the string
					while ( *start == ' ' ) ++start; //skip spaces
					if (control_is_valid_name(start) )
					{
						strcpy(m->name, start);
					}
				} else
				if ((start = strstr(config_line,"Author:")) && !m->author)
				{
					start += 7; //past the string
					while ( *start == ' ' ) ++start; //skip spaces
					m->author = new char[strlen(start) + 1];
					strcpy(m->author, start);
				} else
				if ((start = strstr(config_line,"Comments:")) && !m->comments)
				{
					start += 9; //past the string
					while ( *start == ' ' ) ++start; //skip spaces
					m->comments = new char[strlen(start) + 1];
					strcpy(m->comments, start);
				}
			}
		}
//		sprintf(config_line, "%s:\nRead info: Module: %s \n Author info: %s\n Comments: %s\n", filename, m->name, m->author, m->comments);
//		MessageBox(NULL, config_line, szAppName, MB_OK|MB_SYSTEMMODAL);

		//Close the file
		fclose(config_file_in);
		if (!m->name[0])
		{
			if (!plugin_suppresserrors)
			{
				sprintf(config_line, "%s:\n\nThe module has no name specified.\nMake sure there's a comment line in the file with the format: \"!-- Module: <name>\"", filename);
				MessageBox(NULL, config_line, szAppName, MB_OK|MB_SYSTEMMODAL);
			}
			return 1;
		}
		return 0;
	}

	//Must have been an error
	if (!plugin_suppresserrors)
	{
		sprintf(config_line, "%s:\nThere was an error loading the module.", filename);
		MessageBox(NULL, config_line, szAppName, MB_OK|MB_SYSTEMMODAL);
	}
	return 1;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//module_message
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int module_message(int tokencount, char *tokens[], bool from_core, module* caller)
{
	char *filename;

	if ((tokencount == 3) && !stricmp(tokens[2], szBActionCreate))
	{
		if (filename = dialog_file(szFilterScript, "Save new Module in...", ".rc", config_path_plugin, true))
		{
			if (module *m = module_create_new(filename)) { module_toggle(m); return 0; }
			else return 1;
		}           
	}
	else if ((tokencount > 2) && !stricmp(tokens[2], szBActionLoad))
	{
		if (tokencount == 3)
		{
			if ((filename = dialog_file(szFilterScript, "Load BBI Module", ".rc", config_path_plugin, false)))
			{
				if (module *m = module_create(filename)) module_toggle(m);
				else return 1;
			}
			if (from_core) { menu_update_modules(); menu_update_editmodules(); };
			return 0;
		}
		else if (tokencount == 4)
		{
			if (!module_create(tokens[3])) return 1;

			if (from_core) { menu_update_modules(); menu_update_editmodules(); };
			return 0;
		}
	}
	else if (tokencount == 4 && !stricmp(tokens[2], szBActionToggle) )
	{
		module_toggle(tokens[3]);
		if (from_core) menu_update_modules();
		return 0;
	}
	else if (tokencount == 4 && !stricmp(tokens[2], szBActionSetDefault) )
	{
		if (!stricmp(tokens[3],"*global*"))
			currentmodule = &globalmodule;
		else if (module *m = module_get(tokens[3]))
			currentmodule = m;
		if (from_core) menu_update_setactivemodule();
		return 0;  
	}

	//Following commands need a target module
	module *m = module_get(tokens[3]);
	if (!m) return 1;

	if (tokencount == 4 && !stricmp(tokens[2], szBActionEdit) )
	{
		char editor[MAX_PATH]; GetBlackboxEditor(editor);
		char temppath[MAX_PATH]; config_makepath(temppath, m->filepath);
		BBExecute(NULL, "", editor , temppath, NULL, SW_SHOWNORMAL, false);
		return 0;
	}
	else if (tokencount == 5 && !stricmp(tokens[2], szBActionOnLoad) )
	{
		config_set_str(tokens[4],&(m->actions[MODULE_ACTION_ONLOAD]));
		return 0;
	}
	else if (tokencount == 5 && !stricmp(tokens[2], szBActionOnUnload) )
	{
		config_set_str(tokens[4],&(m->actions[MODULE_ACTION_ONUNLOAD]));
		return 0;
	}
	else if (tokencount == 5 && !stricmp(tokens[2], szBActionRename) )
	{
		return module_rename(m,tokens[4]);
	}
	else if (tokencount == 6 && !stricmp(tokens[2], szBActionSetModuleProperty) )
	{
		if (!stricmp(tokens[4],"Author")) module_set_author(m,tokens[5]);
		else if (!stricmp(tokens[4],"Comments")) module_set_comments(m,tokens[5]);
		return 0;
	}
	return 1;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//module_create_new
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
module* module_create_new(char *filename)
{
	//create default module info into the file specified
	FILE* module_file = config_open(filename,"wt");
	if (!module_file) return NULL;

	//Find a name
	char tempname[64];
	int number = 1;
	do
	{
		sprintf(tempname, "Module%d", number);
		number++;

	} while (list_lookup(modulelist, tempname));

	fprintf(module_file,"!------ Module: %s", tempname);
	fclose(module_file);
	
	//Read back the generated info
	return module_create(filename);
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//module_create
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
module* module_create(char *filepath)
{
	module *m;

	//Create a new module
	m = new module;
	m->filepath = new char[strlen(filepath)+1];
	m->name[0] = 0;
	m->author = m->comments = 0;
	m->controllist = list_create();
	m->controllist_parentsonly = list_create();
	m->variables = list_create();
	m->actions[MODULE_ACTION_ONLOAD] = m->actions[MODULE_ACTION_ONUNLOAD] = 0;

	if (module_get_info(filepath, m)) //if the reading failed, or the module had no name
	{
		module_destroy(m, false);
		return NULL;
	}
	//All the required data were gathered if we get here.
	//Set default values
	strcpy(m->filepath, filepath);
	m->enabled = false;

	//Add to the list of modules
	if (list_add(modulelist, m->name, (void *) m, NULL))
	{
		module_destroy(m, false);
		return NULL;
	}

	//No errors
	return m;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//module_destroy
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int module_destroy(module *m, bool remove_from_list)
{
	//dbg_printf("destroying: %s", m->modulename);

	//Remove it from the list, if we care
	if (remove_from_list)
	{
		list_remove(modulelist, m->name);
	}

	//Free strings
	delete[] m->author;
	delete[] m->comments;
	delete[] m->filepath;
	delete[] m->actions[MODULE_ACTION_ONLOAD];
	delete[] m->actions[MODULE_ACTION_ONUNLOAD];

	//Delete the lists
	list_destroy(m->controllist);
	list_destroy(m->controllist_parentsonly);
	list_destroy(m->variables);

	//Destroy the module itself
	delete m;

	//No errors
	return 0;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//module_save_to_file
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int module_save_to_file(module *m)
{
	config_file_out = config_open(m->filepath,"wt");
	if (config_file_out) {
		config_printf_noskip("!------ Module: %s",m->name);
		if (m->author) config_printf_noskip("!-- Author: %s",m->author);
		if (m->comments) config_printf_noskip("!-- Comments: %s",m->comments);
		config_write( "!--------------------\n");

		//Save OnLoad/OnUnload actions
		if (m->actions[MODULE_ACTION_ONLOAD]) config_write(config_get_module_onload(m));
		if (m->actions[MODULE_ACTION_ONUNLOAD]) config_write(config_get_module_onunload(m));
		//config_write("");

		listnode *ln;
		//Save all variables
		if (m->variables->first)
		{
			dolist(ln,m->variables)
				config_write(config_get_variable_set_static(ln));
			config_write("");
		}

		//Save all controls
		dolist (ln, m->controllist_parentsonly)
			control_save_control((control *) ln->value, NULL);
		
		fclose(config_file_out);
		return 0;
	} // else return 1; // Error creating file, needs better handling. Preferably some suppresserrors thingy.
	
	if (!plugin_suppresserrors) MessageBox(NULL, "There was an error saving a module.", szAppName, MB_OK|MB_SYSTEMMODAL);
	return 1;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//module_save_all
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void module_save_all()
{
	listnode *ln;
	dolist(ln,modulelist)
	{
		module *m = (module *) ln->value;
		if (m->enabled) //Don't update disabled modules.
			module_save_to_file(m);
	}
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//module_toggle
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int module_toggle(module *m)
{
	if (m)
	{
		m->enabled = !m->enabled;
		if (m->enabled) {
			// Set this one as the selected module to add controls to.
			config_load(m->filepath,m);
			// A question would be whether to put this here, or after the active module has been restored.
			message_interpret(m->actions[MODULE_ACTION_ONLOAD], false, m);
		}
		else
		{
			if (plugin_save_modules_on_unload) module_save_to_file(m);
			// Do stuff before controls are unloaded.
			message_interpret(m->actions[MODULE_ACTION_ONUNLOAD], false, m);
			listnode *ln;
			// Destroy the associated controls
			dolist(ln, m->controllist_parentsonly)
				control_destroy((control *) ln->value, false, true);
			// Empty the list
			list_destroy(m->controllist);
			list_destroy(m->controllist_parentsonly);
			m->controllist = list_create();
			m->controllist_parentsonly = list_create();
			// Destroy the associated variables, too.
			dolist (ln, m->variables) free_string((char **)&ln->value);
			list_destroy(m->variables);
			m->variables = list_create();
			// Reset active module if needed.
			if (currentmodule == m) currentmodule = &globalmodule;
		}
		return 0;
	}
	return 1;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//module_toggle
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int module_toggle(char *key)
{
	// Find module	
	module *m = (module *)list_lookup(modulelist, key);
	return module_toggle(m);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//module_state
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bool module_state(char *modulename)
{
	module *m;
	
	// Find module
	m = (module *)list_lookup(modulelist, modulename);
	if (m)
	{
		return m->enabled;
	}
	return false;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//module_menu_editmodule_options
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Menu *module_menu_editmodule_options(module* m)
{
	Menu* menu, *submenu;
	listnode* ln;
	int n;

	menu = make_menu(m->name, m);
	make_menuitem_str(menu, "Name:", config_get_module_rename_s(m), m->name );
	make_menuitem_str(menu, "Author:", config_get_module_setauthor_s(m), m->author);
	make_menuitem_str(menu, "Comments:", config_get_module_setcomments_s(m), m->comments);
	make_menuitem_str(menu, "On load:", config_get_module_onload_s(m), m->actions[MODULE_ACTION_ONLOAD]);
	make_menuitem_str(menu, "On unload:", config_get_module_onunload_s(m), m->actions[MODULE_ACTION_ONUNLOAD]);
	// FIX: insert onload and unload actions, too

	/*
	//Control listing
	// Now, this doesn't seem like a horrible idea, however, it makes the loading time of menus too slow.
	sprintf(temp,"Controls (%s)",m->name);
	submenu = make_menu(temp, m);
	n = 0;
	dolist (ln, m->controllist)
	{
		control* c = (control*) ln->value;
		make_submenu_item(submenu,c->controlname,menu_control_submenu(c));
		++n;
	}
	if (n==0) make_menuitem_nop(submenu, "(None)");
	make_submenu_item(menu, "Controls", submenu);
	*/
	
	if (m->enabled) {
		char temp[120];
		sprintf(temp,"Variables (%s)",m->name);
		submenu = make_menu(temp, m);
		n = 0;
		dolist (ln, m->variables)
		{
			make_menuitem_str(submenu,ln->key,config_getfull_variable_set_static_s(m,ln),(char*)ln->value);
			++n;
		}
		if (n==0) make_menuitem_nop(submenu, "(None)");
		make_submenu_item(menu, "Variables", submenu);
	}

	make_menuitem_cmd(menu, "Edit RC", config_get_module_edit(m));

	return menu;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//module_menu_modulelist
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Menu* module_menu_modulelist(void)
{
	Menu *submenu;
	listnode *ln;
	bool temp;

	submenu = make_menu("Modules");
	if (!modulelist->first) {
		make_menuitem_nop(submenu, "None available.");
	} else
	dolist (ln, modulelist) {
		module *m = (module *) ln->value;
		temp = m->enabled;
		make_menuitem_bol(submenu, m->name, config_get_module_toggle(m), temp);
	}
	return submenu;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//module_menu_editmodules
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Menu* module_menu_editmodules(void)
{
	Menu *submenu;
	listnode *ln;

	submenu = make_menu("Edit Modules");

	dolist (ln, modulelist) {
		module *m = (module *) ln->value;
		make_submenu_item(submenu, m->name, module_menu_editmodule_options(m));
//		make_menuitem_cmd(submenu, m->name, config_get_plugin_editmodule(m));
	}
	make_menuitem_cmd(submenu, "Add module...", config_get_module_load_dialog());
	return submenu;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//module_menu_setactivemodule
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Menu* module_menu_setactivemodule(void)
{
	Menu *submenu;
	listnode *ln;
	bool temp;

	submenu = make_menu("Set Default Module");

	temp = currentmodule == &globalmodule;
	make_menuitem_bol(submenu, "Global", config_get_module_setdefault(&globalmodule), temp);

	dolist (ln, modulelist) {
		module *m = (module *) ln->value;
		if(m->enabled)
		{		
			temp = currentmodule == m;
			make_menuitem_bol(submenu, m->name, config_get_module_setdefault(m), temp);
		}
	}
	return submenu;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//module_save
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void module_save_list()
{
	//Variables
	listnode *ln;

	if (modulelist->first)
	{
		//Save loaded modules
		config_write("\n!---- Loaded modules ----");
		int n = 0; //Just a quick counter for enabled modules
		dolist (ln, modulelist)
		{       
			module *m = (module *) ln->value;
			config_write(config_get_module_load(m));
			if (m->enabled) ++n;
		}

		//Save active modules
		if (n != 0)
		{
			config_write("\n!---- Active modules ----");
			dolist (ln, modulelist)
			{       
				module *m = (module *) ln->value;
				if (m->enabled) config_write(config_get_module_toggle(m));
			}
		}
	}
}
module* module_get(char* key)
{
	return (module*) list_lookup(modulelist,key);
}