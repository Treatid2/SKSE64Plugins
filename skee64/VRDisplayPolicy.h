#pragma once
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>

namespace SKEE::VR::DisplayPolicy
{
	// Logical placement identity, excluding the artwork centre which can settle
	// during initial layout. Repeated requests cannot extend the readiness window.
	using Request = std::array<double, 7>;
	inline void ReleaseTicket(std::atomic<std::uint64_t>& a_slot, std::uint64_t a_ticket)
	{
		auto expected=a_ticket;
		a_slot.compare_exchange_strong(expected,0);
	}
	enum class Decision { attempt, wait, expired };
	class Readiness
	{
	public:
		static constexpr std::uint64_t windowMs = 6000;
		static constexpr std::uint64_t spacingMs = 250;
		static constexpr unsigned maxAttempts = 12;
		Decision Next(const Request& a_request, std::uint64_t a_now)
		{
			if (!started_ || request_ != a_request) {
				request_ = a_request;
				started_ = true;
				start_ = last_ = next_ = a_now;
				attempts_ = 0;
				expired_ = false;
			}
			if (expired_ || a_now < last_ || a_now - start_ >= windowMs || attempts_ >= maxAttempts) {
				expired_ = true;
				return Decision::expired;
			}
			last_ = a_now;
			if (a_now < next_) return Decision::wait;
			++attempts_;
			next_ = a_now + spacingMs;
			return Decision::attempt;
		}
		unsigned Attempts() const { return attempts_; }
		std::uint64_t Elapsed(std::uint64_t a_now) const { return started_ && a_now >= start_ ? a_now - start_ : 0; }
	private:
		Request request_{};
		bool started_{}, expired_{};
		unsigned attempts_{};
		std::uint64_t start_{}, last_{}, next_{};
	};
	inline bool PositiveScale(float a_value) { return std::isfinite(a_value) && a_value > 0.000001F; }
	inline float NormalizedYaw(float a_value) { return std::isfinite(a_value) ? std::remainder(a_value,360.F) : 180.F; }
	inline bool OrthogonalAxes(float a_uLength, float a_vLength, float a_unitDot)
	{
		return PositiveScale(a_uLength) && PositiveScale(a_vLength) &&
			std::isfinite(a_unitDot) && std::abs(a_unitDot) < 0.01F;
	}
	class LogBudget
	{
	public:
		static constexpr unsigned limit = 16;
		bool Take() { if (emitted_ >= limit) { ++suppressed_; return false; } ++emitted_; return true; }
		unsigned Suppressed() const { return suppressed_; }
	private:
		unsigned emitted_{}, suppressed_{};
	};
}
