#pragma once

#include <util/generic/vector.h>
#include <util/memory/pool.h>
#include <util/system/yassert.h>
#include <util/generic/array_ref.h>

#include "private/utility.h"

namespace NOffroad {
    class TStandardSampler {
    public:
        enum {
            TupleSize = 1,
        };

        TStandardSampler(size_t blockSize, size_t maxChunks = 32768)
            : BlockSize_(blockSize)
            , MaxChunks_(maxChunks)
            , Pool_(1024)
        {
            Y_ASSERT(blockSize > 0);
            Y_ASSERT(maxChunks > 0);
        }

        void Reset() {
            Chunks_.clear();
            Pool_.Clear();
            Counter_ = 0;
            ChunkCount_ = 0;
            IsFinished_ = false;
        }

        size_t BlockSize() const {
            return BlockSize_;
        }

        size_t MaxChunks() const {
            return MaxChunks_;
        }

        void SetMaxChunks(size_t maxChunks) {
            Y_ASSERT(maxChunks > 0);

            MaxChunks_ = maxChunks;

            if (Chunks_.size() > MaxChunks_)
                Chunks_.resize(MaxChunks_);
        }

        template <class Range>
        void Write(size_t channel, const Range& chunk) {
            Write(channel, MakeArrayRef(chunk));
        }

        template <class Unsigned>
        void Write(size_t channel, const TArrayRef<const Unsigned>& chunk) {
            NPrivate::AssertSupportedType<Unsigned>();

            Y_ASSERT(chunk.size() == BlockSize_);
            Y_ASSERT(channel == 0);
            Y_UNUSED(channel);

            ChunkCount_++;
            Counter_ = NPrivate::NextPseudoRandom(Counter_);

            if (Chunks_.size() < MaxChunks_) {
                ui32* local = Pool_.AllocateArray<ui32>(BlockSize_, Alignment);
                CopyChunk(chunk, local);
                Chunks_.push_back(TArrayRef<ui32>(local, BlockSize_));
            } else {
                size_t index = Counter_ % ChunkCount_;
                if (index < Chunks_.size())
                    CopyChunk(chunk, Chunks_[index].data());
            }
        }

        void Finish() {
            IsFinished_ = true;
        }

        bool IsFinished() const {
            return IsFinished_;
        }

        const TVector<TArrayRef<ui32>>& Chunks() const {
            return Chunks_;
        }

    private:
        enum {
            Alignment = 16
        };

        template <class Unsigned>
        void CopyChunk(const TArrayRef<const Unsigned>& src, ui32* dst) {
            if (sizeof(Unsigned) == sizeof(ui32)) {
                memcpy(dst, src.data(), src.size() * sizeof(ui32));
            } else {
                for (const Unsigned& value : src)
                    *dst++ = value;
            }
        }

    private:
        size_t BlockSize_ = 0;
        size_t MaxChunks_ = 0;

        TVector<TArrayRef<ui32>> Chunks_;
        TMemoryPool Pool_;
        ui64 Counter_ = 0;
        size_t ChunkCount_ = 0;
        bool IsFinished_ = false;
    };

}
