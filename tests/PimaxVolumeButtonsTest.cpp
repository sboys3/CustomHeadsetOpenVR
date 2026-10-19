#include "../CustomHeadsetOpenVR/src/Helpers/PimaxVolumeButtons.h"
#include <cstdlib>
#include <iostream>

void Check(PimaxVolumeButtons::State s, bool up, bool down) {
	if (s.up != up || s.down != down) {
		std::cerr << "Unexpected volume output\n";
		std::exit(1);
	}
}

int main() {
	PimaxVolumeButtons buttons;
	Check(buttons.Update(false, false, true, 0), false, false);
	Check(buttons.Update(true, false, true, 10), false, false);
	Check(buttons.Update(true, false, true, 2000), false, false); // No hold-repeat.
	Check(buttons.Update(false, false, true, 2010), true, false);
	Check(buttons.Update(false, false, true, 2109), true, false);
	Check(buttons.Update(false, false, true, 2110), false, false);
	Check(buttons.Update(false, true, true, 2200), false, false);
	Check(buttons.Update(false, false, true, 2210), false, true);
	Check(buttons.Update(false, false, true, 2310), false, false);

	// All staggered press/release orders, including a long first press and
	// re-pressing one side while the other remains held, suppress volume.
	for (unsigned first : { 1u, 2u, 3u }) {
		for (unsigned last : { 1u, 2u, 3u }) {
			buttons.Reset();
			Check(buttons.Update(false, false, true, 0), false, false);
			Check(buttons.Update(first & 1, first & 2, true, 10), false, false);
			Check(buttons.Update(true, true, true, 2000), false, false);
			Check(buttons.Update(last & 1, last & 2, true, 2010), false, false);
			Check(buttons.Update(true, true, true, 2020), false, false);
			Check(buttons.Update(last & 1, last & 2, true, 2030), false, false);
			Check(buttons.Update(false, false, true, 2040), false, false);
			Check(buttons.Update(true, false, true, 2050), false, false);
			Check(buttons.Update(false, false, true, 2060), true, false);
		}
	}

	// A held input on startup or after a disabled runtime must be released first.
	buttons.Reset();
	Check(buttons.Update(true, false, true, 0), false, false);
	Check(buttons.Update(false, false, true, 10), false, false);
	Check(buttons.Update(true, false, true, 20), false, false);
	Check(buttons.Update(false, false, true, 30), true, false);
	Check(buttons.Update(false, true, false, 40), false, false);
	Check(buttons.Update(false, true, true, 50), false, false);
	Check(buttons.Update(false, false, true, 60), false, false);

	// Quick distinct taps are queued, with a release interval for SteamVR.
	Check(buttons.Update(true, false, true, 70), false, false);
	Check(buttons.Update(false, false, true, 80), true, false);
	Check(buttons.Update(true, false, true, 90), true, false);
	Check(buttons.Update(false, false, true, 100), true, false);
	Check(buttons.Update(false, true, true, 110), true, false);
	Check(buttons.Update(false, false, true, 120), true, false);
	Check(buttons.Update(false, false, true, 180), false, false);
	Check(buttons.Update(false, false, true, 229), false, false);
	Check(buttons.Update(false, false, true, 230), true, false);
	Check(buttons.Update(false, false, true, 330), false, false);
	Check(buttons.Update(false, false, true, 380), false, true);
	Check(buttons.Update(false, false, true, 480), false, false);
	Check(buttons.Update(false, false, true, 1000), false, false);

	// Disable/disconnect cancels active and queued actions.
	Check(buttons.Update(true, false, true, 1010), false, false);
	Check(buttons.Update(false, false, true, 1020), true, false);
	Check(buttons.Update(true, false, true, 1030), true, false);
	Check(buttons.Update(false, false, true, 1040), true, false);
	Check(buttons.Update(false, false, false, 1050), false, false);
	Check(buttons.Update(false, false, true, 2000), false, false);
	Check(buttons.Update(false, false, true, 3000), false, false);
	std::cout << "PASS: individual taps, long holds, 9 chord orders, startup, queued taps, disconnect\n";
}
