#include <library/cpp/json/json_reader.h>
#include <library/cpp/json/json_writer.h>
#include <library/cpp/protobuf/yql/descriptor.h>
#include <library/cpp/protobuf/yql/ut/a.pb.h>
#include <library/cpp/protobuf/yql/ut/nested.pb.h>
#include <library/cpp/string_utils/base64/base64.h>
#include <library/cpp/testing/unittest/registar.h>

template <typename T>
static inline TString ToProtoString(const T& proto) {
    TString ret;
    Y_PROTOBUF_SUPPRESS_NODISCARD proto.SerializeToString(&ret);
    return ret;
}

static const char* const MESSAGE_PROTOTEXT =
    R"(B {
  Value: 7
  D {
    Data: "text value"
  }
}
)";

static const char* const MESSAGE_PROTOTEXT_NESTED =
    R"(Int: 71
)";

static const char* const MESSAGE_JSON =
    R"({ "B" : {
  "Value" : 7,
  "D" : {
    "Data" : "text value"
  }
} }
)";

Y_UNIT_TEST_SUITE(TDynamicMessage) {
    Y_UNIT_TEST(ParseConfig) {
        auto result = ParseTypeConfig("#A+MTIzNDU2Nzg5MA==");

        UNIT_ASSERT_EQUAL(result.MessageName, "A");
        UNIT_ASSERT_EQUAL(result.Metadata, "MTIzNDU2Nzg5MA==");
        UNIT_ASSERT_EQUAL(result.SkipBytes, 0);

        UNIT_ASSERT_EQUAL(Base64Decode(result.Metadata), "1234567890");
    }

    Y_UNIT_TEST(SkipBytes) {
        auto result = ParseTypeConfig(
            "{\"skip\":4,\"name\":\"A\",\"meta\":\"MTIzNDU2Nzg5MA==\"}");

        UNIT_ASSERT_EQUAL(result.MessageName, "A");
        UNIT_ASSERT_EQUAL(result.Metadata, "MTIzNDU2Nzg5MA==");
        UNIT_ASSERT_EQUAL(result.SkipBytes, 4);

        UNIT_ASSERT_EQUAL(Base64Decode(result.Metadata), "1234567890");
    }

    Y_UNIT_TEST(OptionalLists) {
        auto result = ParseTypeConfig(
            "{\"lists\":{\"optional\":false}}");

        UNIT_ASSERT_EQUAL(result.OptionalLists, false);
    }

    Y_UNIT_TEST(Parse) {
        TString data;
        TString typeConfig = GenerateProtobufTypeConfig<NTest::TA>();

        {
            NTest::TA rec;
            rec.MutableB()->SetValue(7);
            rec.MutableB()->MutableD()->SetData("text value");
            data = ToProtoString(rec);
        }

        auto dyn = TDynamicInfo::Create(typeConfig);
        UNIT_ASSERT(dyn != nullptr);

        auto msg = dyn->Parse(data);
        UNIT_ASSERT(msg != nullptr);

        UNIT_ASSERT_EQUAL(msg->DebugString(), MESSAGE_PROTOTEXT);
    }

    Y_UNIT_TEST(ParseWithFallback) {
        TString data;
        TString typeConfig = GenerateProtobufTypeConfig<NTest::TA>();

        // Эмулируем первоначальный способ хранения имени сообщения,
        // когда сохранялось только имя сообщения первого уровня без
        // имени пакета.
        {
            NJson::TJsonValue value;

            if (NJson::ReadJsonFastTree(typeConfig, &value)) {
                value["name"] = "TA";
                typeConfig = NJson::WriteJson(value, false);
            }
        }

        {
            NTest::TA rec;
            rec.MutableB()->SetValue(7);
            rec.MutableB()->MutableD()->SetData("text value");
            data = ToProtoString(rec);
        }

        auto dyn = TDynamicInfo::Create(typeConfig);
        UNIT_ASSERT(dyn != nullptr);

        auto msg = dyn->Parse(data);
        UNIT_ASSERT(msg != nullptr);

        UNIT_ASSERT_EQUAL(msg->DebugString(), MESSAGE_PROTOTEXT);
    }

    Y_UNIT_TEST(ParseNested) {
        TString data;
        TString typeConfig =
            GenerateProtobufTypeConfig<P1::P2::TTopLevel::TNested>();

        {
            P1::P2::TTopLevel::TNested rec;
            rec.SetInt(71);
            data = ToProtoString(rec);
        }

        auto dyn = TDynamicInfo::Create(typeConfig);
        UNIT_ASSERT(dyn != nullptr);

        auto msg = dyn->Parse(data);
        UNIT_ASSERT(msg != nullptr);

        UNIT_ASSERT_EQUAL(msg->DebugString(), MESSAGE_PROTOTEXT_NESTED);
    }

    Y_UNIT_TEST(ParseWithJsonMeta) {
        TString data;
        TString typeConfig = GenerateProtobufTypeConfig<NTest::TA>(
            TProtoTypeConfigOptions()
                .SetSkipBytes(4));

        {
            NTest::TA rec;
            rec.MutableB()->SetValue(7);
            rec.MutableB()->MutableD()->SetData("text value");
            data = "    " + ToProtoString(rec);
        }

        auto dyn = TDynamicInfo::Create(typeConfig);
        UNIT_ASSERT(dyn != nullptr);

        auto msg = dyn->Parse(data);
        UNIT_ASSERT(msg != nullptr);

        UNIT_ASSERT_EQUAL(msg->DebugString(), MESSAGE_PROTOTEXT);
    }

    Y_UNIT_TEST(ParsePrototextFormat) {
        TString data = MESSAGE_PROTOTEXT;
        TString typeConfig = GenerateProtobufTypeConfig<NTest::TA>(
            TProtoTypeConfigOptions()
                .SetProtoFormat(PF_PROTOTEXT));

        auto dyn = TDynamicInfo::Create(typeConfig);
        UNIT_ASSERT(dyn != nullptr);

        auto msg = dyn->Parse(data);
        UNIT_ASSERT(msg != nullptr);

        UNIT_ASSERT_EQUAL(msg->DebugString(), MESSAGE_PROTOTEXT);
    }

    Y_UNIT_TEST(ParseJsonFormat) {
        TString data = MESSAGE_JSON;
        TString typeConfig = GenerateProtobufTypeConfig<NTest::TA>(
            TProtoTypeConfigOptions()
                .SetProtoFormat(PF_JSON));

        auto dyn = TDynamicInfo::Create(typeConfig);
        UNIT_ASSERT(dyn != nullptr);

        auto msg = dyn->Parse(data);
        UNIT_ASSERT(msg != nullptr);

        UNIT_ASSERT_EQUAL(msg->DebugString(), MESSAGE_PROTOTEXT);
    }
}
