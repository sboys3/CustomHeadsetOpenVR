#pragma once

#include <cstdint>
#include <deque>

// Defer individual volume actions until release so a staggered dashboard chord
// never changes volume. Keep the resulting input visible across compositor frames.
class PimaxVolumeButtons {
public:
	struct State { bool up = false; bool down = false; };

	State Update(bool up, bool down, bool enabled, uint64_t nowMs) {
		if (!enabled) {
			Reset();
			return {};
		}
		const unsigned held = (up ? 1u : 0u) | (down ? 2u : 0u);
		if (!armed) {
			// Ignore a button already held at startup/reconnection.
			armed = held == 0;
			return {};
		}
		if (held) {
			seen |= held;
		} else if (seen) {
			if (seen == 1 || seen == 2) pending.push_back(seen);
			seen = 0;
		}

		if (active && nowMs >= untilMs) {
			active = 0;
			untilMs = nowMs + 50; // Ensure a released input between queued clicks.
		}
		if (!active && nowMs >= untilMs && !pending.empty()) {
			active = pending.front();
			pending.pop_front();
			untilMs = nowMs + 100;
		}
		return { active == 1, active == 2 };
	}

	void Reset() {
		armed = false;
		seen = active = 0;
		untilMs = 0;
		pending.clear();
	}

private:
	bool armed = false;
	unsigned seen = 0;
	unsigned active = 0;
	uint64_t untilMs = 0;
	std::deque<unsigned> pending;
};
