# --------------------------------------------------------------------
# makefile for bbLeanBar - gcc (mingw)

TOP = ../..

BIN = bbLeanBar.dll
OBJ = \
  bbLeanBar.obj \
  bbPlugin.obj \
  sysmenu.obj \
  tooltips.obj \
  drawico.obj

INSTALL_FILES = $(BIN) readme.txt
INSTALL_IF_NEW = bbLeanBar.rc

include $(TOP)/build/makefile.inc

bbLeanBar.obj: \
  bbLeanBar.cpp \
  bbLeanClasses.cpp \
  BuffBmp.cpp \
  TinyDropTarg.cpp
