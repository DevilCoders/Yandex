#include <library/cpp/testing/unittest/registar.h>

#include <random>

#include <util/generic/vector.h>
#include <util/system/tempfile.h>
#include <util/system/getpid.h>
#include <util/stream/file.h>
#include <util/stream/buffer.h>
#include <util/system/thread.h>

#include <kernel/doom/wad/mega_wad_buffer_writer.h>
#include <kernel/doom/offroad_sent_wad/offroad_sent_wad_io.h>

#include "offroad_sent_wad_accessor.h"

Y_UNIT_TEST_SUITE(IndexSent) {
    using namespace NDoom;
    TMap<ui32, TVector<TSentHit>> GenerateHits(ui32 docCnt, ui32 maxSent) {
        std::mt19937 rnd(17631);
        TMap<ui32, TVector<TSentHit>> result;

        for (ui32 docId = 0; docId < docCnt; docId++) {
            ui32 cnt = rnd() % maxSent + 1;
            TSentHit hit;
            ui32 offset = 0;
            hit.SetDocId(docId);
            for (ui32 i = 0; i < cnt; i++) {
                hit.SetOffset(offset);
                result[docId].push_back(hit);
                offset += rnd() % 13 + 1;
            }
        }

        return result;
    }

    IOutputStream& operator<<(IOutputStream& s, const TSentHit& hit) {
        s << "[ " << hit.DocId() << ", " << hit.Offset() << " ]";
        return s;
    }

    template<class TWriter>
    void WriteIo(const TMap<ui32, TVector<TSentHit>>& pairedHits, TWriter& writer) {
        for (const auto& pair_data: pairedHits) {
            for (TSentHit hit : pair_data.second) {
                writer.WriteHit(hit);
            }
            writer.WriteDoc(pair_data.first);
        }
    }

    using TIo = TOffroadFastSentDocWadIo;

    Y_UNIT_TEST(Sent) {
        const ui32 DOCS_COUNT = 10000;
        TMap<ui32, TVector<TSentHit>> pairedHits = GenerateHits(DOCS_COUNT, 31);

        TIo::TSampler sampler;
        WriteIo(pairedHits, sampler);
        auto model = sampler.Finish();
        Cout << "[INFO] model has been prepared" << Endl;

        TBuffer buffer;
        TMegaWadBufferWriter wadWriter(&buffer);
        TIo::TWriter writer(model, &wadWriter);
        WriteIo(pairedHits, writer);
        wadWriter.Finish();

        TVector<TSentHit> hits;
        for (const auto& pair_data: pairedHits) {
            for (TSentHit hit : pair_data.second) {
                hits.push_back(hit);
            }
        }
        Cout << "[INFO] index has been written (" << buffer.Size() << " bytes, " << hits.size() << " hits)" << Endl;

        THolder<IWad> wad = IWad::Open(std::move(buffer));
        TIo::TReader reader(wad.Get());

        ui32 docId;
        // Check content:
        for (const auto& pair_data: pairedHits) {
            TSentHit hit;
            UNIT_ASSERT(reader.ReadDoc(&docId));
            for (TSentHit phit : pair_data.second) {
                UNIT_ASSERT(reader.ReadHit(&hit));
                hit.SetDocId(docId);
                UNIT_ASSERT(hit == phit);
            }
            UNIT_ASSERT(!reader.ReadHit(&hit));
        }
        UNIT_ASSERT(!reader.ReadDoc(&docId));

        TIo::TSearcher seacher(wad.Get());
        TIo::TSearcher::TIterator searchIterator;
        for (ui32 i = 0; i < DOCS_COUNT * 100; i++) {
            /* Try to read documents in random order: */
            TSentHit hit;
            ui32 docId = std::rand() % DOCS_COUNT;
            hit.SetDocId(docId);

            auto it = LowerBound(hits.begin(), hits.end(), hit);
            UNIT_ASSERT(seacher.Find(docId, &searchIterator) && (searchIterator.LowerBound(hit, &hit)));
            UNIT_ASSERT(*it == hit);

            while (searchIterator.ReadHit(&hit)) {
                hit.SetDocId(docId);
                UNIT_ASSERT(*it == hit);
                ++it;
            }

            UNIT_ASSERT(it == hits.end() || it->DocId() != docId);
        }
    }

    static const ui32 TEST_INDEX_DOCS_COUNT = 100;
    class TIndexTest {
    public:
        TIndexTest(ISentenceLengthsReader* reader)
            : Reader_(reader)
            , Ok_(true)
            , Thread_(ThreadProc, this)
        {
            Thread_.Start();
        }

        bool Wait() {
            Thread_.Join();
            return Ok_;
        }
    private:
        void Test() {
            std::mt19937 rnd(19637 + Thread_.Id() * 13);
            TSentenceOffsets offsets;
            offsets.resize(65536);

            for (ui32 i = 0; i < TEST_INDEX_DOCS_COUNT * 1000; i++) {
                ui32 docId = rnd() % TEST_INDEX_DOCS_COUNT;
                offsets.resize(0);
                Reader_->GetOffsets(docId, &offsets);

                ++docId;

                for (ui32 j = 1; j < offsets.size(); j++) {
                    if (offsets[j] - offsets[j - 1] != docId) {
                        Cout << "offsets[j] - offsets[j - 1] = " << offsets[j] - offsets[j - 1] << " ... " << docId << Endl;
                        Ok_ = false;
                        return;
                    }
                }

                if (offsets.size() != 1000) {
                    Ok_ = false;
                    return;
                }
            }
        }

        static void* ThreadProc(void *ptr) {
            TIndexTest* self = static_cast<TIndexTest*>(ptr);
            self->Test();

            return nullptr;
        }
    private:
        ISentenceLengthsReader* Reader_;
        bool Ok_;
        TThread Thread_;
    };

    Y_UNIT_TEST(Index) {
        // Check in multithreaded mode to be sure that TLS works fine.
        TIo::TSampler sampler;
        for (ui32 docId = 0; docId < TEST_INDEX_DOCS_COUNT; docId++) {
            ui32 offset = 0;
            for (ui32 brk = 0; brk < 1000; brk++) {
                sampler.WriteHit(TSentHit(docId, offset));
                offset += docId + 1;
            }
            sampler.WriteDoc(docId);
        }

        auto model = sampler.Finish();
        const TString filename = "./offroad-" + ToString(GetPID()) + ".sent.wad";
        TTempFile tmpfile(filename);
        TMegaWadWriter wadWriter(filename);
        TIo::TWriter writer(model, &wadWriter);
        for (ui32 docId = 0; docId < TEST_INDEX_DOCS_COUNT; docId++) {
            ui32 offset = 0;
            for (ui32 brk = 0; brk < 1000; brk++) {
                writer.WriteHit(TSentHit(docId, offset));
                offset += docId + 1;
            }
            writer.WriteDoc(docId);
        }
        wadWriter.Finish();
        Cout << "Index was written into " << filename << Endl;

        auto reader = NewOffroadFastSentWadIndex(filename, false);
        TVector<THolder<TIndexTest>> test;
        for (ui32 i = 0; i < 5; i++)
            test.emplace_back(new TIndexTest(reader.Get()));

        Cout << "5 threads have been started!" << Endl;
        bool res = true;
        for (auto& t : test)
            res = res && t->Wait();
        Cout << "5 threads have done there work." << Endl;

        UNIT_ASSERT(res);
    }
}
