// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <nlohmann/json_fwd.hpp>
namespace RE { class GFxMovie; }
namespace SKEE::CharacterInspection
{
    bool Install();
    void Register(RE::GFxMovie* movie);
    void Restore();
    // Main-thread capture only; consumers serialize the copied snapshot.
    nlohmann::json CaptureDiagnostics();
}
