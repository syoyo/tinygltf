project(
  'tinygltf',
  'c',
  default_options: ['c_std=c11'],
  meson_version: '>=0.55.0',
)

tinygltf_inc = include_directories('.', 'tests')

if get_option('tests')
  tests_workdir = join_paths(meson.current_source_dir(), 'tests')
  cc = meson.get_compiler('c')

  tester_v3_c = executable(
    'tester_v3_c',
    ['tests/tester_v3_c.c', 'tiny_gltf_v3.c'],
    include_directories: tinygltf_inc,
    c_args: ['-DTINYGLTF3_ENABLE_FS'],
    install: false,
  )
  test('tester_v3_c', tester_v3_c, workdir: tests_workdir)

  tester_v3_c_v1port = executable(
    'tester_v3_c_v1port',
    ['tests/tester_v3_c_v1port.c', 'tiny_gltf_v3.c'],
    include_directories: tinygltf_inc,
    c_args: ['-DTINYGLTF3_ENABLE_FS'],
    install: false,
  )
  test('tester_v3_c_v1port', tester_v3_c_v1port, workdir: tests_workdir)

  tester_v3_json_c = executable(
    'tester_v3_json_c',
    'tests/tester_v3_json_c.c',
    include_directories: tinygltf_inc,
    install: false,
  )
  test('tester_v3_json_c', tester_v3_json_c, workdir: tests_workdir)

  freestanding_args = []
  if cc.get_id() in ['clang', 'gcc']
    freestanding_args += ['-ffreestanding']
  endif
  tester_v3_freestanding = executable(
    'tester_v3_freestanding',
    'tests/tester_v3_freestanding.c',
    include_directories: tinygltf_inc,
    c_args: freestanding_args,
    install: false,
  )
  test('tester_v3_freestanding', tester_v3_freestanding, workdir: tests_workdir)
endif
