#include <iostream>
#include <ffmpeg.hpp>
#include "common.hpp"

int main(int argc, char *argv[])
{
	statistics s;
	bool is_rtsp;
	int64_t t0;
	std::string url;

	if (argc < 2)
		return -1;

	url = argv[1];

	is_rtsp = (url.compare(0, 7, "rtsp://") == 0);

	av::input video;
	if (!video.open(url, is_rtsp ? "rtsp_transport=tcp" : ""))
		return -1;

	t0 = is_rtsp ? get_rtsp_t0(video) : get_metadata_t0(video);
	if (t0 == AV_NOPTS_VALUE)
		return -1;

	av::decoder dec = video.get(0);
	if (!dec)
		return -1;

	av::packet p;
	av::frame f;
	AVRational time_base = video.time_base(0);

	while (video >> p) {
		if (p.stream_index() != 0)
			continue;

		dec << p;

		while (dec >> f) {
			int64_t latency = get_time_us() - av_rescale(f.f->pts, time_base.num * 1000000,
								     time_base.den) - t0;

			// convert to ms
			latency /= 1000;

			s.add(latency);

			std::cerr << "\r\033[2K";
			std::cerr << s;
		}
	}

	return 0;
}
