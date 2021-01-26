project('small utilities', 'cpp', default_options: [ 'cpp_std=c++17' ])

libav = subproject('av-cpp')
libavdep = libav.get_variable('avcpp_dep')

libcommon = static_library('common', 'src/common.cpp',
			   dependencies : libavdep)

executable('synchronize', 'src/synchronize.cpp',
	   link_with: libcommon,
	   dependencies : [ libavdep, dependency('threads') ] )

executable('latency', 'src/latency.cpp',
	   link_with: libcommon,
	   dependencies : libavdep)

executable('transcode', 'src/transcode.cpp',
	   link_with: libcommon,
	   dependencies : libavdep)

executable('rtsp2mpegts', 'src/rtsp2mpegts.cpp',
	   link_with: libcommon,
	   dependencies : libavdep)
