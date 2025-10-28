tests = [
    'log',
    'lrucache',
    'concurrentcache',
    'cluster',
    'creator',
    'dirent',
    'error_in_creator',
    'header',
    'uuid',
    'archive',
    'iterator',
    'reader',
    'find',
    'compression',
    'dirent_lookup',
    'istreamreader',
    'decoderstreamreader',
    'rawstreamreader',
    'bufferstreamer',
    'parseLongPath',
    'random',
    'tooltesting',
    'tinyString',
    'suggestion_iterator',
    'indexing_criteria',
    'counterParsing',
    'illustrations'
]

if xapian_dep.found()
    tests += ['search', 'defaultIndexdata', 'search_iterator', 'suggestion']
endif

datadir = get_option('test_data_dir')
if datadir == 'none'
    test_cpp_args = ['-DWITH_TEST_DATA=0']
else
    test_cpp_args = ['-DWITH_TEST_DATA=1']
    if datadir == ''
        # We need to download the test data.
        datadir = join_paths(meson.current_build_dir(), 'data')
    endif
    run_target('download_test_data', command : [test_data_downloader, '--remove-top-dir', datadir])
endif

testenv = environment()
testenv.set('ZIM_TEST_DATA_DIR', datadir)

if cpp.get_id() == 'gcc' and cpp.version().version_compare('>=12.0.0') and cpp.version().version_compare('<13.0.0')
    test_cpp_args += ['-Wno-error=restrict']
endif

if gtest_dep.found() and not meson.is_cross_build()
    foreach test_name : tests
        test_exe = executable(test_name, [test_name+'.cpp', 'tools.cpp'],
                              implicit_include_directories: false,
                              include_directories: [include_directory, src_directory],
                              link_with: libzim,
                              cpp_args: test_cpp_args,
                              dependencies: all_deps + [gtest_dep],
                              build_rpath: '$ORIGIN')
        test(test_name, test_exe, timeout : 120, env: testenv)
    endforeach
endif
