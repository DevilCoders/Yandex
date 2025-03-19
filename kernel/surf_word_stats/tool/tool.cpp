#include <kernel/surf_word_stats/lib/reader.h>

#include <ysite/yandex/reqanalysis/normalize.h>

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/getopt/modchooser.h>
#include <library/cpp/compute_graph/compute_graph.h>

#include <util/system/mutex.h>
#include <util/system/pipe.h>
#include <util/generic/queue.h>

int main_debug_word(int argc, const char** argv)
{
    TString ywFile;

    NLastGetopt::TOpts options;

    options
        .AddCharOption('i', "--  input file")
        .Required()
        .RequiredArgument("FILE")
        .StoreResult(&ywFile);

    NLastGetopt::TOptsParseResult optParsing(&options, argc, argv);
    Y_UNUSED(optParsing);

    NWordFeatures::TSurfFeaturesCalcer calcer(ywFile);

    NWordFeatures::TSurfFeatures features;

    TString line;
    while (Cin.ReadLine(line)) {
        calcer.FindOrMean(line, features);
        Cout << line << "\t" << ToString(features) << Endl;
    }

    return 0;
}

//================ Multithread ========================================

namespace {
class TSemaphore {
public:
    inline TSemaphore(size_t initialCount = 0) {
        TPipeHandle::Pipe(R_, W_);

        for (size_t i = 0; i < initialCount; ++i) {
            Release();
        }
    }

    inline void Acquire() {
        char ch;
        R_.Read(&ch, 1);
    }

    inline void Release() {
        W_.Write("", 1);
    }

private:
    TPipeHandle R_;
    TPipeHandle W_;
};

struct TChunk {
    ui32 Id;
    TVector<TString>* Data;

    TChunk(ui32 id = - 1, TVector<TString>* data = nullptr)
        : Id(id)
        , Data(data)
    {
    }

    bool Eof() const
    {
        return (Data == nullptr);
    }
};

// Unordered set of chunks.
class TChunkStorage {
private:
    TSemaphore Semaphore;
    TMutex Mutex;
    TList<TChunk> Chunks;
public:
    void AddChunk(const TChunk& chunk)
    {
        {
            TGuard<TMutex> guard(Mutex);
            Chunks.push_back(chunk);
        }
        Semaphore.Release();
    }

    TChunk GetChunk()
    {
        Semaphore.Acquire();
        {
            TGuard<TMutex> guard(Mutex);
            Y_VERIFY(!Chunks.empty(), "Wtf?!");
            const auto r = Chunks.front();
            Chunks.pop_front();
            return r;
        }
    }
};

// Ordered set of chunks.
class TChunkQueue {
private:
    TSemaphore Semaphore;
    TMutex Mutex;

    struct TChunkCompare {
        bool operator()(const TChunk& x, const TChunk& y)
        {
            return (x.Id > y.Id);
        }
    };

    TPriorityQueue<TChunk,TVector<TChunk>,TChunkCompare> Queue;
    ui32 NextId = 0;
public:
    void AddChunk(const TChunk& chunk)
    {
        bool ok = false;
        {
            TGuard<TMutex> guard(Mutex);
            if (NextId == chunk.Id) {
                ok = true;
                ++NextId;
            }
            Queue.push(chunk);
        }
        if (ok) {
            Semaphore.Release();
        }
    }

    TChunk GetChunk()
    {
        TChunk r;

        Semaphore.Acquire();
        bool ok = false;
        {
            TGuard<TMutex> guard(Mutex);
            Y_VERIFY(!Queue.empty(), "Wtf?!");
            r = Queue.top();
            Queue.pop();
            if (!Queue.empty() && Queue.top().Id == NextId) {
                ok = true;
                ++NextId;
            }
        }
        if (ok) {
            Semaphore.Release();
        }

        return r;
    }
};

static
TStringBuf GetField(TStringBuf line, ui32 fieldIndex)
{
    TStringBuf result;
    ui32 index = 0;
    while (true) {
        result = line.NextTok('\t');
        if (index == fieldIndex) {
            break;
        }
        ++index;
    }
    return result;
}

class TWorker {
private:
    TChunkStorage& InChunks;
    TChunkQueue& OutChunks;
    ui32 FieldIndex;
    const NWordFeatures::TSurfFeaturesCalcer& Calcer;
    const bool NormalizeRequests;
    const bool CalcAdditionalFeatures;

    TVector<float> Tmp;
public:
    TWorker(TChunkStorage& inChunks,
            TChunkQueue& outChunks,
            ui32 fieldIndex,
            NWordFeatures::TSurfFeaturesCalcer& calcer,
            bool normalizeRequests,
            bool calcAdditionalFeatures)
        : InChunks(inChunks)
        , OutChunks(outChunks)
        , FieldIndex(fieldIndex)
        , Calcer(calcer)
        , NormalizeRequests(normalizeRequests)
        , CalcAdditionalFeatures(calcAdditionalFeatures)
    {
    }

    void Do()
    {
        while (true) {
            const auto c = InChunks.GetChunk();
            if (c.Eof()) {
                return;
            }
            for (size_t i = 0; i < c.Data->size(); ++i) {
                const auto field = GetField(c.Data->at(i), FieldIndex);
                TString norm;
                if (NormalizeRequests || CalcAdditionalFeatures) {
                    norm = NormalizeRequestUTF8(TString{field});
                }
                if (NormalizeRequests) {
                    Calcer.FillQueryFeatures(norm, Tmp);
                } else {
                    Calcer.FillQueryFeatures(field, Tmp);
                }
                if (CalcAdditionalFeatures) {
                    TVector<float> features(4);

                    // qnormLength
                    features[0] = norm.size() / (norm.size() + 20.0);

                    // numWordsQnorm,numDigits,percentEngLetters
                    ui16 numWords = 1;
                    ui16 numEngLetters = 0;
                    ui16 numSpace = 0;
                    ui16 numDigits = 0;

                    for (size_t i = 0; i < norm.size(); ++i) {
                        const char ch = norm[i];
                        if (ch == ' ') {
                            ++numSpace;
                            ++numWords;
                            continue;
                        }
                        if (ch >= '0' && ch <= '9') {
                            ++numDigits;
                        }
                        if (ch >= 'a' && ch <= 'z') {
                            ++numEngLetters;
                        }
                    }

                    features[1] = numWords / (numWords + 7.0);
                    features[2] = numDigits / (numDigits + 4.0);
                    features[3] = (1.0 * numEngLetters) / (norm.size() - numSpace);

                    Tmp.insert(Tmp.end(), features.begin(), features.end());
                }
                c.Data->at(i) += "\t" + JoinStrings(Tmp.begin(), Tmp.end(), "\t");
            }
            OutChunks.AddChunk(c);
        }
    }
};
}

//=====================================================================

int main_do_sent(int argc, const char** argv)
{
    NLastGetopt::TOpts options;

    TString ywFile;
    options
        .AddCharOption('i', "--  input file")
        .Required()
        .RequiredArgument("FILE")
        .StoreResult(&ywFile);

    options
        .AddCharOption('n', "--  do normalize requests")
        .NoArgument()
        .Optional();

    ui32 threads = 0;
    options
        .AddCharOption('j', "--  thread count")
        .Optional()
        .RequiredArgument("NUM")
        .StoreResult(&threads)
        .DefaultValue("4");

    ui32 fieldIndex = 0;
    options
        .AddCharOption('t', "--  column index with query (starts with 0)")
        .Optional()
        .RequiredArgument("NUM")
        .StoreResult(&fieldIndex)
        .DefaultValue("0");

    options
        .AddCharOption('a', "--  calc additional simple features")
        .Optional()
        .NoArgument();

    NLastGetopt::TOptsParseResult optParsing(&options, argc, argv);
    Y_UNUSED(optParsing);

    if (threads == 0) {
        threads = 1;
    }

    NWordFeatures::TSurfFeaturesCalcer calcer(ywFile);
    TChunkStorage chunkSet;
    TChunkStorage chunkSet2;
    TChunkQueue chunkQueue;

    const bool normRequests = optParsing.Has('n');

    const static size_t NUM_CHUNKS = Max<size_t>(32, threads * 2);
    const static size_t NUM_ELEMENTS = 1000;

    TVector<TVector<TString>> chunks(NUM_CHUNKS);

    for (size_t i = 0; i < NUM_CHUNKS; ++i) {
        chunkSet2.AddChunk(TChunk(-1, &chunks[i]));
    }

    NComputeGraph::TJobRunner runner(threads + 2);

    // Workers.
    for (ui32 i = 0; i < threads; ++i) {
        runner.AddJob([&chunkSet, &chunkQueue, fieldIndex, &calcer, &optParsing, normRequests]() {
            TWorker worker(chunkSet, chunkQueue, fieldIndex, calcer, normRequests, optParsing.Has('a'));
            worker.Do();
        });
    }

    // Reader.
    runner.AddJob([&chunkSet2, &chunkSet, &chunkQueue, threads]() {
        TString line;

        TChunk chunk = chunkSet2.GetChunk();
        chunk.Data->clear();
        chunk.Id = 0;
        ui32 nextId = 1;

        while (Cin.ReadLine(line)) {
            chunk.Data->push_back(line);
            if (chunk.Data->size() == NUM_ELEMENTS) {
                chunkSet.AddChunk(chunk);
                chunk = chunkSet2.GetChunk();
                chunk.Id = nextId;
                chunk.Data->clear();
                nextId ++;
            }
        }

        chunkSet.AddChunk(chunk);

        // Eof.
        for (ui32 i = 0; i < threads; ++i) {
            chunkSet.AddChunk(TChunk());
        }
        chunkQueue.AddChunk(TChunk(nextId, nullptr));
    });

    // Writer.
    runner.AddJob([&chunkQueue, &chunkSet2]() {
        TChunk chunk = chunkQueue.GetChunk();

        while (!chunk.Eof()) {
            for (size_t i = 0; i < chunk.Data->size(); ++i) {
                Cout << chunk.Data->at(i) << "\n";
            }

            chunkSet2.AddChunk(chunk);

            chunk = chunkQueue.GetChunk();
        }
    });

    runner.Run();

    return 0;
}

int main_print_feature_names(int /*argc*/, const char** /*argv*/)
{
    const auto names = NWordFeatures::TSurfFeaturesCalcer::GetQueryFeatureNames();

    for (size_t i = 0; i < names.size(); ++i) {
        Cout << i << "\t" << names[i] << "\n";
    }

    return 0;
}

int main(int argc, const char** argv)
{
    TModChooser modChooser;

    modChooser.AddMode(
        "debug-word",
        main_debug_word,
        "-- debugs word"
    );

    modChooser.AddMode(
        "calc-sent-features",
        main_do_sent,
        "-- calculates surf features for sentences"
    );

    modChooser.AddMode(
        "factor-names",
        main_print_feature_names,
        "-- print feature names"
    );

    return modChooser.Run(argc, argv);
}
