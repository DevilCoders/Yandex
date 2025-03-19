#include <kernel/matrixnet/mn_dynamic.h>
#include <kernel/matrixnet/mn_standalone_binarization.h>
#include <kernel/factor_storage/factors_reader.h>
#include <kernel/factor_storage/factor_storage.h>
#include <kernel/parallel_mx_calcer/parallel_mx_calcer.h>
#include <kernel/web_factors_info/factor_names.h>
#include <kernel/web_meta_factors_info/factor_names.h>

#include <library/cpp/archive/yarchive.h>
#include <library/cpp/string_utils/base64/base64.h>
#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/resource/resource.h>

#include <util/generic/hash.h>
#include <util/generic/string.h>
#include <util/memory/blob.h>
#include <util/stream/mem.h>
#include <util/string/split.h>
#include <util/thread/pool.h>

using namespace NMatrixnet;

Y_UNIT_TEST_SUITE(TMnSseDynamicTest) {
    namespace {
        extern "C" {
        extern const unsigned char TestModels[];
        extern const ui32 TestModelsSize;
        };

        static const TString Ru("/Ru.info");
        static const TString Fresh("/RuFreshExtRelev.info");
        static const TString IP25("/ip25.info");
        static const TString RandomQuarkSlices("/quark_slices.info");

        void SetModel(TMnSseDynamic& model, const TString& name) {
            TArchiveReader archive(TBlob::NoCopy(TestModels, TestModelsSize));
            TBlob modelBlob = archive.ObjectBlobByKey(name);
            TMemoryInput input(modelBlob.AsCharPtr(), modelBlob.Size());
            ::Load(&input, model);
        }

        TMnSseDynamic SumModels(TMnSseDynamic left, const TMnSseDynamic& right) {
            left.Add(right);
            return left;
        }

        THashMap<TString, size_t> GetSliceSizes(const TMnSseInfo& model) {
            THashMap<TString, size_t> result;
            for (size_t i = 0; i < model.GetSlices().size(); ++i) {
                const auto& name = model.GetSliceNames()[i];
                const auto& slice = model.GetSlices()[i];
                result[name] = slice.EndFeatureIndexOffset - slice.StartFeatureIndexOffset;
            }
            return result;
        }

        void TestSumSlices(const TMnSseDynamic& left, const TMnSseDynamic& right) {
            TMnSseDynamic sum = SumModels(left, right);
            UNIT_ASSERT_VALUES_EQUAL(sum.GetSlices().empty(), left.GetSlices().empty());
            UNIT_ASSERT_VALUES_EQUAL(sum.GetSlices().empty(), right.GetSlices().empty());
            UNIT_ASSERT_VALUES_EQUAL(sum.HasCustomSlices(), left.HasCustomSlices() || right.HasCustomSlices());

            auto leftSlices = GetSliceSizes(left);
            auto rightSlices = GetSliceSizes(right);
            auto sumSlices = GetSliceSizes(sum);

            for (auto it : sumSlices) {
                TString name = it.first;
                size_t size = it.second;
                UNIT_ASSERT(leftSlices.contains(name) || rightSlices.contains(name));
                UNIT_ASSERT_VALUES_EQUAL(size, Max(leftSlices[name], rightSlices[name]));
            }
            for (auto it : leftSlices) {
                UNIT_ASSERT(sumSlices.contains(it.first));
            }
            for (auto it : rightSlices) {
                UNIT_ASSERT(sumSlices.contains(it.first));
            }
        }

        TFactorStorage UnpackFactors(TStringBuf compressedFactors) {
            TString data = Base64Decode(compressedFactors);
            TStringInput in(data);
            TVector<ui8> tmpBuf;
            auto reader = NFSSaveLoad::CreateCompressedFactorsReader(&in, tmpBuf);
            TFactorStorage storage;
            reader->ReadTo(storage);
            return storage;
        }

        double CalcRelev(const TFactorStorage& storage, const TMnSseDynamic& model) {
            TVector<const TFactorStorage*> storages { &storage };
            double res = 0.0;
            model.DoSlicedCalcRelevs(storages.data(), &res, 1);
            return res;
        };

        double CalcRelevParallel(const TFactorStorage& storage, const TMnSseDynamic& model) {
            TVector<const TFactorStorage*> storages { &storage, &storage }; // Due ParallelCalc is calling only if storages.size() > MaxDocCount > 0
            TVector<double> res;
            NMatrixnet::TParallelMtpOptions opts;
            TSimpleThreadPool pool;
            pool.Start(3, 500);
            opts.Queue = &pool;
            opts.MaxDocCount = 1;
            NMatrixnet::CalcRelevs(&model, storages, res, &opts);
            Y_ENSURE(res.size() == storages.size() && res.front() == res.back());
            return res.front();
        };
    }

    Y_UNIT_TEST(ModelSumTest) {
        TMnSseDynamic modelRu, modelFresh, modelCustom, modelIP25;
        SetModel(modelRu, Ru);
        SetModel(modelFresh, Fresh);
        SetModel(modelCustom, RandomQuarkSlices);
        SetModel(modelIP25, IP25);

        TestSumSlices(modelRu, modelFresh);
        TestSumSlices(modelIP25, modelIP25);
        TestSumSlices(modelRu, modelCustom);

        UNIT_ASSERT_EXCEPTION(SumModels(modelRu, modelIP25), yexception);
        UNIT_ASSERT_EXCEPTION(SumModels(modelCustom, modelIP25), yexception);
    }

    void TestModelsEqual(const TMnSseDynamic& left, const TMnSseDynamic& right) {
        UNIT_ASSERT_VALUES_EQUAL(left.NumTrees(), right.NumTrees());
        UNIT_ASSERT_VALUES_EQUAL(left.NumFeatures(), right.NumFeatures());
        UNIT_ASSERT_VALUES_EQUAL(left.NumBinFeatures(), right.NumBinFeatures());

        const TMnSseStaticMeta& leftStatic = left.GetSseDataPtrs().Meta;
        const TMnSseStaticMeta& rightStatic = right.GetSseDataPtrs().Meta;

        UNIT_ASSERT_VALUES_EQUAL(leftStatic.ValuesSize, rightStatic.ValuesSize);
        UNIT_ASSERT_VALUES_EQUAL(leftStatic.FeaturesSize, rightStatic.FeaturesSize);
        UNIT_ASSERT_VALUES_EQUAL(leftStatic.NumSlices, rightStatic.NumSlices);
        UNIT_ASSERT_VALUES_EQUAL(leftStatic.DataIndicesSize, rightStatic.DataIndicesSize);
        UNIT_ASSERT_VALUES_EQUAL(leftStatic.Has16Bits, rightStatic.Has16Bits);
        UNIT_ASSERT_VALUES_EQUAL(leftStatic.MissedValueDirectionsSize, rightStatic.MissedValueDirectionsSize);
        UNIT_ASSERT_VALUES_EQUAL(leftStatic.SizeToCountSize, rightStatic.SizeToCountSize);

        auto leftData = left.GetSseDataPtrs().Leaves.Data;
        auto rightData = right.GetSseDataPtrs().Leaves.Data;
        const TMultiData& leftMultiData = std::get<TMultiData>(leftData);
        const TMultiData& rightMultiData = std::get<TMultiData>(rightData);

        UNIT_ASSERT_VALUES_EQUAL(leftMultiData.MultiData.size(), rightMultiData.MultiData.size());
        for (size_t i = 0; i < leftMultiData.MultiData.size(); ++i) {
            UNIT_ASSERT(leftMultiData.MultiData[i].Norm.Compare(rightMultiData.MultiData[i].Norm));
        }
        UNIT_ASSERT_VALUES_EQUAL(leftMultiData.DataSize, rightMultiData.DataSize);
    }

    void TestModelsCalc(const TMnSseDynamic& left, const TMnSseDynamic& right) {
        TVector<float> features(left.MaxFactorIndex() + 1);
        UNIT_ASSERT_VALUES_EQUAL(left.CalcRelev(features), right.CalcRelev(features));
    }

    Y_UNIT_TEST(CopyModelTest) {
        TMnSseDynamic modelRu, modelFresh, modelCustom, modelIP25;
        SetModel(modelRu, Ru);
        SetModel(modelFresh, Fresh);
        SetModel(modelCustom, RandomQuarkSlices);
        SetModel(modelIP25, IP25);

        TMnSseDynamic modelCopy;
        modelCopy = modelRu;
        TestModelsEqual(modelRu, modelCopy);
        TestModelsCalc(modelRu, modelCopy);

        modelCopy = modelFresh;
        TestModelsEqual(modelFresh, modelCopy);
        TestModelsCalc(modelFresh, modelCopy);

        modelCopy = modelCustom;
        TestModelsEqual(modelCustom, modelCopy);
        TestModelsCalc(modelCustom, modelCopy);

        modelCopy = modelIP25;
        TestModelsEqual(modelIP25, modelCopy);
        TestModelsCalc(modelIP25, modelCopy);
    }

    Y_UNIT_TEST(SumDynamicBundle) {
        TFactorStorage storage = UnpackFactors("AYoBAABhbGxbMDszNjgwKSBmb3JtdWxhWzA7MzY4MCkgd2ViWzA7MjcwNCkgd2ViX3Byb2R1Y3Rpb25bMDsxOTIxKSB3ZWJfbWV0YVsxOTIxOzI0NDMpIHdlYl9tZXRhX3JlYXJyYW5nZVsyNDQzOzI0OTYpIHdlYl9tZXRhX2w0WzI0OTY7MjU1Nykgd2ViX21ldGFfcGVyc1syNTU3OzI3MDEpIHdlYl9wcm9kdWN0aW9uX2Zvcm11bGFfZmVhdHVyZXNbMjcwMTsyNzA0KSByYXBpZF9jbGlja3NbMjcwNDszMDk1KSByYXBpZF9wZXJzX2NsaWNrc1szMDk1OzMxNzEpIHdlYl9mcmVzaF9kZXRlY3RvclszMTcxOzMzOTEpIGJlZ2Vtb3RfcXVlcnlfZmFjdG9yc1szMzkxOzM2MzIpIGJlZ2Vtb3RfcXVlcnlfcnRfZmFjdG9yc1szNjMyOzM2NTYpIGJlZ2Vtb3RfcXVlcnlfcnRfbDJfZmFjdG9yc1szNjU2OzM2ODApExIAAP8/ln5+UGgvheyg9ScCpHofluaD/525uDi9ZeZlXPn/2szM7N5dQ0FB/m+ZeRlXXPzubv//P8G9vVUdHX13vxQGBtJQUBB3///v///f/3O5p+GWmZdxxTLzMq5YZl7GlZaZl3HFMvMyrlhmXsYVy8zLuGKZeRlYLDMvA8v////f/ff//4HtSYKsz/g2nRYLx413gLUxSqU6+EE+dTQfH24cFxeumZkGsbC4oYyMxnFxsZKSAresLNyyskaS/4/++h8FeDcaA/1THR0t1dHRvv8d9e+v//8fx7w8R/37i/77+//vqP7/f3eX4uDgP7/Q9v+rNjZ2ekzfio4tItnbxCk6lWeZeRlYLDMvA0vVx8fHKymJhYQE2Hp6MhMTg5GQkBoICKC5uLgYu7paZl7GFcvMy7himXkZVywzL+PKXFWc/f9/DQQE1EBAwJqKivqrMjLy3VH9////jwD+/f///48A/v3jVVW1H0pExJ/o5ubEtrb+En6f+CePuAMTtSuUI25n8bu7ff+0weq0RB2JN3xeM/7/95/q6GgaCQnxD9POjvuX4uDgPzVQ+ax8crLK/1UZGZnS0FCDRESMkYsLCAQEYltbaygoKLSoqMCwsP4Rv74eB/38//+YODiYgYCAAPz/P6p2xm8AAMC/kXx8eCUlqTAw0P3vbGVl+aQV2RdEQ+O3F7fROPbiNhoHcM37Kg8PR2dm5j+4J494Jycj7+5G3t093MvLk11d/R/17+/jkZCwMxMTg/78fM9UVFR6c3M/zrm5/yoNDc1cXFyKg4NtmXkZVywzL+OKFAYGfohr3t0xdXScwsBA7///////////X4aCgg7j3t7////////9///////+////////////7/////+mrDYoAP7JI4n8f6SPj0gfH9nLy4t7drYR/fzCqKi4w7a15Ub08yOAf5ehoKCCamr6////////////////////PwAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAEB/////////////v0nONgCAlavszBFRfgq3dqz/ayYmJhQICEEgIJBbW5vU0tJlICCghoKCaiQkJDQoKBQHB8fU0ZHS0NCGWHj+/wWr58c7ORkAAAAAAAAAAAAAAACYZc9Mm+rNud9CTkpHMWKNci6FgYHgWVmhPz8fkocHIBgYdGdnk4GAAJSGhgKCgWEGAgICYDi3uXB6cEfy8CADAQFoTU1liBs1fDRz9/fK6KL7BVaTxgnd9v/jpNumXkC8oUi3R5y3/f/////7P8wiD4/V4TE2IT8YWp+Qi9EHseggQwGfxJBv+kiy9wTXXiYexpO4uHRIndxEkLwSaGSYQXMyf9x1tGZuODLlemdZEBwP5KMO3/4kXArrBXtFAs06ATKL/Q8AAIChoKC8tbU1jIkJGBYW9fHxWUpKwkpKCt7a2oJBQWEiIgI+KSnk1tYmu7o6VEhIBrKxnRQcvYVQUJCBgAC8tbVlLi6OxsPDaywsjKajo7Ofn88MBAQEgMHAwBoLCyusqqoZCAgIwDUZGVlTUVE1EBCA//7+DAYGuoaCgspwcHDNxsZmJSUllIiIAA8veSEstu4wn4Sqd5ChIGb6KoNeJx3I10kH8nKh7lDS+gj9AAAAULMC8yBW6k/g930Cv+8/rpmZTEREqLSngASvJVOmtEL+hnYZHmhiGB04GdjgR6j5ibY0cbCoqEOHhia0srLo3d2MhIQAhoXFRkYG9vPzDwIBAce8vAHk46P//hZcW7vGwsIggH/HysenoYyMuaurC+Pe3iAREZ2RkJAaCwtDamlpbDw8BNfW5qqqqlBKSohfX2HZ2BBSUVGloaEMZGMbBgACjxHYBJOAot3dDfnvoyyj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");
        TMnSseDynamic modelRu, modelFresh, modelCustom;
        SetModel(modelRu, Ru);
        SetModel(modelFresh, Fresh);
        SetModel(modelCustom, RandomQuarkSlices);
        NFactorSlices::TFullFactorIndex ruIndex =
            { EFactorSlice::WEB_META_REARRANGE, NWebMeta::NSliceWebMetaRearrange::EFactorId::FI_REARR_FAST_SLOT_0 };
        NFactorSlices::TFullFactorIndex modelIndex =
            { EFactorSlice::WEB_META_REARRANGE, NWebMeta::NSliceWebMetaRearrange::EFactorId::FI_REARR_FAST_SLOT_1 };
        NFactorSlices::TFullFactorIndex customIndex =
            { EFactorSlice::WEB_META_REARRANGE, NWebMeta::NSliceWebMetaRearrange::EFactorId::FI_REARR_FAST_SLOT_2 };
        TVector<std::pair<NMatrixnet::TMnSseDynamic, NFactorSlices::TFullFactorIndex>> components = {
            { modelRu, ruIndex },
            { modelFresh, modelIndex },
            { modelCustom, customIndex }
        };
        storage[ruIndex] = 0.123;
        storage[modelIndex] = 0.5;
        storage[customIndex] = 0.8231;
        TMnSseDynamic sum = TMnSseDynamic::CreateDynamicBundle(components);

        UNIT_ASSERT_EQUAL(modelRu.NumTrees() + modelFresh.NumTrees() + modelCustom.NumTrees(), sum.NumTrees());

        NMatrixnet::TDynamicBundle::IsDynamicBundle(sum);

        double res1 = 0.0;
        for (const auto& [model, index] : components) {
            res1 += CalcRelev(storage, model) * storage[index];
        }
        double res2 = CalcRelev(storage, sum);
        double res3 = CalcRelevParallel(storage, sum);
        UNIT_ASSERT_DOUBLES_EQUAL(res1, res2, 1e-6);
        UNIT_ASSERT_DOUBLES_EQUAL(res1, res3, 1e-6);
    }

    Y_UNIT_TEST(CheckNonExistentSlicesForDynamicBundle) {
        TMnSseDynamic modelRu;
        SetModel(modelRu, Ru);
        NFactorSlices::TFullFactorIndex ruIndex =
            { EFactorSlice::FRESH, 10 };
        TVector<std::pair<NMatrixnet::TMnSseDynamic, NFactorSlices::TFullFactorIndex>> components = {
            { modelRu, ruIndex }
        };
        TMnSseDynamic sum = TMnSseDynamic::CreateDynamicBundle(components);
        TString bordersString = "fresh[0;11) web[11;999)";
        UNIT_ASSERT_EQUAL(sum.GetInfo()->at("Slices"), bordersString);

        UNIT_ASSERT(NMatrixnet::TDynamicBundle::IsDynamicBundle(sum));
        UNIT_ASSERT_EQUAL(modelRu.NumTrees(), sum.NumTrees());

        TFactorStorage storage;
        NFactorSlices::TFactorBorders borders;
        NFactorSlices::DeserializeFactorBorders(bordersString, borders);
        storage.SetBorders(borders);
        for (size_t ind = 0; ind < storage.Size(); ++ind) {
            storage[ind] = 0.5 + double(ind) / storage.Size() / 2.0;
        }

        double w1 = 0.123;
        storage[ruIndex] = w1;

        double res1 = CalcRelev(storage, modelRu) * w1;
        double res2 = CalcRelev(storage, sum);
        double res3 = CalcRelevParallel(storage, sum);
        UNIT_ASSERT_DOUBLES_EQUAL(res1, res2, 1e-6);
        UNIT_ASSERT_DOUBLES_EQUAL(res1, res3, 1e-6);
    }

    Y_UNIT_TEST(CheckStandaloneBinarization) {
        TMnSseDynamic modelRu, modelFresh;
        SetModel(modelRu, Ru);
        SetModel(modelFresh, Fresh);

        TFactorBorders borders;
        borders[EFactorSlice::FRESH] = TSliceOffsets(0, 100);
        borders[EFactorSlice::WEB_PRODUCTION] = TSliceOffsets(100, 1088);
        UNIT_ASSERT(NFactorSlices::NDetail::ReConstruct(borders));
        TFactorStorage storage(borders);
        UNIT_ASSERT(storage.Size() == 1088);
        for (size_t i = 0; i < storage.Size(); ++i) {
            storage[i] = float(i) / storage.Size();
        }

        TStandaloneBinarization resBinarization;
        resBinarization.MergeWith(TStandaloneBinarization::From(modelRu));
        resBinarization.MergeWith(TStandaloneBinarization::From(modelFresh));
        auto modelRuRebuilt = resBinarization.RebuildModel(modelRu);
        auto modelFreshRebuilt = resBinarization.RebuildModel(modelFresh);
        UNIT_ASSERT_DOUBLES_EQUAL(CalcRelev(storage, modelRu), CalcRelev(storage, modelRuRebuilt), 0);
        UNIT_ASSERT_DOUBLES_EQUAL(CalcRelev(storage, modelFresh), CalcRelev(storage, modelFreshRebuilt), 0);
    }

    Y_UNIT_TEST(SerializeBinarization) {
        TMnSseDynamic modelRu;
        SetModel(modelRu, Ru);
        auto binarization = TStandaloneBinarization::From(modelRu);
        TString serialized;
        {
            TStringOutput out(serialized);
            binarization.Save(&out);
        }
        TStandaloneBinarization deserialized;
        {
            TStringInput in(serialized);
            deserialized.Load(&in);
        }
        UNIT_ASSERT(binarization.GetFeatures() == deserialized.GetFeatures());
    }
}
