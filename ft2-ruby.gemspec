Gem::Specification.new do |gem|
  gem.authors       = ["Paul Duncan"]
  gem.email         = "pabs@pablotron.org"
  gem.description   = %Q{Ruby libraries to FreeType2}
  gem.summary       = %Q{Ruby libraries to FreeType2}
  gem.homepage      = "http://www.pablotron.org/"

  gem.files         = `git ls-files`.split($\)
  gem.executables   = gem.files.grep(%r{^bin/}).map{ |f| File.basename(f) }
  gem.extensions    = ['extconf.rb']
  gem.test_files    = gem.files.grep(%r{^(test|spec|features)/})
  gem.name          = "ft2-ruby"
  gem.version       = '0.1.4'

  gem.add_development_dependency 'bundler'
  gem.add_development_dependency 'rake'
end
