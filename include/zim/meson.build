zim_config = configure_file(output : 'zim_config.h',
                            configuration : public_conf)

install_headers(
    'archive.h',
    'blob.h',
    'error.h',
    'item.h',
    'entry.h',
    'uuid.h',
    'zim.h',
    zim_config,
    subdir:'zim'
)

if xapian_dep.found()
  install_headers(
    'search.h',
    'search_iterator.h',
    subdir:'zim'
  )
endif

install_headers(
    'writer/item.h',
    'writer/creator.h',
    'writer/contentProvider.h',
    subdir:'zim/writer'
)

