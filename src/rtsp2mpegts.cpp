#include <iostream>
#include <ffmpeg.hpp>
#include "common.hpp"

static int decode_rtsp(const std::string &in_url, const std::string &out_url)
{
	av::input in;
	av::output out;
	av::packet p;

	if (!in.open(in_url, "rtsp_transport=tcp"))
		return -1;

	if (!out.open(out_url))
		return -1;

	if (out.add_stream(in, 0) != 0)
		return -1;

	out.add_metadata("service_name="
			 + std::to_string(get_rtsp_t0(in)));

	while (in >> p)
		if (p.stream_index() == 0)
			out << p;

	return 0;
}

int main(int argc, char *argv[])
{
	if (argc < 3)
		return -1;

	return decode_rtsp(argv[1], argv[2]);
}
