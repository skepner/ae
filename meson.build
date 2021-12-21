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
                        '-Wno-ambiguous-reversed-operator', # ISO C++20 considers use of overloaded operator '<=' to be ambiguous despite there being a unique best viable function
                        language : 'cpp')
endif

# ----------------------------------------------------------------------
# external dependencies
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

subproject('xxhash')
xxhash = dependency('libxxhash')

zlib = dependency('zlib', version : '>=1.2.8')
xz = dependency('liblzma')
bzip2 = meson.get_compiler('cpp').find_library('bz2', required : false)

alglib = dependency('alglib')

# websocketpp
# asio
# apache module

# ----------------------------------------------------------------------
# ae
# ----------------------------------------------------------------------

sources_utlis = ['cc/utils/file.cc', 'cc/utils/log.cc', 'cc/utils/messages.cc', 'cc/ext/date.cc']

sources_locdb_v3 = ['cc/locdb/v3/locdb.cc']

sources_virus = ['cc/virus/name-parse.cc', 'cc/virus/passage-parse.cc']
sources_sequences = ['cc/sequences/seqdb.cc', 'cc/sequences/seqdb-selected.cc', 'cc/sequences/seqdb-export.cc', 'cc/sequences/fasta.cc', 'cc/sequences/clades.cc',
                     'cc/sequences/translate.cc', 'cc/sequences/align.cc', 'cc/sequences/master.cc', 'cc/sequences/deletions.cc']
sources_tree = ['cc/tree/tree.cc', 'cc/tree/tree-iterator.cc', 'cc/tree/newick.cc', 'cc/tree/export.cc']
sources_py = ['cc/py/module.cc', 'cc/py/virus.cc', 'cc/py/sequences.cc', 'cc/py/tree.cc', 'cc/py/utils.cc']

# ----------------------------------------------------------------------
# rjson
# ----------------------------------------------------------------------

sources_ad = [
  'cc/ad/rjson-v2.cc',
  'cc/ad/rjson-v3.cc',
  'cc/ad/rjson-v3-helper.cc',
  'cc/ad/color.cc',
  'cc/ad/color-modifier.cc',
  'cc/ad/color-gradient.cc',
  'cc/ad/color-hsv.cc',
]

# ----------------------------------------------------------------------
# chart 
# ----------------------------------------------------------------------

sources_chart_v2 = [
  'cc/chart/v2/alglib.cc',
  'cc/chart/v2/chart.cc',
  'cc/chart/v2/chart-modify.cc',
  'cc/chart/v2/column-bases.cc',
  'cc/chart/v2/layout.cc',
  'cc/chart/v2/optimize.cc',
  'cc/chart/v2/serum-circle.cc',
  'cc/chart/v2/stress.cc',
  'cc/chart/v2/titers.cc',
]

# ----------------------------------------------------------------------
# tests ae
# ----------------------------------------------------------------------

test_basic = executable('test-basic', 'cc/test/basic.cc',
                 include_directories : include_directories('./cc'),
                 dependencies : [fmt, dependency('xlnt'), simdjson, range_v3, lexy],
                 install : true)

test('ae test basic', test_basic)

test_virus_name = executable(
  'test-virus-name',
  ['cc/test/test-virus-name.cc'] + sources_virus + sources_locdb_v3 + sources_utlis,
  include_directories : include_directories('./cc'),
  dependencies : [fmt, simdjson, range_v3, bzip2, zlib, xz, lexy],
  install : true)

test_passage = executable(
  'test-passage',
  ['cc/test/test-passage.cc', 'cc/virus/passage-parse.cc', 'cc/utils/messages.cc'],
  include_directories : include_directories('./cc'),
  dependencies : [fmt, range_v3, lexy],
  install : true)

test_date = executable(
  'test-date',
  ['cc/test/test-date.cc', 'cc/ext/date.cc'],
  include_directories : include_directories('./cc'),
  dependencies : [fmt],
  install : true)

test_pos = executable(
  'test-pos',
  ['cc/test/test-pos.cc'],
  include_directories : include_directories('./cc'),
  dependencies : [fmt],
  install : true)

test('ae test virus name parsing', test_virus_name)
test('ae test date parsing', test_date)
test('ae test passage parsing', test_passage)

# ----------------------------------------------------------------------
# python module
# https://github.com/mesonbuild/meson/issues/4677
# ----------------------------------------------------------------------

python3 = import('python').find_installation('python3')

ae_lib = python3.extension_module(
  'ae_backend',
  sources : sources_py + sources_virus + sources_sequences + sources_tree + sources_utlis +
            sources_locdb_v3 + sources_chart_v2 + sources_ad,
  include_directories : include_directories('./cc'),
  dependencies : [dependency('python3'), pybind11, fmt, simdjson, range_v3, bzip2, zlib, xz, lexy, xxhash, alglib],
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
