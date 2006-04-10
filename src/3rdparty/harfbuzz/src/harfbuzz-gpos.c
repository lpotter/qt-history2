/*******************************************************************
 *
 *  Copyright 1996-2000 by
 *  David Turner, Robert Wilhelm, and Werner Lemberg.
 *
 *  Copyright 2006  Behdad Esfahbod
 *
 *  This is part of HarfBuzz, an OpenType Layout engine library.
 *
 *  See the file name COPYING for licensing information.
 *
 ******************************************************************/
#include "harfbuzz-impl.h"
#include "harfbuzz-gpos-private.h"
#include "harfbuzz-open-private.h"
#include "harfbuzz-gdef-private.h"

struct  GPOS_Instance_
{
  HB_GPOSHeader*  gpos;
  FT_Face          face;
  FT_Bool          dvi;
  FT_UShort        load_flags;  /* how the glyph should be loaded */
  FT_Bool          r2l;

  FT_UShort        last;        /* the last valid glyph -- used
				   with cursive positioning     */
  FT_Pos           anchor_x;    /* the coordinates of the anchor point */
  FT_Pos           anchor_y;    /* of the last valid glyph             */
};

typedef struct GPOS_Instance_  GPOS_Instance;


static FT_Error  GPOS_Do_Glyph_Lookup( GPOS_Instance*    gpi,
				       FT_UShort         lookup_index,
				       HB_Buffer        buffer,
				       FT_UShort         context_length,
				       int               nesting_level );



/* the client application must replace this with something more
   meaningful if multiple master fonts are to be supported.     */

static FT_Error  default_mmfunc( FT_Face      face,
				 FT_UShort    metric_id,
				 FT_Pos*      metric_value,
				 void*        data )
{
  FT_UNUSED(face);
  FT_UNUSED(metric_id);
  FT_UNUSED(metric_value);
  FT_UNUSED(data);
  return HB_Err_No_MM_Interpreter;
}



FT_Error  HB_Load_GPOS_Table( FT_Face          face,
			      HB_GPOSHeader** retptr,
			      HB_GDEFHeader*  gdef )
{
  FT_ULong         cur_offset, new_offset, base_offset;

  FT_UShort        i, num_lookups;
  HB_GPOSHeader*  gpos;
  HB_Lookup*      lo;

  FT_Stream  stream = face->stream;
  FT_Error   error;
  FT_Memory  memory = face->memory;


  if ( !retptr )
    return FT_Err_Invalid_Argument;

  if ( !stream )
    return FT_Err_Invalid_Face_Handle;

  if (( error = _hb_ftglue_face_goto_table( face, TTAG_GPOS, stream ) ))
    return error;

  base_offset = FILE_Pos();

  if ( ALLOC ( gpos, sizeof( *gpos ) ) )
    return error;

  gpos->memory = memory;
  gpos->gfunc = FT_Load_Glyph;
  gpos->mmfunc = default_mmfunc;

  /* skip version */

  if ( FILE_Seek( base_offset + 4L ) ||
       ACCESS_Frame( 2L ) )
    goto Fail4;

  new_offset = GET_UShort() + base_offset;

  FORGET_Frame();

  cur_offset = FILE_Pos();
  if ( FILE_Seek( new_offset ) ||
       ( error = _HB_OPEN_Load_ScriptList( &gpos->ScriptList,
				  stream ) ) != FT_Err_Ok )
    goto Fail4;
  (void)FILE_Seek( cur_offset );

  if ( ACCESS_Frame( 2L ) )
    goto Fail3;

  new_offset = GET_UShort() + base_offset;

  FORGET_Frame();

  cur_offset = FILE_Pos();
  if ( FILE_Seek( new_offset ) ||
       ( error = _HB_OPEN_Load_FeatureList( &gpos->FeatureList,
				   stream ) ) != FT_Err_Ok )
    goto Fail3;
  (void)FILE_Seek( cur_offset );

  if ( ACCESS_Frame( 2L ) )
    goto Fail2;

  new_offset = GET_UShort() + base_offset;

  FORGET_Frame();

  cur_offset = FILE_Pos();
  if ( FILE_Seek( new_offset ) ||
       ( error = _HB_OPEN_Load_LookupList( &gpos->LookupList,
				  stream, HB_Type_GPOS ) ) != FT_Err_Ok )
    goto Fail2;

  gpos->gdef = gdef;      /* can be NULL */

  /* We now check the LookupFlags for values larger than 0xFF to find
     out whether we need to load the `MarkAttachClassDef' field of the
     GDEF table -- this hack is necessary for OpenType 1.2 tables since
     the version field of the GDEF table hasn't been incremented.

     For constructed GDEF tables, we only load it if
     `MarkAttachClassDef_offset' is not zero (nevertheless, a build of
     a constructed mark attach table is not supported currently).       */

  if ( gdef &&
       gdef->MarkAttachClassDef_offset && !gdef->MarkAttachClassDef.loaded )
  {
    lo          = gpos->LookupList.Lookup;
    num_lookups = gpos->LookupList.LookupCount;

    for ( i = 0; i < num_lookups; i++ )
    {
      if ( lo[i].LookupFlag & HB_LOOKUP_FLAG_IGNORE_SPECIAL_MARKS )
      {
	if ( FILE_Seek( gdef->MarkAttachClassDef_offset ) ||
	     ( error = _HB_OPEN_Load_ClassDefinition( &gdef->MarkAttachClassDef,
					     256, stream ) ) != FT_Err_Ok )
	  goto Fail1;

	break;
      }
    }
  }

  *retptr = gpos;

  return FT_Err_Ok;

Fail1:
  _HB_OPEN_Free_LookupList( &gpos->LookupList, HB_Type_GPOS, memory );

Fail2:
  _HB_OPEN_Free_FeatureList( &gpos->FeatureList, memory );

Fail3:
  _HB_OPEN_Free_ScriptList( &gpos->ScriptList, memory );

Fail4:
  FREE( gpos );

  return error;
}


FT_Error  HB_Done_GPOS_Table( HB_GPOSHeader* gpos )
{
  FT_Memory memory = gpos->memory;

  _HB_OPEN_Free_LookupList( &gpos->LookupList, HB_Type_GPOS, memory );
  _HB_OPEN_Free_FeatureList( &gpos->FeatureList, memory );
  _HB_OPEN_Free_ScriptList( &gpos->ScriptList, memory );

  return FT_Err_Ok;
}


/*****************************
 * SubTable related functions
 *****************************/

/* shared tables */

/* ValueRecord */

/* There is a subtle difference in the specs between a `table' and a
   `record' -- offsets for device tables in ValueRecords are taken from
   the parent table and not the parent record.                          */

static FT_Error  Load_ValueRecord( HB_ValueRecord*  vr,
				   FT_UShort         format,
				   FT_ULong          base_offset,
				   FT_Stream         stream )
{
  FT_Error  error;
  FT_Memory memory = stream->memory;

  FT_ULong cur_offset, new_offset;


  if ( format & HB_GPOS_FORMAT_HAVE_X_PLACEMENT )
  {
    if ( ACCESS_Frame( 2L ) )
      return error;

    vr->XPlacement = GET_Short();

    FORGET_Frame();
  }
  else
    vr->XPlacement = 0;

  if ( format & HB_GPOS_FORMAT_HAVE_Y_PLACEMENT )
  {
    if ( ACCESS_Frame( 2L ) )
      return error;

    vr->YPlacement = GET_Short();

    FORGET_Frame();
  }
  else
    vr->YPlacement = 0;

  if ( format & HB_GPOS_FORMAT_HAVE_X_ADVANCE )
  {
    if ( ACCESS_Frame( 2L ) )
      return error;

    vr->XAdvance = GET_Short();

    FORGET_Frame();
  }
  else
    vr->XAdvance = 0;

  if ( format & HB_GPOS_FORMAT_HAVE_Y_ADVANCE )
  {
    if ( ACCESS_Frame( 2L ) )
      return error;

    vr->YAdvance = GET_Short();

    FORGET_Frame();
  }
  else
    vr->YAdvance = 0;

  if ( format & HB_GPOS_FORMAT_HAVE_X_PLACEMENT_DEVICE )
  {
    if ( ACCESS_Frame( 2L ) )
      return error;

    new_offset = GET_UShort();

    FORGET_Frame();

    if ( new_offset )
    {
      new_offset += base_offset;

      cur_offset = FILE_Pos();
      if ( FILE_Seek( new_offset ) ||
	   ( error = _HB_OPEN_Load_Device( &vr->XPlacementDevice,
				  stream ) ) != FT_Err_Ok )
	return error;
      (void)FILE_Seek( cur_offset );
    }
    else
      goto empty1;
  }
  else
  {
  empty1:
    vr->XPlacementDevice.StartSize  = 0;
    vr->XPlacementDevice.EndSize    = 0;
    vr->XPlacementDevice.DeltaValue = NULL;
  }

  if ( format & HB_GPOS_FORMAT_HAVE_Y_PLACEMENT_DEVICE )
  {
    if ( ACCESS_Frame( 2L ) )
      goto Fail3;

    new_offset = GET_UShort();

    FORGET_Frame();

    if ( new_offset )
    {
      new_offset += base_offset;

      cur_offset = FILE_Pos();
      if ( FILE_Seek( new_offset ) ||
	   ( error = _HB_OPEN_Load_Device( &vr->YPlacementDevice,
				  stream ) ) != FT_Err_Ok )
	goto Fail3;
      (void)FILE_Seek( cur_offset );
    }
    else
      goto empty2;
  }
  else
  {
  empty2:
    vr->YPlacementDevice.StartSize  = 0;
    vr->YPlacementDevice.EndSize    = 0;
    vr->YPlacementDevice.DeltaValue = NULL;
  }

  if ( format & HB_GPOS_FORMAT_HAVE_X_ADVANCE_DEVICE )
  {
    if ( ACCESS_Frame( 2L ) )
      goto Fail2;

    new_offset = GET_UShort();

    FORGET_Frame();

    if ( new_offset )
    {
      new_offset += base_offset;

      cur_offset = FILE_Pos();
      if ( FILE_Seek( new_offset ) ||
	   ( error = _HB_OPEN_Load_Device( &vr->XAdvanceDevice,
				  stream ) ) != FT_Err_Ok )
	goto Fail2;
      (void)FILE_Seek( cur_offset );
    }
    else
      goto empty3;
  }
  else
  {
  empty3:
    vr->XAdvanceDevice.StartSize  = 0;
    vr->XAdvanceDevice.EndSize    = 0;
    vr->XAdvanceDevice.DeltaValue = NULL;
  }

  if ( format & HB_GPOS_FORMAT_HAVE_Y_ADVANCE_DEVICE )
  {
    if ( ACCESS_Frame( 2L ) )
      goto Fail1;

    new_offset = GET_UShort();

    FORGET_Frame();

    if ( new_offset )
    {
      new_offset += base_offset;

      cur_offset = FILE_Pos();
      if ( FILE_Seek( new_offset ) ||
	   ( error = _HB_OPEN_Load_Device( &vr->YAdvanceDevice,
				  stream ) ) != FT_Err_Ok )
	goto Fail1;
      (void)FILE_Seek( cur_offset );
    }
    else
      goto empty4;
  }
  else
  {
  empty4:
    vr->YAdvanceDevice.StartSize  = 0;
    vr->YAdvanceDevice.EndSize    = 0;
    vr->YAdvanceDevice.DeltaValue = NULL;
  }

  if ( format & HB_GPOS_FORMAT_HAVE_X_ID_PLACEMENT )
  {
    if ( ACCESS_Frame( 2L ) )
      goto Fail1;

    vr->XIdPlacement = GET_UShort();

    FORGET_Frame();
  }
  else
    vr->XIdPlacement = 0;

  if ( format & HB_GPOS_FORMAT_HAVE_Y_ID_PLACEMENT )
  {
    if ( ACCESS_Frame( 2L ) )
      goto Fail1;

    vr->YIdPlacement = GET_UShort();

    FORGET_Frame();
  }
  else
    vr->YIdPlacement = 0;

  if ( format & HB_GPOS_FORMAT_HAVE_X_ID_ADVANCE )
  {
    if ( ACCESS_Frame( 2L ) )
      goto Fail1;

    vr->XIdAdvance = GET_UShort();

    FORGET_Frame();
  }
  else
    vr->XIdAdvance = 0;

  if ( format & HB_GPOS_FORMAT_HAVE_Y_ID_ADVANCE )
  {
    if ( ACCESS_Frame( 2L ) )
      goto Fail1;

    vr->YIdAdvance = GET_UShort();

    FORGET_Frame();
  }
  else
    vr->YIdAdvance = 0;

  return FT_Err_Ok;

Fail1:
  _HB_OPEN_Free_Device( &vr->YAdvanceDevice, memory );

Fail2:
  _HB_OPEN_Free_Device( &vr->XAdvanceDevice, memory );

Fail3:
  _HB_OPEN_Free_Device( &vr->YPlacementDevice, memory );
  return error;
}


static void  Free_ValueRecord( HB_ValueRecord*  vr,
			       FT_UShort         format,
			       FT_Memory         memory )
{
  if ( format & HB_GPOS_FORMAT_HAVE_Y_ADVANCE_DEVICE )
    _HB_OPEN_Free_Device( &vr->YAdvanceDevice, memory );
  if ( format & HB_GPOS_FORMAT_HAVE_X_ADVANCE_DEVICE )
    _HB_OPEN_Free_Device( &vr->XAdvanceDevice, memory );
  if ( format & HB_GPOS_FORMAT_HAVE_Y_PLACEMENT_DEVICE )
    _HB_OPEN_Free_Device( &vr->YPlacementDevice, memory );
  if ( format & HB_GPOS_FORMAT_HAVE_X_PLACEMENT_DEVICE )
    _HB_OPEN_Free_Device( &vr->XPlacementDevice, memory );
}


static FT_Error  Get_ValueRecord( GPOS_Instance*    gpi,
				  HB_ValueRecord*  vr,
				  FT_UShort         format,
				  HB_Position      gd )
{
  FT_Pos           value;
  FT_Short         pixel_value;
  FT_Error         error = FT_Err_Ok;
  HB_GPOSHeader*  gpos = gpi->gpos;

  FT_UShort  x_ppem, y_ppem;
  FT_Fixed   x_scale, y_scale;


  if ( !format )
    return FT_Err_Ok;

  x_ppem  = gpi->face->size->metrics.x_ppem;
  y_ppem  = gpi->face->size->metrics.y_ppem;
  x_scale = gpi->face->size->metrics.x_scale;
  y_scale = gpi->face->size->metrics.y_scale;

  /* design units -> fractional pixel */

  if ( format & HB_GPOS_FORMAT_HAVE_X_PLACEMENT )
    gd->x_pos += x_scale * vr->XPlacement / 0x10000;
  if ( format & HB_GPOS_FORMAT_HAVE_Y_PLACEMENT )
    gd->y_pos += y_scale * vr->YPlacement / 0x10000;
  if ( format & HB_GPOS_FORMAT_HAVE_X_ADVANCE )
    gd->x_advance += x_scale * vr->XAdvance / 0x10000;
  if ( format & HB_GPOS_FORMAT_HAVE_Y_ADVANCE )
    gd->y_advance += y_scale * vr->YAdvance / 0x10000;

  if ( !gpi->dvi )
  {
    /* pixel -> fractional pixel */

    if ( format & HB_GPOS_FORMAT_HAVE_X_PLACEMENT_DEVICE )
    {
      _HB_OPEN_Get_Device( &vr->XPlacementDevice, x_ppem, &pixel_value );
      gd->x_pos += pixel_value << 6;
    }
    if ( format & HB_GPOS_FORMAT_HAVE_Y_PLACEMENT_DEVICE )
    {
      _HB_OPEN_Get_Device( &vr->YPlacementDevice, y_ppem, &pixel_value );
      gd->y_pos += pixel_value << 6;
    }
    if ( format & HB_GPOS_FORMAT_HAVE_X_ADVANCE_DEVICE )
    {
      _HB_OPEN_Get_Device( &vr->XAdvanceDevice, x_ppem, &pixel_value );
      gd->x_advance += pixel_value << 6;
    }
    if ( format & HB_GPOS_FORMAT_HAVE_Y_ADVANCE_DEVICE )
    {
      _HB_OPEN_Get_Device( &vr->YAdvanceDevice, y_ppem, &pixel_value );
      gd->y_advance += pixel_value << 6;
    }
  }

  /* values returned from mmfunc() are already in fractional pixels */

  if ( format & HB_GPOS_FORMAT_HAVE_X_ID_PLACEMENT )
  {
    error = (gpos->mmfunc)( gpi->face, vr->XIdPlacement,
			    &value, gpos->data );
    if ( error )
      return error;
    gd->x_pos += value;
  }
  if ( format & HB_GPOS_FORMAT_HAVE_Y_ID_PLACEMENT )
  {
    error = (gpos->mmfunc)( gpi->face, vr->YIdPlacement,
			    &value, gpos->data );
    if ( error )
      return error;
    gd->y_pos += value;
  }
  if ( format & HB_GPOS_FORMAT_HAVE_X_ID_ADVANCE )
  {
    error = (gpos->mmfunc)( gpi->face, vr->XIdAdvance,
			    &value, gpos->data );
    if ( error )
      return error;
    gd->x_advance += value;
  }
  if ( format & HB_GPOS_FORMAT_HAVE_Y_ID_ADVANCE )
  {
    error = (gpos->mmfunc)( gpi->face, vr->YIdAdvance,
			    &value, gpos->data );
    if ( error )
      return error;
    gd->y_advance += value;
  }

  return error;
}


/* AnchorFormat1 */
/* AnchorFormat2 */
/* AnchorFormat3 */
/* AnchorFormat4 */

static FT_Error  Load_Anchor( HB_Anchor*  an,
			      FT_Stream    stream )
{
  FT_Error  error;
  FT_Memory memory = stream->memory;

  FT_ULong cur_offset, new_offset, base_offset;


  base_offset = FILE_Pos();

  if ( ACCESS_Frame( 2L ) )
    return error;

  an->PosFormat = GET_UShort();

  FORGET_Frame();

  switch ( an->PosFormat )
  {
  case 1:
    if ( ACCESS_Frame( 4L ) )
      return error;

    an->af.af1.XCoordinate = GET_Short();
    an->af.af1.YCoordinate = GET_Short();

    FORGET_Frame();
    break;

  case 2:
    if ( ACCESS_Frame( 6L ) )
      return error;

    an->af.af2.XCoordinate = GET_Short();
    an->af.af2.YCoordinate = GET_Short();
    an->af.af2.AnchorPoint = GET_UShort();

    FORGET_Frame();
    break;

  case 3:
    if ( ACCESS_Frame( 6L ) )
      return error;

    an->af.af3.XCoordinate = GET_Short();
    an->af.af3.YCoordinate = GET_Short();

    new_offset = GET_UShort();

    FORGET_Frame();

    if ( new_offset )
    {
      new_offset += base_offset;

      cur_offset = FILE_Pos();
      if ( FILE_Seek( new_offset ) ||
	   ( error = _HB_OPEN_Load_Device( &an->af.af3.XDeviceTable,
				  stream ) ) != FT_Err_Ok )
	return error;
      (void)FILE_Seek( cur_offset );
    }
    else
    {
      an->af.af3.XDeviceTable.StartSize  = 0;
      an->af.af3.XDeviceTable.EndSize    = 0;
      an->af.af3.XDeviceTable.DeltaValue = NULL;
    }

    if ( ACCESS_Frame( 2L ) )
      goto Fail;

    new_offset = GET_UShort();

    FORGET_Frame();

    if ( new_offset )
    {
      new_offset += base_offset;

      cur_offset = FILE_Pos();
      if ( FILE_Seek( new_offset ) ||
	   ( error = _HB_OPEN_Load_Device( &an->af.af3.YDeviceTable,
				  stream ) ) != FT_Err_Ok )
	goto Fail;
      (void)FILE_Seek( cur_offset );
    }
    else
    {
      an->af.af3.YDeviceTable.StartSize  = 0;
      an->af.af3.YDeviceTable.EndSize    = 0;
      an->af.af3.YDeviceTable.DeltaValue = NULL;
    }
    break;

  case 4:
    if ( ACCESS_Frame( 4L ) )
      return error;

    an->af.af4.XIdAnchor = GET_UShort();
    an->af.af4.YIdAnchor = GET_UShort();

    FORGET_Frame();
    break;

  default:
    return HB_Err_Invalid_GPOS_SubTable_Format;
  }

  return FT_Err_Ok;

Fail:
  _HB_OPEN_Free_Device( &an->af.af3.XDeviceTable, memory );
  return error;
}


static void  Free_Anchor( HB_Anchor*  an,
			  FT_Memory    memory)
{
  if ( an->PosFormat == 3 )
  {
    _HB_OPEN_Free_Device( &an->af.af3.YDeviceTable, memory );
    _HB_OPEN_Free_Device( &an->af.af3.XDeviceTable, memory );
  }
}


static FT_Error  Get_Anchor( GPOS_Instance*   gpi,
			     HB_Anchor*      an,
			     FT_UShort        glyph_index,
			     FT_Pos*          x_value,
			     FT_Pos*          y_value )
{
  FT_Error  error = FT_Err_Ok;

  FT_Outline       outline;
  HB_GPOSHeader*  gpos = gpi->gpos;
  FT_UShort        ap;

  FT_Short         pixel_value;
  FT_UShort        load_flags;

  FT_UShort        x_ppem, y_ppem;
  FT_Fixed         x_scale, y_scale;


  x_ppem  = gpi->face->size->metrics.x_ppem;
  y_ppem  = gpi->face->size->metrics.y_ppem;
  x_scale = gpi->face->size->metrics.x_scale;
  y_scale = gpi->face->size->metrics.y_scale;

  switch ( an->PosFormat )
  {
  case 0:
    /* The special case of an empty AnchorTable */

    return HB_Err_Not_Covered;

  case 1:
    *x_value = x_scale * an->af.af1.XCoordinate / 0x10000;
    *y_value = y_scale * an->af.af1.YCoordinate / 0x10000;
    break;

  case 2:
    /* glyphs must be scaled */

    load_flags = gpi->load_flags & ~FT_LOAD_NO_SCALE;

    if ( !gpi->dvi )
    {
      error = (gpos->gfunc)( gpi->face, glyph_index, load_flags );
      if ( error )
	return error;

      if ( gpi->face->glyph->format != ft_glyph_format_outline )
	return HB_Err_Invalid_GPOS_SubTable;

      ap = an->af.af2.AnchorPoint;

      outline = gpi->face->glyph->outline;

      /* if outline.n_points is set to zero by gfunc(), we use the
	 design coordinate value pair.  This can happen e.g. for
	 sbit glyphs                                               */

      if ( !outline.n_points )
	goto no_contour_point;

      if ( ap >= outline.n_points )
	return HB_Err_Invalid_GPOS_SubTable;

      *x_value = outline.points[ap].x;
      *y_value = outline.points[ap].y;
    }
    else
    {
    no_contour_point:
      *x_value = x_scale * an->af.af3.XCoordinate / 0x10000;
      *y_value = y_scale * an->af.af3.YCoordinate / 0x10000;
    }
    break;

  case 3:
    if ( !gpi->dvi )
    {
      _HB_OPEN_Get_Device( &an->af.af3.XDeviceTable, x_ppem, &pixel_value );
      *x_value = pixel_value << 6;
      _HB_OPEN_Get_Device( &an->af.af3.YDeviceTable, y_ppem, &pixel_value );
      *y_value = pixel_value << 6;
    }
    else
      *x_value = *y_value = 0;

    *x_value += x_scale * an->af.af3.XCoordinate / 0x10000;
    *y_value += y_scale * an->af.af3.YCoordinate / 0x10000;
    break;

  case 4:
    error = (gpos->mmfunc)( gpi->face, an->af.af4.XIdAnchor,
			    x_value, gpos->data );
    if ( error )
      return error;

    error = (gpos->mmfunc)( gpi->face, an->af.af4.YIdAnchor,
			    y_value, gpos->data );
    if ( error )
      return error;
    break;
  }

  return error;
}


/* MarkArray */

static FT_Error  Load_MarkArray ( HB_MarkArray*  ma,
				  FT_Stream       stream )
{
  FT_Error  error;
  FT_Memory memory = stream->memory;

  FT_UShort        n, m, count;
  FT_ULong         cur_offset, new_offset, base_offset;

  HB_MarkRecord*  mr;


  base_offset = FILE_Pos();

  if ( ACCESS_Frame( 2L ) )
    return error;

  count = ma->MarkCount = GET_UShort();

  FORGET_Frame();

  ma->MarkRecord = NULL;

  if ( ALLOC_ARRAY( ma->MarkRecord, count, HB_MarkRecord ) )
    return error;

  mr = ma->MarkRecord;

  for ( n = 0; n < count; n++ )
  {
    if ( ACCESS_Frame( 4L ) )
      goto Fail;

    mr[n].Class = GET_UShort();
    new_offset  = GET_UShort() + base_offset;

    FORGET_Frame();

    cur_offset = FILE_Pos();
    if ( FILE_Seek( new_offset ) ||
	 ( error = Load_Anchor( &mr[n].MarkAnchor, stream ) ) != FT_Err_Ok )
      goto Fail;
    (void)FILE_Seek( cur_offset );
  }

  return FT_Err_Ok;

Fail:
  for ( m = 0; m < n; m++ )
    Free_Anchor( &mr[m].MarkAnchor, memory );

  FREE( mr );
  return error;
}


static void  Free_MarkArray( HB_MarkArray*  ma,
			     FT_Memory       memory )
{
  FT_UShort        n, count;

  HB_MarkRecord*  mr;


  if ( ma->MarkRecord )
  {
    count = ma->MarkCount;
    mr    = ma->MarkRecord;

    for ( n = 0; n < count; n++ )
      Free_Anchor( &mr[n].MarkAnchor, memory );

    FREE( mr );
  }
}


/* LookupType 1 */

/* SinglePosFormat1 */
/* SinglePosFormat2 */

static FT_Error  Load_SinglePos( HB_GPOS_SubTable* st,
				 FT_Stream       stream )
{
  FT_Error  error;
  FT_Memory memory = stream->memory;
  HB_SinglePos*   sp = &st->single;

  FT_UShort         n, m, count, format;
  FT_ULong          cur_offset, new_offset, base_offset;

  HB_ValueRecord*  vr;


  base_offset = FILE_Pos();

  if ( ACCESS_Frame( 6L ) )
    return error;

  sp->PosFormat = GET_UShort();
  new_offset    = GET_UShort() + base_offset;

  format = sp->ValueFormat = GET_UShort();

  FORGET_Frame();

  if ( !format )
    return HB_Err_Invalid_GPOS_SubTable;

  cur_offset = FILE_Pos();
  if ( FILE_Seek( new_offset ) ||
       ( error = _HB_OPEN_Load_Coverage( &sp->Coverage, stream ) ) != FT_Err_Ok )
    return error;
  (void)FILE_Seek( cur_offset );

  switch ( sp->PosFormat )
  {
  case 1:
    error = Load_ValueRecord( &sp->spf.spf1.Value, format,
			      base_offset, stream );
    if ( error )
      goto Fail2;
    break;

  case 2:
    if ( ACCESS_Frame( 2L ) )
      goto Fail2;

    count = sp->spf.spf2.ValueCount = GET_UShort();

    FORGET_Frame();

    sp->spf.spf2.Value = NULL;

    if ( ALLOC_ARRAY( sp->spf.spf2.Value, count, HB_ValueRecord ) )
      goto Fail2;

    vr = sp->spf.spf2.Value;

    for ( n = 0; n < count; n++ )
    {
      error = Load_ValueRecord( &vr[n], format, base_offset, stream );
      if ( error )
	goto Fail1;
    }
    break;

  default:
    return HB_Err_Invalid_GPOS_SubTable_Format;
  }

  return FT_Err_Ok;

Fail1:
  for ( m = 0; m < n; m++ )
    Free_ValueRecord( &vr[m], format, memory );

  FREE( vr );

Fail2:
  _HB_OPEN_Free_Coverage( &sp->Coverage, memory );
  return error;
}


static void  Free_SinglePos( HB_GPOS_SubTable* st,
			     FT_Memory       memory )
{
  FT_UShort         n, count, format;
  HB_SinglePos*   sp = &st->single;

  HB_ValueRecord*  v;


  format = sp->ValueFormat;

  switch ( sp->PosFormat )
  {
  case 1:
    Free_ValueRecord( &sp->spf.spf1.Value, format, memory );
    break;

  case 2:
    if ( sp->spf.spf2.Value )
    {
      count = sp->spf.spf2.ValueCount;
      v     = sp->spf.spf2.Value;

      for ( n = 0; n < count; n++ )
	Free_ValueRecord( &v[n], format, memory );

      FREE( v );
    }
    break;
  }

  _HB_OPEN_Free_Coverage( &sp->Coverage, memory );
}

static FT_Error  Lookup_DefaultPos(  GPOS_Instance*    gpi,
				     HB_GPOS_SubTable* st,
				     HB_Buffer        buffer,
				     FT_UShort         flags,
				     FT_UShort         context_length,
				     int               nesting_level )
{
  FT_UNUSED(gpi);
  FT_UNUSED(st);
  FT_UNUSED(buffer);
  FT_UNUSED(flags);
  FT_UNUSED(context_length);
  FT_UNUSED(nesting_level);
  return HB_Err_Not_Covered;
}

static FT_Error  Lookup_SinglePos( GPOS_Instance*    gpi,
				   HB_GPOS_SubTable* st,
				   HB_Buffer        buffer,
				   FT_UShort         flags,
				   FT_UShort         context_length,
				   int               nesting_level )
{
  FT_UShort        index, property;
  FT_Error         error;
  HB_GPOSHeader*  gpos = gpi->gpos;
  HB_SinglePos*   sp = &st->single;

  FT_UNUSED(nesting_level);

  if ( context_length != 0xFFFF && context_length < 1 )
    return HB_Err_Not_Covered;

  if ( CHECK_Property( gpos->gdef, IN_CURITEM(), flags, &property ) )
    return error;

  error = _HB_OPEN_Coverage_Index( &sp->Coverage, IN_CURGLYPH(), &index );
  if ( error )
    return error;

  switch ( sp->PosFormat )
  {
  case 1:
    error = Get_ValueRecord( gpi, &sp->spf.spf1.Value,
			     sp->ValueFormat, POSITION( buffer->in_pos ) );
    if ( error )
      return error;
    break;

  case 2:
    if ( index >= sp->spf.spf2.ValueCount )
      return HB_Err_Invalid_GPOS_SubTable;
    error = Get_ValueRecord( gpi, &sp->spf.spf2.Value[index],
			     sp->ValueFormat, POSITION( buffer->in_pos ) );
    if ( error )
      return error;
    break;

  default:
    return HB_Err_Invalid_GPOS_SubTable;
  }

  (buffer->in_pos)++;

  return FT_Err_Ok;
}


/* LookupType 2 */

/* PairSet */

static FT_Error  Load_PairSet ( HB_PairSet*  ps,
				FT_UShort     format1,
				FT_UShort     format2,
				FT_Stream     stream )
{
  FT_Error  error;
  FT_Memory memory = stream->memory;

  FT_UShort             n, m, count;
  FT_ULong              base_offset;

  HB_PairValueRecord*  pvr;


  base_offset = FILE_Pos();

  if ( ACCESS_Frame( 2L ) )
    return error;

  count = ps->PairValueCount = GET_UShort();

  FORGET_Frame();

  ps->PairValueRecord = NULL;

  if ( ALLOC_ARRAY( ps->PairValueRecord, count, HB_PairValueRecord ) )
    return error;

  pvr = ps->PairValueRecord;

  for ( n = 0; n < count; n++ )
  {
    if ( ACCESS_Frame( 2L ) )
      goto Fail;

    pvr[n].SecondGlyph = GET_UShort();

    FORGET_Frame();

    if ( format1 )
    {
      error = Load_ValueRecord( &pvr[n].Value1, format1,
				base_offset, stream );
      if ( error )
	goto Fail;
    }
    if ( format2 )
    {
      error = Load_ValueRecord( &pvr[n].Value2, format2,
				base_offset, stream );
      if ( error )
      {
	if ( format1 )
	  Free_ValueRecord( &pvr[n].Value1, format1, memory );
	goto Fail;
      }
    }
  }

  return FT_Err_Ok;

Fail:
  for ( m = 0; m < n; m++ )
  {
    if ( format1 )
      Free_ValueRecord( &pvr[m].Value1, format1, memory );
    if ( format2 )
      Free_ValueRecord( &pvr[m].Value2, format2, memory );
  }

  FREE( pvr );
  return error;
}


static void  Free_PairSet( HB_PairSet*  ps,
			   FT_UShort     format1,
			   FT_UShort     format2,
			   FT_Memory     memory )
{
  FT_UShort             n, count;

  HB_PairValueRecord*  pvr;


  if ( ps->PairValueRecord )
  {
    count = ps->PairValueCount;
    pvr   = ps->PairValueRecord;

    for ( n = 0; n < count; n++ )
    {
      if ( format1 )
	Free_ValueRecord( &pvr[n].Value1, format1, memory );
      if ( format2 )
	Free_ValueRecord( &pvr[n].Value2, format2, memory );
    }

    FREE( pvr );
  }
}


/* PairPosFormat1 */

static FT_Error  Load_PairPos1( HB_PairPosFormat1*  ppf1,
				FT_UShort            format1,
				FT_UShort            format2,
				FT_Stream            stream )
{
  FT_Error  error;
  FT_Memory memory = stream->memory;

  FT_UShort     n, m, count;
  FT_ULong      cur_offset, new_offset, base_offset;

  HB_PairSet*  ps;


  base_offset = FILE_Pos() - 8L;

  if ( ACCESS_Frame( 2L ) )
    return error;

  count = ppf1->PairSetCount = GET_UShort();

  FORGET_Frame();

  ppf1->PairSet = NULL;

  if ( ALLOC_ARRAY( ppf1->PairSet, count, HB_PairSet ) )
    return error;

  ps = ppf1->PairSet;

  for ( n = 0; n < count; n++ )
  {
    if ( ACCESS_Frame( 2L ) )
      goto Fail;

    new_offset = GET_UShort() + base_offset;

    FORGET_Frame();

    cur_offset = FILE_Pos();
    if ( FILE_Seek( new_offset ) ||
	 ( error = Load_PairSet( &ps[n], format1,
				 format2, stream ) ) != FT_Err_Ok )
      goto Fail;
    (void)FILE_Seek( cur_offset );
  }

  return FT_Err_Ok;

Fail:
  for ( m = 0; m < n; m++ )
    Free_PairSet( &ps[m], format1, format2, memory );

  FREE( ps );
  return error;
}


static void  Free_PairPos1( HB_PairPosFormat1*  ppf1,
			    FT_UShort            format1,
			    FT_UShort            format2,
			    FT_Memory            memory )
{
  FT_UShort     n, count;

  HB_PairSet*  ps;


  if ( ppf1->PairSet )
  {
    count = ppf1->PairSetCount;
    ps    = ppf1->PairSet;

    for ( n = 0; n < count; n++ )
      Free_PairSet( &ps[n], format1, format2, memory );

    FREE( ps );
  }
}


/* PairPosFormat2 */

static FT_Error  Load_PairPos2( HB_PairPosFormat2*  ppf2,
				FT_UShort            format1,
				FT_UShort            format2,
				FT_Stream            stream )
{
  FT_Error  error;
  FT_Memory memory = stream->memory;

  FT_UShort          m, n, k, count1, count2;
  FT_ULong           cur_offset, new_offset1, new_offset2, base_offset;

  HB_Class1Record*  c1r;
  HB_Class2Record*  c2r;


  base_offset = FILE_Pos() - 8L;

  if ( ACCESS_Frame( 8L ) )
    return error;

  new_offset1 = GET_UShort() + base_offset;
  new_offset2 = GET_UShort() + base_offset;

  /* `Class1Count' and `Class2Count' are the upper limits for class
     values, thus we read it now to make additional safety checks.  */

  count1 = ppf2->Class1Count = GET_UShort();
  count2 = ppf2->Class2Count = GET_UShort();

  FORGET_Frame();

  cur_offset = FILE_Pos();
  if ( FILE_Seek( new_offset1 ) ||
       ( error = _HB_OPEN_Load_ClassDefinition( &ppf2->ClassDef1, count1,
				       stream ) ) != FT_Err_Ok )
    return error;
  if ( FILE_Seek( new_offset2 ) ||
       ( error = _HB_OPEN_Load_ClassDefinition( &ppf2->ClassDef2, count2,
				       stream ) ) != FT_Err_Ok )
    goto Fail3;
  (void)FILE_Seek( cur_offset );

  ppf2->Class1Record = NULL;

  if ( ALLOC_ARRAY( ppf2->Class1Record, count1, HB_Class1Record ) )
    goto Fail2;

  c1r = ppf2->Class1Record;

  for ( m = 0; m < count1; m++ )
  {
    c1r[m].Class2Record = NULL;

    if ( ALLOC_ARRAY( c1r[m].Class2Record, count2, HB_Class2Record ) )
      goto Fail1;

    c2r = c1r[m].Class2Record;

    for ( n = 0; n < count2; n++ )
    {
      if ( format1 )
      {
	error = Load_ValueRecord( &c2r[n].Value1, format1,
				  base_offset, stream );
	if ( error )
	  goto Fail0;
      }
      if ( format2 )
      {
	error = Load_ValueRecord( &c2r[n].Value2, format2,
				  base_offset, stream );
	if ( error )
	{
	  if ( format1 )
	    Free_ValueRecord( &c2r[n].Value1, format1, memory );
	  goto Fail0;
	}
      }
    }

    continue;

  Fail0:
    for ( k = 0; k < n; k++ )
    {
      if ( format1 )
	Free_ValueRecord( &c2r[k].Value1, format1, memory );
      if ( format2 )
	Free_ValueRecord( &c2r[k].Value2, format2, memory );
    }
    goto Fail1;
  }

  return FT_Err_Ok;

Fail1:
  for ( k = 0; k < m; k++ )
  {
    c2r = c1r[k].Class2Record;

    for ( n = 0; n < count2; n++ )
    {
      if ( format1 )
	Free_ValueRecord( &c2r[n].Value1, format1, memory );
      if ( format2 )
	Free_ValueRecord( &c2r[n].Value2, format2, memory );
    }

    FREE( c2r );
  }

  FREE( c1r );
Fail2:

  _HB_OPEN_Free_ClassDefinition( &ppf2->ClassDef2, memory );

Fail3:
  _HB_OPEN_Free_ClassDefinition( &ppf2->ClassDef1, memory );
  return error;
}


static void  Free_PairPos2( HB_PairPosFormat2*  ppf2,
			    FT_UShort            format1,
			    FT_UShort            format2,
			    FT_Memory            memory )
{
  FT_UShort          m, n, count1, count2;

  HB_Class1Record*  c1r;
  HB_Class2Record*  c2r;


  if ( ppf2->Class1Record )
  {
    c1r    = ppf2->Class1Record;
    count1 = ppf2->Class1Count;
    count2 = ppf2->Class2Count;

    for ( m = 0; m < count1; m++ )
    {
      c2r = c1r[m].Class2Record;

      for ( n = 0; n < count2; n++ )
      {
	if ( format1 )
	  Free_ValueRecord( &c2r[n].Value1, format1, memory );
	if ( format2 )
	  Free_ValueRecord( &c2r[n].Value2, format2, memory );
      }

      FREE( c2r );
    }

    FREE( c1r );

    _HB_OPEN_Free_ClassDefinition( &ppf2->ClassDef2, memory );
    _HB_OPEN_Free_ClassDefinition( &ppf2->ClassDef1, memory );
  }
}


static FT_Error  Load_PairPos( HB_GPOS_SubTable* st,
			       FT_Stream     stream )
{
  FT_Error  error;
  FT_Memory memory = stream->memory;
  HB_PairPos*     pp = &st->pair;

  FT_UShort         format1, format2;
  FT_ULong          cur_offset, new_offset, base_offset;


  base_offset = FILE_Pos();

  if ( ACCESS_Frame( 8L ) )
    return error;

  pp->PosFormat = GET_UShort();
  new_offset    = GET_UShort() + base_offset;

  format1 = pp->ValueFormat1 = GET_UShort();
  format2 = pp->ValueFormat2 = GET_UShort();

  FORGET_Frame();

  cur_offset = FILE_Pos();
  if ( FILE_Seek( new_offset ) ||
       ( error = _HB_OPEN_Load_Coverage( &pp->Coverage, stream ) ) != FT_Err_Ok )
    return error;
  (void)FILE_Seek( cur_offset );

  switch ( pp->PosFormat )
  {
  case 1:
    error = Load_PairPos1( &pp->ppf.ppf1, format1, format2, stream );
    if ( error )
      goto Fail;
    break;

  case 2:
    error = Load_PairPos2( &pp->ppf.ppf2, format1, format2, stream );
    if ( error )
      goto Fail;
    break;

  default:
    return HB_Err_Invalid_GPOS_SubTable_Format;
  }

  return FT_Err_Ok;

Fail:
  _HB_OPEN_Free_Coverage( &pp->Coverage, memory );
  return error;
}


static void  Free_PairPos( HB_GPOS_SubTable* st,
			   FT_Memory     memory )
{
  FT_UShort  format1, format2;
  HB_PairPos*     pp = &st->pair;


  format1 = pp->ValueFormat1;
  format2 = pp->ValueFormat2;

  switch ( pp->PosFormat )
  {
  case 1:
    Free_PairPos1( &pp->ppf.ppf1, format1, format2, memory );
    break;

  case 2:
    Free_PairPos2( &pp->ppf.ppf2, format1, format2, memory );
    break;
  }

  _HB_OPEN_Free_Coverage( &pp->Coverage, memory );
}


static FT_Error  Lookup_PairPos1( GPOS_Instance*       gpi,
				  HB_PairPosFormat1*  ppf1,
				  HB_Buffer           buffer,
				  FT_UShort            first_pos,
				  FT_UShort            index,
				  FT_UShort            format1,
				  FT_UShort            format2 )
{
  FT_Error              error;
  FT_UShort             numpvr, glyph2;

  HB_PairValueRecord*  pvr;


  if ( index >= ppf1->PairSetCount )
     return HB_Err_Invalid_GPOS_SubTable;

  pvr = ppf1->PairSet[index].PairValueRecord;
  if ( !pvr )
    return HB_Err_Invalid_GPOS_SubTable;

  glyph2 = IN_CURGLYPH();

  for ( numpvr = ppf1->PairSet[index].PairValueCount;
	numpvr;
	numpvr--, pvr++ )
  {
    if ( glyph2 == pvr->SecondGlyph )
    {
      error = Get_ValueRecord( gpi, &pvr->Value1, format1,
			       POSITION( first_pos ) );
      if ( error )
	return error;
      return Get_ValueRecord( gpi, &pvr->Value2, format2,
			      POSITION( buffer->in_pos ) );
    }
  }

  return HB_Err_Not_Covered;
}


static FT_Error  Lookup_PairPos2( GPOS_Instance*       gpi,
				  HB_PairPosFormat2*  ppf2,
				  HB_Buffer           buffer,
				  FT_UShort            first_pos,
				  FT_UShort            format1,
				  FT_UShort            format2 )
{
  FT_Error           error;
  FT_UShort          cl1, cl2;

  HB_Class1Record*  c1r;
  HB_Class2Record*  c2r;


  error = _HB_OPEN_Get_Class( &ppf2->ClassDef1, IN_GLYPH( first_pos ),
		     &cl1, NULL );
  if ( error && error != HB_Err_Not_Covered )
    return error;
  error = _HB_OPEN_Get_Class( &ppf2->ClassDef2, IN_CURGLYPH(),
		     &cl2, NULL );
  if ( error && error != HB_Err_Not_Covered )
    return error;

  c1r = &ppf2->Class1Record[cl1];
  if ( !c1r )
    return HB_Err_Invalid_GPOS_SubTable;
  c2r = &c1r->Class2Record[cl2];

  error = Get_ValueRecord( gpi, &c2r->Value1, format1, POSITION( first_pos ) );
  if ( error )
    return error;
  return Get_ValueRecord( gpi, &c2r->Value2, format2, POSITION( buffer->in_pos ) );
}


static FT_Error  Lookup_PairPos( GPOS_Instance*    gpi,
				 HB_GPOS_SubTable* st,
				 HB_Buffer        buffer,
				 FT_UShort         flags,
				 FT_UShort         context_length,
				 int               nesting_level )
{
  FT_Error         error;
  FT_UShort        index, property, first_pos;
  HB_GPOSHeader*  gpos = gpi->gpos;
  HB_PairPos*     pp = &st->pair;

  FT_UNUSED(nesting_level);

  if ( buffer->in_pos >= buffer->in_length - 1 )
    return HB_Err_Not_Covered;           /* Not enough glyphs in stream */

  if ( context_length != 0xFFFF && context_length < 2 )
    return HB_Err_Not_Covered;

  if ( CHECK_Property( gpos->gdef, IN_CURITEM(), flags, &property ) )
    return error;

  error = _HB_OPEN_Coverage_Index( &pp->Coverage, IN_CURGLYPH(), &index );
  if ( error )
    return error;

  /* second glyph */

  first_pos = buffer->in_pos;
  (buffer->in_pos)++;

  while ( CHECK_Property( gpos->gdef, IN_CURITEM(),
			  flags, &property ) )
  {
    if ( error && error != HB_Err_Not_Covered )
      return error;

    if ( buffer->in_pos == buffer->in_length )
      return HB_Err_Not_Covered;
    (buffer->in_pos)++;
  }

  switch ( pp->PosFormat )
  {
  case 1:
    error = Lookup_PairPos1( gpi, &pp->ppf.ppf1, buffer,
			     first_pos, index,
			     pp->ValueFormat1, pp->ValueFormat2 );
    break;

  case 2:
    error = Lookup_PairPos2( gpi, &pp->ppf.ppf2, buffer, first_pos,
			     pp->ValueFormat1, pp->ValueFormat2 );
    break;

  default:
    return HB_Err_Invalid_GPOS_SubTable_Format;
  }

  /* adjusting the `next' glyph */

  if ( pp->ValueFormat2 )
    (buffer->in_pos)++;

  return error;
}


/* LookupType 3 */

/* CursivePosFormat1 */

static FT_Error  Load_CursivePos( HB_GPOS_SubTable* st,
				  FT_Stream        stream )
{
  FT_Error  error;
  FT_Memory memory = stream->memory;
  HB_CursivePos*  cp = &st->cursive;

  FT_UShort             n, m, count;
  FT_ULong              cur_offset, new_offset, base_offset;

  HB_EntryExitRecord*  eer;


  base_offset = FILE_Pos();

  if ( ACCESS_Frame( 4L ) )
    return error;

  cp->PosFormat = GET_UShort();
  new_offset    = GET_UShort() + base_offset;

  FORGET_Frame();

  cur_offset = FILE_Pos();
  if ( FILE_Seek( new_offset ) ||
       ( error = _HB_OPEN_Load_Coverage( &cp->Coverage, stream ) ) != FT_Err_Ok )
    return error;
  (void)FILE_Seek( cur_offset );

  if ( ACCESS_Frame( 2L ) )
    goto Fail2;

  count = cp->EntryExitCount = GET_UShort();

  FORGET_Frame();

  cp->EntryExitRecord = NULL;

  if ( ALLOC_ARRAY( cp->EntryExitRecord, count, HB_EntryExitRecord ) )
    goto Fail2;

  eer = cp->EntryExitRecord;

  for ( n = 0; n < count; n++ )
  {
    FT_ULong entry_offset;

    if ( ACCESS_Frame( 2L ) )
      return error;

    entry_offset = new_offset = GET_UShort();

    FORGET_Frame();

    if ( new_offset )
    {
      new_offset += base_offset;

      cur_offset = FILE_Pos();
      if ( FILE_Seek( new_offset ) ||
	   ( error = Load_Anchor( &eer[n].EntryAnchor,
				  stream ) ) != FT_Err_Ok )
	goto Fail1;
      (void)FILE_Seek( cur_offset );
    }
    else
      eer[n].EntryAnchor.PosFormat   = 0;

    if ( ACCESS_Frame( 2L ) )
      return error;

    new_offset = GET_UShort();

    FORGET_Frame();

    if ( new_offset )
    {
      new_offset += base_offset;

      cur_offset = FILE_Pos();
      if ( FILE_Seek( new_offset ) ||
	   ( error = Load_Anchor( &eer[n].ExitAnchor,
				  stream ) ) != FT_Err_Ok )
      {
	if ( entry_offset )
	  Free_Anchor( &eer[n].EntryAnchor, memory );
	goto Fail1;
      }
      (void)FILE_Seek( cur_offset );
    }
    else
      eer[n].ExitAnchor.PosFormat   = 0;
  }

  return FT_Err_Ok;

Fail1:
  for ( m = 0; m < n; m++ )
  {
    Free_Anchor( &eer[m].EntryAnchor, memory );
    Free_Anchor( &eer[m].ExitAnchor, memory );
  }

  FREE( eer );

Fail2:
  _HB_OPEN_Free_Coverage( &cp->Coverage, memory );
  return error;
}


static void  Free_CursivePos( HB_GPOS_SubTable* st,
			      FT_Memory        memory )
{
  FT_UShort             n, count;
  HB_CursivePos*  cp = &st->cursive;

  HB_EntryExitRecord*  eer;


  if ( cp->EntryExitRecord )
  {
    count = cp->EntryExitCount;
    eer   = cp->EntryExitRecord;

    for ( n = 0; n < count; n++ )
    {
      Free_Anchor( &eer[n].EntryAnchor, memory );
      Free_Anchor( &eer[n].ExitAnchor, memory );
    }

    FREE( eer );
  }

  _HB_OPEN_Free_Coverage( &cp->Coverage, memory );
}


static FT_Error  Lookup_CursivePos( GPOS_Instance*    gpi,
				    HB_GPOS_SubTable* st,
				    HB_Buffer        buffer,
				    FT_UShort         flags,
				    FT_UShort         context_length,
				    int               nesting_level )
{
  FT_UShort        index, property;
  FT_Error         error;
  HB_GPOSHeader*  gpos = gpi->gpos;
  HB_CursivePos*  cp = &st->cursive;

  HB_EntryExitRecord*  eer;
  FT_Pos                entry_x, entry_y;
  FT_Pos                exit_x, exit_y;

  FT_UNUSED(nesting_level);

  if ( context_length != 0xFFFF && context_length < 1 )
  {
    gpi->last = 0xFFFF;
    return HB_Err_Not_Covered;
  }

  /* Glyphs not having the right GDEF properties will be ignored, i.e.,
     gpi->last won't be reset (contrary to user defined properties). */

  if ( CHECK_Property( gpos->gdef, IN_CURITEM(), flags, &property ) )
    return error;

  /* We don't handle mark glyphs here.  According to Andrei, this isn't
     possible, but who knows...                                         */

  if ( property == HB_GDEF_MARK )
  {
    gpi->last = 0xFFFF;
    return HB_Err_Not_Covered;
  }

  error = _HB_OPEN_Coverage_Index( &cp->Coverage, IN_CURGLYPH(), &index );
  if ( error )
  {
    gpi->last = 0xFFFF;
    return error;
  }

  if ( index >= cp->EntryExitCount )
    return HB_Err_Invalid_GPOS_SubTable;

  eer = &cp->EntryExitRecord[index];

  /* Now comes the messiest part of the whole OpenType
     specification.  At first glance, cursive connections seem easy
     to understand, but there are pitfalls!  The reason is that
     the specs don't mention how to compute the advance values
     resp. glyph offsets.  I was told it would be an omission, to
     be fixed in the next OpenType version...  Again many thanks to
     Andrei Burago <andreib@microsoft.com> for clarifications.

     Consider the following example:

		      |  xadv1    |
		       +---------+
		       |         |
		 +-----+--+ 1    |
		 |     | .|      |
		 |    0+--+------+
		 |   2    |
		 |        |
		0+--------+
		|  xadv2   |

       glyph1: advance width = 12
	       anchor point = (3,1)

       glyph2: advance width = 11
	       anchor point = (9,4)

       LSB is 1 for both glyphs (so the boxes drawn above are glyph
       bboxes).  Writing direction is R2L; `0' denotes the glyph's
       coordinate origin.

     Now the surprising part: The advance width of the *left* glyph
     (resp. of the *bottom* glyph) will be modified, no matter
     whether the writing direction is L2R or R2L (resp. T2B or
     B2T)!  This assymetry is caused by the fact that the glyph's
     coordinate origin is always the lower left corner for all
     writing directions.

     Continuing the above example, we can compute the new
     (horizontal) advance width of glyph2 as

       9 - 3 = 6  ,

     and the new vertical offset of glyph2 as

       1 - 4 = -3  .


     Vertical writing direction is far more complicated:

     a) Assuming that we recompute the advance height of the lower glyph:

				  --
		       +---------+
	      --       |         |
		 +-----+--+ 1    | yadv1
		 |     | .|      |
	   yadv2 |    0+--+------+        -- BSB1  --
		 |   2    |       --      --        y_offset
		 |        |
   BSB2 --      0+--------+                        --
	--    --

       glyph1: advance height = 6
	       anchor point = (3,1)

       glyph2: advance height = 7
	       anchor point = (9,4)

       TSB is 1 for both glyphs; writing direction is T2B.


	 BSB1     = yadv1 - (TSB1 + ymax1)
	 BSB2     = yadv2 - (TSB2 + ymax2)
	 y_offset = y2 - y1

       vertical advance width of glyph2
	 = y_offset + BSB2 - BSB1
	 = (y2 - y1) + (yadv2 - (TSB2 + ymax2)) - (yadv1 - (TSB1 + ymax1))
	 = y2 - y1 + yadv2 - TSB2 - ymax2 - (yadv1 - TSB1 - ymax1)
	 = y2 - y1 + yadv2 - TSB2 - ymax2 - yadv1 + TSB1 + ymax1


     b) Assuming that we recompute the advance height of the upper glyph:

				  --      --
		       +---------+        -- TSB1
	--    --       |         |
   TSB2 --       +-----+--+ 1    | yadv1   ymax1
		 |     | .|      |
	   yadv2 |    0+--+------+        --       --
    ymax2        |   2    |       --                y_offset
		 |        |
	--      0+--------+                        --
	      --

       glyph1: advance height = 6
	       anchor point = (3,1)

       glyph2: advance height = 7
	       anchor point = (9,4)

       TSB is 1 for both glyphs; writing direction is T2B.

       y_offset = y2 - y1

       vertical advance width of glyph2
	 = TSB1 + ymax1 + y_offset - (TSB2 + ymax2)
	 = TSB1 + ymax1 + y2 - y1 - TSB2 - ymax2


     Comparing a) with b) shows that b) is easier to compute.  I'll wait
     for a reply from Andrei to see what should really be implemented...

     Since horizontal advance widths or vertical advance heights
     can be used alone but not together, no ambiguity occurs.        */

  if ( gpi->last == 0xFFFF )
    goto end;

  /* Get_Anchor() returns HB_Err_Not_Covered if there is no anchor
     table.                                                         */

  error = Get_Anchor( gpi, &eer->EntryAnchor, IN_CURGLYPH(),
		      &entry_x, &entry_y );
  if ( error == HB_Err_Not_Covered )
    goto end;
  if ( error )
    return error;

  if ( gpi->r2l )
  {
    POSITION( buffer->in_pos )->x_advance   = entry_x - gpi->anchor_x;
    POSITION( buffer->in_pos )->new_advance = TRUE;
  }
  else
  {
    POSITION( gpi->last )->x_advance   = gpi->anchor_x - entry_x;
    POSITION( gpi->last )->new_advance = TRUE;
  }

  if ( flags & HB_LOOKUP_FLAG_RIGHT_TO_LEFT )
  {
    POSITION( gpi->last )->cursive_chain = gpi->last - buffer->in_pos;
    POSITION( gpi->last )->y_pos = entry_y - gpi->anchor_y;
  }
  else
  {
    POSITION( buffer->in_pos )->cursive_chain = buffer->in_pos - gpi->last;
    POSITION( buffer->in_pos )->y_pos = gpi->anchor_y - entry_y;
  }

end:
  error = Get_Anchor( gpi, &eer->ExitAnchor, IN_CURGLYPH(),
		      &exit_x, &exit_y );
  if ( error == HB_Err_Not_Covered )
    gpi->last = 0xFFFF;
  else
  {
    gpi->last     = buffer->in_pos;
    gpi->anchor_x = exit_x;
    gpi->anchor_y = exit_y;
  }
  if ( error )
    return error;

  (buffer->in_pos)++;

  return FT_Err_Ok;
}


/* LookupType 4 */

/* BaseArray */

static FT_Error  Load_BaseArray( HB_BaseArray*  ba,
				 FT_UShort       num_classes,
				 FT_Stream       stream )
{
  FT_Error  error;
  FT_Memory memory = stream->memory;

  FT_UShort        m, n, k, count;
  FT_ULong         cur_offset, new_offset, base_offset;

  HB_BaseRecord*  br;
  HB_Anchor*      ban;


  base_offset = FILE_Pos();

  if ( ACCESS_Frame( 2L ) )
    return error;

  count = ba->BaseCount = GET_UShort();

  FORGET_Frame();

  ba->BaseRecord = NULL;

  if ( ALLOC_ARRAY( ba->BaseRecord, count, HB_BaseRecord ) )
    return error;

  br = ba->BaseRecord;

  for ( m = 0; m < count; m++ )
  {
    br[m].BaseAnchor = NULL;

    if ( ALLOC_ARRAY( br[m].BaseAnchor, num_classes, HB_Anchor ) )
      goto Fail;

    ban = br[m].BaseAnchor;

    for ( n = 0; n < num_classes; n++ )
    {
      if ( ACCESS_Frame( 2L ) )
	goto Fail0;

      new_offset = GET_UShort() + base_offset;

      FORGET_Frame();

      if (new_offset == base_offset) {
	/* Doulos SIL Regular is buggy and has zer offsets here.  Skip */
	ban[n].PosFormat = 0;
	continue;
      }

      cur_offset = FILE_Pos();
      if ( FILE_Seek( new_offset ) ||
	   ( error = Load_Anchor( &ban[n], stream ) ) != FT_Err_Ok )
	goto Fail0;
      (void)FILE_Seek( cur_offset );
    }

    continue;
  Fail0:
    for ( k = 0; k < n; k++ )
      Free_Anchor( &ban[k], memory );
    goto Fail;
  }

  return FT_Err_Ok;

Fail:
  for ( k = 0; k < m; k++ )
  {
    ban = br[k].BaseAnchor;

    for ( n = 0; n < num_classes; n++ )
      Free_Anchor( &ban[n], memory );

    FREE( ban );
  }

  FREE( br );
  return error;
}


static void  Free_BaseArray( HB_BaseArray*  ba,
			     FT_UShort       num_classes,
			     FT_Memory       memory )
{
  FT_UShort        m, n, count;

  HB_BaseRecord*  br;
  HB_Anchor*      ban;


  if ( ba->BaseRecord )
  {
    count = ba->BaseCount;
    br    = ba->BaseRecord;

    for ( m = 0; m < count; m++ )
    {
      ban = br[m].BaseAnchor;

      for ( n = 0; n < num_classes; n++ )
	Free_Anchor( &ban[n], memory );

      FREE( ban );
    }

    FREE( br );
  }
}


/* MarkBasePosFormat1 */

static FT_Error  Load_MarkBasePos( HB_GPOS_SubTable* st,
				   FT_Stream         stream )
{
  FT_Error  error;
  FT_Memory memory = stream->memory;
  HB_MarkBasePos* mbp = &st->markbase;

  FT_ULong  cur_offset, new_offset, base_offset;


  base_offset = FILE_Pos();

  if ( ACCESS_Frame( 4L ) )
    return error;

  mbp->PosFormat = GET_UShort();
  new_offset     = GET_UShort() + base_offset;

  FORGET_Frame();

  if (mbp->PosFormat != 1)
    return HB_Err_Invalid_SubTable_Format;

  cur_offset = FILE_Pos();
  if ( FILE_Seek( new_offset ) ||
       ( error = _HB_OPEN_Load_Coverage( &mbp->MarkCoverage, stream ) ) != FT_Err_Ok )
    return error;
  (void)FILE_Seek( cur_offset );

  if ( ACCESS_Frame( 2L ) )
    goto Fail3;

  new_offset = GET_UShort() + base_offset;

  FORGET_Frame();

  cur_offset = FILE_Pos();
  if ( FILE_Seek( new_offset ) ||
       ( error = _HB_OPEN_Load_Coverage( &mbp->BaseCoverage, stream ) ) != FT_Err_Ok )
    goto Fail3;
  (void)FILE_Seek( cur_offset );

  if ( ACCESS_Frame( 4L ) )
    goto Fail2;

  mbp->ClassCount = GET_UShort();
  new_offset      = GET_UShort() + base_offset;

  FORGET_Frame();

  cur_offset = FILE_Pos();
  if ( FILE_Seek( new_offset ) ||
       ( error = Load_MarkArray( &mbp->MarkArray, stream ) ) != FT_Err_Ok )
    goto Fail2;
  (void)FILE_Seek( cur_offset );

  if ( ACCESS_Frame( 2L ) )
    goto Fail1;

  new_offset = GET_UShort() + base_offset;

  FORGET_Frame();

  cur_offset = FILE_Pos();
  if ( FILE_Seek( new_offset ) ||
       ( error = Load_BaseArray( &mbp->BaseArray, mbp->ClassCount,
				 stream ) ) != FT_Err_Ok )
    goto Fail1;

  return FT_Err_Ok;

Fail1:
  Free_MarkArray( &mbp->MarkArray, memory );

Fail2:
  _HB_OPEN_Free_Coverage( &mbp->BaseCoverage, memory );

Fail3:
  _HB_OPEN_Free_Coverage( &mbp->MarkCoverage, memory );
  return error;
}


static void  Free_MarkBasePos( HB_GPOS_SubTable* st,
			       FT_Memory         memory )
{
  HB_MarkBasePos* mbp = &st->markbase;

  Free_BaseArray( &mbp->BaseArray, mbp->ClassCount, memory );
  Free_MarkArray( &mbp->MarkArray, memory );
  _HB_OPEN_Free_Coverage( &mbp->BaseCoverage, memory );
  _HB_OPEN_Free_Coverage( &mbp->MarkCoverage, memory );
}


static FT_Error  Lookup_MarkBasePos( GPOS_Instance*    gpi,
				     HB_GPOS_SubTable* st,
				     HB_Buffer        buffer,
				     FT_UShort         flags,
				     FT_UShort         context_length,
				     int               nesting_level )
{
  FT_UShort        i, j, mark_index, base_index, property, class;
  FT_Pos           x_mark_value, y_mark_value, x_base_value, y_base_value;
  FT_Error         error;
  HB_GPOSHeader*  gpos = gpi->gpos;
  HB_MarkBasePos* mbp = &st->markbase;

  HB_MarkArray*   ma;
  HB_BaseArray*   ba;
  HB_BaseRecord*  br;
  HB_Anchor*      mark_anchor;
  HB_Anchor*      base_anchor;

  HB_Position     o;

  FT_UNUSED(nesting_level);

  if ( context_length != 0xFFFF && context_length < 1 )
    return HB_Err_Not_Covered;

  if ( flags & HB_LOOKUP_FLAG_IGNORE_BASE_GLYPHS )
    return HB_Err_Not_Covered;

  if ( CHECK_Property( gpos->gdef, IN_CURITEM(),
		       flags, &property ) )
    return error;

  error = _HB_OPEN_Coverage_Index( &mbp->MarkCoverage, IN_CURGLYPH(),
			  &mark_index );
  if ( error )
    return error;

  /* now we search backwards for a non-mark glyph */

  i = 1;
  j = buffer->in_pos - 1;

  while ( i <= buffer->in_pos )
  {
    error = HB_GDEF_Get_Glyph_Property( gpos->gdef, IN_GLYPH( j ),
					&property );
    if ( error )
      return error;

    if ( !( property == HB_GDEF_MARK || property & HB_LOOKUP_FLAG_IGNORE_SPECIAL_MARKS ) )
      break;

    i++;
    j--;
  }

  /* The following assertion is too strong -- at least for mangal.ttf. */
#if 0
  if ( property != HB_GDEF_BASE_GLYPH )
    return HB_Err_Not_Covered;
#endif

  if ( i > buffer->in_pos )
    return HB_Err_Not_Covered;

  error = _HB_OPEN_Coverage_Index( &mbp->BaseCoverage, IN_GLYPH( j ),
			  &base_index );
  if ( error )
    return error;

  ma = &mbp->MarkArray;

  if ( mark_index >= ma->MarkCount )
    return HB_Err_Invalid_GPOS_SubTable;

  class       = ma->MarkRecord[mark_index].Class;
  mark_anchor = &ma->MarkRecord[mark_index].MarkAnchor;

  if ( class >= mbp->ClassCount )
    return HB_Err_Invalid_GPOS_SubTable;

  ba = &mbp->BaseArray;

  if ( base_index >= ba->BaseCount )
    return HB_Err_Invalid_GPOS_SubTable;

  br          = &ba->BaseRecord[base_index];
  base_anchor = &br->BaseAnchor[class];

  error = Get_Anchor( gpi, mark_anchor, IN_CURGLYPH(),
		      &x_mark_value, &y_mark_value );
  if ( error )
    return error;

  error = Get_Anchor( gpi, base_anchor, IN_GLYPH( j ),
		      &x_base_value, &y_base_value );
  if ( error )
    return error;

  /* anchor points are not cumulative */

  o = POSITION( buffer->in_pos );

  o->x_pos     = x_base_value - x_mark_value;
  o->y_pos     = y_base_value - y_mark_value;
  o->x_advance = 0;
  o->y_advance = 0;
  o->back      = i;

  (buffer->in_pos)++;

  return FT_Err_Ok;
}


/* LookupType 5 */

/* LigatureAttach */

static FT_Error  Load_LigatureAttach( HB_LigatureAttach*  lat,
				      FT_UShort            num_classes,
				      FT_Stream            stream )
{
  FT_Error  error;
  FT_Memory memory = stream->memory;

  FT_UShort             m, n, k, count;
  FT_ULong              cur_offset, new_offset, base_offset;

  HB_ComponentRecord*  cr;
  HB_Anchor*           lan;


  base_offset = FILE_Pos();

  if ( ACCESS_Frame( 2L ) )
    return error;

  count = lat->ComponentCount = GET_UShort();

  FORGET_Frame();

  lat->ComponentRecord = NULL;

  if ( ALLOC_ARRAY( lat->ComponentRecord, count, HB_ComponentRecord ) )
    return error;

  cr = lat->ComponentRecord;

  for ( m = 0; m < count; m++ )
  {
    cr[m].LigatureAnchor = NULL;

    if ( ALLOC_ARRAY( cr[m].LigatureAnchor, num_classes, HB_Anchor ) )
      goto Fail;

    lan = cr[m].LigatureAnchor;

    for ( n = 0; n < num_classes; n++ )
    {
      if ( ACCESS_Frame( 2L ) )
	goto Fail0;

      new_offset = GET_UShort();

      FORGET_Frame();

      if ( new_offset )
      {
	new_offset += base_offset;

	cur_offset = FILE_Pos();
	if ( FILE_Seek( new_offset ) ||
	     ( error = Load_Anchor( &lan[n], stream ) ) != FT_Err_Ok )
	  goto Fail0;
	(void)FILE_Seek( cur_offset );
      }
      else
	lan[n].PosFormat = 0;
    }

    continue;
  Fail0:
    for ( k = 0; k < n; k++ )
      Free_Anchor( &lan[k], memory );
    goto Fail;
  }

  return FT_Err_Ok;

Fail:
  for ( k = 0; k < m; k++ )
  {
    lan = cr[k].LigatureAnchor;

    for ( n = 0; n < num_classes; n++ )
      Free_Anchor( &lan[n], memory );

    FREE( lan );
  }

  FREE( cr );
  return error;
}


static void  Free_LigatureAttach( HB_LigatureAttach*  lat,
				  FT_UShort            num_classes,
				  FT_Memory            memory )
{
  FT_UShort        m, n, count;

  HB_ComponentRecord*  cr;
  HB_Anchor*           lan;


  if ( lat->ComponentRecord )
  {
    count = lat->ComponentCount;
    cr    = lat->ComponentRecord;

    for ( m = 0; m < count; m++ )
    {
      lan = cr[m].LigatureAnchor;

      for ( n = 0; n < num_classes; n++ )
	Free_Anchor( &lan[n], memory );

      FREE( lan );
    }

    FREE( cr );
  }
}


/* LigatureArray */

static FT_Error  Load_LigatureArray( HB_LigatureArray*  la,
				     FT_UShort           num_classes,
				     FT_Stream           stream )
{
  FT_Error  error;
  FT_Memory memory = stream->memory;

  FT_UShort            n, m, count;
  FT_ULong             cur_offset, new_offset, base_offset;

  HB_LigatureAttach*  lat;


  base_offset = FILE_Pos();

  if ( ACCESS_Frame( 2L ) )
    return error;

  count = la->LigatureCount = GET_UShort();

  FORGET_Frame();

  la->LigatureAttach = NULL;

  if ( ALLOC_ARRAY( la->LigatureAttach, count, HB_LigatureAttach ) )
    return error;

  lat = la->LigatureAttach;

  for ( n = 0; n < count; n++ )
  {
    if ( ACCESS_Frame( 2L ) )
      goto Fail;

    new_offset = GET_UShort() + base_offset;

    FORGET_Frame();

    cur_offset = FILE_Pos();
    if ( FILE_Seek( new_offset ) ||
	 ( error = Load_LigatureAttach( &lat[n], num_classes,
					stream ) ) != FT_Err_Ok )
      goto Fail;
    (void)FILE_Seek( cur_offset );
  }

  return FT_Err_Ok;

Fail:
  for ( m = 0; m < n; m++ )
    Free_LigatureAttach( &lat[m], num_classes, memory );

  FREE( lat );
  return error;
}


static void  Free_LigatureArray( HB_LigatureArray*  la,
				 FT_UShort           num_classes,
				 FT_Memory           memory )
{
  FT_UShort            n, count;

  HB_LigatureAttach*  lat;


  if ( la->LigatureAttach )
  {
    count = la->LigatureCount;
    lat   = la->LigatureAttach;

    for ( n = 0; n < count; n++ )
      Free_LigatureAttach( &lat[n], num_classes, memory );

    FREE( lat );
  }
}


/* MarkLigPosFormat1 */

static FT_Error  Load_MarkLigPos( HB_GPOS_SubTable* st,
				  FT_Stream        stream )
{
  FT_Error  error;
  FT_Memory memory = stream->memory;
  HB_MarkLigPos*  mlp = &st->marklig;

  FT_ULong  cur_offset, new_offset, base_offset;


  base_offset = FILE_Pos();

  if ( ACCESS_Frame( 4L ) )
    return error;

  mlp->PosFormat = GET_UShort();
  new_offset     = GET_UShort() + base_offset;

  FORGET_Frame();

  cur_offset = FILE_Pos();
  if ( FILE_Seek( new_offset ) ||
       ( error = _HB_OPEN_Load_Coverage( &mlp->MarkCoverage, stream ) ) != FT_Err_Ok )
    return error;
  (void)FILE_Seek( cur_offset );

  if ( ACCESS_Frame( 2L ) )
    goto Fail3;

  new_offset = GET_UShort() + base_offset;

  FORGET_Frame();

  cur_offset = FILE_Pos();
  if ( FILE_Seek( new_offset ) ||
       ( error = _HB_OPEN_Load_Coverage( &mlp->LigatureCoverage,
				stream ) ) != FT_Err_Ok )
    goto Fail3;
  (void)FILE_Seek( cur_offset );

  if ( ACCESS_Frame( 4L ) )
    goto Fail2;

  mlp->ClassCount = GET_UShort();
  new_offset      = GET_UShort() + base_offset;

  FORGET_Frame();

  cur_offset = FILE_Pos();
  if ( FILE_Seek( new_offset ) ||
       ( error = Load_MarkArray( &mlp->MarkArray, stream ) ) != FT_Err_Ok )
    goto Fail2;
  (void)FILE_Seek( cur_offset );

  if ( ACCESS_Frame( 2L ) )
    goto Fail1;

  new_offset = GET_UShort() + base_offset;

  FORGET_Frame();

  cur_offset = FILE_Pos();
  if ( FILE_Seek( new_offset ) ||
       ( error = Load_LigatureArray( &mlp->LigatureArray, mlp->ClassCount,
				     stream ) ) != FT_Err_Ok )
    goto Fail1;

  return FT_Err_Ok;

Fail1:
  Free_MarkArray( &mlp->MarkArray, memory );

Fail2:
  _HB_OPEN_Free_Coverage( &mlp->LigatureCoverage, memory );

Fail3:
  _HB_OPEN_Free_Coverage( &mlp->MarkCoverage, memory );
  return error;
}


static void  Free_MarkLigPos( HB_GPOS_SubTable* st,
			      FT_Memory        memory)
{
  HB_MarkLigPos*  mlp = &st->marklig;

  Free_LigatureArray( &mlp->LigatureArray, mlp->ClassCount, memory );
  Free_MarkArray( &mlp->MarkArray, memory );
  _HB_OPEN_Free_Coverage( &mlp->LigatureCoverage, memory );
  _HB_OPEN_Free_Coverage( &mlp->MarkCoverage, memory );
}


static FT_Error  Lookup_MarkLigPos( GPOS_Instance*    gpi,
				    HB_GPOS_SubTable* st,
				    HB_Buffer        buffer,
				    FT_UShort         flags,
				    FT_UShort         context_length,
				    int               nesting_level )
{
  FT_UShort        i, j, mark_index, lig_index, property, class;
  FT_UShort        mark_glyph;
  FT_Pos           x_mark_value, y_mark_value, x_lig_value, y_lig_value;
  FT_Error         error;
  HB_GPOSHeader*  gpos = gpi->gpos;
  HB_MarkLigPos*  mlp = &st->marklig;

  HB_MarkArray*        ma;
  HB_LigatureArray*    la;
  HB_LigatureAttach*   lat;
  HB_ComponentRecord*  cr;
  FT_UShort             comp_index;
  HB_Anchor*           mark_anchor;
  HB_Anchor*           lig_anchor;

  HB_Position    o;

  FT_UNUSED(nesting_level);

  if ( context_length != 0xFFFF && context_length < 1 )
    return HB_Err_Not_Covered;

  if ( flags & HB_LOOKUP_FLAG_IGNORE_LIGATURES )
    return HB_Err_Not_Covered;

  mark_glyph = IN_CURGLYPH();

  if ( CHECK_Property( gpos->gdef, IN_CURITEM(), flags, &property ) )
    return error;

  error = _HB_OPEN_Coverage_Index( &mlp->MarkCoverage, mark_glyph, &mark_index );
  if ( error )
    return error;

  /* now we search backwards for a non-mark glyph */

  i = 1;
  j = buffer->in_pos - 1;

  while ( i <= buffer->in_pos )
  {
    error = HB_GDEF_Get_Glyph_Property( gpos->gdef, IN_GLYPH( j ),
					&property );
    if ( error )
      return error;

    if ( !( property == HB_GDEF_MARK || property & HB_LOOKUP_FLAG_IGNORE_SPECIAL_MARKS ) )
      break;

    i++;
    j--;
  }

  /* Similar to Lookup_MarkBasePos(), I suspect that this assertion is
     too strong, thus it is commented out.                             */
#if 0
  if ( property != HB_GDEF_LIGATURE )
    return HB_Err_Not_Covered;
#endif

  if ( i > buffer->in_pos )
    return HB_Err_Not_Covered;

  error = _HB_OPEN_Coverage_Index( &mlp->LigatureCoverage, IN_GLYPH( j ),
			  &lig_index );
  if ( error )
    return error;

  ma = &mlp->MarkArray;

  if ( mark_index >= ma->MarkCount )
    return HB_Err_Invalid_GPOS_SubTable;

  class       = ma->MarkRecord[mark_index].Class;
  mark_anchor = &ma->MarkRecord[mark_index].MarkAnchor;

  if ( class >= mlp->ClassCount )
    return HB_Err_Invalid_GPOS_SubTable;

  la = &mlp->LigatureArray;

  if ( lig_index >= la->LigatureCount )
    return HB_Err_Invalid_GPOS_SubTable;

  lat = &la->LigatureAttach[lig_index];

  /* We must now check whether the ligature ID of the current mark glyph
     is identical to the ligature ID of the found ligature.  If yes, we
     can directly use the component index.  If not, we attach the mark
     glyph to the last component of the ligature.                        */

  if ( IN_LIGID( j ) == IN_LIGID( buffer->in_pos) )
  {
    comp_index = IN_COMPONENT( buffer->in_pos );
    if ( comp_index >= lat->ComponentCount )
      return HB_Err_Not_Covered;
  }
  else
    comp_index = lat->ComponentCount - 1;

  cr         = &lat->ComponentRecord[comp_index];
  lig_anchor = &cr->LigatureAnchor[class];

  error = Get_Anchor( gpi, mark_anchor, IN_CURGLYPH(),
		      &x_mark_value, &y_mark_value );
  if ( error )
    return error;
  error = Get_Anchor( gpi, lig_anchor, IN_GLYPH( j ),
		      &x_lig_value, &y_lig_value );
  if ( error )
    return error;

  /* anchor points are not cumulative */

  o = POSITION( buffer->in_pos );

  o->x_pos     = x_lig_value - x_mark_value;
  o->y_pos     = y_lig_value - y_mark_value;
  o->x_advance = 0;
  o->y_advance = 0;
  o->back      = i;

  (buffer->in_pos)++;

  return FT_Err_Ok;
}


/* LookupType 6 */

/* Mark2Array */

static FT_Error  Load_Mark2Array( HB_Mark2Array*  m2a,
				  FT_UShort        num_classes,
				  FT_Stream        stream )
{
  FT_Error  error;
  FT_Memory memory = stream->memory;

  FT_UShort         k, m, n, count;
  FT_ULong          cur_offset, new_offset, base_offset;

  HB_Mark2Record*  m2r;
  HB_Anchor*       m2an;


  base_offset = FILE_Pos();

  if ( ACCESS_Frame( 2L ) )
    return error;

  count = m2a->Mark2Count = GET_UShort();

  FORGET_Frame();

  m2a->Mark2Record = NULL;

  if ( ALLOC_ARRAY( m2a->Mark2Record, count, HB_Mark2Record ) )
    return error;

  m2r = m2a->Mark2Record;

  for ( m = 0; m < count; m++ )
  {
    m2r[m].Mark2Anchor = NULL;

    if ( ALLOC_ARRAY( m2r[m].Mark2Anchor, num_classes, HB_Anchor ) )
      goto Fail;

    m2an = m2r[m].Mark2Anchor;

    for ( n = 0; n < num_classes; n++ )
    {
      if ( ACCESS_Frame( 2L ) )
	goto Fail0;

      new_offset = GET_UShort() + base_offset;

      FORGET_Frame();

      cur_offset = FILE_Pos();
      if ( FILE_Seek( new_offset ) ||
	   ( error = Load_Anchor( &m2an[n], stream ) ) != FT_Err_Ok )
	goto Fail0;
      (void)FILE_Seek( cur_offset );
    }

    continue;
  Fail0:
    for ( k = 0; k < n; k++ )
      Free_Anchor( &m2an[k], memory );
    goto Fail;
  }

  return FT_Err_Ok;

Fail:
  for ( k = 0; k < m; k++ )
  {
    m2an = m2r[k].Mark2Anchor;

    for ( n = 0; n < num_classes; n++ )
      Free_Anchor( &m2an[n], memory );

    FREE( m2an );
  }

  FREE( m2r );
  return error;
}


static void  Free_Mark2Array( HB_Mark2Array*  m2a,
			      FT_UShort        num_classes,
			      FT_Memory        memory )
{
  FT_UShort         m, n, count;

  HB_Mark2Record*  m2r;
  HB_Anchor*       m2an;


  if ( m2a->Mark2Record )
  {
    count = m2a->Mark2Count;
    m2r   = m2a->Mark2Record;

    for ( m = 0; m < count; m++ )
    {
      m2an = m2r[m].Mark2Anchor;

      for ( n = 0; n < num_classes; n++ )
	Free_Anchor( &m2an[n], memory );

      FREE( m2an );
    }

    FREE( m2r );
  }
}


/* MarkMarkPosFormat1 */

static FT_Error  Load_MarkMarkPos( HB_GPOS_SubTable* st,
				   FT_Stream         stream )
{
  FT_Error  error;
  FT_Memory memory = stream->memory;
  HB_MarkMarkPos* mmp = &st->markmark;

  FT_ULong  cur_offset, new_offset, base_offset;


  base_offset = FILE_Pos();

  if ( ACCESS_Frame( 4L ) )
    return error;

  mmp->PosFormat = GET_UShort();
  new_offset     = GET_UShort() + base_offset;

  FORGET_Frame();

  cur_offset = FILE_Pos();
  if ( FILE_Seek( new_offset ) ||
       ( error = _HB_OPEN_Load_Coverage( &mmp->Mark1Coverage,
				stream ) ) != FT_Err_Ok )
    return error;
  (void)FILE_Seek( cur_offset );

  if ( ACCESS_Frame( 2L ) )
    goto Fail3;

  new_offset = GET_UShort() + base_offset;

  FORGET_Frame();

  cur_offset = FILE_Pos();
  if ( FILE_Seek( new_offset ) ||
       ( error = _HB_OPEN_Load_Coverage( &mmp->Mark2Coverage,
				stream ) ) != FT_Err_Ok )
    goto Fail3;
  (void)FILE_Seek( cur_offset );

  if ( ACCESS_Frame( 4L ) )
    goto Fail2;

  mmp->ClassCount = GET_UShort();
  new_offset      = GET_UShort() + base_offset;

  FORGET_Frame();

  cur_offset = FILE_Pos();
  if ( FILE_Seek( new_offset ) ||
       ( error = Load_MarkArray( &mmp->Mark1Array, stream ) ) != FT_Err_Ok )
    goto Fail2;
  (void)FILE_Seek( cur_offset );

  if ( ACCESS_Frame( 2L ) )
    goto Fail1;

  new_offset = GET_UShort() + base_offset;

  FORGET_Frame();

  cur_offset = FILE_Pos();
  if ( FILE_Seek( new_offset ) ||
       ( error = Load_Mark2Array( &mmp->Mark2Array, mmp->ClassCount,
				  stream ) ) != FT_Err_Ok )
    goto Fail1;

  return FT_Err_Ok;

Fail1:
  Free_MarkArray( &mmp->Mark1Array, memory );

Fail2:
  _HB_OPEN_Free_Coverage( &mmp->Mark2Coverage, memory );

Fail3:
  _HB_OPEN_Free_Coverage( &mmp->Mark1Coverage, memory );
  return error;
}


static void  Free_MarkMarkPos( HB_GPOS_SubTable* st,
			       FT_Memory         memory)
{
  HB_MarkMarkPos* mmp = &st->markmark;

  Free_Mark2Array( &mmp->Mark2Array, mmp->ClassCount, memory );
  Free_MarkArray( &mmp->Mark1Array, memory );
  _HB_OPEN_Free_Coverage( &mmp->Mark2Coverage, memory );
  _HB_OPEN_Free_Coverage( &mmp->Mark1Coverage, memory );
}


static FT_Error  Lookup_MarkMarkPos( GPOS_Instance*    gpi,
				     HB_GPOS_SubTable* st,
				     HB_Buffer        buffer,
				     FT_UShort         flags,
				     FT_UShort         context_length,
				     int               nesting_level )
{
  FT_UShort        j, mark1_index, mark2_index, property, class;
  FT_Pos           x_mark1_value, y_mark1_value,
		   x_mark2_value, y_mark2_value;
  FT_Error         error;
  HB_GPOSHeader*  gpos = gpi->gpos;
  HB_MarkMarkPos* mmp = &st->markmark;

  HB_MarkArray*    ma1;
  HB_Mark2Array*   ma2;
  HB_Mark2Record*  m2r;
  HB_Anchor*       mark1_anchor;
  HB_Anchor*       mark2_anchor;

  HB_Position    o;

  FT_UNUSED(nesting_level);

  if ( context_length != 0xFFFF && context_length < 1 )
    return HB_Err_Not_Covered;

  if ( flags & HB_LOOKUP_FLAG_IGNORE_MARKS )
    return HB_Err_Not_Covered;

  if ( CHECK_Property( gpos->gdef, IN_CURITEM(),
		       flags, &property ) )
    return error;

  error = _HB_OPEN_Coverage_Index( &mmp->Mark1Coverage, IN_CURGLYPH(),
			  &mark1_index );
  if ( error )
    return error;

  /* now we check the preceding glyph whether it is a suitable
     mark glyph                                                */

  if ( buffer->in_pos == 0 )
    return HB_Err_Not_Covered;

  j = buffer->in_pos - 1;
  error = HB_GDEF_Get_Glyph_Property( gpos->gdef, IN_GLYPH( j ),
				      &property );
  if ( error )
    return error;

  if ( flags & HB_LOOKUP_FLAG_IGNORE_SPECIAL_MARKS )
  {
    if ( property != (flags & 0xFF00) )
      return HB_Err_Not_Covered;
  }
  else
  {
    if ( property != HB_GDEF_MARK )
      return HB_Err_Not_Covered;
  }

  error = _HB_OPEN_Coverage_Index( &mmp->Mark2Coverage, IN_GLYPH( j ),
			  &mark2_index );
  if ( error )
    return error;

  ma1 = &mmp->Mark1Array;

  if ( mark1_index >= ma1->MarkCount )
    return HB_Err_Invalid_GPOS_SubTable;

  class        = ma1->MarkRecord[mark1_index].Class;
  mark1_anchor = &ma1->MarkRecord[mark1_index].MarkAnchor;

  if ( class >= mmp->ClassCount )
    return HB_Err_Invalid_GPOS_SubTable;

  ma2 = &mmp->Mark2Array;

  if ( mark2_index >= ma2->Mark2Count )
    return HB_Err_Invalid_GPOS_SubTable;

  m2r          = &ma2->Mark2Record[mark2_index];
  mark2_anchor = &m2r->Mark2Anchor[class];

  error = Get_Anchor( gpi, mark1_anchor, IN_CURGLYPH(),
		      &x_mark1_value, &y_mark1_value );
  if ( error )
    return error;
  error = Get_Anchor( gpi, mark2_anchor, IN_GLYPH( j ),
		      &x_mark2_value, &y_mark2_value );
  if ( error )
    return error;

  /* anchor points are not cumulative */

  o = POSITION( buffer->in_pos );

  o->x_pos     = x_mark2_value - x_mark1_value;
  o->y_pos     = y_mark2_value - y_mark1_value;
  o->x_advance = 0;
  o->y_advance = 0;
  o->back      = 1;

  (buffer->in_pos)++;

  return FT_Err_Ok;
}


/* Do the actual positioning for a context positioning (either format
   7 or 8).  This is only called after we've determined that the stream
   matches the subrule.                                                 */

static FT_Error  Do_ContextPos( GPOS_Instance*        gpi,
				FT_UShort             GlyphCount,
				FT_UShort             PosCount,
				HB_PosLookupRecord*  pos,
				HB_Buffer            buffer,
				int                   nesting_level )
{
  FT_Error  error;
  FT_UShort i, old_pos;


  i = 0;

  while ( i < GlyphCount )
  {
    if ( PosCount && i == pos->SequenceIndex )
    {
      old_pos = buffer->in_pos;

      /* Do a positioning */

      error = GPOS_Do_Glyph_Lookup( gpi, pos->LookupListIndex, buffer,
				    GlyphCount, nesting_level );

      if ( error )
	return error;

      pos++;
      PosCount--;
      i += buffer->in_pos - old_pos;
    }
    else
    {
      i++;
      (buffer->in_pos)++;
    }
  }

  return FT_Err_Ok;
}


/* LookupType 7 */

/* PosRule */

static FT_Error  Load_PosRule( HB_PosRule*  pr,
			       FT_Stream     stream )
{
  FT_Error  error;
  FT_Memory memory = stream->memory;

  FT_UShort             n, count;
  FT_UShort*            i;

  HB_PosLookupRecord*  plr;


  if ( ACCESS_Frame( 4L ) )
    return error;

  pr->GlyphCount = GET_UShort();
  pr->PosCount   = GET_UShort();

  FORGET_Frame();

  pr->Input = NULL;

  count = pr->GlyphCount - 1;         /* only GlyphCount - 1 elements */

  if ( ALLOC_ARRAY( pr->Input, count, FT_UShort ) )
    return error;

  i = pr->Input;

  if ( ACCESS_Frame( count * 2L ) )
    goto Fail2;

  for ( n = 0; n < count; n++ )
    i[n] = GET_UShort();

  FORGET_Frame();

  pr->PosLookupRecord = NULL;

  count = pr->PosCount;

  if ( ALLOC_ARRAY( pr->PosLookupRecord, count, HB_PosLookupRecord ) )
    goto Fail2;

  plr = pr->PosLookupRecord;

  if ( ACCESS_Frame( count * 4L ) )
    goto Fail1;

  for ( n = 0; n < count; n++ )
  {
    plr[n].SequenceIndex   = GET_UShort();
    plr[n].LookupListIndex = GET_UShort();
  }

  FORGET_Frame();

  return FT_Err_Ok;

Fail1:
  FREE( plr );

Fail2:
  FREE( i );
  return error;
}


static void  Free_PosRule( HB_PosRule*  pr,
			   FT_Memory     memory )
{
  FREE( pr->PosLookupRecord );
  FREE( pr->Input );
}


/* PosRuleSet */

static FT_Error  Load_PosRuleSet( HB_PosRuleSet*  prs,
				  FT_Stream        stream )
{
  FT_Error  error;
  FT_Memory memory = stream->memory;

  FT_UShort     n, m, count;
  FT_ULong      cur_offset, new_offset, base_offset;

  HB_PosRule*  pr;


  base_offset = FILE_Pos();

  if ( ACCESS_Frame( 2L ) )
    return error;

  count = prs->PosRuleCount = GET_UShort();

  FORGET_Frame();

  prs->PosRule = NULL;

  if ( ALLOC_ARRAY( prs->PosRule, count, HB_PosRule ) )
    return error;

  pr = prs->PosRule;

  for ( n = 0; n < count; n++ )
  {
    if ( ACCESS_Frame( 2L ) )
      goto Fail;

    new_offset = GET_UShort() + base_offset;

    FORGET_Frame();

    cur_offset = FILE_Pos();
    if ( FILE_Seek( new_offset ) ||
	 ( error = Load_PosRule( &pr[n], stream ) ) != FT_Err_Ok )
      goto Fail;
    (void)FILE_Seek( cur_offset );
  }

  return FT_Err_Ok;

Fail:
  for ( m = 0; m < n; m++ )
    Free_PosRule( &pr[m], memory );

  FREE( pr );
  return error;
}


static void  Free_PosRuleSet( HB_PosRuleSet*  prs,
			      FT_Memory        memory )
{
  FT_UShort     n, count;

  HB_PosRule*  pr;


  if ( prs->PosRule )
  {
    count = prs->PosRuleCount;
    pr    = prs->PosRule;

    for ( n = 0; n < count; n++ )
      Free_PosRule( &pr[n], memory );

    FREE( pr );
  }
}


/* ContextPosFormat1 */

static FT_Error  Load_ContextPos1( HB_ContextPosFormat1*  cpf1,
				   FT_Stream               stream )
{
  FT_Error  error;
  FT_Memory memory = stream->memory;

  FT_UShort        n, m, count;
  FT_ULong         cur_offset, new_offset, base_offset;

  HB_PosRuleSet*  prs;


  base_offset = FILE_Pos() - 2L;

  if ( ACCESS_Frame( 2L ) )
    return error;

  new_offset = GET_UShort() + base_offset;

  FORGET_Frame();

  cur_offset = FILE_Pos();
  if ( FILE_Seek( new_offset ) ||
       ( error = _HB_OPEN_Load_Coverage( &cpf1->Coverage, stream ) ) != FT_Err_Ok )
    return error;
  (void)FILE_Seek( cur_offset );

  if ( ACCESS_Frame( 2L ) )
    goto Fail2;

  count = cpf1->PosRuleSetCount = GET_UShort();

  FORGET_Frame();

  cpf1->PosRuleSet = NULL;

  if ( ALLOC_ARRAY( cpf1->PosRuleSet, count, HB_PosRuleSet ) )
    goto Fail2;

  prs = cpf1->PosRuleSet;

  for ( n = 0; n < count; n++ )
  {
    if ( ACCESS_Frame( 2L ) )
      goto Fail1;

    new_offset = GET_UShort() + base_offset;

    FORGET_Frame();

    cur_offset = FILE_Pos();
    if ( FILE_Seek( new_offset ) ||
	 ( error = Load_PosRuleSet( &prs[n], stream ) ) != FT_Err_Ok )
      goto Fail1;
    (void)FILE_Seek( cur_offset );
  }

  return FT_Err_Ok;

Fail1:
  for ( m = 0; m < n; m++ )
    Free_PosRuleSet( &prs[m], memory );

  FREE( prs );

Fail2:
  _HB_OPEN_Free_Coverage( &cpf1->Coverage, memory );
  return error;
}


static void  Free_ContextPos1( HB_ContextPosFormat1*  cpf1,
			    FT_Memory               memory )
{
  FT_UShort        n, count;

  HB_PosRuleSet*  prs;


  if ( cpf1->PosRuleSet )
  {
    count = cpf1->PosRuleSetCount;
    prs   = cpf1->PosRuleSet;

    for ( n = 0; n < count; n++ )
      Free_PosRuleSet( &prs[n], memory );

    FREE( prs );
  }

  _HB_OPEN_Free_Coverage( &cpf1->Coverage, memory );
}


/* PosClassRule */

static FT_Error  Load_PosClassRule( HB_ContextPosFormat2*  cpf2,
				    HB_PosClassRule*       pcr,
				    FT_Stream               stream )
{
  FT_Error  error;
  FT_Memory memory = stream->memory;

  FT_UShort             n, count;

  FT_UShort*            c;
  HB_PosLookupRecord*  plr;
  FT_Bool*              d;


  if ( ACCESS_Frame( 4L ) )
    return error;

  pcr->GlyphCount = GET_UShort();
  pcr->PosCount   = GET_UShort();

  FORGET_Frame();

  if ( pcr->GlyphCount > cpf2->MaxContextLength )
    cpf2->MaxContextLength = pcr->GlyphCount;

  pcr->Class = NULL;

  count = pcr->GlyphCount - 1;        /* only GlyphCount - 1 elements */

  if ( ALLOC_ARRAY( pcr->Class, count, FT_UShort ) )
    return error;

  c = pcr->Class;
  d = cpf2->ClassDef.Defined;

  if ( ACCESS_Frame( count * 2L ) )
    goto Fail2;

  for ( n = 0; n < count; n++ )
  {
    c[n] = GET_UShort();

    /* We check whether the specific class is used at all.  If not,
       class 0 is used instead.                                     */

    if ( !d[c[n]] )
      c[n] = 0;
  }

  FORGET_Frame();

  pcr->PosLookupRecord = NULL;

  count = pcr->PosCount;

  if ( ALLOC_ARRAY( pcr->PosLookupRecord, count, HB_PosLookupRecord ) )
    goto Fail2;

  plr = pcr->PosLookupRecord;

  if ( ACCESS_Frame( count * 4L ) )
    goto Fail1;

  for ( n = 0; n < count; n++ )
  {
    plr[n].SequenceIndex   = GET_UShort();
    plr[n].LookupListIndex = GET_UShort();
  }

  FORGET_Frame();

  return FT_Err_Ok;

Fail1:
  FREE( plr );

Fail2:
  FREE( c );
  return error;
}


static void  Free_PosClassRule( HB_PosClassRule*  pcr,
				FT_Memory          memory )
{
  FREE( pcr->PosLookupRecord );
  FREE( pcr->Class );
}


/* PosClassSet */

static FT_Error  Load_PosClassSet( HB_ContextPosFormat2*  cpf2,
				   HB_PosClassSet*        pcs,
				   FT_Stream               stream )
{
  FT_Error  error;
  FT_Memory memory = stream->memory;

  FT_UShort          n, m, count;
  FT_ULong           cur_offset, new_offset, base_offset;

  HB_PosClassRule*  pcr;


  base_offset = FILE_Pos();

  if ( ACCESS_Frame( 2L ) )
    return error;

  count = pcs->PosClassRuleCount = GET_UShort();

  FORGET_Frame();

  pcs->PosClassRule = NULL;

  if ( ALLOC_ARRAY( pcs->PosClassRule, count, HB_PosClassRule ) )
    return error;

  pcr = pcs->PosClassRule;

  for ( n = 0; n < count; n++ )
  {
    if ( ACCESS_Frame( 2L ) )
      goto Fail;

    new_offset = GET_UShort() + base_offset;

    FORGET_Frame();

    cur_offset = FILE_Pos();
    if ( FILE_Seek( new_offset ) ||
	 ( error = Load_PosClassRule( cpf2, &pcr[n],
				      stream ) ) != FT_Err_Ok )
      goto Fail;
    (void)FILE_Seek( cur_offset );
  }

  return FT_Err_Ok;

Fail:
  for ( m = 0; m < n; m++ )
    Free_PosClassRule( &pcr[m], memory );

  FREE( pcr );
  return error;
}


static void  Free_PosClassSet( HB_PosClassSet*  pcs,
			       FT_Memory         memory )
{
  FT_UShort          n, count;

  HB_PosClassRule*  pcr;


  if ( pcs->PosClassRule )
  {
    count = pcs->PosClassRuleCount;
    pcr   = pcs->PosClassRule;

    for ( n = 0; n < count; n++ )
      Free_PosClassRule( &pcr[n], memory );

    FREE( pcr );
  }
}


/* ContextPosFormat2 */

static FT_Error  Load_ContextPos2( HB_ContextPosFormat2*  cpf2,
				   FT_Stream               stream )
{
  FT_Error  error;
  FT_Memory memory = stream->memory;

  FT_UShort         n, m, count;
  FT_ULong          cur_offset, new_offset, base_offset;

  HB_PosClassSet*  pcs;


  base_offset = FILE_Pos() - 2;

  if ( ACCESS_Frame( 2L ) )
    return error;

  new_offset = GET_UShort() + base_offset;

  FORGET_Frame();

  cur_offset = FILE_Pos();
  if ( FILE_Seek( new_offset ) ||
       ( error = _HB_OPEN_Load_Coverage( &cpf2->Coverage, stream ) ) != FT_Err_Ok )
    return error;
  (void)FILE_Seek( cur_offset );

  if ( ACCESS_Frame( 4L ) )
    goto Fail3;

  new_offset = GET_UShort() + base_offset;

  /* `PosClassSetCount' is the upper limit for class values, thus we
     read it now to make an additional safety check.                 */

  count = cpf2->PosClassSetCount = GET_UShort();

  FORGET_Frame();

  cur_offset = FILE_Pos();
  if ( FILE_Seek( new_offset ) ||
       ( error = _HB_OPEN_Load_ClassDefinition( &cpf2->ClassDef, count,
				       stream ) ) != FT_Err_Ok )
    goto Fail3;
  (void)FILE_Seek( cur_offset );

  cpf2->PosClassSet      = NULL;
  cpf2->MaxContextLength = 0;

  if ( ALLOC_ARRAY( cpf2->PosClassSet, count, HB_PosClassSet ) )
    goto Fail2;

  pcs = cpf2->PosClassSet;

  for ( n = 0; n < count; n++ )
  {
    if ( ACCESS_Frame( 2L ) )
      goto Fail1;

    new_offset = GET_UShort() + base_offset;

    FORGET_Frame();

    if ( new_offset != base_offset )      /* not a NULL offset */
    {
      cur_offset = FILE_Pos();
      if ( FILE_Seek( new_offset ) ||
	   ( error = Load_PosClassSet( cpf2, &pcs[n],
				       stream ) ) != FT_Err_Ok )
	goto Fail1;
      (void)FILE_Seek( cur_offset );
    }
    else
    {
      /* we create a PosClassSet table with no entries */

      cpf2->PosClassSet[n].PosClassRuleCount = 0;
      cpf2->PosClassSet[n].PosClassRule      = NULL;
    }
  }

  return FT_Err_Ok;

Fail1:
  for ( m = 0; m < n; n++ )
    Free_PosClassSet( &pcs[m], memory );

  FREE( pcs );

Fail2:
  _HB_OPEN_Free_ClassDefinition( &cpf2->ClassDef, memory );

Fail3:
  _HB_OPEN_Free_Coverage( &cpf2->Coverage, memory );
  return error;
}


static void  Free_ContextPos2( HB_ContextPosFormat2*  cpf2,
			    FT_Memory               memory )
{
  FT_UShort         n, count;

  HB_PosClassSet*  pcs;


  if ( cpf2->PosClassSet )
  {
    count = cpf2->PosClassSetCount;
    pcs   = cpf2->PosClassSet;

    for ( n = 0; n < count; n++ )
      Free_PosClassSet( &pcs[n], memory );

    FREE( pcs );
  }

  _HB_OPEN_Free_ClassDefinition( &cpf2->ClassDef, memory );
  _HB_OPEN_Free_Coverage( &cpf2->Coverage, memory );
}


/* ContextPosFormat3 */

static FT_Error  Load_ContextPos3( HB_ContextPosFormat3*  cpf3,
				   FT_Stream               stream )
{
  FT_Error  error;
  FT_Memory memory = stream->memory;

  FT_UShort             n, count;
  FT_ULong              cur_offset, new_offset, base_offset;

  HB_Coverage*         c;
  HB_PosLookupRecord*  plr;


  base_offset = FILE_Pos() - 2L;

  if ( ACCESS_Frame( 4L ) )
    return error;

  cpf3->GlyphCount = GET_UShort();
  cpf3->PosCount   = GET_UShort();

  FORGET_Frame();

  cpf3->Coverage = NULL;

  count = cpf3->GlyphCount;

  if ( ALLOC_ARRAY( cpf3->Coverage, count, HB_Coverage ) )
    return error;

  c = cpf3->Coverage;

  for ( n = 0; n < count; n++ )
  {
    if ( ACCESS_Frame( 2L ) )
      goto Fail2;

    new_offset = GET_UShort() + base_offset;

    FORGET_Frame();

    cur_offset = FILE_Pos();
    if ( FILE_Seek( new_offset ) ||
	 ( error = _HB_OPEN_Load_Coverage( &c[n], stream ) ) != FT_Err_Ok )
      goto Fail2;
    (void)FILE_Seek( cur_offset );
  }

  cpf3->PosLookupRecord = NULL;

  count = cpf3->PosCount;

  if ( ALLOC_ARRAY( cpf3->PosLookupRecord, count, HB_PosLookupRecord ) )
    goto Fail2;

  plr = cpf3->PosLookupRecord;

  if ( ACCESS_Frame( count * 4L ) )
    goto Fail1;

  for ( n = 0; n < count; n++ )
  {
    plr[n].SequenceIndex   = GET_UShort();
    plr[n].LookupListIndex = GET_UShort();
  }

  FORGET_Frame();

  return FT_Err_Ok;

Fail1:
  FREE( plr );

Fail2:
  for ( n = 0; n < count; n++ )
    _HB_OPEN_Free_Coverage( &c[n], memory );

  FREE( c );
  return error;
}


static void  Free_ContextPos3( HB_ContextPosFormat3*  cpf3,
			    FT_Memory               memory )
{
  FT_UShort      n, count;

  HB_Coverage*  c;


  FREE( cpf3->PosLookupRecord );

  if ( cpf3->Coverage )
  {
    count = cpf3->GlyphCount;
    c     = cpf3->Coverage;

    for ( n = 0; n < count; n++ )
      _HB_OPEN_Free_Coverage( &c[n], memory );

    FREE( c );
  }
}


/* ContextPos */

static FT_Error  Load_ContextPos( HB_GPOS_SubTable* st,
				  FT_Stream        stream )
{
  FT_Error  error;
  HB_ContextPos*   cp = &st->context;


  if ( ACCESS_Frame( 2L ) )
    return error;

  cp->PosFormat = GET_UShort();

  FORGET_Frame();

  switch ( cp->PosFormat )
  {
  case 1:
    return Load_ContextPos1( &cp->cpf.cpf1, stream );

  case 2:
    return Load_ContextPos2( &cp->cpf.cpf2, stream );

  case 3:
    return Load_ContextPos3( &cp->cpf.cpf3, stream );

  default:
    return HB_Err_Invalid_GPOS_SubTable_Format;
  }

  return FT_Err_Ok;               /* never reached */
}


static void  Free_ContextPos( HB_GPOS_SubTable* st,
			      FT_Memory        memory )
{
  HB_ContextPos*   cp = &st->context;

  switch ( cp->PosFormat )
  {
  case 1:
    Free_ContextPos1( &cp->cpf.cpf1, memory );
    break;

  case 2:
    Free_ContextPos2( &cp->cpf.cpf2, memory );
    break;

  case 3:
    Free_ContextPos3( &cp->cpf.cpf3, memory );
    break;
  }
}


static FT_Error  Lookup_ContextPos1( GPOS_Instance*          gpi,
				     HB_ContextPosFormat1*  cpf1,
				     HB_Buffer              buffer,
				     FT_UShort               flags,
				     FT_UShort               context_length,
				     int                     nesting_level )
{
  FT_UShort        index, property;
  FT_UShort        i, j, k, numpr;
  FT_Error         error;
  HB_GPOSHeader*  gpos = gpi->gpos;

  HB_PosRule*     pr;
  HB_GDEFHeader*  gdef;


  gdef = gpos->gdef;

  if ( CHECK_Property( gdef, IN_CURITEM(), flags, &property ) )
    return error;

  error = _HB_OPEN_Coverage_Index( &cpf1->Coverage, IN_CURGLYPH(), &index );
  if ( error )
    return error;

  pr    = cpf1->PosRuleSet[index].PosRule;
  numpr = cpf1->PosRuleSet[index].PosRuleCount;

  for ( k = 0; k < numpr; k++ )
  {
    if ( context_length != 0xFFFF && context_length < pr[k].GlyphCount )
      goto next_posrule;

    if ( buffer->in_pos + pr[k].GlyphCount > buffer->in_length )
      goto next_posrule;                       /* context is too long */

    for ( i = 1, j = buffer->in_pos + 1; i < pr[k].GlyphCount; i++, j++ )
    {
      while ( CHECK_Property( gdef, IN_ITEM( j ), flags, &property ) )
      {
	if ( error && error != HB_Err_Not_Covered )
	  return error;

	if ( j + pr[k].GlyphCount - i == (FT_Long)buffer->in_length )
	  goto next_posrule;
	j++;
      }

      if ( IN_GLYPH( j ) != pr[k].Input[i - 1] )
	goto next_posrule;
    }

    return Do_ContextPos( gpi, pr[k].GlyphCount,
			  pr[k].PosCount, pr[k].PosLookupRecord,
			  buffer,
			  nesting_level );

    next_posrule:
      ;
  }

  return HB_Err_Not_Covered;
}


static FT_Error  Lookup_ContextPos2( GPOS_Instance*          gpi,
				     HB_ContextPosFormat2*  cpf2,
				     HB_Buffer              buffer,
				     FT_UShort               flags,
				     FT_UShort               context_length,
				     int                     nesting_level )
{
  FT_UShort          index, property;
  FT_Error           error;
  FT_Memory          memory = gpi->face->memory;
  FT_UShort          i, j, k, known_classes;

  FT_UShort*         classes;
  FT_UShort*         cl;
  HB_GPOSHeader*    gpos = gpi->gpos;

  HB_PosClassSet*   pcs;
  HB_PosClassRule*  pr;
  HB_GDEFHeader*    gdef;


  gdef = gpos->gdef;

  if ( CHECK_Property( gdef, IN_CURITEM(), flags, &property ) )
    return error;

  /* Note: The coverage table in format 2 doesn't give an index into
	   anything.  It just lets us know whether or not we need to
	   do any lookup at all.                                     */

  error = _HB_OPEN_Coverage_Index( &cpf2->Coverage, IN_CURGLYPH(), &index );
  if ( error )
    return error;

  if ( ALLOC_ARRAY( classes, cpf2->MaxContextLength, FT_UShort ) )
    return error;

  error = _HB_OPEN_Get_Class( &cpf2->ClassDef, IN_CURGLYPH(),
		     &classes[0], NULL );
  if ( error && error != HB_Err_Not_Covered )
    goto End;
  known_classes = 0;

  pcs = &cpf2->PosClassSet[classes[0]];
  if ( !pcs )
  {
    error = HB_Err_Invalid_GPOS_SubTable;
    goto End;
  }

  for ( k = 0; k < pcs->PosClassRuleCount; k++ )
  {
    pr = &pcs->PosClassRule[k];

    if ( context_length != 0xFFFF && context_length < pr->GlyphCount )
      goto next_posclassrule;

    if ( buffer->in_pos + pr->GlyphCount > buffer->in_length )
      goto next_posclassrule;                /* context is too long */

    cl   = pr->Class;

    /* Start at 1 because [0] is implied */

    for ( i = 1, j = buffer->in_pos + 1; i < pr->GlyphCount; i++, j++ )
    {
      while ( CHECK_Property( gdef, IN_ITEM( j ), flags, &property ) )
      {
	if ( error && error != HB_Err_Not_Covered )
	  goto End;

	if ( j + pr->GlyphCount - i == (FT_Long)buffer->in_length )
	  goto next_posclassrule;
	j++;
      }

      if ( i > known_classes )
      {
	/* Keeps us from having to do this for each rule */

	error = _HB_OPEN_Get_Class( &cpf2->ClassDef, IN_GLYPH( j ), &classes[i], NULL );
	if ( error && error != HB_Err_Not_Covered )
	  goto End;
	known_classes = i;
      }

      if ( cl[i - 1] != classes[i] )
	goto next_posclassrule;
    }

    error = Do_ContextPos( gpi, pr->GlyphCount,
			   pr->PosCount, pr->PosLookupRecord,
			   buffer,
			   nesting_level );
    goto End;

  next_posclassrule:
    ;
  }

  error = HB_Err_Not_Covered;

End:
  FREE( classes );
  return error;
}


static FT_Error  Lookup_ContextPos3( GPOS_Instance*          gpi,
				     HB_ContextPosFormat3*  cpf3,
				     HB_Buffer              buffer,
				     FT_UShort               flags,
				     FT_UShort               context_length,
				     int                     nesting_level )
{
  FT_Error         error;
  FT_UShort        index, i, j, property;
  HB_GPOSHeader*  gpos = gpi->gpos;

  HB_Coverage*    c;
  HB_GDEFHeader*  gdef;


  gdef = gpos->gdef;

  if ( CHECK_Property( gdef, IN_CURITEM(), flags, &property ) )
    return error;

  if ( context_length != 0xFFFF && context_length < cpf3->GlyphCount )
    return HB_Err_Not_Covered;

  if ( buffer->in_pos + cpf3->GlyphCount > buffer->in_length )
    return HB_Err_Not_Covered;         /* context is too long */

  c    = cpf3->Coverage;

  for ( i = 1, j = 1; i < cpf3->GlyphCount; i++, j++ )
  {
    while ( CHECK_Property( gdef, IN_ITEM( j ), flags, &property ) )
    {
      if ( error && error != HB_Err_Not_Covered )
	return error;

      if ( j + cpf3->GlyphCount - i == (FT_Long)buffer->in_length )
	return HB_Err_Not_Covered;
      j++;
    }

    error = _HB_OPEN_Coverage_Index( &c[i], IN_GLYPH( j ), &index );
    if ( error )
      return error;
  }

  return Do_ContextPos( gpi, cpf3->GlyphCount,
			cpf3->PosCount, cpf3->PosLookupRecord,
			buffer,
			nesting_level );
}


static FT_Error  Lookup_ContextPos( GPOS_Instance*    gpi,
				    HB_GPOS_SubTable* st,
				    HB_Buffer        buffer,
				    FT_UShort         flags,
				    FT_UShort         context_length,
				    int               nesting_level )
{
  HB_ContextPos*   cp = &st->context;

  switch ( cp->PosFormat )
  {
  case 1:
    return Lookup_ContextPos1( gpi, &cp->cpf.cpf1, buffer,
			       flags, context_length, nesting_level );

  case 2:
    return Lookup_ContextPos2( gpi, &cp->cpf.cpf2, buffer,
			       flags, context_length, nesting_level );

  case 3:
    return Lookup_ContextPos3( gpi, &cp->cpf.cpf3, buffer,
			       flags, context_length, nesting_level );

  default:
    return HB_Err_Invalid_GPOS_SubTable_Format;
  }

  return FT_Err_Ok;               /* never reached */
}


/* LookupType 8 */

/* ChainPosRule */

static FT_Error  Load_ChainPosRule( HB_ChainPosRule*  cpr,
				    FT_Stream          stream )
{
  FT_Error  error;
  FT_Memory memory = stream->memory;

  FT_UShort             n, count;
  FT_UShort*            b;
  FT_UShort*            i;
  FT_UShort*            l;

  HB_PosLookupRecord*  plr;


  if ( ACCESS_Frame( 2L ) )
    return error;

  cpr->BacktrackGlyphCount = GET_UShort();

  FORGET_Frame();

  cpr->Backtrack = NULL;

  count = cpr->BacktrackGlyphCount;

  if ( ALLOC_ARRAY( cpr->Backtrack, count, FT_UShort ) )
    return error;

  b = cpr->Backtrack;

  if ( ACCESS_Frame( count * 2L ) )
    goto Fail4;

  for ( n = 0; n < count; n++ )
    b[n] = GET_UShort();

  FORGET_Frame();

  if ( ACCESS_Frame( 2L ) )
    goto Fail4;

  cpr->InputGlyphCount = GET_UShort();

  FORGET_Frame();

  cpr->Input = NULL;

  count = cpr->InputGlyphCount - 1;  /* only InputGlyphCount - 1 elements */

  if ( ALLOC_ARRAY( cpr->Input, count, FT_UShort ) )
    goto Fail4;

  i = cpr->Input;

  if ( ACCESS_Frame( count * 2L ) )
    goto Fail3;

  for ( n = 0; n < count; n++ )
    i[n] = GET_UShort();

  FORGET_Frame();

  if ( ACCESS_Frame( 2L ) )
    goto Fail3;

  cpr->LookaheadGlyphCount = GET_UShort();

  FORGET_Frame();

  cpr->Lookahead = NULL;

  count = cpr->LookaheadGlyphCount;

  if ( ALLOC_ARRAY( cpr->Lookahead, count, FT_UShort ) )
    goto Fail3;

  l = cpr->Lookahead;

  if ( ACCESS_Frame( count * 2L ) )
    goto Fail2;

  for ( n = 0; n < count; n++ )
    l[n] = GET_UShort();

  FORGET_Frame();

  if ( ACCESS_Frame( 2L ) )
    goto Fail2;

  cpr->PosCount = GET_UShort();

  FORGET_Frame();

  cpr->PosLookupRecord = NULL;

  count = cpr->PosCount;

  if ( ALLOC_ARRAY( cpr->PosLookupRecord, count, HB_PosLookupRecord ) )
    goto Fail2;

  plr = cpr->PosLookupRecord;

  if ( ACCESS_Frame( count * 4L ) )
    goto Fail1;

  for ( n = 0; n < count; n++ )
  {
    plr[n].SequenceIndex   = GET_UShort();
    plr[n].LookupListIndex = GET_UShort();
  }

  FORGET_Frame();

  return FT_Err_Ok;

Fail1:
  FREE( plr );

Fail2:
  FREE( l );

Fail3:
  FREE( i );

Fail4:
  FREE( b );
  return error;
}


static void  Free_ChainPosRule( HB_ChainPosRule*  cpr,
				FT_Memory          memory )
{
  FREE( cpr->PosLookupRecord );
  FREE( cpr->Lookahead );
  FREE( cpr->Input );
  FREE( cpr->Backtrack );
}


/* ChainPosRuleSet */

static FT_Error  Load_ChainPosRuleSet( HB_ChainPosRuleSet*  cprs,
				       FT_Stream             stream )
{
  FT_Error  error;
  FT_Memory memory = stream->memory;

  FT_UShort          n, m, count;
  FT_ULong           cur_offset, new_offset, base_offset;

  HB_ChainPosRule*  cpr;


  base_offset = FILE_Pos();

  if ( ACCESS_Frame( 2L ) )
    return error;

  count = cprs->ChainPosRuleCount = GET_UShort();

  FORGET_Frame();

  cprs->ChainPosRule = NULL;

  if ( ALLOC_ARRAY( cprs->ChainPosRule, count, HB_ChainPosRule ) )
    return error;

  cpr = cprs->ChainPosRule;

  for ( n = 0; n < count; n++ )
  {
    if ( ACCESS_Frame( 2L ) )
      goto Fail;

    new_offset = GET_UShort() + base_offset;

    FORGET_Frame();

    cur_offset = FILE_Pos();
    if ( FILE_Seek( new_offset ) ||
	 ( error = Load_ChainPosRule( &cpr[n], stream ) ) != FT_Err_Ok )
      goto Fail;
    (void)FILE_Seek( cur_offset );
  }

  return FT_Err_Ok;

Fail:
  for ( m = 0; m < n; m++ )
    Free_ChainPosRule( &cpr[m], memory );

  FREE( cpr );
  return error;
}


static void  Free_ChainPosRuleSet( HB_ChainPosRuleSet*  cprs,
				   FT_Memory             memory )
{
  FT_UShort          n, count;

  HB_ChainPosRule*  cpr;


  if ( cprs->ChainPosRule )
  {
    count = cprs->ChainPosRuleCount;
    cpr   = cprs->ChainPosRule;

    for ( n = 0; n < count; n++ )
      Free_ChainPosRule( &cpr[n], memory );

    FREE( cpr );
  }
}


/* ChainContextPosFormat1 */

static FT_Error  Load_ChainContextPos1( HB_ChainContextPosFormat1*  ccpf1,
					FT_Stream                    stream )
{
  FT_Error  error;
  FT_Memory memory = stream->memory;

  FT_UShort             n, m, count;
  FT_ULong              cur_offset, new_offset, base_offset;

  HB_ChainPosRuleSet*  cprs;


  base_offset = FILE_Pos() - 2L;

  if ( ACCESS_Frame( 2L ) )
    return error;

  new_offset = GET_UShort() + base_offset;

  FORGET_Frame();

  cur_offset = FILE_Pos();
  if ( FILE_Seek( new_offset ) ||
       ( error = _HB_OPEN_Load_Coverage( &ccpf1->Coverage, stream ) ) != FT_Err_Ok )
    return error;
  (void)FILE_Seek( cur_offset );

  if ( ACCESS_Frame( 2L ) )
    goto Fail2;

  count = ccpf1->ChainPosRuleSetCount = GET_UShort();

  FORGET_Frame();

  ccpf1->ChainPosRuleSet = NULL;

  if ( ALLOC_ARRAY( ccpf1->ChainPosRuleSet, count, HB_ChainPosRuleSet ) )
    goto Fail2;

  cprs = ccpf1->ChainPosRuleSet;

  for ( n = 0; n < count; n++ )
  {
    if ( ACCESS_Frame( 2L ) )
      goto Fail1;

    new_offset = GET_UShort() + base_offset;

    FORGET_Frame();

    cur_offset = FILE_Pos();
    if ( FILE_Seek( new_offset ) ||
	 ( error = Load_ChainPosRuleSet( &cprs[n], stream ) ) != FT_Err_Ok )
      goto Fail1;
    (void)FILE_Seek( cur_offset );
  }

  return FT_Err_Ok;

Fail1:
  for ( m = 0; m < n; m++ )
    Free_ChainPosRuleSet( &cprs[m], memory );

  FREE( cprs );

Fail2:
  _HB_OPEN_Free_Coverage( &ccpf1->Coverage, memory );
  return error;
}


static void  Free_ChainContextPos1( HB_ChainContextPosFormat1*  ccpf1,
				 FT_Memory                    memory )
{
  FT_UShort             n, count;

  HB_ChainPosRuleSet*  cprs;


  if ( ccpf1->ChainPosRuleSet )
  {
    count = ccpf1->ChainPosRuleSetCount;
    cprs  = ccpf1->ChainPosRuleSet;

    for ( n = 0; n < count; n++ )
      Free_ChainPosRuleSet( &cprs[n], memory );

    FREE( cprs );
  }

  _HB_OPEN_Free_Coverage( &ccpf1->Coverage, memory );
}


/* ChainPosClassRule */

static FT_Error  Load_ChainPosClassRule(
		   HB_ChainContextPosFormat2*  ccpf2,
		   HB_ChainPosClassRule*       cpcr,
		   FT_Stream                    stream )
{
  FT_Error  error;
  FT_Memory memory = stream->memory;

  FT_UShort             n, count;

  FT_UShort*            b;
  FT_UShort*            i;
  FT_UShort*            l;
  HB_PosLookupRecord*  plr;
  FT_Bool*              d;


  if ( ACCESS_Frame( 2L ) )
    return error;

  cpcr->BacktrackGlyphCount = GET_UShort();

  FORGET_Frame();

  if ( cpcr->BacktrackGlyphCount > ccpf2->MaxBacktrackLength )
    ccpf2->MaxBacktrackLength = cpcr->BacktrackGlyphCount;

  cpcr->Backtrack = NULL;

  count = cpcr->BacktrackGlyphCount;

  if ( ALLOC_ARRAY( cpcr->Backtrack, count, FT_UShort ) )
    return error;

  b = cpcr->Backtrack;
  d = ccpf2->BacktrackClassDef.Defined;

  if ( ACCESS_Frame( count * 2L ) )
    goto Fail4;

  for ( n = 0; n < count; n++ )
  {
    b[n] = GET_UShort();

    /* We check whether the specific class is used at all.  If not,
       class 0 is used instead.                                     */

    if ( !d[b[n]] )
      b[n] = 0;
  }

  FORGET_Frame();

  if ( ACCESS_Frame( 2L ) )
    goto Fail4;

  cpcr->InputGlyphCount = GET_UShort();

  if ( cpcr->InputGlyphCount > ccpf2->MaxInputLength )
    ccpf2->MaxInputLength = cpcr->InputGlyphCount;

  FORGET_Frame();

  cpcr->Input = NULL;

  count = cpcr->InputGlyphCount - 1; /* only InputGlyphCount - 1 elements */

  if ( ALLOC_ARRAY( cpcr->Input, count, FT_UShort ) )
    goto Fail4;

  i = cpcr->Input;
  d = ccpf2->InputClassDef.Defined;

  if ( ACCESS_Frame( count * 2L ) )
    goto Fail3;

  for ( n = 0; n < count; n++ )
  {
    i[n] = GET_UShort();

    if ( !d[i[n]] )
      i[n] = 0;
  }

  FORGET_Frame();

  if ( ACCESS_Frame( 2L ) )
    goto Fail3;

  cpcr->LookaheadGlyphCount = GET_UShort();

  FORGET_Frame();

  if ( cpcr->LookaheadGlyphCount > ccpf2->MaxLookaheadLength )
    ccpf2->MaxLookaheadLength = cpcr->LookaheadGlyphCount;

  cpcr->Lookahead = NULL;

  count = cpcr->LookaheadGlyphCount;

  if ( ALLOC_ARRAY( cpcr->Lookahead, count, FT_UShort ) )
    goto Fail3;

  l = cpcr->Lookahead;
  d = ccpf2->LookaheadClassDef.Defined;

  if ( ACCESS_Frame( count * 2L ) )
    goto Fail2;

  for ( n = 0; n < count; n++ )
  {
    l[n] = GET_UShort();

    if ( !d[l[n]] )
      l[n] = 0;
  }

  FORGET_Frame();

  if ( ACCESS_Frame( 2L ) )
    goto Fail2;

  cpcr->PosCount = GET_UShort();

  FORGET_Frame();

  cpcr->PosLookupRecord = NULL;

  count = cpcr->PosCount;

  if ( ALLOC_ARRAY( cpcr->PosLookupRecord, count, HB_PosLookupRecord ) )
    goto Fail2;

  plr = cpcr->PosLookupRecord;

  if ( ACCESS_Frame( count * 4L ) )
    goto Fail1;

  for ( n = 0; n < count; n++ )
  {
    plr[n].SequenceIndex   = GET_UShort();
    plr[n].LookupListIndex = GET_UShort();
  }

  FORGET_Frame();

  return FT_Err_Ok;

Fail1:
  FREE( plr );

Fail2:
  FREE( l );

Fail3:
  FREE( i );

Fail4:
  FREE( b );
  return error;
}


static void  Free_ChainPosClassRule( HB_ChainPosClassRule*  cpcr,
				     FT_Memory               memory )
{
  FREE( cpcr->PosLookupRecord );
  FREE( cpcr->Lookahead );
  FREE( cpcr->Input );
  FREE( cpcr->Backtrack );
}


/* PosClassSet */

static FT_Error  Load_ChainPosClassSet(
		   HB_ChainContextPosFormat2*  ccpf2,
		   HB_ChainPosClassSet*        cpcs,
		   FT_Stream                    stream )
{
  FT_Error  error;
  FT_Memory memory = stream->memory;

  FT_UShort               n, m, count;
  FT_ULong                cur_offset, new_offset, base_offset;

  HB_ChainPosClassRule*  cpcr;


  base_offset = FILE_Pos();

  if ( ACCESS_Frame( 2L ) )
    return error;

  count = cpcs->ChainPosClassRuleCount = GET_UShort();

  FORGET_Frame();

  cpcs->ChainPosClassRule = NULL;

  if ( ALLOC_ARRAY( cpcs->ChainPosClassRule, count,
		    HB_ChainPosClassRule ) )
    return error;

  cpcr = cpcs->ChainPosClassRule;

  for ( n = 0; n < count; n++ )
  {
    if ( ACCESS_Frame( 2L ) )
      goto Fail;

    new_offset = GET_UShort() + base_offset;

    FORGET_Frame();

    cur_offset = FILE_Pos();
    if ( FILE_Seek( new_offset ) ||
	 ( error = Load_ChainPosClassRule( ccpf2, &cpcr[n],
					   stream ) ) != FT_Err_Ok )
      goto Fail;
    (void)FILE_Seek( cur_offset );
  }

  return FT_Err_Ok;

Fail:
  for ( m = 0; m < n; m++ )
    Free_ChainPosClassRule( &cpcr[m], memory );

  FREE( cpcr );
  return error;
}


static void  Free_ChainPosClassSet( HB_ChainPosClassSet*  cpcs,
				    FT_Memory              memory )
{
  FT_UShort               n, count;

  HB_ChainPosClassRule*  cpcr;


  if ( cpcs->ChainPosClassRule )
  {
    count = cpcs->ChainPosClassRuleCount;
    cpcr  = cpcs->ChainPosClassRule;

    for ( n = 0; n < count; n++ )
      Free_ChainPosClassRule( &cpcr[n], memory );

    FREE( cpcr );
  }
}


static FT_Error GPOS_Load_EmptyOrClassDefinition( HB_ClassDefinition*  cd,
					     FT_UShort             limit,
					     FT_ULong              class_offset,
					     FT_ULong              base_offset,
					     FT_Stream             stream )
{
  FT_Error error;
  FT_ULong               cur_offset;

  cur_offset = FILE_Pos();

  if ( class_offset )
    {
      if ( !FILE_Seek( class_offset + base_offset ) )
	error = _HB_OPEN_Load_ClassDefinition( cd, limit, stream );
    }
  else
     error = _HB_OPEN_Load_EmptyClassDefinition ( cd, stream );

  if (error == FT_Err_Ok)
    (void)FILE_Seek( cur_offset ); /* Changes error as a side-effect */

  return error;
}

/* ChainContextPosFormat2 */

static FT_Error  Load_ChainContextPos2( HB_ChainContextPosFormat2*  ccpf2,
					FT_Stream                    stream )
{
  FT_Error  error;
  FT_Memory memory = stream->memory;

  FT_UShort              n, m, count;
  FT_ULong               cur_offset, new_offset, base_offset;
  FT_ULong               backtrack_offset, input_offset, lookahead_offset;

  HB_ChainPosClassSet*  cpcs;


  base_offset = FILE_Pos() - 2;

  if ( ACCESS_Frame( 2L ) )
    return error;

  new_offset = GET_UShort() + base_offset;

  FORGET_Frame();

  cur_offset = FILE_Pos();
  if ( FILE_Seek( new_offset ) ||
       ( error = _HB_OPEN_Load_Coverage( &ccpf2->Coverage, stream ) ) != FT_Err_Ok )
    return error;
  (void)FILE_Seek( cur_offset );

  if ( ACCESS_Frame( 8L ) )
    goto Fail5;

  backtrack_offset = GET_UShort();
  input_offset     = GET_UShort();
  lookahead_offset = GET_UShort();

  /* `ChainPosClassSetCount' is the upper limit for input class values,
     thus we read it now to make an additional safety check. No limit
     is known or needed for the other two class definitions          */

  count = ccpf2->ChainPosClassSetCount = GET_UShort();

  FORGET_Frame();

  if ( ( error = GPOS_Load_EmptyOrClassDefinition( &ccpf2->BacktrackClassDef, 65535,
					      backtrack_offset, base_offset,
					      stream ) ) != FT_Err_Ok )
    goto Fail5;
  if ( ( error = GPOS_Load_EmptyOrClassDefinition( &ccpf2->InputClassDef, count,
					      input_offset, base_offset,
					      stream ) ) != FT_Err_Ok )
    goto Fail4;
  if ( ( error = GPOS_Load_EmptyOrClassDefinition( &ccpf2->LookaheadClassDef, 65535,
					      lookahead_offset, base_offset,
					      stream ) ) != FT_Err_Ok )
    goto Fail3;

  ccpf2->ChainPosClassSet   = NULL;
  ccpf2->MaxBacktrackLength = 0;
  ccpf2->MaxInputLength     = 0;
  ccpf2->MaxLookaheadLength = 0;

  if ( ALLOC_ARRAY( ccpf2->ChainPosClassSet, count, HB_ChainPosClassSet ) )
    goto Fail2;

  cpcs = ccpf2->ChainPosClassSet;

  for ( n = 0; n < count; n++ )
  {
    if ( ACCESS_Frame( 2L ) )
      goto Fail1;

    new_offset = GET_UShort() + base_offset;

    FORGET_Frame();

    if ( new_offset != base_offset )      /* not a NULL offset */
    {
      cur_offset = FILE_Pos();
      if ( FILE_Seek( new_offset ) ||
	   ( error = Load_ChainPosClassSet( ccpf2, &cpcs[n],
					    stream ) ) != FT_Err_Ok )
	goto Fail1;
      (void)FILE_Seek( cur_offset );
    }
    else
    {
      /* we create a ChainPosClassSet table with no entries */

      ccpf2->ChainPosClassSet[n].ChainPosClassRuleCount = 0;
      ccpf2->ChainPosClassSet[n].ChainPosClassRule      = NULL;
    }
  }

  return FT_Err_Ok;

Fail1:
  for ( m = 0; m < n; m++ )
    Free_ChainPosClassSet( &cpcs[m], memory );

  FREE( cpcs );

Fail2:
  _HB_OPEN_Free_ClassDefinition( &ccpf2->LookaheadClassDef, memory );

Fail3:
  _HB_OPEN_Free_ClassDefinition( &ccpf2->InputClassDef, memory );

Fail4:
  _HB_OPEN_Free_ClassDefinition( &ccpf2->BacktrackClassDef, memory );

Fail5:
  _HB_OPEN_Free_Coverage( &ccpf2->Coverage, memory );
  return error;
}


static void  Free_ChainContextPos2( HB_ChainContextPosFormat2*  ccpf2,
				 FT_Memory                    memory )
{
  FT_UShort              n, count;

  HB_ChainPosClassSet*  cpcs;


  if ( ccpf2->ChainPosClassSet )
  {
    count = ccpf2->ChainPosClassSetCount;
    cpcs  = ccpf2->ChainPosClassSet;

    for ( n = 0; n < count; n++ )
      Free_ChainPosClassSet( &cpcs[n], memory );

    FREE( cpcs );
  }

  _HB_OPEN_Free_ClassDefinition( &ccpf2->LookaheadClassDef, memory );
  _HB_OPEN_Free_ClassDefinition( &ccpf2->InputClassDef, memory );
  _HB_OPEN_Free_ClassDefinition( &ccpf2->BacktrackClassDef, memory );

  _HB_OPEN_Free_Coverage( &ccpf2->Coverage, memory );
}


/* ChainContextPosFormat3 */

static FT_Error  Load_ChainContextPos3( HB_ChainContextPosFormat3*  ccpf3,
					FT_Stream                    stream )
{
  FT_Error  error;
  FT_Memory memory = stream->memory;

  FT_UShort             n, nb, ni, nl, m, count;
  FT_UShort             backtrack_count, input_count, lookahead_count;
  FT_ULong              cur_offset, new_offset, base_offset;

  HB_Coverage*         b;
  HB_Coverage*         i;
  HB_Coverage*         l;
  HB_PosLookupRecord*  plr;


  base_offset = FILE_Pos() - 2L;

  if ( ACCESS_Frame( 2L ) )
    return error;

  ccpf3->BacktrackGlyphCount = GET_UShort();

  FORGET_Frame();

  ccpf3->BacktrackCoverage = NULL;

  backtrack_count = ccpf3->BacktrackGlyphCount;

  if ( ALLOC_ARRAY( ccpf3->BacktrackCoverage, backtrack_count,
		    HB_Coverage ) )
    return error;

  b = ccpf3->BacktrackCoverage;

  for ( nb = 0; nb < backtrack_count; nb++ )
  {
    if ( ACCESS_Frame( 2L ) )
      goto Fail4;

    new_offset = GET_UShort() + base_offset;

    FORGET_Frame();

    cur_offset = FILE_Pos();
    if ( FILE_Seek( new_offset ) ||
	 ( error = _HB_OPEN_Load_Coverage( &b[nb], stream ) ) != FT_Err_Ok )
      goto Fail4;
    (void)FILE_Seek( cur_offset );
  }

  if ( ACCESS_Frame( 2L ) )
    goto Fail4;

  ccpf3->InputGlyphCount = GET_UShort();

  FORGET_Frame();

  ccpf3->InputCoverage = NULL;

  input_count = ccpf3->InputGlyphCount;

  if ( ALLOC_ARRAY( ccpf3->InputCoverage, input_count, HB_Coverage ) )
    goto Fail4;

  i = ccpf3->InputCoverage;

  for ( ni = 0; ni < input_count; ni++ )
  {
    if ( ACCESS_Frame( 2L ) )
      goto Fail3;

    new_offset = GET_UShort() + base_offset;

    FORGET_Frame();

    cur_offset = FILE_Pos();
    if ( FILE_Seek( new_offset ) ||
	 ( error = _HB_OPEN_Load_Coverage( &i[ni], stream ) ) != FT_Err_Ok )
      goto Fail3;
    (void)FILE_Seek( cur_offset );
  }

  if ( ACCESS_Frame( 2L ) )
    goto Fail3;

  ccpf3->LookaheadGlyphCount = GET_UShort();

  FORGET_Frame();

  ccpf3->LookaheadCoverage = NULL;

  lookahead_count = ccpf3->LookaheadGlyphCount;

  if ( ALLOC_ARRAY( ccpf3->LookaheadCoverage, lookahead_count,
		    HB_Coverage ) )
    goto Fail3;

  l = ccpf3->LookaheadCoverage;

  for ( nl = 0; nl < lookahead_count; nl++ )
  {
    if ( ACCESS_Frame( 2L ) )
      goto Fail2;

    new_offset = GET_UShort() + base_offset;

    FORGET_Frame();

    cur_offset = FILE_Pos();
    if ( FILE_Seek( new_offset ) ||
	 ( error = _HB_OPEN_Load_Coverage( &l[nl], stream ) ) != FT_Err_Ok )
      goto Fail2;
    (void)FILE_Seek( cur_offset );
  }

  if ( ACCESS_Frame( 2L ) )
    goto Fail2;

  ccpf3->PosCount = GET_UShort();

  FORGET_Frame();

  ccpf3->PosLookupRecord = NULL;

  count = ccpf3->PosCount;

  if ( ALLOC_ARRAY( ccpf3->PosLookupRecord, count, HB_PosLookupRecord ) )
    goto Fail2;

  plr = ccpf3->PosLookupRecord;

  if ( ACCESS_Frame( count * 4L ) )
    goto Fail1;

  for ( n = 0; n < count; n++ )
  {
    plr[n].SequenceIndex   = GET_UShort();
    plr[n].LookupListIndex = GET_UShort();
  }

  FORGET_Frame();

  return FT_Err_Ok;

Fail1:
  FREE( plr );

Fail2:
  for ( m = 0; m < nl; nl++ )
    _HB_OPEN_Free_Coverage( &l[m], memory );

  FREE( l );

Fail3:
  for ( m = 0; m < ni; n++ )
    _HB_OPEN_Free_Coverage( &i[m], memory );

  FREE( i );

Fail4:
  for ( m = 0; m < nb; n++ )
    _HB_OPEN_Free_Coverage( &b[m], memory );

  FREE( b );
  return error;
}


static void  Free_ChainContextPos3( HB_ChainContextPosFormat3*  ccpf3,
				 FT_Memory                    memory )
{
  FT_UShort      n, count;

  HB_Coverage*  c;


  FREE( ccpf3->PosLookupRecord );

  if ( ccpf3->LookaheadCoverage )
  {
    count = ccpf3->LookaheadGlyphCount;
    c     = ccpf3->LookaheadCoverage;

    for ( n = 0; n < count; n++ )
      _HB_OPEN_Free_Coverage( &c[n], memory );

    FREE( c );
  }

  if ( ccpf3->InputCoverage )
  {
    count = ccpf3->InputGlyphCount;
    c     = ccpf3->InputCoverage;

    for ( n = 0; n < count; n++ )
      _HB_OPEN_Free_Coverage( &c[n], memory );

    FREE( c );
  }

  if ( ccpf3->BacktrackCoverage )
  {
    count = ccpf3->BacktrackGlyphCount;
    c     = ccpf3->BacktrackCoverage;

    for ( n = 0; n < count; n++ )
      _HB_OPEN_Free_Coverage( &c[n], memory );

    FREE( c );
  }
}


/* ChainContextPos */

static FT_Error  Load_ChainContextPos( HB_GPOS_SubTable* st,
				       FT_Stream             stream )
{
  FT_Error  error;
  HB_ChainContextPos*  ccp = &st->chain;


  if ( ACCESS_Frame( 2L ) )
    return error;

  ccp->PosFormat = GET_UShort();

  FORGET_Frame();

  switch ( ccp->PosFormat )
  {
  case 1:
    return Load_ChainContextPos1( &ccp->ccpf.ccpf1, stream );

  case 2:
    return Load_ChainContextPos2( &ccp->ccpf.ccpf2, stream );

  case 3:
    return Load_ChainContextPos3( &ccp->ccpf.ccpf3, stream );

  default:
    return HB_Err_Invalid_GPOS_SubTable_Format;
  }

  return FT_Err_Ok;               /* never reached */
}


static void  Free_ChainContextPos( HB_GPOS_SubTable* st,
				   FT_Memory             memory )
{
  HB_ChainContextPos*  ccp = &st->chain;

  switch ( ccp->PosFormat )
  {
  case 1:
    Free_ChainContextPos1( &ccp->ccpf.ccpf1, memory );
    break;

  case 2:
    Free_ChainContextPos2( &ccp->ccpf.ccpf2, memory );
    break;

  case 3:
    Free_ChainContextPos3( &ccp->ccpf.ccpf3, memory );
    break;
  }
}


static FT_Error  Lookup_ChainContextPos1(
		   GPOS_Instance*               gpi,
		   HB_ChainContextPosFormat1*  ccpf1,
		   HB_Buffer                   buffer,
		   FT_UShort                    flags,
		   FT_UShort                    context_length,
		   int                          nesting_level )
{
  FT_UShort          index, property;
  FT_UShort          i, j, k, num_cpr;
  FT_UShort          bgc, igc, lgc;
  FT_Error           error;
  HB_GPOSHeader*    gpos = gpi->gpos;

  HB_ChainPosRule*  cpr;
  HB_ChainPosRule   curr_cpr;
  HB_GDEFHeader*    gdef;


  gdef = gpos->gdef;

  if ( CHECK_Property( gdef, IN_CURITEM(), flags, &property ) )
    return error;

  error = _HB_OPEN_Coverage_Index( &ccpf1->Coverage, IN_CURGLYPH(), &index );
  if ( error )
    return error;

  cpr     = ccpf1->ChainPosRuleSet[index].ChainPosRule;
  num_cpr = ccpf1->ChainPosRuleSet[index].ChainPosRuleCount;

  for ( k = 0; k < num_cpr; k++ )
  {
    curr_cpr = cpr[k];
    bgc      = curr_cpr.BacktrackGlyphCount;
    igc      = curr_cpr.InputGlyphCount;
    lgc      = curr_cpr.LookaheadGlyphCount;

    if ( context_length != 0xFFFF && context_length < igc )
      goto next_chainposrule;

    /* check whether context is too long; it is a first guess only */

    if ( bgc > buffer->in_pos || buffer->in_pos + igc + lgc > buffer->in_length )
      goto next_chainposrule;

    if ( bgc )
    {
      /* Since we don't know in advance the number of glyphs to inspect,
	 we search backwards for matches in the backtrack glyph array    */

      for ( i = 0, j = buffer->in_pos - 1; i < bgc; i++, j-- )
      {
	while ( CHECK_Property( gdef, IN_ITEM( j ), flags, &property ) )
	{
	  if ( error && error != HB_Err_Not_Covered )
	    return error;

	  if ( j + 1 == bgc - i )
	    goto next_chainposrule;
	  j--;
	}

	/* In OpenType 1.3, it is undefined whether the offsets of
	   backtrack glyphs is in logical order or not.  Version 1.4
	   will clarify this:

	     Logical order -      a  b  c  d  e  f  g  h  i  j
					      i
	     Input offsets -                  0  1
	     Backtrack offsets -  3  2  1  0
	     Lookahead offsets -                    0  1  2  3           */

	if ( IN_GLYPH( j ) != curr_cpr.Backtrack[i] )
	  goto next_chainposrule;
      }
    }

    /* Start at 1 because [0] is implied */

    for ( i = 1, j = buffer->in_pos + 1; i < igc; i++, j++ )
    {
      while ( CHECK_Property( gdef, IN_ITEM( j ), flags, &property ) )
      {
	if ( error && error != HB_Err_Not_Covered )
	  return error;

	if ( j + igc - i + lgc == (FT_Long)buffer->in_length )
	  goto next_chainposrule;
	j++;
      }

      if ( IN_GLYPH( j ) != curr_cpr.Input[i - 1] )
	goto next_chainposrule;
    }

    /* we are starting to check for lookahead glyphs right after the
       last context glyph                                            */

    for ( i = 0; i < lgc; i++, j++ )
    {
      while ( CHECK_Property( gdef, IN_ITEM( j ), flags, &property ) )
      {
	if ( error && error != HB_Err_Not_Covered )
	  return error;

	if ( j + lgc - i == (FT_Long)buffer->in_length )
	  goto next_chainposrule;
	j++;
      }

      if ( IN_GLYPH( j ) != curr_cpr.Lookahead[i] )
	goto next_chainposrule;
    }

    return Do_ContextPos( gpi, igc,
			  curr_cpr.PosCount,
			  curr_cpr.PosLookupRecord,
			  buffer,
			  nesting_level );

  next_chainposrule:
    ;
  }

  return HB_Err_Not_Covered;
}


static FT_Error  Lookup_ChainContextPos2(
		   GPOS_Instance*               gpi,
		   HB_ChainContextPosFormat2*  ccpf2,
		   HB_Buffer                   buffer,
		   FT_UShort                    flags,
		   FT_UShort                    context_length,
		   int                          nesting_level )
{
  FT_UShort              index, property;
  FT_Memory              memory = gpi->face->memory;
  FT_Error               error;
  FT_UShort              i, j, k;
  FT_UShort              bgc, igc, lgc;
  FT_UShort              known_backtrack_classes,
			 known_input_classes,
			 known_lookahead_classes;

  FT_UShort*             backtrack_classes;
  FT_UShort*             input_classes;
  FT_UShort*             lookahead_classes;

  FT_UShort*             bc;
  FT_UShort*             ic;
  FT_UShort*             lc;
  HB_GPOSHeader*        gpos = gpi->gpos;

  HB_ChainPosClassSet*  cpcs;
  HB_ChainPosClassRule  cpcr;
  HB_GDEFHeader*        gdef;


  gdef = gpos->gdef;

  if ( CHECK_Property( gdef, IN_CURITEM(), flags, &property ) )
    return error;

  /* Note: The coverage table in format 2 doesn't give an index into
	   anything.  It just lets us know whether or not we need to
	   do any lookup at all.                                     */

  error = _HB_OPEN_Coverage_Index( &ccpf2->Coverage, IN_CURGLYPH(), &index );
  if ( error )
    return error;

  if ( ALLOC_ARRAY( backtrack_classes, ccpf2->MaxBacktrackLength, FT_UShort ) )
    return error;
  known_backtrack_classes = 0;

  if ( ALLOC_ARRAY( input_classes, ccpf2->MaxInputLength, FT_UShort ) )
    goto End3;
  known_input_classes = 1;

  if ( ALLOC_ARRAY( lookahead_classes, ccpf2->MaxLookaheadLength, FT_UShort ) )
    goto End2;
  known_lookahead_classes = 0;

  error = _HB_OPEN_Get_Class( &ccpf2->InputClassDef, IN_CURGLYPH(),
		     &input_classes[0], NULL );
  if ( error && error != HB_Err_Not_Covered )
    goto End1;

  cpcs = &ccpf2->ChainPosClassSet[input_classes[0]];
  if ( !cpcs )
  {
    error = HB_Err_Invalid_GPOS_SubTable;
    goto End1;
  }

  for ( k = 0; k < cpcs->ChainPosClassRuleCount; k++ )
  {
    cpcr = cpcs->ChainPosClassRule[k];
    bgc  = cpcr.BacktrackGlyphCount;
    igc  = cpcr.InputGlyphCount;
    lgc  = cpcr.LookaheadGlyphCount;

    if ( context_length != 0xFFFF && context_length < igc )
      goto next_chainposclassrule;

    /* check whether context is too long; it is a first guess only */

    if ( bgc > buffer->in_pos || buffer->in_pos + igc + lgc > buffer->in_length )
      goto next_chainposclassrule;

    if ( bgc )
    {
      /* Since we don't know in advance the number of glyphs to inspect,
	 we search backwards for matches in the backtrack glyph array.
	 Note that `known_backtrack_classes' starts at index 0.         */

      bc       = cpcr.Backtrack;

      for ( i = 0, j = buffer->in_pos - 1; i < bgc; i++, j-- )
      {
	while ( CHECK_Property( gdef, IN_ITEM( j ), flags, &property ) )
	{
	  if ( error && error != HB_Err_Not_Covered )
	    goto End1;

	  if ( j + 1 == bgc - i )
	    goto next_chainposclassrule;
	  j++;
	}

	if ( i >= known_backtrack_classes )
	{
	  /* Keeps us from having to do this for each rule */

	  error = _HB_OPEN_Get_Class( &ccpf2->BacktrackClassDef, IN_GLYPH( j ),
			     &backtrack_classes[i], NULL );
	  if ( error && error != HB_Err_Not_Covered )
	    goto End1;
	  known_backtrack_classes = i;
	}

	if ( bc[i] != backtrack_classes[i] )
	  goto next_chainposclassrule;
      }
    }

    ic       = cpcr.Input;

    /* Start at 1 because [0] is implied */

    for ( i = 1, j = buffer->in_pos + 1; i < igc; i++, j++ )
    {
      while ( CHECK_Property( gdef, IN_ITEM( j ), flags, &property ) )
      {
	if ( error && error != HB_Err_Not_Covered )
	  goto End1;

	if ( j + igc - i + lgc == (FT_Long)buffer->in_length )
	  goto next_chainposclassrule;
	j++;
      }

      if ( i >= known_input_classes )
      {
	error = _HB_OPEN_Get_Class( &ccpf2->InputClassDef, IN_GLYPH( j ),
			   &input_classes[i], NULL );
	if ( error && error != HB_Err_Not_Covered )
	  goto End1;
	known_input_classes = i;
      }

      if ( ic[i - 1] != input_classes[i] )
	goto next_chainposclassrule;
    }

    /* we are starting to check for lookahead glyphs right after the
       last context glyph                                            */

    lc       = cpcr.Lookahead;

    for ( i = 0; i < lgc; i++, j++ )
    {
      while ( CHECK_Property( gdef, IN_ITEM( j ), flags, &property ) )
      {
	if ( error && error != HB_Err_Not_Covered )
	  goto End1;

	if ( j + lgc - i == (FT_Long)buffer->in_length )
	  goto next_chainposclassrule;
	j++;
      }

      if ( i >= known_lookahead_classes )
      {
	error = _HB_OPEN_Get_Class( &ccpf2->LookaheadClassDef, IN_GLYPH( j ),
			   &lookahead_classes[i], NULL );
	if ( error && error != HB_Err_Not_Covered )
	  goto End1;
	known_lookahead_classes = i;
      }

      if ( lc[i] != lookahead_classes[i] )
	goto next_chainposclassrule;
    }

    error = Do_ContextPos( gpi, igc,
			   cpcr.PosCount,
			   cpcr.PosLookupRecord,
			   buffer,
			   nesting_level );
    goto End1;

  next_chainposclassrule:
    ;
  }

  error = HB_Err_Not_Covered;

End1:
  FREE( lookahead_classes );

End2:
  FREE( input_classes );

End3:
  FREE( backtrack_classes );
  return error;
}


static FT_Error  Lookup_ChainContextPos3(
		   GPOS_Instance*               gpi,
		   HB_ChainContextPosFormat3*  ccpf3,
		   HB_Buffer                   buffer,
		   FT_UShort                    flags,
		   FT_UShort                    context_length,
		   int                          nesting_level )
{
  FT_UShort        index, i, j, property;
  FT_UShort        bgc, igc, lgc;
  FT_Error         error;
  HB_GPOSHeader*  gpos = gpi->gpos;

  HB_Coverage*    bc;
  HB_Coverage*    ic;
  HB_Coverage*    lc;
  HB_GDEFHeader*  gdef;


  gdef = gpos->gdef;

  if ( CHECK_Property( gdef, IN_CURITEM(), flags, &property ) )
    return error;

  bgc = ccpf3->BacktrackGlyphCount;
  igc = ccpf3->InputGlyphCount;
  lgc = ccpf3->LookaheadGlyphCount;

  if ( context_length != 0xFFFF && context_length < igc )
    return HB_Err_Not_Covered;

  /* check whether context is too long; it is a first guess only */

  if ( bgc > buffer->in_pos || buffer->in_pos + igc + lgc > buffer->in_length )
    return HB_Err_Not_Covered;

  if ( bgc )
  {
    /* Since we don't know in advance the number of glyphs to inspect,
       we search backwards for matches in the backtrack glyph array    */

    bc       = ccpf3->BacktrackCoverage;

    for ( i = 0, j = buffer->in_pos - 1; i < bgc; i++, j-- )
    {
      while ( CHECK_Property( gdef, IN_ITEM( j ), flags, &property ) )
      {
	if ( error && error != HB_Err_Not_Covered )
	  return error;

	if ( j + 1 == bgc - i )
	  return HB_Err_Not_Covered;
	j--;
      }

      error = _HB_OPEN_Coverage_Index( &bc[i], IN_GLYPH( j ), &index );
      if ( error )
	return error;
    }
  }

  ic       = ccpf3->InputCoverage;

  for ( i = 0, j = buffer->in_pos; i < igc; i++, j++ )
  {
    /* We already called CHECK_Property for IN_GLYPH ( buffer->in_pos ) */
    while ( j > buffer->in_pos && CHECK_Property( gdef, IN_ITEM( j ), flags, &property ) )
    {
      if ( error && error != HB_Err_Not_Covered )
	return error;

      if ( j + igc - i + lgc == (FT_Long)buffer->in_length )
	return HB_Err_Not_Covered;
      j++;
    }

    error = _HB_OPEN_Coverage_Index( &ic[i], IN_GLYPH( j ), &index );
    if ( error )
      return error;
  }

  /* we are starting to check for lookahead glyphs right after the
     last context glyph                                            */

  lc       = ccpf3->LookaheadCoverage;

  for ( i = 0; i < lgc; i++, j++ )
  {
    while ( CHECK_Property( gdef, IN_ITEM( j ), flags, &property ) )
    {
      if ( error && error != HB_Err_Not_Covered )
	return error;

      if ( j + lgc - i == (FT_Long)buffer->in_length )
	return HB_Err_Not_Covered;
      j++;
    }

    error = _HB_OPEN_Coverage_Index( &lc[i], IN_GLYPH( j ), &index );
    if ( error )
      return error;
  }

  return Do_ContextPos( gpi, igc,
			ccpf3->PosCount,
			ccpf3->PosLookupRecord,
			buffer,
			nesting_level );
}


static FT_Error  Lookup_ChainContextPos(
		   GPOS_Instance*        gpi,
		   HB_GPOS_SubTable* st,
		   HB_Buffer            buffer,
		   FT_UShort             flags,
		   FT_UShort             context_length,
		   int                   nesting_level )
{
  HB_ChainContextPos*  ccp = &st->chain;

  switch ( ccp->PosFormat )
  {
  case 1:
    return Lookup_ChainContextPos1( gpi, &ccp->ccpf.ccpf1, buffer,
				    flags, context_length,
				    nesting_level );

  case 2:
    return Lookup_ChainContextPos2( gpi, &ccp->ccpf.ccpf2, buffer,
				    flags, context_length,
				    nesting_level );

  case 3:
    return Lookup_ChainContextPos3( gpi, &ccp->ccpf.ccpf3, buffer,
				    flags, context_length,
				    nesting_level );

  default:
    return HB_Err_Invalid_GPOS_SubTable_Format;
  }

  return FT_Err_Ok;               /* never reached */
}



/***********
 * GPOS API
 ***********/



FT_Error  HB_GPOS_Select_Script( HB_GPOSHeader*  gpos,
				 FT_ULong         script_tag,
				 FT_UShort*       script_index )
{
  FT_UShort          n;

  HB_ScriptList*    sl;
  HB_ScriptRecord*  sr;


  if ( !gpos || !script_index )
    return FT_Err_Invalid_Argument;

  sl = &gpos->ScriptList;
  sr = sl->ScriptRecord;

  for ( n = 0; n < sl->ScriptCount; n++ )
    if ( script_tag == sr[n].ScriptTag )
    {
      *script_index = n;

      return FT_Err_Ok;
    }

  return HB_Err_Not_Covered;
}



FT_Error  HB_GPOS_Select_Language( HB_GPOSHeader*  gpos,
				   FT_ULong         language_tag,
				   FT_UShort        script_index,
				   FT_UShort*       language_index,
				   FT_UShort*       req_feature_index )
{
  FT_UShort           n;

  HB_ScriptList*     sl;
  HB_ScriptRecord*   sr;
  HB_Script*         s;
  HB_LangSysRecord*  lsr;


  if ( !gpos || !language_index || !req_feature_index )
    return FT_Err_Invalid_Argument;

  sl = &gpos->ScriptList;
  sr = sl->ScriptRecord;

  if ( script_index >= sl->ScriptCount )
    return FT_Err_Invalid_Argument;

  s   = &sr[script_index].Script;
  lsr = s->LangSysRecord;

  for ( n = 0; n < s->LangSysCount; n++ )
    if ( language_tag == lsr[n].LangSysTag )
    {
      *language_index = n;
      *req_feature_index = lsr[n].LangSys.ReqFeatureIndex;

      return FT_Err_Ok;
    }

  return HB_Err_Not_Covered;
}


/* selecting 0xFFFF for language_index asks for the values of the
   default language (DefaultLangSys)                              */


FT_Error  HB_GPOS_Select_Feature( HB_GPOSHeader*  gpos,
				  FT_ULong         feature_tag,
				  FT_UShort        script_index,
				  FT_UShort        language_index,
				  FT_UShort*       feature_index )
{
  FT_UShort           n;

  HB_ScriptList*     sl;
  HB_ScriptRecord*   sr;
  HB_Script*         s;
  HB_LangSysRecord*  lsr;
  HB_LangSys*        ls;
  FT_UShort*          fi;

  HB_FeatureList*    fl;
  HB_FeatureRecord*  fr;


  if ( !gpos || !feature_index )
    return FT_Err_Invalid_Argument;

  sl = &gpos->ScriptList;
  sr = sl->ScriptRecord;

  fl = &gpos->FeatureList;
  fr = fl->FeatureRecord;

  if ( script_index >= sl->ScriptCount )
    return FT_Err_Invalid_Argument;

  s   = &sr[script_index].Script;
  lsr = s->LangSysRecord;

  if ( language_index == 0xFFFF )
    ls = &s->DefaultLangSys;
  else
  {
    if ( language_index >= s->LangSysCount )
      return FT_Err_Invalid_Argument;

    ls = &lsr[language_index].LangSys;
  }

  fi = ls->FeatureIndex;

  for ( n = 0; n < ls->FeatureCount; n++ )
  {
    if ( fi[n] >= fl->FeatureCount )
      return HB_Err_Invalid_GPOS_SubTable_Format;

    if ( feature_tag == fr[fi[n]].FeatureTag )
    {
      *feature_index = fi[n];

      return FT_Err_Ok;
    }
  }

  return HB_Err_Not_Covered;
}


/* The next three functions return a null-terminated list */


FT_Error  HB_GPOS_Query_Scripts( HB_GPOSHeader*  gpos,
				 FT_ULong**       script_tag_list )
{
  FT_Error           error;
  FT_Memory          memory;
  FT_UShort          n;
  FT_ULong*          stl;

  HB_ScriptList*    sl;
  HB_ScriptRecord*  sr;


  if ( !gpos || !script_tag_list )
    return FT_Err_Invalid_Argument;

  memory = gpos->memory;
  sl = &gpos->ScriptList;
  sr = sl->ScriptRecord;

  if ( ALLOC_ARRAY( stl, sl->ScriptCount + 1, FT_ULong ) )
    return error;

  for ( n = 0; n < sl->ScriptCount; n++ )
    stl[n] = sr[n].ScriptTag;
  stl[n] = 0;

  *script_tag_list = stl;

  return FT_Err_Ok;
}



FT_Error  HB_GPOS_Query_Languages( HB_GPOSHeader*  gpos,
				   FT_UShort        script_index,
				   FT_ULong**       language_tag_list )
{
  FT_Error            error;
  FT_Memory           memory;
  FT_UShort           n;
  FT_ULong*           ltl;

  HB_ScriptList*     sl;
  HB_ScriptRecord*   sr;
  HB_Script*         s;
  HB_LangSysRecord*  lsr;


  if ( !gpos || !language_tag_list )
    return FT_Err_Invalid_Argument;

  memory = gpos->memory;
  sl = &gpos->ScriptList;
  sr = sl->ScriptRecord;

  if ( script_index >= sl->ScriptCount )
    return FT_Err_Invalid_Argument;

  s   = &sr[script_index].Script;
  lsr = s->LangSysRecord;

  if ( ALLOC_ARRAY( ltl, s->LangSysCount + 1, FT_ULong ) )
    return error;

  for ( n = 0; n < s->LangSysCount; n++ )
    ltl[n] = lsr[n].LangSysTag;
  ltl[n] = 0;

  *language_tag_list = ltl;

  return FT_Err_Ok;
}


/* selecting 0xFFFF for language_index asks for the values of the
   default language (DefaultLangSys)                              */


FT_Error  HB_GPOS_Query_Features( HB_GPOSHeader*  gpos,
				  FT_UShort        script_index,
				  FT_UShort        language_index,
				  FT_ULong**       feature_tag_list )
{
  FT_UShort           n;
  FT_Error            error;
  FT_Memory           memory;
  FT_ULong*           ftl;

  HB_ScriptList*     sl;
  HB_ScriptRecord*   sr;
  HB_Script*         s;
  HB_LangSysRecord*  lsr;
  HB_LangSys*        ls;
  FT_UShort*          fi;

  HB_FeatureList*    fl;
  HB_FeatureRecord*  fr;


  if ( !gpos || !feature_tag_list )
    return FT_Err_Invalid_Argument;

  memory = gpos->memory;
  sl = &gpos->ScriptList;
  sr = sl->ScriptRecord;

  fl = &gpos->FeatureList;
  fr = fl->FeatureRecord;

  if ( script_index >= sl->ScriptCount )
    return FT_Err_Invalid_Argument;

  s   = &sr[script_index].Script;
  lsr = s->LangSysRecord;

  if ( language_index == 0xFFFF )
    ls = &s->DefaultLangSys;
  else
  {
    if ( language_index >= s->LangSysCount )
      return FT_Err_Invalid_Argument;

    ls = &lsr[language_index].LangSys;
  }

  fi = ls->FeatureIndex;

  if ( ALLOC_ARRAY( ftl, ls->FeatureCount + 1, FT_ULong ) )
    return error;

  for ( n = 0; n < ls->FeatureCount; n++ )
  {
    if ( fi[n] >= fl->FeatureCount )
    {
      FREE( ftl );
      return HB_Err_Invalid_GPOS_SubTable_Format;
    }
    ftl[n] = fr[fi[n]].FeatureTag;
  }
  ftl[n] = 0;

  *feature_tag_list = ftl;

  return FT_Err_Ok;
}


typedef FT_Error  (*Lookup_Pos_Func_Type)  ( GPOS_Instance*    gpi,
					     HB_GPOS_SubTable* st,
					     HB_Buffer        buffer,
					     FT_UShort         flags,
					     FT_UShort         context_length,
					     int               nesting_level );
static const Lookup_Pos_Func_Type Lookup_Pos_Call_Table[] = {
  Lookup_DefaultPos,
  Lookup_SinglePos,		/* HB_GPOS_LOOKUP_SINGLE     1 */
  Lookup_PairPos,		/* HB_GPOS_LOOKUP_PAIR       2 */
  Lookup_CursivePos,		/* HB_GPOS_LOOKUP_CURSIVE    3 */
  Lookup_MarkBasePos,		/* HB_GPOS_LOOKUP_MARKBASE   4 */
  Lookup_MarkLigPos,		/* HB_GPOS_LOOKUP_MARKLIG    5 */
  Lookup_MarkMarkPos,		/* HB_GPOS_LOOKUP_MARKMARK   6 */
  Lookup_ContextPos,		/* HB_GPOS_LOOKUP_CONTEXT    7 */
  Lookup_ChainContextPos,	/* HB_GPOS_LOOKUP_CHAIN      8 */
  Lookup_DefaultPos,		/* HB_GPOS_LOOKUP_EXTENSION  9 */
};

/* Do an individual subtable lookup.  Returns FT_Err_Ok if positioning
   has been done, or HB_Err_Not_Covered if not.                        */
static FT_Error  GPOS_Do_Glyph_Lookup( GPOS_Instance*    gpi,
				       FT_UShort         lookup_index,
				       HB_Buffer        buffer,
				       FT_UShort         context_length,
				       int               nesting_level )
{
  FT_Error             error = HB_Err_Not_Covered;
  FT_UShort            i, flags, lookup_count;
  HB_GPOSHeader*       gpos = gpi->gpos;
  HB_Lookup*           lo;
  int		       lookup_type;
  Lookup_Pos_Func_Type Func;


  nesting_level++;

  if ( nesting_level > HB_MAX_NESTING_LEVEL )
    return HB_Err_Too_Many_Nested_Contexts;

  lookup_count = gpos->LookupList.LookupCount;
  if (lookup_index >= lookup_count)
    return error;

  lo    = &gpos->LookupList.Lookup[lookup_index];
  flags = lo->LookupFlag;
  lookup_type = lo->LookupType;
  if (lookup_type >= ARRAY_LEN (Lookup_Pos_Call_Table))
    lookup_type = 0;
  Func = Lookup_Pos_Call_Table[lookup_type];

  for ( i = 0; i < lo->SubTableCount; i++ )
  {
    error = Func ( gpi,
		   &lo->SubTable[i].st.gpos,
		   buffer,
		   flags, context_length,
		   nesting_level );

    /* Check whether we have a successful positioning or an error other
       than HB_Err_Not_Covered                                         */

    if ( error != HB_Err_Not_Covered )
      return error;
  }

  return HB_Err_Not_Covered;
}


static FT_Error  Load_DefaultPos( HB_GPOS_SubTable* st,
				  FT_Stream         stream )
{
  FT_UNUSED(st);
  FT_UNUSED(stream);
  return HB_Err_Invalid_GPOS_SubTable_Format;
}

typedef FT_Error  (*Load_Pos_Func_Type)( HB_GPOS_SubTable* st,
					 FT_Stream         stream );
static const Load_Pos_Func_Type Load_Pos_Call_Table[] = {
  Load_DefaultPos,
  Load_SinglePos,		/* HB_GPOS_LOOKUP_SINGLE     1 */
  Load_PairPos,			/* HB_GPOS_LOOKUP_PAIR       2 */
  Load_CursivePos,		/* HB_GPOS_LOOKUP_CURSIVE    3 */
  Load_MarkBasePos,		/* HB_GPOS_LOOKUP_MARKBASE   4 */
  Load_MarkLigPos,		/* HB_GPOS_LOOKUP_MARKLIG    5 */
  Load_MarkMarkPos,		/* HB_GPOS_LOOKUP_MARKMARK   6 */
  Load_ContextPos,		/* HB_GPOS_LOOKUP_CONTEXT    7 */
  Load_ChainContextPos,		/* HB_GPOS_LOOKUP_CHAIN      8 */
  Load_DefaultPos,		/* HB_GPOS_LOOKUP_EXTENSION  9 */
};

FT_Error  _HB_GPOS_Load_SubTable( HB_GPOS_SubTable*  st,
				  FT_Stream     stream,
				  FT_UShort     lookup_type )
{
  Load_Pos_Func_Type Func;

  if (lookup_type >= ARRAY_LEN (Load_Pos_Call_Table))
    lookup_type = 0;

  Func = Load_Pos_Call_Table[lookup_type];

  return Func ( st, stream );
}


static void  Free_DefaultPos( HB_GPOS_SubTable* st,
			      FT_Memory         memory )
{
  FT_UNUSED(st);
  FT_UNUSED(memory);
}

typedef void (*Free_Pos_Func_Type)( HB_GPOS_SubTable* st,
				    FT_Memory         memory );
static const Free_Pos_Func_Type Free_Pos_Call_Table[] = {
  Free_DefaultPos,
  Free_SinglePos,		/* HB_GPOS_LOOKUP_SINGLE     1 */
  Free_PairPos,			/* HB_GPOS_LOOKUP_PAIR       2 */
  Free_CursivePos,		/* HB_GPOS_LOOKUP_CURSIVE    3 */
  Free_MarkBasePos,		/* HB_GPOS_LOOKUP_MARKBASE   4 */
  Free_MarkLigPos,		/* HB_GPOS_LOOKUP_MARKLIG    5 */
  Free_MarkMarkPos,		/* HB_GPOS_LOOKUP_MARKMARK   6 */
  Free_ContextPos,		/* HB_GPOS_LOOKUP_CONTEXT    7 */
  Free_ChainContextPos,		/* HB_GPOS_LOOKUP_CHAIN      8 */
  Free_DefaultPos,		/* HB_GPOS_LOOKUP_EXTENSION  9 */
};

void  _HB_GPOS_Free_SubTable( HB_GPOS_SubTable*  st,
			      FT_Memory     memory,
			      FT_UShort     lookup_type )
{
  Free_Pos_Func_Type Func;

  if (lookup_type >= ARRAY_LEN (Free_Pos_Call_Table))
    lookup_type = 0;

  Func = Free_Pos_Call_Table[lookup_type];

  Func ( st, memory );
}



/* apply one lookup to the input string object */

static FT_Error  GPOS_Do_String_Lookup( GPOS_Instance*    gpi,
				   FT_UShort         lookup_index,
				   HB_Buffer        buffer )
{
  FT_Error         error, retError = HB_Err_Not_Covered;
  HB_GPOSHeader*  gpos = gpi->gpos;

  FT_UInt*  properties = gpos->LookupList.Properties;

  int       nesting_level = 0;


  gpi->last  = 0xFFFF;     /* no last valid glyph for cursive pos. */

  buffer->in_pos = 0;

  while ( buffer->in_pos < buffer->in_length )
  {
    if ( ~IN_PROPERTIES( buffer->in_pos ) & properties[lookup_index] )
    {
      /* 0xFFFF indicates that we don't have a context length yet. */

      /* Note that the connection between mark and base glyphs hold
	 exactly one (string) lookup.  For example, it would be possible
	 that in the first lookup, mark glyph X is attached to base
	 glyph A, and in the next lookup it is attached to base glyph B.
	 It is up to the font designer to provide meaningful lookups and
	 lookup order.                                                   */

      error = GPOS_Do_Glyph_Lookup( gpi, lookup_index, buffer,
				    0xFFFF, nesting_level );
      if ( error && error != HB_Err_Not_Covered )
	return error;
    }
    else
    {
      /* Contrary to properties defined in GDEF, user-defined properties
	 will always stop a possible cursive positioning.                */
      gpi->last = 0xFFFF;

      error = HB_Err_Not_Covered;
    }

    if ( error == HB_Err_Not_Covered )
      (buffer->in_pos)++;
    else
      retError = error;
  }

  return retError;
}


static FT_Error  Position_CursiveChain ( HB_Buffer     buffer )
{
  FT_ULong   i, j;
  HB_Position positions = buffer->positions;

  /* First handle all left-to-right connections */
  for (j = 0; j < buffer->in_length; j--)
  {
    if (positions[j].cursive_chain > 0)
      positions[j].y_pos += positions[j - positions[j].cursive_chain].y_pos;
  }

  /* Then handle all right-to-left connections */
  for (i = buffer->in_length; i > 0; i--)
  {
    j = i - 1;

    if (positions[j].cursive_chain < 0)
      positions[j].y_pos += positions[j - positions[j].cursive_chain].y_pos;
  }

  return FT_Err_Ok;
}


FT_Error  HB_GPOS_Add_Feature( HB_GPOSHeader*  gpos,
			       FT_UShort        feature_index,
			       FT_UInt          property )
{
  FT_UShort    i;

  HB_Feature  feature;
  FT_UInt*     properties;
  FT_UShort*   index;
  FT_UShort    lookup_count;

  /* Each feature can only be added once */

  if ( !gpos ||
       feature_index >= gpos->FeatureList.FeatureCount ||
       gpos->FeatureList.ApplyCount == gpos->FeatureList.FeatureCount )
    return FT_Err_Invalid_Argument;

  gpos->FeatureList.ApplyOrder[gpos->FeatureList.ApplyCount++] = feature_index;

  properties = gpos->LookupList.Properties;

  feature = gpos->FeatureList.FeatureRecord[feature_index].Feature;
  index   = feature.LookupListIndex;
  lookup_count = gpos->LookupList.LookupCount;

  for ( i = 0; i < feature.LookupListCount; i++ )
  {
    FT_UShort lookup_index = index[i];
    if (lookup_index < lookup_count)
      properties[lookup_index] |= property;
  }

  return FT_Err_Ok;
}



FT_Error  HB_GPOS_Clear_Features( HB_GPOSHeader*  gpos )
{
  FT_UShort i;

  FT_UInt*  properties;


  if ( !gpos )
    return FT_Err_Invalid_Argument;

  gpos->FeatureList.ApplyCount = 0;

  properties = gpos->LookupList.Properties;

  for ( i = 0; i < gpos->LookupList.LookupCount; i++ )
    properties[i] = 0;

  return FT_Err_Ok;
}



FT_Error  HB_GPOS_Register_Glyph_Function( HB_GPOSHeader*    gpos,
					   HB_GlyphFunction  gfunc )
{
  if ( !gpos )
    return FT_Err_Invalid_Argument;

  gpos->gfunc = gfunc;

  return FT_Err_Ok;
}



FT_Error  HB_GPOS_Register_MM_Function( HB_GPOSHeader*  gpos,
					HB_MMFunction   mmfunc,
					void*            data )
{
  if ( !gpos )
    return FT_Err_Invalid_Argument;

  gpos->mmfunc = mmfunc;
  gpos->data   = data;

  return FT_Err_Ok;
}

/* If `dvi' is TRUE, glyph contour points for anchor points and device
   tables are ignored -- you will get device independent values.         */


FT_Error  HB_GPOS_Apply_String( FT_Face            face,
				HB_GPOSHeader*    gpos,
				FT_UShort          load_flags,
				HB_Buffer         buffer,
				FT_Bool            dvi,
				FT_Bool            r2l )
{
  FT_Error       error, retError = HB_Err_Not_Covered;
  GPOS_Instance  gpi;
  FT_UShort      i, j, feature_index, lookup_count;
  HB_Feature    feature;

  if ( !face || !gpos ||
       !buffer || buffer->in_length == 0 || buffer->in_pos >= buffer->in_length )
    return FT_Err_Invalid_Argument;

  gpi.face       = face;
  gpi.gpos       = gpos;
  gpi.load_flags = load_flags;
  gpi.r2l        = r2l;
  gpi.dvi        = dvi;

  lookup_count = gpos->LookupList.LookupCount;

  for ( i = 0; i < gpos->FeatureList.ApplyCount; i++ )
  {
    /* index of i'th feature */
    feature_index = gpos->FeatureList.ApplyOrder[i];
    feature = gpos->FeatureList.FeatureRecord[feature_index].Feature;

    for ( j = 0; j < feature.LookupListCount; j++ )
    {
      FT_UShort lookup_index = feature.LookupListIndex[j];

      /* Skip nonexistant lookups */
      if (lookup_index >= lookup_count)
       continue;

      error = GPOS_Do_String_Lookup( &gpi, lookup_index, buffer );
      if ( error )
      {
	if ( error != HB_Err_Not_Covered )
	  return error;
      }
      else
	retError = error;
    }
  }

  error = Position_CursiveChain ( buffer );
  if ( error )
    return error;

  return retError;
}

/* END */
