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
  add_project_arguments('-Weverything', '-Wno-c++98-compat', '-Wno-c++98-c++11-compat', '-Wno-c++98-compat-pedantic', language : 'cpp')
endif

# ----------------------------------------------------------------------

cmake = import('cmake')
lexy = cmake.subproject('lexy').dependency('lexy_core')

subproject('fmt')
subproject('xlnt')
# simdjson = import('cmake').subproject('simdjson').dependency('simdjson')
subproject('simdjson')
subproject('range_v3')
subproject('pybind11')

# websocketpp
# asio
# apache module

# ----------------------------------------------------------------------

test_basic = executable('test-basic', 'cc/test/basic.cc',
                 include_directories : include_directories('./cc'),
                 dependencies : [dependency('fmt'), dependency('xlnt'), dependency('simdjson'), dependency('range-v3'),
                                 lexy
                                ],
                 install : true)

test('ae test basic', test_basic)

# ----------------------------------------------------------------------
# python module
# https://github.com/mesonbuild/meson/issues/4677
# ----------------------------------------------------------------------

python3 = import('python').find_installation('python3')

pyb_lib = python3.extension_module(
  'pyb',
  sources : ['cc/core/py.cc'],
  include_directories : include_directories('./cc'),
  dependencies : [dependency('python3'), dependency('pybind11'), dependency('fmt'), dependency('lexy')],
  install : true)

# https://gabmus.org/posts/python-unittest-meson/
# envdata = environment()
# python_paths = [join_paths(meson.current_build_dir(), '..')]
# envdata.append('PYTHONPATH', python_paths)
# envdata.append('TESTS_BUILDDIR', meson.current_build_dir())
test('pyb test 1', python3,
     args : ['-V'],
     # args: ['-m', 'unittest', 'tests'],
     # env: envdata
    )

# ----------------------------------------------------------------------
