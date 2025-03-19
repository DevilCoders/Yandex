#pragma once

#include "codec.h"
#include <util/generic/maybe.h>

namespace NDoom {

template<int DataParts, int ParityParts>
class TLrc final: public NErasure::TLrcJerasure<DataParts, ParityParts, 8, NErasure::TDefaultCodecTraits>, public NDoom::ICodec {
private:
    using TBase = NErasure::TLrcJerasure<DataParts, ParityParts, 8, NErasure::TDefaultCodecTraits>;
    using TPartIndexList = NErasure::TPartIndexList;

    static constexpr int WordSize = 8;

public:
    TLrc() = default;

    using TBase::CanRepair;
    using TBase::GetRepairIndices;
    using TBase::Decode;

    static constexpr size_t TotalPartCount = DataParts + ParityParts;
    static constexpr size_t DataPartCount = DataParts;
    static constexpr size_t ParityPartCount = ParityParts;

    std::vector<TBlob> Encode(const std::vector<TBlob>& blocks) const override {
        return TBase::Encode(blocks);
    }

    bool CanRepair(int target, const TPartIndexList& availableIndices) const override {
        if (NErasure::Contains(availableIndices, target)) {
            return true;
        }

        for (size_t i = 0; i < 2; ++i) {
            if (NErasure::Contains(TBase::GetXorGroups()[i], target)) {
                if (NErasure::Intersection(TBase::GetXorGroups()[i], availableIndices).size() == TBase::GetXorGroups()[i].size() - 1) {
                    return true;
                }
            }
        }

        return TBase::CanRepair(NErasure::Difference(0, DataParts + ParityParts, availableIndices));
    }

    std::optional<TPartIndexList> GetRepairIndices(int target, const TPartIndexList& erasedIndices) const override {
        TPartIndexList indices = NErasure::UniqueSortedIndices(erasedIndices);

        if (!NErasure::Contains(indices, target)) {
            return TPartIndexList{target};
        }

        for (size_t i = 0; i < 2; ++i) {
            if (NErasure::Contains(TBase::GetXorGroups()[i], target) && NErasure::Intersection(TBase::GetXorGroups()[i], indices).size() == 1) {
                return NErasure::Difference(TBase::GetXorGroups()[i], target);
            }
        }

        // In this case we have to invert matrix for decoding, so all parts should be repairable.
        if (CanRepair(indices)) {
            return NErasure::Difference(0, DataParts + ParityParts, indices);
        } else {
            return std::nullopt;
        }
    }

    TMaybe<ui64> EstimateRequestMultiplication(int target, const TPartIndexList& erasedIndices) override {
        if (auto indices = GetRepairIndices(target, erasedIndices)) {
            return indices->size();
        }
        return Nothing();
    }

    TBlob Decode(
        int target,
        const std::vector<TBlob>& blocks,
        const TPartIndexList& blockIndices) const override
    {
        auto iter = std::find(blockIndices.begin(), blockIndices.end(), target);
        if (iter != blockIndices.end()) {
            return blocks[iter - blockIndices.begin()];
        }

        Y_ENSURE(target < DataParts);
        Y_ENSURE(blocks.size() == blockIndices.size());

        std::vector<std::optional<int>> blockMapping(DataParts + ParityParts);
        for (size_t i = 0; i < blockIndices.size(); ++i) {
            blockMapping[blockIndices[i]] = i;
        }

        for (auto& group : TBase::GetXorGroups()) {
            if (NErasure::Contains(group, target)) {
                size_t coverage = 0;
                for (int index: group) {
                    if (blockMapping[index].has_value()) {
                        ++coverage;
                    }
                }
                if (coverage == group.size() - 1) {
                    std::vector<TBlob> groupBlobs;
                    for (int index: group) {
                        if (blockMapping[index].has_value()) {
                            groupBlobs.push_back(blocks[blockMapping[index].value()]);
                        }
                    }
                    return NErasure::Xor<NErasure::TDefaultCodecTraits>(groupBlobs);
                }
            }
        }

        // here we should be able to decode all blocks

        TPartIndexList erasures;
        for (int i = 0; i < TBase::GetTotalPartCount(); ++i) {
            if (!blockMapping[i].has_value()) {
                erasures.push_back(i);
            }
        }
        auto repair = TBase::GetRepairIndices(erasures);
        Y_ENSURE(repair.has_value());
        std::vector<TBlob> blobs;
        for (size_t i = 0; i < repair->size(); ++i) {
            blobs.push_back(blocks[blockMapping[(*repair)[i]].value()]);
        }
        std::vector<TBlob> repaired = TBase::Decode(blobs, erasures);
        for (size_t i = 0; i < erasures.size(); ++i) {
            if (erasures[i] == target) {
                return repaired[i];
            }
        }

        Y_ENSURE(false);
    }

    int GetTotalPartCount() const override {
        return TBase::GetTotalPartCount();
    }

    int GetParityPartCount() const override {
        return TBase::GetParityPartCount();
    }

    int GetDataPartCount() const override {
        return TBase::GetDataPartCount();
    }

    int GetWordSize() const override {
        return TBase::GetWordSize();
    }
};

} // namespace NDoom
