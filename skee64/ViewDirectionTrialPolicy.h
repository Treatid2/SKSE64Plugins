// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cmath>
#include <cstdint>
#include <optional>

namespace SKEE::CharacterInspection
{
    // Experimental quiet-window commit, NOT a physical trigger-release detector.
    // No clock, worker, input interception or scene access is owned by this policy.
    struct ViewDirectionTrial
    {
        static constexpr std::int64_t quietMilliseconds = 400;
        bool pending{};
        float requested{}, baseline{};
        std::int64_t lastInput{};
        std::uint64_t requests{}, commits{}, cancellations{};
        bool Submit(double value, float applied, std::int64_t now)
        {
            if (!std::isfinite(value) || std::abs(value) > 60 || !std::isfinite(applied)) return false;
            if (!pending) baseline = applied;
            requested = static_cast<float>(value); lastInput = now; pending = true; ++requests;
            return true;
        }
        void Cancel() { if (pending) ++cancellations; pending = false; }
        std::optional<float> Take(float applied, std::int64_t now)
        {
            if (!pending) return std::nullopt;
            // Another menu action changed the view: do not overwrite its result.
            if (applied != baseline) { Cancel(); return std::nullopt; }
            if (now < lastInput || now-lastInput < quietMilliseconds) return std::nullopt;
            pending = false; ++commits;
            return -requested; // UI direction inverted; native/sculpt convention unchanged.
        }
    };
}
