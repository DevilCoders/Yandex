#include <kernel/text_machine/basic_core/tm_full_internals.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NTextMachine;
using namespace NTextMachine::NCore;
using namespace NTextMachine::NBasicCore;

#define BUF(A, T) TPodBuffer<T>(A, sizeof(A)/sizeof(T), EStorageMode::Full)
#define FBUF(A) BUF(A, float)

#define UNIT_ASSERT_FEATURE(F, N, V) UNIT_ASSERT_DOUBLES_EQUAL_C(GetFeature(F, N), V, 1e-5, N)

void JoinFeatureIds(TFeaturesBuffer& features,
    const TConstFloatsBuffer& floats,
    const TConstFFIdsBuffer& ids,
    TDeque<TFFIdWithHash>& storage)
{
    Y_VERIFY(floats.Count() == ids.Count());
    features.SetTo(floats.Count());
    for (size_t i : xrange(features.Count())) {
        storage.emplace_back(ids[i]);
        features[i] = TOptFeature(&storage.back(), floats[i]);
    }
}

void PrintFeatures(const TFeaturesBuffer& features) {
    for (const auto& f : features) {
        Cdbg << f.GetId().FullName() << '\t' << f.GetValue() << Endl;
    }
}

float GetFeature(const TFeaturesBuffer& features, const TString& name) {
    for (const auto& f : features) {
        if (f.GetId().FullName() == name) {
            return f.GetValue();
        }
    }

    return -1.0f;
}

class TAggregatorTest : public TTestBase {
private:
    UNIT_TEST_SUITE(TAggregatorTest);
        UNIT_TEST(TestOneFeature);
        UNIT_TEST(TestEmpty);
        UNIT_TEST(TestW2Add);
    UNIT_TEST_SUITE_END();

private:
    TOptFeature MakeFeature(const char* name, float value) {
        IdStorage.emplace_back(TFFId(name));
        return TOptFeature(&IdStorage.back(), value);
    }

    TDeque<TFFIdWithHash> IdStorage;
    TMemoryPool Pool{1UL << 10};

public:
    void TestOneFeature() {
        TFFId ids[] = {TFFId(TStringBuf("Z"))};

        float fv0[] = { 0.0f };
        float fv1[] = { 0.1f };
        float fv2[] = { 0.5f };
        float fv3[] = { 0.9f };

        auto idsBuf = TConstFFIdsBuffer(ids, 1, EStorageMode::Full);
        TVector<TFFId> aggIds;
        aggIds.resize(TFullAggregatorMachine::GetNumFeatures(1));
        auto aggIdsBuf = TFFIdsBuffer::FromVector(aggIds, EStorageMode::Empty);
        TFullAggregatorMachine::SaveFeatureIds(idsBuf, aggIdsBuf);

        TFullAggregatorMachine agg;
        agg.Init(Pool, 1);
        agg.Clear();

        agg.AddFeatures(FBUF(fv0), 1.0);
        agg.AddFeatures(FBUF(fv1), 0.9);
        agg.AddFeatures(FBUF(fv2), 0.5);
        agg.AddFeatures(FBUF(fv3), 0.1);

        TFloatsHolder floats;
        floats.Init(Pool, agg.GetNumFeatures());
        agg.SaveFeatures(floats);

        TFeaturesHolder res;
        res.Init(Pool, agg.GetNumFeatures());
        TDeque<TFFIdWithHash> idStorage;
        JoinFeatureIds(res, floats, aggIdsBuf, idStorage);

        PrintFeatures(res);

        UNIT_ASSERT_FEATURE(res, "All_NumX", 0.285714);
        UNIT_ASSERT_FEATURE(res, "All_SumF_Count_Z", 0.375);
        UNIT_ASSERT_FEATURE(res, "All_SumWF_SumW_Z", 0.172);
        UNIT_ASSERT_FEATURE(res, "All_SumW2F_SumW_Z", 0.086);
        UNIT_ASSERT_FEATURE(res, "All_SumWF_SumF_Z", 0.286666);
        UNIT_ASSERT_FEATURE(res, "All_MaxF_Z", 0.9);
        UNIT_ASSERT_FEATURE(res, "All_MaxWF_Z", 0.25);
        UNIT_ASSERT_FEATURE(res, "All_MaxWF_MaxW_Z", 0.25);
        UNIT_ASSERT_FEATURE(res, "All_MaxWF_SumW_Z", 0.1);
        UNIT_ASSERT_FEATURE(res, "All_MaxWF_MaxF_Z", 0.277777);
        UNIT_ASSERT_FEATURE(res, "All_MaxWF_SumF_Z", 0.166666);
        UNIT_ASSERT_FEATURE(res, "All_MinF_Z", 0.0);

        UNIT_ASSERT_FEATURE(res, "Top_NumX", 0.230769);
        UNIT_ASSERT_FEATURE(res, "Top_SumF_Count_Z", 0.5);
        UNIT_ASSERT_FEATURE(res, "Top_SumF_Count_Z", 0.5);
        UNIT_ASSERT_FEATURE(res, "Top_SumWF_SumW_Z", 0.286666);
        UNIT_ASSERT_FEATURE(res, "Top_SumW2F_SumW_Z", 0.143333);
        UNIT_ASSERT_FEATURE(res, "Top_MinF_Z", 0.1);
    }

    void TestW2Add() {
        NAggregator::TFeaturesW2AddOp op;

        static const size_t N = 5;
        static const size_t M = 3;

        float features[M][N] = {
            { 0.1f, 0.1f, 0.1f, 0.1f, 0.1f },
            { 0.2f, 0.2f, 0.2f, 0.2f, 0.2f },
            { 0.3f, 0.3f, 0.3f, 0.3f, 0.3f }
        };

        float weights[M][N] = {
            { 0.3f, 0.3f, 0.3f, 0.3f, 0.3f },
            { 0.2f, 0.3f, 0.2f, 0.2f, 0.2f },
            { 0.1f, 0.1f, 0.1f, 0.1f, 0.1f }
        };

        NAggregator::TWeightsFeaturesBuf bufs[M];

        for (size_t i : xrange(M)) {
            bufs[i].Init(N);
            bufs[i].SetFeatures(features[i]);
            bufs[i].SetWeights(weights[i]);
        }

        float result[N] = {};
        NAggregator::TFeaturesBuf resBuf;
        resBuf.Init(N);
        resBuf.SetFeatures(result);

        for (size_t i : xrange(M)) {
            op.Store(resBuf, op(resBuf, bufs[i]));
        }

        for (size_t j : xrange(N)) {
            float y = 0.0;

            for (size_t i : xrange(M)) {
                y += features[i][j] * pow(weights[i][j], 2.0f);
            }

            UNIT_ASSERT_EQUAL(y, resBuf.GetFeature(j));
        }
    }

    void TestEmpty() {
        TFFId ids[] = {TFFId(TStringBuf("Z"))};

        auto idsBuf = TConstFFIdsBuffer(ids, 1, EStorageMode::Full);
        TVector<TFFId> aggIds;
        aggIds.resize(TFullAggregatorMachine::GetNumFeatures(1));
        auto aggIdsBuf = TFFIdsBuffer::FromVector(aggIds, EStorageMode::Empty);
        TFullAggregatorMachine::SaveFeatureIds(idsBuf, aggIdsBuf);

        TFullAggregatorMachine agg;
        agg.Init(Pool, 1);
        agg.Clear();

        TFloatsHolder floats;
        floats.Init(Pool, agg.GetNumFeatures());
        agg.SaveFeatures(floats);

        TFeaturesHolder res;
        res.Init(Pool, agg.GetNumFeatures());
        TDeque<TFFIdWithHash> idStorage;
        JoinFeatureIds(res, floats, aggIdsBuf, idStorage);

        PrintFeatures(res);

        UNIT_ASSERT(res.Count() > 0);
        UNIT_ASSERT_EQUAL(res.Count(), agg.GetNumFeatures());

        for (TOptFeature& feature : res) {
            UNIT_ASSERT_EQUAL(feature.GetValue(), 0.0f);
        }
    }
};

UNIT_TEST_SUITE_REGISTRATION(TAggregatorTest);
