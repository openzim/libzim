project('libzim', ['c', 'cpp'],
  version : '2.0.0',
  license : 'GPL2',
  default_options : ['c_std=c11', 'cpp_std=c++11'])

conf = configuration_data()
conf.set('VERSION', '"@0@"'.format(meson.project_version()))
conf.set('DIRENT_CACHE_SIZE', get_option('DIRENT_CACHE_SIZE'))
conf.set('CLUSTER_CACHE_SIZE', get_option('CLUSTER_CACHE_SIZE'))
conf.set('LZMA_MEMORY_SIZE', get_option('LZMA_MEMORY_SIZE'))

add_global_arguments(['-Werror', '-Wall'], language:'cpp')

zlib_dep = dependency('zlib', required:false)
conf.set('ENABLE_ZLIB', zlib_dep.found())
lzma_dep = dependency('liblzma', required:false)
conf.set('ENABLE_LZMA', lzma_dep.found())

xapian_dep = dependency('xapian-core',
                        required:false,
                        static:(get_option('default_library')=='static'))
conf.set('ENABLE_XAPIAN', xapian_dep.found())

pkg_requires = []
if zlib_dep.found()
    pkg_requires += ['zlib']
endif
if lzma_dep.found()
    pkg_requires += ['liblzma']
endif
if xapian_dep.found()
    pkg_requires += ['xapian-core']
    icu_dep = dependency('icu-i18n')
    pkg_requires += ['icu-i18n']
else
    icu_dep = dependency('icu-i18n', required:false)
endif

compiler = meson.get_compiler('cpp')
rt_dep = compiler.find_library('rt', required:false)

gtest_sp = subproject('gtest')

inc = include_directories('include')

subdir('include')
subdir('src')
subdir('examples')
subdir('test')

pkg_mod = import('pkgconfig')
pkg_mod.generate(libraries : libzim,
                 version : meson.project_version(),
                 name : 'libzim',
                 filebase : 'libzim',
                 description : 'A Library to zim.',
                 requires : pkg_requires)
