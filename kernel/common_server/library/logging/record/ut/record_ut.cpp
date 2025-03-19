#include <kernel/common_server/library/logging/record/record.h>
#include <kernel/common_server/util/logging/tskv_log.h>
#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/json/json_value.h>
#include <library/cpp/json/json_reader.h>
#include <util/stream/str.h>
#include <kernel/common_server/library/logging/events.h>


Y_UNIT_TEST_SUITE(TestEventsLogRecordSuite) {
    constexpr static const char* GarbageString = "\"Лажа\" string со всякой 'фигней' && #мусором::";
    static NCS::NLogging::TBaseLogRecord CreateRecord() {
        NCS::NLogging::TBaseLogRecord r;
        r.Add("string", "correct string");
        r.Add("binary", GarbageString);
        r.Add("int", 5);
        return std::move(r);
    }

    Y_UNIT_TEST(TestSerializeToJson) {
        NCS::NLogging::TBaseLogRecord r = CreateRecord();
        NJson::TJsonValue json;
        auto serialized = r.SerializeToString(NCS::NLogging::ELogRecordFormat::Json);
        TStringInput si(serialized);
        UNIT_ASSERT_C(NJson::ReadJsonTree(&si, &json), serialized.c_str());
        UNIT_ASSERT_STRINGS_EQUAL_C(json["string"].GetString(), "correct string", serialized.c_str());
        UNIT_ASSERT_STRINGS_EQUAL_C(json["binary"].GetString(), GarbageString, serialized.c_str());
        UNIT_ASSERT_STRINGS_EQUAL_C(json["int"].GetString(), "5", serialized.c_str());
    }

    Y_UNIT_TEST(TestSerializeToTskv) {
        NCS::NLogging::TBaseLogRecord r = CreateRecord();
        const auto serialized = r.SerializeToString(NCS::NLogging::ELogRecordFormat::TSKV);
        auto map = NUtil::TTSKVRecordParser::Parse(serialized);
        UNIT_ASSERT_STRINGS_EQUAL_C(map["string"], "correct string", serialized.c_str());
        UNIT_ASSERT_STRINGS_EQUAL_C(map["binary"], GarbageString, serialized.c_str());
        UNIT_ASSERT_STRINGS_EQUAL_C(map["int"], "5", serialized.c_str());
    }

    Y_UNIT_TEST(TestPriorities) {
        auto gLogging1 = TFLRecords::StartContext()("module", "1")("*aaa", 1)("b", 1)("c", 1);
        auto gLogging2 = TFLRecords::StartContext()("module", "2")("*aaa", 3)("b", 2)("c", 2);
        NCS::NLogging::TBaseLogRecord r = TFLEventLog::Info("abc")("aaa", 2)("*b", 3)("c", 3);
        TMap<TString, TString> fields;
        const auto pred = [&fields](const TString& key, const TString& value) {
            CHECK_WITH_LOG(fields.emplace(key, value).second);
        };
        r.ScanFields(pred);
        UNIT_ASSERT_EQUAL(fields["module"], "2");
        UNIT_ASSERT_EQUAL(fields["aaa"], "3");
        UNIT_ASSERT_EQUAL(fields["b"], "3");
        UNIT_ASSERT_EQUAL(fields["c"], "1");
    }

};
