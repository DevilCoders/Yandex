#include "wad_writer.h"
#include "lrc.h"
#include "chunked_blob_storage.h"

#include <library/cpp/erasure/codec.h>

#include <kernel/doom/wad/wad.h>
#include <kernel/doom/wad/check_sum_doc_lump.h>
#include <kernel/doom/erasure/mega_wad.h>
#include <kernel/doom/flat_blob_storage/mapped_flat_storage.h>

#include <library/cpp/offroad/flat/flat_searcher.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/system/filemap.h>
#include <util/stream/file.h>
#include <util/random/random.h>

using TCodecType = NDoom::TLrc<12, 4>;

Y_UNIT_TEST_SUITE(ErasureWad) {
    using namespace NDoom;
    constexpr ui32 DocLumpsCount = 3000;

    TVector<TWadLumpId> LUMPS = {
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

    TBlob RandomGarbage(size_t sz) {
        TBuffer buf;
        buf.Resize(sz);

        for (size_t i = 0; i < sz; i++) {
            buf.Data()[i] = RandomNumber<ui8>();
        }

        return TBlob::FromBuffer(buf);
    }

    TVector<TVector<TVector<TBlob>>> GenDocLumps(const ui32 indexCount) {
        static TVector<TVector<TVector<TBlob>>> data;

        while (data.size() < indexCount) {
            data.push_back({});
            for (ui32 i = 0; i < DocLumpsCount; ++i) {
                data.back().push_back({});
                for (ui32 j = 0; j < LUMPS.size(); ++j) {
                    data.back().back().push_back(RandomGarbage(RandomNumber<ui8>()));
                }
            }
        }

        return TVector<TVector<TVector<TBlob>>>(data.begin(), data.begin() + indexCount);
    }

    void EnsureEqual(const TBlob& l, const TBlob& r) {
        UNIT_ASSERT_EQUAL(l.Size(), r.Size());
        if (l.Size() != 0) {
            UNIT_ASSERT(memcmp(l.Data(), r.Data(), l.Size()) == 0);
        }
    }

    void WriteDocToPart(const TString& filePrefix, const TVector<ui32>& docToPart) {
        using TLocationWriter = NOffroad::TFlatWriter<std::nullptr_t, NDoom::TDocInPartLocation, NOffroad::TNullVectorizer, NDoom::TDocInPartLocationVectorizer>;
        TLocationWriter offroadWriter;
        NDoom::TAccumulatingOutput offroadOutput;
        offroadWriter.Reset(&offroadOutput);

        TVector<ui32> numDocsInPart(TCodecType::DataPartCount);

        for (ui32 partIndex : docToPart) {
            offroadWriter.Write(nullptr, NDoom::TDocInPartLocation{partIndex, numDocsInPart[partIndex]});
            ++numDocsInPart[partIndex];
        }

        offroadWriter.Finish();

        NDoom::TMegaWadWriter wadWriter(filePrefix + ".doctopart");
        offroadOutput.Flush(wadWriter.StartGlobalLump({ NDoom::EWadIndexType::ErasureDocInPartLocation, NDoom::EWadLumpRole::Hits }));
        wadWriter.Finish();
    }

    TVector<TString> GetPartsFilesNames(const TString& filePrefix = "", const ui32 partCount = TCodecType::TotalPartCount) {
        TVector<TString> parts;
        for (ui32 i = 0; i < partCount; ++i) {
            parts.push_back(TString::Join(filePrefix, ToString(i), ".part"));
        }
        return parts;
    }

    TVector<TString> GetGlobalsFilesNames(const TString& filePrefix = "", const ui32 indexCount = 1) {
        TVector<TString> globals;
        for (ui32 i = 0; i < indexCount; ++i) {
            globals.push_back(TString::Join(filePrefix, ToString(i), ".global"));
        }
        return globals;
    }

    std::pair<THashMap<TWadLumpId, TBlob>, THashMap<std::pair<size_t, TWadLumpId>, TBlob>> WriteSomething(
        const TString& filePrefix = "",
        const ui32 indexCount = 1,
        const bool useNewMappings = true)
    {
        TErasureWadWriter<TCodecType> writer(64);
        writer.SetSaveDataPartIndexByDocId();

        writer.Reset(GetGlobalsFilesNames(filePrefix, indexCount), GetPartsFilesNames(filePrefix), useNewMappings);

        THashMap<TWadLumpId, TBlob> globalLumps;
        for (auto id: LUMPS) {
            auto* lump = writer.StartGlobalLump(0, id);
            auto data = RandomGarbage(RandomNumber<ui8>());
            lump->Write(data.Data(), data.Length());
            globalLumps[id] = data;

            for (ui32 i = 0; i < indexCount; ++i) {
                writer.RegisterDocLumpType(i, id);
            }
        }

        auto docLumps = GenDocLumps(indexCount);
        THashMap<std::pair<size_t, TWadLumpId>, TBlob> docLumpsMap;
        for (ui32 j = 0; j < DocLumpsCount; ++j) {
            for (ui32 i = 0; i < indexCount; ++i) {
                for (ui32 k = 0; k < LUMPS.size(); ++k) {
                    writer.StartDocLump(i, j, LUMPS[k])->Write(docLumps[i][j][k].Data(), docLumps[i][j][k].Size());
                    docLumpsMap[std::make_pair(j, LUMPS[k])] = docLumps[i][j][k];
                }
            }
        }
        writer.Finish();

        if (useNewMappings) {
            WriteDocToPart(filePrefix, writer.TakeDataPartIndexByDocId());
        }

        return{ globalLumps, docLumpsMap };
    }

    TErasureMegaWadWithOptimizedParts<TMappedFlatStorage> OpenWad(const TString& filePrefix) {
        return {
            GetGlobalsFilesNames(filePrefix)[0],
            MakeHolder<TMappedFlatStorage>(GetPartsFilesNames(filePrefix, TCodecType::DataPartCount)),
            filePrefix + ".doctopart"
        };
    }

    Y_UNIT_TEST(SimpleWrite) {
        auto[globalLumps, docLumps] = WriteSomething();

        auto blobStorage = MakeHolder<TRealErasureChunkedBlobStorage<TMappedFlatStorage>>(
            TConstArrayRef<TString>{".doctopart"},
            GetGlobalsFilesNames(),
            false,
            MakeHolder<TMappedFlatStorage>(GetPartsFilesNames("", TCodecType::DataPartCount)));

        auto wad = OpenWad("");
        TVector<size_t> mapping;
        mapping.resize(std::size(LUMPS));
        wad.MapDocLumps(LUMPS, mapping);

        UNIT_ASSERT_EQUAL(wad.GlobalLumps().size(), 3 + std::size(LUMPS));

        UNIT_ASSERT_EQUAL(wad.Size(), DocLumpsCount);

        for (size_t i = 0; i < DocLumpsCount; ++i) {
            TVector<TArrayRef<const char>> regions;
            regions.resize(mapping.size());

            TBlob erasureBlob = wad.LoadDocLumps(i, mapping, regions);
            TBlob blob = blobStorage->Read(0, i + blobStorage->ChunkInfos()[0].FirstDoc);
            EnsureEqual(erasureBlob, blob);

            for (size_t j = 0; j < std::size(LUMPS); ++j) {
                EnsureEqual(docLumps[std::make_pair(i, LUMPS[j])], TBlob::NoCopy(regions[j].data(), regions[j].size()));
            }
        }

        TVector<ui32> docIds;
        TVector<TVector<TArrayRef<const char>>> regionsData;
        TVector<TArrayRef<TArrayRef<const char>>> regions;
        for (size_t i = 0; i < DocLumpsCount; ++i) {
            for (size_t j = 0; j < 3; ++j) {
                docIds.push_back(i + j * DocLumpsCount);
                regionsData.push_back(TVector<TArrayRef<const char>>(std::size(LUMPS)));
                regions.push_back(regionsData.back());
            }
        }

        auto docBlobs = wad.LoadDocLumps(docIds, mapping, regions);
        for (size_t i = 1; i < docIds.size(); ++i) {
            for (size_t j = 0; j < std::size(LUMPS); ++j) {
                EnsureEqual(docLumps[std::make_pair(docIds[i], LUMPS[j])], TBlob::NoCopy(regions[i][j].data(), regions[i][j].size()));
            }
        }
    }

    Y_UNIT_TEST(CheckErasure) {
        WriteSomething();

        TVector<THolder<TFileMap>> maps;
        size_t total = TCodecType().GetTotalPartCount();
        for (size_t i = 0; i < total; ++i) {
            maps.push_back(MakeHolder<TFileMap>(ToString(i) + ".part", TMemoryMap::oRdOnly));
            maps[i]->Map(0, maps[i]->Length());
        }

        TCodecType lrc;
        TVector<int> erasedIndices = {0, 1};
        auto repair = lrc.GetRepairIndices(erasedIndices);

        TVector<TBlob> inputs;
        for (size_t i: *repair) {
            inputs.push_back(TBlob::NoCopy(maps[i]->Ptr(), maps[i]->Length()));
        }
        auto restored = lrc.Decode(inputs, erasedIndices);
        for (size_t i = 0; i < erasedIndices.size(); ++i) {
            EnsureEqual(TBlob::NoCopy(maps[i]->Ptr(), maps[i]->Length()), restored[erasedIndices[i]]);
        }
    }

    Y_UNIT_TEST(CheckСheckSum) {
        WriteSomething();

        auto wad = OpenWad("");
        TVector<size_t> mapping(1);
        wad.MapDocLumps({CheckSumDocLumpId}, mapping);

        UNIT_ASSERT_EQUAL(wad.Size(), DocLumpsCount);

        for (size_t i = 1; i < DocLumpsCount; ++i) {
            TVector<TArrayRef<const char>> regions(mapping.size());

            TBlob documentBlob = wad.LoadDocLumps(i, mapping, regions);

            UNIT_ASSERT_EQUAL(sizeof(TCrcExtendCalcer::TCheckSum), regions[0].size());

            const auto fetchedCheckSum = FetchCheckSum(regions[0]);
            const auto calculatedCheckSum = CalcBlobCheckSum(documentBlob, regions[0]);

            UNIT_ASSERT(fetchedCheckSum.Defined() && calculatedCheckSum.Defined());
            UNIT_ASSERT_EQUAL(*calculatedCheckSum, *fetchedCheckSum);
        }
    }

    Y_UNIT_TEST(MappingsComparationTest) {
        constexpr ui32 IndexCount = 3;

        WriteSomething("new", IndexCount, true);
        WriteSomething("old", IndexCount, false);

        TVector<THolder<IWad>> oldWads;
        THolder<IWad> newDocToPartWad = IWad::Open("new.doctopart", false);
        THolder<IWad> newWad = IWad::Open(GetGlobalsFilesNames("new", IndexCount)[0]);

        TVector<TErasureLocationResolver> oldResolvers(IndexCount);
        TVector<TPartOptimizedErasureLocationResolver> newResolvers(IndexCount);

        for (ui32 i = 0; i < IndexCount; ++i) {
            oldWads.push_back(IWad::Open(GetGlobalsFilesNames("old", IndexCount)[i]));
            oldResolvers[i].Reset(oldWads[i].Get());
            newResolvers[i].Reset(newWad.Get(), i, newDocToPartWad.Get());
        }
        for (ui32 i = 0; i < IndexCount; ++i) {
            for (ui32 j = 0; j < DocLumpsCount; ++j) {
                auto oldLocation = *oldResolvers[i].Resolve(j);
                auto newLocation = *newResolvers[i].Resolve(j);
                UNIT_ASSERT_EQUAL(oldLocation.Part, newLocation.Part);
                UNIT_ASSERT_EQUAL(oldLocation.Offset, newLocation.Offset);
                UNIT_ASSERT_EQUAL(oldLocation.Size, newLocation.Size);
            }
        }

    }
}
