#include <library/cpp/testing/unittest/registar.h>

#include "stats_writer.h"

#include <library/cpp/json/json_reader.h>

#include <util/generic/strbuf.h>
#include <util/string/join.h>

namespace NAntiRobot {
using namespace NJson;

Y_UNIT_TEST_SUITE(StatsWriterJson) {

Y_UNIT_TEST(AddValue) {
    TStringStream jsonStream;
    {
        TStatsWriter out(&jsonStream);
        out.WriteScalar("a", 1);
    }
    UNIT_ASSERT_VALUES_EQUAL(jsonStream.Str(), R"([["a_deee",1]])");

    jsonStream.Clear();
    {
        TStatsWriter out(&jsonStream);
        out.WriteScalar("a", 1);
        out.WriteHistogram("b", 2);
    }
    UNIT_ASSERT_VALUES_EQUAL(jsonStream.Str(), R"([["a_deee",1],["b_ahhh",2]])");
}

Y_UNIT_TEST(StartGroupSimple) {
    TStringStream jsonStream;
    {
        TStatsWriter out(&jsonStream);

        out.WriteScalar("a", 1);

        out.WithTag("g", "x")
            .WriteScalar("f_1", 1)
            .WriteScalar("f_2", 2);

        out.WriteScalar("b", 2);
    }
    UNIT_ASSERT_VALUES_EQUAL(jsonStream.Str(), R"([["a_deee",1],["g=x;f_1_deee",1],["g=x;f_2_deee",2],["b_deee",2]])");
}

Y_UNIT_TEST(InsertedGroups) {
    TStringStream jsonStream;
    {
       TStatsWriter out(&jsonStream);
       out.WithTag("m", "x").WithTag("n", "y").WriteScalar("v", 1);
    }
    UNIT_ASSERT_VALUES_EQUAL(jsonStream.Str(), R"([["m=x;n=y;v_deee",1]])");
}

Y_UNIT_TEST(ProducesCorrectJson) {
    TStringStream jsonStream;
    {
        TStatsWriter out(&jsonStream);

        out.WriteScalar("a", 1);

        out.WithTag("g", "x")
            .WriteScalar("f_1", 1)
            .WriteScalar("f_2", 2);

        out.WriteScalar("b", 2);

        out.WithTag("d", "y")
            .WriteScalar("k_1", 1)
            .WriteHistogram("k_2", 2);

        out.WriteScalar("c", 3);
    }

    TStringStream in;
    in << R"({"temp_key": )" << jsonStream.Str() << "}";
    TJsonCallbacks temp;
    UNIT_ASSERT(ReadJson(&in, &temp));
}

}

}
