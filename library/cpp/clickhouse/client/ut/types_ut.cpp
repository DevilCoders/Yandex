#include <library/cpp/clickhouse/client/types/types.h>
#include <library/cpp/testing/unittest/registar.h>

using namespace NClickHouse;

Y_UNIT_TEST_SUITE(TTypesTest){
    Y_UNIT_TEST(TypeName){
        UNIT_ASSERT_EQUAL(
            TType::CreateDate()->GetName(),
            "Date");

UNIT_ASSERT_EQUAL(
    TType::CreateArray(TType::CreateSimple<i32>())->GetName(),
    "Array(Int32)");

UNIT_ASSERT_EQUAL(
    TType::CreateNullable(TType::CreateSimple<i32>())->GetName(),
    "Nullable(Int32)");

UNIT_ASSERT_EQUAL(
    TType::CreateArray(TType::CreateSimple<i32>())->GetItemType()->GetCode(),
    TType::Int32);

UNIT_ASSERT_EQUAL(
    TType::CreateTuple({TType::CreateSimple<i32>(),
                        TType::CreateString()})
        ->GetName(),
    "Tuple(Int32, String)");
}

Y_UNIT_TEST(EnumTypes) {
    UNIT_ASSERT_EXCEPTION(TType::CreateEnum8({{"Big", 1000}}), yexception);

    TTypeRef enum8 = TType::CreateEnum8({{"One", 1}, {"Two", 2}});
    UNIT_ASSERT_EQUAL(enum8->GetName(), "Enum8('One' = 1, 'Two' = 2)");
    UNIT_ASSERT(enum8->HasEnumValue(1));
    UNIT_ASSERT(enum8->HasEnumName("Two"));
    UNIT_ASSERT(!enum8->HasEnumValue(10));
    UNIT_ASSERT(!enum8->HasEnumName("Ten"));
    UNIT_ASSERT_EQUAL(enum8->GetEnumName(2), "Two");
    UNIT_ASSERT_EQUAL(enum8->GetEnumValue("Two"), 2);

    TTypeRef enum16 = TType::CreateEnum16({{"Green", 1}, {"Red", 2}, {"Yellow", 3}});
    UNIT_ASSERT_EQUAL(enum16->GetName(), "Enum16('Green' = 1, 'Red' = 2, 'Yellow' = 3)");
    UNIT_ASSERT(enum16->HasEnumValue(3));
    UNIT_ASSERT(enum16->HasEnumName("Green"));
    UNIT_ASSERT(!enum16->HasEnumValue(10));
    UNIT_ASSERT(!enum16->HasEnumName("Black"));
    UNIT_ASSERT_EQUAL(enum16->GetEnumName(2), "Red");
    UNIT_ASSERT_EQUAL(enum16->GetEnumValue("Green"), 1);
}
}
;
