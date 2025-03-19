#include "offroad_invhash_wad.h"

#include <kernel/doom/wad/mega_wad_buffer_writer.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/random/fast.h>
#include <util/random/random.h>
#include <util/random/shuffle.h>
#include <util/stream/output.h>

using namespace NDoom;

class TInvHashIndexData {
public:
    TVector<TInvHashEntry> Data;

    bool operator==(const TInvHashIndexData& rhs) const {
        return Data == rhs.Data;
    }
};

template<>
inline void Out<TInvHashEntry>(IOutputStream& out, const TInvHashEntry& value) {
    out << value.Hash << "." << value.DocId;
}

template<>
inline void Out<TInvHashIndexData>(IOutputStream& out, const TInvHashIndexData& value) {
    out << value.Data.size() << " hits:\n";
    for (const TInvHashEntry& entry : value.Data) {
        out << entry << "\n";
    }
}

namespace {

TInvHashIndexData GenerateIndexData(size_t n, size_t seed) {
    TVector<ui32> docIds(n);
    Iota(docIds.begin(), docIds.end(), 0);
    TFastRng32 gen(seed, 0);
    ShuffleRange(docIds, gen);

    TVector<TInvHashEntry> data(n);
    ui64 curHash = 0;
    ui32 curMax1 = 1;
    ui32 curMax2 = 1;
    for (size_t i = 0; i < n; ++i) {
        data[i].DocId = docIds[i];

        data[i].Hash = curHash;

        if (gen.Uniform(10)== 0) {
            ui32 type = gen.Uniform(3);
            if (type == 0) {
                curMax1 = 1;
            } else if (type == 1) {
                curMax1 = 1 + gen.Uniform(3);
            } else {
                curMax1 = gen.Uniform(1, Max<ui32>());
            }
        }

        if (gen.Uniform(10) == 0) {
            ui32 type = gen.Uniform(3);
            if (type == 0) {
                curMax2 = 1;
            } else if (type == 1) {
                curMax2 = 1 + gen.Uniform(3);
            } else {
                curMax2 = gen.Uniform(1, Max<ui32>());
            }
        }

        curHash += ((1 + (curMax1 == 1 ? 0 : gen.Uniform(curMax1 - 1))) << 8);
        curHash += (1 + (curMax2 == 1 ? 0 : gen.Uniform(curMax2 - 1)));
    }

    return { data };
}

TBuffer WriteIndex(const TInvHashIndexData& data) {
    TInvHashWadIo::TSampler sampler;
    for (const TInvHashEntry& entry : data.Data) {
        sampler.WriteHit(entry);
    }
    TInvHashWadIo::TSampler::TModel model = sampler.Finish();

    TBuffer buffer;
    TMegaWadBufferWriter wadWriter(&buffer);

    TInvHashWadIo::TWriter writer;
    writer.Reset(model, &wadWriter);

    for (const TInvHashEntry& entry : data.Data) {
        writer.WriteHit(entry);
    }

    writer.Finish();
    wadWriter.Finish();

    return buffer;
}

TInvHashIndexData ReadIndex(const TBuffer& indexData) {
    THolder<NDoom::IWad> wad = IWad::Open(indexData);
    TInvHashWadIo::TReader reader;
    reader.Reset(wad.Get());

    TInvHashIndexData data;
    TInvHashEntry entry;
    while (reader.ReadHit(&entry)) {
        data.Data.push_back(entry);
    }
    return data;
}

} // anonymous namespace

Y_UNIT_TEST_SUITE(TestOffroadInvHashWad) {
    void DoTestReadWriteSeek(size_t n, size_t seed = 17) {
        auto data = GenerateIndexData(n, seed);
        TBuffer wadData = WriteIndex(data);
        TInvHashIndexData readData = ReadIndex(wadData);
        UNIT_ASSERT_VALUES_EQUAL(readData, data);

        THolder<NDoom::IWad> wad = IWad::Open(wadData);
        TInvHashWadSearcher searcher;
        searcher.Reset(wad.Get());

        TInvHashWadIterator iterator;

        TFastRng32 gen(seed, 0);
        ShuffleRange(readData.Data, gen);
        for (const TInvHashEntry& entry : readData.Data) {ui32 docId = 0;
            bool found = searcher.Search(entry.Hash, &docId, &iterator);
            UNIT_ASSERT(found);
            UNIT_ASSERT_VALUES_EQUAL(docId, entry.DocId);
        }

        THashSet<ui64> nonExistentHashes;
        for (const TInvHashEntry& entry : readData.Data) {
            nonExistentHashes.insert(entry.Hash + 1);
            nonExistentHashes.insert(entry.Hash - 1);
        }

        for (ui32 i = 0; i < readData.Data.size(); ++i) {
            nonExistentHashes.insert(i);
            nonExistentHashes.insert(gen.GenRand64());
        }

        for (const TInvHashEntry& entry : readData.Data) {
            nonExistentHashes.erase(entry.Hash);
        }

        for (const ui64 hash : nonExistentHashes) {
            ui32 docId = 0;
            bool found = searcher.Search(hash, &docId, &iterator);
            UNIT_ASSERT(!found);
        }
    }

    Y_UNIT_TEST(TestReadWriteSeek) {
        DoTestReadWriteSeek(0);
        DoTestReadWriteSeek(1);
        DoTestReadWriteSeek(2);
        DoTestReadWriteSeek(10);
        DoTestReadWriteSeek(100);
        DoTestReadWriteSeek(10000);
    }
}
