#include "enums.h"

#include <kernel/text_machine/parts/common/static_table.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NTextMachine::NCore;

Y_UNIT_TEST_SUITE(StaticTableTest) {
    Y_UNIT_TEST(TestEmpty) {
        TStaticTable<EKeys, float> table;

        NJson::TJsonValue value;
        table.SaveToJson(value);
        Cdbg << "JSON = " << value << Endl;
        UNIT_ASSERT_EQUAL(ToString(value), "{}");
    }

    Y_UNIT_TEST(TestSingle) {
        TStaticTable<EKeys, float, EKeys::One> table(11.f);
        UNIT_ASSERT_EQUAL(table[table.MakeKey<EKeys::One>()], 11.f);
        table[table.MakeKey<EKeys::One>()] = 12.f;
        UNIT_ASSERT_EQUAL(table[table.MakeKey<EKeys::One>()], 12.f);

        NJson::TJsonValue value;
        table.SaveToJson(value);
        Cdbg << "JSON = " << value << Endl;
        UNIT_ASSERT_EQUAL(ToString(value), "{\"One\":12}");
    }

    Y_UNIT_TEST(TestMultiple) {
        TStaticTable<int, float, 1, 4, 7> table(10.f);
        UNIT_ASSERT_EQUAL(table[table.MakeKey<1>()], 10.f);
        UNIT_ASSERT_EQUAL(table[table.MakeKey<4>()], 10.f);
        UNIT_ASSERT_EQUAL(table[table.MakeKey<7>()], 10.f);
        table.Fill(11.f);
        UNIT_ASSERT_EQUAL(table[table.MakeKey<1>()], 11.f);
        UNIT_ASSERT_EQUAL(table[table.MakeKey<4>()], 11.f);
        UNIT_ASSERT_EQUAL(table[table.MakeKey<7>()], 11.f);
        table[table.MakeKey<4>()] = 12.f;
        UNIT_ASSERT_EQUAL(table[table.MakeKey<4>()], 12.f);

        NJson::TJsonValue value;
        table.SaveToJson(value);
        Cdbg << "JSON = " << value << Endl;
        UNIT_ASSERT_EQUAL(ToString(value), "{\"1\":11,\"7\":11,\"4\":12}");
    }
};

