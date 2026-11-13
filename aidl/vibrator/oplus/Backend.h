/*
 * SPDX-FileCopyrightText: 2026 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <aidl/android/hardware/vibrator/IVibrator.h>
#include <errno.h>

#include <string>
#include <vector>

namespace aidl {
namespace android {
namespace hardware {
namespace vibrator {

/*
 * The haptic hardware of a device. Vibrator picks one at startup and layers everything that does
 * not depend on the hardware on top of it: argument checks, completion callbacks, stepping
 * through compositions.
 *
 * Calls return 0 or a negative errno, and are serialised by the caller.
 */
class Backend {
  public:
    virtual ~Backend() = default;

    virtual const char* name() const = 0;
    // The IVibrator::CAP_* bits to advertise.
    virtual int32_t capabilities() const = 0;

    // playLengthMs is when the vibration will be over, which may differ from what was asked for.
    virtual int on(uint32_t timeoutMs, uint32_t* playLengthMs) = 0;
    // Also forgets the amplitude set through setAmplitude().
    virtual int off() = 0;
    // amplitude is in (0.0, 1.0], and applies to on() only.
    virtual int setAmplitude(float amplitude) = 0;

    virtual std::vector<Effect> supportedEffects() const = 0;
    virtual int perform(Effect effect, EffectStrength strength, uint32_t* playLengthMs) = 0;

    // Negative for primitives that cannot be played.
    virtual int32_t primitiveDurationMs(CompositePrimitive /*primitive*/) const { return -1; }
    // scale is in [0.0, 1.0], with 0.0 the weakest amplitude that can still be felt.
    virtual int playPrimitive(CompositePrimitive /*primitive*/, float /*scale*/,
                              uint32_t* /*playLengthMs*/) {
        return -ENOTSUP;
    }

    // Appends a description of the backend's state for dumpsys.
    virtual void dump(std::string* /*out*/) const {}
};

// For boards without (working) haptics: accepts everything, moves nothing.
class NullBackend : public Backend {
  public:
    const char* name() const override { return "none"; }
    int32_t capabilities() const override { return 0; }

    int on(uint32_t timeoutMs, uint32_t* playLengthMs) override {
        *playLengthMs = timeoutMs;
        return 0;
    }
    int off() override { return 0; }
    int setAmplitude(float /*amplitude*/) override { return 0; }

    std::vector<Effect> supportedEffects() const override { return {}; }
    int perform(Effect /*effect*/, EffectStrength /*strength*/,
                uint32_t* /*playLengthMs*/) override {
        return -ENOTSUP;
    }
};

}  // namespace vibrator
}  // namespace hardware
}  // namespace android
}  // namespace aidl
