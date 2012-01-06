# --------------------------------------------------------------------
# for mingw-w64 compiler

BB64 = 1

include $(TOP)/build/makefile-gcc.inc

ifndef ALWAYS_32
CC      = x86_64-pc-mingw32-gcc
CP      = x86_64-pc-mingw32-g++
WINDRES = x86_64-pc-mingw32-windres.exe
AR      = x86_64-pc-mingw32-ar
endif
