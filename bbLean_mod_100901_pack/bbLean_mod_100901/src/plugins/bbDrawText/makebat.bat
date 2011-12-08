set TARGET=bbDrawText
set LIBS=-lcomdlg32
set BBAPI=../../blackbox
set BBLIB=%BBAPI%/libBlackbox.a
set CFLAGS=-Wall -Os -fno-rtti -fno-exceptions -fomit-frame-pointer
g++ %TARGET%.cpp %BBLIB% %CFLAGS% -I%BBAPI% -mwindows -shared -s %LIBS% -o %TARGET%.dll
