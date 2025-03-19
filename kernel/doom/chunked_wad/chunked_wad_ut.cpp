#include <util/random/shuffle.h>
#include <library/cpp/testing/unittest/registar.h>

#include "doc_chunk_mapping.h"
#include "chunked_mega_wad.h"

#include <kernel/doom/wad/mega_wad_writer.h>

#include <util/stream/buffer.h>
#include <util/system/tempfile.h>
#include <util/memory/blob.h>

#include <random>

using namespace NDoom;

Y_UNIT_TEST_SUITE(TestChunkedWad) {

    struct TTestData {
        ui32 ChunksCount = 0;
        THashMap<TWadLumpId, TString> DataByLump;
        TVector<TWadLumpId> GlobalLumps;
        TVector<TVector<TWadLumpId>> ChunkGlobalLumps;
        TVector<TDocChunkMapping> DocChunksMapping;
        TVector<TVector<TWadLumpId>> DocChunkedLumps;
    };

    void WriteWad(
        const TTestData& testData,
        TOFStream* docChunksMapping,
        TOFStream* common,
        TVector<THolder<IOutputStream>>* chunks)
    {
        Y_VERIFY(testData.DocChunkedLumps.size() == testData.DocChunksMapping.size());
        Y_VERIFY(testData.ChunkGlobalLumps.size() == testData.ChunksCount);
        Y_VERIFY(chunks->size() == testData.ChunksCount);

        // doc chunks mapping
        {
            NOffroad::TFlatWriter<TDocChunkMapping, std::nullptr_t, TDocChunkMappingVectorizer, NOffroad::TNullVectorizer> flatWriter;
            TBufferStream flatBuffer;
            flatWriter.Reset(&flatBuffer);
            for (const TDocChunkMapping& docMapping : testData.DocChunksMapping) {
                flatWriter.Write(docMapping, nullptr);
            }
            flatWriter.Finish();

            TMegaWadWriter writer(docChunksMapping);
            writer.StartGlobalLump(TWadLumpId(ChunkMappingIndexType, EWadLumpRole::Hits))->Write(flatBuffer.Buffer().Data(), flatBuffer.Buffer().Size());
            writer.Finish();
        }

        // global lumps
        {
            TMegaWadWriter writer(common);
            for (TWadLumpId lump : testData.GlobalLumps) {
                writer.StartGlobalLump(lump)->Write(testData.DataByLump.at(lump));
            }
            writer.Finish();
        }

        // doc chunked lumps
        {
            TVector<TVector<std::pair<ui32, ui32>>> chunkDocs(chunks->size());
            for (ui32 docId = 0; docId < testData.DocChunkedLumps.size(); ++docId) {
                const TDocChunkMapping& mapping = testData.DocChunksMapping[docId];
                chunkDocs[mapping.Chunk()].emplace_back(mapping.LocalDocId(), docId);
            }
            for (ui32 chunk = 0; chunk < chunks->size(); ++chunk) {
                Sort(chunkDocs[chunk]);

                TMegaWadWriter writer((*chunks)[chunk].Get());

                for (TWadLumpId lump : testData.ChunkGlobalLumps[chunk]) {
                    writer.StartGlobalLump(lump)->Write(testData.DataByLump.at(lump));
                }

                THashSet<TWadLumpId> allDocLumps;
                for (const auto& doc : chunkDocs[chunk]) {
                    for (TWadLumpId lump : testData.DocChunkedLumps[doc.second]) {
                        allDocLumps.insert(lump);
                    }
                }
                TVector<TWadLumpId> lumps(allDocLumps.begin(), allDocLumps.end());
                Shuffle(lumps.begin(), lumps.end()); // test remapping
                for (TWadLumpId lump : lumps) {
                    writer.RegisterDocLumpType(lump);
                }

                for (const auto& doc : chunkDocs[chunk]) {
                    for (TWadLumpId lump : testData.DocChunkedLumps[doc.second]) {
                        writer.StartDocLump(doc.first, lump)->Write(testData.DataByLump.at(lump));
                    }
                }
                writer.Finish();
            }
        }
    }

    template <class Function>
    void DoWithWad(
        const TTestData& testData,
        const Function& function)
    {
        TTempFile docChunksMappingTempFile("index.mapping.wad");
        TOFStream docChunksMappingOutput(docChunksMappingTempFile.Name());
        TTempFile commonTempFile("index.global.wad");
        TOFStream commonOutput(commonTempFile.Name());
        TVector<TTempFile> chunkTempFiles(Reserve(testData.ChunksCount));
        for (size_t i = 0; i < testData.ChunksCount; ++i) {
            const TString chunkPath = TStringBuilder() << "index." << i << ".wad";
            chunkTempFiles.emplace_back(chunkPath);
        }
        TVector<THolder<IOutputStream>> chunkOutputs(Reserve(testData.ChunksCount));
        for (size_t i = 0; i < testData.ChunksCount; ++i) {
            chunkOutputs.push_back(MakeHolder<TOFStream>(chunkTempFiles[i].Name()));
        }
        WriteWad(testData, &docChunksMappingOutput, &commonOutput, &chunkOutputs);
        docChunksMappingOutput.Finish();
        commonOutput.Finish();
        for (THolder<IOutputStream>& chunkOutput : chunkOutputs) {
            chunkOutput->Finish();
        }
        THolder<TChunkedMegaWad> wad = MakeHolder<TChunkedMegaWad>("index", false);

        function(wad.Get());
    }

    void CheckLump(const THashMap<TWadLumpId, TString>& dataByLump, TWadLumpId lump, const TBlob& blob) {
        UNIT_ASSERT_EQUAL(dataByLump.at(lump), TStringBuf(blob.AsCharPtr(), blob.Size()));
    }

    void CheckDocLump(
        const THashMap<TWadLumpId, TString>& dataByLump,
        const THashSet<TWadLumpId>& docLumps,
        TWadLumpId lump,
        const TBlob& blob)
    {
        if (docLumps.contains(lump)) {
            CheckLump(dataByLump, lump, blob);
        } else {
            UNIT_ASSERT(blob.Size() == 0);
        }
    }

    void CheckWad(const TTestData& testData) {
        TVector<ui32> docIds(testData.DocChunkedLumps.size() + 42);
        Iota(docIds.begin(), docIds.end(), 0);

        std::mt19937 rnd(4243);
        std::shuffle(docIds.begin(), docIds.end(), rnd);

        THashSet<TWadLumpId> allDocLumps;
        for (const TVector<TWadLumpId>& doc : testData.DocChunkedLumps) {
            for (TWadLumpId lump : doc) {
                allDocLumps.insert(lump);
            }
        }
        TVector<TWadLumpId> allDocLumpsList(allDocLumps.begin(), allDocLumps.end());

        DoWithWad(testData, [&](TChunkedMegaWad* wad) {
            TVector<size_t> allDocLumpsMapping(allDocLumps.size());
            wad->MapDocLumps(allDocLumpsList, allDocLumpsMapping);
            UNIT_ASSERT(wad->Chunks() == testData.ChunksCount);
            for (TWadLumpId lump : testData.GlobalLumps) {
                UNIT_ASSERT(wad->HasGlobalLump(lump));
                CheckLump(testData.DataByLump, lump, wad->LoadGlobalLump(lump));
            }
            for (size_t chunk = 0; chunk < testData.ChunksCount; ++chunk) {
                for (TWadLumpId lump : testData.ChunkGlobalLumps[chunk]) {
                    UNIT_ASSERT(wad->HasChunkGlobalLump(chunk, lump));
                    CheckLump(testData.DataByLump, lump, wad->LoadChunkGlobalLump(chunk, lump));
                }
            }
            wad->LoadDocLumps(docIds, allDocLumpsMapping,
                [&](size_t i, TMaybe<NDoom::IWad::TDocLumpData>&& data) {
                    Y_ENSURE(data);

                    TVector<TArrayRef<const char>> regions(allDocLumpsMapping.size());
                    TBlob blob = wad->LoadDocLumps(docIds[i], allDocLumpsMapping, regions);
                    UNIT_ASSERT_EQUAL(TStringBuf(data->Blob.AsCharPtr(), data->Blob.Size()), TStringBuf(blob.AsCharPtr(), blob.Size()));

                    for (size_t i = 0; i < data->Regions.size(); ++i) {
                        auto region1 = data->Regions[i];
                        auto region2 = regions[i];
                        UNIT_ASSERT_EQUAL(TStringBuf(region1.data(), region1.size()), TStringBuf(region2.data(), region2.size()));
                    }
                });

            TVector<size_t> fetcherMapping(allDocLumpsList.size());
            wad->Mapper().MapDocLumps(allDocLumpsList, fetcherMapping);
            wad->Fetch(docIds,
                [&](size_t i, auto* loader) {
                    UNIT_ASSERT(loader);
                    TVector<TArrayRef<const char>> fetcherRegions(allDocLumpsList.size());
                    loader->LoadDocRegions(fetcherMapping, fetcherRegions);

                    TVector<TArrayRef<const char>> regions(allDocLumpsMapping.size());
                    TBlob blob = wad->LoadDocLumps(docIds[i], allDocLumpsMapping, regions);

                    for (size_t j = 0; j < fetcherRegions.size(); ++j) {
                        auto region1 = fetcherRegions[j];
                        auto region2 = regions[j];
                        UNIT_ASSERT_VALUES_EQUAL(TStringBuf(region1.data(), region1.size()), TStringBuf(region2.data(), region2.size()));
                    }
                });

            for (ui32 docId : docIds) {
                if (docId >= wad->Size()) {
                    TVector<TArrayRef<const char>> regions(allDocLumps.size());
                    TBlob blob1 = wad->LoadDocLumps(docId, allDocLumpsMapping, regions);
                    for (size_t i = 0; i < regions.size(); ++i) {
                        UNIT_ASSERT(regions[i].size() == 0);
                    }
                    continue;
                }
                UNIT_ASSERT_EQUAL(testData.DocChunksMapping[docId].Chunk(), wad->DocChunk(docId));
                TVector<TWadLumpId> lumps = testData.DocChunkedLumps[docId];
                THashSet<TWadLumpId> currentDocLumps(lumps.begin(), lumps.end());
                do {
                    for (ui32 from = 0; from < lumps.size(); ++from) {
                        TVector<TWadLumpId> segmentLumps;
                        for (ui32 to = from; to < lumps.size(); ++to) {
                            segmentLumps.push_back(lumps[to]);

                            TVector<size_t> mapping(segmentLumps.size());
                            wad->MapDocLumps(segmentLumps, mapping);

                            TVector<TArrayRef<const char>> regions(segmentLumps.size());
                            TBlob blob1 = wad->LoadDocLumps(docId, mapping, regions);

                            for (size_t i = 0; i < segmentLumps.size(); ++i) {
                                CheckDocLump(
                                    testData.DataByLump,
                                    currentDocLumps,
                                    segmentLumps[i],
                                    TBlob::NoCopy(regions[i].data(), regions[i].size()));
                            }
                        }
                    }
                } while (std::next_permutation(lumps.begin(), lumps.end()));
            }
        });
    }

    static const TVector<TWadLumpId> GlobalLumps = {
        TWadLumpId(FactorAnnIndexType,                       EWadLumpRole::HitsModel),
        TWadLumpId(FactorAnnIndexType,                       EWadLumpRole::KeysModel),
        //OffroadAnnKeysLump,
        TWadLumpId(FactorAnnIndexType,                       EWadLumpRole::KeyFat),
        TWadLumpId(FactorAnnIndexType,                       EWadLumpRole::KeyIdx)
    };

    static const TVector<TWadLumpId> ChunkGlobalLumps = {
        TWadLumpId(FactorAnnIndexType,                       EWadLumpRole::HitSub),
        TWadLumpId(ErfIndexType,                             EWadLumpRole::Struct)
    };

    static const TVector<TWadLumpId> ChunkedDocLumps = {
        TWadLumpId(ImgLinkDataAnnDataIndexType,              EWadLumpRole::Hits),
        TWadLumpId(ImgLinkDataAnnDataIndexType,              EWadLumpRole::HitSub)
    };

    TTestData GenerateBaseData(ui32 chunksCount, bool allChunkGlobalLumps = false) {
        std::mt19937 rnd(4243);

        TTestData data;
        data.ChunksCount = chunksCount;
        data.GlobalLumps = GlobalLumps;

        data.ChunkGlobalLumps.resize(chunksCount);
        for (size_t chunk = 0; chunk < chunksCount; ++chunk) {
            if (allChunkGlobalLumps) {
                data.ChunkGlobalLumps[chunk] = ChunkGlobalLumps;
                continue;
            }
            const ui32 mask = rnd() % (ui32(1) << ChunkGlobalLumps.size());
            for (size_t i = 0; i < ChunkGlobalLumps.size(); ++i) {
                if ((mask >> i) & 1) {
                    data.ChunkGlobalLumps[chunk].push_back(ChunkGlobalLumps[i]);
                }
            }
            struct TRng {
                auto Uniform(size_t x) {
                    return Rng() % x;
                }

                std::mt19937& Rng;
            } rng{rnd};
            Shuffle(data.ChunkGlobalLumps[chunk].begin(), data.ChunkGlobalLumps[chunk].end(), rng);
        }

        TVector<TWadLumpId> allLumps = GlobalLumps;
        allLumps.insert(allLumps.end(), ChunkGlobalLumps.begin(), ChunkGlobalLumps.end());
        allLumps.insert(allLumps.end(), ChunkedDocLumps.begin(), ChunkedDocLumps.end());
        for (TWadLumpId lump : allLumps) {
            Y_VERIFY(!data.DataByLump.contains(lump));
            size_t len = 1 + rnd() % 13;
            TString s;
            for (size_t i = 0; i < len; ++i) {
                s.push_back(static_cast<char>('a' + rnd() % 3));
            }
            data.DataByLump[lump] = s;
        }

        return data;
    }

    void CheckHoles(const TTestData& testData) {
        for (ui32 chunkedDocsMask = 0; chunkedDocsMask < (1 << testData.DocChunkedLumps.size()); ++chunkedDocsMask) {
            TTestData copy = testData;
            for (ui32 chunkedDocId = 0; chunkedDocId < testData.DocChunkedLumps.size(); ++chunkedDocId) {
                if ((chunkedDocsMask & (1 << chunkedDocId)) == 0) {
                    copy.DocChunkedLumps[chunkedDocId].clear();
                }
            }
            CheckWad(copy);
        }
    }

    Y_UNIT_TEST(NoDocs) {
        TTestData testData = GenerateBaseData(4);
        CheckWad(testData);
    }

    Y_UNIT_TEST(ChunkedDocAllLumps) {
        TTestData testData = GenerateBaseData(4);
        std::array<ui32, 4> chunkDocIdPtr = {{}};
        std::mt19937 rnd(4243);
        for (ui32 docId = 0; docId < 53; ++docId) {
            testData.DocChunkedLumps.push_back(ChunkedDocLumps);
            ui32 chunk = rnd() % 4;
            testData.DocChunksMapping.emplace_back(chunk, chunkDocIdPtr[chunk]++);
        }
        CheckWad(testData);
    }

    Y_UNIT_TEST(ChunkedDocRandomLumps) {
        TTestData testData = GenerateBaseData(4);
        std::array<ui32, 4> chunkDocIdPtr = {{}};
        std::mt19937 rnd(4243);
        for (ui32 docId = 0; docId < 64; ++docId) {
            testData.DocChunkedLumps.emplace_back();
            for (size_t i = 0; i < ChunkedDocLumps.size(); ++i) {
                if (rnd() % 2 == 0) {
                    testData.DocChunkedLumps.back().push_back(ChunkedDocLumps[i]);
                }
            }
            ui32 chunk = rnd() % 4;
            testData.DocChunksMapping.emplace_back(chunk, chunkDocIdPtr[chunk]++);
        }
        CheckWad(testData);
    }

    Y_UNIT_TEST(ChunkedDocAllLumpsHoles) {
        TTestData testData = GenerateBaseData(2);
        std::array<ui32, 2> chunkDocIdPtr = {{}};
        std::mt19937 rnd(4243);
        for (ui32 docId = 0; docId < 6; ++docId) {
            testData.DocChunkedLumps.push_back(ChunkedDocLumps);
            ui32 chunk = rnd() % 2;
            testData.DocChunksMapping.emplace_back(chunk, chunkDocIdPtr[chunk]++);
        }
        CheckHoles(testData);
    }

    Y_UNIT_TEST(AllChunkGlobalLumpsChunkedDocAllLumps) {
        TTestData testData = GenerateBaseData(4, true);
        std::array<ui32, 4> chunkDocIdPtr = {{}};
        std::mt19937 rnd(4243);
        for (ui32 docId = 0; docId < 53; ++docId) {
            testData.DocChunkedLumps.push_back(ChunkedDocLumps);
            ui32 chunk = rnd() % 4;
            testData.DocChunksMapping.emplace_back(chunk, chunkDocIdPtr[chunk]++);
        }
        CheckWad(testData);
    }

    Y_UNIT_TEST(AllChunkGlobalLumpsChunkedDocRandomLumps) {
        TTestData testData = GenerateBaseData(4, true);
        std::array<ui32, 4> chunkDocIdPtr = {{}};
        std::mt19937 rnd(4243);
        for (ui32 docId = 0; docId < 64; ++docId) {
            testData.DocChunkedLumps.emplace_back();
            for (size_t i = 0; i < ChunkedDocLumps.size(); ++i) {
                if (rnd() % 2 == 0) {
                    testData.DocChunkedLumps.back().push_back(ChunkedDocLumps[i]);
                }
            }
            ui32 chunk = rnd() % 4;
            testData.DocChunksMapping.emplace_back(chunk, chunkDocIdPtr[chunk]++);
        }
        CheckWad(testData);
    }

    Y_UNIT_TEST(AllChunkGlobalLumpsChunkedDocAllLumpsHoles) {
        TTestData testData = GenerateBaseData(2, true);
        std::array<ui32, 2> chunkDocIdPtr = {{}};
        std::mt19937 rnd(4243);
        for (ui32 docId = 0; docId < 6; ++docId) {
            testData.DocChunkedLumps.push_back(ChunkedDocLumps);
            ui32 chunk = rnd() % 2;
            testData.DocChunksMapping.emplace_back(chunk, chunkDocIdPtr[chunk]++);
        }
        CheckHoles(testData);
    }

}
