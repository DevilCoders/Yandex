#include "consistent_hash_ring.h"

#include <library/cpp/testing/unittest/registar.h>

Y_UNIT_TEST_SUITE(ConsistentHashRing) {
    Y_UNIT_TEST(AddNode) {
        TConsistentHashRing<int> hashRing;
        hashRing.AddNode(1);
        auto k1 = 0;
        auto k2 = 0xFF;
        auto k3 = 0xFFFF;
        auto k4 = 0xFFFFFFFF;
        auto k5 = 0xFFFFFFFFFFFFFFFF;
        UNIT_ASSERT_EQUAL(hashRing.FindNode(k1), 1);
        UNIT_ASSERT_EQUAL(hashRing.FindNode(k2), 1);
        UNIT_ASSERT_EQUAL(hashRing.FindNode(k3), 1);
        UNIT_ASSERT_EQUAL(hashRing.FindNode(k4), 1);
        UNIT_ASSERT_EQUAL(hashRing.FindNode(k5), 1);
    }

    Y_UNIT_TEST(DisableNode) {
        TConsistentHashRing<int> hashRing;
        hashRing.AddNode(1);
        hashRing.AddNode(2);

        hashRing.DisableNode(1);

        auto k1 = 0;
        auto k2 = 0xFF;
        auto k3 = 0xFFFF;
        auto k4 = 0xFFFFFFFF;
        auto k5 = 0xFFFFFFFFFFFFFFFF;
        UNIT_ASSERT_EQUAL(hashRing.FindNode(k1), 2);
        UNIT_ASSERT_EQUAL(hashRing.FindNode(k2), 2);
        UNIT_ASSERT_EQUAL(hashRing.FindNode(k3), 2);
        UNIT_ASSERT_EQUAL(hashRing.FindNode(k4), 2);
        UNIT_ASSERT_EQUAL(hashRing.FindNode(k5), 2);
    }

    Y_UNIT_TEST(EnableNode) {
        TConsistentHashRing<int> hashRing;
        hashRing.AddNode(1);
        hashRing.DisableNode(1);

        auto k = 0xFFFFFFFF;

        UNIT_ASSERT_EQUAL(hashRing.FindNode(k), 0);

        hashRing.EnableNode(1);

        UNIT_ASSERT_EQUAL(hashRing.FindNode(k), 1);
    }

    Y_UNIT_TEST(Stability) {
        const int ringsCount = 20;

        TVector<TConsistentHashRing<int>> hashRings(ringsCount, TConsistentHashRing<int>());
        for (int i = 0; i < ringsCount; ++i) {
            for (int n = 0; n <= i; ++n) {
                hashRings[i].AddNode(n);
            }
        }

        ui64 hash = ULL(0xDEADBEEFDEADBEEF);

        UNIT_ASSERT_EQUAL(hashRings[0].FindNode(hash), 0);
        UNIT_ASSERT_EQUAL(hashRings[1].FindNode(hash), 1);
        UNIT_ASSERT_EQUAL(hashRings[2].FindNode(hash), 0);
        UNIT_ASSERT_EQUAL(hashRings[3].FindNode(hash), 2);
        UNIT_ASSERT_EQUAL(hashRings[4].FindNode(hash), 0);
        UNIT_ASSERT_EQUAL(hashRings[5].FindNode(hash), 1);
        UNIT_ASSERT_EQUAL(hashRings[6].FindNode(hash), 1);
        UNIT_ASSERT_EQUAL(hashRings[7].FindNode(hash), 4);
        UNIT_ASSERT_EQUAL(hashRings[8].FindNode(hash), 6);
        UNIT_ASSERT_EQUAL(hashRings[9].FindNode(hash), 0);
        UNIT_ASSERT_EQUAL(hashRings[10].FindNode(hash), 5);
        UNIT_ASSERT_EQUAL(hashRings[11].FindNode(hash), 8);
        UNIT_ASSERT_EQUAL(hashRings[12].FindNode(hash), 4);
        UNIT_ASSERT_EQUAL(hashRings[13].FindNode(hash), 3);
        UNIT_ASSERT_EQUAL(hashRings[14].FindNode(hash), 14);
        UNIT_ASSERT_EQUAL(hashRings[15].FindNode(hash), 15);
        UNIT_ASSERT_EQUAL(hashRings[16].FindNode(hash), 13);
        UNIT_ASSERT_EQUAL(hashRings[17].FindNode(hash), 6);
        UNIT_ASSERT_EQUAL(hashRings[18].FindNode(hash), 13);
        UNIT_ASSERT_EQUAL(hashRings[19].FindNode(hash), 6);
    }
}
