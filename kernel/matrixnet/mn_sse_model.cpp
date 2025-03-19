#include "mn_sse_model.h"

#include <library/cpp/digest/md5/md5.h>

#include <kernel/factor_slices/factor_borders.h>

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

TMnSseModel::TMnSseModel()
    : MultiData(TOwnedMultiDataVector())
{}

TMnSseModel::TMnSseModel(const TMnSseModel& matrixnet)
    : TMnSseInfo(matrixnet)
    , Values(matrixnet.Values)
    , Features(matrixnet.Features)
    , MissedValueDirections(matrixnet.MissedValueDirections)
    , DataIndices(matrixnet.DataIndices)
    , SizeToCount(matrixnet.SizeToCount)
    , MultiData(matrixnet.MultiData)
{
    UpdateMatrixnet();
}

TMnSseModel& TMnSseModel::operator=(const TMnSseModel& matrixnet) {
    if (this != &matrixnet) {
        TMnSseInfo::operator=(matrixnet);

        Values = matrixnet.Values;
        Features = matrixnet.Features;
        MissedValueDirections = matrixnet.MissedValueDirections;
        DataIndices = matrixnet.DataIndices;
        SizeToCount = matrixnet.SizeToCount;
        MultiData = matrixnet.MultiData;

        UpdateMatrixnet();
    }

    return *this;
}

TMnSseModel::TMnSseModel(const TMnSseStatic& matrixnet)
        : MultiData(TOwnedMultiDataVector())
{
    CopyFrom(matrixnet);
}

TMnSseModel::TMnSseModel(const TMnSseInfo& matrixnet)
        : MultiData(TOwnedMultiDataVector())
{
    CopyFrom(matrixnet);
}

void TMnSseModel::CopyFrom(const TMnSseStaticMeta& meta, const TMnSseStaticLeaves& leaves) {
    Info.clear();
    Values.assign(meta.Values, meta.Values + meta.ValuesSize);
    Features.assign(meta.Features, meta.Features + meta.FeaturesSize);
    if (meta.MissedValueDirections) {
        MissedValueDirections.assign(meta.MissedValueDirections, meta.MissedValueDirections + meta.MissedValueDirectionsSize);
    } else {
        MissedValueDirections.clear();
    }

    Y_ENSURE(!meta.Has16Bits);
    DataIndices.assign((const ui32*) meta.DataIndicesPtr, (const ui32*) meta.DataIndicesPtr + meta.DataIndicesSize);

    SizeToCount.assign(meta.SizeToCount, meta.SizeToCount + meta.SizeToCountSize);

    if (auto* classic = std::get_if<TMultiData>(&leaves.Data)) {
        MultiData = TOwnedMultiDataVector();
        Matrixnet.Leaves.Data = TMultiData(classic->MultiData.size(), classic->DataSize);

        auto& ownMultiData = std::get<TOwnedMultiDataVector>(MultiData);
        auto& ptrMultiData = std::get<TMultiData>(Matrixnet.Leaves.Data);

        ownMultiData.reserve(classic->MultiData.size());
        for (const auto& data : classic->MultiData) {
            ptrMultiData.MultiData.push_back(data);
            ownMultiData.push_back(TVector<int>(data.Data, data.Data + classic->DataSize));
        }

    } else if (auto* compact = std::get_if<TMultiDataCompact>(&leaves.Data)) {
        MultiData = TOwnedMultiDataCompact();
        auto& ownMultiData = std::get<TOwnedMultiDataCompact>(MultiData);

        ownMultiData.assign(compact->Data, compact->Data + compact->DataSize);
        Matrixnet.Leaves.Data = TMultiDataCompact(
                ownMultiData.data(), ownMultiData.size(),
                compact->Norm, compact->NumClasses);

    } else {
        Y_FAIL();
    }

    if (meta.DynamicBundle) {
        DynamicBundle.ConstructInPlace().Components.assign(meta.DynamicBundle.begin(), meta.DynamicBundle.end());
    }
    UpdateMatrixnet();
}

void TMnSseModel::CopyFrom(const TMnSseStatic& matrixnet) {
    CopyFrom(matrixnet.Meta, matrixnet.Leaves);
}

void TMnSseModel::CopyFrom(const TMnSseInfo& matrixnet) {
    CopyFrom(matrixnet.GetSseDataPtrs());
    Info = matrixnet.Info;
    SetSlicesFromInfo();
}

void TMnSseModel::UpdateMatrixnet() {
    // if missed value present, all MissedValueDirections should have corresponding Feature
    Y_ENSURE(MissedValueDirections.empty() || MissedValueDirections.size() == Features.size());

    Matrixnet.Meta.Values = Values.data();
    Matrixnet.Meta.ValuesSize = Values.size();
    Matrixnet.Meta.Features = Features.data();
    Matrixnet.Meta.FeaturesSize = Features.size();
    Matrixnet.Meta.MissedValueDirections = MissedValueDirections.empty() ? nullptr : MissedValueDirections.data();
    Matrixnet.Meta.MissedValueDirectionsSize = MissedValueDirections.size();
    Matrixnet.Meta.DataIndicesPtr = DataIndices.data();
    Matrixnet.Meta.DataIndicesSize = DataIndices.size();
    Matrixnet.Meta.Has16Bits = false;
    Matrixnet.Meta.SizeToCount = SizeToCount.data();
    Matrixnet.Meta.SizeToCountSize = SizeToCount.size();
    Matrixnet.Meta.DynamicBundle = DynamicBundle ? DynamicBundle->Components : TArrayRef<TDynamicBundleComponent>{};

    // Warning: does not update bias/scale

    if (auto* classic = std::get_if<TOwnedMultiDataVector>(&MultiData)) {
        Y_ENSURE(classic->size() == std::get<TMultiData>(Matrixnet.Leaves.Data).MultiData.size());
        ssize_t dataSize = -1;
        for (auto i : xrange(classic->size())) {
            if (dataSize == -1) {
                dataSize = classic->at(i).ysize();
            }
            Y_ENSURE(dataSize == classic->at(i).ysize(), "multidata size differs");
            std::get<TMultiData>(Matrixnet.Leaves.Data).MultiData[i].Data = classic->at(i).data();
        }
        std::get<TMultiData>(Matrixnet.Leaves.Data).DataSize = size_t(dataSize);

    } else if (auto* compact = std::get_if<TOwnedMultiDataCompact>(&MultiData)) {
        std::get<TMultiDataCompact>(Matrixnet.Leaves.Data).Data = compact->data();
        std::get<TMultiDataCompact>(Matrixnet.Leaves.Data).DataSize = compact->size();

    } else {
        Y_FAIL();
    }

    InitMaxFactorIndex();
    SetSlicesFromInfo(); // Slice offsets should be recalculated for new Features and Values
}

void TMnSseModel::Swap(TMnSseModel& obj) {
    DoSwap(Values, obj.Values);
    DoSwap(Features, obj.Features);
    DoSwap(MissedValueDirections, obj.MissedValueDirections);
    DoSwap(DataIndices, obj.DataIndices);
    DoSwap(SizeToCount, obj.SizeToCount);
    DoSwap(MultiData, obj.MultiData);
    TMnSseInfo::Swap(obj);
}

void TMnSseModel::Clear() {
    TMnSseInfo::Clear();
    Values.clear();
    Features.clear();
    MissedValueDirections.clear();
    DataIndices.clear();
    SizeToCount.clear();

    if (auto* classic = std::get_if<TOwnedMultiDataVector>(&MultiData)) {
        classic->clear();
    } else if (auto* compact = std::get_if<TOwnedMultiDataCompact>(&MultiData)) {
        compact->clear();
    } else {
        Y_FAIL();
    }

    UpdateMatrixnet();
}

namespace {
class TMD5Input: public IInputStream {
public:
    inline TMD5Input(IInputStream* slave) noexcept
        : Slave_(slave)
    {
    }

    ~TMD5Input() override {
    }

    inline const char* Sum(char* buf) {
        return MD5Sum.End(buf);
    }

private:
    size_t DoRead(void* buf, size_t len) override {
        const size_t ret = Slave_->Read(buf, len);
        MD5Sum.Update(buf, ret);
        return ret;
    }

    /* Note that default implementation of DoSkip works perfectly fine here as
     * it's implemented in terms of DoRead. */

private:
    IInputStream* Slave_;
    MD5 MD5Sum;
};
} // namespace

void TMnSseModel::LoadFromFlatbuf(IInputStream* in) {
        ui32 messageLength = 0;
        ::Load(in, messageLength);
        TArrayHolder<ui8> arrayHolder(new ui8[messageLength]);
        in->LoadOrFail(arrayHolder.Get(), messageLength);
        {
            flatbuffers::Verifier verifier(arrayHolder.Get(), messageLength);
            Y_ENSURE(NMatrixnetIdl::VerifyTMNSSEModelBuffer(verifier), "flatbuffers model verification failed");
        }
        auto flatbufMxNet = NMatrixnetIdl::GetTMNSSEModel(arrayHolder.Get());
        Values.assign(flatbufMxNet->Values()->data(), flatbufMxNet->Values()->data() + flatbufMxNet->Values()->size());
        DataIndices.assign(flatbufMxNet->DataIndices()->data(), flatbufMxNet->DataIndices()->data() + flatbufMxNet->DataIndices()->size());
        SizeToCount.assign(flatbufMxNet->SizeToCount()->data(), flatbufMxNet->SizeToCount()->data() + flatbufMxNet->SizeToCount()->size());

        Y_ENSURE(!!flatbufMxNet->MultiData() + !!flatbufMxNet->Data() + !!flatbufMxNet->MultiDataCompact() == 1,
                 "Only one of {Data, MultiData, MultiDataCompact} must be present in model");

        if (flatbufMxNet->Data() || flatbufMxNet->MultiData()) {
            MultiData = TOwnedMultiDataVector();
            Matrixnet.Leaves.Data = TMultiData();
            auto& ownMultiData = std::get<TOwnedMultiDataVector>(MultiData);
            auto& ptrMultiData = std::get<TMultiData>(Matrixnet.Leaves.Data).MultiData;

            if (flatbufMxNet->Data()) {
                ownMultiData.resize(1);
                ownMultiData[0].assign(flatbufMxNet->Data()->data(),
                                       flatbufMxNet->Data()->data() + flatbufMxNet->Data()->size());
                ptrMultiData.resize(1);
                ptrMultiData[0].Norm.DataBias = flatbufMxNet->DataBias();
                ptrMultiData[0].Norm.DataScale = flatbufMxNet->DataScale();
            } else if (flatbufMxNet->MultiData()) {
                ownMultiData.resize(flatbufMxNet->MultiData()->size());
                ptrMultiData.resize(flatbufMxNet->MultiData()->size());
                for (auto i : xrange(flatbufMxNet->MultiData()->size())) {
                    const auto& md = (*flatbufMxNet->MultiData())[i];
                    Y_ENSURE(md->Data(), "empty multidata vec in model");
                    ownMultiData[i].assign(md->Data()->data(), md->Data()->data() + md->Data()->size());
                    ptrMultiData[i].Norm.DataBias = md->DataBias();
                    ptrMultiData[i].Norm.DataScale = md->DataScale();
                }
            }
        } else if (flatbufMxNet->MultiDataCompact()) {
            MultiData = TOwnedMultiDataCompact();
            auto& ownMultiData = std::get<TOwnedMultiDataCompact>(MultiData);

            const auto* fb = flatbufMxNet->MultiDataCompact();
            ownMultiData.resize(fb->Data()->size());
            for (size_t i: xrange(ownMultiData.size())) {
                ownMultiData[i].ClassId = fb->Data()->Get(i)->ClassId();
                ownMultiData[i].Value = fb->Data()->Get(i)->Value();
            }
            Matrixnet.Leaves.Data = TMultiDataCompact(
                    ownMultiData, TNormAttributes(fb->DataBias(), fb->DataScale()), fb->NumClasses());
        }

        Features.clear();
        for (const auto feature : *flatbufMxNet->Features()) {
            Features.emplace_back(feature->Index(), feature->Length());
        }

        if (flatbufMxNet->MissedValueDirections()) {
            MissedValueDirections.assign(flatbufMxNet->MissedValueDirections()->data(), flatbufMxNet->MissedValueDirections()->data() + flatbufMxNet->MissedValueDirections()->size());
        }

        Info.clear();
        if (flatbufMxNet->InfoMap()) {
            for (const auto keyValue : *flatbufMxNet->InfoMap()) {
                Info[TString(keyValue->Key()->c_str(), keyValue->Key()->size())] = TString(keyValue->Value()->c_str(), keyValue->Value()->size());
            }
        }
}

/* Load pre flatbuffers formatted model*/
void TMnSseModel::LoadOld(IInputStream* in) {
    ::Load(in, Values);
    ::Load(in, Features);
    ui32 size = 0;
    ::Load(in, size);
    if (size == 0) {
        ::Load(in, DataIndices);
    } else {
        DataIndices.resize(size);
        for (size_t i = 0; i < size; ++i) {
            ui16 val16;
            ::Load(in, val16);
            DataIndices[i] = val16;
        }
    }
    ::Load(in, SizeToCount);

    MultiData = TOwnedMultiDataVector();
    Matrixnet.Leaves.Data = TMultiData();
    auto& ownMultiData = std::get<TOwnedMultiDataVector>(MultiData);
    auto& ptrMultiData = std::get<TMultiData>(Matrixnet.Leaves.Data).MultiData;

    ownMultiData.resize(1);
    ::Load(in, ownMultiData[0]);
    ptrMultiData.resize(1);
    ::Load(in, ptrMultiData[0].Norm.DataBias);
    ::Load(in, ptrMultiData[0].Norm.DataScale);
    try {
        ::Load(in, Info);
    } catch (const TLoadEOF&) {
        // that's normal. Just loading model in old format without props field.
    }
}

void TMnSseModel::Load(IInputStream* rawIn) {
    TMD5Input in(rawIn);

    ui32 formatDescriptor = 0;
    ::Load(&in, formatDescriptor);
    if (formatDescriptor != FLATBUFFERS_MN_MODEL_MARKER) {
        TMemoryInput memInp(&formatDescriptor, sizeof(ui32));
        TMultiInput multiInp(&memInp, &in);
        LoadOld(&multiInp);
    } else {
        LoadFromFlatbuf(&in);
    }
    char md5buf[33];
    MD5Sum = in.Sum(md5buf);

    SetDynamicBundleFromInfo();
    UpdateMatrixnet();
}

void TMnSseModel::SaveFlatbuf(IOutputStream *out) const {
    flatbuffers::FlatBufferBuilder builder(1024);
    std::vector<flatbuffers::Offset<NMatrixnetIdl::TKeyValue>> infoMapValuesVector;
    for (const auto& keyValue : Info) {
        infoMapValuesVector.push_back(NMatrixnetIdl::CreateTKeyValue(builder,
            builder.CreateString(keyValue.first.data(), keyValue.first.size()),
            builder.CreateString(keyValue.second.data(), keyValue.second.size())));
    }
    auto infoMap = builder.CreateVector(infoMapValuesVector);
    auto values = builder.CreateVector(Values);
    auto missedValueDirections = builder.CreateVector(MissedValueDirections);
    auto dataIndices = builder.CreateVector(DataIndices);
    auto sizeToCount = builder.CreateVector(SizeToCount);
    auto features = builder.CreateVectorOfStructs((NMatrixnetIdl::TFeature*)Features.data(), Features.size());

    using TDataOffset = flatbuffers::Offset<flatbuffers::Vector<int>>;
    using TMultiDataVecOffset =  flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<NMatrixnetIdl::TMultiData>>>;
    using TMultiDataCompactOffset = flatbuffers::Offset<NMatrixnetIdl::TMultiDataCompact>;
    TMaybe<std::variant<TDataOffset, TMultiDataVecOffset, TMultiDataCompactOffset>> data;

    if (const auto* mdVector = std::get_if<TOwnedMultiDataVector>(&MultiData)) {
        const auto& mdPtr = std::get<TMultiData>(Matrixnet.Leaves.Data).MultiData;

        if (mdVector->size() == 1) {
            data = builder.CreateVector(mdVector->at(0));
        } else {
            std::vector<flatbuffers::Offset<NMatrixnetIdl::TMultiData>> multiDataVec;
            for (auto i : xrange(mdVector->size())) {
                auto dataVec = builder.CreateVector(mdVector->at(i));
                auto md = NMatrixnetIdl::CreateTMultiData(
                        builder,
                        mdPtr[i].Norm.DataBias,
                        mdPtr[i].Norm.DataScale,
                        dataVec);
                multiDataVec.emplace_back(md);
            }
            data = builder.CreateVector(multiDataVec);
        }

    } else if (const auto* mdCompact = std::get_if<TOwnedMultiDataCompact>(&MultiData)) {
        auto leaves = builder.CreateVectorOfStructs((NMatrixnetIdl::TValueClassLeaf*) mdCompact->data(), mdCompact->size());
        const auto& mdPtr = std::get<TMultiDataCompact>(Matrixnet.Leaves.Data);
        data = NMatrixnetIdl::CreateTMultiDataCompact(
                builder, leaves, mdPtr.Norm.DataBias, mdPtr.Norm.DataScale, mdPtr.NumClasses);
    } else {
        Y_FAIL();
    }

    NMatrixnetIdl::TMNSSEModelBuilder modelBuilder(builder);
    modelBuilder.add_Features(features);
    modelBuilder.add_MissedValueDirections(missedValueDirections);
    modelBuilder.add_Values(values);
    modelBuilder.add_DataIndices(dataIndices);
    modelBuilder.add_SizeToCount(sizeToCount);

    Y_ENSURE(data.Defined());
    if (const auto* dataOffset = std::get_if<TDataOffset>(&data.GetRef())) {
        const auto& mdPtr = std::get<TMultiData>(Matrixnet.Leaves.Data).MultiData;
        modelBuilder.add_Data(*dataOffset);
        modelBuilder.add_DataBias(mdPtr[0].Norm.DataBias);
        modelBuilder.add_DataScale(mdPtr[0].Norm.DataScale);
    } else if (const auto* multidataOffset = std::get_if<TMultiDataVecOffset>(&data.GetRef())) {
        modelBuilder.add_MultiData(*multidataOffset);
    } else if (const auto* compactOffset = std::get_if<TMultiDataCompactOffset>(&data.GetRef())) {
        modelBuilder.add_MultiDataCompact(*compactOffset);
    } else {
        Y_FAIL();
    }

    modelBuilder.add_InfoMap(infoMap);
    NMatrixnetIdl::FinishTMNSSEModelBuffer(builder, modelBuilder.Finish());
    ::Save(out, FLATBUFFERS_MN_MODEL_MARKER); // header for new model format
    ::Save(out, (ui32)builder.GetSize()); // rest message size
    out->Write(builder.GetBufferPointer(), builder.GetSize());
}

void TMnSseModel::Save(IOutputStream *out) const {
    SaveFlatbuf(out);
}

void TMnSseModel::RemoveUnusedFeatures() {
    TVector<bool> usedFeatures(Values.ysize());
    for (int i = 0; i < DataIndices.ysize(); ++i) {
        usedFeatures[DataIndices[i] / 4] = true;
    }

    TMap<int, int> old2new;
    int nextFeatureIdx = 0;
    int nextFactorIdx = 0;
    int curFeatureIdx = 0;
    for (int i = 0; i < Features.ysize(); ++i) {
        const ui32 factorIdx = Features[i].Index;
        int numOfUsed = 0;
        for (size_t j = 0; j < Features[i].Length; ++j, ++curFeatureIdx) {
            if (usedFeatures[curFeatureIdx]) {
                old2new[curFeatureIdx] = nextFeatureIdx;
                Values[nextFeatureIdx] = Values[curFeatureIdx];
                nextFeatureIdx++;
                numOfUsed++;
            }
        }
        if (numOfUsed > 0) {
            Features[nextFactorIdx].Index = factorIdx;
            Features[nextFactorIdx].Length = numOfUsed;
            if (!!MissedValueDirections) {
                MissedValueDirections[nextFactorIdx] = MissedValueDirections[i];
            }
            nextFactorIdx++;
        }
    }

    for (int i = 0; i < DataIndices.ysize(); ++i) {
        const int oldIdx = DataIndices[i] / 4;
        DataIndices[i] = old2new[oldIdx] * 4;
    }
    Features.resize(nextFactorIdx);
    if (!!MissedValueDirections) {
        MissedValueDirections.resize(nextFactorIdx);
    }
    Values.resize(nextFeatureIdx);
    UpdateMatrixnet();
    return;
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

void TMnSseModel::Truncate(const int num) {
    TTreeIndices endIndices = CalcDataIndices(num, SizeToCount);
    DataIndices.resize(endIndices.DataIndicesIndex);
    for (auto& data : std::get<TOwnedMultiDataVector>(MultiData)) {
        data.resize(endIndices.DataIndex);
    }
    SizeToCount.resize(endIndices.SizeToCountIndex + 1);
    SizeToCount[endIndices.SizeToCountIndex] = endIndices.SizeToCountValue;
    UpdateMatrixnet();
}

void TMnSseModel::RemapFactors(const TMap<ui32,ui32> &remap, bool strictCheck/* = false*/) {
    for (int i = 0; i < Features.ysize(); ++i) {
        const ui32 fid = Features[i].Index;
        const TMap<ui32,ui32>::const_iterator it = remap.find(fid);
        if (it != remap.end()) {
            Features[i].Index = it->second;
        } else if (strictCheck) {
            ythrow yexception() << "No mapping specified for factor " << fid << Endl;
        }
    }
    const TVector<TFeature> unsortedFeatures = Features;
    const TVector<float> unsortedValues = Values;
    const TVector<i8> unsortedMissedValuesDirections = MissedValueDirections;
    Y_ASSERT(Values.size() <= Max<ui32>());
    TMap<ui32, ui32> indicesRemap;
    // calculate indicesRemap and fix Features
    TMap<size_t, size_t> oldOffsets;
    TVector<std::pair<ui32, size_t>> oldPositions;
    {
        size_t currentOldOffset = 0;
        for (size_t i = 0; i < unsortedFeatures.size(); ++i) {
            const TFeature& feature = unsortedFeatures[i];
            oldOffsets[i] = currentOldOffset;
            currentOldOffset += feature.Length;
            oldPositions.push_back(std::make_pair(feature.Index, i));
        }
    }
    Sort(oldPositions.begin(), oldPositions.end());
    Y_ASSERT(oldPositions.size() == Features.size());
    {
        size_t currentNewOffset = 0;
        for (size_t i = 0; i < Features.size(); ++i) {
            size_t oldIndex = oldPositions[i].second;
            Features[i] = unsortedFeatures[oldIndex];
            if (!!MissedValueDirections) {
                MissedValueDirections[i] = unsortedMissedValuesDirections[oldIndex];
            }
            ui32 oldOffset = oldOffsets[oldIndex];
            for (size_t j = 0; j < Features[i].Length; ++j) {
                indicesRemap[oldOffset + j] = currentNewOffset + j;
            }
            currentNewOffset += Features[i].Length;
        }
    }

    // fix DataIndices
    for (size_t i = 0; i < DataIndices.size(); ++i) {
        Y_ASSERT(DataIndices[i] % 4 == 0);
        ui32 valueIndex = DataIndices[i] / 4;
        Y_ASSERT(indicesRemap.contains(valueIndex));
        DataIndices[i] = 4 * indicesRemap[valueIndex];
    }
    // fix Values
    for (size_t i = 0; i < unsortedValues.size(); ++i) {
        ui32 oldIndex = i;
        if (indicesRemap.contains(oldIndex)) {
            Values[indicesRemap[oldIndex]] = unsortedValues[oldIndex];
        }
    }
    UpdateMatrixnet();
}

TMnSseStatic TMnSseModel::GetSseDataPtrs() const {
    return Matrixnet;
}

}
