#include "sentence_lengths_coder_data.h"

#include <util/memory/blob.h>

namespace NDataVersion2 {
    #include "sent_lens_data_hashes2.inc"

    TPackedOffsets::TPackedOffsets() {
        size_t size = sizeof(NDataVersion2::LENGTHS_OFFSETS) / sizeof(NDataVersion2::LENGTHS_OFFSETS[0]);

        Offsets.Reserve(size);
        for (size_t i = 0; i < size; ++i) {
            Offsets[i] = NDataVersion2::LENGTHS_OFFSETS[i].Len + (NDataVersion2::LENGTHS_OFFSETS[i].Index << 8);
        }
        Offsets.SetInitializedSize(size);
    }

    TPackedOffsets PackedOffsets;
};

size_t TSentenceLengthsCoderData::GetNumBytesForHashVersion2() {
    return NDataVersion2::LENGTHS_NUMBYTESFORHASH;
}

size_t TSentenceLengthsCoderData::GetBlockCount() {
    return Y_ARRAY_SIZE(NDataVersion2::LENGTHS_OFFSETS);
}

void TSentenceLengthsCoderData::GetBlockVersion2(ui16 index, TSentenceLengths* result) {
    const size_t size = NDataVersion2::LENGTHS_OFFSETS[index].Len;
    const size_t offset = NDataVersion2::LENGTHS_OFFSETS[index].Index;
    result->Reserve(size);
    for (size_t i = 0; i < size; ++i) {
        const ui8 value = NDataVersion2::LENGTHS_SEQUENCES[offset + i];
        (*result)[i] = value;
    }
    result->SetInitializedSize(size);
}
