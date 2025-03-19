#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/yconf/patcher/unstrict_config.h>
#include <kernel/common_server/library/persistent_queue/abstract/pq.h>
#include <kernel/common_server/library/persistent_queue/abstract/config.h>
#include <util/random/random.h>
#include <util/stream/file.h>

constexpr const char* CONFIG = R"(
    ClientId: lb
    Type: logbroker
    Endpoint: localhost:%s
    Database: /Root
    InteractionTimeout: 10s
)";

constexpr const char* CONFIG_READ = R"(
    <Read>
        Topics: %s
        ConsumerName: test
    </Read>
)";

constexpr const char* CONFIG_WRITE = R"(
    <Write>
        Path: %s
        MessagesGroupId: source_id
        MaxInFlight: 10
    </Write>
)";

using namespace NCS;

Y_UNIT_TEST_SUITE(TLogbrokerTestSuite) {

    static NCS::TPQClientConfigContainer BuildConfig(bool read, bool write, const TString& topic) {
        TFileInput port("ydb_endpoint.txt");
        TStringBuilder configText;
        configText << "<PQ>" << Endl;
        configText << Sprintf(CONFIG, port.ReadAll().c_str()) << Endl;
        if (read) {
            configText << Sprintf(CONFIG_READ, topic.c_str ()) << Endl;
        }
        if (write) {
            configText << Sprintf(CONFIG_WRITE, topic.c_str()) << Endl;
        }
        configText << "</PQ>" << Endl;
        Cerr << configText << Endl;
        TUnstrictConfig cfg;
        if (!cfg.ParseMemory(configText)) {
            TString errors;
            cfg.PrintErrors(errors);
            UNIT_ASSERT_C(false, errors);
        }
        NCS::TPQClientConfigContainer pqClientContainer;
        pqClientContainer.Init(cfg.GetFirstChild("PQ"));
        CHECK_WITH_LOG(!!pqClientContainer);
        return pqClientContainer;
    }

    Y_UNIT_TEST(StartStop) {
        const auto pqClientContainer = BuildConfig(true, true, "StartStop");
        TFakePQConstructionContext context;
        auto PQ = pqClientContainer->Construct(context);
        CHECK_WITH_LOG(!!PQ);
        UNIT_ASSERT(!PQ->ReadMessage(nullptr, TDuration::Seconds(2), true));
        {
            auto wMessage = MakeHolder<NCS::TPQMessageSimple>(TBlob::FromString("payload"), "");
            UNIT_ASSERT(!PQ->WriteMessage(wMessage.Release()));
        }
        CHECK_WITH_LOG(PQ->Start());
        CHECK_WITH_LOG(PQ->Stop());
    }
    Y_UNIT_TEST(Read) {
        const auto pqClientContainer = BuildConfig(true, false, "Read");
        TFakePQConstructionContext context;
        auto PQ = pqClientContainer->Construct(context);
        CHECK_WITH_LOG(!!PQ);
        CHECK_WITH_LOG(PQ->Start());
        {
            ui32 fails = 0;
            auto rMessage = PQ->ReadMessage(&fails, TDuration::Seconds(2), true);
            TFLEventLog::Log("read finished")("null", !rMessage)("fails", fails);
            UNIT_ASSERT_EQUAL(fails, 0);
            UNIT_ASSERT(!rMessage);
        }
        CHECK_WITH_LOG(PQ->Stop());
    }
    Y_UNIT_TEST(Write) {
        const auto pqClientContainer = BuildConfig(false, true, "Write");
        TFakePQConstructionContext context;
        auto PQ = pqClientContainer->Construct(context);
        CHECK_WITH_LOG(!!PQ);
        CHECK_WITH_LOG(PQ->Start());
        {
            auto wMessage = MakeHolder<NCS::TPQMessageSimple>(TBlob::FromString("payload"), "");
            UNIT_ASSERT(!!PQ->WriteMessage(wMessage.Release()));
            UNIT_ASSERT(PQ->FlushWritten());
        }
        CHECK_WITH_LOG(PQ->Stop());
    }
    Y_UNIT_TEST(WriteRead) {
        DoInitGlobalLog("cerr", LOG_MAX_PRIORITY, false, false);
        TFakePQConstructionContext context;
        const auto pqClientContainer = BuildConfig(true, true, "WriteRead");
        auto PQ = pqClientContainer->Construct(context);
        CHECK_WITH_LOG(!!PQ);
        CHECK_WITH_LOG(PQ->Start());
        {
            auto wMessage = MakeHolder<NCS::TPQMessageSimple>(TBlob::FromString("payload"), "");
            UNIT_ASSERT(!!PQ->WriteMessage(wMessage.Release()));
            UNIT_ASSERT(PQ->FlushWritten());
        }
        {
            ui32 fails = 0;
            auto rMessage = PQ->ReadMessage(&fails, TDuration::Seconds(2), true);
            TFLEventLog::Log("read finished")("null", !rMessage)("fails", fails);
            UNIT_ASSERT_EQUAL(fails, 0);
            UNIT_ASSERT(rMessage);
            UNIT_ASSERT_STRINGS_EQUAL(rMessage->GetContent().AsStringBuf(), "payload");
            UNIT_ASSERT(PQ->AckMessage(rMessage));
        }
        CHECK_WITH_LOG(PQ->Stop());
    }
    Y_UNIT_TEST(Ack) {
        const auto pqClientContainer = BuildConfig(true, true, "Ack");
        TFakePQConstructionContext context;
        {
            auto PQ = pqClientContainer->Construct(context);
            CHECK_WITH_LOG(PQ->Start());
            auto wMessage = MakeHolder<NCS::TPQMessageSimple>(TBlob::FromString("payload"), "");
            UNIT_ASSERT(!!PQ->WriteMessage(wMessage.Release()));
            ui32 fails = 0;
            auto rMessage = PQ->ReadMessage(&fails, TDuration::Seconds(2), true);
            TFLEventLog::Log("read finished")("null", !rMessage)("fails", fails);
            UNIT_ASSERT_EQUAL(fails, 0);
            UNIT_ASSERT(rMessage);
            UNIT_ASSERT_STRINGS_EQUAL(TString(rMessage->GetContent().AsCharPtr(), rMessage->GetContent().Length()), "payload");
            CHECK_WITH_LOG(PQ->Stop());
        }
        {
            auto PQ = pqClientContainer->Construct(context);
            CHECK_WITH_LOG(PQ->Start());
            ui32 fails = 0;
            auto rMessage = PQ->ReadMessage(&fails, TDuration::Seconds(2), true);
            TFLEventLog::Log("read finished")("null", !rMessage)("fails", fails);
            UNIT_ASSERT_EQUAL(fails, 0);
            UNIT_ASSERT(rMessage);
            UNIT_ASSERT_STRINGS_EQUAL(TString(rMessage->GetContent().AsCharPtr(), rMessage->GetContent().Length()), "payload");
            UNIT_ASSERT(PQ->AckMessage(rMessage));
            CHECK_WITH_LOG(PQ->Stop());
        }
        {
            auto PQ = pqClientContainer->Construct(context);
            CHECK_WITH_LOG(PQ->Start());
            ui32 fails = 0;
            auto rMessage = PQ->ReadMessage(&fails, TDuration::Seconds(2), true);
            TFLEventLog::Log("read finished")("null", !rMessage)("fails", fails);
            UNIT_ASSERT_EQUAL(fails, 0);
            UNIT_ASSERT(!rMessage);
            CHECK_WITH_LOG(PQ->Stop());
        }
    }

    Y_UNIT_TEST(Mulithread) {
        constexpr ui32 threadsCount = 10;
        constexpr ui32 messagesPerThread = 1000;
        constexpr ui32 messagesCount = messagesPerThread * threadsCount;

        const auto pqClientContainer = BuildConfig(true, true, "Multithread");
        TFakePQConstructionContext context;
        {
            auto PQ = pqClientContainer->Construct(context);
            CHECK_WITH_LOG(PQ->Start());
            TThreadPool pool;
            pool.Start(threadsCount);
            for (ui32 i = 0; i < threadsCount; ++i) {
                pool.SafeAddFunc([i, PQ]() {
                    for (ui32 m = 0; m < messagesPerThread; ++m) {
                        const TString data = ToString(i * messagesPerThread + m);
                        UNIT_ASSERT(!!PQ->WriteMessage(MakeAtomicShared<NCS::TPQMessageSimple>(
                            TBlob::FromString(data), ""))
                        );
                    }
                    UNIT_ASSERT(PQ->FlushWritten());
                });
            }
            pool.Stop();
            CHECK_WITH_LOG(PQ->Stop());
        }
        {
            auto PQ = pqClientContainer->Construct(context);
            CHECK_WITH_LOG(PQ->Start());
            TVector<NCS::IPQMessage::TPtr> result;
            ui32 fails = 0;
            UNIT_ASSERT(PQ->ReadMessages(result, &fails, messagesCount, TDuration::Seconds(10), true));
            UNIT_ASSERT_EQUAL(fails, 0);
            UNIT_ASSERT_EQUAL(result.size(), messagesCount);
            TSet<ui32> readed;
            for (const auto& msg: result) {
                readed.emplace(FromString<ui32>(msg->GetContent().AsStringBuf()));
            }
            UNIT_ASSERT_EQUAL(readed.size(), messagesCount);
            CHECK_WITH_LOG(PQ->Stop());
        }
    }

    Y_UNIT_TEST(Retry) {
        TFakePQConstructionContext context;
        const auto pqClientContainer = BuildConfig(true, true, "Retry");
        auto PQ = pqClientContainer->Construct(context);
        CHECK_WITH_LOG(!!PQ);
        for (ui32 i = 0; i < 3; ++i) {
            CHECK_WITH_LOG(PQ->Start());
            UNIT_ASSERT(!!PQ->WriteMessage(MakeAtomicShared<NCS::TPQMessageSimple>(TBlob::FromString("payload 1"), "1")));
            UNIT_ASSERT(!!PQ->WriteMessage(MakeAtomicShared<NCS::TPQMessageSimple>(TBlob::FromString("payload 2"), "2")));
            //resend will be ignored really
            UNIT_ASSERT(!!PQ->WriteMessage(MakeAtomicShared<NCS::TPQMessageSimple>(TBlob::FromString("payload 3"), "1")));
            UNIT_ASSERT(!!PQ->WriteMessage(MakeAtomicShared<NCS::TPQMessageSimple>(TBlob::FromString("payload 4"), "2")));
            UNIT_ASSERT(PQ->FlushWritten());
            CHECK_WITH_LOG(PQ->Stop());
        }
        {
            CHECK_WITH_LOG(PQ->Start());
            TVector<NCS::IPQMessage::TPtr> result;
            ui32 fails = 0;
            UNIT_ASSERT(PQ->ReadMessages(result, &fails, 6, TDuration::Seconds(2), true));
            UNIT_ASSERT_EQUAL(fails, 0);
            UNIT_ASSERT(result.size() == 2);
            UNIT_ASSERT_STRINGS_EQUAL(result[0]->GetContent().AsStringBuf(), "payload 1");
            UNIT_ASSERT_STRINGS_EQUAL(result[1]->GetContent().AsStringBuf(), "payload 2");
            CHECK_WITH_LOG(PQ->Stop());
        }
    }

}
