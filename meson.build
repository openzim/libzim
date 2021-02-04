project('libzim', ['c', 'cpp'],
  version : '7.0.0',
  license : 'GPL2',
  default_options : ['c_std=c11', 'cpp_std=c++11'])

if build_machine.system() != 'windows'
  add_project_arguments('-D_LARGEFILE64_SOURCE=1', '-D_FILE_OFFSET_BITS=64', language: 'cpp')
endif

cpp = meson.get_compiler('cpp')
sizeof_off_t = cpp.sizeof('off_t')

private_conf = configuration_data()
public_conf = configuration_data()

private_conf.set('VERSION', '"@0@"'.format(meson.project_version()))
public_conf.set('LIBZIM_VERSION', '"@0@"'.format(meson.project_version()))
private_conf.set('DIRENT_CACHE_SIZE', get_option('DIRENT_CACHE_SIZE'))
private_conf.set('DIRENT_LOOKUP_CACHE_SIZE', get_option('DIRENT_LOOKUP_CACHE_SIZE'))
private_conf.set('CLUSTER_CACHE_SIZE', get_option('CLUSTER_CACHE_SIZE'))
private_conf.set('LZMA_MEMORY_SIZE', get_option('LZMA_MEMORY_SIZE'))
private_conf.set10('MMAP_SUPPORT_64', sizeof_off_t==8)
if target_machine.system() == 'windows'
    private_conf.set('ENABLE_USE_MMAP', false)
else
    private_conf.set('ENABLE_USE_MMAP', get_option('USE_MMAP'))
endif
private_conf.set('ENABLE_USE_BUFFER_HEADER', get_option('USE_BUFFER_HEADER'))

static_linkage = get_option('static-linkage')
static_linkage = static_linkage or get_option('default_library')=='static'

lzma_dep = dependency('liblzma', static:static_linkage)
if static_linkage
  add_project_arguments('-DLZMA_API_STATIC', language: 'cpp')
endif

zstd_dep = dependency('libzstd', static:static_linkage)

if target_machine.system() == 'freebsd'
    execinfo_dep = cpp.find_library('execinfo')
endif

if get_option('with_xapian')
    xapian_dep = dependency('xapian-core', static:static_linkage)
else
    xapian_dep = dependency('', required:false)
endif
private_conf.set('ENABLE_XAPIAN', xapian_dep.found())
public_conf.set('LIBZIM_WITH_XAPIAN', xapian_dep.found())

pkg_requires = ['liblzma', 'libzstd']
if build_machine.system() == 'windows'
    extra_link_args = ['-lRpcrt4', '-lWs2_32', '-lwinmm', '-licuuc', '-licuin']
    extra_cpp_args = ['-DSORTPP_PASS']
else
    extra_link_args = []
    extra_cpp_args = []
endif

compiler = meson.get_compiler('cpp')
if (compiler.get_id() == 'gcc' and build_machine.system() == 'linux') or target_machine.system() == 'freebsd'
  # C++ std::thread is implemented using pthread on linux by gcc
  thread_dep = dependency('threads')
else
  thread_dep = dependency('', required:false)
endif

if xapian_dep.found()
    pkg_requires += ['xapian-core']
    icu_dep = dependency('icu-i18n', static:static_linkage)
    pkg_requires += ['icu-i18n']
else
    icu_dep = dependency('icu-i18n', required:false, static:static_linkage)
endif

gtest_dep = dependency('gtest', main:true, fallback:['gtest', 'gtest_main_dep'], required:false)

inc = include_directories('include')

subdir('include')
subdir('scripts')
subdir('static')
subdir('src')
subdir('examples')
subdir('test')
if get_option('doc')
  subdir('docs')
endif

pkg_mod = import('pkgconfig')
pkg_mod.generate(libraries : libzim,
                 version : meson.project_version(),
                 name : 'libzim',
                 filebase : 'libzim',
                 description : 'A Library to zim.',
                 requires : pkg_requires)
