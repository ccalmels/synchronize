project('small utilities', 'cpp', default_options: [ 'cpp_std=c++14' ])

libav = subproject('av-cpp')
libavdep = libav.get_variable('avcpp_dep')

executable('synchronize', [
	    'src/common.cpp',
	    'src/synchronize.cpp'
          ],
	   dependencies : [ libavdep, dependency('threads') ] )

executable('latency', [
	    'src/common.cpp',
	    'src/latency.cpp'
          ],
	   dependencies : libavdep)

executable('transcode', [
	    'src/common.cpp',
	    'src/transcode.cpp'
          ],
	   dependencies : libavdep)
