/*******************************************************************
 *
 *  truetype.h
 *
 *    Backwards-compatible high-level interface for the TrueType
 *    driver. This file is a replacement for the old "freetype.h"
 *    from 1.0 and 1.1. It can be used to compile applications
 *    and tools using the old API..
 *
 *    Note that this (old) interface is now deprecated and won't
 *    be modified in the future. Developers will have to switch to
 *    the newer interface to get new features ( kerning, embedded
 *    bitmaps, etc.. ).
 *
 *
 *
 *  Copyright 1996-1998 by
 *  David Turner, Robert Wilhelm, and Werner Lemberg.
 *
 *  This file is part of the FreeType project and may only be used,
 *  modified, and distributed under the terms of the FreeType project
 *  license, LICENSE.TXT.  By continuing to use, modify, or distribute 
 *  this file you indicate that you have read the license and
 *  understand and accept it fully.
 *
 *  Note:
 *
 *    This is the only file that should be included by client            
 *    application sources.  All other types and functions defined
 *    in the "tt*.h" files are library internals and should not be
 *    included.
 *
 ******************************************************************/

#ifndef TRUETYPE_H
#define TRUETYPE_H

/* To make freetype.h independent from configuration files we check */
/* whether EXPORT_DEF has been defined already.                     */

#ifndef EXPORT_DEF
#define EXPORT_DEF extern
#endif

/* The same for TT_Text.  If you define the HAVE_TT_TEXT macro, you */
/* have to provide a typedef declaration for TT_Text before         */
/* including this file.                                             */

#ifndef HAVE_TT_TEXT
#define HAVE_TT_TEXT
  typedef char  TT_Text;              /* the data type to represent */
                                      /* file name string elements  */
#endif

#ifdef __cplusplus
  extern "C" {
#endif


/* The following types are also defined in "drivers/ttlib/ttobjs.h"  */
/* We use the _TRUETYPE_ macro to prevent their redefinition when    */
/* _compiling_ the backwards-compatible layer called "ttapi.c"       */
/*                                                                   */
#ifndef _TRUETYPE_

  /*******************************************************************/
  /*                                                                 */
  /*  FreeType types definitions.                                    */
  /*                                                                 */
  /*  All these begin with a 'TT_' prefix.                           */
  /*                                                                 */
  /*******************************************************************/

  typedef unsigned short  TT_Bool;

  typedef signed long     TT_Fixed;   /* Signed Fixed 16.16 Float */

  typedef signed short    TT_FWord;   /* Distance in FUnits */
  typedef unsigned short  TT_UFWord;  /* Unsigned distance */

  typedef char            TT_String;
  typedef signed char     TT_Char;
  typedef unsigned char   TT_Byte;
  typedef signed short    TT_Short;
  typedef unsigned short  TT_UShort;
  typedef signed long     TT_Long;
  typedef unsigned long   TT_ULong;
  typedef int             TT_Int;

  typedef signed short    TT_F2Dot14; /* Signed fixed float 2.14 used for */
                                      /* unit vectors, with layout:       */
                                      /*                                  */
                                      /*  s : 1  -- sign bit              */
                                      /*  m : 1  -- integer bit           */
                                      /*  f : 14 -- unsigned fractional   */
                                      /*                                  */
                                      /*  's:m' is the 2-bit signed int   */
                                      /*  value to which the positive     */
                                      /*  fractional part should be       */
                                      /*  added.                          */
                                      /*                                  */

  typedef signed long     TT_F26Dot6; /* 26.6 fixed float, used for       */
                                      /* glyph points pixel coordinates.  */

  typedef signed long     TT_Pos;     /* point position, expressed either */
                                      /* in fractional pixels or notional */
                                      /* units, depending on context.     */
                                      /* For example, glyph coordinates   */
                                      /* returned by TT_Load_Glyph are    */
                                      /* expressed in font units when     */
                                      /* scaling wasn't requested, and    */
                                      /* in 26.6 fractional pixels if it  */
                                      /* was.                             */


  struct  TT_UnitVector_      /* guess what...  */
  { 
    TT_F2Dot14  x;
    TT_F2Dot14  y;
  };

  typedef struct TT_UnitVector_  TT_UnitVector;


  struct  TT_Vector_          /* Simple vector type */
  {
    TT_F26Dot6  x;
    TT_F26Dot6  y;
  };

  typedef struct TT_Vector_  TT_Vector;


  /* A simple 2x2 matrix used for transformations. */
  /* You should use 16.16 fixed floats.            */
  /*                                               */
  /*  x' = xx*x + xy*y                             */
  /*  y' = yx*x + yy*y                             */
  /*                                               */

  struct  TT_Matrix_
  {
    TT_Fixed  xx, xy;
    TT_Fixed  yx, yy;
  };

  typedef struct TT_Matrix_  TT_Matrix;


  /* A structure used to describe a simple bounding box */

  struct TT_BBox_
  {
    TT_Pos  xMin;
    TT_Pos  yMin;
    TT_Pos  xMax;
    TT_Pos  yMax;
  };

  typedef struct TT_BBox_  TT_BBox;



#endif /* _TRUETYPE_ */




  /* A structure used to describe the source glyph to the renderer. */

  struct  TT_Outline_
  {
    TT_Short         n_contours;   /* number of contours in glyph        */
    TT_UShort        n_points;     /* number of points in the glyph      */

    TT_Vector*       points;       /* the outline's points   */
    TT_Byte*         flags;        /* the points flags       */
    TT_UShort*       contours;     /* the contour end points */

    /* The following flag indicates that the outline owns the arrays it  */
    /* refers to.  Typically, this is true of outlines created from the  */
    /* TT_New_Outline() API, while it isn't for those returned by        */
    /* TT_Get_Glyph_Outline().                                           */

    TT_Bool          owner;      /* the outline owns the coordinates,    */
                                 /* flags and contours array it uses     */

    /* The following flags are set automatically by                      */
    /* TT_Get_Glyph_Outline().  Their meaning is the following:          */
    /*                                                                   */
    /*  high_precision   When true, the scan-line converter will use     */
    /*                   a higher precision to render bitmaps (i.e. a    */
    /*                   1/1024 pixel precision).  This is important for */
    /*                   small ppem sizes.                               */
    /*                                                                   */
    /*  second_pass      When true, the scan-line converter performs     */
    /*                   a second sweep phase dedicated to find          */
    /*                   vertical drop-outs.  If false, only horizontal  */
    /*                   drop-outs will be checked during the first      */
    /*                   vertical sweep (yes, this is a bit confusing    */
    /*                   but it's really the way it should work).        */
    /*                   This is important for small ppems too.          */
    /*                                                                   */
    /*  dropout_mode     Specifies the TrueType drop-out mode to         */
    /*                   use for continuity checking. valid values       */
    /*                   are 0 (no check), 1, 2, 4, and 5.               */
    /*                                                                   */
    /*  Most of the engine's users will safely ignore these fields...    */

    TT_Bool          high_precision;  /* high precision rendering */
    TT_Bool          second_pass;     /* two sweeps rendering     */
    TT_Char          dropout_mode;    /* dropout mode */
  };

  typedef struct TT_Outline_  TT_Outline;

 
  /* A structure used to return glyph metrics.                          */
  /*                                                                    */
  /* The "bearingX" isn't called "left-side bearing" anymore because    */
  /* it has different meanings depending on the glyph's orientation.    */
  /*                                                                    */
  /* The same is true for "bearingY", which is the top-side bearing     */
  /* defined by the TT_Spec, i.e., the distance from the baseline to    */
  /* the top of the glyph's bbox.  According to our current convention, */
  /* this is always the same as "bbox.yMax" but we make it appear for   */
  /* consistency in its proper field.                                   */
  /*                                                                    */
  /* The "advance" width is the advance width for horizontal layout,    */
  /* and advance height for vertical layouts.                           */
  /*                                                                    */
  /* Finally, the library (ver. 1.1) doesn't support vertical text yet  */
  /* but these changes were introduced to accomodate it, as it will     */
  /* most certainly be introduced in later releases.                    */

  struct  TT_Glyph_Metrics_
  {
    TT_BBox  bbox;      /* glyph bounding box */

    TT_Pos   bearingX;  /* left-side bearing                    */
    TT_Pos   bearingY;  /* top-side bearing, per se the TT spec */

    TT_Pos   advance;   /* advance width (or height) */
  };


  /* A structure used to return horizontal _and_ vertical glyph         */
  /* metrics.                                                           */
  /*                                                                    */
  /* A glyph can be used either in a horizontal or vertical layout.     */
  /* Its glyph metrics vary with orientation.  The Big_Glyph_Metrics    */
  /* structure is used to return _all_ metrics in one call.             */
  /*                                                                    */

  struct TT_Big_Glyph_Metrics_
  {
    TT_BBox  bbox;          /* glyph bounding box */

    TT_Pos   horiBearingX;  /* left side bearing in horizontal layouts */
    TT_Pos   horiBearingY;  /* top side bearing in horizontal layouts  */

    TT_Pos   vertBearingX;  /* left side bearing in vertical layouts */
    TT_Pos   vertBearingY;  /* top side bearing in vertical layouts  */

    TT_Pos   horiAdvance;   /* advance width for horizontal layout */
    TT_Pos   vertAdvance;   /* advance height for vertical layout  */

    /* The following fields represent unhinted scaled metrics values. */
    /* They can be useful for applications needing to do some device  */
    /* independent placement of glyphs.                               */
    /*                                                                */
    /* Applying these metrics to hinted glyphs will most surely ruin  */
    /* the grid fitting performed by the bytecode interpreter.  These */
    /* values are better used to compute accumulated positioning      */
    /* distances.                                                     */

    TT_Pos   linearHoriBearingX;  /* linearly scaled horizontal lsb     */
    TT_Pos   linearHoriAdvance;   /* linearly scaled horizontal advance */

    TT_Pos   linearVertBearingY;  /* linearly scaled vertical tsb     */
    TT_Pos   linearVertAdvance;   /* linearly scaled vertical advance */
  };

  typedef struct TT_Glyph_Metrics_      TT_Glyph_Metrics;
  typedef struct TT_Big_Glyph_Metrics_  TT_Big_Glyph_Metrics;


  /* A structure used to return instance metrics. */

  struct  TT_Instance_Metrics_
  {
    TT_F26Dot6  pointSize;     /* char. size in points (1pt = 1/72 inch) */

    TT_UShort   x_ppem;        /* horizontal pixels per EM square */
    TT_UShort   y_ppem;        /* vertical pixels per EM square   */

    TT_Fixed    x_scale;     /* 16.16 to convert from EM units to 26.6 pix */
    TT_Fixed    y_scale;     /* 16.16 to convert from EM units to 26.6 pix */

    TT_UShort   x_resolution;  /* device horizontal resolution in dpi */
    TT_UShort   y_resolution;  /* device vertical resolution in dpi   */
  };

  typedef struct TT_Instance_Metrics_  TT_Instance_Metrics;


  /* Flow constants:                                             */
  /*                                                             */
  /* The flow of a bitmap refers to the way lines are oriented   */
  /* within the bitmap data, i.e., the orientation of the Y      */
  /* coordinate axis.                                            */

  /* For example, if the first bytes of the bitmap pertain to    */
  /* its top-most line, then the flow is 'down'.  If these bytes */
  /* pertain to its lowest line, the the flow is 'up'.           */

#define TT_Flow_Down  -1  /* bitmap is oriented from top to bottom */
#define TT_Flow_Up     1  /* bitmap is oriented from bottom to top */
#define TT_Flow_Error  0  /* an error occurred during rendering    */


  /* A structure used to describe the target bitmap or pixmap to the   */
  /* renderer.  Note that there is nothing in this structure that      */
  /* gives the nature of the buffer.                                   */

  /* IMPORTANT NOTE:                                                   */
  /*                                                                   */
  /*   In the case of a pixmap, the 'width' and 'cols' fields must     */
  /*   have the _same_ values, and _must_ be padded to 32-bits, i.e.,  */
  /*   be a multiple of 4.  Clipping problems will arise otherwise,    */
  /*   if not even page faults!                                        */
  /*                                                                   */
  /*   The typical settings are:                                       */
  /*                                                                   */
  /*   - for an WxH bitmap:                                            */
  /*                                                                   */
  /*       rows  = H                                                   */
  /*       cols  = (W+7)/8                                             */
  /*       width = W                                                   */
  /*       flow  = your_choice                                         */
  /*                                                                   */
  /*   - for an WxH pixmap:                                            */
  /*                                                                   */
  /*       rows  = H                                                   */
  /*       cols  = (W+3) & ~3                                          */
  /*       width = cols                                                */
  /*       flow  = your_choice                                         */

  struct  TT_Raster_Map_
  {
    int    rows;    /* number of rows                    */
    int    cols;    /* number of columns (bytes) per row */
    int    width;   /* number of pixels per line         */
    int    flow;    /* bitmap orientation                */

    void*  bitmap;  /* bit/pixmap buffer                 */
    long   size;    /* bit/pixmap size in bytes          */
  };

  typedef struct TT_Raster_Map_  TT_Raster_Map;



/* The following tables are also defined in "drivers/ttlib/ttobjs.h" */
/* We use the _TRUETYPE_ macro to prevent their redefinition when    */
/* _compiling_ the backwards-compatible layer called "oldapi.c"      */
/*                                                                   */
#ifndef _TRUETYPE_

  /* ------- The font header TrueType table structure ----- */

  struct  TT_Header_
  {
    TT_Fixed   Table_Version;
    TT_Fixed   Font_Revision;

    TT_Long    CheckSum_Adjust;
    TT_Long    Magic_Number;

    TT_UShort  Flags;
    TT_UShort  Units_Per_EM;

    TT_Long    Created [2];
    TT_Long    Modified[2];

    TT_FWord   xMin;
    TT_FWord   yMin;
    TT_FWord   xMax;
    TT_FWord   yMax;

    TT_UShort  Mac_Style;
    TT_UShort  Lowest_Rec_PPEM;

    TT_Short   Font_Direction;
    TT_Short   Index_To_Loc_Format;
    TT_Short   Glyph_Data_Format;
  };

  typedef struct TT_Header_  TT_Header;


  /* ------- The horizontal header TrueType table structure ----- */

  /*******************************************************/
  /*  This structure is the one defined by the TrueType  */
  /*  specification, plus two fields used to link the    */
  /*  font-units metrics to the header.                  */

  struct  TT_Horizontal_Header_
  {
    TT_Fixed   Version;
    TT_FWord   Ascender;
    TT_FWord   Descender;
    TT_FWord   Line_Gap;

    TT_UFWord  advance_Width_Max;      /* advance width maximum */

    TT_FWord   min_Left_Side_Bearing;  /* minimum left-sb       */
    TT_FWord   min_Right_Side_Bearing; /* minimum right-sb      */
    TT_FWord   xMax_Extent;            /* xmax extents          */
    TT_FWord   caret_Slope_Rise;
    TT_FWord   caret_Slope_Run;

    TT_Short   caret_Offset;           /* only used in vertical header */
    TT_Short   Reserved[4];

    TT_Short   metric_Data_Format;
    TT_UShort  number_Of_HMetrics;
    
    /* The following fields are not defined by the TrueType specification */
    /* but they're used to connect the metrics header to the relevant     */
    /* "HMTX" or "VMTX" table.                                            */

    void*      long_metrics;
    void*      short_metrics;
  };


  /*******************************************************/
  /*  This structure is the one defined by the TrueType  */
  /*  specification.  Note that it has exactly the same  */
  /*  layout as the horizontal header (both are loaded   */
  /*  by the same function).                             */

  struct  TT_Vertical_Header_
  {
    TT_Fixed   Version;
    TT_FWord   Ascender;
    TT_FWord   Descender;
    TT_FWord   Line_Gap;

    TT_UFWord  advance_Height_Max;      /* advance height maximum */

    TT_FWord   min_Top_Side_Bearing;    /* minimum left-sb or top-sb       */
    TT_FWord   min_Bottom_Side_Bearing; /* minimum right-sb or bottom-sb   */
    TT_FWord   yMax_Extent;             /* xmax or ymax extents            */
    TT_FWord   caret_Slope_Rise;
    TT_FWord   caret_Slope_Run;
    TT_FWord   caret_Offset;

    TT_Short   Reserved[4];

    TT_Short   metric_Data_Format;
    TT_UShort  number_Of_VMetrics;
    
    /* The following fields are not defined by the TrueType specification */
    /* but they're used to connect the metrics header to the relevant     */
    /* "HMTX" or "VMTX" table.                                            */

    void*      long_metrics;
    void*      short_metrics;
  };


  typedef struct TT_Horizontal_Header_  TT_Horizontal_Header;
  typedef struct TT_Vertical_Header_    TT_Vertical_Header;


  /* ----------- OS/2 Table ----------------------------- */

  struct  TT_OS2_
  {
    TT_UShort  version;                /* 0x0001 */
    TT_FWord   xAvgCharWidth;
    TT_UShort  usWeightClass;
    TT_UShort  usWidthClass;
    TT_Short   fsType;
    TT_FWord   ySubscriptXSize;
    TT_FWord   ySubscriptYSize;
    TT_FWord   ySubscriptXOffset;
    TT_FWord   ySubscriptYOffset;
    TT_FWord   ySuperscriptXSize;
    TT_FWord   ySuperscriptYSize;
    TT_FWord   ySuperscriptXOffset;
    TT_FWord   ySuperscriptYOffset;
    TT_FWord   yStrikeoutSize;
    TT_FWord   yStrikeoutPosition;
    TT_Short   sFamilyClass;

    TT_Byte    panose[10];

    TT_ULong   ulUnicodeRange1;        /* Bits 0-31   */
    TT_ULong   ulUnicodeRange2;        /* Bits 32-63  */
    TT_ULong   ulUnicodeRange3;        /* Bits 64-95  */
    TT_ULong   ulUnicodeRange4;        /* Bits 96-127 */

    TT_Char    achVendID[4];

    TT_UShort  fsSelection;
    TT_UShort  usFirstCharIndex;
    TT_UShort  usLastCharIndex;
    TT_Short   sTypoAscender;
    TT_Short   sTypoDescender;
    TT_Short   sTypoLineGap;
    TT_UShort  usWinAscent;
    TT_UShort  usWinDescent;

    /* only version 1 tables: */

    TT_ULong   ulCodePageRange1;       /* Bits 0-31   */
    TT_ULong   ulCodePageRange2;       /* Bits 32-63  */
  };

  typedef struct TT_OS2_  TT_OS2;


  /* ----------- Postscript table ------------------------ */

  struct  TT_Postscript_
  {
    TT_Fixed  FormatType;
    TT_Fixed  italicAngle;
    TT_FWord  underlinePosition;
    TT_FWord  underlineThickness;
    TT_ULong  isFixedPitch;
    TT_ULong  minMemType42;
    TT_ULong  maxMemType42;
    TT_ULong  minMemType1;
    TT_ULong  maxMemType1;

    /* Glyph names follow in the file, but we don't         */
    /* load them by default.  See the ftxpost.c extension.  */
  };

  typedef struct TT_Postscript_  TT_Postscript;


  /* ------------ horizontal device metrics "hdmx" ---------- */

  struct  TT_Hdmx_Record_
  {
    TT_Byte   ppem;
    TT_Byte   max_width;
    TT_Byte*  widths;
  };

  typedef struct TT_Hdmx_Record_  TT_Hdmx_Record;


  struct  TT_Hdmx_
  {
    TT_UShort        version;
    TT_Short         num_records;
    TT_Hdmx_Record*  records;
  };

  typedef struct TT_Hdmx_  TT_Hdmx;


#else

  typedef  TT_HoriHeader   TT_Horizontal_Header;
  typedef  TT_VertHeader   TT_Vertical_Header;

#endif /* _TRUETYPE_ */


  /* A structure used to describe face properties.                        */
  /* be aware that this is not the same structure as the one define       */
  /* in "freetype/ttlib/ttdriver.h". A suitable conversion is performed   */
  /* in "oldapi/ttapi.c" to build a binary driver that is backwards       */
  /* compatible with FreeType 1.1, in the function TT_Get_Face_Properties */

  struct  TT_Face_Properties_
  {
    TT_UShort  num_Glyphs;      /* number of glyphs in face              */
    TT_UShort  max_Points;      /* maximum number of points in a glyph   */
    TT_UShort  max_Contours;    /* maximum number of contours in a glyph */

    TT_UShort  num_CharMaps;    /* number of charmaps in the face     */
    TT_UShort  num_Names;       /* number of name records in the face */

    TT_ULong   num_Faces;  /* 1 for normal TrueType files, and the  */
                           /* number of embedded faces for TrueType */
                           /* collections                           */

    TT_Header*             header;        /* TrueType header table          */
    TT_Horizontal_Header*  horizontal;    /* TrueType horizontal header     */
    TT_OS2*                os2;           /* TrueType OS/2 table            */
    TT_Postscript*         postscript;    /* TrueType Postscript table      */
    TT_Hdmx*               hdmx;
    TT_Vertical_Header*    vertical;      /* TT Vertical header, if present */
  };

  typedef struct TT_Face_Properties_  TT_Face_Properties;



  /***********************************************************************/
  /*                                                                     */
  /* <Type> TT_Engine                                                    */
  /*                                                                     */
  /* <Description>                                                       */
  /*    A handle to a TrueType driver/engine instance. Engine objects    */
  /*    can be created with the TT_New_Engine and TT_Build_Engine APIs   */
  /*                                                                     */
  /*    TT_Done_Engine will destroy an engine, as well as all the        */
  /*    objects that were created within it.                             */
  /*                                                                     */

  typedef struct TT_EngineRec_   *TT_Engine;

  /* Note : The type TT_Engine is not defined in "drivers/ttlib/ttobjs.h" */




/* The TT_Face type is already defined in "drivers/ttlib/ttobjs.h"   */
/* We use the _TRUETYPE_ macro to prevent their redefinition when    */
/* _compiling_ the backwards-compatible layer called "oldapi.c"      */
/*                                                                   */
#ifndef _TRUETYPE_

  /***********************************************************************/
  /*                                                                     */
  /* <Type> TT_Face                                                      */
  /*                                                                     */
  /* <Description>                                                       */
  /*    A handle to a TrueType face/font object. A TT_Face encapsulates  */
  /*    the resolution and scaling independent parts of a TrueType font  */
  /*    file. Instances (a.k.a. fontsizes) and glyph objects must be     */
  /*    created from them before a glyph can be loaded in memory.        */
  /*                                                                     */
  /*    They are created through TT_New_Face and destroyed with          */
  /*    TT_Done_Face. This will destroy all instance and glyph objects   */
  /*                                                                     */

  typedef struct TT_FaceRec_*   TT_Face;

#endif  /* _TRUETYPE_ */


  /***********************************************************************/
  /*                                                                     */
  /* <Type> TT_Instance                                                  */
  /*                                                                     */
  /* <Description>                                                       */
  /*    A handle to a TrueType instance/fontsize object. A TT_Instance   */
  /*    encapsulates all the resolution and scaling dependent part of    */
  /*    a given font size, but doesn't contain any glyph. They must be   */
  /*    used with glyph objects in order to load glyph objects from      */
  /*    a TT file.                                                       */
  /*                                                                     */
  /*    They are created from face objects with TT_New_Instance, and     */
  /*    destroyed with TT_Done_Instance                                  */
  /*                                                                     */
  /*    A fresh new instance has a default resolution of 96x96 dpi,      */
  /*    and a default point size of 10 pt. Each of these can be changed  */
  /*    dynamically with various APIs defined below..                    */
  /*                                                                     */

  typedef struct TT_InstanceRec_*  TT_Instance;

  /* Note: The TT_Instance type is not defined in "drivers/ttlib/ttobjs.h" */


  /***********************************************************************/
  /*                                                                     */
  /* <Type> TT_Glyph                                                     */
  /*                                                                     */
  /* <Description>                                                       */
  /*    A handle to a TrueType glyph object. A glyph object acts as a    */
  /*    container for any of the glyphs of a given face object. It       */
  /*    encapsulates glyph metrics as well as outline space sized large  */
  /*    enough to allow the loading of any glyph without further         */
  /*    allocation. A glyph object doesn't contain any bitmap or         */
  /*    pixmap data/buffer.                                              */
  /*                                                                     */
  /*    They are created from face objects with TT_New_Glyph, and        */
  /*    destroyed with TT_Done_Glyph                                     */
  /*                                                                     */
  /*    One can create several glyph objects per face, and use           */
  /*    a single instance to load multiple glyphs, even concurrently     */
  /*    in thread-safe and reentrant modes..                             */
  /*                                                                     */

  typedef struct TT_GlyphRec_*    TT_Glyph;

  /* Note: the TT_Glyph type is not defined in "drivers/ttlib/ttobjs.h" */


/* The TT_CharMap type is already defined in "drivers/ttlib/ttobjs.h" */
/* We use the _TRUETYPE_ macro to prevent their redefinition when     */
/* _compiling_ the backwards-compatible layer called "oldapi.c"       */
/*                                                                    */
#ifndef _TRUETYPE_

  /***********************************************************************/
  /*                                                                     */
  /* <Type> TT_CharMap                                                   */
  /*                                                                     */
  /* <Description>                                                       */
  /*    A handle to a TrueType character mapping object. These objects   */
  /*    are used to convert character codes in a specific locale or      */
  /*    encoding into font/face glyph indexes.                           */
  /*                                                                     */
  /*    The list of character maps found within a face can be            */
  /*    enumerated with API functions defined below. An CharMap object   */
  /*    is created with TT_New_CharMap. They are destroyed automatically */
  /*    when their parent face objects are discarded.                    */
  /*                                                                     */

  typedef struct TT_CharMapRec_*   TT_CharMap;

  typedef long  TT_Error;

#endif /* _TRUETYPE_ */



  EXPORT_DEF
  const TT_Instance   TT_Null_Instance;


  /*******************************************************************/
  /*                                                                 */
  /*  Postscript Names extension                                     */
  /*                                                                 */
  /*******************************************************************/

#define TT_Err_Invalid_Post_Table_Format  0x0B00
#define TT_Err_Invalid_Post_Table         0x0B01

  /* Initialise the Postscript Names extension */
  EXPORT_DEF
  TT_Error TT_Init_Post_Extension( TT_Engine  engine );

  /* Load the Postscript Names table - notice that the 'post' parameter */
  /* will be ignored in 2.0.                                            */
  EXPORT_DEF
  TT_Error TT_Load_PS_Names( TT_Face   face,
                             void*     post );

  /* Gets the postscript name of a single glyph */
  EXPORT_DEF
  TT_Error TT_Get_PS_Name( TT_Face      face,
                           TT_UShort    index,
                           TT_String**  PSname );


  /*******************************************************************/
  /*                                                                 */
  /*  Embedded Bitmaps (sbits) extension                             */
  /*                                                                 */
  /*******************************************************************/

#ifndef _TRUETYPE_
  /*************************************************************/
  /*                                                           */
  /* <Struct> TT_SBit_Metrics                                  */
  /*                                                           */
  /* <Description>                                             */
  /*    A structure used to hold the big metrics of a given    */
  /*    glyph bitmap in a TrueType or OpenType font. These     */
  /*    are usually found in the "EBDT" table.                 */
  /*                                                           */
  /* <Fields>                                                  */
  /*    height       :: glyph height in pixels                 */
  /*    width        :: glyph width in pixels                  */
  /*                                                           */
  /*    horiBearingX :: horizontal left bearing                */
  /*    horiBearingY :: horizontal top bearing                 */
  /*    horiAdvance  :: horizontal advance                     */
  /*                                                           */
  /*    vertBearingX :: vertical left bearing                  */
  /*    vertBearingY :: vertical top bearing                   */
  /*    vertAdvance  :: vertical advance                       */
  /*                                                           */
  typedef struct TT_SBit_Metrics_
  {
    TT_Byte  height;
    TT_Byte  width;

    TT_Char  horiBearingX;
    TT_Char  horiBearingY;
    TT_Byte  horiAdvance;

    TT_Char  vertBearingX;
    TT_Char  vertBearingY;
    TT_Byte  vertAdvance;

  } TT_SBit_Metrics;



  /*************************************************************/
  /*                                                           */
  /* <Struct> TT_SBit_Small_Metrics                            */
  /*                                                           */
  /* <Description>                                             */
  /*    A structure used to hold the small metrics of a given  */
  /*    glyph bitmap in a TrueType or OpenType font. These     */
  /*    are usually found in the "EBDT" table.                 */
  /*                                                           */
  /* <Fields>                                                  */
  /*    height    :: glyph height in pixels                    */
  /*    width     :: glyph width in pixels                     */
  /*                                                           */
  /*    bearingX  :: left-side bearing                         */
  /*    bearingY  :: top-side bearing                          */
  /*    advance   :: advance width or height                   */
  /*                                                           */
  typedef struct TT_SBit_Small_Metrics_
  {
    TT_Byte  height;
    TT_Byte  width;

    TT_Char  bearingX;
    TT_Char  bearingY;
    TT_Byte  advance;

  } TT_SBit_Small_Metrics;


  /*************************************************************/
  /*                                                           */
  /* <Struct> TT_SBit_Line_Metrics                             */
  /*                                                           */
  /* <Description>                                             */
  /*    A structure used to describe the text line metrics of  */
  /*    a given bitmap strike, for either an horizontal or     */
  /*    vertical layout.                                       */
  /*                                                           */
  /* <Fields>                                                  */
  /*    ascender  :: ascender in pixels                        */
  /*    descender :: descender in pixels                       */
  /*    max_width :: maximum glyph width in pixels             */
  /*                                                           */
  /*    caret_slope_enumerator  :: ?                           */
  /*    caret_slope_denominator :: ?                           */
  /*    caret_offset            :: ?                           */
  /*                                                           */
  /*    min_origin_SB  :: ?                                    */
  /*    min_advance_SB :: ?                                    */
  /*    max_before_BL  :: ?                                    */
  /*    min_after_BL   :: ?                                    */
  /*                                                           */
  typedef struct TT_SBit_Line_Metrics_
  {
    TT_Char  ascender;
    TT_Char  descender;
    TT_Byte  max_width;
    TT_Char  caret_slope_numerator;
    TT_Char  caret_slope_denominator;
    TT_Char  caret_offset;
    TT_Char  min_origin_SB;
    TT_Char  min_advance_SB;
    TT_Char  max_before_BL;
    TT_Char  min_after_BL;
    TT_Char  pads[2];

  } TT_SBit_Line_Metrics;



  /*************************************************************/
  /*                                                           */
  /* <Struct> TT_SBit_Range                                    */
  /*                                                           */
  /* <Description>                                             */
  /*    A TrueType/OpenType subIndexTable as defined in the    */
  /*    "EBLC" or "bloc" tables.                               */
  /*                                                           */
  /* <Fields>                                                  */
  /*                                                           */
  /*    first_glyph  :: first glyph index in range             */
  /*    last_glyph   :: last glyph index in range              */
  /*                                                           */
  /*    index_format :: format of index table. valid           */
  /*                    values are 1 to 5.                     */
  /*                                                           */
  /*    image_format :: format of 'EBDT' image data            */
  /*    image_offset :: offset to image data in 'EBDT'         */
  /*                                                           */
  /*    image_size   :: for index formats 2 and 5. This is     */
  /*                    the size in bytes of each glyph bitmap */
  /*                    glyph bitmap                           */
  /*                                                           */
  /*    big_metrics  :: for index formats 2 and 5. This is     */
  /*                    the big metrics for each glyph bitmap  */
  /*                                                           */
  /*    num_glyphs   :: for index formats 4 and 5. This is     */
  /*                    the number of glyphs in the code       */
  /*                    array.                                 */
  /*                                                           */
  /*    glyph_offsets :: for index formats 1 and 3.            */
  /*    glyph_codes   :: for index formats 4 and 5.            */
  /*                                                           */
  /*    table_offset  :: offset of index table in 'EBLC' table */
  /*                     only used during strike loading..     */
  /*                                                           */
  typedef struct TT_SBit_Range
  {
    TT_UShort        first_glyph;
    TT_UShort        last_glyph;

    TT_UShort        index_format;
    TT_UShort        image_format;
    TT_ULong         image_offset;

    TT_ULong         image_size;
    TT_SBit_Metrics  metrics;
    TT_ULong         num_glyphs;

    TT_ULong*        glyph_offsets;
    TT_UShort*       glyph_codes;

    TT_ULong         table_offset;

  } TT_SBit_Range;


  /*************************************************************/
  /*                                                           */
  /* <Struct> TT_SBit_Strike                                   */
  /*                                                           */
  /* <Description>                                             */
  /*    A structure used describe a given bitmap strike in the */
  /*    "EBLC" or "bloc" tables.                               */
  /*                                                           */
  /* <Fields>                                                  */
  /*                                                           */
  /*   num_index_ranges  :: number of index ranges             */
  /*   index_ranges      :: array of glyph index ranges        */
  /*                                                           */
  /*   color_ref   :: unused. color reference ??               */
  /*   hori        :: line metrics for horizontal layouts.     */
  /*   vert        :: line metrics for vertical layouts.       */
  /*                                                           */
  /*   start_glyph :: lowest glyph index for this strike.      */
  /*   end_glyph   :: higher glyph index for this strike.      */
  /*                                                           */
  /*   x_ppem      :: horizontal pixels per EM                 */
  /*   y_ppem      :: vertical pixels per EM                   */
  /*   bit_depth   :: bit depth. valid values are 1, 2, 4 & 8  */
  /*   flags       :: vertical or horizontal ?                 */
  /*                                                           */
  typedef struct TT_SBit_Strike_
  {
    TT_Int                 num_ranges;
    TT_SBit_Range*         sbit_ranges;
    TT_ULong               ranges_offset;

    TT_ULong               color_ref;

    TT_SBit_Line_Metrics   hori;
    TT_SBit_Line_Metrics   vert;

    TT_UShort              start_glyph;
    TT_UShort              end_glyph;

    TT_Byte                x_ppem;
    TT_Byte                y_ppem;
    TT_Byte                bit_depth;
    TT_Char                flags;

  } TT_SBit_Strike;



  
  /*************************************************************/
  /*                                                           */
  /* <Struct> TT_SBit_Component                                */
  /*                                                           */
  /* <Description>                                             */
  /*    A simple structure to describe a compound sbit element */
  /*                                                           */
  /* <Fields>                                                  */
  /*    glyph_code  :: element's glyph index                   */
  /*    x_offset    :: element's left bearing                  */
  /*    y_offset    :: element's top bearing                   */
  /*                                                           */
  typedef struct TT_SBit_Component_
  {
    TT_UShort  glyph_code;
    TT_Char    x_offset;
    TT_Char    y_offset;

  } TT_SBit_Component;



  /*************************************************************/
  /*                                                           */
  /* <Struct> TT_SBit_Scale                                    */
  /*                                                           */
  /* <Description>                                             */
  /*    A structure used describe a given bitmap scaling       */
  /*    table, asdefined for the "EBSC" table.                 */
  /*                                                           */
  /* <Fields>                                                  */
  /*                                                           */
  /*    hori   :: horizontal line metrics                      */
  /*    vert   :: vertical line metrics                        */
  /*                                                           */
  /*    x_ppem :: horizontal pixels per EM                     */
  /*    y_ppem :: vertical pixels per EM                       */
  /*                                                           */
  /*    x_ppem_substitute  :: substitution x_ppem              */
  /*    y_ppem_substitute  :: substitution y_ppem              */
  /*                                                           */
  typedef struct TT_SBit_Scale_
  {
    TT_SBit_Line_Metrics  hori;
    TT_SBit_Line_Metrics  vert;
    TT_Byte               x_ppem;
    TT_Byte               y_ppem;
    TT_Byte               x_ppem_substitute;
    TT_Byte               y_ppem_substitute;

  } TT_SBit_Scale;
#endif /* _TRUETYPE_ */


  /*************************************************************/
  /*                                                           */
  /* <Struct> TT_SBit_Image                                    */
  /*                                                           */
  /* <Description>                                             */
  /*    A structure used to describe a given embedded bitmap   */
  /*    image.                                                 */
  /*                                                           */
  /* <Fields>                                                  */
  /*    map        :: bitmap descriptor                        */
  /*    bit_depth  :: pixel bit depth                          */
  /*    metrics    :: glyph metrics for the bitmap             */
  /*                                                           */
  typedef struct TT_SBit_Image_
  {
    TT_Raster_Map         map;
    int                   bit_depth;
    TT_Big_Glyph_Metrics  metrics;

  } TT_SBit_Image;



  /*************************************************************/
  /*                                                           */
  /* <Struct> TT_EBLC                                          */
  /*                                                           */
  /* <Description>                                             */
  /*    A structure used to describe the "EBLC" table from     */
  /*    a TrueTYpe font.                                       */
  /*                                                           */
  /* <Fields>                                                  */
  /*                                                           */
  /*    version     :: version number of the EBLC table        */
  /*                                                           */
  /*    num_strikes :: the number of strikes, i.e. bitmap      */
  /*                   sizes, present in this font             */
  /*                                                           */
  /*    strikes     :: array of strikes                        */
  /*                                                           */
  typedef struct TT_EBLC_
  {
    TT_ULong         version;
    TT_ULong         num_strikes;
    TT_SBit_Strike*  strikes;

  } TT_EBLC;


  /*************************************************************/
  /*                                                           */
  /* <Function>                                                */
  /*    TT_Init_SBit_Extension                                 */
  /*                                                           */
  /* <Description>                                             */
  /*    Initialise the embedded bitmaps extension for the      */
  /*    FreeType engine.                                       */
  /*                                                           */
  /* <Input>                                                   */
  /*    engine  :: handle to current FreeType library instance */
  /*                                                           */
  /* <Return>                                                  */
  /*    Error code. 0 means success.                           */
  /*                                                           */
  EXPORT_DEF
  TT_Error  TT_Init_SBit_Extension( TT_Engine  engine );


  /*************************************************************/
  /*                                                           */
  /* <Function>                                                */
  /*    TT_Get_Face_Bitmaps                                    */
  /*                                                           */
  /* <Description>                                             */
  /*    Loads the "EBLC" table from a font file, if any.       */
  /*                                                           */
  /* <Input>                                                   */
  /*    face :: handle to the source TrueType font/face        */
  /*                                                           */
  /* <Output>                                                  */
  /*    eblc_table :: a descriptor for the EBLC table          */
  /*                                                           */
  /* <Return>                                                  */
  /*    Error code. 0 means success.                           */
  /*                                                           */
  /* <Note>                                                    */
  /*    This function returns TT_Err_Table_Missing when the    */
  /*    font contains no embedded bitmaps. All fields in       */
  /*    "eblc_table" will then be set to 0.                    */
  /*                                                           */
  EXPORT_DEF
  TT_Error  TT_Get_Face_Bitmaps( TT_Face   face,
                                 TT_EBLC  *eblc_table );

  /*************************************************************/
  /*                                                           */
  /* <Function>                                                */
  /*    TT_New_SBit_Image                                      */
  /*                                                           */
  /* <Description>                                             */
  /*    Allocates a new embedded bitmap container              */
  /*                                                           */
  /* <Output>                                                  */
  /*    image :: sbit image                                    */
  /*                                                           */
  /* <Return>                                                  */
  /*    Error code. 0 means success.                           */
  /*                                                           */
  EXPORT_DEF
  TT_Error  TT_New_SBit_Image( TT_SBit_Image*  *image );


  /*************************************************************/
  /*                                                           */
  /* <Function>                                                */
  /*    TT_Done_SBit_Image                                     */
  /*                                                           */
  /* <Description>                                             */
  /*    Releases an embedded bitmap container                  */
  /*                                                           */
  /* <Input>                                                   */
  /*    image :: sbit image                                    */
  /*                                                           */
  EXPORT_DEF
  void      TT_Done_SBit_Image( TT_SBit_Image*  image );


  /*************************************************************/
  /*                                                           */
  /* <Function>                                                */
  /*    TT_Load_Glyph_Bitmap                                   */
  /*                                                           */
  /* <Description>                                             */
  /*    Loads a given glyph embedded bitmap                    */
  /*                                                           */
  /* <Input>                                                   */
  /*    face        :: handle to the source TrueType font/face */
  /*    instance    :: current size/transform instance         */
  /*    glyph_index :: index of source glyph                   */
  /*    bitmap      :: target embedded bitmap descriptor       */
  /*                                                           */
  /* <Return>                                                  */
  /*    Error code. 0 means success.                           */
  /*                                                           */
  /* <Note>                                                    */
  /*    This function returns an error if there is no          */
  /*    embedded bitmap for the glyph at the given             */
  /*    instance.                                              */
  /*                                                           */
  EXPORT_DEF
  TT_Error  TT_Load_Glyph_Bitmap( TT_Face         face,
                                  TT_Instance     instance,
                                  TT_UShort       glyph_index,
                                  TT_SBit_Image*  bitmap );


  /*******************************************************************/
  /*                                                                 */
  /*  FreeType API                                                   */
  /*                                                                 */
  /*  All these begin with a 'TT_' prefix.                           */
  /*                                                                 */
  /*  Most of them are implemented in the 'ttapi.c' source file.     */
  /*                                                                 */
  /*******************************************************************/

  /* Initialize the engine. */

  EXPORT_DEF
  TT_Error  TT_Init_FreeType( TT_Engine*  engine );


  /* Finalize the engine, and release all allocated objects. */

  EXPORT_DEF
  TT_Error  TT_Done_FreeType( TT_Engine  engine );


  /* Set the gray level palette.  This is an array of 5 bytes used */
  /* to produce the font smoothed pixmaps.  By convention:         */
  /*                                                               */
  /*  palette[0] = background (white)                              */
  /*  palette[1] = light                                           */
  /*  palette[2] = medium                                          */
  /*  palette[3] = dark                                            */
  /*  palette[4] = foreground (black)                              */
  /*                                                               */

  EXPORT_DEF
  TT_Error  TT_Set_Raster_Gray_Palette( TT_Engine       engine,
                                        const TT_Byte*  palette );
  

  /* ----------------------- face management ----------------------- */

  /* Open a new TrueType font file, and returns a handle for */
  /* it in variable '*face'.                                 */

  /* Note: The file can be either a TrueType file (*.ttf) or  */
  /*       a TrueType collection (*.ttc, in this case, only   */
  /*       the first face is opened).  The number of faces in */
  /*       the same collection can be obtained in the face's  */
  /*       properties, using TT_Get_Face_Properties() and the */
  /*       'max_Faces' field.                                 */

  EXPORT_DEF
  TT_Error  TT_Open_Face( TT_Engine       engine,
                          const TT_Text*  fontPathName,
                          TT_Face*        face );


  /* Open a TrueType font file located inside a collection. */
  /* The font is assigned by its index in 'fontIndex'.      */

  EXPORT_DEF
  TT_Error  TT_Open_Collection( TT_Engine       engine,
                                const TT_Text*  collectionPathName,
                                TT_ULong        fontIndex,
                                TT_Face*        face );


  /* Return face properties in the 'properties' structure. */

  EXPORT_DEF
  TT_Error  TT_Get_Face_Properties( TT_Face              face,
                                    TT_Face_Properties*  properties );


  /* Set a face object's generic pointer */

  EXPORT_DEF
  TT_Error  TT_Set_Face_Pointer( TT_Face  face,
                                 void*    data );


  /* Get a face object's generic pointer */

  EXPORT_DEF
  void*     TT_Get_Face_Pointer( TT_Face  face );


  /* Close a face's file handle to save system resources.  The file */
  /* will be re-opened automatically on the next disk access.       */

  EXPORT_DEF
  TT_Error  TT_Flush_Face( TT_Face  face );

  /* Get a face's glyph metrics expressed in font units.  Returns any   */
  /* number of arrays.  Set the fields to NULL if you're not interested */
  /* by a given array.                                                  */

  EXPORT_DEF
  TT_Error  TT_Get_Face_Metrics( TT_Face     face,
                                 TT_UShort   firstGlyph,
                                 TT_UShort   lastGlyph,
                                 TT_Short*   leftBearings,
                                 TT_UShort*  widths,
                                 TT_Short*   topBearings,
                                 TT_UShort*  heights );

  /* Close a given font object, destroying all associated */
  /* instances.                                           */

  EXPORT_DEF
  TT_Error  TT_Close_Face( TT_Face  face );


  /* Get font or table data. */

  EXPORT_DEF
  TT_Error  TT_Get_Font_Data( TT_Face   face,
                              TT_ULong  tag,
                              TT_Long   offset,
                              void*     buffer,
                              TT_Long*  length );


/* A simple macro to build table tags from ASCII chars */

#define MAKE_TT_TAG( _x1, _x2, _x3, _x4 ) \
          (((TT_ULong)_x1 << 24) |        \
           ((TT_ULong)_x2 << 16) |        \
           ((TT_ULong)_x3 << 8)  |        \
            (TT_ULong)_x4)



  /* ----------------------- instance management -------------------- */

  /* Open a new font instance and returns an instance handle */
  /* for it in '*instance'.                                  */

  EXPORT_DEF
  TT_Error  TT_New_Instance( TT_Face       face,
                             TT_Instance*  instance );


  /* Set device resolution for a given instance.  The values are      */
  /* given in dpi (Dots Per Inch).  Default is 96 in both directions. */

  EXPORT_DEF
  TT_Error  TT_Set_Instance_Resolutions( TT_Instance  instance,
                                         TT_UShort    xResolution,
                                         TT_UShort    yResolution );


  /* Set the pointsize for a given instance.  Default is 10pt. */

  EXPORT_DEF
  TT_Error  TT_Set_Instance_CharSize( TT_Instance  instance,
                                      TT_F26Dot6   charSize );

  EXPORT_DEF
  TT_Error  TT_Set_Instance_CharSizes( TT_Instance  instance,
                                       TT_F26Dot6   charWidth,
                                       TT_F26Dot6   charHeight );

#define TT_Set_Instance_PointSize( ins, ptsize )   \
            TT_Set_Instance_CharSize( ins, ptsize*64 )

  EXPORT_DEF
  TT_Error  TT_Set_Instance_PixelSizes( TT_Instance  instance,
                                        TT_UShort    pixelWidth,
                                        TT_UShort    pixelHeight,
                                        TT_F26Dot6   pointSize );


  /* This function has been deprecated !! Do not use it, as it    */
  /* doesn't work reliably. You can perfectly control hinting     */
  /* yourself when loading glyphs, then apply transforms as usual */

  EXPORT_DEF
  TT_Error  TT_Set_Instance_Transform_Flags( TT_Instance  instance,
                                             TT_Bool      rotated,
                                             TT_Bool      stretched );


  /* Return instance metrics in 'metrics'. */

  EXPORT_DEF
  TT_Error  TT_Get_Instance_Metrics( TT_Instance           instance,
                                     TT_Instance_Metrics*  metrics );


  /* Set an instance's generic pointer. */

  EXPORT_DEF
  TT_Error  TT_Set_Instance_Pointer( TT_Instance  instance,
                                     void*        data );


  /* Get an instance's generic pointer. */

  EXPORT_DEF
  void*     TT_Get_Instance_Pointer( TT_Instance  instance );


  /* Close a given instance object, destroying all associated data. */

  EXPORT_DEF
  TT_Error  TT_Done_Instance( TT_Instance  instance );



  /* ----------------------- glyph management ----------------------- */

  /* Create a new glyph object related to the given 'face'. */

  EXPORT_DEF
  TT_Error  TT_New_Glyph( TT_Face    face,
                          TT_Glyph*  glyph );


  /* Discard (and destroy) a given glyph object. */

  EXPORT_DEF
  TT_Error  TT_Done_Glyph( TT_Glyph  glyph );


#define TTLOAD_SCALE_GLYPH  1
#define TTLOAD_HINT_GLYPH   2

#define TTLOAD_DEFAULT  (TTLOAD_SCALE_GLYPH | TTLOAD_HINT_GLYPH)


  /* Load and process (scale/transform and hint) a glyph from the */
  /* given 'instance'.  The glyph and instance handles must be    */
  /* related to the same face object.  The glyph index can be     */
  /* computed with a call to TT_Char_Index().                     */

  /* The 'load_flags' argument is a combination of the macros     */
  /* TTLOAD_SCALE_GLYPH and TTLOAD_HINT_GLYPH.  Hinting will be   */
  /* applied only if the scaling is selected.                     */

  /* When scaling is off (i.e., load_flags = 0), the returned     */
  /* outlines are in EM square coordinates (also called FUnits),  */
  /* extracted directly from the font with no hinting.            */
  /* Other glyph metrics are also in FUnits.                      */

  /* When scaling is on, the returned outlines are in fractional  */
  /* pixel units (i.e. TT_F26Dot6 = 26.6 fixed floats).           */

  /* NOTE:  The glyph index must be in the range 0..num_glyphs-1  */
  /*        where 'num_glyphs' is the total number of glyphs in   */
  /*        the font file (given in the face properties).         */

  EXPORT_DEF
  TT_Error  TT_Load_Glyph( TT_Instance  instance,
                           TT_Glyph     glyph,
                           TT_UShort    glyphIndex,
                           TT_UShort    loadFlags );


  /* Return glyph outline pointers in 'outline'.  Note that the returned */
  /* pointers are owned by the glyph object, and will be destroyed with  */
  /* it.  The client application should _not_ change the pointers.       */
  
  EXPORT_DEF
  TT_Error  TT_Get_Glyph_Outline( TT_Glyph     glyph,
                                  TT_Outline*  outline );


  /* Copy the glyph metrics into 'metrics'. */

  EXPORT_DEF
  TT_Error  TT_Get_Glyph_Metrics( TT_Glyph           glyph,
                                  TT_Glyph_Metrics*  metrics );


  /* Copy the glyph's big metrics into 'metrics'. */
  /* Necessary to obtain vertical metrics.        */

  EXPORT_DEF
  TT_Error  TT_Get_Glyph_Big_Metrics( TT_Glyph               glyph,
                                      TT_Big_Glyph_Metrics*  metrics );


  /* Render the glyph into a bitmap, with given position offsets.     */

  /*  Note:  Only use integer pixel offsets to preserve the fine      */
  /*         hinting of the glyph and the 'correct' anti-aliasing     */
  /*         (where vertical and horizontal stems aren't grayed).     */
  /*         This means that xOffset and yOffset must be multiples    */
  /*         of 64!                                                   */

  EXPORT_DEF
  TT_Error  TT_Get_Glyph_Bitmap( TT_Glyph        glyph,
                                 TT_Raster_Map*  map,
                                 TT_F26Dot6      xOffset,
                                 TT_F26Dot6      yOffset );


  /* Render the glyph into a pixmap, with given position offsets.     */

  /*  Note : Only use integer pixel offsets to preserve the fine      */
  /*         hinting of the glyph and the 'correct' anti-aliasing     */
  /*         (where vertical and horizontal stems aren't grayed).     */
  /*         This means that xOffset and yOffset must be multiples    */
  /*         of 64!                                                   */

  EXPORT_DEF
  TT_Error  TT_Get_Glyph_Pixmap( TT_Glyph        glyph,
                                 TT_Raster_Map*  map,
                                 TT_F26Dot6      xOffset,
                                 TT_F26Dot6      yOffset );



  /* ----------------------- outline support ------------------------ */

  /* Allocate a new outline.  Reserve space for 'numPoints' and */
  /* 'numContours'.                                             */

  EXPORT_DEF
  TT_Error  TT_New_Outline( TT_UShort    numPoints,
                            TT_Short     numContours,
                            TT_Outline*  outline );


  /* Release an outline. */

  EXPORT_DEF
  TT_Error  TT_Done_Outline( TT_Outline*  outline );


  /* Copy an outline into another one. */

  EXPORT_DEF
  TT_Error  TT_Copy_Outline( TT_Outline*  source,
                             TT_Outline*  target );


  /* Render an outline into a bitmap. */

  EXPORT_DEF
  TT_Error  TT_Get_Outline_Bitmap( TT_Engine       engine,
                                   TT_Outline*     outline,
                                   TT_Raster_Map*  map );


  /* Render an outline into a pixmap -- note that this function uses */
  /* a different pixel scale, where 1.0 pixels = 128           XXXX  */

  EXPORT_DEF
  TT_Error  TT_Get_Outline_Pixmap( TT_Engine       engine,
                                   TT_Outline*     outline,
                                   TT_Raster_Map*  map );


  /* Return an outline's bounding box -- this function is slow as it */
  /* performs a complete scan-line process, without drawing, to get  */
  /* the most accurate values.                                 XXXX  */

  EXPORT_DEF
  TT_Error  TT_Get_Outline_BBox( TT_Outline*  outline,
                                 TT_BBox*     bbox );


  /* Apply a transformation to a glyph outline. */

  EXPORT_DEF
  void      TT_Transform_Outline( TT_Outline*  outline,
                                  TT_Matrix*   matrix );


  /* Apply a translation to a glyph outline. */

  EXPORT_DEF
  void      TT_Translate_Outline( TT_Outline*  outline,
                                  TT_F26Dot6   xOffset,
                                  TT_F26Dot6   yOffset );


  /* Apply a transformation to a vector. */

  EXPORT_DEF
  void      TT_Transform_Vector( TT_F26Dot6*  x,
                                 TT_F26Dot6*  y,
                                 TT_Matrix*   matrix );


  /* Multiply a matrix with another -- computes "b := a*b". */

  EXPORT_DEF
  void      TT_Matrix_Multiply( TT_Matrix*  a,
                                TT_Matrix*  b );
                 

  /* Invert a transformation matrix. */

  EXPORT_DEF
  TT_Error  TT_Matrix_Invert( TT_Matrix*  matrix );


  /* Compute A*B/C with 64 bits intermediate precision. */

  EXPORT_DEF
  TT_Long   TT_MulDiv( TT_Long A, TT_Long B, TT_Long C );

  
  /* Compute A*B/0x10000 with 64 bits intermediate precision. */
  /* Useful to multiply by a 16.16 fixed float value.         */

  EXPORT_DEF
  TT_Long   TT_MulFix( TT_Long A, TT_Long B );



  /* ----------------- character mappings support ------------- */

  /* Return the number of character mappings found in this file. */
  /* Returns -1 in case of failure (invalid face handle).        */
  /*                                                             */
  /* DON'T USE THIS FUNCTION! IT HAS BEEN DEPRECATED!            */
  /*                                                             */
  /* It is retained for backwards compatibility only and will    */
  /* fail on 16bit systems.                                      */
  /*                                                             */
  /* You can now get the charmap count in the "num_CharMaps"     */
  /* field of a face's properties.                               */
  /*                                                             */

  EXPORT_DEF
  int  TT_Get_CharMap_Count( TT_Face  face );


  /* Return the ID of charmap number 'charmapIndex' of a given face */
  /* used to enumerate the charmaps present in a TrueType file.     */

  EXPORT_DEF
  TT_Error  TT_Get_CharMap_ID( TT_Face     face,
                               TT_UShort   charmapIndex,
                               TT_UShort*  platformID,
                               TT_UShort*  encodingID );


  /* Look up the character maps found in 'face' and return a handle */
  /* for the one matching 'platformID' and 'platformEncodingID'     */
  /* (see the TrueType specs relating to the 'cmap' table for       */
  /* information on these ID numbers).  Returns an error code.      */
  /* In case of failure, the handle is set to NULL and is invalid.  */

  EXPORT_DEF
  TT_Error  TT_Get_CharMap( TT_Face      face,
                            TT_UShort    charmapIndex,
                            TT_CharMap*  charMap );


  /* Translate a character code through a given character map   */
  /* and return the corresponding glyph index to be used in     */
  /* a TT_Load_Glyph call.  This function returns 0 in case of  */
  /* failure.                                                   */

  EXPORT_DEF
  TT_UShort  TT_Char_Index( TT_CharMap  charMap,
                            TT_UShort   charCode );



  /* --------------------- names table support ------------------- */

  /* Return the number of name strings found in the name table.  */
  /* Returns -1 in case of failure (invalid face handle).        */
  /*                                                             */
  /* DON'T USE THIS FUNCTION! IT HAS BEEN DEPRECATED!            */
  /*                                                             */
  /* It is retained for backwards compatibility only and will    */
  /* fail on 16bit systems.                                      */
  /*                                                             */
  /* You can now get the number of name strings in a face with   */
  /* the "num_Names" field of its properties..                   */
  /*                                                             */

  EXPORT_DEF
  int  TT_Get_Name_Count( TT_Face  face );


  /* Return the ID of the name number 'nameIndex' of a given face */
  /* used to enumerate the charmaps present in a TrueType file.   */

  EXPORT_DEF
  TT_Error  TT_Get_Name_ID( TT_Face     face,
                            TT_UShort   nameIndex,
                            TT_UShort*  platformID,
                            TT_UShort*  encodingID,
                            TT_UShort*  languageID,
                            TT_UShort*  nameID );


  /* Return the address and length of the name number 'nameIndex' */
  /* of a given face.  The string is part of the face object and  */
  /* shouldn't be written to or released.                         */

  /* Note that if for an invalid platformID a null pointer will   */
  /* be returned.                                                 */

  EXPORT_DEF
  TT_Error  TT_Get_Name_String( TT_Face      face,
                                TT_UShort    nameIndex,
                                TT_String**  stringPtr, /* pointer address */
                                TT_UShort*   length );  /* string length
                                                           address */

 

#ifdef __cplusplus
  }
#endif

#endif /* TRUETYPE_H */


/* END */
