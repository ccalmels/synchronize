#include <iostream>
#include <sstream>
#include <ffmpeg.hpp>
#include <sys/time.h>

struct statistics {
	int64_t min, max, total;
	size_t count;

	void add(int64_t latency) {
		if (latency < min)
			min = latency;
		if (latency > max)
			max = latency;

		total += latency;
		count++;
	}
};

std::ostream& operator<<(std::ostream& os, const statistics& s)
{
	os << "min: " << s.min << " max: " << s.max
	   << " mean: " << s.total / s.count;
	return os;
}

static int64_t get_time()
{
	struct timeval tv;

	gettimeofday(&tv, nullptr);

	return tv.tv_usec + 1000000 * tv.tv_sec;
}

int main(int argc, char *argv[])
{
	statistics s = { 1000000, 0, 0, 0 };
	bool is_rtsp;
	int64_t t0 = AV_NOPTS_VALUE;
	std::string url;

	if (argc < 2)
		return -1;

	url = argv[1];

	is_rtsp = (url.compare(0, 7, "rtsp://") == 0);

	av::input video;
	if (!video.open(url, is_rtsp ? "rtsp_transport=tcp" : ""))
		return -1;

	if (!is_rtsp) {
		// get t0 from metadata
		std::stringstream sstr(video.program_metadata(0));
		std::string tmp;

		while (getline(sstr, tmp, ':')) {
			auto equal = tmp.find('=');

			if (equal == std::string::npos)
				continue;

			if (tmp.compare(0, equal, "service_name") == 0) {
				tmp.erase(0, equal + 1);

				t0 = std::stoll(tmp);
				break;
			}
		}

		if (t0 == AV_NOPTS_VALUE) {
			std::cerr << "Unable to find t0" << std::endl;
			return -1;
		}
	}

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
			if (t0 == AV_NOPTS_VALUE) {
				int64_t realtime = video.start_time_realtime();
				if (realtime == AV_NOPTS_VALUE)
					continue;

				t0 = realtime;
			}

			int64_t latency = get_time() - av_rescale(f.f->pts, time_base.num * 1000000,
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
