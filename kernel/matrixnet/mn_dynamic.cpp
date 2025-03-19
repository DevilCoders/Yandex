#include "mn_dynamic.h"

#include <library/cpp/digest/md5/md5.h>

#include <kernel/factor_slices/factor_borders.h>
#include <kernel/factor_storage/factor_storage.h>

#include <util/ysaveload.h>
#include <util/generic/xrange.h>
#include <util/generic/algorithm.h>
#include <util/generic/ymath.h>
#include <util/stream/mem.h>
#include <util/stream/multi.h>
#include <util/string/printf.h>
#include <util/string/cast.h>

#include <utility>

namespace NMatrixnet {

TMnSseDynamic::TMnSseDynamic(
        const float *values,      const size_t valuesSize,
        const TFeature* features, const size_t featuresSize,
        const i8* missedValDirs,  const size_t missedValDirsSize,
        const ui32* dInd,         const size_t dIndSize,
        const int* sizeToCount,   const size_t sizeToCountSize,
        const int* data,          const size_t dataSize,
        const double bias,        const double scale)
{
    CopyFrom(values, valuesSize,
             features, featuresSize,
             missedValDirs, missedValDirsSize,
             dInd, dIndSize,
             sizeToCount, sizeToCountSize,
             data, dataSize,
             bias, scale);
}

TMnSseDynamic::TMnSseDynamic(const TMnSseDynamic& matrixnet)
    : TMnSseModel(matrixnet)
{
    UpdateMatrixnet();
}

TMnSseDynamic& TMnSseDynamic::operator=(const TMnSseDynamic& matrixnet) {
    if (this != &matrixnet) {
        TMnSseModel::CopyFrom(matrixnet);
    }
    return *this;
}

TMnSseDynamic::TMnSseDynamic(const TMnSseStatic& matrixnet) {
    CopyFrom(matrixnet);
}

TMnSseDynamic::TMnSseDynamic(const TMnSseInfo& matrixnet) {
    CopyFrom(matrixnet);
}

void TMnSseDynamic::CopyFrom(
        const float* values,      const size_t valuesSize,
        const TFeature* features, const size_t featuresSize,
        const i8* missedValDirs,  const size_t missedValDirsSize,
        const ui32* dInd,         const size_t dIndSize,
        const int* sizeToCount,   const size_t sizeToCountSize,
        const int* data,          const size_t dataSize,
        const double bias,        const double scale)
{
    TMnSseStaticMeta meta(
            values, valuesSize,
            features, featuresSize,
            missedValDirs, missedValDirsSize,
            dInd, dIndSize,
            sizeToCount, sizeToCountSize);

    TMultiData::TLeafData dataHolder(data, bias, scale);
    TMnSseStaticLeaves leaves(TMultiData({dataHolder}, dataSize));

    TMnSseModel::CopyFrom(meta, leaves);
}

void TMnSseDynamic::CopyFrom(const TMnSseStatic& matrixnet) {
    Y_ENSURE(std::get<TMultiData>(matrixnet.Leaves.Data).MultiData.size() == 1, "TMnSseDynamic copy from multivalue model unsupported");
    TMnSseModel::CopyFrom(matrixnet);
}

void TMnSseDynamic::CopyFrom(const TMnSseInfo& matrixnet) {
    Y_ENSURE(std::get<TMultiData>(matrixnet.GetSseDataPtrs().Leaves.Data).MultiData.size() == 1, "TMnSseDynamic copy from multivalue model unsupported");
    TMnSseModel::CopyFrom(matrixnet);
}

void TMnSseDynamic::Swap(TMnSseDynamic& obj) {
    TMnSseModel::Swap(obj);
}

void TMnSseDynamic::Clear() {
    TMnSseModel::Clear();
}

void TMnSseDynamic::Load(IInputStream* in) {
    TMnSseModel::Load(in);
    Y_ENSURE(OwnedMultiData().size() == 1, "multivalue model loading usupported");
    TModelInfo::const_iterator itBias = Info.find("NativeDataBias");
    TModelInfo::const_iterator itScale = Info.find("NativeDataBias");
    GetSingleMxData().Norm.NativeDataBias
            = (itBias != Info.end()) ? FromString<double>(itBias->second) : GetSingleMxData().Norm.DataBias;
    GetSingleMxData().Norm.NativeDataScale
            = (itScale != Info.end()) ? FromString<double>(itScale->second) : GetSingleMxData().Norm.DataScale;
}

struct TTreeIndices {
    size_t DataIndex;
    size_t DataIndicesIndex;
    size_t SizeToCountIndex;
    size_t SizeToCountValue;
};

/* warning: if treeIndex is more than total amount of trees then
 * this function will return indices to the end of data
 */
static TTreeIndices CalcDataIndices(const size_t treeIndex, const TVector<int>& sizeToCount) {
    size_t treeCondIdx = 0; // number of trees from the beginning to current
    int condNum = 0;     // number of conditions in current tree.
    int treeSize = 1;    // number of leafs in current tree
    int dataIdx = 0;     // index in Data array with tree values
    int dataIndicesIdx = 0;     // index in DataIndices array with tree values
    for (; condNum < sizeToCount.ysize(); condNum++) {
        const size_t curTreesNum = sizeToCount[condNum];
        if (treeCondIdx + curTreesNum >= treeIndex || condNum == sizeToCount.ysize() - 1) {
            break;
        }
        treeCondIdx += curTreesNum;
        dataIndicesIdx += condNum * curTreesNum;
        dataIdx += treeSize * curTreesNum;
        treeSize *= 2;
    }
    Y_ASSERT(condNum < sizeToCount.ysize() || sizeToCount.empty());
    int treeOffset = treeIndex - treeCondIdx;
    if (sizeToCount.empty()) {
        treeOffset = 0;
    } else if (treeOffset > sizeToCount[condNum]) {
        treeOffset = sizeToCount[condNum];
    }
    TTreeIndices result;
    result.DataIndex = dataIdx + treeSize * treeOffset;
    result.DataIndicesIndex = dataIndicesIdx + condNum * treeOffset;
    result.SizeToCountIndex = condNum;
    result.SizeToCountValue = treeOffset;
    return result;
}

void TMnSseDynamic::SplitTreesBySpecifiedFactors(const TSet<ui32>& factorsIndices, TMnSseDynamic &modelWithFactors, TMnSseDynamic &modelWithoutFactors) const {
    Y_ENSURE(OwnedMultiData().size() == 1, "CopyTreeRange currently don't work with multidata models");
    // Prepare flags for features (true if features belongs to specified factor)
    TVector<bool> featuresToKeep(Values.size());
    size_t treeCondIdx1 = 0;
    for (size_t i = 0; i < Features.size(); ++i) {
        const TFeature &factor = Features[i];
        if (factorsIndices.contains(factor.Index)) {
            for (size_t j = 0; j < factor.Length; ++j) {
                featuresToKeep[treeCondIdx1 + j] = true;
            }
        }
        treeCondIdx1 += factor.Length;
    }
    // Do split data
    TVector<int> sizeToCountWith, sizeToCountWithout;
    TVector<ui32> dataIndicesWith, dataIndicesWithout;
    TVector<int> dataWith, dataWithout;
    for (size_t condNum = 0, treeSize = 1, treeCondIdx2 = 0, dataIdx = 0; condNum < SizeToCount.size(); ++condNum, treeSize <<= 1) {
        size_t treeCountWith = 0, treeCountWithout = 0;
        for (size_t treeNum = 0, treeCount = SizeToCount[condNum]; treeNum < treeCount; ++treeNum) {
            bool isGoodTree = false;
            for (size_t i = 0; i < condNum && !isGoodTree; ++i) {
                size_t condIdx = DataIndices[treeCondIdx2 + i] / 4;
                if (featuresToKeep[condIdx]) {
                    isGoodTree = true;
                }
            }
            TVector<ui32> *dataIndices = isGoodTree ? &dataIndicesWith : &dataIndicesWithout;
            TVector<int> *data = isGoodTree ? &dataWith : &dataWithout;
            size_t *newTreeCount = isGoodTree ? &treeCountWith : &treeCountWithout;
            ++*newTreeCount;
            for (size_t i = 0; i < condNum; ++i) {
                dataIndices->push_back(DataIndices[treeCondIdx2 + i]);
            }
            for (size_t i = 0; i < (1u << condNum); ++i) {
                data->push_back(OwnedMultiData()[0][dataIdx + i]);
            }
            treeCondIdx2 += condNum;
            dataIdx += (1 << condNum);
        }
        sizeToCountWith.push_back(treeCountWith);
        sizeToCountWithout.push_back(treeCountWithout);
    }
    // Remove leading zeros
    while (!sizeToCountWith.empty() && sizeToCountWith.back() == 0) {
        sizeToCountWith.pop_back();
    }
    while (!sizeToCountWithout.empty() && sizeToCountWithout.back() == 0) {
        sizeToCountWithout.pop_back();
    }
    // Copy data to results
    modelWithFactors.Clear();
    modelWithoutFactors.Clear();
    if (!Values.empty() && !Features.empty() && !dataIndicesWith.empty() && !sizeToCountWith.empty() && !dataWith.empty()) {
        modelWithFactors.CopyFrom(&Values[0], Values.size(),
                                  &Features[0], Features.size(),
                                  MissedValueDirections.empty() ? nullptr : &MissedValueDirections[0], MissedValueDirections.size(),
                                  &dataIndicesWith[0], dataIndicesWith.size(),
                                  &sizeToCountWith[0], sizeToCountWith.size(),
                                  &dataWith[0], dataWith.size(),
                                  0.0, Scale());
    }
    if (!Values.empty() && !Features.empty() && !dataIndicesWithout.empty() && !sizeToCountWithout.empty() && !dataWithout.empty()) {
        modelWithoutFactors.CopyFrom(&Values[0], Values.size(),
                                     &Features[0], Features.size(),
                                     MissedValueDirections.empty() ? nullptr : &MissedValueDirections[0], MissedValueDirections.size(),
                                     &dataIndicesWithout[0], dataIndicesWithout.size(),
                                     &sizeToCountWithout[0], sizeToCountWithout.size(),
                                     &dataWithout[0], dataWithout.size(),
                                     Bias(), Scale());
    }
    // Copy slices info
    const auto& it = Info.find("Slices");
    if (it != Info.end()) {
        modelWithFactors.Info[it->first] = it->second;
        modelWithoutFactors.Info[it->first] = it->second;
    }
    // UpdateModels
    modelWithFactors.RemoveUnusedFeatures();
    modelWithFactors.UpdateMatrixnet();
    modelWithoutFactors.RemoveUnusedFeatures();
    modelWithoutFactors.UpdateMatrixnet();
}

TMnSseDynamic TMnSseDynamic::CopyTreeRange(const size_t beginIndex, const size_t endIndex) const {
    Y_ENSURE(OwnedMultiData().size() == 1, "CopyTreeRange currently don't work with multidata models");
    TMnSseDynamic result;
    result.Info = Info;
    result.Info["truncated-tree-range"] = "[" + ToString(beginIndex) + ", " + ToString(endIndex) + ")";

    if (beginIndex == 0) {
        result.GetSingleMxData().Norm.DataBias = GetSingleMxData().Norm.DataBias;
    } else {
        result.GetSingleMxData().Norm.DataBias = 0;
    }
    result.GetSingleMxData().Norm.DataScale = GetSingleMxData().Norm.DataScale;
    result.Values = Values;
    result.Features = Features;
    result.MissedValueDirections = MissedValueDirections;
    if (endIndex > beginIndex) {
        TTreeIndices beginIndices = CalcDataIndices(beginIndex, SizeToCount);
        TTreeIndices endIndices = CalcDataIndices(endIndex, SizeToCount);
        result.OwnedMultiData().resize(OwnedMultiData().size());
        for (auto dataId : xrange(OwnedMultiData().size())) {
            result.OwnedMultiData()[dataId].assign(OwnedMultiData()[dataId].begin() + beginIndices.DataIndex, OwnedMultiData()[dataId].begin() + endIndices.DataIndex);
        }
        result.DataIndices.assign(DataIndices.begin() + beginIndices.DataIndicesIndex, DataIndices.begin() + endIndices.DataIndicesIndex);
        result.SizeToCount.resize(endIndices.SizeToCountIndex + 1, 0);
        if (beginIndices.SizeToCountIndex < endIndices.SizeToCountIndex) {
            size_t minSize = beginIndices.SizeToCountIndex;
            size_t maxSize = endIndices.SizeToCountIndex;
            result.SizeToCount[minSize] = SizeToCount[minSize] - beginIndices.SizeToCountValue;
            for (size_t sizeIndex = minSize + 1; sizeIndex < maxSize; ++sizeIndex) {
                result.SizeToCount[sizeIndex] = SizeToCount[sizeIndex];
            }
            result.SizeToCount[maxSize] = endIndices.SizeToCountValue;
        } else if (beginIndices.SizeToCountIndex == endIndices.SizeToCountIndex) {
            size_t treeSize = beginIndices.SizeToCountIndex;
            if (beginIndices.SizeToCountValue < endIndices.SizeToCountValue) {
                result.SizeToCount[treeSize] = endIndices.SizeToCountValue - beginIndices.SizeToCountValue;
            }
        }
    }
    result.RemoveUnusedFeatures();
    result.UpdateMatrixnet();
    return result;
}

TVector<TMnSseDynamic> TMnSseDynamic::SplitTrees(size_t treeCount) const {
    size_t sourceCount = std::accumulate(SizeToCount.begin(), SizeToCount.end(), 0);
    TVector<TMnSseDynamic> result;
    if (sourceCount == 0) {
        return result;
    }
    if (treeCount == 0) {
        treeCount = (sourceCount + 1) / 2;
    }
    for (size_t currentCount = 0; currentCount < sourceCount; currentCount += treeCount) {
        result.push_back(CopyTreeRange(currentCount, currentCount + treeCount));
    }
    return result;
}

static inline int Float2Int(const float fpVar)
{
    if (fpVar > 0) {
        if (static_cast<int>(fpVar + 0.5) != static_cast<i64>(fpVar + 0.5))
            ythrow yexception() << "overflow in float to int conversion (positive float)";
        return static_cast<int>(fpVar + 0.5f);
    } else {
        if (static_cast<int>(fpVar - 0.5) != static_cast<i64>(fpVar - 0.5))
            ythrow yexception() << "overflow in float to int conversion (negative float)";
        return static_cast<int>(fpVar - 0.5f);
    }
}

struct TFeatureIndexWithDirection {
    int FeatureIndex;
    i8 Direction;

    bool operator<(const TFeatureIndexWithDirection& other) const {
        return FeatureIndex < other.FeatureIndex || (FeatureIndex == other.FeatureIndex && Direction < other.Direction);
    }

    bool operator==(const TFeatureIndexWithDirection& other) const {
        return FeatureIndex == other.FeatureIndex && Direction == other.Direction;
    }

    // hash function for hashmap
    explicit operator size_t() const {
        return CombineHashes(THash<int>()(FeatureIndex), THash<i8>()(Direction));
    }
};

struct TValueRemapData {
    const TVector<TFeature>& Features;
    const TVector<float>& Values;
    const TVector<TFeatureIndexWithDirection>& Remap;
public:
    TValueRemapData(const TVector<TFeature>& features, const TVector<float>& values, const TVector<TFeatureIndexWithDirection>& remap)
        : Features(features)
        , Values(values)
        , Remap(remap)
    {
    }
    void FillNewFeatureBorders(TMap<TFeatureIndexWithDirection, TMap<float, int>>& resultBorders) const {
        size_t valueIndex = 0;
        for (size_t i = 0; i < Features.size(); ++i) {
            for (size_t j = 0; j < Features[i].Length; ++j) {
                TMap<float, int>& currentBorders = resultBorders[Remap[i]];
                if (!currentBorders.contains(Values[valueIndex])) {
                    size_t nextId = currentBorders.size();
                    currentBorders[Values[valueIndex]] = nextId;
                }
                ++valueIndex;
            }
        }
    }
    TVector<int> CalcValuesRemap(const THashMap<std::pair<TFeatureIndexWithDirection, float>, int>& binFeatureRemap) const {
        TVector<int> result(Values.size());
        size_t valueIndex = 0;
        for (size_t i = 0; i < Features.size(); ++i) {
            auto featureWithDirection = Remap[i];
            for (size_t j = 0; j < Features[i].Length; ++j) {
                float value = Values[valueIndex];
                result[valueIndex] = binFeatureRemap.at(std::make_pair(featureWithDirection, value));
                ++valueIndex;
            }
        }
        return result;
    }
};

static NMLPool::TFeatureSlices JoinSlices(const NMLPool::TFeatureSlices& borders1, const NMLPool::TFeatureSlices& borders2) {
    NMLPool::TFeatureSlices joinedBorders;
    NFactorSlices::TFactorIndex prevOffset = 0;
    TMap<TString, size_t> sliceSize;
    auto expandSlices = [&sliceSize](const NMLPool::TFeatureSlices& borders) {
        for (auto& border : borders) {
            size_t newSize = border.End - border.Begin;
            size_t& size = sliceSize[border.Name];
            size = Max(newSize, size);
        }
    };
    expandSlices(borders1);
    expandSlices(borders2);
    auto addBorder = [&prevOffset, &joinedBorders](const TString& name, size_t joinedSize) {
        if (joinedSize > 0) {
            if (joinedSize > size_t(Max<NFactorSlices::TFactorIndex>() - prevOffset)) {
                ythrow yexception() << "Too many features or End < Begin in slices";
            }
            joinedBorders.emplace_back(name, prevOffset, prevOffset + joinedSize);
            prevOffset += joinedSize;
        }
    };
    // process known slices first
    const NFactorSlices::TAllSlicesArray& allSlices = NFactorSlices::GetAllFactorSlices();
    for (NFactorSlices::EFactorSlice sliceId : allSlices) {
        const TString name = ToString(sliceId);
        auto it = sliceSize.find(name);
        if (it != sliceSize.end()) {
            addBorder(it->first, it->second);
            sliceSize.erase(it);
        }
    }
    // process unknown slices
    for (const auto& it : sliceSize) {
        addBorder(it.first, it.second);
    }
    return joinedBorders;
}

static void FillFeaturesRemap(TVector<TFeatureIndexWithDirection>& featuresRemap,
                              const TVector<TFeature>& mnFeatures,
                              const TVector<i8>& missedValueDirections,
                              const NMLPool::TFeatureSlices& oldBorders,
                              NMLPool::TFeatureSlices& newBorders) {
    featuresRemap.resize(mnFeatures.size());
    THashMap<TString, NFactorSlices::TSliceOffsets> newOffsetMap;
    for (const NMLPool::TFeatureSlice& slice : newBorders) {
        newOffsetMap[slice.Name] = NFactorSlices::TSliceOffsets(slice.Begin, slice.End);
    }
    for (const NMLPool::TFeatureSlice& slice : oldBorders) {
        NFactorSlices::TSliceOffsets oldOffset(slice.Begin, slice.End);
        if (!oldOffset.Empty()) {
            NFactorSlices::TSliceOffsets newOffset = newOffsetMap.at(slice.Name);
            for (size_t i = 0; i < mnFeatures.size(); ++i) {
                NFactorSlices::TFactorIndex currentFeature = mnFeatures[i].Index;
                if (oldOffset.Contains(currentFeature)) {
                    NFactorSlices::TFactorIndex relativeIndex = oldOffset.GetRelativeIndex(currentFeature);
                    auto missedValueDirection = !!missedValueDirections ? missedValueDirections[i] : (i8)NMatrixnetIdl::EFeatureDirection_None;
                    featuresRemap[i] = TFeatureIndexWithDirection{newOffset.GetIndex(relativeIndex), missedValueDirection};
                }
            }
        }
    }
}

void TMnSseDynamic::Add(const TMnSseDynamic &mn, const float alpha/* = 0.5*/)
{
    AddWithCoefficients(mn, alpha, 1 - alpha);
}

void TMnSseDynamic::AddWithCoefficients(const TMnSseDynamic &mn, const float alpha, const float beta)
{
    const TMnSseDynamic& mn1 = *this;
    const TMnSseDynamic& mn2 = mn;
    Y_ENSURE(mn1.Matrixnet.Meta.DynamicBundle.empty(), "AddWithCoefficients unsupported for dynamic bundles");
    Y_ENSURE(mn2.Matrixnet.Meta.DynamicBundle.empty(), "AddWithCoefficients unsupported for dynamic bundles");
    Y_ENSURE(mn1.OwnedMultiData().size() == 1, "AddWithCoefficients unsupported for multidata models");
    Y_ASSERT(std::get<TMultiData>(mn1.Matrixnet.Leaves.Data).MultiData.size() == std::get<TMultiData>(Matrixnet.Leaves.Data).MultiData.size());
    Y_ENSURE(mn2.OwnedMultiData().size() == 1, "AddWithCoefficients unsupported for multidata models");
    Y_ASSERT(std::get<TMultiData>(mn2.Matrixnet.Leaves.Data).MultiData.size() == std::get<TMultiData>(Matrixnet.Leaves.Data).MultiData.size());
    TMnSseDynamic result;
    const bool hasMissedValueDirections = !!mn1.MissedValueDirections || !!mn2.MissedValueDirections;
    NMLPool::TFeatureSlices borders1;
    NMLPool::TFeatureSlices borders2;
    NMLPool::TFeatureSlices joinedBorders;
    if (mn1.Info.contains("Slices") && mn2.Info.contains("Slices")) {
        const bool customSlices = mn1.HasCustomSlices() || mn2.HasCustomSlices();
        NFactorSlices::DeserializeFeatureSlices(mn1.Info.at("Slices"), customSlices, borders1);
        NFactorSlices::DeserializeFeatureSlices(mn2.Info.at("Slices"), customSlices, borders2);
        joinedBorders = JoinSlices(borders1, borders2);
        if (customSlices) {
            result.Info["Slices"] = ToString(joinedBorders);
            result.Info["CustomSlices"] = "on";
        } else {
            NFactorSlices::TFactorBorders factorBorders;
            NFactorSlices::ParseSlicesVector(joinedBorders, factorBorders);
            result.Info["Slices"] = NFactorSlices::SerializeFactorBorders(factorBorders);
        }
    } else if (mn1.Info.contains("Slices") || mn2.Info.contains("Slices")) {
        ythrow yexception() << "Can not sum model with slices and model without slices";
    }
    TVector<TFeatureIndexWithDirection> mn1FeaturesRemap(mn1.Features.size(), {-1, NMatrixnetIdl::EFeatureDirection_None});
    TVector<TFeatureIndexWithDirection> mn2FeaturesRemap(mn2.Features.size(), {-1, NMatrixnetIdl::EFeatureDirection_None});
    if (mn1.Info.contains("Slices") && mn2.Info.contains("Slices")) {
        FillFeaturesRemap(mn1FeaturesRemap, mn1.Features, mn1.MissedValueDirections, borders1, joinedBorders);
        if (Find(mn1FeaturesRemap.begin(), mn1FeaturesRemap.end(), TFeatureIndexWithDirection{-1, NMatrixnetIdl::EFeatureDirection_None}) != mn1FeaturesRemap.end()) {
            ythrow yexception() << "There are features without slices in mn1";
        }
        FillFeaturesRemap(mn2FeaturesRemap, mn2.Features, mn2.MissedValueDirections, borders2, joinedBorders);
        if (Find(mn2FeaturesRemap.begin(), mn2FeaturesRemap.end(), TFeatureIndexWithDirection{-1, NMatrixnetIdl::EFeatureDirection_None}) != mn2FeaturesRemap.end()) {
            ythrow yexception() << "There are features without slices in mn2";
        }
    } else {
        for (size_t i = 0; i < mn1.Features.size(); ++i) {
            mn1FeaturesRemap[i] = { (int)mn1.Features[i].Index, !!mn1.MissedValueDirections ? mn1.MissedValueDirections[i] : (i8)NMatrixnetIdl::EFeatureDirection_None};
        }
        for (size_t i = 0; i < mn2.Features.size(); ++i) {
            mn2FeaturesRemap[i] = { (int)mn2.Features[i].Index, !!mn2.MissedValueDirections ? mn2.MissedValueDirections[i] : (i8)NMatrixnetIdl::EFeatureDirection_None};
        }
    }
    TValueRemapData mn1RemapData(mn1.Features, mn1.Values, mn1FeaturesRemap);
    TValueRemapData mn2RemapData(mn2.Features, mn2.Values, mn2FeaturesRemap);

    TMap<TFeatureIndexWithDirection, TMap<float, int>> resultFeatureToBorders;
    //TMap<float, int> used for backward compatibility
    mn1RemapData.FillNewFeatureBorders(resultFeatureToBorders);
    mn2RemapData.FillNewFeatureBorders(resultFeatureToBorders);

    THashMap<std::pair<TFeatureIndexWithDirection, float>, int> resultBinFeatureToIndex;
    {
        size_t featureIndex = 0;
        size_t valueIndex = 0;
        result.Features.resize(resultFeatureToBorders.size());
        if (hasMissedValueDirections) {
            result.MissedValueDirections.resize(resultFeatureToBorders.size());
        }
        result.Values.resize(mn1.Values.size() + mn2.Values.size());
        for (const auto& featureBorders : resultFeatureToBorders) {
            result.Features[featureIndex].Index = featureBorders.first.FeatureIndex;
            if (hasMissedValueDirections) {
                result.MissedValueDirections[featureIndex] = featureBorders.first.Direction;
            }
            result.Features[featureIndex].Length = featureBorders.second.size();
            ++featureIndex;
            for (const auto& borderIndex : featureBorders.second) {
                result.Values[valueIndex + borderIndex.second] = borderIndex.first;
                resultBinFeatureToIndex[std::make_pair(featureBorders.first, borderIndex.first)] = valueIndex + borderIndex.second;
            }
            valueIndex += featureBorders.second.size();
        }
        result.Values.resize(valueIndex);
    }

    TVector<int> mn1valuesRemap = mn1RemapData.CalcValuesRemap(resultBinFeatureToIndex);
    TVector<int> mn2valuesRemap = mn2RemapData.CalcValuesRemap(resultBinFeatureToIndex);

    {
        size_t sizeToCountLen = Max(mn1.SizeToCount.size(), mn2.SizeToCount.size());
        result.SizeToCount.resize(sizeToCountLen);
        result.DataIndices.resize(mn1.DataIndices.size() + mn2.DataIndices.size());
        result.OwnedMultiData().resize(1);
        result.OwnedMultiData()[0].resize(mn1.OwnedMultiData()[0].size() + mn2.OwnedMultiData()[0].size());
        int leafsNumber = 1;
        int mn1DataIt = 0;
        int mn1DataIndIt = 0;
        int mn2DataIt = 0;
        int mn2DataIndIt = 0;
        int rDataIt = 0;
        int rDataIndIt = 0;
        const double sfactor = (mn2.GetSingleMxData().Norm.DataScale / mn1.GetSingleMxData().Norm.DataScale) * beta;
        double mn1ScaleFactor = fabs(alpha);
        double mn2ScaleFactor = fabs(sfactor);
        { // calculating scale factors
            double mn1MaxData = 1;
            double mn2MaxData = 1;
            for (auto i : xrange(mn1.OwnedMultiData()[0].size())) {
                mn1MaxData = Max(mn1MaxData, fabs(double(mn1.OwnedMultiData()[0][i] ^ Min<int>())));
            }
            for (auto i : xrange(mn2.OwnedMultiData()[0].size())) {
                mn2MaxData = Max(mn2MaxData, fabs(double(mn2.OwnedMultiData()[0][i] ^ Min<int>())));
            }
            double mn1MaxScale = 2e9 / mn1MaxData;
            double mn2MaxScale = 2e9 / mn2MaxData;
            if (mn1ScaleFactor * mn2MaxScale > mn2ScaleFactor * mn1MaxScale) {
                mn2ScaleFactor *= mn1MaxScale / mn1ScaleFactor;
                mn1ScaleFactor = mn1MaxScale;
            } else {
                mn1ScaleFactor *= mn2MaxScale / mn2ScaleFactor;
                mn2ScaleFactor = mn2MaxScale;
            }
            if (mn1ScaleFactor > 1 || mn2ScaleFactor > 1) {
                //in this case multiplying by integer should be better
                if (mn1ScaleFactor > mn2ScaleFactor) {
                    mn2ScaleFactor *= int(mn1ScaleFactor) / mn1ScaleFactor;
                    mn1ScaleFactor = int(mn1ScaleFactor);
                } else {
                    mn1ScaleFactor *= int(mn2ScaleFactor) / mn2ScaleFactor;
                    mn2ScaleFactor = int(mn2ScaleFactor);
                }
            }
            if (alpha < 0) {
                mn1ScaleFactor = -mn1ScaleFactor;
            }
            if (sfactor < 0) {
                mn2ScaleFactor = -mn2ScaleFactor;
            }
        }
        for (size_t condNumber = 1; condNumber < sizeToCountLen; ++condNumber) {
            leafsNumber *= 2;
            result.SizeToCount[condNumber] = 0;
            if (condNumber < mn1.SizeToCount.size()) {
                const int numTrees = mn1.SizeToCount[condNumber];
                const int totalNumLeafs = leafsNumber * numTrees;
                result.SizeToCount[condNumber] += numTrees;
                const int numConditions = numTrees * condNumber;
                for (int i = 0; i < numConditions; ++i) {
                    const int oldIdx = mn1.DataIndices[mn1DataIndIt++] / 4;
                    result.DataIndices[rDataIndIt++] = mn1valuesRemap[oldIdx] * 4;
                }
                for (int i = 0; i < totalNumLeafs; ++i) {
                    double v = mn1.OwnedMultiData()[0][mn1DataIt++] ^ Min<int>();
                    v *= mn1ScaleFactor;
                    result.OwnedMultiData()[0][rDataIt++] = Float2Int(v) ^ Min<int>();
                }
            }
            if (condNumber < mn2.SizeToCount.size()) {
                const int numTrees = mn2.SizeToCount[condNumber];
                const int totalNumLeafs = leafsNumber * numTrees;
                result.SizeToCount[condNumber] += numTrees;
                const int numConditions = numTrees * condNumber;
                for (int i = 0; i < numConditions; ++i) {
                    const int oldIdx = mn2.DataIndices[mn2DataIndIt++] / 4;
                    result.DataIndices[rDataIndIt++] = mn2valuesRemap[oldIdx] * 4;
                }
                for (int i = 0; i < totalNumLeafs; ++i) {
                    double v = mn2.OwnedMultiData()[0][mn2DataIt++] ^ Min<int>();
                    v *= mn2ScaleFactor;
                    result.OwnedMultiData()[0][rDataIt++] = Float2Int(v) ^ Min<int>();
                }
            }
        }
        Y_VERIFY(mn1ScaleFactor != 0 || mn2ScaleFactor != 0);
        if (mn1ScaleFactor != 0) {
            result.GetSingleMxData().Norm.NativeDataScale
                    = result.GetSingleMxData().Norm.DataScale
                    = mn1.GetSingleMxData().Norm.DataScale * alpha / mn1ScaleFactor;
        } else {
            result.GetSingleMxData().Norm.NativeDataScale
                    = result.GetSingleMxData().Norm.DataScale
                    = mn1.GetSingleMxData().Norm.DataScale * sfactor / mn2ScaleFactor;
        }

        result.GetSingleMxData().Norm.NativeDataBias
                = result.GetSingleMxData().Norm.DataBias
                = alpha * mn1.GetSingleMxData().Norm.DataBias + beta * mn2.GetSingleMxData().Norm.DataBias;
    }

    for (TModelInfo::const_iterator it = mn1.Info.begin(); it != mn1.Info.end(); ++it) {
        result.Info["mn1." + it->first] = it->second;
    }
    for (TModelInfo::const_iterator it = mn2.Info.begin(); it != mn2.Info.end(); ++it) {
        result.Info["mn2." + it->first] = it->second;
    }

    result.Info["_combined"] = Sprintf("%f mn1 + %f mn2", alpha, beta);

    Swap(result);
    UpdateMatrixnet();
}

static TMnSseDynamic CreateEmptyFromSlices(const NMLPool::TFeatureSlices& slices) {
    TVector<float> values;
    TVector<TFeature> features;
    TVector<i8> missedValDirs;
    TVector<ui32> dInd;
    TVector<int> sizesToCount;
    TVector<int> data;

    ui32 featureInd = 0;
    for (const auto& slice : slices) {
        for (ui32 ind = slice.Begin; ind < slice.End; ++ind) {
            features.push_back({ featureInd, 0 });
            ++featureInd;
        }
    }

    TMnSseDynamic res(
        values.data(), values.size(),
        features.data(), features.size(),
        missedValDirs.data(), missedValDirs.size(),
        dInd.data(), dInd.size(),
        sizesToCount.data(), sizesToCount.size(),
        data.data(), data.size(),
        0, 1
    );

    NFactorSlices::TFactorBorders factorBorders;
    NFactorSlices::ParseSlicesVector(slices, factorBorders);
    res.SetInfo("Slices", NFactorSlices::SerializeFactorBorders(factorBorders));
    res.SetSlicesFromInfo();
    return res;
}

TMnSseDynamic TMnSseDynamic::CreateDynamicBundle(const TVector<std::pair<TMnSseDynamic, NFactorSlices::TFullFactorIndex>>& models) {
    Y_ENSURE(!models.empty(), "There must be at least one model");
    TMnSseDynamic res;
    {
        TMap<TString, NMLPool::TFeatureSlice> slices;
        for (const auto& [_, index] : models) {
            TString sliceString = ToString<NFactorSlices::EFactorSlice>(index.Slice);
            NMLPool::TFeatureSlice& slice = slices[sliceString];
            if (!slice.Name) {
                slice.Name = sliceString;
            }
            slice.End = Max<int>(slice.End, index.Index + 1);
        }
        NMLPool::TFeatureSlices borders;
        for (const auto& [name, slice] : slices) {
            borders.push_back(slice);
        }
        auto joinedBorders = JoinSlices({}, borders);
        res = CreateEmptyFromSlices(joinedBorders);
    }

    TVector<std::reference_wrapper<const std::pair<TMnSseDynamic, NFactorSlices::TFullFactorIndex>>> refs;
    TVector<std::pair<TMnSseDynamic, NFactorSlices::TFullFactorIndex>> partsOfMultyLayerModels;

    {
        size_t parts = 0;
        for (const auto& [model, _] : models) {
            size_t nonEmptyLayersCnt = CountIf(model.SizeToCount, [](int size) { return size > 0; });
            if (nonEmptyLayersCnt > 1) {
                parts += nonEmptyLayersCnt;
            }
        }
        partsOfMultyLayerModels.reserve(parts);

        for (const auto& p : models) {
            const auto& [model, featureIndex] = p;
            size_t nonEmptyLayersCnt = CountIf(model.SizeToCount, [](int size) { return size > 0; });
            if (nonEmptyLayersCnt > 1) {
                int treeInd = 0;
                for (int layerSize : model.SizeToCount) {
                    if (layerSize > 0) {
                        partsOfMultyLayerModels.push_back({ model.CopyTreeRange(treeInd, treeInd + layerSize), featureIndex });
                        refs.push_back(std::cref(partsOfMultyLayerModels.back()));
                    }
                    treeInd += layerSize;
                }
            } else {
                refs.push_back(std::cref(p));
            }
        }

        SortBy(
            refs,
            [](const auto& ref) {
                const TMnSseDynamic& model = ref.get().first;
                size_t nonEmptyLayer = std::distance(
                    model.SizeToCount.begin(),
                    FindIf(model.SizeToCount, [](const size_t size) { return size > 0; })
                );
                return nonEmptyLayer;
            }
        );
        for (const auto& ref : refs) {
            const auto& [model, _] = ref.get();
            res.AddWithCoefficients(model, 1.0, 1.0);
        }
        res.GetSingleMxData().Norm.NativeDataBias = res.GetSingleMxData().Norm.DataBias = 0;
    }

    {
        TDynamicBundle bundle;
        bundle.Components.reserve(models.size());
        size_t treeInd = 0;
        for (const auto& ref : refs) {
            const auto& [model, featureIndex] = ref.get();
            size_t nextTreeInd = treeInd + model.NumTrees();
            bundle.Components.push_back({
                treeInd,
                nextTreeInd,
                featureIndex,
                model.Bias(),
                1.0,
                model.GetInfo()->contains("formula-id") ? model.GetInfo()->at("formula-id") : "Unknown fml-id"
            });
            treeInd = nextTreeInd;
        }

        res.SetDynamicBundle(std::move(bundle));
    }
    return res;
}

void TMnSseDynamic::SetDynamicBundle(TDynamicBundle bundle) {
    DynamicBundle = std::move(bundle);
    Matrixnet.Meta.DynamicBundle = DynamicBundle->Components;
    TString dynamicBundleStr;
    {
        TStringOutput out(dynamicBundleStr);
        DynamicBundle->Save(&out);
    }
    Info[TDynamicBundle::InfoKeyValue] = dynamicBundleStr;
}

// add condition without breaking features order
static void addFeatureCondition(ui32 featureIdx, float border, const TVector<TFeature>& sourceFeatures, const TVector<float> sourceValues, TVector<TFeature>& targetFeatures, TVector<float>& targetValues, int& addedConditionIdx, TVector<int>& conditionsRemap) {
    //try to find feature in source features list
    int featureListIdx = -1;
    bool featureIdxFound = false;
    bool conditionFound = false;
    int valuesPos = 0;
    for (int i = 0; i < sourceFeatures.ysize(); ++i) {
        if (sourceFeatures[i].Index == featureIdx) {
            // try to find border in feature borders list
            // or to insert as the end of feature border values group
            addedConditionIdx = valuesPos + sourceFeatures[i].Length;
            for (ui32 j = 0; j < sourceFeatures[i].Length; ++j) {
                if (sourceValues[valuesPos + j] == border) {
                    conditionFound = true;
                    addedConditionIdx = valuesPos + j;
                    break;
                }
            }
            featureListIdx = i;
            featureIdxFound = true;
            break;
        }
        valuesPos += sourceFeatures[i].Length;
    }
    //set correct size for conditions remap
    conditionsRemap.resize(sourceValues.ysize());
    //now we have 2 cases
    if (featureIdxFound) {
        //1st case is when features amount is not changed
        targetFeatures = sourceFeatures;
        if (conditionFound) {
            //subcase: using existing condition addedConditionIdx and trivial remap
            targetValues = sourceValues;
            for (int i = 0; i < sourceValues.ysize(); ++i) {
                conditionsRemap[i] = i;
            }
            return;
        } else {
            //subcase: inserting new condition border
            //update feature with index featureListIdx (inserted 1 border value)
            targetFeatures[featureListIdx] = TFeature(sourceFeatures[featureListIdx].Index, sourceFeatures[featureListIdx].Length + 1);
            //fill new border values and conditionsRemap
            targetValues.resize(sourceValues.size() + 1);
            for (int i = 0; i < addedConditionIdx; ++ i) {
                targetValues[i] = sourceValues[i];
                conditionsRemap[i] = i;
            }
            targetValues[addedConditionIdx] = border;
            for (int i = addedConditionIdx; i < sourceValues.ysize(); ++i) {
                targetValues[i + 1] = sourceValues[i];
                conditionsRemap[i] = i + 1;
            }
            return;
        }
    } else {
        //2nd case is when feature amount is increased
        targetFeatures.resize(sourceFeatures.size() + 1);
        int valuePos = 0;
        for (int i = 0; i < sourceFeatures.ysize(); ++i) {
            // filling target features and finding position
            // for new feature and new border value
            if (featureListIdx == -1 && sourceFeatures[i].Index > featureIdx) {
                // inserting new feature at position featureListIdx = i
                featureListIdx = i;
                targetFeatures[featureListIdx] = TFeature(featureIdx, 1);
            }
            if (featureListIdx == -1) {
                // new feature not inserted before this feature
                targetFeatures[i] = sourceFeatures[i];
                valuePos += sourceFeatures[i].Length;
            } else {
                // new feature inserted before this feature
                targetFeatures[i + 1] = sourceFeatures[i];
            }
        }
        if (featureListIdx == -1) {
            // in this case feature should be inserted to the end of the list
            featureListIdx = sourceFeatures.size();
            targetFeatures[featureListIdx] = TFeature(featureIdx, 1);
        }
        //fill addedConditionIdx, border values and conditionsRemap
        addedConditionIdx = valuePos;
        targetValues.resize(sourceValues.size() + 1);
        for (int i = 0; i < addedConditionIdx; ++ i) {
            targetValues[i] = sourceValues[i];
            conditionsRemap[i] = i;
        }
        targetValues[addedConditionIdx] = border;
        for (int i = addedConditionIdx; i < sourceValues.ysize(); ++i) {
            targetValues[i + 1] = sourceValues[i];
            conditionsRemap[i] = i + 1;
        }
        return;
    }
}

void TMnSseDynamic::AddFeature(int featureIdx, float border, double leftScale, double rightScale) {
    Y_ENSURE(OwnedMultiData().size() == 1, "AddFeature unsupported for multidata models");
    const TMnSseDynamic& source = *this;
    TMnSseDynamic result;
    int addedConditionIndex = 0;
    TVector<int> conditionsRemap;
    addFeatureCondition(featureIdx, border, source.Features, source.Values, result.Features, result.Values, addedConditionIndex, conditionsRemap);
    int treeCount = 0;
    {
        // fill SizeToCount and calculate old tree count
        result.SizeToCount.resize(source.SizeToCount.ysize() + 1);
        result.SizeToCount[0] = 0;
        for (int i = 0; i < source.SizeToCount.ysize(); ++i) {
            treeCount += source.SizeToCount[i];
            if (i == 0) {
                result.SizeToCount[i + 1] = source.SizeToCount[i] + 1;
            } else {
                result.SizeToCount[i + 1] = source.SizeToCount[i];
            }
        }
        if (result.SizeToCount[source.SizeToCount.ysize()] == 0) {
            result.SizeToCount.resize(source.SizeToCount.ysize());
        }
    }
    result.DataIndices.resize(1 + source.DataIndices.ysize() + treeCount);
    TVector<double> dataResult(2 + 2 * source.OwnedMultiData()[0].ysize());
    int resultIndicesPosition = 0;
    int resultDataPosition = 0;
    result.GetSingleMxData().Norm.DataBias = leftScale * source.GetSingleMxData().Norm.DataBias;
    result.GetSingleMxData().Norm.DataScale = 1.;
    {
        // add bias fix tree
        result.DataIndices[resultIndicesPosition] = 4 * addedConditionIndex;
        ++resultIndicesPosition;
        dataResult[resultDataPosition] = 0.;
        ++resultDataPosition;
        dataResult[resultDataPosition] = (rightScale - leftScale) * source.GetSingleMxData().Norm.DataBias;
        ++resultDataPosition;
    }
    // fill DataIndices and dataResult
    int treeDataSize = 1;
    int sourceIndicesPosition = 0;
    int sourceDataPosition = 0;
    for (int treeSize = 0; treeSize < source.SizeToCount.ysize(); ++treeSize) {
        for (int i = 0; i < source.SizeToCount[treeSize]; ++i) {
            // fill new condition
            result.DataIndices[resultIndicesPosition] = 4 * addedConditionIndex;
            ++resultIndicesPosition;
            // fill old conditions
            for (int j = 0; j < treeSize; ++j) {
                int sourceConditionIndex = source.DataIndices[sourceIndicesPosition + j] / 4;
                result.DataIndices[resultIndicesPosition + j] = 4 * conditionsRemap[sourceConditionIndex];
            }
            resultIndicesPosition += treeSize;
            sourceIndicesPosition += treeSize;
            // fill tree leaf values
            for (int j = 0; j < treeDataSize; ++j) {
                double value = source.OwnedMultiData()[0][sourceDataPosition + j] ^ Min<int>();
                dataResult[resultDataPosition + 2 * j] = value * leftScale * source.GetSingleMxData().Norm.DataScale;
                dataResult[resultDataPosition + 2 * j + 1] = value * rightScale * source.GetSingleMxData().Norm.DataScale;
            }
            sourceDataPosition += treeDataSize;
            resultDataPosition += 2 * treeDataSize;
        }
        treeDataSize *= 2;
    }
    // determine optimal scaling (with 1e9 = maximum leaf value)
    double maxElement = 0.;
    for (int i = 0; i < dataResult.ysize(); ++i) {
        maxElement = std::max(maxElement, dataResult[i]);
        maxElement = std::max(maxElement, -dataResult[i]);
    }
    if (maxElement == 0.) {
        maxElement = 1e9;
    }
    // set scale
    result.GetSingleMxData().Norm.DataScale = maxElement / 1e9;
    // fill Data
    result.OwnedMultiData().resize(1);
    result.OwnedMultiData()[0].resize(dataResult.ysize());
    for (int i = 0; i < dataResult.ysize(); ++i) {
        result.OwnedMultiData()[0][i] = Float2Int(dataResult[i] / result.GetSingleMxData().Norm.DataScale) ^ Min<int>();
    }
    result.GetSingleMxData().Norm.NativeDataScale = result.GetSingleMxData().Norm.DataScale;
    result.GetSingleMxData().Norm.NativeDataBias = result.GetSingleMxData().Norm.DataBias;
    for (TModelInfo::const_iterator it = source.Info.begin(); it != source.Info.end(); ++it) {
        result.Info["source." + it->first] = it->second;
    }
    result.Info["_add_feature"] = "-f " + ToString(featureIdx) + " -b " + ToString(border) + " -l " + ToString(leftScale) + " -r " + ToString(rightScale);

    Swap(result);
    UpdateMatrixnet();
}

}
