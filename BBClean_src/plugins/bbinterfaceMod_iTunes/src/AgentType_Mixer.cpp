/*===================================================

	AGENTTYPE_MIXER CODE

===================================================*/

// Global Include
#include "BBApi.h"
#include <string.h>
#include <stdlib.h>
#include <mmsystem.h>
#ifdef _MSC_VER
#pragma comment(lib, "winmm.lib")
#endif

//Parent Include
#include "AgentType_Mixer.h"

//Includes
#include "PluginMaster.h"
#include "AgentMaster.h"
#include "Definitions.h"
#include "ControlMaster.h"
#include "ConfigMaster.h"
#include "MenuMaster.h"

//Local variables
HWND mixer_hwnd = NULL;
const char * mixer_name_scale = "MixerScale";
const char * mixer_name_bool = "MixerBool";
char mixertoken1[30], mixertoken2[30], mixertoken3[30], mixertoken4[30];
char *mixer_tokenptrs[4] = {mixertoken1, mixertoken2, mixertoken3, mixertoken4};
const char mixer_recieverclass[] = "BBInterfaceAgentMixerReciever";
bool mixer_recieverregistered = false;
int mixer_controlcount;
char mixer_outputbuffer[32] = "";

//Internal functions
void agenttype_mixer_createreciever();
void agenttype_mixer_destroyreciever();
LRESULT CALLBACK agenttype_mixer_recieverevent(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
int agenttype_mixer_menu_linecontrols(Menu *menu, control *c, char *action, char *agentname, int format, UINT device, HMIXER mixer_handle, MIXERLINE &mixer_line);
void agenttype_mixer_menu_sourcelines(Menu *menu, control *c, char *action, char *agentname, int format, UINT device, HMIXER mixer_handle, MIXERLINE &mixer_destline);
void agenttype_mixer_menu_destlines(Menu *menu, control *c, char *action, char *agentname, int format, UINT device, HMIXER mixer_handle, MIXERCAPS &mixer_capabilities);
void agenttype_mixer_menu_devices(Menu *menu, control *c, char *action, char *agentname, int format);

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_mixer_startup
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int agenttype_mixer_startup()
{
	//No reciever window yet
	mixer_recieverregistered = false;

	//Register this type with the ControlMaster
	agent_registertype(
		"Mixer",                            //Friendly name of agent type
		"MixerScale",                       //Name of agent type
		CONTROL_FORMAT_SCALE|CONTROL_FORMAT_TEXT,               //Control format
		true,
		&agenttype_mixer_create,            
		&agenttype_mixer_destroy,
		&agenttype_mixer_message,
		&agenttype_mixer_notify,
		&agenttype_mixer_getdata,
		&agenttype_mixerscale_menu_set,
		&agenttype_mixer_menu_context,
		&agenttype_mixer_notifytype
		);

	//Register this type with the ControlMaster
	agent_registertype(
		"Mixer",                            //Friendly name of agent type
		"MixerBool",                        //Name of agent type
		CONTROL_FORMAT_BOOL,                //Control format
		true,
		&agenttype_mixer_create,            
		&agenttype_mixer_destroy,
		&agenttype_mixer_message,
		&agenttype_mixer_notify,
		&agenttype_mixer_getdata,
		&agenttype_mixerbool_menu_set,
		&agenttype_mixer_menu_context,
		&agenttype_mixer_notifytype
		);

	//No errors
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_mixer_shutdown
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int agenttype_mixer_shutdown()
{
	//No errors
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_mixer_create
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int agenttype_mixer_create(agent *a, char *parameterstring)
{
	//If there's an error
	bool errorflag = false;;

	//Check for error conditions    
	if (strlen(parameterstring) >= 30) return false;

	//Break up the parts
	int tokensfound = BBTokenize(parameterstring, mixer_tokenptrs, 4, NULL);

	//Three tokens exactly required
	if (tokensfound != 3) return 1;

	long values[3];
	//Make sure they are all valid integers
	for (int i = 0; i < 3; i++)
	{
		if (!config_set_long(mixer_tokenptrs[i], &values[i])) return 1;
	}

	//Create the details
	agenttype_mixer_details *details = new agenttype_mixer_details;
	a->agentdetails = (void *) details;

	//Copy the values
	details->device = values[0];
	details->line = values[1];
	details->control = values[2];

	//Create the reciever window class if neccessary
	//No errors
	mixer_controlcount++;
	if (mixer_controlcount > 0 && !mixer_recieverregistered)
	{
		if (!window_helper_register(mixer_recieverclass, &agenttype_mixer_recieverevent)) mixer_recieverregistered = true;
		else errorflag = true;
	}

	//Create the reciever window
	details->hwnd_reciever = NULL;
	if (!errorflag)
	{
		details->hwnd_reciever = window_helper_create(mixer_recieverclass);
		if (!details->hwnd_reciever) errorflag = true;
	}

	if (!errorflag)
	{
		//Initialize the mixer values
		if (MMSYSERR_NOERROR != mixerOpen(&details->mixer_handle, details->device, (DWORD) details->hwnd_reciever, 0, CALLBACK_WINDOW)) errorflag = true;
	}

	if (!errorflag)
	{
		//Set the control properties
		details->mixer_controldetails.cbStruct = sizeof(MIXERCONTROLDETAILS);
		details->mixer_controldetails.dwControlID = details->control;
		details->mixer_controldetails.cChannels = 1;
		details->mixer_controldetails.cMultipleItems = 0;
		details->mixer_controldetails.cbDetails = sizeof(MIXERCONTROLDETAILS_UNSIGNED);

		//Set the window property
		SetProp(details->hwnd_reciever, "mixagtptr", a);
	}

	if (errorflag)
	{
			if (details->hwnd_reciever)
				window_helper_destroy(details->hwnd_reciever);

			delete details;
			a->agentdetails = NULL;

			agent_destroy(&a);
			return 1;   
	}

	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_mixer_destroy
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int agenttype_mixer_destroy(agent *a)
{
	if (a->agentdetails)
	{
		//Get the details
		agenttype_mixer_details *details = (agenttype_mixer_details *) a->agentdetails;

		//Close the window
		if (details->hwnd_reciever) window_helper_destroy(details->hwnd_reciever);
		details->hwnd_reciever = NULL;

		//Close the mixer
		mixerClose(details->mixer_handle);

		//Delete the details
		delete details;
		a->agentdetails = NULL;
	}

	//Destroy the window if necessary
	mixer_controlcount--;
	if (mixer_controlcount < 1 && mixer_recieverregistered)
	{
		window_helper_unregister(mixer_recieverclass);      
		mixer_recieverregistered = false;
	}

	//No errors
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_mixer_message
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int agenttype_mixer_message(agent *a, int tokencount, char *tokens[])
{
	return 1;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_mixer_notify
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void agenttype_mixer_notify(agent *a, int notifytype, void *messagedata)
{
	
	//Get the agent details
	agenttype_mixer_details *details;
	details = (agenttype_mixer_details *) a->agentdetails;

	//Variables
	double *value_double = NULL;
	bool *value_bool = NULL;
	MIXERCONTROLDETAILS_UNSIGNED    mixer_setcontrol_value_double;
	MIXERCONTROLDETAILS_BOOLEAN     mixer_setcontrol_value_bool;

	switch (notifytype)
	{
		case NOTIFY_CHANGE:
			//Set up the values
			if (a->agenttypeptr->format & CONTROL_FORMAT_SCALE)
			{
				value_double = (double *) messagedata;
				details->mixer_controldetails.paDetails = &mixer_setcontrol_value_double;
			}
			else if (a->agenttypeptr->format & CONTROL_FORMAT_BOOL)
			{
				value_bool = (bool *) messagedata;              
				details->mixer_controldetails.paDetails = &mixer_setcontrol_value_bool;
			}

			//Retrieve the details
			if (MMSYSERR_NOERROR != mixerGetControlDetails((HMIXEROBJ) details->mixer_handle, &details->mixer_controldetails, MIXER_GETCONTROLDETAILSF_VALUE)) return;

			//Set the value
			if (a->agenttypeptr->format & CONTROL_FORMAT_SCALE)
			{
				mixer_setcontrol_value_double.dwValue = (ULONG) (*value_double * 65535);
			}
			else if (a->agenttypeptr->format & CONTROL_FORMAT_BOOL)
			{
				mixer_setcontrol_value_bool.fValue = *value_bool;
			}

			//Reload the details
			if (MMSYSERR_NOERROR != mixerSetControlDetails((HMIXEROBJ) details->mixer_handle, &details->mixer_controldetails, MIXER_SETCONTROLDETAILSF_VALUE)) return;  

			break;

		case NOTIFY_SAVE_AGENT:
			//Write existance
			char temp[30];
			sprintf(temp, "%d %d %d", (int)details->device, (int)details->line, (int)details->control);
			config_write(config_get_control_setagent_c(a->controlptr, a->agentaction, a->agenttypeptr->agenttypename, temp));
			break;
	}   
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_mixer_getdata
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void *agenttype_mixer_getdata(agent *a, int datatype)
{
	MIXERCONTROLDETAILS_UNSIGNED    mixer_setcontrol_value_double;
	MIXERCONTROLDETAILS_BOOLEAN     mixer_setcontrol_value_bool;
	agenttype_mixer_details *details = (agenttype_mixer_details *) a->agentdetails;
	if (!details) return NULL;

	switch (datatype)
	{
		case DATAFETCH_VALUE_TEXT:
		case DATAFETCH_VALUE_SCALE:
		case DATAFETCH_VALUE_BOOL:				
			//Set up the values
			if (datatype == DATAFETCH_VALUE_BOOL)
			{
				details->mixer_controldetails.paDetails = &mixer_setcontrol_value_bool;				
			}
			else
			{
				details->mixer_controldetails.paDetails = &mixer_setcontrol_value_double;
			}

			//Retrieve the details
			if (MMSYSERR_NOERROR != mixerGetControlDetails((HMIXEROBJ) details->mixer_handle, &details->mixer_controldetails, MIXER_GETCONTROLDETAILSF_VALUE)) return NULL;

			//Get and return the value
			if (datatype == DATAFETCH_VALUE_SCALE || datatype == DATAFETCH_VALUE_TEXT)
			{
				details->value_double = mixer_setcontrol_value_double.dwValue / 65535.0;
				if (details->value_double < 0.0) details->value_double = 0.0;
				if (details->value_double > 1.0) details->value_double = 1.0;

				//Return if it is the simple double value
				if (datatype == DATAFETCH_VALUE_SCALE) return &(details->value_double);

				//Otherwise, it must be text
				int intvalue = 100 * (details->value_double);
				sprintf(mixer_outputbuffer, "%d%%", intvalue);
				return mixer_outputbuffer;
			}
			else if (datatype == DATAFETCH_VALUE_BOOL)
			{
				details->value_bool = (!!mixer_setcontrol_value_bool.fValue);
				return &(details->value_bool);
			}
	}

	return NULL;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_mixerscale_menu_set
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void agenttype_mixerscale_menu_set(Menu *m, control *c, agent *a,  char *action, int controlformat)
{
	agenttype_mixer_menu_devices(m, c, action, "MixerScale", CONTROL_FORMAT_SCALE); 
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_mixerbool_menu_set
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void agenttype_mixerbool_menu_set(Menu *m, control *c, agent *a,  char *action, int controlformat)
{
	agenttype_mixer_menu_devices(m, c, action, "MixerBool", CONTROL_FORMAT_BOOL);   
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_mixer_menu_context
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void agenttype_mixer_menu_context(Menu *m, agent *a)
{
	make_menuitem_nop(m, "No options available.");
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_mixer_notifytype
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void agenttype_mixer_notifytype(int notifytype, void *messagedata)
{

}

//##################################################
//agenttype_mixer_recieverevent
//##################################################
LRESULT CALLBACK agenttype_mixer_recieverevent(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg == MM_MIXM_CONTROL_CHANGE)
	{       
		//When there's a volume change or something
		agent *a = (agent *) GetProp(hwnd, "mixagtptr");
		if (a)
		{           
			agenttype_mixer_details *details = (agenttype_mixer_details *) a->agentdetails;
			//If this is the right control
			//if (details->control == (ULONG) lParam)
			if (details->control == lParam)
			{               
				control_notify(a->controlptr, NOTIFY_NEEDUPDATE, NULL);
			}
		}
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

//##################################################
//agenttype_mixer_menu_linecontrols
//##################################################
int agenttype_mixer_menu_linecontrols(Menu *menu, control *c, char *action, char *agentname, int format, UINT device, HMIXER mixer_handle, MIXERLINE &mixer_line)
{
	//Variables
	MIXERLINECONTROLS   mixer_linecontrols;
	MIXERCONTROL        mixer_controls[8];
	int elementcount = 0;
	char text_item[256];
	char text_params[256];
	const char *type;

	//Setup the function call with required values
	UINT count_control = mixer_line.cControls;
	if (count_control > 8) count_control = 8;   //Maximum 8 controls per line
	mixer_linecontrols.cbStruct = sizeof(MIXERLINECONTROLS);
	mixer_linecontrols.cControls = count_control;
	mixer_linecontrols.dwLineID = mixer_line.dwLineID;
	mixer_linecontrols.pamxctrl = &mixer_controls[0];
	mixer_linecontrols.cbmxctrl = sizeof(MIXERCONTROL);

	//Figure out the type
	if (format == CONTROL_FORMAT_SCALE)
	{
		type = mixer_name_scale;
	}
	else if (format == CONTROL_FORMAT_BOOL)
	{
		type = mixer_name_bool;
	}
	else return 0;

	//Get the line controls
	if (MMSYSERR_NOERROR !=  mixerGetLineControls((HMIXEROBJ)mixer_handle, &mixer_linecontrols, MIXER_GETLINECONTROLSF_ALL))
	{
		return 0;
	}

	//For every control
	for (unsigned int control = 0; control < count_control; control++)
	{
		bool passestest = false;
		switch (format)
		{
			case CONTROL_FORMAT_SCALE:
				passestest = (mixer_controls[control].dwControlType == MIXERCONTROL_CONTROLTYPE_VOLUME
								|| mixer_controls[control].dwControlType == MIXERCONTROL_CONTROLTYPE_BASS
								|| mixer_controls[control].dwControlType == MIXERCONTROL_CONTROLTYPE_TREBLE
								|| mixer_controls[control].dwControlType == MIXERCONTROL_CONTROLTYPE_FADER);
				

				break;
			case CONTROL_FORMAT_BOOL:
				passestest = (mixer_controls[control].dwControlType == MIXERCONTROL_CONTROLTYPE_BOOLEAN
								|| mixer_controls[control].dwControlType == MIXERCONTROL_CONTROLTYPE_BUTTON
								|| mixer_controls[control].dwControlType == MIXERCONTROL_CONTROLTYPE_LOUDNESS
								|| mixer_controls[control].dwControlType == MIXERCONTROL_CONTROLTYPE_MONO
								|| mixer_controls[control].dwControlType == MIXERCONTROL_CONTROLTYPE_MUTE
								|| mixer_controls[control].dwControlType == MIXERCONTROL_CONTROLTYPE_ONOFF
								|| mixer_controls[control].dwControlType == MIXERCONTROL_CONTROLTYPE_STEREOENH);
				break;
		}
		if (passestest)

		{
			elementcount++;
			sprintf(text_item, "- %s", mixer_controls[control].szName);
			sprintf(text_params, "%d %d %d", device, (int)mixer_line.dwLineID, (int)mixer_controls[control].dwControlID);         
			make_menuitem_cmd(menu, text_item, config_getfull_control_setagent_c(c, action, type, text_params));
		}
	}

	return elementcount;
}


//##################################################
//agenttype_mixer_menu_sourcelines
//##################################################
void agenttype_mixer_menu_sourcelines(Menu *menu, control *c, char *action, char *agentname, int format, UINT device, HMIXER mixer_handle, MIXERLINE &mixer_destline)
{
	//Variables
	MIXERLINE mixer_line;

	//Count the devices
	UINT count_sourcelines = mixer_destline.cConnections;
	if (count_sourcelines < 1)
	{
		make_menuitem_nop(menu, "No source lines available.");
		return;
	}

	//For every source line
	for (UINT sourceline = 0; sourceline < count_sourcelines; sourceline++)
	{
		mixer_line.cbStruct = sizeof(MIXERLINE);
		mixer_line.dwDestination = mixer_destline.dwDestination;
		mixer_line.dwSource = sourceline;

		//Load the device capabilities
		if (MMSYSERR_NOERROR == mixerGetLineInfo((HMIXEROBJ) mixer_handle, &mixer_line,  MIXER_GETLINEINFOF_SOURCE))
		{
			//Create a submenu for source lines
			Menu *submenu;
			submenu = make_menu(mixer_line.szName, c);
			int controlcount = agenttype_mixer_menu_linecontrols(submenu, c, action, agentname, format, device, mixer_handle, mixer_line);
			if (controlcount == 0)
			{
				make_menuitem_nop(submenu, "No options available for this item.");
			}
			make_submenu_item(menu, mixer_line.szName, submenu);
		}
	}

	return;
}

//##################################################
//agenttype_mixer_menu_destlines
//##################################################
void agenttype_mixer_menu_destlines(Menu *menu, control *c, char *action, char *agentname, int format, UINT device, HMIXER mixer_handle, MIXERCAPS &mixer_capabilities)
{
	//Variables
	MIXERLINE mixer_line;

	//Count the destination lines
	UINT count_destlines = mixer_capabilities.cDestinations;
	if (count_destlines < 1)
	{
		make_menuitem_nop(menu, "No destination lines available.");
		return;
	}

	//For every destination line
	for (UINT destline = 0; destline < count_destlines; destline++)
	{
		mixer_line.dwDestination = destline;
		mixer_line.cbStruct = sizeof(MIXERLINE);
		mixerGetLineInfo((HMIXEROBJ) mixer_handle, &mixer_line, MIXER_GETLINEINFOF_DESTINATION);

		//Load the device capabilities
		if (MMSYSERR_NOERROR == mixerGetLineInfo((HMIXEROBJ) mixer_handle, &mixer_line, MIXER_GETLINEINFOF_DESTINATION))
		{           
			//Create a submenu for source lines
			Menu *submenu;
			submenu = make_menu(mixer_line.szName, c);

			//Add the controls
			int controlcount = agenttype_mixer_menu_linecontrols(submenu, c, action, agentname, format, device, mixer_handle, mixer_line);

			//Add a spacer
			if (controlcount > 0) make_menuitem_nop(submenu, "");
			
			//Add the source lines
			agenttype_mixer_menu_sourcelines(submenu, c, action, agentname, format, device, mixer_handle, mixer_line);
			make_submenu_item(menu, mixer_line.szName, submenu);
		}
	}

	return;
}

//##################################################
//agenttype_mixer_menu_devices
//##################################################
void agenttype_mixer_menu_devices(Menu *menu, control *c, char *action, char *agentname, int format)
{
	//Variables
	MIXERCAPS       mixer_capabilities;
	HMIXER          mixer_handle;

	//Count the devices
	UINT count_devices = mixerGetNumDevs();
	if (count_devices < 1)
	{
		make_menuitem_nop(menu, "No audio devices present.");
		return;
	}

	//For every device...
	for (UINT device = 0; device < count_devices; device++)
	{
		//Open the mixer
		if (MMSYSERR_NOERROR == mixerOpen(  &mixer_handle, device, 0, 0, 0)
			&& MMSYSERR_NOERROR == mixerGetDevCaps(device, &mixer_capabilities, sizeof(MIXERCAPS)))
		{
			//Create a submenu for destination lines
			Menu *submenu;
			submenu = make_menu(mixer_capabilities.szPname, c);
			agenttype_mixer_menu_destlines(submenu, c, action, agentname, format, device, mixer_handle, mixer_capabilities);
			make_submenu_item(menu, mixer_capabilities.szPname, submenu);

			//Close the mixer
			mixerClose(mixer_handle);
		}
	}
	return;
}

/*=================================================*/
