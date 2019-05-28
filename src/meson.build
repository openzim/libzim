
configure_file(output : 'config.h',
               configuration : conf,
               input : 'config.h.in')

src_directory = include_directories('.')

common_sources = [
#    'config.h',
    'article.cpp',
    'cluster.cpp',
    'dirent.cpp',
    'envvalue.cpp',
    'file.cpp',
    'fileheader.cpp',
    'fileimpl.cpp',
    'file_compound.cpp',
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
    'writer/creator.cpp',
    'writer/lzmastream.cpp',
    'writer/article.cpp',
    'writer/cluster.cpp',
    'writer/dirent.cpp',
    'writer/xapianIndexer.cpp',
    'writer/tee.cpp'
]

if host_machine.system() == 'windows'
    common_sources += 'fs_windows.cpp'
else
    common_sources += 'fs_unix.cpp'
endif

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
                 link_args : extra_link_args,
                 cpp_args : extra_cpp_args,
                 version: meson.project_version(),
                 install : true)
libzim_dep = declare_dependency(link_with: libzim,
                                include_directories: include_directory)
