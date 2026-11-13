/*
 * SPDX-FileCopyrightText: 2026 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <stdint.h>

namespace aidl {
namespace android {
namespace hardware {
namespace vibrator {

enum class TouchStyle {
    DEFAULT,
    SOFT,
};

struct MotorInfo {
    static constexpr uint32_t kTypeUnknown = 0;

    // qcom,vibrator-type as reported by the kernel (809, 8151, 9999), or kTypeUnknown when the
    // kernel does not expose it.
    uint32_t vibratorType = kTypeUnknown;
    TouchStyle touchStyle = TouchStyle::DEFAULT;

    static MotorInfo detect();
};

}  // namespace vibrator
}  // namespace hardware
}  // namespace android
}  // namespace aidl
