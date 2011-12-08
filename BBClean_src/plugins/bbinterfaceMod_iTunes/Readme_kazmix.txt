BBinterface modification by kazmix

Additional works

<New Agents>
  Diskspace Monitor Agent
  	*Text(2 patterns: "xx.xGB" or "xx%")
	*Scale Value 
 
  Network Monitor Agent
  	*Text 
	*Double Value (use only Autoscaled graph agent)

  Autoscaled graph Agent 

  iTunes Support Agent
    iTunes Control (Play, Stop, etc...)
    Playing track information text (title, artist, album, etc ...)
    Playing track Artwork 
    iTunes state (boolean value)

  Clock Agent

  System Information Agent
    Host Name, User Name, etc...
  
<Other>
  Add New Style ("Label" , "Clock" , "Button" and "PressedButton")
  
  Add "Advanced Option" menu
    ( Style - Custom - "Advanced Option" )
  
  Select font face & size
  
  Pressed-Button style selection 
  
  Graph : Add  chart color option
    ( Graph/AutoScaleGraph - Options - Custom Chart Color & Chart Color)

  Slider : Support custom style and add new style options

  Change "Text Shadow" Option
    *delete "enable shadows" option from Global Option section
    *Inherit style's Text-Shadow settings (bbClean, bbLean mod[070421 or later])
    *add "Shadow Color" "Shadow X/Y" settings in 'Style - Custom' section
	
  Add "Border" Option ( Style - Custom - "Boarder" )
    *add borderColor & borderWidth settings


  "Detect Fullscreen App" option
    Hide when an application goes fullscreen.

  Add "Workspace" option (in "Window Option" menu)
	*This option is only shown & affected when "On All Workspaces" is disabled.
    Select a workspace which the Control is shown on.
    
  
  fix: some timers are not killed successfully when reconfigure/reload plugin
  fix: bbLean froze when using Text 6-10 of Compound Text on Label/Frame

sample modules
  kazmix_samplescript\iTunesModule.rc   iTunes Control Frame
  kazmix_samplescript\sysmeterModule.rc System Monitor Frame


--
kazmix
http://www.kazmix.com/cside.php (Japanese)
http://www.kazmix.com/bb4win/
kazmix@kazmix.com
