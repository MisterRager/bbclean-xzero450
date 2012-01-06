# --------------------------------------------------------------------
# common makefile.inc for bbLean core, plugins and tools

# choose compiler (one of gcc, msc, vc8, msc64, gcc64)
# See also makefile-???.inc

COMPILER ?= gcc

# --------------------------------------------------------------------

# --------------------------------------------------------------------

BSL     = $(subst /,\,$1)

MAKE    := @"$(call BSL,$(MAKE))"
COPY    = copy $(call BSL,$1) "$(call BSL,$2)"
MKDIR   = mkdir "$(call BSL,$@)"
RM      = @for %%f in ($(call BSL,$1)) do $(call DEL,%%f)
DIR     = @dir "$(call BSL,$@)" | find "$(call BSL,$@)"
ifeq "$(OS)" "Windows_NT"
DEL     = @if exist $1 (echo -- removing $1 && del $1)
SHELL   = cmd.exe
else
DEL     = if exist $1 del $1
SHELL   = command.com
endif

FUZZY   = $(call BSL,$(TOP)/build/fuzzydoc/fuzzydoc.exe)
INST    = $(call BSL,$(TOP)/build/inst.exe)

# --------------------------------------------------------------------

# --------------------------------------------------------------------

all : all.progs
#Added for codeblocks
Release : all.progs

clean : clean.progs clean.stuff
#Added for codeblocks
cleanRelease : clean.progs clean.stuff

install : install.progs

all.progs clean.progs install.progs :

clean.stuff :
	$(call RM,$(CLEAN_FILES))

.PHONY : all clean install

.SUFFIXES : .exe .dll .progs

%.obj: %.c
	$(COMPILE.c)
%.obj: %.cpp
	$(COMPILE.cpp)
%.res: %.rc
	$(COMPILE.rc)

# --------------------------------------------------------------------

# --------------------------------------------------------------------

include $(TOP)/build/makefile-$(COMPILER).inc
include $(TOP)/build/version.inc

# --------------------------------------------------------------------

# --------------------------------------------------------------------
ifneq "$(BIN)" ""

all : $(BIN)

BBAPI = $(TOP)/blackbox
DEFINES += -I$(BBAPI)
TYPE = $(subst .,,$(suffix $(BIN)))

ifeq "$(SUBSYSTEM)" "CONSOLE"
NO_BBLIB ?= 1
endif

ifeq "$(TYPE)" "lib"
NO_BBLIB ?= 1
endif

ifneq "$(NO_BBLIB)" "1"
DEFINES += -I$(TOP)/lib
LIBS += $(TOP)/lib/$(call LIBNAME,bblib)
endif

ifeq "$(TYPE)" "dll"
VPATH += $(TOP)/plugins/bbPlugin
DEFINES += -I$(TOP)/plugins/bbPlugin
ifneq "$(NO_BBLIB)" "1"
LIBS += $(BBAPI)/$(call LIBNAME,blackbox)
endif
endif

$(BIN): $(OBJ) $(LIBS)
	$(LINK.$(TYPE))
	@echo ---------------------------------------------------------------
	$(DIR)
	@echo ---------------------------------------------------------------

$(OBJ): $(BBAPI)/bbversion.h
$(BBAPI)/bbversion.h:
	echo>$@ #define BBLEAN_VERSION "$(BBLEAN_VERSION)"
	echo>>$@ #define BBLEAN_RELDATE "$(BBLEAN_RELDATE)"
	echo>>$@ #define BBLEAN_NUMVERSION $(BBLEAN_NUMVERSION)

CLEAN_FILES += $(BIN) $(BIN).manifest $(BBAPI)/bbversion.h
CLEAN_FILES += *.obj *.res *.exp *.lib *.a *.pdb *.tds lmakestate

endif
# --------------------------------------------------------------------

# --------------------------------------------------------------------
ifneq "$(INSTDIR)" ""
all :
	@$(INST) "$(INSTDIR)" -d $(INSTALL_FILES) -- -k $(INSTALL_IF_NEW)

endif
# --------------------------------------------------------------------
