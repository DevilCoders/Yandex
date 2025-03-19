#include "filesystem_counters.h"
#include "storage_counters.h"

#include <cloud/filestore/libs/storage/tablet/protos/tablet.pb.h>

#include <cloud/storage/core/protos/media.pb.h>

#include <library/cpp/monlib/dynamic_counters/counters.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/size_literals.h>

namespace NCloud::NFileStore::NStorage {

using namespace NCloud::NProto;
using namespace NMonitoring;

////////////////////////////////////////////////////////////////////////////////

ui64 GetFSCounter(
    const TDynamicCounterPtr& counters,
    const TString& fsId,
    const TString& counterName)
{
    return counters->GetSubgroup("counters", "filestore")
        ->GetSubgroup("component", "storage_fs")
        ->GetSubgroup("host", "cluster")
        ->GetSubgroup("filesystem", fsId)->GetCounter(counterName)->Val();
}

ui64 GetServiceCounter(
    const TDynamicCounterPtr& counters,
    const TString& fsMediaKind,
    const TString& counterName)
{
    return counters->GetSubgroup("counters", "filestore")
        ->GetSubgroup("component", "storage")
        ->GetSubgroup("type", fsMediaKind)->GetCounter(counterName)->Val();
}

////////////////////////////////////////////////////////////////////////////////

struct TBootstrap {
    TDynamicCounterPtr Counters;
    IStorageCountersPtr StorageCounters;
    TFileSystemStatCountersPtr StatCounters;

    TBootstrap(
            TString id = "test",
            EStorageMediaKind kind = STORAGE_MEDIA_HDD,
            TDynamicCounterPtr counters = MakeIntrusive<TDynamicCounters>())
        : Counters(std::move(counters))
        , StorageCounters(CreateStorageCounters(Counters))
    {
        NProto::TFileSystem fs;
        fs.SetBlockSize(4_KB);
        fs.SetFileSystemId(id);
        fs.SetStorageMediaKind(kind);

        StatCounters = std::make_shared<TFileSystemStatCounters>(
            fs,
            StorageCounters);
    }
};

////////////////////////////////////////////////////////////////////////////////

Y_UNIT_TEST_SUITE(TFilesystemCountersTest)
{
    Y_UNIT_TEST(ShouldReportSessionCountForFileSystem)
    {
        TBootstrap bootstrap;

        NProto::TFileSystemStats stats;
        stats.SetUsedSessionsCount(3);

        bootstrap.StatCounters->Update(stats);
        UNIT_ASSERT_VALUES_EQUAL(GetFSCounter(bootstrap.Counters, "test", "SessionCount"), 3);

        stats.SetUsedSessionsCount(2);
        bootstrap.StatCounters->Update(stats);
        UNIT_ASSERT_VALUES_EQUAL(GetFSCounter(bootstrap.Counters, "test", "SessionCount"), 2);
    }

    Y_UNIT_TEST(ShouldReportServiceCountersForDifferentMediaKinds)
    {
        TDynamicCounterPtr counters = MakeIntrusive<TDynamicCounters>();
        TBootstrap bootstrap1("test1", STORAGE_MEDIA_HDD, counters);
        TBootstrap bootstrap2("test2", STORAGE_MEDIA_SSD, counters);
        TBootstrap bootstrap3("test3", STORAGE_MEDIA_SSD, counters);

        NProto::TFileSystemStats stats;
        stats.SetUsedSessionsCount(1);
        bootstrap1.StatCounters->Update(stats);
        bootstrap2.StatCounters->Update(stats);
        bootstrap3.StatCounters->Update(stats);

        UNIT_ASSERT_VALUES_EQUAL(GetServiceCounter(counters, "ssd", "SessionCount"), 2);
        UNIT_ASSERT_VALUES_EQUAL(GetServiceCounter(counters, "hdd", "SessionCount"), 1);

        bootstrap1.StatCounters.reset();
        UNIT_ASSERT_VALUES_EQUAL(GetServiceCounter(counters, "ssd", "SessionCount"), 2);
        UNIT_ASSERT_VALUES_EQUAL(GetServiceCounter(counters, "hdd", "SessionCount"), 0);

        bootstrap2.StatCounters.reset();
        UNIT_ASSERT_VALUES_EQUAL(GetServiceCounter(counters, "ssd", "SessionCount"), 1);
        UNIT_ASSERT_VALUES_EQUAL(GetServiceCounter(counters, "hdd", "SessionCount"), 0);
    }
}

}   // namespace NCloud::NFileStore::NStorage
