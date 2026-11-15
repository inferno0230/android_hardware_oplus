/*
 * Copyright (c) 2018-2021, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 * Copyright (c) 2022-2024, Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#define LOG_TAG "vendor.qti.vibrator"

#include <cutils/properties.h>
#include <dirent.h>
#include <fcntl.h>
#include <inttypes.h>
#include <linux/input.h>
#include <log/log.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <algorithm>
#include <string>
#include <thread>

#include "include/Vibrator.h"
#ifdef USE_EFFECT_STREAM
#include "effect/effect.h"
#endif

namespace aidl {
namespace android {
namespace hardware {
namespace vibrator {

#define STRONG_MAGNITUDE 0x7fff
#define MEDIUM_MAGNITUDE 0x5fff
#define LIGHT_MAGNITUDE 0x3fff
#define INVALID_VALUE -1
#define CUSTOM_DATA_LEN 3
#define NAME_BUF_SIZE 32
#define PRIMITIVE_ID_MASK 0x8000
#define MAX_PATTERN_ID 32767

#define MSM_CPU_LAHAINA 415
#define APQ_CPU_LAHAINA 439
#define MSM_CPU_SHIMA 450
#define MSM_CPU_SM8325 501
#define APQ_CPU_SM8325P 502
#define MSM_CPU_YUPIK 475
#define MSM_CPU_CAPE 530
#define APQ_CPU_CAPE 531
#define MSM_CPU_TARO 457
#define MSM_CPU_TARO_LTE 552
#define MSM_CPU_KALAMA 519
#define QCS_CPU_KALAMAP 603
#define MSM_CPU_PINEAPPLE 557
#define MSM_CPU_SUN 618
#define MSM_CPU_CANOE 660

#define test_bit(bit, array) ((array)[(bit) / 8] & (1 << ((bit) % 8)))

#define LED_DEVICE "/sys/class/leds/vibrator"
#define HAPTICS_SYSFS "/sys/class/qcom-haptics"

static constexpr int32_t ComposeDelayMaxMs = 1000;
static constexpr int32_t ComposeSizeMax = 256;

enum composeEvent {
    STOP_COMPOSE = 0,
};

InputFFDevice::InputFFDevice() {
    DIR* dp;
    FILE* fp = NULL;
    struct dirent* dir;
    uint8_t ffBitmask[FF_CNT / 8];
    char devicename[PATH_MAX];
    const char* INPUT_DIR = "/dev/input/";
    char name[NAME_BUF_SIZE];
    int fd, ret;
    int soc = property_get_int32("ro.vendor.qti.soc_id", -1);

    mVibraFd = INVALID_VALUE;
    mSupportGain = false;
    mSupportEffects = false;
    mSupportExternalControl = false;
    mCurrAppId = INVALID_VALUE;
    mCurrMagnitude = 0x7fff;
    mInExternalControl = false;

    dp = opendir(INPUT_DIR);
    if (!dp) {
        ALOGE("open %s failed, errno = %d", INPUT_DIR, errno);
        return;
    }

    memset(ffBitmask, 0, sizeof(ffBitmask));
    while ((dir = readdir(dp)) != NULL) {
        if (dir->d_name[0] == '.' &&
            (dir->d_name[1] == '\0' || (dir->d_name[1] == '.' && dir->d_name[2] == '\0')))
            continue;

        snprintf(devicename, PATH_MAX, "%s%s", INPUT_DIR, dir->d_name);
        fd = TEMP_FAILURE_RETRY(open(devicename, O_RDWR));
        if (fd < 0) {
            ALOGE("open %s failed, errno = %d", devicename, errno);
            continue;
        }

        ret = TEMP_FAILURE_RETRY(ioctl(fd, EVIOCGNAME(sizeof(name)), name));
        if (ret == -1) {
            ALOGE("get input device name %s failed, errno = %d\n", devicename, errno);
            close(fd);
            continue;
        }

        if (strcmp(name, "qcom-hv-haptics") && strcmp(name, "qti-haptics") &&
            strcmp(name, "aw-haptic-hv") && strcmp(name, "aw8624_haptic") &&
            strcmp(name, "aw8695_haptic") && strcmp(name, "aw8697_haptic") &&
            strcmp(name, "awinic_haptic") && strcmp(name, "drv260x:haptics") &&
            strcmp(name, "drv2624:haptics") && strcmp(name, "haptic_rt") &&
            strcmp(name, "si_haptic")) {
            ALOGD("not a supported haptics device\n");
            close(fd);
            continue;
        }

        ALOGI("%s is detected at %s\n", name, devicename);
        ret = TEMP_FAILURE_RETRY(ioctl(fd, EVIOCGBIT(EV_FF, sizeof(ffBitmask)), ffBitmask));
        if (ret == -1) {
            ALOGE("ioctl failed, errno = %d", errno);
            close(fd);
            continue;
        }

        if (test_bit(FF_CONSTANT, ffBitmask) || test_bit(FF_PERIODIC, ffBitmask)) {
            mVibraFd = fd;
            if (test_bit(FF_CUSTOM, ffBitmask)) mSupportEffects = true;
            if (test_bit(FF_GAIN, ffBitmask)) mSupportGain = true;

            if (soc <= 0 && (fp = fopen("/sys/devices/soc0/soc_id", "r")) != NULL) {
                fscanf(fp, "%u", &soc);
                fclose(fp);
            }
            switch (soc) {
                case MSM_CPU_LAHAINA:
                case APQ_CPU_LAHAINA:
                case MSM_CPU_SHIMA:
                case MSM_CPU_SM8325:
                case APQ_CPU_SM8325P:
                case MSM_CPU_TARO:
                case MSM_CPU_TARO_LTE:
                case MSM_CPU_YUPIK:
                case MSM_CPU_CAPE:
                case APQ_CPU_CAPE:
                case MSM_CPU_KALAMA:
                case QCS_CPU_KALAMAP:
                case MSM_CPU_PINEAPPLE:
                case MSM_CPU_SUN:
                case MSM_CPU_CANOE:
                    mSupportExternalControl = true;
                    break;
                default:
                    mSupportExternalControl = false;
                    break;
            }
            break;
        }

        close(fd);
    }

    closedir(dp);
}

bool InputFFDevice::isPresent() {
    return (mVibraFd != INVALID_VALUE);
}

/** Play vibration
 *
 *  @param effectId:  ID of the predefined effect will be played. If effectId is valid
 *                    (non-negative value), the timeoutMs value will be ignored, and the
 *                    real playing length will be set in param@playLengtMs and returned
 *                    to VibratorService. If effectId is invalid, value in param@timeoutMs
 *                    will be used as the play length for playing a constant effect.
 *  @param timeoutMs: playing length, non-zero means playing, zero means stop playing.
 *  @param playLengthMs: the playing length in ms unit which will be returned to
 *                    VibratorService if the request is playing a predefined effect.
 *                    The custom_data in periodic is reused for returning the playLengthMs
 *                    from kernel space to userspace if the pattern is defined in kernel
 *                    driver. It's been defined with following format:
 *                       <effect-ID, play-time-in-seconds, play-time-in-milliseconds>.
 *                    The effect-ID is used for passing down the predefined effect to
 *                    kernel driver, and the rest two parameters are used for returning
 *                    back the real playing length from kernel driver.
 */
int InputFFDevice::play(int effectId, uint32_t timeoutMs, long* playLengthMs) {
    struct ff_effect effect;
    struct input_event play;
    int16_t data[CUSTOM_DATA_LEN] = {0, 0, 0};
    int ret;
#ifdef USE_EFFECT_STREAM
    const struct effect_stream* stream = NULL;
#endif

    std::lock_guard<std::mutex> lock(mtx);

    /* For QMAA compliance, return OK even if vibrator device doesn't exist */
    if (!isPresent()) {
        if (playLengthMs != NULL) *playLengthMs = 0;
        return 0;
    }

    if (timeoutMs != 0) {
        if (mCurrAppId != INVALID_VALUE) {
            ret = TEMP_FAILURE_RETRY(ioctl(mVibraFd, EVIOCRMFF, mCurrAppId));
            if (ret == -1) {
                ALOGE("ioctl EVIOCRMFF failed, errno = %d", -errno);
                goto errout;
            }
            mCurrAppId = INVALID_VALUE;
        }

        memset(&effect, 0, sizeof(effect));
        if (effectId != INVALID_VALUE) {
            data[0] = effectId;
            effect.type = FF_PERIODIC;
            effect.u.periodic.waveform = FF_CUSTOM;
            effect.u.periodic.magnitude = mCurrMagnitude;
            effect.u.periodic.custom_data = data;
            effect.u.periodic.custom_len = sizeof(int16_t) * CUSTOM_DATA_LEN;
#ifdef USE_EFFECT_STREAM
            stream = get_effect_stream(effectId);
            if (stream != NULL) {
                effect.u.periodic.custom_data = (int16_t*)stream;
                effect.u.periodic.custom_len = sizeof(*stream);
            }
#endif
        } else {
            effect.type = FF_CONSTANT;
            effect.u.constant.level = mCurrMagnitude;
            effect.replay.length = timeoutMs;
        }

        effect.id = mCurrAppId;
        effect.replay.delay = 0;

        ret = TEMP_FAILURE_RETRY(ioctl(mVibraFd, EVIOCSFF, &effect));
        if (ret == -1) {
            ALOGE("ioctl EVIOCSFF failed, errno = %d", -errno);
            goto errout;
        }

        mCurrAppId = effect.id;
        if (effectId != INVALID_VALUE && playLengthMs != NULL) {
            *playLengthMs = data[1] * 1000 + data[2];
#ifdef USE_EFFECT_STREAM
            if (stream != NULL && stream->play_rate_hz != 0)
                *playLengthMs = ((stream->length * 1000) / stream->play_rate_hz) + 1;
#endif
        }

        play.value = 1;
        play.type = EV_FF;
        play.code = mCurrAppId;
        play.time.tv_sec = 0;
        play.time.tv_usec = 0;
        ret = TEMP_FAILURE_RETRY(write(mVibraFd, (const void*)&play, sizeof(play)));
        if (ret == -1) {
            ALOGE("write failed, errno = %d\n", -errno);
            ret = TEMP_FAILURE_RETRY(ioctl(mVibraFd, EVIOCRMFF, mCurrAppId));
            if (ret == -1) ALOGE("ioctl EVIOCRMFF failed, errno = %d", -errno);
            goto errout;
        }
    } else if (mCurrAppId != INVALID_VALUE) {
        ret = TEMP_FAILURE_RETRY(ioctl(mVibraFd, EVIOCRMFF, mCurrAppId));
        if (ret == -1) {
            ALOGE("ioctl EVIOCRMFF failed, errno = %d", -errno);
            goto errout;
        }
        mCurrAppId = INVALID_VALUE;
    }
    return 0;

errout:
    mCurrAppId = INVALID_VALUE;
    return ret;
}

int InputFFDevice::on(int32_t timeoutMs) {
    return play(INVALID_VALUE, timeoutMs, NULL);
}

int InputFFDevice::off() {
    return play(INVALID_VALUE, 0, NULL);
}

int InputFFDevice::setAmplitude(uint8_t amplitude) {
    int32_t tmp;
    int ret;
    struct input_event ie;

    /* For QMAA compliance, return OK even if vibrator device doesn't exist */
    if (!isPresent()) return 0;

    tmp = amplitude * STRONG_MAGNITUDE / 255;
    ie.type = EV_FF;
    ie.code = FF_GAIN;
    ie.value = tmp;

    ret = TEMP_FAILURE_RETRY(write(mVibraFd, &ie, sizeof(ie)));
    if (ret == -1) {
        ALOGE("write FF_GAIN failed, errno = %d", -errno);
        return ret;
    }

    mCurrMagnitude = tmp;
    return 0;
}

int InputFFDevice::playEffect(int effectId, EffectStrength es, long* playLengthMs) {
    if (effectId > MAX_PATTERN_ID) {
        ALOGE("effect id %d exceeds %d", effectId, MAX_PATTERN_ID);
        return -1;
    }

    switch (es) {
        case EffectStrength::LIGHT:
            mCurrMagnitude = LIGHT_MAGNITUDE;
            break;
        case EffectStrength::MEDIUM:
            mCurrMagnitude = MEDIUM_MAGNITUDE;
            break;
        case EffectStrength::STRONG:
            mCurrMagnitude = STRONG_MAGNITUDE;
            break;
        default:
            return -1;
    }

    return play(effectId, INVALID_VALUE, playLengthMs);
}

int InputFFDevice::playPrimitive(int primitiveId, float amplitude, long* playLengthMs) {
    int32_t tmp;
    int ret = 0;

    if (primitiveId > MAX_PATTERN_ID) {
        ALOGE("primitive id %d exceeds %d", primitiveId, MAX_PATTERN_ID);
        return -1;
    }

    primitiveId |= PRIMITIVE_ID_MASK;
    tmp = (uint8_t)(amplitude * 0xff) * STRONG_MAGNITUDE / 255;
    mCurrMagnitude = tmp;

    ret = play(primitiveId, INVALID_VALUE, playLengthMs);
    if (ret != 0) ALOGE("Failed to play primitive %d", primitiveId);

    return ret;
}

LedVibratorDevice::LedVibratorDevice() {
    char devicename[PATH_MAX];
    int fd;

    mDetected = false;
    mIsLdo = false;
    mIsAwinic = false;

    snprintf(devicename, sizeof(devicename), "%s/%s", LED_DEVICE, "activate");
    fd = TEMP_FAILURE_RETRY(open(devicename, O_RDWR));
    if (fd < 0) {
        ALOGE("open %s failed, errno = %d", devicename, errno);
        return;
    }
    close(fd);

    mDetected = true;
    mIsAwinic = access(LED_DEVICE "/waveform_index", W_OK) == 0;

    snprintf(devicename, sizeof(devicename), "%s/%s", LED_DEVICE, "device/driver");
    mIsLdo = realpath(devicename, devicename) && strstr(devicename, "/qcom,qpnp-vibrator-ldo");
}

int LedVibratorDevice::write_value(const char* file, const char* value) {
    int fd;
    int ret;

    fd = TEMP_FAILURE_RETRY(open(file, O_WRONLY));
    if (fd < 0) {
        ALOGE("open %s failed, errno = %d", file, errno);
        return -errno;
    }

    ret = TEMP_FAILURE_RETRY(write(fd, value, strlen(value) + 1));
    if (ret == -1) {
        ret = -errno;
    } else if (ret != strlen(value) + 1) {
        /* even though EAGAIN is an errno value that could be set
           by write() in some cases, none of them apply here.  So, this return
           value can be clearly identified when debugging and suggests the
           caller that it may try to call vibrator_on() again */
        ret = -EAGAIN;
    } else {
        ret = 0;
    }

    errno = 0;
    close(fd);

    return ret;
}

int LedVibratorDevice::write_value(const char* file, int value) {
    return write_value(file, std::to_string(value).c_str());
}

int LedVibratorDevice::on(int32_t timeoutMs) {
    char file[PATH_MAX];
    char value[32];
    int ret;

    if (mIsAwinic) {
        ret = 0;
        if (timeoutMs <= 0) {
            return ret;
        } else if (timeoutMs <= 20) {
            ret |= write_value(LED_DEVICE "/vmax", timeoutMs * 10);
        } else {
            ret |= write_value(LED_DEVICE "/vmax", 1600);
        }
        ret |= write_value(LED_DEVICE "/waveform_index", 7);
        ret |= write_value(LED_DEVICE "/duration", timeoutMs);
        ret |= write_value(LED_DEVICE "/state", "1");
        ret |= write_value(LED_DEVICE "/activate", "1");
        ret |= write_value(LED_DEVICE "/activate", "0");
        return ret;
    }

    snprintf(file, sizeof(file), "%s/%s", LED_DEVICE, "state");
    ret = write_value(file, "1");
    if (ret < 0) goto error;

    snprintf(file, sizeof(file), "%s/%s", LED_DEVICE, "duration");
    snprintf(value, sizeof(value), "%u\n", timeoutMs);
    ret = write_value(file, value);
    if (ret < 0) goto error;

    snprintf(file, sizeof(file), "%s/%s", LED_DEVICE, "activate");
    ret = write_value(file, "1");
    if (ret < 0) goto error;

    return 0;

error:
    ALOGE("Failed to turn on vibrator ret: %d\n", ret);
    return ret;
}

int LedVibratorDevice::off() {
    char file[PATH_MAX];
    int ret;

    snprintf(file, sizeof(file), "%s/%s", LED_DEVICE, "activate");
    ret = write_value(file, "0");
    return ret;
}

/*
 * Writing a primitive id to primitive_duration makes the next read return its
 * duration in microseconds. The write fails for ids the devicetree doesn't define.
 */
static int getPrimitiveDurationFromSysfs(uint32_t primitive_id, int32_t* durationMs) {
    std::string request = std::to_string(primitive_id) + "\n";
    char reply[16] = {};
    int fd, ret;

    fd = TEMP_FAILURE_RETRY(open(HAPTICS_SYSFS "/primitive_duration", O_RDWR | O_CLOEXEC));
    if (fd < 0) {
        ALOGE("open %s failed, errno = %d", HAPTICS_SYSFS "/primitive_duration", errno);
        return -errno;
    }

    ret = TEMP_FAILURE_RETRY(pwrite(fd, request.c_str(), request.size(), 0));
    if (ret < 0) {
        ret = -errno;
        goto close_fd;
    }

    ret = TEMP_FAILURE_RETRY(pread(fd, reply, sizeof(reply) - 1, 0));
    if (ret < 0) {
        ret = -errno;
        ALOGE("read primitive %d duration failed, errno = %d", primitive_id, errno);
        goto close_fd;
    }

    *durationMs = (strtoul(reply, NULL, 10) + 999) / 1000;
    ret = 0;

close_fd:
    close(fd);
    return ret;
}

Vibrator::Vibrator() {
    struct epoll_event ev;

    epollfd = INVALID_VALUE;
    pipefd[0] = INVALID_VALUE;
    pipefd[1] = INVALID_VALUE;
    inComposition = false;

    initPrimitives();
    if (!mSupportCompose) return;

    if (pipe2(pipefd, O_CLOEXEC)) {
        ALOGE("Failed to get pipefd error=%d", errno);
        goto no_compose;
    }

    epollfd = epoll_create1(EPOLL_CLOEXEC);
    if (epollfd < 0) {
        ALOGE("Failed to create epoll fd error=%d", errno);
        goto pipefd_close;
    }

    ev.events = EPOLLIN;
    ev.data.fd = pipefd[0];

    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, pipefd[0], &ev) == -1) {
        ALOGE("Failed to add pipefd to epoll ctl error=%d", errno);
        goto epollfd_close;
    }

    return;

epollfd_close:
    close(epollfd);
    epollfd = INVALID_VALUE;
pipefd_close:
    close(pipefd[0]);
    close(pipefd[1]);
    pipefd[0] = INVALID_VALUE;
    pipefd[1] = INVALID_VALUE;
no_compose:
    mSupportCompose = false;
}

Vibrator::~Vibrator() {
    if (epollfd != INVALID_VALUE) close(epollfd);
    if (pipefd[0] != INVALID_VALUE) close(pipefd[0]);
    if (pipefd[1] != INVALID_VALUE) close(pipefd[1]);
}

/*
 * A primitive comes from the effect stream library when it has one, and from
 * the kernel devicetree otherwise.
 */
void Vibrator::initPrimitives() {
    mPrimitiveDurationsMs.fill(0);
    mSupportCompose = false;

    if (ledVib.mDetected || !ff.isPresent() || !ff.mSupportEffects) return;

    for (uint32_t id = static_cast<uint32_t>(CompositePrimitive::CLICK);
         id < mPrimitiveDurationsMs.size(); id++) {
        int32_t durationMs = 0;
#ifdef USE_EFFECT_STREAM
        const struct effect_stream* stream = get_effect_stream(id | PRIMITIVE_ID_MASK);
        if (stream != NULL && stream->play_rate_hz != 0)
            durationMs = ((stream->length * 1000) / stream->play_rate_hz) + 1;
#endif
        if (durationMs == 0) getPrimitiveDurationFromSysfs(id, &durationMs);

        mPrimitiveDurationsMs[id] = durationMs;
        if (durationMs > 0) {
            ALOGI("primitive-%u duration is %dms", id, durationMs);
            mSupportCompose = true;
        }
    }
}

bool Vibrator::isPrimitiveSupported(CompositePrimitive primitive) {
    size_t id = static_cast<size_t>(primitive);

    if (!mSupportCompose || id >= mPrimitiveDurationsMs.size()) return false;
    return primitive == CompositePrimitive::NOOP || mPrimitiveDurationsMs[id] > 0;
}

ndk::ScopedAStatus Vibrator::getCapabilities(int32_t* _aidl_return) {
    *_aidl_return = IVibrator::CAP_ON_CALLBACK;

    if (ledVib.mDetected) {
        if (ledVib.mIsAwinic) *_aidl_return |= IVibrator::CAP_PERFORM_CALLBACK;
        ALOGD("QTI Vibrator reporting capabilities: %d", *_aidl_return);
        return ndk::ScopedAStatus::ok();
    }

    if (ff.mSupportGain) *_aidl_return |= IVibrator::CAP_AMPLITUDE_CONTROL;
    if (ff.mSupportEffects) *_aidl_return |= IVibrator::CAP_PERFORM_CALLBACK;
    if (mSupportCompose) *_aidl_return |= IVibrator::CAP_COMPOSE_EFFECTS;
    if (ff.mSupportExternalControl) *_aidl_return |= IVibrator::CAP_EXTERNAL_CONTROL;

    ALOGD("QTI Vibrator reporting capabilities: %d", *_aidl_return);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::off() {
    int ret;
    int event = STOP_COMPOSE;

    ALOGD("QTI Vibrator off");
    if (ledVib.mDetected)
        ret = ledVib.off();
    else
        ret = ff.off();
    if (ret != 0) return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_SERVICE_SPECIFIC));

    if (inComposition) {
        ret = write(pipefd[1], &event, sizeof(event));
        if (ret < 0) {
            ALOGE("Failed to send STOP_COMPOSE event");
            return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_SERVICE_SPECIFIC));
        }
    }

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::on(int32_t timeoutMs,
                                const std::shared_ptr<IVibratorCallback>& callback) {
    int ret;

    if (ledVib.mIsLdo) {
        // See QPNP_VIB_MIN_PLAY_MS, QPNP_VIB_MAX_PLAY_MS in leds-qpnp-vibrator-ldo.c
        timeoutMs = std::clamp(timeoutMs, 50, 15000);
    }

    ALOGD("Vibrator on for timeoutMs: %d", timeoutMs);
    if (ledVib.mDetected)
        ret = ledVib.on(timeoutMs);
    else
        ret = ff.on(timeoutMs);

    if (ret != 0) return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_SERVICE_SPECIFIC));

    if (callback != nullptr) {
        std::thread([=] {
            ALOGD("Starting on on another thread");
            usleep(timeoutMs * 1000);
            ALOGD("Notifying on complete");
            if (!callback->onComplete().isOk()) {
                ALOGE("Failed to call onComplete");
            }
        }).detach();
    }

    return ndk::ScopedAStatus::ok();
}

static ndk::ScopedAStatus performAwinicLed(LedVibratorDevice& ledVib, Effect effect,
                                           long* playLengthMs) {
    switch (effect) {
        case Effect::CLICK:
            ledVib.write_value(LED_DEVICE "/rtp", "0");
            ledVib.write_value(LED_DEVICE "/vmax", "2500");
            ledVib.write_value(LED_DEVICE "/waveform_index", "1");
            ledVib.write_value(LED_DEVICE "/brightness", "1");
            ledVib.write_value(LED_DEVICE "/rtp", "0");
            break;
        case Effect::DOUBLE_CLICK:
            ledVib.write_value(LED_DEVICE "/rtp", "0");
            ledVib.write_value(LED_DEVICE "/vmax", "2500");
            ledVib.write_value(LED_DEVICE "/waveform_index", "1");
            ledVib.write_value(LED_DEVICE "/brightness", "1");
            ledVib.write_value(LED_DEVICE "/rtp", "0");
            usleep(100 * 1000);
            ledVib.write_value(LED_DEVICE "/rtp", "0");
            ledVib.write_value(LED_DEVICE "/vmax", "2500");
            ledVib.write_value(LED_DEVICE "/waveform_index", "1");
            ledVib.write_value(LED_DEVICE "/brightness", "1");
            ledVib.write_value(LED_DEVICE "/rtp", "0");
            break;
        case Effect::TICK:
            ledVib.write_value(LED_DEVICE "/rtp", "0");
            ledVib.write_value(LED_DEVICE "/vmax", "1400");
            ledVib.write_value(LED_DEVICE "/waveform_index", "1");
            ledVib.write_value(LED_DEVICE "/brightness", "1");
            ledVib.write_value(LED_DEVICE "/rtp", "0");
            break;
        case Effect::HEAVY_CLICK:
            ledVib.write_value(LED_DEVICE "/rtp", "0");
            ledVib.write_value(LED_DEVICE "/vmax", "2500");
            ledVib.write_value(LED_DEVICE "/waveform_index", "4");
            ledVib.write_value(LED_DEVICE "/brightness", "1");
            ledVib.write_value(LED_DEVICE "/rtp", "0");
            break;
        case Effect::TEXTURE_TICK:
            ledVib.write_value(LED_DEVICE "/rtp", "0");
            ledVib.write_value(LED_DEVICE "/vmax", "60");
            ledVib.write_value(LED_DEVICE "/waveform_index", "2");
            ledVib.write_value(LED_DEVICE "/brightness", "1");
            ledVib.write_value(LED_DEVICE "/rtp", "0");
            break;
        default:
            return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));
    }

    // Return magic value for play length so that we won't end up calling on() / off()
    *playLengthMs = 150;
    return ndk::ScopedAStatus::ok();
}

/*
 * Only the AOSP effects up to HEAVY_CLICK map 1:1 onto OPlus effect ids, the
 * effect stream tables carry unrelated waveforms past that.
 */
static bool isEffectSupported(Effect effect) {
    if (effect < Effect::CLICK || effect > Effect::HEAVY_CLICK) return false;
#ifdef USE_EFFECT_STREAM
    return get_effect_stream(static_cast<uint32_t>(effect)) != NULL;
#else
    return true;
#endif
}

ndk::ScopedAStatus Vibrator::perform(Effect effect, EffectStrength es,
                                     const std::shared_ptr<IVibratorCallback>& callback,
                                     int32_t* _aidl_return) {
    long playLengthMs;
    int ret;

    ALOGD("Vibrator perform effect %d", effect);

    if (ledVib.mDetected) {
        if (!ledVib.mIsAwinic)
            return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));

        ndk::ScopedAStatus status = performAwinicLed(ledVib, effect, &playLengthMs);
        if (!status.isOk()) return status;
    } else {
        if (!isEffectSupported(effect))
            return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));

        if (es != EffectStrength::LIGHT && es != EffectStrength::MEDIUM &&
            es != EffectStrength::STRONG)
            return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));

        ret = ff.playEffect((static_cast<int>(effect)), es, &playLengthMs);
        if (ret != 0) return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_SERVICE_SPECIFIC));
    }

    if (callback != nullptr) {
        std::thread([=] {
            ALOGD("Starting perform on another thread");
            usleep(playLengthMs * 1000);
            ALOGD("Notifying perform complete");
            callback->onComplete();
        }).detach();
    }

    *_aidl_return = playLengthMs;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::getSupportedEffects(std::vector<Effect>* _aidl_return) {
    _aidl_return->clear();

    if (ledVib.mDetected) {
        if (ledVib.mIsAwinic)
            *_aidl_return = {Effect::CLICK, Effect::DOUBLE_CLICK, Effect::TICK, Effect::HEAVY_CLICK,
                             Effect::TEXTURE_TICK};
        return ndk::ScopedAStatus::ok();
    }

    for (int32_t effectId = static_cast<int32_t>(Effect::CLICK);
         effectId <= static_cast<int32_t>(Effect::HEAVY_CLICK); effectId++) {
        if (isEffectSupported(static_cast<Effect>(effectId)))
            _aidl_return->push_back(static_cast<Effect>(effectId));
    }

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::setAmplitude(float amplitude) {
    uint8_t tmp;
    int ret;

    if (ledVib.mDetected)
        return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));

    if (!ff.mSupportGain)
        return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));

    ALOGD("Vibrator set amplitude: %f", amplitude);

    if (amplitude <= 0.0f || amplitude > 1.0f)
        return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_ILLEGAL_ARGUMENT));

    if (ff.mInExternalControl)
        return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));

    tmp = (uint8_t)(amplitude * 0xff);
    ret = ff.setAmplitude(tmp);
    if (ret != 0) return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_SERVICE_SPECIFIC));

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::setExternalControl(bool enabled) {
    if (ledVib.mDetected)
        return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));

    ALOGD("Vibrator set external control: %d", enabled);
    if (!ff.mSupportExternalControl)
        return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));

    ff.mInExternalControl = enabled;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::getCompositionDelayMax(int32_t* maxDelayMs) {
    if (!mSupportCompose)
        return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));

    *maxDelayMs = ComposeDelayMaxMs;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::getCompositionSizeMax(int32_t* maxSize) {
    if (!mSupportCompose)
        return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));

    *maxSize = ComposeSizeMax;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::getSupportedPrimitives(std::vector<CompositePrimitive>* supported) {
    if (!mSupportCompose)
        return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));

    supported->clear();
    for (size_t id = 0; id < mPrimitiveDurationsMs.size(); id++) {
        CompositePrimitive primitive = static_cast<CompositePrimitive>(id);
        if (isPrimitiveSupported(primitive)) supported->push_back(primitive);
    }

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::getPrimitiveDuration(CompositePrimitive primitive,
                                                  int32_t* durationMs) {
    if (!isPrimitiveSupported(primitive))
        return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));

    *durationMs = mPrimitiveDurationsMs[static_cast<size_t>(primitive)];
    return ndk::ScopedAStatus::ok();
}

void Vibrator::composePlayThread(Vibrator* vibrator, const std::vector<CompositeEffect>& composite,
                                 const std::shared_ptr<IVibratorCallback>& callback) {
    struct epoll_event events;
    long playLengthMs = 0;
    int nfd = 0;
    int status = 0;
    int ret = 0;

    ALOGD("start a new thread for composeEffect");
    for (auto& e : composite) {
        if (e.delayMs) {
            nfd = epoll_wait(vibrator->epollfd, &events, 1, e.delayMs);
            if ((nfd == -1) && (errno != EINTR)) {
                ALOGE("Failed to wait delayMs, error=%d", errno);
                break;
            }

            if (nfd > 0) {
                /* It's supposed that STOP_COMPOSE command is received so quit the composition */
                ret = read(vibrator->pipefd[0], &status, sizeof(int));
                if (ret < 0) {
                    ALOGE("Failed to read stop status from pipe(delayMs), status = %d", status);
                    break;
                }
                if (status == STOP_COMPOSE) break;
            }
        }

        /* NOOP only carries a delay, there is nothing to play */
        if (e.primitive == CompositePrimitive::NOOP) continue;

        vibrator->ff.playPrimitive((static_cast<int>(e.primitive)), e.scale, &playLengthMs);
        nfd = epoll_wait(vibrator->epollfd, &events, 1, playLengthMs);
        if (nfd == -1 && (errno != EINTR)) {
            ALOGE("Failed to wait sleep playLengthMs, error=%d", errno);
            break;
        }

        if (nfd > 0) {
            /* It's supposed that STOP_COMPOSE command is received so quit the composition */
            ret = read(vibrator->pipefd[0], &status, sizeof(int));
            if (ret < 0) {
                ALOGE("Failed to read stop status from pipe(playLengthMs), status = %d", status);
                break;
            }
            if (status == STOP_COMPOSE) {
                /*
                 * There is a corner case that the off() command could be executed in
                 * main thread before the primitive play is triggered in the child thread,
                 * such as, when playing a very short primitive effect while the system is
                 * pretty busy, the child thread may not be able to schedule out for running
                 * before the main thread times out on the primitive duration and sent the
                 * off() command, there won't be any off() command coming again to stop the
                 * primitive effect after it's triggered.
                 *
                 * However, the primitive could be played out and stopped automatically but the
                 * haptics driver does expect an explicit off() command to restore HW/SW logic
                 * after that, so call it here. It would result a redundant off() command in
                 * normal case but it won't do any harm because it would be ignored and not sent
                 * to haptics driver because of an invalid mCurrAppId.
                 */
                vibrator->ff.off();
                break;
            }
        }
    }

    ALOGD("Notifying composite complete, playlength= %ld", playLengthMs);
    if (callback) callback->onComplete();

    vibrator->inComposition = false;
}

ndk::ScopedAStatus Vibrator::compose(const std::vector<CompositeEffect>& composite,
                                     const std::shared_ptr<IVibratorCallback>& callback) {
    int status, nfd = 0, timeoutMs = 0;
    struct epoll_event events;

    if (!mSupportCompose)
        return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));

    if (composite.size() > ComposeSizeMax) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }

    for (auto& e : composite) {
        if (e.delayMs < 0 || e.delayMs > ComposeDelayMaxMs) {
            return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
        }
        if (!(e.scale >= 0.0f && e.scale <= 1.0f)) {
            return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
        }
        if (!isPrimitiveSupported(e.primitive)) {
            return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
        }

        timeoutMs += mPrimitiveDurationsMs[static_cast<size_t>(e.primitive)] + e.delayMs;
    }

    /*
     * wait for 2 times of the play length timeout to make sure last play has been
     * terminated successfully.
     */
    timeoutMs = (timeoutMs + 10) * 2;
    /* Stop previous composition if it has not yet been completed */
    if (inComposition) {
        ALOGD("Last composePlayThread has not done yet, stop it manually");
        off();

        while (inComposition && timeoutMs-- > 0) usleep(1000);

        if (inComposition) {
            ALOGE("wait for last composePlayThread done timeout");
            return ndk::ScopedAStatus::fromExceptionCode(EX_SERVICE_SPECIFIC);
        }
    }

    /* Read the pipe again to remove any stale data before triggering a new play */
    nfd = epoll_wait(epollfd, &events, 1, 0);
    if (nfd == -1 && (errno != EINTR)) {
        ALOGE("Failed to wait sleep playLengthMs, error=%d", errno);
        return ndk::ScopedAStatus::fromExceptionCode(EX_SERVICE_SPECIFIC);
    }
    if (nfd > 0) {
        ALOGD("A stale event is cached in the pipe, remove it");
        read(pipefd[0], &status, sizeof(int));
    }

    inComposition = true;
    composeThread = std::thread(composePlayThread, this, composite, callback);
    composeThread.detach();

    ALOGD("trigger composition successfully");
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::getSupportedAlwaysOnEffects(
        std::vector<Effect>* _aidl_return __unused) {
    return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));
}

ndk::ScopedAStatus Vibrator::alwaysOnEnable(int32_t id __unused, Effect effect __unused,
                                            EffectStrength strength __unused) {
    return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));
}

ndk::ScopedAStatus Vibrator::alwaysOnDisable(int32_t id __unused) {
    return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));
}

ndk::ScopedAStatus Vibrator::getResonantFrequency(float* resonantFreqHz __unused) {
    return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));
}

ndk::ScopedAStatus Vibrator::getQFactor(float* qFactor __unused) {
    return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));
}

ndk::ScopedAStatus Vibrator::getFrequencyResolution(float* freqResolutionHz __unused) {
    return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));
}

ndk::ScopedAStatus Vibrator::getFrequencyMinimum(float* freqMinimumHz __unused) {
    return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));
}

ndk::ScopedAStatus Vibrator::getBandwidthAmplitudeMap(std::vector<float>* _aidl_return __unused) {
    return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));
}

ndk::ScopedAStatus Vibrator::getPwlePrimitiveDurationMax(int32_t* durationMs __unused) {
    return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));
}

ndk::ScopedAStatus Vibrator::getPwleCompositionSizeMax(int32_t* maxSize __unused) {
    return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));
}

ndk::ScopedAStatus Vibrator::getSupportedBraking(std::vector<Braking>* supported __unused) {
    return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));
}

ndk::ScopedAStatus Vibrator::composePwle(const std::vector<PrimitivePwle>& composite __unused,
                                         const std::shared_ptr<IVibratorCallback>& callback
                                                 __unused) {
    return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));
}

}  // namespace vibrator
}  // namespace hardware
}  // namespace android
}  // namespace aidl
