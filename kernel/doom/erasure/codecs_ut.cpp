#include "lrc.h"

#include <library/cpp/testing/unittest/registar.h>

#include <util/random/random.h>

std::vector<char> GenData(size_t bufferSize) {
    std::vector<char> data(bufferSize);
    for (size_t i = 0; i < bufferSize; ++i) {
        data[i] = RandomNumber(256u);
    }
    return data;
}

template<typename Codec>
void CheckDecodeOne(size_t len) {
    Codec codec;
    std::vector<std::vector<char>> data;
    std::vector<TBlob> blobs;
    for (int i = 0; i < codec.GetDataPartCount(); ++i) {
        data.push_back(GenData(len));
        blobs.push_back(TBlob::NoCopy(data.back().data(), data.back().size()));
    }
    std::vector<TBlob> parity = codec.Encode(blobs);
    for (size_t i = 0; i < parity.size(); i++) {
        blobs.push_back(parity[i]);
    }

    for (size_t availableMask = 0; availableMask < (1 << codec.GetTotalPartCount()); ++availableMask) {
        for (int target = 0; target < codec.GetDataPartCount(); ++target) {
            NErasure::TPartIndexList available;
            NErasure::TPartIndexList erasures;
            for (int i = 0; i < codec.GetTotalPartCount(); ++i) {
                if ((1 << i) & availableMask) {
                    available.push_back(i);
                } else {
                    erasures.push_back(i);
                }
            }
            auto repairIndices = codec.GetRepairIndices(target, erasures);
            if (!repairIndices.has_value()) {
                UNIT_ASSERT(!codec.CanRepair(erasures));
                UNIT_ASSERT(!codec.CanRepair(target, available));
                continue;
            }
            UNIT_ASSERT(codec.CanRepair(target, available));
            std::vector<TBlob> repairBlobs;
            for (auto i: repairIndices.value()) {
                UNIT_ASSERT((1 << i) & availableMask);
                repairBlobs.push_back(blobs[i]);
            }
            auto repaired = codec.Decode(target, repairBlobs, repairIndices.value());
            UNIT_ASSERT(repaired.Size() == blobs[target].Size());
            UNIT_ASSERT(memcmp(repaired.AsCharPtr(), blobs[target].AsCharPtr(), repaired.Size()) == 0);
        }
    }
}

Y_UNIT_TEST_SUITE(TestRandomText) {
    Y_UNIT_TEST(TestDecodeOneLrc) {
        CheckDecodeOne<NDoom::TLrc<12, 4>>(4096);
    }
}
