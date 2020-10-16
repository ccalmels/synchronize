#pragma once
#include <iostream>

#include <ffmpeg.hpp>

// TODO: use std::chrono::milliseconds instead of int64
class statistics {
public:
	statistics();

	void add(int64_t latency);
private:
	int64_t min, max, total;
	size_t count;
	friend std::ostream& operator<<(std::ostream&, const statistics&);
};

std::ostream& operator<<(std::ostream& os, const statistics& s);

int64_t get_time();
int64_t get_rtsp_t0(av::input &video);
int64_t get_metadata_t0(av::input &video);
