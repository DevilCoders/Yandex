#include "inthash.h"

#include <library/cpp/testing/unittest/registar.h>

class TIntHashTest: public TTestBase
{
    UNIT_TEST_SUITE(TIntHashTest);
        UNIT_TEST(Test);
    UNIT_TEST_SUITE_END();

public:
    void Test();
};

UNIT_TEST_SUITE_REGISTRATION(TIntHashTest);

void TIntHashTest::Test()
{
    // generic tests for correctness
    TIntHash<int, 0x1000>::TPoolType pool(256);
    TIntHash<int, 0x1000> hash(&pool);
    TIntHash<int, 0x1000>::iterator it;
    UNIT_ASSERT(hash.size() == 0);
    UNIT_ASSERT(hash.begin() == hash.end());

    // add one element
    hash.insert(std::make_pair(4, -1));
    UNIT_ASSERT(hash.size() == 1);
    it = hash.begin();
    UNIT_ASSERT(it != hash.end());
    UNIT_ASSERT(it->first == 4);
    UNIT_ASSERT(it->second == -1);
    UNIT_ASSERT(hash.find(4) == it);
    UNIT_ASSERT(++it == hash.end());

    // add second element, no collision
    hash.insert(std::make_pair(2, 3));
    it = hash.find(4);
    UNIT_ASSERT(it != hash.end());
    UNIT_ASSERT(it->first == 4 && it->second == -1);
    it = hash.find(2);
    UNIT_ASSERT(it != hash.end());
    UNIT_ASSERT(it->first == 2 && it->second == 3);

    // add third element with collision
    hash.insert(std::make_pair(0x10004, 42));
    it = hash.find(4);
    UNIT_ASSERT(it != hash.end() && it->first == 4 && it->second == -1);
    it = hash.find(0x10004);
    UNIT_ASSERT(it != hash.end() && it->first == 0x10004 && it->second == 42);

    // walk through the entire hash
    bool was1 = false, was2 = false, was3 = false;
    for (it = hash.begin(); it != hash.end(); ++it) {
        if (it->first == 2 && it->second == 3) {
            UNIT_ASSERT(!was1);
            was1 = true;
        } else if (it->first == 4 && it->second == -1) {
            UNIT_ASSERT(!was2);
            was2 = true;
        } else if (it->first == 0x10004 && it->second == 42) {
            UNIT_ASSERT(!was3);
            was3 = true;
        } else
            UNIT_ASSERT(false);
    }
    UNIT_ASSERT(was1 && was2 && was3);
}
