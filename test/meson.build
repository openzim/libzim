

tests = [
    'cluster',
    'dirent',
    'header',
    'uuid',
    'template'
]

if zlib_dep.found()
#    zimlibtest_sources += ['zlibstream.cpp']
endif
if lzma_dep.found()
#    zimlibtest_sources += ['lzmastream.cpp']
endif

if gtest_dep.found()
    foreach test_name : tests
        test_exe = executable(test_name, [test_name+'.cpp'],
                              include_directories : [include_directory, src_directory],
                              link_with : libzim,
                              link_args: extra_link_args,
                              dependencies : deps + [gtest_dep],
                              build_rpath : '$ORIGIN')
        test(test_name, test_exe, timeout : 60)
    endforeach
endif

if get_option('default_library') != 'static'
    subdir('pytest')
endif
