# --------------------------------------------------------------------
# for mingw-w64 compiler

BB64 = 1

include $(TOP)/build/makefile-gcc.inc

ifndef ALWAYS_32
CC      = x86_64-w64-mingw32-gcc
CP      = x86_64-w64-mingw32-g++
WINDRES = x86_64-w64-mingw32-windres.exe
AR      = x86_64-w64-mingw32-ar
endif
