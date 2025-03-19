#include <kernel/feature_pool/feature_pool.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/xrange.h>

#include <google/protobuf/text_format.h>

using namespace NMLPool;

static TPoolInfo GetPoolInfo() {
    static const char* text =
        "FeatureInfo {\n"
        "  Slice: \"web_production_x\"\n"
        "  Name: \"PR\"\n"
        "  Type: FT_FLOAT\n"
        "  Tags: \"TG_UNUSED\"\n"
        "  Tags: \"TG_STATIC\"\n"
        "  Tags: \"TG_LINK_GRAPH\"\n"
        "  Tags: \"TG_L2\"\n"
        "  Tags: \"TG_DOC\"\n"
        "}\n"
        "FeatureInfo {\n"
        "  Slice: \"web_production_x\"\n"
        "  Name: \"TR\"\n"
        "  Type: FT_FLOAT\n"
        "  Tags: \"TG_UNDOCUMENTED\"\n"
        "  Tags: \"TG_DYNAMIC\"\n"
        "  Tags: \"TG_DOC_TEXT\"\n"
        "  Tags: \"TG_REARR_USE\"\n"
        "  Tags: \"TG_DOC\"\n"
        "}\n"
        "FeatureInfo {\n"
        "  Slice: \"web_production_y\"\n"
        "  Name: \"LR\"\n"
        "  Type: FT_FLOAT\n"
        "  Tags: \"TG_UNDOCUMENTED\"\n"
        "  Tags: \"TG_DYNAMIC\"\n"
        "  Tags: \"TG_DOC\"\n"
        "  Tags: \"TG_LINK_TEXT\"\n"
        "}\n"
        "FeatureInfo {\n"
        "  Slice: \"web_production_y\"\n"
        "  Name: \"PrBonus\"\n"
        "  Type: FT_FLOAT\n"
        "  Tags: \"TG_BINARY\"\n"
        "  Tags: \"TG_DYNAMIC\"\n"
        "  Tags: \"TG_DOC_TEXT\"\n"
        "  Tags: \"TG_OFTEN_ZERO\"\n"
        "  Tags: \"TG_DOC\"\n"
        "}\n"
        "FeatureInfo {\n"
        "  Slice: \"web_production_y\"\n"
        "  Name: \"TRp1\"\n"
        "  Type: FT_FLOAT\n"
        "  Tags: \"TG_BINARY\"\n"
        "  Tags: \"TG_UNDOCUMENTED\"\n"
        "  Tags: \"TG_DYNAMIC\"\n"
        "  Tags: \"TG_DOC_TEXT\"\n"
        "  Tags: \"TG_OFTEN_ZERO\"\n"
        "  Tags: \"TG_REARR_USE\"\n"
        "  Tags: \"TG_DOC\"\n"
        "  ExtJson: \"{\\\"Index\\\": 4, \\\"Group\\\": [\\\"LegacyTR\\\"], \\\"Description\\\": \\\"\u041f\u0440\u0438\u043e\u0440\u0438\u0442\u0435\u0442 strict \u0434\u043b\u044f TR - \u0442\u0435\u043a\u0441\u0442\u043e\u0432\u044b\u0439 \u043f\u0440\u0438\u043e\u0440\u0438\u0442\u0435\u0442 - \u0435\u0441\u0442\u044c \u0432\u0441\u0435 \u0441\u043b\u043e\u0432\u0430 \u0437\u0430\u043f\u0440\u043e\u0441\u0430 \u0433\u0434\u0435-\u0442\u043e \u0432 \u0434\u043e\u043a\u0443\u043c\u0435\u043d\u0442\u0435 (\u043f\u0440\u0438 \u044d\u0442\u043e\u043c \u043e\u043d\u0438 \u043f\u0440\u043e\u0445\u043e\u0434\u044f\u0442 \u043a\u043e\u043d\u0442\u0435\u043a\u0441\u0442\u043d\u044b\u0435 \u043e\u0433\u0440\u0430\u043d\u0438\u0447\u0435\u043d\u0438\u044f \u0437\u0430\u043f\u0440\u043e\u0441\u0430, \u043d\u0430\u043f\u0440\u0438\u043c\u0435\u0440, \u043e\u0431\u0430 \u0441\u043b\u043e\u0432\u0430 \u0434.\u0431. \u0432 \u043e\u0434\u043d\u043e\u043c \u043f\u0440\u0435\u0434\u043b\u043e\u0436\u0435\u043d\u0438\u0438).\\\", \\\"Tags\\\": [20, 70, 1, 101, 157, 159, 151], \\\"CppName\\\": \\\"FI_TEXT_RELEV_ALL_WORDS\\\", \\\"Authors\\\": [\\\"denplusplus\\\", \\\"gulin\\\", \\\"leo\\\"], \\\"Responsibles\\\": [\\\"alsafr\\\", \\\"gulin\\\", \\\"leo\\\"], \\\"Name\\\": \\\"TRp1\\\"}\"\n"
        "}\n";
    TPoolInfo info;
    Y_VERIFY(::google::protobuf::TextFormat::ParseFromString(TString(text), &info));
    return info;
}

Y_UNIT_TEST_SUITE(FeaturePoolTest) {
    Y_UNIT_TEST(TestBordersStr) {
        UNIT_ASSERT_EQUAL(GetBordersStr(TPoolInfo()), "");
        UNIT_ASSERT_EQUAL(GetBordersStr(GetPoolInfo()), "web_production_x[0;2) web_production_y[2;5)");
    }

    Y_UNIT_TEST(TestParseBordersStr) {
        TFeatureSlices slices;
        UNIT_ASSERT(TryParseBordersStr("", slices));
        UNIT_ASSERT(TryParseBordersStr(" \t\n", slices));
        UNIT_ASSERT(TryParseBordersStr(" \t\nx[0;1) \t\n", slices));
        UNIT_ASSERT(TryParseBordersStr(" \t\nx[0;1) \t\ny[0;1) \t\n", slices));
    }

    Y_UNIT_TEST(TestToJson) {
        TFeatureInfo info = GetPoolInfo().GetFeatureInfo(4);
        auto value = ToJson(info);
        UNIT_ASSERT_EQUAL(value["Name"], NJson::TJsonValue("TRp1"));
        UNIT_ASSERT(!value.Has("ExtJson"));
        value = ToJson(info, true);
        UNIT_ASSERT(value.Has("ExtJson"));
        UNIT_ASSERT_EQUAL(value["ExtJson"]["Index"], NJson::TJsonValue(4));
    }
}
