#pragma once

#include <library/cpp/codecs/codecs.h>

#include <util/generic/buffer.h>
#include <util/generic/deque.h>
#include <util/generic/maybe.h>
#include <util/generic/vector.h>
#include <util/memory/blob.h>

#include <functional>

namespace NRTYArchive {
    // readonly vector of blobs
    struct TBlobVector {
        explicit TBlobVector(const NCodecs::ICodec&);
        TBlobVector(const NCodecs::ICodec&, const TBlob& packedBlob);
        void LoadFromBlob(const TBlob& packedBlob);

        size_t GetCount() const;

        TBlob GetCopy(size_t index) const;

        // create blob from |UnpackBuffer| and return its SubBlob
        // |UnpackBuffer| will give away its storage
        TBlob Take(size_t index);

    private:
        const NCodecs::ICodec& Codec;
        TBuffer UnpackBuffer;
        TVector<ui32> Offsets;
        size_t UpperBound = 0;
    };

    // structure for constructing serialized TBlobVector
    struct TBlobVectorBuilder {
        TBlobVectorBuilder(const NCodecs::ICodec& codec, size_t sizeLimit, size_t countLimit);

        bool Empty() const;
        size_t GetCount() const;
        bool IsFull() const;
        void Clear();

        TBlob GetCopy(size_t index) const;
        size_t Put(const TBlob&);

        void SaveToBuffer(TBuffer& out) const;

    private:
        const NCodecs::ICodec& Codec;
        const size_t SizeLimit;
        const size_t CountLimit;
        mutable TBuffer Buffer;
        TVector<ui32> Offsets;
        size_t Count = 0;
    };

    struct TFirstBlock {
        explicit TFirstBlock(const TBlob& blob);

        NCodecs::TCodecPtr GetCodec() const;
        TBlob GetBlobCopy(size_t index) const;
        size_t GetCount() const;

    private:
        NCodecs::TCodecPtr Codec;
        TMaybe<TBlobVector> Blobs;
    };

    struct TCodecLearnQueue {
        // zstd may fail to learn on small data
        static const size_t MinimalLearnSize;
        TCodecLearnQueue(NCodecs::TCodecPtr codec, size_t learnSize, size_t countLimit);

        TBlob GetBlobCopy(size_t index) const;
        size_t PutBlob(const TBlob&);

        bool IsFull() const;
        size_t GetCount() const;

        // returns trained codec or fallback codec (currently LZ4) if learn data is too small
        NCodecs::TCodecPtr TrainCodec(TBuffer& out);

    private:
        const size_t LearnSize;
        const size_t CountLimit;

        NCodecs::TCodecPtr Codec;
        TDeque<TBlob> Blobs;
        size_t TotalSize = 0;
    };
}

