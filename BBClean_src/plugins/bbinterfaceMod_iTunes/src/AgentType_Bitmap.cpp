/*===================================================

	AGENTTYPE_BITMAP CODE

===================================================*/

#include <windows.h>
#include <GdiPlus.h>

// Global Include
#include "BBApi.h"
#include <string.h>

//Parent Include
#include "AgentType_Bitmap.h"

//Includes
#include "PluginMaster.h"
#include "AgentMaster.h"
#include "Definitions.h"
#include "ConfigMaster.h"
#include "StyleMaster.h"
#include "DialogMaster.h"
#include "MessageMaster.h"
#include "MenuMaster.h"
#include "Shellapi.h"

//Local functions
int agenttype_bitmaporicon_create(agent *a, char *parameterstring, bool is_icon);
int agenttype_bitmaporicon_setsource(agent *a, char *parameterstring);

void drawIcon (int px, int py, int size, HICON IconHop, HDC hDC, bool f);
#define saturationValue plugin_icon_sat
#define hueIntensity plugin_icon_hue

//Constant strings
const char *image_haligns[] = {"Center", "Left", "Right", NULL};
const char *image_valigns[] = {"Center", "Top", "Bottom", NULL};


//GDI+ structs
Gdiplus::GdiplusStartupInput gdiplusStartupInput;
ULONG_PTR gdiplusToken;

Gdiplus::ImageAttributes *imageAttr;
Gdiplus::Graphics *graphics;
Gdiplus::Image *pImage;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//controltype_button_startup
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int agenttype_bitmap_startup()
{
	//Register this type with the ControlMaster
	agent_registertype(
		"Bitmap",                           //Friendly name of agent type
		"Bitmap",                           //Name of agent type
		CONTROL_FORMAT_IMAGE,               //Control type
		true,
		&agenttype_bitmap_create,           
		&agenttype_bitmap_destroy,
		&agenttype_bitmap_message,
		&agenttype_bitmap_notify,
		&agenttype_bitmap_getdata,
		&agenttype_bitmap_menu_set,
		&agenttype_bitmap_menu_context,
		&agenttype_bitmap_notifytype
		);

	//Register this type with the ControlMaster
	agent_registertype(
		"Icon",                             //Friendly name of agent type
		"Icon",                             //Name of agent type
		CONTROL_FORMAT_IMAGE,               //Control type
		true,
		&agenttype_icon_create,         
		&agenttype_bitmap_destroy,
		&agenttype_bitmap_message,
		&agenttype_bitmap_notify,
		&agenttype_bitmap_getdata,
		&agenttype_icon_menu_set,
		&agenttype_bitmap_menu_context,
		&agenttype_bitmap_notifytype
		);

	// Initialize GDI+
	if(Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL) != 0)
	{
		BBMessageBox(0, "Error starting GdiPlus.dll", szAppName, MB_OK | MB_ICONERROR | MB_TOPMOST);
		return 1;
	}

	//No errors
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_bitmap_shutdown
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int agenttype_bitmap_shutdown()
{	
	//shutdown the gdi+ engine
	Gdiplus::GdiplusShutdown(gdiplusToken);

	//No errors
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_bitmap_create
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int agenttype_bitmap_create(agent *a, char *parameterstring)
{
	return agenttype_bitmaporicon_create(a, parameterstring, false);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_bitmap_destroy
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int agenttype_bitmap_destroy(agent *a)
{
	//Delete the details if possible
	if (a->agentdetails)
	{
		agenttype_bitmap_details *details =(agenttype_bitmap_details *) a->agentdetails;
		free_string(&details->filename);
		free_string(&details->absolute_path); 
		delete details;
		a->agentdetails = NULL;
	}

	//No errors
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_bitmap_message
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int agenttype_bitmap_message(agent *a, int tokencount, char *tokens[])
{
	//Get the agent details
	agenttype_bitmap_details *details = (agenttype_bitmap_details *) a->agentdetails;
	if (details->is_icon && !stricmp("Size", tokens[5]) && config_set_int(tokens[6], &details->width, 1, 256))
	{
		details->height = details->width;
		control_notify(a->controlptr, NOTIFY_NEEDUPDATE, NULL);
		return 0;
	}
	else if (details->filename && !stricmp("Scale", tokens[5]) && config_set_int(tokens[6], &details->scale, 1, 500))
	{
		control_notify(a->controlptr, NOTIFY_NEEDUPDATE, NULL);
		return 0;
	}
	else if (!stricmp("Source", tokens[5]))
	{
		//Try to set the source
		char *parameterstring = new_string(tokens[6]);
		int setsource = agenttype_bitmaporicon_setsource(a, parameterstring);
		free_string(&parameterstring);
		if (setsource == 0) control_notify(a->controlptr, NOTIFY_NEEDUPDATE, NULL);
		return 0;
	}

	int i;
	if ( (!stricmp("VAlign", tokens[5])) && (-1 != (i = get_string_index(tokens[6], image_valigns))) )
	{
		details->valign = i;
		control_notify(a->controlptr, NOTIFY_NEEDUPDATE, NULL);
		return 0;
	}
	else if (!stricmp("HAlign", tokens[5]) && (-1 != (i = get_string_index(tokens[6], image_haligns))) )
	{
		details->halign = i;
		control_notify(a->controlptr, NOTIFY_NEEDUPDATE, NULL);
		return 0;
	}



	return 1;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_bitmap_notify
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void agenttype_bitmap_notify(agent *a, int notifytype, void *messagedata)
{
	//Get the agent details
	agenttype_bitmap_details *details = (agenttype_bitmap_details *) a->agentdetails;

	styledrawinfo *di;
	int xpos, ypos;

	switch(notifytype)
	{
		case NOTIFY_DRAW:
			di = (styledrawinfo *) messagedata;

			if (details->is_icon)
			{
				HICON load_sysicon(char *filepath, int size);
				HICON hIcon = (HICON)LoadImage(plugin_instance_plugin, details->absolute_path, IMAGE_ICON, details->width, details->height, LR_LOADFROMFILE);
				// this one can retrieve the standard system icons also:
				if (NULL == hIcon) hIcon = load_sysicon(details->absolute_path, details->width);

				if (NULL != hIcon)
				{
					switch (details->halign)
					{
					case 0:
						xpos = ((di->rect.right - di->rect.left) / 2) - (details->width / 2);
						break;
					case 1:
						xpos = 2;
						break;
					case 2:
						xpos = (di->rect.right -  2) - (details->width);
						break;
					}
					switch (details->valign)
					{
					case 0:
						ypos = ((di->rect.bottom - di->rect.top) / 2) - (details->height / 2);
						break;
					case 1:
						ypos = 2;
						break;
					case 2:
						ypos = (di->rect.bottom - 2) - (details->height);
						break;
					}

					drawIcon (xpos, ypos, details->width, (HICON) hIcon, di->buffer, di->apply_sat_hue);
					//DrawIconEx(di->buffer, xpos, ypos, (HICON) hIcon, details->width, details->height, 0, NULL, DI_NORMAL);
					DestroyIcon(hIcon);
				}
			}
			else
			{
			  	WCHAR wTitle[256];
				if(!locale)
					locale = _create_locale(LC_CTYPE,"");
			  	_mbstowcs_l(wTitle, details->absolute_path, strlen(details->absolute_path) + 1,locale);
			  	pImage = new Gdiplus::Image(wTitle);

				if (NULL != pImage)
				{
					details->width = pImage->GetWidth() * ((double)details->scale / 100);
					details->height = pImage->GetHeight() * ((double)details->scale / 100);

					switch (details->halign)
					{
					case 0:
						xpos = ((di->rect.right - di->rect.left) / 2) - (details->width / 2);
						break;
					case 1:
						xpos = 2;
						break;
					case 2:
						xpos = (di->rect.right -  2) - (details->width);
						break;
					}
					switch (details->valign)
					{
					case 0:
						ypos = ((di->rect.bottom - di->rect.top) / 2) - (details->height / 2);
						break;
					case 1:
						ypos = 2;
						break;
					case 2:
						ypos = (di->rect.bottom - 2) - (details->height);
						break;
					}

					imageAttr = new Gdiplus::ImageAttributes();
					imageAttr->SetColorKey(RGB(255,0,255), RGB(255,0,255));

					graphics = new Gdiplus::Graphics(di->buffer);
					graphics->DrawImage(pImage, Gdiplus::Rect(xpos, ypos, details->width, details->height),
                                                            0, 0, pImage->GetWidth(), pImage->GetHeight(),
                                                            Gdiplus::UnitPixel, imageAttr, NULL, NULL);

					delete imageAttr;
					delete graphics;
					delete pImage;
				}
			}
			break;

		case NOTIFY_SAVE_AGENT:
			//Write existance
			config_write(config_get_control_setagent_c(a->controlptr, a->agentaction, a->agenttypeptr->agenttypename, details->filename));
			//Save properties
			if (details->is_icon) config_write(config_get_control_setagentprop_i(a->controlptr, a->agentaction, "Size", &details->width));
			else if (details->filename) config_write(config_get_control_setagentprop_i(a->controlptr, a->agentaction, "Scale", &details->scale));
			config_write(config_get_control_setagentprop_c(a->controlptr, a->agentaction, "VAlign", image_valigns[details->valign]));
			config_write(config_get_control_setagentprop_c(a->controlptr, a->agentaction, "HAlign", image_haligns[details->halign]));
			break;
	}
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_bitmap_getdata
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void *agenttype_bitmap_getdata(agent *a, int datatype)
{
	agenttype_bitmap_details *details = (agenttype_bitmap_details *) a->agentdetails;
	switch (datatype)
	{
		case DATAFETCH_CONTENTSIZES: 
			return &details->width;
	}

	return NULL;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_bitmap_menu_set
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void agenttype_bitmap_menu_set(Menu *m, control *c, agent *a,  char *action, int controlformat)
{
	make_menuitem_cmd(m, "Browse...", config_getfull_control_setagent_c(c, action, "Bitmap", "*browse*"));
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_icon_menu_set
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void agenttype_icon_menu_set(Menu *m, control *c, agent *a,  char *action, int controlformat)
{
	make_menuitem_cmd(m, "Browse...", config_getfull_control_setagent_c(c, action, "Icon", "*browse*"));
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_bitmap_menu_context
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void agenttype_bitmap_menu_context(Menu *m, agent *a)
{
	//Get the agent details
	agenttype_bitmap_details *details = (agenttype_bitmap_details *) a->agentdetails;	

	//For convenience, change the bitmap source without changing the settings
	make_menuitem_cmd(m, "Change Source...", config_getfull_control_setagentprop_c(a->controlptr, a->agentaction, "Source", "*browse*"));

	if (details->is_icon)
	{
		make_menuitem_int(m, "Icon Size",
                                          config_getfull_control_setagentprop_s(a->controlptr, a->agentaction, "Size"),
                                          details->width, 1, 256);
	}
	else if (details->filename)
	{
		make_menuitem_int(m, "Scale",
                                          config_getfull_control_setagentprop_s(a->controlptr, a->agentaction, "Scale"),
                                          details->scale, 1, 500);
	}
	Menu *submenu = make_menu("Image Alignment", a->controlptr);
	make_menuitem_bol(submenu, "Left", config_getfull_control_setagentprop_c(a->controlptr, a->agentaction, "HAlign", "Left"), details->halign == 1);
	make_menuitem_bol(submenu, "Center", config_getfull_control_setagentprop_c(a->controlptr, a->agentaction, "HAlign", "Center"), details->halign == 0);
	make_menuitem_bol(submenu, "Right", config_getfull_control_setagentprop_c(a->controlptr, a->agentaction, "HAlign", "Right"), details->halign == 2);
	make_menuitem_nop(submenu, "");
	make_menuitem_bol(submenu, "Top", config_getfull_control_setagentprop_c(a->controlptr, a->agentaction, "VAlign", "Top"), details->valign == 1);
	make_menuitem_bol(submenu, "Center", config_getfull_control_setagentprop_c(a->controlptr, a->agentaction, "VAlign", "Center"), details->valign == 0);
	make_menuitem_bol(submenu, "Bottom", config_getfull_control_setagentprop_c(a->controlptr, a->agentaction, "VAlign", "Bottom"), details->valign == 2);
	make_submenu_item(m, "Image Alignment", submenu);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_bitmap_notifytype
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void agenttype_bitmap_notifytype(int notifytype, void *messagedata)
{

}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_icon_create
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int agenttype_icon_create(agent *a, char *parameterstring)
{
	return agenttype_bitmaporicon_create(a, parameterstring, true);
}

//##################################################
//agenttype_icon_create
//##################################################

int agenttype_bitmaporicon_create(agent *a, char *parameterstring, bool is_icon)
{
	//Create the details
	agenttype_bitmap_details *details = new agenttype_bitmap_details;
	a->agentdetails = (void *) details;

	//Set the icon state
	details->is_icon = is_icon;

	//Set the defaults
	if (is_icon)
	{
		details->height = 32;
		details->width = 32;
	}
	else
	{
		details->scale = 100;
	}
	details->valign = 0;
	details->halign = 0;

	//Try to set the source
	int setsource = agenttype_bitmaporicon_setsource(a, parameterstring);
	if (setsource != 0)
	{
		//Delete the details and return error
		delete details;
		return 2;
	}

	//No errors
	return 0;
}

//##################################################
//agenttype_bitmaporicon_setsource
//##################################################
int agenttype_bitmaporicon_setsource(agent *a, char *parameterstring)
{
	//Declare variables
	const char *string_dialogtitles[2] = {"Select Image", "Select Icon"};
	const char *string_dialogfilts[2] = {"Image Files(*.png;*.bmp;*.jpg;*.gif;*.tif)\0*.png;*.bmp;*.jpg;*.gif;*.tif\0PNG(*.png)\0*.png\0BMP(*.bmp)\0*.bmp\0JPG(*.jpg)\0*.jpg\0GIF(*.gif)\0*.gif\0All Files(*.*)\0*.*\0\0*.*\0\0",
          "Icon Files(*.ico;*.exe;*.dll;*.icl;*.lnk)\0*.ico;*.exe;*.dll;*.icl;*.lnk\0All Files(*.*)\0*.*\0\0" };
	const char *string_dialogext[2] = {"", ".ico"};
	const UINT uint_loadtype[2] = {IMAGE_BITMAP, IMAGE_ICON};
	char *pathstring;

	//Get the agent details
	agenttype_bitmap_details *details = (agenttype_bitmap_details *) a->agentdetails;	
	int typeindex = (details->is_icon ? 1 : 0);

	//If the browse option is chosen
	if (!stricmp(parameterstring, "*browse*"))
	{       
		parameterstring = dialog_file(string_dialogfilts[typeindex], string_dialogtitles[typeindex], NULL /*config_path_plugin*/, string_dialogext[typeindex], false);
		if (!parameterstring)
		{
			//message_override = true;
			return 2;
		}

		//If we have an absolute path, and only a relative path is necessary
		int lenpath = strlen(config_path_plugin);
		if (!strnicmp(config_path_plugin, parameterstring, lenpath))
		{
			strcpy(parameterstring, &parameterstring[lenpath]);
		}
	}

	//Declare variables
	char plugin_path[MAX_PATH];
	char *temp = parameterstring;

	//If we have an actual string...
	if (temp[0] && temp[1] != ':')
	{
		// reconstruct from relative path
		temp = plugin_path;
		strcpy(temp, config_path_plugin);
		strcat(temp, parameterstring);
	}

	//Copy the parameter string
	details->filename = new_string(parameterstring);
	details->absolute_path = new_string(temp); 

	//Return 0 for success
	return 0;
}

/*=================================================*/
//load_sysicon
/*=================================================*/
#include <commctrl.h>
HICON load_sysicon(char *path, int size)
{
	UINT cbfileinfo =
	(size <= 16) ? SHGFI_SYSICONINDEX|SHGFI_SMALLICON : SHGFI_SYSICONINDEX;
	SHFILEINFO shinfo;
	HIMAGELIST sysimgl = (HIMAGELIST)SHGetFileInfo(path, 0, &shinfo, sizeof(SHFILEINFO), cbfileinfo);
	if (sysimgl) return ImageList_ExtractIcon(NULL, sysimgl, shinfo.iIcon);
	return NULL;
}
/*=================================================*/
// Function: drawIco

static void perform_satnhue(BYTE *pixels, int size, int saturationValue, int hueIntensity)
{
	unsigned char sat = saturationValue;
	unsigned char hue = hueIntensity;
	unsigned short i_sat = 256 - sat;
	unsigned short i_hue = 256 - hue;

	BYTE* mask, *icon, *back;
	mask = pixels;
	unsigned next_ico = size * size * 4;
	int y = size;
	do {
		int x = size;
		do {
			if (0 == *mask)
			{
				back = (icon = mask + next_ico) + next_ico;
				unsigned char r = icon[2];
				unsigned char g = icon[1];
				unsigned char b = icon[0];
				if (sat<255)
				{
					unsigned greyval = (r*79 + g*156 + b*21) * i_sat / 256;
					r = (r * sat + greyval) / 256;
					g = (g * sat + greyval) / 256;
					b = (b * sat + greyval) / 256;
				}
				if (hue)
				{
					r = (r * i_hue + back[2] * hue) / 256;
					g = (g * i_hue + back[1] * hue) / 256;
					b = (b * i_hue + back[0] * hue) / 256;
				}
				back[2] = r;
				back[1] = g;
				back[0] = b;
			}
			mask += 4;
		}
		while (--x);
	}
	while (--y);
}

void drawIcon (int px, int py, int size, HICON IconHop, HDC hDC, bool f)
{
	if (false == f || (saturationValue >= 255 && hueIntensity <= 0))
	{
		DrawIconEx(hDC, px, py, IconHop, size, size, 0, NULL, DI_NORMAL);
		return;
	}

	BITMAPINFOHEADER bv4info;

	ZeroMemory(&bv4info,sizeof(bv4info));
	bv4info.biSize = sizeof(bv4info);
	bv4info.biWidth = size;
	bv4info.biHeight = size * 3;
	bv4info.biPlanes = 1;
	bv4info.biBitCount = 32;
	bv4info.biCompression = BI_RGB;

	BYTE* pixels;

	HBITMAP bufbmp = CreateDIBSection(NULL, (BITMAPINFO*)&bv4info, DIB_RGB_COLORS, (PVOID*)&pixels, NULL, 0);

	if (NULL == bufbmp)
		return;

	HDC bufdc = CreateCompatibleDC(hDC);
	HGDIOBJ other = SelectObject(bufdc, bufbmp);

	// draw the required three things side by side

	// background for hue
	BitBlt(bufdc,       0, 0,       size, size, hDC, px, py, SRCCOPY);

	// background for icon
	BitBlt(bufdc,       0, size,    size, size, hDC, px, py, SRCCOPY);

	// icon, in colors
	DrawIconEx(bufdc,   0, size,    IconHop, size, size, 0, NULL, DI_NORMAL);

	// icon mask
	DrawIconEx(bufdc,   0, size*2,  IconHop, size, size, 0, NULL, DI_MASK);

	perform_satnhue(pixels, size, saturationValue, hueIntensity);

	BitBlt(hDC, px, py, size, size, bufdc, 0, 0, SRCCOPY);

	DeleteObject(SelectObject(bufdc, other));
	DeleteDC(bufdc);
}

//===========================================================================
