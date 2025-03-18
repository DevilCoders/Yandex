#pragma once

#include <nginx/modules/strm_packager/src/base/fatal_exception.h>

#include <library/cpp/threading/future/future.h>

#include <util/generic/vector.h>

namespace NStrm::NPackager {
    namespace {
        class TPackagerWaitExceptionOrAll: public TSimpleRefCount<TPackagerWaitExceptionOrAll> {
        public:
            TPackagerWaitExceptionOrAll(size_t count)
                : Count(count)
                , Finished(false)
                , Promise(NThreading::NewPromise())
            {
            }

            NThreading::TFuture<void> GetFuture() {
                return Promise.GetFuture();
            }

            template <typename Type>
            void Set(const NThreading::TFuture<Type>& future) {
                if (Finished) {
                    return;
                }

                try {
                    future.TryRethrow();
                } catch (...) {
                    Finished = true;
                    Promise.SetException(std::current_exception());
                    return;
                }

                --Count;
                if (Count == 0) {
                    Finished = true;
                    Promise.SetValue();
                }
            };

        private:
            size_t Count;
            bool Finished;
            NThreading::TPromise<void> Promise;
        };
    }

    // NThreading::WaitExceptionOrAll handle exceptions in different way
    template <typename Type>
    inline NThreading::TFuture<void> PackagerWaitExceptionOrAll(const TVector<NThreading::TFuture<Type>>& futures) {
        if (futures.empty()) {
            return NThreading::MakeFuture();
        }

        if (futures.size() == 1) {
            return futures.front().IgnoreResult();
        }

        TIntrusivePtr<TPackagerWaitExceptionOrAll> waiter = new TPackagerWaitExceptionOrAll(futures.size());

        const auto callback = std::function<void(const NThreading::TFuture<Type>&)>(
            [waiter](const NThreading::TFuture<Type>& future) mutable {
                waiter->Set(future);
            });

        for (const auto& f : futures) {
            f.Subscribe(callback);
        }

        return waiter->GetFuture();
    }

    template <typename Type>
    void TransferFutureToPromise(NThreading::TFuture<Type> future, NThreading::TPromise<Type> promise) {
        future.Subscribe([promise](const NThreading::TFuture<Type>& future) mutable {
            try {
                future.TryRethrow();
            } catch (...) {
                promise.SetException(std::current_exception());
                return;
            }
            promise.SetValue(future.GetValue());
        });
    }
}
