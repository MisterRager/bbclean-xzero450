# --------------------------------------------------------------------
# makefile for bbLeanBar+ - gcc (mingw)

TOP = ../..

BIN = bbLeanBar+.dll
OBJ = \
  bbLeanBar+.obj \
  bbPlugin+.obj \
  sysmenu+.obj \
  tooltips+.obj \
  drawico+.obj \
  bbLeanBar+_private.res
  
 DEFINES = -D __LeanBarPlus__

INSTALL_FILES = $(BIN) -as bbLeanBar+.txt readme.txt
INSTALL_IF_NEW = bbLeanBar.rc

include $(TOP)/build/makefile.inc

bbLeanBar+.obj: \
  bbLeanBar+.cpp \
  bbLeanClasses.cpp \
  BuffBmp.cpp \
  TinyDropTarg.cpp
