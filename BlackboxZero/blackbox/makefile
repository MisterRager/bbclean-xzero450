# --------------------------------------------------------------------
# makefile for blackbox.exe - gcc (mingw)
#

TOP     = ..

BIN = blackbox.exe
COREOBJ = \
  Blackbox.obj \
  BBApi.obj \
  Utils.obj \
  BImage.obj \
  Settings.obj \
  PluginManager.obj \
  MessageManager.obj \
  Workspaces.obj \
  Tray.obj \
  Desk.obj \
  Toolbar.obj \
  DesktopMenu.obj \
  ConfigMenu.obj \
  BBRoot.obj \
  BBVWM.obj

MENUOBJ = \
  MenuMaker.obj \
  Menu.obj \
  MenuItem.obj \
  TitleItem.obj \
  FolderItem.obj \
  CommandItem.obj \
  SpecialFolder.obj \
  Contextmenu.obj \
  Dragsource.obj \
  Droptarget.obj

RES = resource.res

OBJ = $(COREOBJ) $(MENUOBJ) $(RES)

DEFINES = -D __BBCORE__
VPATH = Menu
INSTALL_FILES = $(BIN) -to docs Menu/menu-bullets.bmp nls-c.txt
IMPLIB = 1

include $(TOP)/build/makefile.inc

# some dependencies:
$(OBJ): BBApi.h BB.h
$(MENUOBJ): Menu/Menu.h
Blackbox.obj: BBSendData.h
Tray.obj : Tray.h
