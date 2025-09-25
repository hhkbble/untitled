#include <atomic>
#include <chrono>
#include <filesystem>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <unordered_set>
#include <windows.h>

#include <MinHook.h>
#include <easylogging++.h>

#include <SDK/Engine_classes.hpp>
#include <SDK/Engine_parameters.hpp>
#include <SDK/OakGame_classes.hpp>

#include "addon.h"
#include "fields.h"
#include "hook.h"

namespace SDK {

inline FPostProcessSettings &operator|=(FPostProcessSettings &lhs, const FPostProcessSettings &rhs) noexcept {
#define ASSIGN_FIELD(Name)                                                                                             \
    do {                                                                                                               \
        lhs.Name = rhs.Name;                                                                                           \
        lhs.bOverride_##Name = rhs.bOverride_##Name;                                                                   \
    } while (0)
#define APPLY_ASSIGN(Name) ASSIGN_FIELD(Name);
    FOR_EACH(APPLY_ASSIGN, PPS_FIELDS);
#undef APPLY_ASSIGN
#undef ASSIGN_FIELD
    return lhs;
}

} // namespace SDK

using ProcessEvent = void (*)(SDK::UObject *, SDK::UFunction *, void *);

namespace {

template <typename T> class set {
  public:
    bool contains(const T &value) {
        std::shared_lock slock(mutex_);
        if (data_.contains(value))
            return true;
        slock.unlock();
        std::unique_lock ulock(mutex_);
        data_.insert(value);
        return false;
    }

  private:
    std::shared_mutex mutex_;
    std::unordered_set<T> data_;
};

std::mutex GMutex;
ProcessEvent GProcessEvent = nullptr;

set<std::string> GFunctionNames;
std::atomic<std::pair<SDK::UCameraModifier *, SDK::UFunction *> *> MyCameraModifier = nullptr;
std::atomic_bool GUninstalling = false;

void *GetProcessEvent(SDK::UEngine *engine) {
    auto vtable = *reinterpret_cast<void ***>(engine);
    return vtable[SDK::Offsets::ProcessEventIdx];
}

void WaitForReady() {
    while (!SDK::UEngine::GetEngine()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    while (!SDK::UWorld::GetWorld()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void MyBlueprintModifyPostProcess(SDK::Params::CameraModifier_BlueprintModifyPostProcess *params) {
    auto weight = myPostProcessBlendWeight.load(std::memory_order_acquire);
    if (weight > 0.f) {
        params->PostProcessBlendWeight = weight;
        params->PostProcessSettings |= myPostProcessSettings;
    }
}

void MyProcessEvent(SDK::UObject *object, SDK::UFunction *function, void *params) {
    if (auto pair = MyCameraModifier.load(std::memory_order_acquire)) {
        auto name = function->GetName();
        if (object->Outer && object->Outer->IsA(SDK::AOakPlayerCameraManager::StaticClass()) &&
            name == "BlueprintModifyPostProcess") {
            auto modifier = pair->first;
            auto modify = pair->second;
            if (object == modifier && function == modify) {
                GProcessEvent(object, function, params);
                MyBlueprintModifyPostProcess(
                    static_cast<SDK::Params::CameraModifier_BlueprintModifyPostProcess *>(params));
                LOG_N_TIMES(1, WARNING) << "Called MyBlueprintModifyPostProcess";
                return;
            }
        }
    } else if (object && function) {
        auto name = function->GetName();
        if (!GFunctionNames.contains(name)) {
            LOG(INFO) << object->GetName() << " | " << name;
        }
        if (object->Outer && object->Outer->IsA(SDK::AOakPlayerCameraManager::StaticClass()) &&
            name == "BlueprintModifyPostProcess") {
            LOG(INFO) << "Found " << object->GetName() << " | " << name;
            static std::once_flag flag;
            std::call_once(flag, [&] {
                LOG(WARNING) << "Installing CameraModifier";
                auto outer = reinterpret_cast<SDK::AOakPlayerCameraManager *>(object->Outer);
                auto modifier = outer->AddNewCameraModifier(SDK::UCameraModifier::StaticClass());
                modifier->priority = 0;
                modifier->ALPHA = 1.0f;
                auto modify = modifier->Class->GetFunction("CameraModifier", "BlueprintModifyPostProcess");
                MyCameraModifier.store(new std::pair{modifier, modify}, std::memory_order_release);
                LOG(WARNING) << "Installed CameraModifier";
            });
        }
    }
    return GProcessEvent(object, function, params);
}

void InstallMyProcessEvent() {
    std::lock_guard guard(GMutex);

    LOG(INFO) << "Installing ProcessEvent hook";
    auto status = MH_Initialize();
    if (status != MH_OK) {
        LOG(ERROR) << "Failed to initialize MinHook: " << MH_StatusToString(status);
        return;
    }

    auto engine = SDK::UEngine::GetEngine();
    auto pTarget = GetProcessEvent(engine);
    if (!pTarget) {
        LOG(ERROR) << "Failed to get ProcessEvent";
        return;
    }
    auto pDetour = &MyProcessEvent;
    auto ppOriginal = reinterpret_cast<void **>(&GProcessEvent);
    auto retry = 0;
    while (true) {
        status = MH_CreateHook(pTarget, pDetour, ppOriginal);
        if (status == MH_OK)
            break;
        if (retry % 0xf == 0)
            LOG(WARNING) << "Retrying to create hook: " << MH_StatusToString(status);
        if (retry++ > 0xff) {
            LOG(ERROR) << "Failed to create hook: " << MH_StatusToString(status);
            MH_Uninitialize();
            return;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    status = MH_EnableHook(pTarget);
    if (status != MH_OK) {
        LOG(ERROR) << "Failed to enable hook: " << MH_StatusToString(status);
        MH_RemoveHook(pTarget);
        MH_Uninitialize();
        return;
    }
    LOG(WARNING) << "Installed ProcessEvent hook";
}

} // namespace

void UninstallHook() {
    GUninstalling.store(true, std::memory_order_release);
    std::lock_guard guard(GMutex);
    if (!SDK::UEngine::GetEngine() || !SDK::UWorld::GetWorld() || !GProcessEvent) {
        LOG(WARNING) << "No need to uninstall ProcessEvent hook";
        return;
    }

    LOG(INFO) << "Uninstalling ProcessEvent hook";

    auto engine = SDK::UEngine::GetEngine();
    auto pTarget = GetProcessEvent(engine);
    if (!pTarget) {
        LOG(ERROR) << "Failed to get ProcessEvent";
        return;
    }
    auto status = MH_DisableHook(pTarget);
    if (status != MH_OK) {
        LOG(ERROR) << "Failed to disable hook: " << MH_StatusToString(status);
        return;
    }
    status = MH_RemoveHook(pTarget);
    if (status != MH_OK) {
        LOG(ERROR) << "Failed to remove hook: " << MH_StatusToString(status);
        return;
    }
    status = MH_Uninitialize();
    if (status != MH_OK) {
        LOG(ERROR) << "Failed to uninitialize MinHook: " << MH_StatusToString(status);
        return;
    }
    LOG(WARNING) << "Uninstalled ProcessEvent hook";
}

void InstallHook() {
    std::thread([] {
        WaitForReady();
        InstallMyProcessEvent();
        auto engine = SDK::UEngine::GetEngine();
        SDK::UInputSettings::GetDefaultObj()->ConsoleKeys[0].KeyName =
            SDK::UKismetStringLibrary::Conv_StringToName(L"F2");
        auto console = SDK::UGameplayStatics::SpawnObject(engine->ConsoleClass, engine->GameViewport);
        engine->GameViewport->ViewportConsole = reinterpret_cast<SDK::UConsole *>(console);
    }).detach();
}
