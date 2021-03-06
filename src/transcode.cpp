#include <iostream>
#include <ffmpeg.hpp>
#include "common.hpp"

int main(int argc, char *argv[])
{
	std::string gpu = "0", url, out_url;

	if (argc < 3)
		return -1;

	url = argv[1];
	out_url = argv[2];
	if (argc > 3)
		gpu = argv[3];

	av::input in;
	if (!in.open(url, "rtsp_transport=tcp"))
		return -1;

	av::hw_device accel = av::hw_device("cuda", gpu);
	if (!accel)
		return -1;

	av::decoder dec = in.get(accel, 0);
	if (!dec)
		return -1;

	av::output out;
	if (!out.open(out_url))
		return -1;

	out.add_metadata("service_name="
			 + std::to_string(get_rtsp_t0(in)));

	av::packet pin, pout;
	av::frame f;
	av::encoder enc;
	auto options = "preset=llhq:spatial-aq=true:aq-strength=15:qp=33:"
		"time_base=" + av::to_string(av_inv_q(in.frame_rate(0)));

	while (in >> pin) {
		if (pin.stream_index() != 0)
			continue;

		dec << pin;
		while (dec >> f) {
			if (!enc)
				enc = out.add_stream(dec.get_hw_frames(),
						     "h264_nvenc", options);
			enc << f;
			while (enc >> pout)
				out.write_norescale(pout);
		}
	}

	return 0;
}
