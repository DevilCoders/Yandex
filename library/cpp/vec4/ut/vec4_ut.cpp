#include <library/cpp/vec4/vec4.h>

#include <library/cpp/testing/unittest/registar.h>

Y_UNIT_TEST_SUITE(Vec4Test){
    void CheckConversions(TVec4f x, TVec4<int> i, TVec4<int> t){
        Cdbg << x << ".NearbyInt() == " << x.NearbyInt() << Endl;
UNIT_ASSERT_EQUAL(x.NearbyInt(), i);
Cdbg << x << ".Truncate() == " << x.Truncate() << Endl;
UNIT_ASSERT_EQUAL(x.Truncate(), t);
}

Y_UNIT_TEST(TestIntConversions) {
    CheckConversions(
        TVec4f(0.0f, 0.0f, 0.0f, 0.0f),
        TVec4<int>(0, 0, 0, 0),
        TVec4<int>(0, 0, 0, 0));

    CheckConversions(
        TVec4f(0.3f, 0.6f, 0.5f - 1e-6f, 0.5f + 1e-6f),
        TVec4<int>(0, 1, 0, 1),
        TVec4<int>(0, 0, 0, 0));

    CheckConversions(
        TVec4f(-3.1f, -0.6f, 3.1f, 0.6f),
        TVec4<int>(-3, -1, 3, 1),
        TVec4<int>(-3, 0, 3, 0));
}

Y_UNIT_TEST(TestPack) {
    TVec4u a(1, 2, 3, 4);
    TVec4u b(5, 6, 7, 8);
    TVec4u c(9, 10, 11, 12);
    TVec4u d(13, 14, 15, 16);
    ui8 res[16];
    Pack(a, b, c, d).Store(res);
    ui8 cmp[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};

    for (size_t i = 0; i < 16; ++i) {
        UNIT_ASSERT_VALUES_EQUAL(res[i], cmp[i]);
    }
}

Y_UNIT_TEST(TestPack2) {
    TVec4u a(1, 100500, 3, 4);
    TVec4u b(5, 6, 100500, 8);
    TVec4u c(256, 256, 11, 12);
    TVec4u d(13, 255, 255, 256);
    ui8 res[16];
    Pack(a, b, c, d).Store(res);
    ui8 cmp[16] = {1, 255, 3, 4, 5, 6, 255, 8, 255, 255, 11, 12, 13, 255, 255, 255};

    for (size_t i = 0; i < 16; ++i) {
        UNIT_ASSERT_VALUES_EQUAL(res[i], cmp[i]);
    }
}

Y_UNIT_TEST(TestSum) {
    TVec4f v1(0, 1, 2, 3);
    TVec4f v2(1, 2, 3, 4);

    auto x = v1 + v2;

    float v[4];

    x.Store(v);

    UNIT_ASSERT_VALUES_EQUAL(v[0], 1);
    UNIT_ASSERT_VALUES_EQUAL(v[1], 3);
    UNIT_ASSERT_VALUES_EQUAL(v[2], 5);
    UNIT_ASSERT_VALUES_EQUAL(v[3], 7);
}

Y_UNIT_TEST(TestFastInvSqrt) {
    TVector<float> vals;
    float val = 1e-30;
    while (val < 1e30) {
        val = 1.47821 * val * (1 + 0.141461 * cos(val + 1 / val));
        vals.push_back(val);
    }
    for (size_t i = 0; i + 4 <= vals.size(); i += 4) {
        TVec4f v(vals.data() + i);
        float approxInvSqrt[4];
        v.FastInvSqrt().Store(approxInvSqrt);
        for (size_t j = 0; j < 4; ++j) {
            double trueInvSqrt = 1.0 / sqrt(vals[i + j]);
            UNIT_ASSERT_DOUBLES_EQUAL(trueInvSqrt, approxInvSqrt[j], trueInvSqrt / 2048.0);
            // is not none
            UNIT_ASSERT(approxInvSqrt[j] == 0 || approxInvSqrt[j] != 0);
        }
    }
}
}
;
