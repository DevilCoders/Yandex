#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/yconf/patcher/unstrict_config.h>
#include <kernel/common_server/library/persistent_queue/abstract/pq.h>
#include <kernel/common_server/library/persistent_queue/abstract/config.h>
#include <util/system/env.h>
#include <util/random/random.h>

constexpr const char* CONFIG = R"(
    ClientId: kafka
    Type: kafka
    Bootstrap: %s
    Topic: %s
    GroupId: test-group
    Partition: 0
)";

constexpr const char* CONFIG_READ = R"(
    <Read>
        GroupId: test-group
    </Read>
)";

constexpr const char* CONFIG_WRITE = R"(
    <Write>
    </Write>
)";

using namespace NCS;

Y_UNIT_TEST_SUITE(TKafkaTestSuite) {

    static NCS::TPQClientConfigContainer BuildConfig(const TString& topic, bool read = true, bool write = true) {
        TStringBuilder configText;
        configText << "<PQ>" << Endl;
        configText << Sprintf(CONFIG,
            GetEnv("KAFKA_RECIPE_BROKER_LIST").c_str(),
            topic.c_str()
        ) << Endl;
        if (read) {
            configText << CONFIG_READ << Endl;
        }
        if (write) {
            configText << CONFIG_WRITE << Endl;
        }
        configText << "</PQ>" << Endl;
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
        const NCS::TPQClientConfigContainer pqClientContainer = BuildConfig("StartStop");
        TFakePQConstructionContext context;
        auto PQ = pqClientContainer->Construct(context);
        CHECK_WITH_LOG(!!PQ);
        UNIT_ASSERT(!PQ->ReadMessage(nullptr, TDuration::Seconds(2)));
        {
            auto wMessage = MakeHolder<NCS::TPQMessageSimple>(TBlob::FromString("payload"), "msgId");
            UNIT_ASSERT(!PQ->WriteMessage(wMessage.Release()));
        }
        CHECK_WITH_LOG(PQ->Start());
        CHECK_WITH_LOG(PQ->Stop());
    }

    Y_UNIT_TEST(Read) {
        const NCS::TPQClientConfigContainer pqClientContainer = BuildConfig("Read");
        TFakePQConstructionContext context;
        auto PQ = pqClientContainer->Construct(context);
        CHECK_WITH_LOG(!!PQ);
        CHECK_WITH_LOG(PQ->Start());
        {
            const TDuration waitingDuration = TDuration::Seconds(10);
            const TInstant start = Now();
            NCS::IPQMessage::TPtr rMessage;
            while (!rMessage && Now() - start < waitingDuration) {
                ui32 fails = 0;
                rMessage = PQ->ReadMessage(&fails, TDuration::Seconds(2));
                TFLEventLog::Log("read finished")("null", !rMessage)("fails", fails);
                UNIT_ASSERT_EQUAL(fails, 0);
            }
            UNIT_ASSERT(!rMessage);
            CHECK_WITH_LOG(PQ->Stop());
        }
    }

    Y_UNIT_TEST(Write) {
        const NCS::TPQClientConfigContainer pqClientContainer = BuildConfig("Read");
        TFakePQConstructionContext context;
        auto PQ = pqClientContainer->Construct(context);
        CHECK_WITH_LOG(!!PQ);
        CHECK_WITH_LOG(PQ->Start());
        {
            auto wMessage = MakeHolder<NCS::TPQMessageSimple>(TBlob::FromString("payload"), "msgId");
            UNIT_ASSERT(!!PQ->WriteMessage(wMessage.Release()));
            UNIT_ASSERT(PQ->FlushWritten());
        }
        CHECK_WITH_LOG(PQ->Stop());
    }

    Y_UNIT_TEST(WriteRead) {
        TFakePQConstructionContext context;
        {
            auto PQ = BuildConfig("WriteRead", false, true)->Construct(context);
            CHECK_WITH_LOG(PQ->Start());
            auto wMessage = MakeHolder<NCS::TPQMessageSimple>(TBlob::FromString("payload"), "msgId");
            UNIT_ASSERT(!!PQ->WriteMessage(wMessage.Release()));
            UNIT_ASSERT(PQ->FlushWritten());
            CHECK_WITH_LOG(PQ->Stop());
        }
        {
            auto PQ = BuildConfig("WriteRead", true, false)->Construct(context);
            CHECK_WITH_LOG(PQ->Start());
            NCS::IPQMessage::TPtr rMessage;
            ui32 fails = 0;
            rMessage = PQ->ReadMessage(&fails, TDuration::Seconds(2), true);
            TFLEventLog::Log("read finished")("null", !rMessage)("fails", fails);
            UNIT_ASSERT_EQUAL(fails, 0);
            UNIT_ASSERT(rMessage);
            UNIT_ASSERT_STRINGS_EQUAL(rMessage->GetMessageId(), "msgId");
            UNIT_ASSERT_STRINGS_EQUAL(TString(rMessage->GetContent().AsCharPtr(), rMessage->GetContent().Length()), "payload");
            UNIT_ASSERT(PQ->AckMessage(rMessage));
            rMessage.Reset();
            CHECK_WITH_LOG(PQ->Stop());
        }
    }

    Y_UNIT_TEST(Ack) {
        TFakePQConstructionContext context;
        {
            auto PQ = BuildConfig("Ack", false, true)->Construct(context);
            CHECK_WITH_LOG(PQ->Start());
            auto wMessage = MakeHolder<NCS::TPQMessageSimple>(TBlob::FromString("payload"), "msgId");
            UNIT_ASSERT(!!PQ->WriteMessage(wMessage.Release()));
            UNIT_ASSERT(PQ->FlushWritten());
            CHECK_WITH_LOG(PQ->Stop());
        }
        {
            auto PQ = BuildConfig("Ack", true, false)->Construct(context);
            CHECK_WITH_LOG(PQ->Start());
            ui32 fails = 0;
            auto rMessage = PQ->ReadMessage(&fails, TDuration::Seconds(2), true);
            TFLEventLog::Log("read finished")("null", !rMessage)("fails", fails);
            UNIT_ASSERT_EQUAL(fails, 0);
            UNIT_ASSERT(rMessage);
            UNIT_ASSERT_STRINGS_EQUAL(rMessage->GetMessageId(), "msgId");
            UNIT_ASSERT_STRINGS_EQUAL(TString(rMessage->GetContent().AsCharPtr(), rMessage->GetContent().Length()), "payload");
            rMessage.Reset();
            CHECK_WITH_LOG(PQ->Stop());
        }
        {
            auto PQ = BuildConfig("Ack", true, false)->Construct(context);
            CHECK_WITH_LOG(PQ->Start());
            ui32 fails = 0;
            auto rMessage = PQ->ReadMessage(&fails, TDuration::Seconds(2), true);
            TFLEventLog::Log("read finished")("null", !rMessage)("fails", fails);
            UNIT_ASSERT_EQUAL(fails, 0);
            UNIT_ASSERT(rMessage);
            UNIT_ASSERT_STRINGS_EQUAL(rMessage->GetMessageId(), "msgId");
            UNIT_ASSERT_STRINGS_EQUAL(TString(rMessage->GetContent().AsCharPtr(), rMessage->GetContent().Length()), "payload");
            UNIT_ASSERT(PQ->AckMessage(rMessage));
            rMessage.Reset();
            CHECK_WITH_LOG(PQ->Stop());
        }
        {
            auto PQ = BuildConfig("Ack", true, false)->Construct(context);
            CHECK_WITH_LOG(PQ->Start());
            ui32 fails = 0;
            auto rMessage = PQ->ReadMessage(&fails, TDuration::Seconds(2), true);
            TFLEventLog::Log("read finished")("null", !rMessage)("fails", fails);
            UNIT_ASSERT_EQUAL(fails, 0);
            UNIT_ASSERT(!rMessage);
            CHECK_WITH_LOG(PQ->Stop());
        }
    }
}
