#include <kernel/matrixnet/mn_sse.h>
#include <kernel/factor_slices/factor_borders.h>
#include <kernel/factor_storage/factor_storage.h>

#include <library/cpp/archive/yarchive.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/string.h>
#include <util/memory/blob.h>
#include <util/generic/map.h>
#include <util/random/random.h>
#include <util/generic/ymath.h>

using namespace NMatrixnet;
using namespace NFactorSlices;

Y_UNIT_TEST_SUITE(TMnSseInfoTest) {

namespace {
    extern "C" {
        extern const unsigned char TestModels[];
        extern const ui32 TestModelsSize;
    };

    static const TString Ru("/Ru.info");
    static const TString Fresh("/RuFreshExtRelev.info");
    static const TString IP25("/ip25.info");
    static const TString RandomQuarkSlices("/quark_slices.info");

    template <typename T>
    T ClampedLerp(T a, T b, double x) {
        T raw = a + (b - a) * x;
        return ClampVal<T>(raw, Min(a, b), Max(a, b));
    }

    void SetModel(TMnSseInfo& model, const TString& name) {
        TArchiveReader archive(TBlob::NoCopy(TestModels, TestModelsSize));
        TBlob modelBlob = archive.ObjectBlobByKey(name);
        model.InitStatic(modelBlob.Data(), modelBlob.Length());
    }

    TFactorBorders CreateFactorBorders() {
        TFactorBorders result;

        result[EFactorSlice::FRESH] = TSliceOffsets(0, 100);
        result[EFactorSlice::WEB] = TSliceOffsets(100, 1100);
        result[EFactorSlice::PERSONALIZATION] = TSliceOffsets(1100, 1600);

        return result;
    }

    TFactorBorders RescaleFactorBorders(TFactorBorders borders, double scaleFactor) {
        for (TSliceIterator it; it.Valid(); ++it) {
            TSliceOffsets& offset = borders[*it];
            if (!offset.Empty()) {
                offset.Begin *= scaleFactor;
                offset.End *= scaleFactor;
            }
        }

        return borders;
    }


    void FillFactorView(TFactorView view) {
        for (size_t i = 0; i < view.Size(); ++i) {
            view[i] = RandomNumber<float>();
        }
    }
    void FillFactorVector(TVector<float>& result) {
        for (float& f : result)
            f = RandomNumber<float>();
    }

    void FillFactorStorage(TFactorStorage* storage, const TFactorBorders& borders) {
        for (TSliceIterator it; it.Valid(); ++it) {
            if (borders[*it].Empty()) {
                continue;
            }

            FillFactorView(storage->CreateViewFor(*it));
        }
    }

    void CreateFeatures(const size_t number, const TFactorBorders& borders, TVector<TFactorStorage*>& result, TMemoryPool& pool) {
        result.resize(number);

        for (size_t i = 0; i < number; ++i) {
            result[i] = CreateFactorStorageInPool(&pool, borders);
            FillFactorStorage(result[i], borders);
        }
    }

    void CreateSkewedFeatures(const size_t number, const TFactorBorders& borders, double minSkewFactor, double maxSkewFactor, TVector<TFactorStorage*>& result, TMemoryPool& pool) {
        result.resize(number);

        for (size_t i = 0; i < number; ++i) {
            double skewFactor = ClampedLerp(minSkewFactor, maxSkewFactor, i / (double)(number - 1));
            TFactorBorders skewedBorders = RescaleFactorBorders(borders, skewFactor);
            result[i] = CreateFactorStorageInPool(&pool, skewedBorders);
            FillFactorStorage(result[i], skewedBorders);
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

    void CreatePlanar(const TVector<TFactorStorage*>& features, const TMnSseInfo& model, TVector<TVector<float>>& result, bool allowSlicelessFeatures = false) {
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

    void TestSlicedCalc(const TString& modelName) {
        TMnSseInfo model;
        SetModel(model, modelName);

        TFactorBorders borders(CreateFactorBorders());
        TMemoryPool pool(420 * sizeof(TFactorStorage));

        TVector<TFactorStorage*> slicedFeats;
        TVector<TVector<float>> planarFeats;
        TVector<double> slicedRes;
        TVector<double> planarRes;

        // MxNet16 internally
        CreateFeatures(20, borders, slicedFeats, pool);
        CreatePlanar(slicedFeats, model, planarFeats);
        UNIT_ASSERT_EQUAL(slicedFeats.size(), planarFeats.size());
        model.SlicedCalcRelevs(slicedFeats, slicedRes);
        model.CalcRelevs(planarFeats, planarRes);
        UNIT_ASSERT_EQUAL(slicedRes, planarRes);

        // MxNet128 internally
        CreateFeatures(200, borders, slicedFeats, pool);
        CreatePlanar(slicedFeats, model, planarFeats);
        UNIT_ASSERT_EQUAL(slicedFeats.size(), planarFeats.size());
        model.SlicedCalcRelevs(slicedFeats, slicedRes);
        model.CalcRelevs(planarFeats, planarRes);
        UNIT_ASSERT_EQUAL(slicedRes, planarRes);

        // test variable borders
        CreateSkewedFeatures(200, borders, 1./5., 9./5., slicedFeats, pool);
        CreatePlanar(slicedFeats, model, planarFeats);
        UNIT_ASSERT_EQUAL(slicedFeats.size(), planarFeats.size());
        model.SlicedCalcRelevs(slicedFeats, slicedRes);
        model.CalcRelevs(planarFeats, planarRes);
        UNIT_ASSERT_EQUAL(slicedRes, planarRes);
    }

    void TestSlicedCalcInterfaceOnSlicelessModel(const TString& modelName) {
        TMnSseInfo model;
        SetModel(model, modelName);

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
        TMnSseInfo model;
        SetModel(model, Ru);

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
        TMnSseInfo model;
        SetModel(model, Ru);

        TVector<TFactorStorage*> slicedFeats(10);
        TVector<double> slicedRes;

        for (size_t i = 0; i < 10; ++i) {
            slicedFeats[i] = nullptr;
        }

        model.SlicedCalcRelevs(slicedFeats, slicedRes);
        UNIT_ASSERT_EQUAL(slicedRes[0], 0);
    }

    void TestBinarizationCalc(const TString& modelName) {
        TMnSseInfo model;
        SetModel(model, modelName);

        TFactorBorders borders(CreateFactorBorders());
        TMemoryPool pool(420 * sizeof(TFactorStorage));

        TVector<TFactorStorage*> slicedFeats;
        TVector<TVector<float>> planarFeats;
        TVector<double> binarizedRes;
        TVector<double> slisedBinarizedRes;
        TVector<double> plainRes;
        TMnSseInfo::TPreparedBatchPtr preparedBatch;
        TMnSseInfo::TSlicedPreparedBatch slicedPreparedBatch;

        // MxNet16 internally
        CreateFeatures(20, borders, slicedFeats, pool);
        CreatePlanar(slicedFeats, model, planarFeats);
        UNIT_ASSERT_EQUAL(slicedFeats.size(), planarFeats.size());
        preparedBatch = model.CalcBinarization(planarFeats);
        model.CalcRelevs(*preparedBatch, binarizedRes);
        model.CalcRelevs(planarFeats, plainRes);
        UNIT_ASSERT_EQUAL(binarizedRes, plainRes);
        slicedPreparedBatch = model.SlicedCalcBinarization(slicedFeats);
        model.SlicedCalcRelevs(slicedPreparedBatch, slisedBinarizedRes);
        UNIT_ASSERT_EQUAL(slisedBinarizedRes, plainRes);

        // MxNet128 internally
        CreateFeatures(200, borders, slicedFeats, pool);
        CreatePlanar(slicedFeats, model, planarFeats);
        UNIT_ASSERT_EQUAL(slicedFeats.size(), planarFeats.size());
        preparedBatch = model.CalcBinarization(planarFeats);
        model.CalcRelevs(*preparedBatch, binarizedRes);
        model.CalcRelevs(planarFeats, plainRes);
        UNIT_ASSERT_EQUAL(binarizedRes, plainRes);
        slicedPreparedBatch = model.SlicedCalcBinarization(slicedFeats);
        model.SlicedCalcRelevs(slicedPreparedBatch, slisedBinarizedRes);
        UNIT_ASSERT_EQUAL(slisedBinarizedRes, plainRes);

        // test variable borders
        CreateSkewedFeatures(200, borders, 1./5., 9./5., slicedFeats, pool);
        CreatePlanar(slicedFeats, model, planarFeats);
        UNIT_ASSERT_EQUAL(slicedFeats.size(), planarFeats.size());
        preparedBatch = model.CalcBinarization(planarFeats);
        model.CalcRelevs(*preparedBatch, binarizedRes);
        model.CalcRelevs(planarFeats, plainRes);
        UNIT_ASSERT_EQUAL(binarizedRes, plainRes);
        slicedPreparedBatch = model.SlicedCalcBinarization(slicedFeats);
        model.SlicedCalcRelevs(slicedPreparedBatch, slisedBinarizedRes);
        UNIT_ASSERT_EQUAL(slisedBinarizedRes, plainRes);
    }

    void TestBinarizationSlicedCalcOnSlicelessModel(const TString& modelName) {
        TMnSseInfo model;
        SetModel(model, modelName);

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
        model.SlicedCalcRelevs(model.SlicedCalcBinarization(slicedFeats), slicedRes);
        model.CalcRelevs(planarFeats, planarRes);
        UNIT_ASSERT_EQUAL(slicedRes, planarRes);
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

        void CreatePlanarFromSparse(const TVector<TSparseFactors>& features, const TMnSseInfo& model, TVector<TVector<float>>& result) {
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
            TMnSseInfo model;
            SetModel(model, modelName);

            TVector<TSparseFactors> slicedFeats;
            TVector<TVector<float>> planarFeats;
            TVector<double> slicedRes;
            TVector<double> planarRes;

            CreateSparseFeatures(200, SliceBorders, slicedFeats);
            CreatePlanarFromSparse(slicedFeats, model, planarFeats);
            UNIT_ASSERT_VALUES_EQUAL(slicedFeats.size(), planarFeats.size());

            TMap<TString, size_t> visitedSlices;
            model.CustomSlicedCalcRelevs<TSparseFactors>(&slicedFeats[0], slicedRes, slicedFeats.size());
            model.CalcRelevs(planarFeats, planarRes);
            UNIT_ASSERT_VALUES_EQUAL(slicedRes, planarRes);
        }
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

Y_UNIT_TEST(BinarizationCalc) {
    TestBinarizationCalc(Ru);
}

Y_UNIT_TEST(BinarizationSlicedCalcOnSlicelessModel) {
    TestBinarizationSlicedCalcOnSlicelessModel(IP25);
}


}


