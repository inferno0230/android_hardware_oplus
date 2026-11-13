/*
 * SPDX-FileCopyrightText: 2026 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include "MotorInfo.h"

#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/parseint.h>
#include <android-base/properties.h>
#include <android-base/strings.h>

#include <string>

namespace aidl {
namespace android {
namespace hardware {
namespace vibrator {

namespace {

constexpr char kVibratorTypePath[] = "/sys/class/qcom-haptics/vibrator_type";
// 0 = default style, 1 = soft style. Read once at startup.
constexpr char kTouchStyleProperty[] = "persist.vendor.vibrator.touch_style";

}  // anonymous namespace

MotorInfo MotorInfo::detect() {
    MotorInfo info;

    std::string value;
    if (!::android::base::ReadFileToString(kVibratorTypePath, &value)) {
        // Expected on sm8450, whose driver has no such attribute.
        PLOG(INFO) << "Cannot read " << kVibratorTypePath << ", vibrator type is unknown";
    } else if (!::android::base::ParseUint(::android::base::Trim(value), &info.vibratorType)) {
        LOG(WARNING) << "Unexpected vibrator type '" << ::android::base::Trim(value) << "'";
        info.vibratorType = kTypeUnknown;
    }

    if (::android::base::GetIntProperty(kTouchStyleProperty, 0) == 1) {
        info.touchStyle = TouchStyle::SOFT;
    }

    return info;
}

}  // namespace vibrator
}  // namespace hardware
}  // namespace android
}  // namespace aidl
