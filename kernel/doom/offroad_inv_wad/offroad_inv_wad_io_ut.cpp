#include <library/cpp/testing/unittest/registar.h>

#include <kernel/doom/offroad/attributes_hit_adaptors.h>
#include <kernel/doom/offroad/panther_hit_adaptors.h>
#include <kernel/doom/wad/mega_wad_writer.h>
#include <kernel/doom/wad/wad_index_type.h>

#include <library/cpp/digest/md5/md5.h>
#include <library/cpp/offroad/test/test_md5.h>

#include "offroad_inv_wad_io.h"

#include <random>

using namespace NDoom;

Y_UNIT_TEST_SUITE(TestOffroadInvWadIo) {

    template <class Position>
    using TRange = std::pair<Position, Position>;

    template <class Position>
    using TRanges = TVector<TRange<Position>>;

    template <class Hit>
    using THits = TVector<TVector<Hit>>;

    template <class Io>
    struct TIndex {
        using THit = typename Io::TWriter::THit;
        using TPosition = typename Io::TWriter::TPosition;

        THits<THit> Hits;
        TRanges<TPosition> Ranges;
    };

    template <class HitGenerator>
    THits<typename HitGenerator::THit> GenerateHits(ui32 seed, size_t size, HitGenerator&& hitGenerator) {
        using THit = typename HitGenerator::THit;

        std::minstd_rand rand(seed);

        THits<THit> res;
        res.reserve(size);

        for (size_t i = 0; i < size; ++i) {
            res.emplace_back();
            TVector<THit>& hits = res.back();
            const size_t count = 1 + rand() % 100;
            for (size_t j = 0; j < count; ++j) {
                hits.push_back(hitGenerator(rand));
            }
            Sort(hits);
            hits.erase(Unique(hits.begin(), hits.end()), hits.end());
        }

        return res;
    }

    template <class Writer>
    void Transfer(const THits<typename Writer::THit>& index, Writer* writer, TRanges<typename Writer::TPosition>* ranges = nullptr) {
        using THit = typename Writer::THit;
        using TPosition = typename Writer::TPosition;
        for (const TVector<THit>& hits : index) {
            TPosition start = writer->Position();
            for (const THit& hit : hits) {
                writer->WriteHit(hit);
            }
            TPosition end = writer->Position();
            if (ranges) {
                ranges->emplace_back(start, end);
            }
            writer->WriteSeekPoint();
        }
    }

    template <class Io>
    void WriteIndex(const THits<typename Io::TWriter::THit>& hits, TIndex<Io>* index, IOutputStream* output) {
        using TSampler = typename Io::TSampler;
        using TWriter = typename Io::TWriter;

        TSampler sampler;
        Transfer(hits, &sampler);
        auto model = sampler.Finish();

        index->Hits = hits;
        index->Ranges.reserve(hits.size());

        TMegaWadWriter megaWadWriter(output);
        TWriter writer(model, &megaWadWriter);
        Transfer(hits, &writer, &index->Ranges);
        writer.Finish();
        megaWadWriter.Finish();
    }

    template <class Io>
    void TestRead(const IWad* wad, const TIndex<Io>& index) {
        Y_VERIFY(index.Ranges.size() == index.Hits.size());

        using TReader = typename Io::TReader;
        using THit = typename TReader::THit;

        TReader reader(wad);

        for (size_t i = 0; i < index.Ranges.size(); ++i) {
            UNIT_ASSERT(reader.Seek(index.Ranges[i].first, index.Ranges[i].second));
            for (size_t j = 0; j < index.Hits[i].size(); ++j) {
                THit hit;
                UNIT_ASSERT(reader.ReadHit(&hit));
                UNIT_ASSERT_EQUAL(index.Hits[i][j], hit);
            }
            THit hit;
            UNIT_ASSERT(!reader.ReadHit(&hit));
        }
    }

    template <class Io>
    void TestSearcher(const IWad* wad, const TIndex<Io>& index) {
        Y_VERIFY(index.Ranges.size() == index.Hits.size());

        using TSearcher = typename Io::TSearcher;
        using TIterator = typename TSearcher::TIterator;
        using THit = typename TSearcher::THit;

        TSearcher searcher(wad);

        TVector<size_t> indexes(index.Ranges.size());
        Iota(indexes.begin(), indexes.end(), 0);

        for (size_t it = 0; it < 10; ++it) {
            std::shuffle(indexes.begin(), indexes.end(), std::minstd_rand(4243 + it * 42));

            for (size_t ind : indexes) {
                TIterator iterator;
                for (size_t it2 = 0; it2 < 2; ++it2) {
                    UNIT_ASSERT(searcher.Seek(index.Ranges[ind].first, index.Ranges[ind].second, &iterator));

                    for (size_t i = 0; i < index.Hits[ind].size(); ++i) {
                        THit hit;
                        UNIT_ASSERT(iterator.ReadHit(&hit));
                        UNIT_ASSERT_EQUAL(index.Hits[ind][i], hit);
                    }
                    THit hit;
                    UNIT_ASSERT(!iterator.ReadHit(&hit));
                }
            }
        }
    }

    template <class Io>
    void TestLowerBound(const IWad* wad, const TIndex<Io>& index, const THits<typename Io::TWriter::THit>& lowerBoundHits) {
        Y_VERIFY(index.Ranges.size() == index.Hits.size());
        Y_VERIFY(index.Hits.size() == lowerBoundHits.size());

        using TSearcher = typename Io::TSearcher;
        using TIterator = typename TSearcher::TIterator;
        using THit = typename TSearcher::THit;

        TSearcher searcher(wad);

        TVector<size_t> indexes(index.Ranges.size());
        Iota(indexes.begin(), indexes.end(), 0);

        for (size_t it = 0; it < 2; ++it) {
            std::shuffle(indexes.begin(), indexes.end(), std::minstd_rand(4243 + it * 42));

            for (size_t ind : indexes) {
                TIterator iterator;
                for (size_t it2 = 0; it2 < 2; ++it2) {
                    UNIT_ASSERT(searcher.Seek(index.Ranges[ind].first, index.Ranges[ind].second, &iterator));

                    for (size_t it3 = 0; it3 < 2; ++it3) {
                        TVector<THit> hits = lowerBoundHits[ind];
                        std::shuffle(hits.begin(), hits.end(), std::minstd_rand(43 + it2 * 123 + it3 * 19));
                        for (const THit& hit : hits) {
                            size_t pos = LowerBound(index.Hits[ind].begin(), index.Hits[ind].end(), hit) - index.Hits[ind].begin();
                            THit firstHit;
                            if (pos >= index.Hits[ind].size()) {
                                UNIT_ASSERT(!iterator.LowerBound(hit, &firstHit));
                            } else {
                                UNIT_ASSERT(iterator.LowerBound(hit, &firstHit));
                                UNIT_ASSERT_EQUAL(index.Hits[ind][pos], firstHit);
                                for (; pos < index.Hits[ind].size(); ++pos) {
                                    UNIT_ASSERT(iterator.ReadHit(&firstHit));
                                    UNIT_ASSERT_EQUAL(index.Hits[ind][pos], firstHit);
                                }
                                UNIT_ASSERT(!iterator.ReadHit(&firstHit));
                            }
                        }
                    }
                }
            }
        }
    }

    struct TPantherHitGenerator {

        using THit = TPantherHit;

        template <class Rand>
        TPantherHit operator()(Rand&& rand) {
            const ui32 docId = rand() % 42;
            const ui32 relevance = rand() % 42;
            return TPantherHit(docId, relevance);
        }

    };

    struct TAttributesHitGenerator {

        using THit = TAttributesHit;

        template <class Rand>
        TAttributesHit operator()(Rand&& rand) {
            return TAttributesHit(rand() % 4243);
        }

    };

    Y_UNIT_TEST(NoSub) {
        using TIo = TOffroadInvWadIo<PantherIndexType, TPantherHit, TPantherHitVectorizer, TPantherHitSubtractor, NOffroad::TNullVectorizer, NoStandardIoModel>;
        THits<TPantherHit> hits = GenerateHits(42, (1 << 10), TPantherHitGenerator());
        TIndex<TIo> index;
        TBufferStream stream;
        WriteIndex<TIo>(hits, &index, &stream);
        //Cerr << "NoSub = " << MD5::Calc(TStringBuf(stream.Buffer().Data(), stream.Buffer().Size())) << Endl;
        UNIT_ASSERT_MD5_EQUAL(stream.Buffer(), "7711b30d58dbc4d6fcf3934d91021c1f");

        THolder<IWad> wad = IWad::Open(TArrayRef<const char>(stream.Buffer().Data(), stream.Buffer().Size()));
        TestRead<TIo>(wad.Get(), index);
        TestSearcher<TIo>(wad.Get(), index);
    }

    Y_UNIT_TEST(Sub) {
        using TIo = TOffroadInvWadIo<AttributesIndexType, TAttributesHit, TAttributesHitVectorizer, TAttributesHitSubtractor, TAttributesHitVectorizer, NoStandardIoModel>;
        THits<TAttributesHit> hits = GenerateHits(42, (1 << 10), TAttributesHitGenerator());
        TIndex<TIo> index;
        TBufferStream stream;
        WriteIndex<TIo>(hits, &index, &stream);
        //Cerr << "Sub = " << MD5::Calc(TStringBuf(stream.Buffer().Data(), stream.Buffer().Size())) << Endl;
        UNIT_ASSERT_MD5_EQUAL(stream.Buffer(), "6db9964f09c6e6bbb3940a81e6c946a6");

        THolder<IWad> wad = IWad::Open(TArrayRef<const char>(stream.Buffer().Data(), stream.Buffer().Size()));
        TestRead<TIo>(wad.Get(), index);
        TestSearcher<TIo>(wad.Get(), index);

        THits<TAttributesHit> lowerBoundHits(hits.size());

        for (size_t i = 0; i < hits.size(); ++i) {
            lowerBoundHits[i].push_back(TAttributesHit(0));
            for (size_t j = 0; j < hits[i].size(); ++j) {
                for (size_t delta = 0; delta < 3; ++delta) {
                    if (hits[i][j].DocId() >= delta) {
                        lowerBoundHits[i].push_back(TAttributesHit(hits[i][j].DocId() - delta));
                    }
                    lowerBoundHits[i].push_back(TAttributesHit(hits[i][j].DocId() + delta));
                }
            }
            lowerBoundHits[i].push_back(TAttributesHit(100500));
            Sort(lowerBoundHits[i]);
            lowerBoundHits[i].erase(Unique(lowerBoundHits[i].begin(), lowerBoundHits[i].end()), lowerBoundHits[i].end());
        }

        TestLowerBound<TIo>(wad.Get(), index, lowerBoundHits);
    }

}
