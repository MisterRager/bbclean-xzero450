# -------------------------------
# bblib

BBLIB_STATIC = 1

TOP = ..

OBJ = \
  bools.obj \
  colors.obj \
  numbers.obj \
  paths.obj \
  strings.obj \
  tinylist.obj \
  tokenize.obj \
  winutils.obj \
  bbrc.obj \
  bbroot.obj \
  m_alloc.obj \
  pidl.obj \

ifeq "$(BBLIB_STATIC)" "1"
  BIN = $(call LIBNAME,bblib)
  DEFINES += -D BBLIB_STATIC
else
  BIN = bblib.dll
  IMPLIB = 1
  INSTALL_FILES = $(BIN)
endif

DEFINES += -D BBLIB_COMPILING
NO_BBLIB = 1

include $(TOP)/build/makefile.inc

$(OBJ) : $(wildcard *.h)
