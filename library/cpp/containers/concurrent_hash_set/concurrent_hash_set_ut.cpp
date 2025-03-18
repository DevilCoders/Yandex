#include <algorithm>
#include <thread>

#include <library/cpp/containers/concurrent_hash_set/concurrent_hash_set.h>
#include <library/cpp/testing/unittest/registar.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

Y_UNIT_TEST_SUITE(TConcurrentHashSetTest) {
    Y_UNIT_TEST(TestInsert) {
        TConcurrentHashSet<ui32> set;
        for (ui32 i = 0; i < 10; ++i) {
            UNIT_ASSERT(set.Insert(i + 20));
            UNIT_ASSERT_C(!set.Insert(i + 20), "second insert should return false");
        }
        for (ui32 i = 0; i < 10; ++i) {
            UNIT_ASSERT_VALUES_EQUAL(i + 20, set.Get(i + 20));
        }
        UNIT_ASSERT_VALUES_EQUAL(10, set.Size());
        UNIT_CHECK_GENERATED_EXCEPTION(set.Get(0), yexception);
    }

    Y_UNIT_TEST(TestInsertReplace) {
        TConcurrentHashSet<TString> set;
        TString key1 = "10";
        TString key2 = "10";
        UNIT_ASSERT_UNEQUAL(key1.data(), key2.data());
        UNIT_ASSERT(set.Insert(key1));
        UNIT_ASSERT_VALUES_EQUAL(1, set.Size());
        UNIT_ASSERT(!set.InsertReplace(key2));
        UNIT_ASSERT_VALUES_EQUAL(1, set.Size());
        UNIT_ASSERT_EQUAL(key2.data(), set.Get("10").data());
    }

    Y_UNIT_TEST(TestErase) {
        TConcurrentHashSet<ui32> set;
        for (ui32 i = 0; i < 10; ++i) {
            UNIT_ASSERT(set.Insert(i + 20));
        }
        UNIT_ASSERT_VALUES_EQUAL(10, set.Size());
        for (ui32 i = 0; i < 10; ++i) {
            UNIT_ASSERT(!set.Erase(i));
        }
        UNIT_ASSERT_VALUES_EQUAL(10, set.Size());
        for (ui32 i = 0; i < 10; ++i) {
            UNIT_ASSERT(set.Erase(i + 20));
        }
        UNIT_ASSERT_VALUES_EQUAL(0, set.Size());
    }

    Y_UNIT_TEST(TestGet) {
        TConcurrentHashSet<ui32> set;
        for (ui32 i = 0; i < 10; ++i) {
            UNIT_ASSERT(set.Insert(i + 20));
        }
        UNIT_ASSERT_VALUES_EQUAL(10, set.Size());
        for (ui32 i = 0; i < 10; ++i) {
            UNIT_ASSERT_VALUES_EQUAL(i + 20, set.Get(i + 20));
        }
        UNIT_CHECK_GENERATED_EXCEPTION(set.Get(0), yexception);
        UNIT_CHECK_GENERATED_EXCEPTION(set.Get(19), yexception);
    }

    Y_UNIT_TEST(TestFind) {
        TConcurrentHashSet<ui32> set;
        for (ui32 i = 0; i < 10; ++i) {
            UNIT_ASSERT(set.Insert(i + 20));
        }
        UNIT_ASSERT_VALUES_EQUAL(10, set.Size());
        for (ui32 i = 0; i < 10; ++i) {
            auto x = set.Find(i + 20);
            UNIT_ASSERT(x);
            UNIT_ASSERT_VALUES_EQUAL(i + 20, *x);
            UNIT_ASSERT(!set.Find(i));
        }
    }

    Y_UNIT_TEST(TestContains) {
        TConcurrentHashSet<ui32> set;
        for (ui32 i = 0; i < 10; ++i) {
            UNIT_ASSERT(set.Insert(i + 20));
        }
        UNIT_ASSERT_VALUES_EQUAL(10, set.Size());
        for (ui32 i = 0; i < 10; ++i) {
            UNIT_ASSERT(set.Contains(i + 20));
            UNIT_ASSERT(!set.Contains(i));
        }
    }

    template <ui32 N>
    struct TCounter {
        static ui32 Create;
        ;
        static ui32 Copy;
        static ui32 Move;
    };

    template <ui32 N>
    ui32 TCounter<N>::Create = 0;

    template <ui32 N>
    ui32 TCounter<N>::Copy = 0;

    template <ui32 N>
    ui32 TCounter<N>::Move = 0;

    template <class T>
    struct TCountCopy {
        explicit TCountCopy(int v)
            : Value(v)
        {
            ++T::Create;
        }

        TCountCopy(const TCountCopy& other)
            : Value(other.Value)
        {
            ++T::Copy;
        }

        TCountCopy(TCountCopy&& other)
            : Value(std::move(other.Value))
        {
            ++T::Move;
        }

        TCountCopy<T>& operator=(const TCountCopy<T>& other) {
            if (this != &other) {
                ++T::Copy;
                Value = other.Value;
            }
            return *this;
        }

        TCountCopy<T>& operator=(TCountCopy<T>&& other) {
            if (this != &other) {
                ++T::Move;
            }
            return *this;
        }

        int Value;
    };

    template <class T>
    struct TOps {
        bool operator()(const TCountCopy<T>& l, const TCountCopy<T>& r) {
            return std::equal_to<int>()(l.Value, r.Value);
        }

        bool operator()(const TCountCopy<T>& l, const int r) {
            return std::equal_to<int>()(l.Value, r);
        }

        size_t operator()(const TCountCopy<T>& l) {
            return ::THash<int>()(l.Value);
        }

        size_t operator()(const int l) {
            return ::THash<int>()(l);
        }
    };

    Y_UNIT_TEST(TestCountInsertReplace) {
        using TCount = TCounter<__LINE__>;

        TConcurrentHashSet<TCountCopy<TCount>, TOps<TCount>, TOps<TCount>> index;
        UNIT_ASSERT(index.Insert(1));
        UNIT_ASSERT_VALUES_EQUAL(1, TCount::Create);
        UNIT_ASSERT_VALUES_EQUAL(0, TCount::Copy);
        UNIT_ASSERT_VALUES_EQUAL(0, TCount::Move);

        UNIT_ASSERT(!index.Insert(1));
        UNIT_ASSERT_VALUES_EQUAL(2, TCount::Create);
        UNIT_ASSERT_VALUES_EQUAL(0, TCount::Copy);
        UNIT_ASSERT_VALUES_EQUAL(0, TCount::Move);

        UNIT_ASSERT(!index.InsertReplace(1));

        UNIT_ASSERT_VALUES_EQUAL(3, TCount::Create);
        UNIT_ASSERT_VALUES_EQUAL(0, TCount::Copy);
        UNIT_ASSERT_VALUES_EQUAL(0, TCount::Move);
    }

    Y_UNIT_TEST(TestCountInsert) {
        using TCount = TCounter<__LINE__>;
        TConcurrentHashSet<TCountCopy<TCount>, TOps<TCount>, TOps<TCount>> index;
        TCountCopy<TCount> key(1);
        UNIT_ASSERT(index.Insert(key));
        UNIT_ASSERT_VALUES_EQUAL(1, TCount::Copy);
        UNIT_ASSERT_VALUES_EQUAL(1, TCount::Move);
        UNIT_ASSERT_VALUES_EQUAL(1, index.Size());

        index.Insert(TCountCopy<TCount>{2});
        UNIT_ASSERT_VALUES_EQUAL(1, TCount::Copy);
        UNIT_ASSERT_VALUES_EQUAL(3, TCount::Move);
        UNIT_ASSERT_VALUES_EQUAL(2, index.Size());
    }

    Y_UNIT_TEST(TestCountGet) {
        using TCount = TCounter<__LINE__>;
        TConcurrentHashSet<TCountCopy<TCount>, TOps<TCount>, TOps<TCount>> index;
        UNIT_ASSERT(index.Insert(1));
        TCountCopy<TCount> key(1);
        UNIT_ASSERT_VALUES_EQUAL(1, index.Get(key).Value);
        UNIT_ASSERT_VALUES_EQUAL(2, TCount::Copy);
        UNIT_ASSERT_VALUES_EQUAL(0, TCount::Move);

        UNIT_ASSERT_VALUES_EQUAL(1, index.Get(TCountCopy<TCount>{1}).Value);
        UNIT_ASSERT_VALUES_EQUAL(3, TCount::Copy);
        UNIT_ASSERT_VALUES_EQUAL(1, TCount::Move);
    }

    Y_UNIT_TEST(TestCountErase) {
        using TCount = TCounter<__LINE__>;
        TConcurrentHashSet<TCountCopy<TCount>, TOps<TCount>, TOps<TCount>> index;
        UNIT_ASSERT(index.Insert(1));
        UNIT_ASSERT(index.Insert(2));

        TCountCopy<TCount> key(1);
        UNIT_ASSERT(index.Erase(key));
        UNIT_ASSERT_VALUES_EQUAL(1, TCount::Copy);
        UNIT_ASSERT_VALUES_EQUAL(0, TCount::Move);

        UNIT_ASSERT(index.Erase(TCountCopy<TCount>{2}));
        UNIT_ASSERT_VALUES_EQUAL(1, TCount::Copy);
        UNIT_ASSERT_VALUES_EQUAL(1, TCount::Move);
    }

    Y_UNIT_TEST(TestCountFind) {
        using TCount = TCounter<__LINE__>;
        TConcurrentHashSet<TCountCopy<TCount>, TOps<TCount>, TOps<TCount>> index;
        UNIT_ASSERT(index.Insert(1));

        TCountCopy<TCount> key(1);
        UNIT_ASSERT(index.Find(key));
        UNIT_ASSERT_VALUES_EQUAL(2, TCount::Copy);
        UNIT_ASSERT_VALUES_EQUAL(0, TCount::Move);

        UNIT_ASSERT(index.Find(TCountCopy<TCount>{1}));
        UNIT_ASSERT_VALUES_EQUAL(3, TCount::Copy);
        UNIT_ASSERT_VALUES_EQUAL(1, TCount::Move);
    }

    Y_UNIT_TEST(TestConcurrentAccess) {
        TVector<std::thread> threads;
        TConcurrentHashSet<TString> index;
        for (size_t i = 0; i < 100; ++i) {
            threads.emplace_back([&index]() {
                for (size_t i = 0; i < 100; ++i) {
                    index.Insert(ToString(i));
                    index.InsertReplace(ToString(i + 1));
                    index.Erase(ToString(i + 2));
                }
            });
        }
        for (auto& t : threads) {
            t.join();
        }

        UNIT_ASSERT(index.Find("1"));
        UNIT_ASSERT(!index.Find("101"));
    }
}
