#!/usr/bin/env ruby

$FONTDIR = '/usr/X11R6/lib/X11/fonts/truetype'

require 'readline'
require 'ft2'

$commands = {
  'quit' => 'Quit program.',
  'list' => 'List all fonts.',
  'help' => 'Print this list of commands.',
}
$fonts = {}

def tab_complete(str)
  ($commands.keys.map { |i| i =~ /^#{str}/i ? i : nil }.compact! || []) +
  $fonts.keys.map { |i| i =~ /^#{str}/i ? i : nil }.compact!
end

def show_font_info(str)
  path, face = $fonts[str]
  if path and face
    puts 'Name:         ' << face.name,
         'Path:         ' << path,
         'Family:       ' << face.family,
         'Style:        ' << face.style,
         'Bold:         ' << face.bold?.to_s,
         'Italic:       ' << face.italic?.to_s,
         'Scalable:     ' << face.scalable?.to_s,
         'Horizontal:   ' << face.horizontal?.to_s,
         'Vertical:     ' << face.vertical?.to_s,
         'Kerning:      ' << face.kerning?.to_s,
         'Fast Glyphs:  ' << face.fast_glyphs?.to_s,
         'Num Glyphs:   ' << face.num_glyphs.to_s,
         'Num Charmaps: ' << face.num_charmaps.to_s
         
  else
    $stderr.puts "Unknown font \"#{str}\""
  end
end

count = 0
Dir["#$FONTDIR/*"].each { |i|
  count += 1
  next if i =~ /^\./
  begin
    face = FT2::Face.load(i)
  rescue Exception
    next
  end
  # puts [face, face.name, face.num_glyphs ].join ', '
  $fonts[face.name] = [i, face]
}
puts "Loaded #{$fonts.keys.size} of #{count} fonts."

# Readline::completion_case_fold = true
Readline::completion_proc = Proc.new { |str| tab_complete(str) }

done = false
until done
  line = Readline::readline '> ', true

  case line
  when %r|^q(uit)?|
    done = true
  when %r|^h(elp)?|
    $commands.keys.sort { |a, b| a <=> b }.each { |key|
      puts "#{key}: #{$commands[key]}"
    }
  when %r|^l(ist\|s)?|
    $fonts.each { |key, val|
      path, face = val
      puts [key, face.num_glyphs].join ', '
    }
  when /^$/
    next
  else
    show_font_info(line.strip)
  end
end
