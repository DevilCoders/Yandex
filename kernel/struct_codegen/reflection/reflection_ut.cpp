#include "reflection.h"

#include <kernel/struct_codegen/metadata/metadata.pb.h>
#include <google/protobuf/text_format.h>
#include <library/cpp/testing/unittest/registar.h>

using NErf::TStaticFieldDescriptor;
using NErf::TStaticRecordDescriptor;
using NErf::MakeFieldDescriptor;

#pragma pack(push, 1)

struct S1 {
    ui32 A:16;
    ui32 B:11;
    ui32 C:5;
    ui32 D:32;
};
static_assert(sizeof(S1) == 8, "expect sizeof(S1) == 8");

const TStaticFieldDescriptor S1_FIELDS[] = {
    MakeFieldDescriptor(0, 16, "A", nullptr, nullptr),
    MakeFieldDescriptor(16, 11, "B", nullptr, nullptr),
    MakeFieldDescriptor(27, 5, "C", nullptr, nullptr),
    MakeFieldDescriptor(32, 32, "D", nullptr, nullptr),
};

const TStaticRecordDescriptor S1_DESCR("S1", 1, 8, S1_FIELDS, Y_ARRAY_SIZE(S1_FIELDS));

/*
const char* S1TextDescr =
    "Name: 'S1' "
    "Size: 8 "
    "Field { Name: 'A' Width: 16 } "
    "Field { Name: 'B' Width: 11 } "
    "Field { Name: 'C' Width: 5 } "
    "Field { Name: 'D' Width: 32 }";
*/

const TStaticFieldDescriptor S1_RENAMED_FIELDS[] = {
    MakeFieldDescriptor(0, 16, "A", nullptr, nullptr),
    MakeFieldDescriptor(16, 11, "B", nullptr, nullptr),
    MakeFieldDescriptor(27, 5, "X", nullptr, nullptr),
    MakeFieldDescriptor(32, 32, "D", nullptr, nullptr),
};

const TStaticRecordDescriptor S1_RENAMED_DESCR("S1", 1, 8, S1_RENAMED_FIELDS, Y_ARRAY_SIZE(S1_RENAMED_FIELDS));

const TStaticFieldDescriptor S1_RENAMED_FIELDS_WITH_PREV_NAME[] = {
    MakeFieldDescriptor(0, 16, "A", nullptr, nullptr),
    MakeFieldDescriptor(16, 11, "B", nullptr, nullptr),
    MakeFieldDescriptor(27, 5, "X", nullptr, "C"),
    MakeFieldDescriptor(32, 32, "D", nullptr, nullptr),
};

const TStaticRecordDescriptor S1_RENAMED_DESCR_WITH_PREV_NAME("S1", 1, 8, S1_RENAMED_FIELDS_WITH_PREV_NAME, Y_ARRAY_SIZE(S1_RENAMED_FIELDS_WITH_PREV_NAME));

struct S2 {
    ui32 E:16;
    ui32 A:16;
    ui32 D:32;
    float F;
    ui32 B:11;
    ui32 _:16;
    ui32 C:5;
};
static_assert(sizeof(S2) == 16, "expect sizeof(S2) == 16");

const TStaticFieldDescriptor S2_FIELDS[] = {
    MakeFieldDescriptor(0, 16, "E", nullptr, nullptr),
    MakeFieldDescriptor(16, 16, "A", nullptr, nullptr),
    MakeFieldDescriptor(32, 32, "D", nullptr, nullptr),
    MakeFieldDescriptor(64, 32, "F", "float", nullptr),
    MakeFieldDescriptor(96, 11, "B", nullptr, nullptr),
    MakeFieldDescriptor(123, 5, "C", nullptr, nullptr),
};

const TStaticRecordDescriptor S2_DESCR("S2", 1, 16, S2_FIELDS, Y_ARRAY_SIZE(S2_FIELDS));

/*
const char* S2TextDescr =
    "Name: 'S2' "
    "Size: 16 "
    "Field { Name: 'E' Width: 16 } "
    "Field { Name: 'A' Width: 16 } "
    "Field { Name: 'D' Width: 32 } "
    "Field { Name: 'F' Width: 32 Type: 'float' } "
    "Field { Name: 'B' Width: 11 } "
    "Field { Width: 16 } "
    "Field { Name: 'C' Width: 5 } ";
*/

#pragma pack(pop)

class TErfReflectionTest : public TTestBase {
    UNIT_TEST_SUITE(TErfReflectionTest);
        UNIT_TEST(Test)
        UNIT_TEST(TestRename)
    UNIT_TEST_SUITE_END();

    void Test() {
        NErf::TRecordReflection r1(S1_DESCR);
        NErf::TRecordReflection r2(S2_DESCR);

        UNIT_ASSERT(r1.GetFieldCount() == 4);
        UNIT_ASSERT(r1.GetFieldIndex("C") == 2);
        UNIT_ASSERT(TStringBuf(r1.GetFieldName(2)) == "C");
        UNIT_ASSERT(r1.GetFieldWidth(2) == 5);
        UNIT_ASSERT(r1.GetFieldOffset(2) == 16+11);
        UNIT_ASSERT(r1.GetRecordSize() == 8);

        UNIT_ASSERT(r2.GetFieldCount() == 6);
        UNIT_ASSERT(r2.GetFieldIndex("C") == 5);
        UNIT_ASSERT(r2.GetFieldOffset(5) == 16*8-5);
        UNIT_ASSERT(!r2.IsBitField(3));
        UNIT_ASSERT(r2.GetCppFieldSize(3) == sizeof(float));
        UNIT_ASSERT(TStringBuf(r2.GetFieldType(3)) == "float");
        UNIT_ASSERT(r2.GetRecordSize() == 16);

        TVector<int> fieldMap = NErf::MatchFieldsByName(r2, r1);
        for (size_t i = 0; i < r2.GetFieldCount(); i++) {
            if (r2.GetFieldName(i)[0] <= 'D') {
                UNIT_ASSERT(fieldMap[i] >= 0 && r2.GetFieldName(i) == r1.GetFieldName(fieldMap[i]));
            } else {
                UNIT_ASSERT(fieldMap[i] == -1);
            }
        }

        NErf::TRecordTranslator translator(r2, r1, fieldMap);

        for (ui32 a = 0; a < 65536; a += 22222)
        for (ui32 b = 0; b < 2048; b += 888)
        for (ui32 c = 0; c < 32; c += 11)
        for (ui32 d = 0; d < 1000000000; d += 333333333) {
            S1 s1 = { a, b, c, d };
            S2 s2 = { 1, 2, 3, 42.0f, 4, 0, 5 };

            const void* p1 = &s1;
            const void* p2 = &s2;

            UNIT_ASSERT(r1.GetBitField(p1, 2) == c);
            r1.SetBitField((void *)p1, 2, 0xFFFF);  // should be truncated to 5 bits
            UNIT_ASSERT(r1.GetBitField(p1, 2) == 31);
            r1.SetBitField((void *)p1, 2, c);

            UNIT_ASSERT(r1.GetBitField(p1, 0) == a);
            UNIT_ASSERT(r1.GetBitField(p1, 1) == b);
            UNIT_ASSERT(r1.GetBitField(p1, 2) == c);
            UNIT_ASSERT(r1.GetBitField(p1, 3) == d);

            UNIT_ASSERT(r2.GetBitField(p2, 0) == 1);
            UNIT_ASSERT(r2.GetBitField(p2, 1) == 2);
            UNIT_ASSERT(r2.GetBitField(p2, 2) == 3);
            UNIT_ASSERT(r2.GetCppFieldPtr(p2, 3) == (void *)&s2.F);
            UNIT_ASSERT(r2.GetCppField<float>(p2, 3) == 42.0f);
            UNIT_ASSERT(r2.GetBitField(p2, 4) == 4);
            UNIT_ASSERT(r2.GetBitField(p2, 5) == 5);

            translator.Translate((void *)p2, p1);

            UNIT_ASSERT(r2.GetBitField(p2, 0) == 1);
            UNIT_ASSERT(r2.GetBitField(p2, 1) == a);
            UNIT_ASSERT(r2.GetBitField(p2, 2) == d);
            UNIT_ASSERT(r2.GetCppField<float>(p2, 3) == 42.0f);
            UNIT_ASSERT(r2.GetBitField(p2, 4) == b);
            UNIT_ASSERT(r2.GetBitField(p2, 5) == c);
        }
    }

    void TestRename() {
        NErf::TRecordReflection r1(S1_DESCR);

        {
            NErf::TRecordReflection r2(S1_RENAMED_DESCR);

            TVector<int> fieldMap = NErf::MatchFieldsByName(r2, r1);
            for (int i = 0; i < (int)r2.GetFieldCount(); i++)
                UNIT_ASSERT(i == 2 || fieldMap[i] == i);
            UNIT_ASSERT(fieldMap[2] == -1);

            UNIT_ASSERT(NErf::MatchFieldsByName(r1, r2) == fieldMap);
        }

        {
            NErf::TRecordReflection r2(S1_RENAMED_DESCR_WITH_PREV_NAME);

            TVector<int> fieldMap = NErf::MatchFieldsByName(r2, r1);
            for (int i = 0; i < (int)r2.GetFieldCount(); i++)
                UNIT_ASSERT(fieldMap[i] == i);

            UNIT_ASSERT(NErf::MatchFieldsByName(r1, r2) == fieldMap);
        }
    }
};

UNIT_TEST_SUITE_REGISTRATION(TErfReflectionTest);
