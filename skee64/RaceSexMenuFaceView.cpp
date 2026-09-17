#include "pch.h"
#include "RaceSexMenuFaceView.h"
#include <atomic>
#include <cmath>

namespace SKEE::FaceView
{
    namespace
    {
        bool enabled{};
        float distance{45};
        float eyeHeight{5};
        std::atomic<unsigned> view{0};
        std::atomic<bool> queued{false};
        std::atomic<std::uint64_t> generation{0};
        RE::NiPointer<RE::NiNode> room;
        RE::NiPoint3 offset{}, lastLocal{}, direction{}, anchorHmd{};
        RE::NiPoint3 anchorHead{};
        RE::NiAVObject* headIdentity{}; // identity only, never dereferenced
        RE::TESRace* raceIdentity{};
        float avatarScale{};
        std::atomic<unsigned> anchorRefreshes{0}, originReplacements{0}, updates{0};
        bool Finite(const RE::NiPoint3& p) { return std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z); }
        void RemoveOffset()
        {
            // Do not overwrite a replacement origin written by another owner.
            if (room && room->local.translate.GetSquaredDistance(lastLocal) < 0.0001F) {
                room->local.translate -= offset;
                RE::NiUpdateData update{0, RE::NiUpdateData::Flag::kDirty};
                room->Update(update);
            }
            room.reset(); offset = {};
        }
        bool Update(bool entering)
        {
#if defined(ENABLE_SKYRIM_VR)
            auto* player = RE::PlayerCharacter::GetSingleton();
            auto* nodes = player ? player->GetVRNodeData() : nullptr;
            auto* root = player ? player->Get3D(false) : nullptr;
            auto* head = root ? root->GetObjectByName(RE::BSFixedString("NPC Head [Head]")) : nullptr;
            auto* origin = nodes ? nodes->RoomNode.get() : nullptr;
            auto* hmd = nodes ? nodes->HmdNode.get() : nullptr;
            if (!head || !origin || !origin->parent || !hmd || !Finite(head->world.translate) || !Finite(hmd->world.translate)) return false;
            // A tracking-origin translation must never move the avatar itself.
            for (auto* ancestor = head->parent; ancestor; ancestor = ancestor->parent) if (ancestor == origin) return false;
            bool tracked = false;
            for (auto* ancestor = hmd->parent; ancestor; ancestor = ancestor->parent) if (ancestor == origin) tracked = true;
            if (!tracked || origin->parent->world.scale <= 0) return false;
            if (room && room.get() != origin) RemoveOffset();
            if (room && origin->local.translate.GetSquaredDistance(lastLocal) >= 0.0001F) {
                if (originReplacements.fetch_add(1)==0) SKSE::log::warn("RaceMenu face-view tracking origin replaced between updates; possible competing owner");
            }
            auto baseLocal = origin->local.translate;
            auto normalHmd = hmd->world.translate;
            if (room && baseLocal.GetSquaredDistance(lastLocal) < 0.0001F) {
                baseLocal -= offset;
                normalHmd -= origin->parent->world.rotate * (offset * origin->parent->world.scale);
            }
            if (entering) {
                direction = normalHmd-head->world.translate;
                direction.z = 0; // frame the face level, not along the old downward sight line
                if (direction.Unitize() < 1 || !Finite(direction)) return false;
                anchorHmd = normalHmd;
            }
            // Do not chase idle head animation at the 500 ms polling cadence.
            // Refresh the framing anchor only for structural avatar changes.
            if (entering || headIdentity != head || raceIdentity != player->GetRace() ||
                std::abs(avatarScale-root->world.scale) > 0.0001F) {
                anchorHead = head->world.translate;
                anchorHead.z += eyeHeight;
                headIdentity = head; raceIdentity = player->GetRace(); avatarScale = root->world.scale;
                anchorRefreshes.fetch_add(1);
            }
            // Fixed normal-view anchor, NOT the current tracked HMD position:
            // subtracting that every pulse would cancel real head movement.
            auto delta = anchorHead + direction*distance - anchorHmd;
            if (!Finite(delta) || delta.Length() > 2000) return false;
            offset = origin->parent->world.rotate.Transpose()*delta/origin->parent->world.scale;
            room.reset(origin);
            lastLocal = baseLocal+offset;
            origin->local.translate = lastLocal;
            RE::NiUpdateData update{0, RE::NiUpdateData::Flag::kDirty};
            origin->Update(update);
            updates.fetch_add(1);
            return true;
#else
            return false;
#endif
        }
        class Handler final : public RE::GFxFunctionHandler
        {
            void Call(Params& args) override
            {
                const auto operation = reinterpret_cast<std::uintptr_t>(args.userData);
                if (operation == 3 && args.retVal) {
                    args.movie->CreateObject(args.retVal);
                    args.retVal->SetMember("anchorRefreshes",RE::GFxValue{static_cast<double>(anchorRefreshes.load())});
                    args.retVal->SetMember("originReplacements",RE::GFxValue{static_cast<double>(originReplacements.load())});
                    args.retVal->SetMember("updates",RE::GFxValue{static_cast<double>(updates.load())});
                    return;
                }
                if (operation == 0 && args.retVal) args.retVal->SetNumber(Current());
                else if (operation == 1 && args.argCount == 1 && args.args[0].IsNumber()) {
                    const auto requested = args.args[0].GetNumber();
                    const auto accepted = (requested == 0 || requested == 1) && Request(static_cast<unsigned>(requested));
                    if (args.retVal) args.retVal->SetBoolean(accepted);
                } else if (operation == 2 && view.load() == 1) Request(1);
            }
        };
    }
    void Configure(bool value, float requestedDistance, float requestedHeight)
    { enabled = value; distance = std::isfinite(requestedDistance) && requestedDistance >= 25 && requestedDistance <= 150 ? requestedDistance : 45;
      eyeHeight = std::isfinite(requestedHeight) && requestedHeight >= -20 && requestedHeight <= 30 ? requestedHeight : 5; }
    bool Supported() { return enabled && REL::Module::IsVR(); }
    unsigned Current() { return view.load(); } // 0 normal, 1 face, 2 unavailable
    void Restore() { generation.fetch_add(1); RemoveOffset(); view.store(0); }
    bool Request(unsigned requested)
    {
        if (!Supported() || requested > 1) return false;
        auto* tasks = SKSE::GetTaskInterface();
        if (!tasks || queued.exchange(true)) return false;
        const auto session = generation.load();
        try {
            tasks->AddTask([requested, session] {
                if (generation.load() == session) {
                    auto* ui = RE::UI::GetSingleton();
                    if (ui && ui->IsMenuOpen(RE::RaceSexMenu::MENU_NAME)) {
                        if (!requested) { RemoveOffset(); view.store(0); }
                        else if (Update(view.load() != 1)) view.store(1);
                        else { RemoveOffset(); view.store(2); SKSE::log::warn("RaceMenu face view unavailable: independent head/tracking origin not resolved"); }
                    }
                }
                queued.store(false);
            });
        } catch (...) { queued.store(false); return false; }
        return true;
    }
    void Register(RE::GFxMovie* movie, RE::GFxValue* root)
    {
        if (!Supported()) return;
        static RE::GPtr<Handler> handler{new Handler{}};
        constexpr const char* names[]{"GetMenuView", "SetMenuView", "UpdateMenuView", "GetFaceViewDiagnostics"};
        for (std::uintptr_t i = 0; i < std::size(names); ++i) {
            RE::GFxValue function;
            movie->CreateFunction(&function, handler.get(), reinterpret_cast<void*>(i));
            root->SetMember(names[i], function);
        }
    }
}
