# --------------------------------------------------------------------
# makefile for SystemBarEx - gcc (mingw)

# Compile:
#   you may want to change INST below
#   make -f makefile

# Versions:
#   gcc-3.4.2
#   binutils-2.15.90
#   mingw-runtime-3.7
#   w32api-3.2

OBJ     = SystemBarEx.o bbAbout.o bbTooltip.o IconItem.o TinyDropTarg.o
RES     =
BIN     = SystemBarEx.dll
# INST    = c:\bblean\plugins\SystemBarEx

BBAPI	= ../../../bblean/blackbox
#BBAPI   = ../../blackbox
#NOTE: FIXME: The bb4win .a and .def were stolen from bbLean, thus they are incorrect
#BBAPI   = ./bb4win
BBLIB   = $(BBAPI)/libBlackbox.a

ifdef INSTDIR
all: $(INST)/$(BIN)
else
all: $(BIN)
endif

SystemBarEx.o: \
	TinyDropTarg.cpp \
	SystemBarEx.cpp \
	bbAbout.cpp \
	bbTooltip.cpp \
	IconItem.cpp


clean:
	rm -f $(OBJ) $(RES) $(BIN)

$(INST)/$(BIN): $(BIN)
	copy $(BIN) $(INST)

$(BBLIB): $(BBAPI)/libBlackbox.def
	dlltool -d $< -l $@ -D blackbox.exe

# --------------------------------------------------------------------
CC      = gcc
WINDRES = windres.exe -J rc -O coff --include-dir $(<D)

CFLAGS  = -c -Wall -Os -I$(BBAPI)\
	-fno-rtti \
	-fno-exceptions \
	-fomit-frame-pointer

#libs-lcomctl32 -luser32 -lshell32 -lkernel32 -lgdi32
LIBS = -lole32 -luuid -lstdc++

$(BIN): $(OBJ) $(BBLIB) makefile
	$(CC) -mwindows -shared -s -o $(BIN) $(OBJ) $(LIBS) $(BBLIB)
	ls -al $(BIN)


# --------------------------------------------------------------------

%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

%.o: %.cpp
	$(CC) $(CFLAGS) $< -o $@

%.res: %.rc
	$(WINDRES) -i $< -o $@

# --------------------------------------------------------------------

