# Makefile

BBAPI   = ../../../../blackbox
BBLIB   = $(BBAPI)/libBlackbox.a

CC = gcc
BIN = ext_label.dll

CFLAGS  = -c -Wall -Os -I$(BBAPI)\
	-fno-rtti \
	-fno-exceptions \
	-fomit-frame-pointer \

LIBS = \
	-lshell32 -luser32 -lkernel32 -lgdi32 \
	-lcomctl32 -lole32 -luuid \
	-lstdc++ $(BBLIB)

OBJS = label.o labelctl.o initparser.o

all: $(BIN)


$(BIN): $(OBJS)
	gcc -mWindows -s -shared -o $@ $(OBJS) $(LIBS)
	ls -al $(BIN)

%.o: %.cpp
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -rf $(OUTPUT) $(OBJS)
