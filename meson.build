project('threshcorder',
  'cpp',
  version: '0.0.1',
  license: 'MIT',
  meson_version: '>=0.56.0',
  default_options: [
    'cpp_std=c++17',
    'buildtype=release',
    'b_ndebug=if-release',
    'warning_level=3',
    'b_lto=true',
    'pkg_config_path=' + meson.current_build_dir()]
)

if get_option('use_conan')
  conan = find_program('conan')

  if get_option('buildtype').startswith('release')
    conan_bt = 'build_type=Release'
  else
    conan_bt = 'build_type=Debug'
  endif

  r = run_command(conan,
    'install', meson.current_source_dir(),
    '-if', meson.current_build_dir(),
    '-s', conan_bt,
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

fmt = dependency('fmt', include_type: 'system')
alsa = dependency('alsa', include_type: 'system')
magic_enum = dependency('magic_enum', include_type: 'system')

executable('threshcorder',
  ['src/main.cpp', 'src/wav_file.cpp'],
  dependencies: [fmt, alsa, magic_enum],
  install: true
)
