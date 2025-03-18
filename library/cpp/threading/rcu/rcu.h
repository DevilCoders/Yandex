#pragma once

#include "rcu_accessor.h"

#include <library/cpp/threading/future/async.h>

#include <util/generic/ptr.h>
#include <util/system/spinlock.h>
#include <util/thread/pool.h>
#include <util/generic/algorithm.h>
#include <util/generic/function.h>

namespace NThreading {
    namespace NRcuPrivate {
        inline TAtomicSharedPtr<IThreadPool> GetQueue(TAtomicSharedPtr<IThreadPool> queue) {
            if (!queue) {
                queue.Reset(new TThreadPool);
                queue->Start(1);
            }
            return queue;
        }

        template <class T>
        struct IUpdate {
            virtual ~IUpdate() {
            }

            virtual void Apply(T& data) = 0;
            virtual void Fail(const TString& error) = 0;
        };

        template <class T, class F>
        struct TUpdate: public IUpdate<T> {
            using TResult = TFunctionResult<F>;

            F UpdateFunc;
            TPromise<TResult> Promise;
            TPromise<TResult> IntermediatePromise;

            TUpdate(F&& func, TPromise<TResult> promise)
                : UpdateFunc(std::forward<F>(func))
                , Promise(std::move(promise))
                , IntermediatePromise(NewPromise<TResult>())
            {
            }

            void Apply(T& data) override {
                /* Update subscribers should see that update is done only after its results are
           published via TRcuAccessor::Set. That's why we store the update's result in
           an @c IntermediatePromise and move them to @c Promise only in destructor.

           @see TRcu::ApplyUpdates
        */
                NImpl::SetValue(IntermediatePromise, [this, &data] { return UpdateFunc(data); });
            }

            void Fail(const TString& error) override {
                IntermediatePromise.SetException(error);
            }

            ~TUpdate() override {
                try {
                    NImpl::SetValue(Promise, [this] { return IntermediatePromise.GetFuture().GetValue(); });
                } catch (...) {
                    // Promise's subscribers can throw
                }
            }
        };

        class TUpdateLauncher {
        public:
            TUpdateLauncher()
                : LastUpdate(NThreading::MakeFuture())
                , UpdatesCounter(new int)
            {
            }

            ~TUpdateLauncher() {
                LastUpdate.Wait();
            }

            template <class F>
            void Schedule(F&& func) {
                auto& updatesCounter = UpdatesCounter;

                with_lock (LastUpdateLock) {
                    LastUpdate = LastUpdate.Apply([updatesCounter, func](const TFuture<void>&) {
                        return func();
                    });
                }
            }

            int GetUpdatesCount() const {
                return UpdatesCounter.RefCount() - 1;
            }

        private:
            TAdaptiveLock LastUpdateLock;
            TFuture<void> LastUpdate;

            TAtomicSharedPtr<int> UpdatesCounter;
        };

    }

    template <typename T>
    class TRcu {
    public:
        using TReference = typename TRcuAccessor<T>::TReference;

        TRcu(T value = T(), TAtomicSharedPtr<IThreadPool> queue = nullptr)
            : Rcu(std::move(value))
            , Queue(NRcuPrivate::GetQueue(std::move(queue)))
        {
        }

        explicit TRcu(TAtomicSharedPtr<IThreadPool> queue)
            : TRcu(T(), std::move(queue))
        {
        }

        TReference GetCopy() const {
            return Rcu.Get();
        }

        template <typename Func>
        TFuture<TFunctionResult<Func>> UpdateWithFunction(Func&& func) {
            return Schedule(std::forward<Func>(func));
        }

        TFuture<void> UpdateWithValue(T value) {
            auto valuePtr = MakeAtomicShared<T>(std::move(value));
            return Schedule([valuePtr](T& theValue) {
                theValue = std::move(*valuePtr);
            });
        }

    private:
        using IUpdate = NRcuPrivate::IUpdate<T>;

        template <class F>
        using TUpdate = NRcuPrivate::TUpdate<T, F>;

        // Passing vector of updates by value is crucial here. Its items must be destroyed
        // after calling Rcu.Set. @see TUpdate::Apply and TUpdate::~TUpdate.
        void ApplyUpdates(TVector<THolder<IUpdate>> updates) {
            auto copy = *Rcu.Get();

            for (auto& u : updates) {
                try {
                    u->Apply(copy);
                } catch (...) {
                }
            }

            Rcu.Set(std::move(copy));
        }

        TFuture<void> EnqueueUpdates() {
            auto currentUpdates = MakeAtomicShared<TVector<THolder<IUpdate>>>();
            with_lock (UpdatesLock) {
                Updates.swap(*currentUpdates);
            }

            if (currentUpdates->empty()) {
                return NThreading::MakeFuture();
            }

            try {
                // we can safely capture @c this and store it in another thread beacuse
                // in destructor we wait for all updates to be appled. @see ~TUpdateLauncher.
                auto applyUpdates = [this, currentUpdates] {
                    this->ApplyUpdates(std::move(*currentUpdates));
                };

                return NThreading::Async(applyUpdates, *Queue);
            } catch (yexception&) {
                const auto error = CurrentExceptionMessage();

                for (auto& update : *currentUpdates) {
                    update->Fail(error);
                }

                return NThreading::MakeFuture();
            }
        }

        template <class UpdateFunc>
        TFuture<TFunctionResult<UpdateFunc>> Schedule(UpdateFunc&& update) {
            using TResult = TFunctionResult<UpdateFunc>;

            auto promise = NewPromise<TResult>();
            with_lock (UpdatesLock) {
                Updates.push_back(MakeHolder<TUpdate<UpdateFunc>>(std::forward<UpdateFunc>(update), promise));
            }

            // we can safely capture @c this and store it in another thread beacuse
            // industructor we wait for all updates to be appled. @see ~TUpdateLauncher.
            if (UpdateLauncher.GetUpdatesCount() < 2) {
                UpdateLauncher.Schedule([this] { return this->EnqueueUpdates(); });
            }

            return promise;
        }

        TRcuAccessor<T> Rcu;
        TAtomicSharedPtr<IThreadPool> Queue;

        TAdaptiveLock UpdatesLock;
        TVector<THolder<IUpdate>> Updates;

        // must be destroyed before @c Updates and @c Queue because implicitly accesses them
        // @see Schedule, EnqueueUpdates
        NRcuPrivate::TUpdateLauncher UpdateLauncher;
    };

}
