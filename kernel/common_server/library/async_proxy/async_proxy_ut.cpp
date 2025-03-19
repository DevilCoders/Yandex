#include "message.h"
#include "async_delivery.h"
#include "addr.h"
#include "report.h"
#include "shard_source.h"
#include "shard.h"

#include <kernel/common_server/library/neh/server/neh_server.h>
#include <kernel/common_server/library/async_proxy/ut/helper/helper.h>
#include <library/cpp/http/io/stream.h>
#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/tests_data.h>
#include <library/cpp/http/server/http.h>
#include <search/meta/scatter/options/options.h>
#include <search/meta/scatter/source.h>
#include <util/digest/fnv.h>
#include <util/stream/input.h>
#include <util/stream/str.h>

#include <kernel/httpsearchclient/httpsearchclient.h>

#include <array>
#include "shards_report.h"
#include "broadcast_collector.h"

Y_UNIT_TEST_SUITE(AsyncProxySuite) {
    using namespace NAPHelper;

    Y_UNIT_TEST(TestAsyncDeliveringIncomplete) {
        InitGlobalLog2Console(8);

        NSimpleMeta::TConfig metaConfig = NSimpleMeta::TConfig::ParseFromString(
            "MaxAttempts: 3\n"
            "TimeoutSendingms: 3000\n"
        );

        TPortManager portManager;
        std::array<ui32, 3> ports = {{ portManager.GetPort(1234), portManager.GetPort(1235), portManager.GetPort(1236) }};

        auto server1 = BuildServer(ports[0], 8, TDuration::Seconds(5), 200);
        auto server2 = BuildServer(ports[1], 8, TDuration::Seconds(2), 200);
        auto server3 = BuildServer(ports[2], 8, TDuration::MilliSeconds(500), 500);

        try {
            TAsyncDelivery ad;
            ad.Start(1, 8);
            NScatter::TSourceOptions options;
            THolder<NScatter::ISource> source1 = CreateSimpleSource("http://localhost:" + ToString(ports[0]) + " http://localhost:" + ToString(ports[1]), options);
            THolder<NScatter::ISource> source2 = CreateSimpleSource("http://localhost:" + ToString(ports[2]), options);
            THolder<NScatter::ISource> source3 = CreateSimpleSource("http://localhost:1237", options);
            AtomicSet(Counter5xx, 0);
            for (ui32 i = 0; i < 4; ++i) {
                TAsyncTask* message = new TAsyncTask(new TReportBuilderPrint({ 200, 502 }), Now() + TDuration::Seconds(3));
                message->AddShard(new TTrivialShardISource(message, source1.Get(), metaConfig, "", &ad, 0));
                message->AddShard(new TTrivialShardISource(message, source2.Get(), metaConfig, "", &ad, 1));
                message->AddShard(new TTrivialShardISource(message, source3.Get(), metaConfig, "", &ad, 2));
                ad.Send(message);
            }
            CheckRequestsFinishes(TDuration::Seconds(20));
            CHECK_WITH_LOG(AtomicGet(Counter5xx) == 4);
            ad.Stop();
        } catch (...) {
            INFO_LOG << CurrentExceptionMessage() << Endl;
            UNIT_ASSERT(false);
        }
    }

    Y_UNIT_TEST(TestAsyncDeliveringReplyAfterSwitchOneSource) {
        InitGlobalLog2Console(8);

        NSimpleMeta::TConfig metaConfig = NSimpleMeta::TConfig::ParseFromString(
            "MaxAttempts: 3\n"
            "TimeoutSendingms: 30000\n"
        );

        TPortManager portManager;
        ui32 port = portManager.GetPort(3234);
        auto server1 = BuildServer(port, 8, TDuration::Seconds(5), 200);

        try {
            TAsyncDelivery ad;
            ad.Start(1, 8);
            NScatter::TSourceOptions options;
            THolder<NScatter::ISource> source = CreateSimpleSource("http://localhost:" + ToString(port), options);
            AtomicSet(Counter5xx, 0);
            for (ui32 i = 0; i < 4; ++i) {
                TAsyncTask* message = new TAsyncTask(new TReportBuilderPrint({ 200 }), Now() + TDuration::Seconds(30));
                message->AddShard(new TTrivialShardISource(message, source.Get(), metaConfig, "", &ad, 0));
                ad.Send(message);
            }
            CheckRequestsFinishes(TDuration::Seconds(20));
            CHECK_WITH_LOG(AtomicGet(Counter5xx) == 0);
            ad.Stop();
        } catch (...) {
            INFO_LOG << CurrentExceptionMessage() << Endl;
            UNIT_ASSERT(false);
        }
    }


    Y_UNIT_TEST(TestAsyncDeliveringInproc) {
        InitGlobalLog2Console(8);

        NSimpleMeta::TConfig metaConfig = NSimpleMeta::TConfig::ParseFromString(
            "MaxAttempts: 3\n"
            "TimeoutSendingms: 3000\n"
        );

        TPortManager portManager;
        std::array<ui32, 3> ports = { { portManager.GetPort(2234), portManager.GetPort(2235), portManager.GetPort(2236) } };

        auto server1 = BuildServer(ports[0], 24, TDuration::Seconds(5), 200, "inproc", true);
        auto server2 = BuildServer(ports[1], 24, TDuration::Seconds(2), 200, "inproc", true);
        auto server3 = BuildServer(ports[2], 24, TDuration::MilliSeconds(500), 500, "inproc", true);

        try {
            NScatter::TSourceOptions options;
            THolder<NScatter::ISource> source = CreateSimpleSource("inproc://localhost:" + ToString(ports[0]) + " inproc://localhost:" + ToString(ports[1]) + " inproc://localhost:" + ToString(ports[2]), options);
            AtomicSet(Counter5xx, 0);
            TAsyncDelivery ad;
            ad.Start(1, 8);
            for (ui32 i = 0; i < 4; ++i) {
                TAsyncTask* message = new TAsyncTask(new TReportBuilderPrint({ 200 }), Now() + TDuration::Seconds(3));
                message->AddShard(new TTrivialShardISource(message, source.Get(), metaConfig, "text=test1", &ad, 0));
                message->AddShard(new TTrivialShardISource(message, source.Get(), metaConfig, "text=test2", &ad, 1));
                ad.Send(message);
            }
            CheckRequestsFinishes(TDuration::Seconds(20));
            UNIT_ASSERT_VALUES_EQUAL(AtomicGet(Counter5xx), 8);
            ad.Stop();
        } catch (...) {
            INFO_LOG << CurrentExceptionMessage() << Endl;
            UNIT_ASSERT(false);
        }
    }


    Y_UNIT_TEST(TestAsyncDelivering) {
        InitGlobalLog2Console(8);

        NSimpleMeta::TConfig metaConfig = NSimpleMeta::TConfig::ParseFromString(
            "MaxAttempts: 3\n"
            "TimeoutConnectms: 3000\n"
            "TimeoutSendingms: 3000\n"
        );

        TPortManager portManager;
        std::array<ui32, 3> ports = {{ portManager.GetPort(2234), portManager.GetPort(2235), portManager.GetPort(2236) }};

        auto server1 = BuildServer(ports[0], 24, TDuration::Seconds(5), 200);
        auto server2 = BuildServer(ports[1], 24, TDuration::Seconds(2), 200);
        auto server3 = BuildServer(ports[2], 24, TDuration::MilliSeconds(500), 500);

        try {
            NScatter::TSourceOptions options;
            THolder<NScatter::ISource> source = CreateSimpleSource("http://localhost:" + ToString(ports[0]) + " http://localhost:" + ToString(ports[1]) + " http://localhost:" + ToString(ports[2]), options);
            AtomicSet(Counter5xx, 0);
            {
                TAsyncDelivery ad;
                ad.Start(1, 8);
                for (ui32 i = 0; i < 4; ++i) {
                    TAsyncTask* message = new TAsyncTask(new TReportBuilderPrint({ 200 }), Now() + TDuration::Seconds(3));
                    message->AddShard(new TTrivialShardISource(message, source.Get(), metaConfig, "", &ad, 0));
                    message->AddShard(new TTrivialShardISource(message, source.Get(), metaConfig, "", &ad, 1));
                    ad.Send(message);
                }
                CheckRequestsFinishes(TDuration::Seconds(20));
                ad.Stop();
            }
            UNIT_ASSERT_VALUES_EQUAL(AtomicGet(Counter5xx), 8);
        } catch (...) {
            INFO_LOG << CurrentExceptionMessage() << Endl;
            UNIT_ASSERT(false);
        }
    }

    Y_UNIT_TEST(TestBroadcastAsyncDelivering) {
        InitGlobalLog2Console(8);

        NSimpleMeta::TConfig metaConfig = NSimpleMeta::TConfig::ParseFromString(
            "MaxAttempts: 1\n"
            "TimeoutConnectms: 3000\n"
            "TimeoutSendingms: 3000\n"
        );

        TPortManager portManager;
        std::array<ui32, 3> ports = { { portManager.GetPort(2234), portManager.GetPort(2235), portManager.GetPort(2236) } };

        auto server1 = BuildServer(ports[0], 24, TDuration::Seconds(2), 200);
        auto server2 = BuildServer(ports[1], 24, TDuration::MilliSeconds(500), 200);
        auto server3 = BuildServer(ports[2], 24, TDuration::MilliSeconds(500), 200);
        AtomicSet(Counter5xx, 0);
        try {
            TAsyncDelivery ad;

            TVector<TString> vsrcs;
            vsrcs.push_back("http://localhost:" + ToString(ports[0]) + "/req_limit");
            vsrcs.push_back("http://localhost:" + ToString(ports[1]) + "/req_limit");
            vsrcs.push_back("http://localhost:" + ToString(ports[2]) + "/req_limit");
            TBroadcastCollector collector(vsrcs, metaConfig, ad);
            ad.Start(1, 8);
            collector.Collect(new TReportBuilderPrint({ 200 }), Now() + TDuration::Seconds(100));
            CheckRequestsFinishes(TDuration::Seconds(20));
            ad.Stop();
            CheckRequestsFinishes(TDuration::Seconds(20));
            UNIT_ASSERT_VALUES_EQUAL(AtomicGet(Counter5xx), 0);
        } catch (...) {
            INFO_LOG << CurrentExceptionMessage() << Endl;
            UNIT_ASSERT(false);
        }
    }

    Y_UNIT_TEST(TestAsyncDeliveringNoSources) {
        InitGlobalLog2Console(8);

        try {
            NSimpleMeta::TConfig metaConfig = NSimpleMeta::TConfig::ParseFromString(
                "MaxAttempts: 3\n"
                "TimeoutConnectms: 2000\n"
                "TimeoutSendingms: 2000\n"
            );

            NScatter::TSourceOptions options;
            THolder<NScatter::ISource> source = CreateSimpleSource("http://localhost:1234 http://localhost:1235 http://localhost:1236", options);
            THolder<NScatter::ISource> source1 = CreateSimpleSource("tcp2://localhost:1234 tcp2://localhost:1235 tcp2://localhost:1236", options);
            AtomicSet(Counter5xx, 0);
            AtomicSet(CounterDisconnect, 0);
            {
                TAsyncDelivery ad;
                ad.Start(1, 8);
                for (ui32 i = 0; i < 1000; ++i) {
                    TAsyncTask* message = new TAsyncTask(new TReportBuilderPrint({ 502 }), Now() + TDuration::Seconds(3));
                    message->AddShard(new TTrivialShardISource(message, source.Get(), metaConfig, "", &ad, 0));
                    message->AddShard(new TTrivialShardISource(message, source.Get(), metaConfig, "", &ad, 1));
                    message->AddShard(new TTrivialShardISource(message, source1.Get(), metaConfig, "", &ad, 2));
                    ad.Send(message);
                }
                CheckRequestsFinishes(TDuration::Seconds(20));
                ad.Stop();
            }
            UNIT_ASSERT_VALUES_EQUAL(AtomicGet(Counter5xx), 0);
            UNIT_ASSERT_VALUES_EQUAL(AtomicGet(CounterDisconnect), 9000);
        } catch (...) {
            INFO_LOG << CurrentExceptionMessage() << Endl;
            throw;
        }
    }

    Y_UNIT_TEST(TestAsyncDeliveringIterator) {
        InitGlobalLog2Console(8);
        NScatter::TSourceOptions options;
        options.MaxAttempts = 3;
        options.AllowDynamicWeights = true;

        NNeh::TMessage mess;
        mess.Addr = "12345";
        {
            THolder<NScatter::ISource> source = CreateSimpleSource("http://localhost:2234 http://localhost:2235  http://localhost:2236", options);
            NHttpSearchClient::TSeed seed(FnvHash<ui64>(mess.Addr.data(), mess.Addr.size()));
            auto iterator = source->GetSearchConnections(seed);
            ui32 counter = 0;
            while (iterator->Next(counter)) {
                ++counter;
            }
            UNIT_ASSERT_VALUES_EQUAL(counter, 3);
        }
        {
            THolder<NScatter::ISource> source = CreateSimpleSource("http://localhost:2234 http://localhost:2235", options);
            NHttpSearchClient::TSeed seed(FnvHash<ui64>(mess.Addr.data(), mess.Addr.size()));
            auto iterator = source->GetSearchConnections(seed);
            ui32 counter = 0;
            while (iterator->Next(counter)) {
                ++counter;
            }
            UNIT_ASSERT_VALUES_EQUAL(counter, 2);
        }

    }
}
