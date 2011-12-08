/*===================================================

	DIALOG MASTER CODE

===================================================*/

// Global Include
#include "BBApi.h"
#include <commdlg.h>

//Includes
#include "PluginMaster.h"
#include "Definitions.h"

//Define these variables
char *dialog_filename;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//dialog_startup
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int dialog_startup()
{
	dialog_filename = new char[MAX_PATH];
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//dialog_shutdown
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int dialog_shutdown()
{
	delete dialog_filename;
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//dialog_openfile
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
char *dialog_file(const char *filter, const char *title, const char *defaultpath, const char *defaultextension, bool save)
{
	OPENFILENAME ofn;       // common dialog box structure

	// Initialize OPENFILENAME
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = NULL;
	ofn.lpstrFile = dialog_filename;
	//
	// Set lpstrFile[0] to '\0' so that GetOpenFileName does not 
	// use the contents of szFile to initialize itself.
	//
	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrFilter = filter;
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = defaultpath;
	ofn.lpstrTitle = title;
	ofn.lpstrDefExt = defaultextension;

	// Display the Open dialog box. 

	if (save)
	{
		ofn.Flags = OFN_PATHMUSTEXIST;
		if (GetSaveFileName(&ofn)) return dialog_filename;
	}
	else
	{       
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NODEREFERENCELINKS;
		if (GetOpenFileName(&ofn)) return dialog_filename;
	}

	return 0;
}


/*=================================================*/

