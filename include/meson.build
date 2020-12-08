include_directory = include_directories('.')

install_headers(
    'zim/archive.h',
    'zim/blob.h',
    'zim/error.h',
    'zim/item.h',
    'zim/entry.h',
    'zim/uuid.h',
    'zim/zim.h',
    subdir:'zim'
)

if xapian_dep.found()
  install_headers(
    'zim/search.h',
    'zim/search_iterator.h',
    subdir:'zim'
  )
endif

install_headers(
    'zim/writer/item.h',
    'zim/writer/creator.h',
    'zim/writer/contentProvider.h',
    subdir:'zim/writer'
)

