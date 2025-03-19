#pragma once

#include <kernel/compressed_blob/compressed_blob.h>

#include <kernel/tarc/repack/repack.h>

class TWadTextArchiveRepackedBlob : public TCompressedBlob {
public:
    TWadTextArchiveRepackedBlob(TBlob&& blob, NRepack::TCodec& codec)
        : TCompressedBlob(std::move(blob))
        , Codec_(codec)
    {}

    ~TWadTextArchiveRepackedBlob() override = default;

    TBlob UncompressBlob() const override {
        if (Blob_.Empty()) {
            return Blob_;
        }
        return NRepack::RestoreArchiveDocText(Blob_, Codec_);
    }

protected:
    NRepack::TCodec& Codec_;
};
