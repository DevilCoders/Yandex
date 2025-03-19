#pragma once

#include <kernel/factor_slices/factor_borders.h>
#include <kernel/factor_slices/meta_info.h>

#include <util/generic/set.h>
#include <util/generic/noncopyable.h>

class TFactorStorage;

namespace NFSSaveLoad {
    class TParseFailError: public yexception {};
    class TBorderValuesError: public yexception {};

    class TFloatsInput {
    public:
        virtual ~TFloatsInput() {}

        virtual bool IsFinished() const = 0;

        size_t Load(float* dst, size_t n) {
            const size_t loaded = DoLoad(dst, n);
            LoadedCount += loaded;
            return loaded;
        }
        size_t Skip(size_t n) {
            const size_t skipped = DoSkip(n);
            SkippedCount += skipped;
            return skipped;
        }

        size_t GetLoadedCount() const {
            return LoadedCount;
        }
        size_t GetSkippedCount() const {
            return SkippedCount;
        }

    protected:
        virtual size_t DoLoad(float* dst, size_t n) = 0;
        virtual size_t DoSkip(size_t n) = 0;

    private:
        size_t LoadedCount = 0;
        size_t SkippedCount = 0;
    };

    THolder<TFloatsInput> CreateHuffmanFloatsInput(const TStringBuf& data);
    THolder<TFloatsInput> CreateRawFloatsInput(const float* buf, size_t count);

    using TFactorsSkipSet = TSet<NFactorSlices::TSliceOffsets>;

    // O(SkipCount * SliceCount)
    void RemoveSkippedFactors(NFactorSlices::TFactorBorders& borders, const TFactorsSkipSet& skipSet);

    class TSkipFactorsInput
        : public TNonCopyable
    {
    public:
        TSkipFactorsInput(THolder<TFloatsInput>&& input,
            const TFactorsSkipSet& skipSet = TFactorsSkipSet());

        void RemoveFactors(const NFactorSlices::TSliceOffsets& offsets);
        const TFactorsSkipSet& GetSkipSet() const;

        size_t Load(float* buf, size_t count);
        void Skip(size_t count);
        bool CheckFinished();

        size_t GetLoadedCount() const;
        size_t GetSkippedCount() const;

    private:
        NFactorSlices::TFactorIndex DoSkipFactors();

    private:
        THolder<TFloatsInput> Input = nullptr;

        TFactorsSkipSet SkipSet;
        TFactorsSkipSet::const_iterator SkipIter;
    };

    class TFactorsReader {
    public:
        enum class EHostMode {
            PadOnly,        // Fill missing values with zeroes
            PadAndTruncate  // Pad with zeroes, truncate extra values
        };

    public:
        TFactorsReader(const NFactorSlices::TFactorBorders& borders,
            THolder<TFloatsInput>&& input,
            const TFactorsSkipSet& skipSet = TFactorsSkipSet());

        void RemoveFactors(const NFactorSlices::TSliceOffsets& offsets);
        void RemoveSlice(NFactorSlices::EFactorSlice slice);
        void TruncateSlice(NFactorSlices::EFactorSlice slice, size_t bound);

        const NFactorSlices::TFactorBorders& GetBorders() const;
        void PrepareModifiedBorders(NFactorSlices::TFactorBorders& borders) const;

        void ReadTo(TVector<float>& buf);
        void ReadTo(TFactorStorage& storage,
            const NFactorSlices::TSlicesMetaInfo& hostInfo = NFactorSlices::TSlicesMetaInfo(),
            EHostMode mode = EHostMode::PadOnly);

    private:
        TSkipFactorsInput Input;
        NFactorSlices::TFactorBorders Borders;
    };

    THolder<TFactorsReader> CreateReader(const TStringBuf& bordersStr, THolder<TFloatsInput>&& input);
    THolder<TFactorsReader> CreateHuffmanReader(const TStringBuf& bordersStr, const TStringBuf& data);
    THolder<TFactorsReader> CreateRawReader(const TStringBuf& bordersStr, const float* buf, size_t count);
    THolder<TFactorsReader> CreateStorageReader(const TFactorStorage& storage);

    THolder<TFactorsReader> CreateCompressedFactorsReader(IInputStream* input, TVector<ui8>& buf);

    // Convenience function that modifies storage in-place by moving contents of one slice
    // into another. Destination slice is expected to be empty.
    // May return false without changing storage.
    bool MoveSlice(TFactorStorage& storage, NFactorSlices::EFactorSlice from, NFactorSlices::EFactorSlice to);

    TVector<float> TransformFeaturesVectorToSlices(TVector<float> currentFeatures, TStringBuf currentSlices, TStringBuf newSlices);
} // NFSSaveLoad
