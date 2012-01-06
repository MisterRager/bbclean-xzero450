# --------------------------------------------------------------------
# makefile for bbIconBox - gcc (mingw)
#

TOP = ../..

BIN = bbIconBox.dll
OBJ = \
  bbIconBox.obj \
  LoadFolder.obj \
  DropTarget.obj \
  Utils.obj \
  bbPlugin.obj \
  winlist.obj \
  tooltips.obj \
  resource.res \
  drawico.obj

INSTALL_FILES = $(BIN) readme.txt
INSTALL_IF_NEW = bbIconBox.rc

include $(TOP)/build/makefile.inc

$(OBJ) : bbIconBox.h bbPlugin.h
resource.res: drag1.cur drag2.cur
