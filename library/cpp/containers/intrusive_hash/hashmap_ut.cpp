#include "hashmap.h"

#include <library/cpp/testing/unittest/registar.h>

struct TMyOps: public TCommonIntrHashOps {
    static inline size_t Hash(const TString& s) {
        return TCommonIntrHashOps::Hash(s);
    }

    static inline size_t Hash(const char* s) {
        return Default<THash<TStringBuf>>()(s);
    };
};

class THashMapTest: public TTestBase {
private:
    UNIT_TEST_SUITE(THashMapTest);
    UNIT_TEST(TestHashMap1);
    UNIT_TEST(TestHashMap2);
    UNIT_TEST(TestHashMap3);

    UNIT_TEST(TestHashMultiMap1);
    UNIT_TEST(TestHashMultiMap2);
    UNIT_TEST_SUITE_END();

protected:
    void TestHashMap1();
    void TestHashMap2();
    void TestHashMap3();

    void TestHashMultiMap1();
    void TestHashMultiMap2();
};

UNIT_TEST_SUITE_REGISTRATION(THashMapTest);

void THashMapTest::TestHashMap1() {
    using H = THashMapType<TString, int, TMyOps>;

    H h1;
    h1["one"] = 1;
    h1["two"] = 2;

    H h2(h1);

    UNIT_ASSERT_VALUES_EQUAL(h1.Size(), 2);
    UNIT_ASSERT_VALUES_EQUAL(h2.Size(), 2);
    UNIT_ASSERT_VALUES_EQUAL(h1.At("one"), 1);
    UNIT_ASSERT_VALUES_EQUAL(h2.At("two"), 2);

    H h3(std::move(h1));

    UNIT_ASSERT_VALUES_EQUAL(h1.Size(), 0);
    UNIT_ASSERT_VALUES_EQUAL(h3.Size(), 2);
    UNIT_ASSERT_VALUES_EQUAL(h3.At("one"), 1);

    h2["three"] = 3;
    h3 = h2;

    UNIT_ASSERT_VALUES_EQUAL(h2.Size(), 3);
    UNIT_ASSERT_VALUES_EQUAL(h3.Size(), 3);
    UNIT_ASSERT_VALUES_EQUAL(h3.At("three"), 3);

    h2["four"] = 4;
    h3 = std::move(h2);

    UNIT_ASSERT_VALUES_EQUAL(h2.Size(), 0);
    UNIT_ASSERT_VALUES_EQUAL(h3.Size(), 4);
    UNIT_ASSERT_VALUES_EQUAL(h3.At("four"), 4);
}

void THashMapTest::TestHashMap2() {
    using H = THashMapType<char, TString>;
    H h;

    h['l'] = "50";
    h['x'] = "20";
    h['v'] = "5";
    h['i'] = "1";

    UNIT_ASSERT(h['x'] == "20");
    h['x'] = "10";
    UNIT_ASSERT(h['x'] == "10");

    UNIT_ASSERT(!h.Has('z'));
    UNIT_ASSERT(h['z'] == "");
    UNIT_ASSERT(h.Has('z'));

    UNIT_ASSERT(h.Count('z') == 1);

    auto r = h.Insert(std::pair<const char, TString>('c', "100"));
    UNIT_ASSERT(r.second);

    r = h.Insert(std::pair<const char, TString>('c', "100"));
    UNIT_ASSERT(!r.second);

    H::TIterator i(h.Begin());

    H::TConstIterator ci(h.Begin());
    ci = h.Begin();
    ci = static_cast<const H&>(h).Begin();

    UNIT_ASSERT(i == ci);
    UNIT_ASSERT(!(i != ci));
    UNIT_ASSERT(ci == i);
    UNIT_ASSERT(!(ci != i));
}

void THashMapTest::TestHashMap3() {
    using H = THashMapType<TString, size_t, TMyOps>;
    using V = H::TValue;

    {
        H h;

        UNIT_ASSERT(h.Insert(V("foo", 0)).second);
        UNIT_ASSERT(h.Insert(V("bar", 0)).second);
        UNIT_ASSERT(h.Insert(V("abc", 0)).second);

        UNIT_ASSERT(h.Erase("foo") == 1);
        UNIT_ASSERT(h.Erase("bar") == 1);
        UNIT_ASSERT(h.Erase("abc") == 1);
    }

    {
        H h;

        UNIT_ASSERT(h.Insert(V("foo", 0)).second);
        UNIT_ASSERT(h.Insert(V("bar", 0)).second);
        UNIT_ASSERT(h.Insert(V("abc", 0)).second);

        UNIT_ASSERT(h.Erase("abc") == 1);
        UNIT_ASSERT(h.Erase("bar") == 1);
        UNIT_ASSERT(h.Erase("foo") == 1);
    }
}

void THashMapTest::TestHashMultiMap1() {
    using H = THashMultiMapType<TString, int, TMyOps>;

    H h1;
    h1.Insert(H::TValue("one", 1));
    h1.Insert(H::TValue("two", 2));

    H h2(h1);

    UNIT_ASSERT_VALUES_EQUAL(h1.Size(), 2);
    UNIT_ASSERT_VALUES_EQUAL(h2.Size(), 2);

    H h3(std::move(h1));

    UNIT_ASSERT_VALUES_EQUAL(h1.Size(), 0);
    UNIT_ASSERT_VALUES_EQUAL(h3.Size(), 2);

    h2.Insert(H::TValue("three", 3));
    h3 = h2;

    UNIT_ASSERT_VALUES_EQUAL(h2.Size(), 3);
    UNIT_ASSERT_VALUES_EQUAL(h3.Size(), 3);

    h2.Insert(H::TValue("four", 4));
    h3 = std::move(h2);

    UNIT_ASSERT_VALUES_EQUAL(h2.Size(), 0);
    UNIT_ASSERT_VALUES_EQUAL(h3.Size(), 4);
}

void THashMapTest::TestHashMultiMap2() {
    using H1 = THashMultiMapType<char, int>;
    H1 h1;

    UNIT_ASSERT(h1.Count('X') == 0);

    h1.Insert(H1::TValue('X', 10));
    UNIT_ASSERT(h1.Count('X') == 1);

    h1.Insert(H1::TValue('X', 20));
    UNIT_ASSERT(h1.Count('X') == 2);

    h1.Insert(H1::TValue('Y', 32));
    auto r = h1.Find('X'); // Find first match.

    UNIT_ASSERT((*r).first == 'X');
    UNIT_ASSERT_VALUES_EQUAL((*r).second % 10, 0);
    r++;
    UNIT_ASSERT((*r).first == 'X');
    UNIT_ASSERT_VALUES_EQUAL((*r).second % 10, 0);

    r = h1.Find('Y');
    UNIT_ASSERT((*r).first == 'Y');
    UNIT_ASSERT((*r).second == 32);

    r = h1.Find('Z');
    UNIT_ASSERT(r == h1.End());

    const size_t c = h1.Erase('X');
    UNIT_ASSERT(c == 2);

    H1::TIterator i(h1.Begin());
    H1::TConstIterator ci(h1.Begin());

    UNIT_ASSERT((H1::TConstIterator)i == ci);
    UNIT_ASSERT(!((H1::TConstIterator)i != ci));
    UNIT_ASSERT(ci == (H1::TConstIterator)i);
    UNIT_ASSERT(!(ci != (H1::TConstIterator)i));

    using H2 = THashMultiMapType<size_t, size_t>;
    H2 h2;

    for (size_t c2 = 0; c2 < 3077; ++c2) {
        h2.Insert(H2::TValue(1, c2));
    }

    h2.Insert(H2::TValue(12325, 1));
    h2.Insert(H2::TValue(12325, 2));

    UNIT_ASSERT_VALUES_EQUAL(h2.Count((size_t)12325), 2);

    h2.Insert(H2::TValue(23, 0));

    UNIT_ASSERT_VALUES_EQUAL(h2.Count((size_t)12325), 2);
}
