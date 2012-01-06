# --------------------------------------------------------------------
# makefile for bbLeanSkin
#

TOP = ../..

# --------------------------------------------------------------------
ifndef PROG
# --------------------------------------------------------------------

include $(TOP)/build/makefile.inc

all.progs :
	$(MAKE) $* PROG=dll
	$(MAKE) $* PROG=eng
ifeq "$(BB64)" "1"
	$(MAKE) $* PROG=eng32
	$(MAKE) $* PROG=run32
endif

clean.progs :
	$(MAKE) $* PROG=dll
	$(MAKE) $* PROG=eng
	$(MAKE) $* PROG=eng32
	$(MAKE) $* PROG=run32

# --------------------------------------------------------------------
else
# --------------------------------------------------------------------

ifeq "$(PROG)" "dll"
  BIN = bbLeanSkin.dll
  OBJ = bbLeanSkin.obj sysmenu.obj
  INSTALL_FILES = $(BIN) readme.txt
  INSTALL_IF_NEW = bbLeanSkin.rc bbLeanSkinMenu.rc exclusions.rc \
    -subdir buttons buttons_11x11.bmp
endif

ifeq "$(PROG)" "eng"
  BIN = bbLeanSkinEng.dll
  OBJ = hookctl.obj subclass.obj BImage.obj drawico.obj
  NO_BBLIB = 1
  INSTALL_FILES = $(BIN)
  LFLAGS += $(EXTRA_LFLAGS)
endif

ifeq "$(PROG)" "eng32"
  BIN = bbLeanSkinEng32.dll
  OBJ = hookctl32.obj subclass32.obj BImage32.obj drawico32.obj
  NO_BBLIB = 1
  IMPLIB = 1
  ALWAYS_32 = 1
  INSTALL_FILES = $(BIN)
  DEFINES += -D BBLEANSKIN_ENG32
  LFLAGS += $(EXTRA_LFLAGS)
endif

ifeq "$(PROG)" "run32"
  BIN = bbLeanSkinRun32.exe
  OBJ = bbLeanSkinRun32.obj
  NO_BBLIB = 1
  ALWAYS_32 = 1
  LIBS = $(call LIBNAME,bbLeanSkinEng32)
  INSTALL_FILES = $(BIN)
endif

include $(TOP)/build/makefile.inc

VPATH += engine
vpath %.cpp $(BBAPI)
ENG_H = hookinfo.h subclass.h

bbLeanSkin.obj bbLeanSkinRun32.obj : $(ENG_H)
subclass.obj subclass32.obj : $(ENG_H) $(BBAPI)/bImage.cpp
hookctl.obj hookctl32.obj : $(ENG_H)
readstyle.obj : $(BBAPI)/settings.cpp

%32.obj: %.cpp
	$(COMPILE.cpp)

# For some reason the gcc 3.4 (rt 3.7) hangs in it's _DllMainCRTStartup
# That's why it is bypassed here by starting with DllMain directly
ifeq "$(USING_MINGW)" "1"
EXTRA_LFLAGS = -Wl,-e,_DllMain@12
endif

# --------------------------------------------------------------------
endif
# --------------------------------------------------------------------
