/*
 * SPDX-FileCopyrightText: 2026 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <aidl/android/hardware/vibrator/Effect.h>

#include <array>

namespace aidl {
namespace android {
namespace hardware {
namespace vibrator {

struct EffectMapEntry {
    static constexpr int32_t kNone = -1;

    Effect effect;
    // OPlus effect ids to try in order, the first one the active table knows is used.
    std::array<int32_t, 2> ids;
};

/*
 * AOSP effect -> OPlus effect id.
 *
 * The first id is what stock does: Vibrator::perform() (0xe844 in the OnePlus 12R 15.0.0.1301
 * vendor.qti.hardware.vibrator.impl.so) hands the AOSP enum value to InputFFDevice::play()
 * (0x1b3f8) untouched, which uses it as the id for the effect table and for the effects
 * predefined in the kernel devicetree alike.
 *
 * The ELA0809 default table has no effect 0. Stock ends up playing the devicetree pattern for
 * CLICK there, which is not tuned for that motor, so substitute the short click of the table.
 */
inline constexpr std::array<EffectMapEntry, 6> kEffectMap = {{
        {Effect::CLICK, {0, 2}},
        {Effect::DOUBLE_CLICK, {1, EffectMapEntry::kNone}},
        {Effect::TICK, {2, EffectMapEntry::kNone}},
        {Effect::THUD, {3, EffectMapEntry::kNone}},
        {Effect::POP, {4, EffectMapEntry::kNone}},
        {Effect::HEAVY_CLICK, {5, EffectMapEntry::kNone}},
}};

}  // namespace vibrator
}  // namespace hardware
}  // namespace android
}  // namespace aidl
