lib_sources = files(
    'libmconn.c',
    'MConn.cpp',
    'AsyncReader.cpp'
)

libmconn = static_library('libmconn',
    lib_sources,
    include_directories : inc_dir,
    dependencies: dependencies,
    install: true,
)

llibmconn_dep = declare_dependency(
  link_with : libmconn,
  include_directories : inc_dir
)

# Import the pkgconfig module
pkg = import('pkgconfig')

# Generate the pkg-config file
pkg.generate(
  name : 'libmconn',
  version : '1.0',
  libraries : libmconn,
  description : 'Multi Connect Libary'
)