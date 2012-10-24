# -*- encoding: utf-8 -*-
require File.expand_path('../lib/ft2-ruby/version', __FILE__)

Gem::Specification.new do |gem|
  gem.authors       = ["Paul Duncan"]
  gem.email         = ["pabs@pablotron.org"]
  gem.description   = %q{FreeType2 bindings for Ruby 1.8 and 1.9}
  gem.summary       = %q{FreeType2 bindings for Ruby}
  gem.homepage      = "https://github.com/customink/ft2-ruby"

  gem.files         = `git ls-files`.split($\)
  gem.executables   = gem.files.grep(%r{^bin/}).map{ |f| File.basename(f) }
  gem.extensions    = ['ext/ft2-ruby/extconf.rb']
  gem.test_files    = gem.files.grep(%r{^(test|spec|features)/})
  gem.name          = "ft2-ruby"
  gem.require_paths = ["lib"]
  gem.version       = FT2::VERSION
end
