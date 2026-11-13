/*
 * SPDX-FileCopyrightText: 2025-2026 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include "RichtapBackend.h"

#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/stringprintf.h>
#include <android-base/unique_fd.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#include <algorithm>

namespace aidl {
namespace android {
namespace hardware {
namespace vibrator {

namespace {

constexpr char kLibraryName[] = "libaacvibrator.so";

constexpr char kDevicePath[] = "/dev/awinic_haptic";
constexpr char kConfigPath[] = "/odm/etc/aac_richtap.config";

constexpr char kActivateNode[] = "/sys/class/leds/vibrator/oplus_activate";
constexpr char kDurationNode[] = "/sys/class/leds/vibrator/oplus_duration";
constexpr char kStateNode[] = "/sys/class/leds/vibrator/oplus_state";
// Scales what the driver plays by itself, from 0 to kGainMax. Applies to a running vibration too.
constexpr char kGainNode[] = "/sys/class/leds/vibrator/gain";
constexpr int kGainMax = 0x80;

constexpr int32_t kLightStrength = 69;
constexpr int32_t kMediumStrength = 89;
constexpr int32_t kStrongStrength = 99;

int writeNode(const char* path, const std::string& value) {
    ::android::base::unique_fd fd(TEMP_FAILURE_RETRY(open(path, O_WRONLY | O_CLOEXEC)));
    if (fd < 0 || !::android::base::WriteStringToFd(value, fd)) {
        const int error = errno;
        PLOG(ERROR) << "Cannot write " << value << " to " << path;
        return error ? -error : -EIO;
    }
    return 0;
}

template <typename T>
bool resolve(void* handle, const char* symbol, T* out) {
    *out = reinterpret_cast<T>(dlsym(handle, symbol));
    if (!*out) {
        LOG(ERROR) << kLibraryName << " lacks " << symbol;
        return false;
    }
    return true;
}

}  // anonymous namespace

std::unique_ptr<RichtapBackend> RichtapBackend::probe() {
    if (access(kDevicePath, F_OK) != 0) {
        return nullptr;
    }

    // Never closed, the library keeps its own threads around.
    void* handle = dlopen(kLibraryName, RTLD_NOW | RTLD_LOCAL);
    if (!handle) {
        LOG(ERROR) << "Found " << kDevicePath << " but cannot load " << kLibraryName << ": "
                   << dlerror();
        return nullptr;
    }

    // The looper functions are C++ and only exported under their mangled names.
    Library library;
    if (!resolve(handle, "aac_vibra_init", &library.init) ||
        !resolve(handle, "aac_vibra_off", &library.off) ||
        !resolve(handle, "aac_vibra_setAmplitude", &library.setAmplitude) ||
        !resolve(handle, "_Z22aac_vibra_looper_startv", &library.looperStart) ||
        !resolve(handle, "_Z19aac_vibra_looper_onj", &library.looperOn) ||
        !resolve(handle, "_Z32aac_vibra_looper_prebaked_effectji", &library.looperPrebakedEffect)) {
        return nullptr;
    }

    // Where the library looks for the driver and its tuning. The init script may override them.
    setenv("RICHTAP_DEVICE_PATH", kDevicePath, 0 /* overwrite */);
    setenv("ENV_RICHTAP_CONFIG_PATH", kConfigPath, 0 /* overwrite */);

    const bool useSysfsOnOff = access(kActivateNode, F_OK) == 0;
    return std::unique_ptr<RichtapBackend>(new RichtapBackend(
            library, useSysfsOnOff, useSysfsOnOff && access(kGainNode, W_OK) == 0));
}

RichtapBackend::RichtapBackend(const Library& library, bool useSysfsOnOff, bool useSysfsGain)
    : mLibrary(library), mUseSysfsOnOff(useSysfsOnOff), mUseSysfsGain(useSysfsGain) {
    if (mUseSysfsOnOff) {
        LOG(INFO) << "Using sysfs for on/off, " << (mUseSysfsGain ? "with" : "without")
                  << " amplitude control";
    }

    if (int ret = mLibrary.init(&mDeviceType); ret != 0) {
        LOG(ERROR) << "AAC init failed: " << ret;
        return;
    }
    mLibrary.looperStart();
    mInitialized = true;

    LOG(INFO) << "AAC init success, device type " << mDeviceType;
}

int32_t RichtapBackend::capabilities() const {
    int32_t capabilities = IVibrator::CAP_ON_CALLBACK | IVibrator::CAP_PERFORM_CALLBACK;

    if (!mUseSysfsOnOff || mUseSysfsGain) {
        capabilities |= IVibrator::CAP_AMPLITUDE_CONTROL;
    }

    return capabilities;
}

int RichtapBackend::on(uint32_t timeoutMs, uint32_t* playLengthMs) {
    if (mUseSysfsOnOff) {
        if (int ret = writeNode(kStateNode, "1"); ret != 0) {
            return ret;
        }
        if (int ret = writeNode(kDurationNode, std::to_string(timeoutMs)); ret != 0) {
            return ret;
        }
        if (int ret = writeNode(kActivateNode, "1"); ret != 0) {
            return ret;
        }

        *playLengthMs = timeoutMs;
        return 0;
    }

    if (!mInitialized) {
        return -ENODEV;
    }

    const int32_t ret = mLibrary.looperOn(timeoutMs);
    if (ret < 0) {
        LOG(ERROR) << "AAC on failed: " << ret;
        return -EIO;
    }

    *playLengthMs = ret;
    return 0;
}

int RichtapBackend::off() {
    if (mUseSysfsOnOff) {
        if (int ret = writeNode(kActivateNode, "0"); ret != 0) {
            return ret;
        }

        // An amplitude only lasts until the vibration it was set for is turned off. Leave the
        // gain alone otherwise, the library sets its own for the effects it plays.
        if (mGainChanged && writeNode(kGainNode, std::to_string(kGainMax)) == 0) {
            mGainChanged = false;
        }

        // Effects are played by the library, stop those as well.
        if (mInitialized) {
            if (int ret = mLibrary.off(); ret != 0) {
                LOG(WARNING) << "AAC off failed after sysfs off: " << ret;
            }
        }
        return 0;
    }

    if (!mInitialized) {
        return -ENODEV;
    }

    if (int ret = mLibrary.off(); ret != 0) {
        LOG(ERROR) << "AAC off failed: " << ret;
        return -EIO;
    }
    return 0;
}

int RichtapBackend::setAmplitude(float amplitude) {
    if (mUseSysfsOnOff) {
        if (!mUseSysfsGain) {
            return -ENOTSUP;
        }
        mGainChanged = true;
        return writeNode(kGainNode,
                         std::to_string(std::max(1, static_cast<int>(amplitude * kGainMax))));
    }
    if (!mInitialized) {
        return -ENODEV;
    }

    if (int ret = mLibrary.setAmplitude(static_cast<uint8_t>(amplitude * 0xff)); ret != 0) {
        LOG(ERROR) << "AAC set amplitude failed: " << ret;
        return -EIO;
    }
    return 0;
}

std::vector<Effect> RichtapBackend::supportedEffects() const {
    if (!mInitialized) {
        return {};
    }

    return {Effect::CLICK, Effect::DOUBLE_CLICK, Effect::TICK,
            Effect::THUD,  Effect::POP,          Effect::HEAVY_CLICK};
}

int RichtapBackend::perform(Effect effect, EffectStrength strength, uint32_t* playLengthMs) {
    if (!mInitialized) {
        return -ENODEV;
    }

    switch (effect) {
        case Effect::CLICK:
        case Effect::TICK:
            // Carried over from the only device on this backend so far, which preferred the
            // thud over Richtap's own click and tick.
            effect = Effect::THUD;
            break;
        case Effect::DOUBLE_CLICK:
        case Effect::THUD:
        case Effect::POP:
        case Effect::HEAVY_CLICK:
            break;
        default:
            return -ENOTSUP;
    }

    int32_t richtapStrength;
    switch (strength) {
        case EffectStrength::LIGHT:
            richtapStrength = kLightStrength;
            break;
        case EffectStrength::MEDIUM:
            richtapStrength = kMediumStrength;
            break;
        case EffectStrength::STRONG:
            richtapStrength = kStrongStrength;
            break;
        default:
            return -ENOTSUP;
    }

    const int32_t ret =
            mLibrary.looperPrebakedEffect(static_cast<uint32_t>(effect), richtapStrength);
    if (ret < 0) {
        LOG(ERROR) << "AAC perform failed: " << ret;
        return -EIO;
    }

    *playLengthMs = ret;
    return 0;
}

void RichtapBackend::dump(std::string* out) const {
    using ::android::base::StringAppendF;

    StringAppendF(out, "  AAC initialized: %s\n", mInitialized ? "yes" : "no");
    StringAppendF(out, "  AAC device type: %u\n", mDeviceType);
    StringAppendF(out, "  On/off through: %s\n", mUseSysfsOnOff ? "sysfs" : "AAC");
    StringAppendF(out, "  Amplitude through: %s\n",
                  !mUseSysfsOnOff ? "AAC"
                  : mUseSysfsGain ? "sysfs"
                                  : "none");
}

}  // namespace vibrator
}  // namespace hardware
}  // namespace android
}  // namespace aidl
