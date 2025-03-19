#pragma once

#include <util/generic/fwd.h>
#include <util/memory/blob.h>
#include <util/generic/noncopyable.h>

class TCompressedBlob : public TNonCopyable {
public:
    TCompressedBlob() {}

    TCompressedBlob(const TBlob& blob)
        : Blob_(blob)
    {}

    TCompressedBlob(TBlob&& blob)
        : Blob_(std::move(blob))
    {}

    virtual ~TCompressedBlob() {}

    virtual TBlob UncompressBlob() const {
        return Blob_;
    }

protected:
    TBlob Blob_;
};
