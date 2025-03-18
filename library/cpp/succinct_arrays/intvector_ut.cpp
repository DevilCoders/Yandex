#include "intvector.h"

#include <library/cpp/testing/unittest/registar.h>
#include <util/memory/blob.h>
#include <util/stream/buffer.h>
#include <util/random/random.h>

using namespace NSuccinctArrays;

Y_UNIT_TEST_SUITE(TIntVectorTest) {
    Y_UNIT_TEST(Test2) {
        TVector<ui16> data(84517);
        TIntVector iv(data.size(), 12);
        for (size_t i = 0; i < data.size(); ++i) {
            data[i] = RandomNumber<ui16>();
            iv.Set(i, data[i]);
        }
        for (size_t i = 0; i < data.size(); ++i) {
            UNIT_ASSERT_EQUAL(data[i] & 0xFFF, static_cast<ui16>(iv[i]));
        }
    }

    Y_UNIT_TEST(TestReadonlyIntVector) {
        TIntVector v(100, 5);
        for (ui64 i = 0; i < 100; ++i) {
            v.Set(i, i % 32);
        }
        TBufferStream bs;
        TReadonlyIntVector::SaveForReadonlyAccess(&bs, v);
        const auto blob = TBlob::FromBuffer(bs.Buffer());
        TReadonlyIntVector rv;
        rv.LoadFromBlob(blob);
        for (ui64 i = 0; i < 100; ++i) {
            UNIT_ASSERT_VALUES_EQUAL(rv.Get(i), i % 32);
        }
    }
}
