#pragma once

#include <library/cpp/erasure/codec.h>
#include <util/generic/maybe.h>

namespace NDoom {

class ICodec {
public:
    using TPartIndexList = NErasure::TPartIndexList;

public:
    virtual std::vector<TBlob> Encode(const std::vector<TBlob>& blocks) const = 0;

    virtual bool CanRepair(int target, const TPartIndexList& availableIndices) const = 0;

    virtual TBlob Decode(int target, const std::vector<TBlob>& blocks, const TPartIndexList& blockIndices) const = 0;

    virtual std::optional<TPartIndexList> GetRepairIndices(int target, const TPartIndexList& erasedIndices) const = 0;

    virtual TMaybe<ui64> EstimateRequestMultiplication(int, const TPartIndexList&) {
        return 1;
    }

    virtual int GetTotalPartCount() const = 0;

    virtual int GetParityPartCount() const = 0;

    virtual int GetDataPartCount() const = 0;

    virtual int GetWordSize() const = 0;

    virtual ~ICodec() = default;
};


} // namespace NDoom
