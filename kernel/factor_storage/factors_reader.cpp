#include "factors_reader.h"

#include "factor_storage.h"

#include <library/cpp/codecs/float_huffman.h>

using namespace NFactorSlices;

namespace {
    class THuffmanInput
        : public NFSSaveLoad::TFloatsInput
    {
    public:
        explicit THuffmanInput(TStringBuf data)
            : Decoder(data)
        {}

        bool IsFinished() const override {
            return !Decoder.HasMore();
        }

    protected:
        size_t DoLoad(float* dst, size_t n) override {
            Y_ASSERT(n == 0 || !!dst);
            return Decoder.Decode({dst, n});
        }

        size_t DoSkip(size_t n) override {
            return Decoder.Skip(n);
        }

    private:
        NCodecs::NFloatHuff::TDecoder Decoder;
    };

    class TRawInput
        : public NFSSaveLoad::TFloatsInput
    {
    public:
        TRawInput(const float* buf, size_t count)
            : Cur(buf)
            , Count(count)
        {
            Y_ASSERT(Count == 0 || !!Cur);
        }

        bool IsFinished() const override {
            return Count == 0;
        }

    protected:
        size_t DoLoad(float* dst, size_t n) override {
            Y_ASSERT(n == 0 || !!dst);
            const size_t fixCount = Min(n, Count);
            memcpy(dst, Cur, fixCount * sizeof(float));
            Count -= fixCount;
            Cur += fixCount;
            return fixCount;
        }

        size_t DoSkip(size_t n) override {
            const size_t fixCount = Min(n, Count);
            Count -= fixCount;
            Cur += fixCount;
            return fixCount;
        }

    private:
        const float* Cur = nullptr;
        size_t Count = 0;
    };
}

namespace NFSSaveLoad {
    THolder<TFloatsInput> CreateHuffmanFloatsInput(const TStringBuf& data)
    {
        return MakeHolder<THuffmanInput>(data);
    }

    THolder<TFloatsInput> CreateRawFloatsInput(const float* buf, size_t count)
    {
        return MakeHolder<TRawInput>(buf, count);
    }

    void RemoveSkippedFactors(TFactorBorders& borders, const TFactorsSkipSet& skipSet)
    {
        int shift = 0;

        for (auto iter = skipSet.begin(); iter != skipSet.end();) {
            TSliceOffsets offsets = *iter;
            ++iter;

            for (; iter != skipSet.end() && (iter->Overlaps(offsets) || iter->Begin == offsets.End); ++iter) {
                Y_ASSERT(offsets.Begin <= iter->Begin);
                offsets.End = Max(offsets.End, iter->End);
            }

            Y_ASSERT(offsets.Begin >= shift);
            offsets.Erase(TSliceOffsets(0, shift));
            borders.Erase(offsets);
            shift += offsets.Size();
        }
    }

    TSkipFactorsInput::TSkipFactorsInput(THolder<TFloatsInput>&& input,
        const TFactorsSkipSet& skipSet)
        : Input(std::move(input))
        , SkipSet(skipSet)
    {
        Y_ASSERT(Input);
        SkipIter = SkipSet.begin();
    }

    void TSkipFactorsInput::RemoveFactors(const TSliceOffsets& offsets)
    {
        if (!offsets.Empty()) {
            SkipSet.insert(offsets);
            SkipIter = SkipSet.begin();
        }
    }

    const TFactorsSkipSet& TSkipFactorsInput::GetSkipSet() const
    {
        return SkipSet;
    }

    void TSkipFactorsInput::Skip(size_t count) {
        Input->Skip(count);
    }

    size_t TSkipFactorsInput::Load(float* buf, size_t count)
    {
        size_t bufPos = 0;
        while (bufPos < count) {
            TFactorIndex decodePos = DoSkipFactors();

            // Decode next chunk
            //
            size_t countNext = count - bufPos;
            if (SkipIter != SkipSet.end()) {
                countNext = Min<size_t>(count, SkipIter->Begin - decodePos);
            }

            size_t loaded = Input->Load(buf + bufPos, countNext);

            if (loaded < countNext) {
                bufPos += loaded;
                break;
            }
            bufPos += countNext;
        }
        return bufPos;
    }

    bool TSkipFactorsInput::CheckFinished()
    {
        DoSkipFactors();
        return Input->IsFinished();
    }

    size_t TSkipFactorsInput::GetLoadedCount() const
    {
        return Input->GetLoadedCount();
    }

    size_t TSkipFactorsInput::GetSkippedCount() const
    {
        return Input->GetSkippedCount();
    }

    TFactorIndex TSkipFactorsInput::DoSkipFactors()
    {
        TFactorIndex decodePos = Input->GetLoadedCount() + Input->GetSkippedCount();
        Y_ASSERT(SkipSet.end() == SkipIter || decodePos <= SkipIter->Begin);
        for (; SkipIter != SkipSet.end() && decodePos >= SkipIter->Begin; ++SkipIter) {
            const TSliceOffsets& skipOffsets = *SkipIter;
            if (decodePos >= skipOffsets.End) {
                continue;
            }
            Input->Skip(skipOffsets.End - decodePos);
            decodePos = Input->GetLoadedCount() + Input->GetSkippedCount();
            Y_ASSERT(decodePos == skipOffsets.End);
        }

        return decodePos;
    }

    TFactorsReader::TFactorsReader(const TFactorBorders& borders,
        THolder<TFloatsInput>&& input, const TFactorsSkipSet& skipSet)
        : Input(std::forward<THolder<TFloatsInput>>(input), skipSet)
        , Borders(borders)
    {
    }

    void TFactorsReader::RemoveFactors(const TSliceOffsets& offsets)
    {
        Input.RemoveFactors(offsets);
    }

    void TFactorsReader::RemoveSlice(EFactorSlice slice)
    {
        Input.RemoveFactors(Borders[slice]);
    }

    void TFactorsReader::TruncateSlice(EFactorSlice slice, size_t bound)
    {
        if (Borders[slice].Size() > bound) {
            const TSliceOffsets& sliceOffsets = Borders[slice];
            TSliceOffsets tailOffsets(sliceOffsets.Begin + bound, sliceOffsets.End);
            Y_ASSERT(sliceOffsets.Contains(tailOffsets));
            Y_ASSERT(tailOffsets.Size() + bound == sliceOffsets.Size());
            Input.RemoveFactors(tailOffsets);
        }
    }

    const TFactorBorders& TFactorsReader::GetBorders() const
    {
        return Borders;
    }

    void TFactorsReader::PrepareModifiedBorders(TFactorBorders& borders) const
    {
        borders = Borders;
        RemoveSkippedFactors(borders, Input.GetSkipSet());
    }

    void TFactorsReader::ReadTo(TVector<float>& buf)
    {
        TFactorBorders borders;
        PrepareModifiedBorders(borders);

        buf.resize(TFactorDomain(borders).Size());
        const size_t loaded = Input.Load(buf.data(), buf.size());
        Y_ASSERT(loaded == buf.size());
    }

    void TFactorsReader::ReadTo(TFactorStorage& storage,
        const TSlicesMetaInfo& hostInfo, EHostMode mode)
    {
        TFactorBorders borders;
        PrepareModifiedBorders(borders);

        // Handle inconsistencies in slice borders, e.g. missing hier slice
        NFactorSlices::NDetail::TReConstructOptions recOptions;
        recOptions.IgnoreHierarchicalBorders = true;
        NFactorSlices::TSlicesMetaInfo senderInfo;
        if (!NFactorSlices::NDetail::ReConstructMetaInfo(borders, senderInfo, recOptions)) {
            ythrow TBorderValuesError()
                << "failed to reconstruct slice borders from \""
                << SerializeFactorBorders(borders) << "\"";
        }

        NFactorSlices::TFactorDomain senderDomain(senderInfo);

        if (EHostMode::PadAndTruncate == mode) {
            senderInfo = hostInfo;
        } else {
            Y_ASSERT(EHostMode::PadOnly == mode);
            senderInfo.Merge(hostInfo);
        }
        NFactorSlices::TFactorDomain mergedDomain(senderInfo);
        // Invariant: mergedDomain contains hostDomain
        // Invariant: mergedDomain contains senderDomain, if mode == PadOnly

        storage.SetDomain(mergedDomain);

        // Decode chunks of factors into storage;
        // gaps between chunks in storage (due to upsizing and extra meta slices)
        // are filled with zeros
        float* prevPtr = storage.Ptr(0);
        for (auto iter = senderDomain.Begin(); iter.Valid(); iter.NextLeaf()) {
            EFactorSlice currentSlice = iter.GetLeaf();
            size_t size = Min(iter.GetLeafSize(), mergedDomain[currentSlice].Size());
            if (size > 0) {
                float* curPtr = storage.Ptr(0) + mergedDomain[currentSlice].Begin;
                if (curPtr != prevPtr) {
                    Y_ASSERT(prevPtr < curPtr);
                    TGeneralResizeableFactorStorage::FloatClear(prevPtr, curPtr);
                }

                Input.Load(storage.Ptr(iter), size);
                prevPtr = curPtr + size;
            }
            Input.Skip(iter.GetLeafSize() - size);
        }
        float* endPtr = storage.Ptr(storage.Size());
        if (prevPtr != endPtr) {
            Y_ASSERT(prevPtr < endPtr);
            TGeneralResizeableFactorStorage::FloatClear(prevPtr, endPtr);
        }
        Y_ASSERT(Input.CheckFinished());
    }

    THolder<TFactorsReader> CreateReader(const TStringBuf& bordersStr,
        THolder<TFloatsInput>&& input)
    {
        TFactorsSkipSet skipSet;

        NFactorSlices::TFactorBorders borders;
        try {
            DeserializeFactorBorders(bordersStr, borders);
        } catch (NFactorSlices::NDetail::TSliceNameError& error) {
            // Handle unknown slice names here
            for (auto& nameAndOffsets : error.UnknownSlices) {
                const TSliceOffsets& offsets = nameAndOffsets.second;
                if (borders.IsMinimal(offsets)) {
                    skipSet.insert(offsets);
                }
            }
        } catch (NFactorSlices::NDetail::TDeserializeError&) {
            ythrow TParseFailError()
                << "failed to parse slice borders from \""
                << bordersStr << "\"" << Endl;
        }

        return MakeHolder<TFactorsReader>(borders, std::forward<THolder<TFloatsInput>>(input), skipSet);
    }

    THolder<TFactorsReader> CreateHuffmanReader(const TStringBuf& bordersStr,
        const TStringBuf& data)
    {
        return CreateReader(bordersStr, CreateHuffmanFloatsInput(data));
    }

    THolder<TFactorsReader> CreateRawReader(const TStringBuf& bordersStr,
        const float* buf, size_t count)
    {
        return CreateReader(bordersStr, CreateRawFloatsInput(buf, count));
    }

    THolder<TFactorsReader> CreateStorageReader(const TFactorStorage& storage)
    {
        return MakeHolder<TFactorsReader>(storage.GetBorders(),
            CreateRawFloatsInput(storage.Ptr(0), storage.Size()));
    }

    THolder<TFactorsReader> CreateCompressedFactorsReader(IInputStream* input, TVector<ui8>& buf)
    {
        Y_ASSERT(input);
        if (Y_UNLIKELY(!input)) {
            return nullptr;
        }

        ui8 version = 0;
        input->Read(&version, 1);
        Y_ENSURE(version == 1, "unexpected version " + ToString(version));
        TString bordersStr;
        buf.clear();
        ::Load(input, bordersStr);
        ::Load(input, buf);
        TStringBuf huffStr(reinterpret_cast<char*>(buf.data()), buf.size());
        return CreateHuffmanReader(bordersStr, huffStr);
    }

    bool MoveSlice(TFactorStorage& storage, NFactorSlices::EFactorSlice from, NFactorSlices::EFactorSlice to)
    {
        auto info = NFactorSlices::GetSlicesInfo();

        if (!info->IsLeaf(from) || !info->IsLeaf(to)) {
            return false;
        }

        if (to == from) {
            return true;
        }

        const auto& domain = storage.GetDomain();

        if (domain[from].Size() == 0) {
            return true;
        }

        if (!domain.IsNormal() || domain[to].Size() > 0) {
            return false;
        }

        NFactorSlices::TSlicesMetaInfo newMetaInfo;
        bool ok = NFactorSlices::NDetail::ReConstructMetaInfo(domain.GetBorders(), newMetaInfo);

        Y_ASSERT(ok);

        newMetaInfo.SetNumFactors(to, newMetaInfo.GetNumFactors(from));
        newMetaInfo.SetSliceEnabled(to, true);
        newMetaInfo.SetSliceEnabled(from, false);

        NFactorSlices::TFactorBorders newBorders{newMetaInfo};
        NFactorSlices::TFactorDomain newDomain{newMetaInfo};

        Y_ASSERT(newDomain.Size() == domain.Size());

        TGeneralResizeableFactorStorage::FloatMove(storage.Ptr(domain[from].Begin), storage.Ptr(domain[from].End),
            newDomain[to].Begin - domain[from].Begin);
        storage.SetDomain(newDomain);

        return true;
    }
} // NFSSaveLoad

TVector<float> NFSSaveLoad::TransformFeaturesVectorToSlices(TVector<float> currentFeatures, TStringBuf currentSlices, TStringBuf newSlices) {
    NFactorSlices::TFactorBorders currentBorders;
    NFactorSlices::TFactorBorders desiredBorders;

    NFactorSlices::DeserializeFactorBorders(currentSlices, currentBorders);
    NFactorSlices::DeserializeFactorBorders(newSlices, desiredBorders);

    NFactorSlices::TFactorDomain desiredDomain = NFactorSlices::TFactorDomain(desiredBorders);

    TFactorStorage storage(desiredDomain);
    {
        NFSSaveLoad::TFactorsReader reader(
            currentBorders,
            NFSSaveLoad::CreateRawFloatsInput(currentFeatures.begin(), currentFeatures.size())
        );

        NFactorSlices::TSlicesMetaInfo meta;
        NFactorSlices::NDetail::EnsuredReConstructMetaInfo(
            desiredBorders,
            meta,
            NFactorSlices::NDetail::TReConstructOptions::GetRobust()
        );
        reader.ReadTo(storage, meta, NFSSaveLoad::TFactorsReader::EHostMode::PadAndTruncate);
    }

    auto view = storage.CreateView();
    TVector<float> resultFeatures(view.begin(), view.end());
    return resultFeatures;
}
