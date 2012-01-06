# ---------------------------------------------------------------
# artwiz fonts

TOP = ..

ARTWIZ_FONTS = \
    anorexia        \
    aqui            \
    cure            \
    drift           \
    edges           \
    gelly           \
    glisp           \
    lime            \
    mints-mild      \
    mints-strong    \
    nu              \
    snap            \
    kates           \
    fkp             \

VPATH = artwiz-latin1-1.0-src

FNT2FON = $(call BSL,$(TOP)/build/bdf2fon/fnt2fon)
BDF2FNT = $(call BSL,$(TOP)/build/bdf2fon/bdf2fnt)
FNT_FILES = $(patsubst %,%.fnt,$(ARTWIZ_FONTS))
FON_FILES = $(patsubst %,%.fon,$(ARTWIZ_FONTS))

ALL = $(FON_FILES)
# ALL = $(FNT_FILES)

INSTALL_FILES = -to fonts $(ALL) README.TXT
CLEAN_FILES = $(FON_FILES) $(FNT_FILES)

all: $(ALL)

%.fnt : %.de.bdf
	$(BDF2FNT) $< $@ $*
%.fnt : %.se.bdf
	$(BDF2FNT) $< $@ $*
%.fnt : %.bdf
	$(BDF2FNT) $< $@ $*

%.fon : %.fnt
	$(FNT2FON) $< $@

.SUFFIXES : .fon .fnt

include $(TOP)/build/makefile.inc
