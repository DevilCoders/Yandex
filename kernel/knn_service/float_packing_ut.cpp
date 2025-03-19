#include "float_packing.h"
#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/dot_product/dot_product.h>
#include <util/generic/xrange.h>

Y_UNIT_TEST_SUITE(KnnServiceFloatPacking) {
    Y_UNIT_TEST(floatToIntUniformPacking) {
        TVector<float> normedEmbedExample = {-0.05469203740358353, 0.1234554797410965, -0.016504745930433273, 0.09876885265111923, -0.04398241266608238, 0.0021703788079321384, -0.060937728732824326, -0.04892570897936821, -0.06509890407323837, -0.024675793945789337, 0.17165566980838776, -0.09608916938304901, -0.1560257375240326, 0.18542790412902832, -0.12616029381752014, 0.15272143483161926, -0.04579543694853783, -0.1564762145280838, -0.020142175257205963, 0.04538711532950401, -0.08162152767181396, 0.120390884578228, 0.16501682996749878, -0.5702167749404907, 0.2055046558380127, -0.029223136603832245, -0.12732434272766113, -0.08151092380285263, 0.10432847589254379, 0.24336209893226624, -0.17697641253471375, 0.007422665134072304, -0.18230079114437103, -0.03159422054886818, -0.07466048747301102, 0.006565173156559467, 0.08074681460857391, 0.12942616641521454, -0.1912728250026703, -0.2203434705734253, -0.0334734208881855, -0.035811375826597214, -0.10196355730295181, -0.030233560130000114, -0.16046471893787384, 0.15276025235652924, -0.17491652071475983, -0.001575459260493517, 0.023708155378699303, 0.09978161752223969};

        {

            TVector<float> PackUnpack = NKnnService::UnpackEmbedsBase64(
                NKnnService::PackEmbedsBase64(MakeArrayRef(normedEmbedExample))
            );
            UNIT_ASSERT_VALUES_EQUAL(PackUnpack.size(), normedEmbedExample.size());
            for (auto i : xrange(PackUnpack.size())) {
                UNIT_ASSERT_VALUES_EQUAL(PackUnpack[i], normedEmbedExample[i]);
            }
        }

        float blDotProduct = DotProduct(normedEmbedExample.begin(), normedEmbedExample.begin(), normedEmbedExample.size());
        UNIT_ASSERT_DOUBLES_EQUAL(blDotProduct, 1, 1e-7);

        auto calcOldUniformDotProductRes = [](const TVector<float>& x) {
            i32 res = 0;
            for(float v : x) {
                i8 mapped = (Max<i8>() * v);
                res += i32(mapped) * i32(mapped);
            }
            return float(res) / Max<i8>() / Max<i8>();
        };
        float oldUniformDotProduct = calcOldUniformDotProductRes(normedEmbedExample);
        UNIT_ASSERT_DOUBLES_EQUAL(oldUniformDotProduct, 0.96, 1e-3);

        auto calcNaiveUniformDotProductRes = [](const TVector<float>& x) {
            float res = 0;
            for(float v : x) {
                float mapped = i8(Max<i8>() * v) / float(Max<i8>());
                res += mapped * mapped;
            }
            return float(res);
        };
        float naiveUniformDotProduct = calcNaiveUniformDotProductRes(normedEmbedExample);
        UNIT_ASSERT_DOUBLES_EQUAL(naiveUniformDotProduct, 0.96, 1e-3);



        auto calcRoundedUniformDotProductRes = [](const TVector<float>& x) {
            i32 res = 0;
            for(float v : x) {
                i8 mapped = NKnnService::FloatPacking::Pack<i8>(v);
                res += i32(mapped) * i32(mapped);
            }
            return std::sqrt(float(res)) / Max<i8>();
        };
        float roundedUniformDotProduct = calcRoundedUniformDotProductRes(normedEmbedExample);
        UNIT_ASSERT_DOUBLES_EQUAL(roundedUniformDotProduct, 1, 1e-2);
    }

    Y_UNIT_TEST(DistancePackUnpack) {
        for (float v : {0.1, 1., -1., 0., -0.1, 0.5, 0.9, 1e-15, 1e-3, 1e-9}) {
            UNIT_ASSERT_DOUBLES_EQUAL(
                NKnnService::IntegerToDistance(NKnnService::DistanceToInteger(v)), v, 1e-5);
        }

        for (float v : {10, 5, -10, -5}) {
            UNIT_ASSERT_EXCEPTION(
                NKnnService::DistanceToInteger(v), yexception);
        }
    }
}

Y_UNIT_TEST_SUITE(MinMaxFloatPacking) {
    Y_UNIT_TEST(SimplePackTest) {
        UNIT_ASSERT(NKnnService::PackMinMax<ui8>(3.0) == Max<ui8>());

        UNIT_ASSERT(NKnnService::PackMinMax<ui8>(-1.0) == Min<ui8>());

        UNIT_ASSERT(NKnnService::PackMinMax<ui8>(0.5) == 127);

        UNIT_ASSERT(NKnnService::PackMinMax<ui8>(4.0, 0.0, 10.0) == 102);
    }

    Y_UNIT_TEST(SimpleUnpackTest) {
        double eps = 0.0001;
        UNIT_ASSERT_DOUBLES_EQUAL(NKnnService::UnpackMinMax<ui8>(3, 5, 10), 5.0588, eps);

        UNIT_ASSERT_DOUBLES_EQUAL(NKnnService::UnpackMinMax<ui8>(7), 0.0274, eps);

        UNIT_ASSERT_DOUBLES_EQUAL(NKnnService::UnpackMinMax<ui8>(254, 0, 10), 9.9607, eps);
    }

    Y_UNIT_TEST(CombinePackTest) {
        double eps = 0.1;
        float step = 0.25;
        float left = 0;
        float right = 10;
        ui8 packed;

        for (float val = left; val <= right; val += step) {
            packed = NKnnService::PackMinMax<ui8>(val, left, right);
            UNIT_ASSERT_DOUBLES_EQUAL(NKnnService::UnpackMinMax<ui8>(packed, left, right), val, eps);
        }
    }
}
