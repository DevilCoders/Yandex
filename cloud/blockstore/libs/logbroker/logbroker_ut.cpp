#include "logbroker.h"

#include "config.h"

#include <cloud/storage/core/libs/common/error.h>
#include <cloud/storage/core/libs/diagnostics/logging.h>

#include <kikimr/persqueue/sdk/deprecated/cpp/v2/credentials_provider.h>
#include <kikimr/persqueue/sdk/deprecated/cpp/v2/persqueue.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/string/printf.h>
#include <util/system/env.h>
#include <util/system/hostname.h>

namespace NCloud::NBlockStore::NLogbroker {

using namespace NThreading;

namespace {

////////////////////////////////////////////////////////////////////////////////

static constexpr TDuration WaitTimeout = TDuration::Seconds(30);

auto Read(size_t count)
{
    TLogSettings logSettings;
    logSettings.FiltrationLevel = TLOG_DEBUG;
    logSettings.UseLocalTimestamps = true;

    auto logging = CreateLoggingService("console", logSettings);

    NPersQueue::TPQLibSettings settings;
    settings.CompressionPoolThreads = 1;
    settings.DefaultLogger = MakeIntrusive<NPersQueue::TCerrLogger>(7);

    NPersQueue::TPQLib pq(settings);

    NPersQueue::TConsumerSettings consumerSettings;

    consumerSettings.Server.Address = "localhost";
    consumerSettings.Server.Port = FromString<ui32>(GetEnv("LOGBROKER_PORT"));
    consumerSettings.Topics = { "default-topic" };
    consumerSettings.ClientId = "test";

    auto consumer = pq.CreateConsumer(consumerSettings);
    auto start = consumer->Start().GetValue(WaitTimeout);
    UNIT_ASSERT(!start.Response.HasError());

    TVector<TString> result;

    while (result.size() < count) {
        auto msg = consumer->GetNextMessage();

        if (!msg.Wait(WaitTimeout)) {
            break;
        }

        auto value = msg.GetValue();

        if (value.Type != NPersQueue::EMT_DATA) {
            break;
        }

        for (const auto& t : value.Response.data().message_batch()) {
            for (const auto& m : t.message()) {
                result.push_back(m.data());
            }
        }
    }

    return result;
}

auto CreateData()
{
    TVector<TMessage> messages{
        { "hello", 100 },
        { "world", 200 }
    };

    return messages;
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

Y_UNIT_TEST_SUITE(TLogbrokerTest)
{
    Y_UNIT_TEST(ShouldStubWriteData)
    {
        auto service = CreateServiceStub();
        service->Start();

        auto r = service->Write(CreateData(), Now())
            .GetValue(WaitTimeout);

        UNIT_ASSERT_C(!HasError(r), "Unexpected error: " << FormatError(r));

        service->Stop();
    }

    Y_UNIT_TEST(ShouldWriteData)
    {
        auto config = std::make_shared<TLogbrokerConfig>([] {
            NProto::TLogbrokerConfig proto;

            proto.SetAddress("localhost");
            proto.SetPort(FromString<ui32>(GetEnv("LOGBROKER_PORT")));
            proto.SetDatabase("/Root");
            proto.SetTopic("default-topic");
            proto.SetSourceId(Sprintf(
                "test:%s:%lu",
                GetFQDNHostName(),
                TInstant::Now().MilliSeconds()));

            return proto;
        }());

        TLogSettings logSettings;
        logSettings.FiltrationLevel = TLOG_DEBUG;
        logSettings.UseLocalTimestamps = true;

        auto logging = CreateLoggingService("console", logSettings);

        auto service = CreateService(config, logging);
        service->Start();

        auto error = [&] {
            for (;;) {
                auto r = service->Write(CreateData(), Now())
                    .GetValue(WaitTimeout);

                if (GetErrorKind(r) != EErrorKind::ErrorRetriable) {
                    return r;
                }
            }
        }();

        UNIT_ASSERT_C(!HasError(error), "Unexpected error: " << FormatError(error));

        auto data = Read(2);

        UNIT_ASSERT_VALUES_EQUAL(2, data.size());

        UNIT_ASSERT_VALUES_EQUAL("hello", data[0]);
        UNIT_ASSERT_VALUES_EQUAL("world", data[1]);

        service->Stop();
    }

    void ShouldHandleErrorImpl(TLogbrokerConfigPtr config)
    {
        TLogSettings logSettings;
        logSettings.UseLocalTimestamps = true;

        auto logging = CreateLoggingService("console", logSettings);

        auto service = CreateService(config, logging);
        service->Start();

        auto r = service->Write(CreateData(), Now())
            .GetValue(WaitTimeout);

        UNIT_ASSERT(HasError(r));

        service->Stop();
    }

    Y_UNIT_TEST(ShouldHandleConnectError)
    {
        NProto::TLogbrokerConfig proto;

        proto.SetDatabase("/Root");
        proto.SetTopic("topic");
        proto.SetSourceId("test");
        proto.SetAddress("unknown");

        ShouldHandleErrorImpl(std::make_shared<TLogbrokerConfig>(proto));
    }

    Y_UNIT_TEST(ShouldHandleUnknownTopic)
    {
        NProto::TLogbrokerConfig proto;

        proto.SetAddress("localhost");
        proto.SetPort(FromString<ui32>(GetEnv("LOGBROKER_PORT")));
        proto.SetDatabase("/Root");
        proto.SetTopic("unknown-topic");
        proto.SetSourceId(Sprintf(
            "test:%s:%lu",
            GetFQDNHostName(),
            TInstant::Now().MilliSeconds()));

        ShouldHandleErrorImpl(std::make_shared<TLogbrokerConfig>(proto));
    }
}

}   // namespace NCloud::NBlockStore::NLogbroker
