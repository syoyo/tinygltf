project('mesh-conv', 'cpp', default_options : ['cpp_std=c++11'])

thread_dep = dependency('threads')

incdir = include_directories(['../../', '../common'])

executable('mesh-conv', ['mesh-conv.cc', 'mesh-util.cc', 'tinygltf_impl.cc'], include_directories : incdir, dependencies : thread_dep)
