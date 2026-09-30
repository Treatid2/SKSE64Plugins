#include "BodyMorphDispatchPolicy.h"

#include <cassert>
#include <stdexcept>
#include <vector>

namespace
{
	struct TrackingMutex
	{
		void lock()
		{
			assert(!locked);
			locked = true;
		}

		void unlock()
		{
			assert(locked);
			locked = false;
		}

		bool locked{ false };
	};
}

int main()
{
	TrackingMutex cacheMutex;
	bool dispatched = false;

	const auto result = SKEE::BodyMorphDispatch::PrepareThenDispatch(
		cacheMutex,
		[&]() {
			assert(cacheMutex.locked);
			return std::vector<int>{ 1, 2, 3 };
		},
		[&](std::vector<int> pending) {
			assert(!cacheMutex.locked);
			dispatched = true;
			return pending.size();
		});

	assert(dispatched);
	assert(result == 3);

	bool threw = false;
	try {
		SKEE::BodyMorphDispatch::PrepareThenDispatch(
			cacheMutex,
			[&]() -> std::vector<int> {
				assert(cacheMutex.locked);
				throw std::runtime_error("prepare failed");
			},
			[](std::vector<int>) {
				assert(false && "dispatch must not run after a prepare failure");
			});
	} catch (const std::runtime_error&) {
		threw = true;
	}

	assert(threw);
	assert(!cacheMutex.locked);
	return 0;
}
