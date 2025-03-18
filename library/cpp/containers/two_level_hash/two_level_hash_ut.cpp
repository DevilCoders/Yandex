#include "two_level_hash.h"

#include <util/generic/string.h>
#include <util/generic/ptr.h>

#include <library/cpp/testing/unittest/registar.h>

Y_UNIT_TEST_SUITE(TwoLevelHashTest) {
    const size_t BucketSize = 256;

    Y_UNIT_TEST(CharKey) {
        typedef TTwoLevelHash<char, TString> maptype;

        maptype m;
        // Store mappings between roman numerals and decimals
        m['l'] = "50";
        m['x'] = "20"; // Deliberate mistake
        m['v'] = "5";
        m['i'] = "1";
        UNIT_ASSERT_EQUAL(m['x'], TString("20"));
        m['x'] = "10"; // Correct mistake
        UNIT_ASSERT_EQUAL(m['x'], TString("10"));

        UNIT_ASSERT(!m.FindPtr('z'));
        UNIT_ASSERT_EQUAL(m['z'], TString());
        UNIT_ASSERT(m.FindPtr('z'));
    }

    Y_UNIT_TEST(SameHashValuedKeys) {
        TTwoLevelHash<ui32, ui32, BucketSize> hash;
        hash[0] = 100500;
        hash[BucketSize] = 100501;
        UNIT_ASSERT(hash.FindPtr(256));
        const ui32* val = hash.FindPtr(256);
        if (val)
            UNIT_ASSERT_EQUAL(*val, 100501);
    }

    Y_UNIT_TEST(CollideNumTries) {
        TTwoLevelHash<i32, ui32, BucketSize> hash;

        // 5 > NUM_TRIES
        for (size_t i = 0; i < 5; ++i)
            hash[i * BucketSize] = ui32(100500 + i);

        for (size_t i = 0; i < 5; ++i)
            UNIT_ASSERT_EQUAL(*hash.FindPtr(i * BucketSize), ui32(100500 + i));
    }

    Y_UNIT_TEST(SignedKeys) {
        TTwoLevelHash<i32, ui32, BucketSize> hash;

        // 5 > NUM_TRIES
        for (size_t i = 0; i < 5; ++i)
            hash[-i * BucketSize] = ui32(100500 + i);

        for (size_t i = 0; i < 5; ++i)
            UNIT_ASSERT_EQUAL(*hash.FindPtr(-i * BucketSize), ui32(100500 + i));
    }

    Y_UNIT_TEST(SizeTest) {
        TTwoLevelHash<ui32, ui32, BucketSize> hash;
        UNIT_ASSERT_EQUAL(hash.Size(), 0);
        hash[0] = 100500;
        hash[BucketSize] = 100501;
        UNIT_ASSERT_EQUAL(hash.Size(), 2);
        for (ui32 k = 0; k < 100500; ++k)
            hash[k] = k;
        UNIT_ASSERT_EQUAL(hash.Size(), 100500);

        hash.Clear();
        UNIT_ASSERT_EQUAL(hash.Size(), 0);
    }

    Y_UNIT_TEST(IteratorTest) {
        TTwoLevelHash<int, int> hash;

        for (int i = 0; i < 4096; i++)
            hash[i] = i;

        int iterations = 0;
        for (auto it = hash.Begin(); it != hash.End(); ++it) {
            UNIT_ASSERT_EQUAL(it.First(), it.Second());
            iterations++;
        }

        UNIT_ASSERT_EQUAL(iterations, 4096);
    }

    Y_UNIT_TEST(ClearTest) {
        TTwoLevelHash<int, TSimpleSharedPtr<int>> hash;
        TSimpleSharedPtr<int> value(new int(0));

        long refCount = value.RefCount();

        for (int i = 0; i < 4096; i++)
            hash[i] = value;
        hash.Clear();

        /* Check that destructors for all shared pointers were actually called. */
        UNIT_ASSERT_EQUAL(value.RefCount(), refCount);
    }

} // Y_UNIT_TEST_SUITE
