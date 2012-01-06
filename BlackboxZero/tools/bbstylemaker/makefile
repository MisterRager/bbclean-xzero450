# --------------------------------------------------------------------
# Makefile for bbStyleMaker
#

TOP = ../..

BIN = bbstylemaker.exe
OBJ = \
  main.obj \
  writestyle.obj \
  BImage.obj \
  Settings.obj \
  resource.res

DEFINES = -D __BBSM__

INSTALL_IF_NEW = bbstylemaker.rc
INSTALL_FILES = $(BIN) -as docs/bbstylemaker.txt readme.txt

include $(TOP)/build/makefile.inc

vpath %.cpp $(BBAPI)
Settings.obj : DEFINES += -D BBSETTING_STYLEREADER_ONLY

main.obj: \
  main.cpp \
  bbstylemaker.h \
  bbdialog.h \
  bbdialog.cpp \
  snap.cpp

resource.res: \
  resource.rc \
  bbstylemaker.ico

writestyle.obj: \
  writestyle.cpp \
  rcfile.cpp \
  utils.cpp \
  readroot.cpp \
  Settings.cpp

