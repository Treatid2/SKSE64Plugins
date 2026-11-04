// Controlled tests of the actual production relink/clone policies, not engine ABI.
#include "HeadExportPolicy.h"
#include "ExportSkinClonePolicy.h"
#include <iostream>
#include <memory>
#include <stdexcept>

int main()
{
    unsigned checks = 0;
    const auto check = [&](bool value, const char* why) {
        ++checks;
        if (!value) throw std::runtime_error(why);
    };
    try {
        unsigned descriptor = 11;
        auto oldSkin = std::make_shared<int>(1);
        auto target = oldSkin;
        auto source = std::make_shared<int>(2);
        unsigned calls = 0;
        auto refuse = [&](int*) { ++calls; return std::shared_ptr<int>{}; };
        check(!HeadExportPolicy::TryRelinkSkin(descriptor, target, 22u, source, refuse), "failed clone accepted");
        check(descriptor == 11 && target == oldSkin && calls == 1, "failed clone changed geometry");
        const std::shared_ptr<int> missing;
        check(!HeadExportPolicy::TryRelinkSkin(descriptor, target, 22u, missing, refuse), "missing skin accepted");
        check(descriptor == 11 && target == oldSkin && calls == 1, "missing skin changed geometry");
        bool threw = false;
        try {
            HeadExportPolicy::TryRelinkSkin(descriptor, target, 22u, source,
                [](int*) -> std::shared_ptr<int> { throw std::runtime_error("clone"); });
        } catch (const std::runtime_error&) { threw = true; }
        check(threw && descriptor == 11 && target == oldSkin, "clone exception changed geometry");
        const auto aliases = [&](int* skin) {
            return SKEE::RequestIndependentExportSkin(skin, [&](int*) { return source; });
        };
        const auto references = source.use_count();
        check(!HeadExportPolicy::TryRelinkSkin(descriptor, target, 22u, source, aliases), "source alias accepted");
        check(descriptor == 11 && target == oldSkin && source.use_count() == references, "alias refusal changed state/lifetime");
        auto replacement = std::make_shared<int>(3);
        const auto independent = [&](int* skin) {
            return SKEE::RequestIndependentExportSkin(skin, [&](int* input) {
                check(input == source.get(), "wrong source");
                return replacement;
            });
        };
        check(HeadExportPolicy::TryRelinkSkin(descriptor, target, 22u, source, independent), "valid clone refused");
        check(descriptor == 22 && target == replacement && *source == 2, "replacement pair not committed");
        std::cout << checks << " overlay relink checks passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
