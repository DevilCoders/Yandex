#include "coded_blob_simple_builder.h"

#include "coded_blob_array_builder.h"
#include "coded_blob_trie_builder.h"

#include <util/stream/buffer.h>

namespace NCodedBlob {
    struct TBLIter {
        TBLIter(const TVector<char*>& b, const TVector<ui32>& lens, ui64 off, NUtils::TTempDataCodec& tmpcodec,
                const TVector<ui32>* skips = nullptr)
            : Begins(b)
            , Lengths(lens)
            , Skips(skips)
            , Offset(off)
            , TempCodec(tmpcodec)
        {
            Y_VERIFY(b.size() == lens.size(), " invalid mapping");
            Y_VERIFY(!skips || skips->size() == lens.size(), " invalid mapping");
        }

        TBLIter& operator++() {
            ++Offset;
            return *this;
        }

        i64 operator-(const TBLIter& it) const {
            return (i64)Offset - (i64)it.Offset;
        }

        TStringBuf operator*() const {
            Y_VERIFY(Offset < Begins.size(), " invalid offset");
            TStringBuf raw = TempCodec.Decompress(TStringBuf(Begins[Offset], Lengths[Offset]));
            TStringBuf res = Skips ? raw.Tail((*Skips)[Offset]) : raw;
            return res;
        }

        bool operator==(const TBLIter& it) const {
            return Offset == it.Offset;
        }

        bool operator!=(const TBLIter& it) const {
            return !(*this == it);
        }

        const TVector<char*>& Begins;
        const TVector<ui32>& Lengths;
        const TVector<ui32>* Skips = nullptr;
        ui64 Offset = -1;
        NUtils::TTempDataCodec& TempCodec;
    };

    void TCodedBlobSimpleBuilder::Finish(IOutputStream* out) {
        DoFinish(out);
    }

    TBlob TCodedBlobSimpleBuilder::Finish() {
        TBufferOutput bout;
        DoFinish(&bout);
        return TBlob::FromBuffer(bout.Buffer());
    }

    template <typename TBuilder>
    static void DoFinishBuilder(IOutputStream* out,
                                NCodecs::TCodecPtr codec,
                                const TVector<char*>& begins,
                                const TVector<ui32>& lengths,
                                NUtils::TTempDataCodec& tempcodec) {
        if (!!codec) {
            codec->Learn(TBLIter(begins, lengths, 0, tempcodec),
                         TBLIter(begins, lengths, begins.size(), tempcodec));
        }

        TBuilder builder;
        builder.Init(out, codec);
        TBuffer tmp;

        for (ui64 i = 0, sz = begins.size(); i < sz; ++i) {
            builder.AddCompressed(NUtils::CompressValueForBlob(codec, tempcodec.Decompress(begins[i], lengths[i]), tmp));
        }

        builder.Finish();
    }

    void TCodedBlobSimpleBuilder::DoFinish(IOutputStream* out) {
        DoFinishBuilder<TCodedBlobBuilder>(out, Codec, Begins, Lengths, TempCodec);
    }

    void TCodedBlobArraySimpleBuilder::DoFinish(IOutputStream* out) {
        DoFinishBuilder<TCodedBlobArrayBuilder>(out, Codec, Begins, Lengths, TempCodec);
    }

    static void RetrieveKeys(TStringBuf packedKeys, TVector<TStringBuf>& keys) {
        keys.clear();
        NUtils::TLengthPacker packer;
        while (packedKeys) {
            ui64 len = 0;
            packer.UnpackLeaf(packedKeys.data(), len);

            const size_t packedLen = packer.SkipLeaf(packedKeys.data());
            Y_VERIFY(packedLen <= packedKeys.size());
            packedKeys.Skip(packedLen);

            Y_VERIFY(len <= packedKeys.size());
            keys.push_back(TStringBuf(packedKeys.data(), len));
            packedKeys.Skip(len);
        }
    }

    void TCodedBlobTrieSimpleBuilder::DoFinish(IOutputStream* out) {
        if (!!Codec) {
            Codec->Learn(TBLIter(Begins, Lengths, 0, TempCodec, &KeyLengths),
                         TBLIter(Begins, Lengths, Begins.size(), TempCodec, &KeyLengths));
        }

        TCodedBlobTrieBuilder builder(Unsorted);
        builder.Init(out, Codec);
        TBuffer tmp;

        TVector<TStringBuf> keys;
        for (ui64 i = 0, sz = Begins.size(); i < sz; ++i) {
            TStringBuf keyAndVal = TempCodec.Decompress(Begins[i], Lengths[i]);
            TStringBuf codedKeys = keyAndVal.Head(KeyLengths[i]);
            TStringBuf val = keyAndVal.Tail(KeyLengths[i]);

            RetrieveKeys(codedKeys, keys);
            builder.AddCompressed(keys.begin(), keys.end(), NUtils::CompressValueForBlob(Codec, val, tmp));
        }

        builder.Finish();
    }

    size_t TCodedBlobTrieSimpleBuilder::PackKey(const TStringBuf& key) {
        char lenbuf[16];
        NUtils::TLengthPacker packer;
        size_t len = packer.MeasureLeaf(key.size());
        packer.PackLeaf(lenbuf, key.size(), len);

        KeyAndDataBuffer.Append(lenbuf, len);
        KeyAndDataBuffer.Append(key.data(), key.size());
        return key.size() + len;
    }

}
