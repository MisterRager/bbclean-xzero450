# --------------------------------------------------------------------
# makefile for BBPager - gcc (mingw)
#

TOP = ../..

BIN = BBPager.dll
OBJ =  \
	BBPager.obj \
	Drawing.obj \
	Settings.obj \
	StdAfx.obj \
	Style.obj
	
SYSLIBS = -lshlwapi
INSTALL_FILES = $(BIN) changes.txt
INSTALL_IF_NEW = bbpager.rc

include $(TOP)/build/makefile.inc
