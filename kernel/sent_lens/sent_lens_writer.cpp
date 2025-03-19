#include "sent_lens_writer.h"

#include "sent_lens_data_trie.h"

#include "data/sentence_lengths_coder_data.h"

#include <util/system/yassert.h>
#include <util/generic/singleton.h>
#include <util/digest/murmur.h>

namespace {
    const size_t HashBytes = TSentenceLengthsCoderData::GetNumBytesForHashVersion2();

    size_t FindLongestPrefixInCollection(const ui8* begin, size_t size, ui16* index) {
        return Singleton<TSentLensDataCollectionTrie>()->FindLongestPrefix(begin, size, index);
    }
}

TSentenceLengthsWriter::TSentenceLengthsWriter(const TString& name, ui32 version)
    : Version(version)
    , FOut(new TFileOutput(name))
    , Out(FOut.Get())
    , ChunkedOut(*Out)
{
    InternalInit();
}

TSentenceLengthsWriter::TSentenceLengthsWriter(IOutputStream* out, ui32 version)
    : Version(version)
    , Out(out)
    , ChunkedOut(*Out)
{
    InternalInit();
}

void TSentenceLengthsWriter::InternalInit() {
    Estimated = 0;
    Blocks = 0;
    ChunkedOut.NewBlock();
    char buffer[TSentenceLengthsReader::SIZEOF_HEADER];
    memset(buffer, 0, TSentenceLengthsReader::SIZEOF_HEADER);
    memcpy(buffer, &Version, sizeof(Version));
    ChunkedOut.Write(buffer, sizeof(buffer));
    ChunkedOut.NewBlock();
}

struct TDPState {
    ui64 Hash;
    ui32 Len;
    ui32 PrevLen;
    ui32 PrevOffset;
    ui16 Index;
};

void TSentenceLengthsWriter::AddVersion2(const ui8* begin, size_t size) {
    if (size > 0) {
        size_t i = 0;
        while (i < size) {
            ui16 maxIndex = 0;
            size_t max = FindLongestPrefixInCollection(begin + i, size - i, &maxIndex);

            if (!max)
                ythrow yexception() << "corrupted sentlens data: subsequence can't be found in collection" << (int)begin[i];

            ChunkedOut.Write(&maxIndex, HashBytes);
            i += max;
            Estimated += 2;
            ++Blocks;
        }
    }
}

void TSentenceLengthsWriter::Add(ui32 docId, const TSentenceLengths& lengths) {
    Add(docId, ~lengths, +lengths);
}

void TSentenceLengthsWriter::Add(ui32 docId, const ui8* begin, size_t size) {
    if (Offsets.size() < docId + 1)
        Offsets.resize(docId + 1, TSentenceLengthsReader::INVALID_OFFSET);

    if (size) {
        ui64 hash = MurmurHash<ui64>(begin, size);

        THash2Index::const_iterator toHash = Hash2Index.find(hash);
        if (toHash == Hash2Index.end()) {
            const ui64 before = ChunkedOut.GetCurrentBlockOffset();
            Y_ASSERT(size < (1 << 16));
            const ui16 sLen = size;
            ChunkedOut.Write(&sLen, 2);

            if (Version == 2)
                AddVersion2(begin, size);
            else
                ythrow yexception() << "unknown version";

            Hash2Index[hash] = docId;
            Offsets[docId] = TSentenceLengthsReader::TOffset(before);
        } else {
            Offsets[docId] = Offsets[toHash->second];
        }
    }
}

void TSentenceLengthsWriter::AddPacked(ui32 docId, const TSentenceLengths& lengthsPacked) {
    if (Offsets.size() < docId + 1)
        Offsets.resize(docId + 1, TSentenceLengthsReader::INVALID_OFFSET);

    if (+lengthsPacked) {
        ui64 hash = MurmurHash<ui64>(~lengthsPacked, +lengthsPacked);

        THash2Index::const_iterator toHash = PackedHash2Index.find(hash);
        if (toHash == PackedHash2Index.end()) {
            const ui64 before = ChunkedOut.GetCurrentBlockOffset();
            for (size_t i = 0; i < +lengthsPacked; ++i)
                ChunkedOut.Write(&lengthsPacked[i], 1);

            PackedHash2Index[hash] = docId;
            Offsets[docId] = TSentenceLengthsReader::TOffset(before);
        } else {
            Offsets[docId] = Offsets[toHash->second];
        }
    }
}

TSentenceLengthsWriter::~TSentenceLengthsWriter() {
    ChunkedOut.NewBlock();
    ChunkedOut.Write(Offsets.data(), Offsets.size()*sizeof(Offsets[0]));
    ChunkedOut.WriteFooter();
}

size_t TSentenceLengthsWriter::GetEstimated() const {
    return Estimated;
}

size_t TSentenceLengthsWriter::GetBlocks() const {
    return Blocks;
}
