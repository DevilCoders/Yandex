#pragma once
#include <util/generic/ptr.h>
#include <util/system/mutex.h>
#include <util/system/guard.h>
#include <util/generic/vector.h>
#include <library/cpp/logger/global/global.h>
#include <util/system/condvar.h>
#include <util/thread/pool.h>
#include <util/system/event.h>
#include <kernel/common_server/util/accessor.h>
#include <util/generic/deque.h>
#include <kernel/common_server/library/unistat/cache.h>
#include <kernel/common_server/library/logging/events.h>

template <class TObject>
class TObjectsPool: TNonCopyable {
public:
    class TObjectFromPool;

private:
    TVector<TAtomicSharedPtr<TObject>> Objects;
    TMutex Mutex;

private:
    using TPtr = TAtomicSharedPtr<TObject>;

    struct TRestoreObjectInPool {
        static inline void Destroy(TObjectFromPool* t) noexcept;
        static inline void Destroy(void* t) noexcept;
    };

    class TObjectContainer {
    private:
        TPtr Object;
        TObjectsPool<TObject>* Parent;
    public:
        TObjectContainer(TPtr object, TObjectsPool<TObject>* parent)
            : Object(object)
            , Parent(parent)
        {
            if (!!Parent && !!Object) {
                AtomicIncrement(Parent->TakenObjects);
            }
        }

        void Release() {
            if (Object && Parent) {
                AtomicDecrement(Parent->TakenObjects);
                TGuard<TMutex> g(Parent->Mutex);
                for (ui32 i = 0; i < Parent->Objects.size(); ++i) {
                    CHECK_WITH_LOG(Parent->Objects[i].Get() != Object.Get());
                }
                Parent->Objects.push_back(Object);
                Object = nullptr;
                Parent = nullptr;
            }
        }

        ~TObjectContainer() {
            Release();
        }

        TObject* GetObject() {
            return Object.Get();
        }

        const TObject* GetObject() const {
            return Object.Get();
        }

        TPtr GetPtr() {
            return Object;
        }

        const TPtr GetPtr() const {
            return Object;
        }

        void Destroy() {
            Object = nullptr;
        }
    };

    TAtomic TakenObjects = 0;

public:
    ~TObjectsPool() {
        CHECK_WITH_LOG(AtomicGet(TakenObjects) == 0) << AtomicGet(TakenObjects) << Endl;
    }

    class TObjectGuard: public TAtomicSharedPtr<TObjectContainer> {
    private:
        using TBase = TAtomicSharedPtr<TObjectContainer>;

    public:
        TObjectGuard()
            : TBase(nullptr, nullptr)
        {
        }

        TObjectGuard(TPtr object, TObjectsPool<TObject>* parent)
            : TBase(MakeAtomicShared<TObjectContainer>(object, parent))
        {
        }

        TPtr operator*() {
            return TBase::Get()->GetPtr();
        }

        const TPtr operator*() const {
            return TBase::Get()->GetPtr();
        }

        bool operator!() const {
            return !TBase::Get()->GetPtr();
        }

        TObject* operator->() {
            return TBase::Get()->GetObject();
        }

        const TObject* operator->() const {
            return Get()->GetObject();
        }

        void Destroy() {
            TBase::Get()->Destroy();
        }

        void Release() {
            TBase::Get()->Release();
        }
    };

    template <class T>
    TObjectGuard GetDefault() {
        TObjectGuard result = Get();
        if (!result) {
            result = Add(MakeAtomicShared<T>());
        };
        return result;
    }

    template <class T, class... TArgs>
    TObjectGuard Get(TArgs&... args) {
        TObjectGuard result = Get();
        if (!result) {
            result = Add(MakeAtomicShared<T>(args...));
        };
        return result;
    }

    template <class T>
    TObjectGuard Get() {
        TObjectGuard result = Get();
        if (!result) {
            result = Add(MakeAtomicShared<T>());
        };
        return result;
    }

    template <class T, class... TArgs>
    TObjectGuard Get(TArgs*... args) {
        TObjectGuard result = Get();
        if (!result) {
            result = Add(MakeAtomicShared<T>(args...));
        };
        return result;
    }

    TObjectGuard Get() {
        TGuard<TMutex> g(Mutex);
        if (!Objects.size()) {
            return TObjectGuard(nullptr, this);
        }
        auto result = Objects.back();
        Objects.pop_back();
        return TObjectGuard(result, this);
    }

    TObjectGuard Add(TPtr object) {
        return TObjectGuard(object, this);
    }

    ui32 Size() const {
        TGuard<TMutex> g(Mutex);
        return Objects.size();
    }

    void Clear() {
        TGuard<TMutex> g(Mutex);
        Objects.clear();
    }
};

template <class T>
class IConstrainedObjectsPool: TNonCopyable {
private:

    using TSelf = IConstrainedObjectsPool<T>;

    class TObjContainer {
    protected:
        THolder<T> Object;
        TSelf* Parent = nullptr;
        TAtomic* Counter = nullptr;
        THolder<T> Release() {
            return std::move(Object);
        }

    public:

        TObjContainer& operator=(TObjContainer&& obj) {
            Object = std::move(obj.Object);
            Parent = obj.Parent;
            obj.Parent = nullptr;
            Counter = obj.Counter;
            return *this;
        }

        TObjContainer(TObjContainer&& obj)
            : Object(std::move(obj.Object))
            , Parent(obj.Parent)
            , Counter(obj.Counter) {
            obj.Parent = nullptr;
        }

        bool operator !() const {
            return !Object;
        }

        T& operator*() {
            return *Object;
        }

        const T& operator*() const {
            return *Object;
        }

        T* operator->() {
            return Object.Get();
        }

        const T* operator->() const {
            return Object.Get();
        }

        TObjContainer() {

        }

        ~TObjContainer() {
            if (!!Counter && !!Parent) {
                CHECK_WITH_LOG(AtomicDecrement(*Counter) >= 0);
            }
        }

        TObjContainer(T* obj, TSelf* parent, TAtomic* counter)
            : Object(obj)
            , Parent(parent)
            , Counter(counter) {
            if (Counter) {
                AtomicIncrement(*Counter);
            }
        }

        TObjContainer(TObjContainer&& object, TAtomic* counter)
            : Object(std::move(object.Release()))
            , Parent(object.Parent)
            , Counter(counter) {
            if (Counter) {
                AtomicIncrement(*Counter);
            }
        }

    };
public:
    class TActiveObject: public TObjContainer {
        RTLINE_ACCEPTOR(TActiveObject, IsAllocation, bool, false);
    public:

        T* Release() {
            return TObjContainer::Release().Release();
        }

        TActiveObject(T* obj, TSelf* parent, TAtomic* counter, bool isAllocation)
            : TObjContainer(obj, parent, counter)
            , IsAllocation(isAllocation) {
        }

        TActiveObject() = default;

        TActiveObject(TActiveObject&& obj) = default;

        using TObjContainer::TObjContainer;

        ~TActiveObject() {
            if (TObjContainer::Parent && !!TObjContainer::Object) {
                TObjContainer::Parent->ReturnObject(std::move(*this));
            }
        }
    };

private:
    class TPassiveObject: public TObjContainer {
        RTLINE_READONLY_ACCEPTOR(LastUsage, TInstant, Now());
    public:

        using TObjContainer::TObjContainer;

        TPassiveObject(TPassiveObject&& obj) = default;
        TPassiveObject& operator=(TPassiveObject&& obj) = default;

        ~TPassiveObject() {
        }
    };
    TDuration AgeForKill = TDuration::Minutes(5);
    TAtomic ActiveFlag = 0;
    ui32 HardLimit = 500;
    TDuration TimeoutHardLimitWaiting;
    TDeque<TPassiveObject> Objects;
    TMutex Mutex;
    TCondVar CondVar;
    TThreadPool Watchers;
    TSystemEvent EventStop;
    bool PanicOnProblems = true;
    TString PoolName;

    void ReturnObject(TActiveObject&& obj) {
        TGuardEvent gEvent(*this);
        TGuard<TMutex> g(Mutex);
//        DEBUG_LOG << "Return object into pool " << PoolName << Endl;
        {
            TActiveObject current = std::move(obj);
            DeactivateObject(*current);
            Objects.emplace_back(current.Release(), this, &PassiveObjects);
        }
        CondVar.Signal();
    }

    TActiveObject TakePassiveObjectUnsafe(TGuard<TMutex>& g) {
        CHECK_WITH_LOG(Objects.size());
        TActiveObject result(std::move(Objects.back()), &ActiveObjects);
        Objects.pop_back();

        g.Release();
        if (ActivateObject(*result)) {
            return result;
        }
        return TActiveObject();
    }

    class TGuardEvent {
    private:
        TSelf& Parent;
    public:

        TGuardEvent(TSelf& parent)
            : Parent(parent) {

        }

        ~TGuardEvent() {
            Parent.OnEvent();
        }
    };

    class TOldPassiveWatcher: public IObjectInQueue {
    private:
        TSelf& Parent;
    public:

        TOldPassiveWatcher(TSelf& parent)
            : Parent(parent) {

        }

        virtual void Process(void* /*threadSpecificResource*/) override {
            INFO_LOG << "deprecated removing started for " << Parent.PoolName << Endl;
            while (AtomicGet(Parent.ActiveFlag)) {
                Parent.EventStop.WaitT(Min<TDuration>(TDuration::Minutes(1), Parent.GetAgeForKill()));
                TGuardEvent gEvent(Parent);
                TGuard<TMutex> g(Parent.Mutex);
                ui32 removed = 0;
                while (Parent.Objects.size() && Parent.Objects.front().GetLastUsage() + Parent.GetAgeForKill() < Now()) {
                    Parent.Objects.pop_front();
                    ++removed;
                }
                TCSSignals::Signal("objects_pool", removed)("code", "removed")("pool", Parent.PoolName);
                TCSSignals::Signal("objects_pool", Parent.Objects.size())("code", "remained")("pool", Parent.PoolName);
            }
            INFO_LOG << "deprecated removing finished for " << Parent.PoolName << Endl;
        }
    };

    TDuration GetAgeForKill() const {
        return AgeForKill;
    }

protected:
    TAtomic ActiveObjects = 0;
    TAtomic PassiveObjects = 0;
    virtual T* AllocateObject() = 0;
    virtual bool ActivateObject(T& obj) = 0;
    virtual void DeactivateObject(T& obj) = 0;
    virtual void OnEvent() {

    }
public:

    ui32 GetHardLimitPoolSize() const {
        return HardLimit;
    }

    i64 GetActiveObjects() const {
        return AtomicGet(ActiveObjects);
    }

    i64 GetPassiveObjects() const {
        return AtomicGet(PassiveObjects);
    }

    TSelf& SetPanicOnProblem(const bool value) {
        PanicOnProblems = value;
        return *this;
    }

    TSelf& SetHardLimitInfo(const ui32 hardLimit, const TDuration hardLimitWaiting) {
        HardLimit = hardLimit;
        TimeoutHardLimitWaiting = hardLimitWaiting;
        return *this;
    }

    TSelf& SetAgeForKill(const TDuration ageForKill) {
        AgeForKill = ageForKill;
        return *this;
    }

    IConstrainedObjectsPool& SetPoolName(const TString& poolName) {
        PoolName = poolName;
        return *this;
    }

    const TString& GetPoolName() const {
        return PoolName;
    }

    TActiveObject GetObject() {
        CHECK_WITH_LOG(AtomicGet(ActiveFlag) == 1);
        auto gLogging = TFLRecords::StartContext().SignalId("objects_pool")("&pool", PoolName);
        TGuardEvent gEvent(*this);
        TGuard<TMutex> g(Mutex);
        while (true) {
            if (Objects.size()) {
                CHECK_WITH_LOG((i64)Objects.size() == AtomicGet(PassiveObjects));
                TFLEventLog::JustSignal()("&code", "passive_object_allocation");
                auto result = TakePassiveObjectUnsafe(g);
                TFLEventLog::JustSignal()("&code", "passive_object_allocated");
                return result;
            } else {
                CHECK_WITH_LOG(AtomicGet(PassiveObjects) == 0) << AtomicGet(PassiveObjects) << Endl;
                if (!HardLimit || (AtomicGet(ActiveObjects) < HardLimit)) {
                    TFLEventLog::JustSignal()("&code", "new_object_allocation");
                    g.Release();
                    auto result = TActiveObject(AllocateObject(), this, &ActiveObjects, true);
                    TFLEventLog::JustSignal()("&code", "new_object_allocated");
                    return result;
                } else {
                    TFLEventLog::Warning().Signal()("&code", "hard_limit_enriched");
                    const TString errorDescription = "hardlimit exceeded for pool \"" + PoolName + "\" of objects \"" + TypeName<T>() + "\", active objects: " + ::ToString(AtomicGet(ActiveObjects)) + ", hard limit: " + ::ToString(HardLimit) + ".";
                    if (PanicOnProblems) {
                        CHECK_WITH_LOG(CondVar.WaitT(Mutex, TimeoutHardLimitWaiting)) << "Cannot use object - " << errorDescription;
                    } else {
                        Y_ENSURE(CondVar.WaitT(Mutex, TimeoutHardLimitWaiting), "Cannot use object - " + errorDescription);
                    }
                    continue;
                }
            }
        }
    }

    virtual ~IConstrainedObjectsPool() {
        AtomicSet(ActiveFlag, 0);
        EventStop.Signal();
        Watchers.Stop();
        {
            TGuard<TMutex> g(Mutex);
            while (AtomicGet(PassiveObjects) + AtomicGet(ActiveObjects) != (i64)Objects.size()) {
                CHECK_WITH_LOG(CondVar.WaitT(Mutex, TDuration::Minutes(1))) << "Cannot stop connections gracefully" << Endl;
            }
            Objects.clear();
        }
    }

    void Start() {
        CHECK_WITH_LOG(AtomicGet(ActiveFlag) == 0);
        AtomicSet(ActiveFlag, 1);
        Watchers.Start(1);
        Watchers.SafeAddAndOwn(MakeHolder<TOldPassiveWatcher>(*this));
    }

    IConstrainedObjectsPool() {
    }

};
