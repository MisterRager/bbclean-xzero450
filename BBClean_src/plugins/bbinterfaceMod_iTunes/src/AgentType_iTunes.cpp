/*===================================================

	AGENTTYPE_ITUNES CODE

===================================================*/

// Global Include

#include "BBApi.h"

#include <string.h>
#include <stdlib.h>
#include <windows.h>
#include <string>
#include <comutil.h>
#include <tlhelp32.h>
#include <GdiPlus.h>

#ifndef _ATL_CSTRING_EXPLICIT_CONSTRUCTORS
#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS
#endif
#include <atlbase.h>
#include <atlstr.h>
#include <atlcom.h>

#undef GetFreeSpace
//Parent Include
#include "iTunesCOMInterface.h"
#include "iTunesCOMInterface_i.c"
#include "AgentType_iTunes.h"

//Includes
#include "PluginMaster.h"
#include "StyleMaster.h"
#include "AgentMaster.h"
#include "Definitions.h"
#include "ConfigMaster.h"
#include "MenuMaster.h"
#include "ListMaster.h"

#define set_time(s,x) _snprintf(s,sizeof(s),"%d:%02d",x/60,x%60)
#define is2byte(x) ((x>=0x81 && x<=0x9F)||(x>=0xE0 && x<=0xFC))
#define selectchar(x,y) (strlen(x)>0)?x:y
#define vnot(x) (x==VARIANT_TRUE)?VARIANT_FALSE:VARIANT_TRUE


//Local variables
const char agenttype_itunes_timerclass[] = "BBInterfaceAgentiTunes";
bool agenttype_itunes_windowclassregistered;
LRESULT CALLBACK agenttype_itunes_event(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
VOID CALLBACK agenttype_itunes_timercall(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
HWND agenttype_itunes_window;
bool agenttype_itunes_hastimer = false;

void initialize_iTunes();
void delete_iTunes();
bool isRunningiTunes();
void initialize_iTunes_Properties();
void initialize_iTunes_Properties(bool reset);
HRESULT GetIDOfName(IDispatch* pDisp, OLECHAR* wszName, DISPID* pdispID);
HRESULT GetProperty(IDispatch* pDisp, OLECHAR* wszName, VARIANT* pvResult);
HRESULT GetProperty(IDispatch* pDisp, OLECHAR* wszName, DISPPARAMS dispParams , VARIANT* pvResult);
HRESULT GetiTrackProperties(IDispatch *idisp,agenttype_itunes_itrackproperties *ip,bool *changed);
HRESULT GetiTrackProperties(IITTrack* iTrack,agenttype_itunes_itrackproperties* ip,bool *changed);
HRESULT GetiTrackArtworks(IDispatch* idisp);
unsigned long agenttype_itunes_counter;
list *agenttype_itunes_agents;

//The pieces of data that might be fetched

agenttype_itunes_itrackproperties agenttype_itunes_itrack_properties;
double agenttype_itunes_trackposition = 0.0;
double agenttype_itunes_rating = 0.0;
char agenttype_itunes_ratingtext[2] = "";
char agenttype_itunes_timeelapsed[16] = "";
char agenttype_itunes_timeremaining[16] = "";
bool agenttype_itunes_isplaying = false;
bool agenttype_itunes_isrewinding = false;
bool agenttype_itunes_isfastforwarding = false;
bool agenttype_itunes_isminimized = false;
bool agenttype_itunes_isshuffle = false;
bool agenttype_itunes_isrepeatone = false;
bool agenttype_itunes_isrepeatall = false;
long agenttype_itunes_volume=0;
double agenttype_itunes_dvolume=0;
char agenttype_itunes_itrack_scroll_title[1028] = "";
char agenttype_itunes_itrack_scroll_artist[1028] = "";
char agenttype_itunes_itrack_scroll_album[1028] = "";
bool gdi_initialized = false;
bool com_disabled = false;
bool trackchanged = false;
char agenttype_itunes_artworkpath[1024];

//Function prototypes
void agenttype_itunespoller_updatevalues();

Gdiplus::GdiplusStartupInput gdiplusStartupInputiTunes;
ULONG_PTR gdiplusTokeniTunes;
Gdiplus::ImageAttributes *imageAttriTunes;
Gdiplus::Graphics *graphicsiTunes;
Gdiplus::Image *pImageiTunes;

//Definitions
#define ITUNES_POLLINGTYPECOUNT 23
#define ITUNES_POLLINGTYPE_LASTBOOL 8
#define ITUNES_POLLINGTYPE_LASTSCALE 11

 enum ITUNES_POLLINGTYPE
{
	ITUNES_POLLINGTYPE_NONE=0,
	ITUNES_POLLINGTYPE_ISPLAYING,
	ITUNES_POLLINGTYPE_ISREWINDING,
	ITUNES_POLLINGTYPE_ISFASTFORWARDING,
	ITUNES_POLLINGTYPE_ISMINIMIZED,
	ITUNES_POLLINGTYPE_ISSHUFFLE,
	ITUNES_POLLINGTYPE_ISREPEATONE,
	ITUNES_POLLINGTYPE_ISREPEATALL,
	ITUNES_POLLINGTYPE_HASARTWORK,
	ITUNES_POLLINGTYPE_POSITION,
	ITUNES_POLLINGTYPE_VOLUME,
	ITUNES_POLLINGTYPE_RATING,
	ITUNES_POLLINGTYPE_TITLE,
	ITUNES_POLLINGTYPE_ARTIST,
	ITUNES_POLLINGTYPE_ALBUM,
	ITUNES_POLLINGTYPE_TITLESCROLL,
	ITUNES_POLLINGTYPE_ARTISTSCROLL,
	ITUNES_POLLINGTYPE_ALBUMSCROLL,
	ITUNES_POLLINGTYPE_TIME_ELAPSED,
	ITUNES_POLLINGTYPE_TIME_REMAINING,
	ITUNES_POLLINGTYPE_TIME_TOTAL,
	ITUNES_POLLINGTYPE_BITRATE,
	ITUNES_POLLINGTYPE_SAMPLERATE
};
const char *agenttype_itunes_pollingnames[ITUNES_POLLINGTYPECOUNT] =
{
	"None",
	"IsPlaying",
	"IsRewinding",
	"IsFastForwarding",
	"IsMinimized",
	"IsShuffle",
	"IsRepeatOne",
	"IsRepeatAll",
	"HasArtwork",
	"TrackPosition",
	"VolumeValue",
	"Rating",
	"TrackTitle",
	"TrackArtist",
	"TrackAlbum",
	"TrackTitleAutoScroll",
	"TrackArtistAutoScroll",
	"TrackAlbumAutoScroll",
	"TimeElapsed",
	"TimeRemaining",
	"TimeTotal",
	"Bitrate",
	"Samplerate"
};

const char *agenttype_itunes_friendlynames[ITUNES_POLLINGTYPECOUNT] =
{
	"None",
	"Track Is Playing",
	"Track Is Rewinding",
	"Track Is FastForwarding",
	"Is Minimized",
	"Is Shuffle",
	"Is Repeat Song",
	"Is Repeat Playlist",
	"Track Has Artwork",
	"Track Position",
	"Volume Value",
	"Rating",
	"Track Title",
	"Artist Name Of Current Track",
	"Album Name Of Current Track",
	"Track Title (Scrolling)",
	"Artist Name (Scrolling)",
	"Album Name (Scrolling)",
	"Time Elapsed",
	"Time Remaining",
	"Time Total",
	"Bitrate",
	"Samplerate"
};

#define ITUNES_ACTIONTYPECOUNT 21
char *itunes_actions[ITUNES_ACTIONTYPECOUNT] ={
	"Start iTunes(Only Launch)",
	"Quit iTunes",
	"Previous Track",
	"Next Track",
	"Play",
	"Stop",
	"Pause",
	"Play/Pause Toggle",
	"Rewind",
	"Fast Forward",
	"Mute/Unmute Toggle",
	"Volume Up 1%",
	"Volume Down 1%",
	"Minimize/Restore Window",
	"Shuffle Play Toggle",
	"RepeatMode (Off->One->All)",
	"Update iPod",
	"Rescroll Track Title",
	"Rescroll Artist Name",
	"Rescroll Album Name",
	"Resume FastForward/Rewind"
};

enum ITUNES_ACTIONTYPE
{
	ITUNES_ACTIONTYPE_START = 0,
	ITUNES_ACTIONTYPE_QUIT,
	ITUNES_ACTIONTYPE_PREVIOUS,
	ITUNES_ACTIONTYPE_NEXT,
	ITUNES_ACTIONTYPE_PLAY,
	ITUNES_ACTIONTYPE_STOP,
	ITUNES_ACTIONTYPE_PAUSE,
	ITUNES_ACTIONTYPE_PPTOGGLE,
	ITUNES_ACTIONTYPE_REWIND,
	ITUNES_ACTIONTYPE_FASTFORWARD,
	ITUNES_ACTIONTYPE_MUTE,
	ITUNES_ACTIONTYPE_VOLUMEUP,
	ITUNES_ACTIONTYPE_VOLUMEDOWN,
	ITUNES_ACTIONTYPE_MINIMIZE,
	ITUNES_ACTIONTYPE_SHUFFLE,
	ITUNES_ACTIONTYPE_REPEAT,
	ITUNES_ACTIONTYPE_UPDATEIPOD,
	ITUNES_ACTIONTYPE_RESCROLLTITLE,
	ITUNES_ACTIONTYPE_RESCROLLARTIST,
	ITUNES_ACTIONTYPE_RESCROLLALBUM,
	ITUNES_ACTIONTYPE_RESUME

};

/* iTunesEvents : sink class*/
#define SINKID_iTunesEvents 0
class ATL_NO_VTABLE iTunesEvents : public CComObjectRootEx<CComSingleThreadModel>, public IDispEventImpl<SINKID_iTunesEvents, iTunesEvents, &DIID__IiTunesEvents,&LIBID_iTunesLib, 1, 7>
{
public:
	iTunesEvents() {}
	BEGIN_COM_MAP(iTunesEvents)
		COM_INTERFACE_ENTRY_IID(DIID__IiTunesEvents, iTunesEvents)
	END_COM_MAP()
	BEGIN_SINK_MAP(iTunesEvents)
		SINK_ENTRY_EX(SINKID_iTunesEvents, DIID__IiTunesEvents,ITEventDatabaseChanged,&handle_OnDatabaseChangedEvent)
		SINK_ENTRY_EX(SINKID_iTunesEvents, DIID__IiTunesEvents,ITEventPlayerPlay,&handle_OnPlayerPlayEvent)
		SINK_ENTRY_EX(SINKID_iTunesEvents, DIID__IiTunesEvents,ITEventPlayerStop,&handle_OnPlayerStopEvent)
		SINK_ENTRY_EX(SINKID_iTunesEvents, DIID__IiTunesEvents,ITEventPlayerPlayingTrackChanged,&handle_OnPlayerPlayingTrackChangedEvent)
		SINK_ENTRY_EX(SINKID_iTunesEvents, DIID__IiTunesEvents,ITEventUserInterfaceEnabled,&handle_OnUserInterfaceEnabledEvent)
		SINK_ENTRY_EX(SINKID_iTunesEvents, DIID__IiTunesEvents,ITEventCOMCallsDisabled,&handle_OnCOMCallsDisabledEvent)
		SINK_ENTRY_EX(SINKID_iTunesEvents, DIID__IiTunesEvents,ITEventCOMCallsEnabled,&handle_OnCOMCallsEnabledEvent)
		SINK_ENTRY_EX(SINKID_iTunesEvents, DIID__IiTunesEvents,ITEventQuitting,&handle_OnQuittingEvent)
		SINK_ENTRY_EX(SINKID_iTunesEvents, DIID__IiTunesEvents,ITEventAboutToPromptUserToQuit,&handle_OnAboutToPromptUserToQuitEvent)
		SINK_ENTRY_EX(SINKID_iTunesEvents, DIID__IiTunesEvents,ITEventSoundVolumeChanged,&handle_OnSoundVolumeChangedEvent)
	END_SINK_MAP()
	virtual void OnDatabaseChangedEvent(VARIANT deletedObjectIDs,VARIANT changedObjectIDs)=0;
	HRESULT _stdcall handle_OnDatabaseChangedEvent(VARIANT deletedObjectIDs,VARIANT changedObjectIDs){OnDatabaseChangedEvent(deletedObjectIDs,changedObjectIDs);return S_OK;}

	virtual void OnPlayerPlayEvent(VARIANT iTrack)=0;
	HRESULT _stdcall handle_OnPlayerPlayEvent(VARIANT iTrack){OnPlayerPlayEvent(iTrack);return S_OK;}

	virtual void OnPlayerStopEvent(VARIANT iTrack)=0;
	HRESULT _stdcall handle_OnPlayerStopEvent(VARIANT iTrack){OnPlayerStopEvent(iTrack);return S_OK;}

	virtual void OnPlayerPlayingTrackChangedEvent (VARIANT iTrack)=0;
	HRESULT _stdcall handle_OnPlayerPlayingTrackChangedEvent(VARIANT iTrack){OnPlayerPlayingTrackChangedEvent(iTrack);return S_OK;}

	virtual void OnUserInterfaceEnabledEvent ()=0;
	HRESULT _stdcall handle_OnUserInterfaceEnabledEvent(){OnUserInterfaceEnabledEvent();return S_OK;}

	virtual void OnCOMCallsDisabledEvent (ITCOMDisabledReason reason)=0;
	HRESULT _stdcall handle_OnCOMCallsDisabledEvent (ITCOMDisabledReason reason){OnCOMCallsDisabledEvent(reason);return S_OK;}

	virtual void OnCOMCallsEnabledEvent()=0;
	HRESULT _stdcall handle_OnCOMCallsEnabledEvent(){OnCOMCallsEnabledEvent();return S_OK;}

	virtual void OnQuittingEvent()=0;
	HRESULT _stdcall handle_OnQuittingEvent(){OnQuittingEvent();return S_OK;}

	virtual void OnAboutToPromptUserToQuitEvent()=0;
	HRESULT _stdcall handle_OnAboutToPromptUserToQuitEvent(){OnAboutToPromptUserToQuitEvent();return S_OK;}

	virtual void OnSoundVolumeChangedEvent(long newVolume)=0;
	HRESULT _stdcall handle_OnSoundVolumeChangedEvent(long newVolume){OnSoundVolumeChangedEvent(newVolume);return S_OK;}

};

CComPtr<IiTunes> iTunes;

class myiTunesEvents : public iTunesEvents {
	void OnDatabaseChangedEvent(VARIANT deletedObjectIDs,VARIANT changedObjectIDs){
	};
	void OnPlayerPlayEvent(VARIANT iTrack) {
		agenttype_itunes_isplaying = true;

		GetiTrackProperties(iTrack.pdispVal,&agenttype_itunes_itrack_properties,&trackchanged);
		//trackchanged=true;
	}
	void OnPlayerStopEvent(VARIANT iTrack) {
		agenttype_itunes_isplaying = false;
	}
	void OnPlayerPlayingTrackChangedEvent(VARIANT iTrack){
		GetiTrackProperties(iTrack.pdispVal,&agenttype_itunes_itrack_properties,&trackchanged);
		//trackchanged=true;
	};	
	void OnUserInterfaceEnabledEvent(){
	};
	void OnCOMCallsDisabledEvent (ITCOMDisabledReason reason){
		com_disabled = true;	
	};
	void OnCOMCallsEnabledEvent(){
		com_disabled = false;	
	};
	void OnQuittingEvent(){
		delete_iTunes();
	};
	void OnAboutToPromptUserToQuitEvent(){
	//if iTunes is not playing, iTunes->Release() cause error...
		iTunes->put_Mute(VARIANT_TRUE);
		iTunes->Play();
		delete_iTunes();
	};
	void OnSoundVolumeChangedEvent(long newVolume){
		agenttype_itunes_volume = newVolume;
	};
};

CComObject<myiTunesEvents> *sink;
CComPtr<IUnknown> unknown;


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_itunes_startup
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int agenttype_itunes_startup()
{
	
	//Register the window class
	agenttype_itunes_windowclassregistered = false;
	if (window_helper_register(agenttype_itunes_timerclass, &agenttype_itunes_event))
	{
		//Couldn't register the window
		return 1;
	}
	agenttype_itunes_windowclassregistered = true;

	//Create the window
	agenttype_itunes_window = window_helper_create(agenttype_itunes_timerclass);
	if (!agenttype_itunes_window)
	{
		//Couldn't create the window
		return 1;
	}

	//Create the list
	agenttype_itunes_agents = list_create();


	//Register this type with the ControlMaster
	agent_registertype(
		"iTunes",                   //Friendly name of agent type
		"iTunes",                           //Name of agent type
		CONTROL_FORMAT_TRIGGER,             //Control format
		true,
		&agenttype_itunes_create,           
		&agenttype_itunes_destroy,
		&agenttype_itunes_message,
		&agenttype_itunes_notify,
		&agenttype_itunes_getdata,
		&agenttype_itunes_menu_set,
		&agenttype_itunes_menu_context,
		&agenttype_itunes_notifytype
		);

	//Register this type with the ControlMaster
	agent_registertype(
		"iTunes",                          //Friendly name of agent type
		"iTunesScale",                      //Name of agent type
		CONTROL_FORMAT_SCALE,               //Control format
		true,
		&agenttype_itunespoller_create,           
		&agenttype_itunes_destroy,
		&agenttype_itunes_message,
		&agenttype_itunespoller_notify,
		&agenttype_itunespoller_getdata,
		&agenttype_itunespoller_scale_menu_set,
		&agenttype_itunes_menu_context,
		&agenttype_itunes_notifytype
		);

	//Register this type with the ControlMaster
	agent_registertype(
		"iTunes",                          //Friendly name of agent type
		"iTunesBool",                       //Name of agent type
		CONTROL_FORMAT_BOOL,                //Control format
		false,
		&agenttype_itunespoller_create,           
		&agenttype_itunes_destroy,
		&agenttype_itunes_message,
		&agenttype_itunespoller_notify,
		&agenttype_itunespoller_getdata,
		&agenttype_itunespoller_bool_menu_set,
		&agenttype_itunes_menu_context,
		&agenttype_itunes_notifytype
		);
	//Register this type with the ControlMaster
	agent_registertype(
		"iTunes",                          //Friendly name of agent type
		"iTunesText",                       //Name of agent type
		CONTROL_FORMAT_TEXT,                //Control format
		true,
		&agenttype_itunespoller_create,           
		&agenttype_itunes_destroy,
		&agenttype_itunes_message,
		&agenttype_itunespoller_notify,
		&agenttype_itunespoller_getdata,
		&agenttype_itunespoller_text_menu_set,
		&agenttype_itunes_menu_context,
		&agenttype_itunes_notifytype
		);

	agent_registertype(
		"iTunes",                          //Friendly name of agent type
		"iTunesImage",                       //Name of agent type
		CONTROL_FORMAT_IMAGE,                //Control format
		true,
		&agenttype_itunespoller_create,           
		&agenttype_itunes_destroy,
		&agenttype_itunes_message,
		&agenttype_itunesimage_notify,
		&agenttype_itunespoller_getdata,
		&agenttype_itunesimage_menu_set,
		&agenttype_itunes_menu_context,
		&agenttype_itunes_notifytype
		);

	GetTempPath(sizeof(agenttype_itunes_artworkpath),agenttype_itunes_artworkpath);
	strncat(agenttype_itunes_artworkpath,"bbinterfacde__tmp_artwork",sizeof(agenttype_itunes_artworkpath));
	if(!gdi_initialized){
		if(Gdiplus::GdiplusStartup(&gdiplusTokeniTunes, &gdiplusStartupInputiTunes, NULL) != 0)
		{
			MessageBox(0, "Error starting GdiPlus.dll", szAppName, MB_OK | MB_ICONERROR | MB_TOPMOST);
			return 1;
		}else{
			gdi_initialized = true;
		}
	}
	
	initialize_iTunes_Properties(true);
	//if iTunes is already Running, initialize itunes
	if(isRunningiTunes()){
		initialize_iTunes();
		initialize_iTunes_Properties();
	}

	//No errors
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_itunes_shutdown
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int agenttype_itunes_shutdown()
{
	delete_iTunes();
	
	if(agenttype_itunes_hastimer){
		agenttype_itunes_hastimer = false;	
		KillTimer(agenttype_itunes_window,0);
	}

	//Destroy the internal tracking list
	if (agenttype_itunes_agents)
		list_destroy(agenttype_itunes_agents);
	
	if(gdi_initialized){
		Gdiplus::GdiplusShutdown(gdiplusTokeniTunes);
		gdi_initialized = false;
	}
	
	//Destroy the window
	if (agenttype_itunes_window) window_helper_destroy(agenttype_itunes_window);

	//Unregister the window class
	if (agenttype_itunes_windowclassregistered) window_helper_unregister(agenttype_itunes_timerclass);
	
		
	//No errors
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_itunes_create
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int agenttype_itunes_create(agent *a, char *parameterstring)
{
	int code = atoi(parameterstring);

	//Create the details
	agenttype_itunes_details *details = new agenttype_itunes_details;
	a->agentdetails = (void *) details;

	//Copy the code
	details->commandcode = code;
	details->internal_identifier = NULL;

	//No errors
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_itunes_create
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int agenttype_itunespoller_create(agent *a, char *parameterstring)
{
	//Figure out what type of agent this is
	int commandcode = 0;	
	for (int i = 1; i < ITUNES_POLLINGTYPECOUNT; i++)
	{
		if (!stricmp(parameterstring, agenttype_itunes_pollingnames[i]))
			commandcode = i;
	}
	if(!stricmp(parameterstring,"iTunesArtwork"))
		commandcode = ITUNES_POLLINGTYPECOUNT;  //Temporary value
	if (commandcode == 0) return 1;

	//Create the details
	agenttype_itunes_details *details = new agenttype_itunes_details;
	a->agentdetails = (void *) details;

	//Create a unique string to assign to this (just a number from a counter)
	char identifierstring[64];
	sprintf(identifierstring, "%ul", agenttype_itunes_counter);
	details->internal_identifier = new_string(identifierstring);

	//Add this to our internal tracking list
	agent *oldagent; //Unused, but we have to pass it
	list_add(agenttype_itunes_agents, details->internal_identifier, (void *) a, (void **) &oldagent);

	//Copy the code
	details->commandcode = commandcode;

	if(!agenttype_itunes_hastimer)
	{
		SetTimer(agenttype_itunes_window, 0, 500, agenttype_itunes_timercall);
		agenttype_itunes_hastimer = true;	
	}

	//Increment the counter
	agenttype_itunes_counter++;

	//No errors
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_itunes_menu_context
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int agenttype_itunes_destroy(agent *a)
{
	if (a->agentdetails)
	{
		agenttype_itunes_details *details = (agenttype_itunes_details *) a->agentdetails;

		if (details->internal_identifier != NULL)
		{
			list_remove(agenttype_itunes_agents, details->internal_identifier);
			free_string(&details->internal_identifier);
		}

		delete (agenttype_itunes_details *) a->agentdetails;   
		a->agentdetails = NULL;
	}

	//No errors
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_itunes_message
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int agenttype_itunes_message(agent *a, int tokencount, char *tokens[])
{
	return 1;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_itunes_notify
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void agenttype_itunes_notify(agent *a, int notifytype, void *messagedata)
{
	//Get the agent details
	agenttype_itunes_details *details = (agenttype_itunes_details *) a->agentdetails;
	VARIANT_BOOL vbool;
	bool check;
	IITBrowserWindow *window;
	IITPlaylist *playlist;
	IITTrack *iTrack;
	ITPlaylistRepeatMode repeatmode;
	ITPlayerState state;
	switch(notifytype)
	{
		case NOTIFY_CHANGE:
			if(!com_disabled && !iTunes.p && details->commandcode != ITUNES_ACTIONTYPE_QUIT){
				check = isRunningiTunes();
				initialize_iTunes();
				if(check){
					initialize_iTunes_Properties();
				}
			}
			if(com_disabled || !iTunes.p){
				return;
			}
			switch(details->commandcode)
			{
				case ITUNES_ACTIONTYPE_START:
					//Nothing to do
					break;
				case ITUNES_ACTIONTYPE_QUIT:
					iTunes->Quit();
					break;
				case ITUNES_ACTIONTYPE_PLAY:
					iTunes->Play();
					break;
				case ITUNES_ACTIONTYPE_NEXT:
					iTunes->NextTrack();
					iTunes->get_PlayerState(&state);
					if(state == ITPlayerStateStopped){
						if(iTunes->get_CurrentTrack(&iTrack) == S_OK){
							GetiTrackProperties(iTrack,&agenttype_itunes_itrack_properties,&trackchanged);
						}
					}
					break;
				case ITUNES_ACTIONTYPE_PREVIOUS:
					iTunes->PreviousTrack();
					iTunes->get_PlayerState(&state);
					if(state == ITPlayerStateStopped){
						if(iTunes->get_CurrentTrack(&iTrack) == S_OK){
							GetiTrackProperties(iTrack,&agenttype_itunes_itrack_properties,&trackchanged);
						}
					}
					break;
				case ITUNES_ACTIONTYPE_STOP:
					iTunes->Stop();
					break;
				case ITUNES_ACTIONTYPE_PAUSE:
					iTunes->Pause();
					break;
				case ITUNES_ACTIONTYPE_PPTOGGLE:
					iTunes->PlayPause();
					break;
				case ITUNES_ACTIONTYPE_REWIND:
					iTunes->Rewind();
					break;
				case ITUNES_ACTIONTYPE_FASTFORWARD:
					iTunes->FastForward();
					break;
				case ITUNES_ACTIONTYPE_RESUME:
					iTunes->Resume();
					break;
				case ITUNES_ACTIONTYPE_MUTE:
					iTunes->get_Mute(&vbool);
					iTunes->put_Mute(vnot(vbool));
					break;
				case ITUNES_ACTIONTYPE_VOLUMEUP:
					iTunes->get_SoundVolume(&agenttype_itunes_volume);
					if(agenttype_itunes_volume<100)
						agenttype_itunes_volume++;
					iTunes->put_SoundVolume(agenttype_itunes_volume);
					break;
				case ITUNES_ACTIONTYPE_VOLUMEDOWN:
					iTunes->get_SoundVolume(&agenttype_itunes_volume);
					if(agenttype_itunes_volume>0)
						agenttype_itunes_volume--;
					iTunes->put_SoundVolume(agenttype_itunes_volume);
					break;
				case ITUNES_ACTIONTYPE_MINIMIZE:
					if(iTunes->get_BrowserWindow(&window)==S_OK){
						window->get_Minimized(&vbool);
						window->put_Minimized(vnot(vbool));
						window->Release();
					}
					break;
				case ITUNES_ACTIONTYPE_SHUFFLE:
					if(iTunes->get_CurrentPlaylist(&playlist) == S_OK){
						playlist->get_Shuffle(&vbool);
						playlist->put_Shuffle(vnot(vbool));
						playlist->Release();
					}
					break;
				case ITUNES_ACTIONTYPE_REPEAT:
					if(iTunes->get_CurrentPlaylist(&playlist) == S_OK){
						playlist->get_SongRepeat(&repeatmode);
						if(repeatmode == ITPlaylistRepeatModeOff){
							repeatmode = ITPlaylistRepeatModeOne;
						}else if(repeatmode == ITPlaylistRepeatModeOne){
							repeatmode = ITPlaylistRepeatModeAll;
						}else{
							repeatmode = ITPlaylistRepeatModeOff;
						}
						playlist->put_SongRepeat(repeatmode);
						playlist->Release();
					}
					break;
				case ITUNES_ACTIONTYPE_UPDATEIPOD:
					iTunes->UpdateIPod();
					break;
				case ITUNES_ACTIONTYPE_RESCROLLTITLE:
					_snprintf(agenttype_itunes_itrack_scroll_title,
							sizeof(agenttype_itunes_itrack_scroll_title),
							" %s  ",
							agenttype_itunes_itrack_properties.title);
					break;
				case ITUNES_ACTIONTYPE_RESCROLLARTIST:
					_snprintf(agenttype_itunes_itrack_scroll_artist,
							sizeof(agenttype_itunes_itrack_scroll_artist),
							" %s  ",
							agenttype_itunes_itrack_properties.artist);
					break;
				case ITUNES_ACTIONTYPE_RESCROLLALBUM:
					_snprintf(agenttype_itunes_itrack_scroll_album,
							sizeof(agenttype_itunes_itrack_scroll_album),
							" %s  ",
							agenttype_itunes_itrack_properties.album);
					break;
			}	
			break;
		case NOTIFY_SAVE_AGENT:
			//Write existance
			config_write(config_get_control_setagent_i(a->controlptr, a->agentaction, a->agenttypeptr->agenttypename, &details->commandcode));
			break;
	}
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_itunes_notify
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void agenttype_itunespoller_notify(agent *a, int notifytype, void *messagedata)
{
	//Get the agent details
	agenttype_itunes_details *details = (agenttype_itunes_details *) a->agentdetails;

	double *val;
	long l;

	switch(notifytype)
	{
		case NOTIFY_CHANGE:
			if(!com_disabled && iTunes.p){
				val =  (double *)messagedata;
				switch(details->commandcode)
				{
					case ITUNES_POLLINGTYPE_POSITION:
						agenttype_itunes_trackposition  = (long) (agenttype_itunes_itrack_properties.sec * (*val));
						iTunes->put_PlayerPosition(agenttype_itunes_trackposition);
						break;
					case ITUNES_POLLINGTYPE_VOLUME:
						l = 100*(*val);
						agenttype_itunes_dvolume = *val;
						if(agenttype_itunes_volume != l){
							agenttype_itunes_volume = l;
							iTunes->put_SoundVolume(l);
						}
						break;
					case ITUNES_POLLINGTYPE_RATING:
						IITTrack *iTrack =NULL;
						if(S_OK==(iTunes->get_CurrentTrack(&iTrack))){
							l = 100 * (*val);
							l = ((l + 10) / 20) * 20;
							agenttype_itunes_rating = ((double)l)/100;
							agenttype_itunes_itrack_properties.rating = l;
							iTrack->put_Rating(l);
						}
						break;
				}
			}
			break;
		case NOTIFY_SAVE_AGENT:		
			config_write(config_get_control_setagent_c(a->controlptr, a->agentaction, a->agenttypeptr->agenttypename, agenttype_itunes_pollingnames[details->commandcode]));
			break;
	}
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_itunesimage_notify
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void agenttype_itunesimage_notify(agent *a, int notifytype, void *messagedata)
{
	//Get the agent details
	agenttype_itunes_details *details = (agenttype_itunes_details *) a->agentdetails;
	styledrawinfo *di;
	double widthpct,heightpct;
	int xpos,ypos;
	int width,height;
	int gwidth,gheight;
	switch(notifytype)
	{
		case NOTIFY_DRAW:
			if(!agenttype_itunes_itrack_properties.hasartwork){
				return;
			}
			di = (styledrawinfo *) messagedata;

			WCHAR wTitle[1024];
			
			if(!locale)
				locale = _create_locale(LC_CTYPE,"");
			_mbstowcs_l(wTitle,agenttype_itunes_artworkpath,strlen(agenttype_itunes_artworkpath)+1,locale);
			pImageiTunes = new Gdiplus::Image(wTitle);
			
			if(pImageiTunes)
			{
				gwidth = pImageiTunes->GetWidth();
				gheight = pImageiTunes->GetHeight();
				widthpct = ((double)di->rect.right-4)/gwidth;
				heightpct = ((double)di->rect.bottom-4)/gheight;
				if(widthpct<heightpct){
					width  = di->rect.right-4;
					height = gheight * widthpct;
				}else{
					width  = gwidth * heightpct;
					height = di->rect.bottom-4;
				}
				xpos = (di->rect.right)/2 - width/2;
				ypos = (di->rect.bottom)/2 - height/2;
				imageAttriTunes = new Gdiplus::ImageAttributes();
				imageAttriTunes->SetColorKey(RGB(255,0,255),RGB(255,0,255));
				graphicsiTunes = new Gdiplus::Graphics(di->buffer);
				graphicsiTunes->DrawImage(pImageiTunes,Gdiplus::Rect(xpos,ypos,width,height),
						0,0,pImageiTunes->GetWidth(),pImageiTunes->GetHeight(),
						Gdiplus::UnitPixel, imageAttriTunes, NULL,NULL);
				delete imageAttriTunes;
				delete graphicsiTunes;
				delete pImageiTunes;
			}
			break;
		case NOTIFY_SAVE_AGENT:		
			config_write(config_get_control_setagent_c(a->controlptr, a->agentaction, a->agenttypeptr->agenttypename, "iTunesArtwork"));
			break;
	}
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_itunes_getdata
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void *agenttype_itunes_getdata(agent *a, int datatype)
{
	return NULL;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_itunes_getdata
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void *agenttype_itunespoller_getdata(agent *a, int datatype)
{
	agenttype_itunes_details *details = (agenttype_itunes_details *) a->agentdetails;
	
	ITPlayerState state;
	ITPlaylistRepeatMode repeatmode;
	IITBrowserWindow *window;
	IITPlaylist *playlist;
	VARIANT_BOOL vbool;

	switch(datatype)
	{
		case DATAFETCH_VALUE_TEXT:
			switch(details->commandcode)
			{
				case ITUNES_POLLINGTYPE_TITLE: 
					return agenttype_itunes_itrack_properties.title; 
					break;
				case ITUNES_POLLINGTYPE_ARTIST: 
					return agenttype_itunes_itrack_properties.artist; 
					break;
				case ITUNES_POLLINGTYPE_ALBUM: 
					return agenttype_itunes_itrack_properties.album; 
					break;

				case ITUNES_POLLINGTYPE_TITLESCROLL:
					return selectchar(agenttype_itunes_itrack_scroll_title,agenttype_itunes_itrack_properties.title);
					break;
				case ITUNES_POLLINGTYPE_ALBUMSCROLL:
					return selectchar(agenttype_itunes_itrack_scroll_album,agenttype_itunes_itrack_properties.album);
					break;
				case ITUNES_POLLINGTYPE_ARTISTSCROLL:
					return selectchar(agenttype_itunes_itrack_scroll_artist,agenttype_itunes_itrack_properties.artist);
					break;
							       
				case ITUNES_POLLINGTYPE_TIME_ELAPSED: 
					return agenttype_itunes_timeelapsed; 
					break;
				case ITUNES_POLLINGTYPE_TIME_REMAINING: 
					return agenttype_itunes_timeremaining; 
					break;
				
				case ITUNES_POLLINGTYPE_TIME_TOTAL: 
					return agenttype_itunes_itrack_properties.time; 
					break;
				
				case ITUNES_POLLINGTYPE_BITRATE:
					return agenttype_itunes_itrack_properties.sbitrate; 
					break;
				case ITUNES_POLLINGTYPE_SAMPLERATE:
					return agenttype_itunes_itrack_properties.ssamplerate; 
					break;
			
				case ITUNES_POLLINGTYPE_RATING:
					_snprintf(agenttype_itunes_ratingtext,sizeof(agenttype_itunes_ratingtext),"%1d", (agenttype_itunes_itrack_properties.rating / 20));
					return agenttype_itunes_ratingtext;
					break;

				default: return NULL; break;
			}
			break;

		case DATAFETCH_VALUE_SCALE:
			switch(details->commandcode)
			{
				case ITUNES_POLLINGTYPE_POSITION:
					return &agenttype_itunes_trackposition;
					break;
				case ITUNES_POLLINGTYPE_VOLUME:
					agenttype_itunes_dvolume = ((double)agenttype_itunes_volume)/100;	
					return &agenttype_itunes_dvolume;
					break;
				case ITUNES_POLLINGTYPE_RATING:
					agenttype_itunes_rating = ((double)agenttype_itunes_itrack_properties.rating) / 100;
					return &agenttype_itunes_rating;
					break;
					
			}
			break;
		case DATAFETCH_VALUE_BOOL:
			switch(details->commandcode)
			{	
				case ITUNES_POLLINGTYPE_HASARTWORK:
					return &agenttype_itunes_itrack_properties.hasartwork;
					break;
				case ITUNES_POLLINGTYPE_ISPLAYING:
					return &agenttype_itunes_isplaying;
					break;
				case ITUNES_POLLINGTYPE_ISREWINDING:
					if(!com_disabled && iTunes.p){
						iTunes->get_PlayerState(&state);

						agenttype_itunes_isrewinding = (state==ITPlayerStateRewind);
					}
					return &agenttype_itunes_isrewinding;
					break;
				case ITUNES_POLLINGTYPE_ISFASTFORWARDING:
					if(!com_disabled && iTunes.p){
						iTunes->get_PlayerState(&state);
						
						agenttype_itunes_isfastforwarding = (state==ITPlayerStateFastForward);
					}
					return &agenttype_itunes_isfastforwarding;
					break;
				case ITUNES_POLLINGTYPE_ISMINIMIZED:
					if(!com_disabled && iTunes.p){
						if(iTunes->get_BrowserWindow(&window)==S_OK){
							window->get_Minimized(&vbool);
							window->Release();
						}
						agenttype_itunes_isminimized =(vbool==VARIANT_TRUE);
					}
					return &agenttype_itunes_isminimized;
					break;
				case ITUNES_POLLINGTYPE_ISSHUFFLE:
					if(!com_disabled && iTunes.p){
						if(iTunes->get_CurrentPlaylist(&playlist)==S_OK){
							playlist->get_Shuffle(&vbool);
							playlist->Release();
						}
						agenttype_itunes_isshuffle =(vbool==VARIANT_TRUE);
					}
					return &agenttype_itunes_isshuffle;
					break;
				case ITUNES_POLLINGTYPE_ISREPEATONE:
					if(!com_disabled && iTunes.p){
						if(iTunes->get_CurrentPlaylist(&playlist)==S_OK){
							playlist->get_SongRepeat(&repeatmode);
							playlist->Release();
						}
						agenttype_itunes_isrepeatone= (repeatmode == ITPlaylistRepeatModeOne);
					}
					return &agenttype_itunes_isrepeatone;
					break;
				case ITUNES_POLLINGTYPE_ISREPEATALL:
					if(!com_disabled && iTunes.p){
						if(iTunes->get_CurrentPlaylist(&playlist)==S_OK){
							playlist->get_SongRepeat(&repeatmode);
							playlist->Release();
						}
						agenttype_itunes_isrepeatall=(repeatmode == ITPlaylistRepeatModeAll);
					}
					return &agenttype_itunes_isrepeatall;
					break;
			}
			break;
	}
	return NULL;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_itunes_menu_set
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void agenttype_itunes_menu_set(Menu *m, control *c, agent *a,  char *action, int controlformat)
{
	int set = -1;
	if (a)
	{
		agenttype_itunes_details *details = (agenttype_itunes_details *) a->agentdetails;
		set = details->commandcode;
	}

	for (int i = 0; i < ITUNES_ACTIONTYPECOUNT; i++)
	{
		make_menuitem_bol(m, itunes_actions[i], config_getfull_control_setagent_i(c, action, "iTunes", &i), set == i);
	}
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_itunespoller_bool_menu_set
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void agenttype_itunespoller_bool_menu_set(Menu *m, control *c, agent *a,  char *action, int controlformat)
{
	for (int i = 1; i <= ITUNES_POLLINGTYPE_LASTBOOL; i++)
	{
		make_menuitem_cmd(m, agenttype_itunes_friendlynames[i], config_getfull_control_setagent_c(c, action, "iTunesBool", agenttype_itunes_pollingnames[i]));
	}
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_itunespoller_scale_menu_set
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void agenttype_itunespoller_scale_menu_set(Menu *m, control *c, agent *a,  char *action, int controlformat)
{
	for (int i = ITUNES_POLLINGTYPE_LASTBOOL + 1; i <= ITUNES_POLLINGTYPE_LASTSCALE; i++)
	{
		make_menuitem_cmd(m, agenttype_itunes_friendlynames[i], config_getfull_control_setagent_c(c, action, "iTunesScale", agenttype_itunes_pollingnames[i]));
	}
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_itunespoller_text_menu_set
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void agenttype_itunespoller_text_menu_set(Menu *m, control *c, agent *a,  char *action, int controlformat)
{
	for (int i = ITUNES_POLLINGTYPE_LASTSCALE + 1; i < ITUNES_POLLINGTYPECOUNT; i++)
	{
		make_menuitem_cmd(m, agenttype_itunes_friendlynames[i], config_getfull_control_setagent_c(c, action, "iTunesText", agenttype_itunes_pollingnames[i]));
	}

	// add Rating menu  (
	make_menuitem_cmd(m, agenttype_itunes_friendlynames[ITUNES_POLLINGTYPE_RATING], config_getfull_control_setagent_c(c, action, "iTunesText", agenttype_itunes_pollingnames[ITUNES_POLLINGTYPE_RATING]));
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_itunes_image_menu_set
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void agenttype_itunesimage_menu_set(Menu *m, control *c, agent *a,  char *action, int controlformat)
{
	make_menuitem_cmd(m, "iTunes album artwork", config_getfull_control_setagent_c(c, action, "iTunesImage", "iTunesArtwork"));
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_itunes_menu_context
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void agenttype_itunes_menu_context(Menu *m, agent *a)
{
	make_menuitem_nop(m, "No options available.");
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_itunes_notifytype
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void agenttype_itunes_notifytype(int notifytype, void *messagedata)
{

}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_itunes_event
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
LRESULT CALLBACK agenttype_itunes_event(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_itunes_timercall
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
VOID CALLBACK agenttype_itunes_timercall(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	//If there are agents left
	if (agenttype_itunes_agents->first != NULL)
	{
		agenttype_itunespoller_updatevalues();
	}
	else
	{
		agenttype_itunes_hastimer = false;	
		KillTimer(hwnd, 0);
	}
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//agenttype_itunesspoller_updatevalues
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void agenttype_itunespoller_updatevalues()
{
	//This will need to be resized
	int offset;
	bool needsupdate[ITUNES_POLLINGTYPECOUNT+1];
	for (int i = 1; i < ITUNES_POLLINGTYPECOUNT+1; i++)
	{
		needsupdate[i] = false;
	}

	//Go through every agent type, see what needs updating
	listnode *currentnode;
	agent *currentagent;
	dolist(currentnode, agenttype_itunes_agents)
	{
		currentagent = (agent *) currentnode->value;
		needsupdate[((agenttype_itunes_details *) currentagent->agentdetails)->commandcode] = true;
	}

	if(!com_disabled && iTunes.p){
		if(needsupdate[ITUNES_POLLINGTYPE_TIME_ELAPSED]
		|| needsupdate[ITUNES_POLLINGTYPE_TIME_REMAINING]
		|| needsupdate[ITUNES_POLLINGTYPE_POSITION])
		{
			long currentpos;
			iTunes->get_PlayerPosition(&currentpos);
			long remain = agenttype_itunes_itrack_properties.sec - currentpos;
			set_time(agenttype_itunes_timeelapsed,currentpos);
			set_time(agenttype_itunes_timeremaining,remain);
		
			agenttype_itunes_trackposition = agenttype_itunes_itrack_properties.sec?(double)currentpos/agenttype_itunes_itrack_properties.sec:0;
		
		}
	
		if(needsupdate[ITUNES_POLLINGTYPE_TITLESCROLL]
			&& strlen(agenttype_itunes_itrack_scroll_title)>0)
		{	
			offset = is2byte((unsigned char)agenttype_itunes_itrack_scroll_title[0])?2:1;
			strcpy(agenttype_itunes_itrack_scroll_title,
				agenttype_itunes_itrack_scroll_title + offset);
		}
	
		if(needsupdate[ITUNES_POLLINGTYPE_ALBUMSCROLL] 
			&& strlen(agenttype_itunes_itrack_scroll_album)>0)
		{
			offset = is2byte((unsigned char)agenttype_itunes_itrack_scroll_album[0])?2:1;
			strcpy(agenttype_itunes_itrack_scroll_album,
				agenttype_itunes_itrack_scroll_album + offset);
		}	
	
		if(needsupdate[ITUNES_POLLINGTYPE_ARTISTSCROLL] 
			&& strlen(agenttype_itunes_itrack_scroll_artist)>0)
		{
			offset = is2byte((unsigned char)agenttype_itunes_itrack_scroll_artist[0])?2:1;
			strcpy(agenttype_itunes_itrack_scroll_artist,
				agenttype_itunes_itrack_scroll_artist + offset);
		}
	}
	
	//Go through every agent
	dolist(currentnode, agenttype_itunes_agents)
	{
		//Get the agent
		currentagent = (agent *) currentnode->value;
		if(((agenttype_itunes_details *) currentagent->agentdetails)->commandcode != ITUNES_POLLINGTYPECOUNT){
			control_notify(currentagent->controlptr, NOTIFY_NEEDUPDATE, NULL);
		}
		else
		{
			if(trackchanged){
				//artwork redraw
				trackchanged=false;
				control_notify(currentagent->controlptr, NOTIFY_NEEDUPDATE, NULL);
			}
		}
	}
}


/*=================================================*/

void initialize_iTunes(){
	if (!iTunes.p) 
	{ 
		if(FAILED(iTunes.CoCreateInstance(CLSID_iTunesApp))){
			MessageBox(NULL,"iTunes create error","",MB_OK);
			return;
		} 
		if(FAILED(CComObject<myiTunesEvents>::CreateInstance(&sink))){
			MessageBox(NULL,"sink create error","",MB_OK);
			return;
		}
		sink->QueryInterface(IID_IUnknown,(void **)&unknown);
		if(FAILED(sink->DispEventAdvise(iTunes))){
			MessageBox(NULL,"Advise Error","",MB_OK);
			return;
		}
	}
}

void delete_iTunes(){
	if(iTunes.p){
		sink->DispEventUnadvise(iTunes);
		sink->Release();
		iTunes.Release();
	}
	initialize_iTunes_Properties(true);
}

bool isRunningiTunes() // if "itunes.exe" process is exist, return true
{
	
	PROCESSENTRY32 prent;
	bool isRunning=false;
	char fnbuf[_MAX_FNAME],extbuf[_MAX_EXT];
	
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	prent.dwSize = sizeof(prent);
	
	if(Process32First(hSnapshot,&prent)){
		do{
			_splitpath(prent.szExeFile,NULL,NULL,fnbuf,extbuf);
			if(!_stricmp("itunes",fnbuf)&&!_stricmp(".exe",extbuf)){
				isRunning = true;
				break;
			}
		}while(Process32Next(hSnapshot,&prent));
	}
	return isRunning;
}

void initialize_iTunes_Properties(){
	initialize_iTunes_Properties(false);
}
void initialize_iTunes_Properties(bool reset){
	if(reset){
		//initialize all properties 
		strcpy(agenttype_itunes_itrack_properties.title,"");
		strcpy(agenttype_itunes_itrack_properties.artist,"");
		strcpy(agenttype_itunes_itrack_properties.album,"");
		strcpy(agenttype_itunes_itrack_properties.time,"");
		strcpy(agenttype_itunes_itrack_properties.sbitrate,"");
		strcpy(agenttype_itunes_itrack_properties.ssamplerate,"");
		agenttype_itunes_itrack_properties.sec=0;
		agenttype_itunes_itrack_properties.bitrate=0;
		agenttype_itunes_itrack_properties.samplerate=0;
		agenttype_itunes_trackposition = 0.0;
		agenttype_itunes_rating = 0;
		strcpy(agenttype_itunes_timeelapsed,"");
		strcpy(agenttype_itunes_timeremaining,"");
		strcpy(agenttype_itunes_ratingtext,"");
		agenttype_itunes_isplaying = false;
		agenttype_itunes_isrewinding = false;
		agenttype_itunes_isfastforwarding = false;
		agenttype_itunes_isminimized = false;
		agenttype_itunes_isshuffle = false;
		agenttype_itunes_isrepeatone = false;
		agenttype_itunes_isrepeatall = false;
		agenttype_itunes_volume=0;
		agenttype_itunes_dvolume=0;
		strcpy(agenttype_itunes_itrack_scroll_title," ");
		strcpy(agenttype_itunes_itrack_scroll_artist," ");
		strcpy(agenttype_itunes_itrack_scroll_album," ");
		agenttype_itunes_itrack_properties.hasartwork = false;
		trackchanged = true;
	}
	else{
		//set new properties
		
		ITPlayerState state;
		IITTrack *iTrack =NULL;
		IITBrowserWindow *window;
		IITPlaylist *playlist;
		ITPlaylistRepeatMode repeatmode;
		VARIANT_BOOL vbool;

		if(SUCCEEDED(iTunes->get_PlayerState(&state))){
			agenttype_itunes_isplaying = (state == ITPlayerStatePlaying);
		}

		if(iTunes->get_BrowserWindow(&window)==S_OK){
			window->get_Minimized(&vbool);
			window->Release();
			agenttype_itunes_isminimized =(vbool==VARIANT_TRUE);
		}

		if(iTunes->get_CurrentPlaylist(&playlist)==S_OK){
			playlist->get_Shuffle(&vbool);
			playlist->get_SongRepeat(&repeatmode);
			agenttype_itunes_isshuffle=(vbool==VARIANT_TRUE);
			agenttype_itunes_isrepeatone=(repeatmode == ITPlaylistRepeatModeOne);
			agenttype_itunes_isrepeatall=(repeatmode == ITPlaylistRepeatModeAll);
		}
		iTunes->get_SoundVolume(&agenttype_itunes_volume);
		
		if(S_OK==(iTunes->get_CurrentTrack(&iTrack))){
			GetiTrackProperties(iTrack,&agenttype_itunes_itrack_properties,&trackchanged);
		}
		
	}
}

HRESULT GetIDOfName(IDispatch* idisp, OLECHAR* wszName, DISPID* idispID)
{
   HRESULT hr;
   hr = idisp->GetIDsOfNames(
		   IID_NULL, &wszName, 1, LOCALE_USER_DEFAULT, idispID
	);
  return hr;
}

HRESULT GetProperty(IDispatch* idisp, OLECHAR* wszName, DISPPARAMS *dispParams , VARIANT* pvResult)
{
  HRESULT hr; 
  DISPID dispID;
  hr = GetIDOfName(idisp, wszName, &dispID);
  if (FAILED(hr)){ return hr; }
  hr = idisp->Invoke(
		dispID, IID_NULL, LOCALE_USER_DEFAULT, 
		DISPATCH_PROPERTYGET, dispParams, pvResult, 
		NULL, NULL);
  return hr; 
}

HRESULT GetProperty(IDispatch* idisp, OLECHAR* wszName, VARIANT* pvResult)
{
  DISPPARAMS dispParams = {NULL, NULL, 0, 0};
  return GetProperty(idisp,wszName,&dispParams,pvResult);
}

HRESULT GetiTrackProperties(IDispatch* idisp,agenttype_itunes_itrackproperties* ip,bool *changed){
	//Get Track Properties(name,artist,etc...)
	
	char *tmpstring;
	char oldvalue[1024];
	HRESULT hr;
	VARIANT *ppvResult = new VARIANT;
	
	*changed = false;

	VariantInit(ppvResult);
	strncpy(oldvalue,ip->title,sizeof(oldvalue));
	hr = GetProperty(idisp,L"Name",ppvResult);
	if(SUCCEEDED(hr) && ppvResult->bstrVal){
		tmpstring = _com_util::ConvertBSTRToString(ppvResult->bstrVal);
		strncpy(ip->title,tmpstring,sizeof(ip->title));
		delete[] tmpstring;
	}else{
		strncpy(ip->title,"",sizeof(ip->title));
	}
	if(strcmp(oldvalue,ip->title)){
		*changed = true;
	}
	VariantClear(ppvResult);

	VariantInit(ppvResult);
	if(!*changed){
		strncpy(oldvalue,ip->album,sizeof(oldvalue));
	}
	hr = GetProperty(idisp,L"Album",ppvResult);
	if(SUCCEEDED(hr) && ppvResult->bstrVal){
		tmpstring = _com_util::ConvertBSTRToString(ppvResult->bstrVal);
		strncpy(ip->album,tmpstring,sizeof(ip->album));
		delete[] tmpstring;
	}else{
		strncpy(ip->album,"",sizeof(ip->album));
	}
	if(!*changed && strcmp(oldvalue,ip->album)){
		*changed = true;
	}
	VariantClear(ppvResult);
	
	VariantInit(ppvResult);
	if(!*changed){
		strncpy(oldvalue,ip->artist,sizeof(oldvalue));
	}
	hr = GetProperty(idisp,L"Artist",ppvResult);
	if(SUCCEEDED(hr) && ppvResult->bstrVal){
		tmpstring = _com_util::ConvertBSTRToString(ppvResult->bstrVal);
		strncpy(ip->artist,tmpstring,sizeof(ip->artist));
		delete[] tmpstring;
	}else{
		strncpy(ip->artist,"",sizeof(ip->artist));
	}
	if(!*changed && strcmp(oldvalue,ip->artist)){
		*changed = true;
	}
	VariantClear(ppvResult);
	
	VariantInit(ppvResult);
	hr = GetProperty(idisp,L"Duration",ppvResult);
	if(SUCCEEDED(hr) && ppvResult->plVal){
		ip->sec =  ppvResult->lVal;
		set_time(ip->time,ip->sec);
	}
	VariantClear(ppvResult);
	VariantInit(ppvResult);
	hr = GetProperty(idisp,L"SampleRate",ppvResult);
	if(SUCCEEDED(hr) && ppvResult->plVal){
		ip->samplerate = ppvResult->lVal;
		_snprintf(ip->ssamplerate,sizeof(ip->ssamplerate),"%dHz",ip->samplerate);
	}
	VariantClear(ppvResult);
	VariantInit(ppvResult);
	hr = GetProperty(idisp,L"BitRate",ppvResult);
	if(SUCCEEDED(hr) && ppvResult->plVal){
		ip->bitrate = ppvResult->lVal;
		_snprintf(ip->sbitrate,sizeof(ip->sbitrate),"%dkbps",ip->bitrate);
	}
	VariantClear(ppvResult);
	VariantInit(ppvResult);
	hr = GetProperty(idisp,L"Rating",ppvResult);
	if(SUCCEEDED(hr) && ppvResult->plVal){
		ip->rating = ppvResult->lVal;
	}else{
		ip->rating = 0;
	}
	VariantClear(ppvResult);
	
	//if track is changed, update value for scrolling 
	if(*changed){
		_snprintf(agenttype_itunes_itrack_scroll_title,sizeof(agenttype_itunes_itrack_scroll_title),"  %s  ",ip->title);
		_snprintf(agenttype_itunes_itrack_scroll_album,sizeof(agenttype_itunes_itrack_scroll_album),"  %s  ",ip->album);
		_snprintf(agenttype_itunes_itrack_scroll_artist,sizeof(agenttype_itunes_itrack_scroll_artist),"  %s  ",ip->artist);
	}


	hr = GetiTrackArtworks(idisp);

	ip->hasartwork = SUCCEEDED(hr);
	return hr;

}

HRESULT GetiTrackProperties(IITTrack* iTrack,agenttype_itunes_itrackproperties* ip,bool *changed){
	HRESULT hr = E_FAIL;
	if(iTrack != NULL){
		IDispatch *disp;
		iTrack->QueryInterface(IID_IDispatch,(void **)&disp);
		hr = GetiTrackProperties(disp,&agenttype_itunes_itrack_properties,changed);
	}
	return hr;
}

HRESULT GetiTrackArtworks(IDispatch* idisp){
	HRESULT hr;
	VARIANT *pvResult = new VARIANT;

	VariantInit(pvResult);
	//Get IITArtworkCollection
	hr = GetProperty(idisp,L"Artwork",pvResult);
	if(FAILED(hr))return hr;
 
	//Get IITArtwork
	VARIANT v;
	VARIANTARG vArg[1];
	VariantInit(&v);
	VariantInit(&vArg[0]);
	v.vt = VT_I4;
	v.lVal = 1;
	VariantCopy(&vArg[0],const_cast<VARIANTARG *>(&v));
	DISPPARAMS dispParams = {vArg ,NULL, 1, 0};
	VARIANT tmpVar;
	VariantInit(&tmpVar);
	
	hr = GetProperty(pvResult->pdispVal,L"Item",&dispParams,&tmpVar);
	
	VariantClear(&v);
	VariantClear(&vArg[0]);
	VariantClear(pvResult);
	
	if(FAILED(hr))return hr;
	if(tmpVar.pdispVal == NULL)return E_FAIL;

	//Save Artwork to file
	VariantInit(pvResult);
	VariantInit(&v);
	VariantInit(&vArg[0]);
	v.vt = VT_BSTR;
	v.bstrVal = _com_util::ConvertStringToBSTR(agenttype_itunes_artworkpath); 
	VariantCopy(&vArg[0],const_cast<VARIANTARG *>(&v));
	dispParams.rgvarg = vArg;
	dispParams.rgdispidNamedArgs = NULL;
	dispParams.cArgs = 1;
	dispParams.cNamedArgs = 0;
 	DISPID dispID;
  	hr = GetIDOfName(tmpVar.pdispVal,L"SaveArtworkToFile", &dispID);
  	if (SUCCEEDED(hr)){
	  	hr = tmpVar.pdispVal->Invoke(
			dispID, IID_NULL, LOCALE_USER_DEFAULT, 
			DISPATCH_METHOD, &dispParams, pvResult, NULL, NULL);
		VariantClear(&v);
		VariantClear(&vArg[0]);
		VariantClear(&tmpVar);
	}
	VariantClear(pvResult);
	return hr; 
}
