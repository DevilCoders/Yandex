#include "embeddings.h"

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/vector.h>


using namespace NUnstructuredFeatures;

namespace {
    void AssertValues(const TVector<float>& a, const TVector<float>& b) {
        for (size_t i = 0; i < a.size(); i++) {
            UNIT_ASSERT_DOUBLES_EQUAL(a[i], b[i], std::numeric_limits<float>::epsilon());
        }
    }
}

Y_UNIT_TEST_SUITE(Embeddings) {
    Y_UNIT_TEST(AssignTest) {
        TEmbedding e1;
        e1.Assign(TVector<float>(4, 1.0f));
        AssertValues(e1.NormVec, TVector<float>(4, 0.5f));

        TEmbedding e2;
        e2.Assign({0.0f, 1e-15f});
        AssertValues(e2.NormVec, TVector<float>(2, 0.0f));
    }

    Y_UNIT_TEST(SumTest) {
        TVector<TEmbedding> v;
        TEmbedding e;
        e.Assign({0.178f, 0.443f, 0.343f, 0.063f, -0.355f});
        v.push_back(e);
        e.Assign({-0.160f, -0.320f, 0.163f, 0.136f, -0.330f});
        v.push_back(e);
        e.Assign({-0.012f, -0.413f, 0.094f, 0.261f, -0.361f});
        v.push_back(e);

        Sum(v, e, 5);
        AssertValues(e.Vec, {0.00600000657f, -0.289999992f, 0.599999964f, 0.460000008f, -1.046f});
        AssertValues(e.NormVec, {0.00453577051f, -0.21922867f, 0.453576535f, 0.347742051f, -0.790735126f});

        Sum(TVector<TEmbedding>(), e, 7);
        AssertValues(e.Vec, TVector<float>(7, 0.0f));
        AssertValues(e.NormVec, TVector<float>(7, 0.0f));
    }

    Y_UNIT_TEST(PartialEmbeddingSumTest) {
        TVector<TEmbedding> v;
        TEmbedding e;
        e.Assign({0.178f, 0.443f, 0.343f, 0.063f, -0.355f});
        v.push_back(e);
        e.Assign({-0.160f, -0.320f, 0.163f, 0.136f, -0.330f});
        v.push_back(e);
        e.Assign({-0.012f, -0.413f, 0.094f, 0.261f, -0.361f});
        v.push_back(e);

        TPartialEmbeddingSum partialSum(v, 5);

        {
            TEmbedding sum;
            partialSum.GetSum(2, 0, sum);
            AssertValues(sum.Vec, TVector<float>(5, 0.0f));
            AssertValues(sum.NormVec, TVector<float>(5, 0.0f));
        }
        {
            TEmbedding sum;
            partialSum.GetSum(1, 2, sum);
            AssertValues(sum.Vec, {-0.171999991f, -0.73299998f, 0.256999969f, 0.397000015f, -0.690999985f});
            AssertValues(sum.NormVec, {-0.152744919f, -0.650942028f, 0.228229299f, 0.352556616f, -0.613643825f});
        }
        {
            TEmbedding sum;
            partialSum.GetSum(0, 3, sum);
            AssertValues(sum.Vec, {0.00600000657f, -0.289999992f, 0.599999964f, 0.460000008f, -1.046f});
            AssertValues(sum.NormVec, {0.00453577051f, -0.21922867f, 0.453576535f, 0.347742051f, -0.790735126f});
        }
    }

    Y_UNIT_TEST(SubtractTest) {
        TEmbedding e1, e2;
        e1.Assign({2.261f, -51.6781f, 0.0342f, -21.435f});
        e2.Assign({-31.00328f, -1.81002f, 14.7f, 8.10019f});
        Subtract(e1, e2);

        AssertValues(e1.Vec, {33.2642822f, -49.8680801f, -14.6658001f, -29.5351906f});
        AssertValues(e1.NormVec, {0.486206055f, -0.72889483f, -0.214362085f, -0.431699961f});
    }

    Y_UNIT_TEST(MultiplyTest) {
        TEmbedding e1, e2, e3;
        e1.Assign({-11.111f, 2.222f, -333.333f, 0.004444f});
        Multiply(e1, 2.0);
        AssertValues(e1.Vec, {-22.2220001f, 4.44399977f, -666.666016f, 0.00888799969f});
        AssertValues(e1.NormVec, {-0.0333137885f, 0.00666215783f, -0.999422729f, 1.33243157e-05f});

        e2.Assign({-11.111f, 2.222f, -333.333f, 0.004444f});
        Multiply(e2, -3.531);
        AssertValues(e2.Vec, {39.2329407f, -7.84588146f, 1176.99878f, -0.0156917628f});
        AssertValues(e2.NormVec, {0.0333137922f, -0.00666215876f, 0.999422789f, -1.33243166e-05f});

        e3.Assign({-11.111f, 2.222f, -333.333f, 0.004444f});
        Multiply(e3, 1e-15f);
        AssertValues(e3.Vec, {-1.11110004e-14f, 2.22199993e-15f, -3.33332998e-13f, 4.44399972e-18f});
        AssertValues(e3.NormVec, {0.0f, 0.0f, 0.0f, 0.0f});
    }

    Y_UNIT_TEST(L2NormTest) {
        TEmbedding e1, e2;
        e1.Assign({0.227f, -1.138f, -0.734f, 8.35f});
        e2.Assign({1e-8f, -1e-8f, 1e-8f, -1e-8f});

        UNIT_ASSERT_DOUBLES_EQUAL(L2Norm(e1), 71.60783386f, std::numeric_limits<float>::epsilon());
        UNIT_ASSERT_DOUBLES_EQUAL(L2Norm(e2), 4e-16f, std::numeric_limits<float>::epsilon());
    }
}
