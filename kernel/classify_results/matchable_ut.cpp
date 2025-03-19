#include <library/cpp/binsaver/bin_saver.h>
#include <library/cpp/testing/unittest/registar.h>
#include <util/charset/wide.h>

#include "matchable.h"

Y_UNIT_TEST_SUITE(TMatchableTest) {
    Y_UNIT_TEST(DefIsFalse) {
        TMatchable m;
        UNIT_ASSERT(!m.IsMatch(u""));
        UNIT_ASSERT(!m.IsMatch(u"asd"));
        int res = 0xDEADBEEF;
        m.IsMatch(u"asd", &res);
        UNIT_ASSERT_VALUES_EQUAL(res, (int)0xDEADBEEF);
    }
    Y_UNIT_TEST(TrueCases) {
        TMatchable m;
        m.AddSimple(u"Карл у Клары");
        m.AddSimple(u"Украл кораллы", 7);
        m.AddRx(u"у[Кк][Рр][Аа][Лл] кораллы", 7);

        int res = 0xDEADBEEF;
        UNIT_ASSERT(m.IsMatch(u"Карл у Клары", &res));
        UNIT_ASSERT_VALUES_EQUAL(res, 0);
        UNIT_ASSERT(m.IsMatch(u"Украл кораллы", &res));
        UNIT_ASSERT_VALUES_EQUAL(res, 7);
        UNIT_ASSERT(m.IsMatch(u"уКРАЛ кораллы", &res));
        UNIT_ASSERT_VALUES_EQUAL(res, 7);
    }
    Y_UNIT_TEST(FalseCases) {
        TMatchable m;
        m.AddRx(u"\\d", 7);

        int res = 0xDEADBEEF;
        UNIT_ASSERT(!m.IsMatch(u"asd", &res));
        UNIT_ASSERT_VALUES_EQUAL(res, (int)0xDEADBEEF);
        UNIT_ASSERT(!m.IsMatch(u"", &res));
        UNIT_ASSERT_VALUES_EQUAL(res, (int)0xDEADBEEF);
    }
}
