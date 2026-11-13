/*
 * SPDX-FileCopyrightText: 2026 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include "Vibrator.h"

#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/stringprintf.h>

#include <unistd.h>

#include <algorithm>

namespace aidl {
namespace android {
namespace hardware {
namespace vibrator {

namespace {

constexpr int32_t kComposeDelayMaxMs = 1000;
constexpr int32_t kComposeSizeMax = 256;

ndk::ScopedAStatus unsupported() {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus illegalArgument() {
    return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
}

// For errors of the hardware itself.
ndk::ScopedAStatus toStatus(int ret) {
    if (ret == 0) {
        return ndk::ScopedAStatus::ok();
    }
    if (ret == -ENOTSUP) {
        return unsupported();
    }
    return ndk::ScopedAStatus::fromExceptionCode(EX_SERVICE_SPECIFIC);
}

void notifyComplete(const std::shared_ptr<IVibratorCallback>& callback) {
    if (callback && !callback->onComplete().isOk()) {
        LOG(ERROR) << "Failed to call onComplete";
    }
}

}  // anonymous namespace

Vibrator::Vibrator(std::unique_ptr<Backend> backend)
    : mBackend(std::move(backend)),
      mCapabilities(mBackend->capabilities()),
      mSupportedEffects(mBackend->supportedEffects()) {
    if (mCapabilities & IVibrator::CAP_COMPOSE_EFFECTS) {
        for (CompositePrimitive primitive : ndk::enum_range<CompositePrimitive>()) {
            if (mBackend->primitiveDurationMs(primitive) >= 0) {
                mSupportedPrimitives.push_back(primitive);
            }
        }
    }

    LOG(INFO) << "Backend " << mBackend->name() << ", capabilities 0x" << std::hex << mCapabilities
              << std::dec << ", " << mSupportedEffects.size() << " effects, "
              << mSupportedPrimitives.size() << " primitives";
}

ndk::ScopedAStatus Vibrator::getCapabilities(int32_t* _aidl_return) {
    *_aidl_return = mCapabilities;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::off() {
    LOG(DEBUG) << "off";

    std::lock_guard<std::mutex> lock(mLock);

    // Whoever is still waiting for a vibration to end gets told now.
    mScheduler.cancel();
    mExternalControl = false;
    return toStatus(mBackend->off());
}

ndk::ScopedAStatus Vibrator::on(int32_t timeoutMs,
                                const std::shared_ptr<IVibratorCallback>& callback) {
    LOG(DEBUG) << "on for " << timeoutMs << " ms";

    if (timeoutMs <= 0) {
        return illegalArgument();
    }

    std::lock_guard<std::mutex> lock(mLock);

    mScheduler.cancel();

    uint32_t playLengthMs = 0;
    if (int ret = mBackend->on(timeoutMs, &playLengthMs); ret != 0) {
        return toStatus(ret);
    }

    if (callback) {
        mScheduler.schedule(playLengthMs, [callback] { notifyComplete(callback); });
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::perform(Effect effect, EffectStrength strength,
                                     const std::shared_ptr<IVibratorCallback>& callback,
                                     int32_t* _aidl_return) {
    LOG(DEBUG) << "perform " << toString(effect) << " " << toString(strength);

    if (std::find(mSupportedEffects.begin(), mSupportedEffects.end(), effect) ==
        mSupportedEffects.end()) {
        return unsupported();
    }
    if (strength != EffectStrength::LIGHT && strength != EffectStrength::MEDIUM &&
        strength != EffectStrength::STRONG) {
        return unsupported();
    }

    std::lock_guard<std::mutex> lock(mLock);

    mScheduler.cancel();

    uint32_t playLengthMs = 0;
    if (int ret = mBackend->perform(effect, strength, &playLengthMs); ret != 0) {
        return toStatus(ret);
    }

    if (callback) {
        mScheduler.schedule(playLengthMs, [callback] { notifyComplete(callback); });
    }
    *_aidl_return = playLengthMs;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::getSupportedEffects(std::vector<Effect>* _aidl_return) {
    *_aidl_return = mSupportedEffects;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::setAmplitude(float amplitude) {
    LOG(DEBUG) << "setAmplitude " << amplitude;

    if (!(mCapabilities & IVibrator::CAP_AMPLITUDE_CONTROL)) {
        return unsupported();
    }
    if (amplitude <= 0.0f || amplitude > 1.0f) {
        return illegalArgument();
    }

    std::lock_guard<std::mutex> lock(mLock);

    // Whatever drives the haptics now is not ours to scale.
    if (mExternalControl) {
        return unsupported();
    }
    return toStatus(mBackend->setAmplitude(amplitude));
}

ndk::ScopedAStatus Vibrator::setExternalControl(bool enabled) {
    LOG(DEBUG) << "setExternalControl " << enabled;

    if (!(mCapabilities & IVibrator::CAP_EXTERNAL_CONTROL)) {
        return unsupported();
    }

    std::lock_guard<std::mutex> lock(mLock);

    // The haptics change hands: whatever we were playing ends here, and when control comes
    // back the vibrator has to be off.
    mScheduler.cancel();
    if (int ret = mBackend->off(); ret != 0) {
        return toStatus(ret);
    }

    mExternalControl = enabled;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::getCompositionDelayMax(int32_t* maxDelayMs) {
    if (!(mCapabilities & IVibrator::CAP_COMPOSE_EFFECTS)) {
        return unsupported();
    }

    *maxDelayMs = kComposeDelayMaxMs;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::getCompositionSizeMax(int32_t* maxSize) {
    if (!(mCapabilities & IVibrator::CAP_COMPOSE_EFFECTS)) {
        return unsupported();
    }

    *maxSize = kComposeSizeMax;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::getSupportedPrimitives(std::vector<CompositePrimitive>* supported) {
    if (!(mCapabilities & IVibrator::CAP_COMPOSE_EFFECTS)) {
        return unsupported();
    }

    *supported = mSupportedPrimitives;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::getPrimitiveDuration(CompositePrimitive primitive,
                                                  int32_t* durationMs) {
    if (std::find(mSupportedPrimitives.begin(), mSupportedPrimitives.end(), primitive) ==
        mSupportedPrimitives.end()) {
        return unsupported();
    }

    *durationMs = mBackend->primitiveDurationMs(primitive);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::compose(const std::vector<CompositeEffect>& composite,
                                     const std::shared_ptr<IVibratorCallback>& callback) {
    LOG(DEBUG) << "compose " << composite.size() << " primitives";

    if (!(mCapabilities & IVibrator::CAP_COMPOSE_EFFECTS)) {
        return unsupported();
    }
    if (composite.size() > static_cast<size_t>(kComposeSizeMax)) {
        return illegalArgument();
    }
    for (const auto& element : composite) {
        if (element.delayMs < 0 || element.delayMs > kComposeDelayMaxMs) {
            return illegalArgument();
        }
        if (!(element.scale >= 0.0f && element.scale <= 1.0f)) {
            return illegalArgument();
        }
        if (std::find(mSupportedPrimitives.begin(), mSupportedPrimitives.end(),
                      element.primitive) == mSupportedPrimitives.end()) {
            return unsupported();
        }
    }

    std::lock_guard<std::mutex> lock(mLock);

    mScheduler.cancel();
    mScheduler.post([this, composite, callback](const CallbackScheduler::Context& context) {
        for (const auto& element : composite) {
            if (element.delayMs > 0 && !context.sleepFor(element.delayMs)) {
                break;
            }

            uint32_t playLengthMs = 0;
            {
                // Whoever cancels does so with the lock held and then takes care of the hardware
                // (off() stops it, a new vibration replaces what is playing). Checking under
                // the same lock keeps a primitive from starting behind their back.
                std::lock_guard<std::mutex> lock(mLock);
                if (context.cancelled()) {
                    break;
                }
                if (int ret = mBackend->playPrimitive(element.primitive, element.scale,
                                                      &playLengthMs);
                    ret != 0) {
                    LOG(ERROR) << "Failed to play " << toString(element.primitive) << ": " << ret;
                    break;
                }
            }

            if (!context.sleepFor(playLengthMs)) {
                break;
            }
        }

        notifyComplete(callback);
    });

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::getSupportedAlwaysOnEffects(std::vector<Effect>* /*_aidl_return*/) {
    return unsupported();
}

ndk::ScopedAStatus Vibrator::alwaysOnEnable(int32_t /*id*/, Effect /*effect*/,
                                            EffectStrength /*strength*/) {
    return unsupported();
}

ndk::ScopedAStatus Vibrator::alwaysOnDisable(int32_t /*id*/) {
    return unsupported();
}

ndk::ScopedAStatus Vibrator::getResonantFrequency(float* /*resonantFreqHz*/) {
    return unsupported();
}

ndk::ScopedAStatus Vibrator::getQFactor(float* /*qFactor*/) {
    return unsupported();
}

ndk::ScopedAStatus Vibrator::getFrequencyResolution(float* /*freqResolutionHz*/) {
    return unsupported();
}

ndk::ScopedAStatus Vibrator::getFrequencyMinimum(float* /*freqMinimumHz*/) {
    return unsupported();
}

ndk::ScopedAStatus Vibrator::getBandwidthAmplitudeMap(std::vector<float>* /*_aidl_return*/) {
    return unsupported();
}

ndk::ScopedAStatus Vibrator::getPwlePrimitiveDurationMax(int32_t* /*durationMs*/) {
    return unsupported();
}

ndk::ScopedAStatus Vibrator::getPwleCompositionSizeMax(int32_t* /*maxSize*/) {
    return unsupported();
}

ndk::ScopedAStatus Vibrator::getSupportedBraking(std::vector<Braking>* /*supported*/) {
    return unsupported();
}

ndk::ScopedAStatus Vibrator::composePwle(const std::vector<PrimitivePwle>& /*composite*/,
                                         const std::shared_ptr<IVibratorCallback>& /*callback*/) {
    return unsupported();
}

binder_status_t Vibrator::dump(int fd, const char** /*args*/, uint32_t /*numArgs*/) {
    using ::android::base::StringAppendF;

    std::string out;
    StringAppendF(&out, "Backend: %s\n", mBackend->name());
    StringAppendF(&out, "  Capabilities: 0x%x\n", mCapabilities);
    mBackend->dump(&out);

    if (!::android::base::WriteStringToFd(out, fd)) {
        return STATUS_UNKNOWN_ERROR;
    }
    fsync(fd);
    return STATUS_OK;
}

}  // namespace vibrator
}  // namespace hardware
}  // namespace android
}  // namespace aidl
