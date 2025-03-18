#include <library/cpp/testing/unittest/registar.h>

#include "stats_output.h"

#include <library/cpp/json/json_reader.h>

#include <util/generic/strbuf.h>
#include <util/string/join.h>

namespace NAntiRobot {
using namespace NJson;

Y_UNIT_TEST_SUITE(StatsOutputJson) {

Y_UNIT_TEST(AddValue) {
    TStringStream jsonStream;
    {
        TStatsJsonOutput out(jsonStream);
        out.AddScalar("a", 1);
    }
    UNIT_ASSERT_VALUES_EQUAL(jsonStream.Str(), R"([["a_deee", 1]])");

    jsonStream.Clear();
    {
        TStatsJsonOutput out(jsonStream);
        out.AddScalar("a", 1);
        out.AddHistogram("b", 2);
    }
    UNIT_ASSERT_VALUES_EQUAL(jsonStream.Str(), R"([["a_deee", 1],["b_ahhh", 2]])");
}

Y_UNIT_TEST(StartGroupSimple) {
    TStringStream jsonStream;
    {
        TStatsJsonOutput out(jsonStream);

        out.AddScalar("a", 1);

        out.StartGroup("g")
           ->AddScalar("f_1", 1)
            .AddScalar("f_2", 2);

        out.AddScalar("b", 2);
    }
    UNIT_ASSERT_VALUES_EQUAL(jsonStream.Str(), R"([["a_deee", 1],["g.f_1_deee", 1],["g.f_2_deee", 2],["b_deee", 2]])");
}

Y_UNIT_TEST(StartGroupWithKeyValue) {
    TStringStream jsonStream;
    {
        TStatsJsonOutput out(jsonStream);

        out.AddScalar("a", 1);

        out.StartGroup("g", {{TStringBuf("id"), {TStringBuf("8")}}})
           ->AddScalar("f_1", 1)
            .AddScalar("f_2", 2);

        out.AddScalar("b", 2);
    }
    UNIT_ASSERT_VALUES_EQUAL(jsonStream.Str(), R"([["a_deee", 1],["g_id_8.f_1_deee", 1],["g_id_8.f_2_deee", 2],["b_deee", 2]])");
}

Y_UNIT_TEST(InsertedGroups) {
    TStringStream jsonStream;
    {
       TStatsJsonOutput out(jsonStream);
       out.StartGroup("m")->StartGroup("n", {{TStringBuf("id"), {TStringBuf("8")}}})->AddScalar("v", 1);
    }
    UNIT_ASSERT_VALUES_EQUAL(jsonStream.Str(), R"([["m.n_id_8.v_deee", 1]])");
}

Y_UNIT_TEST(ProducesCorrectJson) {
    TStringStream jsonStream;
    {
        TStatsJsonOutput out(jsonStream);

        out.AddScalar("a", 1);

        out.StartGroup("g", {{TStringBuf("id"), {TStringBuf("8")}}})
           ->AddScalar("f_1", 1)
            .AddScalar("f_2", 2);

        out.AddScalar("b", 2);

        out.StartGroup("d")
           ->AddScalar("k_1", 1)
            .AddHistogram("k_2", 2);

        out.AddScalar("c", 3);
    }

    TStringStream in;
    in << R"({"temp_key": )" << jsonStream.Str() << "}";
    TJsonCallbacks temp;
    UNIT_ASSERT(ReadJson(&in, &temp));
}

}

}
