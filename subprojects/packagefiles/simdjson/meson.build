project('simdjson', 'cpp')

simdjson_dir = include_directories('src', 'include', '.')
simdjson_sys = include_directories(is_system : true)
simdjson_inc = [simdjson_dir, simdjson_sys]
simdjson_src = files('src/simdjson.cpp')
simdjson = shared_library(
  'simdjson',
  simdjson_src,
  build_by_default : true,
  link_args : [],
  link_with : [],
  include_directories : simdjson_inc,
  install : true,
  override_options : ['cpp_std=c++17'],
  objects : [],
  install_dir : 'lib',
  cpp_args : [
    '-O3',
    '-DNDEBUG',
    '-Weffc++',
    '-Wsign-compare',
    '-Wshadow',
    '-Wwrite-strings',
    '-Wpointer-arith',
    '-Winit-self',
    '-Wconversion',
    '-Wno-sign-conversion',
    '-DSIMDJSON_THREADS_ENABLED=1',
    '-Dsimdjson_EXPORTS'
  ]
)
simdjson_dep = declare_dependency(
  link_args : [],
  link_with : simdjson,
  compile_args : [
    '-fPIC',
    '-Wall',
    '-Wextra',
    '-Weffc++',
    '-Wsign-compare',
    '-Wshadow',
    '-Wwrite-strings',
    '-Wpointer-arith',
    '-Winit-self',
    '-Wconversion',
    '-Wno-sign-conversion',
    '-DSIMDJSON_THREADS_ENABLED=1'
  ],
  include_directories : simdjson_inc
)
