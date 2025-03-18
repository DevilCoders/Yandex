#include "id_for_string.h"
#include "make_vector.h"

#include <library/cpp/testing/unittest/registar.h>

Y_UNIT_TEST_SUITE(TIdForString) {
    Y_UNIT_TEST(TestIterator0) {
        TIdForString s(3, MakeVector<TString>());

        TIdForString::TIterator it = s.Iterator();

        UNIT_ASSERT(!it.Next());
    }

    Y_UNIT_TEST(TestIterator1) {
        TIdForString s(3, MakeVector<TString>("aaa"));

        TIdForString::TIterator it = s.Iterator();

        UNIT_ASSERT(it.Next());
        UNIT_ASSERT_VALUES_EQUAL(TIdForString::TId(0), it->Id);

        UNIT_ASSERT(!it.Next());
    }

    Y_UNIT_TEST(TestIterator2) {
        TIdForString s(3, MakeVector<TString>("aaa", "bbb"));

        TIdForString::TIterator it = s.Iterator();

        UNIT_ASSERT(it.Next());
        UNIT_ASSERT_VALUES_EQUAL(TIdForString::TId(0), it->Id);

        UNIT_ASSERT(it.Next());
        UNIT_ASSERT_VALUES_EQUAL(TIdForString::TId(1), it->Id);

        UNIT_ASSERT(!it.Next());
    }
}
