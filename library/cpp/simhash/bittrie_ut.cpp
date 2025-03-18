#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/simhash/bittrie.h>
#include <library/cpp/simhash/math_util.h>

#include <util/generic/yexception.h>
#include <util/generic/hash.h>
#include <util/generic/hash_set.h>
#include <util/generic/algorithm.h>

#include <util/random/random.h>

namespace {
    template <class TKey, ui32 ChunkSize>
    void SimpleTestHardcoded() {
        const TKey m = (TKey) ~((TKey)0);
        TBitTrie<TKey, TKey, ChunkSize> bt;

        UNIT_ASSERT_EQUAL(bt.GetCount(), 0);

        UNIT_ASSERT(bt.Insert(0, 0));
        UNIT_ASSERT_EQUAL(bt.GetCount(), 1);
        UNIT_ASSERT(bt.Insert(1, 1));
        UNIT_ASSERT_EQUAL(bt.GetCount(), 2);
        UNIT_ASSERT(bt.Insert(m - 1, m - 1));
        UNIT_ASSERT_EQUAL(bt.GetCount(), 3);
        UNIT_ASSERT(bt.Insert(m, m));
        UNIT_ASSERT_EQUAL(bt.GetCount(), 4);

        TKey* oldValue = nullptr;

        UNIT_ASSERT(!bt.Insert(0, 0, &oldValue));
        UNIT_ASSERT_EQUAL(bt.GetCount(), 4);
        UNIT_ASSERT_EQUAL(*oldValue, 0);
        UNIT_ASSERT(!bt.Insert(1, 1, &oldValue));
        UNIT_ASSERT_EQUAL(bt.GetCount(), 4);
        UNIT_ASSERT_EQUAL(*oldValue, 1);
        UNIT_ASSERT(!bt.Insert(m - 1, m - 1, &oldValue));
        UNIT_ASSERT_EQUAL(bt.GetCount(), 4);
        UNIT_ASSERT_EQUAL(*oldValue, m - 1);
        UNIT_ASSERT(!bt.Insert(m, m, &oldValue));
        UNIT_ASSERT_EQUAL(bt.GetCount(), 4);
        UNIT_ASSERT_EQUAL(*oldValue, m);

        UNIT_ASSERT(bt.Has(0));
        UNIT_ASSERT(bt.Has(1));
        UNIT_ASSERT(bt.Has(m - 1));
        UNIT_ASSERT(bt.Has(m));

        UNIT_ASSERT(!bt.Has(2));
        UNIT_ASSERT(!bt.Has(3));
        UNIT_ASSERT(!bt.Has(m - 3));
        UNIT_ASSERT(!bt.Has(m - 2));

        UNIT_ASSERT_EQUAL(bt.Get(0), 0);
        UNIT_ASSERT_EQUAL(bt.Get(1), 1);
        UNIT_ASSERT_EQUAL(bt.Get(m - 1), m - 1);
        UNIT_ASSERT_EQUAL(bt.Get(m), m);

        UNIT_ASSERT_EXCEPTION(bt.Get(2), yexception);
        UNIT_ASSERT_EQUAL(bt.Get(2, true), 0);
        bt.Get(2) = 2;
        UNIT_ASSERT_EQUAL(bt.Get(2), 2);
        UNIT_ASSERT_EQUAL(bt.GetCount(), 5);
        TKey val = 0;
        UNIT_ASSERT(bt.Delete(2, &val));
        UNIT_ASSERT_EQUAL(val, 2);
        UNIT_ASSERT_EQUAL(bt.GetCount(), 4);
        UNIT_ASSERT(!bt.Delete(2));
        UNIT_ASSERT_EQUAL(bt.GetCount(), 4);

        UNIT_ASSERT(bt.TryGet(0, &oldValue));
        UNIT_ASSERT_EQUAL(*oldValue, 0);
        UNIT_ASSERT(bt.TryGet(1, &oldValue));
        UNIT_ASSERT_EQUAL(*oldValue, 1);
        UNIT_ASSERT(bt.TryGet(m - 1, &oldValue));
        UNIT_ASSERT_EQUAL(*oldValue, m - 1);
        UNIT_ASSERT(bt.TryGet(m, &oldValue));
        UNIT_ASSERT_EQUAL(*oldValue, m);
        UNIT_ASSERT(!bt.TryGet(2, &oldValue));

        UNIT_ASSERT(bt.Delete(0, &val));
        UNIT_ASSERT_EQUAL(val, 0);
        UNIT_ASSERT_EQUAL(bt.GetCount(), 3);
        UNIT_ASSERT(bt.Delete(1, &val));
        UNIT_ASSERT_EQUAL(val, 1);
        UNIT_ASSERT_EQUAL(bt.GetCount(), 2);
        UNIT_ASSERT(bt.Delete(m - 1, &val));
        UNIT_ASSERT_EQUAL(val, m - 1);
        UNIT_ASSERT_EQUAL(bt.GetCount(), 1);
        UNIT_ASSERT(bt.Delete(m, &val));
        UNIT_ASSERT_EQUAL(val, m);
        UNIT_ASSERT_EQUAL(bt.GetCount(), 0);

        UNIT_ASSERT(!bt.Has(0));
        UNIT_ASSERT(!bt.Has(1));
        UNIT_ASSERT(!bt.Has(m - 1));
        UNIT_ASSERT(!bt.Has(m));
    }

    template <class TKey, ui32 ChunkSize>
    void TestNeighbors(
        const TBitTrie<TKey, TKey, ChunkSize>& Bt,
        const THashMap<TKey, TKey>& Check,
        TKey Key,
        ui32 Distance) {
        THashSet<TKey> check;
        for (typename THashMap<TKey, TKey>::const_iterator it = Check.begin();
             it != Check.end();
             ++it) {
            if (HammingDistance(Key, it->first) <= Distance) {
                UNIT_ASSERT(check.insert(it->first).second);
            }
        }
        TVector<TKey> test;
        Bt.GetNeighbors(Key, Distance, &test);

        Sort(test.begin(), test.end());
        for (ui32 i = 1; i < (ui32)test.size(); ++i) {
            UNIT_ASSERT(test[i - 1] != test[i]);
        }

        UNIT_ASSERT_EQUAL(check.size(), test.size());
        for (ui32 i = 0; i < (ui32)test.size(); ++i) {
            UNIT_ASSERT(check.contains(test[i]));
            check.erase(test[i]);
        }
        UNIT_ASSERT_EQUAL(check.size(), 0);

        THashMap<TKey, std::pair<TKey*, size_t>> counts;
        Bt.CountNeighbors(counts, Distance);
        for (typename THashMap<TKey, TKey>::const_iterator it = Check.begin();
             it != Check.end();
             ++it) {
            const TKey key = it->first;

            TVector<TKey> group;
            Bt.GetNeighbors(key, Distance, &group);

            const size_t count1 = counts[key].second;
            const size_t count2 = Bt.GetNeighborCount(key, Distance);

            if (group.size() != count1 || group.size() != count2) {
                return;
            }

            UNIT_ASSERT_EQUAL(group.size(), count1);
            UNIT_ASSERT_EQUAL(group.size(), count2);
        }
    }

    template <class TKey, ui32 ChunkSize>
    void RandomTest(
        const ui32 Iterations,
        const ui32 CheckPeriod,
        TBitTrie<TKey, TKey, ChunkSize>& bt,
        THashMap<TKey, TKey>& check) {
        for (ui32 i = 1; i <= Iterations; ++i) {
            TKey key = RandomNumber<TKey>();
            TKey old_key1 = 0;
            TKey* pold_key1 = nullptr;
            TKey old_key2 = 0;

            bool res1 = bt.Insert(key, key, &pold_key1);
            if (!res1) {
                old_key1 = *pold_key1;
            }
            std::pair<typename THashMap<TKey, TKey>::iterator, bool> temp = check.insert(std::make_pair(key, key));
            bool res2 = temp.second;
            if (!res2) {
                old_key2 = temp.first->first;
            }
            UNIT_ASSERT_EQUAL(res1, res2);
            if (!res1) {
                UNIT_ASSERT_EQUAL(old_key1, key);
                UNIT_ASSERT_EQUAL(old_key1, old_key2);
            }

            if (i % CheckPeriod == 0) {
                UNIT_ASSERT_EQUAL(bt.GetCount(), (ui32)check.size());
                for (typename THashMap<TKey, TKey>::const_iterator it = check.begin();
                     it != check.end();
                     ++it) {
                    UNIT_ASSERT_EQUAL(bt.Get(it->first), it->second);
                }

                TestNeighbors(bt, check, key, 0);
                TestNeighbors(bt, check, key, RandomNumber<ui32>(8));

                for (ui32 j = 0; j < 10; ++j) {
                    TKey key2 = RandomNumber<TKey>();

                    TestNeighbors(bt, check, key2, 2);
                }
            }
        }

        TestNeighbors(bt, check, RandomNumber<TKey>(), 12);
        TestNeighbors(bt, check, RandomNumber<TKey>(), 13);
    }

    template <class TKey, ui32 ChunkSize>
    void RandomRemove(
        TBitTrie<TKey, TKey, ChunkSize>& bt,
        THashMap<TKey, TKey>& check) {
        UNIT_ASSERT_EQUAL(bt.GetCount(), (ui32)check.size());

        UNIT_ASSERT(bt.Min() <= bt.Max());
        UNIT_ASSERT(bt.CMin() <= bt.CMax());

        typename TBitTrie<TKey, TKey, ChunkSize>::TIt it1 = bt.Min();

        if (bt.GetCount() != 0) {
            UNIT_ASSERT(bt.Min() < bt.Max());
            UNIT_ASSERT(bt.CMin() < bt.CMax());

            {
                typename TBitTrie<TKey, TKey, ChunkSize>::TConstIt cit = bt.CMin();

                UNIT_ASSERT(!it1.IsEndReached());
                UNIT_ASSERT(!cit.IsEndReached());
                UNIT_ASSERT(it1 == bt.Min());
                UNIT_ASSERT(cit == bt.CMin());

                it1.Dec();
                cit.Dec();

                UNIT_ASSERT(it1.IsMinEndReached());
                UNIT_ASSERT(cit.IsMinEndReached());

                it1.Inc();
                cit.Inc();

                UNIT_ASSERT(!it1.IsEndReached());
                UNIT_ASSERT(!cit.IsEndReached());
                UNIT_ASSERT(it1 == bt.Min());
                UNIT_ASSERT(cit == bt.CMin());

                it1 = bt.Max();
                cit = bt.CMax();

                UNIT_ASSERT(!it1.IsEndReached());
                UNIT_ASSERT(!cit.IsEndReached());
                UNIT_ASSERT(it1 == bt.Max());
                UNIT_ASSERT(cit == bt.CMax());

                it1.Inc();
                cit.Inc();

                UNIT_ASSERT(it1.IsMaxEndReached());
                UNIT_ASSERT(cit.IsMaxEndReached());

                it1.Dec();
                cit.Dec();

                UNIT_ASSERT(!it1.IsEndReached());
                UNIT_ASSERT(!cit.IsEndReached());
                UNIT_ASSERT(it1 == bt.Max());
                UNIT_ASSERT(cit == bt.CMax());
            }
        } else {
            UNIT_ASSERT(bt.Min() == bt.Max());
            UNIT_ASSERT(bt.Min().IsEndReached());
            UNIT_ASSERT(bt.Max().IsEndReached());
        }

        for (it1 = bt.Min(); !it1.IsEndReached(); it1.Inc()) {
            TKey key = it1.GetCurrentKey();

            UNIT_ASSERT(check.contains(key));
            UNIT_ASSERT(check[key] == it1.GetCurrentValue());
        }

        for (it1 = bt.Max(); !it1.IsEndReached(); it1.Dec()) {
            TKey key = it1.GetCurrentKey();

            UNIT_ASSERT(check.contains(key));
            UNIT_ASSERT(check[key] == it1.GetCurrentValue());
        }

        TVector<TKey> toDelete;
        for (typename THashMap<TKey, TKey>::iterator it2 = check.begin();
             it2 != check.end();
             ++it2) {
            if (RandomNumber<size_t>(1000) < 666) {
                TKey value = 0;
                UNIT_ASSERT(bt.Delete(it2->first, &value));
                UNIT_ASSERT_EQUAL(value, it2->second);
                toDelete.push_back(it2->first);
            }
        }
        for (size_t i = 0; i < toDelete.size(); ++i) {
            check.erase(toDelete[i]);
        }
        UNIT_ASSERT_EQUAL(bt.GetCount(), (ui32)check.size());
    }

    template <class TKey, ui32 ChunkSize>
    void RandomTest() {
        TBitTrie<TKey, TKey, ChunkSize> bt;
        THashMap<TKey, TKey> check;

        RandomRemove(bt, check);

        RandomTest(10, 1, bt, check);

        RandomRemove(bt, check);

        RandomTest(50, 10, bt, check);

        RandomRemove(bt, check);

        RandomTest(100, 10, bt, check);

        RandomRemove(bt, check);
    }
}

Y_UNIT_TEST_SUITE(TBitTrieTest) {
    Y_UNIT_TEST(SimpleTest8) {
        SimpleTestHardcoded<ui8, 1>();
        SimpleTestHardcoded<ui8, 2>();
        SimpleTestHardcoded<ui8, 3>();
        SimpleTestHardcoded<ui8, 4>();
        SimpleTestHardcoded<ui8, 5>();
        SimpleTestHardcoded<ui8, 6>();
        SimpleTestHardcoded<ui8, 7>();
        SimpleTestHardcoded<ui8, 8>();
    }

    Y_UNIT_TEST(SimpleTest16) {
        SimpleTestHardcoded<ui16, 1>();
        SimpleTestHardcoded<ui16, 2>();
        SimpleTestHardcoded<ui16, 3>();
        SimpleTestHardcoded<ui16, 4>();
        SimpleTestHardcoded<ui16, 5>();
        SimpleTestHardcoded<ui16, 6>();
        SimpleTestHardcoded<ui16, 7>();
        SimpleTestHardcoded<ui16, 8>();
    }

    Y_UNIT_TEST(SimpleTest32) {
        SimpleTestHardcoded<ui32, 1>();
        SimpleTestHardcoded<ui32, 2>();
        SimpleTestHardcoded<ui32, 3>();
        SimpleTestHardcoded<ui32, 4>();
        SimpleTestHardcoded<ui32, 5>();
        SimpleTestHardcoded<ui32, 6>();
        SimpleTestHardcoded<ui32, 7>();
        SimpleTestHardcoded<ui32, 8>();
    }

    Y_UNIT_TEST(SimpleTest64) {
        SimpleTestHardcoded<ui64, 1>();
        SimpleTestHardcoded<ui64, 2>();
        SimpleTestHardcoded<ui64, 3>();
        SimpleTestHardcoded<ui64, 4>();
        SimpleTestHardcoded<ui64, 5>();
        SimpleTestHardcoded<ui64, 6>();
        SimpleTestHardcoded<ui64, 7>();
        SimpleTestHardcoded<ui64, 8>();
    }

    Y_UNIT_TEST(RandomTestGenerated8) {
        RandomTest<ui8, 1>();
        RandomTest<ui8, 2>();
        RandomTest<ui8, 3>();
        RandomTest<ui8, 4>();
        RandomTest<ui8, 5>();
        RandomTest<ui8, 6>();
        RandomTest<ui8, 7>();
        RandomTest<ui8, 8>();
    }

    Y_UNIT_TEST(RandomTestGenerated16) {
#ifdef FULL_BITTRIE_TEST
        RandomTest<ui16, 1>();
        RandomTest<ui16, 2>();
        RandomTest<ui16, 3>();
        RandomTest<ui16, 4>();
        RandomTest<ui16, 5>();
        RandomTest<ui16, 6>();
        RandomTest<ui16, 7>();
        RandomTest<ui16, 8>();
#else
        RandomTest<ui16, 1>();
        RandomTest<ui16, 2>();
        RandomTest<ui16, 4>();
        RandomTest<ui16, 8>();
#endif
    }

    Y_UNIT_TEST(RandomTestGenerated32) {
#ifdef FULL_BITTRIE_TEST
        RandomTest<ui32, 1>();
        RandomTest<ui32, 2>();
        RandomTest<ui32, 3>();
        RandomTest<ui32, 4>();
        RandomTest<ui32, 5>();
        RandomTest<ui32, 6>();
        RandomTest<ui32, 7>();
        RandomTest<ui32, 8>();
#else
        RandomTest<ui32, 1>();
        RandomTest<ui32, 2>();
        RandomTest<ui32, 4>();
#endif
    }

    Y_UNIT_TEST(RandomTestGenerated64) {
#ifdef FULL_BITTRIE_TEST
        RandomTest<ui64, 1>();
        RandomTest<ui64, 2>();
        RandomTest<ui64, 3>();
        RandomTest<ui64, 4>();
        RandomTest<ui64, 5>();
        RandomTest<ui64, 6>();
        RandomTest<ui64, 7>();
        RandomTest<ui64, 8>();
#else
        RandomTest<ui64, 2>();
        RandomTest<ui64, 3>();
        RandomTest<ui64, 4>();
#endif
    }
}
