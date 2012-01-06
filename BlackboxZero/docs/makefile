# -----------------------------------------------
# makefile for bblean docs

TOP = ..

SUBDOCFILES = \
  bblean_toc.htm \
  bblean_overview.htm \
  bblean_details.htm \
  bblean_info.htm \

DOCFILES = \
  bblean.htm \
  $(SUBDOCFILES) \
  bblean_logo.png \
  bblean_style.css \

INSTALL_FILES = -to docs $(DOCFILES)
CLEAN_FILES = bblean*.htm

all : $(DOCFILES)

$(SUBDOCFILES) : bblean.htm

bblean.htm : bblean.txt
	$(FUZZY) bblean.txt

include $(TOP)/build/makefile.inc
