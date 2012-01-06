# --------------------------------------------------------------------
# for MS compilers (default MSVC 6,0)

USING_MSC = 1

SUBSYSTEM   ?= WINDOWS
_CFLAGS     = /Zi /MD
_CPPFLAGS   = /GX

CFLAGS      += /O1 /W3 $(_CFLAGS) /nologo
LFLAGS      += /SUBSYSTEM:$(SUBSYSTEM) /DEBUG /NOLOGO /OPT:REF,ICF,NOWIN98
CPPFLAGS    += $(_CPPFLAGS)

CC          = cl
LINK        = link
RC          = rc

SYSLIBS += \
  kernel32.lib    \
  user32.lib      \
  gdi32.lib       \
  shell32.lib     \
  comctl32.lib    \
  comdlg32.lib    \
  shlwapi.lib     \
  advapi32.lib    \
  version.lib     \
  ole32.lib       \
  winmm.lib

# full source path for the debugger
SRCDIR = "$(CURDIR)"/

COMPILE.c   = $(CC) $(CFLAGS) /Fo$@ /c $(SRCDIR)$< $(DEFINES)
COMPILE.cpp = $(CC) $(CFLAGS) $(CPPFLAGS) /Fo$@ /c $(SRCDIR)$< $(DEFINES)
COMPILE.rc  = $(RC) $(DEFINES) /fo$@ $(SRCDIR)$<

define LINK.exe
 $(LINK) $(LFLAGS) /OUT:$@ $^ $(SYSLIBS)
 $(call RM,$(STUFF))
 $(MANIFEST_CMD.exe)
endef

define LINK.dll
 $(LINK) $(LFLAGS) /DLL /OUT:$@ $^ $(SYSLIBS)
 $(call RM,$(STUFF))
 $(MANIFEST_CMD.dll)
endef

define LINK.lib
 $(call RM,$@)
 $(LINK) /lib /nologo -out:$@ $^
endef

STUFF = $*.exp
ifndef IMPLIB
STUFF += $*.lib
endif

LIBNAME = $1.lib

# --------------------------------------------------------------------
