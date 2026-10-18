/*
 * SPDX-FileCopyrightText: The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include <dlfcn.h>
#include <string.h>

#include <hardware/audio_effect.h>
#include <log/log.h>
#include <system/audio.h>

namespace {

#ifdef BACKEND_LIB_NAME
constexpr auto kLibPath = "/odm/lib64/soundfx/" BACKEND_LIB_NAME;
#else
constexpr auto kLibPath = "/odm/lib64/soundfx/liboplus_spatializer.so";
#endif

// The OPLUS framework sends this vendor parameter to tell the effect how many input
// channels it should render. Until it is set the effect outputs silence.
constexpr int32_t kParamMaxChannelCount = 0x100100ba;

struct WrappedEffect {
    const struct effect_interface_s* itfe;
    effect_handle_t backend;
};

audio_effect_library_t* gBackendLib = nullptr;

bool loadBackendLib() {
    if (gBackendLib != nullptr) return true;

    void* handle = dlopen(kLibPath, RTLD_NOW);
    if (handle == nullptr) {
        ALOGE("dlopen %s failed: %s", kLibPath, dlerror());
        return false;
    }

    gBackendLib = static_cast<audio_effect_library_t*>(
            dlsym(handle, AUDIO_EFFECT_LIBRARY_INFO_SYM_AS_STR));
    if (gBackendLib == nullptr) {
        ALOGE("%s is missing from %s", AUDIO_EFFECT_LIBRARY_INFO_SYM_AS_STR, kLibPath);
        dlclose(handle);
        return false;
    }

    return true;
}

void setMaxChannelCount(effect_handle_t backend, const effect_config_t* config) {
    int32_t channels = audio_channel_count_from_out_mask(config->inputCfg.channels);

    uint32_t buf[sizeof(effect_param_t) / sizeof(uint32_t) + 2];
    auto* p = reinterpret_cast<effect_param_t*>(buf);
    p->psize = sizeof(int32_t);
    p->vsize = sizeof(int32_t);
    memcpy(p->data, &kParamMaxChannelCount, sizeof(int32_t));
    memcpy(p->data + sizeof(int32_t), &channels, sizeof(int32_t));

    int32_t reply = 0;
    uint32_t replySize = sizeof(reply);
    int ret = (*backend)->command(backend, EFFECT_CMD_SET_PARAM, sizeof(buf), p, &replySize,
                                  &reply);
    ALOGI("Set max channel count to %d: ret %d reply %d", channels, ret, reply);
}

int wrapperProcess(effect_handle_t self, audio_buffer_t* in, audio_buffer_t* out) {
    auto* e = reinterpret_cast<WrappedEffect*>(self);
    return (*e->backend)->process(e->backend, in, out);
}

int wrapperCommand(effect_handle_t self, uint32_t cmdCode, uint32_t cmdSize, void* pCmdData,
                   uint32_t* replySize, void* pReplyData) {
    auto* e = reinterpret_cast<WrappedEffect*>(self);
    int ret = (*e->backend)->command(e->backend, cmdCode, cmdSize, pCmdData, replySize,
                                     pReplyData);

    if (cmdCode == EFFECT_CMD_SET_CONFIG && ret == 0 && pCmdData != nullptr &&
        cmdSize >= sizeof(effect_config_t) && pReplyData != nullptr &&
        *static_cast<int32_t*>(pReplyData) == 0) {
        setMaxChannelCount(e->backend, static_cast<effect_config_t*>(pCmdData));
    }

    return ret;
}

int wrapperGetDescriptor(effect_handle_t self, effect_descriptor_t* desc) {
    auto* e = reinterpret_cast<WrappedEffect*>(self);
    return (*e->backend)->get_descriptor(e->backend, desc);
}

int wrapperProcessReverse(effect_handle_t self, audio_buffer_t* in, audio_buffer_t* out) {
    auto* e = reinterpret_cast<WrappedEffect*>(self);
    if ((*e->backend)->process_reverse == nullptr) return -ENOSYS;
    return (*e->backend)->process_reverse(e->backend, in, out);
}

const struct effect_interface_s kWrapperInterface = {
        wrapperProcess,
        wrapperCommand,
        wrapperGetDescriptor,
        wrapperProcessReverse,
};

int createEffect(const effect_uuid_t* uuid, int32_t sessionId, int32_t ioId,
                 effect_handle_t* pHandle) {
    if (pHandle == nullptr || !loadBackendLib()) return -EINVAL;

    effect_handle_t backend = nullptr;
    int ret = gBackendLib->create_effect(uuid, sessionId, ioId, &backend);
    if (ret != 0) return ret;

    auto* e = new WrappedEffect{&kWrapperInterface, backend};
    *pHandle = reinterpret_cast<effect_handle_t>(e);
    return 0;
}

int releaseEffect(effect_handle_t handle) {
    if (handle == nullptr || gBackendLib == nullptr) return -EINVAL;

    auto* e = reinterpret_cast<WrappedEffect*>(handle);
    int ret = gBackendLib->release_effect(e->backend);
    delete e;
    return ret;
}

int getDescriptor(const effect_uuid_t* uuid, effect_descriptor_t* pDescriptor) {
    if (!loadBackendLib()) return -EINVAL;
    return gBackendLib->get_descriptor(uuid, pDescriptor);
}

}  // namespace

extern "C" __attribute__((visibility("default"))) audio_effect_library_t AUDIO_EFFECT_LIBRARY_INFO_SYM = {
        .tag = AUDIO_EFFECT_LIBRARY_TAG,
        .version = EFFECT_LIBRARY_API_VERSION,
        .name = "OPLUS Spatializer Wrapper",
        .implementor = "The LineageOS Project",
        .create_effect = createEffect,
        .release_effect = releaseEffect,
        .get_descriptor = getDescriptor,
};
