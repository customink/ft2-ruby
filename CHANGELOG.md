# ft2-ruby changelog #

## 0.1.4 ##
### Fri Apr 27 11:39:28 2012 by [sethvargo](http://www.github.com/sethvargo) ###
- Added MIT license

### Fri Apr 27 11:39:28 2012 by [wingrunr21](http://www.github.com/wingrunr21) ###
- Restructured gem to match modern standards
- Changed instances of RSTRING(foo)->ptr to RSTRING_PTR(foo)
- Gem is now managed with Bundler.
- Removed unnecessary files (MANIFEST, VERSION)
- Converted information files to Markdown

## 0.1.3 ##
### Tue Oct 19 14:07:07 2010 by [dvanderson](http://www.github.com/dvanderson) ###
- fix a cast that was breaking compilation on OS X

## 0.1.2 ##
### Wed Oct 19 15:23:38 2010 by [dvanderson](http://www.github.com/dvanderson) ###
- fix to get cbox for glyphs

### Wed Feb 10 19:45:20 2010 by [andrewwillis](http://www.github.com/andrewwillis) ###
- move gem to jeweler
- Update native extension to work correctly with ft2
- fix compiler errors

## 0.1.1 and before ##
### Thu Nov 21 15:18:26 2002 by [pabs](pabs@pablotron.org) and [andrewwillis](http://www.github.com/andrewwillis) ###
  - ft2.c: finished documenting and implementing FT2::Glyph,
    FT2::BitmapGlyph, and FT2::OutlineGlyph.
  - ft2.c: compiles clean with -W -Wall -pedantic again
  - TODO: updated

### Wed Nov 20 16:11:10 2002 by [pabs](pabs@pablotron.org) ###
  - ft2.c: finished documenting the FT2::SizeMetrics methods (had to
    pull the documentation for them out of the second part of the
    FreeType2 tutorial, since they're not available in the API
    reference).

### Sat Nov 16 01:45:42 2002 by [pabs](pabs@pablotron.org) ###
  - ft2.c: finished documentation for FT2::GlyphSlot methods.

### Fri Nov 15 15:15:37 2002 by [pabs](pabs@pablotron.org) ###
  - ft2.c: finished documentation for FT2::GlyphMetrics methods.

### Fri Nov 15 03:52:46 2002 by [pabs](pabs@pablotron.org) ###
  - ft2.c: added FT2::Face#current_charmap

### Fri Nov 15 03:27:50 2002 by [pabs](pabs@pablotron.org) ###
  - ft2.c: finished documentation for FT2::Face methods.

### Tue Nov 12 17:20:23 2002 by [pabs](pabs@pablotron.org) ###
  - made some changes according to the notes in the freetype2 headers:
  - ft2.c: removed FT2::PixelMode::RGB and FT2::PixelMode::RGBA constants
  - ft2.c: removed Ft2::PaletteMode
  - compiles clean with -W -Wall -pedantic again

### Thu Aug 15 09:39:24 2002 by [pabs](pabs@pablotron.org) ###
  - created changelog.

