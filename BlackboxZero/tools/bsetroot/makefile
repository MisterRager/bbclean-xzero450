# --------------------------------------------------------------------
# makefile for bsetroot (mingw gcc)

TOP = ../..

# ---------------------------------------------------------------------------
ifndef PROG
# ---------------------------------------------------------------------------

include $(TOP)/build/makefile.inc

all.progs clean.progs :
	$(MAKE) $* PROG=lib
	$(MAKE) $* PROG=bsetroot
	$(MAKE) $* PROG=bsetbg

# ---------------------------------------------------------------------------
else
# ---------------------------------------------------------------------------

ifeq "$(PROG)" "lib"
# ------------------
# CxImage objects

# Used CxImage Configuration (ximage.h):
# // CxImage supported features
# #define CXIMAGE_SUPPORT_ALPHA          0
# #define CXIMAGE_SUPPORT_SELECTION      0
# #define CXIMAGE_SUPPORT_TRANSFORMATION 1
# #define CXIMAGE_SUPPORT_DSP            0
#
# #define CXIMAGE_SUPPORT_DECODE  1
# #define CXIMAGE_SUPPORT_ENCODE  1
# #define CXIMAGE_SUPPORT_WINDOWS 1
# #define CXIMAGE_SUPPORT_WINCE   0
#
# // CxImage supported formats
# #define CXIMAGE_SUPPORT_BMP 1
# #define CXIMAGE_SUPPORT_GIF 1
# #define CXIMAGE_SUPPORT_JPG 1
# #define CXIMAGE_SUPPORT_PNG 1
#
# #define CXIMAGE_SUPPORT_TIF 0
# #define CXIMAGE_SUPPORT_MNG 0
# #define CXIMAGE_SUPPORT_ICO 0
# #define CXIMAGE_SUPPORT_TGA 0
# #define CXIMAGE_SUPPORT_PCX 0
# #define CXIMAGE_SUPPORT_WBMP 0
# #define CXIMAGE_SUPPORT_WMF 0
# #define CXIMAGE_SUPPORT_J2K 0
# #define CXIMAGE_SUPPORT_JBG 0

XIMA_OBJ=\
  ximage.obj      \
  ximaenc.obj     \
  ximapal.obj     \
  ximatran.obj    \
  ximawnd.obj     \
  xmemfile.obj    \
  \
  ximabmp.obj     \
  ximagif.obj     \
  ximajpg.obj     \
  ximapng.obj     \

XIMA_OTHER_OBJ=\
  ximaico.obj     \
  ximatif.obj     \
  ximalpha.obj    \
  ximasel.obj     \
  ximadsp.obj     \
  ximajbg.obj     \
  ximaj2k.obj     \
  ximawbmp.obj    \
  ximaexif.obj    \
  ximawmf.obj     \
  ximamng.obj     \
  ximapcx.obj     \
  ximatga.obj     \

PNG_OBJ=\
  pngwutil.obj    \
  pngwtran.obj    \
  pngwrite.obj    \
  pngwio.obj      \
  pngvcrd.obj     \
  pngtrans.obj    \
  pngset.obj      \
  pngrutil.obj    \
  pngrtran.obj    \
  pngrio.obj      \
  pngread.obj     \
  pngpread.obj    \
  pngmem.obj      \
  pngget.obj      \
  pnggccrd.obj    \
  pngerror.obj    \
  png.obj         \

JPEG_OBJ=\
  jutils.obj      \
  jquant2.obj     \
  jquant1.obj     \
  jmemnobs.obj    \
  jmemmgr.obj     \
  jidctred.obj    \
  jidctint.obj    \
  jidctfst.obj    \
  jidctflt.obj    \
  jcapimin.obj    \
  jfdctfst.obj    \
  jfdctflt.obj    \
  jerror.obj      \
  jdtrans.obj     \
  jdsample.obj    \
  jdpostct.obj    \
  jdphuff.obj     \
  jdmerge.obj     \
  jdmaster.obj    \
  jdmarker.obj    \
  jdmainct.obj    \
  jdinput.obj     \
  jdhuff.obj      \
  jddctmgr.obj    \
  jdcolor.obj     \
  jdcoefct.obj    \
  jdatasrc.obj    \
  jdatadst.obj    \
  jdapistd.obj    \
  jdapimin.obj    \
  jctrans.obj     \
  jcsample.obj    \
  jcprepct.obj    \
  jcphuff.obj     \
  jcparam.obj     \
  jcomapi.obj     \
  jcmaster.obj    \
  jcmarker.obj    \
  jcmainct.obj    \
  jcinit.obj      \
  jchuff.obj      \
  jcdctmgr.obj    \
  jccolor.obj     \
  jccoefct.obj    \
  jcapistd.obj    \
  jfdctint.obj    \

TIF_OBJ=\
  tif_xfile.obj   \
  tif_zip.obj     \
  tif_write.obj   \
  tif_warning.obj \
  tif_version.obj \
  tif_tile.obj    \
  tif_thunder.obj \
  tif_swab.obj    \
  tif_strip.obj   \
  tif_read.obj    \
  tif_print.obj   \
  tif_predict.obj \
  tif_pixarlog.obj \
  tif_packbits.obj \
  tif_open.obj    \
  tif_ojpeg.obj   \
  tif_next.obj    \
  tif_lzw.obj     \
  tif_luv.obj     \
  tif_jpeg.obj    \
  tif_getimage.obj \
  tif_flush.obj   \
  tif_fax3.obj    \
  tif_error.obj   \
  tif_dumpmode.obj \
  tif_dirwrite.obj \
  tif_dirread.obj \
  tif_dirinfo.obj \
  tif_dir.obj     \
  tif_compress.obj \
  tif_codec.obj   \
  tif_close.obj   \
  tif_aux.obj     \
  fax3sm_winnt.obj \

ZLIB_OBJ=\
  zutil.obj       \
  uncompr.obj     \
  trees.obj       \
  infutil.obj     \
  inftrees.obj    \
  inflate.obj     \
  inffast.obj     \
  infcodes.obj    \
  infblock.obj    \
  gzio.obj        \
  deflate.obj     \
  crc32.obj       \
  compress.obj    \
  adler32.obj     \


# ------------------
# FreeImage objects

FI_BASE_OBJ = \
  NNQuantizer.obj \
  WuQuantizer.obj \
  Conversion.obj \
  Conversion16_555.obj \
  Conversion16_565.obj \
  Conversion24.obj \
  Conversion32.obj \
  Conversion4.obj \
  Conversion8.obj \
  ConversionRGBF.obj \
  ConversionType.obj \
  Halftoning.obj \
  tmoColorConvert.obj \
  tmoDrago03.obj \
  tmoReinhard05.obj \
  ToneMapping.obj \
  Plugin.obj \
  PluginBMP.obj \
  PluginCUT.obj \
  PluginDDS.obj \
  PluginGIF.obj \
  PluginHDR.obj \
  PluginICO.obj \
  PluginIFF.obj \
  PluginJPEG.obj \
  PluginKOALA.obj \
  PluginMNG.obj \
  PluginPCD.obj \
  PluginPCX.obj \
  PluginPNG.obj \
  PluginPNM.obj \
  PluginPSD.obj \
  PluginRAS.obj \
  PluginTARGA.obj \
  PluginTIFF.obj \
  PluginWBMP.obj \
  PluginXBM.obj \
  PluginXPM.obj \
  DeprecationMgr.obj \
  CacheFile.obj \
  MultiPage.obj \
  ZLibInterface.obj \
  Exif.obj \
  FIRational.obj \
  FreeImageTag.obj \
  IPTC.obj \
  TagConversion.obj \
  TagLib.obj \
  XTIFF.obj \
  BitmapAccess.obj \
  ColorLookup.obj \
  FreeImage.obj \
  FreeImageIO.obj \
  GetType.obj \
  MemoryIO.obj \
  PixelAccess.obj \
  BSplineRotate.obj \
  Channels.obj \
  ClassicRotate.obj \
  Colors.obj \
  CopyPaste.obj \
  Display.obj \
  Flip.obj \
  JPEGTransform.obj \
  Rescale.obj \
  Resize.obj \

FI_ZLIB_OBJ = \
  adler32.obj \
  compress.obj \
  crc32.obj \
  deflate.obj \
  gzio.obj \
  infback.obj \
  inffast.obj \
  inflate.obj \
  inftrees.obj \
  trees.obj \
  uncompr.obj \
  zutil.obj 

FI_MNG_OBJ = \
  libmng_callback_xs.obj \
  libmng_chunk_descr.obj \
  libmng_chunk_io.obj \
  libmng_chunk_prc.obj \
  libmng_chunk_xs.obj \
  libmng_cms.obj \
  libmng_display.obj \
  libmng_dither.obj \
  libmng_error.obj \
  libmng_filter.obj \
  libmng_hlapi.obj \
  libmng_jpeg.obj \
  libmng_object_prc.obj \
  libmng_pixels.obj \
  libmng_prop_xs.obj \
  libmng_read.obj \
  libmng_trace.obj \
  libmng_write.obj \
  libmng_zlib.obj 

FI_JPG_OBJ = \
  jcapimin.obj \
  jcapistd.obj \
  jccoefct.obj \
  jccolor.obj \
  jcdctmgr.obj \
  jchuff.obj \
  jcinit.obj \
  jcmainct.obj \
  jcmarker.obj \
  jcmaster.obj \
  jcomapi.obj \
  jcparam.obj \
  jcphuff.obj \
  jcprepct.obj \
  jcsample.obj \
  jctrans.obj \
  jdapimin.obj \
  jdapistd.obj \
  jdatadst.obj \
  jdatasrc.obj \
  jdcoefct.obj \
  jdcolor.obj \
  jddctmgr.obj \
  jdhuff.obj \
  jdinput.obj \
  jdmainct.obj \
  jdmarker.obj \
  jdmaster.obj \
  jdmerge.obj \
  jdphuff.obj \
  jdpostct.obj \
  jdsample.obj \
  jdtrans.obj \
  jerror.obj \
  jfdctflt.obj \
  jfdctfst.obj \
  jfdctint.obj \
  jidctflt.obj \
  jidctfst.obj \
  jidctint.obj \
  jidctred.obj \
  jmemmgr.obj \
  jmemnobs.obj \
  jquant1.obj \
  jquant2.obj \
  jutils.obj \
  transupp.obj 

FI_PNG_OBJ = \
  png.obj \
  pngerror.obj \
  pnggccrd.obj \
  pngget.obj \
  pngmem.obj \
  pngpread.obj \
  pngread.obj \
  pngrio.obj \
  pngrtran.obj \
  pngrutil.obj \
  pngset.obj \
  pngtrans.obj \
  pngvcrd.obj \
  pngwio.obj \
  pngwrite.obj \
  pngwtran.obj \
  pngwutil.obj 

FI_TIF_OBJ = \
  tif_aux.obj \
  tif_close.obj \
  tif_codec.obj \
  tif_color.obj \
  tif_compress.obj \
  tif_dir.obj \
  tif_dirinfo.obj \
  tif_dirread.obj \
  tif_dirwrite.obj \
  tif_dumpmode.obj \
  tif_error.obj \
  tif_extension.obj \
  tif_fax3.obj \
  tif_fax3sm.obj \
  tif_flush.obj \
  tif_getimage.obj \
  tif_jpeg.obj \
  tif_luv.obj \
  tif_lzw.obj \
  tif_next.obj \
  tif_open.obj \
  tif_packbits.obj \
  tif_pixarlog.obj \
  tif_predict.obj \
  tif_print.obj \
  tif_read.obj \
  tif_strip.obj \
  tif_swab.obj \
  tif_thunder.obj \
  tif_tile.obj \
  tif_version.obj \
  tif_warning.obj \
  tif_write.obj \
  tif_zip.obj 

# ------------------
# You need either
#   CxImage - http://www.codeproject.com/bitmap/cximage.asp
#   (tested with version 5.11)
# or
#   FreeImage - http://freeimage.sourceforge.net/
#   (tested with version 3.7.0)

USELIB = CxImage
# USELIB = FreeImage

# ------------------

ifeq "$(USELIB)" "CxImage"
  IMGOBJ = image_cx.obj $(XIMA_OBJ) $(PNG_OBJ) $(ZLIB_OBJ) $(JPEG_OBJ)
  IMGDEF = -I CXIMAGE/zlib -D WIN32
  VP1 = CXIMAGE/CxImage
  VP2 = CXIMAGE/zlib
  VP3 = CXIMAGE/png
  VP4 = CXIMAGE/jpeg
  VP5 = CXIMAGE/tiff
else ifeq "$(USELIB)" "FreeImage"
  IMGOBJ = image_fr.obj $(FI_BASE_OBJ) $(FI_ZLIB_OBJ) $(FI_PNG_OBJ) $(FI_JPG_OBJ) $(FI_MNG_OBJ) $(FI_TIF_OBJ)
  IMGDEF = -I FreeImage/Source -I FreeImage/Source/ZLib -D FREEIMAGE_LIB -D HAVE_VSNPRINTF -D NOSAVE -D WIN32
  VP1 = FreeImage/Source/FreeImage
  VP2 = FreeImage/Source/DeprecationManager
  VP3 = FreeImage/Source/Metadata
  VP4 = FreeImage/Source/FreeImageToolkit
  VP5 = FreeImage/Source/LibJPEG
  VP6 = FreeImage/Source/LibMNG
  VP7 = FreeImage/Source/LibPNG
  VP8 = FreeImage/Source/LibTIFF
  VP9 = FreeImage/Source/ZLib
else
  IMGOBJ = image_tiny.obj
  IMGDEF =
endif

BIN = $(call LIBNAME,Image)
OBJ = $(IMGOBJ)
DEFINES += $(IMGDEF)
VPATH = $(VP1) $(VP2) $(VP3) $(VP4) $(VP5) $(VP6) $(VP7) $(VP8) $(VP9)

include $(TOP)/build/makefile.inc

# --------------------------------------------------------------------
endif

ifeq "$(PROG)" "bsetroot"
BIN = bsetroot.exe
OBJ = bsetroot.obj BImage.obj bsrt-rsc.res $(call LIBNAME,Image)

INSTALL_FILES = $(BIN) -to docs bsetroot.htm
INSTALL_IF_NEW = bsetroot.rc
CLEAN_FILES = bsetroot.htm

all : bsetroot.htm

bsetroot.htm : bsetroot.txt ../../docs/bblean_style.css
	-$(FUZZY) bsetroot.txt

include $(TOP)/build/makefile.inc

vpath %.cpp $(BBAPI)

# --------------------------------------------------------------------
endif

ifeq "$(PROG)" "bsetbg"
BIN = bsetbg.exe
OBJ = bsetbg.obj bsbg-rsc.res
INSTALL_FILES = $(BIN)

include $(TOP)/build/makefile.inc
endif

endif
# --------------------------------------------------------------------
