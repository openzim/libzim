
cython = find_program('cython3', required : false)
py3_dep = dependency('python3', required : false)

if cython.found() and py3_dep.found()
  py3_mod = import('python3')
  py3 = py3_mod.find_python()

  has_pytest = (run_command(py3, ['-m', 'pytest', '--version']).returncode() == 0)

  if has_pytest
    subdir('wrapper')

    test('pytest',
      py3,
      args: ['-m', 'pytest'],
      env: ['PYTHONPATH=' + pydir],
      workdir: meson.current_source_dir()
    )
  endif
endif
