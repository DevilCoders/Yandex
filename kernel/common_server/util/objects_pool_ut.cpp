#include <kernel/common_server/util/queue.h>

#include "objects_pool.h"

#include <library/cpp/testing/unittest/registar.h>
#include <util/thread/pool.h>
#include <library/cpp/logger/global/global.h>

class TTestObjectsPool: public IConstrainedObjectsPool<ui32> {
protected:
    virtual ui32* AllocateObject() {
        AtomicIncrement(Allocations);
        return new ui32(100);
    }

    virtual bool ActivateObject(ui32& obj) {
        AtomicIncrement(Activations);
        obj = 200;
        return true;
    }

    virtual void DeactivateObject(ui32& obj) {
        AtomicIncrement(Deactivations);
        obj = 0;
    }

    virtual void OnEvent() {

    }
    TAtomic Deactivations = 0;
    TAtomic Activations = 0;
    TAtomic Allocations = 0;

public:

    i64 GetAllocations() const {
        return AtomicGet(Allocations);
    }

    i64 GetDeactivations() const {
        return AtomicGet(Deactivations);
    }

    i64 GetActivations() const {
        return AtomicGet(Activations);
    }

    TTestObjectsPool() {
        SetPanicOnProblem(false);

    }
};

Y_UNIT_TEST_SUITE(ObjectsPool) {
    Y_UNIT_TEST(Simple) {
        DoInitGlobalLog("console", 7, false, false);
        TTestObjectsPool pool;
        pool.SetHardLimitInfo(100, TDuration::Seconds(5));
        pool.SetAgeForKill(TDuration::Seconds(15));
        pool.Start();
        {
            TVector<TTestObjectsPool::TActiveObject> objects;
            for (ui32 i = 0; i < 100; ++i) {
                objects.push_back(pool.GetObject());
                CHECK_WITH_LOG(*objects.back() == 100) << *objects.back();
                *objects.back() = 201;
                CHECK_WITH_LOG(pool.GetActiveObjects() == i + 1);
                CHECK_WITH_LOG(pool.GetPassiveObjects() == 0);
            }
            TInstant start = Now();
            try {
                pool.GetObject();
                S_FAIL_LOG << "Incorrect behaviour" << Endl;
            } catch (...) {

            }
            CHECK_WITH_LOG(Now() - start > TDuration::Seconds(4));
            CHECK_WITH_LOG(Now() - start < TDuration::Seconds(10));
            objects.resize(50);
            CHECK_WITH_LOG(pool.GetActiveObjects() == 50);
            CHECK_WITH_LOG(pool.GetPassiveObjects() == 50);
        }
        CHECK_WITH_LOG(pool.GetDeactivations() == 100);
        CHECK_WITH_LOG(pool.GetActivations() == 0);
        CHECK_WITH_LOG(pool.GetAllocations() == 100);
        {
            auto obj = pool.GetObject();
            CHECK_WITH_LOG(*obj == 200);
            CHECK_WITH_LOG(pool.GetDeactivations() == 100);
            CHECK_WITH_LOG(pool.GetActivations() == 1);
            CHECK_WITH_LOG(pool.GetAllocations() == 100);
        }
        CHECK_WITH_LOG(pool.GetDeactivations() == 101);
        CHECK_WITH_LOG(pool.GetActiveObjects() == 0);
        CHECK_WITH_LOG(pool.GetPassiveObjects() == 100);
        TInstant start = Now();
        while (Now() - start < TDuration::Seconds(60)) {
            Sleep(TDuration::Seconds(1));
            if (pool.GetActiveObjects() == 0 && pool.GetPassiveObjects() == 1)
                break;
            auto obj = pool.GetObject();
        }
        CHECK_WITH_LOG(pool.GetActiveObjects() == 0);
        CHECK_WITH_LOG(pool.GetPassiveObjects() == 1);

        auto obj = pool.GetObject();
        CHECK_WITH_LOG(*obj == 200);
        CHECK_WITH_LOG(pool.GetActiveObjects() == 1);
        CHECK_WITH_LOG(pool.GetPassiveObjects() == 0);
    }

    class TDataAquire: public IObjectInQueue {
    private:
        TTestObjectsPool& Pool;
        TAtomic& Data;
    public:

        TDataAquire(TTestObjectsPool& pool, TAtomic& data)
            : Pool(pool)
            , Data(data)
        {

        }

        virtual void Process(void* /*threadSpecificResource*/) {
            auto obj = Pool.GetObject();
            {
                auto data = AtomicIncrement(Data);
                CHECK_WITH_LOG(data <= Pool.GetHardLimitPoolSize() && data >= 0);
            }
            Sleep(TDuration::MilliSeconds(100));
            if (obj.GetIsAllocation()) {
                CHECK_WITH_LOG(*obj == 100);
            } else {
                CHECK_WITH_LOG(*obj == 200);
            }
            *obj = 201;
            {
                auto data = AtomicDecrement(Data);
                CHECK_WITH_LOG(data + 1 <= Pool.GetHardLimitPoolSize() && data >= 0);
            }
        }
    };

    Y_UNIT_TEST(SimpleMT) {
        DoInitGlobalLog("console", 7, false, false);
        TTestObjectsPool pool;
        TThreadPool queue;
        TAtomic data = 0;
        pool.SetHardLimitInfo(2, TDuration::Seconds(500));
        pool.Start();
        queue.Start(16);
        for (ui32 i = 0; i < 600; ++i) {
            queue.SafeAddAndOwn(MakeHolder<TDataAquire>(pool, data));
        }
        queue.Stop();
    }

    Y_UNIT_TEST(SimpleMT1) {
        DoInitGlobalLog("console", 7, false, false);
        TTestObjectsPool pool;
        TThreadPool queue;
        TAtomic data = 0;
        pool.SetHardLimitInfo(24, TDuration::Seconds(500));
        pool.Start();
        queue.Start(16);
        for (ui32 i = 0; i < 600; ++i) {
            queue.SafeAddAndOwn(MakeHolder<TDataAquire>(pool, data));
        }
        queue.Stop();
    }

    Y_UNIT_TEST(SimpleMT2) {
        DoInitGlobalLog("console", 7, false, false);
        TTestObjectsPool pool;
        TThreadPool queue;
        TAtomic data = 0;
        pool.SetHardLimitInfo(14, TDuration::Seconds(500));
        pool.Start();
        queue.Start(16);
        for (ui32 i = 0; i < 600; ++i) {
            queue.SafeAddAndOwn(MakeHolder<TDataAquire>(pool, data));
        }
        queue.Stop();
    }
}
