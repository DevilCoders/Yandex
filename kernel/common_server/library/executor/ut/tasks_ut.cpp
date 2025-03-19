#include <kernel/common_server/library/executor/vstorage/queue.h>
#include <kernel/common_server/library/executor/vstorage/storage.h>
#include <kernel/common_server/library/executor/ut/helpers/config.h>
#include <kernel/common_server/library/executor/ut/proto/task.pb.h>
#include <kernel/common_server/library/executor/executor.h>
#include <kernel/daemon/config/daemon_config.h>
#include <library/cpp/logger/global/global.h>
#include <library/cpp/protobuf/json/json2proto.h>
#include <library/cpp/protobuf/json/proto2json.h>
#include <library/cpp/testing/common/env.h>
#include <library/cpp/testing/unittest/registar.h>
#include <util/generic/cast.h>
#include <util/string/vector.h>

class TCounterData: public IDistributedData {
protected:
    using TBase = IDistributedData;
    NFrontendProto::TTestData Task;
    static TFactory::TRegistrator<TCounterData> Registrator;
public:

    using TBase::TBase;
    static const TString TypeName;

    TCounterData(const NJson::TJsonValue& info)
        : IDistributedData(TDataConstructionContext::Build<TCounterData>("1"))
    {
        NJson::TJsonValue::TMapType mapGlobal;
        CHECK_WITH_LOG(info.GetMap(&mapGlobal));
        auto it = mapGlobal.find("data");
        CHECK_WITH_LOG(it != mapGlobal.end());
        NProtobufJson::Json2Proto(it->second, Task);
    }

    virtual NJson::TJsonValue DoGetInfo(const TCgiParameters* /*cgi*/) const override {
        NJson::TJsonValue result(NJson::JSON_MAP);
        NProtobufJson::Proto2Json(Task, result);
        return result;
    }

    virtual TBlob Serialize() const override {
        TString result;
        UNIT_ASSERT(Task.SerializeToString(&result));
        return TBlob::FromString(result);
    }

    virtual bool Deserialize(const TBlob& data) override {
        return Task.ParseFromArray(data.AsCharPtr(), data.Size());
    }

    NFrontendProto::TTestData& MutableData() {
        return Task;
    }

    TCounterData()
        : IDistributedData(TDataConstructionContext::Build<TCounterData>("1"))
    {
        Task.SetCounter(1);
        Task.SetCounterDeferred(0);
        Task.SetUsageCounter(0);
        Task.SetCounterClear(0);
    }
};

const TString TCounterData::TypeName = "CounterData";
TCounterData::TFactory::TRegistrator<TCounterData> TCounterData::Registrator(TCounterData::TypeName);


class TDataWatcher: public IDistributedTask {
private:
    using TBase = IDistributedTask;
protected:
    virtual bool IsCorrect() const = 0;
    virtual void IncrementUsage() = 0;

    virtual bool DoExecute(IDistributedTask::TPtr self) noexcept override {
        TCounterData* data = &GetData<TCounterData>();
        INFO_LOG << GetIdentifier() << ": " << data->GetDataInfo() << Endl;
        data->MutableData().SetUsageCounter(data->MutableData().GetUsageCounter() + 1);
        CHECK_WITH_LOG(Executor->StoreData2(data));
        IncrementUsage();
        if (data->MutableData().GetCounterDeferred() == 1 && IsCorrect()) {
            INFO_LOG << "WATCHER_FINISHED" << Endl;
            return true;
        } else {
            Executor->RescheduleTask(self.Get(), Now() + TDuration::MilliSeconds(200));
            return false;
        }
    }
public:
    using TBase::TBase;
};

class TDataWatcherCorrect: public TDataWatcher {
private:
    using TBase = TDataWatcher;
    static TFactory::TRegistrator<TDataWatcherCorrect> Registrator;
protected:
    virtual bool IsCorrect() const override {
        return true;
    }

    virtual void IncrementUsage() override {
        INFO_LOG << "correct: " << AtomicIncrement(UsageCounter) << Endl;
    }
public:
    using TBase::TBase;
    static TAtomic UsageCounter;
    static const TString TypeName;

    TDataWatcherCorrect()
        : TDataWatcher(TTaskConstructionContext::Build<TDataWatcherCorrect>("watcher-correct")) {
    }
};

const TString TDataWatcherCorrect::TypeName = "DataWatcherCorrect";
TDataWatcher::TFactory::TRegistrator<TDataWatcherCorrect> TDataWatcherCorrect::Registrator(TDataWatcherCorrect::TypeName);
TAtomic TDataWatcherCorrect::UsageCounter = 0;

class TDataWatcherIncorrect: public TDataWatcher {
private:
    using TBase = TDataWatcher;
    static TFactory::TRegistrator<TDataWatcherIncorrect> Registrator;
protected:
    virtual bool IsCorrect() const override {
        return false;
    }

    virtual void IncrementUsage() override {
        AtomicIncrement(UsageCounter);
    }
public:
    using TBase::TBase;
    static TAtomic UsageCounter;
    static const TString TypeName;

    TDataWatcherIncorrect()
        : TDataWatcher(TTaskConstructionContext::Build<TDataWatcherIncorrect>("watcher-incorrect")) {
    }
};

const TString TDataWatcherIncorrect::TypeName = "DataWatcherIncorrect";
TDataWatcherIncorrect::TFactory::TRegistrator<TDataWatcherIncorrect> TDataWatcherIncorrect::Registrator(TDataWatcherIncorrect::TypeName);
TAtomic TDataWatcherIncorrect::UsageCounter = 0;

class TClearTask: public IDistributedTask {
private:
    using TBase = IDistributedTask;
    static TFactory::TRegistrator<TClearTask> Registrator;
protected:
    virtual bool DoExecute(IDistributedTask::TPtr /*self*/) noexcept override {
        TCounterData* data = &GetData<TCounterData>();
        INFO_LOG << GetIdentifier() << ": " << data->GetDataInfo() << Endl;
        data->MutableData().SetCounterClear(data->MutableData().GetCounterClear() + 1);
        data->SetDeadline(Now() + TDuration::Seconds(20));
        CHECK_WITH_LOG(Executor->StoreData2(data));
        return true;
    }
public:

    static const TString TypeName;
    using TBase::TBase;

    TClearTask()
        : IDistributedTask(TTaskConstructionContext::Build<TClearTask>("clear")) {
    }
};

const TString TClearTask::TypeName = "Clear";
TClearTask::TFactory::TRegistrator<TClearTask> TClearTask::Registrator(TClearTask::TypeName);

class TDeferredPrintTask: public IDistributedTask {
private:
    using TBase = IDistributedTask;
    static TFactory::TRegistrator<TDeferredPrintTask> Registrator;
protected:
    virtual bool DoExecute(IDistributedTask::TPtr /*self*/) noexcept override {
        AtomicIncrement(UsageCounter);
        TCounterData* data = &GetData<TCounterData>();
        INFO_LOG << data->MutableData().GetCounterDeferred() << Endl;
        INFO_LOG << GetIdentifier() << ": " << data->GetDataInfo() << Endl;
        data->MutableData().SetCounterDeferred(data->MutableData().GetCounterDeferred() + 1);
        CHECK_WITH_LOG(Executor->StoreData2(data));

        TClearTask task;

        CHECK_WITH_LOG(Executor->StoreTask2(&task));
        Executor->EnqueueTask(task.GetIdentifier(), data->GetIdentifier());
        return true;
    }
public:
    static TAtomic UsageCounter;
    static const TString TypeName;
    using TBase::TBase;

    TDeferredPrintTask()
        : IDistributedTask(TTaskConstructionContext::Build<TDeferredPrintTask>("def-2")) {
    }
};

const TString TDeferredPrintTask::TypeName = "DeferredPrint";
TDeferredPrintTask::TFactory::TRegistrator<TDeferredPrintTask> TDeferredPrintTask::Registrator(TDeferredPrintTask::TypeName);
TAtomic TDeferredPrintTask::UsageCounter = 0;

class TConstructorTask: public IDistributedTask {
private:
    using TBase = IDistributedTask;
    static TFactory::TRegistrator<TConstructorTask> Registrator;
    NFrontendProto::TTestTask Task;
protected:
    virtual bool DoExecute(IDistributedTask::TPtr /*self*/) noexcept override {
        AtomicIncrement(UsageCounter);
        TCounterData data;
        data.MutableData().SetCounter(data.MutableData().GetCounter() + Task.GetIncrement());
        CHECK_WITH_LOG(Executor->StoreData2(&data));
        return true;
    }
public:
    static TAtomic UsageCounter;

    virtual TBlob Serialize() const override {
        TString result;
        UNIT_ASSERT(Task.SerializeToString(&result));
        return TBlob::FromString(result);
    }

    virtual bool Deserialize(const TBlob& data) override {
        return Task.ParseFromArray(data.AsCharPtr(), data.Size());
    }

    static const TString TypeName;
    using TBase::TBase;

    TConstructorTask(const TString& id)
        : IDistributedTask(TTaskConstructionContext::Build<TConstructorTask>(id)) {
        Task.SetIncrement(1000);
    }
};

const TString TConstructorTask::TypeName = "Constructor";
TConstructorTask::TFactory::TRegistrator<TConstructorTask> TConstructorTask::Registrator(TConstructorTask::TypeName);
TAtomic TConstructorTask::UsageCounter = 0;

class TPrintTask: public IDistributedTask {
private:
    using TBase = IDistributedTask;
    static TFactory::TRegistrator<TPrintTask> Registrator;
protected:
    virtual bool DoExecute(IDistributedTask::TPtr /*self*/) noexcept override {
        TCounterData* data = &GetData<TCounterData>();
        INFO_LOG << data->MutableData().GetCounter() << Endl;
        INFO_LOG << GetIdentifier() << ": " << data->GetDataInfo() << Endl;
        data->MutableData().SetCounter(data->MutableData().GetCounter() + 1);
        CHECK_WITH_LOG(Executor->StoreData2(data));
        AtomicIncrement(UsageCounter);
        return true;
    }
public:

    static const TString TypeName;
    using TBase::TBase;
    static TAtomic UsageCounter;

    TPrintTask(const TString& id)
        : IDistributedTask(TTaskConstructionContext::Build<TPrintTask>(id)) {
    }


};

TAtomic TPrintTask::UsageCounter = 0;
const TString TPrintTask::TypeName = "Print";
TPrintTask::TFactory::TRegistrator<TPrintTask> TPrintTask::Registrator(TPrintTask::TypeName);

class TCalcerTask: public IDistributedTask {
private:
    using TBase = IDistributedTask;
    static TFactory::TRegistrator<TCalcerTask> Registrator;
    ui32 IdHash = 100;
protected:

    bool IsLocal() const {
        return GetIdentifier().find("local") != TString::npos;
    }

    virtual bool DoOnTimeout(IDistributedTask::TPtr /*self*/) noexcept override {
        AtomicIncrement(TimeoutCounter);
        return true;
    }

    virtual bool DoExecute(IDistributedTask::TPtr /*self*/) noexcept override {
        if (GetIdentifier().find("checker") != TString::npos) {
            if (IsLocal()) {
                CHECK_WITH_LOG(IdHash == 200);
            } else {
                CHECK_WITH_LOG(IdHash == 100);
            }
            CHECK_WITH_LOG(AtomicGet(UsageCounter) == 100 || AtomicGet(UsageCounter) == 101) << AtomicGet(UsageCounter) << " / " << GetIdentifier() << Endl;
        }
        AtomicIncrement(UsageCounter);
        const i64 current = AtomicIncrement(UsageCurrent);
        Sleep(TDuration::MilliSeconds(500));
        {
            TGuard<TMutex> g(Mutex);
            AtomicSet(MaxInTime, Max<i64>(AtomicGet(MaxInTime), current));
        }
        AtomicDecrement(UsageCurrent);
        return true;
    }
public:

    static const TString TypeName;
    using TBase::TBase;
    static TAtomic UsageCounter;
    static TAtomic MaxInTime;
    static TAtomic UsageCurrent;
    static TMutex Mutex;
    static TAtomic TimeoutCounter;

    TCalcerTask(const TString& id)
        : IDistributedTask(TTaskConstructionContext::Build<TCalcerTask>(id)) {
        IdHash = 200;
    }


};

TMutex TCalcerTask::Mutex;
TAtomic TCalcerTask::UsageCounter = 0;
TAtomic TCalcerTask::UsageCurrent = 0;
TAtomic TCalcerTask::TimeoutCounter = 0;
TAtomic TCalcerTask::MaxInTime = 0;
const TString TCalcerTask::TypeName = "CalcerTask";
TCalcerTask::TFactory::TRegistrator<TCalcerTask> TCalcerTask::Registrator(TCalcerTask::TypeName);

template <bool Strict = true>
void Wait(TAtomic& counter, const ui32 purpouseValue, const TDuration checkDurationLimit, const TDuration checkDurationAfterPurposeReady = TDuration::Zero()) {
    bool found = false;
    {
        const TInstant start = Now();
        while (Now() - start < checkDurationLimit) {
            if ((Strict && AtomicGet(counter) == purpouseValue) || (!Strict && AtomicGet(counter) >= purpouseValue)) {
                found = true;
                break;
            }
            Sleep(TDuration::MilliSeconds(100));
        }
    }
    CHECK_WITH_LOG(found);
    TInstant start = Now();
    while (Now() - start <= checkDurationAfterPurposeReady) {
        CHECK_WITH_LOG((Strict && AtomicGet(counter) == purpouseValue) || (!Strict && AtomicGet(counter) >= purpouseValue));
        Sleep(TDuration::MilliSeconds(100));
    }
}

void WaitQueueSize(TTaskExecutor& executor, const ui32 expectedSize, const TDuration checkDurationLimit = TDuration::Seconds(100)) {
    const TInstant start = Now();
    while (Now() - start < checkDurationLimit) {
        ui32 count = 0;
        for (auto&& i : executor.GetQueueNodes()) {
            if (i != "deprecated-cleaner") {
                ++count;
            }
        }
        if (count == expectedSize) {
            return;
        }
        Sleep(TDuration::MilliSeconds(100));
    }
    S_FAIL_LOG << "cannot wait queue size " << expectedSize;
}

class TTaskExecutorGuard {
private:
    const TString StorageType;
    THolder<TTaskExecutor> Executor;
public:

    TTaskExecutorGuard(TTaskExecutorGuard&& guard)
        : StorageType(guard.StorageType)
        , Executor(guard.Executor.Release())
    {

    }

    TTaskExecutorGuard(const TTaskExecutorConfig& config, const TString& storageType)
        : StorageType(storageType)
    {
        Executor = MakeHolder<TTaskExecutor>(config, nullptr);
        Executor->SetCleaningInterval(TDuration::Seconds(5));
        Executor->Start();
    }

    ~TTaskExecutorGuard() {
        if (!!Executor) {
            Executor->Stop();
            Executor.Destroy();
            if (StorageType == "LOCAL") {
                {
                    TVector<TString> names;
                    TFsPath("./000/tasks_queue/").ListNames(names);
                    CHECK_WITH_LOG(names.size() <= 3) << names.size();
                }
                {
                    TVector<TString> names;
                    TFsPath("./111/").ListNames(names);
                    CHECK_WITH_LOG(names.size() <= 2) << JoinStrings(names, ",") << Endl;
                }
            }
        }
    }

    const TTaskExecutor& operator*() const {
        return *Executor;
    }

    TTaskExecutor& operator*() {
        return *Executor;
    }

    const TTaskExecutor* operator->() const {
        return Executor.Get();
    }

    TTaskExecutor* operator->() {
        return Executor.Get();
    }
};

TTaskExecutorGuard BuildExecutor(const TString& storageType, const ui32 threadsCount = 16, const bool syncMode = true) {
    TTaskExecutorGuard result(BuildExecutorConfig(storageType, threadsCount, syncMode), storageType);
    return result;
}

/* Possible storage types:
    - LOCAL
    - Postgres
    - Postgres-ZOO - Postgres with a locker type ZOO
Usage:
    ut_binary --test-param storage-type=Postgres
*/
Y_UNIT_TEST_SUITE(FrontendTasks) {
    Y_UNIT_TEST(TestLocksPool) {
        TString storageType = GetTestParam("storage-type", "LOCAL");

        TTaskExecutorGuard executor = BuildExecutor(storageType, 16, false);
        IDTasksQueue::TRestoreDataMeta dataLock("1_lock");
        dataLock.SetExistanceRequirement(EExistanceRequirement::LockPool);
        dataLock.SetLockPoolSize(10);
        for (ui32 i = 0; i < 100; ++i) {
            auto constructorTask = MakeAtomicShared<TCalcerTask>(ToString(i) + "-calc");
            CHECK_WITH_LOG(executor->StoreTask2(constructorTask.Get()));
        }
        {
            auto constructorTask = MakeAtomicShared<TCalcerTask>("calc-checker");
            CHECK_WITH_LOG(executor->StoreTask2(constructorTask.Get()));
        }
        {
            auto constructorTask = MakeAtomicShared<TCalcerTask>("calc-checker-deadline");
            CHECK_WITH_LOG(executor->StoreTask2(constructorTask.Get()));
        }
        {
            auto constructorTask = MakeAtomicShared<TCalcerTask>("calc-checker-deadline0");
            CHECK_WITH_LOG(executor->StoreTask2(constructorTask.Get()));
        }
        {
            executor->StoreLocalTask(MakeAtomicShared<TCalcerTask>("calc-checker-local"));
            executor->StoreLocalTask(MakeAtomicShared<TCalcerTask>("calc-checker-local-deadline"));
            executor->StoreLocalTask(MakeAtomicShared<TCalcerTask>("calc-checker-local-deadline0"));
        }
        TVector<TString> waitTasks;
        for (ui32 i = 0; i < 100; ++i) {
            executor->EnqueueTask(ToString(i) + "-calc", dataLock, "", Now() + TDuration::Seconds(10));
            waitTasks.push_back(ToString(i) + "-calc");
        }
        executor->EnqueueTask("calc-checker", { dataLock }, waitTasks);
        executor->EnqueueTask("calc-checker-local", { dataLock }, waitTasks);

        executor->EnqueueTask("calc-checker-deadline", { dataLock }, waitTasks, TInstant::Zero(), Now() + TDuration::Seconds(1));
        executor->EnqueueTask("calc-checker-local-deadline", { dataLock }, waitTasks, TInstant::Zero(), Now() + TDuration::Seconds(1));
        executor->EnqueueTask("calc-checker-deadline0", { dataLock }, waitTasks, TInstant::Zero(), Now());
        executor->EnqueueTask("calc-checker-local-deadline0", { dataLock }, waitTasks, TInstant::Zero(), Now());

        Wait(TCalcerTask::UsageCounter, 102, TDuration::Seconds(100), TDuration::Seconds(5));
        CHECK_WITH_LOG(AtomicGet(TCalcerTask::TimeoutCounter) == 4) << AtomicGet(TCalcerTask::TimeoutCounter) << Endl;

        CHECK_WITH_LOG(AtomicGet(TCalcerTask::MaxInTime) <= 10);
        CHECK_WITH_LOG(AtomicGet(TCalcerTask::MaxInTime) > 2);
        INFO_LOG << AtomicGet(TCalcerTask::MaxInTime) << Endl;
    }

    Y_UNIT_TEST(TestConcurrentConstruction) {
        TString storageType = GetTestParam("storage-type", "LOCAL");

        TTaskExecutorGuard executor = BuildExecutor(storageType, 16, false);

        IDTasksQueue::TRestoreDataMeta dataMeta("1");
        dataMeta.SetExistanceRequirement(EExistanceRequirement::NoExists);
        for (ui32 i = 0; i < 10; ++i) {
            auto constructorTask = MakeAtomicShared<TConstructorTask>(ToString(i) + "-construction");
            CHECK_WITH_LOG(executor->StoreTask2(constructorTask.Get()));
        }
        for (ui32 i = 0; i < 10; ++i) {
            executor->EnqueueTask(ToString(i) + "-construction", dataMeta);
        }

        Wait(TConstructorTask::UsageCounter, 1, TDuration::Seconds(100), TDuration::Seconds(5));
        executor->RemoveData("1");
    }

    Y_UNIT_TEST(Test1) {
        TString storageType = GetTestParam("storage-type", "LOCAL");

        TTaskExecutorGuard executor = BuildExecutor(storageType, 16, false);

        {
            const ui32 numberOfTasks = 5;
            const TDuration delayDeferredTask = TDuration::Seconds(5);

            TCounterData data;
            auto dTask = MakeAtomicShared<TDeferredPrintTask>();
            auto wTaskCorrect = MakeAtomicShared<TDataWatcherCorrect>();
            auto wTaskIncorrect = MakeAtomicShared<TDataWatcherIncorrect>();

            IDTasksQueue::TRestoreDataMeta dataMeta(data.GetIdentifier());
            dataMeta.SetExistanceRequirement(EExistanceRequirement::NoExists);
            auto constructorTask = MakeAtomicShared<TConstructorTask>("0-construction");
            CHECK_WITH_LOG(executor->StoreTask2(constructorTask.Get()));
            executor->EnqueueTask("0-construction", dataMeta);
            Wait(TConstructorTask::UsageCounter, 1, TDuration::Seconds(100));

            CHECK_WITH_LOG(executor->StoreTask2(wTaskCorrect.Get()));
            executor->EnqueueTask(wTaskCorrect->GetIdentifier(), data.GetIdentifier());

            CHECK_WITH_LOG(executor->StoreTask2(wTaskIncorrect.Get()));
            executor->EnqueueTask(wTaskIncorrect->GetIdentifier(), data.GetIdentifier());

            Wait<false>(TDataWatcherCorrect::UsageCounter, 10, TDuration::Seconds(100));
            Wait<false>(TDataWatcherIncorrect::UsageCounter, 10, TDuration::Seconds(100));

            CHECK_WITH_LOG(executor->StoreTask2(dTask.Get()));
            const TInstant startDeferredTask = Now();
            executor->EnqueueTask(dTask->GetIdentifier(), data.GetIdentifier(), "", Now() + delayDeferredTask);
            for (ui32 i = 0; i < numberOfTasks; ++i) {
                auto task = MakeAtomicShared<TPrintTask>("print-" + ToString(i));
                CHECK_WITH_LOG(executor->StoreTask2(task.Get()));
                executor->EnqueueTask(task->GetIdentifier(), data.GetIdentifier());
            }
            {
                auto task = MakeAtomicShared<TPrintTask>("print-incorrect");
                executor->EnqueueTask(task->GetIdentifier(), "1");
            }

            Wait(TPrintTask::UsageCounter, numberOfTasks, TDuration::Seconds(100));

#define COMPARE_WITH_LOG_EQ(A, B) \
    CHECK_WITH_LOG((A) == (B)) << "; actual: " << (A)

#define COMPARE_WITH_LOG_GT(A, B) \
    CHECK_WITH_LOG((A) > (B)) << "; actual: " << (A)

            {
                TCounterData dataCheck(executor->GetDataInfo("1"));
                COMPARE_WITH_LOG_EQ(dataCheck.MutableData().GetCounter(), 1000 + numberOfTasks + 1);
                if (Now() - startDeferredTask < delayDeferredTask) {
                    COMPARE_WITH_LOG_EQ(dataCheck.MutableData().GetCounterDeferred(), 0);
                }
            }

            Wait(TDeferredPrintTask::UsageCounter, 1, TDuration::Seconds(100), TDuration::Seconds(10));
            INFO_LOG << executor->GetDataInfo("1").GetStringRobust() << Endl;
            {
                TCounterData dataCheck(executor->GetDataInfo("1"));
                COMPARE_WITH_LOG_EQ(dataCheck.MutableData().GetCounter(), 1000 + numberOfTasks + 1);
                COMPARE_WITH_LOG_EQ(dataCheck.MutableData().GetCounterDeferred(), 1);
                COMPARE_WITH_LOG_EQ(dataCheck.MutableData().GetCounterClear(), 1);
                COMPARE_WITH_LOG_EQ(dataCheck.MutableData().GetUsageCounter(), AtomicGet(TDataWatcherCorrect::UsageCounter) + AtomicGet(TDataWatcherIncorrect::UsageCounter));
            }
            INFO_LOG << JoinStrings(executor->GetQueueNodes(), ",") << Endl;
            WaitQueueSize(*executor, 1);
            WaitQueueSize(*executor, 0);
        }
    }
}
