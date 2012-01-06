/*
 * File:	ximaexif.cpp
 * Purpose:	Platform Independent JBG Image Class Loader and Writer
 * 18/Aug/2002 <ing.davide.pizzolato@libero.it>
 * CxImage version 5.50 07/Jan/2003
 */

#include "ximajpg.h"

#if CXIMAGEJPG_SUPPORT_EXIF

////////////////////////////////////////////////////////////////////////////////
CxImageJPG::CxExifInfo::CxExifInfo(EXIFINFO* info)
{
	m_exifinfo = info;
	memset(m_exifinfo, 0, sizeof(EXIFINFO));
	m_szLastError[0]='\0';
}
////////////////////////////////////////////////////////////////////////////////
bool CxImageJPG::CxExifInfo::DecodeInfo(struct jpeg_decompress_struct cinfo)
{
    struct jpeg_marker_struct * markerP;
    bool   found_one=false;

    for (markerP = cinfo.marker_list; markerP; markerP = markerP->next) {
        if (is_exif(*markerP)) {

			if (!exif_process(markerP->data, markerP->data_length, m_exifinfo)) return false;

            found_one = true;
        }
	}

    if (!found_one){
		strcpy(m_szLastError,"No EXIF info in image.");
		return false;
	}

	return true;
}
////////////////////////////////////////////////////////////////////////////////
/*--------------------------------------------------------------------------
   Process a EXIF marker
   Describes all the drivel that most digital cameras include...
--------------------------------------------------------------------------*/
bool CxImageJPG::CxExifInfo::exif_process(unsigned char * CharBuf, unsigned int length, EXIFINFO* pInfo)
{
    int ExifSettingsLength;
    unsigned char * LastExifRefd;

    pInfo->FlashUsed = 0; 
    /* If it's from a digicam, and it used flash, it says so. */
    pInfo->Comments[0] = '\0';  /* Initial value - null string */

    FocalplaneXRes = 0;
    FocalplaneUnits = 0;
    ExifImageWidth = 0;


    {   /* Check the EXIF header component */
        static const unsigned char ExifHeader[] = "Exif\0\0";
        if (memcmp(CharBuf+0, ExifHeader,6)){
			strcpy(m_szLastError,"Incorrect Exif header");
			return false;
		}
    }

    if (memcmp(CharBuf+6,"II",2) == 0){
        MotorolaOrder = 0;
    }else{
        if (memcmp(CharBuf+6,"MM",2) == 0){
            MotorolaOrder = 1;
        }else{
            strcpy(m_szLastError,"Invalid Exif alignment marker.");
			return false;
        }
    }

    /* Check the next two values for correctness. */
    if (Get16u(CharBuf+8) != 0x2a || Get32u(CharBuf+10) != 0x08){
        strcpy(m_szLastError,"Invalid Exif start (1)");
		return false;
    }

    LastExifRefd = CharBuf;

    /* First directory starts 16 bytes in.  Offsets start at 8 bytes in. */
    if (!ProcessExifDir(CharBuf+14, CharBuf+6, length-6, pInfo, &LastExifRefd))
		return false;

    /* This is how far the interesting (non thumbnail) part of the exif went.
     */
    ExifSettingsLength = LastExifRefd - CharBuf;

    /* Compute the CCD width, in milimeters. */
    if (FocalplaneXRes != 0){
        pInfo->CCDWidth = 
            (float)(ExifImageWidth * FocalplaneUnits / FocalplaneXRes);
    }

	return true;
}
////////////////////////////////////////////////////////////////////////////////
/*--------------------------------------------------------------------------
   Convert a 16 bit unsigned value from file's native byte order
--------------------------------------------------------------------------*/
int CxImageJPG::CxExifInfo::Get16u(void * Short)
{
    if (MotorolaOrder){
        return (((unsigned char *)Short)[0] << 8) | ((unsigned char *)Short)[1];
    }else{
        return (((unsigned char *)Short)[1] << 8) | ((unsigned char *)Short)[0];
    }
}
////////////////////////////////////////////////////////////////////////////////
/*--------------------------------------------------------------------------
   Convert a 32 bit signed value from file's native byte order
--------------------------------------------------------------------------*/
int CxImageJPG::CxExifInfo::Get32s(void * Long)
{
    if (MotorolaOrder){
        return  ((( char *)Long)[0] << 24) | (((unsigned char *)Long)[1] << 16)
              | (((unsigned char *)Long)[2] << 8 ) | (((unsigned char *)Long)[3] << 0 );
    }else{
        return  ((( char *)Long)[3] << 24) | (((unsigned char *)Long)[2] << 16)
              | (((unsigned char *)Long)[1] << 8 ) | (((unsigned char *)Long)[0] << 0 );
    }
}
////////////////////////////////////////////////////////////////////////////////
/*--------------------------------------------------------------------------
   Convert a 32 bit unsigned value from file's native byte order
--------------------------------------------------------------------------*/
unsigned long CxImageJPG::CxExifInfo::Get32u(void * Long)
{
    return (unsigned long)Get32s(Long) & 0xffffffff;
}
////////////////////////////////////////////////////////////////////////////////

/* Describes format descriptor */
static const int BytesPerFormat[] = {0,1,1,2,4,8,1,1,2,4,8,4,8};
#define NUM_FORMATS 12

#define FMT_BYTE       1 
#define FMT_STRING     2
#define FMT_USHORT     3
#define FMT_ULONG      4
#define FMT_URATIONAL  5
#define FMT_SBYTE      6
#define FMT_UNDEFINED  7
#define FMT_SSHORT     8
#define FMT_SLONG      9
#define FMT_SRATIONAL 10
#define FMT_SINGLE    11
#define FMT_DOUBLE    12

/* Describes tag values */

#define TAG_EXIF_OFFSET       0x8769
#define TAG_INTEROP_OFFSET    0xa005

#define TAG_MAKE              0x010F
#define TAG_MODEL             0x0110

#define TAG_EXPOSURETIME      0x829A
#define TAG_FNUMBER           0x829D

#define TAG_SHUTTERSPEED      0x9201
#define TAG_APERTURE          0x9202
#define TAG_MAXAPERTURE       0x9205
#define TAG_FOCALLENGTH       0x920A

#define TAG_DATETIME_ORIGINAL 0x9003
#define TAG_USERCOMMENT       0x9286

#define TAG_SUBJECT_DISTANCE  0x9206
#define TAG_FLASH             0x9209

#define TAG_FOCALPLANEXRES    0xa20E
#define TAG_FOCALPLANEUNITS   0xa210
#define TAG_EXIF_IMAGEWIDTH   0xA002
#define TAG_EXIF_IMAGELENGTH  0xA003

/* the following is added 05-jan-2001 vcs */
#define TAG_EXPOSURE_BIAS     0x9204
#define TAG_WHITEBALANCE      0x9208
#define TAG_METERING_MODE     0x9207
#define TAG_EXPOSURE_PROGRAM  0x8822
#define TAG_ISO_EQUIVALENT    0x8827
#define TAG_COMPRESSION_LEVEL 0x9102

#define TAG_THUMBNAIL_OFFSET  0x0201
#define TAG_THUMBNAIL_LENGTH  0x0202


/*--------------------------------------------------------------------------
   Process one of the nested EXIF directories.
--------------------------------------------------------------------------*/
bool CxImageJPG::CxExifInfo::ProcessExifDir(unsigned char * DirStart, unsigned char * OffsetBase, unsigned ExifLength,
                           EXIFINFO * const pInfo, unsigned char ** const LastExifRefdP )
{
    int de;
    int a;
    int NumDirEntries;
    unsigned ThumbnailOffset = 0;
    unsigned ThumbnailSize = 0;

    NumDirEntries = Get16u(DirStart);

    if ((DirStart+2+NumDirEntries*12) > (OffsetBase+ExifLength)){
        strcpy(m_szLastError,"Illegally sized directory");
		return false;
    }

    for (de=0;de<NumDirEntries;de++){
        int Tag, Format, Components;
        unsigned char * ValuePtr;
            /* This actually can point to a variety of things; it must be
               cast to other types when used.  But we use it as a byte-by-byte
               cursor, so we declare it as a pointer to a generic byte here.
            */
        int ByteCount;
        unsigned char * DirEntry;
        DirEntry = DirStart+2+12*de;

        Tag = Get16u(DirEntry);
        Format = Get16u(DirEntry+2);
        Components = Get32u(DirEntry+4);

        if ((Format-1) >= NUM_FORMATS) {
            /* (-1) catches illegal zero case as unsigned underflows
               to positive large.  
            */
            strcpy(m_szLastError,"Illegal format code in EXIF dir");
			return false;
		}

        ByteCount = Components * BytesPerFormat[Format];

        if (ByteCount > 4){
            unsigned OffsetVal;
            OffsetVal = Get32u(DirEntry+8);
            /* If its bigger than 4 bytes, the dir entry contains an offset.*/
            if (OffsetVal+ByteCount > ExifLength){
                /* Bogus pointer offset and / or bytecount value */
                strcpy(m_szLastError,"Illegal pointer offset value in EXIF.");
				return false;
            }
            ValuePtr = OffsetBase+OffsetVal;
        }else{
            /* 4 bytes or less and value is in the dir entry itself */
            ValuePtr = DirEntry+8;
        }

        if (*LastExifRefdP < ValuePtr+ByteCount){
            /* Keep track of last byte in the exif header that was
               actually referenced.  That way, we know where the
               discardable thumbnail data begins.
            */
            *LastExifRefdP = ValuePtr+ByteCount;
        }

        /* Extract useful components of tag */
        switch(Tag){

            case TAG_MAKE:
                strncpy(pInfo->CameraMake, (char*)ValuePtr, 31);
                break;

            case TAG_MODEL:
                strncpy(pInfo->CameraModel, (char*)ValuePtr, 39);
                break;

            case TAG_DATETIME_ORIGINAL:
                strncpy(pInfo->DateTime, (char*)ValuePtr, 19);
                break;

            case TAG_USERCOMMENT:
                /* Olympus has this padded with trailing spaces.
                   Remove these first. 
                */
                for (a=ByteCount;;){
                    a--;
                    if (((char*)ValuePtr)[a] == ' '){
                        ((char*)ValuePtr)[a] = '\0';
                    }else{
                        break;
                    }
                    if (a == 0) break;
                }

                /* Copy the comment */
                if (memcmp(ValuePtr, "ASCII",5) == 0){
                    for (a=5;a<10;a++){
                        char c;
                        c = ((char*)ValuePtr)[a];
                        if (c != '\0' && c != ' '){
                            strncpy(pInfo->Comments, (char*)ValuePtr+a, 
                                    199);
                            break;
                        }
                    }
                    
                }else{
                    strncpy(pInfo->Comments, (char*)ValuePtr, 199);
                }
                break;

            case TAG_FNUMBER:
                /* Simplest way of expressing aperture, so I trust it the most.
                   (overwrite previously computd value if there is one)
                   */
                pInfo->ApertureFNumber = (float)ConvertAnyFormat(ValuePtr, Format);
                break;

            case TAG_APERTURE:
            case TAG_MAXAPERTURE:
                /* More relevant info always comes earlier, so only
                 use this field if we don't have appropriate aperture
                 information yet. 
                */
                if (pInfo->ApertureFNumber == 0){
                    pInfo->ApertureFNumber = (float)
                        exp(ConvertAnyFormat(ValuePtr, Format)*log(2)*0.5);
                }
                break;

            case TAG_FOCALLENGTH:
                /* Nice digital cameras actually save the focal length
                   as a function of how farthey are zoomed in. 
                */

                pInfo->FocalLength = 
                    (float)ConvertAnyFormat(ValuePtr, Format);
                break;

            case TAG_SUBJECT_DISTANCE:
                /* Inidcates the distacne the autofocus camera is focused to.
                   Tends to be less accurate as distance increases.
                */
                pInfo->Distance = 
                    (float)ConvertAnyFormat(ValuePtr, Format);
                break;

            case TAG_EXPOSURETIME:
                /* Simplest way of expressing exposure time, so I
                   trust it most.  (overwrite previously computd value
                   if there is one) 
                */
                pInfo->ExposureTime = 
                    (float)ConvertAnyFormat(ValuePtr, Format);
                break;

            case TAG_SHUTTERSPEED:
                /* More complicated way of expressing exposure time,
                   so only use this value if we don't already have it
                   from somewhere else.  
                */
                if (pInfo->ExposureTime == 0){
                    pInfo->ExposureTime = (float)
                        (1/exp(ConvertAnyFormat(ValuePtr, Format)*log(2)));
                }
                break;

            case TAG_FLASH:
                if (ConvertAnyFormat(ValuePtr, Format)){
                    pInfo->FlashUsed = 1;
                }
                break;

            case TAG_EXIF_IMAGELENGTH:
            case TAG_EXIF_IMAGEWIDTH:
                /* Use largest of height and width to deal with images
                   that have been rotated to portrait format.  
                */
                a = (int)ConvertAnyFormat(ValuePtr, Format);
                if (ExifImageWidth < a) ExifImageWidth = a;
                break;

            case TAG_FOCALPLANEXRES:
                FocalplaneXRes = ConvertAnyFormat(ValuePtr, Format);
                break;

            case TAG_FOCALPLANEUNITS:
                switch((int)ConvertAnyFormat(ValuePtr, Format)){
                    case 1: FocalplaneUnits = 25.4; break; /* 1 inch */
                    case 2: 
                        /* According to the information I was using, 2
                           means meters.  But looking at the Cannon
                           powershot's files, inches is the only
                           sensible value.  
                        */
                        FocalplaneUnits = 25.4;
                        break;

                    case 3: FocalplaneUnits = 10;   break;  /* 1 centimeter*/
                    case 4: FocalplaneUnits = 1;    break;  /* 1 millimeter*/
                    case 5: FocalplaneUnits = .001; break;  /* 1 micrometer*/
                }
                break;

                /* Remaining cases contributed by: Volker C. Schoech
                   (schoech@gmx.de)
                */

            case TAG_EXPOSURE_BIAS:
                pInfo->ExposureBias = 
                    (float) ConvertAnyFormat(ValuePtr, Format);
                break;

            case TAG_WHITEBALANCE:
                pInfo->Whitebalance = 
                    (int)ConvertAnyFormat(ValuePtr, Format);
                break;

            case TAG_METERING_MODE:
                pInfo->MeteringMode = 
                    (int)ConvertAnyFormat(ValuePtr, Format);
                break;

            case TAG_EXPOSURE_PROGRAM:
                pInfo->ExposureProgram = 
                    (int)ConvertAnyFormat(ValuePtr, Format);
                break;

            case TAG_ISO_EQUIVALENT:
                pInfo->ISOequivalent = 
                    (int)ConvertAnyFormat(ValuePtr, Format);
                if ( pInfo->ISOequivalent < 50 ) 
                    pInfo->ISOequivalent *= 200;
                break;

            case TAG_COMPRESSION_LEVEL:
                pInfo->CompressionLevel = 
                    (int)ConvertAnyFormat(ValuePtr, Format);
                break;

            case TAG_THUMBNAIL_OFFSET:
                ThumbnailOffset = (unsigned)ConvertAnyFormat(ValuePtr, Format);
                break;

            case TAG_THUMBNAIL_LENGTH:
                ThumbnailSize = (unsigned)ConvertAnyFormat(ValuePtr, Format);
                break;

        }

        if (Tag == TAG_EXIF_OFFSET || Tag == TAG_INTEROP_OFFSET){
            unsigned char * SubdirStart;
            SubdirStart = OffsetBase + Get32u(ValuePtr);
            if (SubdirStart < OffsetBase || 
                SubdirStart > OffsetBase+ExifLength){
                strcpy(m_szLastError,"Illegal subdirectory link");
				return false;
            }
            ProcessExifDir(SubdirStart, OffsetBase, ExifLength, pInfo, LastExifRefdP);
            continue;
        }
    }


    {
        /* In addition to linking to subdirectories via exif tags,
           there's also a potential link to another directory at the end
           of each directory.  This has got to be the result of a
           committee!  
        */
        unsigned char * SubdirStart;
        unsigned Offset;
        Offset = Get16u(DirStart+2+12*NumDirEntries);
        if (Offset){
            SubdirStart = OffsetBase + Offset;
            if (SubdirStart < OffsetBase 
                || SubdirStart > OffsetBase+ExifLength){
                strcpy(m_szLastError,"Illegal subdirectory link");
				return false;
            }
            ProcessExifDir(SubdirStart, OffsetBase, ExifLength, pInfo, LastExifRefdP);
        }
    }


    if (ThumbnailSize && ThumbnailOffset){
        if (ThumbnailSize + ThumbnailOffset <= ExifLength){
            /* The thumbnail pointer appears to be valid.  Store it. */
            pInfo->ThumbnailPointer = OffsetBase + ThumbnailOffset;
            pInfo->ThumbnailSize = ThumbnailSize;
        }
    }

	return true;
}
////////////////////////////////////////////////////////////////////////////////
/*--------------------------------------------------------------------------
   Evaluate number, be it int, rational, or float from directory.
--------------------------------------------------------------------------*/
double CxImageJPG::CxExifInfo::ConvertAnyFormat(void * ValuePtr, int Format)
{
    double Value;
    Value = 0;

    switch(Format){
        case FMT_SBYTE:     Value = *(signed char *)ValuePtr;  break;
        case FMT_BYTE:      Value = *(unsigned char *)ValuePtr;        break;

        case FMT_USHORT:    Value = Get16u(ValuePtr);          break;
        case FMT_ULONG:     Value = Get32u(ValuePtr);          break;

        case FMT_URATIONAL:
        case FMT_SRATIONAL: 
            {
                int Num,Den;
                Num = Get32s(ValuePtr);
                Den = Get32s(4+(char *)ValuePtr);
                if (Den == 0){
                    Value = 0;
                }else{
                    Value = (double)Num/Den;
                }
                break;
            }

        case FMT_SSHORT:    Value = (signed short)Get16u(ValuePtr);  break;
        case FMT_SLONG:     Value = Get32s(ValuePtr);                break;

        /* Not sure if this is correct (never seen float used in Exif format)
         */
        case FMT_SINGLE:    Value = (double)*(float *)ValuePtr;      break;
        case FMT_DOUBLE:    Value = *(double *)ValuePtr;             break;
    }
    return Value;
}
////////////////////////////////////////////////////////////////////////////////
/*----------------------------------------------------------------------------
   Return true iff the JPEG miscellaneous marker 'marker' is an Exif 
   header.
-----------------------------------------------------------------------------*/
bool CxImageJPG::CxExifInfo::is_exif(struct jpeg_marker_struct const marker)
{
    bool retval;
    
    if (marker.marker == JPEG_APP0+1) {
        if (memcmp(marker.data, "Exif", 4) == 0)
            retval = true;
        else retval = false;
    }
    else retval = false;

    return retval;
}
////////////////////////////////////////////////////////////////////////////////
#endif 	// CXIMAGEJPG_SUPPORT_EXIF

