#include "regexp_classifier.h"
#include <library/cpp/testing/unittest/registar.h>

Y_UNIT_TEST_SUITE(TTestRegexpClassifier) {
    Y_UNIT_TEST(TestTrivial) {
        enum class EType {
            unknown,
            typ1,
            typ2,
            typ3
        };

        const TRegexpClassifier<EType> classify(
            {
                {"1", EType::typ1},
                {"2", EType::typ2},
                {"3", EType::typ3},
            },
            EType::unknown);

        UNIT_ASSERT_EQUAL(classify["1"], EType::typ1);
        UNIT_ASSERT_EQUAL(classify["2"], EType::typ2);
        UNIT_ASSERT_EQUAL(classify["3"], EType::typ3);
    }

    Y_UNIT_TEST(TestMultiScanner) {
        enum class EType {
            unknown,
            typ1,
            typ2,
            typ3
        };

        const TRegexpClassifier<EType> classify(
            {{"@@@1", EType::typ1},
             {".*aaajkljklj*", EType::typ1},
             {".*abb.*", EType::typ1},
             {".*abbklklkl.*", EType::typ1},
             {".*abbjhkjhjkh*.", EType::typ1},
             {".*abcdef.*", EType::typ1},
             {".*abcdefg.*", EType::typ1},
             {".*abcdefgh.*", EType::typ1},
             {".*abcdefghi.*", EType::typ1},
             {".*abcdefghij.*", EType::typ1},
             {".*abcdefghijk.*", EType::typ1},
             {".*abcdefghijkl.*", EType::typ1},
             {".*abcdefghijklm.*", EType::typ1},
             {".*abcdefghijklmn.*", EType::typ1},
             {".*abcdefghijklmno.*", EType::typ1},
             {".*abcdefghijklmnop.*", EType::typ1},
             {".*abcdefghijklmnopq.*", EType::typ1},
             {".*abcdefghijklmnopqr.*", EType::typ1},
             {".*abcdefghijklmnopqrs.*", EType::typ1},
             {".*abcdefghijklmnopqrst.*", EType::typ1},
             {".*abjkljkljlkjlkjlkjstu.*", EType::typ1},
             {".*abcdefghijklmnopqrstuv.*", EType::typ1},
             {".*abcdgdfgfgdfgdfgdrstuvw.*", EType::typ1},
             {".*abcdefdfgdfgdfgdfgstuvwx.*", EType::typ1},
             {".*abcdefghijklmnopqrstuvwxy.*", EType::typ1},
             {".*abcdesfgsdfgsdfgsdfgdfgxyz.*", EType::typ1},
             {".*abcdegdsfgsdfgsdfgdfgdfxyz1.*", EType::typ1},
             {".*abcdegsdfgfsdgfsdgfgsdfxyz12.*", EType::typ1},
             {".*abcdgdsgfdfgsdfgdfgdfgfxyz123.*", EType::typ1},
             {".*agfdgdfsgfgsdfgsdfgdfgdfgz1234.*", EType::typ1},
             {".*abfgdsgfsdfgfdffqrstuvwxyz12345.*", EType::typ1},
             {".*abcdfgdsfgdsfgdfgdfgdfwxyz123456.*", EType::typ1},
             {".*agfdsgfgsdfgdfgdfgdfgsdfgz1234567.*", EType::typ1},
             {".*abgfdsgfgsdfgdfgdfgtuvwxyz12345678.*", EType::typ1},
             {".*abcdefghijklmnopqrstuvwxyz123456789a.*", EType::typ1},
             {".*abcdefghijklmnopggfgsdfgsdfg3456789ab.*", EType::typ1},
             {"@@@2", EType::typ2},
             {".*afgsdfgdfgdfsfgsdfgsdfgxyz123456789abc.*", EType::typ1},
             {".*dfsgfsgdfgdfhdfgfdgdfgdfyz123456789abcd.*", EType::typ1},
             {".*abcdgfsdfgsdfgsdfgdfgdgxyz123456789abcde.*", EType::typ1},
             {".*abcgfdsgfgsdfgdfgdfgdfwxyz123456789abcdef.*", EType::typ1},
             {"@@@3", EType::typ3}},
            EType::unknown);

        UNIT_ASSERT_VALUES_EQUAL(3, classify.GetScannersSize());
        UNIT_ASSERT_VALUES_EQUAL(int(EType::typ1), int(classify["@@@1"]));
        UNIT_ASSERT_VALUES_EQUAL(int(EType::typ2), int(classify["@@@2"]));
        UNIT_ASSERT_VALUES_EQUAL(int(EType::typ3), int(classify["@@@3"]));
    }
}
