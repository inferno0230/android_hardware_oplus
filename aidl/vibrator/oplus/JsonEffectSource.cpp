/*
 * SPDX-FileCopyrightText: 2025-2026 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include "EffectSource.h"

#include <android-base/file.h>
#include <android-base/logging.h>
#include <json/json.h>

#include <sstream>
#include <unordered_map>
#include <vector>

/*
 * Example of a valid vibrator_effect.json:
 *
 * {
 *   "9999": {
 *     "def_style": [
 *       {
 *         "effect_file": "/odm/etc/vibrator/9999/def/effect_0.bin",
 *         "effect_id": 0,
 *         "play_rate_hz": 24000
 *       },
 *       ...
 *     ],
 *     "soft_style": [
 *       {
 *         "effect_file": "/odm/etc/vibrator/9999/soft/effect_0.bin",
 *         "effect_id": 0,
 *         "play_rate_hz": 24000
 *       },
 *       ...
 *     ]
 *   },
 *   ...
 * }
 *
 * The top level keys are vibrator types. soft_style only lists the effects that have a softer
 * variant, every other effect is shared with def_style.
 */

namespace aidl {
namespace android {
namespace hardware {
namespace vibrator {

namespace {

constexpr char kConfigPath[] = "/odm/etc/vibrator/vibrator_effect.json";

constexpr char kKeyDefStyle[] = "def_style";
constexpr char kKeySoftStyle[] = "soft_style";
constexpr char kKeyEffectFile[] = "effect_file";
constexpr char kKeyEffectId[] = "effect_id";
constexpr char kKeyPlayRateHz[] = "play_rate_hz";

class JsonEffectSource : public EffectSource {
  public:
    const effect_stream* get(uint32_t effectId) const override {
        auto it = mEffects.find(effectId);
        return it != mEffects.end() ? &it->second.stream : nullptr;
    }

    const std::string& description() const override { return mDescription; }

    bool load(const MotorInfo& motor);

  private:
    struct Entry {
        std::vector<int8_t> samples;
        effect_stream stream;
    };

    // Later calls replace the effects loaded by earlier ones.
    size_t loadStyle(const Json::Value& effects);

    std::unordered_map<uint32_t, Entry> mEffects;
    std::string mDescription;
};

size_t JsonEffectSource::loadStyle(const Json::Value& effects) {
    size_t loaded = 0;

    if (!effects.isArray()) {
        return loaded;
    }

    for (const auto& node : effects) {
        const auto& id = node[kKeyEffectId];
        const auto& rate = node[kKeyPlayRateHz];
        const auto& file = node[kKeyEffectFile];
        if (!id.isUInt() || !rate.isUInt() || !file.isString()) {
            LOG(ERROR) << "Skipping malformed effect entry";
            continue;
        }
        if (rate.asUInt() == 0) {
            LOG(ERROR) << "Skipping effect " << id.asUInt() << " without a play rate";
            continue;
        }

        std::string content;
        if (!::android::base::ReadFileToString(file.asString(), &content)) {
            PLOG(ERROR) << "Skipping effect " << id.asUInt() << ", cannot read " << file.asString();
            continue;
        }
        if (content.empty()) {
            LOG(ERROR) << "Skipping effect " << id.asUInt() << ", " << file.asString()
                       << " is empty";
            continue;
        }

        // The stream points into the vector's heap buffer, which stays put when the entry moves.
        Entry entry;
        entry.samples.assign(content.begin(), content.end());
        entry.stream = {
                .effect_id = id.asUInt(),
                .length = static_cast<uint32_t>(entry.samples.size()),
                .play_rate_hz = rate.asUInt(),
                .data = entry.samples.data(),
        };
        mEffects.insert_or_assign(id.asUInt(), std::move(entry));
        loaded++;
    }

    return loaded;
}

bool JsonEffectSource::load(const MotorInfo& motor) {
    std::string config;
    if (!::android::base::ReadFileToString(kConfigPath, &config)) {
        PLOG(INFO) << "Cannot read " << kConfigPath;
        return false;
    }

    Json::Value root;
    std::string errors;
    std::istringstream stream(config);
    if (!Json::parseFromStream(Json::CharReaderBuilder(), stream, &root, &errors)) {
        LOG(ERROR) << "Failed to parse " << kConfigPath << ": " << errors;
        return false;
    }
    if (!root.isObject()) {
        LOG(ERROR) << kConfigPath << ": root is not an object";
        return false;
    }

    // Prefer the table of the fitted motor. Kernels that cannot tell us the type are only found
    // on devices whose config has a single one.
    std::string type = std::to_string(motor.vibratorType);
    if (motor.vibratorType == MotorInfo::kTypeUnknown || !root[type].isObject()) {
        type.clear();
        for (const auto& name : root.getMemberNames()) {
            if (root[name].isObject()) {
                type = name;
                break;
            }
        }
        if (type.empty()) {
            LOG(ERROR) << kConfigPath << ": no vibrator type found";
            return false;
        }
        LOG(WARNING) << kConfigPath << ": no entry for vibrator type " << motor.vibratorType
                     << ", using the one for " << type;
    }

    const size_t def = loadStyle(root[type][kKeyDefStyle]);
    size_t soft = 0;
    if (motor.touchStyle == TouchStyle::SOFT) {
        soft = loadStyle(root[type][kKeySoftStyle]);
    }
    if (mEffects.empty()) {
        LOG(ERROR) << kConfigPath << ": no usable effect for vibrator type " << type;
        return false;
    }

    mDescription = std::string(kConfigPath) + " (type " + type + ", " + std::to_string(def) +
                   " default + " + std::to_string(soft) + " soft effects)";
    return true;
}

}  // anonymous namespace

std::unique_ptr<EffectSource> createJsonEffectSource(const MotorInfo& motor) {
    auto source = std::make_unique<JsonEffectSource>();
    if (!source->load(motor)) {
        return nullptr;
    }
    return source;
}

}  // namespace vibrator
}  // namespace hardware
}  // namespace android
}  // namespace aidl
