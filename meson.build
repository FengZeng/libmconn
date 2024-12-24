project('libmconn', 'c', 'cpp',
  version : '1.0',
  default_options : ['c_std=c11', 'cpp_std=c++17']
)

inc_dir = include_directories('include')

# ffmpeg
libavcodec = dependency('libavcodec', version: '>= 58.134.100')
libavfilter = dependency('libavfilter', version: '>= 7.110.100')
libavformat = dependency('libavformat', version: '>= 58.76.100')
libavutil = dependency('libavutil', version: '>= 56.70.100')
libswresample = dependency('libswresample', version: '>= 3.9.100')
libswscale = dependency('libswscale', version: '>= 5.9.100')

dependencies = [libavcodec,
                libavfilter,
                libavformat,
                libavutil,
                libswresample,
                libswscale]

subdir('src')
if host_machine.system() != 'windows'
  subdir('test')
endif

install_headers('include/libmconn.h')