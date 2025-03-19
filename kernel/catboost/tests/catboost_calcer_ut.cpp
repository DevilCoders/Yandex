#include <catboost/yandex_specific/libs/mn_sse_conversion/convertor.h>

#include <kernel/factor_slices/factor_borders.h>
#include <kernel/factor_storage/factor_storage.h>
#include <kernel/catboost/catboost_calcer.h>
#include <kernel/matrixnet/mn_sse.h>
#include <kernel/matrixnet/mn_dynamic.h>

#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/archive/yarchive.h>

#include <quality/relev_tools/mx_model_lib/mx_model.h>

#include <util/generic/string.h>
#include <util/memory/blob.h>
#include <util/generic/map.h>
#include <util/random/random.h>
#include <util/generic/ymath.h>
#include <util/stream/fwd.h>
#include <util/stream/file.h>

using namespace NCatboostCalcer;
using namespace NMatrixnet;
using namespace NFactorSlices;

namespace {
    extern "C" {
        extern const unsigned char TestModels[];
        extern const ui32 TestModelsSize;
    };

    static const TString Ru("/Ru.info");
    static const TString Fresh("/RuFreshExtRelev.info");
    static const TString IP25("/ip25.info");
    static const TString RandomQuarkSlices("/quark_slices.info");

    const size_t FLOAT_FORMULA_DATA_SIZE = 3;
    const size_t FLOAT_FORMULA_FACTORS_COUNT = 101;
    const float FLOAT_FORMULA_DATA[FLOAT_FORMULA_DATA_SIZE][FLOAT_FORMULA_FACTORS_COUNT] = {
            {2.21074e+08, 1.43609e+06, 33957, 0.649597, 37, 120000, 18.9875, 0.508095, 0.0479452, 2474, 0, 0.428815, 5.11, 0.0387097, 2, 1, 97101, 0, 0, 0, 0, 0, -230.259, 0.969231, 509208, 0, 1.08494e+06, 26743, 0.204, 0, 0, 0, 1343144064, -2.03244, 0, 948, 343, 132879, 42, 16, 0, 85, 1, 1343111552, 1343111552, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -0.310355, -1.12571, 0, 0, 13, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0.0777285, 361, -6.64386, 0, 0, 0, 140, 46, 24, 10, 0, 96, 1, 1343124864, 1343124864, 0.341, 0.0633271, 0.0340909, 0, 0.00225047468216, 0.00179685179685, 0.0002673002673, 0.0056811852512, 0.000222791578478, 0.000414651002073, 0.0, 0.0056811852512},
            {954636, 115393, 9601, 12.0876, 26, 96458, 39.9231, 0.349938, 0.160959, 1431, 0, 65.4699, -1, 0.0173077, 4, 0, 0, 0.0956522, 0, 0, 0, 0, -230.259, 0.840909, 46200, 1, 75392, 3856, 0.0277, 0.075, 0.8, 0.444949, 1344566912, 14.5119, 0, 625, 54, 0, 32, 6, 0, 1846, 1, 1344445696, 1344445696, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 42, 12.8984, 4.49869, 11.7552, 4.65881, 0, 0, 0, 0, 0, 174, 45, 28, 0.16092, 0.622222, 0.641667, 12, 4.92835, 0, 9, 0, 5, 3, 86, 3, 0, 40, 1, 1344508416, 1344508416, 0, 0.523759, 0.00424528, 0, 0.20320059546, 0.0299789621318, 0.00569775596073, 0.5, 0.0, 0.5, 4.0, 0.833333333333},
            {3.17168e+07, 107270, 1411, 0.338212, 34, 113100, 13.6048, 0.0791059, 0.0538178, 592, 0, 2.52863, -1, 0.0772926, 2, 0, 0, 0, 0, 0, 0, 0, -230.259, 0, 0, 0, 0, 0, 0, 0.112919, 0, 0, 1345626624, -2.97696, 0, 802, 165, 0, 209, 59, 0, 20, 1, 1345594240, 1345594240, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -3.11626, -1.91191, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -5.97361, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.00277900308778, 0.00149758779544, 0.000155458940703, 0.000589622641509, 0.0, 0.0, 0.0, 0.0}
    };
    const double FLOAT_FORMULA_VALUES[FLOAT_FORMULA_DATA_SIZE] = {0.1113263824, 0.583931613, 0.05911373111};
    const double EPS = 1e-6;


    const size_t ADULT_FORMULA_DATA_SIZE = 10;
    const size_t ADULT_FORMULA_FLOAT_FACTORS_COUNT = 10;

    const float ADULT_FORMULA_FLOAT_FACTORS[ADULT_FORMULA_DATA_SIZE][ADULT_FORMULA_FLOAT_FACTORS_COUNT] = {
        { 73.0, 30958.0, 10.0, 0.0, 0.0, 25.0 },
        { 33.0, 123031.0, 9.0, 0.0, 0.0, 40.0 },
        { 30.0, 159442.0, 9.0, 0.0, 0.0, 35.0 },
        { 30.0, 187279.0, 9.0, 0.0, 0.0, 44.0 },
        { 41.0, 112763.0, 9.0, 2597.0, 0.0, 40.0 },
        { 51.0, 123053.0, 14.0, 5013.0, 0.0, 40.0 },
        { 31.0, 218322.0, 10.0, 0.0, 0.0, 30.0 },
        { 71.0, 94314.0, 9.0, 1173.0, 0.0, 18.0 },
        { 59.0, 130532.0, 10.0, 0.0, 0.0, 40.0 },
        { 33.0, 157747.0, 12.0, 0.0, 0.0, 40.0 }
    };

    const size_t ADULT_FORMULA_CATEG_FACTORS_COUNT = 8;
    const char* ADULT_FORMULA_CATEG_FACTORS[ADULT_FORMULA_DATA_SIZE][ADULT_FORMULA_CATEG_FACTORS_COUNT] = {
        { "Self-emp-not-inc", "Some-college", "Married-civ-spouse", "Sales", "Husband", "White", "Male", "United-States" },
        { "Private", "HS-grad", "Married-spouse-absent", "Adm-clerical", "Unmarried", "Amer-Indian-Eskimo", "Male", "United-States" },
        { "Private", "HS-grad", "Never-married", "Adm-clerical", "Not-in-family", "White", "Female", "Ireland" },
        { "Private", "HS-grad", "Married-civ-spouse", "Handlers-cleaners", "Husband", "White", "Male", "United-States" },
        { "Private", "HS-grad", "Divorced", "Handlers-cleaners", "Own-child", "White", "Female", "United-States" },
        { "Private", "Masters", "Married-civ-spouse", "Prof-specialty", "Husband", "Asian-Pac-Islander", "Male", "India" },
        { "Private", "Some-college", "Never-married", "Other-service", "Not-in-family", "White", "Male", "United-States" },
        { "?", "HS-grad", "Married-civ-spouse", "?", "Wife", "White", "Female", "United-States" },
        { "Local-gov", "Some-college", "Widowed", "Other-service", "Not-in-family", "White", "Female", "Poland" },
        { "Private", "Assoc-acdm", "Married-civ-spouse", "Handlers-cleaners", "Husband", "White", "Male", "United-States" }
    };
    const double ADULT_FORMULA_VALUES[ADULT_FORMULA_DATA_SIZE] = {0.3310662227, 0.9201657399, 0.9368763874,
                                                                  0.6223672533, 0.9240067868, -0.3023358455,
                                                                  0.9461575804, 0.6631042396, 0.9173958168,
                                                                  0.4399752033};

    TCatboostCalcer GetModel(const TString& name) {
        TMnSseDynamic infoModel;
        TArchiveReader archive(TBlob::NoCopy(TestModels, TestModelsSize));
        TBlob modelBlob = archive.ObjectBlobByKey(name);
        TMemoryInput str(modelBlob.Data(), modelBlob.Length());
        infoModel.Load(&str);

        TFullModel cbmodel;
        ConvertFromMnSSE(infoModel, &cbmodel);

        TCatboostCalcer calcer(cbmodel);
        return calcer;
    }

    TFactorBorders CreateFactorBorders() {
        TFactorBorders result;
        result[EFactorSlice::FRESH] = TSliceOffsets(0, 100);
        result[EFactorSlice::WEB] = TSliceOffsets(100, 1100);
        result[EFactorSlice::PERSONALIZATION] = TSliceOffsets(1100, 1600);

        return result;
    }

    void FillFactorView(TFactorView view) {
        for (size_t i = 0; i < view.Size(); ++i) {
            view[i] = RandomNumber<float>();
        }
    }

    void FillFactorVector(TVector<float> &result) {
        for (float &f : result)
            f = RandomNumber<float>();
    }

    void FillFactorStorage(TFactorStorage *storage, const TFactorBorders &borders) {
        for (TSliceIterator it; it.Valid(); ++it) {
            if (borders[*it].Empty()) {
                continue;
            }
            FillFactorView(storage->CreateViewFor(*it));
        }
    }

    void CreateFeatures(const size_t number, const TFactorBorders &borders, TVector<TFactorStorage *> &result,
                        TMemoryPool &pool) {
        result.resize(number);

        for (size_t i = 0; i < number; ++i) {
            result[i] = CreateFactorStorageInPool(&pool, borders);
            FillFactorStorage(result[i], borders);
        }
    }

    template<typename T>
    T ClampedLerp(T a, T b, double x) {
        T raw = a + (b - a) * x;
        return ClampVal<T>(raw, Min(a, b), Max(a, b));
    }

    TFactorBorders RescaleFactorBorders(TFactorBorders borders, double scaleFactor) {
        for (TSliceIterator it; it.Valid(); ++it) {
            TSliceOffsets &offset = borders[*it];
            if (!offset.Empty()) {
                offset.Begin *= scaleFactor;
                offset.End *= scaleFactor;
            }
        }

        return borders;
    }

    void CreateSkewedFeatures(const size_t number, const TFactorBorders &borders, double minSkewFactor, double maxSkewFactor,
                         TVector<TFactorStorage *> &result, TMemoryPool &pool) {
        result.resize(number);

        for (size_t i = 0; i < number; ++i) {
            double skewFactor = ClampedLerp(minSkewFactor, maxSkewFactor, i / (double) (number - 1));
            TFactorBorders skewedBorders = RescaleFactorBorders(borders, skewFactor);
            result[i] = CreateFactorStorageInPool(&pool, skewedBorders);
            FillFactorStorage(result[i], skewedBorders);
        }
    }

    void CreatePlanar(const TVector<TFactorStorage *> &features, const TCatboostCalcer &model,
                      TVector<TVector<float>> &result, bool allowSlicelessFeatures = false) {
        result.resize(features.size());

        for (size_t i = 0; i < features.size(); ++i) {
            result[i].assign(model.GetNumFeats(), 0.0f);
            if (!model.GetSlices().empty()) {
                for (size_t j = 0; j < model.GetSliceNames().size(); ++j) {
                    EFactorSlice slice = FromString(model.GetSliceNames()[j]);
                    TFeatureSlice offset = model.GetSlices()[j];
                    ui32 startIndex = Min<ui32>(offset.StartFeatureIndexOffset, model.GetNumFeats());
                    ui32 endIndex = Max<ui32>(Min<ui32>(offset.EndFeatureIndexOffset, model.GetNumFeats()), startIndex);
                    ui32 requiredFeatures = endIndex - startIndex;
                    const auto sliceView = features[i]->CreateConstViewFor(slice);
                    memcpy(&result[i][startIndex],
                           sliceView.GetConstFactors(),
                           Min<size_t>(requiredFeatures, sliceView.Size()) * sizeof(float));
                }
            } else {
                UNIT_ASSERT_C(allowSlicelessFeatures, "Formula has not slices");
                const auto view = features[i]->CreateConstView();
                memcpy(&result[i][0], view.GetConstFactors(), Min(view.Size(), result[i].size()) * sizeof(float));
            }
        }
    }

    void CreateSkewedSlicelessFeatures(const size_t number, const size_t minLength, const size_t maxLength, TVector<TFactorStorage*>& result, TMemoryPool& pool) {
        result.resize(number);

        for (size_t i = 0; i < number; ++i) {
            const size_t featuresNum = ClampedLerp(minLength, maxLength, i / (double)(number - 1));
            TFactorStorage* tmp = CreateFactorStorageInPool(&pool, featuresNum);
            FillFactorView(tmp->CreateView());
            result[i] = tmp;
        }
    }

    void TestSlicedCalcInterfaceOnSlicelessModel(const TString& modelName) {
        auto model = GetModel(modelName);

        TMemoryPool pool(200 * sizeof(TFactorStorage));

        TVector<TFactorStorage*> slicedFeats;
        TVector<TVector<float>> planarFeats;
        TVector<double> slicedRes;
        TVector<double> planarRes;

        const size_t minFeaturesNum = model.GetNumFeats() * 1 / 5;
        const size_t maxFeaturesNum = model.GetNumFeats() * 9 / 5;
        CreateSkewedSlicelessFeatures(200, minFeaturesNum, maxFeaturesNum, slicedFeats, pool);
        CreatePlanar(slicedFeats, model, planarFeats, true);
        UNIT_ASSERT_EQUAL(slicedFeats.size(), planarFeats.size());
        model.SlicedCalcRelevs(slicedFeats, slicedRes);
        model.CalcRelevs(planarFeats, planarRes);
        UNIT_ASSERT_EQUAL(slicedRes, planarRes);
    }

    void TestInvalidFactorStorage() { //Create factor storages without factors and calc a MXNet on them; expect a zero.
        auto model = GetModel(Ru);

        TMemoryPool pool(10 * sizeof(TFactorStorage));

        TVector<TFactorStorage*> slicedFeats(10);
        TVector<double> slicedRes;

        for (size_t i = 0; i < 10; ++i) {
            slicedFeats[i] = new (pool.Allocate(sizeof(TFactorStorage))) TFactorStorage(&pool);
        }

        model.SlicedCalcRelevs(slicedFeats, slicedRes);
        UNIT_ASSERT_EQUAL(slicedRes[0], 0);
    }

    void TestNullFactorStorage() { //Pass zero pointers to a MXNet; expect no crash and a zero result
        auto model = GetModel(Ru);

        TVector<TFactorStorage*> slicedFeats(10);
        TVector<double> slicedRes;

        for (size_t i = 0; i < 10; ++i) {
            slicedFeats[i] = nullptr;
        }

        model.SlicedCalcRelevs(slicedFeats, slicedRes);
        UNIT_ASSERT_EQUAL(slicedRes[0], 0);
    }

    namespace NCustomSlices {
        using TSliceMap = TMap<TString, TSliceOffsets>;
        using TSparseFactorsMap = TMap<TString, TVector<float>>;

        struct TSparseFactors: public TSparseFactorsMap {
            // new struct for the ADL lookup
        };

        TArrayRef<const float> GetFactorsRegion(const TSparseFactors& factors, TStringBuf name) {
            if (const TVector<float>* ptr = factors.FindPtr(name)) {
                return {ptr->data(), ptr->size()};
            }
            return {};
        }

        static const TSliceMap SliceBorders = {
            {"up", TSliceOffsets(0, 100)},
            {"down", TSliceOffsets(100, 200)},
            {"charm", TSliceOffsets(200, 300)},
            {"strange", TSliceOffsets(300, 400)},
            {"top", TSliceOffsets(400, 500)},
            {"bottom", TSliceOffsets(500, 600)},
        };

        void FillSparseFactors(TSparseFactors& result, const TSliceMap& borders) {
            for (const auto& kv : borders) {
                auto& factors = result[kv.first];
                factors.resize(kv.second.Size());
                FillFactorVector(factors);
            }
        }

        void CreateSparseFeatures(const size_t number, const TSliceMap& borders, TVector<TSparseFactors>& result) {
            result.resize(number);
            for (auto& docFactors : result)
                FillSparseFactors(docFactors, borders);
        }

        void CreatePlanarFromSparse(const TVector<TSparseFactors>& features, const TCatboostCalcer& model, TVector<TVector<float>>& result) {
            result.resize(features.size());
            UNIT_ASSERT_C(!model.GetSlices().empty(), "Formula has no slices");
            UNIT_ASSERT_C(model.HasCustomSlices(), "Formula has no custom slices");

            for (size_t i = 0; i < features.size(); ++i) {
                result[i].assign(model.GetNumFeats(), 0.0f);
                for (size_t j = 0; j < model.GetSliceNames().size(); ++j) {
                    TString sliceName = model.GetSliceNames()[j];
                    TFeatureSlice offset = model.GetSlices()[j];
                    ui32 startIndex = Min<ui32>(offset.StartFeatureIndexOffset, model.GetNumFeats());
                    ui32 endIndex = Max<ui32>(Min<ui32>(offset.EndFeatureIndexOffset, model.GetNumFeats()), startIndex);
                    ui32 requiredFeatures = endIndex - startIndex;
                    if (const TVector<float>* pfactors = features[i].FindPtr(sliceName)) {
                        memcpy(&result[i][startIndex], &(*pfactors)[0], Min<size_t>(requiredFeatures, pfactors->size()) * sizeof(float));
                    }
                }
            }
        }

        void TestCustomSlice(const TString& modelName) {
            auto model = GetModel(modelName);

            TVector<TSparseFactors> slicedFeats;
            TVector<TVector<float>> planarFeats;
            TVector<double> slicedRes;
            TVector<double> planarRes;

            CreateSparseFeatures(200, SliceBorders, slicedFeats);
            CreatePlanarFromSparse(slicedFeats, model, planarFeats);
            UNIT_ASSERT_VALUES_EQUAL(slicedFeats.size(), planarFeats.size());

            model.CustomSlicedCalcRelevs<TSparseFactors>(&slicedFeats[0], slicedRes, slicedFeats.size());
            model.CalcRelevs(planarFeats, planarRes);
            UNIT_ASSERT_VALUES_EQUAL(slicedRes, planarRes);
        }
    }

    void TestSlicedCalc(const TString& modelName) {
        auto calcer = GetModel(modelName);

        TFactorBorders borders(CreateFactorBorders());
        TMemoryPool pool(420 * sizeof(TFactorStorage));

        TVector<TFactorStorage*> slicedFeats;
        TVector<TVector<float>> planarFeats;
        TVector<double> slicedRes;
        TVector<double> planarRes;

        CreateFeatures(20, borders, slicedFeats, pool);
        CreatePlanar(slicedFeats, calcer, planarFeats);
        UNIT_ASSERT_EQUAL(slicedFeats.size(), planarFeats.size());
        calcer.SlicedCalcRelevs(slicedFeats, slicedRes);
        calcer.CalcRelevs(planarFeats, planarRes);
        UNIT_ASSERT_EQUAL(slicedRes, planarRes);

        CreateFeatures(200, borders, slicedFeats, pool);
        CreatePlanar(slicedFeats, calcer, planarFeats);
        UNIT_ASSERT_EQUAL(slicedFeats.size(), planarFeats.size());
        calcer.SlicedCalcRelevs(slicedFeats, slicedRes);
        calcer.CalcRelevs(planarFeats, planarRes);
        UNIT_ASSERT_EQUAL(slicedRes, planarRes);

        // test variable borders
        CreateSkewedFeatures(200, borders, 1./5., 9./5., slicedFeats, pool);
        CreatePlanar(slicedFeats, calcer, planarFeats);
        UNIT_ASSERT_EQUAL(slicedFeats.size(), planarFeats.size());
        calcer.SlicedCalcRelevs(slicedFeats, slicedRes);
        calcer.CalcRelevs(planarFeats, planarRes);
        UNIT_ASSERT_EQUAL(slicedRes, planarRes);
    }
}

extern const TCatboostCalcer CatboostAdultModel;
extern const TCatboostCalcer CatboostFloatModel;

Y_UNIT_TEST_SUITE(CatboostCalcerTests) {

    TFullModel GetCatboostFloatModel() {
        CB_ENSURE(CatboostFloatModel.GetModel().GetNumFloatFeatures() <= FLOAT_FORMULA_FACTORS_COUNT);
        return CatboostFloatModel.GetModel();
    }

    TFullModel GetCatboostAdultModel() {
        CB_ENSURE(CatboostAdultModel.GetModel().GetNumFloatFeatures() <= ADULT_FORMULA_FLOAT_FACTORS_COUNT);
        CB_ENSURE(CatboostAdultModel.GetModel().GetNumCatFeatures() <= ADULT_FORMULA_CATEG_FACTORS_COUNT);
        return CatboostAdultModel.GetModel();
    }

    Y_UNIT_TEST(GetNumFeatsTest) {
        TCatboostCalcer calcer(GetCatboostFloatModel());
        UNIT_ASSERT_EQUAL(FLOAT_FORMULA_FACTORS_COUNT, calcer.GetNumFeats());
    }

    Y_UNIT_TEST(DoCalcRelevTest) {
        TCatboostCalcer calcer(GetCatboostFloatModel());

        for (size_t idx = 0; idx < FLOAT_FORMULA_DATA_SIZE; ++idx) {
            double value = calcer.DoCalcRelev(FLOAT_FORMULA_DATA[idx]);
            UNIT_ASSERT_DOUBLES_EQUAL(value, FLOAT_FORMULA_VALUES[idx], EPS);
        }
    }

    Y_UNIT_TEST(DoCalcRelevsTest) {
        TCatboostCalcer calcer(GetCatboostFloatModel());
        double results[FLOAT_FORMULA_DATA_SIZE] = {1, 2, 3};
        const float* featurePtrs[] = {FLOAT_FORMULA_DATA[0],
                                FLOAT_FORMULA_DATA[1],
                                FLOAT_FORMULA_DATA[2]};
        calcer.DoCalcRelevs(featurePtrs, results, FLOAT_FORMULA_DATA_SIZE);
        for (size_t idx = 0; idx < FLOAT_FORMULA_DATA_SIZE; ++idx) {
            UNIT_ASSERT_DOUBLES_EQUAL(results[idx], FLOAT_FORMULA_VALUES[idx], EPS);
        }
    }

    Y_UNIT_TEST(DoCalcRelevsTestAdult) {
        TCatboostCalcer calcer(GetCatboostAdultModel());
        TVector<double> results(ADULT_FORMULA_DATA_SIZE);
        TVector<TVector<TStringBuf>> catFeatures(ADULT_FORMULA_DATA_SIZE);
        TVector<TConstArrayRef<float>> floatFeatures(ADULT_FORMULA_DATA_SIZE);
        for (size_t docId = 0; docId < ADULT_FORMULA_DATA_SIZE; ++docId) {
            for (size_t factorId = 0; factorId < ADULT_FORMULA_CATEG_FACTORS_COUNT; ++factorId) {
                catFeatures[docId].push_back(TStringBuf(ADULT_FORMULA_CATEG_FACTORS[docId][factorId]));
            }
            floatFeatures[docId] = MakeArrayRef(ADULT_FORMULA_FLOAT_FACTORS[docId], ADULT_FORMULA_FLOAT_FACTORS_COUNT);
        }
        calcer.CalcRelevs(floatFeatures, catFeatures, results);
        for (size_t idx = 0; idx < ADULT_FORMULA_DATA_SIZE; ++idx) {
            UNIT_ASSERT_DOUBLES_EQUAL(results[idx], ADULT_FORMULA_VALUES[idx], EPS);
        }
    }

    Y_UNIT_TEST(CopyTreeRangeTest) {
        TCatboostCalcer calcer(GetCatboostFloatModel());
        auto calcerLowHalf = calcer.CopyTreeRange(0, calcer.GetTreeCount() / 2);
        auto calcerHighHalf = calcer.CopyTreeRange(calcer.GetTreeCount() / 2, calcer.GetTreeCount());
        for (size_t idx = 0; idx < FLOAT_FORMULA_DATA_SIZE; ++idx) {
            double result = calcer.DoCalcRelev(FLOAT_FORMULA_DATA[idx]);

            double result02 = calcerLowHalf->DoCalcRelev(FLOAT_FORMULA_DATA[idx]);

            double result25 = calcerHighHalf->DoCalcRelev(FLOAT_FORMULA_DATA[idx]);
            UNIT_ASSERT_DOUBLES_EQUAL(result, FLOAT_FORMULA_VALUES[idx], EPS);
            UNIT_ASSERT_DOUBLES_EQUAL(result02 + result25, result, EPS);
        }
    }

    Y_UNIT_TEST(SlicedCalcSingleSliceTest) {
        TestSlicedCalc(Ru);
    }

    Y_UNIT_TEST(SlicedCalcReverseSliceTest) {
        TestSlicedCalc(Fresh);
    }

    Y_UNIT_TEST(SlicedCalcOnSlicelessModel) {
        TestSlicedCalcInterfaceOnSlicelessModel(IP25);
    }

    Y_UNIT_TEST(CustomSlicesTest) {
        NCustomSlices::TestCustomSlice(RandomQuarkSlices);
    }

    Y_UNIT_TEST(InvalidFactorsStorage) {
        TestInvalidFactorStorage();
    }

    Y_UNIT_TEST(NullFactorsStorage) {
        TestNullFactorStorage();
    }
}

