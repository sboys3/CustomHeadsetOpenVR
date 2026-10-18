#pragma once

#include <atomic>
#include <string>
#include <thread>

// Eye tracking from another process through a small shared memory file, for platforms without the Pimax runtime
// (Linux). The file is written by an external eye tracker driver; the reader forwards each new sample to
// EyeTrackingOutput, which hands it to SteamVR's eye tracking component (OpenXR XR_EXT_eye_gaze_interaction).
//
// Default file: /dev/shm/omarchyvr-eye-gaze (environment CUSTOM_HEADSET_EYE_GAZE_SHM overrides it). The format
// (136 bytes, little endian, OpenXR head frame: x right, y up, -z forward; CLOCK_MONOTONIC nanoseconds):
//   0 u64 seq_begin | 8 u32 magic "OVEG" | 12 u32 version 1 | 16 i64 sample time | 24 i64 publish time
//   32 u32 flags (1 left dir, 2 right dir, 4 combined dir, 8 left origin, 16 right origin) | 36 f32 ipd mm
//   40 f32[3] left origin | 52 f32[3] left dir | 64 right origin | 76 right dir | 88 combined origin | 100 combined dir
//   112 u64 sample number | 120 u32 state (2 = streaming) | 124 u32 reserved | 128 u64 seq_end
// The writer stores an odd seq_begin, the body, the new even value in seq_end, then in seq_begin; a copy is
// consistent when both are equal and even.
class SharedMemoryEyeSource {
public:
	~SharedMemoryEyeSource();
	void Start();
	void Stop();
	bool IsRunning() const { return running; }

private:
	void Thread();
	std::atomic<bool> running = false;
	std::thread thread;
};

extern SharedMemoryEyeSource sharedMemoryEyeSource;
