project('fuzz_tinygltf', 'cpp', default_options : ['cpp_std=c++11'])

incdirs = include_directories('../../')
executable('fuzz_gltf',
  'fuzz_gltf.cc',
  include_directories : incdirs,
  cpp_args : '-fsanitize=address,fuzzer',
  link_args : '-fsanitize=address,fuzzer' )

executable('fuzz_gltf_customjson',
  'fuzz_gltf_customjson.cc',
  include_directories : incdirs,
  cpp_args : ['-fsanitize=address,fuzzer', '-DTINYGLTF_USE_CUSTOM_JSON'],
  link_args : '-fsanitize=address,fuzzer' )

