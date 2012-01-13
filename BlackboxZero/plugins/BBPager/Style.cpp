#include "BBPager.h"

//===========================================================================

//struct FRAME frame;
//struct DESKTOP desktop;
//struct ACTIVEDESKTOP activeDesktop;
//struct WINDOW window;
//struct FOCUSEDWINDOW focusedWindow;

char bspath[MAX_PATH];
char stylepath[MAX_PATH];

//===========================================================================

void GetStyleSettings()
{
	char tempstring[MAX_LINE_LENGTH], tempstyle[MAX_LINE_LENGTH];

	// Get the path to the current style file from Blackbox...
	strcpy(stylepath, stylePath());

//===========================================================
// bbpager.frame: -> this is for the BBPager frame/background

	strcpy(tempstring, ReadString(bspath, "bbpager.frame:", "doesnotexist"));
	
	if (!_stricmp(tempstring, "doesnotexist")) // bbpager.frame style is NOT in BB file
	{
		strcpy(tempstring, ReadString(stylepath, "bbpager.frame:", "doesnotexist"));		
		if (!_stricmp(tempstring, "doesnotexist")) // bbpager.frame NOT in STYLE
		{
			strcpy(tempstyle, ReadString(stylepath, "toolbar:", "Flat Gradient Vertical"));
			if (frame.style) delete frame.style;
			frame.style = new StyleItem;
			ParseItem(tempstyle, frame.style);

			frame.color = ReadColor(stylepath, "toolbar.color:", "#000000");
			frame.colorTo = ReadColor(stylepath, "toolbar.colorTo:", "#FFFFFF");
		}
		else // bbpager.frame IS in STYLE
		{
			strcpy(tempstyle, ReadString(stylepath, "bbpager.frame:", "Flat Gradient Vertical"));
			if (frame.style) delete frame.style;
			frame.style = new StyleItem;
			ParseItem(tempstyle, frame.style);

			frame.color = ReadColor(stylepath, "bbpager.frame.color:", "#000000");
			frame.colorTo = ReadColor(stylepath, "bbpager.frame.colorTo:", "#FFFFFF");
		}
	}
	else // bbpager.frame style IS in BB file
	{
		strcpy(tempstyle, ReadString(bspath, "bbpager.frame:", "Flat Gradient Vertical"));
		if (frame.style) delete frame.style;
		frame.style = new StyleItem;
		ParseItem(tempstyle, frame.style);

		frame.color = ReadColor(bspath, "bbpager.frame.color:", "#000000");
		frame.colorTo = ReadColor(bspath, "bbpager.frame.colorTo:", "#FFFFFF");	
	}

//===========================================================
// bbpager.frame.borderColor: -> this is the colour for the border around BBPager

	strcpy(tempstring, ReadString(bspath, "bbpager.frame.borderColor:", "doesnotexist"));
	
	if (!_stricmp(tempstring, "doesnotexist")) // bbpager.frame.borderColor NOT in BB
	{
		strcpy(tempstring, ReadString(stylepath, "bbpager.frame.borderColor:", "doesnotexist"));
		
		if (!_stricmp(tempstring, "doesnotexist")) // bbpager.frame.borderColor NOT in STYLE
		{
			frame.borderColor = ReadColor(stylepath, "borderColor:", "#000000");
		}
		else // bbpager.frame.borderColor IS in STYLE
		{
			frame.borderColor = ReadColor(stylepath, "bbpager.frame.borderColor:", "#000000");
		}
	}
	else // bbpager.frame.borderColor IS in BB
	{
		frame.borderColor = ReadColor(bspath, "bbpager.frame.borderColor:", "#000000");
	}

//===========================================================
// font settings -> used for Desktop Numbers

	strcpy(desktop.fontFace, ReadString(stylepath, "toolbar.font:", ""));

	if (!_stricmp(desktop.fontFace, "")) 
		strcpy(desktop.fontFace, ReadString(stylepath, "*font:", "Tahoma"));

	desktop.fontSize = ReadInt(stylepath, "toolbar.fontHeight:", 666);
	if (desktop.fontSize == 666)
		desktop.fontSize = ReadInt(stylepath, "*fontHeight:", 12);

	char temp[32];
	strcpy(temp, ReadString(stylepath, "toolbar.fontWeight:", ""));

	if (!_stricmp(temp, "")) 
		strcpy(temp, ReadString(stylepath, "*fontWeight:", "normal"));

	if (!_stricmp(temp, "bold"))
		desktop.fontWeight = FW_BOLD;
	else
		desktop.fontWeight = FW_NORMAL;

	desktop.fontColor = ReadColor(stylepath, "toolbar.label.textColor:", "#FFFFFF");

//===========================================================
// bbpager.desktop: -> this is for the normal desktops

	strcpy(tempstring, ReadString(bspath, "bbpager.desktop:", "doesnotexist"));	

	if (!_stricmp(tempstring, "doesnotexist")) // bbpager.desktop NOT in BB
	{
		strcpy(tempstring, ReadString(stylepath, "bbpager.desktop:", "doesnotexist"));	
		if (!_stricmp(tempstring, "doesnotexist")) // bbpager.desktop NOT in STYLE
		{ 
			strcpy(tempstyle, ReadString(stylepath, "toolbar.label:", "Flat Gradient Vertical"));
			if (desktop.style) delete desktop.style;
			desktop.style = new StyleItem;
			ParseItem(tempstyle, desktop.style);
			desktop.color = ReadColor(stylepath, "toolbar.label.color:", "#000000");
			desktop.colorTo = ReadColor(stylepath, "toolbar.label.colorTo:", "#FFFFFF");
		}
		else // bbpager.desktop IS in STYLE
		{
			strcpy(tempstyle, ReadString(stylepath, "bbpager.desktop:", "Flat Gradient Vertical"));
			if (desktop.style) delete desktop.style;
			desktop.style = new StyleItem;
			ParseItem(tempstyle, desktop.style);

			desktop.color = ReadColor(stylepath, "bbpager.desktop.color:", "#000000");
			desktop.colorTo = ReadColor(stylepath, "bbpager.desktop.colorTo:", "#FFFFFF");
		}
	}
	else // bbpager.desktop IS in BB
	{
		strcpy(tempstyle, ReadString(bspath, "bbpager.desktop:", "Flat Gradient Vertical"));
		if (desktop.style) delete desktop.style;
		desktop.style = new StyleItem;
		ParseItem(tempstyle, desktop.style);

		desktop.color = ReadColor(bspath, "bbpager.desktop.color:", "#000000");
		desktop.colorTo = ReadColor(bspath, "bbpager.desktop.colorTo:", "#FFFFFF");
	}

//===========================================================
// bbpager.desktop.focusStyle: -> specifies how to draw active desktop

	strcpy(tempstring, ReadString(bspath, "bbpager.desktop.focusStyle:", "doesnotexist"));

	if (!_stricmp(tempstring, "doesnotexist")) // bbpager.active.desktop.focusStyle NOT in BB
	{ 
		strcpy(tempstring, ReadString(stylepath, "bbpager.desktop.focusStyle:", "doesnotexist"));
		if (!_stricmp(tempstring, "doesnotexist")) // bbpager.active.desktop.focusStyle NOT in STYLE
		{
			strcpy(activeDesktop.styleType, "border"); //default to border
		}
		else // bbpager.active.desktop.focusStyle IS in STYLE
		{
			strcpy(activeDesktop.styleType, ReadString(stylepath, "bbpager.desktop.focusStyle:", "border"));
		}
	}
	else // bbpager.active.desktop.focusStyle IS in BB
	{
		strcpy(activeDesktop.styleType, ReadString(bspath, "bbpager.desktop.focusStyle:", "border"));
	}

//===========================================================
// bbpager.desktop.focus: -> style definition used for current workspace

//	if (!_stricmp(activeDesktop.styleType, "texture")) 
//	{
		activeDesktop.useDesktopStyle = false;

		strcpy(tempstring, ReadString(bspath, "bbpager.desktop.focus:", "doesnotexist"));	
		if (!_stricmp(tempstring, "doesnotexist")) // bbpager.desktop.focus NOT in BB
		{
			strcpy(tempstring, ReadString(stylepath, "bbpager.desktop.focus:", "doesnotexist"));	
			if (!_stricmp(tempstring, "doesnotexist")) // bbpager.desktop.focus NOT in STYLE
			{ 
				// bbpager.desktop.focus style definition not defined, so inherit normal desktop definition
				activeDesktop.useDesktopStyle = true;
				activeDesktop.color = desktop.color;
				activeDesktop.colorTo = desktop.colorTo;
			}
			else // bbpager.desktop.focus IS in STYLE
			{
				strcpy(tempstyle, ReadString(stylepath, "bbpager.desktop.focus:", "Flat Gradient Vertical"));
				if (activeDesktop.style) delete activeDesktop.style;
				activeDesktop.style = new StyleItem;
				ParseItem(tempstyle, activeDesktop.style);

				activeDesktop.color = ReadColor(stylepath, "bbpager.desktop.focus.color:", "#000000");
				activeDesktop.colorTo = ReadColor(stylepath, "bbpager.desktop.focus.colorTo:", "#FFFFFF");
			}
		}
		else // bbpager.desktop.focus IS in BB
		{ 
			strcpy(tempstyle, ReadString(bspath, "bbpager.desktop.focus:", "Flat Gradient Vertical"));
			if (activeDesktop.style) delete activeDesktop.style;
			activeDesktop.style = new StyleItem;
			ParseItem(tempstyle, activeDesktop.style);

			activeDesktop.color = ReadColor(bspath, "bbpager.desktop.focus.color:", "#000000");
			activeDesktop.colorTo = ReadColor(bspath, "bbpager.desktop.focus.colorTo:", "#FFFFFF");
		}
/*	}
	// bbpager.desktop.focus: border, so use normal desktop settings w/border
	else if (IsInString(activeDesktop.styleType, "border")) 
	{
		activeDesktop.color = desktop.color;
		activeDesktop.colorTo = desktop.colorTo;
	}
*/
//===========================================================
// bbpager.active.desktop.borderColor:

	strcpy(tempstring, ReadString(bspath, "bbpager.active.desktop.borderColor:", "doesnotexist"));

	if (!_stricmp(tempstring, "doesnotexist")) // bbpager.active.desktop.borderColor NOT in BB
	{
		strcpy(tempstring, ReadString(stylepath, "bbpager.active.desktop.borderColor:", "doesnotexist"));

		if (!_stricmp(tempstring, "doesnotexist")) // bbpager.active.desktop.borderColor NOT in STYLE
		{
			//activeDesktop.borderColor = ReadColor(stylepath, "toolbar.label.textColor:", "#FFFFFF");
			activeDesktop.borderColor = desktop.fontColor;
		}
		else // bbpager.active.desktop.borderColor IS in STYLE
		{
			activeDesktop.borderColor = ReadColor(stylepath, "bbpager.active.desktop.borderColor:", "#ffffff");
		}
	}
	else // bbpager.active.desktop.borderColor IS in BB
	{ 
		activeDesktop.borderColor = ReadColor(bspath, "bbpager.active.desktop.borderColor:", "#ffffff");
	}

//===========================================================
// frame.bevelWidth: spacing between desktops and edges of BBPager
	
	frame.bevelWidth = ReadInt(bspath, "bbpager.bevelWidth:", 666);
	
	if (frame.bevelWidth == 666) // bbpager.bevelWidth NOT in BB
	{
		frame.bevelWidth = ReadInt(stylepath, "bbpager.bevelWidth:", 666);

		if (frame.bevelWidth == 666) // bbpager.bevelWidth NOT in STYLE
		{
			frame.bevelWidth = ReadInt(stylepath, "bevelWidth:", 2);
		}
	}

//===========================================================
// frame.borderWidth: width of border around BBPager

	if (drawBorder)
	{
		frame.borderWidth = ReadInt(bspath, "bbpager.borderWidth:", 666);

		if (frame.borderWidth == 666) // bbpager.borderWidth NOT in BB
		{
			frame.borderWidth = ReadInt(stylepath, "bbpager.borderWidth:", 666);

			if (frame.borderWidth == 666) // bbpager.borderWidth NOT in STYLE
			{
				frame.borderWidth = ReadInt(stylepath, "borderWidth:", 1);
			}
		}
	}
	else
	{
		frame.borderWidth = 0;
	}

//===========================================================
// frame.hideWidth: amount of pager seen in the hidden state

	frame.hideWidth = frame.bevelWidth + frame.borderWidth;
	if (frame.hideWidth < 1) frame.hideWidth = 1;

//===========================================================
// bbpager.window: -> this is for the normal windows

	strcpy(tempstring, ReadString(bspath, "bbpager.window:", "doesnotexist"));	

	if (!_stricmp(tempstring, "doesnotexist")) // bbpager.window NOT in BB
	{
		strcpy(tempstring, ReadString(stylepath, "bbpager.window:", "doesnotexist"));	
		if (!_stricmp(tempstring, "doesnotexist")) // bbpager.window NOT in STYLE
		{ 
			strcpy(tempstyle, ReadString(stylepath, "window.label.unfocus:", "Flat Gradient Vertical"));
			if (window.style) delete window.style;
			window.style = new StyleItem;
			ParseItem(tempstyle, window.style);

			window.color = ReadColor(stylepath, "window.label.unfocus.color:", "#000000");
			window.colorTo = ReadColor(stylepath, "window.label.unfocus.colorTo:", "#FFFFFF");

			if (window.style->parentRelative)
			{
				strcpy(tempstyle, ReadString(stylepath, "window.title.unfocus:", "Flat Gradient Vertical"));
				if (window.style) delete window.style;
				window.style = new StyleItem;
				ParseItem(tempstyle, window.style);

				window.color = ReadColor(stylepath, "window.title.unfocus.color:", "#000000");
				window.colorTo = ReadColor(stylepath, "window.title.unfocus.colorTo:", "#FFFFFF");
			}
		}
		else // bbpager.window IS in STYLE
		{
			strcpy(tempstyle, ReadString(stylepath, "bbpager.window:", "Flat Gradient Vertical"));
			if (window.style) delete window.style;
			window.style = new StyleItem;
			ParseItem(tempstyle, window.style);

			window.color = ReadColor(stylepath, "bbpager.window.color:", "#000000");
			window.colorTo = ReadColor(stylepath, "bbpager.window.colorTo:", "#FFFFFF");
		}
	}
	else // bbpager.window IS in BB
	{
		strcpy(tempstyle, ReadString(bspath, "bbpager.window:", "Flat Gradient Vertical"));
		if (window.style) delete window.style;
		window.style = new StyleItem;
		ParseItem(tempstyle, window.style);

		window.color = ReadColor(bspath, "bbpager.window.color:", "#000000");
		window.colorTo = ReadColor(bspath, "bbpager.window.colorTo:", "#FFFFFF");
	}

//===========================================================
// bbpager.inactive.window.borderColor:

	strcpy(tempstring, ReadString(bspath, "bbpager.inactive.window.borderColor:", "doesnotexist"));

	if (!_stricmp(tempstring, "doesnotexist")) // bbpager.inactive.windowborderColor NOT in BB
	{
		strcpy(tempstring, ReadString(stylepath, "bbpager.inactive.window.borderColor:", "doesnotexist"));

		if (!_stricmp(tempstring, "doesnotexist")) // bbpager.inactive.window.borderColor NOT in STYLE
		{
			window.borderColor = RGB(0,0,0);
		}
		else // bbpager.active.window.borderColor IS in STYLE
		{
			window.borderColor = ReadColor(stylepath, "bbpager.inactive.window.borderColor:", "#ffffff");
		}
	}
	else // bbpager.active.window.borderColor IS in BB
	{ 
		window.borderColor = ReadColor(bspath, "bbpager.inactive.window.borderColor:", "#ffffff");
	}

//===========================================================
// bbpager.window.focusStyle: -> specifies how to draw active desktop

	strcpy(tempstring, ReadString(bspath, "bbpager.window.focusStyle:", "doesnotexist"));

	if (!_stricmp(tempstring, "doesnotexist")) // bbpager.window.focusStyle NOT in BB
	{ 
		strcpy(tempstring, ReadString(stylepath, "bbpager.window.focusStyle:", "doesnotexist"));
		if (!_stricmp(tempstring, "doesnotexist")) // bbpager.window.focusStyle NOT in STYLE
		{
			strcpy(focusedWindow.styleType, "texture"); //default to texture
		}
		else // bbpager.window.focusStyle IS in STYLE
		{
			strcpy(focusedWindow.styleType, ReadString(stylepath, "bbpager.window.focusStyle:", "texture"));
		}
	}
	else // bbpager.active.desktop.focusStyle IS in BB
	{
		strcpy(focusedWindow.styleType, ReadString(bspath, "bbpager.window.focusStyle:", "texture"));
	}

//===========================================================
// bbpager.window.focus: -> style definition used for active window

//	if (!_stricmp(focusedWindow.styleType, "texture")) 
//	{
		strcpy(tempstring, ReadString(bspath, "bbpager.window.focus:", "doesnotexist"));	
		if (!_stricmp(tempstring, "doesnotexist")) // bbpager.active.desktop NOT in BB
		{
			strcpy(tempstring, ReadString(stylepath, "bbpager.window.focus:", "doesnotexist"));	
			if (!_stricmp(tempstring, "doesnotexist")) // bbpager.window.focus NOT in STYLE
			{ 
				strcpy(tempstyle, ReadString(stylepath, "window.label.focus:", "Flat Gradient Vertical"));
				if (focusedWindow.style) delete focusedWindow.style;
				focusedWindow.style = new StyleItem;
				ParseItem(tempstyle, focusedWindow.style);

				focusedWindow.color = ReadColor(stylepath, "window.label.focus.color:", "#000000");
				focusedWindow.colorTo = ReadColor(stylepath, "window.label.focus.colorTo:", "#FFFFFF");

				if (focusedWindow.style->parentRelative)
				{
					strcpy(tempstyle, ReadString(stylepath, "window.title.focus:", "Flat Gradient Vertical"));
					if (focusedWindow.style) delete focusedWindow.style;
					focusedWindow.style = new StyleItem;
					ParseItem(tempstyle, focusedWindow.style);

					focusedWindow.color = ReadColor(stylepath, "window.title.focus.color:", "#000000");
					focusedWindow.colorTo = ReadColor(stylepath, "window.title.focus.colorTo:", "#FFFFFF");
				}
			}
			else // bbpager.window.focus IS in STYLE
			{
				strcpy(tempstyle, ReadString(stylepath, "bbpager.window.focus:", "Flat Gradient Vertical"));
				if (focusedWindow.style) delete focusedWindow.style;
				focusedWindow.style = new StyleItem;
				ParseItem(tempstyle, focusedWindow.style);

				focusedWindow.color = ReadColor(stylepath, "bbpager.window.focus.color:", "#000000");
				focusedWindow.colorTo = ReadColor(stylepath, "bbpager.window.focus.colorTo:", "#FFFFFF");
			}
		}
		else // bbpager.active.desktop IS in BB
		{ 
			strcpy(tempstyle, ReadString(bspath, "bbpager.window.focus:", "Flat Gradient Vertical"));
			if (focusedWindow.style) delete focusedWindow.style;
			focusedWindow.style = new StyleItem;
			ParseItem(tempstyle, focusedWindow.style);

			focusedWindow.color = ReadColor(bspath, "bbpager.window.focus.color:", "#000000");
			focusedWindow.colorTo = ReadColor(bspath, "bbpager.window.focus.colorTo:", "#FFFFFF");
		}
/*	}
	// bbpager.desktop.active: border, so use normal desktop settings w/border
	else if (IsInString(focusedWindow.styleType, "border")) 
	{
		focusedWindow.color = window.color;
		focusedWindow.colorTo = window.colorTo;
	}
*/
//===========================================================
// bbpager.active.window.borderColor:

	strcpy(tempstring, ReadString(bspath, "bbpager.active.window.borderColor:", "doesnotexist"));

	if (!_stricmp(tempstring, "doesnotexist")) // bbpager.active.windowborderColor NOT in BB
	{
		strcpy(tempstring, ReadString(stylepath, "bbpager.active.window.borderColor:", "doesnotexist"));

		if (!_stricmp(tempstring, "doesnotexist")) // bbpager.active.window.borderColor NOT in STYLE
		{
			focusedWindow.borderColor = RGB(0,0,0);
		}
		else // bbpager.active.window.borderColor IS in STYLE
		{
			focusedWindow.borderColor = ReadColor(stylepath, "bbpager.active.window.borderColor:", "#ffffff");
		}
	}
	else // bbpager.active.window.borderColor IS in BB
	{ 
		focusedWindow.borderColor = ReadColor(bspath, "bbpager.active.window.borderColor:", "#ffffff");
	}
}