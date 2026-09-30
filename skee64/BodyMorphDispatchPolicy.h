#pragma once

#include <functional>
#include <mutex>
#include <type_traits>
#include <utility>

namespace SKEE::BodyMorphDispatch
{
	// Prepare work while the cache is protected, then release the cache lock
	// before handing that work to an external queue. Queue implementations may
	// execute tasks while holding their own lock, so dispatching while the cache
	// is locked creates a lock-order inversion when a queued task re-enters the
	// cache.
	template <class Mutex, class Prepare, class Dispatch>
	decltype(auto) PrepareThenDispatch(Mutex& a_mutex, Prepare&& a_prepare, Dispatch&& a_dispatch)
	{
		using pending_type = std::invoke_result_t<Prepare>;
		pending_type pending = [&]() {
			std::lock_guard lock(a_mutex);
			return std::invoke(std::forward<Prepare>(a_prepare));
		}();

		return std::invoke(std::forward<Dispatch>(a_dispatch), std::move(pending));
	}
}
