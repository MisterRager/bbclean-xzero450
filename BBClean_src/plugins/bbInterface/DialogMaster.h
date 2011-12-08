/*===================================================

	DIALOG MASTER HEADERS

===================================================*/

//Multiple definition prevention
#ifndef BBInterface_DialogMaster_h
#define BBInterface_DialogMaster_h

//Includes
#include "Definitions.h"

//Define these structures

//Define these functions internally
int dialog_startup();
int dialog_shutdown();
char *dialog_file(const char *filter, const char *title, const char *defaultpath, const char *defaultextension, bool save);

#endif
/*=================================================*/
