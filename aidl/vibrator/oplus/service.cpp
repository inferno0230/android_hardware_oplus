/*
 * SPDX-FileCopyrightText: 2026 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

#include "FfBackend.h"
#include "RichtapBackend.h"
#include "Vibrator.h"

using aidl::android::hardware::vibrator::Backend;
using aidl::android::hardware::vibrator::FfBackend;
using aidl::android::hardware::vibrator::NullBackend;
using aidl::android::hardware::vibrator::RichtapBackend;
using aidl::android::hardware::vibrator::Vibrator;

namespace {

// Devices differ in their haptics, not in this binary: find out what this one has.
std::unique_ptr<Backend> createBackend() {
    if (auto backend = FfBackend::probe()) {
        return backend;
    }
    if (auto backend = RichtapBackend::probe()) {
        return backend;
    }

    LOG(ERROR) << "No haptics found, the vibrator will not do anything";
    return std::make_unique<NullBackend>();
}

}  // anonymous namespace

int main() {
    ABinderProcess_setThreadPoolMaxThreadCount(0);
    std::shared_ptr<Vibrator> vibrator = ndk::SharedRefBase::make<Vibrator>(createBackend());

    const std::string instance = std::string() + Vibrator::descriptor + "/default";
    binder_status_t status =
            AServiceManager_addService(vibrator->asBinder().get(), instance.c_str());
    CHECK_EQ(status, STATUS_OK);

    ABinderProcess_joinThreadPool();
    return EXIT_FAILURE;  // should not reach
}
