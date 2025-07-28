MRuby::Gem::Specification.new('picoruby-m5unified') do |spec|
  spec.license = 'MIT'
  spec.author  = 'HASUMI Hitoshi'
  spec.summary = 'M5Unified integration for PicoRuby'
  
  # Add dependencies for basic hardware access
  spec.add_dependency 'picoruby-gpio'
  spec.add_dependency 'picoruby-i2c'
  spec.add_dependency 'picoruby-spi'
end