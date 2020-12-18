#include <limits>
#include <chrono>
#include <sstream>

#include "common.hpp"

statistics::statistics() : min(std::numeric_limits<int64_t>::max()),
			   max(0), total(0), count(0) {}

void statistics::add(int64_t latency)
{
	if (latency < min)
		min = latency;
	if (latency > max)
		max = latency;

	total += latency;
	count++;
}

std::ostream& operator<<(std::ostream& os, const statistics& s)
{
	if (s.count)
		os << "min: " << s.min << " max: " << s.max
		   << " mean: " << s.total / s.count;
	else
		os << "no stats";
	return os;
}

int64_t get_time_us()
{
	auto now = std::chrono::system_clock::now().time_since_epoch();

	return std::chrono::duration_cast<std::chrono::microseconds>(
		now).count();
}

int64_t get_rtsp_t0(av::input &video)
{
	av::packet p;

	while (video >> p && video.start_time_realtime() == AV_NOPTS_VALUE) {}

	return video.start_time_realtime();
}

int64_t get_metadata_t0(av::input &video)
{
	int64_t t0 = AV_NOPTS_VALUE;
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
	return t0;
}
