#include <kernel/saas_trie/disk_io.h>
#include <kernel/saas_trie/disk_trie.h>
#include <kernel/saas_trie/trie_complex_key_iterator.h>

#include <kernel/saas_trie/idl/trie_key.h>

#include <library/cpp/getopt/small/last_getopt.h>

#include <util/datetime/base.h>
#include <util/generic/deque.h>
#include <util/stream/file.h>
#include <util/string/split.h>
#include <library/cpp/deprecated/atomic/atomic.h>
#include <util/thread/pool.h>

#include <cmath>

struct TConfig {
    TString IndexPath;
    TString KeysPath;
    TString OverrideMainKey;
    ui32 Iterations = 1;
    ui32 Threads = 1;
    ui32 Segments = 1;
    bool UrlMaskSearch = false;

    void Print(IOutputStream& out) {
        out << "--- Config --\n";
        out << "IndexPath: " << IndexPath << '\n';
        out << "KeysPath: " << KeysPath << '\n';
        out << "Iterations: " << Iterations << '\n';
        out << "Threads: " << Threads << '\n';
        out << "Segments: " << Segments << '\n';
        out << "Override MainKey: " << OverrideMainKey << '\n';
        out << "UrlMask search: " << UrlMaskSearch << '\n';
        out << "---" << Endl;
    }

    void Init(int argc, const char* argv[]) {
        auto makeOpt = [](TString name, TString help, bool required = true) {
            NLastGetopt::TOpt opt;
            opt.AddLongName(name);
            opt.Help(help);
            if (required) {
                opt.Required();
            }
            return opt;
        };
        NLastGetopt::TOpts opts;
        opts.AddHelpOption('h');
        opts.AddOption(makeOpt("index", "path to trie index", true).StoreResult(&IndexPath));
        opts.AddOption(makeOpt("keys", "path to keys", true).StoreResult(&KeysPath));
        opts.AddOption(makeOpt("iter", "number of iterations", false).StoreResult(&Iterations));
        opts.AddOption(makeOpt("threads", "number of threads", false).StoreResult(&Threads));
        opts.AddOption(makeOpt("segments", "number of segements (emulated)", false).StoreResult(&Segments));
        opts.AddOption(makeOpt("mainkey", "override MainKey", false).StoreResult(&OverrideMainKey));
        opts.AddOption(makeOpt("urlmask", "enable url mask search", false).StoreResult(&UrlMaskSearch));
        opts.AddHelpOption('h');
        NLastGetopt::TOptsParseResult parser(&opts, argc, argv);
    }
};

TDeque<TString> LoadKeys(const TString& path, const TString& overrideMainKey, bool urlMaskSearch) {
    TDeque<TString> result;
    auto content = TIFStream(path).ReadAll();
    for (const auto& it : StringSplitter(content).Split('\n').SkipEmpty()) {
        NSaasTrie::TComplexKey key;
        NSaasTrie::DeserializeFromCgi(key, it.Token(), true);
        if (!overrideMainKey.empty()) {
            key.SetMainKey(overrideMainKey);
        }
        if (urlMaskSearch) {
            key.SetUrlMaskPrefix(key.GetMainKey());
            key.ClearMainKey();
        }
        result.emplace_back(NSaasTrie::SerializeToCgi(key, true));
    }
    return result;
}

void TestThread(ui32 iterations, ui32 segments, const NSaasTrie::ITrieStorageReader& trie, const TDeque<TString>& keys, TAtomic& resultKeys, TAtomic& resultDocs) {
    TString stub;
    size_t foundKeys = 0;
    size_t foundDocs = 0;
    for (ui32 iter = 0; iter < iterations; iter += segments) {
        for (auto& strKey : keys) {
            NSaasTrie::TComplexKey key;
            NSaasTrie::DeserializeFromCgi(key, strKey, true);
            auto prepKey = NSaasTrie::PreprocessComplexKey(key, false, stub);
            for (ui32 segment = 0; segment < segments; ++segment) {
                auto iterator = CreateTrieComplexKeyIterator(trie, prepKey);
                if (!iterator->AtEnd()) {
                    ++foundKeys;
                    do {
                        ++foundDocs;
                    } while (iterator->Next());
                }
            }
        }
    }
    foundKeys /= iterations;
    foundDocs /= iterations;
    AtomicSet(resultKeys, foundKeys);
    AtomicSet(resultDocs, foundDocs);
}

int main(int argc, const char* argv[]) {
    try {
        TConfig config;
        config.Init(argc, argv);
        config.Print(Cout);

        auto keys = LoadKeys(config.KeysPath, config.OverrideMainKey, config.UrlMaskSearch);

        NSaasTrie::TDiskIO disk;
        auto trie = NSaasTrie::OpenDiskTrie(config.IndexPath, disk, true);
        Y_ENSURE(trie, "Unable to open trie index");

        Cout << "--- Start searching ---" << Endl;
        auto startTime = TInstant::Now();

        TThreadPool queue;
        queue.Start(config.Threads);

        TAtomic foundKeys = 0;
        TAtomic foundDocs = 0;
        for (ui32 i = 0; i < config.Threads; ++i) {
            queue.SafeAddFunc([&]() {
                TestThread(config.Iterations, config.Segments, *trie, keys, foundKeys, foundDocs);
            });
        }

        queue.Stop();
        auto endTime = TInstant::Now();

        Cout << "Total found: " << foundKeys << '/' << foundDocs << Endl;

        double totalTime = (endTime - startTime).MicroSeconds();
        Cout << "Time per iteration: " << floor(totalTime / config.Iterations + 0.5) << "us" << Endl;
        Cout << "Time per key:       " << floor(totalTime / (config.Iterations * keys.size()) + 0.5) << "us" << Endl;

        return 0;

    } catch (...) {
        Cerr << CurrentExceptionMessage() << Endl;
        return 1;
    }
}
