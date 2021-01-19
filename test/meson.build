subdir('data')

tests = [
    'lrucache',
    'cluster',
    'dirent',
    'header',
    'uuid',
    'template',
    'archive',
    'iterator',
    'find',
    'compression',
    'dirent_lookup',
    'istreamreader',
    'decoderstreamreader',
    'rawstreamreader',
    'bufferstreamer',
    'parseLongPath',
    'search'
]

if gtest_dep.found() and not meson.is_cross_build()
    foreach test_name : tests
        test_exe = executable(test_name, [test_name+'.cpp', 'tools.cpp'],
                              implicit_include_directories: false,
                              include_directories : [include_directory, src_directory],
                              link_with : libzim,
                              link_args: extra_link_args,
                              dependencies : deps + [gtest_dep],
                              build_rpath : '$ORIGIN')
        test(test_name, test_exe, timeout : 120,
             workdir: meson.current_build_dir())
    endforeach
endif
