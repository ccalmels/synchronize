project('small utilities', 'cpp', default_options: [ 'cpp_std=c++17' ])

avcpp_dep = dependency(
  'av-cpp',
  version : '>=0.1.0',
  fallback : [ 'av-cpp', 'avcpp_dep' ],
)

libcommon = static_library('common', 'src/common.cpp',
			   dependencies : avcpp_dep)

executable('synchronize', 'src/synchronize.cpp',
	   link_with: libcommon,
	   dependencies : [ avcpp_dep, dependency('threads') ] )

executable('latency', 'src/latency.cpp',
	   link_with: libcommon,
	   dependencies : avcpp_dep)

executable('transcode', 'src/transcode.cpp',
	   link_with: libcommon,
	   dependencies : avcpp_dep)

executable('rtsp2mpegts', 'src/rtsp2mpegts.cpp',
	   link_with: libcommon,
	   dependencies : avcpp_dep)
