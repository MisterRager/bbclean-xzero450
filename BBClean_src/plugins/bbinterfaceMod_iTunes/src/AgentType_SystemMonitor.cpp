/*===================================================

	AGENTTYPE_SYSTEMMONITOR CODE

===================================================*/

// Global Include
#include "BBApi.h"
#include <string.h>
#include <windows.h>
//Parent Include
#include "AgentType_SystemMonitor.h"

//Includes
#include "PluginMaster.h"
#include "AgentMaster.h"
#include "Definitions.h"
#include "ConfigMaster.h"
#include "MessageMaster.h"
#include "ListMaster.h"



//Declare the function prototypes;
VOID CALLBACK agenttype_systemmonitor_timercall(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
LRESULT CALLBACK agenttype_systemmonitor_event(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void agenttype_systemmonitor_propogatenewvalues();
void agenttype_systemmonitor_updatevalue(int monitor_type);

// Some windowing variables
HWND agenttype_systemmonitor_window;
bool agenttype_systemmonitor_windowclassregistered;
HMODULE agenttype_systemmonitor_ntdllmodule;

//The function call to get system information
agenttype_systemmonitor_NtQuerySystemInformation agenttype_systemmonitor_ntquerysysteminformation;

//Local primitives
double agenttype_systemmonitor_number_processors;
long double agenttype_systemmonitor_last_system_time = 0;
long double agenttype_systemmonitor_last_idle_time = 0;
unsigned long agenttype_systemmonitor_counter;
const char agenttype_systemmonitor_timerclass[] = "BBInterfaceAgentSystemMon";

//A list of this type of agent
list *agenttype_systemmonitor_agents;
bool agenttype_systemmonitor_hastimer = false;

//Declare the number of types
#define SYSTEMMONITOR_NUMTYPES 9

//Array of string types - must have SYSTEMMONITOR_NUMTYPES entries
enum SYSTEMMONITOR_TYPE
{
	SYSTEMMONITOR_TYPE_NONE = 0,
	SYSTEMMONITOR_TYPE_CPUUSAGE,
	SYSTEMMONITOR_TYPE_PHYSMEMFREE,
	SYSTEMMONITOR_TYPE_PHYSMEMUSED,
	SYSTEMMONITOR_TYPE_VIRTMEMFREE,
	SYSTEMMONITOR_TYPE_VIRTMEMUSED,
	SYSTEMMONITOR_TYPE_PAGEFILEFREE,
	SYSTEMMONITOR_TYPE_PAGEFILEUSED,
	SYSTEMMONITOR_TYPE_BATTERYPOWER
};

//Must match the enum ordering above! Must have SYSTEMMONITOR_NUMTYPES entries
const char *agenttype_systemmonitor_types[] =
{
	"None", // Unused
	"CPUUsage",
	"PhysicalMemoryFree",
	"PhysicalMemoryUsed",
	"VirtualMemoryFree",
	"VirtualMemoryUsed",
	"PageFileFree",
	"PageFileUsed",
	"BatteryPower"
};

//Must match the enum ordering above! Must have SYSTEMMONITOR_NUMTYPES entries
const char *agenttype_systemmonitor_friendlytypes[] =
{
	"None", // Unused
	"CPU Usage",
	"Physical Memory Free",
	"Physical Memory Used",
	"Virtual Memory Free",
	"Virtual Memory Used",
	"Page File Free",
	"Page File Used",
	"Battery Power"
};

bool is2kxp = false;

//Must be the same size as the above array and enum
double agenttype_systemmonitor_previous_values[SYSTEMMONITOR_NUMTYPES];
double agenttype_systemmonitor_values[SYSTEMMONITOR_NUMTYPES];

char agenttype_systemmonitor_textvalues[SYSTEMMONITOR_NUMTYPES][32];

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//controltype_button_startup
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int agenttype_systemmonitor_startup()
{
	//This code stolen from the bb4win SDK example


	OSVERSIONINFO osvinfo;
	ZeroMemory(&osvinfo, sizeof(osvinfo));
	osvinfo.dwOSVersionInfoSize = sizeof (osvinfo);
	GetVersionEx(&osvinfo);
	is2kxp =	osvinfo.dwPlatformId == VER_PLATFORM_WIN32_NT;

	//Initialize the usages to 0.0 to start
	for (int i = 0; i < SYSTEMMONITOR_NUMTYPES; i++)
	{
		agenttype_systemmonitor_previous_values[i] = -1.0;
		agenttype_systemmonitor_values[i] = 0.0;
		strcpy(agenttype_systemmonitor_textvalues[i], "0%");
	}

	//Create the list
	agenttype_systemmonitor_agents = list_create();

	//Register the window class
	agenttype_systemmonitor_windowclassregistered = false;
	if (window_helper_register(agenttype_systemmonitor_timerclass, &agenttype_systemmonitor_event))
	{
		//Couldn't register the window
		BBMessageBox(NULL, "failed on register class", "test", MB_OK);
		return 1;
	}
	agenttype_systemmonitor_windowclassregistered = true;

	//Create the window
	agenttype_systemmonitor_window = window_helper_create(agenttype_systemmonitor_timerclass);
	if (!agenttype_systemmonitor_window)
	{
		//Couldn't create the window
		BBMessageBox(NULL, "failed on window", "test", MB_OK);
		return 1;
	}

	//Load the library
	if(is2kxp)
	{
		agenttype_systemmonitor_ntdllmodule = NULL;
		agenttype_systemmonitor_ntdllmodule = LoadLibrary("ntdll.dll");

		//Check to make sure it loaded properly
		if (agenttype_systemmonitor_ntdllmodule == NULL)
		{
			//We couldn't load the NTDLL library
			//Return immediately
			return 1;
			BBMessageBox(NULL, "failed on ntdll", "test", MB_OK);
		}

		//Get the NtQuerySystemInformation function
		agenttype_systemmonitor_ntquerysysteminformation = NULL;
		agenttype_systemmonitor_ntquerysysteminformation = (agenttype_systemmonitor_NtQuerySystemInformation) GetProcAddress(agenttype_systemmonitor_ntdllmodule, "NtQuerySystemInformation");

		//Check to make sure we could get the function
		if (agenttype_systemmonitor_ntquerysysteminformation == NULL)
		{
			//Error - couldn't get the function call
			FreeLibrary(agenttype_systemmonitor_ntdllmodule);
			BBMessageBox(NULL, "failed on get proc address", "test", MB_OK);
			return 1;
		}


		//Get the number of processors
		BOOL status;
		SYSTEM_BASIC_INFORMATION basicinformation;
		status = agenttype_systemmonitor_ntquerysysteminformation(agenttype_systemmonitor_SystemBasicInformation, &basicinformation, sizeof(basicinformation), NULL);
		if (status != NO_ERROR)
		{
			//Couldn't get the number of processors
			FreeLibrary(agenttype_systemmonitor_ntdllmodule);
			BBMessageBox(NULL, "failed on processor", "test", MB_OK);
			return 1;
		}

		//Record the number of processors
		agenttype_systemmonitor_number_processors = basicinformation.bKeNumberProcessors;

		//If it is less than 1 or more than 64... assume an error (I don't think any super clusters are running BBI)
		if (agenttype_systemmonitor_number_processors < 1 || agenttype_systemmonitor_number_processors > 64)
		{
			FreeLibrary(agenttype_systemmonitor_ntdllmodule);
			BBMessageBox(NULL, "failed on number of processors", "test", MB_OK);
			return 1;
		}
	}

		//If we got this far, we can successfully use this function
		//Register this type with the AgentMaster
	agent_registertype(
		"System Monitor",                   //Friendly name of agent type
		"SystemMonitor",                    //Name of agent type
		CONTROL_FORMAT_SCALE|CONTROL_FORMAT_TEXT,				//Control type
		false,
		&agenttype_systemmonitor_create,
		&agenttype_systemmonitor_destroy,
		&agenttype_systemmonitor_message,
		&agenttype_systemmonitor_notify,
		&agenttype_systemmonitor_getdata,
		&agenttype_systemmonitor_menu_set,
		&agenttype_systemmonitor_menu_context,
		&agenttype_systemmonitor_notifytype
		);

	//No errors
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_systemmonitor_shutdown
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int agenttype_systemmonitor_shutdown()
{
	if(agenttype_systemmonitor_hastimer){
		agenttype_systemmonitor_hastimer = false;
		KillTimer(agenttype_systemmonitor_window, 0);
	}
	//Destroy the internal tracking list
	if (agenttype_systemmonitor_agents) list_destroy(agenttype_systemmonitor_agents);
	//Destroy the window
	if (agenttype_systemmonitor_window) window_helper_destroy(agenttype_systemmonitor_window);

	//Unregister the window class
	if (agenttype_systemmonitor_windowclassregistered) window_helper_unregister(agenttype_systemmonitor_timerclass);

	//If we have the library, free it
	if (agenttype_systemmonitor_ntdllmodule != NULL)
	{
		FreeLibrary(agenttype_systemmonitor_ntdllmodule);
	}

	//No errors
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_systemmonitor_create
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int agenttype_systemmonitor_create(agent *a, char *parameterstring)
{


	if (0 == * parameterstring)
		return 2; // no param, no agent

	//Find the monitor type
	int monitor_type = SYSTEMMONITOR_TYPE_NONE;
	for (int i = 1; i < SYSTEMMONITOR_NUMTYPES; i++)
	{
		if (stricmp(agenttype_systemmonitor_types[i], parameterstring) == 0)
		{
			monitor_type = i;
			break;
		}
	}

	//If we didn't find a correct monitor type
	if (monitor_type == SYSTEMMONITOR_TYPE_NONE)
	{
		//On an error
		if (!plugin_suppresserrors)
		{
			char buffer[1000];
			sprintf(buffer,	"There was an error setting the System Monitor agent:\n\nType \"%s\" is not a valid type.", parameterstring);
			BBMessageBox(NULL, buffer, szAppName, MB_OK|MB_SYSTEMMODAL);
		}
		return 1;
	}


	//Create the details
	agenttype_systemmonitor_details *details = new agenttype_systemmonitor_details;
	details->monitor_type = monitor_type;

	//Create a unique string to assign to this (just a number from a counter)
	char identifierstring[64];
	sprintf(identifierstring, "%ul", agenttype_systemmonitor_counter);
	details->internal_identifier = new_string(identifierstring);

	//Set the details
	a->agentdetails = (void *)details;

	//Add this to our internal tracking list
	agent *oldagent; //Unused, but we have to pass it
	list_add(agenttype_systemmonitor_agents, details->internal_identifier, (void *) a, (void **) &oldagent);

	//Increment the counter
	agenttype_systemmonitor_counter++;

	if (!agenttype_systemmonitor_hastimer)
	{
		SetTimer(agenttype_systemmonitor_window, 0, 1000, agenttype_systemmonitor_timercall);
		agenttype_systemmonitor_hastimer = true;
	}

	//No errors
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_systemmonitor_destroy
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int agenttype_systemmonitor_destroy(agent *a)
{
	if (a->agentdetails)
	{
		agenttype_systemmonitor_details *details = (agenttype_systemmonitor_details *) a->agentdetails;

		//Delete from the tracking list
		list_remove(agenttype_systemmonitor_agents, details->internal_identifier);

		//Free the string
		free_string(&details->internal_identifier);

		delete details;
		a->agentdetails = NULL;
	}

	//No errors
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_systemmonitor_message
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int agenttype_systemmonitor_message(agent *a, int tokencount, char *tokens[])
{
	//No errors
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_systemmonitor_notify
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void agenttype_systemmonitor_notify(agent *a, int notifytype, void *messagedata)
{
	//Get the agent details
	agenttype_systemmonitor_details *details = (agenttype_systemmonitor_details *) a->agentdetails;

	switch(notifytype)
	{
		case NOTIFY_CHANGE:
			control_notify(a->controlptr, NOTIFY_NEEDUPDATE, NULL);
			break;

		case NOTIFY_SAVE_AGENT:
			//Write existance
			config_write(config_get_control_setagent_c(a->controlptr, a->agentaction, a->agenttypeptr->agenttypename, agenttype_systemmonitor_types[details->monitor_type]));
			break;
	}
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_systemmonitor_getdata
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void *agenttype_systemmonitor_getdata(agent *a, int datatype)
{
	//Get the agent details
	agenttype_systemmonitor_details *details = (agenttype_systemmonitor_details *) a->agentdetails;

	switch (datatype)
	{
		case DATAFETCH_VALUE_SCALE:
			return &agenttype_systemmonitor_values[details->monitor_type];
			break;
		case DATAFETCH_VALUE_TEXT:
			return &agenttype_systemmonitor_textvalues[details->monitor_type];
	}

	return NULL;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_systemmonitor_menu_set
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void agenttype_systemmonitor_menu_set(Menu *m, control *c, agent *a,  char *action, int controlformat)
{
	//Add a menu item for every type
	for (int i = 1; i < SYSTEMMONITOR_NUMTYPES; i++)
	{
		make_menuitem_cmd(m, agenttype_systemmonitor_friendlytypes[i], config_getfull_control_setagent_c(c, action, "SystemMonitor", agenttype_systemmonitor_types[i]));
	}
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_systemmonitor_menu_context
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void agenttype_systemmonitor_menu_context(Menu *m, agent *a)
{
	make_menuitem_nop(m, "No options available.");
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_systemmonitor_notifytype
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void agenttype_systemmonitor_notifytype(int notifytype, void *messagedata)
{

}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_systemmonitor_timercall
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
VOID CALLBACK agenttype_systemmonitor_timercall(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	//If there are agents left
	if (agenttype_systemmonitor_agents->first != NULL)
	{
		agenttype_systemmonitor_propogatenewvalues();
	}
	else
	{
		agenttype_systemmonitor_hastimer = false;
		KillTimer(hwnd, 0);
	}
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_systemmonitor_event
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
LRESULT CALLBACK agenttype_systemmonitor_event(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_systemmonitor_propogatenewvalues
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void agenttype_systemmonitor_propogatenewvalues()
{
	//Declare variables
	agent *currentagent;

	//Reset all values to unknown
	for (int i = 0; i < SYSTEMMONITOR_NUMTYPES; i++)
	{
		agenttype_systemmonitor_previous_values[i] = agenttype_systemmonitor_values[i];
		agenttype_systemmonitor_values[i] = -1.0;
	}

	//Go through every agent
	listnode *currentnode;
	dolist(currentnode, agenttype_systemmonitor_agents)
	{
		//Get the agent
		currentagent = (agent *) currentnode->value;

		//Get the monitor type
		int monitor_type = ((agenttype_systemmonitor_details *) (currentagent->agentdetails))->monitor_type;

		//Calculate a new value for this type
		agenttype_systemmonitor_updatevalue(monitor_type);

		//Tell the agent to update itself if the value has changed

		if (agenttype_systemmonitor_previous_values[monitor_type] != agenttype_systemmonitor_values[monitor_type])
		{
			control_notify(currentagent->controlptr, NOTIFY_NEEDUPDATE, NULL);
		}
	}
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_systemmonitor_getvalue
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void agenttype_systemmonitor_updatevalue(int monitor_type)
{
	//Multipurpose value
	double foundvalue;
	//If we already know its value, then return it
	if (agenttype_systemmonitor_values[monitor_type] != -1) return;

	//Get the memory info
	MEMORYSTATUS memstatus;
	GlobalMemoryStatus(&memstatus);

	//Otherwise, figure it out
	switch(monitor_type)
	{
		//CPU Monitoring type
		case SYSTEMMONITOR_TYPE_CPUUSAGE:
		if (is2kxp)
		{
				//Variables
				long double diff_system_time;
				long double diff_idle_time;
				BOOL status;
				SYSTEM_TIME_INFORMATION SysTimeInfo;
				SYSTEM_PERFORMANCE_INFORMATION SysPerfInfo;

				//Get metrics
				status = agenttype_systemmonitor_ntquerysysteminformation(agenttype_systemmonitor_SystemTimeInformation, &SysTimeInfo, sizeof(SysTimeInfo), NULL );
				if (status == NO_ERROR) status = agenttype_systemmonitor_ntquerysysteminformation(agenttype_systemmonitor_SystemPerformanceInformation, &SysPerfInfo, sizeof(SysPerfInfo), NULL );

				//If we succeeded
				if (status == NO_ERROR)
				{
					//Convert to long doubles
					long double current_system_time = (long double) SysTimeInfo.liKeSystemTime.QuadPart;
					long double current_idle_time = (long double) SysPerfInfo.liIdleTime.QuadPart;

					//If this is the first time
					if (agenttype_systemmonitor_last_system_time == 0)
					{
						foundvalue = 0.0;
					}
					else
					{
						//Calculate difference
						diff_system_time = current_system_time - agenttype_systemmonitor_last_system_time;
						diff_idle_time = current_idle_time - agenttype_systemmonitor_last_idle_time;

						//Calculate usage
						foundvalue = 1.0 - (((diff_idle_time / diff_system_time)) / agenttype_systemmonitor_number_processors);
						if (foundvalue < 0.0) foundvalue = 0.0;
						if (foundvalue > 1.0) foundvalue = 1.0;
					}

					//Set variables for next time
					agenttype_systemmonitor_last_system_time = current_system_time;
					agenttype_systemmonitor_last_idle_time = current_idle_time;

					//Set what we got
					agenttype_systemmonitor_values[SYSTEMMONITOR_TYPE_CPUUSAGE] = foundvalue;
				}
			break;
		}
		else
		{
			HKEY hkey;
			DWORD dwDataSize;
			DWORD dwType;
			DWORD dwCpuUsage;
			

			RegOpenKeyEx(HKEY_DYN_DATA, "PerfStats\\StatData", 0, KEY_QUERY_VALUE, &hkey);
			dwDataSize = sizeof(dwCpuUsage);
			RegQueryValueEx(hkey, "KERNEL\\CPUUsage", NULL, &dwType, (LPBYTE)&dwCpuUsage, &dwDataSize);
			RegCloseKey(hkey);

			agenttype_systemmonitor_values[SYSTEMMONITOR_TYPE_CPUUSAGE] = ((double)dwCpuUsage)/100;
			break;
		}
			//Any of the easy to get memory information types
			case SYSTEMMONITOR_TYPE_PHYSMEMFREE:
				agenttype_systemmonitor_values[SYSTEMMONITOR_TYPE_PHYSMEMFREE] = (100-memstatus.dwMemoryLoad) / 100.0;
				break;
			case SYSTEMMONITOR_TYPE_PHYSMEMUSED:
				agenttype_systemmonitor_values[SYSTEMMONITOR_TYPE_PHYSMEMUSED] = memstatus.dwMemoryLoad / 100.0;
				break;
			case SYSTEMMONITOR_TYPE_VIRTMEMFREE:
				agenttype_systemmonitor_values[SYSTEMMONITOR_TYPE_VIRTMEMFREE] = memstatus.dwAvailVirtual/(double) memstatus.dwTotalVirtual;
				break;
			case SYSTEMMONITOR_TYPE_VIRTMEMUSED:
				agenttype_systemmonitor_values[SYSTEMMONITOR_TYPE_VIRTMEMUSED] = 1.0 - (memstatus.dwAvailVirtual/(double) memstatus.dwTotalVirtual);
				break;
			case SYSTEMMONITOR_TYPE_PAGEFILEFREE:
				agenttype_systemmonitor_values[SYSTEMMONITOR_TYPE_PAGEFILEFREE] = memstatus.dwAvailPageFile/(double) memstatus.dwTotalPageFile;
				break;
			case SYSTEMMONITOR_TYPE_PAGEFILEUSED:				
				agenttype_systemmonitor_values[SYSTEMMONITOR_TYPE_PAGEFILEUSED] = 1.0 - (memstatus.dwAvailPageFile/(double) memstatus.dwTotalPageFile);
				break;

			case SYSTEMMONITOR_TYPE_BATTERYPOWER:
				SYSTEM_POWER_STATUS powerstatus;
				GetSystemPowerStatus(&powerstatus);

				foundvalue = powerstatus.BatteryLifePercent / 100.0;
				if (foundvalue < 0.0) foundvalue = 0.0;
				else if (foundvalue > 1.0) foundvalue = 1.0;
				agenttype_systemmonitor_values[SYSTEMMONITOR_TYPE_BATTERYPOWER] = foundvalue;
				break;
		default:
			agenttype_systemmonitor_values[monitor_type] = 0.0; //Should never happen
			break;
	}

	//In all monitor type cases, copy the double value to the string value
	int intvalue = (int)(100*agenttype_systemmonitor_values[monitor_type]);
	sprintf(agenttype_systemmonitor_textvalues[monitor_type], "%d%%", intvalue);
	//We now know the value - return
	return;
}

