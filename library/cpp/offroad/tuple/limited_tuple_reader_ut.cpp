#include <util/random/shuffle.h>
#include <library/cpp/testing/unittest/registar.h>

#include "limited_tuple_reader.h"
#include "limited_tuple_sub_reader.h"
#include "tuple_reader.h"
#include "tuple_sampler.h"
#include "tuple_writer.h"

#include <library/cpp/offroad/custom/subtractors.h>
#include <library/cpp/offroad/custom/ui32_vectorizer.h>

#include <library/cpp/offroad/sub/sub_sampler.h>
#include <library/cpp/offroad/sub/sub_writer.h>

#include <util/generic/buffer.h>
#include <util/stream/buffer.h>

#include <random>

using namespace NOffroad;

Y_UNIT_TEST_SUITE(TestLimitedTupleReader) {
    Y_UNIT_TEST(ReadHits) {
        std::minstd_rand rand(4243);
        const size_t blockSize = 64;
        const size_t blocksCount = 3;
        for (size_t lastBlockSize = 1; lastBlockSize <= blockSize; ++lastBlockSize) {
            size_t totalSize = (blocksCount - 1) * blockSize + lastBlockSize;
            THashSet<ui32> elemsSet;
            while (elemsSet.size() < totalSize) {
                elemsSet.insert(1 + rand() % 424243);
            }
            TVector<ui32> elems(elemsSet.begin(), elemsSet.end());
            Sort(elems);

            using TSampler = TTupleSampler<ui32, TUi32Vectorizer, TI1Subtractor>;
            using TModel = TSampler::TModel;
            TSampler sampler;
            for (ui32 elem : elems) {
                sampler.WriteHit(elem);
            }
            TModel model = sampler.Finish();

            using TWriter = TTupleWriter<ui32, TUi32Vectorizer, TI1Subtractor>;
            using TWriterTable = TWriter::TTable;
            THolder<TWriterTable> writerTable = MakeHolder<TWriterTable>();
            writerTable->Reset(model);
            TBufferStream stream;
            TWriter writer(writerTable.Get(), &stream);

            TVector<ui64> blockOffsets;
            TVector<ui32> startElems;
            ui32 lastElem = 0;

            for (ui32 elem : elems) {
                if (writer.Position().Index() == 0) {
                    blockOffsets.push_back(writer.Position().Offset());
                    startElems.push_back(lastElem);
                }
                writer.WriteHit(elem);
                lastElem = elem;
            }
            writer.Finish();

            Y_VERIFY(blockOffsets.size() == blocksCount && startElems.size() == blocksCount);

            using TReader = TLimitedTupleReader<ui32, TUi32Vectorizer, TI1Subtractor>;
            UNIT_ASSERT_VALUES_EQUAL(blockSize, static_cast<size_t>(TReader::BlockSize));

            using TReaderTable = TReader::TTable;
            THolder<TReaderTable> readerTable = MakeHolder<TReaderTable>();
            readerTable->Reset(model);
            TReader reader(readerTable.Get(), TArrayRef<const char>(stream.Buffer().data(), stream.Buffer().size()));

            for (size_t startBlock = 0; startBlock < blocksCount; ++startBlock) {
                size_t startBlockSize = (startBlock + 1 < blocksCount ? blockSize : lastBlockSize);
                for (size_t endBlock = startBlock; endBlock < blocksCount; ++endBlock) {
                    size_t endBlockSize = (endBlock + 1 < blocksCount ? blockSize : lastBlockSize);
                    for (size_t startBlockElem = 0; startBlockElem < startBlockSize; ++startBlockElem) {
                        for (size_t endBlockElem = (startBlock == endBlock ? startBlockElem : 0); endBlockElem < endBlockSize; ++endBlockElem) {
                            TDataOffset startOffset(blockOffsets[startBlock], startBlockElem);
                            bool seekSuccess = reader.Seek(startOffset, startElems[startBlock]);
                            Y_VERIFY(seekSuccess);

                            TDataOffset endOffset;
                            if (endBlockElem + 1 < endBlockSize) {
                                endOffset = TDataOffset(blockOffsets[endBlock], endBlockElem + 1);
                            } else if (endBlock + 1 < blocksCount) {
                                endOffset = TDataOffset(blockOffsets[endBlock + 1], 0);
                            } else {
                                endOffset = TDataOffset::Max();
                            }
                            reader.SetLimits(startOffset, endOffset);

                            TVector<ui32> expected;
                            if (startBlock == endBlock) {
                                for (size_t i = startBlockElem; i <= endBlockElem; ++i) {
                                    expected.push_back(elems[startBlock * blockSize + i]);
                                }
                            } else {
                                for (size_t i = startBlockElem; i < startBlockSize; ++i) {
                                    expected.push_back(elems[startBlock * blockSize + i]);
                                }
                                for (size_t i = startBlock + 1; i < endBlock; ++i) {
                                    for (size_t j = 0; j < blockSize; ++j) {
                                        expected.push_back(elems[i * blockSize + j]);
                                    }
                                }
                                for (size_t i = 0; i <= endBlockElem; ++i) {
                                    expected.push_back(elems[endBlock * blockSize + i]);
                                }
                            }

                            TVector<ui32> actual;
                            auto consumer = [&](ui32 x) {
                                actual.push_back(x);
                                return true;
                            };
                            while (reader.ReadHits(consumer))
                                ;

                            UNIT_ASSERT_VALUES_EQUAL(expected, actual);
                        }
                    }
                }
            }
        }
    }

    struct THit {
        ui32 DocId = 0;
        ui32 Break = 0;
        ui32 Word = 0;

        THit() = default;

        THit(ui32 docId, ui32 breuk, ui32 word)
            : DocId(docId)
            , Break(breuk)
            , Word(word)
        {
        }

        friend bool operator<(const THit& l, const THit& r) {
            return std::tie(l.DocId, l.Break, l.Word) < std::tie(r.DocId, r.Break, r.Word);
        }

        friend bool operator>=(const THit& l, const THit& r) {
            return std::tie(l.DocId, l.Break, l.Word) >= std::tie(r.DocId, r.Break, r.Word);
        }

        friend bool operator!=(const THit& l, const THit& r) {
            return std::tie(l.DocId, l.Break, l.Word) != std::tie(r.DocId, r.Break, r.Word);
        }

        friend bool operator==(const THit& l, const THit& r) {
            return std::tie(l.DocId, l.Break, l.Word) == std::tie(r.DocId, r.Break, r.Word);
        }
    };

    struct THitVectorizer {
        enum {
            TupleSize = 3
        };

        template <class Slice>
        static void Scatter(const THit& hit, Slice&& slice) {
            slice[0] = hit.DocId;
            slice[1] = hit.Break;
            slice[2] = hit.Word;
        }

        template <class Slice>
        static void Gather(Slice&& slice, THit* hit) {
            *hit = THit(slice[0], slice[1], slice[2]);
        }
    };

    struct THitPrefixVectorizer {
        enum {
            TupleSize = 1
        };

        template <class Slice>
        static void Scatter(const THit& hit, Slice&& slice) {
            slice[0] = hit.DocId;
        }

        template <class Slice>
        static void Gather(Slice&& slice, THit* hit) {
            *hit = THit(slice[0], 0, 0);
        }
    };

    struct THitSubtractor {
        enum {
            TupleSize = 3,
            PrefixSize = 1
        };

        template <class Storage>
        static void Integrate(Storage&& storage) {
            Integrate1(storage.Chunk(0));
        }

        template <class Value, class Delta, class Next>
        Y_FORCE_INLINE static void Integrate(Value&& value, Delta&& delta, Next&& next) {
            if (value[0] != delta[0]) {
                next.Assign(delta);
                return;
            }
            next[0] = value[0];
            next[1] = value[1] + delta[1];
            if (delta[1]) {
                next[2] = delta[2];
                return;
            }
            next[2] = value[2] + delta[2];
        }

        template <class Value, class Delta, class Next>
        Y_FORCE_INLINE static void Differentiate(Value&& value, Next&& next, Delta&& delta) {
            delta[0] = next[0] - value[0];
            if (delta[0]) {
                delta[1] = next[1];
                delta[2] = next[2];
                return;
            }
            delta[1] = next[1] - value[1];
            if (delta[1]) {
                delta[2] = next[2];
                return;
            }
            delta[2] = next[2] - value[2];
        }
    };

    TVector<THit> GenerateAllHits() {
        TVector<THit> hits(10 * 10 * 10);
        size_t ptr = 0;
        for (ui32 docId = 0; docId < 10; ++docId) {
            for (ui32 breuk = 0; breuk < 10; ++breuk) {
                for (ui32 word = 0; word < 10; ++word) {
                    Y_VERIFY(ptr < hits.size());
                    hits[ptr++] = THit(docId, breuk, word);
                }
            }
        }
        return hits;
    }

    static const TVector<THit> AllHits = GenerateAllHits();

    template <class T>
    void Shuffle(std::minstd_rand & rand, TVector<T> & hits) {
        struct TRng {
            inline auto Uniform(size_t n) {
                return Rand() % n;
            }

            std::minstd_rand& Rand;
        } rng{rand};

        ::Shuffle(hits.begin(), hits.end(), rng);
    }

    TVector<THit> GenerateHits(std::minstd_rand & rand, size_t begin, size_t end) {
        size_t count = end - begin;
        Y_VERIFY(count <= AllHits.size());
        TVector<THit> hits = AllHits;
        Shuffle(rand, hits);
        hits.resize(count);
        Sort(hits);
        return hits;
    }

    template <class Writer, size_t blockSize, bool returnOffsets>
    std::pair<TTupleSubOffset, TTupleSubOffset> WriteHits(Writer * writer, const TVector<THit>& prev, const TVector<THit>& cur, const TVector<THit>& next) {
        ui32 subIndex = 0;
        ui32 hitIndex = 0;
        for (const THit& hit : prev) {
            writer->WriteHit(hit);
            if (++hitIndex % blockSize == 0) {
                ++subIndex;
            }
        }
        writer->WriteSeekPoint();

        TTupleSubOffset start;
        if (returnOffsets) {
            start = writer->Position();
        }

        for (const THit& hit : cur) {
            writer->WriteHit(hit);
            if (++hitIndex % blockSize == 0) {
                ++subIndex;
            }
        }
        writer->WriteSeekPoint();

        TTupleSubOffset end;
        if (returnOffsets) {
            end = writer->Position();
        }

        for (const THit& hit : next) {
            writer->WriteHit(hit);
        }
        writer->WriteSeekPoint();

        return std::make_pair(start, end);
    }

    void AddPrefixesWithOffset(const TVector<THit>& hits, i32 maxOffset, TVector<THit>* prefixes) {
        for (const THit& hit : hits) {
            for (i32 offset = -maxOffset; offset <= maxOffset; ++offset) {
                if (i32(hit.DocId) + offset >= 0) {
                    prefixes->push_back(THit(i32(hit.DocId) + offset, 0, 0));
                }
            }
        }
    }

    template <class HitGenerator>
    void SubTest(HitGenerator && hitGenerator) {
        using TSampler = TSubSampler<THitPrefixVectorizer, TTupleSampler<THit, THitVectorizer, THitSubtractor>>;
        using TModel = TSampler::TModel;
        using TWriter = TSubWriter<THitPrefixVectorizer, TTupleWriter<THit, THitVectorizer, THitSubtractor>>;
        using TWriterTable = typename TWriter::TTable;
        using TReader = TLimitedTupleSubReader<THitPrefixVectorizer, TLimitedTupleReader<THit, THitVectorizer, THitSubtractor, TDecoder64, 1, PlainOldBuffer>>;
        using TReaderTable = TReader::TTable;

        static constexpr size_t blockSize = TWriter::BlockSize;
        static constexpr size_t blocksCount = 4;
        std::minstd_rand rand(4243);

        for (size_t startBlock = 0; startBlock < blocksCount; ++startBlock) {
            for (size_t startPos = 0; startPos < blockSize; ++startPos) {
                for (size_t endBlock = startBlock; endBlock < blocksCount; ++endBlock) {
                    for (size_t endPos = (startBlock == endBlock ? startPos : 0); endPos < blockSize; ++endPos) {
                        const size_t firstHitNum = startBlock * blockSize + startPos;
                        const size_t lastHitNum = endBlock * blockSize + endPos;
                        TVector<THit> prev = hitGenerator(rand, 0, firstHitNum);
                        TVector<THit> cur = hitGenerator(rand, firstHitNum, lastHitNum + 1);
                        TVector<THit> next = hitGenerator(rand, lastHitNum + 1, blocksCount * blockSize);

                        TSampler sampler;
                        WriteHits<TSampler, blockSize, false>(&sampler, prev, cur, next);
                        TModel model = sampler.Finish();

                        THolder<TWriterTable> writerTable = MakeHolder<TWriterTable>();
                        writerTable->Reset(model);

                        TBufferStream sub;
                        TBufferStream out;
                        TWriter writer(&sub, writerTable.Get(), &out);

                        TTupleSubOffset start;
                        TTupleSubOffset end;
                        std::tie(start, end) = WriteHits<TWriter, blockSize, true>(&writer, prev, cur, next);

                        writer.Finish();

                        THolder<TReaderTable> readerTable = MakeHolder<TReaderTable>();
                        readerTable->Reset(model);

                        TReader reader(TArrayRef<const char>(sub.Buffer().Data(), sub.Buffer().Size()), readerTable.Get(), TArrayRef<const char>(out.Buffer().Data(), out.Buffer().Size()));

                        UNIT_ASSERT(reader.Seek(start, THit(), TSeekPointSeek()));

                        reader.SetLimits(start, end);

                        TVector<THit> prefixes;
                        AddPrefixesWithOffset(prev, 2, &prefixes);
                        AddPrefixesWithOffset(cur, 2, &prefixes);
                        AddPrefixesWithOffset(next, 2, &prefixes);
                        prefixes.push_back(THit(0, 0, 0));
                        prefixes.push_back(THit(100500, 0, 0));
                        Shuffle(rand, prefixes);

                        for (const THit& prefix : prefixes) {
                            size_t pos = LowerBound(cur.begin(), cur.end(), prefix) - cur.begin();
                            THit first;
                            if (pos >= cur.size()) {
                                UNIT_ASSERT(!reader.LowerBound(prefix, &first));
                            } else {
                                UNIT_ASSERT(reader.LowerBound(prefix, &first));
                                UNIT_ASSERT(first == cur[pos]);
                                for (size_t it = 0; it < 10; ++it) {
                                    if (reader.ReadHit(&first)) {
                                        UNIT_ASSERT(pos < cur.size());
                                        UNIT_ASSERT_EQUAL(cur[pos], first);
                                        ++pos;
                                    } else {
                                        UNIT_ASSERT(pos >= cur.size());
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    Y_UNIT_TEST(Sub) {
        SubTest(GenerateHits);
    }

    TVector<THit> GenerateRepetetiveHits(std::minstd_rand & rand, size_t begin, size_t end) {
        static int iteration = 0;
        static int shift = rand() % 1467;
        if (++iteration == 3) {
            iteration = 0;
            shift = rand() % 1467;
        }
        TVector<THit> hits;
        hits.reserve(end - begin);
        for (size_t i = begin + shift; i < end + shift; ++i) {
            hits.push_back(THit(i / 100, i / 2, i));
        }
        return hits;
    }

    Y_UNIT_TEST(SubUnique) {
        SubTest(GenerateRepetetiveHits);
    }

    Y_UNIT_TEST(SubSeek) {
        using TSampler = TTupleSampler<THit, THitVectorizer, THitSubtractor>;
        using TModel = TSampler::TModel;
        using TWriter = TTupleWriter<THit, THitVectorizer, THitSubtractor>;
        using TWriterTable = TWriter::TTable;
        using TReader = TLimitedTupleReader<THit, THitVectorizer, THitSubtractor, TDecoder64, 1, PlainOldBuffer>;
        using TReaderTable = TReader::TTable;

        static constexpr size_t blockSize = TWriter::BlockSize;

        UNIT_ASSERT_EQUAL(64, blockSize);

        static const TVector<size_t> sizes = {2, 1, 3, 2, 4, 10, 3, 3, 1, 2, 31, 1, 1};
        UNIT_ASSERT_EQUAL(64, Accumulate(sizes, static_cast<size_t>(0)));

        std::minstd_rand rand(4243);

        TVector<TVector<THit>> hits;
        for (size_t size : sizes) {
            hits.push_back(GenerateHits(rand, 0, size));
        }

        TSampler sampler;
        for (const TVector<THit>& x : hits) {
            for (const THit& y : x) {
                sampler.WriteHit(y);
            }
            sampler.WriteSeekPoint();
        }

        TModel model = sampler.Finish();

        THolder<TWriterTable> writerTable = MakeHolder<TWriterTable>();
        writerTable->Reset(model);

        TBufferStream out;
        TWriter writer(writerTable.Get(), &out);

        TVector<TDataOffset> offsets;
        for (const TVector<THit>& x : hits) {
            for (const THit& y : x) {
                writer.WriteHit(y);
            }
            writer.WriteSeekPoint();
            offsets.push_back(writer.Position());
        }
        writer.Finish();

        THolder<TReaderTable> readerTable = MakeHolder<TReaderTable>();
        readerTable->Reset(model);

        TReader reader(readerTable.Get(), TArrayRef<const char>(out.Buffer().Data(), out.Buffer().Size()));

        TVector<size_t> blocks(sizes.size());
        Iota(blocks.begin(), blocks.end(), 0);
        Shuffle(rand, blocks);

        for (size_t block : blocks) {
            TDataOffset start;
            TDataOffset end;
            if (block > 0) {
                start = offsets[block - 1];
                end = offsets[block];
            } else {
                start = TDataOffset();
                end = offsets[block];
            }
            UNIT_ASSERT(reader.Seek(start, THit(), TSeekPointSeek()));
            reader.SetLimits(start, end);
            THit hit;
            size_t pos = 0;
            while (reader.ReadHit(&hit)) {
                UNIT_ASSERT(pos < hits[block].size());
                UNIT_ASSERT_EQUAL(hits[block][pos], hit);
                ++pos;
            }
            UNIT_ASSERT(pos >= hits[block].size());
        }
    }
}
