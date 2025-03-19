#include "library/helper.h"

#include <kernel/daemon/config/daemon_config.h>
#include <util/generic/cast.h>
#include <util/string/join.h>
#include <kernel/common_server/library/storage/config.h>
#include <kernel/common_server/library/storage/abstract.h>
#include <kernel/common_server/library/storage/postgres/postgres_storage.h>
#include <kernel/common_server/library/storage/postgres/table_accessor.h>
#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/common/env.h>
#include <util/thread/pool.h>


using namespace NStorage;
using namespace NRTProc;


Y_UNIT_TEST_SUITE(Storages) {

    class TIncDataActor: public IObjectInQueue {
    private:
        ui32* Counter;
        IVersionedStorage::TPtr Storage;
    public:

        TIncDataActor(ui32* counter, IVersionedStorage::TPtr storage)
            : Counter(counter)
            , Storage(storage)
        {

        }

        virtual void Process(void* /*threadSpecificResource*/) override {
            auto lock = Storage->WriteLockNode(ToString((ui64)Counter));
            ++(*Counter);
            if ((*Counter) % 10 == 0) {
                INFO_LOG << (*Counter) << Endl;
            }
        }
    };

    void DoKvTest(const TStorageOptions& options, const TString& storageType, bool storeHistory = true) {
        IVersionedStorage::TPtr storage = options.ConstructStorage();

        UNIT_ASSERT(storage->SetValue("k1", "xxx", storeHistory));

        TString value;
        UNIT_ASSERT(storage->GetValue("k1", value));
        UNIT_ASSERT_VALUES_EQUAL(value, "xxx");
        UNIT_ASSERT(storage->ExistsNode("k1"));
        UNIT_ASSERT(!storage->ExistsNode("k2"));

        UNIT_ASSERT(storage->RemoveNode("k1", storeHistory));
        UNIT_ASSERT(!storage->ExistsNode("k1"));

        UNIT_ASSERT(storage->SetValue("/d1", "1", storeHistory));
        UNIT_ASSERT(storage->SetValue("/d2", "2", storeHistory));
        UNIT_ASSERT(storage->SetValue("/d1/d11", "11", storeHistory));
        UNIT_ASSERT(storage->SetValue("/d1/d12", "12", storeHistory));
        UNIT_ASSERT(storage->SetValue("/d1/n13", "13", storeHistory));
        UNIT_ASSERT(storage->SetValue("/d1/d11/n111", "111", storeHistory));
        UNIT_ASSERT(storage->SetValue("/d1/d11/n112", "112", storeHistory));
        UNIT_ASSERT(storage->SetValue("/d1/d11/d111/n1111", "1111", storeHistory));
        UNIT_ASSERT(storage->SetValue("/d1/d12/n121", "121", storeHistory));
        UNIT_ASSERT(storage->SetValue("/d11/d12/n121", "121", storeHistory));
        UNIT_ASSERT(storage->SetValue("/d2/d21/n211", "211", storeHistory));

        TVector<TString> nodes;
        UNIT_ASSERT(storage->GetNodes("/d1", nodes, false));
        INFO_LOG << JoinRange(",", nodes.begin(), nodes.end()) << Endl;
        UNIT_ASSERT_VALUES_EQUAL(nodes.size(), 3); // expected: n13,d11,d12
        nodes.clear();
        UNIT_ASSERT(storage->GetNodes("/d1", nodes, true));
        INFO_LOG << JoinRange(",", nodes.begin(), nodes.end()) << Endl;
        UNIT_ASSERT_VALUES_EQUAL(nodes.size(), 3); // expected: n13,d11,d12

        nodes.clear();
        UNIT_ASSERT(storage->GetNodes("/d2", nodes, false));
        INFO_LOG << JoinRange(",", nodes.begin(), nodes.end()) << Endl;
        UNIT_ASSERT_VALUES_EQUAL(nodes.size(), 0); // expected: (empty)
        nodes.clear();
        UNIT_ASSERT(storage->GetNodes("/d2", nodes, true));
        INFO_LOG << JoinRange(",", nodes.begin(), nodes.end()) << Endl;
        UNIT_ASSERT_VALUES_EQUAL(nodes.size(), 1); // expected: d21

        UNIT_ASSERT(storage->SetValue("/d2/d21", "21", storeHistory));

        if (storageType != "RTY") {
            nodes.clear();
            UNIT_ASSERT(storage->GetNodes("/d2", nodes, false));
            INFO_LOG << JoinRange(",", nodes.begin(), nodes.end()) << Endl;
            UNIT_ASSERT_VALUES_EQUAL(nodes.size(), 1); // expected: d21
            nodes.clear();
            UNIT_ASSERT(storage->GetNodes("/d2", nodes, true));
            INFO_LOG << JoinRange(",", nodes.begin(), nodes.end()) << Endl;
            UNIT_ASSERT_VALUES_EQUAL(nodes.size(), 1); // expected: d21
        }

        nodes.clear();
        UNIT_ASSERT(storage->GetNodes("/d1/d11", nodes, false));
        INFO_LOG << JoinRange(",", nodes.begin(), nodes.end()) << Endl;
        UNIT_ASSERT_VALUES_EQUAL(nodes.size(), 2); // expected: n111,n112
        nodes.clear();
        UNIT_ASSERT(storage->GetNodes("/d1/d11", nodes, true));
        INFO_LOG << JoinRange(",", nodes.begin(), nodes.end()) << Endl;
        UNIT_ASSERT_VALUES_EQUAL(nodes.size(), 3); // expected: n111,n112,d111

        UNIT_ASSERT(storage->RemoveNode("/d1", storeHistory));
        nodes.clear();
        UNIT_ASSERT(!storage->GetNodes("/d1", nodes, true));
        INFO_LOG << JoinRange(",", nodes.begin(), nodes.end()) << Endl;
        UNIT_ASSERT_VALUES_EQUAL(nodes.size(), 0); // expected: (empty)

        UNIT_ASSERT(storage->RemoveNode("/d2", storeHistory));
        nodes.clear();
        UNIT_ASSERT(!storage->GetNodes("/d2", nodes, true));
        INFO_LOG << JoinRange(",", nodes.begin(), nodes.end()) << Endl;
        UNIT_ASSERT_VALUES_EQUAL(nodes.size(), 0); // expected: (empty)

        UNIT_ASSERT(storage->RemoveNode("/d11", storeHistory));
        nodes.clear();
        UNIT_ASSERT(!storage->GetNodes("/d11", nodes, true));
        INFO_LOG << JoinRange(",", nodes.begin(), nodes.end()) << Endl;
        UNIT_ASSERT_VALUES_EQUAL(nodes.size(), 0); // expected: (empty)


        // check versioning
        i64 ver1;
        if (!storeHistory) {
            UNIT_ASSERT(storage->SetValue("k1", "v1", false, true, &ver1));
        } else {
            UNIT_ASSERT(storage->SetValue("k1", "v1", true, true, &ver1));
        }
        INFO_LOG << "ver1: " << ver1 << Endl;
        TString last_value, version_value;
        UNIT_ASSERT(storage->GetValue("k1", last_value));
        UNIT_ASSERT(storage->GetValue("k1", version_value, ver1));
        UNIT_ASSERT_VALUES_EQUAL(last_value, version_value);

        i64 ver2, verg;
        if (!storeHistory) {
            UNIT_ASSERT(storage->SetValue("k1", "v2", false, true, &ver2));
        } else {
            UNIT_ASSERT(storage->SetValue("k1", "v2", true, true, &ver2));
        }
        UNIT_ASSERT(storage->GetValue("k1", version_value));
        UNIT_ASSERT_VALUES_EQUAL(version_value, "v2");

        INFO_LOG << "ver2: " << ver2 << Endl;
        UNIT_ASSERT(storage->GetVersion("k1", verg));
        UNIT_ASSERT_VALUES_EQUAL(ver2, verg);

        if (!storeHistory && storageType != "LOCAL") {
            if (ver1 != -1) {
                UNIT_ASSERT(!storage->GetValue("k1", version_value, ver1));
            }
        } else {
            UNIT_ASSERT(storage->GetValue("k1", version_value, ver1));
            UNIT_ASSERT_VALUES_EQUAL(last_value, version_value);
            UNIT_ASSERT(storage->GetValue("k1", version_value, ver2));
            UNIT_ASSERT_VALUES_UNEQUAL(last_value, version_value);
        }
        UNIT_ASSERT(storage->ExistsNode("k1"));
        UNIT_ASSERT(!storage->ExistsNode("k2"));
        UNIT_ASSERT(storage->RemoveNode("k1", false));
        UNIT_ASSERT(!storage->ExistsNode("k1"));

        if (!storeHistory || storageType == "LOCAL") {
            UNIT_ASSERT(!storage->GetValue("k1", version_value, ver1));
        } else {
            UNIT_ASSERT(storage->GetValue("k1", version_value, ver1));
            UNIT_ASSERT_VALUES_EQUAL(last_value, version_value);
            UNIT_ASSERT(storage->GetValue("k1", version_value, ver2));
            UNIT_ASSERT_VALUES_UNEQUAL(last_value, version_value);
            UNIT_ASSERT(!storage->RemoveNode("k1", true));
            UNIT_ASSERT(storage->SetValue("k1", "xxx", true));
            UNIT_ASSERT(storage->RemoveNode("k1", true));
            UNIT_ASSERT(!storage->GetValue("k1", version_value, ver1));
            UNIT_ASSERT(!storage->GetValue("k1", version_value, ver2));
            UNIT_ASSERT(!storage->ExistsNode("k1"));
        }
    }

    Y_UNIT_TEST(TestLock) {
        /* Possible storage types:
            - LOCAL
            - ZOO
            - RTY
            - Postgres
            - Postgres-versioned
            - Postgres-ZOO - Postgres with a locker type ZOO
         */
        TString storageType = GetTestParam("storage-type", "LOCAL");

        InitGlobalLog2Console();
        TStorageOptions options = BuildStorageOptions(storageType);
        IVersionedStorage::TPtr storage = options.ConstructStorage();

        TThreadPool queue;
        queue.Start(16);
        TVector<ui32> counters;
        counters.resize(8, 0);
        for (ui32 i = 0; i < 1000; ++i) {
            for (auto&& c : counters) {
                queue.SafeAddAndOwn(MakeHolder<TIncDataActor>(&c, storage));
            }
        }
        queue.Stop();
        for (auto&& c : counters) {
            CHECK_WITH_LOG(c == 1000);
        }
    }

    Y_UNIT_TEST(TestKvStorage) {
        /* Possible storage types:
            - RTY
            - ZOO
            - Postgres
            - Postgres-versioned
            - Postgres-ZOO - Postgres with a locker type ZOO
        */
        TString storageType = GetTestParam("storage-type", "LOCAL");
        TString debug = GetTestParam("debug", "");
        ui64 cache = FromString<ui64>(GetTestParam("cache", "0"));
        bool consistency = FromString<bool>(GetTestParam("consistency", "0"));

        InitGlobalLog2Console(debug.empty() ? TLOG_INFO: TLOG_DEBUG);
        TStorageOptions options = BuildStorageOptions(storageType, cache, consistency);
        IVersionedStorage::TPtr storage = options.ConstructStorage();
        DoKvTest(options, storageType, false);
        if (storageType != "Postgres" && storageType != "RTY") {
            DoKvTest(options, storageType);
        }
    }
}

