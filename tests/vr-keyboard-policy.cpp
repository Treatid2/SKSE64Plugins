#include "RaceSexMenuVRKeyboardPolicy.h"
#include <iostream>
#include <string>

int main()
{
	using SKEE::VR::KeyboardPolicy::IsValidUTF8;
	unsigned failures{};
	const auto check = [&](std::string_view text, bool expected, const char* name) {
		if (IsValidUTF8(text) != expected) { std::cerr << "FAIL " << name << '\n'; ++failures; }
	};
	check("", true, "empty");
	check("Prisoner", true, "ascii");
	check("\xC3\xA9", true, "two_byte");
	check("\xE2\x82\xAC", true, "three_byte");
	check("\xF0\x9F\x98\x80", true, "four_byte");
	check("\xF4\x8F\xBF\xBF", true, "maximum_scalar");
	check("\x80", false, "orphan_continuation");
	check("\xC0\x80", false, "overlong_two");
	check("\xE0\x80\x80", false, "overlong_three");
	check("\xF0\x80\x80\x80", false, "overlong_four");
	check("\xED\xA0\x80", false, "surrogate");
	check("\xF4\x90\x80\x80", false, "above_unicode_maximum");
	check("\xF5\x80\x80\x80", false, "invalid_lead");
	check("\xF0\x9F\x98", false, "truncated");
	check("\xE2\x28\xAC", false, "bad_continuation");
	std::string multi;
	for (unsigned i = 0; i < 128; ++i) multi += "\xC3\xA9";
	check(multi, true, "256_byte_valid_unicode");
	if (multi.size() != 256) ++failures;
	if (failures) return 1;
	std::cout << "PASS: native VR keyboard UTF-8 policy cases (not runtime keyboard qualification)\n";
	return 0;
}
