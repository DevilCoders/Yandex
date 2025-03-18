#include <library/cpp/json/json_reader.h>
#include <library/cpp/testing/unittest/registar.h>

#include <library/cpp/monlib/push_client/pull_client.h>
#include <library/cpp/monlib/push_client/push_client.h>

#include <util/string/builder.h>

using namespace NSolomon;

namespace {

    NJson::TJsonValue ReadJson(const TString& s) {
        NJson::TJsonValue jsonValue;

        TStringInput in(s);
        NJson::ReadJsonTree(&in, &jsonValue, /*throwOnError*/ true);
        return jsonValue;
    }

} // namespace anonymous

Y_UNIT_TEST_SUITE(TSolomonTest) {
    Y_UNIT_TEST(TestPull) {
        TSolomonJsonBuilder builder{TSolomonJsonBuilder::EFormat::LegacyJson};
        builder.AddSensor({{"key1", "value1"}, {"key2", "value2"}}, 50l);
        builder.AddDerivSensor({{"key1", "value1"}, {"key2", "value2"}}, 50l);
        builder.AddSensor({{"key3", "value3"}}, 49.5);

        NJson::TJsonValue expected;
        auto& sensors = expected["sensors"];
        {
            auto& sensor = sensors[0];
            sensor["labels"]["key1"] = "value1";
            sensor["labels"]["key2"] = "value2";
            sensor["value"] = 50;
        }
        {
            auto& sensor = sensors[1];
            sensor["labels"]["key1"] = "value1";
            sensor["labels"]["key2"] = "value2";
            sensor["value"] = 50;
            sensor["mode"] = "deriv";
        }
        {
            auto& sensor = sensors[2];
            sensor["labels"]["key3"] = "value3";
            sensor["value"] = 49.5;
        }

        UNIT_ASSERT_VALUES_EQUAL(ReadJson(builder.GetJsonValue()), expected);
    }

    class TTestBuilder : public TSolomonPushJsonBuilder {
    private:
        using TParent = TSolomonPushJsonBuilder;

    public:
        TTestBuilder(const TSolomonPushConfig& config)
            : TSolomonPushJsonBuilder(config, EFormat::LegacyJson)
        {
        }

        TString GetResultForTest() {
            TParent::Finish();
            return GetJsonValue();
        }
    };

    Y_UNIT_TEST(TestPush) {
        TTestBuilder builder(
            TSolomonPushConfig()
                .WithProject("MyProject")
                .WithCluster("MyCluster")
                .WithService("MyService")
                .WithHost("localhost")
        );

        builder.AddSensor({{"key1", "value1"}, {"key2", "value2"}}, 50l);
        builder.AddSensor({{"key3", "value3"}}, TInstant::ParseIso8601("2018-06-22T00:01:01Z"), 500l);

        NJson::TJsonValue expected;
        auto& sensors = expected["sensors"];
        {
            auto& sensor = sensors[0];
            sensor["labels"]["key1"] = "value1";
            sensor["labels"]["key2"] = "value2";
            sensor["value"] = 50;
        }
        {
            auto& sensor = sensors[1];
            sensor["labels"]["key3"] = "value3";
            sensor["ts"] = 1529625661;
            sensor["value"] = 500;
        }

        auto& common = expected["commonLabels"];
        common["host"] = "localhost";

        UNIT_ASSERT_VALUES_EQUAL(ReadJson(builder.GetResultForTest()), expected);
    }
};
