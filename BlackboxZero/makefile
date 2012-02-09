# --------------------------------------------------------------------
# toplevel makefile for bbLean and plugins (for gnu make)

TOP = .

INSTALL_IF_NEW = \
  menu.rc \
  blackbox.rc \
  extensions.rc \
  plugins.rc \
  shellfolders.rc \
  stickywindows.ini

INSTALL_FILES = \
  -subdir fonts . \
  -subdir styles lean_blue \
  -subdir backgrounds .

CLEAN_FILES = \
  bbnote.ini \
  docs/bbnote.* \
  bbstylemaker.rc \
  docs/bbstylemaker.txt \
  docs/nls-c.txt \
  bsetroot.rc \
  docs/bsetroot.* \
  docs/*.bmp \
  *.exe *.dll

# gmake's abspath is broken
ABSPATH = $(if $(findstring :,$1),$1,$(abspath $1))
ifneq "$(INSTDIR)" ""
override INSTDIR := $(call ABSPATH,$(INSTDIR))
I = INSTDIR=$(INSTDIR)
else
I = IGNORE=.
endif

MAKE_CORE   = $(MAKE) $* -C $1 "$I"
MAKE_PLUGIN = $(MAKE) $* -C plugins/$1 "$I/plugins/$1"
MAKE_TOOL   = $(MAKE) $* -C tools/$1 "$I"

include $(TOP)/build/makefile.inc

all.progs clean.progs :
	$(call MAKE_CORE,build)

	$(call MAKE_CORE,lib)
	$(call MAKE_CORE,blackbox)
	$(call MAKE_CORE,blackbox/Hooks)
	$(call MAKE_CORE,build/fuzzydoc)
	$(call MAKE_CORE,docs)

	$(call MAKE_PLUGIN,bbKeys)
	$(call MAKE_PLUGIN,bbLeanBar)
	$(call MAKE_PLUGIN,bbLeanSkin)
	$(call MAKE_PLUGIN,bbSlit)
	$(call MAKE_PLUGIN,bbAnalog)
	$(call MAKE_PLUGIN,bbIconBox)
	$(call MAKE_PLUGIN,bbColor3dc)
	$(call MAKE_PLUGIN,bbWorkspaceWheel)

	$(call MAKE_PLUGIN,bbLeanBar+)
	$(call MAKE_PLUGIN,bbRecycleBin)
	$(call MAKE_PLUGIN,BBPager)
	$(call MAKE_PLUGIN,BBStyle)

	$(call MAKE_TOOL,bsetshell)
	$(call MAKE_TOOL,bsetroot)
	$(call MAKE_TOOL,bbstylemaker)
	$(call MAKE_TOOL,bbnote)

	$(call MAKE_CORE,build/bdf2fon)
	$(call MAKE_CORE,fonts)

# --------------------------------------------------------------------
