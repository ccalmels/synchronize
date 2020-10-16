#include <iostream>
#include <iomanip>
#include <thread>
#include <future>
#include <mutex>
#include <condition_variable>
#include <list>

#include <ffmpeg.hpp>
#include "common.hpp"

struct queue {
	queue(size_t capacity = 90) : capacity(capacity), max(0), closed(false) {}

	void enqueue(av::frame &f) {
		std::lock_guard<std::mutex> l(m);
#if 0
		std::cerr << (void*)this << std::dec
			  << ": queue size: " << filled.size()
			  << " enqueue frame: " << f.f->pts
			  << std::endl;
#endif
		filled.push_back(f);

		if (filled.size() > capacity)
			filled.pop_front();

		if (max < filled.size())
			max = filled.size();

		cv.notify_one();
	}

	av::frame acquire() {
		av::frame f;
		std::unique_lock<std::mutex> l(m);

		while (filled.empty() && !closed)
			cv.wait(l);

		if (!filled.empty()) {
			f = filled.front();
			filled.pop_front();
		}

		return f;
	}

	bool acquire(av::frame &f, int64_t pts) {
		std::unique_lock<std::mutex> l(m);

		if (filled.empty())
			return false;

		if (pts < filled.front().f->pts) {
			/*
			 * we are reading faster than the other stream
			 * the only choice is to let the other keeps
			 * reading
			 */
			return false;
		}

		while ((pts >= filled.back().f->pts) && !closed) {
			/*
			 * the other thread is in advance just wait a
			 * little bit to receive some new frames
			 */
			cv.wait(l);
		}

		if (closed)
			return false;

		/*
		 * From here we can say that the queue looks like
		 * this:
		 *
		 * filled.front()->pts <= pts < filled.back()->pts
		 *
		 * So we should be able to find the corresponding
		 * frame now. We will take the last frame that is
		 * "lower" than pts. We are sure to find it because we
		 * know that at least the last frame in the queue is
		 * greater than pts.
		 *
		 * In order to retrieve the righ frame we just take
		 * the first one. If the next frame pts is greater
		 * than pts than we already took the right frame. If
		 * not just drop the current frame and take the next
		 * one.
		 */
		do {
			f = filled.front();
			filled.pop_front();
		} while (pts >= filled.front().f->pts);

		return true;
	}

	void close(bool clear = false) {
		std::lock_guard<std::mutex> l(m);

		closed = true;
		if (clear)
			filled.clear();

		cv.notify_all();
	}

	bool is_closed() {
		std::lock_guard<std::mutex> l(m);

		return closed && filled.empty();
	}

	size_t size() {
		std::lock_guard<std::mutex> l(m);

		return filled.size();
	}

	size_t max_size() {
		std::lock_guard<std::mutex> l(m);

		return max;
	}

	std::mutex m;
	std::condition_variable cv;
	std::list<av::frame> filled;
	size_t capacity, max;
	bool closed;
};

#define VIDEO_STREAM_INDEX 0

static void read_input(av::input &in, queue &q)
{
	av::hw_device accel;
#if 0
	accel = av::hw_device("cuda");
	if (!accel)
		accel = av::hw_device("vaapi");
#endif
	av::decoder dec = in.get(accel, VIDEO_STREAM_INDEX);
	if (!dec) {
		std::cerr << "Unable to get a decoder" << std::endl;
		return;
	}

	av::packet p;
	av::frame f;

	while (in >> p) {
		if (p.stream_index() != VIDEO_STREAM_INDEX)
			continue;

		dec << p;

		while (dec >> f)
			q.enqueue(f);
	}

	// flush
	dec.flush();
	while (dec >> f)
		q.enqueue(f);
}

static void read_video(const std::string &url,
		       std::promise<int64_t> t0_future, queue &q)
{
	av::input in;
	bool is_rtsp;

	is_rtsp = (url.compare(0,7, "rtsp://") == 0);

	if (in.open(url, is_rtsp ? "rtsp_transport=tcp" : "")) {
		int64_t t0 = is_rtsp ? get_rtsp_t0(in) : get_metadata_t0(in);

		std::cerr << "t0: " << t0 << std::endl;

		AVRational time_base = in.time_base(VIDEO_STREAM_INDEX);
		/*
		 * In this part we will convert t0 that is a NTP date
		 * (us since epoch) to the video stream timebase. The
		 * goal here is to only manipulate date and duration
		 * on the same timebase.
		 *
		 * The timebase of the ntpdate is 1 / 1000000. (us)
		 * The timebase of the stream is time_base.num / time_base.den.
		 *
		 * So convertion of the ntp date is done like this:
		 * t0_in_stream_timebase = (t0 / 1000000) / (time_base.num / time_base.den)
		 *                       = t0 * time_base.den / (1000000 * time_base.num)
		 *
		 * av_rescale(a, b, c) is equivalent to a * b / c
		 */
		t0_future.set_value(av_rescale(t0, time_base.den, time_base.num * 1000000));

		read_input(in, q);
	} else
		std::cerr << "Unable to open '" << url << "'" << std::endl;

	q.close();
}

int main(int argc, char *argv[])
{
	statistics s;

	if (argc < 3) {
		std::cerr << "Usage: " << argv[0]
			  << " <url> <url>" << std::endl;
		return -1;
	}

	queue q1, q2;
	std::thread t1,t2;
	std::promise<int64_t> t0_promise1, t0_promise2;
	auto t0_future1 = t0_promise1.get_future();
	auto t0_future2 = t0_promise2.get_future();

	t1 = std::thread(read_video, argv[1], std::move(t0_promise1), std::ref(q1));
	t2 = std::thread(read_video, argv[2], std::move(t0_promise2), std::ref(q2));

	int64_t delta = t0_future1.get() - t0_future2.get();

	while (!q1.is_closed() && !q2.is_closed()) {
		// start reading frame here
		av::frame f1, f2;

		std::cerr << "\r\033[K";
		std::cerr << "q1: " << std::setw(2) << q1.size()
			  << "/" << std::setw(2) << q1.max_size()
			  << " q2: " << std::setw(2) << q2.size()
			  << "/" << std::setw(2) << q2.max_size();

		f1 = q1.acquire();

		if (q2.acquire(f2, f1.f->pts + delta)) {
			// do something with f1 and f2
		} else {
			std::cerr << std::endl << "can't find a second frame" << std::endl;
		}

	}

	t1.join();
	t2.join();
	return 0;
}
