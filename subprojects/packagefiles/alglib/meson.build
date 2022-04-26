project('alglib', 'cpp')

alglib_inc = [
  include_directories('src'),
  include_directories(is_system : true)
]
alglib_src = files(
  'src/alglibinternal.cpp',
  'src/alglibmisc.cpp',
  'src/ap.cpp',
  'src/dataanalysis.cpp',
  'src/diffequations.cpp',
  'src/fasttransforms.cpp',
  'src/integration.cpp',
  'src/interpolation.cpp',
  'src/kernels_avx2.cpp',
  'src/kernels_fma.cpp',
  'src/kernels_sse2.cpp',
  'src/linalg.cpp',
  'src/optimization.cpp',
  'src/solvers.cpp',
  'src/specialfunctions.cpp',
  'src/statistics.cpp'
)

alglib = shared_library(
  'alglib',
  alglib_src,
  build_by_default : true,
  link_args : [],
  link_with : [],
  include_directories : alglib_inc,
  install : true,
  override_options : ['cpp_std=c++20'],
  install_dir : 'lib',
  cpp_args : [
    '-DAE_COMPILE_MINLBFGS',
    '-DAE_COMPILE_PCA',
    '-DAE_COMPILE_MINCG',
    '-mfpmath=sse',
    # '-Wno-undef',
    # '-Wno-shorten-64-to-32',
    '-Wno-unused-parameter',
    '-Wno-null-pointer-subtraction',
    '-Wno-deprecated-volatile',
    '-Wno-uninitialized',       # g++13
    # '-Wno-volatile',            # g++13, clang 13 complains
  ]
)
alglib_dep = declare_dependency(
  link_args : [],
  link_with : alglib,
  compile_args : [],
  include_directories : include_directories('src')
)
