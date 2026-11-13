/*
 * Copyright (c) 2018-2021, The Linux Foundation. All rights reserved.
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-FileCopyrightText: 2026 The LineageOS Project
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "FfBackend.h"

#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/parseint.h>
#include <android-base/properties.h>
#include <android-base/stringprintf.h>
#include <android-base/strings.h>
#include <dirent.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <algorithm>

#include "EffectMap.h"

namespace aidl {
namespace android {
namespace hardware {
namespace vibrator {

namespace {

using ::android::base::unique_fd;

constexpr char kInputDir[] = "/dev/input";
constexpr char kPrimitiveDurationPath[] = "/sys/class/qcom-haptics/primitive_duration";

constexpr int16_t kStrongMagnitude = 0x7fff;
constexpr int16_t kMediumMagnitude = 0x5fff;
constexpr int16_t kLightMagnitude = 0x3fff;

// A predefined effect is requested with <id, 0, 0>, the kernel answers with
// <id, play time seconds, play time milliseconds>.
constexpr size_t kCustomDataLen = 3;
// Marks the id as one of the devicetree's primitives rather than one of its effects.
constexpr uint16_t kPrimitiveIdBit = 0x8000;

// The SoCs whose audio DSP can drive the haptics (audio coupled haptics).
constexpr int kExternalControlSocIds[] = {
        415,  // MSM_CPU_LAHAINA
        439,  // APQ_CPU_LAHAINA
        450,  // MSM_CPU_SHIMA
        501,  // MSM_CPU_SM8325
        502,  // APQ_CPU_SM8325P
        457,  // MSM_CPU_TARO
        475,  // MSM_CPU_YUPIK
        519,  // MSM_CPU_KALAMA
        557,  // MSM_CPU_PINEAPPLE
        618,  // MSM_CPU_SUN
        660,  // MSM_CPU_CANOE
};

bool testBit(size_t bit, const uint8_t* array) {
    return array[bit / 8] & (1 << (bit % 8));
}

int readSocId() {
    int soc = ::android::base::GetIntProperty("ro.vendor.qti.soc_id", -1);
    if (soc <= 0) {
        std::string value;
        if (::android::base::ReadFileToString("/sys/devices/soc0/soc_id", &value)) {
            ::android::base::ParseInt(::android::base::Trim(value), &soc);
        }
    }
    return soc;
}

// Maps [0.0, 1.0] onto the magnitudes of the three effect strengths, so that the low end can
// still be felt.
int16_t scaleToMagnitude(float scale) {
    scale = std::clamp(scale, 0.0f, 1.0f);
    return kLightMagnitude + static_cast<int16_t>(scale * (kStrongMagnitude - kLightMagnitude));
}

uint32_t streamLengthMs(const effect_stream* stream) {
    return static_cast<uint32_t>(static_cast<uint64_t>(stream->length) * 1000 /
                                 stream->play_rate_hz) +
           1;
}

}  // anonymous namespace

std::unique_ptr<FfBackend> FfBackend::probe() {
    std::unique_ptr<DIR, decltype(&closedir)> dir(opendir(kInputDir), closedir);
    if (!dir) {
        PLOG(ERROR) << "Cannot open " << kInputDir;
        return nullptr;
    }

    while (struct dirent* entry = readdir(dir.get())) {
        if (strncmp(entry->d_name, "event", 5) != 0) {
            continue;
        }

        const std::string path = std::string(kInputDir) + "/" + entry->d_name;
        unique_fd fd(TEMP_FAILURE_RETRY(open(path.c_str(), O_RDWR | O_CLOEXEC)));
        if (fd < 0) {
            PLOG(WARNING) << "Cannot open " << path;
            continue;
        }

        char name[32] = {};
        if (TEMP_FAILURE_RETRY(ioctl(fd, EVIOCGNAME(sizeof(name) - 1), name)) < 0) {
            PLOG(WARNING) << "Cannot get the name of " << path;
            continue;
        }
        if (strcmp(name, "qcom-hv-haptics") != 0 && strcmp(name, "qti-haptics") != 0) {
            continue;
        }

        uint8_t ffBits[FF_CNT / 8] = {};
        if (TEMP_FAILURE_RETRY(ioctl(fd, EVIOCGBIT(EV_FF, sizeof(ffBits)), ffBits)) < 0) {
            PLOG(WARNING) << "Cannot get the force feedback features of " << path;
            continue;
        }
        if (!testBit(FF_CONSTANT, ffBits) && !testBit(FF_PERIODIC, ffBits)) {
            continue;
        }

        LOG(INFO) << "Found " << name << " at " << path;
        return std::unique_ptr<FfBackend>(
                new FfBackend(std::move(fd), testBit(FF_GAIN, ffBits), testBit(FF_CUSTOM, ffBits)));
    }

    return nullptr;
}

FfBackend::FfBackend(unique_fd fd, bool supportsGain, bool supportsEffects)
    : mFd(std::move(fd)),
      mSupportsGain(supportsGain),
      mSupportsEffects(supportsEffects),
      mMagnitude(kStrongMagnitude) {
    const int soc = readSocId();
    mSupportsExternalControl =
            std::find(std::begin(kExternalControlSocIds), std::end(kExternalControlSocIds), soc) !=
            std::end(kExternalControlSocIds);

    mMotor = MotorInfo::detect();
    LOG(INFO) << "Vibrator type " << mMotor.vibratorType << ", "
              << (mMotor.touchStyle == TouchStyle::SOFT ? "soft" : "default") << " touch style";

    if (mSupportsEffects) {
        mSource = EffectSource::create(mMotor);
        resolveEffects();
        queryPrimitives();
    }
}

void FfBackend::resolveEffects() {
    for (const auto& entry : kEffectMap) {
        if (!mSource) {
            // Whether the devicetree has the effect only shows when it is played.
            mEffects.push_back({entry.effect, static_cast<uint32_t>(entry.ids[0]), nullptr});
            continue;
        }

        bool resolved = false;
        for (int32_t id : entry.ids) {
            if (id == EffectMapEntry::kNone) {
                continue;
            }
            const effect_stream* stream = mSource->get(id);
            if (!stream) {
                continue;
            }
            LOG(INFO) << "Effect " << toString(entry.effect) << " -> id " << id << ", "
                      << stream->length << " samples at " << stream->play_rate_hz << " Hz";
            mEffects.push_back({entry.effect, static_cast<uint32_t>(id), stream});
            resolved = true;
            break;
        }
        if (!resolved) {
            LOG(WARNING) << "No waveform for effect " << toString(entry.effect);
        }
    }
}

void FfBackend::queryPrimitives() {
    unique_fd fd(TEMP_FAILURE_RETRY(open(kPrimitiveDurationPath, O_RDWR | O_CLOEXEC)));
    if (fd < 0) {
        PLOG(WARNING) << "Cannot open " << kPrimitiveDurationPath << ", no primitives";
        return;
    }

    // Writing the id of a primitive makes the next read return its duration in microseconds.
    // The write fails for ids the devicetree does not define. NOOP needs no hardware.
    for (size_t id = static_cast<size_t>(CompositePrimitive::CLICK); id < kPrimitiveCount; id++) {
        const std::string request = std::to_string(id) + "\n";
        if (TEMP_FAILURE_RETRY(pwrite(fd, request.c_str(), request.size(), 0)) < 0) {
            continue;
        }

        char reply[16] = {};
        uint32_t durationUs = 0;
        if (TEMP_FAILURE_RETRY(pread(fd, reply, sizeof(reply) - 1, 0)) <= 0 ||
            !::android::base::ParseUint(::android::base::Trim(reply), &durationUs)) {
            LOG(WARNING) << "Cannot read the duration of primitive " << id;
            continue;
        }

        mPrimitiveDurationsMs[id] = (durationUs + 999) / 1000;
        LOG(INFO) << "Primitive " << toString(static_cast<CompositePrimitive>(id)) << ": "
                  << mPrimitiveDurationsMs[id] << " ms";
    }
}

int32_t FfBackend::capabilities() const {
    int32_t capabilities = IVibrator::CAP_ON_CALLBACK;

    if (mSupportsEffects) {
        capabilities |= IVibrator::CAP_PERFORM_CALLBACK;
    }
    if (mSupportsGain) {
        capabilities |= IVibrator::CAP_AMPLITUDE_CONTROL;
    }
    if (mSupportsExternalControl) {
        capabilities |= IVibrator::CAP_EXTERNAL_CONTROL;
    }
    if (std::any_of(mPrimitiveDurationsMs.begin(), mPrimitiveDurationsMs.end(),
                    [](int32_t duration) { return duration > 0; })) {
        capabilities |= IVibrator::CAP_COMPOSE_EFFECTS;
    }

    return capabilities;
}

int FfBackend::eraseLocked() {
    if (mSlot < 0) {
        return 0;
    }

    // This is also what stops the vibration.
    if (TEMP_FAILURE_RETRY(ioctl(mFd, EVIOCRMFF, mSlot)) < 0) {
        const int error = errno;
        // The kernel still owns the effect then. Hold on to the slot, there may not be another.
        PLOG(ERROR) << "EVIOCRMFF failed";
        return -error;
    }

    mSlot = -1;
    return 0;
}

int FfBackend::startLocked(struct ff_effect* effect) {
    if (int ret = eraseLocked(); ret != 0) {
        return ret;
    }

    effect->id = -1;
    effect->replay.delay = 0;
    if (TEMP_FAILURE_RETRY(ioctl(mFd, EVIOCSFF, effect)) < 0) {
        const int error = errno;
        PLOG(ERROR) << "EVIOCSFF failed";
        return -error;
    }
    mSlot = effect->id;

    struct input_event play = {};
    play.type = EV_FF;
    play.code = mSlot;
    play.value = 1;
    if (TEMP_FAILURE_RETRY(write(mFd, &play, sizeof(play))) != static_cast<ssize_t>(sizeof(play))) {
        const int error = errno;
        PLOG(ERROR) << "Starting the effect failed";
        eraseLocked();
        return -error;
    }

    return 0;
}

int FfBackend::startPredefinedLocked(int16_t id, int16_t magnitude, uint32_t* playLengthMs) {
    int16_t data[kCustomDataLen] = {id, 0, 0};

    struct ff_effect effect = {};
    effect.type = FF_PERIODIC;
    effect.u.periodic.waveform = FF_CUSTOM;
    effect.u.periodic.magnitude = magnitude;
    effect.u.periodic.custom_data = data;
    effect.u.periodic.custom_len = sizeof(data);

    if (int ret = startLocked(&effect); ret != 0) {
        return ret;
    }

    *playLengthMs = data[1] * 1000 + data[2];
    return 0;
}

int FfBackend::on(uint32_t timeoutMs, uint32_t* playLengthMs) {
    std::lock_guard<std::mutex> lock(mMutex);

    struct ff_effect effect = {};
    effect.type = FF_CONSTANT;
    effect.u.constant.level = mMagnitude;
    // The kernel stops by itself after this long, and it only has 16 bits for it.
    effect.replay.length = std::min<uint32_t>(timeoutMs, UINT16_MAX);

    if (int ret = startLocked(&effect); ret != 0) {
        return ret;
    }

    *playLengthMs = effect.replay.length;
    return 0;
}

int FfBackend::off() {
    std::lock_guard<std::mutex> lock(mMutex);

    // An amplitude only lasts until the vibration it was set for is turned off.
    mMagnitude = kStrongMagnitude;
    return eraseLocked();
}

int FfBackend::setAmplitude(float amplitude) {
    if (!mSupportsGain) {
        return -ENOTSUP;
    }

    std::lock_guard<std::mutex> lock(mMutex);

    const int16_t magnitude = scaleToMagnitude(amplitude);

    struct input_event gain = {};
    gain.type = EV_FF;
    gain.code = FF_GAIN;
    gain.value = magnitude;
    if (TEMP_FAILURE_RETRY(write(mFd, &gain, sizeof(gain))) != static_cast<ssize_t>(sizeof(gain))) {
        const int error = errno;
        PLOG(ERROR) << "Setting FF_GAIN failed";
        return -error;
    }

    mMagnitude = magnitude;
    return 0;
}

std::vector<Effect> FfBackend::supportedEffects() const {
    std::vector<Effect> effects;
    for (const auto& resolved : mEffects) {
        effects.push_back(resolved.effect);
    }
    return effects;
}

int FfBackend::perform(Effect effect, EffectStrength strength, uint32_t* playLengthMs) {
    const auto resolved = std::find_if(mEffects.begin(), mEffects.end(),
                                       [&](const auto& e) { return e.effect == effect; });
    if (resolved == mEffects.end()) {
        return -ENOTSUP;
    }

    int16_t magnitude;
    switch (strength) {
        case EffectStrength::LIGHT:
            magnitude = kLightMagnitude;
            break;
        case EffectStrength::MEDIUM:
            magnitude = kMediumMagnitude;
            break;
        case EffectStrength::STRONG:
            magnitude = kStrongMagnitude;
            break;
        default:
            return -ENOTSUP;
    }

    std::lock_guard<std::mutex> lock(mMutex);

    if (!resolved->stream) {
        return startPredefinedLocked(resolved->id, magnitude, playLengthMs);
    }

    // The kernel takes custom data of exactly this size as a description of FIFO samples
    // (struct custom_fifo_data), and only reads it.
    struct ff_effect ffEffect = {};
    ffEffect.type = FF_PERIODIC;
    ffEffect.u.periodic.waveform = FF_CUSTOM;
    ffEffect.u.periodic.magnitude = magnitude;
    ffEffect.u.periodic.custom_data =
            const_cast<int16_t*>(reinterpret_cast<const int16_t*>(resolved->stream));
    ffEffect.u.periodic.custom_len = sizeof(*resolved->stream);

    if (int ret = startLocked(&ffEffect); ret != 0) {
        return ret;
    }

    *playLengthMs = streamLengthMs(resolved->stream);
    return 0;
}

int32_t FfBackend::primitiveDurationMs(CompositePrimitive primitive) const {
    if (primitive == CompositePrimitive::NOOP) {
        return 0;
    }

    const size_t id = static_cast<size_t>(primitive);
    if (id >= kPrimitiveCount || mPrimitiveDurationsMs[id] <= 0) {
        return -1;
    }
    return mPrimitiveDurationsMs[id];
}

int FfBackend::playPrimitive(CompositePrimitive primitive, float scale, uint32_t* playLengthMs) {
    if (primitive == CompositePrimitive::NOOP) {
        *playLengthMs = 0;
        return 0;
    }
    const int32_t durationMs = primitiveDurationMs(primitive);
    if (durationMs < 0) {
        return -ENOTSUP;
    }

    std::lock_guard<std::mutex> lock(mMutex);

    if (int ret = startPredefinedLocked(
                static_cast<int16_t>(static_cast<uint16_t>(primitive) | kPrimitiveIdBit),
                scaleToMagnitude(scale), playLengthMs);
        ret != 0) {
        return ret;
    }

    // The kernel rounds the play time down to milliseconds here. Never be done sooner than the
    // duration that was advertised for the primitive.
    *playLengthMs = std::max<uint32_t>(*playLengthMs, durationMs);
    return 0;
}

void FfBackend::dump(std::string* out) const {
    using ::android::base::StringAppendF;

    StringAppendF(out, "  Vibrator type: %u\n", mMotor.vibratorType);
    StringAppendF(out, "  Touch style: %s\n",
                  mMotor.touchStyle == TouchStyle::SOFT ? "soft" : "default");
    StringAppendF(out, "  Effect source: %s\n",
                  mSource ? mSource->description().c_str() : "kernel devicetree");

    StringAppendF(out, "  Effects:\n");
    for (const auto& resolved : mEffects) {
        if (resolved.stream) {
            StringAppendF(out, "    %s: id %u, %u samples at %u Hz (%u ms)\n",
                          toString(resolved.effect).c_str(), resolved.id, resolved.stream->length,
                          resolved.stream->play_rate_hz, streamLengthMs(resolved.stream));
        } else {
            StringAppendF(out, "    %s: devicetree effect %u\n", toString(resolved.effect).c_str(),
                          resolved.id);
        }
    }

    StringAppendF(out, "  Primitives:\n");
    for (size_t id = 0; id < kPrimitiveCount; id++) {
        if (mPrimitiveDurationsMs[id] > 0) {
            StringAppendF(out, "    %s: %d ms\n",
                          toString(static_cast<CompositePrimitive>(id)).c_str(),
                          mPrimitiveDurationsMs[id]);
        }
    }
}

}  // namespace vibrator
}  // namespace hardware
}  // namespace android
}  // namespace aidl
