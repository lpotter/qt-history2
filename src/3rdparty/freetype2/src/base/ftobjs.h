/***************************************************************************/
/*                                                                         */
/*  ftobjs.h                                                               */
/*                                                                         */
/*  The FreeType private base classes (specification).                     */
/*                                                                         */
/*  Copyright 1996-1999 by                                                 */
/*  David Turner, Robert Wilhelm, and Werner Lemberg                       */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used        */
/*  modified and distributed under the terms of the FreeType project       */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /*  This file contains the definition of all internal FreeType classes.  */
  /*                                                                       */
  /*************************************************************************/

#ifndef FTOBJS_H
#define FTOBJS_H

#include <ftconfig.h>
#include <ftsystem.h>
#include <ftdriver.h>

  /*************************************************************************/
  /*                                                                       */
  /* Some generic definitions.                                             */
  /*                                                                       */
#ifndef TRUE
#define TRUE  1
#endif

#ifndef FALSE
#define FALSE  0
#endif

#ifndef NULL
#define NULL  (void*)0
#endif

#ifndef UNUSED
#define UNUSED( arg )  ( (void)(arg) )
#endif


  /*************************************************************************/
  /*                                                                       */
  /* The min and max functions missing in C.  As usual, be careful not to  */
  /* write things like MIN( a++, b++ ) to avoid side effects.              */
  /*                                                                       */
#ifndef MIN
#define MIN( a, b )  ( (a) < (b) ? (a) : (b) )
#endif

#ifndef MAX
#define MAX( a, b )  ( (a) > (b) ? (a) : (b) )
#endif

#ifndef ABS
#define ABS( a )     ( (a) < 0 ? -(a) : (a) )
#endif


  /*************************************************************************/
  /*                                                                       */
  /* <Macro>                                                               */
  /*    FT_SET_ERROR                                                       */
  /*                                                                       */
  /* <Description>                                                         */
  /*    This macro is used to set an implicit `error' variable to a given  */
  /*    expression's value (usually a function call), and convert it to a  */
  /*    boolean which is set whenever the value is != 0.                   */
  /*                                                                       */
#undef  FT_SET_ERROR
#define FT_SET_ERROR( expression ) \
          ( (error = (expression)) != 0 )


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /****                                                                 ****/
  /****                                                                 ****/
  /****                           M E M O R Y                           ****/
  /****                                                                 ****/
  /****                                                                 ****/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/

  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Alloc                                                           */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Allocates a new block of memory.  The returned area is always      */
  /*    zero-filled, this is a strong convention in many FreeType parts.   */
  /*                                                                       */
  /* <Input>                                                               */
  /*    memory :: A handle to a given `memory object' where allocation     */
  /*              occurs.                                                  */
  /*                                                                       */
  /*    size   :: The size in bytes of the block to allocate.              */
  /*                                                                       */
  /* <Output>                                                              */
  /*    P      :: A pointer to the fresh new block.  It should be set to   */
  /*              NULL if `size' is 0, or in case of error.                */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  BASE_DEF
  FT_Error  FT_Alloc( FT_Memory  memory,
                      FT_Long    size,
                      void**     P );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Realloc                                                         */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Reallocates a block of memory pointed to by `*P' to `Size' bytes   */
  /*    from the heap, possibly changing `*P'.                             */
  /*                                                                       */
  /* <Input>                                                               */
  /*    memory :: A handle to a given `memory object' where allocation     */
  /*              occurs.                                                  */
  /*                                                                       */
  /*    current :: current block size in bytes                             */
  /*    size    :: the new block size in bytes                              */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    P      :: A pointer to the fresh new block.  It should be set to   */
  /*              NULL if `size' is 0, or in case of error.                */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    All callers of FT_Realloc _must_ provide the current block size    */
  /*    as well as the new one.                                            */
  /*                                                                       */
  /*    When the memory object's flag FT_memory_FLAG_NO_REALLOC is         */
  /*    set, this function will try to emulate a realloc through uses      */
  /*    of FT_Alloc and FT_Free. Otherwise, it will call the memory-       */
  /*    specific "realloc" implementation.                                 */
  /*                                                                       */
  /*    (Some embedded memorys do not have a working realloc).             */
  /*                                                                       */
  BASE_DEF
  FT_Error  FT_Realloc( FT_Memory  memory,
                        FT_Long    current,
                        FT_Long    size,
                        void**     P );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Free                                                            */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Releases a given block of memory allocated through FT_Alloc().     */
  /*                                                                       */
  /* <Input>                                                               */
  /*    memory :: A handle to a given `memory object' where allocation     */
  /*              occured.                                                 */
  /*                                                                       */
  /*    P      :: This is the _address_ of a _pointer_ which points to the */
  /*              allocated block.  It is always set to NULL on exit.      */
  /*                                                                       */
  /* <Note>                                                                */
  /*    If P or *P are NULL, this function should return successfully.     */
  /*    This is a strong convention within all of FreeType and its         */
  /*    drivers.                                                           */
  /*                                                                       */
  BASE_DEF
  void  FT_Free( FT_Memory  memory,
                 void**     P );



  /* This include is needed by the MEM_xxx() macros, it should be */
  /* available on every platform we know !!                       */
#include <string.h>

#define MEM_Set( dest, byte, count )  memset( dest, byte, count )

#ifdef HAVE_MEMCPY
#define MEM_Copy( dest, source, count )  memcpy( dest, source, count )
#else
#define MEM_Copy( dest, source, count )  bcopy( source, dest, count )
#endif

#define MEM_Move( dest, source, count )  memmove( dest, source, count )


  /*************************************************************************/
  /*                                                                       */
  /* We now support closures to produce completely reentrant code.  This   */
  /* means the allocation functions now takes an additional argument       */
  /* (`memory').  It is a handle to a given memory object, responsible for */
  /* all low-level operations, including memory management and             */
  /* synchronisation.                                                      */
  /*                                                                       */
  /* In order to keep our code readable and use the same macros in the     */
  /* font drivers and the rest of the library, MEM_Alloc(), ALLOC(), and   */
  /* ALLOC_ARRAY() now use an implicit variable, `memory'.  It must be     */
  /* defined at all locations where a memory operation is queried.         */
  /*                                                                       */

  /*                                                                       */
  /* Note that ALL memory allocation functions need an IMPLICIT argument   */
  /* called `memory' to point to the current memory object.                */
  /*                                                                       */
#define MEM_Alloc( _pointer_, _size_ ) \
          FT_Alloc( memory, _size_, (void**)&(_pointer_) )

#define MEM_Realloc( _pointer_, _current_, _size_ ) \
          FT_Realloc( memory, _current_, _size_, (void**)&(_pointer_) )

#define ALLOC( _pointer_, _size_ ) \
          FT_SET_ERROR( MEM_Alloc( _pointer_, _size_ ) )

#define REALLOC( _pointer_, _current_, _size_ ) \
          FT_SET_ERROR( MEM_Realloc( _pointer_, _current_, _size_ ) )

#define ALLOC_ARRAY( _pointer_, _count_, _type_ ) \
          FT_SET_ERROR( MEM_Alloc( _pointer_, (_count_)*sizeof (_type_) ) )

#define REALLOC_ARRAY( _pointer_, _current_, _count_, _type_ ) \
          FT_SET_ERROR( MEM_Realloc( _pointer_, (_current_)*sizeof(_type_), \
                         (_count_)*sizeof(_type_) ) )

#define FREE( _pointer_ )  FT_Free( memory, (void**)&(_pointer_) )



  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /****                                                                 ****/
  /****                                                                 ****/
  /****                         D R I V E R S                           ****/
  /****                                                                 ****/
  /****                                                                 ****/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/

  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    FT_DriverRec                                                       */
  /*                                                                       */
  /* <Description>                                                         */
  /*    The root font driver class.  A font driver is responsible for      */
  /*    managing and loading font files of a given format.                 */
  /*                                                                       */
  /*  <Fields>                                                             */
  /*     library     :: A handle to the driver's parent library.           */
  /*                                                                       */
  /*     memory      :: A handle to the driver's memory object.  This is a */
  /*                    duplicate of `library->memory'.                    */
  /*                                                                       */
  /*     interface   :: A set of driver methods that implement its         */
  /*                    behaviour.  These methods are called by the        */
  /*                    various FreeType functions like FT_New_Face(),     */
  /*                    FT_Load_Glyph(), etc.                              */
  /*                                                                       */
  /*     format      :: A typeless pointer, used to store the address of   */
  /*                    the driver's format specific interface.  This is a */
  /*                    table of other driver methods that are specific to */
  /*                    its format.  Format specific interfaces are        */
  /*                    defined in the driver's header files (e.g.,        */
  /*                    `freetype/drivers/ttlib/ttdriver.h').              */
  /*                                                                       */
  /*     version     :: The driver's version.  It can be used for          */
  /*                    versioning and dynamic driver update if needed.    */
  /*                                                                       */
  /*     description :: An ASCII string describing the driver's supported  */
  /*                    format, like `truetype', `type1', etc.             */
  /*                                                                       */
  /*     faces_list  :: The list of faces currently opened by this driver. */
  /*                                                                       */
  /*     extensions  :: a typeless pointer to the driver's extensions      */
  /*                    registry, when they are supported through the      */
  /*                    config macro FT_CONFIG_OPTION_EXTENSIONS           */
  /*                                                                       */
  typedef struct  FT_DriverRec_
  {
    FT_Library          library;
    FT_Memory           memory;

    FT_Generic          generic;

    FT_DriverInterface  interface;
    FT_FormatInterface  format;

    FT_Int              version;      /* driver version     */
    FT_String*          description;  /* format description */

    FT_ListRec          faces_list;   /* driver's faces list    */

    void*               extensions;

  } FT_DriverRec;


#ifdef FT_CONFIG_OPTION_ALTERNATE_GLYPH_FORMATS

 /************************************************************************
  *
  *  <Struct>
  *     FT_GlyphZone
  *
  *  <Description>
  *     A glyph zone is used to load, scale and hint glyph outline
  *     coordinates.
  *
  *  <Fields>
  *     memory       :: handle to memory manager
  *     max_points   :: max size in points of zone
  *     max_contours :: max size in contours of zone
  *     n_points     :: current number of points in zone
  *     n_contours   :: current number of contours in zone
  *     org          :: original glyph coordinates (font units/scaled)
  *     cur          :: current glyph coordinates  (scaled/hinted)
  *     tags         :: point control tags 
  *     contours     :: contour end points
  *
  ***********************************************************************/
  
  typedef struct  FT_GlyphZone_
  {
    FT_Memory   memory;
    FT_UShort   max_points;
    FT_UShort   max_contours;
    FT_UShort   n_points;   /* number of points in zone    */
    FT_Short    n_contours; /* number of contours          */

    FT_Vector*  org;        /* original point coordinates  */
    FT_Vector*  cur;        /* current point coordinates   */

    FT_Byte*    tags;       /* current touch flags         */
    FT_UShort*  contours;   /* contour end points          */

  } FT_GlyphZone;

  BASE_DEF
  FT_Error  FT_New_GlyphZone( FT_Memory      memory,
                              FT_UShort      maxPoints,
                              FT_Short       maxContours,
                              FT_GlyphZone*  zone );

  BASE_DEF
  void      FT_Done_GlyphZone( FT_GlyphZone*  zone );
  
  BASE_DEF
  FT_Error  FT_Update_GlyphZone( FT_GlyphZone*  zone,
                                 FT_UShort      num_points,
                                 FT_Short       num_contours );


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /****                                                                 ****/
  /****                                                                 ****/
  /****                     G L Y P H   F O R M A T S                   ****/
  /****                                                                 ****/
  /****                                                                 ****/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/

 /*************************************************************************
  *
  * <Struct>
  *   FT_Glyph_Format
  *
  * <Description>
  *   A structure used to model various properties of a non-standard   
  *   glyph image format.
  *
  * <Fields>
  *   format_tag        :: the glyph format tag
  *
  *   raster_interface  :: the default rasters interface for this glyph
  *                        format.
  *
  *   raster            :: the default raster object for this glyph format
  *                        if set to nil, a new object will be allocated
  *                        automatically through the raster interface.
  *
  *   raster_owned      :: a boolean used internally by the library. If
  *                        set, if indicates that the current raster object
  *                        was allocated by the library.
  *
  *************************************************************************/
  
  typedef struct FT_Glyph_Format_
  {
    FT_Glyph_Tag           format_tag;
    FT_Raster_Interface*   raster_interface;
    FT_Raster              raster;
    FT_Bool                raster_allocated;
  
  } FT_Glyph_Format;


 /*************************************************************************
  *
  * <Function>
  *   FT_Add_Glyph_Format
  *
  * <Description>
  *   Register a new glyph format into the library
  *
  * <Input>
  *   library   :: handle to target library object
  *   interface :: pointer to glyph format interface
  *
  * <Return>
  *   Error code. 0 means success
  *
  * <Note>
  *   This function should normally be called by those font drivers which
  *   need to use their own glyph image format.
  *
  *************************************************************************/
  
  EXPORT_DEF
  FT_Error  FT_Add_Glyph_Format( FT_Library        library,
                                 FT_Glyph_Format*  format );


 /*************************************************************************
  *
  * <Function>
  *   FT_Remove_Glyph_Format
  *
  * <Description>
  *   Un-Register a given glyph format from the library
  *
  * <Input>
  *   library      :: handle to target library object
  *   glyph_format :: glyph format tag
  *
  * <Return>
  *   Error code. 0 means success
  *
  * <Note>
  *   This function should normally be called by those font drivers which
  *   need to use their own glyph image format.
  *
  *************************************************************************/
  
  EXPORT_DEF
  FT_Error  FT_Remove_Glyph_Format( FT_Library     library,
                                    FT_Glyph_Tag   glyph_format );

 /*************************************************************************
  *
  * <Function>
  *   FT_Get_Glyph_Format
  *
  * <Description>
  *   Return a pointer to the glyph format descriptor corresponding to
  *   a given format tag.
  *
  * <Input>
  *   library    :: handle to source library object
  *
  *   format_tag :: glyph format tag
  *
  * <Return>
  *   a pointer to the corresponding glyph format descriptor, if it was
  *   registered in the library. 0 otherwise.
  *
  *************************************************************************/
  
  BASE_DEF
  FT_Glyph_Format*  FT_Get_Glyph_Format( FT_Library    library,
                                         FT_Glyph_Tag  format_tag );



#endif /* FT_CONFIG_OPTION_ALTERNATE_GLYPH_FORMATS */

  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /****                                                                 ****/
  /****                                                                 ****/
  /****                       L I B R A R I E S                         ****/
  /****                                                                 ****/
  /****                                                                 ****/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/

#define FT_DEBUG_HOOK_TRUETYPE   0
#define FT_DEBUG_HOOK_TYPE1      1

  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    FT_LibraryRec                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    The FreeType library class.  This is the root of all FreeType      */
  /*    data.  Use FT_New_Library() to create a library object, and        */
  /*    FT_Done_Library() to discard it and all child objects.             */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    memory         :: The library's memory object.  Manages memory     */
  /*                      allocation                                       */
  /*                                                                       */
  /*    generic        :: Client data variable.  Used to extend the        */
  /*                      Library class by higher levels and clients.      */
  /*                                                                       */
  /*    num_drivers    :: The number of drivers currently registered       */
  /*                      within this library.  This is set to 0 for new   */
  /*                      libraries.  New drivers are added through the    */
  /*                      FT_Add_Driver() API function.                    */
  /*                                                                       */
  /*    drivers        :: A table used to store handles to the currently   */
  /*                      registered font drivers.  Note that each driver  */
  /*                      contains a list of its opened faces.             */
  /*                                                                       */
  /*    glyph_formats  :: A table used to store glyph format descriptors   */
  /*                      for new image formats that may have been         */
  /*                      registered within the library                    */
  /*                                                                       */
  /*    raster_pool    :: The raster object's render pool.  This can       */
  /*                      ideally be changed dynamically at run-time.      */
  /*                                                                       */
  typedef  void  (*FT_DebugHook_Func)( void* arg );
  
  typedef struct  FT_LibraryRec_
  {
    FT_Memory           memory;         /* library's memory object          */

    FT_Generic          generic;

    FT_Int              num_drivers;
    FT_Driver           drivers[ FT_MAX_DRIVERS ];  /* driver objects  */

    FT_Glyph_Format     glyph_formats[FT_MAX_GLYPH_FORMATS];

    void*               raster_pool;    /* scan-line conversion render pool */

    FT_DebugHook_Func   debug_hooks[4];

  } FT_LibraryRec;


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_New_Library                                                     */
  /*                                                                       */
  /* <Description>                                                         */
  /*    This function is used to create a new FreeType library instance    */
  /*    from a given memory object.  It is thus possible to use libraries  */
  /*    with distinct memory allocators within the same program.           */
  /*                                                                       */
  /* <Input>                                                               */
  /*    memory  :: A handle to the original memory object.                 */
  /*                                                                       */
  /* <Output>                                                              */
  /*    library :: A handle to a new library object.                       */
  /*                                                                       */
  /* <Return>                                                              */
  /*    Error code.  0 means success.                                      */
  /*                                                                       */
  /* <Note>                                                                */
  /*    This function is normally not called by client applications,       */
  /*    unless they want to create a specific instance of FreeType which   */
  /*    uses a specific memory allocator.                                  */
  /*                                                                       */
  EXPORT_DEF
  FT_Error  FT_New_Library( FT_Memory    memory,
                            FT_Library*  library );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Done_Library                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Discards a given library object.  This closes all drivers and      */
  /*    discards all face objects.                                         */
  /*                                                                       */
  /* <Input>                                                               */
  /*    library :: A handle to the target library.                         */
  /*                                                                       */
  /* <Return>                                                              */
  /*    Error code.  0 means success.                                      */
  /*                                                                       */
  EXPORT_DEF
  FT_Error  FT_Done_Library( FT_Library  library );



  EXPORT_DEF
  void  FT_Set_Debug_Hook( FT_Library         library,
                           FT_UInt            hook_index,
                           FT_DebugHook_Func  debug_hook );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Add_Driver                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Registers a new driver in a given library object.  This function   */
  /*    takes only a pointer to a driver interface.  It uses it to create  */
  /*    the new driver, then sets up some important fields.                */
  /*                                                                       */
  /* <Input>                                                               */
  /*    library          :: A handle to the target library object.         */
  /*                                                                       */
  /*    driver_interface :: A pointer to a driver interface table.         */
  /*                                                                       */
  /* <Return>                                                              */
  /*    Error code.  0 means success.                                      */
  /*                                                                       */
  /* <Note>                                                                */
  /*    This function doesn't check whether the driver is already          */
  /*    installed!                                                         */
  /*                                                                       */
  EXPORT_DEF
  FT_Error  FT_Add_Driver( FT_Library                 library,
                           const FT_DriverInterface*  driver_interface );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Remove_Driver                                                   */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Unregister a given driver.  This closes the driver, which in turn  */
  /*    destroys all faces, sizes, slots, etc. associated with it.         */
  /*                                                                       */
  /*    This function also DESTROYS the driver object.                     */
  /*                                                                       */
  /* <Input>                                                               */
  /*    driver :: A handle to target driver object.                        */
  /*                                                                       */
  /* <Return>                                                              */
  /*    Error code.  0 means success.                                      */
  /*                                                                       */
  EXPORT_DEF
  FT_Error  FT_Remove_Driver( FT_Driver  driver );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Get_Driver                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    returns the handle of the driver responsible for a given format    */
  /*    (or service) according to its `name'.                              */
  /*                                                                       */
  /* <Input>                                                               */
  /*    library     :: handle to library object.                           */
  /*    driver_name :: name of driver to look-up.                          */
  /*                                                                       */
  /* <Return>                                                              */
  /*    handle to driver object. 0 otherwise                               */
  /*                                                                       */
  EXPORT_DEF
  FT_Driver  FT_Get_Driver( FT_Library  library,
                            char*       driver_name );

#ifndef FT_CONFIG_OPTION_NO_DEFAULT_SYSTEM

 /**************************************************************************
  *
  * <Function>
  *   FT_New_Stream
  *
  * <Description>
  *   Open a new stream from a given standard ASCII file path name
  *
  * <Input>
  *   filepathname  :: an ASCII string naming the file to be opened
  * 
  * <Output>
  *   astream :: the opened stream descriptor to be used by the library
  *
  * <Return>
  *   Error code. 0 means success
  *
  * <Note>
  *   This function must be implemented by the system-specific part
  *   of the engine, i.e. `ftsystem.c'.
  *
  *   This function should only fill the stream descriptor. Note that
  *   the stream's `memory' field should be left to the caller.
  *
  **************************************************************************/
  
  extern
  FT_Error  FT_New_Stream( const char*  filepathname,
                           FT_Stream    astream );


 /**************************************************************************
  *
  * <Function>
  *   FT_New_Memory
  *
  * <Description>
  *   Returns a handle to a new memory object
  *
  * <Return>
  *   Handle to the memory object. 0 means failure
  *
  * <Note>
  *   This function must be implemented by the system-specific part
  *   of the engine, i.e. `ftsystem.c'.
  *
  *   It is only used by `ftinit' in order to implement the function
  *   FT_Init_FreeType.
  *
  **************************************************************************/
  
  extern
  FT_Memory  FT_New_Memory( void );

#endif

/* Define default raster's interface. The default raster is located in `src/base/ftraster.c' */
/*                                                                                           */
/* Client applications can register new rasters through the FT_Set_Raster API..              */
/*                                                                                           */
#ifndef FT_NO_DEFAULT_RASTER
  extern
  FT_Raster_Interface  ft_default_raster;
#endif


#endif /* FTOBJS_H */


/* END */
