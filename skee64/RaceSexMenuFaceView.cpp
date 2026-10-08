#include "pch.h"
#include "RaceSexMenuFaceView.h"
#include "RaceSexCameraPolicy.h"
#include "CharacterInspectionControls.h"
#include "MenuExtensions.h"
#include "RaceSexMenuVRInput.h"
#include "ViewDirectionTrialPolicy.h"
#include "AvatarBoundProbe.h"
#include <chrono>
#include <atomic>
#include <algorithm>
#include <cmath>
#include <array>
#include <mutex>
#include <sstream>
#include <unordered_set>
#include <vector>
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
        std::atomic<std::uint64_t> avatarRevision{0};
        std::uint64_t anchorRevision{};
        std::atomic<unsigned> anchorRefreshes{0}, originReplacements{0}, updates{0};
        std::atomic<unsigned> cameraReads{0}, cameraMoves{0}, cameraRejected{0}, cameraCancelled{0};
        RE::NiPointer<RE::NiNode> yawRoom, yawParent;
        RE::NiPointer<RE::NiAVObject> yawHmd, yawAvatar, yawMenu, yawQuad;
        RE::NiTransform yawOriginal{}, yawLast{}, yawParentWorld{};
        RE::NiPoint3 yawCompensation{};
        std::atomic<unsigned> yawState{0}; // 0 neutral, 1 applied, 2 rejected, 3 cancelled
        std::atomic<float> yawDegrees{0};
        std::atomic<unsigned> yawPending{0};
        // Inspection is an explicit, bounded main-thread capture. GFx readers
        // only serialize copied values: never dereference live scene nodes.
        struct InspectionNode
        {
            bool present{}, localValid{}, worldValid{}, ancestryComplete{}, sharesOrigin{};
            bool boundValid{}, fixedBound{};
            std::string identity, parentIdentity, name;
            RE::NiTransform local{}, world{};
            RE::NiBound worldBound{};
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
            nlohmann::json controls;
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
            result.worldBound = node->worldBound;
            result.boundValid = BoundProbe::Valid({{result.worldBound.center.x,
                result.worldBound.center.y, result.worldBound.center.z}, result.worldBound.radius});
            result.fixedBound = node->GetFlags().any(RE::NiAVObject::Flag::kFixedBound);
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
                            next.controls = CharacterInspection::CaptureDiagnostics();
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
                    value.SetMember("worldBoundValid", RE::GFxValue{node.boundValid});
                    value.SetMember("fixedBound", RE::GFxValue{node.fixedBound});
                    if (node.boundValid) {
                        RE::GFxValue bound; movie->CreateObject(&bound);
                        WritePoint(movie, bound, "center", node.worldBound.center);
                        bound.SetMember("radius", RE::GFxValue{static_cast<double>(node.worldBound.radius)});
                        value.SetMember("worldBound", bound);
                    }
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
                result["controls"] = snapshot.controls;
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
                        value["worldBoundValid"] = node.boundValid; value["fixedBound"] = node.fixedBound;
                        if (node.boundValid) value["worldBound"] = {
                            {"center", InspectionPoint(node.worldBound.center)}, {"radius", node.worldBound.radius}};
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
                std::abs(avatarScale-root->world.scale) > 0.0001F || anchorRevision != avatarRevision.load()) {
                anchorHead = head->world.translate;
                anchorHead.z += eyeHeight;
                headIdentity = head; raceIdentity = player->GetRace(); avatarScale = root->world.scale;
                anchorRevision = avatarRevision.load();
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
                yawMenu.get()!=menu || yawQuad.get()!=quad)) return false;
            // Race/sex can replace the independent avatar root without changing
            // ownership of the tracked origin. ResolveCamera already rechecks
            // that the replacement avatar is not descended from that origin.
            if (yawRoom) yawAvatar = nodes.avatar;
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
            // Preserve every absolute input (especially the final zero), rather
            // than dropping the latest slider position behind a queued task.
            if (!tasks) return false;
            yawPending.fetch_add(1);
            const auto session=generation.load();
            try {
                tasks->AddTask([requested,identity,session] {
                    try {
                        if (generation.load()!=session) yawState.store(3);
                        else if (!ApplyViewYawOnGameTask(requested, identity)) yawState.store(2);
                    } catch (...) { yawState.store(2); }
                    yawPending.fetch_sub(1);
                });
            } catch (...) { yawPending.fetch_sub(1); return false; }
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
                    args.retVal->SetMember("yawQueued",RE::GFxValue{yawPending.load() != 0});
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
    float ViewYaw() { return yawDegrees.load(); }
    bool ApplyViewYawOnGameTask(float degrees, RE::GFxMovie* identity)
    {
        auto* ui = RE::UI::GetSingleton();
        auto menu = ui ? ui->GetMenu<RE::RaceSexMenu>() : RE::GPtr<RE::RaceSexMenu>{};
        if (!Supported() || !identity || !menu || menu->uiMovie.get() != identity ||
            !ui->IsMenuOpen(RE::RaceSexMenu::MENU_NAME)) return false;
        const bool applied = ApplyYaw(degrees);
        MenuExtensions::GetInterface()->SetValue("RaceMenuVR2", "viewYaw", yawDegrees.load());
        return applied;
    }
    void RefreshAvatarAnchor() { avatarRevision.fetch_add(1); }
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

// Preview-only controls. Scene mutation is owned by the native menu update,
// never by a rendering callback, saved actor angles, or an animation controller.
namespace SKEE::CharacterInspection
{
    namespace
    {
        constexpr const char* provider = "RaceMenuVR2";
        constexpr std::size_t maxNodes = 4096, maxDepth = 64;
        using Process = RE::UI_MESSAGE_RESULTS (*)(RE::RaceSexMenu*, RE::UIMessage&);
        using SceneFunction = void (*)(RE::NiAVObject*);
        Process original{};
        bool installed{}, rejected{};
        bool unsafeTrialEnabled{}; // explicit per-menu opt-in, never persisted
        std::uint64_t unsafeTrialApplications{}, unsafeTrialRestoreConflicts{};
        RE::GFxMovie* movieIdentity{}; // identity only, accessed on menu/game tasks
        float requestedYaw{}, appliedYaw{};
        ViewDirectionTrial viewTrial;
        nlohmann::json refusal;
        bool InRdata(std::uintptr_t address, std::size_t bytes);
        std::int64_t NowMilliseconds()
        {
            return std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count();
        }
        bool Fail(const char* reason, RE::NiAVObject* object = nullptr, std::size_t depth = 0,
            nlohmann::json* diagnostic = nullptr)
        {
            // Record the first failing contract of this validation attempt.
            // Identity/virtual targets are strings, never lossy GFx numbers.
            // A read-only preflight has its own result. Never overwrite the
            // latched production refusal, even if later capture throws.
            auto& failure = diagnostic ? *diagnostic : refusal;
            failure = {{"reason", reason}, {"depth", depth}, {"objectPresent", object != nullptr}};
            if (object) {
                std::ostringstream identity; identity << "0x" << std::hex << reinterpret_cast<std::uintptr_t>(object);
                failure["identity"] = identity.str();
                failure["name"] = std::string(object->name.c_str() ? object->name.c_str() : "").substr(0, 128);
                failure["fixedBound"] = object->GetFlags().any(RE::NiAVObject::Flag::kFixedBound);
                const auto table = *reinterpret_cast<const std::uintptr_t*>(object);
                std::ostringstream address; address << "0x" << std::hex << table;
                failure["vtable"] = address.str();
                if (InRdata(table, 0x31*sizeof(std::uintptr_t))) {
                    const auto* targets = reinterpret_cast<const std::uintptr_t*>(table);
                    for (const auto slot : {0x26, 0x30}) {
                        std::ostringstream target; target << "0x" << std::hex << targets[slot];
                        failure[slot == 0x26 ? "composeTarget" : "boundsTarget"] = target.str();
                    }
                }
            }
            return false;
        }
        RE::NiPointer<RE::NiAVObject> avatar;
        RE::NiPointer<RE::NiNode> parent;
        RE::NiMatrix3 nativeRotation{}, previewRotation{};
        struct Node { RE::NiPointer<RE::NiAVObject> object; std::size_t depth; };
        struct GraphInspection { nlohmann::json failure; std::size_t validatedNodes{}; };
        struct TrialNode
        {
            RE::NiPointer<RE::NiAVObject> object;
            RE::NiAVObject* parentIdentity{};
            RE::NiTransform nativeWorld{}, trialWorld{};
            decltype(RE::NiAVObject::worldBound) nativeBound{}, trialBound{};
        };
        std::vector<TrialNode> trialNodes;

        bool SameRotation(const RE::NiMatrix3& a, const RE::NiMatrix3& b)
        {
            for (unsigned i = 0; i < 3; ++i) for (unsigned j = 0; j < 3; ++j)
                if (!std::isfinite(a.entry[i][j]) || !std::isfinite(b.entry[i][j]) ||
                    std::abs(a.entry[i][j] - b.entry[i][j]) > 0.0001F) return false;
            return true;
        }
        bool Frame(const RE::NiTransform& value)
        {
            if (!CameraPolicy::Valid(value)) return false;
            const auto& m = value.rotate.entry;
            const float determinant = m[0][0]*(m[1][1]*m[2][2]-m[1][2]*m[2][1]) -
                m[0][1]*(m[1][0]*m[2][2]-m[1][2]*m[2][0]) + m[0][2]*(m[1][0]*m[2][1]-m[1][1]*m[2][0]);
            return std::abs(determinant - 1.F) <= 0.001F;
        }
        bool InRdata(std::uintptr_t address, std::size_t bytes)
        {
            const auto data = REL::Module::get().segment(REL::Segment::Name::rdata);
            return address >= data.address() && address <= data.address()+data.size() &&
                bytes <= data.address()+data.size()-address;
        }
        using ProtectedNodes = std::array<RE::NiAVObject*, 4>;
        bool TrackingNodes(ProtectedNodes& protectedNodes, nlohmann::json* diagnostic = nullptr)
        {
#if defined(ENABLE_SKYRIM_VR)
            auto* player = RE::PlayerCharacter::GetSingleton();
            auto* vr = player ? player->GetVRNodeData() : nullptr;
            if (!REL::Module::IsVR() || !vr || !vr->RoomNode || !vr->HmdNode) return Fail("tracking-unavailable", nullptr, 0, diagnostic);
            protectedNodes = {
                vr->RoomNode.get(), vr->HmdNode.get(), vr->uiNode.get(), vr->InWorldUIQuadGeo.get()};
            return true;
#else
            return Fail("tracking-unavailable", nullptr, 0, diagnostic);
#endif
        }
        bool CollectAncestors(RE::NiAVObject* root,
            std::vector<RE::NiPointer<RE::NiNode>>& ancestors, GraphInspection* inspection = nullptr)
        {
            auto* diagnostic = inspection ? &inspection->failure : nullptr;
            if (!root) return Fail("missing-root", nullptr, 0, diagnostic);
            const auto base = REL::Module::get().base();
            std::unordered_set<RE::NiAVObject*> seen;
            for (auto* p = root->parent; p; p = p->parent) {
                const auto table = *reinterpret_cast<const std::uintptr_t*>(p);
                if (ancestors.size() >= maxDepth || p == root || !seen.insert(p).second)
                    return Fail("ancestor-cycle-or-depth", p, ancestors.size(), diagnostic);
                if (!InRdata(table, 0x31*sizeof(std::uintptr_t))) return Fail("ancestor-vtable", p, ancestors.size(), diagnostic);
                if (reinterpret_cast<const std::uintptr_t*>(table)[0x30] != base+0xC9DC10)
                    return Fail("ancestor-bounds-target", p, ancestors.size(), diagnostic);
                if (p->GetFlags().any(RE::NiAVObject::Flag::kFixedBound) && !inspection)
                    return Fail("ancestor-fixed-bound", p, ancestors.size());
                ancestors.emplace_back(p);
            }
            return true;
        }
        bool CollectObjects(RE::NiAVObject* root, std::vector<Node>& nodes,
            const std::vector<RE::NiPointer<RE::NiNode>>& ancestors,
            const ProtectedNodes& protectedNodes, GraphInspection* inspection = nullptr)
        {
            auto* diagnostic = inspection ? &inspection->failure : nullptr;
            if (!root) return Fail("missing-root", nullptr, 0, diagnostic);
            const auto base = REL::Module::get().base();
            std::unordered_set<RE::NiAVObject*> seen;
            for (const auto& ancestor : ancestors) seen.insert(ancestor.get());
            if (!seen.insert(root).second) return Fail("child-cycle-or-limit", root, 0, diagnostic);
            nodes.push_back({RE::NiPointer<RE::NiAVObject>{root}, 0});
            for (std::size_t i = 0; i < nodes.size(); ++i) {
                auto* object = nodes[i].object.get(); const auto depth = nodes[i].depth;
                // Refuse a mod-altered hierarchy containing tracked/interactive
                // nodes. Whole-avatar rotation must never rotate the headset,
                // controllers' tracking origin, menu surface or pointer quad.
                if (std::find(protectedNodes.begin(), protectedNodes.end(), object) != protectedNodes.end()) return Fail("protected-tracking-or-ui-node", object, depth, diagnostic);
                const auto table = *reinterpret_cast<const std::uintptr_t*>(object);
                if (!InRdata(table, 0x31*sizeof(std::uintptr_t))) return Fail("object-vtable", object, depth, diagnostic);
                if (!Frame(object->local)) return Fail("object-local-frame", object, depth, diagnostic);
                if (!Frame(object->world)) return Fail("object-world-frame", object, depth, diagnostic);
                const auto* functions = reinterpret_cast<const std::uintptr_t*>(table);
                // VR-only extra virtual at 0x26 is the pure transform pass.
                // Bounds functions are the independently inspected node/geometry
                // implementations, not UpdateWorldData's collision/controller path.
                if (functions[0x26] != base+0xC9BCE0 && functions[0x26] != base+0xC9DEA0) return Fail("object-compose-target", object, depth, diagnostic);
                if (functions[0x30] != base+0xC9DC10 && functions[0x30] != base+0xCB78C0 && functions[0x30] != base+0xC9C700) return Fail("object-bounds-target", object, depth, diagnostic);
                if (object->GetFlags().any(RE::NiAVObject::Flag::kFixedBound) && !inspection)
                    return Fail("object-fixed-bound", object, depth);
                if (auto* node = object->AsNode()) {
                    auto& children = node->GetChildren();
                    if (children.capacity() > maxNodes || children.free_idx() > children.capacity() ||
                        children.size() > children.free_idx()) return Fail("child-array-contract", object, depth, diagnostic);
                    for (std::size_t j = 0; j < children.free_idx(); ++j) {
                        auto* child = children[static_cast<std::uint16_t>(j)].get();
                        if (!child) continue;
                        if (child->parent != node) return Fail("child-parent-mismatch", child, depth+1, diagnostic);
                        if (depth+1 > maxDepth || nodes.size() >= maxNodes || !seen.insert(child).second) return Fail("child-cycle-or-limit", child, depth+1, diagnostic);
                        nodes.push_back({RE::NiPointer<RE::NiAVObject>{child}, depth+1});
                    }
                }
                if (inspection) inspection->validatedNodes = i+1;
            }
            return true;
        }
        bool Collect(RE::NiAVObject* root, std::vector<Node>& nodes,
            std::vector<RE::NiPointer<RE::NiNode>>& ancestors)
        {
            if (!root) return Fail("missing-root");
            ProtectedNodes protectedNodes{};
            if (!TrackingNodes(protectedNodes)) return false;
            // Application still requires BOTH strict contracts before any write.
            // Only the explicit read-only capture below inspects them independently.
            if (!CollectAncestors(root, ancestors)) return false;
            return CollectObjects(root, nodes, ancestors, protectedNodes);
        }
        BoundProbe::Sphere WorldSphere(const RE::NiAVObject* object)
        {
            const auto& b = object->worldBound;
            return {{b.center.x, b.center.y, b.center.z}, b.radius};
        }
        nlohmann::json BoundObservation(const RE::NiAVObject* object)
        {
            const auto sphere = WorldSphere(object);
            std::ostringstream identity; identity << "0x" << std::hex << reinterpret_cast<std::uintptr_t>(object);
            nlohmann::json result{{"identity", identity.str()},
                {"name", std::string(object->name.c_str() ? object->name.c_str() : "").substr(0, 128)},
                {"fixedBound", object->GetFlags().any(RE::NiAVObject::Flag::kFixedBound)},
                {"worldBoundValid", BoundProbe::Valid(sphere)}};
            if (BoundProbe::Valid(sphere)) result["worldBound"] = {
                {"center", sphere.center}, {"radius", sphere.radius}};
            return result;
        }
        RE::NiAVObject* LiveAvatar();
        nlohmann::json InspectGraphBounds()
        {
            // Called only by the explicit main-thread capture, never per frame.
            // Inspect the two contracts independently, bypassing only fixed-bound
            // rejection. An unsupported ancestor must not hide avatar descendants.
            // Neither pass can call Propagate/ApplyPreview or change active refusal.
            nlohmann::json result{{"mode", "read-only-sampled-spheres"},
                {"rotationQualified", false}, {"fixedBoundGuardRelaxedForApplication", false}};
            if (!installed) { result["state"] = "hook-unavailable"; return result; }
            auto* root = LiveAvatar();
            GraphInspection ancestorInspection, inspection;
            std::vector<Node> nodes;
            std::vector<RE::NiPointer<RE::NiNode>> ancestors;
            ProtectedNodes protectedNodes{};
            bool ancestorComplete = false, descendantsComplete = false;
            if (!root) Fail("missing-avatar", nullptr, 0, &inspection.failure);
            else if (TrackingNodes(protectedNodes, &inspection.failure)) {
                if (!root->parent) Fail("missing-avatar-parent", root, 0, &ancestorInspection.failure);
                else if (!Frame(root->parent->world)) Fail("avatar-parent-frame", root->parent, 0, &ancestorInspection.failure);
                else ancestorComplete = CollectAncestors(root, ancestors, &ancestorInspection);
                // No virtual bounds/composer call is made. Retain prefix coverage
                // and independently report descendant failures without masking them.
                descendantsComplete = CollectObjects(root, nodes, ancestors, protectedNodes, &inspection);
            }
            const bool complete = ancestorComplete && descendantsComplete;
            result["state"] = complete ? "graph-contracts-inspected" : "partial";
            result["graphCompleteIgnoringFixedBound"] = complete;
            result["firstOtherFailure"] = ancestorInspection.failure.is_null() ? inspection.failure : ancestorInspection.failure;
            result["ancestorFailure"] = ancestorInspection.failure;
            result["descendantFailure"] = inspection.failure;
            result["descendantGraphCompleteIgnoringFixedBound"] = descendantsComplete;
            result["enumeratedNodeCount"] = nodes.size();
            result["validatedNodeCount"] = inspection.validatedNodes;
            result["sphereCoverage"] = "validated-nodes-only";
            result["ancestorChainComplete"] = ancestorComplete;
            result["ancestorCount"] = ancestors.size();
            BoundProbe::Sphere envelope;
            if (root) envelope.center = {root->world.translate.x, root->world.translate.y, root->world.translate.z};
            std::size_t empty = 0, invalid = 0, positive = 0, fixed = 0, geometry = 0, skipCompose = 0;
            auto fixedObjects = nlohmann::json::array();
            const auto base = REL::Module::get().base();
            for (std::size_t i = 0; i < inspection.validatedNodes; ++i) {
                auto* object = nodes[i].object.get();
                const auto sphere = WorldSphere(object);
                if (!BoundProbe::Valid(sphere) || !BoundProbe::Include(envelope, sphere)) ++invalid;
                else if (sphere.radius == 0) ++empty;
                else ++positive;
                if (object->GetFlags().any(RE::NiAVObject::Flag::kFixedBound)) {
                    ++fixed;
                    if (fixedObjects.size() < 8) fixedObjects.push_back(BoundObservation(object));
                }
                // Exact CB78C0 can copy skin-owned bounds; this count is not
                // proof that a particular geometry has a skin instance.
                if ((*reinterpret_cast<const std::uintptr_t* const*>(object))[0x30] == base+0xCB78C0) ++geometry;
                // C9BCE0 copies the parent world transform when this raw bit is set.
                // Do not infer semantics from CommonLib's borrowed flag name.
                if (object->GetFlags().any(static_cast<RE::NiAVObject::Flag>(1u<<9))) ++skipCompose;
            }
            const bool envelopeComplete = descendantsComplete && invalid == 0 && positive > 0 && BoundProbe::Valid(envelope);
            result["positiveSphereCount"] = positive; result["emptySphereCount"] = empty;
            result["invalidSphereCount"] = invalid; result["fixedObjectCount"] = fixed;
            result["firstFixedObjects"] = std::move(fixedObjects);
            result["fixedObjectsTruncated"] = fixed > 8;
            result["geometryBoundsRoutineCount"] = geometry;
            result["composerSkipFlagBit9Count"] = skipCompose;
            result["sampledEnvelopeComplete"] = envelopeComplete;
            if (positive > 0 && BoundProbe::Valid(envelope)) result["sampledFullTurnEnvelope"] = {
                {"center", envelope.center}, {"radius", envelope.radius}};
            constexpr double margin = 0.01; // world units; explicit observation, not a safety permit
            result["containmentMarginWorldUnits"] = margin;
            auto containers = nlohmann::json::array();
            for (const auto& ancestor : ancestors) {
                auto value = BoundObservation(ancestor.get());
                if (envelopeComplete && BoundProbe::Valid(WorldSphere(ancestor.get())))
                    value["containsSampledFullTurnEnvelope"] = BoundProbe::Contains(WorldSphere(ancestor.get()), envelope, margin);
                else value["containsSampledFullTurnEnvelope"] = nullptr;
                containers.push_back(std::move(value));
            }
            result["ancestorsParentFirst"] = std::move(containers);
            result["limitations"] = "Sampled spheres may be stale; skin-owned sphere/AABB and rendered geometry propagation remain unqualified.";
            return result;
        }
        void Propagate(const std::vector<Node>& nodes, const std::vector<RE::NiPointer<RE::NiNode>>& ancestors)
        {
            // All virtual targets/topology were checked before writing. Use the
            // non-recursive pure composer in parent-first order: bounded depth
            // does not become an unbounded native recursion or animation tick.
            const auto base = REL::Module::get().base();
            const auto compose = reinterpret_cast<SceneFunction>(base+0xC9BCE0);
            for (const auto& node : nodes) compose(node.object.get());
            for (auto it = nodes.rbegin(); it != nodes.rend(); ++it) {
                auto* object = it->object.get();
                const auto bound = (*reinterpret_cast<std::uintptr_t**>(object))[0x30];
                if (bound != base+0xC9C700) reinterpret_cast<SceneFunction>(bound)(object);
            }
            // Refit containing scene bounds too; otherwise a turned tail or
            // hairstyle can lie outside a parent's stale culling volume.
            const auto refit = reinterpret_cast<SceneFunction>(base+0xC9DC10);
            for (const auto& node : ancestors) refit(node.get());
        }
        RE::NiAVObject* LiveAvatar()
        {
            auto* player = RE::PlayerCharacter::GetSingleton();
            return player ? player->Get3D(false) : nullptr;
        }
        bool SamePoint(const RE::NiPoint3& a, const RE::NiPoint3& b)
        {
            return CameraPolicy::Finite(a) && CameraPolicy::Finite(b) &&
                (a-b).Length() <= 0.0001F;
        }
        bool SameWorld(const RE::NiTransform& a, const RE::NiTransform& b)
        {
            return SameRotation(a.rotate, b.rotate) && SamePoint(a.translate, b.translate) &&
                std::isfinite(a.scale) && std::isfinite(b.scale) && std::abs(a.scale-b.scale) <= 0.0001F;
        }
        bool RemoveTrialPreview()
        {
            // Strong references keep the copied branch alive, but never replay a
            // detached race/sex branch onto its replacement or foreign hierarchy.
            bool owned = true;
            if (avatar.get() == LiveAvatar() && avatar && avatar->parent == parent.get()) {
                if (SameRotation(avatar->local.rotate, previewRotation)) avatar->local.rotate = nativeRotation;
                else owned = false;
                for (const auto& node : trialNodes) {
                    auto* object = node.object.get();
                    if (object->parent != node.parentIdentity) { owned = false; continue; }
                    // Restore only values that still match OUR copied preview.
                    // Foreign animation/renderer writes are not overwritten.
                    if (SameWorld(object->world, node.trialWorld)) object->world = node.nativeWorld;
                    else owned = false;
                    if (SamePoint(object->worldBound.center, node.trialBound.center) &&
                        object->worldBound.radius == node.trialBound.radius) object->worldBound = node.nativeBound;
                    else owned = false;
                }
            }
            // Replacement roots need no writes: the obsolete branch is discarded.
            else if (avatar.get() == LiveAvatar()) owned = false;
            if (!owned) {
                ++unsafeTrialRestoreConflicts;
                SKSE::log::warn("Unsafe avatar trial restore conflict; only still-owned preview values restored");
            }
            trialNodes.clear(); avatar.reset(); parent.reset(); appliedYaw = 0;
            return owned;
        }
        bool ApplyTrialPreview(RE::NiAVObject* root)
        {
            // Deliberately experimental: do NOT execute unqualified virtual
            // composers, bounds routines, animation, collision or ancestor refits.
            // Instead rigidly rotate copied world poses/sphere centers after the
            // native menu update. Skin/culling/cache correctness is NOT qualified.
            ProtectedNodes protectedNodes{};
            if (!TrackingNodes(protectedNodes)) return false;
            std::unordered_set<RE::NiAVObject*> seen;
            std::vector<Node> nodes{{RE::NiPointer<RE::NiAVObject>{root}, 0}};
            seen.insert(root);
            for (std::size_t i = 0; i < nodes.size(); ++i) {
                auto* object = nodes[i].object.get(); const auto depth = nodes[i].depth;
                if (std::find(protectedNodes.begin(), protectedNodes.end(), object) != protectedNodes.end())
                    return Fail("trial-protected-tracking-or-ui-node", object, depth);
                if (!InRdata(*reinterpret_cast<const std::uintptr_t*>(object), 0x31*sizeof(std::uintptr_t)))
                    return Fail("trial-object-vtable", object, depth);
                if (!Frame(object->local) || !Frame(object->world) || !BoundProbe::Valid(WorldSphere(object)))
                    return Fail("trial-object-frame-or-sphere", object, depth);
                if (auto* node = object->AsNode()) {
                    auto& children = node->GetChildren();
                    if (children.capacity() > maxNodes || children.free_idx() > children.capacity() ||
                        children.size() > children.free_idx()) return Fail("trial-child-array-contract", object, depth);
                    for (std::size_t j = 0; j < children.free_idx(); ++j) {
                        auto* child = children[static_cast<std::uint16_t>(j)].get();
                        if (!child) continue;
                        if (child->parent != node) return Fail("trial-child-parent-mismatch", child, depth+1);
                        if (depth+1 > maxDepth || nodes.size() >= maxNodes || !seen.insert(child).second)
                            return Fail("trial-child-cycle-or-limit", child, depth+1);
                        nodes.push_back({RE::NiPointer<RE::NiAVObject>{child}, depth+1});
                    }
                }
            }
            // Prove no cyclic ancestor lies inside the branch; do not touch it.
            std::unordered_set<RE::NiAVObject*> ancestorSeen;
            for (auto* p = root->parent; p; p = p->parent) {
                if (ancestorSeen.size() >= maxDepth || seen.count(p) || !ancestorSeen.insert(p).second)
                    return Fail("trial-ancestor-cycle-or-depth", p, ancestorSeen.size());
            }
            RE::NiTransform identity;
            const auto yaw = CameraPolicy::YawAroundEye(identity, {}, requestedYaw).rotate;
            const auto pivot = root->world.translate;
            const auto& p = root->parent->world.rotate;
            auto nextLocal = root->local;
            nextLocal.rotate = p.Transpose()*yaw*p*root->local.rotate;
            if (!Frame(nextLocal)) return Fail("trial-composed-local-frame", root);
            std::vector<TrialNode> planned;
            planned.reserve(nodes.size());
            for (const auto& node : nodes) {
                auto* object = node.object.get();
                TrialNode copy{node.object, object->parent, object->world, object->world,
                    object->worldBound, object->worldBound};
                copy.trialWorld.rotate = yaw*copy.nativeWorld.rotate;
                copy.trialWorld.translate = pivot+yaw*(copy.nativeWorld.translate-pivot);
                // Empty spheres carry no geometry; preserve their arbitrary center.
                if (copy.nativeBound.radius > 0)
                    copy.trialBound.center = pivot+yaw*(copy.nativeBound.center-pivot);
                if (!Frame(copy.trialWorld) || !CameraPolicy::Finite(copy.trialBound.center))
                    return Fail("trial-composed-world-frame-or-sphere", object, node.depth);
                planned.push_back(std::move(copy));
            }
            // Complete bounded preflight and allocation BEFORE the first write.
            trialNodes = std::move(planned);
            avatar.reset(root); parent.reset(root->parent);
            nativeRotation = root->local.rotate; previewRotation = nextLocal.rotate;
            root->local.rotate = previewRotation;
            for (const auto& node : trialNodes) {
                node.object->world = node.trialWorld;
                node.object->worldBound = node.trialBound;
            }
            appliedYaw = requestedYaw; ++unsafeTrialApplications;
            return true;
        }
        bool RemovePreview(bool propagate = true)
        {
            if (!trialNodes.empty()) return RemoveTrialPreview();
            if (!avatar) return true;
            // Never replay a detached race/sex root onto its replacement.
            if (avatar.get() != LiveAvatar()) { avatar.reset(); parent.reset(); appliedYaw = 0; return true; }
            if (avatar->parent != parent.get() || !SameRotation(avatar->local.rotate, previewRotation)) return false;
            std::vector<Node> nodes;
            std::vector<RE::NiPointer<RE::NiNode>> ancestors;
            if (propagate && !Collect(avatar.get(), nodes, ancestors)) return false;
            avatar->local.rotate = nativeRotation; // keep current translation/scale
            if (propagate) Propagate(nodes, ancestors);
            avatar.reset(); parent.reset(); appliedYaw = 0;
            return true;
        }
        void Reject(const char* reason)
        {
            if (!rejected) {
                refusal["stage"] = reason;
                refusal["attemptedAvatarYaw"] = requestedYaw;
            }
            requestedYaw = 0;
            if (!rejected) SKSE::log::warn("RaceMenu avatar preview rotation unavailable: {}; diagnostic={}", reason,
                refusal.dump(-1, ' ', false, nlohmann::json::error_handler_t::replace));
            rejected = true;
            MenuExtensions::GetInterface()->SetValue(provider, "avatarYaw", appliedYaw);
        }
        bool ApplyPreview()
        {
            auto* root = LiveAvatar();
            if (!root) return Fail("missing-avatar");
            if (!root->parent) return Fail("missing-avatar-parent", root);
            if (!Frame(root->local)) return Fail("avatar-local-frame", root);
            if (!Frame(root->parent->world)) return Fail("avatar-parent-frame", root->parent);
            if (unsafeTrialEnabled) return ApplyTrialPreview(root);
            std::vector<Node> nodes;
            std::vector<RE::NiPointer<RE::NiNode>> ancestors;
            if (!Collect(root, nodes, ancestors)) return false;
            // World-Z turn around the root position, converted back through the
            // parent frame. Actor reference rotation and position are untouched.
            RE::NiTransform identity;
            const auto yaw = CameraPolicy::YawAroundEye(identity, {}, requestedYaw).rotate;
            const auto baseline = root->local.rotate;
            const auto& p = root->parent->world.rotate;
            auto next = root->local; next.rotate = p.Transpose()*yaw*p*baseline;
            if (!Frame(next)) return Fail("composed-preview-frame", root);
            avatar.reset(root); parent.reset(root->parent);
            nativeRotation = baseline; previewRotation = next.rotate;
            root->local.rotate = previewRotation;
            Propagate(nodes, ancestors);
            appliedYaw = requestedYaw;
            return true;
        }
        void AvatarSlider(double value, void*)
        {
            if (!installed || rejected || !movieIdentity || !std::isfinite(value) || std::abs(value) > 180) {
                MenuExtensions::GetInterface()->SetValue(provider, "avatarYaw", appliedYaw); return;
            }
            requestedYaw = static_cast<float>(value);
        }
        void UnsafeTrialSlider(double value, void*)
        {
            if (!installed || !movieIdentity || (value != 0 && value != 1)) return;
            if (!RemovePreview()) {
                unsafeTrialEnabled = false;
                Reject("unsafe trial restoration ownership conflict");
            } else {
                unsafeTrialEnabled = value == 1;
                requestedYaw = appliedYaw = 0; rejected = false; refusal = nullptr;
                SKSE::log::warn("Unsafe avatar rotation trial {}: no ancestor refit or native skin/culling qualification",
                    unsafeTrialEnabled ? "enabled by explicit menu opt-in" : "disabled");
            }
            auto* service = MenuExtensions::GetInterface();
            service->SetValue(provider, "avatarYaw", appliedYaw);
            service->SetValue(provider, "unsafeAvatarTrial", unsafeTrialEnabled ? 1 : 0);
            FaceView::RefreshAvatarAnchor();
        }
        void ViewSlider(double value, void*)
        {
            // Experimental debounce, not a release detector. No scene movement
            // during callback bursts, no renderer rebuild while input is pending.
            if (!movieIdentity || !viewTrial.Submit(value, FaceView::ViewYaw(), NowMilliseconds()))
                MenuExtensions::GetInterface()->SetValue(provider, "viewYaw", -FaceView::ViewYaw());
        }
        void CommitViewTrial()
        {
            const auto next = viewTrial.Take(FaceView::ViewYaw(), NowMilliseconds());
            if (!next) return;
            const bool accepted = FaceView::ApplyViewYawOnGameTask(*next, movieIdentity);
            VR::RecordExtensionTrace(movieIdentity, accepted ? "view_trial_applied" : "view_trial_rejected",
                provider, "viewYaw", viewTrial.requested, FaceView::ViewYaw());
        }
        RE::UI_MESSAGE_RESULTS ProcessHook(RE::RaceSexMenu* menu, RE::UIMessage& message)
        {
            auto* ui = RE::UI::GetSingleton();
            auto liveMenu = ui ? ui->GetMenu<RE::RaceSexMenu>() : RE::GPtr<RE::RaceSexMenu>{};
            const bool update = message.type == RE::UI_MESSAGE_TYPE::kUpdate && liveMenu.get() == menu &&
                ui && ui->IsMenuOpen(RE::RaceSexMenu::MENU_NAME);
            const bool close = menu && menu->uiMovie.get() == movieIdentity &&
                (message.type == RE::UI_MESSAGE_TYPE::kHide || message.type == RE::UI_MESSAGE_TYPE::kForceHide);
            const float priorYaw = appliedYaw;
            auto* priorRoot = avatar.get();
            if (update || close) {
                try {
                    if (close) Restore();
                    else if (!RemovePreview(false)) Reject("preview ownership/topology changed");
                } catch (...) { Reject("pre-update validation failed"); }
            }
            // Exactly one native call, unchanged arguments, regardless of feature failure.
            const auto result = original(menu, message);
            if (update && menu && menu->uiMovie) {
                liveMenu = ui ? ui->GetMenu<RE::RaceSexMenu>() : RE::GPtr<RE::RaceSexMenu>{};
                if (ui && liveMenu.get() == menu && ui->IsMenuOpen(RE::RaceSexMenu::MENU_NAME)) {
                    try {
                        Register(menu->uiMovie.get());
                        CommitViewTrial();
                        if (!rejected && requestedYaw != 0 && !ApplyPreview()) Reject("unsupported avatar transform/bounds graph");
                        if (priorYaw != appliedYaw || (priorRoot && priorRoot != LiveAvatar())) FaceView::RefreshAvatarAnchor();
                        MenuExtensions::GetInterface()->SetValue(provider, "avatarYaw", appliedYaw);
                        if (!viewTrial.pending) MenuExtensions::GetInterface()->SetValue(provider, "viewYaw", -FaceView::ViewYaw());
                    } catch (...) { Reject("post-update validation failed"); }
                }
            }
            return result;
        }
        template <std::size_t N>
        bool Code(std::uintptr_t rva, const std::array<std::uint8_t, N>& prefix)
        {
            const auto& module = REL::Module::get();
            const auto text = module.segment(REL::Segment::Name::textx);
            const auto address = module.base()+rva;
            return address >= text.address() && address <= text.address()+text.size() &&
                N <= text.address()+text.size()-address && !std::memcmp(reinterpret_cast<const void*>(address), prefix.data(), N);
        }
    }
    bool Install()
    {
#if defined(ENABLE_SKYRIM_VR)
        if (installed) return true;
        if (!REL::Module::IsVR() || REL::Module::get().version() != REL::Version(1,4,15,0)) return false;
        REL::Relocation<std::uintptr_t> table{RE::VTABLE_RaceSexMenu[0]};
        if (!InRdata(table.address(), 5*sizeof(std::uintptr_t)) ||
            reinterpret_cast<const std::uintptr_t*>(table.address())[4] != REL::Module::get().base()+0x8DD6E0 ||
            !Code(0x8DD6E0, std::array<std::uint8_t, 16>{0x48,0x8B,0xC4,0x55,0x53,0x56,0x57,0x41,0x56,0x48,0x8D,0xA8,0xC8,0xFE,0xFF,0xFF}) ||
            !Code(0xC9BCE0, std::array<std::uint8_t, 9>{0x40,0x53,0x48,0x83,0xEC,0x60,0x48,0x8B,0xD9}) ||
            !Code(0xC9DEA0, std::array<std::uint8_t, 9>{0x40,0x56,0x48,0x83,0xEC,0x20,0x48,0x8B,0xF1}) ||
            !Code(0xC9DC10, std::array<std::uint8_t, 11>{0x40,0x55,0x48,0x83,0xEC,0x20,0x8B,0x81,0x0C,0x01,0x00}) ||
            !Code(0xCB78C0, std::array<std::uint8_t, 10>{0x48,0x8B,0xC4,0x48,0x89,0x58,0x08,0x57,0x48,0x81})) {
            SKSE::log::warn("RaceMenu avatar preview hook unavailable: exact VR native contract mismatch; existing menu unchanged");
            return false;
        }
        original = reinterpret_cast<Process>(table.write_vfunc(4, ProcessHook)); installed = true;
        SKSE::log::info("Installed VR avatar preview rotation after native menu update (no saved actor angle changes)");
        return true;
#else
        return false;
#endif
    }
    void Register(RE::GFxMovie* movie)
    {
        if (!installed || !movie || movie == movieIdentity) return;
        Restore(); movieIdentity = movie; rejected = false; refusal = nullptr; viewTrial = {};
        auto* service = MenuExtensions::GetInterface();
        const bool section = service->RegisterSection({provider, "view", "View", 1u<<30, 1000});
        if (!section || !service->RegisterSlider({provider,"view","avatarYaw","Avatar rotation",-180,180,1,0,AvatarSlider,nullptr}) ||
            !service->RegisterSlider({provider,"view","unsafeAvatarTrial","Unsafe avatar trial",0,1,1,0,UnsafeTrialSlider,nullptr}) ||
            (FaceView::Supported() && !service->RegisterSlider({provider,"view","viewYaw","View direction",-60,60,1,-FaceView::ViewYaw(),ViewSlider,nullptr}))) {
            service->UnregisterProvider(provider); Reject("View category registration failed");
        }
    }
    void Restore()
    {
        if (!installed) return;
        viewTrial.Cancel();
        try {
            if (!RemovePreview()) Reject("cannot restore a competing preview pose");
        } catch (...) { Reject("preview restoration validation failed"); }
        avatar.reset(); parent.reset(); requestedYaw = appliedYaw = 0;
        trialNodes.clear(); unsafeTrialEnabled = false;
        unsafeTrialApplications = unsafeTrialRestoreConflicts = 0;
        movieIdentity = nullptr;
        // Fresh registration tokens cancel old queued inputs even if an engine
        // allocator reuses a movie address on the next menu opening.
        MenuExtensions::GetInterface()->UnregisterProvider(provider);
        FaceView::RefreshAvatarAnchor();
    }
    nlohmann::json CaptureDiagnostics()
    {
        return {{"avatarRequestedYaw", requestedYaw}, {"avatarAppliedYaw", appliedYaw},
            {"avatarRejected", rejected}, {"avatarRefusal", refusal},
            {"avatarBoundProbe", InspectGraphBounds()},
            {"unsafeAvatarTrial", {{"enabled", unsafeTrialEnabled}, {"mode", "rigid-world-preview-no-native-refit"},
                {"applications", std::to_string(unsafeTrialApplications)}, {"restoreConflicts", std::to_string(unsafeTrialRestoreConflicts)},
                {"ownedNodeCount", trialNodes.size()}, {"ancestorRefit", false}, {"rotationQualified", false},
                {"bypasses", "fixed-bound and pure-composer/bounds qualification; skin/culling/cache behaviour unqualified"}}},
            {"viewTrial", {{"mode", "quiet-window-not-release"}, {"quietMilliseconds", ViewDirectionTrial::quietMilliseconds},
                {"pending", viewTrial.pending}, {"requestedDisplayYaw", viewTrial.requested},
                {"appliedNativeYaw", FaceView::ViewYaw()}, {"displayYaw", -FaceView::ViewYaw()},
                {"requests", std::to_string(viewTrial.requests)}, {"commits", std::to_string(viewTrial.commits)},
                {"cancellations", std::to_string(viewTrial.cancellations)}}}};
    }
}
