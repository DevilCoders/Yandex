#pragma once

#include "progress.h"

#include <library/cpp/threading/future/async.h>

#include <util/thread/pool.h>
#include <util/system/mutex.h>

namespace NProgressBar {
    template <typename T>
    using TAsyncUpdater = std::function<TProgress<T>()>;

    template <typename T, typename Base>
    class TAsyncProgressBar: private Base {
    public:
        TAsyncProgressBar(IOutputStream& outputStream, const TString& head, TAsyncUpdater<T>&& updater,
                          TDuration updateInterval = TDuration::MilliSeconds(250), size_t length = 80)
            : Base(outputStream, head, length)
            , Updater_(std::forward<TAsyncUpdater<T>>(updater))
            , UpdateInterval_(updateInterval)
        {
            auto producer = [&]() {
                while (TryUpdate(Updater_())) {
                    Sleep(UpdateInterval_);
                }
            };

            ThreadPool_.Start(1);
            NThreading::Async(std::move(producer), ThreadPool_);
        }

        void Stop() {
            if (Stopped_) {
                return;
            }

            if (TryUpdate(Updater_())) {
                {
                    TGuard<TMutex> guard(Lock_);
                    Stopped_ = true;
                }

                ThreadPool_.Stop();
            }

            Base::Flush();
        }

        ~TAsyncProgressBar() {
            Stop();
        }

    private:
        bool TryUpdate(const TProgress<T>& progress) {
            TGuard<TMutex> guard(Lock_);
            if (Stopped_) {
                return false;
            }
            Base::Update(progress);
            return true;
        }

        TMutex Lock_;
        bool Stopped_ = false;
        TAsyncUpdater<T> Updater_;
        TDuration UpdateInterval_ = TDuration::MilliSeconds(250);
        TThreadPool ThreadPool_;
    };

}
