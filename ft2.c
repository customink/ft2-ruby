/************************************************************************/
/* Copyright (c) 2002 Paul Duncan                                       */
/*                                                                      */
/* Permission is hereby granted, free of charge, to any person          */
/* obtaining a copy of this software and associated documentation files */
/* (the "Software"), to deal in the Software without restriction,       */
/* including without limitation the rights to use, copy, modify, merge, */
/* publish, distribute, sublicense, and/or sell copies of the Software, */
/* and to permit persons to whom the Software is furnished to do so,    */
/* subject to the following conditions:                                 */
/*                                                                      */
/* The above copyright notice and this permission notice shall be       */
/* included in all copies of the Software, its documentation and        */
/* marketing & publicity materials, and acknowledgment shall be given   */
/* in the documentation, materials and software packages that this      */
/* Software was used.                                                   */
/*                                                                      */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,      */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF   */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND                */
/* NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY     */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE    */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.               */
/************************************************************************/

#include <ruby.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

#define VERSION "0.1.0"
#define UNUSED(a) ((void) (a))
#define ABS(a) (((a) < 0) ? -(a) : (a))

/* convert to and from FT2 fixed values */
#define FTFIX2DBL(a) ((double) (a) / 0x10000)
#define DBL2FTFIX(a) ((double) (a) * 0x10000)

static FT_Library library;
static VALUE mFt2,
             mBBox,         /* GlyphBBox */
             mEnc,          /* encoding */
             mGlyphFormat,
             mKerningMode,
             mLoad,
             /* mPaletteMode, */
             mPixelMode,
             mRenderMode,

             cBitmap,
             cBitmapGlyph,
             cCharMap,
             cFace,
             cGlyph,
             cGlyphClass,
             cGlyphSlot,
             cGlyphMetrics,
             cLibrary,
             cMemory,
             cOutline,
             cOutlineGlyph,
             /* cRaster, */
             cSubGlyph,
             cSize,
             cSizeMetrics;

static void face_free(void *ptr);
static void glyph_free(void *ptr);

static void dont_free(void *ptr) { UNUSED(ptr); }

static void handle_error(FT_Error err) {
#undef __FTERRORS_H__
#define FT_ERRORDEF( e, v, s )  { e, s },
#define FT_ERROR_START_LIST     {
#define FT_ERROR_END_LIST       { 0, 0 } };
  int i = 0;
  static const struct {
    int          code;
    const char  *str;
  } errors[] =
#include FT_ERRORS_H

  for (i = 0; ((unsigned int) i) < sizeof(errors) / sizeof(errors[0]); i++)
    if (err == errors[i].code)
      rb_raise(rb_eException, "FreeType2 Error: %s.", errors[i].str);

  rb_raise(rb_eException, "FreeType2 Error: Unknown error %d.", err);
}

static double ft_fixed_to_double(FT_Fixed fixed) {
  return ((0xffff0000 & fixed) >> 16) +
         ((double) (0xffff & fixed) / 0xffff);
}

static VALUE ft_version(VALUE klass) {
  char buf[1024];
  FT_Int ver[3];
  UNUSED(klass);

  FT_Library_Version(library, &(ver[0]), &(ver[1]), &(ver[2]));

  snprintf(buf, sizeof(buf), "%d.%d.%d", ver[0], ver[1], ver[2]);
  return rb_str_new2(buf);
}


/***********************/
/* FT2::Bitmap methods */
/***********************/

/*
 * Constructor for FT2::Bitmap.
 *
 * This method is currently empty.  You should never call this method
 * directly unless you're instantiating a derived class (ie, you know
 * what you're doing).
 *
 */
static VALUE ft_bitmap_init(VALUE self) {
  return self;
}

/*
 * Return the number of rows in a FT2::Bitmap object.
 *
 * Examples:
 *   rows = bitmap.rows
 *
 */
static VALUE ft_bitmap_rows(VALUE self) {
  FT_Bitmap *bitmap;
  Data_Get_Struct(self, FT_Bitmap, bitmap);
  return INT2FIX(bitmap->rows);
}

/*
 * Return the width of an FT2::Bitmap object.
 *
 * Examples:
 *   width = bitmap.width
 *
 */
static VALUE ft_bitmap_width(VALUE self) {
  FT_Bitmap *bitmap;
  Data_Get_Struct(self, FT_Bitmap, bitmap);
  return INT2FIX(bitmap->width);
}

/*
 * Return the pitch (bytes per row) of an FT2::Bitmap object.
 *
 * Return the pitch (bytes per row, including alignment padding) of an
 * FT2::Bitmap object.
 * 
 * Examples:
 *   width = bitmap.width
 *
 */
static VALUE ft_bitmap_pitch(VALUE self) {
  FT_Bitmap *bitmap;
  Data_Get_Struct(self, FT_Bitmap, bitmap);
  return INT2FIX(bitmap->pitch);
}

/*
 * Return the buffer (e.g. raw pixel data) of an FT2::Bitmap object.
 *
 * Examples:
 *   # return the rendered pixels as a binary string
 *   buffer = bitmap.buffer
 *
 *   # assume a render method called render_font(buffer, width, height)
 *   render_font(bitmap.buffer, bitmap.width, bitmap.rows)
 *
 */
static VALUE ft_bitmap_buffer(VALUE self) {
  FT_Bitmap *bitmap;
  Data_Get_Struct(self, FT_Bitmap, bitmap);
  return rb_str_new((char*) bitmap->buffer, ABS(bitmap->pitch) * bitmap->rows);
}

/*
 * Return the number of grays in an FT2::Bitmap object.
 *
 * Note:
 *   Only used if Ft2::Bitmap#pixel_mode is FT2::PixelMode::GRAYS.
 *
 * Examples:
 *   depth = bitmap.num_grays
 *
 */
static VALUE ft_bitmap_num_grays(VALUE self) {
  FT_Bitmap *bitmap;
  Data_Get_Struct(self, FT_Bitmap, bitmap);
  return INT2FIX(bitmap->num_grays);
}

/*
 * Return the pixel mode (e.g. bit depth) of an FT2::Bitmap object.
 *
 * Note:
 *   Always returns one of the values avaiable in FT2::PixelMode (for a
 *   full list, try the following code: "p FT2::PixelMode.constants.sort")
 *
 * Examples:
 *   case bitmap.pixel_mode
 *   when FT2::PixelMode::RGB32
 *     puts "wow that's a lot of colors!"
 *   when FT2::PixelMode::MONO
 *     puts "time to get a better monitor..."
 *   end
 *
 */
static VALUE ft_bitmap_pixel_mode(VALUE self) {
  FT_Bitmap *bitmap;
  Data_Get_Struct(self, FT_Bitmap, bitmap);
  return INT2FIX(bitmap->pixel_mode);
}

/*
 * Return the palette mode of an FT2::Bitmap object.
 *
 * Note:
 *   Always returns one of the values avaiable in FT2::PaletteMode.
 *
 * Examples:
 *   case bitmap.palette_mode
 *   when FT2::PaletteMode::RGBA
 *     puts "we have an alpha channel!"
 *   when FT2::PaletteMode::RGB
 *     puts "no alpha channel for you"
 *   end
 *
 */
static VALUE ft_bitmap_palette_mode(VALUE self) {
  FT_Bitmap *bitmap;
  Data_Get_Struct(self, FT_Bitmap, bitmap);
  return INT2FIX(bitmap->palette_mode);
}

/*
 * Return the palette of an FT2::Bitmap object.
 *
 * Note:
 *   Returns a binary string of RGB or RGBA elements (check
 *   FT2::Bitmap#palette_mode to determine which).
 *
 * Examples:
 *   case bitmap.palette_mode
 *   when FT2::PaletteMode::RGBA
 *     rgba_palette_string = bitmap.palette
 *   when FT2::PaletteMode::RGB
 *     rgb_palette_string = bitmap.palette
 *   end
 *
 */
static VALUE ft_bitmap_palette(VALUE self) {
  FT_Bitmap *bitmap;
  /* int size; */
  Data_Get_Struct(self, FT_Bitmap, bitmap);

  return Qnil;
  /* FIXME i don't know how big the palette memory is, so i'll just
   * assume it's 256 entries :/
   *
   * none of this shit compiles, so I'm disabling it for now.
  if (bitmap->palette_mode == ft_palette_mode_rgb)
    size = 3 * 256;
  else if (bitmap->palette_mode == ft_palette_mode_rgba)
    size = 4 * 256;
  else
    rb_raise(rb_eException, "Unknown palette mode: %d.",
             INT2FIX(bitmap->palette_mode));

  return rb_str_new(bitmap->palette, size); */
}

/*********************/
/* FT2::Face methods */
/*********************/
static void face_free(void *ptr) {
  FT_Face  face = *((FT_Face *) ptr);
  FT_Error err;

  if ((err = FT_Done_Face(face)) != FT_Err_Ok)
    handle_error(err);
  free(ptr);
}

/*
 * Allocate and initialize a new FT2::Face object.
 *
 * Note:
 *   FreeType2-Ruby creates a new (hidden) FT2::Library instance at
 *   runtime, which FT2::Face objects are automatically initialized with
 *   if an a library is not specified.
 *
 * Aliases:
 *   FT2::Face.load
 *
 * Examples:
 *   # load font from file "yudit.ttf"
 *   face = FT2::Face.new 'yudit.ttf'
 *
 *   # load second face from from file "yudit.ttf"
 *   face = FT2::Face.new 'yudit.ttf', 1
 *
 *   # load font from file "yudit.ttf" under FT2::Library instance lib
 *   face = FT2::Face.new lib, 'yudit.ttf'
 *
 *   # load second face from file "yudit.ttf" under FT2::Library instance lib
 *   face = FT2::Face.new lib, 'yudit.ttf', 1
 */
VALUE ft_face_new(int argc, VALUE *argv, VALUE klass) {
  VALUE self, path;
  FT_Library *lib;
  FT_Face *face;
  FT_Error err;
  FT_Long face_index;

  lib = &library;
  switch (argc) {
    case 1:
      path = argv[0];
      face_index = 0;
      break;
    case 2:
      if (rb_obj_is_kind_of(argv[0], rb_cString)) {
        path = argv[0];
        face_index = NUM2INT(argv[1]);
      } else if (rb_obj_is_kind_of(argv[0], cLibrary)) {
        Data_Get_Struct(argv[0], FT_Library, lib);
        path = argv[0];
        face_index = 0;
      } else {
        rb_raise(rb_eArgError, "Invalid first argument.");
      }
      break;
    case 3:
      Data_Get_Struct(argv[0], FT_Library, lib);
      path = argv[1];
      face_index = NUM2INT(argv[1]);
      break;
    default:
      rb_raise(rb_eArgError, "Invalid argument count: %d.", argc);
  }

  face = malloc(sizeof(FT_Face));
  err = FT_New_Face(*lib, RSTRING(path)->ptr, face_index, face);
  if (err != FT_Err_Ok)
    handle_error(err);

  self = Data_Wrap_Struct(klass, 0, face_free, face);
  rb_obj_call_init(self, 0, NULL);

  return self;
}

/*
 * Allocate and initialize a new FT2::Face object from in-memory buffer.
 *
 * Note:
 *   FreeType2-Ruby creates a new (hidden) FT2::Library instance at
 *   runtime, which FT2::Face objects are automatically initialized with
 *   if an a library is not specified.
 *
 * Examples:
 *   # load font from string _buffer_
 *   face = FT2::Face.new buffer, buffer_size
 *
 *   # load second face from string _buffer_
 *   face = FT2::Face.new buffer, buffer_size, 1
 *
 *   # load font from string _buffer_ under FT2::Library instance _lib_
 *   face = FT2::Face.new lib, buffer, buffer_size
 *
 *   # load second face from string _buffer_ under FT2::Library instance _lib_
 *   face = FT2::Face.new lib, buffer, buffer_size, 1
 */
VALUE ft_face_new_from_memory(int argc, VALUE *argv, VALUE klass) {
  VALUE self;
  FT_Library *lib;
  FT_Face *face;
  FT_Error err;
  FT_Long face_index;
  void *mem;
  int len;

  lib = &library;
  switch (argc) {
    case 2:
      mem = RSTRING(argv[0])->ptr;
      len = NUM2INT(argv[1]);
      face_index = 0;
      break;
    case 3:
      if (rb_obj_is_kind_of(argv[0], rb_cString)) {
        mem = RSTRING(argv[0])->ptr;
        len = NUM2INT(argv[1]);
        face_index = NUM2INT(argv[2]);
      } else if (rb_obj_is_kind_of(argv[0], cLibrary)) {
        Data_Get_Struct(argv[0], FT_Library, lib);
        mem = RSTRING(argv[1])->ptr;
        len = NUM2INT(argv[2]);
        face_index = 0;
      } else {
        rb_raise(rb_eArgError, "Invalid first argument.");
      }
      break;
    case 4:
      Data_Get_Struct(argv[0], FT_Library, lib);
      mem = RSTRING(argv[1])->ptr;
      len = NUM2INT(argv[2]);
      face_index = NUM2INT(argv[3]);
      break;
    default:
      rb_raise(rb_eArgError, "Invalid argument count: %d.", argc);
  }

  face = malloc(sizeof(FT_Face));
  err = FT_New_Memory_Face(*lib, mem, len, face_index, face);
  if (err != FT_Err_Ok)
    handle_error(err);

  self = Data_Wrap_Struct(klass, 0, face_free, face);
  rb_obj_call_init(self, 0, NULL);

  return self;
}

/*
 * Constructor for FT2::Face.
 *
 * This method is currently empty.  You should never call this method
 * directly unless you're instantiating a derived class (ie, you know
 * what you're doing).
 *
 */
static VALUE ft_face_init(VALUE self) {
  return self;
}

/*
 * Return the number of faces in an FT2::Face object.
 *
 * Aliases:
 *   FT2::Face#num_faces
 *
 * Examples:
 *   num_faces = face.faces
 *
 */
static VALUE ft_face_faces(VALUE self) {
  FT_Face *face;
  Data_Get_Struct(self, FT_Face, face);
  return INT2FIX((int) (*face)->num_faces);
}

/*
 * Return the index of an FT2::Face object in its font file.
 *
 * Note:
 *   This is almost always zero.
 *
 * Aliases:
 *   FT2::Face#face_index
 *
 * Examples:
 *   index = face.index
 *
 */
static VALUE ft_face_index(VALUE self) {
  FT_Face *face;
  Data_Get_Struct(self, FT_Face, face);
  return INT2FIX((int) (*face)->face_index);
}

/*
 * Return the face flags of an FT2::Face object.
 *
 * You can binary OR this with any of the following values to obtain the
 * value of that flag (note that this is returned as an Integer and not
 * as a boolean value; eg non-zero for true and zero for false).
 *
 * Face Flags:
 *   FT2::Face::SCALABLE
 *   FT2::Face::FIXED_SIZES
 *   FT2::Face::FIXED_WIDTH
 *   FT2::Face::FIXED_HORIZONTAL
 *   FT2::Face::FIXED_VERTICAL
 *   FT2::Face::SFNT
 *   FT2::Face::KERNING
 *   FT2::Face::MULTIPLE_MASTERS
 *   FT2::Face::GLYPH_NAMES
 *   FT2::Face::EXTERNAL_STREAM
 *   FT2::Face::FAST_GLYPHS
 *
 * Alternatively, if you're only checking one flag, it's slightly faster
 * (and arguably more concise), to use the following flag methods, which
 * DO return true or false.
 *
 * Individual Flag Methods:
 *   FT2::Face#scalable?
 *   FT2::Face#fixed_sizes?
 *   FT2::Face#fixed_width?
 *   FT2::Face#horizontal?
 *   FT2::Face#vertical?
 *   FT2::Face#sfnt?
 *   FT2::Face#kerning?
 *   FT2::Face#external_stream?
 *   FT2::Face#fast_glyphs?
 * 
 * Aliases:
 *   FT2::Face#face_flags
 *
 * Examples:
 *   if (face.flags & (FT2::Face::FAST_GLYPHS | FT2::Face::KERNING) != 0)
 *     puts 'face contains fast glyphs and kerning information'
 *   end
 *
 */
static VALUE ft_face_flags(VALUE self) {
  FT_Face *face;
  Data_Get_Struct(self, FT_Face, face);
  return INT2FIX((int) (*face)->face_flags);
}

/*
 * Is this FT2::Face scalable?
 *
 * Examples:
 *   puts "is scalable" if face.scalable?
 *
 */
static VALUE ft_face_flag_scalable(VALUE self) {
  FT_Face *face;
  Data_Get_Struct(self, FT_Face, face);
  return ((*face)->face_flags & FT_FACE_FLAG_SCALABLE) ? Qtrue : Qfalse;
}

/* 
 * Does this FT2::Face contain bitmap strikes for some pixel sizes?
 *
 * Examples:
 *   puts "contains fixed sized glyphs" if face.fixed_sizes?
 *
 */
static VALUE ft_face_flag_fixed_sizes(VALUE self) {
  FT_Face *face;
  Data_Get_Struct(self, FT_Face, face);
  return ((*face)->face_flags & FT_FACE_FLAG_FIXED_SIZES) ? Qtrue : Qfalse;
}

/*
 * Does this FT2::Face contain fixed with characters?
 *
 * Examples:
 *   puts "contains fixed width characters" if face.fixed_width?
 *
 */
static VALUE ft_face_flag_fixed_width(VALUE self) {
  FT_Face *face;
  Data_Get_Struct(self, FT_Face, face);
  return ((*face)->face_flags & FT_FACE_FLAG_FIXED_WIDTH) ? Qtrue : Qfalse;
}

/*
 * Does this FT2::Face contain horizontal glyph metrics? 
 *
 * Note:
 *   This flag is true for virtually all fonts.
 *
 * Examples:
 *   puts "contains horizontal glyph metrics" if face.horizontal?
 *
 */
static VALUE ft_face_flag_horizontal(VALUE self) {
  FT_Face *face;
  Data_Get_Struct(self, FT_Face, face);
  return ((*face)->face_flags & FT_FACE_FLAG_HORIZONTAL) ? Qtrue : Qfalse;
}

/*
 * Does this FT2::Face contain vertical glyph metrics? 
 *
 * Note:
 *   If this flag is not set, the glyph loader will synthesize vertical
 *   metrics itself.
 *
 * Examples:
 *   puts "contains vertical glyph metrics" if face.vertical?
 *
 */
static VALUE ft_face_flag_vertical(VALUE self) {
  FT_Face *face;
  Data_Get_Struct(self, FT_Face, face);
  return ((*face)->face_flags & FT_FACE_FLAG_VERTICAL) ? Qtrue : Qfalse;
}

/*
 * Is this FT2::Face stored in the 'sfnt' storage format? 
 *
 * Note:
 *   This currently means the file was either TrueType or OpenType.
 *
 * Examples:
 *   puts "sfnt format font" if face.sfnt?
 *
 */
static VALUE ft_face_flag_sfnt(VALUE self) {
  FT_Face *face;
  Data_Get_Struct(self, FT_Face, face);
  return ((*face)->face_flags & FT_FACE_FLAG_SFNT) ? Qtrue : Qfalse;
}

/*
 * Does this FT2::Face contain kerning information? 
 *
 * Examples:
 *   puts "face contains kerning information" if face.kerning?
 *
 */
static VALUE ft_face_flag_kerning(VALUE self) {
  FT_Face *face;
  Data_Get_Struct(self, FT_Face, face);
  return ((*face)->face_flags & FT_FACE_FLAG_KERNING) ? Qtrue : Qfalse;
}

/*
 * Was this FT2::Face loaded from an external stream?
 *
 * Examples:
 *   puts "face loaded from external stream" if face.external_stream?
 *
 */
static VALUE ft_face_flag_external_stream(VALUE self) {
  FT_Face *face;
  Data_Get_Struct(self, FT_Face, face);
  return ((*face)->face_flags & FT_FACE_FLAG_EXTERNAL_STREAM) ? Qtrue : Qfalse;
}

/*
 * Does this FT2::Face contain fast glyphs? 
 *
 * Note:
 *   This flag is usually set for fixed-size formats like FNT.
 * 
 * Examples:
 *   puts "face contains fast glyphs" if face.fast_glyphs?
 *
 */
static VALUE ft_face_flag_fast_glyphs(VALUE self) {
  FT_Face *face;
  Data_Get_Struct(self, FT_Face, face);
  return ((*face)->face_flags & FT_FACE_FLAG_FAST_GLYPHS) ? Qtrue : Qfalse;
}

/*
 * Return the style flags of an FT2::Face object.
 *
 * You can binary OR this with any of the following values to obtain the
 * value of that style flag (note that this is returned as an Integer
 * and not as a boolean value; eg non-zero for true and zero for false).
 *
 * Style Flags:
 *   FT2::Face::BOLD
 *   FT2::Face::ITALIC
 *
 * Alternatively, if you're only checking one style flag, it's slightly
 * faster (and arguably more concise), to use the following style flag
 * methods, which DO return true or false.
 *
 * Individual Style Flag Methods:
 *   FT2::Face#bold?
 *   FT2::Face#italic?
 *
 * Examples:
 *   style = face.style_flags
 *
 */
static VALUE ft_face_style_flags(VALUE self) {
  FT_Face *face;
  Data_Get_Struct(self, FT_Face, face);
  return INT2FIX((int) (*face)->style_flags);
}

/*
 * Is this a bold FT2::Face? 
 *
 * Examples:
 *   puts "bold face" if face.bold?
 *
 */
static VALUE ft_face_flag_bold(VALUE self) {
  FT_Face *face;
  Data_Get_Struct(self, FT_Face, face);
  return ((*face)->style_flags & FT_STYLE_FLAG_BOLD) ? Qtrue : Qfalse;
}

/*
 * Is this an italic FT2::Face? 
 *
 * Examples:
 *   puts "italic face" if face.italic?
 *
 */
static VALUE ft_face_flag_italic(VALUE self) {
  FT_Face *face;
  Data_Get_Struct(self, FT_Face, face);
  return ((*face)->style_flags & FT_STYLE_FLAG_ITALIC) ? Qtrue : Qfalse;
}

/*
 * Return the number of glyphs in an FT2::Face object. 
 *
 * Aliases:
 *   FT2::Face#num_glyphs
 *
 * Examples:
 *   count = face.glyphs
 *   puts "face has #{count.to_str} glyphs"
 *
 */
static VALUE ft_face_glyphs(VALUE self) {
  FT_Face *face;
  Data_Get_Struct(self, FT_Face, face);
  return INT2FIX((int) (*face)->num_glyphs);
}

/*
 * Return the family name of an FT2::Face object. 
 *
 * Description: 
 *   This is an ASCII string, usually in English, which describes the
 *   FT2::Face object's family (eg "Times New Roman" or "Geneva"). Some
 *   formats (eg Truetype and OpenType) provide localized and Unicode
 *   versions of this string, which are accessable via the format specific
 *   interfaces.
 * 
 * Examples:
 *   puts 'family: ' << face.family
 *
 */
static VALUE ft_face_family(VALUE self) {
  FT_Face *face;
  Data_Get_Struct(self, FT_Face, face);
  return rb_str_new2((*face)->family_name);
}

/*
 * Return the style name of an FT2::Face object. 
 *
 * Description:
 *   This is an ASCII string, usually in English, which describes the
 *   FT2::Face object's style (eg "Bold", "Italic", "Condensed", etc).
 *   This field is optional and may be set to nil.
 * 
 * Examples:
 *   puts 'style: ' << face.style if face.style
 *
 */
static VALUE ft_face_style(VALUE self) {
  FT_Face *face;
  Data_Get_Struct(self, FT_Face, face);
  if (!(*face)->style_name)
    return Qnil;
  return rb_str_new2((*face)->style_name);
}

/*
 * Return the number of fixed sizes in an FT2::Face object. 
 *
 * Note:
 *   This should be set to 0 for scalable fonts, unless the FT2::Face
 *   object contains a complete set of glyphs for the specified size.
 * 
 * Aliases:
 *   FT2::Face#num_fixed_sizes
 *
 * Examples:
 *   puts 'fized sizes count: ' << face.fixed_sizes
 *
 */
static VALUE ft_face_fixed_sizes(VALUE self) {
  FT_Face *face;
  Data_Get_Struct(self, FT_Face, face);
  return INT2FIX((int) (*face)->num_fixed_sizes);
}

/*
 * Return an array of sizes in an FT2::Face object. 
 *
 * Note:
 *   This method does not currently work..
 * 
 * Examples:
 *   face.available_sizesdflksjaflksdjf FIXME
 *
 */
static VALUE ft_face_available_sizes(VALUE self) {
  FT_Face *face;
  Data_Get_Struct(self, FT_Face, face);
  /* FIXME!! */
  return INT2FIX((*face)->available_sizes);
}

/*
 * Return the number of charmaps in an FT2::Face object.
 *
 * Examples:
 *   puts 'number of charmaps: ' << face.num_charmaps
 *
 */
static VALUE ft_face_num_charmaps(VALUE self) {
  FT_Face *face;
  Data_Get_Struct(self, FT_Face, face);
  return INT2FIX((int) (*face)->num_charmaps);
}

/*
 * Return an array of charmaps in an FT2::Face object.
 * 
 * Note:
 *   This method may not work correctly at the moment (FIXME).
 *
 * Examples:
 *   face.charmaps.each { |map| puts map.to_str }
 *
 */
static VALUE ft_face_charmaps(VALUE self) {
  FT_Face *face;
  VALUE ary;
  int i;
  Data_Get_Struct(self, FT_Face, face);

  /* FIXME */
  rb_bug("not implemented yet");
  ary = rb_ary_new();
  for (i = 0; i < (*face)->num_charmaps; i++)
    rb_ary_push(ary, INT2FIX(i));
  
  return ary;
}

/*
 * Return the bounding box of an FT2::Face object.
 * 
 * Note:
 *   This method is not currently implemented (FIXME).
 *
 * Examples:
 *   FIXME
 *
 */
static VALUE ft_face_bbox(VALUE self) {
  UNUSED(self);
  /* FIXME */
  rb_bug("not implemented yet");
  return Qnil;
}

/*
 * Return the number of font units per EM for this FT2::Face object.
 * 
 * Description:
 *   This value is typically 2048 for TrueType fonts, 1000 for Type1
 *   fonts, and should be set to the (unrealistic) value 1 for
 *   fixed-size fonts.
 *
 * Aliases:
 *   FT2::Face#units_per_EM
 *
 * Examples:
 *   em = face.units_per_em
 *
 */
static VALUE ft_face_units_per_em(VALUE self) {
  FT_Face *face;
  Data_Get_Struct(self, FT_Face, face);
  return INT2FIX((int) (*face)->units_per_EM);
}

/*
 * Return the ascender for this FT2::Face object.
 * 
 * Description:
 *   An FT2::Face object's ascender is the vertical distance, in font
 *   units, from the baseline to the topmost point of any glyph in the
 *   face.
 *
 * Examples:
 *   asc = face.ascender
 *
 */
static VALUE ft_face_ascender(VALUE self) {
  FT_Face *face;
  Data_Get_Struct(self, FT_Face, face);
  return INT2FIX((int) (*face)->ascender);
}

/*
 * Return the descender for this FT2::Face object.
 * 
 * Description:
 *   An FT2::Face object's descender is the vertical distance, in font
 *   units, from the baseline to the bottommost point of any glyph in
 *   the face.
 *
 * Examples:
 *   asc = face.descender
 *
 */
static VALUE ft_face_descender(VALUE self) {
  FT_Face *face;
  Data_Get_Struct(self, FT_Face, face);
  return INT2FIX((int) (*face)->descender);
}

/*
 * Return the height of this FT2::Face object.
 * 
 * Description:
 *   An FT2::Face object's height is the vertical distance, in font
 *   units, from the baseline of one line to the baseline of the next.
 *   The value can be computed as 'ascender + descender + line_gap',
 *   where 'line_gap' is also called 'external leading'.
 *
 * Examples:
 *   h = face.height
 *
 */
static VALUE ft_face_height(VALUE self) {
  FT_Face *face;
  Data_Get_Struct(self, FT_Face, face);
  return INT2FIX((int) (*face)->height);
}

/*
 * Return the maximal advance width of this FT2::Face object.
 * 
 * Description:
 *   The maximal advance width, in font units, for all glyphs in this
 *   FT2::Face object.  This can be used to make word-wrapping
 *   computations faster.  Only relevant for scalable formats.
 *
 * Examples:
 *   maw = face.max_advance_width
 *
 */
static VALUE ft_face_max_advance_width(VALUE self) {
  FT_Face *face;
  Data_Get_Struct(self, FT_Face, face);
  return INT2FIX((int) (*face)->max_advance_width);
}

/*
 * Return the maximal advance height of this FT2::Face object.
 * 
 * Description:
 *   The maximal advance height, in font units, for all glyphs in this
 *   FT2::Face object.  This can be used to make word-wrapping
 *   computations faster.  Only relevant for scalable formats.
 *
 * Examples:
 *   mah = face.max_advance_height
 *
 */
static VALUE ft_face_max_advance_height(VALUE self) {
  FT_Face *face;
  Data_Get_Struct(self, FT_Face, face);
  return INT2FIX((int) (*face)->max_advance_height);
}

/*
 * Return the underline position of this FT2::Face object.
 * 
 * Description:
 *   The position, in font units, of the underline line for this face.
 *   It's the center of the underlining stem. Only relevant for scalable
 *   formats.
 *
 * Examples:
 *   uh = face.underline_position
 *
 */
static VALUE ft_face_underline_position(VALUE self) {
  FT_Face *face;
  Data_Get_Struct(self, FT_Face, face);
  return INT2FIX((int) (*face)->underline_position);
}

/*
 * Return the underline thickness of this FT2::Face object.
 * 
 * Description:
 *  The thickness, in font units, of the underline for this face. Only
 *  relevant for scalable formats.
 *
 * Examples:
 *   ut = face.underline_thickness
 *
 */
static VALUE ft_face_underline_thickness(VALUE self) {
  FT_Face *face;
  Data_Get_Struct(self, FT_Face, face);
  return INT2FIX((int) (*face)->underline_thickness);
}
/*
 * Return the glyph slot associated with this FT2::Face object.
 * 
 * Description:
 *  The face's associated glyph slot(s) (a FT2::GlyphSlot). This object
 *  is created automatically with a new FT2::Face object. However,
 *  certain kinds of applications (mainly tools like converters) can
 *  need more than one slot to ease their task.
 *
 * Examples:
 *   glyph = face.glyph
 *
 */
static VALUE ft_face_glyph(VALUE self) {
  FT_Face *face;
  Data_Get_Struct(self, FT_Face, face);

  if ((*face)->glyph) 
    return Data_Wrap_Struct(cGlyphSlot, 0, dont_free, &((*face)->glyph));
  else
    return Qnil;
}

/*
 * Return the current active size of this FT2::Face object.
 * 
 * Examples:
 *   size = face.size
 *
 */
static VALUE ft_face_size(VALUE self) {
  FT_Face *face;
  Data_Get_Struct(self, FT_Face, face);

  if ((*face)->size) 
    return Data_Wrap_Struct(cSize, 0, dont_free, (*face)->size);
  else
    return Qnil;
}

/*
 * Return the current active FT2::CharMap of this FT2::Face object.
 * 
 * Examples:
 *   size = face.size
 *
 */
static VALUE ft_face_charmap(VALUE self) {
  FT_Face *face;
  Data_Get_Struct(self, FT_Face, face);

  if ((*face)->charmap) 
    return Data_Wrap_Struct(cCharMap, 0, dont_free, (*face)->charmap);
  else
    return Qnil;
}

/* 
 * Attach a font file to this FT2::Face object.
 *
 * Description:
 *   This is usually to read additional information for a single face
 *   object. For example, it is used to read the AFM files that come
 *   with Type 1 fonts in order to add kerning data and other metrics.
 *   Throws an exception if the font file could not be loaded.
 *   FreeType2 also supports loading from an input stream, but this
 *   feature is not implemented in FT2-Ruby.
 *
 * Examples:
 *   path = 'fonts'yudit.ttf'
 *   begin
 *     face.attach path  # attach file "fonts/yudit.ttf"
 *   rescue Exception
 *     $stderr.puts "Couldn't open font file \"#{path}\": " << $!
 *   end
 *
 */
static VALUE ft_face_attach(VALUE self, VALUE path) {
  FT_Face *face;
  FT_Error err;
  Data_Get_Struct(self, FT_Face, face);
  if ((err = FT_Attach_File(*face, RSTRING(path)->ptr)) != FT_Err_Ok)
    handle_error(err);
  return self;
}

/* 
 * Set the character dimensions of this FT2::Face object.
 *
 * Description:
 *   Sets the character dimensions of a FT2::Face object. The
 *   `char_width' and `char_height' values are used for the width and
 *   height, respectively, expressed in 26.6 fractional points.
 *
 *   If the horizontal or vertical resolution values are zero, a default
 *   value of 72dpi is used. Similarly, if one of the character
 *   dimensions is zero, its value is set equal to the other.
 *
 *   When dealing with fixed-size faces (i.e., non-scalable formats),
 *   use the function FT2::Face#set_pixel_sizes .
 *
 * Examples:
 *   face.set_char_size char_width, char_height, horiz_res, vert_res
 *
 */
static VALUE ft_face_set_char_size(VALUE self, VALUE c_w, VALUE c_h, VALUE h_r, VALUE v_r) {
  FT_Face *face;
  FT_Error err;
  Data_Get_Struct(self, FT_Face, face);
  err = FT_Set_Char_Size(*face, NUM2DBL(c_w), NUM2DBL(c_h), NUM2INT(h_r), NUM2INT(v_r));
  if (err != FT_Err_Ok)
    handle_error(err);
  return self;
}

/* 
 * Set the character dimensions of this FT2::Face object.
 *
 * Description:
 *   Sets the character dimensions of a FT2::Face object. The width and
 *   height are expressed in integer pixels.
 *
 *   If one of the character dimensions is zero, its value is set equal
 *   to the other.
 *    
 *   The values of `pixel_width' and `pixel_height' correspond to the
 *   pixel values of the typographic character size, which are NOT
 *   necessarily the same as the dimensions of the glyph `bitmap cells'.
 *
 *   The `character size' is really the size of an abstract square
 *   called the `EM', used to design the font. However, depending on the
 *   font design, glyphs will be smaller or greater than the EM.
 *
 *   This means that setting the pixel size to, say, 8x8 doesn't
 *   guarantee in any way that you will get glyph bitmaps that all fit
 *   within an 8x8 cell (sometimes even far from it).
 *   
 * Examples:
 *   face.set_pixel_sizes pixel_width, pixel_height
 *
 */
static VALUE ft_face_set_pixel_sizes(VALUE self, VALUE pixel_w, VALUE pixel_h) {
  FT_Face *face;
  FT_Error err;
  Data_Get_Struct(self, FT_Face, face);
  err = FT_Set_Pixel_Sizes(*face, NUM2INT(pixel_w), NUM2INT(pixel_h));
  if (err != FT_Err_Ok)
    handle_error(err);
  return self;
}

/* 
 * Set the pre-render transoformation matrix and vector of a FT2::Face object.
 *
 * Description:
 *   Used to set the transformation that is applied to glyph images just
 *   before they are converted to bitmaps in a FT2::GlyphSlot when
 *   FT2::GlyphSlot#render is called.
 *   
 *   matrix: The transformation's 2x2 matrix. Use nil for the identity
 *           matrix.
 *   delta: The translation vector. Use nil for the null vector.
 *     
 * Note:
 *   The transformation is only applied to scalable image formats after
 *   the glyph has been loaded. It means that hinting is unaltered by
 *   the transformation and is performed on the character size given in
 *   the last call to FT2::Face#set_char_sizes or
 *   FT2::Face#set_pixel_sizes.
 *
 * Examples:
 *   matrix = [[0, 1],
 *             [0, 1]]
 *   vector = nil
 *
 *   face.set_transform matrix, vector
 *
 */
static VALUE ft_face_set_transform(VALUE self, VALUE matrix, VALUE delta) {
  FT_Face *face;
  FT_Matrix m;
  FT_Vector v;

  Data_Get_Struct(self, FT_Face, face);

  if (matrix != Qnil) {
    /* FIXME: do I have these reversed? */
    m.xx = DBL2FTFIX(NUM2DBL(rb_ary_entry(rb_ary_entry(matrix, 0), 0)));
    m.xy = DBL2FTFIX(NUM2DBL(rb_ary_entry(rb_ary_entry(matrix, 1), 0)));
    m.yx = DBL2FTFIX(NUM2DBL(rb_ary_entry(rb_ary_entry(matrix, 0), 1)));
    m.yy = DBL2FTFIX(NUM2DBL(rb_ary_entry(rb_ary_entry(matrix, 1), 1)));
  }

  if (delta != Qnil) {
    v.x = NUM2INT(rb_ary_entry(delta, 0));
    v.y = NUM2INT(rb_ary_entry(delta, 1));
  }

  if (matrix != Qnil && delta != Qnil)
    FT_Set_Transform(*face, &m, &v);
  else if (matrix == Qnil && delta != Qnil)
    FT_Set_Transform(*face, NULL, &v);
  else if (matrix != Qnil && delta == Qnil)
    FT_Set_Transform(*face, &m, NULL);
  else
    FT_Set_Transform(*face, NULL, NULL);

  return self;
}

/*
 * Load a glyph at a given size into a glyph slot of a FT2::Face object.
 * 
 * Description:
 *   Load a glyph at a given size into a glyph slot of a FT2::Face
 *   object.
 *
 *   glyph_index: The index of the glyph in the font file.
 *   load_flags: A flag indicating what to load for this glyph. The
 *               FT2::Load::XXXX constants can be used to control the
 *               glyph loading process (e.g., whether the outline should
 *               be scaled, whether to load bitmaps or not, whether to
 *               hint the outline, etc).
 *
 * Note:
 *   If the glyph image is not a bitmap, and if the bit flag
 *   FT2::Load::IGNORE_TRANSFORM is unset, the glyph image will be
 *   transformed with the information passed to a previous call to
 *   FT2::Face#set_transform
 *
 *   Note that this also transforms the `face.glyph.advance' field, but
 *   not the values in `face.glyph.metrics'.
 *
 * Load Flags:
 *   FT2::Load::DEFAULT
 *   FT2::Load::RENDER
 *   FT2::Load::MONOCHROME
 *   FT2::Load::LINEAR_DESIGN
 *   FT2::Load::NO_SCALE
 *   FT2::Load::NO_HINTING
 *   FT2::Load::NO_BITMAP
 *   FT2::Load::CROP_BITMAP
 *   FT2::Load::VERTICAL_LAYOUT
 *   FT2::Load::IGNORE_TRANSFORM
 *   FT2::Load::IGNORE_GLOBAL_ADVANCE_WIDTH
 *   FT2::Load::FORCE_AUTOHINT
 *   FT2::Load::NO_RECURSE
 *   FT2::Load::PEDANTIC
 *     
 * Examples:
 *   face.load_glyph 5, FT2::Load::DEFAULT
 *
 */
static VALUE ft_face_load_glyph(VALUE self, VALUE glyph_index, VALUE flags) {
  FT_Face *face;
  FT_Error err;

  Data_Get_Struct(self, FT_Face, face);
  if (flags == Qnil)
    flags = INT2FIX(FT_LOAD_DEFAULT);
  
  err = FT_Load_Glyph(*face, NUM2INT(glyph_index), NUM2INT(flags));
  if (err != FT_Err_Ok)
    handle_error(err);

  return self;
}

/*
 * Load a glyph at a given size into a glyph slot of a FT2::Face object.
 * 
 * Description:
 *   Load a glyph at a given size into a glyph slot of a FT2::Face
 *   object according to its character code.
 *   
 *   char_code: The glyph's character code, according to the current
 *              charmap used in the FT2::Face object.
 *   load_flags: A flag indicating what to load for this glyph. The
 *               FT2::Load::XXXX constants can be used to control the
 *               glyph loading process (e.g., whether the outline should
 *               be scaled, whether to load bitmaps or not, whether to
 *               hint the outline, etc).
 *     
 * Note:
 *   If the face has no current charmap, or if the character code is not
 *   defined in the charmap, this function will return an error.
 *
 *   If the glyph image is not a bitmap, and if the bit flag
 *   FT2::Load::IGNORE_TRANSFORM is unset, the glyph image will be
 *   transformed with the information passed to a previous call to
 *   FT2::Face#set_transform
 *
 *   Note that this also transforms the `face.glyph.advance' field, but
 *   not the values in `face.glyph.metrics'.
 *
 * Load Flags:
 *   FT2::Load::DEFAULT
 *   FT2::Load::RENDER
 *   FT2::Load::MONOCHROME
 *   FT2::Load::LINEAR_DESIGN
 *   FT2::Load::NO_SCALE
 *   FT2::Load::NO_HINTING
 *   FT2::Load::NO_BITMAP
 *   FT2::Load::CROP_BITMAP
 *   FT2::Load::VERTICAL_LAYOUT
 *   FT2::Load::IGNORE_TRANSFORM
 *   FT2::Load::IGNORE_GLOBAL_ADVANCE_WIDTH
 *   FT2::Load::FORCE_AUTOHINT
 *   FT2::Load::NO_RECURSE
 *   FT2::Load::PEDANTIC
 *     
 * Examples:
 *   face.load_char 5, FT2::Load::DEFAULT
 *
 */
static VALUE ft_face_load_char(VALUE self, VALUE char_code, VALUE flags) {
  FT_Face *face;
  FT_Error err;
  Data_Get_Struct(self, FT_Face, face);
  err = FT_Load_Char(*face, NUM2INT(char_code), NUM2INT(flags));
  if (err != FT_Err_Ok)
    handle_error(err);
  return self;
}

/*
 * Get the glyph index of a character code.
 *
 * Note:
 *   This function uses a charmap object in order to do the translation.
 *
 *   FreeType computes its own glyph indices which are not necessarily
 *   the same as used in the font in case the font is based on glyph
 *   indices. Reason for this behaviour is to assure that index 0 is
 *   never used, representing the missing glyph.
 *    
 *   A return value of 0 means `undefined character code'.
 *   
 * Examples:
 *   index = face.char_index 65
 *   puts 'undefined character code' if index == 0
 *
 */
static VALUE ft_face_char_index(VALUE self, VALUE char_code) {
  FT_Face *face;
  Data_Get_Struct(self, FT_Face, face);
  return INT2FIX(FT_Get_Char_Index(*face, NUM2INT(char_code)));
}

/* 
 * Get the glyph index of a given glyph name.
 *
 * Note: 
 *   This method uses driver specific objects to do the translation.
 *
 *   A return value of 0 means `undefined character code'.
 *
 * Examples:
 *   index = face.name_index glyph_name
 *   
 */
static VALUE ft_face_name_index(VALUE self, VALUE glyph_name) {
  FT_Face *face;
  Data_Get_Struct(self, FT_Face, face);
  return INT2FIX(FT_Get_Name_Index(*face, RSTRING(glyph_name)->ptr));
}

/*
 * Get the kerning vector between two glyphs of a FT2::Face object.
 *
 * Description:
 *   Get the kerning vector between two glyphs of a FT2::Face object.
 *   
 *   left_glyph: The index of the left glyph in the kern pair.
 *   right_glyph: The index of the right glyph in the kern pair.
 *   kern_mode: One of the FT2::KerningMode::XXXX constants. Determines
 *              the scale/dimension of the returned kerning vector.
 *   
 *   Passing kern_mode == nil is the same as FT2::KerningMode::DEFAULT.
 *   
 *   Returns a kerning vector (actually a two-element array). This is in
 *   font units for scalable formats, and in pixels for fixed-sizes
 *   formats.
 *
 * Kerning Modes:
 *   FT2::KerningMode::DEFAULT
 *   FT2::KerningMode::UNFITTED
 *   FT2::KerningMode::UNSCALED
 *
 * Examples:
 *   left_glyph = 10
 *   left_glyph = 11
 *   k_v = face.kerning left_glyph, right_glyph, nil
 *
 *   # another example, w/o default options
 *   k_v = face.get_kerning 12, 13, FT2::KerningMode::UNFITTED
 *
 */
static VALUE ft_face_kerning(VALUE self, VALUE left_glyph, VALUE right_glyph, VALUE kern_mode) {
  FT_Face *face;
  FT_Error err;
  FT_Vector v;
  VALUE ary;

  Data_Get_Struct(self, FT_Face, face);
  ary = rb_ary_new();

  if (kern_mode == Qnil)
    kern_mode = NUM2INT(ft_kerning_default);
  
  err = FT_Get_Kerning(*face, NUM2INT(left_glyph), NUM2INT(right_glyph), NUM2INT(kern_mode), &v);
  if (err != FT_Err_Ok)
    handle_error(err);

  rb_ary_push(ary, INT2FIX(v.x));
  rb_ary_push(ary, INT2FIX(v.y));
  
  return ary;
}

/*
 * Get the ASCII name of a glyph in a FT2::Face object.
 *
 * Note: 
 *   If the face doesn't provide glyph names or if the glyph index is
 *   invalid, nil is returned.  The glyph name is truncated if it is
 *   longer than 1024 characters.
 *
 * Examples:
 *   glyph_index = 45
 *   glyph_name = face.glyph_name glyph_index
 *
 */
static VALUE ft_face_glyph_name(VALUE self, VALUE glyph_index) {
  FT_Face *face;
  FT_Error err;
  char buf[1024];
  Data_Get_Struct(self, FT_Face, face);
  err = FT_Get_Glyph_Name(*face, NUM2INT(glyph_index), buf, sizeof(buf));
  if (err != FT_Err_Ok)
    handle_error(err);

  return (buf && strlen(buf)) ? rb_str_new2(buf) : Qnil;
}

/*
 * Get the ASCII Postscript name of a FT2::Face object.
 *
 * Note: 
 *   This should only work with Postscript and TrueType fonts. If the
 *   PostScript name is un-avaialble, nil is returned.
 *
 * Examples:
 *   ps_name = face.postscript_name
 *   name = face.name
 *
 */
static VALUE ft_face_ps_name(VALUE self) {
  FT_Face *face;
  const char *str;
  Data_Get_Struct(self, FT_Face, face);
  
  if ((str = FT_Get_Postscript_Name(*face)) != NULL)
    return rb_str_new2(str);
  else
    return Qnil;
}

/*
 * Select a FT2::Face object's charmap by its encoding tag.
 *
 * Encoding Tags:
 *   FT2::Encoding::NONE
 *   FT2::Encoding::SYMBOL
 *   FT2::Encoding::UNICODE
 *   FT2::Encoding::LATIN_1
 *
 * Examples:
 *   face.select_charmap FT2::Encoding::UNICODE
 *
 */
static VALUE ft_face_select_charmap(VALUE self, VALUE encoding) {
  FT_Face *face;
  FT_Error err;
  Data_Get_Struct(self, FT_Face, face);
  err = FT_Select_Charmap(*face, NUM2INT(encoding));
  if (err != FT_Err_Ok)
    handle_error(err);
  return self;
}

/*
 * Select the FT2::Face object's charmap for character code to glyph index decoding.
 *
 * Examples:
 *   charmap = face.charmaps[0]
 *   face.set_charmap charmap
 *
 *   charmap = face.charmaps[0]
 *   face.charmap = charmap
 *
 */
static VALUE ft_face_set_charmap(VALUE self, VALUE charmap) {
  FT_Face *face;
  FT_CharMap *cm;
  FT_Error err;
  Data_Get_Struct(self, FT_Face, face);
  Data_Get_Struct(charmap, FT_CharMap, cm);
  err = FT_Set_Charmap(*face, *cm);
  if (err != FT_Err_Ok)
    handle_error(err);
  return self;
}

/*
 * Return the first character code of the selected charmap and corresponding glyph index of a FT2::Face object.
 * 
 * Note:
 *   Using this with FT2::Face#next_char will allow you to iterate
 *   through the charmap => glyph index mapping for the selected
 *   charmap.  
 *
 *   You should probably use the method FT2::Face#current_charmap
 *   instead.
 *
 * Examples:
 *   c_code, g_idx = face.first_char
 *   while g_idx != 0
 *     puts "#{c_code} => #{g_idx}"
 *     c_code, g_idx = face.next_char
 *   end
 *
 */
static VALUE ft_face_first_char(VALUE self) {
  FT_Face *face;
  VALUE ary;
  FT_ULong char_code;
  FT_UInt  glyph_index;
  Data_Get_Struct(self, FT_Face, face);
  ary = rb_ary_new();

  char_code = FT_Get_First_Char(*face, &glyph_index);
  rb_ary_push(ary, UINT2NUM(char_code));
  rb_ary_push(ary, UINT2NUM(glyph_index));

  return ary;
}

/*
 * Return the next character code of the selected charmap and corresponding glyph index of a FT2::Face object.
 * 
 * Note:
 *   Using this with FT2::Face#first_char will allow you to iterate
 *   through the charmap => glyph index mapping for the selected
 *   charmap.  Returns 0 if the charmap is empty, or if there are no
 *   more codes in the charmap.
 *
 *   You should probably use the method FT2::Face#current_charmap
 *   instead.
 *
 * Examples:
 *   c_code, g_idx = face.first_char
 *   while g_idx != 0
 *     puts "#{c_code} => #{g_idx}"
 *     c_code, g_idx = face.next_char
 *   end
 *
 */
static VALUE ft_face_next_char(VALUE self, VALUE char_code) {
  FT_Face *face;
  VALUE ary;
  FT_ULong ret_char_code;
  FT_UInt  glyph_index;
  Data_Get_Struct(self, FT_Face, face);
  ary = rb_ary_new();

  ret_char_code = FT_Get_Next_Char(*face, char_code, &glyph_index);
  rb_ary_push(ary, UINT2NUM(ret_char_code));
  rb_ary_push(ary, UINT2NUM(glyph_index));

  return ary;
}

/*
 * Return the character code to glyph index map of the selected charmap of a FT2::Face object.
 * 
 * Note:
 *   Returns nil if the selected charmap is empty.
 *
 * Examples:
 *   mapping = face.charmap
 *
 */
static VALUE ft_face_current_charmap(VALUE self) {
  FT_Face *face;
  FT_ULong c_code;
  FT_UInt g_idx;
  VALUE rtn;

  rtn = Qnil;
  Data_Get_Struct(self, FT_Face, face);

  c_code = FT_Get_First_Char(*face, &g_idx);
  if (!g_idx || !c_code)
    return rtn;

  rtn = rb_hash_new();
  while (c_code != 0) {
    rb_hash_aset(rtn, UINT2NUM(c_code), UINT2NUM(g_idx));
    c_code = FT_Get_Next_Char(*face, c_code, &g_idx);
  }
  
  return rtn;
}


/*****************************/
/* FT2::GlyphMetrics methods */
/*****************************/

/*
 * Constructor for FT2::GlyphMetrics
 *
 * This method is currently empty.  You should never call this method
 * directly unless you're instantiating a derived class (ie, you know
 * what you're doing).
 *
 */
static VALUE ft_glyphmetrics_init(VALUE self) {
  return self;
}

/* 
 * Get the glyph's width.
 *
 * Note: 
 *   Values are expressed in 26.6 fractional pixel format or in font
 *   units, depending on context.
 * 
 * Aliases:
 *   FT2::GlyphMetrics#w
 *
 * Examples:
 *   w = slot.metrics.width
 *   w = slot.metrics.w
 *
 */
static VALUE ft_glyphmetrics_width(VALUE self) {
  FT_Glyph_Metrics *glyph;
  Data_Get_Struct(self, FT_Glyph_Metrics, glyph);
  return INT2NUM(glyph->width);
}

/* 
 * Get the glyph's height.
 *
 * Note: 
 *   Values are expressed in 26.6 fractional pixel format or in font
 *   units, depending on context.
 * 
 * Aliases:
 *   FT2::GlyphMetrics#h
 *
 * Examples:
 *   h = slot.metrics.height
 *   h = slot.metrics.h
 *
 */
static VALUE ft_glyphmetrics_height(VALUE self) {
  FT_Glyph_Metrics *glyph;
  Data_Get_Struct(self, FT_Glyph_Metrics, glyph);
  return INT2NUM(glyph->height);
}

/* 
 * Get the glyph's left side bearing in horizontal layouts.
 *
 * Note: 
 *   Values are expressed in 26.6 fractional pixel format or in font
 *   units, depending on context.
 * 
 * Aliases:
 *   FT2::GlyphMetrics#horiBearingX
 *   FT2::GlyphMetrics#h_bear_x
 *   FT2::GlyphMetrics#hbx
 *
 * Examples:
 *   hbx = slot.metrics.h_bearing_x
 *   hbx = slot.metrics.horiBearingX
 *   hbx = slot.metrics.h_bear_x
 *   hbx = slot.metrics.hbx
 *
 */
static VALUE ft_glyphmetrics_h_bear_x(VALUE self) {
  FT_Glyph_Metrics *glyph;
  Data_Get_Struct(self, FT_Glyph_Metrics, glyph);
  return INT2NUM(glyph->horiBearingX);
}

/* 
 * Get the glyph's top side bearing in horizontal layouts.
 *
 * Note: 
 *   Values are expressed in 26.6 fractional pixel format or in font
 *   units, depending on context.
 * 
 * Aliases:
 *   FT2::GlyphMetrics#horiBearingY
 *   FT2::GlyphMetrics#h_bear_y
 *   FT2::GlyphMetrics#hby
 *
 * Examples:
 *   hby = slot.metrics.h_bearing_y
 *   hby = slot.metrics.horiBearingY
 *   hby = slot.metrics.h_bear_y
 *   hby = slot.metrics.hby
 *
 */
static VALUE ft_glyphmetrics_h_bear_y(VALUE self) {
  FT_Glyph_Metrics *glyph;
  Data_Get_Struct(self, FT_Glyph_Metrics, glyph);
  return INT2NUM(glyph->horiBearingY);
}

/* 
 * Get the glyph's advance width for horizontal layouts.
 *
 * Note: 
 *   Values are expressed in 26.6 fractional pixel format or in font
 *   units, depending on context.
 * 
 * Aliases:
 *   FT2::GlyphMetrics#horiAdvance
 *   FT2::GlyphMetrics#ha
 *
 * Examples:
 *   ha = slot.metrics.h_advance
 *   ha = slot.metrics.horiAdvance
 *   ha = slot.metrics.ha
 *
 */
static VALUE ft_glyphmetrics_h_advance(VALUE self) {
  FT_Glyph_Metrics *glyph;
  Data_Get_Struct(self, FT_Glyph_Metrics, glyph);
  return INT2NUM(glyph->horiAdvance);
}

/* 
 * Get the glyph's left side bearing in vertical layouts.
 *
 * Note: 
 *   Values are expressed in 26.6 fractional pixel format or in font
 *   units, depending on context.
 * 
 * Aliases:
 *   FT2::GlyphMetrics#vertBearingX
 *   FT2::GlyphMetrics#v_bear_x
 *   FT2::GlyphMetrics#vbx
 *
 * Examples:
 *   vbx = slot.metrics.v_bearing_x
 *   vbx = slot.metrics.vertBearingX
 *   vbx = slot.metrics.v_bear_x
 *   vbx = slot.metrics.vbx
 *
 */
static VALUE ft_glyphmetrics_v_bear_x(VALUE self) {
  FT_Glyph_Metrics *glyph;
  Data_Get_Struct(self, FT_Glyph_Metrics, glyph);
  return INT2NUM(glyph->vertBearingX);
}

/* 
 * Get the glyph's top side bearing in vertical layouts.
 *
 * Note: 
 *   Values are expressed in 26.6 fractional pixel format or in font
 *   units, depending on context.
 * 
 * Aliases:
 *   FT2::GlyphMetrics#vertBearingY
 *   FT2::GlyphMetrics#v_bear_y
 *   FT2::GlyphMetrics#vby
 *
 * Examples:
 *   vby = slot.metrics.v_bearing_y
 *   vby = slot.metrics.vertBearingY
 *   vby = slot.metrics.v_bear_y
 *   vby = slot.metrics.vby
 *
 */
static VALUE ft_glyphmetrics_v_bear_y(VALUE self) {
  FT_Glyph_Metrics *glyph;
  Data_Get_Struct(self, FT_Glyph_Metrics, glyph);
  return INT2NUM(glyph->vertBearingY);
}

/* 
 * Get the glyph's advance width for vertical layouts.
 *
 * Note: 
 *   Values are expressed in 26.6 fractional pixel format or in font
 *   units, depending on context.
 * 
 * Aliases:
 *   FT2::GlyphMetrics#vertAdvance
 *   FT2::GlyphMetrics#va
 *
 * Examples:
 *   va = slot.metrics.v_advance
 *   va = slot.metrics.vertAdvance
 *   va = slot.metrics.va
 *
 */
static VALUE ft_glyphmetrics_v_advance(VALUE self) {
  FT_Glyph_Metrics *glyph;
  Data_Get_Struct(self, FT_Glyph_Metrics, glyph);
  return INT2NUM(glyph->vertAdvance);
}


/**************************/
/* FT2::GlyphSlot methods */
/**************************/

/*
 * Constructor for FT2::GlyphSlot class.
 *
 * This method is currently empty.  You should never call this method
 * directly unless you're instantiating a derived class (ie, you know
 * what you're doing).
 *
 */
static VALUE ft_glyphslot_init(VALUE self) {
  return self;
}

/*
 * Convert a FT2::GlyphSlot object to a bitmap.
 *
 * Description:
 *   Converts a given FT2::GlyphSlot to a bitmap. It does so by
 *   $inspecting the FT2::GlyphSlot format, finding the relevant
 *   renderer, and invoking it.
 *
 *   render_mode: This is the render mode used to render the glyph image
 *                into a bitmap. See below for a list of possible
 *                values.  If render_mode is nil, then it defaults to
 *                FT2::RenderMode::NORMAL.
 *
 * Aliases:
 *   FT2::GlyphSlot#render_glyph
 *
 * Render Modes:
 *   FT2::RenderMode::NORMAL
 *   FT2::RenderMode::MONO
 *   
 * Examples:
 *   slot.render FT2::RenderMode::NORMAL
 *
 */
static VALUE ft_glyphslot_render(VALUE self, VALUE render_mode) {
  FT_Error err;
  FT_GlyphSlot *glyph;

  Data_Get_Struct(self, FT_GlyphSlot, glyph);
  if (render_mode == Qnil)
    render_mode = INT2FIX(ft_render_mode_normal);
  
  err = FT_Render_Glyph(*glyph, NUM2INT(render_mode));
  if (err != FT_Err_Ok)
    handle_error(err);

  return self;
}

static VALUE ft_glyphslot_glyph(VALUE self) {
  FT_Error err;
  FT_GlyphSlot *slot;
  FT_Glyph *glyph;

  glyph = malloc(sizeof(FT_Glyph));
  Data_Get_Struct(self, FT_GlyphSlot, slot);
  err = FT_Get_Glyph(*slot, glyph);
  if (err != FT_Err_Ok) {
    free(glyph);
    handle_error(err);
  }

  return Data_Wrap_Struct(cGlyph, 0, glyph_free, glyph);
}

/*
 * Get the FT2::Library instance this FT2::GlyphSlot object belongs to.
 *
 * Examples:
 *   lib = slot.library
 *
 */
static VALUE ft_glyphslot_library(VALUE self) {
  FT_GlyphSlot *glyph;
  FT_Library *lib;

  Data_Get_Struct(self, FT_GlyphSlot, glyph);
  lib = malloc(sizeof(FT_Library));
  *lib = (*glyph)->library;

  return Data_Wrap_Struct(cLibrary, 0, dont_free, lib);
}

/*
 * Get the FT2::Face object this FT2::GlyphSlot object belongs to.
 *
 * Examples:
 *   face = slot.face
 *
 */
static VALUE ft_glyphslot_face(VALUE self) {
  FT_GlyphSlot *glyph;
  FT_Face *face;

  Data_Get_Struct(self, FT_GlyphSlot, glyph);
  face = malloc(sizeof(FT_Face));
  *face = (*glyph)->face;
  
  /* do we really want to dont_free() cb here? */
  return Data_Wrap_Struct(cFace, 0, dont_free, face);
}

/*
 * Get the next FT2::GlyphSlot object.
 *
 * Description:
 *   In some cases (like some font tools), several FT2::GlyphSlot s per
 *   FT2::Face object can be a good thing. As this is rare, the
 *   FT2::GlyphSlot s are listed through a direct, single-linked list
 *   using its `next' field.
 *
 * Examples:
 *   next_slot = slot.next
 *
 */
static VALUE ft_glyphslot_next(VALUE self) {
  FT_GlyphSlot *glyph;
  FT_GlyphSlot *next;

  Data_Get_Struct(self, FT_GlyphSlot, glyph);
  next = malloc(sizeof(FT_GlyphSlot));
  *next = (*glyph)->next;
  
  /* do we really want to dont_free() cb here? */
  return Data_Wrap_Struct(cGlyphSlot, 0, dont_free, next);
}

/*
 * Get the FT2::GlyphMetrics of a FT2::GlyphSlot object.
 *
 * Examples:
 *   metrics = slot.metrics
 *
 */
static VALUE ft_glyphslot_metrics(VALUE self) {
  FT_GlyphSlot *glyph;
  FT_Glyph_Metrics *metrics;

  Data_Get_Struct(self, FT_GlyphSlot, glyph);
  metrics = malloc(sizeof(FT_Glyph_Metrics));
  *metrics = (*glyph)->metrics;
  
  return Data_Wrap_Struct(cGlyphMetrics, 0, dont_free, metrics);
}

/*
 * Get the linearly scaled horizontal advance width of a FT2::GlyphSlot object.
 *
 * Description:
 *   For scalable formats only, this field holds the linearly scaled
 *   horizontal advance width for the FT2:GlyphSlot (i.e. the scaled and
 *   unhinted value of the hori advance). This can be important to
 *   perform correct WYSIWYG layout.
 * 
 * Note:
 *   The return value is expressed by default in 16.16 pixels.  However,
 *   when the FT2::GlyphSlot is loaded with the FT2::Load::LINEAR_DESIGN
 *   flag, this field contains simply the value of the advance in
 *   original font units.
 *
 * Aliases:
 *   FT2::GlyphSlot#linearHoriAdvance
 *   FT2::GlyphSlot#h_adv
 *   FT2::GlyphSlot#ha
 *
 * Examples:
 *   h_adv = slot.h_advance
 *
 */
static VALUE ft_glyphslot_h_advance(VALUE self) {
  FT_GlyphSlot *glyph;

  Data_Get_Struct(self, FT_GlyphSlot, glyph);
  return rb_float_new(FTFIX2DBL((*glyph)->linearHoriAdvance));
}

/*
 * Get the linearly scaled vertical advance height of a FT2::GlyphSlot object.
 *
 * Description:
 *   For scalable formats only, this field holds the linearly scaled
 *   vertical advance height for the FT2:GlyphSlot (i.e. the scaled and
 *   unhinted value of the hori advance). This can be important to
 *   perform correct WYSIWYG layout.
 * 
 * Note:
 *   The return value is expressed by default in 16.16 pixels.  However,
 *   when the FT2::GlyphSlot is loaded with the FT2::Load::LINEAR_DESIGN
 *   flag, this field contains simply the value of the advance in
 *   original font units.
 *
 * Aliases:
 *   FT2::GlyphSlot#linearVertAdvance
 *   FT2::GlyphSlot#v_adv
 *   FT2::GlyphSlot#va
 *
 * Examples:
 *   v_adv = slot.v_advance
 *
 */
static VALUE ft_glyphslot_v_advance(VALUE self) {
  FT_GlyphSlot *glyph;

  Data_Get_Struct(self, FT_GlyphSlot, glyph);
  return rb_float_new(ft_fixed_to_double((*glyph)->linearVertAdvance));
}

/*
 * Get the transformed advance width for a FT2::GlyphSlot object.
 *
 * Examples:
 *   adv = slot.advance
 *
 */
static VALUE ft_glyphslot_advance(VALUE self) {
  FT_GlyphSlot *glyph;
  VALUE ary;

  Data_Get_Struct(self, FT_GlyphSlot, glyph);
  ary = rb_ary_new();

  rb_ary_push(ary, INT2NUM((*glyph)->advance.x));
  rb_ary_push(ary, INT2NUM((*glyph)->advance.y));
  return ary;
}

/*
 * Get the format of a FT2::GlyphSlot object.
 *
 * Glyph Formats:
 *   FT2::GlyphFormat::COMPOSITE
 *   FT2::GlyphFormat::BITMAP
 *   FT2::GlyphFormat::OUTLINE
 *   FT2::GlyphFormat::PLOTTER
 *
 * Examples:
 *   fmt = slot.format
 *
 */
static VALUE ft_glyphslot_format(VALUE self) {
  FT_GlyphSlot *glyph;
  Data_Get_Struct(self, FT_GlyphSlot, glyph);
  return INT2NUM((*glyph)->format);
}

/*
 * Get the bitmap of a bitmap format FT2::GlyphSlot object.
 *
 * Examples:
 *   bmap = slot.bitmap
 *
 */
static VALUE ft_glyphslot_bitmap(VALUE self) {
  FT_GlyphSlot *glyph;
  FT_Bitmap *bitmap;

  Data_Get_Struct(self, FT_GlyphSlot, glyph);
  bitmap = malloc(sizeof(FT_Bitmap));
  *bitmap = (*glyph)->bitmap;
  
  return Data_Wrap_Struct(cBitmap, 0, dont_free, bitmap);
}

/*
 * Get the left bearing (in pixels) of a bitmap format FT2::GlyphSlot object.
 *
 * Note: 
 *   Only valid if the format is FT2::GlyphFormat::BITMAP.
 *
 * Examples:
 *   left = slot.bitmap_left
 *
 */
static VALUE ft_glyphslot_bitmap_left(VALUE self) {
  FT_GlyphSlot *glyph;
  Data_Get_Struct(self, FT_GlyphSlot, glyph);
  return INT2NUM((*glyph)->bitmap_left);
}

/*
 * Get the top bearing (in pixels) of a bitmap format FT2::GlyphSlot object.
 *
 * Note: 
 *   Only valid if the format is FT2::GlyphFormat::BITMAP.  The value
 *   returned is the distance from the baseline to the topmost glyph
 *   scanline, upwards y-coordinates being positive.
 *
 * Examples:
 *   top = slot.bitmap_top
 *
 */
static VALUE ft_glyphslot_bitmap_top(VALUE self) {
  FT_GlyphSlot *glyph;
  Data_Get_Struct(self, FT_GlyphSlot, glyph);
  return INT2NUM((*glyph)->bitmap_top);
}

/*
 * Get the outline of a bitmap outline format FT2::GlyphSlot object.
 *
 * Note:
 *   Only valid if the format is FT2::GlyphFormat::OUTLINE.
 *
 * Examples:
 *   outline = slot.outline
 *
 */
static VALUE ft_glyphslot_outline(VALUE self) {
  FT_GlyphSlot *glyph;
  FT_Outline *outline;

  Data_Get_Struct(self, FT_GlyphSlot, glyph);
  outline = malloc(sizeof(FT_Outline));
  *outline = (*glyph)->outline;
  
  return Data_Wrap_Struct(cOutline, 0, dont_free, outline);
}

/*
 * Get the number of subglyphs of a FT2::GlyphSlot object.
 *
 * Note:
 *   Only valid if the format is FT2::GlyphFormat::COMPOSITE.
 *
 * Examples:
 *   outline = slot.outline
 *
 */
static VALUE ft_glyphslot_num_subglyphs(VALUE self) {
  FT_GlyphSlot *glyph;
  Data_Get_Struct(self, FT_GlyphSlot, glyph);
  return INT2NUM((*glyph)->num_subglyphs);
}

/*
 * Get a list of subglyphs of a composite format FT2::GlyphSlot object.
 *
 * Note:
 *   Only valid if the format is FT2::GlyphFormat::COMPOSITE. FIXME:
 *   this method may not work correctly at the moment.
 *
 * Examples:
 *   sub_glyphs = slot.subglyphs
 *
 */
static VALUE ft_glyphslot_subglyphs(VALUE self) {
  FT_GlyphSlot *glyph;
  VALUE rtn;
  int num;

  Data_Get_Struct(self, FT_GlyphSlot, glyph);
  if ((num = (*glyph)->num_subglyphs) < 1)
    return Qnil;

  /* FIXME: this probably doesn't work */
  rtn = rb_ary_new();
/* 
 *   for (i = 0; i < num; i++)
 *     rb_ary_push(rtn, Data_Wrap_Struct(cSubGlyph, 0,
 *                                       dont_free,
 *                                       (*glyph)->subglyphs[i]));
 */ 
  return rtn;
}

/*
 * Get optional control data for a FT2::GlyphSlot object.
 *
 * Description:
 *   Certain font drivers can also return the control data for a given
 *   FT2::GlyphSlot (e.g. TrueType bytecode, Type 1 charstrings, etc.).
 *   This field is a pointer to such data.
 *
 * Examples:
 *   data = slot.control_data
 *
 */
static VALUE ft_glyphslot_control_data(VALUE self) {
  FT_GlyphSlot *glyph;
  Data_Get_Struct(self, FT_GlyphSlot, glyph);
  return rb_str_new((*glyph)->control_data, (*glyph)->control_len);
}

/*
 * Get the length of the optional control data for a FT2::GlyphSlot object.
 *
 * Examples:
 *   len = slot.control_len
 *
 */
static VALUE ft_glyphslot_control_len(VALUE self) {
  FT_GlyphSlot *glyph;
  Data_Get_Struct(self, FT_GlyphSlot, glyph);
  return INT2NUM((*glyph)->control_len);
}


/*********************/
/* FT2::Size methods */
/*********************/

/*
 * Constructor for FT2::Size class.
 *
 * This method is currently empty.  You should never call this method
 * directly unless you're instantiating a derived class (ie, you know
 * what you're doing).
 *
 */
static VALUE ft_size_init(VALUE self) {
  return self;
}

/*
 * Get the FT2::Face object this FT2::Size object is associated with.
 *
 * Examples:
 *   face = size.face
 *
 */
static VALUE ft_size_face(VALUE self) {
  FT_Size *size;
  FT_Face *face;

  Data_Get_Struct(self, FT_Size, size);
  face = malloc(sizeof(FT_Face));
  *face = (*size)->face;

  return Data_Wrap_Struct(cFace, 0, dont_free, face);
}

/*
 * Get the FT2::SizeMetrics associated with a FT2::Size object.
 *
 * Examples:
 *   s_metrics = size.metrics
 *
 */
static VALUE ft_size_metrics(VALUE self) {
  FT_Size *size;
  FT_Size_Metrics *metrics;

  Data_Get_Struct(self, FT_Size, size);
  metrics = malloc(sizeof(FT_Size_Metrics));
  *metrics = (*size)->metrics;

  return Data_Wrap_Struct(cSizeMetrics, 0, dont_free, metrics);
}


/****************************/
/* FT2::SizeMetrics methods */
/****************************/

/*
 * Constructor for FT2::SizeMetrics class.
 *
 * This method is currently empty.  You should never call this method
 * directly unless you're instantiating a derived class (ie, you know
 * what you're doing).
 *
 */
static VALUE ft_size_metrics_init(VALUE self) {
  return self;
}

/*
 * Get the X pixels per EM of this FT2::SizeMetrics object.
 *
 * Description:
 *   x_ppem stands for the size in integer pixels of the EM square.
 *   which also is the horizontal character pixel size.
 *
 * Examples:
 *   x_ppem = face.size.metrics.x_ppem
 *
 */
static VALUE ft_size_metrics_x_ppem(VALUE self) {
  FT_Size_Metrics *size_metrics;
  Data_Get_Struct(self, FT_Size_Metrics, size_metrics);
  return INT2FIX((int) (*size_metrics).x_ppem);
}

/*
 * Get the Y pixels per EM of this FT2::SizeMetrics object.
 *
 * Description:
 *   y_ppem stands for the size in integer pixels of the EM square.
 *   which also is the vertical character pixel size.
 *
 * Examples:
 *   y_ppem = face.size.metrics.y_ppem
 *
 */
static VALUE ft_size_metrics_y_ppem(VALUE self) {
  FT_Size_Metrics *size_metrics;
  Data_Get_Struct(self, FT_Size_Metrics, size_metrics);
  return INT2FIX((int) (*size_metrics).y_ppem);
}

/*
 * Get the horizontal scale of a FT2::SizeMetrics object.
 *
 * Description:
 *   Scale that is used to directly scale horizontal distances from
 *   design space to 1/64th of device pixels. 
 *   
 * Examples:
 *   x_scale = face.size.metrics.x_scale
 *
 */
static VALUE ft_size_metrics_x_scale(VALUE self) {
  FT_Size_Metrics *size_metrics;
  Data_Get_Struct(self, FT_Size_Metrics, size_metrics);
  return INT2FIX((int) (*size_metrics).x_scale);
}

/*
 * Get the vertical scale of a FT2::SizeMetrics object.
 *
 * Description:
 *   Scale that is used to directly scale vertical distances from
 *   design space to 1/64th of device pixels. 
 *   
 * Examples:
 *   y_scale = face.size.metrics.y_scale
 *
 */
static VALUE ft_size_metrics_y_scale(VALUE self) {
  FT_Size_Metrics *size_metrics;
  Data_Get_Struct(self, FT_Size_Metrics, size_metrics);
  return INT2FIX((int) (*size_metrics).y_scale);
}


/**********************/
/* FT2::Glyph methods */
/**********************/
static void glyph_free(void *ptr) {
  FT_Glyph glyph = *((FT_Glyph *) ptr);
  FT_Done_Glyph(glyph);
  free(ptr);
}


/*
 * Constructor for FT2::Glyph class.
 *
 * This method is currently empty.  You should never call this method
 * directly unless you're instantiating a derived class (ie, you know
 * what you're doing).
 *
 */
static VALUE ft_glyph_init(VALUE self) {
  return self;
}

/*
 * Get the library of a FT2::Glyph object.
 * 
 * Note: 
 *   Glyph objects are not owned or tracked by the library.
 *
 * Examples:
 *   lib = glyph.library
 *
 */
static VALUE ft_glyph_library(VALUE self) {
  FT_Glyph *glyph;
  Data_Get_Struct(self, FT_Glyph, glyph);
  return Data_Wrap_Struct(cLibrary, 0, dont_free, (*glyph)->library);
}

/*
 * Get the FreeType2 class of a FT2::Glyph object.
 * 
 * Note:
 *   This is _not_ the Ruby class of the object.
 *
 * Aliases:
 *   FT2::Glyph#clazz
 *
 * Examples:
 *   c = glyph.class
 *
 */
static VALUE ft_glyph_class(VALUE self) {
  FT_Glyph *glyph;
  Data_Get_Struct(self, FT_Glyph, glyph);
  return Data_Wrap_Struct(cGlyphClass, 0, dont_free, &(*glyph)->clazz);
}

/*
 * Get the format of a FT2::Glyph object's image.
 * 
 * Glyph Formats:
 *   FT2::GlyphFormat::COMPOSITE
 *   FT2::GlyphFormat::BITMAP
 *   FT2::GlyphFormat::OUTLINE
 *   FT2::GlyphFormat::PLOTTER
 *
 * Examples:
 *   format = glyph.format
 *
 */
static VALUE ft_glyph_format(VALUE self) {
  FT_Glyph *glyph;
  Data_Get_Struct(self, FT_Glyph, glyph);
  return INT2NUM((*glyph)->format);
}

/*
 * Get the advance of a FT2::Glyph object.
 * 
 * Description:
 *   This vector gives the FT2::Glyph object's advance width.
 *
 * Examples:
 *   advance = glyph.advance
 *
 */
static VALUE ft_glyph_advance(VALUE self) {
  FT_Glyph *glyph;
  VALUE ary;

  Data_Get_Struct(self, FT_Glyph, glyph);
  ary = rb_ary_new();
  rb_ary_push(ary, INT2FIX((*glyph)->advance.x));
  rb_ary_push(ary, INT2FIX((*glyph)->advance.y));

  return ary;
}

/*
 * Duplicate a FT2::Glyph object.
 *
 * Aliases:
 *   FT2::Glyph#copy
 *
 * Examples:
 *   new_glyph = glyph.dup
 *
 */
static VALUE ft_glyph_dup(VALUE self) {
  FT_Error err;
  FT_Glyph *glyph, *new_glyph;

  new_glyph = malloc(sizeof(FT_Glyph));
  Data_Get_Struct(self, FT_Glyph, glyph);
  err = FT_Glyph_Copy(*glyph, new_glyph);
  if (err != FT_Err_Ok)
    handle_error(err);
  
  return Data_Wrap_Struct(cGlyph, 0, glyph_free, new_glyph);
}

/*
 * Transform a FT2::Glyph object if it's format is scalable.
 * 
 * Description:
 *   matrix: A pointer to a 2x2 matrix to apply.
 *   delta:  A pointer to a 2d vector to apply. Coordinates are
 *           expressed in 1/64th of a pixel.
 *     
 * Note:
 *   The transformation matrix is also applied to the glyph's advance
 *   vector.  This method returns an error if the glyph format is not
 *   scalable (eg, if it's not equal to zero).
 *     
 * Examples:
 *   matrix = [[1, 0],
 *             [0, 1]]
 *   delta = [1, 1]
 *   transform = glyph.transform matrix, delta
 *
 */
static VALUE ft_glyph_transform(VALUE self, VALUE matrix_ary, VALUE delta_ary) {
  FT_Error err;
  FT_Glyph *glyph;
  FT_Matrix matrix;
  FT_Vector delta;

  matrix.xx = DBL2FTFIX(NUM2DBL(rb_ary_entry(rb_ary_entry(matrix_ary, 0), 0)));
  matrix.xy = DBL2FTFIX(NUM2DBL(rb_ary_entry(rb_ary_entry(matrix_ary, 0), 1)));
  matrix.yx = DBL2FTFIX(NUM2DBL(rb_ary_entry(rb_ary_entry(matrix_ary, 1), 0)));
  matrix.yy = DBL2FTFIX(NUM2DBL(rb_ary_entry(rb_ary_entry(matrix_ary, 1), 1)));

  delta.x = NUM2INT(rb_ary_entry(delta_ary, 0));
  delta.y = NUM2INT(rb_ary_entry(delta_ary, 1));

  Data_Get_Struct(self, FT_Glyph, glyph);
  err = FT_Glyph_Transform(*glyph, &matrix, &delta);
  if (err != FT_Err_Ok)
    handle_error(err);
  
  return self;
}

/*
 * Get the control box of a FT2::Glyph object.
 *
 * Description:
 *   Returns a FT2::Glyph object's `control box'. The control box
 *   encloses all the outline's points, including Bezier control points.
 *   Though it coincides with the exact bounding box for most FT2::Glyph
 *   objects, it can be slightly larger in some situations (like when
 *   rotating an outline which contains Bezier outside arcs).
 *
 *   Computing the control box is very fast, while getting the bounding
 *   box can take much more time as it needs to walk over all segments
 *   and arcs in the outline. To get the latter, you can use the
 *   `ftbbox' component which is dedicated to this single task.
 *    
 * Notes:
 *   Coordinates are relative to the FT2::Glyph object's origin, using
 *   the Y-upwards convention.
 *
 *   If the FT2::Glyph object has been loaded with FT2::Load::NO_SCALE,
 *   `bbox_mode' must be set to FT2::GlyphBBox::UNSCALED to get unscaled
 *   font units.
 *
 *   If `bbox_mode' is set to FT2::GlyphBBox::SUBPIXELS the
 *   bbox coordinates are returned in 26.6 pixels (i.e. 1/64th of
 *   pixels).
 *
 *   Note that the maximum coordinates are exclusive, which means that
 *   one can compute the width and height of the FT2::Glyph object image
 *   (be it in integer or 26.6 pixels) as:
 *
 *   width = bbox.xMax - bbox.xMin; height = bbox.yMax - bbox.yMin;
 *
 *   Note also that for 26.6 coordinates, if `bbox_mode' is set to
 *   FT2::GlyphBBox::GRIDFIT, the coordinates will also be
 *   grid-fitted, which corresponds to:
 *
 *   bbox.xMin = FLOOR(bbox.xMin); bbox.yMin = FLOOR(bbox.yMin);
 *   bbox.xMax = CEILING(bbox.xMax); bbox.yMax = CEILING(bbox.yMax);
 *
 *   To get the bbox in pixel coordinates, set `bbox_mode' to
 *   FT2::GlyphBBox::TRUNCATE.
 *
 *   To get the bbox in grid-fitted pixel coordinates, set `bbox_mode'
 *   to FT2::GlyphBBox::PIXELS.
 *
 *   The default value for `bbox_mode' is FT2::GlyphBBox::PIXELS.
 *    
 * Aliases:
 *   FT2::Glyph#control_box
 *
 * Examples:
 *   bbox_mode = FT2::GlyphBBox::PIXELS
 *   x_min, y_min, x_max, y_max = glyph.cbox bbox_mode
 *
 */
static VALUE ft_glyph_cbox(VALUE self, VALUE bbox_mode) {
  FT_Glyph *glyph;
  FT_BBox  bbox;
  VALUE    ary;

  Data_Get_Struct(self, FT_Glyph, glyph);
  FT_Glyph_Get_CBox(*glyph, FIX2INT(bbox_mode), &bbox);

  ary = rb_ary_new();
  rb_ary_push(ary, INT2FIX(bbox.xMin));
  rb_ary_push(ary, INT2FIX(bbox.yMin));
  rb_ary_push(ary, INT2FIX(bbox.xMax));
  rb_ary_push(ary, INT2FIX(bbox.yMax));
  
  return ary;
}

/*
 * Render a FT2::Glyph object as a FT2::BitmapGlyph object.
 *
 * Description:
 *   Converts a FT2::Glyph object to a FT2::BitmapGlyph object.
 *   
 *   render_mode: A set of bit flags that describe how the data is.
 *   origin:      A vector used to translate the glyph image before
 *                rendering. Can be nil (if no translation).  The
 *                origin is expressed in 26.6 pixels.
 *   destroy:     A boolean that indicates that the original glyph
 *                image should be destroyed by this function. It is
 *                never destroyed in case of error.
 *       
 * Aliases:
 *   FT2::Glyph#to_bitmap
 *
 * Examples:
 *   glyph_to_bmap = glyph.glyph_to_bmap
 *
 */
static VALUE ft_glyph_to_bmap(VALUE self, VALUE render_mode, VALUE origin, VALUE destroy) {
  FT_Error err;
  FT_Glyph *glyph;
  FT_Vector v;
  FT_Bool d;

  Data_Get_Struct(self, FT_Glyph, glyph);
  v.x = NUM2INT(rb_ary_entry(origin, 0));
  v.y = NUM2INT(rb_ary_entry(origin, 1));
  d = (destroy != Qnil) ? 1 : 0;

  err = FT_Glyph_To_Bitmap(glyph, FIX2INT(render_mode), &v, d);
  if (err != FT_Err_Ok)
    handle_error(err);

  return self;
}


/****************************/
/* FT2::BitmapGlyph methods */
/****************************/

/*
 * Constructor for FT2::BitmapGlyph class.
 *
 * This method is currently empty.  You should never call this method
 * directly unless you're instantiating a derived class (ie, you know
 * what you're doing).
 *
 */
static VALUE ft_bmapglyph_init(VALUE self) {
  return self;
}

/*
 * Get the top-side bearing of a BitmapGlyph object.
 *
 * Description:
 *   The top-side bearing, i.e., the vertical distance from the current
 *   pen position to the top border of the glyph bitmap. This distance
 *   is positive for upwards-y.
 *   
 * Note:
 *   FT2::BitmapGlyph is a subclass of FT2::Glyph.
 *
 * Examples:
 *   y = bmap_glyph.top
 *   
 */
static VALUE ft_bmapglyph_top(VALUE self) {
  FT_BitmapGlyph *glyph;
  Data_Get_Struct(self, FT_BitmapGlyph, glyph);
  return INT2FIX((*glyph)->top);
}

/*
 * Get the left-side bearing of a BitmapGlyph object.
 *
 * Description:
 *   The left-side bearing, i.e., the horizontal distance from the
 *   current pen position to the left border of the glyph bitmap.
 *   
 * Note:
 *   FT2::BitmapGlyph is a subclass of FT2::Glyph.
 *
 * Examples:
 *   x = bmap_glyph.left
 *   
 */
static VALUE ft_bmapglyph_left(VALUE self) {
  FT_BitmapGlyph *glyph;
  Data_Get_Struct(self, FT_BitmapGlyph, glyph);
  return INT2FIX((*glyph)->left);
}

/*
 * Get the FT2::Bitmap of a FT2::BitmapGlyph object.
 *
 * Examples:
 *   bmap = bmap_glyph.bitmap
 *
 */
static VALUE ft_bmapglyph_bitmap(VALUE self) {
  FT_BitmapGlyph *glyph;
  Data_Get_Struct(self, FT_BitmapGlyph, glyph);
  return Data_Wrap_Struct(cBitmapGlyph, 0, glyph_free, &(*glyph)->bitmap);
}


/*****************************/
/* FT2::OutlineGlyph methods */
/*****************************/

/*
 * Constructor for FT2::OutlineGlyph class.
 *
 * This method is currently empty.  You should never call this method
 * directly unless you're instantiating a derived class (ie, you know
 * what you're doing).
 *
 */
static VALUE ft_outlineglyph_init(VALUE self) {
  return self;
}

/*
 * Get the outline of a FT2::OutlineGlyph object.
 *
 * Note:
 *   FT2::OutlineGlyph is a subclass of FT2::Glyph.
 *
 * Examples:
 *   outline = oline.outline
 *
 */
static VALUE ft_outlineglyph_outline(VALUE self) {
  FT_OutlineGlyph *glyph;
  Data_Get_Struct(self, FT_OutlineGlyph, glyph);
  return Data_Wrap_Struct(cOutline, 0, dont_free, &(*glyph)->outline);
}
static void define_constants(void) {
  /***********************************/
  /* define FT2::PixelMode constants */
  /***********************************/
  mPixelMode = rb_define_module_under(mFt2, "PixelMode");
  rb_define_const(mPixelMode, "NONE", INT2FIX(FT_PIXEL_MODE_NONE));
  rb_define_const(mPixelMode, "MONO", INT2FIX(FT_PIXEL_MODE_MONO));
  rb_define_const(mPixelMode, "GRAY2", INT2FIX(FT_PIXEL_MODE_GRAY2));
  rb_define_const(mPixelMode, "GRAY4", INT2FIX(FT_PIXEL_MODE_GRAY4));
  rb_define_const(mPixelMode, "LCD", INT2FIX(FT_PIXEL_MODE_LCD));
  rb_define_const(mPixelMode, "LCD_V", INT2FIX(FT_PIXEL_MODE_LCD_V));
  rb_define_const(mPixelMode, "MAX", INT2FIX(FT_PIXEL_MODE_MAX));

  /* old pixel mode stuff (according to tutorial)
  rb_define_const(mPixelMode, "NONE", INT2FIX(ft_pixel_mode_none));
  rb_define_const(mPixelMode, "MONO", INT2FIX(ft_pixel_mode_mono));
  rb_define_const(mPixelMode, "GRAYS", INT2FIX(ft_pixel_mode_grays));
  rb_define_const(mPixelMode, "PAL2", INT2FIX(ft_pixel_mode_pal2));
  rb_define_const(mPixelMode, "PAL4", INT2FIX(ft_pixel_mode_pal4));
  rb_define_const(mPixelMode, "PAL8", INT2FIX(ft_pixel_mode_pal8));
  rb_define_const(mPixelMode, "RGB15", INT2FIX(ft_pixel_mode_rgb15));
  rb_define_const(mPixelMode, "RGB16", INT2FIX(ft_pixel_mode_rgb16));
  rb_define_const(mPixelMode, "RGB24", INT2FIX(ft_pixel_mode_rgb24));
  rb_define_const(mPixelMode, "RGB32", INT2FIX(ft_pixel_mode_rgb32));
  */

  /*************************************/
  /* define FT2::PaletteMode constants */
  /*************************************/
  /* FIXME: doesn't compile, so I'm disabling it for now
  mPaletteMode = rb_define_module_under(mFt2, "PaletteMode");
  rb_define_const(mPaletteMode, "RGB", INT2FIX(ft_palette_mode_rgb));
  rb_define_const(mPaletteMode, "RGBA", INT2FIX(ft_palette_mode_rgba));
  */

  /*************************************/
  /* define FT2::GlyphFormat constants */
  /*************************************/
  mGlyphFormat = rb_define_module_under(mFt2, "GlyphFormat");
  rb_define_const(mGlyphFormat, "COMPOSITE", INT2NUM(ft_glyph_format_composite));
  rb_define_const(mGlyphFormat, "BITMAP", INT2NUM(ft_glyph_format_bitmap));
  rb_define_const(mGlyphFormat, "OUTLINE", INT2NUM(ft_glyph_format_outline));
  rb_define_const(mGlyphFormat, "PLOTTER", INT2NUM(ft_glyph_format_plotter));

  /**********************************/
  /* define FT2::Encoding constants */
  /**********************************/
  mEnc = rb_define_module_under(mFt2, "Encoding");
  rb_define_const(mEnc, "NONE", INT2FIX(ft_encoding_none));
  rb_define_const(mEnc, "SYMBOL", INT2FIX(ft_encoding_symbol));
  rb_define_const(mEnc, "UNICODE", INT2FIX(ft_encoding_unicode));
  rb_define_const(mEnc, "LATIN_1", INT2FIX(ft_encoding_latin_1));
  /* disabled as per the ft2 header documentation */
  /* rb_define_const(mEnc, "LATIN_2", INT2FIX(ft_encoding_latin_2)); */
  rb_define_const(mEnc, "SJIS", INT2FIX(ft_encoding_sjis));
  rb_define_const(mEnc, "GB2312", INT2FIX(ft_encoding_gb2312));
  rb_define_const(mEnc, "BIG5", INT2FIX(ft_encoding_big5));
  rb_define_const(mEnc, "WANSUNG", INT2FIX(ft_encoding_wansung));
  rb_define_const(mEnc, "JOHAB", INT2FIX(ft_encoding_johab));
  rb_define_const(mEnc, "ADOBE_STANDARD", INT2FIX(ft_encoding_adobe_standard));
  rb_define_const(mEnc, "ADOBE_EXPERT", INT2FIX(ft_encoding_adobe_expert));
  rb_define_const(mEnc, "ADOBE_CUSTOM", INT2FIX(ft_encoding_adobe_custom));
  rb_define_const(mEnc, "APPLE_ROMAN", INT2FIX(ft_encoding_apple_roman));

  /************************************/
  /* define FT2::RenderMode constants */
  /************************************/
  mRenderMode = rb_define_module_under(mFt2, "RenderMode");
  rb_define_const(mRenderMode, "NORMAL", INT2FIX(ft_render_mode_normal));
  rb_define_const(mRenderMode, "MONO", INT2FIX(ft_render_mode_mono));

  /*************************************/
  /* define FT2::KerningMode constants */
  /*************************************/
  mKerningMode = rb_define_module_under(mFt2, "KerningMode");
  rb_define_const(mKerningMode, "DEFAULT", INT2FIX(ft_kerning_default));
  rb_define_const(mKerningMode, "UNFITTED", INT2FIX(ft_kerning_unfitted));
  rb_define_const(mKerningMode, "UNSCALED", INT2FIX(ft_kerning_unscaled));

  /******************************/
  /* define FT2::Load constants */
  /******************************/
  mLoad = rb_define_module_under(mFt2, "Load");
  rb_define_const(mLoad, "DEFAULT", INT2NUM(FT_LOAD_DEFAULT));
  rb_define_const(mLoad, "RENDER", INT2NUM(FT_LOAD_RENDER));
  rb_define_const(mLoad, "MONOCHROME", INT2NUM(FT_LOAD_MONOCHROME));
  rb_define_const(mLoad, "LINEAR_DESIGN", INT2NUM(FT_LOAD_LINEAR_DESIGN));
  rb_define_const(mLoad, "NO_SCALE", INT2NUM(FT_LOAD_NO_SCALE));
  rb_define_const(mLoad, "NO_HINTING", INT2NUM(FT_LOAD_NO_HINTING));
  rb_define_const(mLoad, "NO_BITMAP", INT2NUM(FT_LOAD_NO_BITMAP));
  rb_define_const(mLoad, "CROP_BITMAP", INT2NUM(FT_LOAD_CROP_BITMAP));
  rb_define_const(mLoad, "VERTICAL_LAYOUT", INT2NUM(FT_LOAD_VERTICAL_LAYOUT));
  rb_define_const(mLoad, "IGNORE_TRANSFORM", INT2NUM(FT_LOAD_IGNORE_TRANSFORM));
  rb_define_const(mLoad, "IGNORE_GLOBAL_ADVANCE_WIDTH", INT2NUM(FT_LOAD_IGNORE_GLOBAL_ADVANCE_WIDTH));
  rb_define_const(mLoad, "FORCE_AUTOHINT", INT2NUM(FT_LOAD_FORCE_AUTOHINT));
  rb_define_const(mLoad, "NO_RECURSE", INT2NUM(FT_LOAD_NO_RECURSE));
  rb_define_const(mLoad, "PEDANTIC", INT2NUM(FT_LOAD_PEDANTIC));

  /***********************************/
  /* define FT2::GlyphBBox constants */
  /***********************************/
  mBBox = rb_define_module_under(mFt2, "GlyphBBox");
  rb_define_const(mBBox, "UNSCALED", INT2NUM(ft_glyph_bbox_unscaled));
  rb_define_const(mBBox, "SUBPIXELS", INT2NUM(ft_glyph_bbox_subpixels));
  rb_define_const(mBBox, "GRIDFIT", INT2NUM(ft_glyph_bbox_gridfit));
  rb_define_const(mBBox, "TRUNCATE", INT2NUM(ft_glyph_bbox_truncate));
  rb_define_const(mBBox, "PIXELS", INT2NUM(ft_glyph_bbox_pixels));
}

void Init_ft2(void) {
  FT_Error err;

  if ((err = FT_Init_FreeType(&library)) != FT_Err_Ok)
    handle_error(err);

  /* define top-level FT2 module */
  mFt2 = rb_define_module("FT2");

  /* define FT2::VERSION */
  rb_define_const(mFt2, "VERSION", rb_str_new2(VERSION));
  rb_define_singleton_method(mFt2, "version", ft_version, 0);
  
  define_constants();
  
  /****************************/
  /* define FT2::Bitmap class */
  /****************************/
  cBitmap = rb_define_class_under(mFt2, "Bitmap", rb_cObject);
  rb_define_singleton_method(cBitmap, "initialize", ft_bitmap_init, 0);
  rb_define_method(cBitmap, "rows", ft_bitmap_rows, 0);
  rb_define_method(cBitmap, "width", ft_bitmap_width, 0);
  rb_define_method(cBitmap, "pitch", ft_bitmap_pitch, 0);
  rb_define_method(cBitmap, "buffer", ft_bitmap_buffer, 0);
  rb_define_method(cBitmap, "num_grays", ft_bitmap_num_grays, 0);
  rb_define_method(cBitmap, "pixel_mode", ft_bitmap_pixel_mode, 0);
  rb_define_method(cBitmap, "palette_mode", ft_bitmap_palette_mode, 0);
  rb_define_method(cBitmap, "palette", ft_bitmap_palette, 0);

  /*****************************/
  /* define FT2::CharMap class */
  /*****************************/
  cCharMap = rb_define_class_under(mFt2, "CharMap", rb_cObject);

  /**************************/
  /* define FT2::Face class */
  /**************************/
  cFace = rb_define_class_under(mFt2, "Face", rb_cObject);
  rb_define_singleton_method(cFace, "new", ft_face_new, -1);
  rb_define_singleton_method(cFace, "load", ft_face_new, -1);
  rb_define_singleton_method(cFace, "new_from_memory", ft_face_new_from_memory, -1);

  rb_define_singleton_method(cFace, "initialize", ft_face_init, 0);

  rb_define_method(cFace, "faces", ft_face_faces, 0);
  rb_define_alias(cFace, "num_faces", "faces");
  
  rb_define_method(cFace, "index", ft_face_index, 0);
  rb_define_alias(cFace, "face_index", "index");
  
  rb_define_method(cFace, "flags", ft_face_flags, 0);
  rb_define_alias(cFace, "face_flags", "flags");

  rb_define_const(cFace, "SCALABLE", INT2FIX(FT_FACE_FLAG_SCALABLE));
  rb_define_const(cFace, "FIXED_SIZES", INT2FIX(FT_FACE_FLAG_FIXED_SIZES));
  rb_define_const(cFace, "FIXED_WIDTH", INT2FIX(FT_FACE_FLAG_FIXED_WIDTH));
  rb_define_const(cFace, "FIXED_HORIZONTAL", INT2FIX(FT_FACE_FLAG_HORIZONTAL));
  rb_define_const(cFace, "FIXED_VERTICAL", INT2FIX(FT_FACE_FLAG_VERTICAL));
  rb_define_const(cFace, "SFNT", INT2FIX(FT_FACE_FLAG_SFNT));
  rb_define_const(cFace, "KERNING", INT2FIX(FT_FACE_FLAG_KERNING));
  rb_define_const(cFace, "MULTIPLE_MASTERS", INT2FIX(FT_FACE_FLAG_MULTIPLE_MASTERS));
  rb_define_const(cFace, "GLYPH_NAMES", INT2FIX(FT_FACE_FLAG_GLYPH_NAMES));
  rb_define_const(cFace, "EXTERNAL_STREAM", INT2FIX(FT_FACE_FLAG_EXTERNAL_STREAM));
  rb_define_const(cFace, "FAST_GLYPHS", INT2FIX(FT_FACE_FLAG_FAST_GLYPHS));
  
  rb_define_method(cFace, "scalable?", ft_face_flag_scalable, 0);
  rb_define_method(cFace, "fixed_sizes?", ft_face_flag_fixed_sizes, 0);
  rb_define_method(cFace, "fixed_width?", ft_face_flag_fixed_width, 0);
  rb_define_method(cFace, "horizontal?", ft_face_flag_horizontal, 0);
  rb_define_method(cFace, "vertical?", ft_face_flag_vertical, 0);
  rb_define_method(cFace, "sfnt?", ft_face_flag_sfnt, 0);
  rb_define_method(cFace, "kerning?", ft_face_flag_kerning, 0);
  rb_define_method(cFace, "external_stream?", ft_face_flag_external_stream, 0);
  rb_define_method(cFace, "fast_glyphs?", ft_face_flag_fast_glyphs, 0);
  
  rb_define_method(cFace, "style_flags", ft_face_style_flags, 0);
  
  rb_define_const(cFace, "BOLD", INT2FIX(FT_STYLE_FLAG_BOLD));
  rb_define_const(cFace, "ITALIC", INT2FIX(FT_STYLE_FLAG_ITALIC));

  rb_define_method(cFace, "bold?", ft_face_flag_bold, 0);
  rb_define_method(cFace, "italic?", ft_face_flag_italic, 0);
  
  rb_define_method(cFace, "glyphs", ft_face_glyphs, 0);
  rb_define_alias(cFace, "num_glyphs", "glyphs");

  rb_define_method(cFace, "family", ft_face_family, 0);
  rb_define_method(cFace, "style", ft_face_style, 0);

  rb_define_method(cFace, "fixed_sizes", ft_face_fixed_sizes, 0);
  rb_define_alias(cFace, "num_fixed_sizes", "fixed_sizes");

  rb_define_method(cFace, "available_sizes", ft_face_available_sizes, 0);
  rb_define_alias(cFace, "num_available_sizes", "available_sizes");
  
  rb_define_method(cFace, "num_charmaps", ft_face_num_charmaps, 0);
  rb_define_method(cFace, "charmaps", ft_face_charmaps, 0);
  
  rb_define_method(cFace, "bbox", ft_face_bbox, 0);

  rb_define_method(cFace, "units_per_em", ft_face_units_per_em, 0);
  rb_define_alias(cFace, "units_per_EM", "units_per_em");

  rb_define_method(cFace, "ascender", ft_face_ascender, 0);
  rb_define_method(cFace, "descender", ft_face_descender, 0);
  rb_define_method(cFace, "height", ft_face_height, 0);
  rb_define_method(cFace, "max_advance_width", ft_face_max_advance_width, 0);
  rb_define_method(cFace, "max_advance_height", ft_face_max_advance_height, 0);
  rb_define_method(cFace, "underline_position", ft_face_underline_position, 0);
  rb_define_method(cFace, "underline_thickness", ft_face_underline_thickness, 0);
  rb_define_method(cFace, "glyph", ft_face_glyph, 0);
  rb_define_method(cFace, "size", ft_face_size, 0);
  rb_define_method(cFace, "charmap", ft_face_charmap, 0);

  rb_define_method(cFace, "attach", ft_face_attach, 1);
  rb_define_alias(cFace, "attach_file", "attach");

  rb_define_method(cFace, "load_glyph", ft_face_load_glyph, 2);
  rb_define_method(cFace, "load_char", ft_face_load_char, 2);

  rb_define_method(cFace, "char_index", ft_face_char_index, 1);
  rb_define_method(cFace, "name_index", ft_face_name_index, 1);
  
  rb_define_method(cFace, "kerning", ft_face_kerning, 3);
  rb_define_alias(cFace, "get_kerning", "kerning");
  
  rb_define_method(cFace, "glyph_name", ft_face_glyph_name, 1);
  rb_define_method(cFace, "postscript_name", ft_face_ps_name, 0);
  rb_define_alias(cFace, "name", "postscript_name");

  rb_define_method(cFace, "select_charmap", ft_face_select_charmap, 1);
  rb_define_method(cFace, "set_charmap", ft_face_set_charmap, 1);
  rb_define_alias(cFace, "charmap=", "set_charmap");
  
  rb_define_method(cFace, "first_char", ft_face_first_char, 0);
  rb_define_method(cFace, "next_char", ft_face_next_char, 1);
  
  rb_define_method(cFace, "current_charmap", ft_face_current_charmap, 0);
  
  rb_define_method(cFace, "set_char_size", ft_face_set_char_size, 4);
  rb_define_method(cFace, "set_pixel_sizes", ft_face_set_pixel_sizes, 2);
  rb_define_method(cFace, "set_transform", ft_face_set_transform, 2);

  /**********************************/
  /* define FT2::GlyphMetrics class */
  /**********************************/
  cGlyphMetrics = rb_define_class_under(mFt2, "GlyphMetrics", rb_cObject);
  rb_define_singleton_method(cGlyphMetrics, "initialize", ft_glyphmetrics_init, 0);

  rb_define_method(cGlyphMetrics, "width", ft_glyphmetrics_width, 0);
  rb_define_alias(cGlyphMetrics, "w", "width");
  rb_define_method(cGlyphMetrics, "height", ft_glyphmetrics_height, 0);
  rb_define_alias(cGlyphMetrics, "h", "height");

  rb_define_method(cGlyphMetrics, "h_bearing_x", ft_glyphmetrics_h_bear_x, 0);
  rb_define_alias(cGlyphMetrics, "horiBearingX", "h_bearing_x");
  rb_define_alias(cGlyphMetrics, "h_bear_x", "h_bearing_x");
  rb_define_alias(cGlyphMetrics, "hbx", "h_bearing_x");
  rb_define_method(cGlyphMetrics, "h_bearing_y", ft_glyphmetrics_h_bear_y, 0);
  rb_define_alias(cGlyphMetrics, "horiBearingY", "h_bearing_y");
  rb_define_alias(cGlyphMetrics, "h_bear_y", "h_bearing_y");
  rb_define_alias(cGlyphMetrics, "hby", "h_bearing_y");
  rb_define_method(cGlyphMetrics, "h_advance", ft_glyphmetrics_h_advance, 0);
  rb_define_alias(cGlyphMetrics, "horiAdvance", "h_advance");
  rb_define_alias(cGlyphMetrics, "ha", "h_advance");

  rb_define_method(cGlyphMetrics, "v_bearing_x", ft_glyphmetrics_v_bear_x, 0);
  rb_define_alias(cGlyphMetrics, "vertBearingX", "v_bearing_x");
  rb_define_alias(cGlyphMetrics, "v_bear_x", "v_bearing_x");
  rb_define_alias(cGlyphMetrics, "vbx", "v_bearing_x");
  rb_define_method(cGlyphMetrics, "v_bearing_y", ft_glyphmetrics_v_bear_y, 0);
  rb_define_alias(cGlyphMetrics, "vertBearingY", "v_bearing_y");
  rb_define_alias(cGlyphMetrics, "v_bear_y", "v_bearing_y");
  rb_define_alias(cGlyphMetrics, "vby", "v_bearing_y");
  rb_define_method(cGlyphMetrics, "v_advance", ft_glyphmetrics_v_advance, 0);
  rb_define_alias(cGlyphMetrics, "vertAdvance", "v_advance");
  rb_define_alias(cGlyphMetrics, "va", "v_advance");

  /*******************************/
  /* define FT2::GlyphSlot class */
  /*******************************/
  cGlyphSlot = rb_define_class_under(mFt2, "GlyphSlot", rb_cObject);
  rb_define_singleton_method(cGlyphSlot, "initialize", ft_glyphslot_init, 0);

  rb_define_method(cGlyphSlot, "library", ft_glyphslot_library, 0);
  rb_define_method(cGlyphSlot, "face", ft_glyphslot_face, 0);
  rb_define_method(cGlyphSlot, "next", ft_glyphslot_next, 0);
  rb_define_method(cGlyphSlot, "metrics", ft_glyphslot_metrics, 0);

  rb_define_method(cGlyphSlot, "h_advance", ft_glyphslot_h_advance, 0);
  rb_define_alias(cGlyphSlot, "linearHoriAdvance", "h_advance");
  rb_define_alias(cGlyphSlot, "h_adv", "h_advance");
  rb_define_alias(cGlyphSlot, "ha", "h_advance");

  rb_define_method(cGlyphSlot, "v_advance", ft_glyphslot_v_advance, 0);
  rb_define_alias(cGlyphSlot, "linearVertAdvance", "h_advance");
  rb_define_alias(cGlyphSlot, "v_adv", "h_advance");
  rb_define_alias(cGlyphSlot, "va", "h_advance");
  
  rb_define_method(cGlyphSlot, "advance", ft_glyphslot_advance, 0);
  rb_define_method(cGlyphSlot, "format", ft_glyphslot_format, 0);
  rb_define_method(cGlyphSlot, "bitmap", ft_glyphslot_bitmap, 0);
  rb_define_method(cGlyphSlot, "bitmap_left", ft_glyphslot_bitmap_left, 0);
  rb_define_method(cGlyphSlot, "bitmap_top", ft_glyphslot_bitmap_top, 0);
  rb_define_method(cGlyphSlot, "outline", ft_glyphslot_outline, 0);
  rb_define_method(cGlyphSlot, "num_subglyphs", ft_glyphslot_num_subglyphs, 0);
  rb_define_method(cGlyphSlot, "subglyphs", ft_glyphslot_subglyphs, 0);
  rb_define_method(cGlyphSlot, "control_data", ft_glyphslot_control_data, 0);
  rb_define_method(cGlyphSlot, "control_len", ft_glyphslot_control_len, 0);
  
  rb_define_method(cGlyphSlot, "render", ft_glyphslot_render, 1);
  rb_define_alias(cGlyphSlot, "render_glyph", "render");

  rb_define_method(cGlyphSlot, "glyph", ft_glyphslot_glyph, 0);
  rb_define_alias(cGlyphSlot, "get_glyph", "glyph");

  /*****************************/
  /* define FT2::Library class */
  /*****************************/
  cLibrary = rb_define_class_under(mFt2, "Library", rb_cObject);
  
  /****************************/
  /* define FT2::Memory class */
  /****************************/
  cMemory = rb_define_class_under(mFt2, "Memory", rb_cObject);
  
  /*****************************/
  /* define FT2::Outline class */
  /*****************************/
  cOutline = rb_define_class_under(mFt2, "Outline", rb_cObject);

  /**************************/
  /* define FT2::Size class */
  /**************************/
  cSize = rb_define_class_under(mFt2, "Size", rb_cObject);
  rb_define_singleton_method(cSize, "initialize", ft_size_init, 0);
  rb_define_method(cSize, "face", ft_size_face, 0);
  rb_define_method(cSize, "metrics", ft_size_metrics, 0);

  /*********************************/
  /* define FT2::SizeMetrics class */
  /*********************************/
  cSizeMetrics = rb_define_class_under(mFt2, "SizeMetrics", rb_cObject);
  rb_define_singleton_method(cSizeMetrics, "initialize", ft_size_metrics_init, 0);
  rb_define_method(cSizeMetrics, "x_ppem", ft_size_metrics_x_ppem, 0);
  rb_define_method(cSizeMetrics, "y_ppem", ft_size_metrics_y_ppem, 0);
  rb_define_method(cSizeMetrics, "x_scale", ft_size_metrics_x_scale, 0);
  rb_define_method(cSizeMetrics, "y_scale", ft_size_metrics_y_scale, 0);

  /***************************/
  /* define FT2::Glyph class */
  /***************************/
  cGlyph = rb_define_class_under(mFt2, "Glyph", rb_cObject);
  rb_define_singleton_method(cGlyph, "initialize", ft_glyph_init, 0);
  rb_define_method(cGlyph, "library", ft_glyph_library, 0);
  rb_define_method(cGlyph, "class", ft_glyph_class, 0);
  rb_define_alias(cGlyph, "clazz", "class");
  rb_define_method(cGlyph, "format", ft_glyph_format, 0);
  rb_define_method(cGlyph, "advance", ft_glyph_advance, 0);

  rb_define_method(cGlyph, "dup", ft_glyph_dup, 0);
  rb_define_alias(cGlyph, "copy", "dup");

  rb_define_method(cGlyph, "transform", ft_glyph_transform, 2);

  rb_define_method(cGlyph, "cbox", ft_glyph_cbox, 1);
  rb_define_alias(cGlyph, "control_box", "cbox");

  rb_define_method(cGlyph, "to_bmap", ft_glyph_to_bmap, 3);
  rb_define_alias(cGlyph, "to_bitmap", "to_bmap");

  /*********************************/
  /* define FT2::BitmapGlyph class */
  /*********************************/
  cBitmapGlyph = rb_define_class_under(mFt2, "BitmapGlyph", cGlyph);
  rb_define_singleton_method(cBitmapGlyph, "initialize", ft_bmapglyph_init, 0);

  rb_define_method(cBitmapGlyph, "left", ft_bmapglyph_left, 0);
  rb_define_method(cBitmapGlyph, "top", ft_bmapglyph_top, 0);
  rb_define_method(cBitmapGlyph, "bitmap", ft_bmapglyph_bitmap, 0);

  /*********************************/
  /* define FT2::OutlineGlyph class */
  /*********************************/
  cOutlineGlyph = rb_define_class_under(mFt2, "OutlineGlyph", cGlyph);
  rb_define_singleton_method(cOutlineGlyph, "initialize", ft_outlineglyph_init, 0);

  /********************************/
  /* define FT2::GlyphClass class */
  /********************************/
  cGlyphClass = rb_define_class_under(mFt2, "GlyphClass", rb_cObject);

  /******************************/
  /* define FT2::SubGlyph class */
  /******************************/
  cSubGlyph = rb_define_class_under(mFt2, "SubGlyph", rb_cObject);
}
