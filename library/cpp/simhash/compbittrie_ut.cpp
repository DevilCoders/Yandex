#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/simhash/compbittrie.h>
#include <library/cpp/simhash/math_util.h>

#include <util/generic/yexception.h>
#include <util/generic/hash.h>
#include <util/generic/hash_set.h>
#include <util/generic/algorithm.h>

#include <util/random/random.h>

namespace {
    template <class TKey>
    void SimpleTestHardcoded() {
        const TKey m = (TKey) ~((TKey)0);
        TCompBitTrie<TKey, TKey> bt;

        UNIT_ASSERT_EQUAL(bt.GetCount(), 0);
        bt.CheckConsistensy();
        UNIT_ASSERT(bt.Insert(0, 0));
        bt.CheckConsistensy();
        UNIT_ASSERT_EQUAL(bt.GetCount(), 1);
        bt.CheckConsistensy();
        UNIT_ASSERT(bt.Insert(1, 1));
        bt.CheckConsistensy();
        UNIT_ASSERT_EQUAL(bt.GetCount(), 2);
        bt.CheckConsistensy();
        UNIT_ASSERT(bt.Insert(m - 1, m - 1));
        bt.CheckConsistensy();
        UNIT_ASSERT_EQUAL(bt.GetCount(), 3);
        bt.CheckConsistensy();
        UNIT_ASSERT(bt.Insert(m, m));
        bt.CheckConsistensy();
        UNIT_ASSERT_EQUAL(bt.GetCount(), 4);
        bt.CheckConsistensy();

        TKey* oldValue = nullptr;

        UNIT_ASSERT(!bt.Insert(0, 0, &oldValue));
        bt.CheckConsistensy();
        UNIT_ASSERT_EQUAL(bt.GetCount(), 4);
        bt.CheckConsistensy();
        UNIT_ASSERT_EQUAL(*oldValue, 0);
        bt.CheckConsistensy();
        UNIT_ASSERT(!bt.Insert(1, 1, &oldValue));
        bt.CheckConsistensy();
        UNIT_ASSERT_EQUAL(bt.GetCount(), 4);
        bt.CheckConsistensy();
        UNIT_ASSERT_EQUAL(*oldValue, 1);
        bt.CheckConsistensy();
        UNIT_ASSERT(!bt.Insert(m - 1, m - 1, &oldValue));
        bt.CheckConsistensy();
        UNIT_ASSERT_EQUAL(bt.GetCount(), 4);
        bt.CheckConsistensy();
        UNIT_ASSERT_EQUAL(*oldValue, m - 1);
        bt.CheckConsistensy();
        UNIT_ASSERT(!bt.Insert(m, m, &oldValue));
        bt.CheckConsistensy();
        UNIT_ASSERT_EQUAL(bt.GetCount(), 4);
        bt.CheckConsistensy();
        UNIT_ASSERT_EQUAL(*oldValue, m);
        bt.CheckConsistensy();

        UNIT_ASSERT(bt.Has(0));
        bt.CheckConsistensy();
        UNIT_ASSERT(bt.Has(1));
        bt.CheckConsistensy();
        UNIT_ASSERT(bt.Has(m - 1));
        bt.CheckConsistensy();
        UNIT_ASSERT(bt.Has(m));
        bt.CheckConsistensy();

        UNIT_ASSERT(!bt.Has(2));
        bt.CheckConsistensy();
        UNIT_ASSERT(!bt.Has(3));
        bt.CheckConsistensy();
        UNIT_ASSERT(!bt.Has(m - 3));
        bt.CheckConsistensy();
        UNIT_ASSERT(!bt.Has(m - 2));
        bt.CheckConsistensy();

        UNIT_ASSERT_EQUAL(bt.Get(0), 0);
        bt.CheckConsistensy();
        UNIT_ASSERT_EQUAL(bt.Get(1), 1);
        bt.CheckConsistensy();
        UNIT_ASSERT_EQUAL(bt.Get(m - 1), m - 1);
        bt.CheckConsistensy();
        UNIT_ASSERT_EQUAL(bt.Get(m), m);
        bt.CheckConsistensy();

        UNIT_ASSERT_EXCEPTION(bt.Get(2), yexception);
        bt.CheckConsistensy();
        UNIT_ASSERT_EQUAL(bt.Get(2, true), 0);
        bt.CheckConsistensy();
        bt.Get(2) = 2;
        bt.CheckConsistensy();
        UNIT_ASSERT_EQUAL(bt.Get(2), 2);
        bt.CheckConsistensy();
        UNIT_ASSERT_EQUAL(bt.GetCount(), 5);
        bt.CheckConsistensy();
        TKey val = 0;
        UNIT_ASSERT(bt.Delete(2, &val));
        bt.CheckConsistensy();
        UNIT_ASSERT_EQUAL(val, 2);
        bt.CheckConsistensy();
        UNIT_ASSERT_EQUAL(bt.GetCount(), 4);
        bt.CheckConsistensy();
        UNIT_ASSERT(!bt.Delete(2));
        bt.CheckConsistensy();
        UNIT_ASSERT_EQUAL(bt.GetCount(), 4);
        bt.CheckConsistensy();

        UNIT_ASSERT(bt.TryGet(0, &oldValue));
        bt.CheckConsistensy();
        UNIT_ASSERT_EQUAL(*oldValue, 0);
        bt.CheckConsistensy();
        UNIT_ASSERT(bt.TryGet(1, &oldValue));
        bt.CheckConsistensy();
        UNIT_ASSERT_EQUAL(*oldValue, 1);
        bt.CheckConsistensy();
        UNIT_ASSERT(bt.TryGet(m - 1, &oldValue));
        bt.CheckConsistensy();
        UNIT_ASSERT_EQUAL(*oldValue, m - 1);
        bt.CheckConsistensy();
        UNIT_ASSERT(bt.TryGet(m, &oldValue));
        bt.CheckConsistensy();
        UNIT_ASSERT_EQUAL(*oldValue, m);
        bt.CheckConsistensy();
        UNIT_ASSERT(!bt.TryGet(2, &oldValue));
        bt.CheckConsistensy();

        UNIT_ASSERT(bt.Delete(0, &val));
        bt.CheckConsistensy();
        UNIT_ASSERT_EQUAL(val, 0);
        bt.CheckConsistensy();
        UNIT_ASSERT_EQUAL(bt.GetCount(), 3);
        bt.CheckConsistensy();
        UNIT_ASSERT(bt.Delete(1, &val));
        bt.CheckConsistensy();
        UNIT_ASSERT_EQUAL(val, 1);
        bt.CheckConsistensy();
        UNIT_ASSERT_EQUAL(bt.GetCount(), 2);
        bt.CheckConsistensy();
        UNIT_ASSERT(bt.Delete(m - 1, &val));
        bt.CheckConsistensy();
        UNIT_ASSERT_EQUAL(val, m - 1);
        bt.CheckConsistensy();
        UNIT_ASSERT_EQUAL(bt.GetCount(), 1);
        bt.CheckConsistensy();
        UNIT_ASSERT(bt.Delete(m, &val));
        bt.CheckConsistensy();
        UNIT_ASSERT_EQUAL(val, m);
        bt.CheckConsistensy();
        UNIT_ASSERT_EQUAL(bt.GetCount(), 0);
        bt.CheckConsistensy();

        UNIT_ASSERT(!bt.Has(0));
        bt.CheckConsistensy();
        UNIT_ASSERT(!bt.Has(1));
        bt.CheckConsistensy();
        UNIT_ASSERT(!bt.Has(m - 1));
        bt.CheckConsistensy();
        UNIT_ASSERT(!bt.Has(m));
        bt.CheckConsistensy();
    }

    template <class TKey>
    void TestNeighbors(
        TCompBitTrie<TKey, TKey>& Bt,
        const THashMap<TKey, TKey>& Check,
        TKey Key,
        ui32 Distance) {
        Bt.CheckConsistensy();

        THashSet<TKey> check;
        for (typename THashMap<TKey, TKey>::const_iterator it = Check.begin();
             it != Check.end();
             ++it) {
            if (HammingDistance(Key, it->first) <= Distance) {
                UNIT_ASSERT(check.insert(it->first).second);
            }
        }
        TVector<NCompBitTrieGetters::TSimhashGetter<TKey>> test;
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

            TVector<NCompBitTrieGetters::TSimhashGetter<TKey>> group;
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

    template <class TKey>
    void RandomTest(
        const ui32 Iterations,
        const ui32 CheckPeriod,
        TCompBitTrie<TKey, TKey>& bt,
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
                bt.CheckConsistensy();

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

    template <class TKey>
    void RandomRemove(
        TCompBitTrie<TKey, TKey>& bt,
        THashMap<TKey, TKey>& check) {
        UNIT_ASSERT_EQUAL(bt.GetCount(), (ui32)check.size());

        UNIT_ASSERT(bt.Min() <= bt.Max());
        UNIT_ASSERT(bt.CMin() <= bt.CMax());

        typename TCompBitTrie<TKey, TKey>::TIt it1 = bt.Min();

        if (bt.GetCount() != 0) {
            UNIT_ASSERT(bt.Min() < bt.Max());
            UNIT_ASSERT(bt.CMin() < bt.CMax());

            {
                typename TCompBitTrie<TKey, TKey>::TConstIt cit = bt.CMin();

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
                bt.CheckConsistensy();
                UNIT_ASSERT_EQUAL(value, it2->second);
                toDelete.push_back(it2->first);
            }
        }
        for (size_t i = 0; i < toDelete.size(); ++i) {
            check.erase(toDelete[i]);
        }
        UNIT_ASSERT_EQUAL(bt.GetCount(), (ui32)check.size());
    }

    template <class TKey>
    void RandomTest() {
        TCompBitTrie<TKey, TKey> bt;
        THashMap<TKey, TKey> check;

        RandomRemove(bt, check);

        RandomTest(10, 1, bt, check);

        RandomRemove(bt, check);

        RandomTest(50, 10, bt, check);

        RandomRemove(bt, check);

        RandomTest(100, 10, bt, check);

        RandomRemove(bt, check);

        RandomTest(1000, 100, bt, check);

        RandomRemove(bt, check);

        //RandomTest(5000, 1000, bt, check);
    }
}

Y_UNIT_TEST_SUITE(TBitKurevoTest) {
    Y_UNIT_TEST(SimpleTest8) {
#ifdef _unix_
        SimpleTestHardcoded<ui8>();
#endif
    }

    Y_UNIT_TEST(SimpleTest16) {
#ifdef _unix_
        SimpleTestHardcoded<ui16>();
#endif
    }

    Y_UNIT_TEST(SimpleTest32) {
#ifdef _unix_
        SimpleTestHardcoded<ui32>();
#endif
    }

    Y_UNIT_TEST(SimpleTest64) {
#ifdef _unix_
        SimpleTestHardcoded<ui64>();
#endif
    }

    Y_UNIT_TEST(RandomTestGenerated8) {
#ifdef _unix_
        RandomTest<ui8>();
#endif
    }

    Y_UNIT_TEST(RandomTestGenerated16) {
#ifdef _unix_
        RandomTest<ui16>();
#endif
    }

    Y_UNIT_TEST(RandomTestGenerated32) {
#ifdef _unix_
        RandomTest<ui32>();
#endif
    }

    Y_UNIT_TEST(RandomTestGenerated64) {
#ifdef _unix_
        RandomTest<ui64>();
#endif
    }
}
