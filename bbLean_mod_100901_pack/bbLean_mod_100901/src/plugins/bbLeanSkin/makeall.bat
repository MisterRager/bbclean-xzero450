make -f makefile-gcc COMPILE_OPT='-DDRAW_ICON'
mv bbLeanSkinEng.dll bbLeanSkinEng_draw_icon.dll
make -f makefile-gcc -W engine/subclass.cpp
