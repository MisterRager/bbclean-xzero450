#include "BBApi.h"

#include "ColorMaster.h"

//##################################################

//===========================================================================
// Function: ParseLiteralColor
// Purpose: Parses a given literal colour and returns the hex value
// In: LPCSTR = color to parse (eg. "black", "white")
// Out: COLORREF (DWORD) of rgb value
// (old)Out: LPCSTR = literal hex value
//===========================================================================

static struct litcolor1 { char *cname; COLORREF cref; } litcolor1_ary[] = {

    { "ghostwhite", RGB(248,248,255) },
    { "whitesmoke", RGB(245,245,245) },
    { "gainsboro", RGB(220,220,220) },
    { "floralwhite", RGB(255,250,240) },
    { "oldlace", RGB(253,245,230) },
    { "linen", RGB(250,240,230) },
    { "antiquewhite", RGB(250,235,215) },
    { "papayawhip", RGB(255,239,213) },
    { "blanchedalmond", RGB(255,235,205) },
    { "bisque", RGB(255,228,196) },
    { "peachpuff", RGB(255,218,185) },
    { "navajowhite", RGB(255,222,173) },
    { "moccasin", RGB(255,228,181) },
    { "cornsilk", RGB(255,248,220) },
    { "ivory", RGB(255,255,240) },
    { "lemonchiffon", RGB(255,250,205) },
    { "seashell", RGB(255,245,238) },
    { "honeydew", RGB(240,255,240) },
    { "mintcream", RGB(245,255,250) },
    { "azure", RGB(240,255,255) },
    { "aliceblue", RGB(240,248,255) },
    { "lavender", RGB(230,230,250) },
    { "lavenderblush", RGB(255,240,245) },
    { "mistyrose", RGB(255,228,225) },
    { "white", RGB(255,255,255) },
    { "black", RGB(0,0,0) },
    { "darkslategray", RGB(47,79,79) },
    { "dimgray", RGB(105,105,105) },
    { "slategray", RGB(112,128,144) },
    { "lightslategray", RGB(119,136,153) },
    { "gray", RGB(190,190,190) },
    { "lightgray", RGB(211,211,211) },
    { "midnightblue", RGB(25,25,112) },
    { "navy", RGB(0,0,128) },
    { "navyblue", RGB(0,0,128) },
    { "cornflowerblue", RGB(100,149,237) },
    { "darkslateblue", RGB(72,61,139) },
    { "slateblue", RGB(106,90,205) },
    { "mediumslateblue", RGB(123,104,238) },
    { "lightslateblue", RGB(132,112,255) },
    { "mediumblue", RGB(0,0,205) },
    { "royalblue", RGB(65,105,225) },
    { "blue", RGB(0,0,255) },
    { "dodgerblue", RGB(30,144,255) },
    { "deepskyblue", RGB(0,191,255) },
    { "skyblue", RGB(135,206,235) },
    { "lightskyblue", RGB(135,206,250) },
    { "steelblue", RGB(70,130,180) },
    { "lightsteelblue", RGB(176,196,222) },
    { "lightblue", RGB(173,216,230) },
    { "powderblue", RGB(176,224,230) },
    { "paleturquoise", RGB(175,238,238) },
    { "darkturquoise", RGB(0,206,209) },
    { "mediumturquoise", RGB(72,209,204) },
    { "turquoise", RGB(64,224,208) },
    { "cyan", RGB(0,255,255) },
    { "lightcyan", RGB(224,255,255) },
    { "cadetblue", RGB(95,158,160) },
    { "mediumaquamarine", RGB(102,205,170) },
    { "aquamarine", RGB(127,255,212) },
    { "darkgreen", RGB(0,100,0) },
    { "darkolivegreen", RGB(85,107,47) },
    { "darkseagreen", RGB(143,188,143) },
    { "seagreen", RGB(46,139,87) },
    { "mediumseagreen", RGB(60,179,113) },
    { "lightseagreen", RGB(32,178,170) },
    { "palegreen", RGB(152,251,152) },
    { "springgreen", RGB(0,255,127) },
    { "lawngreen", RGB(124,252,0) },
    { "green", RGB(0,255,0) },
    { "chartreuse", RGB(127,255,0) },
    { "mediumspringgreen", RGB(0,250,154) },
    { "greenyellow", RGB(173,255,47) },
    { "limegreen", RGB(50,205,50) },
    { "yellowgreen", RGB(154,205,50) },
    { "forestgreen", RGB(34,139,34) },
    { "olivedrab", RGB(107,142,35) },
    { "darkkhaki", RGB(189,183,107) },
    { "khaki", RGB(240,230,140) },
    { "palegoldenrod", RGB(238,232,170) },
    { "lightgoldenrodyellow", RGB(250,250,210) },
    { "lightyellow", RGB(255,255,224) },
    { "yellow", RGB(255,255,0) },
    { "gold", RGB(255,215,0) },
    { "lightgoldenrod", RGB(238,221,130) },
    { "goldenrod", RGB(218,165,32) },
    { "darkgoldenrod", RGB(184,134,11) },
    { "rosybrown", RGB(188,143,143) },
    { "indianred", RGB(205,92,92) },
    { "saddlebrown", RGB(139,69,19) },
    { "sienna", RGB(160,82,45) },
    { "peru", RGB(205,133,63) },
    { "burlywood", RGB(222,184,135) },
    { "beige", RGB(245,245,220) },
    { "wheat", RGB(245,222,179) },
    { "sandybrown", RGB(244,164,96) },
    { "tan", RGB(210,180,140) },
    { "chocolate", RGB(210,105,30) },
    { "firebrick", RGB(178,34,34) },
    { "brown", RGB(165,42,42) },
    { "darksalmon", RGB(233,150,122) },
    { "salmon", RGB(250,128,114) },
    { "lightsalmon", RGB(255,160,122) },
    { "orange", RGB(255,165,0) },
    { "darkorange", RGB(255,140,0) },
    { "coral", RGB(255,127,80) },
    { "lightcoral", RGB(240,128,128) },
    { "tomato", RGB(255,99,71) },
    { "orangered", RGB(255,69,0) },
    { "red", RGB(255,0,0) },
    { "hotpink", RGB(255,105,180) },
    { "deeppink", RGB(255,20,147) },
    { "pink", RGB(255,192,203) },
    { "lightpink", RGB(255,182,193) },
    { "palevioletred", RGB(219,112,147) },
    { "maroon", RGB(176,48,96) },
    { "mediumvioletred", RGB(199,21,133) },
    { "violetred", RGB(208,32,144) },
    { "magenta", RGB(255,0,255) },
    { "violet", RGB(238,130,238) },
    { "plum", RGB(221,160,221) },
    { "orchid", RGB(218,112,214) },
    { "mediumorchid", RGB(186,85,211) },
    { "darkorchid", RGB(153,50,204) },
    { "darkviolet", RGB(148,0,211) },
    { "blueviolet", RGB(138,43,226) },
    { "purple", RGB(160,32,240) },
    { "mediumpurple", RGB(147,112,219) },
    { "thistle", RGB(216,191,216) },

    { "darkgray", RGB(169,169,169) },
    { "darkblue", RGB(0,0,139) },
    { "darkcyan", RGB(0,139,139) },
    { "darkmagenta", RGB(139,0,139) },
    { "darkred", RGB(139,0,0) },
    { "lightgreen", RGB(144,238,144) }
    };

static struct litcolor4 { char *cname; COLORREF cref[4]; } litcolor4_ary[] = {

    { "snow", { RGB(255,250,250), RGB(238,233,233), RGB(205,201,201), RGB(139,137,137) }},
    { "seashell", { RGB(255,245,238), RGB(238,229,222), RGB(205,197,191), RGB(139,134,130) }},
    { "antiquewhite", { RGB(255,239,219), RGB(238,223,204), RGB(205,192,176), RGB(139,131,120) }},
    { "bisque", { RGB(255,228,196), RGB(238,213,183), RGB(205,183,158), RGB(139,125,107) }},
    { "peachpuff", { RGB(255,218,185), RGB(238,203,173), RGB(205,175,149), RGB(139,119,101) }},
    { "navajowhite", { RGB(255,222,173), RGB(238,207,161), RGB(205,179,139), RGB(139,121,94) }},
    { "lemonchiffon", { RGB(255,250,205), RGB(238,233,191), RGB(205,201,165), RGB(139,137,112) }},
    { "cornsilk", { RGB(255,248,220), RGB(238,232,205), RGB(205,200,177), RGB(139,136,120) }},
    { "ivory", { RGB(255,255,240), RGB(238,238,224), RGB(205,205,193), RGB(139,139,131) }},
    { "honeydew", { RGB(240,255,240), RGB(224,238,224), RGB(193,205,193), RGB(131,139,131) }},
    { "lavenderblush", { RGB(255,240,245), RGB(238,224,229), RGB(205,193,197), RGB(139,131,134) }},
    { "mistyrose", { RGB(255,228,225), RGB(238,213,210), RGB(205,183,181), RGB(139,125,123) }},
    { "azure", { RGB(240,255,255), RGB(224,238,238), RGB(193,205,205), RGB(131,139,139) }},
    { "slateblue", { RGB(131,111,255), RGB(122,103,238), RGB(105,89,205), RGB(71,60,139) }},
    { "royalblue", { RGB(72,118,255), RGB(67,110,238), RGB(58,95,205), RGB(39,64,139) }},
    { "blue", { RGB(0,0,255), RGB(0,0,238), RGB(0,0,205), RGB(0,0,139) }},
    { "dodgerblue", { RGB(30,144,255), RGB(28,134,238), RGB(24,116,205), RGB(16,78,139) }},
    { "steelblue", { RGB(99,184,255), RGB(92,172,238), RGB(79,148,205), RGB(54,100,139) }},
    { "deepskyblue", { RGB(0,191,255), RGB(0,178,238), RGB(0,154,205), RGB(0,104,139) }},
    { "skyblue", { RGB(135,206,255), RGB(126,192,238), RGB(108,166,205), RGB(74,112,139) }},
    { "lightskyblue", { RGB(176,226,255), RGB(164,211,238), RGB(141,182,205), RGB(96,123,139) }},
    { "slategray", { RGB(198,226,255), RGB(185,211,238), RGB(159,182,205), RGB(108,123,139) }},
    { "lightsteelblue", { RGB(202,225,255), RGB(188,210,238), RGB(162,181,205), RGB(110,123,139) }},
    { "lightblue", { RGB(191,239,255), RGB(178,223,238), RGB(154,192,205), RGB(104,131,139) }},
    { "lightcyan", { RGB(224,255,255), RGB(209,238,238), RGB(180,205,205), RGB(122,139,139) }},
    { "paleturquoise", { RGB(187,255,255), RGB(174,238,238), RGB(150,205,205), RGB(102,139,139) }},
    { "cadetblue", { RGB(152,245,255), RGB(142,229,238), RGB(122,197,205), RGB(83,134,139) }},
    { "turquoise", { RGB(0,245,255), RGB(0,229,238), RGB(0,197,205), RGB(0,134,139) }},
    { "cyan", { RGB(0,255,255), RGB(0,238,238), RGB(0,205,205), RGB(0,139,139) }},
    { "darkslategray", { RGB(151,255,255), RGB(141,238,238), RGB(121,205,205), RGB(82,139,139) }},
    { "aquamarine", { RGB(127,255,212), RGB(118,238,198), RGB(102,205,170), RGB(69,139,116) }},
    { "darkseagreen", { RGB(193,255,193), RGB(180,238,180), RGB(155,205,155), RGB(105,139,105) }},
    { "seagreen", { RGB(84,255,159), RGB(78,238,148), RGB(67,205,128), RGB(46,139,87) }},
    { "palegreen", { RGB(154,255,154), RGB(144,238,144), RGB(124,205,124), RGB(84,139,84) }},
    { "springgreen", { RGB(0,255,127), RGB(0,238,118), RGB(0,205,102), RGB(0,139,69) }},
    { "green", { RGB(0,255,0), RGB(0,238,0), RGB(0,205,0), RGB(0,139,0) }},
    { "chartreuse", { RGB(127,255,0), RGB(118,238,0), RGB(102,205,0), RGB(69,139,0) }},
    { "olivedrab", { RGB(192,255,62), RGB(179,238,58), RGB(154,205,50), RGB(105,139,34) }},
    { "darkolivegreen", { RGB(202,255,112), RGB(188,238,104), RGB(162,205,90), RGB(110,139,61) }},
    { "khaki", { RGB(255,246,143), RGB(238,230,133), RGB(205,198,115), RGB(139,134,78) }},
    { "lightgoldenrod", { RGB(255,236,139), RGB(238,220,130), RGB(205,190,112), RGB(139,129,76) }},
    { "lightyellow", { RGB(255,255,224), RGB(238,238,209), RGB(205,205,180), RGB(139,139,122) }},
    { "yellow", { RGB(255,255,0), RGB(238,238,0), RGB(205,205,0), RGB(139,139,0) }},
    { "gold", { RGB(255,215,0), RGB(238,201,0), RGB(205,173,0), RGB(139,117,0) }},
    { "goldenrod", { RGB(255,193,37), RGB(238,180,34), RGB(205,155,29), RGB(139,105,20) }},
    { "darkgoldenrod", { RGB(255,185,15), RGB(238,173,14), RGB(205,149,12), RGB(139,101,8) }},
    { "rosybrown", { RGB(255,193,193), RGB(238,180,180), RGB(205,155,155), RGB(139,105,105) }},
    { "indianred", { RGB(255,106,106), RGB(238,99,99), RGB(205,85,85), RGB(139,58,58) }},
    { "sienna", { RGB(255,130,71), RGB(238,121,66), RGB(205,104,57), RGB(139,71,38) }},
    { "burlywood", { RGB(255,211,155), RGB(238,197,145), RGB(205,170,125), RGB(139,115,85) }},
    { "wheat", { RGB(255,231,186), RGB(238,216,174), RGB(205,186,150), RGB(139,126,102) }},
    { "tan", { RGB(255,165,79), RGB(238,154,73), RGB(205,133,63), RGB(139,90,43) }},
    { "chocolate", { RGB(255,127,36), RGB(238,118,33), RGB(205,102,29), RGB(139,69,19) }},
    { "firebrick", { RGB(255,48,48), RGB(238,44,44), RGB(205,38,38), RGB(139,26,26) }},
    { "brown", { RGB(255,64,64), RGB(238,59,59), RGB(205,51,51), RGB(139,35,35) }},
    { "salmon", { RGB(255,140,105), RGB(238,130,98), RGB(205,112,84), RGB(139,76,57) }},
    { "lightsalmon", { RGB(255,160,122), RGB(238,149,114), RGB(205,129,98), RGB(139,87,66) }},
    { "orange", { RGB(255,165,0), RGB(238,154,0), RGB(205,133,0), RGB(139,90,0) }},
    { "darkorange", { RGB(255,127,0), RGB(238,118,0), RGB(205,102,0), RGB(139,69,0) }},
    { "coral", { RGB(255,114,86), RGB(238,106,80), RGB(205,91,69), RGB(139,62,47) }},
    { "tomato", { RGB(255,99,71), RGB(238,92,66), RGB(205,79,57), RGB(139,54,38) }},
    { "orangered", { RGB(255,69,0), RGB(238,64,0), RGB(205,55,0), RGB(139,37,0) }},
    { "red", { RGB(255,0,0), RGB(238,0,0), RGB(205,0,0), RGB(139,0,0) }},
    { "deeppink", { RGB(255,20,147), RGB(238,18,137), RGB(205,16,118), RGB(139,10,80) }},
    { "hotpink", { RGB(255,110,180), RGB(238,106,167), RGB(205,96,144), RGB(139,58,98) }},
    { "pink", { RGB(255,181,197), RGB(238,169,184), RGB(205,145,158), RGB(139,99,108) }},
    { "lightpink", { RGB(255,174,185), RGB(238,162,173), RGB(205,140,149), RGB(139,95,101) }},
    { "palevioletred", { RGB(255,130,171), RGB(238,121,159), RGB(205,104,137), RGB(139,71,93) }},
    { "maroon", { RGB(255,52,179), RGB(238,48,167), RGB(205,41,144), RGB(139,28,98) }},
    { "violetred", { RGB(255,62,150), RGB(238,58,140), RGB(205,50,120), RGB(139,34,82) }},
    { "magenta", { RGB(255,0,255), RGB(238,0,238), RGB(205,0,205), RGB(139,0,139) }},
    { "orchid", { RGB(255,131,250), RGB(238,122,233), RGB(205,105,201), RGB(139,71,137) }},
    { "plum", { RGB(255,187,255), RGB(238,174,238), RGB(205,150,205), RGB(139,102,139) }},
    { "mediumorchid", { RGB(224,102,255), RGB(209,95,238), RGB(180,82,205), RGB(122,55,139) }},
    { "darkorchid", { RGB(191,62,255), RGB(178,58,238), RGB(154,50,205), RGB(104,34,139) }},
    { "purple", { RGB(155,48,255), RGB(145,44,238), RGB(125,38,205), RGB(85,26,139) }},
    { "mediumpurple", { RGB(171,130,255), RGB(159,121,238), RGB(137,104,205), RGB(93,71,139) }},
    { "thistle", { RGB(255,225,255), RGB(238,210,238), RGB(205,181,205), RGB(139,123,139) }}
    };


COLORREF ParseLiteralColor(LPCSTR colour)
{
    int i, n; unsigned l; char *p, c, buf[32];
    l = strlen(colour) + 1;
    if (l > 2 && l < sizeof buf)
    {
        memcpy(buf, colour, l); //strlwr(buf);
        while (NULL!=(p=strchr(buf,' '))) strcpy(p, p+1);
        if (NULL!=(p=strstr(buf,"grey"))) p[2]='a';
        if (0==memcmp(buf,"gray", 4) && (c=buf[4]) >= '0' && c <= '9')
        {
            i = atoi(buf+4);
            if (i >= 0 && i <= 100)
            {
                i = (i * 255 + 50) / 100;
                return RGB(i,i,i);
            }
        }
        i = *(p = &buf[l-2]) - '1';
        if (i>=0 && i<4)
        {
            *p=0; --l;
            struct litcolor4 *cp4=litcolor4_ary;
            n = sizeof(litcolor4_ary) / sizeof(*cp4);
            do { if (0==memcmp(buf, cp4->cname, l)) return cp4->cref[i]; cp4++; }
            while (--n);
        }
        else
        {
            struct litcolor1 *cp1=litcolor1_ary;
            n = sizeof(litcolor1_ary) / sizeof(*cp1);
            do { if (0==memcmp(buf, cp1->cname, l)) return cp1->cref; cp1++; }
            while (--n);
        }
    }
    return (COLORREF)-1;
}


COLORREF switch_rgb (COLORREF c)
{ return (c&0x0000ff)<<16 | (c&0x00ff00) | (c&0xff0000)>>16; }
//===========================================================================
// Function: ReadColorFromString
// Purpose: parse a literal or hexadezimal color string
// --- Straight from the bblean source.

COLORREF ReadColorFromString(char * string)
{
    if (NULL == string) return (COLORREF)-1;
    char rgbstr[7];
    char *s = strlwr(string);
    if ('#'==*s) s++;
    for (;;)
    {
        COLORREF cr = 0; char *d, c;
        // check if its a valid hex number
        for (d = s; (c = *d) != 0; ++d)
        {
            cr <<= 4;
            if (c >= '0' && c <= '9') cr |= c - '0';
            else
            if (c >= 'a' && c <= 'f') cr |= c - ('a'-10);
            else goto check_rgb;
        }

        if (d - s == 3) // #AB4 short type colors
            cr = (cr&0xF00)<<12 | (cr&0xFF0)<<8 | (cr&0x0FF)<<4 | cr&0x00F;

        return switch_rgb(cr);

check_rgb:
        // check if its an "rgb:12/ee/4c" type string
//        s = stub;
        if (0 == memcmp(s, "rgb:", 4))
        {
            int j=3; s+=4; d = rgbstr;
            do {
                d[0] = *s && '/'!=*s ? *s++ : '0';
                d[1] = *s && '/'!=*s ? *s++ : d[0];
                d+=2; if ('/'==*s) ++s;
            } while (--j);
            *d=0; s = rgbstr;
            continue;
        } else
		// Check for an "rgb10:123/45/255" type string
       if (0 == memcmp(s, "rgb10:", 6))
        {
            s+=6;
			int cval[3];
			if (sscanf(s,"%d/%d/%d",cval,cval+1,cval+2) == 3)
			{
				sprintf(rgbstr,"%02x%02x%02x",cval[0]%256,cval[1]%256,cval[2]%256);
				s = rgbstr;
				continue;
			}
        }

        // must be one of the literal color names (or is invalid)
        return ParseLiteralColor(s);
    }
}
