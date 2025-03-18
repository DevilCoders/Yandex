#include <util/generic/buffer.h>
#include <util/system/hp_timer.h>
#include <util/stream/output.h>
#include <util/stream/buffer.h>

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/offroad/codec/multi_table.h>
#include <library/cpp/offroad/standard/standard_index_writer.h>
#include <library/cpp/offroad/standard/standard_index_searcher.h>
#include <library/cpp/offroad/test/test_index.h>

using TWriter = NOffroad::TStandardIndexWriter<NOffroad::TTestData, NOffroad::TOffsetKeyData, NOffroad::TTestDataVectorizer, NOffroad::TTestDataSubtractor>;
using THitSampler = NOffroad::TStandardIndexHitSampler<NOffroad::TTestData, NOffroad::TOffsetKeyData, NOffroad::TTestDataVectorizer, NOffroad::TTestDataSubtractor>;
using TKeySampler = NOffroad::TStandardIndexKeySampler<NOffroad::TTestData, NOffroad::TOffsetKeyData, NOffroad::TTestDataVectorizer, NOffroad::TTestDataSubtractor>;
using TSearcher = NOffroad::TStandardIndexSearcher<NOffroad::TTestData, NOffroad::TOffsetKeyData, NOffroad::TTestDataVectorizer, NOffroad::TTestDataSubtractor>;

template <class MemoryIndex, class IndexWriter>
void WriteIndex(const MemoryIndex& index, IndexWriter* writer) {
    for (const auto& pair : index) {
        for (ui64 hit : pair.second)
            writer->WriteHit(hit);
        writer->WriteKey(pair.first);
    }
}

struct TIndexBenchmarkOptions {
    size_t RunCount = 4;
};

void ParseOptions(int argc, const char** argv, TIndexBenchmarkOptions* options) {
    NLastGetopt::TOpts opts;
    opts.SetTitle("index_benchmark - benchmark for offroad index.");
    opts.SetFreeArgsNum(0);
    opts.AddCharOption('z', "number of runs (default 1)").RequiredArgument("<number>").StoreResult(&options->RunCount);

    NLastGetopt::TOptsParseResult parseResult(&opts, argc, argv);
}

template <class MemoryIndex>
double RunBenchmark(const TSearcher& searcher, const MemoryIndex& index) {
    NHPTimer::STime start;
    NHPTimer::GetTime(&start);

    ui64 sum = 0;

    TSearcher::TIterator iterator;
    for (const auto& pair : index) {
        if (searcher.Find(pair.first, &iterator)) {
            ui64 hit;
            while (iterator.ReadHit(&hit))
                sum += hit;
        }
    }

    return NHPTimer::GetTimePassed(&start) + sum / 1.e20; /* Use sum here so that it's not optimized out. */
}

int IndexBenchmark(int argc, const char** argv) {
    TIndexBenchmarkOptions options;
    ParseOptions(argc, argv, &options);

    auto index = NOffroad::MakeTestIndex(1024, 31445);

    THitSampler hitSampler;
    WriteIndex(index, &hitSampler);
    auto hitTables = NOffroad::NewMultiTable(hitSampler.Finish());

    TKeySampler keySampler(hitTables.Get());
    WriteIndex(index, &keySampler);
    auto keyTables = NOffroad::NewMultiTable(keySampler.Finish());

    TBufferStream hitStream, keyStream, fatStream, fatSubStream;

    TWriter writer(hitTables.Get(), keyTables.Get(), &hitStream, &keyStream, &fatStream, &fatSubStream);
    WriteIndex(index, &writer);
    writer.Finish();

    TSearcher searcher(hitTables.Get(), keyTables.Get(),
                       TArrayRef<const char>(hitStream.Buffer().data(), hitStream.Buffer().size()),
                       TArrayRef<const char>(keyStream.Buffer().data(), keyStream.Buffer().size()),
                       TArrayRef<const char>(fatStream.Buffer().data(), fatStream.Buffer().size()),
                       TArrayRef<const char>(fatSubStream.Buffer().data(), fatSubStream.Buffer().size()));

    double warmup = RunBenchmark(searcher, index);
    Cout << "WARMUP:\t" << warmup << Endl;

    double total = 0.0;
    for (size_t i = 0; i < options.RunCount; i++) {
        double time = RunBenchmark(searcher, index);
        total += time;

        Cout << "RUN" << i << ":\t" << time << Endl;
    }

    Cout << "MEAN:\t" << (total / options.RunCount) << Endl;

    return 0;
}

int main(int argc, const char** argv) {
    try {
        return IndexBenchmark(argc, argv);
    } catch (...) {
        Cerr << CurrentExceptionMessage() << Endl;
        return 1;
    }
}
