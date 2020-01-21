project('fuzz_tinygltf', 'cpp', default_options : ['cpp_std=c++11'])

incdirs = include_directories('../../')
executable('fuzz_gltf',
  'fuzz_gltf.cc',
  include_directories : incdirs,
  cpp_args : '-fsanitize=address,fuzzer',
  link_args : '-fsanitize=address,fuzzer' )

