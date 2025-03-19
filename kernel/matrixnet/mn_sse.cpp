#include "mn_sse.h"

#if defined(MATRIXNET_WITHOUT_ARCADIA)
#include "without_arcadia/util/utility.h"
#include "without_arcadia/util/yexception.h"
#include "without_arcadia/util/string_cast.h"
#include "without_arcadia/util/compiler.h"
#include <xmmintrin.h>
#include <emmintrin.h>
#else // !defined(MATRIXNET_WITHOUT_ARCADIA)
#include "sliced_util.h"
#include <util/generic/algorithm.h>
#include <util/generic/buffer.h>
#include <util/generic/ptr.h>
#include <util/generic/array_ref.h>
#include <util/ysaveload.h>
#include <util/stream/mem.h>
#include <util/stream/format.h>
#include <util/string/cast.h>
#include <util/string/type.h>
#include <util/string/split.h>
#include <util/string/strip.h>
#include <util/system/unaligned_mem.h>
#include <library/cpp/digest/md5/md5.h>
#include <kernel/factor_slices/factor_borders.h>
#include <kernel/factor_storage/factor_view.h>
#include <kernel/factor_storage/factor_storage.h>
#include <library/cpp/sse/sse.h>
#endif // !defined(MATRIXNET_WITHOUT_ARCADIA)

#include <numeric> // for accumulate
#include <utility>

namespace NMatrixnet {

void TFeature::Swap(TFeature& obj) {
    DoSwap(Index, obj.Index);
    DoSwap(Length, obj.Length);
}

#if !defined(MATRIXNET_WITHOUT_ARCADIA)
void TFeature::Save(IOutputStream *out) const {
    ::Save(out, Index);
    ::Save(out, Length);
}

void TFeature::Load(IInputStream *in) {
    ::Load(in, Index);
    ::Load(in, Length);
}
#endif // !defined(MATRIXNET_WITHOUT_ARCADIA)

void TFeatureSlice::Swap(TFeatureSlice& obj) {
    DoSwap(StartFeatureIndexOffset, obj.StartFeatureIndexOffset);
    DoSwap(StartFeatureOffset, obj.StartFeatureOffset);
    DoSwap(StartValueOffset, obj.StartValueOffset);
    DoSwap(EndFeatureIndexOffset, obj.EndFeatureIndexOffset);
    DoSwap(EndFeatureOffset, obj.EndFeatureOffset);
    DoSwap(EndValueOffset, obj.EndValueOffset);
}

#if !defined(MATRIXNET_WITHOUT_ARCADIA)
void TFeatureSlice::Save(IOutputStream *out) const {
    ::Save(out, StartFeatureIndexOffset);
    ::Save(out, StartFeatureOffset);
    ::Save(out, StartValueOffset);
    ::Save(out, EndFeatureIndexOffset);
    ::Save(out, EndFeatureOffset);
    ::Save(out, EndValueOffset);
}

void TFeatureSlice::Load(IInputStream *in) {
    ::Load(in, StartFeatureIndexOffset);
    ::Load(in, StartFeatureOffset);
    ::Load(in, StartValueOffset);
    ::Load(in, EndFeatureIndexOffset);
    ::Load(in, EndFeatureOffset);
    ::Load(in, EndValueOffset);
}
#endif // !defined(MATRIXNET_WITHOUT_ARCADIA)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TMnSseStaticMeta::TMnSseStaticMeta(
        const float *values,      const size_t values_size,
        const TFeature* features, const size_t features_size,
        const TFeatureSlice* slices, const size_t numSlices,
        const void* dInd,         const size_t dInd_size, bool has16Bits,
        const int* sizeToCount,   const size_t sizeToCount_size)
        : Values(values)
        , ValuesSize(values_size)
        , Features(features)
        , FeaturesSize(features_size)
        , FeatureSlices(slices)
        , NumSlices(numSlices)
        , DataIndicesPtr(dInd)
        , DataIndicesSize(dInd_size)
        , Has16Bits(has16Bits)
        , SizeToCount(sizeToCount)
        , SizeToCountSize(sizeToCount_size)
{
}

TMnSseStaticMeta::TMnSseStaticMeta(
        const float *values,      const size_t values_size,
        const TFeature* features, const size_t features_size,
        const i8* missedValueDirections, const size_t missedValueDirectionsSize,
        const ui32* dInd,         const size_t dInd_size,
        const int* sizeToCount,   const size_t sizeToCount_size)
        : Values(values)
        , ValuesSize(values_size)
        , Features(features)
        , FeaturesSize(features_size)
        , DataIndicesPtr(dInd)
        , DataIndicesSize(dInd_size)
        , Has16Bits(false)
        , MissedValueDirections(missedValueDirections)
        , MissedValueDirectionsSize(missedValueDirectionsSize)
        , SizeToCount(sizeToCount)
        , SizeToCountSize(sizeToCount_size)
{
}

void TMnSseStaticMeta::Swap(TMnSseStaticMeta& obj) {
    DoSwap(Values, obj.Values);
    DoSwap(ValuesSize, obj.ValuesSize);
    DoSwap(Features, obj.Features);
    DoSwap(FeaturesSize, obj.FeaturesSize);
    DoSwap(FeatureSlices, obj.FeatureSlices);
    DoSwap(MissedValueDirections, obj.MissedValueDirections);
    DoSwap(MissedValueDirectionsSize, obj.MissedValueDirectionsSize);
    DoSwap(NumSlices, obj.NumSlices);
    DoSwap(DataIndicesPtr, obj.DataIndicesPtr);
    DoSwap(DataIndicesSize, obj.DataIndicesSize);
    DoSwap(Has16Bits, obj.Has16Bits);
    DoSwap(SizeToCount, obj.SizeToCount);
    DoSwap(SizeToCountSize, obj.SizeToCountSize);
    DoSwap(DynamicBundle, obj.DynamicBundle);
}

bool TMnSseStaticMeta::Empty() const {
    return !(ValuesSize || FeaturesSize || DataIndicesSize || SizeToCountSize);
}

void TMnSseStaticMeta::Clear() {
    ValuesSize = 0;
    FeaturesSize = 0;
    MissedValueDirectionsSize = 0;
    DataIndicesSize = 0;
    SizeToCountSize = 0;
    DynamicBundle = {};
}

bool TMnSseStaticMeta::CompareValues(const TMnSseStaticMeta& obj) const {
#define COMPARE_ARRAY_VALS(size, ptr) if (size != obj.size || !std::equal(ptr, ptr + size, obj.ptr) ) return false;
    COMPARE_ARRAY_VALS(ValuesSize, Values);
    COMPARE_ARRAY_VALS(FeaturesSize, Features);
    COMPARE_ARRAY_VALS(NumSlices, FeatureSlices);
    COMPARE_ARRAY_VALS(MissedValueDirectionsSize, MissedValueDirections);
    if (Has16Bits != obj.Has16Bits) return false;
    if (DataIndicesSize != obj.DataIndicesSize) return false;
    if (Has16Bits) {
        if (!std::equal((const ui16*)DataIndicesPtr, (const ui16*)DataIndicesPtr + DataIndicesSize, (const ui16*)obj.DataIndicesPtr)) {
            return false;
        }
    } else {
        if (!std::equal((const ui32*)DataIndicesPtr, (const ui32*)DataIndicesPtr + DataIndicesSize, (const ui32*)obj.DataIndicesPtr)) {
            return false;
        }
    }
    COMPARE_ARRAY_VALS(SizeToCountSize, SizeToCount);
    if (!(DynamicBundle == obj.DynamicBundle)) {
        return false;
    }
    return true;
#undef COMPARE_ARRAY_VALS
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void TMnSseStaticLeaves::Swap(TMnSseStaticLeaves& obj) {
    DoSwap(Data, obj.Data);
}

bool TMnSseStaticLeaves::Empty() const {
    return
            std::get_if<TMultiData>(&Data) && std::get<TMultiData>(Data).DataSize == 0 ||
            std::get_if<TMultiDataCompact>(&Data) && std::get<TMultiDataCompact>(Data).DataSize == 0;
}

void TMnSseStaticLeaves::Clear() {
    if (auto* classic = std::get_if<TMultiData>(&Data)) {
        classic->Clear();
    } else if (auto* compact = std::get_if<TMultiDataCompact>(&Data)) {
        compact->Clear();
    }
}


bool TMnSseStaticLeaves::CompareValues(const TMnSseStaticLeaves& obj) const {
    Y_ENSURE(Data.index() == obj.Data.index());
    if (auto* l1 = std::get_if<TMultiData>(&Data)) {
        return l1->Compare(std::get<TMultiData>(obj.Data));
    } else if (auto* l2 = std::get_if<TMultiDataCompact>(&Data)) {
        return l2->Compare(std::get<TMultiDataCompact>(obj.Data));
    } else {
        Y_FAIL();
    }
    return true;
}

TNormAttributes& TMnSseStaticLeaves::CommonNorm(const TString& errorMessage) {
    if (auto* old = std::get_if<TMultiData>(&Data)) {
        Y_ENSURE(old->MultiData.size() == 1, errorMessage);
        return old->MultiData[0].Norm;
    } else if (auto* compact = std::get_if<TMultiDataCompact>(&Data)) {
        return compact->Norm;
    } else {
        Y_FAIL();
    }
}

const TNormAttributes& TMnSseStaticLeaves::CommonNorm(const TString& errorMessage) const {
    if (auto* old = std::get_if<TMultiData>(&Data)) {
        Y_ENSURE(old->MultiData.size() == 1, errorMessage);
        return old->MultiData[0].Norm;
    } else if (auto* compact = std::get_if<TMultiDataCompact>(&Data)) {
        return compact->Norm;
    } else {
        Y_FAIL();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TMnSseStatic::TMnSseStatic(
        const TMnSseStaticMeta& features,
        const int* data,          const size_t data_size,
        const double bias,        const double scale)
    : Meta(features)
    , Leaves(TMultiData({TMultiData::TLeafData(data, bias, scale, bias, scale)}, data_size))
{
}

TMnSseStatic::TMnSseStatic(const TMnSseStaticMeta& features, const TMnSseStaticLeaves& leaves)
    : Meta(features)
    , Leaves(leaves)
{
}

void TMnSseStatic::Swap(TMnSseStatic& obj) {
    DoSwap(Meta, obj.Meta);
    DoSwap(Leaves, obj.Leaves);
}

bool TMnSseStatic::Empty() const {
    return Meta.Empty() && Leaves.Empty();
}

void TMnSseStatic::Clear() {
    Meta.Clear();
    Leaves.Clear();
}


bool TMnSseStatic::CompareValues(const TMnSseStatic& obj) const {
    return Meta.CompareValues(obj.Meta) && Leaves.CompareValues(obj.Leaves);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template<typename T>
static inline T ReadFromBuf(const void *&buf, const void *const end);

namespace {
template<typename T>
struct TBufReader {
    static T Read(const void*& buf, const void* const end) {
        const char* cur = static_cast<const char*>(buf);
        if (cur + sizeof(T) > end)
            ythrow TLoadEOF();
#ifndef MATRIXNET_WITHOUT_ARCADIA
        T result = ReadUnaligned<T>(cur);
#else
        memcpy(&result, cur, sizeof(T));
#endif
        buf = cur + sizeof(T);
        return result;
    }
};

template<>
struct TBufReader<TString> {
    static TString Read(const void *&buf, const void *const end) {
        size_t size = ReadFromBuf<ui32>(buf, end);
        const char* cur = static_cast<const char*>(buf);
        if (cur + size > end)
            ythrow TLoadEOF();

        TString result(cur, size);
        buf = cur + size;
        return result;
    }
};

template<typename T1, typename T2>
struct TBufReader<std::pair<T1, T2>> {
    static std::pair<T1, T2> Read(const void *&buf, const void *const end) {
        std::pair<T1, T2> result;
        result.first = ReadFromBuf<T1>(buf, end);
        result.second = ReadFromBuf<T2>(buf, end);
        return result;
    }
};

template<typename K, typename V>
struct TBufReader<TMap<K, V>> {
    static TMap<K, V> Read(const void *&buf, const void *const end) {
        size_t size = ReadFromBuf<ui32>(buf, end);
        TMap<K, V> result;
        while (size--) {
            std::pair<K, V> keyValuePair = ReadFromBuf<std::pair<K, V>>(buf, end);
            result.insert(keyValuePair);
        }
        return result;
    }
};
}

template<typename T>
static inline T ReadFromBuf(const void *&buf, const void *const end) {
    return TBufReader<T>::Read(buf, end);
}

template<typename T>
static inline const T* ReadFromBufArray(const void *&buf, const void *const end, const size_t size) {
    const T* result = static_cast<const T*>(buf);
    buf = (char*)buf + sizeof(T) * size;

    if (buf > end)
        ythrow TLoadEOF();

    return result;
}

#if !defined(MATRIXNET_WITHOUT_ARCADIA)
TString ModelNameFromFilePath(const TStringBuf fname) {
    const size_t dot = fname.rfind('.');
    const size_t slash = fname.rfind('/');
    const size_t begin = (slash == TStringBuf::npos) ? 0 : (slash + 1);
    const size_t end = (dot > begin) ? dot : TStringBuf::npos;
    if (end == TStringBuf::npos)
        return TString{fname.substr(begin)};
    else
        return TString{fname.substr(begin,end-begin)};
}
#endif

TMnSseInfo::TMnSseInfo() {
}

TMnSseInfo::TMnSseInfo(const void* const buf, const size_t size, const char* const name) {
    InitStatic(buf, size, name);
}

void TMnSseInfo::InitStatic(const void* const buf, const size_t size, const char* const name) {
    Clear();

    SourceData = buf;
    SourceSize = size;

    const void* p = buf;
    const void* end = (char*)buf + size;

    Matrixnet.Meta.ValuesSize = ReadFromBuf<ui32>(p, end);
    if (Matrixnet.Meta.ValuesSize != FLATBUFFERS_MN_MODEL_MARKER) {
        Matrixnet.Meta.Values = ReadFromBufArray<float>(p, end, Matrixnet.Meta.ValuesSize);

        Matrixnet.Meta.FeatureSlices = nullptr;
        Matrixnet.Meta.NumSlices = 0;
        Matrixnet.Meta.FeaturesSize = ReadFromBuf<ui32>(p, end);
        if (Matrixnet.Meta.FeaturesSize == 0) {
            Matrixnet.Meta.FeaturesSize = ReadFromBuf<ui32>(p, end);
            Matrixnet.Meta.Features = ReadFromBufArray<TFeature>(p, end, Matrixnet.Meta.FeaturesSize);
            // slices info is saved as Info["Slices"] so we just skip it here
            size_t numSlices = ReadFromBuf<ui32>(p, end);
            ReadFromBufArray<TFeatureSlice>(p, end, numSlices);
        } else {
            Matrixnet.Meta.Features = ReadFromBufArray<TFeature>(p, end, Matrixnet.Meta.FeaturesSize);
        }
        Matrixnet.Meta.DataIndicesSize = ReadFromBuf<ui32>(p, end);
        if (Matrixnet.Meta.DataIndicesSize == 0) {
            Matrixnet.Meta.Has16Bits = false;
            Matrixnet.Meta.DataIndicesSize = ReadFromBuf<ui32>(p, end);
            Matrixnet.Meta.DataIndicesPtr = ReadFromBufArray<ui32>(p, end, Matrixnet.Meta.DataIndicesSize);
        } else {
            Matrixnet.Meta.Has16Bits = true;
            Matrixnet.Meta.DataIndicesPtr = ReadFromBufArray<ui16>(p, end, Matrixnet.Meta.DataIndicesSize);
        }

        Matrixnet.Meta.SizeToCountSize = ReadFromBuf<ui32>(p, end);
        Matrixnet.Meta.SizeToCount = ReadFromBufArray<int>(p, end, Matrixnet.Meta.SizeToCountSize);

        size_t dataSize = ReadFromBuf<ui32>(p, end);

        TMultiData::TLeafData dataStruct;
        dataStruct.Data = ReadFromBufArray<int>(p, end, dataSize);
        dataStruct.Norm.DataBias = ReadFromBuf<double>(p, end);
        dataStruct.Norm.DataScale = ReadFromBuf<double>(p, end);
        Matrixnet.Leaves.Data = TMultiData({dataStruct}, dataSize);
        if (p < end) {
            Info = ReadFromBuf<TModelInfo>(p, end);
        }
    } else {
        ui32 expectedModelSize = ReadFromBuf<ui32>(p, end);
        // TODO(kirillovs): update check to strict size check after global static initialization size problem fix
        // Y_ENSURE(expectedModelSize + FLATBUFF_MODEL_MODEL_HEADER_SIZE == size, "Invalid model size: " << size << " expected: " << expectedModelSize + FLATBUFF_MODEL_MODEL_HEADER_SIZE);
        Y_ENSURE(expectedModelSize + FLATBUFF_MODEL_MODEL_HEADER_SIZE <= size, "Invalid model size: " << size << " expected: " << expectedModelSize + FLATBUFF_MODEL_MODEL_HEADER_SIZE);
        {
            flatbuffers::Verifier verifier((const ui8*)p, size - FLATBUFF_MODEL_MODEL_HEADER_SIZE);
            Y_ENSURE(NMatrixnetIdl::VerifyTMNSSEModelBuffer(verifier), "flatbuffers model verification failed");
        }
        auto fbModel = NMatrixnetIdl::GetTMNSSEModel(p);
        Matrixnet.Meta.Values = fbModel->Values()->data();
        Matrixnet.Meta.ValuesSize = fbModel->Values()->size();

        Matrixnet.Meta.Features = (TFeature*)fbModel->Features()->data();
        Matrixnet.Meta.FeaturesSize = fbModel->Features()->size();

        Matrixnet.Meta.Has16Bits = false;
        Matrixnet.Meta.DataIndicesPtr = fbModel->DataIndices()->data();
        Matrixnet.Meta.DataIndicesSize = fbModel->DataIndices()->size();

        Matrixnet.Meta.SizeToCount = fbModel->SizeToCount()->data();
        Matrixnet.Meta.SizeToCountSize = fbModel->SizeToCount()->size();

        if (fbModel->Data()) {
            TMultiData::TLeafData data;
            data.Data = fbModel->Data()->data();
            data.Norm.DataBias = fbModel->DataBias();
            data.Norm.DataScale = fbModel->DataScale();
            Matrixnet.Leaves.Data = TMultiData({data}, fbModel->Data()->size());
        } else if (fbModel->MultiData()) {
            Matrixnet.Leaves.Data = TMultiData({}, 0);
            TMultiData& multidataRef = std::get<TMultiData>(Matrixnet.Leaves.Data);
            for (const auto& md : *fbModel->MultiData()) {
                if (multidataRef.DataSize == 0) {
                    multidataRef.DataSize = md->Data()->size();
                } else {
                    Y_VERIFY(multidataRef.DataSize == md->Data()->size(), "different data sizes in multidata model");
                }
                TMultiData::TLeafData data;
                data.Data = md->Data()->data();
                data.Norm.DataBias = md->DataBias();
                data.Norm.DataScale = md->DataScale();
                multidataRef.MultiData.push_back(data);
            }
        } else if (fbModel->MultiDataCompact()) {
            static_assert(sizeof(TValueClassLeaf) == sizeof(NMatrixnetIdl::TValueClassLeaf), "NMatrixnetIdl::TValueClassLeaf and NMatrixnet::TValueClassLeaf structures sizes differs");
            const auto* dataCompact = fbModel->MultiDataCompact();
            Matrixnet.Leaves.Data = TMultiDataCompact(
                reinterpret_cast<const TValueClassLeaf*>(dataCompact->Data()->data()),
                dataCompact->Data()->size(),
                TNormAttributes(dataCompact->DataBias(), dataCompact->DataScale()),
                dataCompact->NumClasses());
        } else {
            ythrow yexception() << "Missing one of: Data, MultiData or MultiDataCompact";
        }

        if (fbModel->MissedValueDirections()) {
            Matrixnet.Meta.MissedValueDirections = fbModel->MissedValueDirections()->data();
            Matrixnet.Meta.MissedValueDirectionsSize = fbModel->MissedValueDirections()->size();
        } else {
            Matrixnet.Meta.MissedValueDirections = nullptr;
            Matrixnet.Meta.MissedValueDirectionsSize = 0;
        }
        if (fbModel->InfoMap()) {
            for (const auto keyValue : *fbModel->InfoMap()) {
                Info[TString(keyValue->Key()->c_str(), keyValue->Key()->size())] = TString(keyValue->Value()->c_str(), keyValue->Value()->size());
            }
        }
    }

    InitMaxFactorIndex();


#if !defined(MATRIXNET_WITHOUT_ARCADIA)
    if (name) {
        Info["fname"] = ModelNameFromFilePath(name);
        if (Info["formula-id"].empty())
            Info["formula-id"] = "md5-" + MD5();
    }
#endif // !defined(MATRIXNET_WITHOUT_ARCADIA)
    if (auto* multiData = std::get_if<TMultiData>(&Matrixnet.Leaves.Data)) {
        if (Y_UNLIKELY(multiData->MultiData.size() == 1)) {
            TNormAttributes& norm = multiData->MultiData[0].Norm;
            TString* x = Info.FindPtr("NativeDataBias");
            norm.NativeDataBias = (x != nullptr) ? FromString<double>(*x) : norm.DataBias;
            norm.NativeDataScale = ((x = Info.FindPtr("NativeDataScale")) != nullptr)
                                   ? FromString<double>(*x) : norm.DataScale;
        }
    }
    SetSlicesFromInfo();
    SetDynamicBundleFromInfo();
    if (DynamicBundle) {
        Matrixnet.Meta.DynamicBundle = DynamicBundle->Components;
    }
}

void TMnSseInfo::SetSlicesFromInfo() {
    Matrixnet.Meta.FeatureSlices = nullptr;
    Matrixnet.Meta.NumSlices = 0;

#if !defined(MATRIXNET_WITHOUT_ARCADIA)
    Slices.clear();
    SliceNames.clear();
    TString* x = Info.FindPtr("Slices");
    if (x != nullptr) {
        InitSlices(*x, HasCustomSlices());
    }
#endif // !defined(MATRIXNET_WITHOUT_ARCADIA)
}

void TMnSseInfo::SetDynamicBundleFromInfo() {
    if (TDynamicBundle::IsDynamicBundle(*this)) {
        const TString& dynamicBundleStr = Info.at(TDynamicBundle::InfoKeyValue);
        TStringInput in(dynamicBundleStr);
        DynamicBundle.ConstructInPlace().Load(&in);
    }
}

#if !defined(MATRIXNET_WITHOUT_ARCADIA)
bool TMnSseInfo::HasCustomSlices() const {
    if (const TString* allowCustomNamesStr = Info.FindPtr("CustomSlices"))
        return IsTrue(*allowCustomNamesStr);
    return false;
}

void TMnSseInfo::InitSlices(const TString& slicesStr, bool allowCustomNames) {
    NMLPool::TFeatureSlices namedSlices;
    if (!NFactorSlices::TryToDeserializeFeatureSlices(slicesStr, allowCustomNames, namedSlices))
        return;
    auto slicesCmp = [](const NMLPool::TFeatureSlice& a, const NMLPool::TFeatureSlice& b) {
        TSliceOffsets offsetA(a.Begin, a.End);
        TSliceOffsets offsetB(b.Begin, b.End);
        return NFactorSlices::TCompareSliceOffsets()(offsetA, offsetB);
    };
    Sort(namedSlices, slicesCmp);

    NFactorSlices::TSliceOffsets prevOffsets;
    // Update slices to store incremental offsets as apparently required by Peter's calcer
    SliceNames.clear();
    Slices.clear();
    for (const auto& slice : namedSlices) {
        const TSliceOffsets offsets(slice.Begin, slice.End);

        if (prevOffsets.Overlaps(offsets)) {
            // We do not support intersecting slices
            SliceNames.clear();
            Slices.clear();
            return;
        }
        if (!offsets.Empty()) {
            prevOffsets = offsets;
            SliceNames.push_back(slice.Name);
            Slices.push_back(TFeatureSlice(
                        prevOffsets.Begin,
                        ComputeFeatureOffset(prevOffsets.Begin),
                        ComputeValueOffset(prevOffsets.Begin),
                        prevOffsets.End,
                        ComputeFeatureOffset(prevOffsets.End),
                        ComputeValueOffset(prevOffsets.End)));
        }
    }

    if (!Slices.empty()) {
        Matrixnet.Meta.FeatureSlices = &Slices[0];
        Matrixnet.Meta.NumSlices = Slices.size();
    }
}

ui32 TMnSseInfo::ComputeFeatureOffset(const ui32 featureIndex) const {
    if (Matrixnet.Meta.FeaturesSize > 0 && Matrixnet.Meta.Features[0].Index > featureIndex) {
        return 0;
    }

    for (size_t i = 0; i < Matrixnet.Meta.FeaturesSize; ++i) {
        if (featureIndex <= Matrixnet.Meta.Features[i].Index) {
            return i;
        }
    }

    return Matrixnet.Meta.FeaturesSize;
}

ui32 TMnSseInfo::ComputeValueOffset(const ui32 featureIndex) const {
    ui32 result = 0;

    for (size_t i = 0; i < Matrixnet.Meta.FeaturesSize; ++i) {
        if (featureIndex <= Matrixnet.Meta.Features[i].Index) {
            return result;
        }

        result += Matrixnet.Meta.Features[i].Length;
    }

    return Matrixnet.Meta.ValuesSize;
}
#endif // !defined(MATRIXNET_WITHOUT_ARCADIA)

TMnSseInfo::TMnSseInfo(
        const TMnSseStaticMeta& features,
        const int* data,   const size_t dataSize,
        const double bias, const double scale)
    : Matrixnet(features, data, dataSize, bias, scale)
{
    InitMaxFactorIndex();
}

void TMnSseInfo::InitMaxFactorIndex() {
    // precalculate widely-used values
    size_t maxIndex = 0;
    for (size_t i = 0; i < Matrixnet.Meta.FeaturesSize; ++i) {
        if (Matrixnet.Meta.Features[i].Index > maxIndex) {
            maxIndex = Matrixnet.Meta.Features[i].Index;
        }
    }
    MaxFactorIdx = maxIndex;
}

void TMnSseInfo::Swap(TMnSseInfo& obj) {
    DoSwap(Matrixnet, obj.Matrixnet);
    DoSwap(Info, obj.Info);
    DoSwap(SourceData, obj.SourceData);
    DoSwap(SourceSize, obj.SourceSize);
    DoSwap(MaxFactorIdx, obj.MaxFactorIdx);
    DoSwap(DynamicBundle, obj.DynamicBundle);
#if !defined(MATRIXNET_WITHOUT_ARCADIA)
    DoSwap(MD5Sum, obj.MD5Sum);
    DoSwap(Slices, obj.Slices);
    DoSwap(SliceNames, obj.SliceNames);
#endif // !defined(MATRIXNET_WITHOUT_ARCADIA)
}

bool TMnSseInfo::Empty() const {
    return Matrixnet.Empty();
}

void TMnSseInfo::Clear() {
    Matrixnet.Clear();
    Info.clear();
    SourceData = nullptr;
    SourceSize = 0;
    MaxFactorIdx = size_t(-1);
    DynamicBundle.Clear();
#if !defined(MATRIXNET_WITHOUT_ARCADIA)
    MD5Sum.clear();
#endif // !defined(MATRIXNET_WITHOUT_ARCADIA)
}

int TMnSseInfo::NumFeatures() const {
    return Matrixnet.Meta.FeaturesSize;
}

ui64 TMnSseInfo::NumBinFeatures() const {
    ui64 numBinFeatures = 0;
    for (size_t i = 0; i < Matrixnet.Meta.FeaturesSize; ++i) {
        numBinFeatures += Matrixnet.Meta.Features[i].Length;
    }
    return numBinFeatures;
}

int TMnSseInfo::NumTrees() const {
    return std::accumulate(&Matrixnet.Meta.SizeToCount[0], &Matrixnet.Meta.SizeToCount[Matrixnet.Meta.SizeToCountSize], 0);
}

int TMnSseInfo::NumTrees(const int treeSize) const {
    if (treeSize < 0)
        return 0;
    if ((size_t)treeSize >= Matrixnet.Meta.SizeToCountSize)
        return 0;
    return Matrixnet.Meta.SizeToCount[treeSize];
}

int TMnSseInfo::MaxTreeSize() const {
    return Matrixnet.Meta.SizeToCountSize - 1;
}

double TMnSseInfo::Bias() const {
    return Matrixnet.Leaves.CommonNorm().DataBias;
}

double TMnSseInfo::Scale() const {
    return Matrixnet.Leaves.CommonNorm().DataScale;
}

double TMnSseInfo::NativeBias() const {
    return Matrixnet.Leaves.CommonNorm().NativeDataBias;
}

double TMnSseInfo::NativeScale() const {
    return Matrixnet.Leaves.CommonNorm().NativeDataScale;
}

void TMnSseInfo::UsedFactors(TSet<ui32>& factors) const {
    factors.clear();
    for (size_t i = 0; i < Matrixnet.Meta.FeaturesSize; ++i) {
        factors.insert(Matrixnet.Meta.Features[i].Index);
    }
}

#if !defined(MATRIXNET_WITHOUT_ARCADIA)
void TMnSseInfo::UsedFactors(TSet<NFactorSlices::TFullFactorIndex>& factors) const {
    factors.clear();

    TFactorsMap factorsMap;
    FactorsMap(factorsMap);

    for (const auto& entry : factorsMap) {
        factors.insert(entry.second);
    }
}
#endif // !defined(MATRIXNET_WITHOUT_ARCADIA)

ui32 TMnSseInfo::MaxFactorIndex() const {
    return ui32(MaxFactorIdx);
}

void TMnSseInfo::UpdateNormalization(const double bias, const double scale) {
    auto& m = Matrixnet.Leaves.CommonNorm("UpdateNormalization currently don't work with multidata models");
    UpdateNativeScaleAndBias();

    m.DataScale /= scale;
    m.DataBias = (m.DataBias + bias) / scale;
}

void TMnSseInfo::Renorm(const double scale, const double bias) {
    auto& m = Matrixnet.Leaves.CommonNorm("Renorm currently don't work with multidata models");
    UpdateNativeScaleAndBias();

    m.DataScale *= scale;
    m.DataBias = m.DataBias * scale + bias;
}

void TMnSseInfo::SetScaleAndBias(const double scale, const double bias) {
    auto& m = Matrixnet.Leaves.CommonNorm("SetScaleAndBias currently don't work with multidata models");
    UpdateNativeScaleAndBias();

    m.DataScale = scale;
    m.DataBias = bias;
}

void TMnSseInfo::SetNativeScaleAndBias(const double scale, const double bias) {
    auto& m = Matrixnet.Leaves.CommonNorm("SetNativeScaleAndBias don't work with multidata models");
    m.NativeDataScale = scale;
    m.NativeDataBias = bias;

    Info["NativeDataScale"] = ToString<double>(m.NativeDataScale);
    Info["NativeDataBias"] = ToString<double>(m.NativeDataBias);
}

void TMnSseInfo::UpdateNativeScaleAndBias() {
    auto& m = Matrixnet.Leaves.CommonNorm("UpdateNativeScaleAndBias currently don't work with multidata models");
    if (Info.contains("NativeDataScale")) {
        return;
    }

    SetNativeScaleAndBias(m.DataScale, m.DataBias);
}

void TMnSseInfo::Borders(TBorders& borders) const {
    int vid = 0;
    for (size_t i = 0; i < Matrixnet.Meta.FeaturesSize; ++i) {
        const ui32 fid = Matrixnet.Meta.Features[i].Index;
        const ui32 len = Matrixnet.Meta.Features[i].Length;
        TBorder& values = borders[fid];
        for (ui32 j = 0; j < len; ++j) {
            values.push_back(Matrixnet.Meta.Values[vid++]);
        }
    }
}

#if !defined(MATRIXNET_WITHOUT_ARCADIA)
void TMnSseInfo::FactorsMap(TFactorsMap& factorsMap) const {
    factorsMap.clear();

    auto mxFactors = TArrayRef<const TFeature>(Matrixnet.Meta.Features, Matrixnet.Meta.FeaturesSize);
    auto factorIter = mxFactors.begin();

    for (size_t sliceIndex = 0;
        factorIter < mxFactors.end() && sliceIndex < Slices.size();
        ++sliceIndex)
    {
        const auto& slice = Slices[sliceIndex];
        EFactorSlice sliceId = EFactorSlice::COUNT;
        if (Y_UNLIKELY(!TryFromString(SliceNames[sliceIndex], sliceId))) {
            Y_ASSERT(false);
            factorsMap.clear();
            return;
        }

        if (Y_UNLIKELY(factorIter->Index < slice.StartFeatureIndexOffset)) {
            Y_ASSERT(false);
            factorsMap.clear();
            return;
        }

        for (;
            factorIter < mxFactors.end() && factorIter->Index < slice.EndFeatureIndexOffset;
            ++factorIter)
        {
            // Both factors and slices must be sorted by index
            Y_ASSERT(factorIter->Index >= slice.StartFeatureIndexOffset);

            factorsMap.insert(std::make_pair(factorIter->Index,
                NFactorSlices::TFullFactorIndex(sliceId,
                    factorIter->Index - slice.StartFeatureIndexOffset)));
        }
    }
}

void TMnSseInfo::ExplainTrees(const float* factors, const TTreeExplanationParams& cfg, TTreeExplanations& expls) const {
    const auto* oldMultiData = std::get_if<TMultiData>(&Matrixnet.Leaves.Data);
    Y_ENSURE(oldMultiData && oldMultiData->MultiData.size() == 1, "ExplainTrees currently don't work with multidata models");
    auto& multiData0 = oldMultiData->MultiData[0];

    // condition index to factor index
    TVector<size_t> cond2findex;
    // binary features
    TVector<bool> features;
    {
        size_t condIdx = 0;
        for (size_t i = 0; i < Matrixnet.Meta.FeaturesSize; ++i) {
            const size_t factorIndex = Matrixnet.Meta.Features[i].Index;
            const size_t bordersNum  = Matrixnet.Meta.Features[i].Length;
            for (size_t borderIdx = 0; borderIdx < bordersNum; ++borderIdx) {
                cond2findex.push_back(factorIndex);
                features.push_back(factors[factorIndex] > Matrixnet.Meta.Values[condIdx++]);
            }
        }
    }

    // create explanation info for matrixnet trees
    {
        // index for mnInfo.Data
        size_t dataIdx = Matrixnet.Meta.GetSizeToCount(0); // skip trees with 0 conditions.
        // index for tree conditions
        size_t treeCondIdx = 0;
        for (size_t condNum = 1; condNum < Matrixnet.Meta.SizeToCountSize; ++condNum) {
            for (int treeIdx = 0; treeIdx < Matrixnet.Meta.SizeToCount[condNum]; ++treeIdx) {
                TSimpleSharedPtr<TTreeExplanation> tree = new TTreeExplanation();
                size_t valueIdx1 = 0;
                tree->DefaultValue = true;
                bool hasFilteredFactors = false;
                tree->Conditions.resize(condNum);
                for (size_t i = 0; i < condNum; ++i) {
                    const size_t condIdx = Matrixnet.Meta.GetIndex(treeCondIdx++);
                    const size_t factorIdx = cond2findex[condIdx];
                    hasFilteredFactors |=
                        cfg.FactorsFilter.find(factorIdx) != cfg.FactorsFilter.end();
                    tree->Conditions[i].Factor = factorIdx;
                    tree->Conditions[i].Border = Matrixnet.Meta.Values[condIdx];
                    tree->Conditions[i].Value  = features[condIdx];
                    tree->DefaultValue = tree->DefaultValue && features[condIdx];
                    valueIdx1 |= features[condIdx] << i;
                }
                tree->Value = multiData0.Data[dataIdx + valueIdx1] ^ (1 << 31);
                tree->Value = tree->Value * multiData0.Norm.DataScale;
                tree->Value *= cfg.Scale;
                bool filtered  = false;
                filtered |= !cfg.FactorsFilter.empty() && !hasFilteredFactors;
                filtered |= tree->DefaultValue && cfg.HideDefaultValues;

                if (filtered) {
                    continue;
                }

                // finding min and max value of tree
                tree->Min = Max<double>();
                tree->Max = -Max<double>();
                for (int valueIdx2 = 0; valueIdx2 < (1 << condNum); valueIdx2++) {
                    double value = multiData0.Data[dataIdx++] ^ (1 << 31);
                    value = value * multiData0.Norm.DataScale * cfg.Scale;
                    tree->Min = Min(tree->Min, value);
                    tree->Max = Max(tree->Max, value);
                }

                expls.push_back(tree);
            }
        }
    }

    if (cfg.Sort) {
        // sorting trees by abs(Value) in desc
        Sort(expls.begin(), expls.end(), TTreeExplanation::TPtrComparator());
        std::reverse(expls.begin(), expls.end());
    }

    // calculating accumulated value for each tree
    double sumNeg = 0.0; // sum of negative tree values
    double sumPos = 0.0; // sum of positive tree values
    {
        double accum = 0.0;
        const double bias = !cfg.SkipBias ? multiData0.Norm.DataBias : 0.0f;
        for (int i = expls.ysize() - 1; i >= 0; --i) {
            const double val = expls[i]->Value;
            accum += val;
            expls[i]->Accum = accum + bias;
            if (val > 0) {
                sumPos += val;
            } else {
                sumNeg += val;
            }
        }
    }

    // filling ValuePercent field
    {
        for (int i = expls.ysize() - 1; i >= 0; --i) {
            const double val = expls[i]->Value;
            expls[i]->ValuePercent = val * 100 / (val > 0 ? sumPos : sumNeg);
        }
    }
}

void TMnSseInfo::ExplainFactorBorders(const float* factors, TFactorBorderExplanations& expls) const {
    TSet<size_t> factorIds;
    ExplainFactorBorders(factors, factorIds, expls);
}

void TMnSseInfo::ExplainFactorBorders(const float* factors, const TSet<size_t> &factorIds, TFactorBorderExplanations& expls) const {

    size_t factorsNum = GetNumFeats();
    const size_t defaultBordersNum = 32;
    TVector<TSimpleSharedPtr<TVector<float>>> curFactorPlanes(defaultBordersNum);
    TVector<const float*> planesPtr(defaultBordersNum);
    TVector<double> curMnValues(defaultBordersNum);

    for (size_t i = 0; i < defaultBordersNum; ++i) {
        curFactorPlanes[i] = new TVector<float>(factors, factors + factorsNum);
        planesPtr[i] = &*curFactorPlanes[i]->begin();
    }

    const bool hasFilter = !factorIds.empty();
    size_t condIdx = 0;
    for (size_t i = 0; i < Matrixnet.Meta.FeaturesSize; ++i) {
        const size_t factorIndex = Matrixnet.Meta.Features[i].Index;
        const size_t bordersNum  = Matrixnet.Meta.Features[i].Length;
        if (hasFilter && factorIds.find(factorIndex) == factorIds.end()) {
            condIdx += bordersNum;
            continue;
        }
        TSimpleSharedPtr<TFactorBorderExplanation> expl =
            new TFactorBorderExplanation(factorIndex, factors[factorIndex]);

        expl->Borders.resize(bordersNum + 1);
        // make sure that we have enough planes in curFactorPlanes
        for (size_t borderIdx = curFactorPlanes.ysize(); borderIdx < bordersNum + 1; borderIdx++) {
            curFactorPlanes.push_back(new TVector<float>(factors, factors + factorsNum));
            planesPtr.push_back(&*curFactorPlanes.back()->begin());
            curMnValues.push_back(0.0);
        }

        TVector<float> borders(&Matrixnet.Meta.Values[condIdx], &Matrixnet.Meta.Values[condIdx + bordersNum]);
        Sort(borders.begin(), borders.end());
        float prevBorder = borders[0] - 1.0f;
        for (int borderIdx = 0; borderIdx < borders.ysize(); borderIdx++) {
            const float curBorder = borders[borderIdx];
            expl->Borders[borderIdx].Border = curBorder;
            // change factorId to be less than current border but more than prev border
            curFactorPlanes[borderIdx]->at(factorIndex) = (prevBorder + curBorder) / 2;
            prevBorder = curBorder;
        }
        {
            // adding last element with +inf border
            const size_t borderIdx = bordersNum;
            expl->Borders[borderIdx].Border = Max<float>();
            // change factorId to be less than current border but more than prev border
            curFactorPlanes[borderIdx]->at(factorIndex) = prevBorder + 1.0f;
        }

        condIdx += bordersNum;

        DoCalcRelevs(&*planesPtr.begin(), &*curMnValues.begin(), bordersNum + 1);
        expl->MnMinValue = Max<double>();
        expl->MnMaxValue = -Max<double>();
        // reset planes and calculate min/max value
        for (size_t borderIdx = 0; borderIdx < bordersNum + 1; borderIdx++) {
            const double val = curMnValues[borderIdx];
            expl->Borders[borderIdx].MnValue = val;
            curFactorPlanes[borderIdx]->at(factorIndex) = factors[factorIndex];
            expl->MnMinValue = Min(expl->MnMinValue, val);
            expl->MnMaxValue = Max(expl->MnMaxValue, val);
        }

        Sort(expl->Borders.begin(), expl->Borders.end());
        expls.push_back(expl);
    }

    // sorting result by spread of matrixnet value.
    TFactorBorderExplanation::TPtrMnSpreadComparator cmp;
    Sort(expls.begin(), expls.end(), cmp);
    std::reverse(expls.begin(), expls.end());
}


const TString& TMnSseInfo::MD5() const {
    if (MD5Sum.empty() && SourceData) {
        // calc md5
        char md5buf[33];
        MD5Sum = MD5::Data(static_cast<const unsigned char*>(SourceData), SourceSize, md5buf);
    }
    return MD5Sum;
}
#endif // !defined(MATRIXNET_WITHOUT_ARCADIA)

double TMnSseInfo::DoCalcRelev(const float* factors) const {
    double result;
    this->DoCalcRelevs(&factors, &result, 1);
    return result;
}

const TMnSseStatic& TMnSseInfo::GetSseDataPtrs() const {
    return Matrixnet;
}

/// Calculate matrixnet value using SSE-optimized method only for num <=128 documents
void MxNetLong(const TMnSseStatic &info, const float* const* factors, double* res, size_t num, size_t stride, size_t numSlices, size_t rangeBegin, size_t rangeFinish, TVector<ui8>* buffer);

void TMnSseInfo::DoCalcRelevs(const float* const* docsFactors, double* resultRelev, const size_t numDocs) const {
    DoCalcRelevs(docsFactors, resultRelev, numDocs, 1);
}

void TMnSseInfo::DoCalcRelevs(const float* const* docsFactors, double* resultRelev, const size_t numDocs, const size_t numSlices) const {
    DoCalcMultiDataRelevs(docsFactors, resultRelev, numDocs, numSlices, 1);
}

void TMnSseInfo::DoCalcRelevs(const float* const* docsFactors, double* resultRelev, const size_t numDocs, const size_t numSlices, const size_t rangeBegin, const size_t rangeFinish) const {
    DoCalcMultiDataRelevs(docsFactors, resultRelev, numDocs, numSlices, 1, rangeBegin, rangeFinish);
}

void TMnSseInfo::DoCalcMultiDataRelevs(const float* const* docsFactors, double* resultRelev, const size_t numDocs, const size_t numSlices, const size_t numValues) const {
    DoCalcMultiDataRelevs(docsFactors, resultRelev, numDocs, numSlices, numValues, 0, Max<size_t>());
}

void TMnSseInfo::DoCalcMultiDataRelevs(const float* const* docsFactors, double* resultRelev, const size_t numDocs, const size_t numSlices, const size_t numValues, const size_t rangeBegin, const size_t rangeFinish) const {
    const auto& leafData = Matrixnet.Leaves.Data;
    if (const auto* oldMultiData = std::get_if<TMultiData>(&leafData)) {
        Y_ENSURE(numValues == oldMultiData->MultiData.size(), "DoCalcMultiDataRelevs: values number not equal to MultiData count: " << numValues << " != " << oldMultiData->MultiData.size());
    } else if (const auto* compactMultiData = std::get_if<TMultiDataCompact>(&leafData)) {
        Y_ENSURE(numValues == compactMultiData->NumClasses, "DoCalcMultiDataRelevs: values number not equal to compact MultiData class count: " << numValues << " != " << compactMultiData->NumClasses);
    }
    size_t remainder = numDocs % 128;
    size_t numDocs128 = numDocs - remainder;
    size_t i = 0;
#if !defined(MATRIXNET_WITHOUT_ARCADIA)
    NDetail::TCalcContext& context = CalcContext.GetRef();
    for (; i < numDocs128; i += 128) {
        MxNetLong(Matrixnet, docsFactors + i, resultRelev + i * numValues, 128, numDocs, numSlices, rangeBegin, rangeFinish, &context.MxNet128Buffer);
    }
    if (remainder > 0) {
        MxNetLong(Matrixnet, docsFactors + i, resultRelev + i * numValues, remainder, numDocs, numSlices, rangeBegin, rangeFinish, &context.MxNet128Buffer);
    }
#else
    for (; i < numDocs128; i += 128) {
        MxNetLong(Matrixnet, docsFactors + i, resultRelev + i * numValues, 128, numDocs, numSlices, rangeBegin, rangeFinish, nullptr);
    }
    if (remainder > 0) {
        MxNetLong(Matrixnet, docsFactors + i, resultRelev + i * numValues, remainder, numDocs, numSlices, rangeBegin, rangeFinish, nullptr);
    }
#endif // !defined(MATRIXNET_WITHOUT_ARCADIA)
}

TMnSseInfo::TPreparedBatchPtr MxNetLongBinarization(const TMnSseStatic &info, const float* const* factors, size_t num, size_t stride, size_t numSlices);

void MxNetLongApply(const TMnSseInfo::TPreparedBatch& preparedBatch, const TMnSseStatic &info, double* res, const size_t numValues, size_t rangeBegin, size_t rangeFinish);

TMnSseInfo::TPreparedBatchPtr TMnSseInfo::CalcBinarization(const float* const* docsFactors, const size_t numDocs, const size_t numSlices) const {
    return MxNetLongBinarization(Matrixnet, docsFactors, numDocs, numDocs, numSlices);
}

void TMnSseInfo::DoCalcRelevs(const TMnSseInfo::TPreparedBatch& preparedBatch, double* resultRelev) const {
    DoCalcMultiDataRelevs(preparedBatch, resultRelev, 1);
}

void TMnSseInfo::DoCalcRelevs(const TMnSseInfo::TPreparedBatch& preparedBatch, double* resultRelev, const size_t rangeBegin, const size_t rangeFinish) const {
    DoCalcMultiDataRelevs(preparedBatch, resultRelev, 1, rangeBegin, rangeFinish);
}

void TMnSseInfo::DoCalcMultiDataRelevs(const TMnSseInfo::TPreparedBatch& preparedBatch, double* resultRelev, const size_t numValues) const {
    DoCalcMultiDataRelevs(preparedBatch, resultRelev, numValues, 0, Max<size_t>());
}

void TMnSseInfo::DoCalcMultiDataRelevs(const TMnSseInfo::TPreparedBatch& preparedBatch, double* resultRelev, const size_t numValues, const size_t rangeBegin, const size_t rangeFinish) const {
    const auto& leafData = Matrixnet.Leaves.Data;
    if (const auto* oldMultiData = std::get_if<TMultiData>(&leafData)) {
        Y_ENSURE(numValues == oldMultiData->MultiData.size(), "DoCalcMultiDataRelevs: values number not equal to MultiData count: " << numValues << " != " << oldMultiData->MultiData.size());
    } else if (const auto* compactMultiData = std::get_if<TMultiDataCompact>(&leafData)) {
        Y_ENSURE(numValues == compactMultiData->NumClasses, "DoCalcMultiDataRelevs: values number not equal to compact MultiData class count: " << numValues << " != " << compactMultiData->NumClasses);
    }

    MxNetLongApply(preparedBatch, Matrixnet, resultRelev, numValues, rangeBegin, rangeFinish);
}

size_t GetNumDocs(const TMnSseInfo::TPreparedBatch& preparedBatch);

size_t TMnSseInfo::TSlicedPreparedBatch::GetDocCount() const {
    return PreparedBatch ? GetNumDocs(*PreparedBatch) : 0;
}

void TMnSseInfo::CalcRelevs(const TMnSseInfo::TPreparedBatch& preparedBatch, TVector<double> &result_relev) const {
    result_relev.resize(GetNumDocs(preparedBatch));
    DoCalcRelevs(preparedBatch, result_relev.data());
}

TMnSseInfo::TPreparedBatchPtr TMnSseInfo::CalcBinarization(const TVector<const float*> &docs_features) const {
    return CalcBinarization(docs_features.data(), docs_features.size(), 1);
}

TMnSseInfo::TPreparedBatchPtr TMnSseInfo::CalcBinarization(const TVector< TVector<float> > &features) const {
    TVector<const float*> fPtrs(features.ysize());
    for (int i = 0; i < features.ysize(); ++i) {
        fPtrs[i] = (features[i]).data();
    }
    return CalcBinarization(fPtrs);
}


#if !defined(MATRIXNET_WITHOUT_ARCADIA)

NThreading::TThreadLocalValue<NDetail::TCalcContext> TMnSseInfo::CalcContext;

bool Y_FORCE_INLINE ValidateFactorStorage(const TFactorStorage* const* features, size_t numDocs) {
    if (Y_UNLIKELY(!features || numDocs < 1)) {
        return false;
    }

    // SEARCHPRODINCIDENTS-1690
    bool valid = ValidAndHasFactors(features[0]);
    if (Y_UNLIKELY(!valid)) {
        return false;
    }
    for (size_t doc = 0; doc < numDocs; ++doc) {
        // indeed, they should be either ALL valid or ALL INVALID. Try to prove it with assertion (mvel@, ankineri@)
        Y_ASSERT(valid == ValidAndHasFactors(features[doc]));
        if (Y_UNLIKELY(!ValidAndHasFactors(features[doc]))) {
            return false;
        }
    }
    return true;
}

void TMnSseInfo::DoSlicedCalcRelevs(const TFactorStorage* const* features, double* relevs, size_t numDocs) const {
    if (DynamicBundle) {
        std::fill(relevs, relevs + numDocs, 0);
        for (auto& bundleComponent : DynamicBundle->Components) {
            TVector<double> res(numDocs);
            DoSlicedCalcRelevs(features, res.data(), numDocs, bundleComponent.TreeIndexFrom, bundleComponent.TreeIndexTo);
            for (size_t i = 0; i < numDocs; ++i) {
                bool existsWeight = features[i]->Ptr(bundleComponent.FeatureIndex);
                Y_ASSERT(existsWeight);
                relevs[i] += (
                    (res[i] * bundleComponent.Scale + bundleComponent.Bias) *
                    (existsWeight ? (*features[i])[bundleComponent.FeatureIndex] : 1.0)
                );
            }
        }
    } else {
        DoSlicedCalcRelevs(features, relevs, numDocs, 0, Max<size_t>());
    }
}

void TMnSseInfo::DoSlicedCalcRelevs(const TFactorStorage* const* features, double* relevs, size_t numDocs, size_t rangeBegin, size_t rangeFinish) const {
    if (Y_UNLIKELY(!ValidateFactorStorage(features, numDocs))) {
        return;
    }
    Y_ASSERT(GetSliceNames().size() == GetSlices().size());

    if (GetSlices().empty()) {
        IRelevCalcer::DoSlicedCalcRelevs(features, relevs, numDocs);
        return;
    }

    NDetail::TCalcContext& context = CalcContext.GetRef();
    FillSlicedFeatures(features, numDocs, context.FinalFeatures, context.ExtFeaturesHolder);
    DoCalcRelevs(&context.FinalFeatures[0], relevs, numDocs, GetSlices().size(), rangeBegin, rangeFinish);
}

size_t TMnSseInfo::SlicedCalcNumberOfFeaturesPerDoc() const {
    size_t numFeatsPerDoc = 0;
    for (const auto& slice : Slices) {
        numFeatsPerDoc += slice.EndFeatureIndexOffset - slice.StartFeatureIndexOffset;
    }
    return numFeatsPerDoc;
}

TMnSseInfo::TSlicedPreparedBatch TMnSseInfo::DoSlicedCalcBinarization(const TFactorStorage* const* features, size_t numDocs) const {
    Y_ASSERT(ValidateFactorStorage(features, numDocs));
    Y_ASSERT(GetSliceNames().size() == GetSlices().size());

    NDetail::TCalcContext& context = CalcContext.GetRef();
    if (GetSlices().empty()) {
        const size_t numFeats = GetNumFeats();
        size_t totalFeatures = 0;
        for (size_t i = 0; i < numDocs; ++i) {
            totalFeatures += (features[i]->Size() < numFeats ? numFeats : 0);
        }

        context.ExtFeaturesHolder.clear();
        context.ExtFeaturesHolder.reserve(totalFeatures);
        context.FinalFeatures.resize(numDocs, nullptr);
        for (size_t i = 0; i < numDocs; ++i) {
            const auto& view = features[i]->CreateConstView();
            TArrayRef<const float> region{~view, +view};
            context.FinalFeatures[i] = GetOrCopyFeatures(region, numFeats, context.ExtFeaturesHolder);
        }

        return {CalcBinarization(&context.FinalFeatures[0], numDocs, 1), features};
    } else {
        FillSlicedFeatures(features, numDocs, context.FinalFeatures, context.ExtFeaturesHolder);
        return {CalcBinarization(&context.FinalFeatures[0], numDocs, GetSlices().size()), features};
    }
}

void TMnSseInfo::DoSlicedCalcRelevs(const TMnSseInfo::TSlicedPreparedBatch& slicedPreparedBatch, double* relevs) const {
    if (DynamicBundle) {
        size_t numDocs = GetNumDocs(*slicedPreparedBatch.PreparedBatch);
        std::fill(relevs, relevs + numDocs, 0);
        for (auto& bundleComponent : DynamicBundle->Components) {
            TVector<double> res(numDocs);
            DoCalcRelevs(*slicedPreparedBatch.PreparedBatch, res.data(), bundleComponent.TreeIndexFrom, bundleComponent.TreeIndexTo);
            for (size_t i = 0; i < numDocs; ++i) {
                bool existsWeight = slicedPreparedBatch.Features[i]->Ptr(bundleComponent.FeatureIndex);
                Y_ASSERT(existsWeight);
                relevs[i] += (
                    (res[i] * bundleComponent.Scale + bundleComponent.Bias) *
                    (existsWeight ? (*slicedPreparedBatch.Features[i])[bundleComponent.FeatureIndex] : 1.0)
                );
            }
        }
    } else {
        DoCalcRelevs(*slicedPreparedBatch.PreparedBatch, relevs);
    }
}

TMnSseInfo::TSlicedPreparedBatch TMnSseInfo::SlicedCalcBinarization(const TVector<const TFactorStorage*> &features) const {
    return DoSlicedCalcBinarization(features.data(), features.size());
}

TMnSseInfo::TSlicedPreparedBatch TMnSseInfo::SlicedCalcBinarization(const TVector<TFactorStorage*> &features) const {
    return DoSlicedCalcBinarization(features.data(), features.size());
}

void TMnSseInfo::SlicedCalcRelevs(const TMnSseInfo::TSlicedPreparedBatch& slicedPreparedBatch, TVector<double> &relevs) const {
    relevs.resize(GetNumDocs(*slicedPreparedBatch.PreparedBatch));
    DoSlicedCalcRelevs(slicedPreparedBatch, relevs.data());
}
#endif // !defined(MATRIXNET_WITHOUT_ARCADIA)

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// starting from here this is all about MxNetLong (matrixnet sse calculation)
//-----------------------------------------------------------------------------


// this function is actualy used nowhere, here it is just for understanding the data inside TMnSseStatic
/*
static double _mncalc_simple(const TMnSseStatic &mn, const float* plane) {
    TVector<int> features(mn.ValuesSize);
    int treeCondIdx = 0;
    for (size_t i = 0; i < mn.FeaturesSize; ++i) {
        for (size_t j = 0; j < mn.Features[i].Length; ++j) {
            features[treeCondIdx] = plane[mn.Features[i].Index] > mn.Values[treeCondIdx] ? 1 : 0;
            treeCondIdx++;
        }
    }

    int dataIdx = 0;
    treeCondIdx = 0;
    i64 res = 0L;
    i64 totalNumTrees = 0L;
    for (size_t condNum = 0; condNum < mn.SizeToCountSize; ++condNum) {
        const int treesNum = mn.SizeToCount[condNum];
        totalNumTrees += treesNum;
        for (int treeIdx = 0; treeIdx < treesNum; ++treeIdx) {
            int valueIdx = 0;
            for (size_t i = 0; i < condNum; ++i) {
                const int condIdx = mn.DataIndices[treeCondIdx] / 4;
                valueIdx += features[condIdx] << i;
                treeCondIdx++;
            }
            res += ui32(mn.Data[dataIdx + valueIdx]);
            dataIdx += 1 << condNum;
        }
    }
    res -= totalNumTrees << 31;
    return res*mn.DataScale + mn.DataBias;
}
*/

#ifdef NDEBUG
#define NFORCED_INLINE Y_FORCE_INLINE
#else
#define NFORCED_INLINE
#endif

static __m128i NFORCED_INLINE GetCmp16(const __m128 &c0, const __m128 &c1, const __m128 &c2, const __m128 &c3, const __m128 test) {
    const __m128i r0 = _mm_castps_si128(_mm_cmpgt_ps(c0, test));
    const __m128i r1 = _mm_castps_si128(_mm_cmpgt_ps(c1, test));
    const __m128i r2 = _mm_castps_si128(_mm_cmpgt_ps(c2, test));
    const __m128i r3 = _mm_castps_si128(_mm_cmpgt_ps(c3, test));
    const __m128i packed = _mm_packs_epi16(_mm_packs_epi32(r0, r1), _mm_packs_epi32(r2, r3));
    return _mm_and_si128(_mm_set1_epi8(0x01), packed);
}

static __m128i NFORCED_INLINE GetCmp16(const float *factors, const __m128 test) {
    const __m128 *ptr = (__m128 *)factors;
    return GetCmp16(ptr[0], ptr[1], ptr[2], ptr[3], test);
}

template<class X>
static X *GetAligned(X *val) {
    uintptr_t off = ((uintptr_t)val) & 0xf;
    val = (X *)((ui8 *)val - off + 0x10);
    return val;
}

namespace {
template<int X>
struct TGetSign {
    static constexpr int Value = (X > 0) ? 1 : ((X < 0) ? -1 : 0);
};

template<int X, int Shift, int ShiftSign>
struct TSelBitImpl;

template<int X, int Shift>
struct TSelBitImpl<X, Shift, 1> {
    static NFORCED_INLINE __m128i SelBit(__m128i value) {
        return _mm_and_si128(_mm_slli_epi16(value, +Shift), _mm_slli_epi16(_mm_set1_epi8(0x01), X));
    }
};

template<int X, int Shift>
struct TSelBitImpl<X, Shift, -1> {
    static NFORCED_INLINE __m128i SelBit(__m128i value) {
        return _mm_and_si128(_mm_srli_epi16(value, -Shift), _mm_slli_epi16(_mm_set1_epi8(0x01), X));
    }
};

template<int X>
struct TSelBitImpl<X, 0, 0> {
    static NFORCED_INLINE __m128i SelBit(__m128i value) {
        return _mm_and_si128(value, _mm_slli_epi16(_mm_set1_epi8(0x01), X));
    }
};
}

template<int X, int Shift>
static NFORCED_INLINE __m128i SelBit(__m128i value) {
    return TSelBitImpl<X, Shift, TGetSign<Shift>::Value>::SelBit(value);
}

template<int X>
static void NFORCED_INLINE AssignAdd(__m128i &v, __m128i add) {
    if (X != 0)
        v = _mm_add_epi32(v, add);
    else
        v = add;
}

template<size_t Depth, int X>
static void NFORCED_INLINE DoBitSet(
    __m128i &v0,
    __m128i &v1,
    __m128i &v2,
    __m128i &v3,
    __m128i &v4,
    __m128i &v5,
    __m128i &v6,
    __m128i &v7,
    __m128i b,
    const void *fetch)
{
    Y_PREFETCH_READ(fetch, 3);
    AssignAdd<X>(v0, SelBit<X, X - 0>(b));
    AssignAdd<X>(v1, SelBit<X, X - 1>(b));
    AssignAdd<X>(v2, SelBit<X, X - 2>(b));
    AssignAdd<X>(v3, SelBit<X, X - 3>(b));
    AssignAdd<X>(v4, SelBit<X, X - 4>(b));
    AssignAdd<X>(v5, SelBit<X, X - 5>(b));
    AssignAdd<X>(v6, SelBit<X, X - 6>(b));
    AssignAdd<X>(v7, SelBit<X, X - 7>(b));
}


template<size_t Num, size_t Depth, typename T>
static void DoMXLane(const ui32 *val, const T *indices, const T *end, ui8 *res) {
    const T *fetch = Min(end - Depth, indices + 10);
    __m128i v0 = _mm_setzero_si128();
    __m128i v1 = _mm_setzero_si128();
    __m128i v2 = _mm_setzero_si128();
    __m128i v3 = _mm_setzero_si128();
    __m128i v4 = _mm_setzero_si128();
    __m128i v5 = _mm_setzero_si128();
    __m128i v6 = _mm_setzero_si128();
    __m128i v7 = _mm_setzero_si128();

    if (0 < Depth) {
        DoBitSet<Depth, 0>(v0, v1, v2, v3, v4, v5, v6, v7, _mm_load_si128((__m128i *)(val + indices[0])), val + fetch[0]);
    }
    if (1 < Depth) {
        DoBitSet<Depth, 1>(v0, v1, v2, v3, v4, v5, v6, v7, _mm_load_si128((__m128i *)(val + indices[1])), val + fetch[1]);
    }
    if (2 < Depth) {
        DoBitSet<Depth, 2>(v0, v1, v2, v3, v4, v5, v6, v7, _mm_load_si128((__m128i *)(val + indices[2])), val + fetch[2]);
    }
    if (3 < Depth) {
        DoBitSet<Depth, 3>(v0, v1, v2, v3, v4, v5, v6, v7, _mm_load_si128((__m128i *)(val + indices[3])), val + fetch[3]);
    }
    if (4 < Depth) {
        DoBitSet<Depth, 4>(v0, v1, v2, v3, v4, v5, v6, v7, _mm_load_si128((__m128i *)(val + indices[4])), val + fetch[4]);
    }
    if (5 < Depth) {
        DoBitSet<Depth, 5>(v0, v1, v2, v3, v4, v5, v6, v7, _mm_load_si128((__m128i *)(val + indices[5])), val + fetch[5]);
    }
    if (6 < Depth) {
        DoBitSet<Depth, 6>(v0, v1, v2, v3, v4, v5, v6, v7, _mm_load_si128((__m128i *)(val + indices[6])), val + fetch[6]);
    }
    if (7 < Depth) {
        DoBitSet<Depth, 7>(v0, v1, v2, v3, v4, v5, v6, v7, _mm_load_si128((__m128i *)(val + indices[7])), val + fetch[7]);
    }

    Y_PREFETCH_READ(indices, 3);
    if (Num > 0)
        _mm_store_si128((__m128i *)(res + 16 * 0), v0);
    if (Num > 1)
        _mm_store_si128((__m128i *)(res + 16 * 1), v1);
    if (Num > 2)
        _mm_store_si128((__m128i *)(res + 16 * 2), v2);
    if (Num > 3)
        _mm_store_si128((__m128i *)(res + 16 * 3), v3);
    if (Num > 4)
        _mm_store_si128((__m128i *)(res + 16 * 4), v4);
    if (Num > 5)
        _mm_store_si128((__m128i *)(res + 16 * 5), v5);
    if (Num > 6)
        _mm_store_si128((__m128i *)(res + 16 * 6), v6);
    if (Num > 7)
        _mm_store_si128((__m128i *)(res + 16 * 7), v7);
}


static NFORCED_INLINE void Group4(float *dst, const float* const factors[128], size_t index0, size_t index1, size_t index2, size_t index3, size_t num, const __m128& substitutionVal) {
    for (size_t i = 0; i < num; ++i) {
        const float *s = factors[i];
        const __m128 tmpvals = _mm_set_ps(s[index3], s[index2], s[index1], s[index0]);
        const __m128 masks = _mm_cmpunord_ps(tmpvals, tmpvals);
        const __m128 result = _mm_or_ps(_mm_andnot_ps(masks, tmpvals), _mm_and_ps(masks, substitutionVal));
        _mm_store_ss(&dst[i + 0 * 128], result);
        _mm_store_ss(&dst[i + 1 * 128], _mm_shuffle_ps(result,result, _MM_SHUFFLE(0,0,0,1)));
        _mm_store_ss(&dst[i + 2 * 128], _mm_shuffle_ps(result,result, _MM_SHUFFLE(0,0,0,2)));
        _mm_store_ss(&dst[i + 3 * 128], _mm_shuffle_ps(result,result, _MM_SHUFFLE(0,0,0,3)));
    }
}

static NFORCED_INLINE void Group4(float *dst, const float* const factors[128], size_t index0, size_t index1, size_t index2, size_t index3, size_t num) {
    for (size_t i = 0; i < num; ++i) {
        const float *factor = factors[i];
        dst[i + 0 * 128] = factor[index0];
        dst[i + 1 * 128] = factor[index1];
        dst[i + 2 * 128] = factor[index2];
        dst[i + 3 * 128] = factor[index3];
    }
}

template<size_t Num>
static NFORCED_INLINE void DoLane(size_t length, const float *factors, ui32 *&dst, const float *&values) {
    for (size_t i = 0; i < length; ++i) {
        __m128 value = _mm_set1_ps(values[0]);
        __m128i agg =                               GetCmp16(factors + 0 * 16, value);
        if (Num > 1)
            agg = _mm_add_epi16(agg, _mm_slli_epi16(GetCmp16(factors + 1 * 16, value), 1));
        if (Num > 2)
            agg = _mm_add_epi16(agg, _mm_slli_epi16(GetCmp16(factors + 2 * 16, value), 2));
        if (Num > 3)
            agg = _mm_add_epi16(agg, _mm_slli_epi16(GetCmp16(factors + 3 * 16, value), 3));
        if (Num > 4)
            agg = _mm_add_epi16(agg, _mm_slli_epi16(GetCmp16(factors + 4 * 16, value), 4));
        if (Num > 5)
            agg = _mm_add_epi16(agg, _mm_slli_epi16(GetCmp16(factors + 5 * 16, value), 5));
        if (Num > 6)
            agg = _mm_add_epi16(agg, _mm_slli_epi16(GetCmp16(factors + 6 * 16, value), 6));
        if (Num > 7)
            agg = _mm_add_epi16(agg, _mm_slli_epi16(GetCmp16(factors + 7 * 16, value), 7));
        _mm_store_si128((__m128i *)dst, agg);
        dst += 4;
        ++values;
    }
}

static void  NFORCED_INLINE DoFetch2(ui16 value, const int *d0, __m128i &val) {
    const __m128i lo = _mm_cvtsi32_si128(d0[value & 0xff]);
    const __m128i hi = _mm_cvtsi32_si128(d0[value >> 8]);
    val = _mm_add_epi64(val, _mm_unpacklo_epi64(lo, hi));
}

template <typename TLeaf, typename TDocValues, size_t X>
struct TFetcherWorker;

template<size_t... I>
static NFORCED_INLINE void DoFetchMany(__m128i* datas, const int *d0, const int *d1, const int *d2, const int *d3, const ui16 *bins, const std::index_sequence<I...>&) {
    __m128i val;
    auto dummy = {
        (
        val = datas[I],
        DoFetch2(bins[I + 0 * 64], d0, val),
        DoFetch2(bins[I + 1 * 64], d1, val),
        DoFetch2(bins[I + 2 * 64], d2, val),
        DoFetch2(bins[I + 3 * 64], d3, val),
        datas[I] = val)...};
    Y_UNUSED(dummy);
}

template <size_t X>
struct TFetcherWorker<int, ui64[128], X> {
    static void DoFetch128(const int *d0, const int *d1, const int *d2, const int *d3, const ui16 *bins, ui64 (*datasOrig)[128], size_t num) {
        __m128i* datas = (__m128i*) datasOrig;
        if constexpr (X > 1) {
            while (num >= 16) {
                DoFetchMany(datas, d0, d1, d2, d3, bins, std::make_index_sequence<8>());
                num -= 16;
                datas += 8;
                bins += 8;
            }
        }

        for (size_t i = 0; i < num; i += 2) {
            __m128i val = datas[0];
            DoFetch2(bins[0 * 64], d0, val);
            DoFetch2(bins[1 * 64], d1, val);
            DoFetch2(bins[2 * 64], d2, val);
            DoFetch2(bins[3 * 64], d3, val);
            datas[0] = val;
            ++datas;
            ++bins;
        }
    }
};

namespace {
struct TMnSseStaticTraits {
    using TLeaf = int;
    using TDocValues128 = ui64[128];
    static TLeaf zeroes[256];
};
}

TMnSseStaticTraits::TLeaf TMnSseStaticTraits::zeroes[256];

namespace {
template <typename Traits>
struct TFetcherBase {
    using TLeaf = typename Traits::TLeaf;
    using TDocValues128 = typename Traits::TDocValues128;

    alignas(0x10) ui8 Bins[4][128];
    alignas(0x10) const TLeaf *Data[4];
    alignas(0x10) TDocValues128 Vals;
    size_t Num = 0;
    size_t Count = 0;

    TFetcherBase(size_t num)
    : Num(num)
    {
#if defined(_msan_enabled_)
    // Bypass use-of-uninitialized-value error
    memset(Bins, 0, sizeof(Bins));
    memset(Data, 0, sizeof(Data));
#endif
    }

    void InitVals(TDocValues128&& vals) {
        Vals = std::move(vals);
    }

    template <size_t X>
    NFORCED_INLINE ui8 *Alloc(const TLeaf *data) {
        DoFetch<X>();
        ui8 *res = Bins[Count];
        Data[Count] = data;
        if constexpr (X > 1) {
            Y_PREFETCH_READ(data + 0 * 16, 3);
            Y_PREFETCH_READ(data + 1 * 16, 3);
            Y_PREFETCH_READ(data + 2 * 16, 3);
            Y_PREFETCH_READ(data + 3 * 16, 3);
        }
        ++Count;
        return res;
    }

    template <size_t X>
    NFORCED_INLINE void DoFlush() {
        if (DoFetch<X>())
            return;
        for (size_t i = Count; i < 4; ++i) {
            Alloc<X>(Traits::zeroes);
        }
        DoFetch<X>();
    }

    template <size_t X>
    NFORCED_INLINE bool DoFetch() {
        if (Count == 4){
            TFetcherWorker<TLeaf, TDocValues128, X>::DoFetch128(Data[0], Data[1], Data[2], Data[3], (const ui16 *)&Bins[0][0], &Vals, Num);
            Count = 0;
        }
        return Count == 0;
    }
};

struct TFetcher : public TFetcherBase<TMnSseStaticTraits> {
    using Base = TFetcherBase<TMnSseStaticTraits>;

    TFetcher(size_t num)
    : Base(num)
    {
        ClearVals();
    }

    inline void ClearVals() {
        Base::Count = 0;
        memset(Base::Vals, 0, sizeof(Base::Vals));
        for (size_t i = 0; i < 4; ++i) {
            Base::Data[i] = TMnSseStaticTraits::zeroes;
        }
    }
};
}

template <size_t X>
void CalcFactors(const TFeature *features, const i8* missedValueDirections, const float *compares, size_t featureLength, const float* const factors[128], size_t num, ui32 *out, const ui32 featsIndexOff = 0) {
    alignas(0x10) float factorsGrouped[4][128];
    memset(factorsGrouped, 0, sizeof(factorsGrouped));
    const size_t last = featureLength ? featureLength - 1 : 0;
    const size_t remainder = featureLength % 4;
    const size_t featureLength4 = featureLength - remainder;
    size_t i = 0;
    using namespace NMatrixnetIdl;
    const auto infinity = std::numeric_limits<float>::infinity(); // use alias just to keep code smaller
    for (; i < featureLength4; i += 4) {
        if (Y_UNLIKELY(missedValueDirections && (
            missedValueDirections[i + 0] != EFeatureDirection_None ||
            missedValueDirections[i + 1] != EFeatureDirection_None ||
            missedValueDirections[i + 2] != EFeatureDirection_None ||
            missedValueDirections[i + 3] != EFeatureDirection_None))) {
            const __m128 defaultValuesForMissing = _mm_set_ps(
                missedValueDirections[i + 3] == EFeatureDirection_Left ? -infinity : infinity,
                missedValueDirections[i + 2] == EFeatureDirection_Left ? -infinity : infinity,
                missedValueDirections[i + 1] == EFeatureDirection_Left ? -infinity : infinity,
                missedValueDirections[i + 0] == EFeatureDirection_Left ? -infinity : infinity
            );
            Group4(
                &factorsGrouped[0][0],
                factors,
                features[i + 0].Index - featsIndexOff,
                features[i + 1].Index - featsIndexOff,
                features[i + 2].Index - featsIndexOff,
                features[i + 3].Index - featsIndexOff,
                num,
                defaultValuesForMissing
            );
        } else {
            Group4(
                &factorsGrouped[0][0],
                factors,
                features[i + 0].Index - featsIndexOff,
                features[i + 1].Index - featsIndexOff,
                features[i + 2].Index - featsIndexOff,
                features[i + 3].Index - featsIndexOff,
                num
            );
        }
        DoLane<X>(features[i + 0].Length, &factorsGrouped[0][0], out, compares);
        DoLane<X>(features[i + 1].Length, &factorsGrouped[1][0], out, compares);
        DoLane<X>(features[i + 2].Length, &factorsGrouped[2][0], out, compares);
        DoLane<X>(features[i + 3].Length, &factorsGrouped[3][0], out, compares);
    }

    if (remainder > 0) {
        if (Y_UNLIKELY(missedValueDirections && (
            missedValueDirections[Min(i + 0, last)] != EFeatureDirection_None ||
            missedValueDirections[Min(i + 1, last)] != EFeatureDirection_None ||
            missedValueDirections[Min(i + 2, last)] != EFeatureDirection_None ||
            missedValueDirections[Min(i + 3, last)] != EFeatureDirection_None))) {
            const __m128 defaultVals = _mm_set_ps(
                missedValueDirections[Min(i + 3, last)] == EFeatureDirection_Left ? -infinity : infinity,
                missedValueDirections[Min(i + 2, last)] == EFeatureDirection_Left ? -infinity : infinity,
                missedValueDirections[Min(i + 1, last)] == EFeatureDirection_Left ? -infinity : infinity,
                missedValueDirections[Min(i + 0, last)] == EFeatureDirection_Left ? -infinity : infinity
            );
            Group4(
                &factorsGrouped[0][0],
                factors,
                features[Min(i + 0, last)].Index - featsIndexOff,
                features[Min(i + 1, last)].Index - featsIndexOff,
                features[Min(i + 2, last)].Index - featsIndexOff,
                features[Min(i + 3, last)].Index - featsIndexOff,
                num,
                defaultVals);

        } else {
            Group4(
                &factorsGrouped[0][0],
                factors,
                features[Min(i + 0, last)].Index - featsIndexOff,
                features[Min(i + 1, last)].Index - featsIndexOff,
                features[Min(i + 2, last)].Index - featsIndexOff,
                features[Min(i + 3, last)].Index - featsIndexOff,
                num);
        }
        for (size_t j = 0; j < remainder; ++j) {
            DoLane<X>(features[i + j].Length, &factorsGrouped[j][0], out, compares);
        }
    }
}


template <size_t Num>
static void CalculateFeatures(const TMnSseStaticMeta& info, const float* const* factors, size_t numDocs, size_t stride, size_t numSlices, ui32* val) {
    if (numSlices == 1 || info.FeatureSlices == nullptr) {
        CalcFactors<Num>(info.Features, info.MissedValueDirections, info.Values, info.FeaturesSize, factors, numDocs, val);
    } else {
        const TFeature *feats = info.Features;
        const float *values = info.Values;
        ui32 *dst = val;
        for (size_t i = 0; i < Min(numSlices, info.NumSlices); ++i) {
            ui32 startFeatsOff = info.FeatureSlices[i].StartFeatureOffset;
            ui32 startValuesOff = info.FeatureSlices[i].StartValueOffset;
            ui32 endFeatsOff = info.FeatureSlices[i].EndFeatureOffset;
            ui32 endValuesOff = info.FeatureSlices[i].EndValueOffset;
            ui32 startFeatsIndexOff = info.FeatureSlices[i].StartFeatureIndexOffset;
            const i8* featureDirections = info.MissedValueDirectionsSize ? info.MissedValueDirections + startFeatsOff : nullptr;
            CalcFactors<Num>(feats + startFeatsOff, featureDirections, values + startValuesOff, endFeatsOff - startFeatsOff, factors, numDocs, dst, startFeatsIndexOff);
            factors += stride;
            dst += (endValuesOff - startValuesOff) * 4;
        }
    }
}

template<size_t Num, typename T, typename Fetcher, size_t Depth>
static void CalculateAllTreesOnDepth(
    const TMnSseStaticMeta& info,
    const typename Fetcher::TLeaf*& data,
    const T*& beginDataIndices,
    const T* endDataIndices,
    Fetcher* fetcher,
    const ui32* val,
    const int treeRangeStart,
    const int treeRangeFinish,
    int& proceed
) {
    const int size = info.GetSizeToCount(Depth);
    const int startIndex = std::max(0, treeRangeStart - proceed);
    const int endIndex = std::min(size, treeRangeFinish - proceed);
    const T* beginDataIndicesCopy = beginDataIndices;
    const typename Fetcher::TLeaf* dataCopy = data;
    beginDataIndicesCopy += Depth * startIndex;
    dataCopy += (1 << Depth) * startIndex;
    for (int i = startIndex; i < endIndex; ++i) {
        DoMXLane<Num, Depth, T>(val, beginDataIndicesCopy, endDataIndices, fetcher->template Alloc<Num>(dataCopy));
        beginDataIndicesCopy += Depth;
        dataCopy += (1 << Depth);
    }
    beginDataIndices += Depth * size;
    data += (1 << Depth) * size;
    proceed += size;
}

template <size_t Num, typename T, typename Fetcher>
static void CalculateAllTrees(
    const TMnSseStaticMeta& info,
    const typename Fetcher::TLeaf* data,
    const T *dataIndices,
    Fetcher* fetcher,
    const ui32* val,
    const int treeRangeStart = 0,
    const int treeRangeFinish = Max()
) {
    const T *indices = dataIndices;
    const T *end = info.DataIndicesSize + indices;
    int proceed = 0;

    CalculateAllTreesOnDepth<Num, T, Fetcher, 0>(info, data, indices, end, fetcher, val, treeRangeStart, treeRangeFinish, proceed);
    CalculateAllTreesOnDepth<Num, T, Fetcher, 1>(info, data, indices, end, fetcher, val, treeRangeStart, treeRangeFinish, proceed);
    CalculateAllTreesOnDepth<Num, T, Fetcher, 2>(info, data, indices, end, fetcher, val, treeRangeStart, treeRangeFinish, proceed);
    CalculateAllTreesOnDepth<Num, T, Fetcher, 3>(info, data, indices, end, fetcher, val, treeRangeStart, treeRangeFinish, proceed);
    CalculateAllTreesOnDepth<Num, T, Fetcher, 4>(info, data, indices, end, fetcher, val, treeRangeStart, treeRangeFinish, proceed);
    CalculateAllTreesOnDepth<Num, T, Fetcher, 5>(info, data, indices, end, fetcher, val, treeRangeStart, treeRangeFinish, proceed);
    CalculateAllTreesOnDepth<Num, T, Fetcher, 6>(info, data, indices, end, fetcher, val, treeRangeStart, treeRangeFinish, proceed);
    CalculateAllTreesOnDepth<Num, T, Fetcher, 7>(info, data, indices, end, fetcher, val, treeRangeStart, treeRangeFinish, proceed);
    CalculateAllTreesOnDepth<Num, T, Fetcher, 8>(info, data, indices, end, fetcher, val, treeRangeStart, treeRangeFinish, proceed);
}

class TPreparedSubBatch {
public:
    TPreparedSubBatch(size_t valuesSize, size_t numFactors, TVector<ui8>* buffer = nullptr)
        : NumFactors(numFactors)
    {
        if (buffer) {
            buffer->resize(valuesSize * sizeof(__m128) + 0x20);
            Val = GetAligned(reinterpret_cast<ui32*>(buffer->data()));
        } else {
            Hold.Reset(new ui8[valuesSize * sizeof(__m128) + 0x20]);
            Val = GetAligned(reinterpret_cast<ui32*>(Hold.Get()));
        }
    }

    TPreparedSubBatch(size_t numFactors, ui32* buffer)
        : NumFactors(numFactors)
    {
        Val = buffer;
        Y_ASSERT(((uintptr_t)Val & 0xf) == 0);
    }

    ui32* GetVal() {
        return Val;
    }
    const ui32* GetVal() const {
        return Val;
    }

    size_t NumFactors = 0;
private:
    ui32* Val = nullptr;
    TArrayHolder<ui8> Hold;
};

template<size_t Num, typename T>
static void MxNet128ClassicApply(const TPreparedSubBatch& preparedSubBatch, TFetcher* fetcher, const TMnSseStatic &info, const T *dataIndices, size_t rangeBegin, size_t rangeFinish, double* res) {
    const TVector<TMultiData::TLeafData>& multiData = std::get<TMultiData>(info.Leaves.Data).MultiData;

    for (size_t dataIndex = 0; dataIndex < multiData.size(); ++dataIndex) {
        ui64 sub = info.Meta.GetSizeToCount(0) +
            info.Meta.GetSizeToCount(1) +
            info.Meta.GetSizeToCount(2) +
            info.Meta.GetSizeToCount(3) +
            info.Meta.GetSizeToCount(4) +
            info.Meta.GetSizeToCount(5) +
            info.Meta.GetSizeToCount(6) +
            info.Meta.GetSizeToCount(7) +
            info.Meta.GetSizeToCount(8);

        double k = 1.0;
        if (!(rangeBegin == 0 && rangeFinish == Max<size_t>())) {
            CalculateAllTrees<Num, T, TFetcher>(info.Meta, multiData[dataIndex].Data, dataIndices,
                fetcher, preparedSubBatch.GetVal(), rangeBegin, rangeFinish);
            if (sub != 0) {
                k = double(rangeFinish - rangeBegin) / sub;
            }
        } else {
            CalculateAllTrees<Num, T, TFetcher>(info.Meta, multiData[dataIndex].Data, dataIndices,
                fetcher, preparedSubBatch.GetVal());
        }
        fetcher->template DoFlush<Num>();

        sub = (sub << 31);
        const auto multiDataSize = multiData.size();
        const double dataScale = multiData[dataIndex].Norm.DataScale;
        const double dataBias = multiData[dataIndex].Norm.DataBias;
        const i64* src = (i64*)fetcher->Vals;
        if (Y_LIKELY(multiDataSize == 1)) {
            for (size_t i = 0; i < preparedSubBatch.NumFactors; ++i) {
                res[i] = (src[i] - sub * k) * dataScale + dataBias * k;
            }
        } else {
            double* curResult = &res[dataIndex];
            for (size_t i = 0; i < preparedSubBatch.NumFactors; ++i) {
                *curResult = (src[i] - sub * k) * dataScale + dataBias * k;
                curResult += multiDataSize;
            }
            fetcher->ClearVals();
        }
    }
}


template<size_t Num, typename T>
static void MxNet128Classic(const TMnSseStatic &info, const float* const* factors, double* res, size_t num,
                            const T *dataIndices, size_t stride, size_t numSlices, size_t rangeBegin, size_t rangeFinish, TVector<ui8>* buffer) {
    TPreparedSubBatch preparedSubBatch(info.Meta.ValuesSize, num, buffer);
    TFetcher fetcher(num);

    CalculateFeatures<Num>(info.Meta, factors, num, stride, numSlices, preparedSubBatch.GetVal());

    MxNet128ClassicApply<Num, T>(preparedSubBatch, &fetcher, info, dataIndices, rangeBegin, rangeFinish, res);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class TDocClassValues {
public:
    struct TValueCount {
        ui64 Value;
        ui32 Count;
    };

private:
    size_t Docs;
    size_t Classes;
    TValueCount Data[128 * MAX_COMPACT_MODEL_CLASSES];

public:
    TDocClassValues() = default;

    TDocClassValues(size_t docs, size_t classes)
        : Docs(docs)
        , Classes(classes)
    {
        Y_ASSERT(Classes <= MAX_COMPACT_MODEL_CLASSES);
    }

    ~TDocClassValues() = default;

    void Add(size_t doc, size_t cla, ui64 delta) {
        Y_ASSERT(doc < Docs);
        Y_ASSERT(cla < Classes);
        TValueCount& v = Data[doc * Classes + cla];
        v.Value += delta;
        v.Count += 1;
    }

    const TValueCount At(size_t doc, size_t cla) const {
        Y_ASSERT(doc < Docs);
        Y_ASSERT(cla < Classes);
        return Data[doc * Classes + cla];
    }

    void Clear() {
        memset(Data, 0, Docs * Classes * sizeof(TValueCount));
    }
};

struct TMnSseStaticCompactTraits {
    using TLeaf = TValueClassLeaf;
    using TDocValues128 = TDocClassValues;
    static TLeaf zeroes[256];
};

TMnSseStaticCompactTraits::TLeaf TMnSseStaticCompactTraits::zeroes[256];

template <typename TLeaf>
static std::pair<TLeaf, TLeaf> NFORCED_INLINE DoFetch2Compact(ui16 value, const TLeaf *d0) {
    ui16 bin_hi = value >> 8;
    ui16 bin_lo = value & 0xff;
    return std::make_pair(d0[bin_lo], d0[bin_hi]);
}

template <size_t X>
struct TFetcherWorker<TValueClassLeaf, TDocClassValues, X> {
    static void DoFetch128(
        const TValueClassLeaf *d0,
        const TValueClassLeaf *d1,
        const TValueClassLeaf *d2,
        const TValueClassLeaf *d3,
        const ui16 *bins,
        TDocClassValues* datas,
        size_t num)
    {
        const TValueClassLeaf* ds[] = {d0, d1, d2, d3};
        for (size_t i = 0; i < num; i += 2) {
            for (size_t j = 0; j < 4; ++j) {
                std::pair<TValueClassLeaf, TValueClassLeaf> leaves = DoFetch2Compact<TValueClassLeaf>(bins[j * 64], ds[j]);
                datas->Add(i, leaves.first.ClassId, ui64(ui32(leaves.first.Value)));
                if (i + 1 < num) {
                    datas->Add(i + 1, leaves.second.ClassId, ui64(ui32(leaves.second.Value)));
                }
            }
            ++bins;
        }
    }
};

struct TFetcherCompact : public TFetcherBase<TMnSseStaticCompactTraits> {
    using Base = TFetcherBase<TMnSseStaticCompactTraits>;
    using TDocValues = typename TMnSseStaticCompactTraits::TDocValues128;

    TFetcherCompact(int num, size_t nclasses)
    : Base(num)
    {
        Base::InitVals(TDocValues(num, nclasses));
        ClearVals();
    }

    inline void ClearVals() {
        Base::Count = 0;
        Base::Vals.Clear();
        for (size_t i = 0; i < 4; ++i) {
            Base::Data[i] = TMnSseStaticCompactTraits::zeroes;
        }
    }
};

template<size_t Num, typename T>
static void MxNet128Compact(const TMnSseStatic &info, const float* const* factors, double* res, size_t num,
                            const T *dataIndices, size_t stride, size_t numSlices, TVector<ui8>* buffer)
{
    const TMultiDataCompact& multiData = std::get<TMultiDataCompact>(info.Leaves.Data);

    TArrayHolder<ui8> hold;
    ui32* val;
    if (buffer) {
        buffer->resize(sizeof(ui32) * info.Meta.ValuesSize * 4 + 0x20);
        val = GetAligned(reinterpret_cast<ui32*>(buffer->data()));
    } else {
        hold.Reset(new ui8[sizeof(ui32) * info.Meta.ValuesSize * 4 + 0x20]);
        val = GetAligned(reinterpret_cast<ui32*>(hold.Get()));
    }

    using TFetcherType = TFetcherCompact;
    TFetcherType *fetcher = new(GetAligned((TFetcherType *)alloca(sizeof(TFetcherType) + 0x20))) TFetcherType(num, multiData.NumClasses);

    CalculateFeatures<Num>(info.Meta, factors, num, stride, numSlices, val);
    CalculateAllTrees<Num, T, TFetcherType>(info.Meta, multiData.Data, dataIndices, fetcher, val);

    fetcher->template DoFlush<Num>();

    const double dataScale = multiData.Norm.DataScale;
    const double dataBias = multiData.Norm.DataBias;
    const TDocClassValues& vals = fetcher->Vals;

    double* curResult = res;
    for (size_t i = 0; i < num; ++i) {
        for (size_t j = 0; j < multiData.NumClasses; ++j) {
            TDocClassValues::TValueCount v = vals.At(i, j);
            double result = (i64(v.Value) - (i64(v.Count) << 31)) * dataScale + dataBias;
            *curResult++ = result;
        }
    }

    fetcher->ClearVals();
}

////////////////////////////////////////////////////////////////////////////////////////////////

template<size_t Num, typename T>
static void MxNet128(const TMnSseStatic &info, const float* const* factors, double* res, size_t num,
                     const T *dataIndices, size_t stride, size_t numSlices, size_t rangeBegin, size_t rangeFinish, TVector<ui8>* buffer)
{
    if (std::get_if<TMultiData>(&info.Leaves.Data)) {
        MxNet128Classic<Num, T>(info, factors, res, num, dataIndices, stride, numSlices, rangeBegin, rangeFinish, buffer);
    } else if (std::get_if<TMultiDataCompact>(&info.Leaves.Data)) {
        MxNet128Compact<Num, T>(info, factors, res, num, dataIndices, stride, numSlices, buffer);
    } else {
        Y_FAIL();
    }
};


template<typename T>
static void MxNetLongTyped(const TMnSseStatic &info, const float* const factors[128], double* res, size_t num, size_t stride, size_t numSlices, size_t rangeBegin, size_t rangeFinish, TVector<ui8>* buffer) {
    if (num == 0)
        return;
    switch ((num - 1) / 16) {
        case 0:
            return MxNet128<1, T>(info, factors, res, num, (const T*) info.Meta.DataIndicesPtr, stride, numSlices, rangeBegin, rangeFinish, buffer);
        case 1:
            return MxNet128<2, T>(info, factors, res, num, (const T*) info.Meta.DataIndicesPtr, stride, numSlices, rangeBegin, rangeFinish, buffer);
        case 2:
            return MxNet128<3, T>(info, factors, res, num, (const T*) info.Meta.DataIndicesPtr, stride, numSlices, rangeBegin, rangeFinish, buffer);
        case 3:
            return MxNet128<4, T>(info, factors, res, num, (const T*) info.Meta.DataIndicesPtr, stride, numSlices, rangeBegin, rangeFinish, buffer);
        case 4:
            return MxNet128<5, T>(info, factors, res, num, (const T*) info.Meta.DataIndicesPtr, stride, numSlices, rangeBegin, rangeFinish, buffer);
        case 5:
            return MxNet128<6, T>(info, factors, res, num, (const T*) info.Meta.DataIndicesPtr, stride, numSlices, rangeBegin, rangeFinish, buffer);
        case 6:
            return MxNet128<7, T>(info, factors, res, num, (const T*) info.Meta.DataIndicesPtr, stride, numSlices, rangeBegin, rangeFinish, buffer);
        case 7:
            return MxNet128<8, T>(info, factors, res, num, (const T*) info.Meta.DataIndicesPtr, stride, numSlices, rangeBegin, rangeFinish, buffer);
    }
}

void MxNetLong(const TMnSseStatic &info, const float* const factors[128], double* res, size_t num, size_t stride,
               size_t numSlices, size_t rangeBegin, size_t rangeFinish, TVector<ui8>* buffer)
{
    if (info.Meta.Has16Bits) {
        Y_ENSURE(!std::get_if<TMultiDataCompact>(&info.Leaves.Data));
        return MxNetLongTyped<ui16>(info, factors, res, num, stride, numSlices, rangeBegin, rangeFinish, buffer);
    }
    return MxNetLongTyped<ui32>(info, factors, res, num, stride, numSlices, rangeBegin, rangeFinish, buffer);
}

////////////////////////////////////////////////////////////////////////////////////////////////

class TMnSseInfo::TPreparedBatch {
public:
    TPreparedBatch(const TMnSseStatic &info, size_t numDocs) {
        Y_ENSURE(std::get_if<TMultiData>(&info.Leaves.Data));
        NumDocs = numDocs;
        if (NumDocs > 0) {
            size_t numSubBatches = (NumDocs + 128 - 1) / 128;
            const size_t valSize = info.Meta.ValuesSize * sizeof(__m128);
            Vals = TBuffer(valSize * numSubBatches + 0x20);
            char* valsAligned = GetAligned(Vals.Data());
            Batches.reserve(numSubBatches);
            for (size_t i = 0; i < numSubBatches - 1; ++i) {
                Batches.emplace_back(128, reinterpret_cast<ui32*>(valsAligned + i * valSize));
            }
            Batches.emplace_back(NumDocs - (numSubBatches - 1) * 128, reinterpret_cast<ui32*>(valsAligned + (numSubBatches - 1) * valSize));
        }
    }
    size_t GetNumDocs() const {
        return NumDocs;
    }

    TVector<TPreparedSubBatch> Batches;

private:
    size_t NumDocs = 0;
    TBuffer Vals;
};

void TMnSseInfo::TPreparedBatchDeleter::Destroy(TMnSseInfo::TPreparedBatch* obj) {
    delete obj;
}

size_t GetNumDocs(const TMnSseInfo::TPreparedBatch& preparedBatch) {
    return preparedBatch.GetNumDocs();
}

TMnSseInfo::TPreparedBatchPtr MxNetLongBinarization(const TMnSseStatic &info, const float* const* factors, size_t num, size_t stride, size_t numSlices) {
    TMnSseInfo::TPreparedBatchPtr preparedBatch{new TMnSseInfo::TPreparedBatch(info, num)};
    for (size_t i = 0; i < preparedBatch->Batches.size(); ++i) {
        switch ((preparedBatch->Batches[i].NumFactors - 1) / 16) {
            case 0:
                CalculateFeatures<1>(info.Meta, factors + i * 128, preparedBatch->Batches[i].NumFactors, stride, numSlices, preparedBatch->Batches[i].GetVal());
                break;
            case 1:
                CalculateFeatures<2>(info.Meta, factors + i * 128, preparedBatch->Batches[i].NumFactors, stride, numSlices, preparedBatch->Batches[i].GetVal());
                break;
            case 2:
                CalculateFeatures<3>(info.Meta, factors + i * 128, preparedBatch->Batches[i].NumFactors, stride, numSlices, preparedBatch->Batches[i].GetVal());
                break;
            case 3:
                CalculateFeatures<4>(info.Meta, factors + i * 128, preparedBatch->Batches[i].NumFactors, stride, numSlices, preparedBatch->Batches[i].GetVal());
                break;
            case 4:
                CalculateFeatures<5>(info.Meta, factors + i * 128, preparedBatch->Batches[i].NumFactors, stride, numSlices, preparedBatch->Batches[i].GetVal());
                break;
            case 5:
                CalculateFeatures<6>(info.Meta, factors + i * 128, preparedBatch->Batches[i].NumFactors, stride, numSlices, preparedBatch->Batches[i].GetVal());
                break;
            case 6:
                CalculateFeatures<7>(info.Meta, factors + i * 128, preparedBatch->Batches[i].NumFactors, stride, numSlices, preparedBatch->Batches[i].GetVal());
                break;
            case 7:
                CalculateFeatures<8>(info.Meta, factors + i * 128, preparedBatch->Batches[i].NumFactors, stride, numSlices, preparedBatch->Batches[i].GetVal());
                break;
        }
    }
    return preparedBatch;
}

template <typename T>
void MxNetLongApplyTyped(const TMnSseInfo::TPreparedBatch& preparedBatch, const TMnSseStatic &info, double* res, const size_t numValues, size_t rangeBegin, size_t rangeFinish) {
    for (size_t i = 0; i < preparedBatch.Batches.size(); ++i) {
        TFetcher fetcher(preparedBatch.Batches[i].NumFactors);
        switch ((preparedBatch.Batches[i].NumFactors - 1) / 16) {
            case 0:
                MxNet128ClassicApply<1, T>(preparedBatch.Batches[i], &fetcher, info, (const T*) info.Meta.DataIndicesPtr, rangeBegin, rangeFinish, res + i * 128 * numValues);
                break;
            case 1:
                MxNet128ClassicApply<2, T>(preparedBatch.Batches[i], &fetcher, info, (const T*) info.Meta.DataIndicesPtr, rangeBegin, rangeFinish, res + i * 128 * numValues);
                break;
            case 2:
                MxNet128ClassicApply<3, T>(preparedBatch.Batches[i], &fetcher, info, (const T*) info.Meta.DataIndicesPtr, rangeBegin, rangeFinish, res + i * 128 * numValues);
                break;
            case 3:
                MxNet128ClassicApply<4, T>(preparedBatch.Batches[i], &fetcher, info, (const T*) info.Meta.DataIndicesPtr, rangeBegin, rangeFinish, res + i * 128 * numValues);
                break;
            case 4:
                MxNet128ClassicApply<5, T>(preparedBatch.Batches[i], &fetcher, info, (const T*) info.Meta.DataIndicesPtr, rangeBegin, rangeFinish, res + i * 128 * numValues);
                break;
            case 5:
                MxNet128ClassicApply<6, T>(preparedBatch.Batches[i], &fetcher, info, (const T*) info.Meta.DataIndicesPtr, rangeBegin, rangeFinish, res + i * 128 * numValues);
                break;
            case 6:
                MxNet128ClassicApply<7, T>(preparedBatch.Batches[i], &fetcher, info, (const T*) info.Meta.DataIndicesPtr, rangeBegin, rangeFinish, res + i * 128 * numValues);
                break;
            case 7:
                MxNet128ClassicApply<8, T>(preparedBatch.Batches[i], &fetcher, info, (const T*) info.Meta.DataIndicesPtr, rangeBegin, rangeFinish, res + i * 128 * numValues);
                break;
        }
    }
}

void MxNetLongApply(const TMnSseInfo::TPreparedBatch& preparedBatch, const TMnSseStatic &info, double* res, const size_t numValues, size_t rangeBegin, size_t rangeFinish) {
    if (info.Meta.Has16Bits) {
        Y_ENSURE(!std::get_if<TMultiDataCompact>(&info.Leaves.Data));
        return MxNetLongApplyTyped<ui16>(preparedBatch, info, res, numValues, rangeBegin, rangeFinish);
    }
    return MxNetLongApplyTyped<ui32>(preparedBatch, info, res, numValues, rangeBegin, rangeFinish);
}

}
