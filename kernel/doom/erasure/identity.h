#pragma once

#include "codec.h"

namespace NDoom {

template<int DataParts, int WordSize = 8>
class TIdentityCodec : public NDoom::ICodec {
public:
    std::vector<TBlob> Encode(const std::vector<TBlob>&) const override {
        return {};
    }

    bool CanRepair(int target, const TPartIndexList& availableIndices) const override {
        return NErasure::Contains(availableIndices, target);
    }

    TBlob Decode(int target, const std::vector<TBlob>& blocks, const TPartIndexList& blockIndices) const override {
        for (size_t i = 0; i < blockIndices.size(); ++i) {
            if (blockIndices[i] == target) {
                return blocks[i];
            }
        }
        Y_ENSURE(false);
    }

    std::optional<TPartIndexList> GetRepairIndices(int target, const TPartIndexList& erasedIndices) const override {
        if (NErasure::Contains(erasedIndices, target)) {
            return std::nullopt;
        } else {
            return TPartIndexList{target};
        }
    }

    int GetTotalPartCount() const override {
        return GetDataPartCount() + GetParityPartCount();
    }

    int GetParityPartCount() const override {
        return 0;
    }

    int GetDataPartCount() const override {
        return DataParts;
    }

    int GetWordSize() const override {
        return WordSize * sizeof(long);
    }
};

} // namespace NDoom
