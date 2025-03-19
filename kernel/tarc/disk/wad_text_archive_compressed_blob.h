#pragma once

#include <util/stream/mem.h>
#include <util/stream/zlib.h>
#include <util/generic/maybe.h>
#include <util/generic/vector.h>
#include <util/generic/string.h>

#include <kernel/compressed_blob/compressed_blob.h>
#include <kernel/extarc_compression/compressor.h>

class TWadTextArchiveCompressedBlob : public TCompressedBlob {
public:
    TWadTextArchiveCompressedBlob(const TBlob& blob, bool isCompressed)
        : TCompressedBlob(blob)
        , IsCompressed_(isCompressed)
    {}

    TWadTextArchiveCompressedBlob(TBlob&& blob, bool isCompressed)
        : TCompressedBlob(std::move(blob))
        , IsCompressed_(isCompressed)
    {}

    TWadTextArchiveCompressedBlob(const TBlob& blob, bool isCompressed, bool isExtArc, const TMaybe<TVector<TString>>& compressKeys)
        : TCompressedBlob(blob)
        , IsCompressed_(isCompressed)
        , IsExtArc_(isExtArc)
        , CompressKeys_(compressKeys)
    {}

    TWadTextArchiveCompressedBlob(TBlob&& blob, bool isCompressed, bool isExtArc, const TMaybe<TVector<TString>>& compressKeys)
        : TCompressedBlob(std::move(blob))
        , IsCompressed_(isCompressed)
        , IsExtArc_(isExtArc)
        , CompressKeys_(compressKeys)
    {}

    ~TWadTextArchiveCompressedBlob() override = default;

    TBlob UncompressBlob() const override {
        if (Blob_.Size() == 0) {
            return Blob_;
        }

        if (CompressKeys_) {
            Y_ENSURE(IsExtArc_);
            return DecompressExtArc(Blob_, *CompressKeys_);
        }

        if (!IsCompressed_) {
            return Blob_;
        }

        TMemoryInput input(Blob_.Data(), Blob_.Size());
        TZLibDecompress decompressor((IInputStream*)&input);
        TBlob uncompressedBlob = TBlob::FromStream(decompressor);
        return uncompressedBlob;
    }

protected:
    bool IsCompressed_ = false;
    bool IsExtArc_ = false;
    TMaybe<TVector<TString>> CompressKeys_ = Nothing();
};
