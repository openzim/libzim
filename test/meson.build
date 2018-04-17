

tests = [
    'cluster',
    'dirent',
    'header',
    'uuid',
    'template'
]


gtest_dep = dependency('gtest', fallback: ['gtest', 'gtest_dep'])

foreach test_name : tests
    test_exe = executable(test_name, [test_name+'.cpp'],
                          include_directories : include_directory,
                          link_with : libzim,
                          dependencies : deps + [gtest_dep],
                          build_rpath : '$ORIGIN')
    test(test_name, test_exe)
endforeach

if get_option('default_library') != 'static'
    subdir('pytest')
endif
