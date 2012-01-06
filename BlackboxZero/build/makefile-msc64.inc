# --------------------------------------------------------------------
# for MS AMD64 cross-compiler from the PSDK

BB64 = 1

include $(TOP)/build/makefile-msc.inc

ifndef ALWAYS_32
ifdef BIN

PROGRAMFILES ?= $(ProgramFiles)
PSDK := $(PROGRAMFILES)\Microsoft Platform SDK for Windows Server 2003 R2
PATH := $(PSDK)\bin\win64\x86\AMD64;$(PSDK)\bin;$(PATH)
INCLUDE := $(PSDK)\Include;$(PSDK)\Include\crt
LIB := $(PSDK)\Lib\AMD64
LFLAGS += /MACHINE:AMD64

export LIB
export INCLUDE
export PATH

DEFINES += -D_WIN64 -DWIN64
_CFLAGS = /Zi /MD /GS- /wd4244 /wd4267
_CPPFLAGS = /EHsc

endif
endif


# --------------------------------------------------------------------
