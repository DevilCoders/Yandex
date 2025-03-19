#include <kernel/dups/banned_info/dense_vector_pool.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/hash.h>
#include <util/generic/string.h>
#include <util/random/random.h>
#include <util/generic/algorithm.h>

using namespace NDups;

Y_UNIT_TEST_SUITE(TDenseVectorPoolTest) {

    static bool Intersects(const TArrayRef<ui32> a, const TArrayRef<ui32> b) {
        const auto* sa = a.data();
        const auto* ea = a.data() + a.size();

        const auto* sb = b.data();
        const auto* eb = b.data() + b.size();

        return (sb < ea) && (sa < eb);
    }

    static bool HasInnerIntersection(TVector<TArrayRef<ui32>> arrays) {
        SortBy(arrays, [](const auto& ref) -> const ui32* { return ref.data(); });

        for (size_t i = 1; i < arrays.size(); ++i) {
            const auto& prev = arrays[i - 1];
            const auto& curr = arrays[i - 0];

            if (Intersects(prev, curr)) {
                return true;
            }
        }
        return 0;
    }

    Y_UNIT_TEST(TestAllocation) {
        TMemoryPool pool{1024};
        TAppendOnlyDenseVectorPool dvp{&pool};

        TVector<TArrayRef<ui32>> arrays;
        size_t itemsAllocated = 0;
        ui64 hashSum = 0;
        for (size_t i = 0; i < 10000; ++i) {
            const size_t length = RandomNumber<size_t>(384);
            TArrayRef<ui32> array{};
            TArrayRef<ui32> fill = dvp.GrowArray(array, length);
            UNIT_ASSERT_VALUES_EQUAL(length, fill.size());
            UNIT_ASSERT_VALUES_EQUAL(length, array.size());

            itemsAllocated += length;

            for (ui32& e : fill) {
                const ui32 random = RandomNumber<ui32>();

                e = random;
                hashSum += random;
            }
            arrays.push_back(array);
        }
        UNIT_ASSERT(pool.MemoryAllocated() >= itemsAllocated * sizeof(ui32));

        UNIT_ASSERT_VALUES_EQUAL(HasInnerIntersection(arrays), false);

        UNIT_ASSERT_VALUES_EQUAL(dvp.WastedItems(), 0);
        UNIT_ASSERT_VALUES_EQUAL(dvp.WastedBytes(), 0);

        auto calcHashSum = [&arrays]() -> ui64 {
            ui64 r = 0;
            for (const auto& array : arrays) {
                for (const ui32 e : array) {
                    r += e;
                }
            }
            return r;
        };

        UNIT_ASSERT_VALUES_EQUAL(hashSum, calcHashSum());
    }

    Y_UNIT_TEST(Triangle) {
        TMemoryPool pool{1024};
        TAppendOnlyDenseVectorPool dvp{&pool};

        const size_t topSize = 200;
        TArrayRef<ui32> array{};
        while(array.size() < topSize) {
            TArrayRef<ui32> fill = dvp.GrowArray(array, 1);

            UNIT_ASSERT_VALUES_EQUAL(fill.size(), 1);
            UNIT_ASSERT_VALUES_EQUAL(fill.begin(), array.end() - 1);
            UNIT_ASSERT_VALUES_EQUAL(fill.end(), array.end());
            UNIT_ASSERT(Intersects(fill, array));
        }

        UNIT_ASSERT_VALUES_EQUAL(dvp.WastedItems(), topSize * (topSize - 1) / 2);
    }

    Y_UNIT_TEST(Ladder) {
        TMemoryPool pool{1024};
        TAppendOnlyDenseVectorPool dvp{&pool};

        TVector<TArrayRef<ui32>> arrays;
        TVector<ui32> oldDataStorage;
        for (size_t topSize = 200; topSize > 0; --topSize) {
            TArrayRef<ui32> array{};

            while(array.size() < topSize) {
                oldDataStorage.assign(array.begin(), array.end());
                TArrayRef<ui32> fill = dvp.GrowArray(array, 1);
                UNIT_ASSERT_VALUES_EQUAL(fill.size(), 1);
                UNIT_ASSERT_VALUES_EQUAL(array.size() - oldDataStorage.size(), 1);
                for (ui32& e : fill) {
                    e = RandomNumber<ui32>();
                }
                for (size_t i = 0; i < oldDataStorage.size(); ++i) {
                    UNIT_ASSERT_VALUES_EQUAL(oldDataStorage[i], array[i]);
                }
            }

            arrays.push_back(array);
        }
        UNIT_ASSERT_VALUES_EQUAL(HasInnerIntersection(arrays), false);

        UNIT_ASSERT_VALUES_EQUAL(dvp.WastedItems(), 0);
        UNIT_ASSERT_VALUES_EQUAL(dvp.WastedBytes(), 0);
    }
}
