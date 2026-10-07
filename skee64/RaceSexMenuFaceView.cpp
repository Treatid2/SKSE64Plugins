#include "pch.h"
#include "RaceSexMenuFaceView.h"
#include "RaceSexCameraPolicy.h"
#include <atomic>
#include <cmath>
#include <array>
#include <mutex>
#include <sstream>
#include <nlohmann/json.hpp>

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
        RE::NiPoint3 manualWorld{};
        RE::NiPoint3 anchorHead{};
        RE::NiAVObject* headIdentity{}; // identity only, never dereferenced
        RE::TESRace* raceIdentity{};
        float avatarScale{};
        std::atomic<unsigned> anchorRefreshes{0}, originReplacements{0}, updates{0};
        std::atomic<unsigned> cameraReads{0}, cameraMoves{0}, cameraRejected{0}, cameraCancelled{0};
        RE::NiPointer<RE::NiNode> yawRoom, yawParent;
        RE::NiPointer<RE::NiAVObject> yawHmd, yawAvatar, yawMenu, yawQuad;
        RE::NiTransform yawOriginal{}, yawLast{}, yawParentWorld{};
        RE::NiPoint3 yawCompensation{};
        std::atomic<unsigned> yawState{0}; // 0 neutral, 1 applied, 2 rejected, 3 cancelled
        std::atomic<float> yawDegrees{0};
        std::atomic<bool> yawQueued{false};
        // Inspection is an explicit, bounded main-thread capture. GFx readers
        // only serialize copied values: never dereference live scene nodes.
        struct InspectionNode
        {
            bool present{}, localValid{}, worldValid{}, ancestryComplete{}, sharesOrigin{};
            std::string identity, parentIdentity, name;
            RE::NiTransform local{}, world{};
            std::array<std::string, 16> ancestors{};
            unsigned ancestorCount{};
        };
        struct Inspection
        {
            RE::GFxMovie* movieIdentity{}; // comparison only, never dereferenced
            std::uint64_t session{}, serial{};
            const char* state{"not-captured"};
            std::uint32_t raceID{}, cellID{};
            unsigned menuView{};
            float viewYaw{};
            bool actorAngleValid{}, actorPositionValid{};
            RE::NiPoint3 actorAngle{}, actorPosition{};
            std::array<InspectionNode, 10> nodes{};
        };
        constexpr const char* inspectionRoles[]{"avatar", "avatarParent", "head", "pelvis", "tail",
            "trackingOrigin", "trackingParent", "headset", "menu", "menuQuad"};
        static_assert(std::size(inspectionRoles) == 10);
        std::mutex inspectionMutex;
        Inspection inspection;
        std::atomic<bool> inspectionQueued{false};
        std::atomic<std::uint64_t> inspectionSerial{0};
        std::string NodeIdentity(const RE::NiAVObject* node)
        {
            if (!node) return {};
            std::ostringstream value;
            value << "0x" << std::hex << reinterpret_cast<std::uintptr_t>(node);
            return value.str(); // Never round a 64-bit pointer through a GFx double.
        }
        InspectionNode CaptureNode(RE::NiAVObject* node, const RE::NiNode* origin)
        {
            InspectionNode result;
            if (!node) return result;
            result.present = true;
            result.identity = NodeIdentity(node);
            result.parentIdentity = NodeIdentity(node->parent);
            if (const auto* name = node->name.c_str()) result.name = std::string(name).substr(0, 128);
            result.local = node->local; result.world = node->world;
            result.localValid = CameraPolicy::Valid(result.local);
            result.worldValid = CameraPolicy::Valid(result.world);
            auto* ancestor = node;
            while (ancestor && result.ancestorCount < result.ancestors.size()) {
                result.ancestors[result.ancestorCount++] = NodeIdentity(ancestor);
                if (origin && ancestor == origin) result.sharesOrigin = true;
                ancestor = ancestor->parent;
            }
            // A truncated chain is unknown, not evidence of disjoint ownership.
            result.ancestryComplete = !ancestor;
            return result;
        }
        bool RequestInspection(RE::GFxMovie* identity)
        {
            auto* tasks = SKSE::GetTaskInterface();
            if (!Supported() || !identity || !tasks || inspectionQueued.exchange(true)) return false;
            const auto session = generation.load();
            const auto serial = inspectionSerial.fetch_add(1) + 1;
            {
                std::lock_guard lock(inspectionMutex);
                inspection = {}; inspection.movieIdentity = identity;
                inspection.session = session; inspection.serial = serial;
                inspection.state = "queued";
            }
            try {
                tasks->AddTask([identity, session, serial] {
                    Inspection next;
                    next.movieIdentity = identity;
                    next.session = session; next.serial = serial; next.state = "unavailable";
                    try {
                    auto* ui = RE::UI::GetSingleton();
                    auto menu = ui ? ui->GetMenu<RE::RaceSexMenu>() : RE::GPtr<RE::RaceSexMenu>{};
                    if (generation.load() != session || !menu || menu->uiMovie.get() != identity ||
                        !ui->IsMenuOpen(RE::RaceSexMenu::MENU_NAME)) next.state = "cancelled";
#if defined(ENABLE_SKYRIM_VR)
                    else if (REL::Module::IsVR()) {
                        auto* player = RE::PlayerCharacter::GetSingleton();
                        auto* vr = player ? player->GetVRNodeData() : nullptr;
                        auto* avatar = player ? player->Get3D(false) : nullptr;
                        if (player && vr && avatar) {
                            next.state = "captured";
                            if (auto* race = player->GetRace()) next.raceID = race->GetFormID();
                            if (auto* cell = player->GetParentCell()) next.cellID = cell->GetFormID();
                            next.actorAngle = player->GetAngle(); next.actorPosition = player->GetPosition();
                            next.actorAngleValid = CameraPolicy::Finite(next.actorAngle);
                            next.actorPositionValid = CameraPolicy::Finite(next.actorPosition);
                            next.menuView = view.load(); next.viewYaw = yawDegrees.load();
                            auto* origin = vr->RoomNode.get();
                            const std::array<RE::NiAVObject*, 10> nodes{avatar, avatar->parent,
                                avatar->GetObjectByName(RE::BSFixedString("NPC Head [Head]")),
                                avatar->GetObjectByName(RE::BSFixedString("NPC Pelvis [Pelv]")),
                                avatar->GetObjectByName(RE::BSFixedString("NPC Tail [Tail]")),
                                origin, origin ? origin->parent : nullptr, vr->HmdNode.get(),
                                vr->uiNode.get(), vr->InWorldUIQuadGeo.get()};
                            for (std::size_t i = 0; i < nodes.size(); ++i) next.nodes[i] = CaptureNode(nodes[i], origin);
                        }
                    }
#endif
                    } catch (...) { next.state = "capture-failed"; }
                    {
                        std::lock_guard lock(inspectionMutex);
                        // Menu close invalidates both a completed snapshot and
                        // an in-flight capture. Do not republish an old session.
                        if (generation.load() == session && inspection.serial == serial) inspection = std::move(next);
                    }
                    inspectionQueued.store(false);
                });
            } catch (...) {
                std::lock_guard lock(inspectionMutex);
                if (inspection.serial == serial) inspection.state = "dispatch-failed";
                inspectionQueued.store(false); return false;
            }
            return true;
        }
        void WritePoint(RE::GFxMovie* movie, RE::GFxValue& target, const char* key, const RE::NiPoint3& point)
        {
            RE::GFxValue value; movie->CreateArray(&value);
            value.PushBack(RE::GFxValue{static_cast<double>(point.x)});
            value.PushBack(RE::GFxValue{static_cast<double>(point.y)});
            value.PushBack(RE::GFxValue{static_cast<double>(point.z)});
            target.SetMember(key, value);
        }
        void WriteTransform(RE::GFxMovie* movie, RE::GFxValue& target, const char* key, const RE::NiTransform& transform)
        {
            RE::GFxValue value, matrix; movie->CreateObject(&value); movie->CreateArray(&matrix);
            WritePoint(movie, value, "position", transform.translate);
            value.SetMember("scale", RE::GFxValue{static_cast<double>(transform.scale)});
            for (unsigned i = 0; i < 3; ++i) for (unsigned j = 0; j < 3; ++j)
                matrix.PushBack(RE::GFxValue{static_cast<double>(transform.rotate.entry[i][j])});
            value.SetMember("rotationRowMajor", matrix); target.SetMember(key, value);
        }
        Inspection CopyInspection(RE::GFxMovie* movie)
        {
            Inspection snapshot;
            {
                std::lock_guard lock(inspectionMutex);
                snapshot = inspection;
            }
            if (snapshot.movieIdentity && snapshot.movieIdentity != movie) snapshot.state = "wrong-movie";
            if (snapshot.serial && snapshot.session != generation.load()) snapshot.state = "expired";
            return snapshot;
        }
        void ReadInspection(RE::GFxMovie* movie, RE::GFxValue* result)
        {
            const auto snapshot = CopyInspection(movie);
            movie->CreateObject(result);
            result->SetMember("schemaVersion", RE::GFxValue{1.0});
            result->SetMember("state", RE::GFxValue{snapshot.state});
            const auto serial = std::to_string(snapshot.serial), session = std::to_string(snapshot.session);
            result->SetMember("serial", RE::GFxValue{serial.c_str()});
            result->SetMember("session", RE::GFxValue{session.c_str()});
            result->SetMember("queued", RE::GFxValue{inspectionQueued.load()});
            if (std::string_view(snapshot.state) != "captured") return;
            result->SetMember("raceFormID", RE::GFxValue{static_cast<double>(snapshot.raceID)});
            result->SetMember("cellFormID", RE::GFxValue{static_cast<double>(snapshot.cellID)});
            result->SetMember("menuView", RE::GFxValue{static_cast<double>(snapshot.menuView)});
            result->SetMember("viewYawDegrees", RE::GFxValue{static_cast<double>(snapshot.viewYaw)});
            result->SetMember("actorAngleValid", RE::GFxValue{snapshot.actorAngleValid});
            result->SetMember("actorPositionValid", RE::GFxValue{snapshot.actorPositionValid});
            if (snapshot.actorAngleValid) WritePoint(movie, *result, "actorAngleRadians", snapshot.actorAngle);
            if (snapshot.actorPositionValid) WritePoint(movie, *result, "actorPosition", snapshot.actorPosition);
            RE::GFxValue nodes; movie->CreateArray(&nodes);
            for (std::size_t i = 0; i < snapshot.nodes.size(); ++i) {
                const auto& node = snapshot.nodes[i];
                RE::GFxValue value; movie->CreateObject(&value);
                value.SetMember("role", RE::GFxValue{inspectionRoles[i]});
                value.SetMember("present", RE::GFxValue{node.present});
                if (node.present) {
                    value.SetMember("identity", RE::GFxValue{node.identity.c_str()});
                    value.SetMember("parentIdentity", RE::GFxValue{node.parentIdentity.c_str()});
                    value.SetMember("name", RE::GFxValue{node.name.c_str()});
                    value.SetMember("localValid", RE::GFxValue{node.localValid});
                    value.SetMember("worldValid", RE::GFxValue{node.worldValid});
                    if (node.localValid) WriteTransform(movie, value, "local", node.local);
                    if (node.worldValid) WriteTransform(movie, value, "world", node.world);
                    value.SetMember("ancestryComplete", RE::GFxValue{node.ancestryComplete});
                    value.SetMember("sharesTrackingOrigin", RE::GFxValue{node.sharesOrigin});
                    RE::GFxValue ancestors; movie->CreateArray(&ancestors);
                    for (unsigned j = 0; j < node.ancestorCount; ++j) ancestors.PushBack(RE::GFxValue{node.ancestors[j].c_str()});
                    value.SetMember("ancestorsSelfFirst", ancestors);
                }
                nodes.PushBack(value);
            }
            result->SetMember("nodes", nodes);
        }
        nlohmann::json InspectionPoint(const RE::NiPoint3& point)
        { return nlohmann::json::array({point.x, point.y, point.z}); }
        nlohmann::json InspectionTransform(const RE::NiTransform& transform)
        {
            auto matrix = nlohmann::json::array();
            for (unsigned i = 0; i < 3; ++i) for (unsigned j = 0; j < 3; ++j)
                matrix.push_back(transform.rotate.entry[i][j]);
            return {{"position", InspectionPoint(transform.translate)}, {"scale", transform.scale},
                {"rotationRowMajor", std::move(matrix)}};
        }
        std::string InspectionJSON(const Inspection& snapshot)
        {
            nlohmann::json result{{"schemaVersion", 1}, {"state", snapshot.state},
                {"serial", std::to_string(snapshot.serial)}, {"session", std::to_string(snapshot.session)},
                {"queued", inspectionQueued.load()}};
            if (std::string_view(snapshot.state) == "captured") {
                result["raceFormID"] = snapshot.raceID; result["cellFormID"] = snapshot.cellID;
                result["menuView"] = snapshot.menuView; result["viewYawDegrees"] = snapshot.viewYaw;
                result["actorAngleValid"] = snapshot.actorAngleValid;
                result["actorPositionValid"] = snapshot.actorPositionValid;
                if (snapshot.actorAngleValid) result["actorAngleRadians"] = InspectionPoint(snapshot.actorAngle);
                if (snapshot.actorPositionValid) result["actorPosition"] = InspectionPoint(snapshot.actorPosition);
                auto nodes = nlohmann::json::array();
                for (std::size_t i = 0; i < snapshot.nodes.size(); ++i) {
                    const auto& node = snapshot.nodes[i];
                    nlohmann::json value{{"role", inspectionRoles[i]}, {"present", node.present}};
                    if (node.present) {
                        value["identity"] = node.identity; value["parentIdentity"] = node.parentIdentity;
                        value["name"] = node.name;
                        value["localValid"] = node.localValid; value["worldValid"] = node.worldValid;
                        if (node.localValid) value["local"] = InspectionTransform(node.local);
                        if (node.worldValid) value["world"] = InspectionTransform(node.world);
                        value["ancestryComplete"] = node.ancestryComplete;
                        value["sharesTrackingOrigin"] = node.sharesOrigin;
                        auto ancestors = nlohmann::json::array();
                        for (unsigned j = 0; j < node.ancestorCount; ++j) ancestors.push_back(node.ancestors[j]);
                        value["ancestorsSelfFirst"] = std::move(ancestors);
                    }
                    nodes.push_back(std::move(value));
                }
                result["nodes"] = std::move(nodes);
            }
            // Skeleton names may contain non-UTF-8 bytes. Escape non-ASCII and
            // replace invalid encoding rather than losing the whole observation.
            return result.dump(-1, ' ', true, nlohmann::json::error_handler_t::replace);
        }
        bool PublishInspection(RE::GFxMovie* movie)
        {
            if (!movie) return false;
            constexpr const char* failed = "{\"schemaVersion\":1,\"state\":\"readout-failed\",\"queued\":false}";
            try {
                const auto text = InspectionJSON(CopyInspection(movie));
                if (text.size() > 64 * 1024) {
                    movie->SetVariable("_root.VRCharacterInspectionJSON", RE::GFxValue{failed});
                    return false;
                }
                // UI.Invoke discards GFx returns. This explicit movie-local
                // string is readable using SKSE UI.GetString; no scene access.
                return movie->SetVariable("_root.VRCharacterInspectionJSON", RE::GFxValue{text.c_str()});
            } catch (...) {
                movie->SetVariable("_root.VRCharacterInspectionJSON", RE::GFxValue{failed});
                return false;
            }
        }
        bool OwnYaw()
        { return yawRoom && yawRoom->parent==yawParent.get() && CameraPolicy::Same(yawRoom->local,yawLast) &&
            CameraPolicy::Same(yawParent->world,yawParentWorld); }
        void NoteYawTranslation()
        { if (yawRoom && yawRoom.get()==room.get()) yawLast.translate=room->local.translate; }
        bool RemoveYaw()
        {
            const bool owned=OwnYaw();
            if (owned) {
                yawRoom->local.rotate=yawOriginal.rotate;
                yawRoom->local.translate-=yawCompensation;
                if (room.get()==yawRoom.get()) lastLocal=room->local.translate;
                RE::NiUpdateData update{0,RE::NiUpdateData::Flag::kDirty}; yawRoom->Update(update);
            }
            yawRoom.reset(); yawParent.reset(); yawHmd.reset(); yawAvatar.reset(); yawMenu.reset(); yawQuad.reset();
            yawCompensation={}; yawDegrees.store(0);
            yawState.store(owned ? 0 : 2);
            return owned;
        }
        bool Finite(const RE::NiPoint3& p) { return std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z); }
        void RemoveOffset()
        {
            // Do not overwrite a replacement origin written by another owner.
            if (room && (!yawRoom || OwnYaw()) && room->local.translate.GetSquaredDistance(lastLocal) < 0.0001F) {
                room->local.translate -= offset;
                NoteYawTranslation();
                RE::NiUpdateData update{0, RE::NiUpdateData::Flag::kDirty};
                room->Update(update);
            }
            room.reset(); offset = {}; manualWorld = {};
        }
        struct CameraNodes
        {
            RE::NiPointer<RE::NiNode> origin;
            RE::NiPointer<RE::NiAVObject> hmd, avatar;
            RE::NiPointer<RE::NiNode> originParent, avatarParent;
            RE::NiTransform frame;
        };
        bool ResolveCamera(CameraNodes& result)
        {
#if defined(ENABLE_SKYRIM_VR)
            if (!REL::Module::IsVR()) return false;
            auto* ui = RE::UI::GetSingleton();
            if (!ui || !ui->IsMenuOpen(RE::RaceSexMenu::MENU_NAME)) return false;
            auto* player = RE::PlayerCharacter::GetSingleton();
            auto* nodes = player ? player->GetVRNodeData() : nullptr;
            auto* avatar = player ? player->Get3D(false) : nullptr;
            auto* origin = nodes ? nodes->RoomNode.get() : nullptr;
            auto* hmd = nodes ? nodes->HmdNode.get() : nullptr;
            if (!avatar || !origin || !origin->parent || !hmd) return false;
            // Moving this origin must move the headset, NOT the edited avatar.
            for (auto* p = avatar; p; p = p->parent) if (p == origin) return false;
            bool tracked = false;
            for (auto* p = hmd->parent; p; p = p->parent) if (p == origin) tracked = true;
            const RE::NiTransform frame = avatar->parent ? avatar->parent->world : RE::NiTransform{};
            if (!tracked || !CameraPolicy::Valid(frame) || !CameraPolicy::Valid(origin->parent->world) ||
                !CameraPolicy::Valid(hmd->world) || !Finite(origin->local.translate)) return false;
            result.origin.reset(origin); result.hmd.reset(hmd); result.avatar.reset(avatar); result.frame = frame;
            result.originParent.reset(origin->parent); result.avatarParent.reset(avatar->parent);
            return true;
#else
            return false;
#endif
        }
        bool MoveCamera(const RE::NiPoint3& delta, const CameraNodes& expected)
        {
            CameraNodes current;
            if (!CameraPolicy::SafeDelta(delta) || !ResolveCamera(current) ||
                current.origin.get() != expected.origin.get() || current.hmd.get() != expected.hmd.get() ||
                current.avatar.get() != expected.avatar.get() || current.originParent.get() != expected.originParent.get() ||
                current.avatarParent.get() != expected.avatarParent.get()) return false;
            auto* origin = current.origin.get();
            if (yawRoom && !OwnYaw()) return false;
            // A replaced origin belongs to its new owner. Do not reapply an old
            // offset or overwrite that owner's position with a stale snapshot.
            if (room && (room.get() != origin || origin->local.translate.GetSquaredDistance(lastLocal) >= 0.0001F)) return false;
            const auto localDelta = CameraPolicy::LocalDelta(origin->parent->world, delta);
            const auto nextManual = manualWorld + delta;
            if (!CameraPolicy::SafeDelta(nextManual) || !Finite(localDelta) ||
                !Finite(origin->local.translate + localDelta)) return false;
            room = current.origin;
            offset += localDelta; manualWorld = nextManual;
            lastLocal = origin->local.translate + localDelta;
            origin->local.translate = lastLocal;
            NoteYawTranslation();
            RE::NiUpdateData update{0, RE::NiUpdateData::Flag::kDirty};
            origin->Update(update);
            cameraMoves.fetch_add(1);
            return true;
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
            if (yawRoom && (!OwnYaw() || yawRoom.get()!=origin)) return false;
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
            if (yawRoom) baseLocal-=yawCompensation;
            auto normalHmd = hmd->world.translate;
            if (room && origin->local.translate.GetSquaredDistance(lastLocal) < 0.0001F) {
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
            auto delta = anchorHead + direction*distance - anchorHmd + manualWorld;
            if (!Finite(delta) || delta.Length() > 2000) return false;
            offset = origin->parent->world.rotate.Transpose()*delta/origin->parent->world.scale;
            room.reset(origin);
            lastLocal = baseLocal+offset+yawCompensation;
            origin->local.translate = lastLocal;
            NoteYawTranslation();
            RE::NiUpdateData update{0, RE::NiUpdateData::Flag::kDirty};
            origin->Update(update);
            updates.fetch_add(1);
            return true;
#else
            return false;
#endif
        }
        bool ApplyYaw(float requested)
        {
#if defined(ENABLE_SKYRIM_VR)
            CameraNodes nodes;
            if (!std::isfinite(requested) || std::abs(requested)>60 || !ResolveCamera(nodes)) return false;
            auto* vr=RE::PlayerCharacter::GetSingleton()->GetVRNodeData();
            auto* menu=vr ? vr->uiNode.get() : nullptr;
            auto* quad=vr ? vr->InWorldUIQuadGeo.get() : nullptr;
            const auto contains=[](RE::NiAVObject* object,RE::NiNode* ancestor) {
                for (auto* p=object;p;p=p->parent) if (p==ancestor) return true;
                return false;
            };
            if (!menu || !quad || contains(menu,nodes.origin.get()) || contains(quad,nodes.origin.get()) ||
                !CameraPolicy::Valid(nodes.origin->local)) return false;
            if (room && (room.get()!=nodes.origin.get() || nodes.origin->local.translate.GetSquaredDistance(lastLocal)>=0.0001F)) return false;
            if (yawRoom && (!OwnYaw() || yawRoom.get()!=nodes.origin.get() || yawHmd.get()!=nodes.hmd.get() ||
                yawAvatar.get()!=nodes.avatar.get() || yawMenu.get()!=menu || yawQuad.get()!=quad)) return false;
            if (requested==0) {
                if (yawRoom) return RemoveYaw();
                yawState.store(0); return true;
            }
            const auto world=CameraPolicy::YawAroundEye(nodes.origin->world,nodes.hmd->world.translate,requested-yawDegrees.load());
            const auto& parent=nodes.originParent->world;
            auto next=nodes.origin->local;
            next.rotate=parent.rotate.Transpose()*world.rotate;
            next.translate=CameraPolicy::LocalPoint(parent,world.translate);
            const auto compensation=yawCompensation+next.translate-nodes.origin->local.translate;
            if (!CameraPolicy::Valid(world) || !CameraPolicy::Valid(next) || !CameraPolicy::SafeDelta(compensation)) return false;
            if (!yawRoom) {
                yawRoom=nodes.origin; yawParent=nodes.originParent; yawHmd=nodes.hmd; yawAvatar=nodes.avatar;
                yawMenu.reset(menu); yawQuad.reset(quad); yawOriginal=nodes.origin->local; yawParentWorld=parent;
            }
            yawCompensation=compensation; yawLast=next;
            nodes.origin->local=next;
            if (room.get()==nodes.origin.get()) lastLocal=next.translate;
            RE::NiUpdateData update{0,RE::NiUpdateData::Flag::kDirty}; nodes.origin->Update(update);
            yawDegrees.store(requested); yawState.store(1); return true;
#else
            return false;
#endif
        }
        bool RequestYaw(float requested, RE::GFxMovie* identity)
        {
            if (!Supported() || !std::isfinite(requested) || std::abs(requested)>60) return false;
            auto* tasks=SKSE::GetTaskInterface();
            if (!tasks || yawQueued.exchange(true)) return false;
            const auto session=generation.load();
            try {
                tasks->AddTask([requested,identity,session] {
                    auto* ui=RE::UI::GetSingleton();
                    auto menu=ui ? ui->GetMenu<RE::RaceSexMenu>() : RE::GPtr<RE::RaceSexMenu>{};
                    if (generation.load()!=session || !menu || menu->uiMovie.get()!=identity) yawState.store(3);
                    else if (!ApplyYaw(requested)) yawState.store(2);
                    yawQueued.store(false);
                });
            } catch (...) { yawQueued.store(false); return false; }
            return true;
        }
        class Handler final : public RE::GFxFunctionHandler
        {
            void Call(Params& args) override
            {
                const auto operation = reinterpret_cast<std::uintptr_t>(args.userData);
                if (operation == 5) {
                    const bool accepted = args.argCount == 0 && RequestInspection(args.movie);
                    if (args.retVal) args.retVal->SetBoolean(accepted);
                    return;
                }
                if (operation == 6) {
                    if (args.argCount == 0 && args.retVal) ReadInspection(args.movie, args.retVal);
                    return;
                }
                if (operation == 7) {
                    const bool published = args.argCount == 0 && PublishInspection(args.movie);
                    if (args.retVal) args.retVal->SetBoolean(published);
                    return;
                }
                if (operation == 3 && args.retVal) {
                    args.movie->CreateObject(args.retVal);
                    args.retVal->SetMember("anchorRefreshes",RE::GFxValue{static_cast<double>(anchorRefreshes.load())});
                    args.retVal->SetMember("originReplacements",RE::GFxValue{static_cast<double>(originReplacements.load())});
                    args.retVal->SetMember("updates",RE::GFxValue{static_cast<double>(updates.load())});
                    args.retVal->SetMember("cameraReads",RE::GFxValue{static_cast<double>(cameraReads.load())});
                    args.retVal->SetMember("cameraMoves",RE::GFxValue{static_cast<double>(cameraMoves.load())});
                    args.retVal->SetMember("cameraRejected",RE::GFxValue{static_cast<double>(cameraRejected.load())});
                    args.retVal->SetMember("cameraCancelled",RE::GFxValue{static_cast<double>(cameraCancelled.load())});
                    args.retVal->SetMember("yawState",RE::GFxValue{static_cast<double>(yawState.load())});
                    args.retVal->SetMember("yawDegrees",RE::GFxValue{static_cast<double>(yawDegrees.load())});
                    args.retVal->SetMember("yawQueued",RE::GFxValue{yawQueued.load()});
#if defined(ENABLE_SKYRIM_VR)
                    // Read-only topology evidence for a future bounded view-yaw
                    // control. Never assume RoomNode excludes the menu/quad.
                    CameraNodes nodes;
                    const bool resolved = ResolveCamera(nodes);
                    auto* player = resolved ? RE::PlayerCharacter::GetSingleton() : nullptr;
                    auto* vrNodes = player ? player->GetVRNodeData() : nullptr;
                    const auto contains = [](RE::NiAVObject* object, RE::NiNode* ancestor) {
                        if (!object || !ancestor) return false;
                        for (auto* p = object; p; p = p->parent) if (p == ancestor) return true;
                        return false;
                    };
                    const bool topologyReady = vrNodes && vrNodes->uiNode && vrNodes->InWorldUIQuadGeo;
                    args.retVal->SetMember("rotationTopologyAvailable",RE::GFxValue{topologyReady});
                    args.retVal->SetMember("menuSharesTrackingOrigin",RE::GFxValue{
                        topologyReady && contains(vrNodes->uiNode.get(),nodes.origin.get())});
                    args.retVal->SetMember("quadSharesTrackingOrigin",RE::GFxValue{
                        topologyReady && contains(vrNodes->InWorldUIQuadGeo.get(),nodes.origin.get())});
#endif
                    return;
                }
                if (operation == 4) {
                    const bool accepted=args.argCount==1 && args.args[0].IsNumber() &&
                        std::isfinite(args.args[0].GetNumber()) && std::abs(args.args[0].GetNumber())<=60 &&
                        RequestYaw(static_cast<float>(args.args[0].GetNumber()),args.movie);
                    if (!accepted) yawState.store(2);
                    if (args.retVal) args.retVal->SetBoolean(accepted);
                } else if (operation == 0 && args.retVal) args.retVal->SetNumber(Current());
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
    bool GetCameraTransform(RE::NiPoint3& position, RE::NiMatrix3& rotation)
    {
        CameraNodes nodes;
        if (!ResolveCamera(nodes)) return false;
        position = CameraPolicy::LocalPoint(nodes.frame, nodes.hmd->world.translate);
        rotation = nodes.frame.rotate.Transpose() * nodes.hmd->world.rotate;
        cameraReads.fetch_add(1);
        return Finite(position);
    }
    bool RequestCameraPosition(const RE::NiPoint3& position)
    {
        CameraNodes nodes;
        if (!Finite(position) || !ResolveCamera(nodes)) return false;
        const auto current = CameraPolicy::LocalPoint(nodes.frame, nodes.hmd->world.translate);
        // Capture an input delta, not an absolute future HMD pose: legitimate
        // head movement while the task is queued must remain intact.
        const auto delta = CameraPolicy::WorldDelta(nodes.frame, position-current);
        if (!CameraPolicy::SafeDelta(delta)) return false;
        auto* tasks = SKSE::GetTaskInterface();
        if (!tasks) return false;
        const auto session = generation.load();
        try {
            tasks->AddTask([delta, nodes, session] {
                if (generation.load() != session) { cameraCancelled.fetch_add(1); return; }
                if (!MoveCamera(delta, nodes) && cameraRejected.fetch_add(1) == 0)
                    SKSE::log::warn("RaceMenu VR camera move rejected: tracking origin changed or unavailable");
            });
        } catch (...) { return false; }
        return true;
    }
    void Restore()
    {
        generation.fetch_add(1);
        {
            std::lock_guard lock(inspectionMutex);
            inspection.state = "expired";
        }
        if (yawRoom && !RemoveYaw()) {
            // A competing transform owns the whole pose. Do not subtract a
            // stale face offset merely because its translation still matches.
            room.reset(); offset={}; manualWorld={};
        } else RemoveOffset();
        view.store(0);
    }
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
        // Every queued operation is bound to this exact RaceSex movie and
        // revalidates live topology. Keep the diagnostic alias for inspection.
        RE::GFxValue testYaw;
        movie->CreateFunction(&testYaw,handler.get(),reinterpret_cast<void*>(4));
        root->SetMember("SetViewYaw",testYaw);
        movie->SetVariable("_root.TestVRViewYaw",testYaw);
        constexpr const char* inspectionNames[]{"CaptureCharacterInspection", "GetCharacterInspection", "PublishCharacterInspection"};
        constexpr const char* inspectionAliases[]{"_root.CaptureVRCharacterInspection", "_root.GetVRCharacterInspection", "_root.PublishVRCharacterInspection"};
        for (std::uintptr_t i = 0; i < std::size(inspectionNames); ++i) {
            RE::GFxValue function;
            movie->CreateFunction(&function, handler.get(), reinterpret_cast<void*>(5 + i));
            root->SetMember(inspectionNames[i], function);
            movie->SetVariable(inspectionAliases[i], function);
        }
        // New movie instances must not inherit a transport copy from another
        // menu session. A read requires an explicit publish in this movie.
        movie->SetVariable("_root.VRCharacterInspectionJSON", RE::GFxValue{
            "{\"schemaVersion\":1,\"state\":\"not-published\",\"queued\":false}"});
    }
}
