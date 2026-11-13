/*
 * SPDX-FileCopyrightText: 2026 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>
#include <string>

#include "MotorInfo.h"
#include "effect_stream.h"

namespace aidl {
namespace android {
namespace hardware {
namespace vibrator {

// A table of FIFO waveforms for the motor of this device, keyed by OPlus effect id.
class EffectSource {
  public:
    virtual ~EffectSource() = default;

    // The returned stream stays valid for the lifetime of the source.
    virtual const effect_stream* get(uint32_t effectId) const = 0;
    // Where the table comes from, for logs and dumpsys.
    virtual const std::string& description() const = 0;

    // Picks the table to use on this device, or nullptr when it ships none and effects have to
    // come from the ones predefined in the kernel devicetree.
    static std::unique_ptr<EffectSource> create(const MotorInfo& motor);
};

// From /odm/etc/vibrator/vibrator_effect.json and the waveform files it points at.
std::unique_ptr<EffectSource> createJsonEffectSource(const MotorInfo& motor);
// From the tables compiled into the stock libqtivibratoreffect.so.
std::unique_ptr<EffectSource> createBlobEffectSource(const MotorInfo& motor);

}  // namespace vibrator
}  // namespace hardware
}  // namespace android
}  // namespace aidl
