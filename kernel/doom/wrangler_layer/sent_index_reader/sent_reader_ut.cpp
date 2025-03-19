#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/digest/md5/md5.h>
#include <library/cpp/offroad/test/test_md5.h>

#include <util/random/shuffle.h>
#include <util/random/easy.h>
#include <random>

#include <kernel/doom/wrangler_layer/sent_index_reader/sent_reader.h>
#include <kernel/doom/offroad_sent_wad/offroad_sent_wad_io.h>
#include <kernel/doom/wad/mega_wad_buffer_writer.h>
#include <kernel/sent_lens/sent_lens.h>
#include <kernel/doom/algorithm/transfer.h>


using namespace NDoom;
using namespace NOffroad;



Y_UNIT_TEST_SUITE(TestIndexSent) {
    using Io = TOffroadSentWadIo;
    using TSampler = Io::TSampler;
    using TWriter = Io::TWriter;
    using TSearcher = Io::TSearcher;


    TString oldIndexPath = BinaryPath("kernel/doom/wrangler_layer/sent_index_reader//old_sent/indexsent");


    THolder<IWad> TransferOldIndex() {
        TSampler sampler;
        TSentIndexReader oldReader(oldIndexPath);
        Cout << "Docs in indexsent: " << oldReader.Size() << Endl;
        TransferIndex(&oldReader, &sampler);

        TWriter::TModel model = sampler.Finish();
        TBufferOutput output;
        TMegaWadWriter wadWriter(&output);
        TWriter writer(model, &wadWriter);
        oldReader.Restart();
        TransferIndex(&oldReader, &writer);
        writer.Finish();
        wadWriter.Finish();

        return IWad::Open(std::move(output.Buffer()));
    }

    void CompareIndices(THolder<IWad>&& wad) {
        Y_UNUSED(wad);
        TSentenceLengthsReader oldReader(oldIndexPath);
        TSentenceLengthsReader newReader(wad.Get(), wad.Get(), NDoom::SentIndexType);
        size_t size = oldReader.GetSize();
        UNIT_ASSERT(size = newReader.GetSize());


        for (ui32 docId = 0; docId < size; docId++) {
            TSentenceLengths oldLengths;
            TSentenceLengths newLengths;
            oldReader.Get(docId, &oldLengths);
            newReader.Get(docId, &newLengths);

            UNIT_ASSERT_EQUAL(+oldLengths, +newLengths);
            for(size_t i = 0; i < +oldLengths; ++i) {
                UNIT_ASSERT_EQUAL(oldLengths[i], newLengths[i]);
            }

            TSentenceOffsets oldOffsets;
            TSentenceOffsets newOffsets;

            oldReader.GetOffsets(docId, &oldOffsets);
            newReader.GetOffsets(docId, &newOffsets);

            UNIT_ASSERT_EQUAL(+oldOffsets, +newOffsets);
            for(size_t i = 0; i < +oldLengths; ++i) {
                UNIT_ASSERT_EQUAL(oldOffsets[i], newOffsets[i]);
            }
        }
    }

    Y_UNIT_TEST(OldNewCompare) {
        CompareIndices(
            TransferOldIndex()
        );
    }
}


