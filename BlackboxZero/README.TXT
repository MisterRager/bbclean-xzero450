  ---------------------------------------------------------------------

  This is the source code for bbLean
  Copyright © 2004-2009 grischka

  http://bb4win.sourceforge.net/bblean
  http://developer.berlios.de/projects/bblean

  bbLean is free software, released under the GNU General Public
  License (GPL version 2). For details see:

  http://www.fsf.org/licenses/gpl.html

  This software is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for details.

  ---------------------------------------------------------------------

  Prerequisites:
    - A compiler (MinGW or MSC)
    - GNU make from MinGW

  Make:
    Change into the source directory and type:

        make COMPILER=xxx

    where xxx is one of:
        gcc         for MinGW (default)
        msc         for MSVC 6.0
        vc8         for MS VC8 Express
        msc64       for MS AMD64 compiler
        gcc64       for MinGW-w64 compiler

    Some options in the build/makefile-xxx.inc files might need
    tweaking (e.g. for compiler installation paths).

  Clean:
    make clean

  Install into the source tree
    make COMPILER=xxx INSTDIR=.

  Install into directory elsewhere
    make COMPILER=xxx INSTDIR=<directory>

  Tested (among others) with MinGW package versions:
    gcc/g++         3.4.5
    binutils        2.18.50
    mingw-runtime   3.15.1
    w32api          3.12
    make            3.81

  2009-05-20

  --- grischka
