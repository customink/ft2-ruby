#!/usr/bin/env ruby

require 'ft2'

puts FT2::version
puts FT2::VERSION

face = FT2::Face.new 'fonts/yudit.ttf'
puts "glyphs:   #{face.glyphs}",
     "charmaps: #{face.num_charmaps}",
     "horiz:    #{face.horizontal?}",
     "vert:     #{face.vertical?}"

# cycle through default charmap
puts 'listing character codes with first_char/next_char'
c_code, g_idx = face.first_char
while g_idx != 0
  puts "#{c_code} => " << face.glyph_name(g_idx)
  c_code, g_idx = face.next_char c_code
end

puts 'listing charcodes with current_charmap'

cmap = face.current_charmap
cmap.keys.sort.each { |c_code| 
  puts "#{c_code} => " << face.glyph_name(cmap[c_code])
}
