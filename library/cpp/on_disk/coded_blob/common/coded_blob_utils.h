#pragma once

#include <library/cpp/packers/packers.h>
#include <library/cpp/codecs/codecs.h>

#include <contrib/libs/snappy/snappy.h>

#include <util/generic/buffer.h>
#include <util/generic/strbuf.h>

#include <util/memory/blob.h>

#include <util/stream/mem.h>

#include <util/ysaveload.h>

namespace NCodedBlob {
    class TCodedBlobException: public yexception {};

    namespace NUtils {
        const ui64 INVALID_OFFSET = -1;

        using TLengthPacker = NPackers::TIntegralPacker<ui64>;

        inline TStringBuf PrepareCompressedValueForBlob(TStringBuf codedval, TBuffer& out) {
            char lenbuf[16];
            TLengthPacker packer;

            ui32 plen = packer.MeasureLeaf(codedval.size());
            packer.PackLeaf(lenbuf, codedval.size(), plen);
            out.Reserve(plen + codedval.size());
            out.Assign(lenbuf, plen);
            out.Append(codedval.data(), codedval.size());

            return TStringBuf(out.data(), out.size());
        }

        inline TStringBuf CompressValueForBlob(const NCodecs::TCodecPtr& codec, TStringBuf rawval, TBuffer& out) {
            if (!!codec) {
                out.Clear();
                codec->Encode(rawval, out);
                rawval = TStringBuf(out.data(), out.size());
            }

            return rawval;
        }

        inline TStringBuf PrepareValueForBlob(const NCodecs::TCodecPtr& codec, TStringBuf rawval, TBuffer& out, TBuffer& tmp) {
            return PrepareCompressedValueForBlob(CompressValueForBlob(codec, rawval, tmp), out);
        }

        struct TKeyTag {
            typedef TString TBuf;
        };

        struct TValueTag {
            typedef TBuffer TBuf;
        };

        template <typename TTag>
        struct TCachedHelper {
            TCachedHelper()
                : HasCached()
            {
            }

            template <typename TPartner>
            TStringBuf GetCurrent(const TPartner* partner) const {
                if (!HasCached) {
                    HasCached = true;
                    Cached = partner->FetchCurrent(TTag(), CachedBuffer);
                }

                return Cached;
            }

            void Invalidate() {
                HasCached = false;
            }

        private:
            mutable TStringBuf Cached;
            mutable typename TTag::TBuf CachedBuffer;
            mutable bool HasCached;
        };

        using TKeyHelper = TCachedHelper<TKeyTag>;
        using TValueHelper = TCachedHelper<TValueTag>;

        struct TTempDataCodec {
            enum ECodecType {
                CT_NONE = 0,
                CT_SNAPPY
            };

            TTempDataCodec(TStringBuf name) {
                if ("snappy" == name) {
                    Codec = CT_SNAPPY;
                }
            }

            TTempDataCodec() = default;

            TStringBuf Compress(TStringBuf data) {
                if (CT_SNAPPY == Codec) {
                    Buffer.Resize(snappy::MaxCompressedLength(data.size()));
                    size_t reslen = 0;
                    snappy::RawCompress(data.data(), data.size(), Buffer.Data(), &reslen);
                    data = TStringBuf(Buffer.data(), reslen);
                }

                return data;
            }

            TStringBuf Decompress(TStringBuf data) {
                if (CT_SNAPPY == Codec) {
                    size_t reslen = 0;
                    snappy::GetUncompressedLength(data.data(), data.size(), &reslen);
                    Buffer.Resize(reslen);
                    Y_VERIFY(snappy::RawUncompress(data.data(), data.size(), Buffer.Data()), " invalid compression");
                    data = TStringBuf(Buffer.data(), Buffer.size());
                }

                return data;
            }

            TStringBuf Decompress(const char* data, size_t size) {
                return Decompress(TStringBuf(data, size));
            }

        private:
            TBuffer Buffer;
            ECodecType Codec = CT_NONE;
        };

        const ui32 CODED_BLOB_MAGIC_SIZE = 8;

        ui64 DoReadHeader(TBlob&, const char* magic, ui64 maxversion);

        TBlob DoSkipBody(TBlob&);

        void DoWriteHeader(IOutputStream*, const char* magic, ui64 version);

        void DoWriteWithFooter(IOutputStream*, const char* begin, ui64 size);

        void MoveToRam(TBlob& mmapped);

        void SetRandomAccessed(TBlob mmapped);

    }
}
