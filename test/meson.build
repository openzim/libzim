

tests = [
    'cluster',
    'dirent',
    'header',
    'uuid',
    'zint',
    'template'
]

if zlib_dep.found()
#    zimlibtest_sources += ['zlibstream.cpp']
endif
if lzma_dep.found()
#    zimlibtest_sources += ['lzmastream.cpp']
endif

gtest_dep = gtest_sp.get_variable('gtest_dep')

foreach test_name : tests
    test_exe = executable(test_name, [test_name+'.cpp'],
                          include_directories : include_directory,
                          link_with : libzim,
                          dependencies : deps + [gtest_dep])
    test(test_name, test_exe)
endforeach

