#pragma once

#include <nginx/modules/strm_packager/src/base/workers.h>

#include <util/generic/array_ref.h>
#include <util/generic/buffer.h>

namespace NStrm::NPackager::NAvcCBox {
    ui8 GetNaluSizeLength(const TBuffer& avcC);

    void SetNaluSizeLength(TBuffer& avcC, int newSizeLength);

    struct TSpsPpsRange {
        TVector<TSimpleBlob> Sps;
        TVector<TSimpleBlob> Pps;
    };

    TSpsPpsRange GetSpsPps(const TBuffer& avcC);
}
