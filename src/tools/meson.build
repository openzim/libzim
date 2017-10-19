
src_directory = include_directories('..')
include_dirs = [src_directory, include_directory]

executable('zimdump', 'zimDump.cpp',
           link_with: libzim,
           dependencies : deps,
           include_directories: include_dirs,
           install: true)
