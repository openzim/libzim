
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
    'file_compound.cpp',
    'file_part.cpp',
    'file_reader.cpp',
    'blob.cpp',
    'buffer.cpp',
    'md5.c',
    'md5stream.cpp',
    'search.cpp',
    'search_iterator.cpp',
    'template.cpp',
    'uuid.cpp',
    'levenshtein.cpp',
    'tools.cpp',
    'writer/zimcreatorimpl.cpp',
    'writer/lzmastream.cpp',
    'writer/cluster.cpp',
    'writer/xapianIndexer.cpp',
    'writer/tee.cpp'
]

zlib_sources = [
    'writer/deflatestream.cpp'
]

xapian_sources = [
    'xapian/htmlparse.cc',
    'xapian/myhtmlparse.cc'
]

sources = common_sources
deps = [thread_dep, lzma_dep]

if zlib_dep.found()
    sources += zlib_sources
    deps += [zlib_dep]
endif

if xapian_dep.found()
    sources += xapian_sources
    sources += lib_resources
    deps += [xapian_dep, icu_dep]
endif

libzim = library('zim',
                 sources,
                 include_directories : inc,
                 dependencies : deps,
                 version: meson.project_version(),
                 install : true,
                 build_rpath : join_paths(get_option('prefix'), get_option('libdir')),
                 install_rpath: '$ORIGIN')
libzim_dep = declare_dependency(link_with: libzim,
                                include_directories: include_directory)
