/*
 * SPDX-FileCopyrightText: 2026 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include "EffectSource.h"

#include <android-base/logging.h>
#include <dlfcn.h>

namespace aidl {
namespace android {
namespace hardware {
namespace vibrator {

namespace {

/*
 * Devices without vibrator_effect.json have their waveforms compiled into the stock
 * libqtivibratoreffect.so, as one table per vibrator type and touch style with a C++ lookup
 * function each. The first name is the one device trees install the blob under, since Soong
 * already has an open source module called libqtivibratoreffect.
 */
constexpr const char* kLibraries[] = {
        "libqtivibratoreffect-oplus.so",
        "libqtivibratoreffect.so",
};

// const effect_stream* get_effect_stream(unsigned int): a generic table, not tuned for any of
// the motors below.
constexpr char kGenericLookup[] = "_Z17get_effect_streamj";

constexpr struct {
    uint32_t vibratorType;
    const char* defaultLookup;
    const char* softLookup;
} kLookups[] = {
        {809, "_Z22get_effect_stream_0809j", "_Z27get_effect_stream_soft_0809j"},
        {8151, "_Z25get_effect_stream_sla0815j", "_Z22get_effect_stream_softj"},
        {9999, "_Z22get_effect_stream_9999j", "_Z27get_effect_stream_soft_9999j"},
};

constexpr uint32_t kPlayRateMinHz = 1000;
constexpr uint32_t kPlayRateMaxHz = 48000;

using LookupFn = const effect_stream* (*)(uint32_t);

// A blob built against another table layout would hand out garbage, catch that.
bool isSane(const effect_stream* stream) {
    return stream->length > 0 && stream->data != nullptr &&
           stream->play_rate_hz >= kPlayRateMinHz && stream->play_rate_hz <= kPlayRateMaxHz;
}

class BlobEffectSource : public EffectSource {
  public:
    const effect_stream* get(uint32_t effectId) const override {
        for (LookupFn lookup : {mSoftLookup, mDefaultLookup}) {
            if (!lookup) {
                continue;
            }
            const effect_stream* stream = lookup(effectId);
            if (!stream) {
                continue;
            }
            if (!isSane(stream)) {
                LOG(WARNING) << "Ignoring implausible stream for effect " << effectId;
                continue;
            }
            return stream;
        }
        return nullptr;
    }

    const std::string& description() const override { return mDescription; }

    bool load(const MotorInfo& motor);

  private:
    LookupFn mDefaultLookup = nullptr;
    // Only knows the effects that have a softer variant.
    LookupFn mSoftLookup = nullptr;
    std::string mDescription;
};

bool BlobEffectSource::load(const MotorInfo& motor) {
    // Never closed, the tables are used for as long as the service runs.
    void* handle = nullptr;
    const char* library = nullptr;
    for (const char* name : kLibraries) {
        handle = dlopen(name, RTLD_NOW | RTLD_LOCAL);
        if (handle) {
            library = name;
            break;
        }
        LOG(INFO) << "Cannot load " << name << ": " << dlerror();
    }
    if (!handle) {
        return false;
    }

    const char* defaultSymbol = nullptr;
    const char* softSymbol = nullptr;
    for (const auto& lookup : kLookups) {
        if (lookup.vibratorType == motor.vibratorType) {
            defaultSymbol = lookup.defaultLookup;
            softSymbol = lookup.softLookup;
            break;
        }
    }

    const char* symbol = defaultSymbol;
    if (symbol) {
        mDefaultLookup = reinterpret_cast<LookupFn>(dlsym(handle, symbol));
    }
    if (!mDefaultLookup) {
        LOG(WARNING) << library << " has no table for vibrator type " << motor.vibratorType
                     << ", using the generic one";
        symbol = kGenericLookup;
        softSymbol = nullptr;
        mDefaultLookup = reinterpret_cast<LookupFn>(dlsym(handle, symbol));
    }
    if (!mDefaultLookup) {
        LOG(ERROR) << library << " has no effect table at all";
        return false;
    }

    mDescription = std::string(library) + " (" + symbol;
    if (motor.touchStyle == TouchStyle::SOFT && softSymbol) {
        mSoftLookup = reinterpret_cast<LookupFn>(dlsym(handle, softSymbol));
        if (mSoftLookup) {
            mDescription += std::string(" + ") + softSymbol;
        }
    }
    mDescription += ")";
    return true;
}

}  // anonymous namespace

std::unique_ptr<EffectSource> createBlobEffectSource(const MotorInfo& motor) {
    auto source = std::make_unique<BlobEffectSource>();
    if (!source->load(motor)) {
        return nullptr;
    }
    return source;
}

}  // namespace vibrator
}  // namespace hardware
}  // namespace android
}  // namespace aidl
