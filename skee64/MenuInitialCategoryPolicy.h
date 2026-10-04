// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <string_view>

namespace SKEE::MenuConfiguration
{
    // Zero means preserve stock All, whose composite mask can be extended by
    // providers. Race is the stable category flag, not a heading or list index.
    enum class InitialCategory : unsigned { All = 0, Race = 2 };
    struct InitialCategoryOption { InitialCategory category; bool valid; };

    constexpr InitialCategoryOption ParseInitialCategory(std::string_view value)
    {
        const auto whitespace = [](char c) {
            return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\v' || c == '\f';
        };
        while (!value.empty() && whitespace(value.front())) value.remove_prefix(1);
        while (!value.empty() && whitespace(value.back())) value.remove_suffix(1);
        const auto equal = [value](std::string_view token) {
            if (value.size() != token.size()) return false;
            for (std::size_t i = 0; i < value.size(); ++i) {
                auto c = value[i];
                if (c >= 'A' && c <= 'Z') c = static_cast<char>(c + ('a' - 'A'));
                if (c != token[i]) return false;
            }
            return true;
        };
        if (value.empty() || equal("all")) return { InitialCategory::All, true };
        if (equal("race")) return { InitialCategory::Race, true };
        return { InitialCategory::All, false };
    }
}
