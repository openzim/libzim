
configure_file(input : 'pytest/data/wikibooks_be_all_nopic_2017-02.zim',
               output : 'wikibooks_be_all_nopic_2017-02.zim',
               copy: true )

tests = [
    'cluster',
    'dirent',
    'header',
    'uuid',
    'template',
    'iterator'
]

if zlib_dep.found()
#    zimlibtest_sources += ['zlibstream.cpp']
endif
if lzma_dep.found()
#    zimlibtest_sources += ['lzmastream.cpp']
endif

if gtest_dep.found() and not meson.is_cross_build()
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

if get_option('default_library') != 'static' and not meson.is_cross_build()
    subdir('pytest')
endif
