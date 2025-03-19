#include <library/cpp/testing/unittest/registar.h>

#include "mega_wad.h"
#include "mega_wad_writer.h"

#include <random>

bool operator==(const TBlob& a, const TBlob& b) {
    return std::equal(a.Begin(), a.End(), b.Begin(), b.End());
}

IOutputStream& operator<<(IOutputStream& out, const TBlob& blob) {
    out << TStringBuf(blob.AsCharPtr(), blob.Size());
    return out;
}

Y_UNIT_TEST_SUITE(MegaWad) {
    using namespace NDoom;

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
    };

    const size_t LUMPS_COUNT = sizeof(LUMPS) / sizeof(LUMPS[0]);

    Y_UNIT_TEST(Write) {
        TVector<TWadLumpId> docLumps(LUMPS, LUMPS + LUMPS_COUNT);

        TBufferStream source;
        TMegaWadWriter writer(&source);
        writer.RegisterDocLumpTypes(docLumps);

        ui64 written = 0;

        for (size_t i = 0; i < LUMPS_COUNT / 2; i++) {
            IOutputStream* out = writer.StartGlobalLump(LUMPS[i]);
            TString lump = ToString(LUMPS[i]);
            out->Write(lump);
            written += lump.length();
        }

        std::mt19937 rnd(12417);
        std::shuffle(docLumps.begin(), docLumps.end(), rnd);

        for (size_t i = 0; i <= LUMPS_COUNT; i++) {
            for (size_t j = 0; j < i; j++) {
                TString lump = ToString(LUMPS[j]);
                IOutputStream* out = writer.StartDocLump(i, LUMPS[j]);
                out->Write(lump);
                written += lump.length();
            }
        }

        for (size_t i = LUMPS_COUNT / 2; i < LUMPS_COUNT; i++) {
            IOutputStream* out = writer.StartGlobalLump(LUMPS[i]);
            TString lump = ToString(LUMPS[i]);
            out->Write(lump);
            written += lump.length();
        }

        writer.Finish();

        Cout << "WRITTEN: " << written << ", SIZE: " << source.Buffer().Size() << " PCS: " << ((double)written / (double)source.Buffer().Size() * 100.0) << Endl;
        TMegaWad wad(TArrayRef<const char>(source.Buffer().data(), source.Buffer().size()));

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
            TBlob blob = wad.LoadGlobalLump(LUMPS[i]);
            UNIT_ASSERT_VALUES_EQUAL(TString(blob.AsCharPtr(), blob.Size()), ToString(LUMPS[i]));
        }

        UNIT_ASSERT_VALUES_EQUAL(wad.Size(), LUMPS_COUNT + 1);

        /* Check documents. */
        for (size_t i = 0; i <= LUMPS_COUNT; i++) {
            TVector<size_t> mapping;
            TVector<TArrayRef<const char>> dataRegions;

            mapping.resize(i);
            dataRegions.resize(i);

            wad.MapDocLumps(TArrayRef<const TWadLumpId>(LUMPS, LUMPS + i), TArrayRef<size_t>(mapping.begin(), mapping.end()));
            TBlob blob1 = wad.LoadDocLumps(i, mapping, dataRegions);

            for (size_t j = 0; j < i; j++) {
                TString regionData(dataRegions[j].data(), dataRegions[j].size());
                UNIT_ASSERT_VALUES_EQUAL(regionData, ToString(LUMPS[j]));
            }
        }
    }

    Y_UNIT_TEST(UnknownIndexType) {
        static constexpr TStringBuf UnknownLumpIdName = "unknown_lump_id.struct";
        static constexpr TWadLumpId KnownLumpId = TWadLumpId(FactorAnnDataIndexType,  EWadLumpRole::HitsModel);
        static const TString KnownLumpIdName = ToString(KnownLumpId);
        static TVector<TString> AllLumpIdNames = { TString(UnknownLumpIdName), KnownLumpIdName };
        Sort(AllLumpIdNames);

        TBufferStream source;
        {
            TMegaWadWriter writer(&source);
            writer.RegisterDocLumpType(UnknownLumpIdName);
            writer.RegisterDocLumpType(KnownLumpId);

            *writer.StartGlobalLump(UnknownLumpIdName) << "global unknown";
            *writer.StartGlobalLump(KnownLumpId) << "global known";

            *writer.StartDocLump(0, UnknownLumpIdName) << "doc unknown";
            *writer.StartDocLump(0, KnownLumpId) << "doc known";

            writer.Finish();
        }

        {
            TMegaWad wad(TArrayRef<const char>(source.Buffer().data(), source.Buffer().size()));
            TVector<TWadLumpId> globalLumps = wad.GlobalLumps();
            UNIT_ASSERT_VALUES_EQUAL(globalLumps, TVector<TWadLumpId>{ KnownLumpId });

            TVector<TString> globalLumpNames = wad.GlobalLumpsNames();
            Sort(globalLumpNames);
            UNIT_ASSERT_VALUES_EQUAL(globalLumpNames, AllLumpIdNames);

            TVector<TWadLumpId> docLumps = wad.DocLumps();
            UNIT_ASSERT_VALUES_EQUAL(docLumps, TVector<TWadLumpId>{ KnownLumpId });

            TVector<TStringBuf> docLumpNames = wad.DocLumpsNames();
            Sort(docLumpNames);
            UNIT_ASSERT_VALUES_EQUAL(TVector<TString>(docLumpNames.begin(), docLumpNames.end()), AllLumpIdNames);

            UNIT_ASSERT_VALUES_EQUAL(wad.LoadGlobalLump(UnknownLumpIdName), TBlob::FromString("global unknown"));
            UNIT_ASSERT_VALUES_EQUAL(wad.LoadGlobalLump(KnownLumpIdName), TBlob::FromString("global known"));
            UNIT_ASSERT_VALUES_EQUAL(wad.LoadGlobalLump(KnownLumpId), TBlob::FromString("global known"));

            {
                std::array<size_t, 2> mapping;
                std::array<TArrayRef<const char>, 2> regions;
                std::array<TStringBuf, 2> names = { UnknownLumpIdName, KnownLumpIdName };
                wad.MapDocLumps(names, mapping);
                TBlob blob = wad.LoadDocLumps(0, mapping, regions);
                UNIT_ASSERT_VALUES_EQUAL(TStringBuf(regions[0].begin(), regions[0].end()), "doc unknown");
                UNIT_ASSERT_VALUES_EQUAL(TStringBuf(regions[1].begin(), regions[1].end()), "doc known");
            }
            {
                std::array<size_t, 1> mapping;
                std::array<TArrayRef<const char>, 1> regions;
                std::array<TWadLumpId, 1> names = { KnownLumpId };
                wad.MapDocLumps(names, mapping);
                TBlob blob = wad.LoadDocLumps(0, mapping, regions);
                UNIT_ASSERT_VALUES_EQUAL(TStringBuf(regions[0].begin(), regions[0].end()), "doc known");
            }
        }
    }
}

