#pragma once

#include <library/cpp/on_disk/coded_blob/common/coded_blob_utils.h>

#include <library/cpp/codecs/codecs.h>

#include <util/generic/ymath.h>
#include <util/memory/pool.h>

namespace NCodedBlob {
    struct TCodedBlobBuilderOpts {
        TString Codec = "solar-16:huffman";
    };

    class TCodedBlobSimpleBuilder {
    public:
        TCodedBlobSimpleBuilder(size_t sz = 1 << 19, TStringBuf tempDataCodec = "snappy")
            : Pool(sz, TMemoryPool::TLinearGrow::Instance())
            , Codec(NCodecs::ICodec::GetInstance("solar-16k:huffman"))
            , TempCodec(tempDataCodec)
        {
        }

        virtual ~TCodedBlobSimpleBuilder() {
        }

        virtual void Init(TStringBuf codec) {
            Pool.ClearKeepFirstChunk();
            Begins.clear();
            Lengths.clear();
            Codec = NCodecs::ICodec::GetInstance(codec);
        }

        virtual void InitBuilder(const TCodedBlobBuilderOpts& opts) {
            Init(opts.Codec);
        }

        void Add(TStringBuf data) {
            data = TempCodec.Compress(data);
            Begins.push_back(Pool.Append(data.data(), data.size()));
            Lengths.push_back(data.size());
        }

        void Finish(IOutputStream* out);
        TBlob Finish();

    protected:
        virtual void DoFinish(IOutputStream* out);

    protected:
        TMemoryPool Pool;

        NCodecs::TCodecPtr Codec;
        TVector<char*> Begins;
        TVector<ui32> Lengths;
        NUtils::TTempDataCodec TempCodec;
    };

    class TCodedBlobArraySimpleBuilder: public TCodedBlobSimpleBuilder {
    public:
        TCodedBlobArraySimpleBuilder(size_t sz = 1 << 19, TStringBuf tempDataCodec = "snappy")
            : TCodedBlobSimpleBuilder(sz, tempDataCodec)
        {
        }

    protected:
        void DoFinish(IOutputStream* out) override;
    };

    class TCodedBlobTrieSimpleBuilder: public TCodedBlobSimpleBuilder {
    public:
        TCodedBlobTrieSimpleBuilder(size_t sz = 1 << 19, bool unsorted = false, TStringBuf tempDataCodec = "snappy")
            : TCodedBlobSimpleBuilder(sz, tempDataCodec)
            , Unsorted(unsorted)
        {
        }

        void SetUnsorted(bool unsorted) {
            Unsorted = unsorted;
        }

        void Init(TStringBuf codec) override {
            TCodedBlobSimpleBuilder::Init(codec);
            KeyLengths.clear();
        }

        template <class TKeyIter>
        void Add(TKeyIter keysBegin, TKeyIter keysEnd, TStringBuf data) {
            KeyAndDataBuffer.Clear();

            const ui32 keysSize = PackKeys(keysBegin, keysEnd);

            Y_ENSURE_EX(keysSize, TCodedBlobException() << "No keys");

            KeyAndDataBuffer.Append(data.data(), data.size());

            TCodedBlobSimpleBuilder::Add(TStringBuf(KeyAndDataBuffer.data(), KeyAndDataBuffer.size()));

            KeyLengths.push_back(keysSize);
        }

        void Add(TStringBuf key, TStringBuf data) {
            const TStringBuf* keysBegin = &key;
            const TStringBuf* keysEnd = keysBegin + 1;
            Add(keysBegin, keysEnd, data);
        }

    protected:
        void DoFinish(IOutputStream* out) override;

        size_t PackKey(const TStringBuf& key);

        template <class TKeyIter>
        ui32 PackKeys(TKeyIter keysBegin, TKeyIter keysEnd) {
            size_t size = 0;

            for (TKeyIter i = keysBegin; i != keysEnd; ++i) {
                TStringBuf key = *i;
                size += PackKey(key);
            }

            Y_ENSURE_EX(size == static_cast<ui32>(size), TCodedBlobException() << "the size of keys is greater than ui32");

            return size;
        }

    protected:
        TBuffer KeyAndDataBuffer;
        TVector<ui32> KeyLengths;
        bool Unsorted = false;
    };

}
