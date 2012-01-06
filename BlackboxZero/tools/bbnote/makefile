# ---------------------------------------------------------------------------
# Makefile for bbnote - gcc/mingw

TOP = ../..

# ---------------------------------------------------------------------------
ifndef PROG
# ---------------------------------------------------------------------------

include $(TOP)/build/makefile.inc

all.progs clean.progs :
	$(MAKE) $* PROG=1
	$(MAKE) $* PROG=2

# ---------------------------------------------------------------------------
else
# ---------------------------------------------------------------------------

ifeq "$(PROG)" "1"
  BIN = bbnote.exe

  OBJ =\
    bbnote.obj \
    edfiles.obj \
    edfunc.obj \
    match3.obj \
    edprint.obj \
    edproc.obj \
    bbapi-small.obj \
    readstyle.obj \
    BImage.obj \
    Settings.obj \
    resource.res

  INSTALL_FILES = $(BIN) -to docs bbnote.txt bbnote.htm
endif

ifeq "$(PROG)" "2"
  BIN = bbnote.dll
  OBJ = bbnote-proxy.obj
endif

include $(TOP)/build/makefile.inc

Settings.obj : DEFINES += -D BBSETTING_STYLEREADER_ONLY -D BBSETTING_NOWINDOW
vpath %.cpp $(BBAPI)

bbnote.obj: bbstyle.h
resource.res: bbnote.ico

# --------------------------------------------------------------------
endif
# --------------------------------------------------------------------
