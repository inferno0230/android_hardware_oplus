/*
 * Copyright (C) 2022 The LineageOS Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "vendor.lineage.touch@1.0-service.oplus"

#include "HighTouchPollingRate.h"

#include <android-base/file.h>

#ifdef USES_OPLUS_TOUCH
#include <aidl/vendor/oplus/hardware/touch/IOplusTouch.h>
#include <android/binder_manager.h>
#include <android-base/logging.h>
#else
using ::android::base::ReadFileToString;
using ::android::base::WriteStringToFile;
#endif // USES_OPLUS_TOUCH

namespace {

#ifdef USES_OPLUS_TOUCH
#define GAME_MODE_ENABLE_SWITCH 26
#define GAME_MODE_PARAM_VAL 0x258
#else
constexpr const char* kGameSwitchEnablePath = "/proc/touchpanel/game_switch_enable";
#endif // USES_OPLUS_TOUCH

}  // anonymous namespace

namespace vendor {
namespace lineage {
namespace touch {
namespace V1_0 {
namespace implementation {

#ifdef USES_OPLUS_TOUCH
using ::aidl::vendor::oplus::hardware::touch::IOplusTouch;
static std::string value;
#endif // USES_OPLUS_TOUCH

Return<bool> HighTouchPollingRate::isEnabled() {
#ifdef USES_OPLUS_TOUCH
    std::shared_ptr<IOplusTouch> mTouchService;
    const std::string instance = std::string() + IOplusTouch::descriptor + "/default";
    mTouchService = IOplusTouch::fromBinder(ndk::SpAIBinder(
        AServiceManager_waitForService(instance.c_str())));

    mTouchService->touchReadNodeFile(0, GAME_MODE_ENABLE_SWITCH, &value);
    return value != "0";
#else
    std::string value;
    return ReadFileToString(kGameSwitchEnablePath, &value) && value[0] != '0';
#endif // USES_OPLUS_TOUCH
}

Return<bool> HighTouchPollingRate::setEnabled(bool enabled) {
#ifdef USES_OPLUS_TOUCH
    std::shared_ptr<IOplusTouch> mTouchService;
    const std::string instance = std::string() + IOplusTouch::descriptor + "/default";
    mTouchService = IOplusTouch::fromBinder(ndk::SpAIBinder(
        AServiceManager_waitForService(instance.c_str())));

    int returnValue;
    mTouchService->touchWriteNodeFile(0, GAME_MODE_ENABLE_SWITCH, enabled ? std::to_string(GAME_MODE_PARAM_VAL) : "0", &returnValue);
    return true;
#else
    return WriteStringToFile(enabled ? "1" : "0", kGameSwitchEnablePath, true);
#endif // USES_OPLUS_TOUCH
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace touch
}  // namespace lineage
}  // namespace vendor
