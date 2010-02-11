require 'rubygems'
require 'rake'

begin
  require 'jeweler'
  Jeweler::Tasks.new do |gem|
    gem.name = "ft2-ruby"
    gem.summary = %Q{Ruby libraries to FreeType2}
    gem.email = "pabs@pablotron.org"
    gem.homepage = "http://www.pablotron.org/"
    gem.authors = ["Paul Duncan"]
    gem.extensions = ["extconf.rb"]
    # gem is a Gem::Specification... see http://www.rubygems.org/read/chapter/20 for additional settings
  end
  Jeweler::GemcutterTasks.new
rescue LoadError
  puts "Jeweler (or a dependency) not available. Install it with: gem install jeweler"
end

# 
# task :default => :test
# 
require 'rake/rdoctask'
Rake::RDocTask.new do |rdoc|
  version = File.exist?('VERSION') ? File.read('VERSION') : ""

  rdoc.rdoc_dir = 'rdoc'
  rdoc.title = "ft2-ruby #{version}"
  rdoc.rdoc_files.include('README')
  rdoc.rdoc_files.include('TODO')
  rdoc.rdoc_files.include('ft2.c')
end
