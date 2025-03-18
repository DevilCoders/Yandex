#include "array4d_poly.h"
#include "memory4d_poly.h"

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/buffer.h>
#include <util/generic/map.h>
#include <util/random/mersenne.h>
#include <util/random/random.h>
#include <util/stream/buffer.h>
#include <util/system/tempfile.h>

Y_UNIT_TEST_SUITE(TArray4DPolyTest) {
    class TRandomDataSet {
    private:
        typedef TArray4DPolyWriter TWriter;
        typedef TArray4DPoly TReader;
        typedef TArray4DPolyRowsWriter TRowWriter;
        typedef TMultiMap<size_t, TString> TTypeLevel;
        typedef TMap<size_t, TTypeLevel> TRegionLevel;
        typedef TMap<size_t, TRegionLevel> TBreakLevel;
        typedef TMap<size_t, TBreakLevel> TDocLevel;

        static const size_t MAX_DOCS = 100;
        static const size_t MAX_BREAKS = 20;
        static const size_t MAX_ANNOTATIONS = 20;
        static const size_t MAX_REGIONS = 20;
        static const size_t MAX_TYPES = 20;
        static const size_t MAX_LENGTH = 100;

        template <class TLevel>
        static bool HasKey() {
            return std::is_same<TLevel, TRegionLevel>::value || std::is_same<TLevel, TTypeLevel>::value;
        }

        const ui32 Seed;
        TDocLevel Data;
        const TString ErrorMessage;

        TReader::TTypeInfo RequiredTypes, CorruptedTypes;
        static const size_t InitialUnluckyType = MAX_TYPES + 1;
        size_t UnluckyType;

        static TString GenerateValue(TMersenne<ui32>& random) {
            TString result;
            TStringOutput output(result);
            size_t length = random.GenRand() % MAX_LENGTH + 1;
            for (size_t i = 0; i < length; i += sizeof(ui32)) {
                ui32 value = random.GenRand();
                size_t toWrite = Min<size_t>(length - i, sizeof(ui32));
                output.Write(&value, toWrite);
            }
            return result;
        }

        void CreateRandom(ui32 seed) {
            TDocLevel ret;
            TMersenne<ui32> random(seed);
            size_t numDocs = random.GenRand() % (MAX_DOCS + 1);
            size_t numRegions = random.GenRand() % MAX_REGIONS + 1;
            size_t numTypes = random.GenRand() % MAX_TYPES + 1;
            for (size_t docId = 0; docId < numDocs; ++docId) {
                ui32 numBreaks = random.GenRand() % (MAX_BREAKS + 1);
                if (!numBreaks) {
                    continue;
                }
                ui32 numAnnotations = random.GenRand() % (MAX_ANNOTATIONS + 1);
                for (size_t i = 0; i < numAnnotations; ++i) {
                    size_t breakId = random.GenRand() % numBreaks;
                    size_t regionId = random.GenRand() % numRegions;
                    size_t typeId = random.GenRand() % numTypes;
                    TString value = GenerateValue(random);
                    ret[docId][breakId][regionId].insert(std::make_pair(typeId, value));

                    TWriter::TData data(value.data(), value.size());
                    if (data.Length) {
                        size_t& length = RequiredTypes[typeId];
                        if (!length || length > data.Length) {
                            length = data.Length;
                        }
                    }
                    if (data.Length == 12) {
                        UnluckyType = typeId;
                    }
                }
            }

            Data = ret;

            // Do something weird with types. It must not affect other actions
            RequiredTypes[MAX_TYPES + 2] += 100; // Add unexisting type
            CorruptedTypes = RequiredTypes;
            CorruptedTypes[UnluckyType] = MAX_LENGTH + 10; // Causes polite mode to be required
        }

        TString MethodErrorMessage(const char* method, const char* levelName) const {
            return ErrorMessage + " For " + TString(levelName) + " level " + TString(method) + " method returns incorrect value!";
        }

        template <class TLevel>
        static size_t GetCount(const TLevel& level) {
            bool hasKey = HasKey<TLevel>();
            return hasKey ? level.size() : level.empty() ? 0 : level.rbegin()->first + 1;
        }

        template <class TIter>
        void CheckIter(const TIter& iter, size_t count, const char* levelName) const {
            UNIT_ASSERT_VALUES_EQUAL_C(iter.GetCount(), count, MethodErrorMessage("GetCount()", levelName));
        }

    public:
        TRandomDataSet()
            : Seed(RandomNumber<ui32>())
            , ErrorMessage(TStringBuilder() << "Error with seed=" << Seed << ": ")
            , UnluckyType(InitialUnluckyType)
        {
            CreateRandom(Seed);
        }

        TRandomDataSet(ui32 seed)
            : Seed(seed)
            , ErrorMessage(TStringBuilder() << "Error with seed=" << Seed << ": ")
            , UnluckyType(InitialUnluckyType)
        {
            CreateRandom(Seed);
        }

        class I4dArrayView {
        public:
            virtual TArray4DPoly::TElementsLayer GetBreakLevel(size_t docId) const = 0;
        };

        class T4dArrayPolyView: public I4dArrayView {
        private:
            TReader Reader;

        public:
            T4dArrayPolyView(const TString& fileName, const TReader::TTypeInfo& currentTypes, bool polite) {
                Reader.Load(fileName, currentTypes, polite, /*quiet*/ true);
            }
            T4dArrayPolyView(const TBlob& data, const TString& debugName, const TReader::TTypeInfo& currentTypes, bool polite) {
                Reader.Load(data, debugName, currentTypes, polite, /*quiet*/ true);
            }
            TArray4DPoly::TElementsLayer GetBreakLevel(size_t docId) const override {
                return Reader.GetSubLayer(docId);
            }
            const TReader& GetTopIter() const {
                return Reader;
            }
        };

        class T4dArrayMemoryView: public I4dArrayView {
        private:
            const TMemory4DArray& MemArray;

        public:
            T4dArrayMemoryView(const TMemory4DArray& memArray)
                : MemArray(memArray)
            {
            }
            TArray4DPoly::TElementsLayer GetBreakLevel(size_t docId) const override {
                return MemArray.GetRow(docId);
            }
        };

        class I4dArrayWriter {
        public:
            virtual void AddDoc(ui32 docId, ui32 breakId, ui32 regionId, ui32 typeId, const TWriter::TData& data) = 0;
            virtual void FinalizeRow(ui32 docId) = 0;
        };

        class T4dArrayPolyWriter: public I4dArrayWriter {
        private:
            TWriter Writer;

        public:
            T4dArrayPolyWriter(const TString& fileName)
                : Writer(fileName)
            {
            }
            T4dArrayPolyWriter(IOutputStream& stream)
                : Writer(stream)
            {
            }
            void AddDoc(ui32 docId, ui32 breakId, ui32 regionId, ui32 typeId, const TWriter::TData& data) override {
                Writer.Add(docId, breakId, regionId, typeId, data);
            }
            void FinalizeRow(ui32 /*docId*/) override {
            }
        };

        class T4dArrayMemoryWriter: public I4dArrayWriter {
        private:
            TMemory4DArray& MemArray;
            TRowWriter RowWriter;

        public:
            T4dArrayMemoryWriter(TMemory4DArray& memArray, const TReader::TTypeInfo& requiredTypes)
                : MemArray(memArray)
                , RowWriter(requiredTypes)
            {
            }
            void AddDoc(ui32 /*docId*/, ui32 breakId, ui32 regionId, ui32 typeId, const TWriter::TData& data) override {
                RowWriter.Add(breakId, regionId, typeId, data);
            }
            void FinalizeRow(ui32 docId) override {
                TBuffer rowBuffer;
                char layerJump;
                RowWriter.Finalize(rowBuffer, layerJump);
                MemArray.SetRow(docId, rowBuffer, layerJump);
            }
        };

        void Write4dArray(I4dArrayWriter* writer) const {
            for (const auto& docIter : Data) {
                size_t docId = docIter.first;
                const TBreakLevel& breakLevel = docIter.second;
                for (const auto& breakIter : breakLevel) {
                    size_t breakId = breakIter.first;
                    const TRegionLevel& regionLevel = breakIter.second;
                    for (const auto& regionIter : regionLevel) {
                        size_t regionId = regionIter.first;
                        const TTypeLevel& typeLevel = regionIter.second;
                        for (const auto& typeIter : typeLevel) {
                            size_t typeId = typeIter.first;
                            const TString& data = typeIter.second;
                            writer->AddDoc(docId, breakId, regionId, typeId, TWriter::TData(data.data(), data.size()));
                        }
                    }
                }
                writer->FinalizeRow(docId);
            }
        }

        void Verify4dArray(I4dArrayView& arr, bool changed, const TReader::TTypeInfo& currentTypes) const {
            TDocLevel docLevel = Data;
            size_t numDocs = GetCount(docLevel);
            for (ui32 docId = 0; docId < numDocs; ++docId) {
                TBreakLevel& breaks = docLevel[docId];
                size_t numBreaks = GetCount(breaks);
                TArray4DPoly::TElementsLayer breakIdLayer = arr.GetBreakLevel(docId);
                CheckIter(breakIdLayer, numBreaks, "break");
                for (ui32 breakId = 0; breakId < numBreaks; ++breakId) {
                    TRegionLevel& regions = breaks[breakId];
                    TReader::TEntriesLayer regionIdLayer = breakIdLayer.GetSubLayer(breakId);
                    size_t numRegions = GetCount(regions);
                    CheckIter(regionIdLayer, numRegions, "region");
                    TRegionLevel::iterator regionIt = regions.begin();
                    for (ui32 regionId = 0; regionId < numRegions; ++regionId, ++regionIt) {
                        {
                            const auto& region = regionIt->first;
                            size_t pos;
                            UNIT_ASSERT(regionIdLayer.TryFindPosByKey(0, regionIdLayer.GetCount(), region, pos));
                            UNIT_ASSERT_EQUAL(regionIdLayer.GetKey(pos), region);
                        }
                        TTypeLevel& types = regionIt->second;
                        TReader::TItemsLayer typeIdLayer = regionIdLayer.GetSubLayer(regionId);
                        size_t numTypes = GetCount(types);
                        CheckIter(typeIdLayer, numTypes, "type");
                        TTypeLevel::iterator typeIt = types.begin();
                        for (ui32 typeId = 0; typeId < numTypes; ++typeId, ++typeIt) {
                            size_t type = typeIt->first;
                            TString& originalData = typeIt->second;
                            if (changed) {
                                TReader::TTypeInfo::const_iterator typeInfoIt = currentTypes.find(type);
                                if (typeInfoIt != currentTypes.end() && originalData.size() < typeInfoIt->second) {
                                    originalData.resize(typeInfoIt->second, '\0');
                                }
                            }
                            TWriter::TData data = typeIdLayer.GetData(typeId);
                            TString dataStr(data.Start, Min<size_t>(data.Length, originalData.size()));
                            UNIT_ASSERT_VALUES_EQUAL_C(dataStr, originalData, MethodErrorMessage("GetCurrentData()", "type"));
                        }
                    }
                }
            }
        }

        void Test() const {
            // Allocate temporary file
            TTempFile tempFile("array4d_poly_ut.tmp.file");

            // Write
            {
                T4dArrayPolyWriter writer(tempFile.Name());
                Write4dArray(&writer);
            }

            // Check to fail in non polite mode
            if (UnluckyType != InitialUnluckyType) {
                bool exceptionHappened = false;
                try {
                    TReader reader;
                    reader.Load(tempFile.Name().data(), CorruptedTypes, /*polite=*/false, /*quiet*/ true);
                } catch (const yexception&) {
                    exceptionHappened = true;
                }
                UNIT_ASSERT_C(exceptionHappened, ErrorMessage + "Exception not happened in non polite mode!");
            }

            // Check to not fail in polite mode
            if (UnluckyType != InitialUnluckyType) {
                try {
                    TReader reader;
                    reader.Load(tempFile.Name().data(), CorruptedTypes, /*polite=*/true, /*quiet*/ true);
                } catch (const yexception& e) {
                    UNIT_ASSERT_C(false, ErrorMessage + "Exception happened in polite mode: " + e.what());
                }
            }

            // Read and check
            for (size_t mode = 0; mode < 3; ++mode) {
                bool changed = mode == 2;
                bool polite = (bool)mode;
                const TReader::TTypeInfo& currentTypes = changed ? CorruptedTypes : RequiredTypes;

                T4dArrayPolyView arrView(tempFile.Name(), currentTypes, (bool)polite);
                CheckIter(arrView.GetTopIter(), GetCount(Data), "doc");
                Verify4dArray(arrView, changed, currentTypes);
            }
        }

        void TestMemoryImpl(const TReader::TTypeInfo& typesInfo) const {
            TReader::TTypeInfo emptyTypes;
            TMemory4DArray memArray(MAX_DOCS);

            // Write
            {
                T4dArrayMemoryWriter memWriter(memArray, typesInfo);
                Write4dArray(&memWriter);
            }

            // Read and check
            T4dArrayMemoryView memView(memArray);
            Verify4dArray(memView, true, typesInfo);
        }

        void TestMemory() const {
            TestMemoryImpl(RequiredTypes);
            TestMemoryImpl(CorruptedTypes);
        }

        void TestBufferImpl(const TReader::TTypeInfo& typesInfo, bool polite) const {
            TBuffer data;
            {
                TBufferOutput out(data);
                T4dArrayPolyWriter writer(out);
                Write4dArray(&writer);
            }
            T4dArrayPolyView arrView(TBlob::FromBuffer(data), "buffer test", typesInfo, polite);
            Verify4dArray(arrView, true, typesInfo);
        }

        void TestBuffer() const {
            TestBufferImpl(RequiredTypes, false);
            TestBufferImpl(CorruptedTypes, true);
        }
    };

    Y_UNIT_TEST(TestArray4DPolyRandom) {
        ui32 seeds[] = {4144315448, 3624831857, 1487361857, 701539876};
        for (auto seed : seeds) {
            TRandomDataSet data(seed);
            data.Test();
        }
    }

    Y_UNIT_TEST(TestArray4DPolyMemoryRandom) {
        ui32 seeds[] = {4144315448, 3624831857, 1487361857, 701539876};
        for (auto seed : seeds) {
            TRandomDataSet data(seed);
            data.TestMemory();
        }
    }

    Y_UNIT_TEST(TestArray4DPolyBufferRandom) {
        ui32 seeds[] = {4144315448, 3624831857, 1487361857, 701539876};
        for (auto seed : seeds) {
            TRandomDataSet data(seed);
            data.TestBuffer();
        }
    }
};
