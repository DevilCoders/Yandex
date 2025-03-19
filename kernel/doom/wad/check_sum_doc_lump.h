#pragma once

#include <library/cpp/digest/crc32c/crc32c.h>

#include <util/memory/blob.h>
#include <util/generic/fwd.h>
#include <util/generic/maybe.h>

#include "mapper.h"
#include "wad_lump_id.h"


namespace NDoom {

constexpr TWadLumpId CheckSumDocLumpId = { EWadIndexType::CheckSumIndexType, EWadLumpRole::Struct };

class TCrcExtendCalcer {
public:
    using TCheckSum = ui32;

public:
    TCheckSum operator()(TCheckSum init, const void* buf, size_t len) {
        return Crc32cExtend(init, buf, len);
    }
};

TMaybe<TCrcExtendCalcer::TCheckSum> CalcBlobCheckSum(const TBlob& documentBlob, TConstArrayRef<char> checkSumRegion);
TMaybe<TCrcExtendCalcer::TCheckSum> FetchCheckSum(TConstArrayRef<char> checkSumRegion);

template<typename BaseMapper = NDoom::IDocLumpMapper>
class TCheckingMapper: public NDoom::IDocLumpMapper {
public:
    TCheckingMapper() = default;

    TCheckingMapper(BaseMapper* mapper) {
        Reset(mapper);
    }

    void Reset(BaseMapper* mapper) {
        Mapper_ = mapper;
        auto lumps = Mapper_->DocLumps();
        for (size_t i = 0; i < lumps.size(); ++i) {
            if (lumps[i] == CheckSumDocLumpId) {
                size_t idx;
                Mapper_->MapDocLumps({ CheckSumDocLumpId }, { &idx, 1 });
                CheckSumDocLumpIdx_ = idx;
            }
        }
    }

    template<typename BaseLoader>
    bool Validate(const TBlob& docBlob, const BaseLoader& loader) const {
        if (!CheckSumDocLumpIdx_) {
            return true;
        } else {
            std::array<TConstArrayRef<char>, 1> lumps;
            loader.LoadDocRegions({ *CheckSumDocLumpIdx_ }, lumps);
            auto checkSum = FetchCheckSum(lumps[0]);
            return (checkSum && checkSum == CalcBlobCheckSum(docBlob, lumps[0]));
        }
    }

    TVector<TWadLumpId> DocLumps() const override {
        TVector<TWadLumpId> lumps = Mapper_->DocLumps();
        size_t newSz = 0;
        for (size_t i = 0; i < lumps.size(); ++i) {
            if (lumps[i] != CheckSumDocLumpId) {
                lumps[newSz++] = lumps[i];
            }
        }
        lumps.resize(newSz);
        return lumps;
    }

    void MapDocLumps(const TArrayRef<const NDoom::TWadLumpId>& ids, TArrayRef<size_t> mapping) const override {
        Mapper_->MapDocLumps(ids, mapping);
    }

private:
    BaseMapper* Mapper_ = nullptr;
    TMaybe<size_t> CheckSumDocLumpIdx_;
};

} // namespace NDoom
