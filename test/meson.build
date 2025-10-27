writer_dependant_tests = [
    'cluster',
    'dirent',
    'creator',
    'error_in_creator',
    'indexing_criteria',
    'tinyString',
    'suggestion_iterator',
    'archive',
    'header',
    'reader',
    'iterator',
    'find'
]
xapian_writer_dependant_tests = [
    'search', 
    'defaultIndexdata', 
    'suggestion', 
    'search_iterator'
]

all_tests = [
    'log',
    'lrucache',
    'concurrentcache',
    'uuid',
    'compression',
    'dirent_lookup',
    'istreamreader',
    'decoderstreamreader',
    'rawstreamreader',
    'bufferstreamer',
    'parseLongPath',
    'random',
    'tooltesting',
    'counterParsing',
    'illustrations'
]

if not get_option('without_writer')
    if xapian_dep.found()
        writer_dependant_tests += xapian_writer_dependant_tests
    endif
    all_tests += writer_dependant_tests
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
    foreach test_name : all_tests
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
