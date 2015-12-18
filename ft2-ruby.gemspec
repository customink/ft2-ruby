# coding: utf-8
lib = File.expand_path('../lib', __FILE__)
$LOAD_PATH.unshift(lib) unless $LOAD_PATH.include?(lib)
require 'ft2-ruby/version'

Gem::Specification.new do |spec|
  spec.name          = "ft2-ruby"
  spec.version       = FT2::VERSION
  spec.authors       = ["Paul Duncan", "Stafford Brunk"]
  spec.email         = ["pabs@pablotron.org", "sbrunk@customink.com"]
  spec.description   = %q{FreeType2 bindings for Ruby}
  spec.summary       = %q{FreeType2 bindings for Ruby}
  spec.homepage      = "https://github.com/customink/ft2-ruby"
  spec.license       = "MIT"

  spec.files         = `git ls-files -z`.split("\x0")
  spec.executables   = spec.files.grep(%r{^bin/}) { |f| File.basename(f) }
  spec.extensions    = ['ext/ft2-ruby/extconf.rb']
  spec.test_files    = spec.files.grep(%r{^(test|spec|features)/})
  spec.require_paths = ["lib"]

  spec.add_development_dependency "rake-compiler"
end
