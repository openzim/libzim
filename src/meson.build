
configure_file(output : 'config.h',
               configuration : private_conf,
               input : 'config.h.in')

src_directory = include_directories('.')

common_sources = [
#    'config.h',
    'archive.cpp',
    'cluster.cpp',
    'buffer_reader.cpp',
    'dirent.cpp',
    'dirent_accessor.cpp',
    'entry.cpp',
    'fileheader.cpp',
    'fileimpl.cpp',
    'file_compound.cpp',
    'file_reader.cpp',
    'item.cpp',
    'blob.cpp',
    'buffer.cpp',
    'md5.c',
    'uuid.cpp',
    'tools.cpp',
    'compression.cpp',
    'istreamreader.cpp',
    'namedthread.cpp',
    'log.cpp',
    'writer/contentProvider.cpp',
    'writer/creator.cpp',
    'writer/item.cpp',
    'writer/cluster.cpp',
    'writer/dirent.cpp',
    'writer/workers.cpp',
    'writer/clusterWorker.cpp',
    'writer/titleListingHandler.cpp',
    'writer/counterHandler.cpp',
    'suggestion.cpp',
    'suggestion_iterator.cpp',
    'version.cpp'
]

if host_machine.system() == 'windows'
    common_sources += 'fs_windows.cpp'
else
    common_sources += 'fs_unix.cpp'
endif

xapian_sources = [
    'search.cpp',
    'search_iterator.cpp',
    'xapian/htmlparse.cc',
    'xapian/myhtmlparse.cc',
    'writer/xapianIndexer.cpp',
    'writer/xapianWorker.cpp',
    'writer/xapianHandler.cpp'
]

sources = common_sources

if target_machine.system() == 'freebsd'
    deps += [execinfo_dep]
endif

if xapian_dep.found()
    sources += xapian_sources
    sources += lib_resources
endif

libzim = library('zim',
                 sources,
                 include_directories : inc,
                 dependencies : all_deps,
                 version: meson.project_version(),
                 install : true)
libzim_dep = declare_dependency(link_with: libzim,
                                include_directories: include_directory)
