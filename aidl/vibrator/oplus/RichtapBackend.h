/*
 * SPDX-FileCopyrightText: 2025-2026 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>

#include "Backend.h"

namespace aidl {
namespace android {
namespace hardware {
namespace vibrator {

// Awinic haptic drivers, driven through AAC's proprietary Richtap library.
class RichtapBackend : public Backend {
  public:
    // Returns nullptr when the device has no such haptics or lacks the library.
    static std::unique_ptr<RichtapBackend> probe();

    const char* name() const override { return "richtap"; }
    int32_t capabilities() const override;

    int on(uint32_t timeoutMs, uint32_t* playLengthMs) override;
    int off() override;
    int setAmplitude(float amplitude) override;

    std::vector<Effect> supportedEffects() const override;
    int perform(Effect effect, EffectStrength strength, uint32_t* playLengthMs) override;

    void dump(std::string* out) const override;

  private:
    // The parts of libaacvibrator.so that are used here.
    struct Library {
        int (*init)(uint32_t* deviceType);
        int (*off)();
        int (*setAmplitude)(uint8_t amplitude);
        void (*looperStart)();
        // Both return the play time in milliseconds, or a negative value on failure.
        int32_t (*looperOn)(uint32_t timeoutMs);
        int32_t (*looperPrebakedEffect)(uint32_t effect, int32_t strength);
    };

    RichtapBackend(const Library& library, bool useSysfsOnOff, bool useSysfsGain);

    const Library mLibrary;
    // Let the kernel driver time plain on/off vibrations instead of the library.
    const bool mUseSysfsOnOff;
    // The driver's gain node can be used to scale those.
    const bool mUseSysfsGain;
    bool mGainChanged = false;
    uint32_t mDeviceType = 0;
    bool mInitialized = false;
};

}  // namespace vibrator
}  // namespace hardware
}  // namespace android
}  // namespace aidl
