#include "offroad_keyinv_wad_io_ut.h"

#include <library/cpp/testing/unittest/registar.h>

Y_UNIT_TEST_SUITE(TestOffroadKeyInvWad) {
    Y_UNIT_TEST(Simple) {
        TIndexGenerator generator;
        TIndexGenerator::TReader reader = generator.GetReader();

        for (const auto& entry : generator.GetIndex()) {
            if (entry.second.empty()) {
                continue;
            }
            TStringBuf key;
            UNIT_ASSERT(reader.ReadKey(&key));
            UNIT_ASSERT_EQUAL(entry.first, key);
            Y_ENSURE(reader.NextLayer());
            for (const TPantherHit& expectedHit : entry.second) {
                TPantherHit hit;
                UNIT_ASSERT(reader.ReadHit(&hit));
                UNIT_ASSERT_EQUAL(expectedHit, hit);
            }
            TPantherHit hit;
            UNIT_ASSERT(!reader.ReadHit(&hit));
            Y_ENSURE(!reader.NextLayer());
        }
        TStringBuf key;
        UNIT_ASSERT(!reader.ReadKey(&key));
    }
}
