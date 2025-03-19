#include <kernel/matrixnet/mn_sse.h>
#include <kernel/matrixnet/mn_dynamic.h>

#include <library/cpp/archive/yarchive.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/buffer.h>
#include <util/memory/blob.h>
#include <util/stream/buffer.h>
#include <util/stream/mem.h>

Y_UNIT_TEST_SUITE(TMnSerializerTest) {

namespace {
    using namespace NMatrixnet;

    extern "C" {
        extern const unsigned char TestModels[];
        extern const ui32 TestModelsSize;
    };

    static const TString Ru("/Ru.info");
    static const TString IP25("/ip25.info");
    static const TString Fresh("/RuFreshExtRelev.info");
    static const TString RandomQuarkSlices("/quark_slices.info");
    static const TString FlatbufSingleDataModel("/FlatbufSingleDataModel.info");

    TBlob GetBlob(const TString& modelName) {
        TArchiveReader archive(TBlob::NoCopy(TestModels, TestModelsSize));
        return archive.ObjectBlobByKey(modelName);
    }

    void AssertSseStaticsEqual(const TMnSseStatic& leftAll, const TMnSseStatic& rightAll) {
        const TMnSseStaticMeta& left = leftAll.Meta;
        const TMnSseStaticMeta& right = leftAll.Meta;

        UNIT_ASSERT_VALUES_EQUAL(left.ValuesSize, right.ValuesSize);
        if (left.ValuesSize == right.ValuesSize) {
            for (size_t i = 0; i != left.ValuesSize; ++i)
                UNIT_ASSERT_VALUES_EQUAL_C(left.Values[i], right.Values[i], "i=" << i);
        }

        UNIT_ASSERT_VALUES_EQUAL(left.FeaturesSize, right.FeaturesSize);
        if (left.FeaturesSize == right.FeaturesSize) {
            for (size_t i = 0; i != left.FeaturesSize; ++i) {
                UNIT_ASSERT_VALUES_EQUAL_C(left.Features[i].Index, right.Features[i].Index, "i=" << i);
                UNIT_ASSERT_VALUES_EQUAL_C(left.Features[i].Length, right.Features[i].Length, "i=" << i);
            }
        }

        UNIT_ASSERT_VALUES_EQUAL(left.NumSlices, right.NumSlices);
        if (left.NumSlices == right.NumSlices) {
            for (size_t i = 0; i != left.NumSlices; ++i) {
                UNIT_ASSERT_VALUES_EQUAL_C(left.FeatureSlices[i].StartFeatureIndexOffset,
                                           right.FeatureSlices[i].StartFeatureIndexOffset, "i=" << i);
                UNIT_ASSERT_VALUES_EQUAL_C(left.FeatureSlices[i].StartFeatureOffset,
                                           right.FeatureSlices[i].StartFeatureOffset, "i=" << i);
                UNIT_ASSERT_VALUES_EQUAL_C(left.FeatureSlices[i].StartValueOffset,
                                           right.FeatureSlices[i].StartValueOffset, "i=" << i);
                UNIT_ASSERT_VALUES_EQUAL_C(left.FeatureSlices[i].EndFeatureIndexOffset,
                                           right.FeatureSlices[i].EndFeatureIndexOffset, "i=" << i);
                UNIT_ASSERT_VALUES_EQUAL_C(left.FeatureSlices[i].EndFeatureOffset,
                                           right.FeatureSlices[i].EndFeatureOffset, "i=" << i);
                UNIT_ASSERT_VALUES_EQUAL_C(left.FeatureSlices[i].EndValueOffset,
                                           right.FeatureSlices[i].EndValueOffset, "i=" << i);
            }
        }

        UNIT_ASSERT_VALUES_EQUAL(left.DataIndicesSize, right.DataIndicesSize);
        UNIT_ASSERT_VALUES_EQUAL(left.Has16Bits, right.Has16Bits);
        if (left.DataIndicesSize == right.DataIndicesSize &&
            left.Has16Bits == right.Has16Bits) {
            if (left.Has16Bits) {
                for (size_t i = 0; i != left.DataIndicesSize; ++i)
                    UNIT_ASSERT_VALUES_EQUAL_C(((const ui16*)left.DataIndicesPtr)[i],
                                               ((const ui16*)right.DataIndicesPtr)[i], "i=" << i);
            } else {
                for (size_t i = 0; i != left.DataIndicesSize; ++i)
                    UNIT_ASSERT_VALUES_EQUAL_C(((const ui32*)left.DataIndicesPtr)[i],
                                               ((const ui32*)right.DataIndicesPtr)[i], "i=" << i);
            }
        }

        UNIT_ASSERT_VALUES_EQUAL(left.SizeToCountSize, right.SizeToCountSize);
        if (left.SizeToCountSize == right.SizeToCountSize) {
            for (size_t i = 0; i != left.SizeToCountSize; ++i)
                UNIT_ASSERT_VALUES_EQUAL_C(left.SizeToCount[i], right.SizeToCount[i], "i=" << i);
        }

        const TMultiData& leftMultiData = std::get<TMultiData>(leftAll.Leaves.Data);
        const TMultiData& rightMultiData = std::get<TMultiData>(rightAll.Leaves.Data);

        UNIT_ASSERT_VALUES_EQUAL(leftMultiData.MultiData.size(), rightMultiData.MultiData.size());
        UNIT_ASSERT_VALUES_EQUAL(leftMultiData.DataSize, rightMultiData.DataSize);
        if (leftMultiData.DataSize == rightMultiData.DataSize) {
            for (size_t i = 0; i != leftMultiData.MultiData.size(); ++i) {
                for (size_t j = 0; j != leftMultiData.DataSize; ++j) {
                    UNIT_ASSERT_VALUES_EQUAL_C(leftMultiData.MultiData[i].Data[j], rightMultiData.MultiData[i].Data[j], "dataId=" << i);
                }
                UNIT_ASSERT(leftMultiData.MultiData[i].Norm.Compare(rightMultiData.MultiData[i].Norm));
            }
        }
    }

    void AssertModelInfosEqual(const TModelInfo& left, const TModelInfo& right) {
        for (auto it = left.begin(); it != left.end(); ++it) {
            const TString& key = it->first;
            const TString* leftValue = &it->second;
            const TString* rightValue = right.FindPtr(key);
            UNIT_ASSERT_C(rightValue, "Key '" << key << "' exists in the left map only");
            if (rightValue)
                UNIT_ASSERT_VALUES_EQUAL_C(*leftValue, *rightValue, "key=" << key);
        }
        if (left.size() != right.size()) {
            for (auto it = right.begin(); it != right.end(); ++it) {
                const TString& key = it->first;
                UNIT_ASSERT_C(left.contains(key), "Key '" << key << "' exists in the right map only");
            }
        }
    }

    void AssertSseInfosEqual(const TMnSseInfo& left, const TMnSseInfo& right) {
        AssertSseStaticsEqual(left.GetSseDataPtrs(), right.GetSseDataPtrs());
        AssertModelInfosEqual(*left.GetInfo(), *right.GetInfo());
    }

    void TestLoadVsInitStaticEquivalence(const TString& modelName) {
        TBlob blob = GetBlob(modelName);
        TMemoryInput in(blob.Data(), blob.Length());
        TMnSseDynamic m1;
        m1.Load(&in);

        TMnSseInfo m2;
        m2.InitStatic(blob.Data(), blob.Length());
        AssertSseInfosEqual(m1, m2);
    }

    void TestSaveThenLoadDontChangeModel(const TString& modelName) {
        TBlob blob = GetBlob(modelName);
        TMemoryInput in(blob.Data(), blob.Length());
        TMnSseDynamic m1;
        m1.Load(&in);

        TBuffer buf;
        TBufferOutput out(buf);
        m1.Save(&out);

        TMnSseDynamic m2;
        in.Reset(buf.Data(), buf.Size());
        m2.Load(&in);
        AssertSseInfosEqual(m1, m2);
    }

    void TestSaveLoadContainerWithModels(const TVector<TString>& modelNames) {
        THashMap<TString, TMnSseDynamic> models;
        models.reserve(modelNames.size());

        for (const TString& modelName : modelNames) {
            TBlob blob = GetBlob(modelName);
            TMemoryInput in(blob.Data(), blob.Length());
            TMnSseDynamic m;
            m.Load(&in);
            models[modelName] = m;
        }

        TBuffer buf;
        TBufferOutput out(buf);
        ::Save(&out, models);

        THashMap<TString, TMnSseDynamic> restoredModels;
        TBufferInput in(buf);
        ::Load(&in, restoredModels);

        for (const auto& model : models) {
            AssertSseInfosEqual(models[model.first], restoredModels[model.first]);
        }
    }

} // namespace

Y_UNIT_TEST(LoadVsInitStaticEquivalence) {
    TestLoadVsInitStaticEquivalence(Ru);
    TestLoadVsInitStaticEquivalence(FlatbufSingleDataModel);
}

Y_UNIT_TEST(SaveThenLoadDontChangeModel) {
    TestSaveThenLoadDontChangeModel(Ru);
    TestSaveThenLoadDontChangeModel(IP25);
    TestSaveThenLoadDontChangeModel(Fresh);
    TestSaveThenLoadDontChangeModel(RandomQuarkSlices);
    TestSaveThenLoadDontChangeModel(FlatbufSingleDataModel);

    TestSaveLoadContainerWithModels({Ru, IP25, Fresh, RandomQuarkSlices, FlatbufSingleDataModel});
}

}
