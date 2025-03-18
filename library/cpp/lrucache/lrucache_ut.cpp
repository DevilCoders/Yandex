#include "lrucache.h"

#include <library/cpp/testing/unittest/registar.h>


void InitValue(bool* initCalled, ui64 key, ui64& setTo) {
    *initCalled = true;
    setTo = key;
}

bool SetAndCheckIfHad(TLRUCache<ui64, ui64>& cache, ui64 key) {
    bool initCalled = false;
    auto func = std::bind(&InitValue, &initCalled, std::placeholders::_1, std::placeholders::_2);
    cache.FindOrCalc(key, func);
    return !initCalled;
}

Y_UNIT_TEST_SUITE(lrucache_ut)
{
    Y_UNIT_TEST(objects_with_same_hash)
    {
        TLRUCache<ui64, ui64> cache;
        cache.Init(2, 4);

        UNIT_ASSERT(SetAndCheckIfHad(cache, 1) == false);
        UNIT_ASSERT(SetAndCheckIfHad(cache, 9) == false);
        UNIT_ASSERT(SetAndCheckIfHad(cache, 1) == true);
    }

    Y_UNIT_TEST(storage_size_correspondence)
    {
        TLRUCache<ui64, ui64> cache;
        cache.Init(2, 4);

        UNIT_ASSERT(SetAndCheckIfHad(cache, 0) == false);
        UNIT_ASSERT(SetAndCheckIfHad(cache, 1) == false);
        UNIT_ASSERT(SetAndCheckIfHad(cache, 2) == false);
        UNIT_ASSERT(SetAndCheckIfHad(cache, 3) == false);

        UNIT_ASSERT(SetAndCheckIfHad(cache, 0) == true);
        UNIT_ASSERT(SetAndCheckIfHad(cache, 1) == true);
        UNIT_ASSERT(SetAndCheckIfHad(cache, 2) == true);
        UNIT_ASSERT(SetAndCheckIfHad(cache, 3) == true);
    }

    Y_UNIT_TEST(delete_correct_one)
    {
        TLRUCache<ui64, ui64> cache;
        cache.Init(2, 4);

        UNIT_ASSERT(SetAndCheckIfHad(cache, 0) == false);
        UNIT_ASSERT(SetAndCheckIfHad(cache, 1) == false);
        UNIT_ASSERT(SetAndCheckIfHad(cache, 4) == false);
        UNIT_ASSERT(SetAndCheckIfHad(cache, 5) == false);

        UNIT_ASSERT(SetAndCheckIfHad(cache, 1) == true);
        UNIT_ASSERT(SetAndCheckIfHad(cache, 5) == true);
        UNIT_ASSERT(SetAndCheckIfHad(cache, 0) == true);

        UNIT_ASSERT(SetAndCheckIfHad(cache, 2) == false);
        UNIT_ASSERT(SetAndCheckIfHad(cache, 3) == false);

        UNIT_ASSERT(SetAndCheckIfHad(cache, 0) == true);
        UNIT_ASSERT(SetAndCheckIfHad(cache, 5) == true);
        UNIT_ASSERT(SetAndCheckIfHad(cache, 2) == true);
        UNIT_ASSERT(SetAndCheckIfHad(cache, 3) == true);

        UNIT_ASSERT(SetAndCheckIfHad(cache, 4) == false);
        UNIT_ASSERT(SetAndCheckIfHad(cache, 0) == false);
    }
};
