#pragma once

#include <library/cpp/threading/future/future.h>

#include <util/system/mutex.h>
#include <util/generic/ptr.h>
#include <util/generic/vector.h>
#include <util/generic/deque.h>
#include <util/generic/list.h>
#include <util/generic/yexception.h>

namespace NStrm::NRepeatableFutureImpl {
    // When Atomic flag is on, Future and Promise can be used concurrently from different threads
    template <typename T, bool Atomic = false>
    class TRepFuture;

    template <typename T, bool Atomic = false>
    class TRepPromise;
}

namespace NStrm {
    template <typename T, bool Atomic = false>
    using TRepFuture = NRepeatableFutureImpl::TRepFuture<T, Atomic>;

    template <typename T, bool Atomic = false>
    using TRepPromise = NRepeatableFutureImpl::TRepPromise<T, Atomic>;

    template <typename T, bool Atomic>
    class TRepFutureCallbackHolder;

    template <typename T>
    using TAtomicRepFuture = TRepFuture<T, /*Atomic=*/true>;

    template <typename T>
    using TAtomicRepPromise = TRepPromise<T, /*Atomic=*/true>;

    template <typename T>
    using TAtomicRepFutureCallbackHolder = TRepFutureCallbackHolder<T, /*Atomic=*/true>;

    template <typename T, bool Atomic>
    TRepFuture<T, Atomic> Concat(const TVector<TRepFuture<T, Atomic>>& futures);
}

namespace NStrm::NRepeatableFutureImpl {
    template <typename T>
    class TDataRef {
    public:
        using TData = T;

        explicit TDataRef();

        explicit TDataRef(const TData&);

        explicit TDataRef(std::exception_ptr);

        bool Empty() const;                   // throws if has exception, otherwise return !Data_ - this will be in the last callback call
        std::exception_ptr Exception() const; // return exception inside or nullptr [ do we really need this? ]
        const TData& Data() const;            // throws if has exception or empty it, otherwise return data

    private:
        std::exception_ptr const Exception_;
        TData const* const Data_;
    };

    template <typename T, bool Atomic>
    class TRepFutureState: public std::conditional_t<Atomic, TAtomicRefCount<TRepFutureState<T, Atomic>>, TSimpleRefCount<TRepFutureState<T, Atomic>>> {
    public:
        using TPtr = TIntrusivePtr<TRepFutureState>;
        using TData = T;
        using TDataRef = TDataRef<TData>;
        using TCallback = std::function<void(const TDataRef&)>;
        using TCallbackId = uint64_t;

        explicit TRepFutureState(size_t capacity = SIZE_MAX);

        size_t GetCapacity() const;

        TMaybe<TCallbackId> AddCallback(const TCallback&);

        // Returns true if callback was found and removed
        bool RemoveCallback(TCallbackId callbackId);

        template <typename... TArgs>
        void PutData(TArgs... args);

        void FinishWithException(std::exception_ptr);

        void Finish();

        bool TryFinish();

        bool IsTerminated() const;

    private:
        using TMaybeMutex = std::conditional_t<Atomic, TMutex, TFakeMutex>;
        const size_t Capacity;

        TMap<TCallbackId, TCallback> Callbacks;
        TDeque<TData> Data;
        std::exception_ptr Exception;
        bool Finished;
        TCallbackId CallbackCounter;
        TMaybeMutex Mutex;
    };

    template <typename T, bool Atomic>
    class TRepFuture {
    public:
        using TData = T;
        using TDataRef = TDataRef<TData>;
        using TCallback = std::function<void(const TDataRef&)>;
        using TState = TRepFutureState<TData, Atomic>;
        using TCallbackId = typename TState::TCallbackId;
        using TCallbackHolder = TRepFutureCallbackHolder<T, Atomic>;

        bool Initialized() const;

        size_t GetCapacity() const;

        TMaybe<TCallbackId> AddCallback(const TCallback&) const;

        bool RemoveCallback(TCallbackId callbackId) const;

        template <typename FF, typename FFRet = decltype(std::declval<FF>()(std::declval<const T&>()))>
        TRepFuture<FFRet, Atomic> Apply(FF&& func);

        bool IsTerminated() const;

    private:
        typename TState::TPtr State;

        friend class TRepPromise<T, Atomic>;
    };

    template <typename T, bool Atomic>
    class TRepPromise {
    public:
        using TData = T;
        using TDataRef = TDataRef<TData>;

        static TRepPromise Make(size_t capacity = SIZE_MAX);

        bool Initialized() const;

        size_t GetCapacity() const;

        TRepFuture<TData, Atomic> GetFuture() const;

        template <typename... TArgs>
        void PutData(TArgs... args) const;

        void FinishWithException(std::exception_ptr) const;

        void Finish() const;

        bool TryFinish() const;

        bool IsTerminated() const;

    private:
        typename TRepFutureState<TData, Atomic>::TPtr State;
    };

    // ======= implementation

    template <typename T>
    inline TDataRef<T>::TDataRef()
        : Exception_(nullptr)
        , Data_(nullptr)
    {
    }

    template <typename T>
    inline TDataRef<T>::TDataRef(const TData& data)
        : Exception_(nullptr)
        , Data_(&data)
    {
    }

    template <typename T>
    inline TDataRef<T>::TDataRef(std::exception_ptr exception)
        : Exception_(exception)
        , Data_(nullptr)
    {
        Y_ENSURE(exception);
    }

    template <typename T>
    inline bool TDataRef<T>::Empty() const {
        if (Exception_) {
            std::rethrow_exception(Exception_);
        } else {
            return !Data_;
        }
    }

    template <typename T>
    inline std::exception_ptr TDataRef<T>::Exception() const {
        return Exception_;
    }

    template <typename T>
    inline const T& TDataRef<T>::Data() const {
        if (Exception_) {
            std::rethrow_exception(Exception_);
        }
        Y_ENSURE(Data_);
        return *Data_;
    }

    template <typename T, bool Atomic>
    inline TRepFutureState<T, Atomic>::TRepFutureState(size_t capacity)
        : Capacity(capacity)
        , Exception(nullptr)
        , Finished(false)
        , CallbackCounter(0)
    {
    }

    template <typename T, bool Atomic>
    inline size_t TRepFutureState<T, Atomic>::GetCapacity() const {
        return Capacity;
    }

    template <typename T, bool Atomic>
    inline TMaybe<typename TRepFutureState<T, Atomic>::TCallbackId> TRepFutureState<T, Atomic>::AddCallback(const TCallback& callback) {
        TGuard<TMaybeMutex> guard(Mutex);
        Y_ENSURE(callback);
        for (const TData& d : Data) {
            callback(TDataRef(d));
        }
        if (Exception) {
            callback(TDataRef(Exception));
            return {};
        }
        if (Finished) {
            callback(TDataRef());
            return {};
        }
        ++CallbackCounter;
        Callbacks[CallbackCounter] = callback;
        return CallbackCounter;
    }

    template <typename T, bool Atomic>
    bool TRepFutureState<T, Atomic>::RemoveCallback(TCallbackId callbackId) {
        TGuard<TMaybeMutex> guard(Mutex);
        return static_cast<bool>(Callbacks.erase(callbackId));
    }

    template <typename T, bool Atomic>
    template <typename... TArgs>
    inline void TRepFutureState<T, Atomic>::PutData(TArgs... args) {
        TGuard<TMaybeMutex> guard(Mutex);
        Y_ENSURE(!Finished && !Exception);
        Data.emplace_back(std::forward<TArgs>(args)...);
        for (const auto& [clbkId, clbk] : Callbacks) {
            clbk(TDataRef(Data.back()));
        }
        while (Data.size() > Capacity) {
            Data.pop_front();
        }
    }

    template <typename T, bool Atomic>
    inline void TRepFutureState<T, Atomic>::FinishWithException(const std::exception_ptr exception) {
        TGuard<TMaybeMutex> guard(Mutex);
        Y_ENSURE(exception);
        Y_ENSURE(!Finished && !Exception);
        Exception = exception;
        for (const auto& [clbkId, clbk] : Callbacks) {
            clbk(TDataRef(Exception));
        }
        Callbacks.clear();
    }

    template <typename T, bool Atomic>
    inline void TRepFutureState<T, Atomic>::Finish() {
        Y_ENSURE(TryFinish());
    }

    template <typename T, bool Atomic>
    inline bool TRepFutureState<T, Atomic>::TryFinish() {
        TGuard<TMaybeMutex> guard(Mutex);
        if (Finished || Exception) {
            return false;
        }
        Finished = true;
        for (const auto& [clbkId, clbk] : Callbacks) {
            clbk(TDataRef());
        }
        Callbacks.clear();
        return true;
    }

    template <typename T, bool Atomic>
    inline bool TRepFutureState<T, Atomic>::IsTerminated() const {
        TGuard<TMaybeMutex> guard(Mutex);
        return Finished || Exception;
    }

    template <typename T, bool Atomic>
    inline bool TRepFuture<T, Atomic>::Initialized() const {
        return (bool)State;
    }

    template <typename T, bool Atomic>
    inline size_t TRepFuture<T, Atomic>::GetCapacity() const {
        Y_ENSURE(State);
        return State->GetCapacity();
    }

    template <typename T, bool Atomic>
    inline TMaybe<typename TRepFuture<T, Atomic>::TCallbackId> TRepFuture<T, Atomic>::AddCallback(const TCallback& callback) const {
        Y_ENSURE(State);
        return State->AddCallback(callback);
    }

    template <typename T, bool Atomic>
    inline bool TRepFuture<T, Atomic>::RemoveCallback(TRepFuture<T, Atomic>::TCallbackId callbackId) const {
        Y_ENSURE(State);
        return State->RemoveCallback(callbackId);
    }

    template <typename T, bool Atomic>
    template <typename FF, typename FFRet>
    TRepFuture<FFRet, Atomic> TRepFuture<T, Atomic>::Apply(FF&& functor) {
        auto promise = TRepPromise<FFRet, Atomic>::Make(GetCapacity());
        auto future = promise.GetFuture();
        AddCallback([func = std::move(functor), promise = std::move(promise)](const TDataRef& dataRef) {
            if (promise.IsTerminated()) {
                return;
            }
            if (dataRef.Exception()) {
                promise.FinishWithException(dataRef.Exception());
                return;
            }
            if (dataRef.Empty()) {
                promise.Finish();
                return;
            }

            try {
                promise.PutData(func(dataRef.Data()));
            } catch (...) {
                promise.FinishWithException(std::current_exception());
            }
        });
        return future;
    }

    template <typename T, bool Atomic>
    inline bool TRepFuture<T, Atomic>::IsTerminated() const {
        Y_ENSURE(State);
        return State->IsTerminated();
    }

    template <typename T, bool Atomic>
    inline TRepPromise<T, Atomic> TRepPromise<T, Atomic>::Make(size_t capacity) {
        TRepPromise result;
        result.State = new TRepFutureState<T, Atomic>(capacity);
        return result;
    }

    template <typename T, bool Atomic>
    inline bool TRepPromise<T, Atomic>::Initialized() const {
        return (bool)State;
    }

    template <typename T, bool Atomic>
    inline size_t TRepPromise<T, Atomic>::GetCapacity() const {
        Y_ENSURE(State);
        return State->GetCapacity();
    }

    template <typename T, bool Atomic>
    inline TRepFuture<T, Atomic> TRepPromise<T, Atomic>::GetFuture() const {
        Y_ENSURE(State);
        TRepFuture<T, Atomic> result;
        result.State = State;
        return result;
    }

    template <typename T, bool Atomic>
    template <typename... TArgs>
    inline void TRepPromise<T, Atomic>::PutData(TArgs... args) const {
        Y_ENSURE(State);
        State->PutData(std::forward<TArgs>(args)...);
    }

    template <typename T, bool Atomic>
    inline void TRepPromise<T, Atomic>::FinishWithException(const std::exception_ptr exception) const {
        Y_ENSURE(State);
        State->FinishWithException(exception);
    }

    template <typename T, bool Atomic>
    inline void TRepPromise<T, Atomic>::Finish() const {
        Y_ENSURE(State);
        State->Finish();
    }

    template <typename T, bool Atomic>
    inline bool TRepPromise<T, Atomic>::TryFinish() const {
        Y_ENSURE(State);
        return State->TryFinish();
    }

    template <typename T, bool Atomic>
    inline bool TRepPromise<T, Atomic>::IsTerminated() const {
        Y_ENSURE(State);
        return State->IsTerminated();
    }
}

namespace NStrm {
    namespace NRepeatableFutureImpl {
        template <typename T, bool Atomic>
        void ConcatAcceptCallback(
            const typename TRepFuture<T, Atomic>::TDataRef& dataRef,
            TRepPromise<T, Atomic> promise,
            const TVector<TRepFuture<T, Atomic>>& futures,
            const size_t index) {
            if (dataRef.Exception()) {
                promise.FinishWithException(dataRef.Exception());
            } else if (dataRef.Empty()) {
                if (index + 1 < futures.size()) {
                    futures[index + 1].AddCallback(
                        std::bind(
                            ConcatAcceptCallback<T, Atomic>,
                            std::placeholders::_1,
                            promise,
                            futures,
                            index + 1));
                } else {
                    promise.Finish();
                }
            } else {
                promise.PutData(dataRef.Data());
            }
        }
    }

    template <typename T, bool Atomic>
    inline TRepFuture<T, Atomic> Concat(const TVector<TRepFuture<T, Atomic>>& futures) {
        Y_ENSURE(!futures.empty());
        if (futures.size() == 1) {
            return futures.front();
        }

        TRepPromise<T, Atomic> promise = TRepPromise<T, Atomic>::Make();

        futures[0].AddCallback(std::bind(
            &NRepeatableFutureImpl::ConcatAcceptCallback<T, Atomic>,
            std::placeholders::_1,
            promise,
            futures,
            0));

        return promise.GetFuture();
    }

    template <typename T, bool Atomic = false>
    class TRepFutureCallbackHolder {
    public:
        using TRepFuture = TRepFuture<T, Atomic>;
        using TCallbackId = typename TRepFuture::TCallbackId;

        TRepFutureCallbackHolder() = default;

        TRepFutureCallbackHolder(const TRepFutureCallbackHolder&) = delete;
        TRepFutureCallbackHolder& operator=(const TRepFutureCallbackHolder&) = delete;

        TRepFutureCallbackHolder(TRepFutureCallbackHolder&& other) {
            other.Swap(*this);
        }

        TRepFutureCallbackHolder& operator=(TRepFutureCallbackHolder&& other) {
            other.Swap(*this);
            return *this;
        }

        TRepFutureCallbackHolder(TRepFuture& future, TMaybe<TCallbackId> callbackId) {
            Set(future, callbackId);
        }

        void Set(TRepFuture& future, TMaybe<TCallbackId> callbackId) {
            Y_ENSURE(future.Initialized());
            Y_ENSURE(CallbackId.Empty());
            Future = future;
            CallbackId = callbackId;
        }

        void Swap(TRepFutureCallbackHolder& other) {
            std::swap(Future, other.Future);
            std::swap(CallbackId, other.CallbackId);
        }

        ~TRepFutureCallbackHolder() {
            if (CallbackId.Defined()) {
                Future.RemoveCallback(*CallbackId);
            }
        }

    private:
        TRepFuture Future;
        TMaybe<TCallbackId> CallbackId;
    };

    template <typename T>
    class TRepFutureReceiver {
    public:
        using TRepFuture = TAtomicRepFuture<T>;
        using TDataRef = typename TRepFuture::TDataRef;
        using TFuture = NThreading::TFuture<TMaybe<T>>;
        using TPromise = NThreading::TPromise<TMaybe<T>>;
        using TCallbackId = typename TRepFuture::TState::TCallbackId;

        explicit TRepFutureReceiver(TRepFuture future);

        TRepFutureReceiver(TRepFutureReceiver&&) noexcept = default;

        TFuture GetFuture();

    private:
        using TIterator = typename TList<TPromise>::iterator;

        void Callback(const TDataRef& result);

        // Removes already processed promises from the beginning of the list
        // After the removal, at least one of NextGet, NextSet is pointing at the beginning of the list
        void CleanPromises();

        // Safely increments iterator. Performs push_back before incrementation if required.
        // Cleans already processed promises
        // Returns false if iterator is pointing at the last element of the list after the incrementation, otherwise true
        bool IncIterator(TIterator& iterator);

        bool IncNextGet();

        bool IncNextSet();

        // Sets all promises ahead of NextSet iterator with result value
        void SetAllPromises(const TDataRef& result);

        TList<TPromise> Promises;
        // Iterators pointing at next promise to GetFuture/SetValue
        // Invariant: must be dereferenceable at any moment, use helpers to move them forward
        TIterator NextGet;
        TIterator NextSet;
        bool Finished;
        std::exception_ptr Exception;
        TMutex Mutex;

        TRepFutureCallbackHolder<T, true> CallbackHolder;
    };

    // ======= implementation

    template <typename T>
    void TRepFutureReceiver<T>::Callback(const TDataRef& result) {
        TGuard<TMutex> guard(Mutex);

        if (result.Exception()) {
            Exception = result.Exception();
            SetAllPromises(result);
            return;
        }

        if (result.Empty()) {
            Finished = true;
            SetAllPromises(result);
            return;
        }

        NextSet->SetValue(result.Data());
        IncNextSet();
    }

    template <typename T>
    void TRepFutureReceiver<T>::CleanPromises() {
        while (NextGet != Promises.begin() && NextSet != Promises.begin()) {
            Promises.pop_front();
        }
    }

    template <typename T>
    typename TRepFutureReceiver<T>::TFuture TRepFutureReceiver<T>::GetFuture() {
        TGuard<TMutex> guard(Mutex);
        TFuture future;

        if (std::next(NextGet) != Promises.end()) {
            future = NextGet->GetFuture();
            IncNextGet();
        } else {
            if (Finished) {
                future = NThreading::MakeFuture(TMaybe<T>());
            } else if (Exception) {
                future = NThreading::MakeErrorFuture<TMaybe<T>>(Exception);
            } else {
                future = NextGet->GetFuture();
                IncNextGet();
            }
        }

        return future;
    }

    template <typename T>
    TRepFutureReceiver<T>::TRepFutureReceiver(TRepFuture future)
        : Promises(/*size=*/1, NThreading::NewPromise<TMaybe<T>>())
        , NextGet(Promises.begin())
        , NextSet(Promises.begin())
        , Finished(false)
        , Exception(nullptr)
    {
        CallbackHolder.Set(
            future,
            future.AddCallback(
                [this](const TDataRef& result) {
                    Callback(result);
                }));
    }

    template <typename T>
    bool TRepFutureReceiver<T>::IncIterator(TRepFutureReceiver::TIterator& iterator) {
        if (std::next(iterator) == Promises.end()) {
            Promises.push_back(NThreading::NewPromise<TMaybe<T>>());
        }
        ++iterator;
        CleanPromises();
        return std::next(iterator) != Promises.end();
    }

    template <typename T>
    bool TRepFutureReceiver<T>::IncNextGet() {
        return IncIterator(NextGet);
    }

    template <typename T>
    bool TRepFutureReceiver<T>::IncNextSet() {
        return IncIterator(NextSet);
    }

    template <typename T>
    void TRepFutureReceiver<T>::SetAllPromises(const TRepFutureReceiver<T>::TDataRef& result) {
        do {
            if (result.Exception()) {
                NextSet->SetException(result.Exception());
            } else if (result.Empty()) {
                NextSet->SetValue(TMaybe<T>());
            } else {
                NextSet->SetValue(result.Data());
            }
        } while (IncNextSet());
    }

    // supposed to use on infinite rep future
    template <typename T>
    class TRepFutureQueueReceiver {
    public:
        using TRepFuture = TAtomicRepFuture<T>;

        explicit TRepFutureQueueReceiver(TRepFuture future) {
            CallbackHolder.ConstructInPlace(future, future.AddCallback(std::bind(&TRepFutureQueueReceiver::Push, this, std::placeholders::_1)));
        }

        TMaybe<T> Pop() {
            const TGuard guard(Mutex);

            if (Queue.empty()) {
                if (Exception) {
                    std::rethrow_exception(Exception);
                }
                return {};
            } else {
                TMaybe<T> result(std::move(Queue.front()));
                Queue.pop();
                return result;
            }
        }

        void Unsubscribe() {
            CallbackHolder.Clear();
        }

    private:
        void Push(const typename TRepFuture::TDataRef& data) {
            const TGuard guard(Mutex);
            if (data.Exception()) {
                Exception = data.Exception();
                return;
            }

            if (data.Empty()) {
                return; // just ignore finish
            }

            Queue.push(data.Data());
        }

        TMutex Mutex;
        TQueue<T> Queue;
        std::exception_ptr Exception;

        TMaybe<TRepFutureCallbackHolder<T, true>> CallbackHolder;
    };
}
