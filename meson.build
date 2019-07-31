project('libzim', ['c', 'cpp'],
  version : '5.0.1',
  license : 'GPL2',
  default_options : ['c_std=c11', 'cpp_std=c++11', 'werror=true'])

if build_machine.system() != 'windows'
  add_project_arguments('-D_LARGEFILE64_SOURCE=1', '-D_FILE_OFFSET_BITS=64', language: 'cpp')
endif

sizeof_off_t = meson.get_compiler('cpp').sizeof('off_t')

conf = configuration_data()
conf.set('VERSION', '"@0@"'.format(meson.project_version()))
conf.set('DIRENT_CACHE_SIZE', get_option('DIRENT_CACHE_SIZE'))
conf.set('CLUSTER_CACHE_SIZE', get_option('CLUSTER_CACHE_SIZE'))
conf.set('LZMA_MEMORY_SIZE', get_option('LZMA_MEMORY_SIZE'))
conf.set10('MMAP_SUPPORT_64', sizeof_off_t==8)
if target_machine.system() == 'windows'
    conf.set('ENABLE_USE_MMAP', false)
else
    conf.set('ENABLE_USE_MMAP', get_option('USE_MMAP'))
endif
conf.set('ENABLE_USE_BUFFER_HEADER', get_option('USE_BUFFER_HEADER'))

static_linkage = get_option('static-linkage')
static_linkage = static_linkage or get_option('default_library')=='static'

zlib_dep = dependency('zlib', required:false, static:static_linkage)
conf.set('ENABLE_ZLIB', zlib_dep.found())

lzma_dep = dependency('liblzma', static:static_linkage)

xapian_dep = dependency('xapian-core',
                        required:false,
                        static:static_linkage)
conf.set('ENABLE_XAPIAN', xapian_dep.found())

pkg_requires = ['liblzma']
if build_machine.system() == 'windows'
    thread_dep = dependency('libpthreadVC2')
    pkg_requires += ['libpthreadVC2']
    extra_link_args = ['-lRpcrt4', '-lWs2_32', '-lwinmm', '-licuuc', '-licuin']
    extra_cpp_args = ['-DSORTPP_PASS']
else
    thread_dep = dependency('threads')
    extra_link_args = []
    extra_cpp_args = []
endif
if zlib_dep.found()
    pkg_requires += ['zlib']
endif
if xapian_dep.found()
    pkg_requires += ['xapian-core']
    icu_dep = dependency('icu-i18n', static:static_linkage)
    pkg_requires += ['icu-i18n']
else
    icu_dep = dependency('icu-i18n', required:false, static:static_linkage)
endif

gtest_dep = dependency('gtest', main:true, fallback:['gtest', 'gtest_dep'], required:false)

inc = include_directories('include')

subdir('include')
subdir('scripts')
subdir('static')
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
