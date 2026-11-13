/*
 * SPDX-FileCopyrightText: 2026 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>

namespace aidl {
namespace android {
namespace hardware {
namespace vibrator {

/*
 * Runs what has to happen after a binder call returned (completion callbacks, the steps of a
 * composition) on one worker thread, in the order it was requested.
 *
 * cancel() does not drop anything: it only makes the waits of everything posted so far return
 * at once, so every task still runs to its end and every completion callback is still delivered
 * exactly once. Nothing runs anymore once the scheduler is destroyed.
 */
class CallbackScheduler {
  public:
    class Context {
      public:
        bool cancelled() const;
        // Returns false if the task got cancelled before the time was up.
        bool sleepFor(uint32_t ms) const;

      private:
        friend class CallbackScheduler;
        Context(CallbackScheduler* scheduler, uint64_t id) : mScheduler(scheduler), mId(id) {}

        CallbackScheduler* mScheduler;
        uint64_t mId;
    };

    using Task = std::function<void(const Context&)>;

    CallbackScheduler();
    ~CallbackScheduler();

    void post(Task task);
    // Calls callback once durationMs have passed, or right away when cancelled.
    void schedule(uint32_t durationMs, std::function<void()> callback);
    void cancel();

  private:
    struct Job {
        uint64_t id;
        Task task;
    };

    void run();
    bool isCancelledLocked(uint64_t id) const { return mExit || id <= mCancelledUpTo; }

    std::mutex mMutex;
    std::condition_variable mCondition;
    std::deque<Job> mJobs;
    uint64_t mLastId = 0;
    uint64_t mCancelledUpTo = 0;
    bool mExit = false;
    std::thread mThread;
};

}  // namespace vibrator
}  // namespace hardware
}  // namespace android
}  // namespace aidl
