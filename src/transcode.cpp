#include <iostream>
#include <ffmpeg.hpp>

int main(int argc, char *argv[])
{
	int gpu_index = 0;
	std::string url, out_url;

	if (argc < 3)
		return -1;

	url = argv[1];
	out_url = argv[2];

	av::input video;
	if (!video.open(url, "rtsp_transport=tcp"))
		return -1;

	av::hw_device accel = av::hw_device("cuda", std::to_string(gpu_index));
	if (!accel)
		return -1;

	av::decoder dec = video.get(accel, 0);
	if (!dec)
		return -1;

	av::output out;
	if (!out.open(out_url))
		return -1;

	av::encoder enc;

	av::packet pin, pout;
	av::frame f;
	AVRational avg_frame_rate = video.avg_frame_rate(0);
	std::string time_base = std::to_string(avg_frame_rate.den) + "/" + std::to_string(avg_frame_rate.num);
	bool metadata_written = false;

	while (video >> pin) {
		if (pin.stream_index() != 0)
			continue;

		dec << pin;

		while (dec >> f) {
			if (!metadata_written) {
				int64_t realtime = video.start_time_realtime();
				if (realtime == AV_NOPTS_VALUE)
					continue;

				metadata_written = true;
				out.add_metadata("service_name=" + std::to_string(realtime));
			}

			if (!enc)
				enc = out.add_stream(dec.get_hw_frames(),
						     "h264_nvenc",
						     "preset=llhq:spatial-aq=true:aq-strength=15:qp=33:gpu=" + std::to_string(gpu_index) + ":time_base=" + time_base);

			enc << f;
			while (enc >> pout)
				out.write_norescale(pout);
		}
	}

	return 0;
}
