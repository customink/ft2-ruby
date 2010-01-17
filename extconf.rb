require 'mkmf'

ft2_config = with_config("freetype-config", "freetype-config")

$CFLAGS << ' ' << `#{ft2_config} --cflags`.chomp
$LDFLAGS << ' ' << `#{ft2_config} --libs`.chomp

have_library("freetype", "FT_Init_FreeType") and
  create_makefile("ft2")

