#include <library/cpp/testing/unittest/registar.h>

#include <library/cpp/digest/md5/md5.h>
#include <library/cpp/offroad/test/test_md5.h>

#include <util/generic/algorithm.h>
#include <util/random/random.h>
#include <util/random/shuffle.h>
#include <util/random/easy.h>
#include <random>

#include <kernel/doom/offroad_reg_herf_wad/reg_herf_io.h>
#include <kernel/doom/wrangler_layer/reg_herf/reg_herf_reader.h>
#include <kernel/doom/wad/mega_wad_buffer_writer.h>
#include <kernel/relevstatic/calc_regstatic.h>

#include <ysite/yandex/erf/erf_manager.h>

using namespace NDoom;
using namespace NOffroad;

Y_UNIT_TEST_SUITE(TestRegErfWriter) {
    using Io = TRegHostErfIo;
    using TSampler = typename Io::TSampler;
    using TWriter = typename Io::TWriter;
    using TSearcher = typename Io::TSearcher;
    using TData = TRegHostErfInfo;
    struct TEntry {
        ui32 DocId = 0;
        ui32 Region = 0;
        TData Data;
    };
    using TIndex = TVector<TEntry>;

    TData GenerateSimpleData(ui16 count) {
        Y_UNUSED(count);
        TData result;
        result.Reserved = count;
        return result;
    }

    TIndex GenerateSimpleIndex(ui32 docIdCount, ui32 regionCount) {
        //Keys longer than 56 bits are not supported
        TVector<ui16> docIdCounts(docIdCount);
        TVector<ui16> regionCounts(regionCount);

        Iota(docIdCounts.begin(), docIdCounts.end(), 0);
        Iota(regionCounts.begin(), regionCounts.end(), 0);

        TIndex result;

        for (ui32 d: docIdCounts) {
            for (ui32 r: regionCounts) {
                TEntry entry;
                entry.DocId = d;
                entry.Region = r;
                entry.Data = GenerateSimpleData(RandomNumber<ui16>());
                result.push_back(entry);
            }
        }

        return result;
    }

    TBuffer WriteIndex(const TIndex& index) {
        TSampler sampler;

        for (const auto& entry: index) {
            sampler.Write(typename TSampler::TKey(entry.DocId, entry.Region), &entry.Data);
        }

        auto model = sampler.Finish();
        TBufferOutput output;
        TMegaWadWriter wadwriter(&output);

        TWriter writer(model, &wadwriter);

        for (const auto& entry: index) {
            writer.Write(typename TSampler::TKey(entry.DocId, entry.Region), &entry.Data);
        }

        writer.Finish();
        wadwriter.Finish();

        return output.Buffer();
    }

    bool EntryCmp(TEntry a, TEntry b) {
        return std::pair<ui16, ui16>(a.DocId, a.Region) < std::pair<ui16, ui16>(b.DocId, b.Region);
    }

    void CheckIndex(TIndex& index) {
        auto buffer = WriteIndex(index);

        auto wad = IWad::Open(TArrayRef<const char>(buffer.data(), buffer.size()));
        TSearcher searcher(wad.Get());
        TRegHostErfAccessor accessor(&searcher);
        Shuffle(index.begin(), index.end());

        for (size_t i = 0; i < index.size(); ++i) {
            const auto& entry = index[i];
            auto foundHit = accessor.GetRegErf(entry.DocId, entry.Region);
            UNIT_ASSERT(memcmp(foundHit, &entry.Data, sizeof(TData)) == 0);
        }

        for (size_t i = 0; i < index.size(); ++i) {
            TEntry item;
            item.DocId = RandomNumber<ui16>();
            item.Region = RandomNumber<ui16>();
            while (LowerBound(index.begin(), index.end(), item, EntryCmp) != index.end()) {
                item.DocId = RandomNumber<ui16>();
                item.Region = RandomNumber<ui16>();
            }
            UNIT_ASSERT(accessor.GetRegErf(item.DocId, item.Region) == nullptr);
        }
    }

    Y_UNIT_TEST(SimpleIndexSearchTest) {
        TIndex index_empty = GenerateSimpleIndex(0, 0);
        CheckIndex(index_empty);
        TIndex index1_2 = GenerateSimpleIndex(1, 2);
        CheckIndex(index1_2);
        TIndex index1_1000 = GenerateSimpleIndex(1, 1000);
        CheckIndex(index1_1000);
        TIndex index1000_1 = GenerateSimpleIndex(1000, 1);
        CheckIndex(index1000_1);
        TIndex index50_50 = GenerateSimpleIndex(50, 50);
        CheckIndex(index50_50);
    }
}


