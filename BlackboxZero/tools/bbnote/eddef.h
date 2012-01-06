/*---------------------------------------------------------------------------*

  This file is part of the BBNote source code

  Copyright 2003-2009 grischka@users.sourceforge.net

  BBNote is free software, released under the GNU General Public License
  (GPL version 2). For details see:

  http://www.fsf.org/licenses/gpl.html

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
  for more details.

 *---------------------------------------------------------------------------*/
// idemain.h

#define DLG_ABOUT    199

#define DLG_LINE    200
#define LINE_TXT    201
#define LINE_EDIT   202

#define DLG_0300    300
#define TEXTFELD    301
#define IDMAKE      302
#define ERRLOG      303
#define OUTLOG      304
#define IDRUN       305

#define DLG_SEA     400
#define SEA_LINE    401
#define IDUP        402
#define IDCASE      403
#define RPL_LINE    404
#define IDWORD      405
#define IDRPL       406
#define IDREXP      407
#define IDALLF      408
#define IDGREP      409
#define SEA_INIT    410

#define DLG_TOOL    700
#define WTOOL       701

#define DLG_YNAN    800
#define IDALWAYS    11
#define IDNEVER     12
#define YNAN_TEXT   803
#define YNAN_BORDER 804
#define YNAN_ICON   805

#define DLG_PBOX   900
#define PBOX_LIST  901
#define PBOX_REM   902
#define PBOX_ADD   903

#define DLG_PROJ    500
#define PRJ_NAME    501
#define PRJ_HOME    502
#define PRJ_MAKE    503
#define PRJ_DIR     504
#define PRJ_ENV     505
#define PRJ_RUN     506
#define PRJ_RDIR    507
#define PRJ_DBG     508
#define PRJ_HELP    509

#define DLG_CONF    600
#define CONF_TABS   601
#define CONF_SMART  602
#define CONF_BAK    603
#define CONF_COLOR  604
#define CONF_CHOOSE 605
#define CONF_FONT   606
#define CONF_FNTSIZ 607
#define CONF_BOLD   608

#define CONF_CEOL   640
#define CONF_MMRK   641
#define CONF_LMRK   642
#define CONF_HSB    643
#define CONF_BAKD   644
#define CONF_SDLY   645
#define CONF_WORD   646
#define CONF_TUSE   647

#define CONF_NONE   649

#define COL_B   610
#define COL_G   611
#define COL_R   612

#define COLOR_EDIT 615
#define COLOR_STAT 616
#define COLOR_LOGW 617

#define COLOR_0 620
#define COLOR_1 621
#define COLOR_2 622
#define COLOR_3 623
#define COLOR_4 624
#define COLOR_5 625
#define COLOR_6 626
#define COLOR_7 627
#define COLOR_8 628
#define COLOR_9 629

#define MNU_MAIN    1000
#define CMD_NEW     1001
#define CMD_OPEN    1002
#define CMD_RELOAD  1003
#define CMD_SAVE    1004
#define CMD_SAVEAS  1005
#define CMD_EXIT    1006
#define CMD_OPTIONS 1007
#define CMD_MAKE    1008
#define CMD_SEARCH  1009
#define CMD_RUN     1010
#define CMD_PRJCFG  1011
#define CMD_INFO    1012
#define CMD_SAVEALL 1013
#define CMD_HELP    1014
#define CMD_ABOUT   1015
#define CMD_COLORS  1016
#define CMD_RELOAD_2 1017
#define CMD_UPD     1018
#define CMD_RESETUNDO 1019
#define CMD_CHELP   1020
#define CMD_DEBUG   1021

#define CMD_RUNTOOL 1022
#define CMD_NEWTOOL 1023
#define CMD_DELTOOL 1024
#define CMD_KILLTOOL 1025
#define CMD_SPAWNEXIT 1026
#define CMD_SAVETORUN 1027
#define CMD_WINAPI  1028
#define CMD_PRJOPEN 1029
#define CMD_PRJCLEAN 1030
#define CMD_PRJBUILD 1031
#define CMD_OWCLOSE 1032
#define CMD_TWCLOSE 1033
#define CMD_CLOSE   1034
#define CMD_PRJMGR  1035
#define CMD_FILECHG 1036

#define CMD_NONE    1099

#define CMD_NSEARCH  1101
#define CMD_LOADFILE 1102
#define CMD_GOTOLINE 1103

#define CMD_PRJLIST    4000
#define CMD_PRJLIST_M  4199
#define CMD_FILE       4200
#define CMD_FILE_M     7999

#define KEY_RET     2000
#define KEY_ESC     2001
#define KEY_BACK    2002
#define KEY_TAB     2003
#define KEY_UP      2010
#define KEY_DOWN    2011
#define KEY_RIGHT   2012
#define KEY_LEFT    2013
#define KEY_NEXT    2014
#define KEY_PRIOR   2015
#define KEY_HOME    2016
#define KEY_END     2017
#define KEY_INSERT  2018
#define KEY_DELETE  2019
#define KEY_F1      2030
#define KEY_F2      2031
#define KEY_F3      2032
#define KEY_F4      2033
#define KEY_F5      2034
#define KEY_F6      2035
#define KEY_F7      2036
#define KEY_F8      2037
#define KEY_F9      2038
#define KEY_F10     2039
#define KEY_F11     2040
#define KEY_F12     2041

#define KEY_S_RET   2100
#define KEY_S_BACK  2102
#define KEY_S_TAB   2103
#define KEY_S_UP    2110
#define KEY_S_DOWN  2111
#define KEY_S_RIGHT 2112
#define KEY_S_LEFT  2113
#define KEY_S_NEXT  2114
#define KEY_S_PRIOR 2115
#define KEY_S_HOME  2116
#define KEY_S_END   2117
#define KEY_S_INSERT 2118
#define KEY_S_DELETE 2119
#define KEY_S_F1    2130
#define KEY_S_F2    2131
#define KEY_S_F3    2132
#define KEY_S_F4    2133
#define KEY_S_F5    2134
#define KEY_S_F6    2135
#define KEY_S_F7    2136
#define KEY_S_F8    2137
#define KEY_S_F9    2138
#define KEY_S_F10   2139
#define KEY_S_F11   2140
#define KEY_S_F12   2141

#define KEY_C_RET   2200
#define KEY_C_BACK  2202
#define KEY_C_TAB   2203
#define KEY_C_UP    2210
#define KEY_C_DOWN  2211
#define KEY_C_RIGHT 2212
#define KEY_C_LEFT  2213
#define KEY_C_NEXT  2214
#define KEY_C_PRIOR 2215
#define KEY_C_HOME  2216
#define KEY_C_END   2217
#define KEY_C_INSERT 2218
#define KEY_C_DELETE 2219
#define KEY_C_F1    2230
#define KEY_C_F2    2231
#define KEY_C_F3    2232
#define KEY_C_F4    2233
#define KEY_C_F5    2234
#define KEY_C_F6    2235
#define KEY_C_F7    2236
#define KEY_C_F8    2237
#define KEY_C_F9    2238
#define KEY_C_F10   2239
#define KEY_C_F11   2240
#define KEY_C_F12   2241

#define KEY_C_A     2250
#define KEY_C_B     2251
#define KEY_C_C     2252
#define KEY_C_D     2253
#define KEY_C_E     2254
#define KEY_C_F     2255
#define KEY_C_G     2256
#define KEY_C_H     2257
#define KEY_C_I     2258
#define KEY_C_J     2259
#define KEY_C_K     2260
#define KEY_C_L     2261
#define KEY_C_M     2262
#define KEY_C_N     2263
#define KEY_C_O     2264
#define KEY_C_P     2265
#define KEY_C_Q     2266
#define KEY_C_R     2267
#define KEY_C_S     2268
#define KEY_C_T     2269
#define KEY_C_U     2270
#define KEY_C_V     2271
#define KEY_C_W     2272
#define KEY_C_X     2273
#define KEY_C_Y     2274
#define KEY_C_Z     2275

#define KEY_CS_Z    2276

#define KEY_C_0     2280
#define KEY_C_1     2281
#define KEY_C_2     2282
#define KEY_C_3     2283
#define KEY_C_4     2284
#define KEY_C_5     2285
#define KEY_C_6     2286
#define KEY_C_7     2287
#define KEY_C_8     2288
#define KEY_C_9     2289

#define KEY_A_RET    2300
#define KEY_A_BACK   2302
#define KEY_A_TAB    2303
#define KEY_A_UP     2310
#define KEY_A_DOWN   2311
#define KEY_A_RIGHT  2312
#define KEY_A_LEFT   2313
#define KEY_A_NEXT   2314
#define KEY_A_PRIOR  2315
#define KEY_A_HOME   2316
#define KEY_A_END    2317
#define KEY_A_INSERT 2318
#define KEY_A_DELETE 2319

#define KEY_A_F1    2330
#define KEY_A_F2    2331
#define KEY_A_F3    2332
#define KEY_A_F4    2333
#define KEY_A_F5    2334
#define KEY_A_F6    2335
#define KEY_A_F7    2336
#define KEY_A_F8    2337
#define KEY_A_F9    2338
#define KEY_A_F10   2339
#define KEY_A_F11   2340
#define KEY_A_F12   2341
#define ACC_MAIN    3000
#define ACC_SEA     3001
