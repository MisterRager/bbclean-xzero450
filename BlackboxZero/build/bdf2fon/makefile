# ---------------------------------------------------------------
# bdf2fon

TOP = ../..

# ---------------------------------------------------------------
ifeq "$(PROG)" "1"

BIN = bdf2fnt.exe
OBJ = bdf2fnt.obj
SUBSYSTEM = CONSOLE

# ---------------------------------------------------------------
else ifeq "$(PROG)" "2"

BIN = fnt2fon.exe
OBJ = fnt2fon.obj
SUBSYSTEM = CONSOLE

# ---------------------------------------------------------------
else

all :
all.progs clean.progs :
	$(MAKE) $* PROG=1
	$(MAKE) $* PROG=2

endif
# ---------------------------------------------------------------

ALWAYS_32 = 1
include $(TOP)/build/makefile.inc
