CHANGELOG
=========

v0.1.4
------
* Upgraded C-extension to compile natively on Ruby 1.9

Old Changelog
-------------

Thu Aug 15 09:39:24 2002, pabs <pabs@pablotron.org>
  * created changelog

Tue Nov 12 17:20:23 2002, pabs <pabs@pablotron.org>
  * made some changes according to the notes in the freetype2 headers:
  * ft2.c: removed FT2::PixelMode::RGB and FT2::PixelMode::RGBA constants
  * ft2.c: removed Ft2::PaletteMode
  * compiles clean with -W -Wall -pedantic again

Fri Nov 15 03:27:50 2002, pabs <pabs@pablotron.org>
  * ft2.c: finished documentation for FT2::Face methods.

Fri Nov 15 03:52:46 2002, pabs <pabs@pablotron.org>
  * ft2.c: added FT2::Face#current_charmap

Fri Nov 15 15:15:37 2002, pabs <pabs@pablotron.org>
  * ft2.c: finished documentation for FT2::GlyphMetrics methods.

Sat Nov 16 01:45:42 2002, pabs <pabs@pablotron.org>
  * ft2.c: finished documentation for FT2::GlyphSlot methods.

Wed Nov 20 16:11:10 2002, pabs <pabs@pablotron.org>
  * ft2.c: finished documenting the FT2::SizeMetrics methods (had to
    pull the documentation for them out of the second part of the
    FreeType2 tutorial, since they're not available in the API
    reference).

Thu Nov 21 15:18:26 2002, pabs <pabs@pablotron.org>
  * ft2.c: finished documenting and implementing FT2::Glyph,
    FT2::BitmapGlyph, and FT2::OutlineGlyph.
  * ft2.c: compiles clean with -W -Wall -pedantic again
  * TODO: updated
