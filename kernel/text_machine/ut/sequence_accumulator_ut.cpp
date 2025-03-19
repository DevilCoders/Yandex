#include <kernel/text_machine/parts/accumulators/sequence_accumulator.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/random/random.h>

using namespace NTextMachine;
using namespace NTextMachine::NCore;

Y_UNIT_TEST_SUITE(TSequenceAccumulatorTest) {
    Y_UNIT_TEST(TestSimple) {
        TMemoryPool pool(32 << 10);
        TSequenceAccumulator sequenceAccumulator;
        sequenceAccumulator.Init(pool, 3);

        for (size_t counter = 0; counter < 2; ++counter) {
            UNIT_ASSERT_EQUAL(sequenceAccumulator.GetMaxSequenceLength(), 0);

            sequenceAccumulator.Update(0, 0);
            UNIT_ASSERT_EQUAL(sequenceAccumulator.GetMaxSequenceLength(), 1);

            sequenceAccumulator.Update(1, 1);
            UNIT_ASSERT_EQUAL(sequenceAccumulator.GetMaxSequenceLength(), 2);

            sequenceAccumulator.Update(2, 2);
            UNIT_ASSERT_EQUAL(sequenceAccumulator.GetMaxSequenceLength(), 3);

            sequenceAccumulator.Clear();
            UNIT_ASSERT_EQUAL(sequenceAccumulator.GetMaxSequenceLength(), 0);
        }
    }

    Y_UNIT_TEST(TestLongSequence) {
        TMemoryPool pool(32 << 10);
        TSequenceAccumulator sequenceAccumulator;
        sequenceAccumulator.Init(pool, 200);

        sequenceAccumulator.Update(0, 0);
        sequenceAccumulator.Update(1, 1);
        UNIT_ASSERT_EQUAL(sequenceAccumulator.GetMaxSequenceLength(), 2);

        sequenceAccumulator.Update(2, 100);
        UNIT_ASSERT_EQUAL(sequenceAccumulator.GetMaxSequenceLength(), 2);
        sequenceAccumulator.Update(3, 101);
        sequenceAccumulator.Update(4, 102);
        sequenceAccumulator.Update(5, 103);
        sequenceAccumulator.Update(6, 104);
        sequenceAccumulator.Update(7, 105);
        sequenceAccumulator.Update(8, 106);
        sequenceAccumulator.Update(9, 107);
        UNIT_ASSERT_EQUAL(sequenceAccumulator.GetMaxSequenceLength(), 8);

        sequenceAccumulator.Clear();
        UNIT_ASSERT_EQUAL(sequenceAccumulator.GetMaxSequenceLength(), 0);
        sequenceAccumulator.Update(1, 1);
        sequenceAccumulator.Update(8, 8);
        sequenceAccumulator.Update(2, 56);
        sequenceAccumulator.Update(3, 57);
        sequenceAccumulator.Update(4, 58);
        sequenceAccumulator.Update(5, 59);
        sequenceAccumulator.Update(6, 60);
        sequenceAccumulator.Update(7, 61);
        UNIT_ASSERT_EQUAL(sequenceAccumulator.GetMaxSequenceLength(), 6);
    }

    Y_UNIT_TEST(MultiHits) {
        TMemoryPool pool(32 << 10);
        TSequenceAccumulator sequenceAccumulator;
        sequenceAccumulator.Init(pool, 1500);

        sequenceAccumulator.Update(4, 0);
        sequenceAccumulator.Update(18, 0);
        sequenceAccumulator.Update(20, 0);
        sequenceAccumulator.Update(37, 0);
        UNIT_ASSERT_EQUAL(sequenceAccumulator.GetMaxSequenceLength(), 1);

        sequenceAccumulator.Update(97, 1);
        sequenceAccumulator.Update(30, 1);
        UNIT_ASSERT_EQUAL(sequenceAccumulator.GetMaxSequenceLength(), 1);
        sequenceAccumulator.Update(21, 1);
        UNIT_ASSERT_EQUAL(sequenceAccumulator.GetMaxSequenceLength(), 2);
        sequenceAccumulator.Update(45, 1);

        sequenceAccumulator.Update(22, 2);
        UNIT_ASSERT_EQUAL(sequenceAccumulator.GetMaxSequenceLength(), 3);
        sequenceAccumulator.Update(55, 2);
        sequenceAccumulator.Update(66, 2);

        sequenceAccumulator.Update(1001, 3);
        sequenceAccumulator.Update(1002, 3);
        sequenceAccumulator.Update(1003, 3);
        UNIT_ASSERT_EQUAL(sequenceAccumulator.GetMaxSequenceLength(), 3);
        sequenceAccumulator.Update(23, 3);
        UNIT_ASSERT_EQUAL(sequenceAccumulator.GetMaxSequenceLength(), 4);
    }

    Y_UNIT_TEST(FullHits) {
        TMemoryPool pool(32 << 10);
        TSequenceAccumulator sequenceAccumulator;

        for (size_t n = 1; n <= 10; ++n) {
            sequenceAccumulator.Init(pool, n);
            for (size_t i = 0; i != n; ++i) {
                for (size_t j = 0; j != n; ++j) {
                    sequenceAccumulator.Update(j, i);
                }
            }
            UNIT_ASSERT_EQUAL(sequenceAccumulator.GetMaxSequenceLength(), n);
        }
    }

    Y_UNIT_TEST(FullHitsShifted) {
        TMemoryPool pool(32 << 10);
        TSequenceAccumulator sequenceAccumulator;

        for (size_t n = 1; n <= 10; ++n) {
            sequenceAccumulator.Init(pool, n);
            for (size_t i = 0; i != n; ++i) {
                for (size_t j = 0; j != n; ++j) {
                    sequenceAccumulator.Update((j + i) % n, i);
                }
            }
            UNIT_ASSERT_EQUAL(sequenceAccumulator.GetMaxSequenceLength(), n);
        }
    }

    Y_UNIT_TEST(FullHitsFlip) {
        TMemoryPool pool(32 << 10);
        TSequenceAccumulator sequenceAccumulator;

        for (size_t n = 1; n <= 10; ++n) {
            sequenceAccumulator.Init(pool, n);
            for (size_t i = 0; i != n; ++i) {
                const bool flip = i % 2 == 0;
                for (size_t j = 0; j != n; ++j) {
                    sequenceAccumulator.Update((flip ? n - j: j) % n, i);
                }
            }
            UNIT_ASSERT_EQUAL(sequenceAccumulator.GetMaxSequenceLength(), n);
        }
    }
};
