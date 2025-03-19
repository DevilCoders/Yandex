#include <kernel/matrixnet/mn_dynamic.h>

#include <library/cpp/archive/yarchive.h>
#include <library/cpp/testing/unittest/registar.h>

//#include <util/generic/string.h>
#include <util/memory/blob.h>
//#include <util/random/random.h>
//#include <util/generic/ymath.h>
#include <util/stream/mem.h>


using namespace NMatrixnet;
//using namespace NFactorSlices;
//using namespace NSlicedCalcer;

Y_UNIT_TEST_SUITE(TSplitBySpecifiedTest) {

namespace {
    extern "C" {
        extern const unsigned char TestModels[];
        extern const ui32 TestModelsSize;
    };

    static const TString ip25("/ip25.info");

    void ExtractModel(TMnSseDynamic &model) {
        TArchiveReader archive(TBlob::NoCopy(TestModels, TestModelsSize));
        TBlob modelBlob = archive.ObjectBlobByKey(ip25);
        TMemoryInput str(modelBlob.Data(), modelBlob.Length());
        model.Load(&str);
    }

    void MakeFactors(TVector< TVector<float> > &factors) {
        TVector<float> sample = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 91000.0f, 46.912100000000002f, -1.0f, -1.0f, 1280000000.0f, 0.0f, 154.78800000000001f, 19.77f, 0.30866700000000002f, 2.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0083999999999999995f, 1.0f, 0.0f, -230.25899999999999f, 0.990676f, 1415260.0f, 0.0f, 13011.0f, 1444.0f, 0.34999999999999998f, 0.62307900000000005f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1594.0f, 238.0f, 23774.0f, 205.0f, 191.0f, 48.0f, 215.0f, 1.0f, 1329338624.0f, 1329338624.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 11.6326f, 4.5612599999999999f, 7.1691200000000004f, 6.8840599999999998f, 14323.0f, 1823.0f, 1011.0f, 0.070585800000000004f, 0.55457999999999996f, 14246.0f, 3788.0f, 2349.0f, 0.16488800000000001f, 0.620116f, 0.65059f, 814.0f, 0.0f, 0.0f, 46.0f, 0.377056f, 314.0f, 68.0f, 142.0f, 71.0f, 19.0f, 88.0f, 1.0f, 1329427584.0f, 1329427584.0f, 0.27800000000000002f, 30.491499999999998f, 0.165432f, 0.040194500000000001f};
        TVector<float> firstFactor = {1029790.0f, 270051.0f, 270053.0f, 548652.0f, 548654.0f, 886775.0f, 886777.0f};
        factors.clear();
        factors.resize(firstFactor.size(), sample);
        for (size_t i = 0, cnt = firstFactor.size(); i < cnt; ++i)
            factors[i][0] = firstFactor[i];
    }

    void DoTest() {
        TMnSseDynamic model;
        ExtractModel(model);

        TVector< TVector<float> > factors;
        MakeFactors(factors);

        TMnSseDynamic with, without;
        TSet<ui32> factorsToKeep;
        factorsToKeep.insert(0);
        model.SplitTreesBySpecifiedFactors(factorsToKeep, with, without);

        TVector<double> a, b, c;
        model.CalcRelevs(factors, c);
        with.CalcRelevs(factors, a);
        without.CalcRelevs(factors, b);

        for (size_t i = 0, cnt = factors.size(); i < cnt; ++i)
            UNIT_ASSERT_EQUAL(a[i] + b[i], c[i]);
        for (size_t i = 1, cnt = factors.size(); i < cnt; ++i)
            UNIT_ASSERT_EQUAL(b[i - 1], b[i]);
    }
}

Y_UNIT_TEST(DoSplitTest) {
    DoTest();
}

}
