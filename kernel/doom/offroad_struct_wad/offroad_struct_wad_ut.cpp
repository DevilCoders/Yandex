#include <library/cpp/testing/unittest/registar.h>

#include "offroad_struct_wad_ut.h"

Y_UNIT_TEST_SUITE(TOffroadStructWadTest) {
    template <EStructType structType, ECompressionType compressionType>
    void DoTest(ui32 sz, ui32 minDocId = 0, ui32 maxDocId = 0, ui32 docCount = 0) {
        using TSelectedIndex = TIndex<structType, compressionType>;
        using TReader = typename TSelectedIndex::TReader;
        using TSearcher = typename TSelectedIndex::TSearcher;
        using TReaderHit = typename TReader::THit;
        using TIterator = typename TSearcher::TIterator;
        using TSearcherHit = typename TSearcher::THit;

        TSelectedIndex index(sz, minDocId, maxDocId, docCount);
        // Read all
        {
            THolder<TReader> reader = index.GetReader();
            ui32 docId;
            TReaderHit hit;
            for (ui32 iter = 0; iter < 2; ++iter, reader->Reset(index.GetWad())) {
                for (const auto& docHit : index.GetData()) {
                    UNIT_ASSERT(reader->ReadDoc(&docId));
                    UNIT_ASSERT_VALUES_EQUAL(docId, docHit.first);
                    UNIT_ASSERT(reader->ReadHit(&hit));
                    UNIT_ASSERT(hit.compare(docHit.second) == 0);
                    UNIT_ASSERT(!reader->ReadHit(&hit));
                }
                UNIT_ASSERT(!reader->ReadDoc(&docId));
            }
        }
        // Search all
        {
            THolder<TSearcher> searcher = index.GetSearcher();
            TIterator it;
            TSearcherHit hit;
            const typename TSelectedIndex::TData& testData = index.GetData();
            const ui32 FIRST = sz ? testData[0].first : 0;
            const ui32 LAST = sz ? testData.back().first : 0;
            auto dataIter = testData.begin();
            TVector <std::pair<ui32, ui32> > val;
            for (ui32 i = FIRST; i <= LAST + 1; ++i) {
                while (dataIter != testData.end() && i > dataIter->first) {
                    ++dataIter;
                }
                val.push_back({
                    i, (dataIter != testData.end() && dataIter->first == i) ? dataIter - testData.begin() : Max<ui32>()
                });
            }
            Shuffle(val.begin(), val.end());

            for (const auto& lr : val) {
                if (lr.second == Max<ui32>()) {
                    UNIT_ASSERT(!searcher->Find(lr.first, &it));
                } else {
                    UNIT_ASSERT(searcher->Find(lr.first, &it));
                    UNIT_ASSERT(it.ReadHit(&hit));
                    UNIT_ASSERT(hit.compare(testData[lr.second].second) == 0);
                    UNIT_ASSERT(!it.ReadHit(&hit));
                }
            }
        }
    }

    // OffroadCompressionType, AutoEofStructType
    Y_UNIT_TEST(TinyTestsAutoEofStructType) {
        for (ui32 docCount = 0; docCount < 5; ++docCount) {
            for (ui32 iter = 0; iter < 60; ++iter) {
                DoTest<AutoEofStructType, OffroadCompressionType>(docCount);
            }
        }
    }

    Y_UNIT_TEST(GeneralBigTestAutoEofStructType) {
        DoTest<AutoEofStructType, OffroadCompressionType>(7000);
    }

    Y_UNIT_TEST(GenerateAllDocIDsAutoEofStructType) {
        ui32 sz = 30;
        ui32 docs = 20000;
        DoTest<AutoEofStructType, OffroadCompressionType>(sz, 0, docs - 1, docs);
    }

    // OffroadCompressionType, FixedSizeStructType
    Y_UNIT_TEST(TinyTestsFixedSizeStructType) {
        for (ui32 docCount = 0; docCount < 5; ++docCount) {
            for (ui32 iter = 0; iter < 60; ++iter) {
                DoTest<FixedSizeStructType, OffroadCompressionType>(docCount);
            }
        }
    }

    Y_UNIT_TEST(GeneralBigTestFixedStructType) {
        DoTest<FixedSizeStructType, OffroadCompressionType>(7000);
    }

    Y_UNIT_TEST(GenerateAllDocIDsFixedSizeStructType) {
        ui32 sz = 30;
        ui32 docs = 20000;
        DoTest<FixedSizeStructType, OffroadCompressionType>(sz, 0, docs - 1, docs);
    }

    // OffroadCompressionType, VariableSizeStructType
    Y_UNIT_TEST(TinyTestsVariableSizeStructType) {
        for (ui32 docCount = 0; docCount < 5; ++docCount) {
            for (ui32 iter = 0; iter < 60; ++iter) {
                DoTest<VariableSizeStructType, OffroadCompressionType>(docCount);
            }
        }
    }

    Y_UNIT_TEST(GeneralBigTestVariableSizeStructType) {
        DoTest<VariableSizeStructType, OffroadCompressionType>(7000);
    }

    Y_UNIT_TEST(GenerateAllDocIDsVariableSizeStructType) {
        ui32 sz = 30;
        ui32 docs = 20000;
        DoTest<VariableSizeStructType, OffroadCompressionType>(sz, 0, docs - 1, docs);
    }


    // RawCompressionType, AutoEofStructType
    Y_UNIT_TEST(TinyTestsAutoEofStructTypeRaw) {
        for (ui32 docCount = 0; docCount < 5; ++docCount) {
            for (ui32 iter = 0; iter < 60; ++iter) {
                DoTest<AutoEofStructType, RawCompressionType>(docCount);
            }
        }
    }

    Y_UNIT_TEST(GeneralBigTestAutoEofStructTypeRaw) {
        DoTest<AutoEofStructType, RawCompressionType>(7000);
    }

    Y_UNIT_TEST(GenerateAllDocIDsAutoEofStructTypeRaw) {
        ui32 sz = 30;
        ui32 docs = 20000;
        DoTest<AutoEofStructType, RawCompressionType>(sz, 0, docs - 1, docs);
    }

    // RawCompressionType, FixedSizeStructType
    Y_UNIT_TEST(TinyTestsFixedSizeStructTypeRaw) {
        for (ui32 docCount = 0; docCount < 5; ++docCount) {
            for (ui32 iter = 0; iter < 60; ++iter) {
                DoTest<FixedSizeStructType, RawCompressionType>(docCount);
            }
        }
    }

    Y_UNIT_TEST(GeneralBigTestFixedStructTypeRaw) {
        DoTest<FixedSizeStructType, RawCompressionType>(7000);
    }

    Y_UNIT_TEST(GenerateAllDocIDsFixedSizeStructTypeRaw) {
        ui32 sz = 30;
        ui32 docs = 20000;
        DoTest<FixedSizeStructType, RawCompressionType>(sz, 0, docs - 1, docs);
    }

    // RawCompressionType, VariableSizeStructType
    Y_UNIT_TEST(TinyTestsVariableSizeStructTypeRaw) {
        for (ui32 docCount = 0; docCount < 5; ++docCount) {
            for (ui32 iter = 0; iter < 60; ++iter) {
                DoTest<VariableSizeStructType, RawCompressionType>(docCount);
            }
        }
    }

    Y_UNIT_TEST(GeneralBigTestVariableSizeStructTypeRaw) {
        DoTest<VariableSizeStructType, RawCompressionType>(7000);
    }

    Y_UNIT_TEST(GenerateAllDocIDsVariableSizeStructTypeRaw) {
        ui32 sz = 30;
        ui32 docs = 20000;
        DoTest<VariableSizeStructType, RawCompressionType>(sz, 0, docs - 1, docs);
    }

    struct TTestDataSmall {
        ui32 Field1 = 0;

        TTestDataSmall(){}

        TTestDataSmall(ui32 f1) : Field1(f1) {}

        static TTestDataSmall Generate() {
            return TTestDataSmall(RandomNumber<ui32>());
        }

        bool operator == (const TTestDataSmall& other) {
            return Field1 == other.Field1;
        }
    };

    struct TTestDataBig : TTestDataSmall {
        ui32 Field2 = 0;

        bool operator == (const TTestDataSmall& other) {
            return Field1 == other.Field1 && Field2 == 0;
        }
    };

    template<class Data>
    struct TTestDataSerializer {
        static void Deserialize(const TArrayRef<const char>& serialized, Data* data) {
            *data = ReadUnaligned<Data>(serialized.data());
        }

        static ui32 Serialize(const Data& data, IOutputStream* stream) {
            stream->Write(&data, sizeof(Data));
            return sizeof(Data);
        }

        static constexpr ui32 DataSize = sizeof(Data);
    };
    template <ECompressionType compressionType>
    void DoCompatibilityTest() {
        using IOSmall = TOffroadStructWadIo<OmniUrlType, TTestDataSmall, TTestDataSerializer<TTestDataSmall>, FixedSizeStructType, compressionType>;
        using IOBig = TOffroadStructWadIo<OmniUrlType, TTestDataBig, TTestDataSerializer<TTestDataBig>, FixedSizeStructType, compressionType>;
        using TWriterSmall  = typename IOSmall::TWriter;
        using TReaderSmall  = typename IOSmall::TReader;
        using TReaderBig = typename IOBig::TReader;
        using TSamplerSmall = typename IOSmall::TSampler;
        TVector<std::pair<ui32, TTestDataSmall>> testData(1000);
        for (ui32 doc = 0; doc < 1000; doc++) {
            testData[doc] = {doc, TTestDataSmall::Generate()};
        }
        TBuffer buffer;
        // Sample all
        {
            typename TSamplerSmall::TModel model;
            {
                TSamplerSmall sampler;
                for (const auto& docHit : testData) {
                    sampler.WriteHit(docHit.second);
                    sampler.WriteDoc(docHit.first);
                }
                model = sampler.Finish();
            }
        // Write all
            TMegaWadBufferWriter megaWadWriter(&buffer);
            TWriterSmall writer(model, &megaWadWriter);
            for (const auto& docHit : testData) {
                writer.WriteHit(docHit.second);
                writer.WriteDoc(docHit.first);
            }
            writer.Finish();
            megaWadWriter.Finish();
        }
        // Read small
        {
            TMegaWad wad(TArrayRef<const char>(buffer.data(), buffer.size()));
            TReaderSmall reader(&wad);
            ui32 docId;
            typename TReaderSmall::THit hit;
            for (ui32 iter = 0; iter < 2; ++iter, reader.Reset(&wad)) {
                for (const auto& docHit : testData) {
                    UNIT_ASSERT(reader.ReadDoc(&docId));
                    UNIT_ASSERT_VALUES_EQUAL(docId, docHit.first);
                    UNIT_ASSERT(reader.ReadHit(&hit));
                    UNIT_ASSERT(hit == docHit.second);
                    UNIT_ASSERT(!reader.ReadHit(&hit));
                }
                UNIT_ASSERT(!reader.ReadDoc(&docId));
            }
        }
        // Read big
        {
            TMegaWad wad(TArrayRef<const char>(buffer.data(), buffer.size()));
            TReaderBig reader(&wad);
            ui32 docId;
            typename TReaderBig::THit hit;
            for (ui32 iter = 0; iter < 2; ++iter, reader.Reset(&wad)) {
                for (const auto& docHit : testData) {
                    UNIT_ASSERT(reader.ReadDoc(&docId));
                    UNIT_ASSERT_VALUES_EQUAL(docId, docHit.first);
                    UNIT_ASSERT(reader.ReadHit(&hit));
                    UNIT_ASSERT(hit == docHit.second);
                    UNIT_ASSERT(!reader.ReadHit(&hit));
                }
                UNIT_ASSERT(!reader.ReadDoc(&docId));
            }
        }
    }

    Y_UNIT_TEST(ResetFixedSizeStructTestOffroad) {
        DoCompatibilityTest<OffroadCompressionType>();
    }
    Y_UNIT_TEST(ResetFixedSizeStructTestRaw) {
        DoCompatibilityTest<RawCompressionType>();
    }
}
