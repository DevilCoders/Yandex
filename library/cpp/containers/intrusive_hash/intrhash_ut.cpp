#include "intrhash.h"

#include <library/cpp/testing/unittest/registar.h>

struct TMyItem: public std::pair<const TString, const int>, public TIntrusiveHashItem<TMyItem> {
    inline TMyItem(const TString& first, int second)
        : std::pair<const TString, const int>(first, second)
    {
    }
};

struct TMyOps: public TCommonIntrHashOps {
    static inline const TString& ExtractKey(const TMyItem& from) noexcept {
        return from.first;
    }

    static inline size_t Hash(const TString& s) {
        return TCommonIntrHashOps::Hash(s);
    }

    static inline size_t Hash(const char* s) {
        return Default<THash<TStringBuf>>()(s);
    };
};

class TIntrusiveHashTest: public TTestBase {
private:
    UNIT_TEST_SUITE(TIntrusiveHashTest);
    UNIT_TEST(TestIntrusiveHash1);
    UNIT_TEST(TestIntrusiveHash2);
    UNIT_TEST_SUITE_END();

protected:
    void TestIntrusiveHash1();
    void TestIntrusiveHash2();
};

UNIT_TEST_SUITE_REGISTRATION(TIntrusiveHashTest);

void TIntrusiveHashTest::TestIntrusiveHash1() {
    TIntrusiveHash<TMyItem, TMyOps> h;
    TMyItem i1("one", 1), i2("one", 11), i3("two", 2), i4("two", 12), i5("three", 3);

    UNIT_ASSERT(!i1.Linked() && !i2.Linked() && !i3.Linked() && !i4.Linked() && !i5.Linked());

    h.Push(&i1);
    h.Push(&i2);
    h.Push(&i3);

    UNIT_ASSERT(!h.FindOrPush("two", [&i4]() { return &i4; }).second);

    h.Push(&i4);

    UNIT_ASSERT(h.FindOrPush("three", [&i5]() { return &i5; }).second);

    UNIT_ASSERT(i1.Linked() && i2.Linked() && i3.Linked() && i4.Linked() && i5.Linked());

    UNIT_ASSERT(h.Has("one"));
    UNIT_ASSERT_VALUES_EQUAL(h.Count("one"), 2);
    UNIT_ASSERT(h.Find("one")->second % 10 == 1);

    UNIT_ASSERT(h.Has("two"));
    UNIT_ASSERT_VALUES_EQUAL(h.Count("two"), 2);
    UNIT_ASSERT(h.Find("two")->second % 10 == 2);

    UNIT_ASSERT(h.Has("three"));
    UNIT_ASSERT_VALUES_EQUAL(h.Count("three"), 1);
    UNIT_ASSERT_VALUES_EQUAL(h.Find("three")->second, 3);

    uintptr_t one = (uintptr_t)h.Pop("one");
    UNIT_ASSERT((one == (uintptr_t)&i1 || one == (uintptr_t)&i2) && h.Count("one") == 1);

    uintptr_t two = 0;
    h.PopAll("two", [&two](TMyItem* item) {
        two = !two ? (uintptr_t)item : two ^ (uintptr_t)item;
    });
    UNIT_ASSERT_VALUES_EQUAL(two, (uintptr_t)&i3 ^ (uintptr_t)&i4);
    UNIT_ASSERT_VALUES_EQUAL(h.Count("two"), 0);

    UNIT_ASSERT(h.Has("three"));

    h.Decompose();

    UNIT_ASSERT(h.Empty());

    UNIT_ASSERT(!h.Has("one") && !h.Has("two") && !h.Has("three"));

    UNIT_ASSERT(!i1.Linked() && !i2.Linked() && !i3.Linked() && !i4.Linked() && !i5.Linked());
}

void TIntrusiveHashTest::TestIntrusiveHash2() {
    TMyItem i1("one", 1), i2("one", 11), i3("two", 2), i4("two", 12), i5("three", 3);
    TIntrusiveHash<TMyItem, TMyOps> h;

    h.Push(&i1);
    h.Push(&i2);
    h.Push(&i3);
    h.Push(&i4);
    h.Push(&i5);

    int s = 0;
    for (const auto& i : h) {
        s += i.second;
    }
    UNIT_ASSERT_VALUES_EQUAL(s, i1.second + i2.second + i3.second + i4.second + i5.second);

    auto e = h.EqualRange("two");
    UNIT_ASSERT(std::distance(e.first, e.second) == 2);
    for (auto i = e.first; i != e.second; ++i) {
        UNIT_ASSERT(i->second % 10 == 2);
    }

    h.Resize(100);

    e = h.EqualRange("two");
    UNIT_ASSERT(std::distance(e.first, e.second) == 2);
    for (auto i = e.first; i != e.second; ++i) {
        UNIT_ASSERT(i->second % 10 == 2);
    }

    h.Pop("two");
    e = h.EqualRange("two");
    UNIT_ASSERT(std::distance(e.first, e.second) == 1);

    h.Pop("two");
    e = h.EqualRange("two");
    UNIT_ASSERT(std::distance(e.first, e.second) == 0);
}
