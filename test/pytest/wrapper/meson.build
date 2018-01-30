
pyx_cpp = custom_target('libzim_pyx',
  output : 'libzim_pyx.cpp',
  input : 'libzim_ext.pyx',
  command : [cython, '--gdb', '--cplus', '@INPUT@', '-o', '@OUTPUT@'],
)

slib = py3_mod.extension_module('libzim_ext', pyx_cpp,
  dependencies : py3_dep,
  link_with: libzim)

pydir = meson.current_build_dir()

