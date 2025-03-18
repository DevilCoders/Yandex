#include <library/cpp/clickhouse/client/columns/array.h>
#include <library/cpp/clickhouse/client/columns/date.h>
#include <library/cpp/clickhouse/client/columns/enum.h>
#include <library/cpp/clickhouse/client/columns/numeric.h>
#include <library/cpp/clickhouse/client/columns/string.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NClickHouse;

static TVector<ui32> MakeNumbers() {
    return TVector<ui32>{1, 2, 3, 7, 11, 13, 17, 19, 23, 29, 31};
}

static TVector<TString> MakeFixedStrings() {
    return TVector<TString>{"aaa", "bbb", "ccc", "ddd"};
}

static TVector<TString> MakeStrings() {
    return TVector<TString>{"a", "ab", "abc", "abcd"};
}

static TVector<TInstant> MakeTimeInstants() {
    TInstant t1 = TInstant();
    TInstant t2 = t1 + TDuration::MilliSeconds(10000000);
    TInstant t3 = t2 + TDuration::MilliSeconds(50000000);
    return {t1, t2, t3};
}

Y_UNIT_TEST_SUITE(TColumnsTest){
    Y_UNIT_TEST(NumericInit){
        auto col = TColumnUInt32::Create(MakeNumbers());

UNIT_ASSERT_EQUAL(col->Size(), 11u);
UNIT_ASSERT_EQUAL(col->At(3), 7u);
UNIT_ASSERT_EQUAL(col->At(10), 31u);
}

Y_UNIT_TEST(NumericSlice) {
    auto col = TColumnUInt32::Create(MakeNumbers());
    auto sub = col->Slice(3, 3)->As<TColumnUInt32>();

    UNIT_ASSERT_EQUAL(sub->Size(), 3u);
    UNIT_ASSERT_EQUAL(sub->At(0), 7u);
    UNIT_ASSERT_EQUAL(sub->At(2), 13u);
}

Y_UNIT_TEST(FixedStringInit) {
    auto col = TColumnFixedString::Create(3);
    for (const auto& s : MakeFixedStrings()) {
        col->Append(s);
    }

    UNIT_ASSERT_EQUAL(col->Size(), 4u);
    UNIT_ASSERT_EQUAL(col->At(1), "bbb");
    UNIT_ASSERT_EQUAL(col->At(3), "ddd");

    auto colFromData = TColumnFixedString::Create(3, MakeFixedStrings());
    UNIT_ASSERT_EQUAL(colFromData->Size(), col->Size());
    for (size_t i = 0; i < colFromData->Size(); ++i) {
        UNIT_ASSERT_EQUAL(colFromData->At(i), col->At(i));
    }
}

Y_UNIT_TEST(StringInit) {
    auto col = TColumnString::Create(MakeStrings());

    UNIT_ASSERT_EQUAL(col->Size(), 4u);
    UNIT_ASSERT_EQUAL(col->At(1), "ab");
    UNIT_ASSERT_EQUAL(col->At(3), "abcd");
}

Y_UNIT_TEST(ArrayAppend) {
    auto arr1 = TColumnArray::Create(TColumnUInt64::Create());
    auto arr2 = TColumnArray::Create(TColumnUInt64::Create());

    auto id = TColumnUInt64::Create();
    id->Append(1);
    arr1->AppendAsColumn(id);

    id->Append(3);
    arr2->AppendAsColumn(id);

    arr1->Append(arr2);

    auto col = arr1->GetAsColumn(1);

    UNIT_ASSERT_EQUAL(arr1->Size(), 2u);
    UNIT_ASSERT_EQUAL(col->As<TColumnUInt64>()->At(0), 1u);
    UNIT_ASSERT_EQUAL(col->As<TColumnUInt64>()->At(1), 3u);
}

Y_UNIT_TEST(DateAppend) {
    auto col1 = TColumnDate::Create();
    auto col2 = TColumnDate::Create();
    auto now = std::time(nullptr);

    col1->Append(TInstant::Seconds(now));
    col2->Append(col1);

    UNIT_ASSERT_EQUAL(col2->Size(), 1u);
    UNIT_ASSERT_EQUAL(col2->At(0), (now / 86400) * 86400);
}

Y_UNIT_TEST(DateInit) {
    auto timeInstants = MakeTimeInstants();

    auto col = TColumnDate::Create();
    for (const auto& value : timeInstants) {
        col->Append(value);
    }

    UNIT_ASSERT_EQUAL(col->Size(), timeInstants.size());
    for (size_t i = 0; i < col->Size(); ++i) {
        UNIT_ASSERT_EQUAL(col->At(i), (time_t)(timeInstants[i].Seconds() / 86400) * 86400);
    }

    auto colFromData = TColumnDate::Create(timeInstants);
    UNIT_ASSERT_EQUAL(colFromData->Size(), col->Size());
    for (size_t i = 0; i < colFromData->Size(); ++i) {
        UNIT_ASSERT_EQUAL(colFromData->At(i), col->At(i));
    }
}

Y_UNIT_TEST(DateTimeInit) {
    auto timeInstants = MakeTimeInstants();

    auto col = TColumnDateTime::Create();
    for (const auto& value : timeInstants) {
        col->Append(value);
    }

    UNIT_ASSERT_EQUAL(col->Size(), timeInstants.size());
    for (size_t i = 0; i < col->Size(); ++i) {
        UNIT_ASSERT_EQUAL(col->At(i), (time_t)timeInstants[i].Seconds());
    }

    auto colFromData = TColumnDateTime::Create(timeInstants);
    UNIT_ASSERT_EQUAL(colFromData->Size(), col->Size());
    for (size_t i = 0; i < colFromData->Size(); ++i) {
        UNIT_ASSERT_EQUAL(colFromData->At(i), col->At(i));
    }
}

Y_UNIT_TEST(ColumnEnumTest) {
    TVector<TEnumItem> enumItems = {{"Hi", 1}, {"Hello", 2}};

    auto col = TColumnEnum8::Create(enumItems);
    UNIT_ASSERT(col->Type()->IsEqual(TType::CreateEnum8(enumItems)));

    col->Append(1);
    UNIT_ASSERT_EQUAL(col->Size(), 1);
    UNIT_ASSERT_EQUAL(col->At(0), 1);
    UNIT_ASSERT_EQUAL(col->NameAt(0), "Hi");

    col->Append("Hello");
    UNIT_ASSERT_EQUAL(col->Size(), 2);
    UNIT_ASSERT_EQUAL(col->At(1), 2);
    UNIT_ASSERT_EQUAL(col->NameAt(1), "Hello");

    UNIT_ASSERT_EXCEPTION(col->Append(10, true), yexception);

    auto colFromValues = TColumnEnum8::Create(enumItems, {2, 1, 2}, true);
    UNIT_ASSERT_EQUAL(colFromValues->Size(), 3);
    UNIT_ASSERT_EQUAL(colFromValues->At(0), 2);
    UNIT_ASSERT_EQUAL(colFromValues->At(1), 1);
    UNIT_ASSERT_EQUAL(colFromValues->At(2), 2);

    auto colFromNames = TColumnEnum8::Create(enumItems, {"Hi", "Hello", "Hello"});
    UNIT_ASSERT_EQUAL(colFromNames->Size(), 3);
    UNIT_ASSERT_EQUAL(colFromNames->At(0), 1);
    UNIT_ASSERT_EQUAL(colFromNames->At(1), 2);
    UNIT_ASSERT_EQUAL(colFromNames->At(2), 2);

    UNIT_ASSERT_EXCEPTION(TColumnEnum8::Create(enumItems, {3, 4}, true), yexception);

    auto col16 = TColumnEnum16::Create(enumItems);
    UNIT_ASSERT(col16->Type()->IsEqual(TType::CreateEnum16(enumItems)));
}

Y_UNIT_TEST(TestMutation) {
    auto colNumeric = TColumnInt32::Create({1, 2, 3});
    colNumeric->SetAt(2, 10);
    UNIT_ASSERT_EQUAL(colNumeric->At(2), 10);

    TVector<TEnumItem> enumItems = {{"Hi", 1}, {"Hello", 2}};
    auto colEnum = TColumnEnum8::Create(enumItems, {1, 1, 1});

    colEnum->SetAt(0, 2);
    UNIT_ASSERT_EQUAL(colEnum->At(0), 2);
    UNIT_ASSERT_EQUAL(colEnum->NameAt(0), "Hello");

    colEnum->SetNameAt(1, "Hello");
    UNIT_ASSERT_EQUAL(colEnum->At(1), 2);
    UNIT_ASSERT_EQUAL(colEnum->NameAt(1), "Hello");

    UNIT_ASSERT_EXCEPTION(colEnum->SetAt(1, 10, true), yexception);

    auto colDate = TColumnDateTime::Create(MakeTimeInstants());
    TInstant t = TInstant::MicroSeconds(1000);
    colDate->SetAt(2, t);
    UNIT_ASSERT_EQUAL(colDate->At(2), (time_t)(t.Seconds() / 86400) * 86400);

    auto colDateTime = TColumnDateTime::Create(MakeTimeInstants());
    colDateTime->SetAt(2, t);
    UNIT_ASSERT_EQUAL(colDateTime->At(2), (time_t)t.Seconds());

    auto colString = TColumnString::Create(MakeStrings());
    colString->SetAt(1, "New");
    UNIT_ASSERT_EQUAL(colString->At(1), "New");

    auto colFixedString = TColumnFixedString::Create(5, MakeStrings());
    colFixedString->SetAt(1, "New");
    UNIT_ASSERT_EQUAL(colFixedString->At(1), "New  ");
}
}
;
