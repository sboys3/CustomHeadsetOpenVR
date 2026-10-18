#include "SharedMemoryEyeSource.h"

#ifndef _WIN32

#include "EyeTrackingOutput.h"
#include "../Driver/DriverLog.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>

namespace {

constexpr uint32_t kMagic = 0x4745564fu; // "OVEG"
constexpr uint32_t kVersion = 1;
constexpr size_t kSize = 136;
constexpr uint32_t kLeft = 1, kRight = 2, kCombined = 4;
constexpr uint32_t kStreaming = 2;

struct Snapshot {
	int64_t sampleNs;
	uint32_t flags;
	float ipdMm;
	float dir[3][3]; // left, right, combined
	uint64_t sample;
	uint32_t state;
};

template <typename T> T At(const uint8_t* b, size_t offset) {
	T v;
	memcpy(&v, b + offset, sizeof(T));
	return v;
}

// seq_end, body, seq_begin, each with its own pread (the order the writer's pwrites guarantee).
bool ReadSnapshot(int fd, Snapshot& out) {
	for (int attempt = 0; attempt < 8; attempt++) {
		uint8_t b[kSize];
		if (pread(fd, b + 128, 8, 128) != 8 || pread(fd, b + 8, 120, 8) != 120 || pread(fd, b, 8, 0) != 8) {
			return false;
		}
		const uint64_t begin = At<uint64_t>(b, 0), end = At<uint64_t>(b, 128);
		if (begin != end || begin == 0 || (begin & 1)) {
			continue;
		}
		if (At<uint32_t>(b, 8) != kMagic || At<uint32_t>(b, 12) != kVersion) {
			return false;
		}
		out.sampleNs = At<int64_t>(b, 16);
		out.flags = At<uint32_t>(b, 32);
		out.ipdMm = At<float>(b, 36);
		for (int i = 0; i < 3; i++) {
			memcpy(out.dir[i], b + 52 + 24 * i, 12);
		}
		out.sample = At<uint64_t>(b, 112);
		out.state = At<uint32_t>(b, 120);
		return true;
	}
	return false;
}

int64_t MonotonicNs() {
	timespec ts{};
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return int64_t(ts.tv_sec) * 1000000000 + ts.tv_nsec;
}

// Angle right / up of a head-frame direction (-z forward); false when it does not point forward.
bool Angles(const float d[3], float& x, float& y) {
	if (!(d[2] < -1e-3f)) {
		return false;
	}
	x = atan2f(d[0], -d[2]);
	y = atan2f(d[1], -d[2]);
	return std::isfinite(x) && std::isfinite(y);
}

} // namespace

SharedMemoryEyeSource::~SharedMemoryEyeSource() {
	Stop();
}

void SharedMemoryEyeSource::Start() {
	if (!running.exchange(true)) {
		DriverLog("Starting the shared memory eye tracking thread");
		thread = std::thread(&SharedMemoryEyeSource::Thread, this);
	}
}

void SharedMemoryEyeSource::Stop() {
	if (running.exchange(false) && thread.joinable()) {
		DriverLog("Stopping the shared memory eye tracking thread");
		thread.join();
		thread = {};
	}
}

void SharedMemoryEyeSource::Thread() {
	const char* env = getenv("CUSTOM_HEADSET_EYE_GAZE_SHM");
	const std::string path = env && *env ? env : "/dev/shm/omarchyvr-eye-gaze";
	int fd = -1;
	uint64_t lastSample = 0;
	bool loggedWaiting = false, loggedStreaming = false;
	while (running) {
		if (fd < 0) {
			fd = open(path.c_str(), O_RDONLY | O_CLOEXEC);
			if (fd < 0) {
				if (!loggedWaiting) {
					DriverLog("Eye tracking: waiting for %s (the eye tracker driver)", path.c_str());
					loggedWaiting = true;
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(500));
				continue;
			}
			DriverLog("Eye tracking: reading the gaze from %s", path.c_str());
		}
		Snapshot s{};
		if (!ReadSnapshot(fd, s)) {
			// a writer that replaced the file, or another layout: open again
			close(fd);
			fd = -1;
			std::this_thread::sleep_for(std::chrono::milliseconds(200));
			continue;
		}
		if (s.state == kStreaming && s.sample != lastSample && (s.flags & (kLeft | kRight | kCombined))) {
			lastSample = s.sample;
			float lx = 0, ly = 0, rx = 0, ry = 0, cx = 0, cy = 0;
			const bool c = (s.flags & kCombined) && Angles(s.dir[2], cx, cy);
			bool l = (s.flags & kLeft) && Angles(s.dir[0], lx, ly);
			bool r = (s.flags & kRight) && Angles(s.dir[1], rx, ry);
			// one eye lost (blink of one eye, glasses): both follow the other eye, or the combined gaze
			if (!l && r) { lx = rx; ly = ry; l = true; }
			if (!r && l) { rx = lx; ry = ly; r = true; }
			if (!l && c) { lx = rx = cx; ly = ry = cy; l = r = true; }
			if (l && r) {
				// the sample time on the clock EyeTrackingOutput uses (high_resolution_clock, seconds)
				const double age = double(MonotonicNs() - s.sampleNs) * 1e-9;
				const double now = std::chrono::duration_cast<std::chrono::microseconds>(
					std::chrono::high_resolution_clock::now().time_since_epoch()).count() / 1000000.0;
				eyeTrackingOutput.SetEyeTrackingData(lx, ly, rx, ry, s.ipdMm > 40.f ? s.ipdMm : 0.f,
					now - std::max(0.0, std::min(age, 0.5)));
				if (!loggedStreaming) {
					DriverLog("Eye tracking: gaze is arriving");
					loggedStreaming = true;
				}
			}
		}
		// the tracker runs at about 120 Hz
		std::this_thread::sleep_for(std::chrono::milliseconds(2));
	}
	if (fd >= 0) {
		close(fd);
	}
}

#else

SharedMemoryEyeSource::~SharedMemoryEyeSource() {}
void SharedMemoryEyeSource::Start() {}
void SharedMemoryEyeSource::Stop() {}
void SharedMemoryEyeSource::Thread() {}

#endif

SharedMemoryEyeSource sharedMemoryEyeSource = {};
