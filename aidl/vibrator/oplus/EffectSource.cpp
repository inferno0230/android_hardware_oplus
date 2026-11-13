/*
 * SPDX-FileCopyrightText: 2026 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include "EffectSource.h"

#include <android-base/logging.h>

namespace aidl {
namespace android {
namespace hardware {
namespace vibrator {

std::unique_ptr<EffectSource> EffectSource::create(const MotorInfo& motor) {
    auto source = createJsonEffectSource(motor);
    if (!source) {
        source = createBlobEffectSource(motor);
    }

    if (source) {
        LOG(INFO) << "Effect source: " << source->description();
    } else {
        LOG(INFO) << "No effect table on this device, using the effects predefined in the kernel";
    }
    return source;
}

}  // namespace vibrator
}  // namespace hardware
}  // namespace android
}  // namespace aidl
