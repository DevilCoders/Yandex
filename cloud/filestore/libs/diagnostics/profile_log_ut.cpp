#include "profile_log.h"

#include <cloud/filestore/libs/diagnostics/events/profile_events.ev.pb.h>
#include <cloud/storage/core/libs/common/scheduler_test.h>
#include <cloud/storage/core/libs/common/timer.h>

#include <library/cpp/eventlog/dumper/evlogdump.h>
#include <library/cpp/eventlog/eventlog.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/folder/tempdir.h>
#include <util/generic/algorithm.h>
#include <util/string/builder.h>

namespace NCloud::NFileStore {

namespace {

////////////////////////////////////////////////////////////////////////////////

using TRanges =
    google::protobuf::RepeatedPtrField<NProto::TProfileLogBlockRange>;

TString RangesToString(const TRanges& ranges)
{
    TStringBuilder sb;

    for (int i = 0; i < ranges.size(); ++i) {
        if (i) {
            sb << " ";
        }

        sb << ranges[i].GetNodeId()
            << "," << ranges[i].GetOffset()
            << "," << ranges[i].GetBytes();
    }

    return std::move(sb);
}

////////////////////////////////////////////////////////////////////////////////

struct TEventProcessor
    : TProtobufEventProcessor
{
    TVector<TString> FlatMessages;

    void DoProcessEvent(const TEvent* ev, IOutputStream* out) override
    {
        Y_UNUSED(out);

        auto message =
            dynamic_cast<const NProto::TProfileLogRecord*>(ev->GetProto());
        if (message) {
            for (const auto& r: message->GetRequests()) {
                FlatMessages.push_back(
                    TStringBuilder() << message->GetFileSystemId()
                        << "\t" << r.GetTimestampMcs()
                        << "\t" << r.GetRequestType()
                        << "\t" << r.GetDurationMcs()
                        << "\t" << RangesToString(r.GetRanges())
                );
            }
        }
    }
};

////////////////////////////////////////////////////////////////////////////////

struct TEnv
{
    ITimerPtr Timer;
    std::shared_ptr<TTestScheduler> Scheduler;
    TTempDir TempDir;
    TProfileLogSettings Settings;
    IProfileLogPtr ProfileLog;
    TEventProcessor EventProcessor;

    TEnv()
        : Timer(CreateWallClockTimer())
        , Scheduler(new TTestScheduler())
        , Settings{TempDir.Path() / "profile.log", TDuration::Seconds(1)}
        , ProfileLog(CreateProfileLog(Settings, Timer, Scheduler))
    {
        ProfileLog->Start();
    }

    ~TEnv()
    {
        ProfileLog->Stop();
    }

    void ProcessLog()
    {
        Scheduler->RunAllScheduledTasks();

        EventProcessor.FlatMessages.clear();
        const char* argv[] = {"foo", Settings.FilePath.c_str()};
        IterateEventLog(NEvClass::Factory(), &EventProcessor, 2, argv);
        Sort(EventProcessor.FlatMessages);
    }
};

////////////////////////////////////////////////////////////////////////////////

struct TRequestInfoBuilder
{
    NProto::TProfileLogRequestInfo R;

    auto& SetTimestamp(TInstant t)
    {
        R.SetTimestampMcs(t.MicroSeconds());
        return *this;
    }

    auto& SetDuration(TDuration d)
    {
        R.SetDurationMcs(d.MicroSeconds());
        return *this;
    }

    auto& SetRequestType(ui32 t)
    {
        R.SetRequestType(t);
        return *this;
    }

    auto& AddRange(ui64 nodeId, ui64 offset, ui32 size)
    {
        auto* range = R.AddRanges();
        range->SetNodeId(nodeId);
        range->SetOffset(offset);
        range->SetBytes(size);
        return *this;
    }

    auto Build() const
    {
        return R;
    }
};

}   // namespace

////////////////////////////////////////////////////////////////////////////////

Y_UNIT_TEST_SUITE(TProfileLogTest)
{
    Y_UNIT_TEST(TestDumpMessages)
    {
        TEnv env;

        env.ProfileLog->Write({
            "fs1",
            TRequestInfoBuilder()
                .SetTimestamp(TInstant::Seconds(1))
                .SetDuration(TDuration::MilliSeconds(100))
                .SetRequestType(1)
                .AddRange(111, 200, 10)
                .AddRange(111, 300, 5)
                .Build()
        });

        env.ProfileLog->Write({
            "fs1",
            TRequestInfoBuilder()
                .SetTimestamp(TInstant::Seconds(2))
                .SetDuration(TDuration::MilliSeconds(200))
                .SetRequestType(3)
                .AddRange(6000, 500, 20)
                .Build()
        });

        env.ProfileLog->Write({
            "fs2",
            TRequestInfoBuilder()
                .SetTimestamp(TInstant::Seconds(3))
                .SetDuration(TDuration::MilliSeconds(400))
                .SetRequestType(7)
                .Build()
        });

        env.ProcessLog();

        UNIT_ASSERT_VALUES_EQUAL(3, env.EventProcessor.FlatMessages.size());
        UNIT_ASSERT_VALUES_EQUAL(
            "fs1\t1000000\t1\t100000\t111,200,10 111,300,5",
            env.EventProcessor.FlatMessages[0]
        );
        UNIT_ASSERT_VALUES_EQUAL(
            "fs1\t2000000\t3\t200000\t6000,500,20",
            env.EventProcessor.FlatMessages[1]
        );
        UNIT_ASSERT_VALUES_EQUAL(
            "fs2\t3000000\t7\t400000\t",
            env.EventProcessor.FlatMessages[2]
        );

        env.ProfileLog->Write({
            "fs3",
            TRequestInfoBuilder()
                .SetTimestamp(TInstant::Seconds(4))
                .SetDuration(TDuration::MilliSeconds(300))
                .SetRequestType(4)
                .Build()
        });

        env.ProcessLog();

        UNIT_ASSERT_VALUES_EQUAL(4, env.EventProcessor.FlatMessages.size());
        UNIT_ASSERT_VALUES_EQUAL(
            "fs3\t4000000\t4\t300000\t",
            env.EventProcessor.FlatMessages[3]
        );
    }

    Y_UNIT_TEST(TestSmoke)
    {
        TEnv env;

        for (ui32 i = 1; i <= 100000; ++i) {
            env.ProfileLog->Write({
                TStringBuilder() << "fs" << (i % 10),
                TRequestInfoBuilder()
                    .SetTimestamp(TInstant::MilliSeconds(i))
                    .SetDuration(TDuration::MilliSeconds(i % 200))
                    .SetRequestType(i % 3)
                    .AddRange(i % 1000, i, i * 2)
                    .Build()
            });
        }

        env.ProcessLog();

        UNIT_ASSERT_VALUES_EQUAL(100000, env.EventProcessor.FlatMessages.size());
    }
}

}   // namespace NCloud::NFileStore
