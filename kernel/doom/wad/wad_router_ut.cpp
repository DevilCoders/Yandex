#include <library/cpp/testing/unittest/registar.h>

#include <kernel/doom/wad/mega_wad_writer.h>

#include "wad_router.h"

#include <random>

Y_UNIT_TEST_SUITE(WadRouter) {
    using namespace NDoom;

    static constexpr size_t DOC_COUNT = 1000;

    static TVector<THolder<TBufferStream>> Buffers;

    THolder<IWad> GenerateWad(const TVector<TWadLumpId>& lumps) {
        Buffers.emplace_back(new TBufferStream());
        TBufferStream& source = *Buffers.back();
        TMegaWadWriter writer(&source);
        TVector<TWadLumpId> docLumps = lumps;
        writer.RegisterDocLumpTypes(docLumps);

        ui64 written = 0;

        for (size_t i = 0; i < lumps.size() / 2; i++) {
            IOutputStream* out = writer.StartGlobalLump(lumps[i]);
            TString lump = ToString(lumps[i]);
            out->Write(lump);
            written += lump.length();
        }

        std::mt19937 rnd(12417);
        std::shuffle(docLumps.begin(), docLumps.end(), rnd);

        for (size_t i = 0; i < DOC_COUNT + 1 - Buffers.size(); i++) {
            for (size_t j = 0; j < lumps.size(); j++) {
                TString lump = ToString(lumps[j]) + "-" + ToString(i);
                IOutputStream* out = writer.StartDocLump(i, lumps[j]);
                out->Write(lump);
                written += lump.length();
            }
        }

        for (size_t i = lumps.size() / 2; i < lumps.size(); i++) {
            IOutputStream* out = writer.StartGlobalLump(lumps[i]);
            TString lump = ToString(lumps[i]);
            out->Write(lump);
            written += lump.length();
        }

        writer.Finish();

        Cout << "WRITTEN: " << written << ", SIZE: " << source.Buffer().Size() << " PCS: " << ((double)written / (double)source.Buffer().Size() * 100.0) << Endl;

        return {IWad::Open(TArrayRef<const char>(source.Buffer().data(), source.Buffer().size()))};
    }

    TWadLumpId LUMPS[] = {
        TWadLumpId(FactorAnnDataIndexType,  EWadLumpRole::HitsModel),
        TWadLumpId(FactorAnnDataIndexType,  EWadLumpRole::Hits),
        TWadLumpId(FactorAnnDataIndexType,  EWadLumpRole::HitSub),

        TWadLumpId(FactorAnnIndexType,      EWadLumpRole::HitsModel),
        TWadLumpId(FactorAnnIndexType,      EWadLumpRole::KeysModel),
        TWadLumpId(FactorAnnIndexType,      EWadLumpRole::Keys),
        TWadLumpId(FactorAnnIndexType,      EWadLumpRole::KeyFat),
        TWadLumpId(FactorAnnIndexType,      EWadLumpRole::KeyIdx),
        TWadLumpId(FactorAnnIndexType,      EWadLumpRole::Hits),
        TWadLumpId(FactorAnnIndexType,      EWadLumpRole::HitSub),

        TWadLumpId(FastSentIndexType,      EWadLumpRole::HitsModel),
        TWadLumpId(FastSentIndexType,      EWadLumpRole::KeysModel),
        TWadLumpId(FastSentIndexType,      EWadLumpRole::Keys),
        TWadLumpId(FastSentIndexType,      EWadLumpRole::KeyFat),
        TWadLumpId(FastSentIndexType,      EWadLumpRole::KeyIdx),
        TWadLumpId(FastSentIndexType,      EWadLumpRole::Hits),
        TWadLumpId(FastSentIndexType,      EWadLumpRole::HitSub),

        TWadLumpId(OmniDssmAggregatedAnnRegEmbeddingType,      EWadLumpRole::HitsModel),
        TWadLumpId(OmniDssmAggregatedAnnRegEmbeddingType,      EWadLumpRole::KeysModel),
        TWadLumpId(OmniDssmAggregatedAnnRegEmbeddingType,      EWadLumpRole::Keys),
        TWadLumpId(OmniDssmAggregatedAnnRegEmbeddingType,      EWadLumpRole::KeyFat),
        TWadLumpId(OmniDssmAggregatedAnnRegEmbeddingType,      EWadLumpRole::KeyIdx),
        TWadLumpId(OmniDssmAggregatedAnnRegEmbeddingType,      EWadLumpRole::Hits),
        TWadLumpId(OmniDssmAggregatedAnnRegEmbeddingType,      EWadLumpRole::HitSub),

        TWadLumpId(PantherIndexType,      EWadLumpRole::HitsModel),
        TWadLumpId(PantherIndexType,      EWadLumpRole::KeysModel),
        TWadLumpId(PantherIndexType,      EWadLumpRole::Keys),
        TWadLumpId(PantherIndexType,      EWadLumpRole::KeyFat),
        TWadLumpId(PantherIndexType,      EWadLumpRole::KeyIdx),
        TWadLumpId(PantherIndexType,      EWadLumpRole::Hits),
        TWadLumpId(PantherIndexType,      EWadLumpRole::HitSub),
    };

    const size_t LUMPS_COUNT = sizeof(LUMPS) / sizeof(LUMPS[0]);

    Y_UNIT_TEST(Route) {
        std::mt19937 rnd(65537);

        TVector<TWadLumpId> docLumps(LUMPS, LUMPS + LUMPS_COUNT);
        std::array<TVector<TWadLumpId>, 5> lumpParts;

        for (size_t i = 0; i < LUMPS_COUNT; ++i) {
            size_t idx = rnd() % lumpParts.size();
            lumpParts[idx].push_back(LUMPS[i]);
        }

        TWadRouter wad;

        for (auto& lumps : lumpParts) {
            wad.AddWad(GenerateWad(lumps));
        }
        wad.Finish();

        /* List all global lumps. */
        auto globalLumps = wad.GlobalLumps();

        /* Ensure we have all lumps here. */
        UNIT_ASSERT_VALUES_EQUAL(globalLumps.size(), LUMPS_COUNT);
        for (size_t i = 0; i < LUMPS_COUNT; i++) {
            bool ok = false;
            for (size_t j = 0; j < LUMPS_COUNT; j++) {
                if (globalLumps[i] == LUMPS[j])
                    ok = true;
            }
            UNIT_ASSERT(ok);
        }

        for (size_t i = 0; i < LUMPS_COUNT; i++) {
            UNIT_ASSERT(wad.HasGlobalLump(LUMPS[i]));
            TBlob blob = wad.LoadGlobalLump(LUMPS[i]);
            UNIT_ASSERT_VALUES_EQUAL(TString(blob.AsCharPtr(), blob.Size()), ToString(LUMPS[i]));
        }

        UNIT_ASSERT_VALUES_EQUAL(wad.Size(), DOC_COUNT);

        /* Check documents. */
        for (size_t i = 0; i < DOC_COUNT; i++) {
            size_t lumpCnt = (i % LUMPS_COUNT) + 1;
            TVector<size_t> mapping;
            TVector<TArrayRef<const char>> dataRegions;
            mapping.resize(lumpCnt);
            dataRegions.resize(lumpCnt);

            wad.MapDocLumps(TArrayRef<const TWadLumpId>(LUMPS, LUMPS + lumpCnt), TArrayRef<size_t>(mapping.begin(), mapping.end()));
            TBlob blob1 = wad.LoadDocLumps(i, mapping, dataRegions);

            for (size_t j = 0; j < lumpCnt; j++) {
                TString regionData(dataRegions[j].data(), dataRegions[j].size());
                if (i < DOC_COUNT - lumpParts.size() || regionData.size()) {
                    UNIT_ASSERT_VALUES_EQUAL(regionData, ToString(LUMPS[j]) + "-" + ToString(i));
                }
            }
        }
    }
}

