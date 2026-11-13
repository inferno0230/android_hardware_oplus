/*
 * Copyright (c) 2018,2020-2021, The Linux Foundation. All rights reserved.
 * SPDX-FileCopyrightText: 2026 The LineageOS Project
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <android-base/unique_fd.h>
#include <linux/input.h>

#include <array>
#include <memory>
#include <mutex>

#include "Backend.h"
#include "EffectSource.h"
#include "MotorInfo.h"

namespace aidl {
namespace android {
namespace hardware {
namespace vibrator {

// The PMIC haptics module, driven through the force feedback interface of qcom-hv-haptics.
class FfBackend : public Backend {
  public:
    // Returns nullptr when the device has no such haptics.
    static std::unique_ptr<FfBackend> probe();

    const char* name() const override { return "qcom-hv-haptics"; }
    int32_t capabilities() const override;

    int on(uint32_t timeoutMs, uint32_t* playLengthMs) override;
    int off() override;
    int setAmplitude(float amplitude) override;

    std::vector<Effect> supportedEffects() const override;
    int perform(Effect effect, EffectStrength strength, uint32_t* playLengthMs) override;

    int32_t primitiveDurationMs(CompositePrimitive primitive) const override;
    int playPrimitive(CompositePrimitive primitive, float scale, uint32_t* playLengthMs) override;

    void dump(std::string* out) const override;

  private:
    struct ResolvedEffect {
        Effect effect;
        uint32_t id;
        // nullptr: play effect `id` of the kernel devicetree instead.
        const effect_stream* stream;
    };

    static constexpr size_t kPrimitiveCount = static_cast<size_t>(CompositePrimitive::LOW_TICK) + 1;

    FfBackend(::android::base::unique_fd fd, bool supportsGain, bool supportsEffects);

    void resolveEffects();
    void queryPrimitives();

    int eraseLocked();
    int startLocked(struct ff_effect* effect);
    // Plays effect or primitive `id` as predefined in the kernel devicetree.
    int startPredefinedLocked(int16_t id, int16_t magnitude, uint32_t* playLengthMs);

    const ::android::base::unique_fd mFd;
    const bool mSupportsGain;
    const bool mSupportsEffects;
    bool mSupportsExternalControl = false;

    MotorInfo mMotor;
    std::unique_ptr<EffectSource> mSource;
    std::vector<ResolvedEffect> mEffects;
    // Indexed by CompositePrimitive, 0 for the ones the kernel does not have.
    std::array<int32_t, kPrimitiveCount> mPrimitiveDurationsMs = {};

    std::mutex mMutex;
    // Slot of the effect currently uploaded to the kernel, or -1.
    int16_t mSlot = -1;
    // Level for on(), as last set through setAmplitude().
    int16_t mMagnitude;
};

}  // namespace vibrator
}  // namespace hardware
}  // namespace android
}  // namespace aidl
