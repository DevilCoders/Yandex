#include <library/cpp/protobuf/json/json2proto.h>
#include <library/cpp/protobuf/json/proto2json.h>
#include <library/cpp/protobuf/yt/proto2schema.h>
#include <library/cpp/protobuf/yt/proto2yt.h>
#include <library/cpp/protobuf/yt/ut/proto/test.pb.h>
#include <library/cpp/protobuf/yt/yt2proto.h>
#include <library/cpp/resource/resource.h>

#include <library/cpp/testing/unittest/registar.h>

#include <library/cpp/yson/node/node_io.h>

#include <util/generic/xrange.h>
#include <util/stream/file.h>

auto GetInner(const TWifiProfile& profile) {
    TVector<std::pair<TString, TWifiInfo>> result;
    for (const auto& p : profile.wifi_profile()) {
        result.push_back({p.first, p.second});
    }
    const auto cmp = [&](const auto& a, const auto& b) {
        return a.first < b.first;
    };
    Sort(result.begin(), result.end(), cmp);
    return result;
}

void CheckEqual(TWifiProfile first, TWifiProfile second) {
    auto innerFirst = GetInner(first);
    auto innerSecond = GetInner(second);
    UNIT_ASSERT_VALUES_EQUAL(innerFirst.size(), innerSecond.size());
    auto protoToJsonConfig = NProtobufJson::TProto2JsonConfig().SetMapAsObject(true);
    for (size_t i : xrange(innerFirst.size())) {
        UNIT_ASSERT_STRINGS_EQUAL(innerFirst[i].first, innerSecond[i].first);
        UNIT_ASSERT_STRINGS_EQUAL(
            NProtobufJson::Proto2Json(innerFirst[i].second, protoToJsonConfig),
            NProtobufJson::Proto2Json(innerSecond[i].second, protoToJsonConfig)
        );
    }
}

void FillTestMessageFromDefaults(::google::protobuf::Message& message) {
    NYT::TNode node({
        {"uint64", 1ull},
        {"sint64", 0ll},
        {"fixed64", 0ull},
        {"float_field", 0.0f},
        {"double_field", 0.0},
        {"string", ""},
        {"bytes", ""},
        {"enum_field", 1},
        {"repeated_ui64", NYT::TNode::CreateList()},
        {"sub_message", NYT::TNode::CreateMap()("key", "")("value", false)},
    });
    YtNodeToProto(node, message, TParseConfig().SetSkipEmptyOptionalFields(true));
}

Y_UNIT_TEST_SUITE(TProtoFromYson) {
    Y_UNIT_TEST(TStringKeyMap) {
        auto expectedJsonStr = NResource::Find("/wifi_profile.json");
        TWifiProfile expectedProto;
        NProtobufJson::Json2Proto(expectedJsonStr, expectedProto, NProtobufJson::TJson2ProtoConfig().SetMapAsObject(true));
        auto node = NYT::NodeFromJsonString(expectedJsonStr);
        TWifiProfile receivedProto;
        YtNodeToProto(node, receivedProto);
        CheckEqual(expectedProto, receivedProto);
    }

    Y_UNIT_TEST(TIntKeyMap) {
        NYT::TNode node = NYT::TNode::CreateMap();
        auto& innerNode = node["test_map"] = NYT::TNode::CreateMap();
        innerNode["1"] = "one";
        innerNode["2"] = "two";
        TTestIntKeyMap protoMap;
        YtNodeToProto(node, protoMap);
        UNIT_ASSERT_EQUAL(protoMap.test_map_size(), 2);
        UNIT_ASSERT_STRINGS_EQUAL(protoMap.test_map().at(1), "one");
        UNIT_ASSERT_STRINGS_EQUAL(protoMap.test_map().at(2), "two");
    }

    Y_UNIT_TEST(TIntKeyMapFromList) {
        NYT::TNode node = NYT::TNode::CreateMap();
        auto& innerNode = node["test_map"] = NYT::TNode::CreateList();
        innerNode.AsList().push_back(NYT::TNode()("key", 1)("value", "one"));
        innerNode.AsList().push_back(NYT::TNode()("key", 2)("value", "two"));
        TTestIntKeyMap protoMap;
        YtNodeToProto(node, protoMap, TParseConfig().SetProtoMapFromList());
        UNIT_ASSERT_EQUAL(protoMap.test_map_size(), 2);
        UNIT_ASSERT_STRINGS_EQUAL(protoMap.test_map().at(1), "one");
        UNIT_ASSERT_STRINGS_EQUAL(protoMap.test_map().at(2), "two");
    }

    Y_UNIT_TEST(TBoolKeyMap) {
        NYT::TNode node = NYT::TNode::CreateMap();
        auto& innerNode = node["test_map"] = NYT::TNode::CreateMap();
        innerNode["false"] = "false";
        innerNode["true"] = "true";
        TTestBoolKeyMap protoMap;
        YtNodeToProto(node, protoMap);
        UNIT_ASSERT_EQUAL(protoMap.test_map_size(), 2);
        UNIT_ASSERT_STRINGS_EQUAL(protoMap.test_map().at(false), "false");
        UNIT_ASSERT_STRINGS_EQUAL(protoMap.test_map().at(true), "true");
    }

    Y_UNIT_TEST(TBoolKeyMapFromList) {
        NYT::TNode node = NYT::TNode::CreateMap();
        auto& innerNode = node["test_map"] = NYT::TNode::CreateList();
        innerNode.AsList().push_back(NYT::TNode()("key", false)("value", "false"));
        innerNode.AsList().push_back(NYT::TNode()("key", true)("value", "true"));
        TTestBoolKeyMap protoMap;
        YtNodeToProto(node, protoMap, TParseConfig().SetProtoMapFromList());
        UNIT_ASSERT_EQUAL(protoMap.test_map_size(), 2);
        UNIT_ASSERT_STRINGS_EQUAL(protoMap.test_map().at(false), "false");
        UNIT_ASSERT_STRINGS_EQUAL(protoMap.test_map().at(true), "true");
    }

    Y_UNIT_TEST(TZeroMap) {
        NYT::TNode node = NYT::TNode::CreateMap();
        node["test_map"] = NYT::TNode::CreateEntity();
        TTestBoolKeyMap protoMap;
        YtNodeToProto(node, protoMap);
        UNIT_ASSERT_EQUAL(protoMap.test_map_size(), 0);
    }

    Y_UNIT_TEST(TZeroMapFromList) {
        NYT::TNode node = NYT::TNode::CreateMap();
        node["test_map"] = NYT::TNode::CreateList();
        TTestBoolKeyMap protoMap;
        YtNodeToProto(node, protoMap, TParseConfig().SetProtoMapFromList());
        UNIT_ASSERT_EQUAL(protoMap.test_map_size(), 0);
    }

    Y_UNIT_TEST(TSkipDefaultsWhenRequired) {
        TestMessageWithRequiredFields proto;
        FillTestMessageFromDefaults(proto);
        UNIT_ASSERT(proto.IsInitialized()); // check that all required fields are set
    }

    Y_UNIT_TEST(TSkipDefaultsWhenOptional) {
        TestMessageWithOptionalFields proto;
        FillTestMessageFromDefaults(proto);
        UNIT_ASSERT(!proto.has_uint64());
        UNIT_ASSERT(!proto.has_sint64());
        UNIT_ASSERT(!proto.has_fixed64());
        UNIT_ASSERT(!proto.has_float_field());
        UNIT_ASSERT(!proto.has_double_field());
        UNIT_ASSERT(!proto.has_string());
        UNIT_ASSERT(!proto.has_bytes());
        UNIT_ASSERT(!proto.has_enum_field());
        UNIT_ASSERT(!proto.sub_message().has_key());
        UNIT_ASSERT(!proto.sub_message().has_value());
    }

    Y_UNIT_TEST(TFillNonDefaultsForOptional) {
        NYT::TNode node({
            {"uint64", 0ull},
            {"sint64", 1ll},
            {"fixed64", 2ull},
            {"float_field", 3.0f},
            {"double_field", 4.0},
            {"string", "5"},
            {"bytes", "6"},
            {"enum_field", 2},
            {"repeated_ui64", NYT::TNode::CreateList().Add(7ull)},
            {"sub_message", NYT::TNode::CreateMap()("key", "8")("value", true)},
        });
        TestMessageWithOptionalFields proto;
        YtNodeToProto(node, proto, TParseConfig().SetSkipEmptyOptionalFields(true));
        UNIT_ASSERT_EQUAL(proto.uint64(), 0ull);
        UNIT_ASSERT_EQUAL(proto.sint64(), 1ull);
        UNIT_ASSERT_EQUAL(proto.fixed64(), 2ull);
        UNIT_ASSERT_EQUAL(proto.float_field(), 3.0f);
        UNIT_ASSERT_EQUAL(proto.double_field(), 4.0f);
        UNIT_ASSERT_EQUAL(proto.string(), "5");
        UNIT_ASSERT_EQUAL(proto.bytes(), "6");
        UNIT_ASSERT_EQUAL(proto.enum_field(), ETestEnum::SECOND);
        UNIT_ASSERT_EQUAL(proto.repeated_ui64().size(), 1u);
        UNIT_ASSERT_EQUAL(proto.repeated_ui64(0), 7ull);
        UNIT_ASSERT_EQUAL(proto.sub_message().key(), "8");
        UNIT_ASSERT_EQUAL(proto.sub_message().value(), true);
    }

    Y_UNIT_TEST(TParseEnumFromString) {
        NYT::TNode node({
            {"string_enum_field", "SECOND"},
            {"number_enum_field", 2},
            {"repeated_enum_field", NYT::TNode::CreateList().Add("FIRST").Add(1).Add(2).Add("SECOND")}
        });
        auto checkNode2ProtoEnumSimple = [node](const TParseConfig& config) {
            TestMessageWithEnumFields proto;
            YtNodeToProto(node, proto, config);
            UNIT_ASSERT_EQUAL(proto.string_enum_field(), ETestEnum::SECOND);
            UNIT_ASSERT_EQUAL(proto.number_enum_field(), ETestEnum::SECOND);
            UNIT_ASSERT_EQUAL(proto.repeated_enum_field_size(), 4u);
            UNIT_ASSERT_EQUAL(proto.repeated_enum_field(0), ETestEnum::FIRST);
            UNIT_ASSERT_EQUAL(proto.repeated_enum_field(1), ETestEnum::FIRST);
            UNIT_ASSERT_EQUAL(proto.repeated_enum_field(2), ETestEnum::SECOND);
            UNIT_ASSERT_EQUAL(proto.repeated_enum_field(3), ETestEnum::SECOND);
        };
        checkNode2ProtoEnumSimple(TParseConfig());
        checkNode2ProtoEnumSimple(TParseConfig().SetSaveEnumsAsString(false));
        checkNode2ProtoEnumSimple(TParseConfig().SetSaveEnumsAsString(true));
    }

    Y_UNIT_TEST(TParseEnumFromStringWithSkipDefaults) {
        NYT::TNode node({
            {"string_enum_field", "FIRST"},
            {"number_enum_field", 1}
        });
        TestMessageWithEnumFields proto;
        YtNodeToProto(node, proto, TParseConfig().SetSkipEmptyOptionalFields(true));
        UNIT_ASSERT(!proto.has_string_enum_field());
        UNIT_ASSERT(!proto.has_number_enum_field());
    }

    Y_UNIT_TEST(TParseEnumFromStringWithEnumCaseInsensitive) {
        NYT::TNode node({
            {"string_enum_field", "fIrsT"},
            {"repeated_enum_field", NYT::TNode::CreateList().Add("Second").Add(2).Add("second")}
        });
        auto checkNode2ProtoEnumCaseInsensetive = [node](const TParseConfig& config) {
            TestMessageWithEnumFields proto;
            YtNodeToProto(node, proto, config);
            UNIT_ASSERT(!proto.has_string_enum_field());
            UNIT_ASSERT_EQUAL(proto.repeated_enum_field_size(), 3u);
            UNIT_ASSERT_EQUAL(proto.repeated_enum_field(0), ETestEnum::SECOND);
            UNIT_ASSERT_EQUAL(proto.repeated_enum_field(1), ETestEnum::SECOND);
            UNIT_ASSERT_EQUAL(proto.repeated_enum_field(1), ETestEnum::SECOND);
        };

        checkNode2ProtoEnumCaseInsensetive(TParseConfig().SetEnumCaseInsensitive(true).SetSkipEmptyOptionalFields(true));
        checkNode2ProtoEnumCaseInsensetive(TParseConfig().SetEnumCaseInsensitive(true).SetSkipEmptyOptionalFields(true).SetSaveEnumsAsString(false));
        checkNode2ProtoEnumCaseInsensetive(TParseConfig().SetEnumCaseInsensitive(true).SetSkipEmptyOptionalFields(true).SetSaveEnumsAsString(true));
    }

    Y_UNIT_TEST(TParseWithCastRobust) {
        NYT::TNode node({
            {"sub_message", NYT::TNode({
                {"uint64", 1337},
                {"int64", 31337u},
                {"uint32", "16"},
                {"int32", false},
                {"float_field", "17.5"},
                {"double_field", 3},
                {"bool_field", 1.2},
                {"string", 21},
                {"enum_field", 1u}
            })},
            {"rep_sub_message", NYT::TNode({
                {"uint64", NYT::TNode::CreateList().Add("1").Add(1)},
                {"int64", NYT::TNode::CreateList().Add("2").Add(2u)},
                {"uint32", NYT::TNode::CreateList().Add("3").Add(3)},
                {"int32", NYT::TNode::CreateList().Add("4").Add(4u)},
                {"float_field", NYT::TNode::CreateList().Add("5").Add(0)},
                {"double_field", NYT::TNode::CreateList().Add("6.125").Add(1.25f)},
                {"bool_field", NYT::TNode::CreateList().Add("true").Add(0)},
                {"string", NYT::TNode::CreateList().Add(8).Add("something good")},
                {"enum_field", NYT::TNode::CreateList().Add(2u).Add(1)}
            })},
        });
        TestMessageWithTypeFields proto;
        YtNodeToProto(node, proto, TParseConfig().SetCastRobust(true).SetSkipEmptyOptionalFields(true));
        UNIT_ASSERT_EQUAL(proto.sub_message().uint64(), 1337ull);
        UNIT_ASSERT_EQUAL(proto.sub_message().int64(), 31337ll);
        UNIT_ASSERT_EQUAL(proto.sub_message().uint32(), 16ul);
        UNIT_ASSERT_EQUAL(proto.sub_message().int32(), 0l);
        UNIT_ASSERT_EQUAL(proto.sub_message().float_field(), 17.5f);
        UNIT_ASSERT_EQUAL(proto.sub_message().double_field(), 3.0f);
        UNIT_ASSERT_EQUAL(proto.sub_message().bool_field(), true);
        UNIT_ASSERT_EQUAL(proto.sub_message().string(), "21");
        UNIT_ASSERT_EQUAL(proto.sub_message().enum_field(), ETestEnum::FIRST);

        UNIT_ASSERT_EQUAL(proto.rep_sub_message().uint64(0), 1ull);
        UNIT_ASSERT_EQUAL(proto.rep_sub_message().uint64(1), 1ull);
        UNIT_ASSERT_EQUAL(proto.rep_sub_message().int64(0), 2ll);
        UNIT_ASSERT_EQUAL(proto.rep_sub_message().int64(1), 2ll);
        UNIT_ASSERT_EQUAL(proto.rep_sub_message().uint32(0), 3ul);
        UNIT_ASSERT_EQUAL(proto.rep_sub_message().uint32(1), 3ul);
        UNIT_ASSERT_EQUAL(proto.rep_sub_message().int32(0), 4ll);
        UNIT_ASSERT_EQUAL(proto.rep_sub_message().int32(1), 4ll);
        UNIT_ASSERT_EQUAL(proto.rep_sub_message().float_field(0), 5.0f);
        UNIT_ASSERT_EQUAL(proto.rep_sub_message().float_field(1), 0.0f);
        UNIT_ASSERT_EQUAL(proto.rep_sub_message().double_field(0), 6.125f);
        UNIT_ASSERT_EQUAL(proto.rep_sub_message().double_field(1), 1.25f);
        UNIT_ASSERT_EQUAL(proto.rep_sub_message().bool_field(0), true);
        UNIT_ASSERT_EQUAL(proto.rep_sub_message().bool_field(1), false);
        UNIT_ASSERT_EQUAL(proto.rep_sub_message().string(0), "8");
        UNIT_ASSERT_EQUAL(proto.rep_sub_message().string(1), "something good");
        UNIT_ASSERT_EQUAL(proto.rep_sub_message().enum_field(0), ETestEnum::SECOND);
        UNIT_ASSERT_EQUAL(proto.rep_sub_message().enum_field(1), ETestEnum::FIRST);
    }

    Y_UNIT_TEST(TTestSkipEmptyRepeatedFields) {
        TestMessageWithTypeFields proto;

        proto.mutable_rep_sub_message()->add_uint64(1ull);
        proto.mutable_rep_sub_message()->add_uint64(2ull);

        proto.mutable_rep_sub_message()->add_int64(1ll);
        proto.mutable_rep_sub_message()->add_int64(2ll);

        TParseConfig config;
        config.SetSkipEmptyRepeatedFields();

        auto node = ProtoToYtNode(proto, config);

        const auto presentedFields = {"uint64", "int64"};
        const auto notPresentedFields = {"uint32", "int32", "float_field", "double_field", "bool_field", "string", "enum_field"};

        UNIT_ASSERT(node.HasKey("rep_sub_message"));

        const auto& rep = node.At("rep_sub_message");

        for (const auto& fieldName : presentedFields) {
            UNIT_ASSERT(rep.HasKey(fieldName));

            const auto& field = rep.At(fieldName);

            UNIT_ASSERT_EQUAL(field.ChildIntCast<ui64>(0), 1ull);
            UNIT_ASSERT_EQUAL(field.ChildIntCast<ui64>(1), 2ull);
        }

        for (const auto& fieldName : notPresentedFields) {
            UNIT_ASSERT(!rep.HasKey(fieldName));
        }
    }
}

Y_UNIT_TEST_SUITE(TProtoToYson) {

    Y_UNIT_TEST(TTestProtoToYtNodeWithEnums) {
        TestMessageWithEnumFields proto;
        proto.set_string_enum_field(FIRST);
        proto.set_number_enum_field(SECOND);
        proto.add_repeated_enum_field(FIRST);
        proto.add_repeated_enum_field(SECOND);

        {
            auto node = ProtoToYtNode(proto, TParseConfig());
            UNIT_ASSERT(node.HasKey("string_enum_field") && node["string_enum_field"].IsInt64());
            UNIT_ASSERT_EQUAL(node["string_enum_field"].AsInt64(), 1);
            UNIT_ASSERT(node.HasKey("number_enum_field") && node["number_enum_field"].IsInt64());
            UNIT_ASSERT_EQUAL(node["number_enum_field"].AsInt64(), 2);
            UNIT_ASSERT(!node.HasKey("missing_enum_field"));
            UNIT_ASSERT(node.HasKey("repeated_enum_field") && node["repeated_enum_field"].IsList());
            UNIT_ASSERT(node["repeated_enum_field"].AsList().size() == 2);
            UNIT_ASSERT_EQUAL(node["repeated_enum_field"].AsList()[0].AsInt64(), 1);
            UNIT_ASSERT_EQUAL(node["repeated_enum_field"].AsList()[1].AsInt64(), 2);
        }

        {
            auto node = ProtoToYtNode(proto, TParseConfig().SetUseImplicitDefault(true));
            UNIT_ASSERT(node.HasKey("string_enum_field") && node["string_enum_field"].IsInt64());
            UNIT_ASSERT_EQUAL(node["string_enum_field"].AsInt64(), 1);
            UNIT_ASSERT(node.HasKey("number_enum_field") && node["number_enum_field"].IsInt64());
            UNIT_ASSERT_EQUAL(node["number_enum_field"].AsInt64(), 2);
            UNIT_ASSERT(node.HasKey("missing_enum_field") && node["missing_enum_field"].IsInt64());
            UNIT_ASSERT_EQUAL(node["missing_enum_field"].AsInt64(), 1);
            UNIT_ASSERT(node.HasKey("repeated_enum_field") && node["repeated_enum_field"].IsList());
            UNIT_ASSERT(node["repeated_enum_field"].AsList().size() == 2);
            UNIT_ASSERT_EQUAL(node["repeated_enum_field"].AsList()[0].AsInt64(), 1);
            UNIT_ASSERT_EQUAL(node["repeated_enum_field"].AsList()[1].AsInt64(), 2);
        }


        {
            auto node = ProtoToYtNode(proto, TParseConfig().SetSaveEnumsAsString(true));
            UNIT_ASSERT(node.HasKey("string_enum_field") && node["string_enum_field"].IsString());
            UNIT_ASSERT_EQUAL(node["string_enum_field"].AsString(), "FIRST");
            UNIT_ASSERT(node.HasKey("number_enum_field") && node["number_enum_field"].IsString());
            UNIT_ASSERT_EQUAL(node["number_enum_field"].AsString(), "SECOND");
            UNIT_ASSERT(!node.HasKey("missing_enum_field"));
            UNIT_ASSERT(node.HasKey("repeated_enum_field") && node["repeated_enum_field"].IsList());
            UNIT_ASSERT(node["repeated_enum_field"].AsList().size() == 2);
            UNIT_ASSERT_EQUAL(node["repeated_enum_field"].AsList()[0].AsString(), "FIRST");
            UNIT_ASSERT_EQUAL(node["repeated_enum_field"].AsList()[1].AsString(), "SECOND");
        }

        {
            auto node = ProtoToYtNode(proto, TParseConfig().SetSaveEnumsAsString(true).SetUseImplicitDefault(true));
            UNIT_ASSERT(node.HasKey("string_enum_field") && node["string_enum_field"].IsString());
            UNIT_ASSERT_EQUAL(node["string_enum_field"].AsString(), "FIRST");
            UNIT_ASSERT(node.HasKey("number_enum_field") && node["number_enum_field"].IsString());
            UNIT_ASSERT_EQUAL(node["number_enum_field"].AsString(), "SECOND");
            UNIT_ASSERT(node.HasKey("missing_enum_field") && node["missing_enum_field"].IsString());
            UNIT_ASSERT_EQUAL(node["missing_enum_field"].AsString(), "FIRST");
            UNIT_ASSERT(node.HasKey("repeated_enum_field") && node["repeated_enum_field"].IsList());
            UNIT_ASSERT(node["repeated_enum_field"].AsList().size() == 2);
            UNIT_ASSERT_EQUAL(node["repeated_enum_field"].AsList()[0].AsString(), "FIRST");
            UNIT_ASSERT_EQUAL(node["repeated_enum_field"].AsList()[1].AsString(), "SECOND");
        }
    }

}
