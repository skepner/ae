# -*- Meson -*-

project('ae', ['cpp'],
        license         : 'MIT',
        version         : '1',
        default_options : [
          'warning_level=3',
          'cpp_std=c++20', # g++-11 does not support c++20
          'buildtype=debugoptimized',
          'werror=false'
        ],
       )

if meson.get_compiler('cpp').get_id() == 'clang'
  add_project_arguments(
    '-Weverything',
    '-Wno-weak-vtables', '-Wno-padded', '-Wno-c++98-compat', '-Wno-c++98-c++11-compat', '-Wno-c++98-compat-pedantic',
    '-Wno-ambiguous-reversed-operator', # ISO C++20 considers use of overloaded operator '<=' to be ambiguous despite there being a unique best viable function
    '-Wno-zero-as-null-pointer-constant', # clang13 somtimes with operator<=>
    language : 'cpp')
else
  # add_project_arguments(
  #   '-Wno-unknown-pragmas',     # xlnt: ignoring ‘#pragma clang diagnostic’
  #   '-Wno-volatile',            # alglib: ‘++’ expression of ‘volatile’-qualified type is deprecated
  #   language : 'cpp')
endif

# ----------------------------------------------------------------------
# external dependencies
# ----------------------------------------------------------------------

cmake = import('cmake')
lexy = cmake.subproject('lexy').dependency('lexy_core')

subproject('xlnt')
xlnt = dependency('xlnt')

# simdjson = import('cmake').subproject('simdjson').dependency('simdjson')
subproject('simdjson')
simdjson = dependency('simdjson')

subproject('range_v3')
range_v3 = dependency('range_v3')

subproject('pybind11')
pybind11 = dependency('pybind11')

subproject('xxhash')
xxhash = dependency('libxxhash')

subproject('fmt')
fmt = dependency('fmt')

brotli_enc = dependency('libbrotlienc')
brotli_dec = dependency('libbrotlidec')
zlib = dependency('zlib', version : '>=1.2.8')
xz = dependency('liblzma')
bzip2 = meson.get_compiler('cpp').find_library('bz2', required : false)

alglib = dependency('alglib')

omp = dependency('openmp')
if host_machine.system() == 'darwin'
  libomp_dir = ['-L' + run_command('brew', '--prefix', check: true).stdout().strip() + '/lib']
else
  libomp_dir = []
endif

# mylib = meson.get_compiler('cpp').find_library('omp', dirs: '/opt/homebrew/lib')
message(host_machine.system())

# websocketpp
# asio
# apache module

include_cc = include_directories('./cc')

# ----------------------------------------------------------------------
# ae
# ----------------------------------------------------------------------

sources_py = [
  'cc/py/module.cc',
  'cc/py/virus.cc',
  'cc/py/sequences.cc',
  'cc/py/tree.cc',
  'cc/py/whocc.cc',
  'cc/py/locdb-v3.cc',
  'cc/py/chart-v3.cc',
  'cc/py/chart-v3-antigens.cc',
  'cc/py/chart-v3-plot-spec.cc',
  'cc/py/chart-v3-tests.cc',
  'cc/py/chart-v2.cc',
  'cc/py/utils.cc',
]

sources_tree = [
  'cc/tree/tree.cc',
  'cc/tree/tree-iterator.cc',
  'cc/tree/newick.cc',
  'cc/tree/export.cc',
  'cc/tree/raxml-ancestral-state.cc',
  'cc/tree/aa-transitions.cc'
]

sources_sequences = [
  'cc/sequences/seqdb.cc',
  'cc/sequences/seqdb-selected.cc',
  'cc/sequences/seqdb-export.cc',
  'cc/sequences/fasta.cc',
  'cc/sequences/clades.cc',
  'cc/sequences/translate.cc',
  'cc/sequences/align.cc',
  'cc/sequences/master.cc',
  'cc/sequences/deletions.cc'
]

sources_locdb_v3 = [
  'cc/locdb/v3/locdb.cc'
]

sources_whocc = [
  'cc/whocc/xlsx/csv-parser.cc',
  'cc/whocc/xlsx/sheet-extractor.cc',
  'cc/whocc/xlsx/sheet.cc',
]

sources_virus = [
  'cc/virus/name-parse.cc',
  'cc/virus/passage.cc',
  'cc/virus/reassortant.cc'
]

sources_utils = [
  'cc/utils/file.cc',
  'cc/utils/log.cc',
  'cc/utils/messages.cc',
  'cc/ext/date.cc',
  'cc/utils/collection-json.cc',
]

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

sources_chart_v3 = [
  'cc/chart/v3/chart.cc',
  'cc/chart/v3/info.cc',
  'cc/chart/v3/antigens.cc',
  'cc/chart/v3/titers.cc',
  'cc/chart/v3/layout.cc',
  'cc/chart/v3/projections.cc',
  'cc/chart/v3/chart-import.cc',
  'cc/chart/v3/chart-export.cc',
  'cc/chart/v3/stress.cc',
  'cc/chart/v3/table-distances.cc',
  'cc/chart/v3/randomizer.cc',
  'cc/chart/v3/optimize.cc',
  'cc/chart/v3/alglib.cc',
  'cc/chart/v3/common.cc',
  'cc/chart/v3/merge.cc',
  'cc/chart/v3/procrustes.cc',
  'cc/chart/v3/grid-test.cc',
  'cc/chart/v3/avidity-test.cc',
  'cc/chart/v3/serum-circles.cc',
  'cc/chart/v3/chart-seqdb.cc',
  'cc/chart/v3/styles.cc',
]

sources_chart_v2 = [
  'cc/chart/v2/acd1-import.cc',
  'cc/chart/v2/ace-export.cc',
  'cc/chart/v2/ace-import.cc',
  'cc/chart/v2/alglib.cc',
  'cc/chart/v2/blobs.cc',
  'cc/chart/v2/chart.cc',
  'cc/chart/v2/chart-modify.cc',
  'cc/chart/v2/column-bases.cc',
  'cc/chart/v2/common.cc',
  'cc/chart/v2/factory-import.cc',
  'cc/chart/v2/factory-export.cc',
  'cc/chart/v2/grid-test.cc',
  'cc/chart/v2/layout.cc',
  'cc/chart/v2/lispmds-encode.cc',
  'cc/chart/v2/lispmds-export.cc',
  'cc/chart/v2/lispmds-import.cc',
  'cc/chart/v2/lispmds-token.cc',
  'cc/chart/v2/name-format.cc',
  'cc/chart/v2/optimize.cc',
  'cc/chart/v2/procrustes.cc',
  'cc/chart/v2/randomizer.cc',
  'cc/chart/v2/rjson-import.cc',
  'cc/chart/v2/serum-circle.cc',
  'cc/chart/v2/stress.cc',
  'cc/chart/v2/text-export.cc',
  'cc/chart/v2/titers.cc',
]

# ----------------------------------------------------------------------
# libae
# ----------------------------------------------------------------------

libae = library(
  'ae',
  sources : sources_tree + sources_locdb_v3 +
            sources_sequences + sources_virus + sources_whocc + sources_chart_v3 +
            sources_utils +
            sources_chart_v2 + sources_ad,
  include_directories : include_cc,
  dependencies : [alglib, omp, xxhash, fmt, simdjson, range_v3, lexy, xlnt, brotli_enc, brotli_dec, bzip2, xz, zlib],
  link_args : libomp_dir,
  install : true)

# ----------------------------------------------------------------------
# ad executables
# ----------------------------------------------------------------------

chart_relax = executable(
  'chart-relax',
  sources : ['cc/chart/v2/chart-relax.cc'],
  include_directories : include_cc,
  dependencies : [fmt, range_v3, simdjson],
  link_with : [libae],
  install : true)

# ----------------------------------------------------------------------
# tests ae
# ----------------------------------------------------------------------

# test_basic = executable('test-basic', 'cc/test/basic.cc',
#                  include_directories : include_cc,
#                  dependencies : [fmt, dependency('xlnt'), simdjson, range_v3, lexy],
#                  install : true)

# test('ae test basic', test_basic)

test_virus_name = executable(
  'test-virus-name',
  sources : ['cc/test/test-virus-name.cc'],
  include_directories : include_cc,
  dependencies : [fmt, simdjson, range_v3],
  link_with : [libae],
  install : true)

test_passage = executable(
  'test-passage',
  sources : ['cc/test/test-passage.cc'],
  include_directories : include_cc,
  dependencies : [fmt, range_v3, lexy],
  link_with : [libae],
  install : true)

test_date = executable(
  'test-date',
  sources : ['cc/test/test-date.cc'],
  include_directories : include_cc,
  dependencies : [fmt],
  link_with : [libae],
  install : true)

test_named_vector = executable(
  'test-named-vector',
  sources : ['cc/test/test-named-vector.cc'],
  include_directories : include_cc,
  dependencies : [fmt],
  link_with : [libae],
  install : true)

test_pos = executable(
  'test-pos',
  sources : ['cc/test/test-pos.cc'],
  include_directories : include_cc,
  dependencies : [fmt],
  install : true)

chart_v3_test = executable(
  'chart-v3-test',
  sources : ['cc/test/chart-v3-test.cc'],
  include_directories : include_cc,
  dependencies : [fmt, range_v3, simdjson],
  link_with : [libae],
  install : true)

test('ae test virus name parsing', test_virus_name)
test('ae test date parsing', test_date)
test('ae test passage parsing', test_passage)
test('ae test_named_vector', test_named_vector)
test('ae test_pos', test_pos)
test('ae chart_v3_test', chart_v3_test)

# ----------------------------------------------------------------------
# python module
# https://github.com/mesonbuild/meson/issues/4677
# ----------------------------------------------------------------------

python3 = import('python').find_installation('python3.10')

ae_backend = python3.extension_module(
  'ae_backend',
  sources : sources_py,
  include_directories : include_cc,
  dependencies : [python3.dependency(), xxhash, pybind11, xlnt, fmt, range_v3, simdjson],
  link_with : [libae],
  install : true)

if host_machine.system() == 'darwin'
    # stubs for mypy
    stubgen = find_program('stubgen')
    ae_backend_stubs = custom_target(
      'ae_backend_stubs',
      build_by_default: true,
      input : [ae_backend],
      output: ['stubs'],
      command: [stubgen, '--package', 'ae_backend', '--output', '@OUTPUT@']
               )
endif

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
