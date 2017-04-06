
configure_file(output : 'config.h',
               configuration : conf,
               input : 'config.h.in')

common_sources = [
#    'config.h',
    'article.cpp',
    'articlesource.cpp',
    'cluster.cpp',
    'dirent.cpp',
    'envvalue.cpp',
    'file.cpp',
    'fileheader.cpp',
    'fileimpl.cpp',
    'fstream.cpp',
    'indexarticle.cpp',
    'md5.c',
    'md5stream.cpp',
    'ptrstream.cpp',
    'search.cpp',
    'tee.cpp',
    'template.cpp',
    'unicode.cpp',
    'uuid.cpp',
    'zimcreator.cpp',
    'zintstream.cpp'
]

zlib_sources = [
    'deflatestream.cpp',
    'inflatestream.cpp'
]

bzip2_sources = [
    'bunzip2stream.cpp',
    'bzip2.cpp',
    'bzip2stream.cpp'
]

lzma_sources = [
    'lzmastream.cpp',
    'unlzmastream.cpp'
]

sources = common_sources
deps = []

if zlib_dep.found()
    sources += zlib_sources
    deps += [zlib_dep]
endif

if bzip2_dep.found()
    sources += bzip2_sources
    deps += [bzip2_dep]
endif

if lzma_dep.found()
    sources += lzma_sources
    deps += [lzma_dep]
endif

libzim = library('zim',
                 sources,
                 include_directories : inc,
                 dependencies : deps,
                 install : true)

subdir('tools')
