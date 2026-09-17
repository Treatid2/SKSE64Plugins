#pragma once
#include <cstdint>
#include <string_view>

namespace SKEE::VR::KeyboardPolicy
{
	inline bool IsValidUTF8(std::string_view a_text)
	{
		for (std::size_t i = 0; i < a_text.size();) {
			const auto lead = static_cast<unsigned char>(a_text[i++]);
			if (lead < 0x80) continue;
			unsigned continuation{};
			std::uint32_t code{}, minimum{};
			if (lead >= 0xC2 && lead <= 0xDF) { continuation = 1; code = lead & 0x1F; minimum = 0x80; }
			else if (lead >= 0xE0 && lead <= 0xEF) { continuation = 2; code = lead & 0x0F; minimum = 0x800; }
			else if (lead >= 0xF0 && lead <= 0xF4) { continuation = 3; code = lead & 0x07; minimum = 0x10000; }
			else return false;
			if (a_text.size() - i < continuation) return false;
			while (continuation--) {
				const auto next = static_cast<unsigned char>(a_text[i++]);
				if ((next & 0xC0) != 0x80) return false;
				code = (code << 6) | (next & 0x3F);
			}
			if (code < minimum || code > 0x10FFFF || (code >= 0xD800 && code <= 0xDFFF)) return false;
		}
		return true;
	}
}
