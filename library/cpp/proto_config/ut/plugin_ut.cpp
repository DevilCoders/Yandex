#include <library/cpp/proto_config/config.h>

#include <library/cpp/proto_config/ut/config.cfgproto.pb.h>

#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/config/sax.h>

#include <util/stream/file.h>
#include <util/stream/output.h>
#include <util/stream/str.h>

using namespace NProtoConfig;

Y_UNIT_TEST_SUITE(TestProtoconfig) {
    Y_UNIT_TEST(Default) {
        NOptions::TOptions options;
        UNIT_ASSERT(!options.HasDeadline());
        UNIT_ASSERT(options.Deadline() == TDuration());
        UNIT_ASSERT(!options.HasDefaultDeadline());
        UNIT_ASSERT(options.DefaultDeadline() == TDuration::MilliSeconds(123));

        NProtoConfig::TConfig config;
        UNIT_ASSERT(!config.OtherOptions().HasDeadline());
        UNIT_ASSERT(config.OtherOptions().Deadline() == TDuration());
        UNIT_ASSERT(!config.OtherOptions().HasDefaultDeadline());
        UNIT_ASSERT(config.OtherOptions().DefaultDeadline() == TDuration::MilliSeconds(123));

        UNIT_ASSERT(config.Proto3Options().Delay() == TDuration());

        UNIT_ASSERT(config.CustomOptions1().V1 == "v1");
        UNIT_ASSERT(config.CustomOptions1().V2 == "v2");
        UNIT_ASSERT(config.CustomOptions2().V1 == "default");
        UNIT_ASSERT(config.CustomOptions2().V2 == "default");

        UNIT_ASSERT(config.InnerOptions().V == 0);
        UNIT_ASSERT(config.InnerOptions().T == TDuration::MilliSeconds(10));

        {
            NProtoConfig::TInner inner = {
                123,
                TDuration::MilliSeconds(100)
            };

            UNIT_ASSERT(inner.V == 123);
            UNIT_ASSERT(inner.V2 == 1);
            UNIT_ASSERT(inner.T == TDuration::MilliSeconds(100));
            UNIT_ASSERT(!inner.M1.Defined());
            UNIT_ASSERT(!inner.M2.Defined());
        }

        {
            NProtoConfig::TInner inner = {
                .T = TDuration::MilliSeconds(100),
                .M1 = 123,
                .V2 = 5
            };

            UNIT_ASSERT(inner.V == 0);
            UNIT_ASSERT(inner.V2 == 5);
            UNIT_ASSERT(inner.T == TDuration::MilliSeconds(100));
            UNIT_ASSERT(inner.M1.Defined() && *inner.M1 == 123);
            UNIT_ASSERT(!inner.M2.Defined());
        }
    }

    void CheckParsed(const NProtoConfig::TConfig& config) {
        UNIT_ASSERT(config.Name() == "xxx");
        UNIT_ASSERT(config.RequestTimeout() == TDuration::MilliSeconds(10));
        UNIT_ASSERT(config.request_timeout() == TDuration::Seconds(5));

        UNIT_ASSERT(config.InnerOptions().V == 111);
        UNIT_ASSERT(config.InnerOptions().V2 == 1);
        UNIT_ASSERT(config.InnerOptions().T == TDuration::MilliSeconds(10));

        UNIT_ASSERT(!config.InnerOptions().M1.Defined());
        UNIT_ASSERT(*config.InnerOptions().M2 == 123);

        UNIT_ASSERT(config.OtherOptions().HasDeadline());
        UNIT_ASSERT(config.OtherOptions().Deadline() == TDuration::Seconds(1));
        UNIT_ASSERT(!config.OtherOptions().HasDefaultDeadline());
        UNIT_ASSERT(config.OtherOptions().DefaultDeadline() == TDuration::MilliSeconds(123));
        UNIT_ASSERT(config.OtherOptions().MaxLimit() == Max<ui32>());
        UNIT_ASSERT(config.OtherOptions().YetAnotherDeadline() == TDuration::Seconds(15));

        UNIT_ASSERT(config.RepeatedOptions().size() == 2);
        UNIT_ASSERT(config.RepeatedOptions()[0].DefaultDeadline() == TDuration::MilliSeconds(123));
        UNIT_ASSERT(config.RepeatedOptions()[1].DefaultDeadline() == TDuration::Seconds(10));

        UNIT_ASSERT(config.Proto3Options().Delay() == TDuration::Seconds(1));

        UNIT_ASSERT(config.CustomOptions1().V1 == "value1");
        UNIT_ASSERT(config.CustomOptions1().V2 == "value2");
        UNIT_ASSERT(config.CustomOptions2().V1 == "default");
        UNIT_ASSERT(config.CustomOptions2().V2 == "default");

        UNIT_ASSERT(config.SimpleMap().contains("a"));
        UNIT_ASSERT(config.SimpleMap().at("a") == "b");
        UNIT_ASSERT(config.SimpleMap().contains("c"));
        UNIT_ASSERT(config.SimpleMap().at("c") == "d");
        UNIT_ASSERT(config.SomeMap().contains("a"));
        UNIT_ASSERT(config.SomeMap().at("a").OtherMap().contains("b"));
        UNIT_ASSERT(config.SomeMap().at("a").OtherMap().at("b").V == 222);
        UNIT_ASSERT(config.SomeMap().contains("c"));
        UNIT_ASSERT(config.SomeMap().at("c").OtherMap().contains("d"));
        UNIT_ASSERT(config.SomeMap().at("c").OtherMap().at("d").V == 333);

        UNIT_ASSERT(config.SomeEnum() == NProtoConfig::Config::ONE);
    }

    Y_UNIT_TEST(ParseFromTextFormat) {
        TFileInput fi(ArcadiaSourceRoot() + "/library/cpp/proto_config/ut/config.txt");

        auto config = ParseConfigFromTextFormat<TConfig>(fi);
        CheckParsed(config);
    }

    void CheckNConfig(IInputStream& input) {
        THolder<NConfig::IConfig> parsed = NConfig::ConfigParser(input);

        {
            TVector<TString> unknownFields;
            auto config = ParseConfig<TConfig>(*parsed, [&](const TString& key, NConfig::IConfig::IValue*) {
                unknownFields.push_back(key);
            });

            CheckParsed(config);
            Sort(unknownFields);
            UNIT_ASSERT_VALUES_EQUAL(unknownFields.size(), 2);
            UNIT_ASSERT_VALUES_EQUAL(unknownFields[0], "unknown_field");
            UNIT_ASSERT_VALUES_EQUAL(unknownFields[1], "unknown_field_r");
        }
        {
            TVector<std::pair<TKeyStack, TString>> unknownFields;
            auto config = ParseConfig<TConfig>(*parsed,
                [&](const TKeyStack& parents, const TString& key, NConfig::IConfig::IValue*)
            {
                unknownFields.emplace_back(parents, key);
            });

            CheckParsed(config);
            SortBy(unknownFields, [](auto&& s) { return s.second; });
            UNIT_ASSERT_VALUES_EQUAL(unknownFields.size(), 2);
            UNIT_ASSERT_VALUES_EQUAL(unknownFields[0].first.size(), 0);
            UNIT_ASSERT_VALUES_EQUAL(unknownFields[1].first.size(), 2);
            UNIT_ASSERT_VALUES_EQUAL(unknownFields[0].second, "unknown_field");
            UNIT_ASSERT(std::holds_alternative<TField>(unknownFields[1].first[0]));
            UNIT_ASSERT_VALUES_EQUAL(std::get<TField>(unknownFields[1].first[0]).Name, "RepeatedOptions");
            UNIT_ASSERT(std::holds_alternative<TIdx>(unknownFields[1].first[1]));
            UNIT_ASSERT_VALUES_EQUAL(std::get<TIdx>(unknownFields[1].first[1]).Idx, 1);
            UNIT_ASSERT_VALUES_EQUAL(unknownFields[1].second, "unknown_field_r");
        }
    }

    Y_UNIT_TEST(ParseFromLua) {
        TFileInput fi(ArcadiaSourceRoot() + "/library/cpp/proto_config/ut/config.lua");
        CheckNConfig(fi);
    }

    Y_UNIT_TEST(ParseFromJson1) {
        TFileInput fi(ArcadiaSourceRoot() + "/library/cpp/proto_config/ut/config.json");
        auto config = ParseConfigFromJson<TConfig>(fi);
        CheckParsed(config);
    }

    Y_UNIT_TEST(ParseFromJson1WithComments) {
        TFileInput fi(ArcadiaSourceRoot() + "/library/cpp/proto_config/ut/config_with_comments.json");
        auto config = ParseConfigFromJson<TConfig>(fi);
        CheckParsed(config);
    }

    Y_UNIT_TEST(ParseFromJson2) {
        TFileInput fi(ArcadiaSourceRoot() + "/library/cpp/proto_config/ut/config.json");
        CheckNConfig(fi);
    }

    Y_UNIT_TEST(Mutable) {
        TFileInput fi(ArcadiaSourceRoot() + "/library/cpp/proto_config/ut/config.txt");

        auto config = ParseConfigFromTextFormat<TConfig>(fi);
        UNIT_ASSERT(config.Name() == "xxx");
        config.SetName("zzz");
        UNIT_ASSERT(config.Name() == "zzz");

        UNIT_ASSERT(config.InnerOptions().V == 111);
        config.InnerOptions().V = 333;
        UNIT_ASSERT(config.InnerOptions().V == 333);

        UNIT_ASSERT(!config.OtherOptions().HasDefaultDeadline());
        UNIT_ASSERT(config.OtherOptions().DefaultDeadline() == TDuration::MilliSeconds(123));
        config.OtherOptions().SetDefaultDeadline(TDuration::Seconds(123));
        UNIT_ASSERT(config.OtherOptions().HasDefaultDeadline());
        UNIT_ASSERT(config.OtherOptions().DefaultDeadline() == TDuration::Seconds(123));

        UNIT_ASSERT(config.Proto3Options().Delay() == TDuration::Seconds(1));
        config.Proto3Options().SetDelay(TDuration::Seconds(2));
        UNIT_ASSERT(config.Proto3Options().Delay() == TDuration::Seconds(2));

        UNIT_ASSERT(config.CustomOptions2().V1 == "default");
        TCustomOptions tmp = config.CustomOptions2();
        tmp.V1 = "newV1";
        config.SetCustomOptions2(tmp);
        UNIT_ASSERT(config.CustomOptions2().V1 == "newV1");
    }

    Y_UNIT_TEST(Override) {
        TFileInput fi(ArcadiaSourceRoot() + "/library/cpp/proto_config/ut/config.txt");

        auto proto = ParseConfigFromTextFormat<TConfig::TProto>(fi);

        TConfig config1(proto);

        UNIT_ASSERT(config1.Name() == "xxx");
        UNIT_ASSERT(config1.InnerOptions().V == 111);

        OverrideConfig(proto, "Name=zzz");
        OverrideConfig(proto, "InnerOptions.V=333");

        TConfig config2(proto);

        UNIT_ASSERT(config2.Name() == "zzz");
        UNIT_ASSERT(config2.InnerOptions().V == 333);
    }

    Y_UNIT_TEST(TestRequired) {
        UNIT_ASSERT_EXCEPTION_CONTAINS(([]() {
            TStringStream ss("instance = { Required = {}; };");
            THolder<NConfig::IConfig> parsed = NConfig::ConfigParser(ss);
            TVector<TString> unknownFields;
            ParseConfig<TConfig>(*parsed);
        })(), yexception, "Required field X not set at .Required");

        UNIT_ASSERT_EXCEPTION_CONTAINS(([]() {
            TStringStream ss("instance = { RepRequired = { {}; }; };");
            THolder<NConfig::IConfig> parsed = NConfig::ConfigParser(ss);
            TVector<TString> unknownFields;
            ParseConfig<TConfig>(*parsed);
        })(), yexception, "Required field X not set at .RepRequired[0]");

        UNIT_ASSERT_EXCEPTION_CONTAINS(([]() {
            TStringStream ss("instance = { MapRequired = { {}; }; };");
            THolder<NConfig::IConfig> parsed = NConfig::ConfigParser(ss);
            TVector<TString> unknownFields;
            ParseConfig<TConfig>(*parsed);
        })(), yexception, "Required field X not set at .MapRequired[\"0\"]");

        UNIT_ASSERT_EXCEPTION_CONTAINS(([]() {
            TStringStream ss("instance = { MapSubRequired = { a = { Required = {}; }; }; };");
            THolder<NConfig::IConfig> parsed = NConfig::ConfigParser(ss);
            TVector<TString> unknownFields;
            ParseConfig<TConfig>(*parsed);
        })(), yexception, "Required field X not set at .MapSubRequired[\"a\"].Required");
    }

    Y_UNIT_TEST(ParseUnknownEnumValue) {
        {
            TFileInput fi(ArcadiaSourceRoot() + "/library/cpp/proto_config/ut/config_unknown_enum.json");
            UNIT_ASSERT_EXCEPTION_CONTAINS(ParseConfigFromJson<TConfig>(fi), yexception, "Invalid string value of JSON enum field: THREE.");
        }

        {
            TFileInput fi(ArcadiaSourceRoot() + "/library/cpp/proto_config/ut/config_unknown_enum.txt");
            UNIT_ASSERT_EXCEPTION_CONTAINS(ParseConfigFromTextFormat<TConfig>(fi), yexception, "ParseFromTextFormat failed on Parse for NProtoConfig.Config");
        }
    }
}
