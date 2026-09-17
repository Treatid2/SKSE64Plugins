// SPDX-License-Identifier: GPL-3.0-or-later
#include "RaceSexMenuSwfPatch.h"
#if defined(ENABLE_SKYRIM_VR)
#include "SwfBytePatch.h"
#include <RE/B/BSResourceNiBinaryStream.h>
#include <RE/B/BSScaleformManager.h>
#include <RE/G/GFxLoader.h>
#include <RE/G/GMemory.h>
#include <RE/G/GFxState.h>
#include <RE/Offsets_VTABLE.h>
#include <REL/Relocation.h>
#include <SKSE/SKSE.h>
#include <atomic>
#include <cstring>
#include <mutex>
#include <stdexcept>
#include <string_view>

namespace SKEE::RaceSexMenuSwfPatch
{
    namespace
    {
        constexpr char kMoviePath[] = "Interface/VR/RaceSex_menu.swf";
        constexpr char kPatchPath[] = "SKSE/Plugins/RaceMenuVR2/racesex-menu.rmp";
        // Skyrim VR 1.4.15, statically qualified against retained memory dump.
        // OpenFile (slot 1) dispatches through OpenFileEx (slot 3). All other
        // opener methods and all other URLs continue through the engine unchanged.
        constexpr std::uintptr_t kOpenFileExRva = 0xF20B40;
        constexpr std::uintptr_t kMemoryFileCtorRva = 0xF217F0;
        constexpr std::size_t kMemoryFileSize = 0x30;
        using OpenFileEx = void* (*)(void*, const char*, void*, int, int);
        using MemoryFileCtor = void* (*)(void*, const char*, const std::uint8_t*, int);
        std::atomic<OpenFileEx> original{};
        std::once_flag preparation;
        bool ready{};
        std::atomic<bool> published{};
        // GMemoryFile borrows its backing bytes. Retain the immutable movie for
        // the process lifetime, including asynchronous loads and menu reopens.
        SwfBytePatch::Bytes movie;
        std::atomic<std::uint64_t> served{};

        bool IsMovie(const char* path)
        {
            if (!path) return false;
            constexpr std::string_view expected = "interface/vr/racesex_menu.swf";
            for (std::size_t i=0; i<expected.size(); ++i) {
                auto ch = static_cast<unsigned char>(path[i]);
                if (!ch) return false;
                if (ch == '\\') ch = '/';
                if (ch >= 'A' && ch <= 'Z') ch += 'a'-'A';
                if (ch != expected[i]) return false;
            }
            return path[expected.size()] == '\0';
        }

        SwfBytePatch::Bytes ReadResource(const char* path)
        {
            RE::BSResourceNiBinaryStream stream(path);
            if (!stream.good() || !stream.stream) throw std::runtime_error(std::string("Missing resource: ")+path);
            const auto size = stream.stream->totalSize;
            if (!size || size > SwfBytePatch::kLimit) throw std::runtime_error("Resource size exceeds patch limit");
            SwfBytePatch::Bytes bytes(size);
            if (!stream.read(bytes.data(), size)) throw std::runtime_error(std::string("Short resource read: ")+path);
            return bytes;
        }

        void* Open(void* self, const char* path, void* log, int flags, int mode)
        {
            if (!IsMovie(path)) return original.load(std::memory_order_acquire)(self,path,log,flags,mode);
            if (!published.load(std::memory_order_acquire)) return nullptr;
            // The engine constructor initializes the GString path, refcount=1,
            // borrowed buffer at +18, signed length at +20, cursor at +24 and
            // valid flag at +28. Its virtual destructor frees the same GFx heap.
            auto* storage = RE::GMemory::Alloc(kMemoryFileSize);
            if (!storage) {
                SKSE::log::error("RaceMenu runtime SWF patch: GFx memory-file allocation failed");
                return nullptr;
            }
            REL::Relocation<MemoryFileCtor> ctor{REL::Offset(kMemoryFileCtorRva)};
            auto* file = ctor(storage,path,movie.data(),static_cast<int>(movie.size()));
            const auto ordinal = served.fetch_add(1, std::memory_order_relaxed)+1;
            SKSE::log::info("RaceMenu runtime SWF patch: served verified FWS, bytes={}, open={}",movie.size(),ordinal);
            return file;
        }

        void Initialize(RE::BSScaleformManager* manager)
        {
            if (!manager || !manager->loader) throw std::runtime_error("Scaleform loader unavailable");
            auto opener = manager->loader->GetState(RE::GFxState::StateType::kFileOpener);
            REL::Relocation<std::uintptr_t> table{RE::VTABLE_BSScaleformFileOpener[0]};
            const auto expectedOpen = REL::Module::get().base()+kOpenFileExRva;
            if (!opener || *reinterpret_cast<std::uintptr_t*>(opener.get()) != table.address() ||
                reinterpret_cast<std::uintptr_t*>(table.address())[3] != expectedOpen)
                throw std::runtime_error("Unqualified/modified Scaleform file opener; refusing to replace another mod's hook");
            constexpr std::uint8_t ctorPrefix[]{0x48,0x89,0x4c,0x24,0x08,0x57,0x48,0x83,0xec,0x30};
            constexpr std::uint8_t openPrefix[]{0x4c,0x8b,0xdc,0x57,0x41,0x56,0x41,0x57,0x48,0x83,0xec,0x70};
            if (std::memcmp(reinterpret_cast<void*>(REL::Module::get().base()+kMemoryFileCtorRva),ctorPrefix,sizeof(ctorPrefix)) ||
                std::memcmp(reinterpret_cast<void*>(expectedOpen),openPrefix,sizeof(openPrefix)))
                throw std::runtime_error("Runtime SWF adapter code signature mismatch");
            const auto source = ReadResource(kMoviePath);
            const auto patch = ReadResource(kPatchPath);
            movie = SwfBytePatch::ApplyRelease(source,patch);
            // Publish only fully verified immutable bytes. Set the forwarder
            // before patching the vtable so concurrent unrelated opens are safe.
            original.store(reinterpret_cast<OpenFileEx>(expectedOpen),std::memory_order_release);
            published.store(true,std::memory_order_release);
            table.write_vfunc(3,Open);
            ready = true;
            SKSE::log::info("RaceMenu runtime SWF patch: exact original verified; reconstructed {} bytes in memory",movie.size());
        }
    }

    bool Prepare(RE::BSScaleformManager* manager)
    {
        std::call_once(preparation,[&] {
            try { Initialize(manager); }
            catch (const std::exception& error) {
                SKSE::log::error("RaceMenu runtime SWF patch FAILED: {}. Install original RaceMenu SE 0.4.20.0 and matching add-on; disable loose VR layout/generated SWFs. Original files were not changed.",error.what());
            }
        });
        return ready;
    }

    bool HasServedMovie() { return served.load(std::memory_order_relaxed) != 0; }
}
#endif
