/*===================================================

	MESSAGE MASTER HEADERS

===================================================*/

//Multiple definition prevention
#ifndef BBInterface_MessageMaster_h
#define BBInterface_MessageMaster_h

//Includes
#include "AgentMaster.h"
#include "ControlMaster.h"
#include "Definitions.h"

//Global variables
//extern bool message_override;

//Define these functions internally

int message_startup();
int message_shutdown();
void message_interpret(const char *message, bool from_core, module* caller);

extern HWND message_window;
void shell_exec(const char *command, const char *arguments = NULL, const char *workingdir = NULL);
char *message_preprocess(char *buffer, module* defmodule = currentmodule);

#endif
/*=================================================*/
