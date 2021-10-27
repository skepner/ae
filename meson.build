# -*- Meson -*-

project('ae', ['cpp'],
        license         : 'MIT',
        version         : '1',
        default_options : [
          'warning_level=3',
          'cpp_std=c++20',
          'buildtype=debugoptimized',
          'werror=false'
        ],
       )

if meson.get_compiler('cpp').get_id() == 'clang'
  add_project_arguments('-Weverything',
                        '-Wno-weak-vtables', '-Wno-padded', '-Wno-c++98-compat', '-Wno-c++98-c++11-compat', '-Wno-c++98-compat-pedantic',
                        language : 'cpp')
endif

# ----------------------------------------------------------------------

cmake = import('cmake')
lexy = cmake.subproject('lexy').dependency('lexy_core')

subproject('fmt')
fmt = dependency('fmt')

subproject('xlnt')

# simdjson = import('cmake').subproject('simdjson').dependency('simdjson')
subproject('simdjson')
simdjson = dependency('simdjson')

subproject('range_v3')
range_v3 = dependency('range_v3')

subproject('pybind11')
pybind11 = dependency('pybind11')

zlib = dependency('zlib', version : '>=1.2.8')
xz = dependency('liblzma')
bzip2 = meson.get_compiler('cpp').find_library('bz2', required : false)

# websocketpp
# asio
# apache module

# ----------------------------------------------------------------------

sources_sequences = ['cc/sequences/fasta.cc']
sources_utlis = ['cc/utils/file.cc', 'cc/utils/messages.cc']

# ----------------------------------------------------------------------

test_basic = executable('test-basic', 'cc/test/basic.cc',
                 include_directories : include_directories('./cc'),
                 dependencies : [fmt, dependency('xlnt'), simdjson, range_v3, lexy],
                 install : true)

test('ae test basic', test_basic)

test_virus_name = executable('test-virus-name', 'cc/test/virus-name.cc',
                             'cc/virus/name-parse.cc',
                             'cc/locdb/locdb.cc',
                             'cc/utils/file.cc', 'cc/utils/messages.cc',
                 include_directories : include_directories('./cc'),
                 dependencies : [fmt, simdjson, range_v3, bzip2, zlib, xz, lexy],
                 install : true)

test('ae test virus name parsing', test_virus_name)


# ----------------------------------------------------------------------
# python module
# https://github.com/mesonbuild/meson/issues/4677
# ----------------------------------------------------------------------

python3 = import('python').find_installation('python3')

ae_lib = python3.extension_module(
  'ae',
  sources : ['cc/py/module.cc', 'cc/py/sequences.cc'] + sources_sequences + sources_utlis,
  include_directories : include_directories('./cc'),
  dependencies : [dependency('python3'), pybind11, fmt, simdjson, range_v3, bzip2, zlib, xz, lexy],
  install : true)

# https://gabmus.org/posts/python-unittest-meson/
# envdata = environment()
# python_paths = [join_paths(meson.current_build_dir(), '..')]
# envdata.append('PYTHONPATH', python_paths)
# envdata.append('TESTS_BUILDDIR', meson.current_build_dir())
# test('pyb test 1', python3,
#      args : ['-V'],
#      # args: ['-m', 'unittest', 'tests'],
#      # env: envdata
#     )

# ----------------------------------------------------------------------
