/*===================================================

	AGENTTYPE_SYSTEMMONITOR HEADERS

===================================================*/

//Multiple definition prevention
#ifndef BBInterface_AgentType_SystemMonitor_h
#define BBInterface_AgentType_SystemMonitor_h

//Includes
#include "AgentMaster.h"

//Define these structures
struct agenttype_systemmonitor_details
{
	char *internal_identifier;
	int monitor_type;
};

struct SYSTEM_BASIC_INFORMATION
{
	DWORD dwUnknown1;
	ULONG uKeMaximumIncrement;
	ULONG uPageSize;
	ULONG uMmNumberOfPhysicalPages;
	ULONG uMmLowestPhysicalPage;
	ULONG uMmHighestPhysicalPage;
	ULONG uAllocationGranularity;
	PVOID pLowestUserAddress;
	PVOID pMmHighestUserAddress;
	ULONG uKeActiveProcessors;
	BYTE bKeNumberProcessors;
	BYTE bUnknown2;
	WORD wUnknown3;
};

struct SYSTEM_PERFORMANCE_INFORMATION
{
	LARGE_INTEGER liIdleTime;
	DWORD dwSpare[76];
};

struct SYSTEM_TIME_INFORMATION
{
	LARGE_INTEGER liKeBootTime;
	LARGE_INTEGER liKeSystemTime;
	LARGE_INTEGER liExpTimeZoneBias;
	ULONG uCurrentTimeZoneId;
	DWORD dwReserved;
};

//Define these function pointer types
typedef BOOL (__stdcall *agenttype_systemmonitor_NtQuerySystemInformation)(DWORD SystemInformationClass, PVOID SystemInformation, ULONG SystemInformationLength, PULONG ReturnLength);

//Defines from the original 
const int agenttype_systemmonitor_SystemBasicInformation = 0;
const int agenttype_systemmonitor_SystemPerformanceInformation = 2;
const int agenttype_systemmonitor_SystemTimeInformation = 3;

//Define these functions internally

int agenttype_systemmonitor_startup();
int agenttype_systemmonitor_shutdown();

int     agenttype_systemmonitor_create(agent *a, char *parameterstring);
int     agenttype_systemmonitor_destroy(agent *a);
int     agenttype_systemmonitor_message(agent *a, int tokencount, char *tokens[]);
void    agenttype_systemmonitor_notify(agent *a, int notifytype, void *messagedata);
void*   agenttype_systemmonitor_getdata(agent *a, int datatype);
void    agenttype_systemmonitor_menu_set(Menu *m, control *c, agent *a,  char *action, int controlformat);
void    agenttype_systemmonitor_menu_context(Menu *m, agent *a);
void    agenttype_systemmonitor_notifytype(int notifytype, void *messagedata);

#endif
/*=================================================*/
