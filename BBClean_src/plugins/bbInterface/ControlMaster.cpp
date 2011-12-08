/*===================================================

	CONTROL MASTER CODE

===================================================*/
// Global Include
#include "BBApi.h"
#include <string.h>
#include <stdlib.h>

//Parent Include
#include "ControlMaster.h"

//Includes
#include "Definitions.h"
#include "PluginMaster.h"
#include "WindowMaster.h"
#include "StyleMaster.h"
#include "ListMaster.h"
#include "MenuMaster.h"
#include "ConfigMaster.h"
#include "MessageMaster.h"
#include "DialogMaster.h"
#include "ModuleMaster.h"

//Local variables
// No more global controllist - prevents name conflicts by restraining names to smaller scopes: modules.
list *controltypelist = NULL;
int controlcount = 0;

list *variables_temp = NULL;

//Locally defined functions
int control_message_create(int tokencount, char *tokens[], bool ischild, module* parentmodule);
int control_message_delete(control *c, int tokencount, char *tokens[]);

void control_save_control(control *c, struct renamed_control **);
void control_child_add(control *c_parent, control *c_child);
void control_child_remove(control *c_child);
bool control_is_valid_name(char *name);
//control* control_get(const char *name);

// These were unused?
//void control_push(control *c);
//control *control_pop();

//Constant variables
const char *control_lastcontrol[] =
{
"@BBInterface Control Create Button LastControl",
"@BBInterface Control SetWindowProperty LastControl X 10",
"@BBInterface Control SetWindowProperty LastControl Y 10",
"@BBInterface Control SetWindowProperty LastControl Width 180",
"@BBInterface Control SetWindowProperty LastControl Height 50",
"@BBInterface Control SetAgent LastControl Caption StaticText \"About This Control\"",
"@BBInterface Control SetAgent LastControl MouseUp Bro@m \"@BBInterface Plugin About LastControl\"",
NULL
};

//Access to modules
extern list *modulelist;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//control_startup
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int control_startup()
{
	controlcount = 0;

	controltypelist = list_create();

	//No errors
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//control_shutdown
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int control_shutdown()
{
	listnode *ln;

	//Destroy all controls
	// NOTE: Could be merged with module code. Since it's important that control types are registered/unregistered before manipulating controls.

	//Destroy all controltypes
	dolist (ln, controltypelist)
		control_unregistertype((controltype *) ln->value);

	//Destroy the lists themselves
	list_destroy(controltypelist);

	//No errors
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//control_create
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int control_create(char *controlname, char *controltypename, char *controlparentname, bool include_parent, module* parentmodule)
{
/*	char buffer[1000];
	sprintf(buffer,"control_create\nControl: %s\nParent: %s\nDefault module: %s",name,parentmodule->name);
	MessageBox(NULL, buffer, szAppName, MB_OK|MB_SYSTEMMODAL);*/

	//Variables
	control *controlpointer;
	control *controlpointer_parent = NULL;

	//Check name lenght
	if (strlen(controlname) >= 64)
	{
		return 1;
	}

	//If the parent exists, find it!
	if (include_parent)
	{
		controlpointer_parent = control_get(controlparentname, parentmodule);
		if (!controlpointer_parent) return 1;
	}

	//Create a new control
	controlpointer = new control;

	//Update controlcount
	controlcount++;

	//Set default pointers
	controlpointer->windowptr = NULL;
	controlpointer->controltypeptr = NULL;
	controlpointer->parentptr = controlpointer_parent;
	controlpointer->moduleptr = parentmodule;
	controlpointer->firstchild = NULL;
	controlpointer->lastchild = NULL;
	controlpointer->mychildnode = NULL;
	strcpy(controlpointer->controlname, controlname);

	//Identify & set the controltype
	controlpointer->controltypeptr = (controltype *) list_lookup(controltypelist, controltypename);
	if (controlpointer->controltypeptr == NULL)
	{
		control_destroy(controlpointer, false, false);
		return 1;
	}

	//Type-specific creation details
	(controlpointer->controltypeptr->func_create)(controlpointer);

	//Add to the list of controls
	if (list_add(parentmodule->controllist, controlname, (void *) controlpointer, NULL) )
	{
		control_destroy(controlpointer, false, false);
		return 1;
	}
 
	//If a parent, add to the list of parents and control's list of children
	if (!controlpointer->parentptr)
	{ 
		// Add it to the currently active module
		list_add(parentmodule->controllist_parentsonly, controlname, (void *) controlpointer, NULL);
	}
	//Otherwise, let the parent know it's there
	else
	{
		control_child_add(controlpointer_parent, controlpointer);
	}

	//Create the window
	if (window_create(controlpointer))
	{
		control_destroy(controlpointer, true, false);
		return 1;
	}

	char buffer[200];
	sprintf(buffer,"%s:%s",parentmodule->name,controlname);
	variables_set(false, "LastControl", buffer);

	//No errors
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//control_destroy
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int control_destroy(control *c, bool remove_from_list, bool save_last)
{
	//dbg_printf("destroying: %s", c->controlname);

	//If we have children - destroy them first!
	controlchild *currentchild = c->firstchild;
	controlchild *tempchild;
	while (currentchild)
	{
		tempchild = currentchild->nextchildptr;
		control_destroy(currentchild->controlptr, remove_from_list, save_last);
		currentchild = tempchild;
	}

	//Type-specific destroy
	if (c->controltypeptr) (c->controltypeptr->func_destroy)(c);

	//Remove it from the list, if we care
	if (remove_from_list)
	{
		list_remove(c->moduleptr->controllist, c->controlname);
		if (!c->parentptr)
		{
			list_remove(c->moduleptr->controllist_parentsonly, c->controlname);
		}
	}

	//Remove the child pointer
	control_child_remove(c);

	//Destroy the control itself
	delete c;

	//Update controlcount 
	controlcount--;

	//Save the last control if there are none left, and we care
	if (save_last && controlcount == 0)
	{
		control_checklast();
	}

	//No errors
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//control_event
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int control_rename(control *c, char *newname)
{
	//Check the name to make sure it is valid
	if (!control_is_valid_name(newname)) return 1;

	//Rename the control if possible
	if (list_rename(c->moduleptr->controllist, c->controlname, newname)) return 1;

	//Rename it in the parents list, if it's there
	if (!c->parentptr) {
		list_rename(c->moduleptr->controllist_parentsonly, c->controlname, newname);
	}

	//Change the name
	strcpy(c->controlname, newname);

	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//control_event
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
LRESULT control_event(control *c, HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{   
	return (c->controltypeptr->func_event)(c, hwnd, msg, wParam, lParam);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//control_set_pluginproperty
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int control_set_pluginproperty(control *c, int tokencount, char *tokens[], module* caller)
{
	for (int i = 4; i < tokencount; i++)
		tokens[i-1] = tokens[i];
	return plugin_message(tokencount-1, tokens, false, caller);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//control_message_saveas
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int control_saveas_control(control *c, int tokencount, char **tokens)
{
	if (5 != tokencount) return 1;

	char *filename = tokens[4];

	//If the browse option is chosen
	if (!stricmp(filename, "*browse*"))
	{       
		filename = dialog_file(szFilterScript, "Save This Control As", ".rc", config_path_plugin, true);
		if (!filename)
		{
			//message_override = true;
			return 2;
		}
	}

	config_file_out = config_open(filename, "wt");
	if (NULL == config_file_out) return 1;

	control_save_control(c, NULL);
	fclose(config_file_out);
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// oops, what's that :)

struct renamed_control
{
	struct renamed_control *next;
	control *c;
	char old_name[80];
};

int control_clone(control *c)
{
	static char tmp_file[] = "BBI_TEMPFILE.$$$";
	config_file_out = config_open(tmp_file, "wt");
	if (NULL == config_file_out) return 1;

	c->windowptr->x += 3;
	c->windowptr->y += 2;

	struct renamed_control *RC = NULL;
	control_save_control(c, &RC);
	fclose(config_file_out);
	while (RC)
	{
		strcpy(RC->c->controlname, RC->old_name);
		struct renamed_control *RC2 = RC->next;
		delete RC; RC = RC2;
	}
	c->windowptr->x -= 3;
	c->windowptr->y -= 2;

	config_load(tmp_file, NULL);
	config_delete(tmp_file);
	return 0;
}

bool lookup_renamed_controls(struct renamed_control *RC, char *new_name)
{
	while (RC)
	{
		if (0 == stricmp(new_name, RC->c->controlname))
			break;
		RC = RC->next;
	}
	return NULL != RC;
}

void replace_name(control *c, struct renamed_control ** p_renamed_list)
{
	char new_name[80];
	strcpy(new_name, c->controlname);
	char *e;
	for (e = strchr(new_name, 0); e > new_name && e[-1] >= '0' && e[-1]<='9'; e--);
	int number = *e ? 1 + atoi(e) : 1;
	do sprintf(e, "%d", number++);
	while (list_lookup(c->moduleptr->controllist, new_name) || lookup_renamed_controls(*p_renamed_list, new_name));

	struct renamed_control *p_renamed_node = new struct renamed_control;
	strcpy(p_renamed_node->old_name, c->controlname);
	strcpy(c->controlname, new_name);
	p_renamed_node->c = c;

	p_renamed_node->next = *p_renamed_list;
	*p_renamed_list = p_renamed_node;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//control_can_be_assigned_to
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bool control_can_be_assigned_to(control *c, module* m)
{
	if (list_lookup(m->controllist,c->controlname)) return false;

	for	(	controlchild *currentchild = c->firstchild;
			currentchild;
			currentchild = currentchild->nextchildptr
		) if (list_lookup(m->controllist,currentchild->controlptr->controlname)) return false;

	return true;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//control_assign_to_module
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int control_assign_to_module(control* c, const char* modulename)
{
	module* m = (module *) list_lookup(modulelist, modulename);
	if (!m) return 1;
	if (!control_can_be_assigned_to(c,m)) return 1;
	//Re-assign the control itself
	if ( !list_add(m->controllist_parentsonly, c->controlname, (void *) c, NULL))
	{
		list_remove(globalmodule.controllist_parentsonly, c->controlname); //Can only be a parent
	}
	if ( !list_add(m->controllist, c->controlname, (void *) c, NULL) )
	{
		list_remove(globalmodule.controllist, c->controlname); //Should only be in the global module
		c->moduleptr = m;

		//Re-assign the children as well.
		controlchild *currentchild = c->firstchild;
		controlchild *tempchild;
		while (currentchild)
		{
			tempchild = currentchild->nextchildptr;
			
			if ( !list_add(m->controllist, currentchild->controlptr->controlname, (void *) currentchild->controlptr, NULL))
			{
				list_remove(globalmodule.controllist, currentchild->controlptr->controlname);
				currentchild->controlptr->moduleptr = m;
			}

			currentchild = tempchild;
		}
	}
	
	return 0;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//control_detach_from_module
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int control_detach_from_module(control* c)
{
	if (!c->moduleptr || c->moduleptr == &globalmodule) return 1;
	if (!control_can_be_assigned_to(c,&globalmodule)) return 1;
	if (!c->parentptr && !list_remove(c->moduleptr->controllist_parentsonly, c->controlname))
	{
		list_add(globalmodule.controllist_parentsonly, c->controlname, (void *) c, NULL);
	}
	if (!list_remove(c->moduleptr->controllist, c->controlname))
	{
		list_add(globalmodule.controllist, c->controlname, (void *) c, NULL);
		c->moduleptr = &globalmodule;

		controlchild *currentchild = c->firstchild;
		controlchild *tempchild;
		while (currentchild)
		{
			tempchild = currentchild->nextchildptr;

			if ( !list_remove(c->moduleptr->controllist, currentchild->controlptr->controlname) )
			{
				list_add(globalmodule.controllist, currentchild->controlptr->controlname, (void *) currentchild->controlptr, NULL);
				currentchild->controlptr->moduleptr = &globalmodule;
			}

			currentchild = tempchild;
		}

		return 0;
	}
	return 1;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//control_message
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int control_message(int tokencount, char *tokens[], bool from_core, module* caller)
{
/*	//DEBUG STUFF
	char buffer[1000];
	sprintf(buffer,"control_message\nmodule: %s",name,caller->name);
	MessageBox(NULL, buffer, szAppName, MB_OK|MB_SYSTEMMODAL);*/
	//At this point, we know arguments 0 and 1 are
	//@BBInterface and Control

	//Variables
	control *c;

	//We need at least 4 arguments here
	if (tokencount < 4) return 1;
	
	//The create scenario, we don't need to find a control
	if (!stricmp(tokens[2], szBActionCreate))
		return control_message_create(tokencount, tokens, false, caller);
	
	//Find the control
	c = control_get(tokens[3], caller);
	if (!c) return 1;

	//Create child only if allowed
	if (!stricmp(tokens[2], szBActionCreateChild))
		return control_message_create(tokencount, tokens, c->controltypeptr->can_parent, c->moduleptr);

	// "Delete"
	if (!stricmp(tokens[2], szBActionDelete))
		return control_message_delete(c, tokencount, tokens);

	// "Save This Control As..."
	if (!stricmp(tokens[2], szBActionSaveAs))
		return control_saveas_control(c, tokencount, tokens);

	// "Clone"
	if (!stricmp(tokens[2], "Clone"))
		return control_clone(c);

	int result;

	// "Set Window Property"
	if (!stricmp(tokens[2], szBActionSetWindowProperty))
		result = window_message_setproperty(c, tokencount, tokens);
	else
	//If it's rename, try to rename it
	if (tokencount == 5 && !stricmp(tokens[2], szBActionRename))
		result = control_rename(c, tokens[4]);
	else
	if (tokencount == 5 && !stricmp(tokens[2], szBActionAssignToModule))
		result = control_assign_to_module(c, tokens[4]);
	else
	if (!stricmp(tokens[2], szBActionDetachFromModule))
		result = control_detach_from_module(c);
	else
	//If it's a plugin property message, reroute it to the plugin_message handler
	if (!stricmp(tokens[2], szBActionSetPluginProperty))
		result = control_set_pluginproperty(c, tokencount, tokens, caller);
	else
	//Pass it up to the control, since we can't handle it here
		result = (c->controltypeptr->func_message)(c, tokencount, tokens);

	// if "ok" and not interpreting from file, update the menu
	if (!result && from_core)
		menu_control (c, false);

	return result;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//control_notify
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void control_notify(control *c, int notifytype, void *messagedata)
{
	(c->controltypeptr->func_notify)(c, notifytype, messagedata);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void control_pluginsvisible(bool arevisible)
{
	//Notify the window manager that plugins were toggled
	window_pluginsvisible(arevisible);

	listnode *ln, *ln2;
	dolist (ln2, globalmodule.controllist_parentsonly)
	{
		//Get the control
		control *c = (control *) ln2->value;
		//Tell it that plugins were toggled
		window_update(c->windowptr, false, false, true, false);
	}
	dolist (ln, modulelist)
	{       
		module *m = (module *) ln->value;
		if (m->enabled) // don't bother with disabled modules
		dolist (ln2, m->controllist_parentsonly)
		{
			//Get the control
			control *c = (control *) ln2->value;

			//Tell it that plugins were toggled
			window_update(c->windowptr, false, false, true, false);
		}
	}
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void controls_clickraise(void)
{
	//Variables
	listnode *ln, *ln2;
	dolist (ln2, globalmodule.controllist_parentsonly)
	{       
		//Get the control
		control *c = (control *) ln2->value;
		PostMessage(c->windowptr->hwnd, BB_DESKCLICK, 0, 0);
	}
	dolist (ln, modulelist)
	{       
		module *m = (module *) ln->value;
		if (m->enabled) // don't bother with disabled modules
		dolist (ln2, m->controllist_parentsonly)
		{       
			//Get the control
			control *c = (control *) ln2->value;
			PostMessage(c->windowptr->hwnd, BB_DESKCLICK, 0, 0);
		}
	}
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//control_invalidate
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void control_invalidate(void)
{
	//Variables
	listnode *ln, *ln2;
	dolist (ln2, globalmodule.controllist)
	{       
		//Get the control
		control *c = (control *) ln2->value;
		style_draw_invalidate(c);
	}
	dolist (ln, modulelist)
	{       
		module *m = (module *) ln->value;
		if (m->enabled) // don't bother with disabled modules
		dolist (ln2, m->controllist)
		{       
			//Get the control
			control *c = (control *) ln2->value;
			style_draw_invalidate(c);
		}
	}
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//control_menu_create
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void control_menu_create(Menu *m, control *c, bool createchild)
{
	//Create the list
	listnode *ln;
	dolist (ln, controltypelist)
	{
		controltype *ct = (controltype *) ln->value;
		if (createchild && ct->can_child)
		{
			make_menuitem_cmd(m, ln->key, config_getfull_control_create_child(c, ct));
		}
		else if (!createchild && ct->can_parentless)
		{
			make_menuitem_cmd(m, ln->key, config_get_control_create(ct));
		}
	}
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//control_menu_settings
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bool is_descendant (control *c, control *parent)
{
	do if (parent == c) return true;
	while (NULL != (c = c->parentptr));
	return false;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void control_menu_settings(Menu *m, control *c)
{
	//Variables
	listnode *ln, *ln2; int n; Menu *sub, *sub2; const bool btrue = true;
	const char *sub_title;

	make_menuitem_cmd(m, "Clone This Control", config_getfull_control_clone(c));
	make_menuitem_cmd(m, "Save This Control As...", config_getfull_control_saveas(c, "*Browse*"));

	n = 0; sub = make_menu("Parent Control", c);

	// ------- change child status of controls --------
	if (c->controltypeptr->can_parentless && c->parentptr)
	{
		sub_title = "Detach From";
		++n, make_menuitem_bol(sub, c->parentptr->controlname, config_getfull_control_setwindowprop_s(c, "Detach"), true);
	}
	else
	{
		sub_title = "Attach To";
		if (c->controltypeptr->can_child && NULL == c->parentptr && false == c->windowptr->is_slitted)
		{
			sub2 = make_menu("Global", c);
			n = 0;
			dolist (ln2, globalmodule.controllist)
			{
				control *pspec = (control *) ln2->value;
				if (pspec->controltypeptr->can_parent && !is_descendant(pspec, c))
				{
					char buffer[110]; sprintf(buffer, "AttachTo %s:%s", pspec->moduleptr->name, pspec->controlname);
					++n, make_menuitem_cmd(sub2, pspec->controlname, config_getfull_control_setwindowprop_s(c, buffer));
				}
			}
			if (0 == n) make_menuitem_nop(sub2, "Not Available");
			make_submenu_item(sub, "Global", sub2);
			// add the rest of the modules as well
			dolist (ln,modulelist)
			{
				module *mod = (module*) ln->value;
				if (mod->enabled)
				{
					sub2 = make_menu(mod->name, c);
					// fill contents of submenu for modules.
					n = 0;
					dolist (ln2, mod->controllist)
					{
						control *pspec = (control *) ln2->value;
						if (pspec->controltypeptr->can_parent && !is_descendant(pspec, c))
						{
							char buffer[110]; sprintf(buffer, "AttachTo %s:%s", pspec->moduleptr->name, pspec->controlname);
							++n, make_menuitem_cmd(sub2, pspec->controlname, config_getfull_control_setwindowprop_s(c, buffer));
						}
					}
					if (0 == n) make_menuitem_nop(sub2, "Not Available");
					make_submenu_item(sub, mod->name, sub2);
				}
			}
		}
	}
	make_submenu_item(m, sub_title, sub);

	// ------- change group (module) status -------
	if (!c->parentptr)
		if (c->moduleptr != &globalmodule)
		{	
			sub = make_menu("Detach From", c);
			make_menuitem_cmd(sub, c->moduleptr->name, config_getfull_control_detachfrommodule(c));
			make_submenu_item(m, "Detach From Module", sub);
		} else
		{
			n = 0; sub = make_menu("Assign To", c);
			dolist (ln, modulelist)
			{
				module* m = (module*) ln->value;
				if (m->enabled) //Hide disabled modules
				{
					++n; make_menuitem_cmd(sub, m->name, config_getfull_control_assigntomodule(c, m));
				}
			}
			if (0 == n) make_menuitem_nop(sub, "Not Available");
			make_submenu_item(m, "Assign To Module", sub);
		}

	// ------- recover hidden controls --------
	sub = make_menu(sub_title = "Unhide Control", c);

	sub2 = make_menu("Global", c);
	n = 0;
	dolist (ln2, globalmodule.controllist)
	{
		control *cspec = (control *) ln2->value;
		if (false == cspec->windowptr->is_visible)
			++n, make_menuitem_cmd(sub2, cspec->controlname, config_getfull_control_setwindowprop_b(cspec, szWPisvisible, &btrue));
	}
	if (0 == n) make_menuitem_nop(sub2, "None Hidden");
	make_submenu_item(sub, "Global", sub2);
	dolist (ln, modulelist)
	{
		module *mod = (module *) ln->value;
		if (mod->enabled)
		{
			sub2 = make_menu(mod->name, c);
			n = 0;
			dolist (ln2, mod->controllist)
			{
				control *cspec = (control *) ln2->value;
				if (false == cspec->windowptr->is_visible)
					++n, make_menuitem_cmd(sub2, cspec->controlname, config_getfull_control_setwindowprop_b(cspec, szWPisvisible, &btrue));
			}
			if (0 == n) make_menuitem_nop(sub2, "None Hidden");
			make_submenu_item(sub, mod->name, sub2);
		}
	}

	make_submenu_item(m, sub_title, sub);
	// -------

	make_menuitem_nop(m, NULL);
	make_menuitem_cmd(m, "Delete This Control", config_getfull_control_delete(c));
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bool control_make_childof(control *c, const char *parentname)
{
	control *pc = control_get(parentname);
	if (NULL == pc
	 || false == c->controltypeptr->can_child
	 || c->parentptr
	 || c->windowptr->is_slitted
	 || false == pc->controltypeptr->can_parent
	 || is_descendant(pc, c)
	 ) return false;

	control_detach_from_module(c); //assign to the same module as its new parent
	control_assign_to_module(c,pc->moduleptr->name);
	list_remove(c->moduleptr->controllist_parentsonly, c->controlname);

	c->parentptr = pc;
	control_child_add(pc, c);
	return true;
}

bool control_make_parentless(control *c)
{
	if (false == c->controltypeptr->can_parentless || NULL == c->parentptr)
		return false;

	control_child_remove(c);
	c->parentptr = NULL;
	list_add(c->moduleptr->controllist_parentsonly, c->controlname, (void *)c, NULL);

	return true;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//control_menu_context
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void control_menu_context(Menu *m, control *c)
{   
	//Type-specific menu
	(c->controltypeptr->func_menu_context)(m, c);
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//control_save
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void control_save()
{
	//Variables
	listnode *ln;
	controltype *ct;

	//Save all agent type specific data
	dolist (ln, controltypelist)
	{
		ct = (controltype *) ln->value;
		(ct->func_notifytype)(NOTIFY_SAVE_CONTROLTYPE, NULL);
	}

	//Save all controls
	dolist (ln, globalmodule.controllist_parentsonly)
	{       
		//Get the control
		control *c = (control *) ln->value;
		control_save_control(c, NULL);
	}
}

//##################################################
//control_save_control
//##################################################
void control_save_control(control *c, struct renamed_control ** p_renamed_list)
{
	// for "Clone control"
	if (p_renamed_list) replace_name(c, p_renamed_list);

	//Save the control's existance
	if (c->parentptr)
	{
		config_printf("!---- %s::%s ----", c->parentptr->controlname, c->controlname);
		config_write(config_get_control_create_child_named(c->parentptr, c->controltypeptr, c));
	}
	else
	{
		config_printf("!---- %s ----", c->controlname);
		config_write(config_get_control_create_named(c->controltypeptr, c));
	}

	//Save the window details
	window_save_control(c);

	//Save the control type-specific details (including agents)
	(c->controltypeptr->func_notify)(c, NOTIFY_SAVE_CONTROL, NULL);

	//Save the control's children
	controlchild *currentchild = c->firstchild;
	controlchild *tempchild;
	while (currentchild)
	{
		tempchild = currentchild->nextchildptr;
		//Save the child
		control_save_control(currentchild->controlptr, p_renamed_list);
		currentchild = tempchild;
	}
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//control_registertype
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void control_checklast()
{
	//If controls exist, or zero is allowed, we don't care
	if (controlcount > 0 || plugin_zerocontrolsallowed) return;

	//If this is the last control, create a new one!
	for (const char ** p = control_lastcontrol; *p; p++)
		message_interpret(*p, false, &globalmodule);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//control_registertype
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void control_registertype(
	char    *controltypename,
	bool    can_parentless,
	bool    can_parent,
	bool    can_child,
	char    id,
	int     (*func_create)(control *c),
	int     (*func_destroy)(control *c),
	LRESULT (*func_event)(control *c, HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam),
	void    (*func_notify)(control *c, int notifytype, void *messagedata),
	int     (*func_message)(control *c, int tokencount, char *tokens[]),
	void*   (*func_getdata)(control *c, int datatype),
	bool    (*func_getstringvalue)(control *c, char *buffer, char *propertyname),
	void    (*func_menu_context)(Menu *m, control *c),
	void    (*func_notifytype)(int notifytype, void *messagedata)
	)
{
	//Error conditions
	if (strlen(controltypename) >= 64) return;

	controltype *controltypepointer;
	controltypepointer = new controltype;

	strcpy(controltypepointer->controltypename, controltypename);
	
	controltypepointer->can_parentless = can_parentless;
	controltypepointer->can_parent = can_parent;
	controltypepointer->can_child = can_child;
	controltypepointer->id = id;
	controltypepointer->func_create = func_create;
	controltypepointer->func_destroy = func_destroy;
	controltypepointer->func_event = func_event;
	controltypepointer->func_notify = func_notify;
	controltypepointer->func_message = func_message;
	controltypepointer->func_getdata = func_getdata;
	controltypepointer->func_getstringvalue = func_getstringvalue;
	controltypepointer->func_menu_context = func_menu_context;
	controltypepointer->func_notifytype = func_notifytype;

	list_add(controltypelist, controltypename, (void *) controltypepointer, NULL);

	return;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//control_unregistertype
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void control_unregistertype(controltype *ct)
{
	//Delete the type, that's all
	delete ct;
}

//##################################################
//control_message_create
//##################################################
int control_message_create(int tokencount, char *tokens[], bool ischild, module* defaultmodule)
{
	//Variables
	int index_newname, index_targettype;

	//Find out the indexes
	if (ischild)
	{   index_newname = 5; index_targettype = 4; }
	else
	{   index_newname = 4; index_targettype = 3; }

	//Determine target module and controltype
	char* mname = new_string(tokens[index_targettype]); //module name
	char* tname = strchr(mname,':'); //type name
	module* parentmodule;
	if (tname)
	{
		*tname++ = 0;
		if (*mname) parentmodule = (module*) list_lookup(modulelist,mname);
		else parentmodule = &globalmodule;
		if (!parentmodule) return 0; //module not found
	}
	else // assume default module, only typename
	{
		tname = mname;
		parentmodule = defaultmodule;
	}

	//Figure out the name of the control
	if (index_newname == tokencount)
	{
		//Find a new unique name for the new control
		//This is dirty - possible better solution later!
		char tempname[64];
		int number = 1;
		do
		{
			sprintf(tempname, "%s%d", tname, number);
			number++;

		} while (list_lookup(parentmodule->controllist, tempname));
		
		//Create the control, return the result
		if (!ischild && 0 == control_create(tempname, tname, "", false, parentmodule))
			return 0;
		else
		if (ischild && 0 == control_create(tempname, tname, tokens[3], true, parentmodule))
			return 0;
	}
	else
	if (index_newname + 1 == tokencount)
	{
		//We were given a name - check it for proper encoding
		if (!control_is_valid_name(tokens[index_newname]))
			return 1;

		//Create the control, return the result
		if (!ischild && 0 == control_create(tokens[4], tname, "", false, parentmodule))
			return 0;
		else
		if (ischild && 0 == control_create(tokens[5], tname, tokens[3], true, parentmodule))
			return 0;
	}
	return 1;
}

//##################################################
//control_message_delete
//##################################################
int control_message_delete(control *c, int tokencount, char *tokens[])
{
	if (tokencount == 4 && !strcmp(tokens[2], "Delete"))
	{
		control_destroy(c, true, true);
		return 0;       
	}
	return 1;
}

//##################################################
//control_child_add
//##################################################
void control_child_add(control *c_parent, control *c_child)
{
	//Make a new controlchild node
	controlchild *child = new controlchild;
	child->controlptr = c_child;
	child->nextchildptr = NULL;
	child->prevchildptr = c_parent->lastchild;

	//Add it to the list at the end
	if (!c_parent->firstchild) c_parent->firstchild = child;
	if (c_parent->lastchild) c_parent->lastchild->nextchildptr = child;
	c_parent->lastchild = child;

	//Let the control know what it's childpointer is
	c_child->mychildnode = child;
}

//##################################################
//control_child_remove
//##################################################
void control_child_remove(control *c_child)
{
	//Get the child node to delete
	controlchild *child = c_child->mychildnode;
	if (!child) return;
	c_child->mychildnode = NULL;

	//Before removing, see if it's the first or the last
	if (!child->nextchildptr) c_child->parentptr->lastchild = child->prevchildptr;
	else child->nextchildptr->prevchildptr = child->prevchildptr;
	if (!child->prevchildptr) c_child->parentptr->firstchild = child->nextchildptr;
	else child->prevchildptr->nextchildptr = child->nextchildptr;

	//Actually delete it
	delete child;
}

//##################################################
//control_child_remove
//##################################################
bool control_is_valid_name(char *name)
{
	//Length check
	if (strlen(name) >= 64) return false;

	//It can only contain letters and numbers and underscores
	bool is_error = false;
	int index = 0;
	while (!is_error && name[index] != '\0')
	{
		char letter = name[index];
		if (!(letter >= 'a' && letter <= 'z')
			&& !(letter >= 'A' && letter <= 'Z')
			&& !(letter >= '0' && letter <= '9')
			&& !(letter == '_')) is_error = true;
		index++;
	}

	return !is_error;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//control_get
// Returns controls with the fully qualified name "Module:Control" or NULL if not found.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
control* control_get(const char *name, module* deflt)
{
	char* mname = new_string(name);
	char* cname = strchr(mname,':');
	module* m;
	if (cname)
	{
		*cname++ = 0;
		if (*mname) m = (module*) list_lookup(modulelist,mname);
		else m = &globalmodule;
		if (!m) return NULL;
	}
	else // assume default module, only controlname
	{
		cname = mname;
		m = deflt;
	}
	return (control*) list_lookup(m->controllist,cname);
}

//##################################################
//control_getstringdata
//##################################################
bool control_getstringdata(control *c, char *buffer, char *propertyname)
{
	if (c == NULL) return NULL;
	return c->controltypeptr->func_getstringvalue(c, buffer, propertyname);
}

//##################################################

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//variables_startup
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void variables_startup(void)
{
	variables_temp = list_create();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//variables_shutdown
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void variables_shutdown(void)
{
	if (variables_temp)
	{
		listnode *ln;
		dolist (ln, variables_temp) free_string((char **)&ln->value);
		list_destroy(variables_temp);
		variables_temp = NULL;
	}

	free_string(&plugin_desktop_drop_command);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//variables_save
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void variables_save(void)
{
	if (!globalmodule.variables->first) return;
	config_printf("!---- Global variables ----");
	listnode *ln;
	dolist(ln,globalmodule.variables)
		config_write(config_get_variable_set_static(ln));
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//variables_set
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void variables_set(bool is_static, const char *key, const char *val, module* defmodule)
{
	char *old_val;
	if (is_static)
	{
		char* mname = new_string(key);
		char* vname = strrchr(mname,':');
		module* m;
		if (vname)
		{
			*vname++ = 0;
			if (*mname)
				m = (module*) list_lookup(modulelist,mname);
			else
				m = &globalmodule;
			if (!m) return;
		}
		else // assume default module, only controlname
		{
			vname = mname;
			m = defmodule;
		}
		list_add(m->variables, vname, new_string(val), (void**)&old_val);
		free_string(&old_val);
	}
	else
	{
		const char *p = strrchr(key,':');
		list_add(variables_temp, p ? p+1 : key, new_string(val), (void**)&old_val);
		free_string(&old_val);
	}
}

void variables_set(bool is_static, const char *key, int val, module* defmodule)
{
	char buffer[20];
	sprintf(buffer, "%d", val);
	variables_set(is_static, key, buffer, defmodule);
}
/*
void variables_set_local(module *m, const char *key, const char *val)
{
	if (!m) return; // m->variables should never be null, unless I screwed up somewhere.
	char *old_val;
	list_add(m->variables, key, new_string(val), (void**)&old_val);
	free_string(&old_val);
}

void variables_set_local(module *m, const char *key, int val)
{
	char buffer[20];
	sprintf(buffer, "%d", val);
	variables_set_local(m, key, buffer);
}

*/
const char *variables_get(const char *key, const char *deflt, module* defmodule)
{
	char* mname = new_string(key);
	char* vname = strrchr(mname,':');
	module* m;
	if (vname)
	{
		*vname++ = 0;
		if (*mname) m = (module*) list_lookup(modulelist,mname);
		else m = &globalmodule;
		if (!m) return NULL;
	}
	else // assume default module, only controlname
	{
		vname = mname;
		m = defmodule;
	}
	char* s = (char*) list_lookup(m->variables,vname);
	if (s) return s;

	s = (char*) list_lookup(variables_temp, vname);
	if (s) return s;
	
	return deflt;
}


