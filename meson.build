project('threshcorder',
  'cpp',
  version: '0.0.1',
  license: 'MIT',
  meson_version: '>=0.55.0',
  default_options: [
    'cpp_std=c++17',
    'buildtype=release',
    'b_ndebug=if-release',
    'warning_level=3',
    'b_lto=true']
)


if get_option('use_conan')
  conan = find_program('conan')

  r = run_command(conan,
    'install', '.',
    '-if', meson.current_build_dir(),
    '--build=missing')

  if r.returncode() != 0
    error('Failed to install dependencies with conan')
  endif
endif

add_project_arguments(
  '-Wshadow',
  '-Wconversion',
  '-Wpedantic',
  '-Wlogical-op',
  '-Wduplicated-cond',
  '-Wduplicated-branches',
  '-Wformat=2',
  '-Wold-style-cast',
  '-Wuseless-cast',
  '-Wnull-dereference',
  language: 'cpp'
)

executable('threshcorder',
  ['src/main.cpp', 'src/wav_file.cpp'],
  dependencies: [
    dependency('fmt'),
    dependency('alsa')
  ],
  install: true
)
