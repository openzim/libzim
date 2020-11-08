data_files = [
  'small.zim',
  'invalid.smaller_than_header.zim',
  'invalid.outofbounds_urlptrpos.zim',
  'invalid.outofbounds_titleptrpos.zim',
  'invalid.outofbounds_clusterptrpos.zim',
  'wikibooks_be_all_nopic_2017-02.zim',
  'wikibooks_be_all_nopic_2017-02_splitted.zimaa',
  'wikibooks_be_all_nopic_2017-02_splitted.zimab',
  'wikibooks_be_all_nopic_2017-02_splitted.zimac',
  'wikipedia_en_climate_change_nopic_2020-01.zim'
]

foreach file : data_files
  # configure_file(input : file, output : file, copy: true)
  #
  # Above (commented) command doesn't work with Meson versions below 0.47
  # (in which the 'copy' keyword was first introduced). We want to keep
  # compatibility with Ubuntu 18.04 Bionic (which has Meson version 0.45)
  # until its EOL.
  #
  # Below is a python based workaround.
  configure_file(input : file,
                 output : file,
                 command: [
                   find_program('python3'),
                    '-c',
                    'import sys; import shutil; shutil.copy(sys.argv[1], sys.argv[2])',
                    '@INPUT@',
                    '@OUTPUT@'
                 ])
endforeach
