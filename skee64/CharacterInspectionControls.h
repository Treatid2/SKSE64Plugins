// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
namespace RE { class GFxMovie; }
namespace SKEE::CharacterInspection
{
    bool Install();
    void Register(RE::GFxMovie* movie);
    void Restore();
}
