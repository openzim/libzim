project('libzim', ['c', 'cpp'],
  version : '1.4',
  license : 'GPL2')

abi_current=2
abi_revision=0
abi_age=0
  
conf = configuration_data()
conf.set('VERSION', '"@0@"'.format(meson.project_version()))
conf.set('DIRENT_CACHE_SIZE', get_option('DIRENT_CACHE_SIZE'))
conf.set('CLUSTER_CACHE_SIZE', get_option('CLUSTER_CACHE_SIZE'))
conf.set('LZMA_MEMORY_SIZE', get_option('LZMA_MEMORY_SIZE'))

zlib_dep = dependency('zlib', required:false)
conf.set('ENABLE_ZLIB', zlib_dep.found())
lzma_dep = dependency('liblzma', required:false)
conf.set('ENABLE_LZMA', lzma_dep.found())
bzip2_dep = dependency('bzip2', required:false)
conf.set('ENABLE_BZIP2', bzip2_dep.found())

pkg_requires = []
if zlib_dep.found()
    pkg_requires += ['zlib']
endif
if lzma_dep.found()
    pkg_requires += ['liblzma']
endif
if bzip2_dep.found()
    pkg_requires += ['bzip2']
endif

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
