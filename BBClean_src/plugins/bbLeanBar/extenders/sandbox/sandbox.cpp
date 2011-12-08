/*
	Extender Sandbox for bbLeanBar 1.16 (bbClean)
	
	(c) 2007, Noccy
	
	Released under GPL2+
	
	Runs some basic tests on a extender to make sure it works as intended. Was
	actually written to test the LoadLibrary calls. Commented for those who
	are trying to learn anything from it. Like for example dynamic lodaing
	of functions from a DLL ;)
*/

#include "../barextender.h"
#include <iostream>

using namespace std;

int main(int argc, const char* argv[]) {

	// We declare hPlugin of the type HINSTANCE. This is where our plugin handle
	// will be loaded.
	HINSTANCE hPlugin;

	// Some Swedish Chef for y'all!
	cout << "Extender Sandbox Bork Bork Bork!\n\n";

	// Check the number of parameters. We want at least one of those here. Or
	// we will whine at the user.
	if (argc == 1) 
	{
		// No parameters. Show some basic help.
		cout << "Use like sandbox <extender.dll>\n";
	} 
	else 
	{
		// Display information about the specified library.
		cout << "Library. . . : " << argv[1] << "\n";
		
		// Here we actually attempt to load the library. A return value of NULL 
		// indicates that things did not go as expected.
		hPlugin = LoadLibrary(argv[1]);
		if (hPlugin == NULL) 
		{
			// So, we inform the user of this tragedy.
			cout << "Error: Failed to load the DLL!\n";
		} 
		else 
		{
			// Okay, We have a handle to the loaded library now, so we declare a variable
			// getExtenderInfoFunc as the type DLL_getExtenderInfo. We then find the proc
			// entry point of the "getExtenderInfo" function and shove it in there.
			DLL_getExtenderInfo getExtenderInfoFunc;
			getExtenderInfoFunc = (DLL_getExtenderInfo)GetProcAddress(hPlugin,"getExtenderInfo");
			if (getExtenderInfoFunc == NULL) 
			{
				// Again, a NULL is not a good thing to get. If we got it here, it means that
				// the procedure entry point could not be found. So tell the user that.
				cout << "Error: Couldn't find procedure entry point of getExtenderInfo!\n";
			} 
			else 
			{
				// Since we were successful, go on with polling the information from the
				// function we just imported. 
				cout << "Name . . . . : " << getExtenderInfoFunc(EIF_NAME) << "\n";
				cout << "Item Key . . : " << getExtenderInfoFunc(EIF_KEY) << "\n";
				cout << "Multiuse . . : " << getExtenderInfoFunc(EIF_MULTIUSE) << "\n";
				cout << "Dynamic. . . : " << getExtenderInfoFunc(EIF_DYNAMICWIDTH) << "\n";
				cout << "\n";
				cout << "User supplied init string is " << ((argc==2)?"not present":argv[2]) << "\n";
				cout << "\n";
				
				// Time for some actual testing. We start by checking how the plugin reacts to
				// requesting an unknown extender info id. Plugins compiled against one of the
				// sample plugins will handle this by returning NULL. Or well, at least they
				// *should* do it ;)
				try 
				{
					cout << "Testing unknown info query...";
					LPCSTR test = getExtenderInfoFunc(255);
					cout << "Success\n";
				} 
				catch(...) 
				{
					cout << "Failed!\n";
				}
				
				// Time to test the actual extender code. Once again we declare a var of the type
				// we desire, and pull the proc entry point into it.
				try {
					cout << "Testing createExtender...";
					DLL_createExtender createExtenderFunc = (DLL_createExtender)GetProcAddress(hPlugin,"createExtender");
					
					// If we got a escond parameter on the command line, we pass it as the init string,
					// and if not we just pass an empty string here.
					HWND test2 = createExtenderFunc(argc==3?argv[2]:"");
					if (test2 == NULL) 
					{
						// Not a good thing. Plugins should return their extenders HWND from the 
						// createExtender function, so that the leanbar can take this handle and
						// make it a child window of itself using SetParent.
						cout << "Failed: No HWND returned\n";
					} 
					else 
					{
						// This extender did exactly that. Returning a HWND is a good thing. However,
						// since we're just testing, we'll just go ahead and destroy it again. Same
						// importing done here, and we pass the HWND we got from the create call.
						cout << "Success\n";
						
						cout << "Testing destroyExtender...";
						DLL_destroyExtender destroyExtenderFunc = (DLL_destroyExtender)GetProcAddress(hPlugin,"destroyExtender");
						destroyExtenderFunc(test2);
						cout << "Success\n";
					}
						
				} 
				catch(...) 
				{
					// Not good. We got an exception somewhere along the line. Extenders should
					// not do this. So, back to the IDE you go ;)
					cout << "Failed!\n";
				}
				
			}
			// Finally, we call FreeLibrary to free the library we loaded with LoadLibrary.
			// This is good practice.
			FreeLibrary(hPlugin);
		}
	}
}
