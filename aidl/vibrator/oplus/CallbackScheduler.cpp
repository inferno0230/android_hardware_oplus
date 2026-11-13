/*
 * SPDX-FileCopyrightText: 2026 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include "CallbackScheduler.h"

#include <chrono>

namespace aidl {
namespace android {
namespace hardware {
namespace vibrator {

bool CallbackScheduler::Context::cancelled() const {
    std::lock_guard<std::mutex> lock(mScheduler->mMutex);
    return mScheduler->isCancelledLocked(mId);
}

bool CallbackScheduler::Context::sleepFor(uint32_t ms) const {
    std::unique_lock<std::mutex> lock(mScheduler->mMutex);
    return !mScheduler->mCondition.wait_for(lock, std::chrono::milliseconds(ms),
                                            [this] { return mScheduler->isCancelledLocked(mId); });
}

CallbackScheduler::CallbackScheduler() : mThread(&CallbackScheduler::run, this) {}

CallbackScheduler::~CallbackScheduler() {
    {
        std::lock_guard<std::mutex> lock(mMutex);
        mExit = true;
    }
    mCondition.notify_all();
    mThread.join();
}

void CallbackScheduler::post(Task task) {
    {
        std::lock_guard<std::mutex> lock(mMutex);
        mJobs.push_back({++mLastId, std::move(task)});
    }
    mCondition.notify_all();
}

void CallbackScheduler::schedule(uint32_t durationMs, std::function<void()> callback) {
    post([durationMs, callback = std::move(callback)](const Context& context) {
        context.sleepFor(durationMs);
        callback();
    });
}

void CallbackScheduler::cancel() {
    {
        std::lock_guard<std::mutex> lock(mMutex);
        mCancelledUpTo = mLastId;
    }
    mCondition.notify_all();
}

void CallbackScheduler::run() {
    std::unique_lock<std::mutex> lock(mMutex);
    while (true) {
        mCondition.wait(lock, [this] { return mExit || !mJobs.empty(); });
        if (mExit) {
            return;
        }

        Job job = std::move(mJobs.front());
        mJobs.pop_front();

        lock.unlock();
        job.task(Context(this, job.id));
        // Release whatever the task captured before going back to sleep.
        job.task = nullptr;
        lock.lock();
    }
}

}  // namespace vibrator
}  // namespace hardware
}  // namespace android
}  // namespace aidl
