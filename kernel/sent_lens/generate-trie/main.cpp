#include <kernel/sent_lens/sent_lens.h>

#include <library/cpp/containers/comptrie/comptrie_builder.h>

#include <util/stream/output.h>

int main() {
    TCompactTrieBuilder<ui8, ui16> builder;
    const size_t blockCount = TSentenceLengthsCoderData::GetBlockCount();
    for (size_t i = 0; i != blockCount; ++i) {
        TSentenceLengths sentlens;
        TSentenceLengthsCoderData::GetBlockVersion2(i, &sentlens);
        builder.Add(~sentlens, +sentlens, i);
    }
    TBufferOutput bufout;
    builder.Save(Cout);
    return 0;
}
