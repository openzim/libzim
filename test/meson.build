

tests = [
    'cluster',
    'dirent',
    'header',
    'uuid',
    'zint',
    'template'
]


gtest_dep = dependency('gtest', fallback: ['gtest', 'gtest_dep'])

foreach test_name : tests
    test_exe = executable(test_name, [test_name+'.cpp'],
                          include_directories : include_directory,
                          link_with : libzim,
                          dependencies : deps + [gtest_dep])
    test(test_name, test_exe)
endforeach

