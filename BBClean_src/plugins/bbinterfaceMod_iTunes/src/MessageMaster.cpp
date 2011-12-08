/*===================================================

	MESSAGE MASTER CODE

===================================================*/

// Global Include
#include "BBApi.h"
#include <stdlib.h>
#include <shellapi.h>

//Includes
#include "AgentMaster.h"
#include "ControlMaster.h"
#include "MessageMaster.h"
#include "PluginMaster.h"
#include "WindowMaster.h"
#include "Definitions.h"
#include "ConfigMaster.h"
#include "ListMaster.h"

//Define these variables
const char szMessageMasterName[] = "BBInterfaceMessageMaster";
HWND message_window = NULL;

int varset_message(int tokencount, char *tokens[], bool from_core, module* caller);

const int MESSAGE_ENTITY_COUNT = 6;

int (*message_functions[MESSAGE_ENTITY_COUNT])(int tokencount, char *tokens[], bool from_core, module* caller)
		= {&control_message, &window_message, &agent_message, &plugin_message, &varset_message, &module_message};

const char *message_entitynames[MESSAGE_ENTITY_COUNT]
		= {szBEntityControl, szBEntityWindow, szBEntityAgent, szBEntityPlugin, szBEntityVarset, szBEntityModule };

//Define these functions internally
LRESULT CALLBACK message_event(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

int tokenize_message(const char * srcString, int nBuffers, char **lpszBuffers, char *buffer, module* defmodule);


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//message_startup
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int message_startup()
{
	if (window_helper_register(szMessageMasterName, &message_event)) return 1;
	if (!(message_window = window_helper_create(szMessageMasterName))) return 1;

	//No errors
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//message_shutdown
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int message_shutdown()
{
	// Destroy the hwnd...
	window_helper_destroy(message_window);
	message_window = NULL;

	//Unregister the class
	window_helper_unregister(szMessageMasterName);

	//No errors
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//message_interpret
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void message_interpret(const char *message, bool from_core, module* caller)
{
	if (!message || !(*message)) return; //If the message is invalid, or empty, don't bother
	if (caller == NULL) caller = currentmodule;

	char buffer[BBI_MAX_LINE_LENGTH];

	// Check, if the message is for us...
	if (memcmp(message, szBBroam, szBBroamLength))
	{
		//The standard BlackBox Messages
		if (!stricmp(message, "@BBShowPlugins"))
		{
			control_pluginsvisible(true);
			return;
		}
		if (!stricmp(message, "@BBHidePlugins"))
		{
			control_pluginsvisible(false);
			return;
		}
		if (!memicmp(message, "@Script",7))
		{
			char* buf = new char[strlen(message)+1]; // local buffer.
			strcpy(buf,message); //NOTE: possible alternate method would be copying out the messages one by one.
			char *start = strchr(buf,'[');
			char *end = strrchr(buf,']');
			if (start && end)
			{
				++start;
				*end = 0; //terminate string here.
				while (end = strchr(start,'|'))
				{
					*end = 0;
					while (*start == ' ') ++start; // skip whitespace
					message_interpret(start, from_core, caller);
					start = end+1;
				}
				while (*start == ' ') ++start; // skip whitespace
				message_interpret(start, from_core, caller); // interpret message after last separator character
			}
			else if (!plugin_suppresserrors)
			{
				sprintf(buffer,"Invalid @Script syntax in line:\n\n%s",buf);
				MessageBox(NULL, buffer, szAppName, MB_OK|MB_SYSTEMMODAL);
			}
			delete[] buf;
			return;
		}

		if (from_core) return;
		message = message_preprocess(strcpy(buffer, message)); //NOTE: FIX, possibly should this use the caller argument as well?
		if ('@' == message[0])
		{
			SendMessage(plugin_hwnd_blackbox, BB_BROADCAST, 0, (LPARAM)buffer);
		}
		else
		{
			// bblean only: SendMessage(plugin_hwnd_blackbox, BB_EXECUTE, 0, (LPARAM)string);
			char command[MAX_PATH], arguments[MAX_PATH], *token = command;
			BBTokenize(message, &token, 1, arguments);
			shell_exec(command, arguments);
		}
		return;
	}
	//Tokenize the string
	char *message_tokenptrs[32];
	int tokensfound = tokenize_message(message, 32, message_tokenptrs, buffer, caller);

	//Token Check - we always need at least two tokens for all purposes
	if (tokensfound < 2) return;

	//Find someone to send the message to
	int result = 1;

	for (int i = 0; i < MESSAGE_ENTITY_COUNT; i++)
	{
		if (!strcmp(message_tokenptrs[1], message_entitynames[i]))
		{   
			result = (message_functions[i])(tokensfound, message_tokenptrs, from_core, caller);
				break;
		}
	}

	if (1 == result) // 0=ok, 1=error, 2=error&dontcare
	{
		//On an error
		if (!plugin_suppresserrors) 
		{
			sprintf(buffer,
				"There was an error executing your Bro@m command:"
				"\n"
				"\n%s"
				"\n"
				"\nPossible reasons:"
				"\n - An control or agent referenced may not exist"
				"\n - The command may be malformed"				
				"\n - An error occurred while performing the requested action"
				, message
				);
			MessageBox(NULL, buffer, szAppName, MB_OK|MB_SYSTEMMODAL);
		}
	}
}

//##################################################
//message_event
//##################################################

LRESULT CALLBACK message_event(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{   
	static const int message_blackboxmessages[] = {BB_RECONFIGURE, BB_BROADCAST, BB_DESKCLICK, BB_DRAGTODESKTOP, BB_HIDEMENU, BB_TASKSUPDATE,BB_WORKSPACE,0};

	switch(msg)
	{
		case WM_CREATE:
			//Register the window to recieve BlackBox events    
			SendMessage(plugin_hwnd_blackbox, BB_REGISTERMESSAGE, (WPARAM)hwnd, (LPARAM)message_blackboxmessages);
			break;

		case WM_DESTROY:
			//Unregister to recieve BlackBox events
			SendMessage(plugin_hwnd_blackbox, BB_UNREGISTERMESSAGE, (WPARAM)hwnd, (LPARAM)message_blackboxmessages);
			break;

		case BB_DRAGTODESKTOP:
			if (NULL == plugin_desktop_drop_command) return false;
			if (!(wParam & 1))
			{
				variables_set(false,"DroppedFile", (const char *)lParam);
				message_interpret(plugin_desktop_drop_command, false, NULL);
			}
			return true;
		case BB_TASKSUPDATE:
			controls_updatetasks();
			break;
		case BBI_POSTCOMMAND:
			SendMessage(plugin_hwnd_blackbox, BB_BROADCAST, 0, lParam);
			delete (char *)lParam;
			break;

		case BB_RECONFIGURE:
			plugin_reconfigure(false);
			break;

		case BB_DESKCLICK:
			if (lParam == 0)
				controls_clickraise();
			break;

		case BB_BROADCAST:
			message_interpret((const char *)lParam, true, NULL);
			break;

		case WM_TIMER:
			KillTimer(hwnd, wParam);
			if (1 == wParam)
				config_backup(config_path_mainscript);
			break;

		default:
			return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 0;
}

//===========================================================================
int varset_message(int tokencount, char *tokens[], bool from_core, module* caller)
{
	int varnameidx; bool is_static;
	if (tokencount == 4)
		{	varnameidx = 2; is_static = false;	}
	else if (tokencount == 5 && !strcmp(tokens[2], "Static") )
		{	varnameidx = 3; is_static = true;	}
	else
		return 1;

	int i = atoi(variables_get(tokens[varnameidx],"0", caller));
	if	(	config_set_int(tokens[varnameidx+1],&i) //Try reading it as an int
		||	config_set_int_expr(tokens[varnameidx+1],&i) // Try calculating the string as an expression
		) 
		variables_set(is_static,tokens[varnameidx],i, caller);
	else variables_set(is_static, tokens[varnameidx], tokens[varnameidx+1], caller);
	return 0;
}

//===========================================================================
// Function: tokenize_message
//===========================================================================

int tokenize_message(const char * srcString, int nBuffers, char **lpszBuffers, char *buffer_ptr, module* defmodule)
{
	int tokencount = 0;
	while (tokencount < nBuffers)
	{
		const char *a, *e; char  c;
		while (0 != (c = *srcString) && (unsigned char) c <= 32 ) ++srcString;
		if (0 == c) break;

		if ('\"' == c)
		{
			a = ++srcString, e = (srcString += strlen(a));
			if (e > a && e[-1] == '\"') --e;
		}
		else
		{
			a = srcString;
			while (0 != (c = *srcString) && (unsigned char) c > ' ')
			{
				++srcString;
				//if ('$' == c) { e = strchr(srcString, c); if (e) srcString = e+1; }
			}
			e = srcString;
			while (e > a && (unsigned char) e[-1] <= 32) --e;
			if (c) ++srcString;
		}

		int len = e - a;
		extract_string(buffer_ptr, a, len);

		// Removed "buffer_ptr[0] != '@'" from the 'if'
		// dont preprocess (quoted) broams
		if (strchr(buffer_ptr, '$'))
			len = strlen(message_preprocess(buffer_ptr,defmodule));

		*lpszBuffers++ = buffer_ptr;
		buffer_ptr += len + 1;
		++tokencount;
	}
	return tokencount;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// token_check
int token_check(struct token_check *t, int *curtok, int tokencount, char *tokens[])
{
start:
	if (*curtok < tokencount)
		while (t->key)
		{
			if (0 == stricmp(t->key, tokens[*curtok]))
			{
				if (t->id > 100)
				{
					*curtok += t->args;
					t = (struct token_check*)t->id;
					goto start;
				}
				if (*curtok + t->args + 1 == tokencount)
				{
					++ *curtok;
					return t->id;
				}
			}
			t++;
		}
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


void get_property_by_name(char *targetstr, control *c, char *propname)
{
	window *w = c->windowptr;
	if (!strcmp(propname,"X"))
		sprintf(targetstr, "%d", w->x);
	else if (!strcmp(propname,"Y"))
		sprintf(targetstr, "%d", w->y);
	else if (!strcmp(propname,"Width"))
		sprintf(targetstr, "%d", w->width);
	else if (!strcmp(propname,"Height"))
		sprintf(targetstr, "%d", w->height);
	else
	{
		//If we didn't get any value from the above, see if the control can return it's own value
		bool gotvalue = c->controltypeptr->func_getstringvalue(c, targetstr, propname);
		if (gotvalue == false) sprintf(targetstr, "");
	}
	
}

char *message_preprocess(char *buffer, module* defmodule)
{
	char *start = buffer, *end = NULL;
	while (NULL != (start = strchr(start, '$')) && NULL != (end = strchr(start+1, '$')))
	{
		int varlen = ++end - start - 2;
		const char *replacement = NULL;
		if (varlen)
		{
			char expression[400]; extract_string(expression, start+1, varlen);
			if (0 == memcmp(expression, "Mouse.", 6))
			{
				POINT pt; GetCursorPos(&pt);
				switch (expression[6]) {
				case 'X': sprintf(expression, "%d", (int)pt.x-10),  replacement = expression; break;
				case 'Y': sprintf(expression, "%d", (int)pt.y-10),  replacement = expression; break;
				}
			}
			// Check if it's indirect access
			else if (expression[0] == '*')
			{
				//Indirection.
				if (char* cnameend = strchr(expression, '.')) // Refers to a property of a control.
				{
					char cname[64], propname[64];
					int cnamelen = cnameend-expression-1;
					extract_string(cname, expression+1, cnamelen);
					extract_string(propname, cnameend+1, varlen-cnamelen-1);
					if (control *c = control_get(variables_get(cname, NULL, defmodule),defmodule) )
					{
						get_property_by_name(expression, c, propname);
						replacement = expression;
					} else replacement = NULL; // No '.' in other variable names - we failed locating the control.
				}
				else
				{
					const char *key = variables_get(expression+1, NULL, defmodule);
					replacement = key ? variables_get(key,NULL, defmodule) : NULL;
				}
			}
			else
			{
				//No indirection.
				if (char* cnameend = strchr(expression, '.')) // Refers to a property of a control.
				{
					char cname[64], propname[64];
					int cnamelen = cnameend-expression;
					extract_string(cname, expression, cnamelen);
					extract_string(propname, cnameend+1, varlen-cnamelen-1);
					// Try and look up the control with the given name
					if (!strcmp(cname,"DroppedFile")) // "fake" properties of the DroppedFile
					{
						strcpy(expression, variables_get("DroppedFile", NULL, defmodule));
						if (!strcmp(propname, "Path"))
						{
							char* s = strrchr(expression, '\\');
							replacement = s ? *s = 0, expression : NULL;
						}
						else if (!strcmp(propname, "Name"))
						{
							char* s = strrchr(expression, '\\');
							char* t = strrchr(expression, '.');
							s = s ? s+1 : expression; // step s past final slash, or set to start
							if (t && (t-s > 0)) *t = 0;
							replacement = s;
						}
						else if (!strcmp(propname, "Extension"))
						{
							char* s = strrchr(expression, '.');
							replacement = s ? s+1 : NULL;
						}
						else if (!strcmp(propname, "Filename"))
						{
							char* s = strrchr(expression, '\\');
							replacement = s ? s+1 : NULL;
						}
						else replacement = NULL;
					}
					else if (control *c = control_get(cname, defmodule) )
					{
						get_property_by_name(expression, c, propname);
						replacement = expression;
					} else replacement = NULL; // No '.' in other variable names - we failed locating the control.

				} else
					replacement = variables_get(expression, NULL, defmodule);
			}

		}
		else
			replacement = "$"; // default: $$ becomes $

		if (replacement)
		{
			int newlen = strlen(replacement);
			memmove(start + newlen, end, strlen(end) + 1);
			memmove(start, replacement, newlen);
			end = start + newlen;
		}
		start = end;
	}
	return buffer;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

char *get_dragged_file(char *buffer, WPARAM wParam)
{
	HDROP hDrop = (HDROP) wParam;
	buffer[0] = 0;
	DragQueryFile(hDrop, 0, buffer, MAX_PATH);
	DragFinish(hDrop);
	return buffer;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// execute shell command + argumenent

void shell_exec(const char *command, const char *arguments, const char *workingdir)
{
	char buffer[MAX_PATH];
	if (NULL == workingdir)
	{
		workingdir = strrchr(command, '\\');
		if (workingdir)
		{
			int l = workingdir - command;
			((char*)memcpy(buffer, command, l))[l] = 0;
			workingdir = buffer;
		}
	}
	BBExecute(plugin_hwnd_blackbox, "", command, arguments, workingdir, SW_SHOWNORMAL, false);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// strdup
char *new_string(const char *s)
{
	return s ? strcpy(new char[strlen(s)+1], s) : NULL;
}

// save strfree
void free_string(char **s)
{
	if (*s)
	{
		//dbg_printf("free_str: %s", *s);
		delete *s;
		*s = NULL;
	}
}

char *extract_string(char *d, const char *s, int n)
{
	((char*)memcpy(d, s, n))[n] = 0;
	return d;
}

//get_string_index
int get_string_index (const char *key, const char **string_list)
{
	int i;
	for (i=0; *string_list; i++, string_list++)
		if (0==stricmp(key, *string_list)) return i;
	return -1;
}

// integer min/max functions
int imax(int a, int b) {
	return a>b?a:b;
}

int imin(int a, int b) {
	return a<b?a:b;
}

int iminmax(int a, int b, int c) {
	if (a<b) a=b;
	if (a>c) a=c;
	return a;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// debug
void dbg_printf (const char *fmt, ...)
{
	char buffer[4096]; va_list arg;
	va_start(arg, fmt);
	vsprintf (buffer, fmt, arg);
	OutputDebugString(buffer);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// borland dependency

#ifdef CRT_STRING
int _turboFloat = 0;
#endif

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
