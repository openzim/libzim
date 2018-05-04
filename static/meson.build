
resources_list = 'resources_list.txt'

lib_resources = custom_target('resources',
  input: resources_list,
  output: ['libzim-resources.cpp', 'libzim-resources.h'],
  command:[res_compiler,
           '--cxxfile', '@OUTPUT0@',
           '--hfile', '@OUTPUT1@',
           '--source_dir', '@OUTDIR@',
           '@INPUT@']
)
