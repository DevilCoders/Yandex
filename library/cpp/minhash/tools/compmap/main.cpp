/*
http://wiki.yandex-team.ru/stanislavzadorozhnyjj/minhash
*/

#include <library/cpp/minhash/iterators.h>
#include <library/cpp/minhash/minhash_builder.h>
#include <library/cpp/minhash/minhash_func.h>
#include <library/cpp/minhash/table.h>

#include <library/cpp/getopt/last_getopt.h>

#include <library/cpp/getopt/modchooser.h>
#include <util/generic/algorithm.h>
#include <library/cpp/streams/factory/factory.h>
#include <util/string/vector.h>
#include <util/string/split.h>

using namespace NLastGetopt;
using namespace NMinHash;

struct TGenerateTableMode: public TMainClass {
    double LoadFactor;
    ui32 KeysPerBucket;
    ui32 Seed;
    ui16 ErrorBits;
    bool Wide;
    TString InputFile;
    TString OutputFile;

    TGenerateTableMode()
        : LoadFactor(0.99)
        , KeysPerBucket(5)
        , Seed(0)
        , ErrorBits(0)
        , Wide(false)
    {
    }

    int operator()(const int argc, const char** argv) override {
        TOpts opts = NLastGetopt::TOpts::Default();
        opts.AddHelpOption();
        opts.AddLongOption('l', "load-factor", "load factor").StoreResult(&LoadFactor).DefaultValue("0.99");
        opts.AddLongOption('b', "bucket-size", "bucket size (number of keys per bucket)").StoreResult(&KeysPerBucket).DefaultValue("5");
        opts.AddLongOption('s', "seed", "random seed").StoreResult(&Seed).DefaultValue("0");
        opts.AddLongOption('e', "error", "number of bits for checksum").StoreResult(&ErrorBits).DefaultValue("0");
        opts.AddLongOption('w', "wide", "convert to wide bytes").NoArgument().StoreValue(&Wide, true);
        opts.SetFreeArgTitle(0, "INPUT", "input file; tab delimited");
        opts.SetFreeArgTitle(1, "OUTPUT", "output file");
        opts.SetFreeArgsMin(1);
        opts.SetFreeArgsMax(2);
        TOptsParseResult res(&opts, argc, argv);
        TVector<TString> freeArgs = res.GetFreeArgs();
        freeArgs.resize(2);
        InputFile = freeArgs[0];
        OutputFile = freeArgs[1];
        Run();
        return 0;
    }

    void Run() {
        ui64 numKeys = TKeysContainer(InputFile).Size();
        size_t numColumns = 0;
        {
            TAutoPtr<IInputStream> inp = OpenInput(InputFile);
            TString line = inp->ReadLine();
            numColumns = Count(line.begin(), line.vend(), '\t');
        }
        Y_ASSERT(numColumns > 0);
        TVector<TVector<ui64>> cols(numColumns);
        for (size_t i = 0; i < cols.size(); ++i) {
            cols[i].resize(numKeys, 0);
        }

        TChdHashBuilder builder(numKeys, LoadFactor, KeysPerBucket, Seed, ErrorBits);
        TKeysContainer keys(InputFile, Wide);
        TAutoPtr<TChdMinHashFunc> hash = builder.Build(keys);

        TString line;
        TString key;
        TUtf16String tmp;
        TVector<TString> parts;

        TAutoPtr<IInputStream> inp = OpenInput(InputFile);
        while (inp->ReadLine(line)) {
            StringSplitter(line).Split('\t').SkipEmpty().Collect(&parts);
            key = parts[0];
            if (Wide) {
                tmp.AssignUtf8(key);
                key.assign((const char*)tmp.Data(), tmp.Size() * sizeof(TUtf16String::TChar));
            }
            ui32 pos = hash->Get(key.data(), key.size(), false);
            for (size_t i = 0; i < cols.size(); ++i) {
                cols[i][pos] = FromString<ui64>(parts[i + 1]);
            }
        }

        TTable<ui64, TChdMinHashFunc> tbl(hash.Release());
        for (size_t i = 0; i < cols.size(); ++i) {
            tbl.Add(cols[i].begin(), cols[i].end());
        }
        TAutoPtr<IOutputStream> out = OpenOutput(OutputFile);
        ::Save(out.Get(), tbl);
    }
};

struct TQueryTableMode: public TMainClass {
    bool Wide;
    TString TableFile;
    TString InputFile;
    TString OutputFile;

    TQueryTableMode()
        : Wide(false)
    {
    }

    int operator()(const int argc, const char** argv) override {
        TOpts opts = NLastGetopt::TOpts::Default();
        opts.AddLongOption('w', "wide", "convert to wide bytes").NoArgument().StoreValue(&Wide, true);
        opts.SetFreeArgTitle(0, "TABLE", "table file;");
        opts.SetFreeArgTitle(1, "INPUT", "input file; tab delimited");
        opts.SetFreeArgTitle(2, "OUTPUT", "output file");
        opts.SetFreeArgsMin(1);
        opts.SetFreeArgsMax(3);
        TOptsParseResult res(&opts, argc, argv);
        TVector<TString> freeArgs = res.GetFreeArgs();
        freeArgs.resize(3);
        TableFile = freeArgs[0];
        InputFile = freeArgs[1];
        OutputFile = freeArgs[2];
        Run();
        return 0;
    }

    void Run() {
        TTable<ui64, TChdMinHashFunc> tbl(TableFile);
        TKeysContainer keys(InputFile, Wide);
        TAutoPtr<IOutputStream> out = OpenOutput(OutputFile);
        TString key;
        for (auto it = keys.begin(); it != keys.end(); ++it) {
            key = *it;
            *out << key;
            ui32 pos = tbl.Hash()->Get(key.data(), key.size());
            if (pos == TChdMinHashFunc::npos) {
                *out << '\t' << -1 << Endl;
                continue;
            }
            for (size_t i = 0; i < tbl.Columns().size(); ++i) {
                *out << '\t' << tbl.Get(i, pos);
            }
            *out << Endl;
        }
    }
};

struct TInfoTableMode: public TMainClass {
    TString TableFile;
    TString OutputFile;

    int operator()(const int argc, const char** argv) override {
        TOpts opts = NLastGetopt::TOpts::Default();
        opts.SetFreeArgTitle(0, "TABLE", "table file;");
        opts.SetFreeArgTitle(1, "OUTPUT", "output file");
        opts.SetFreeArgsMin(1);
        opts.SetFreeArgsMax(2);
        TOptsParseResult res(&opts, argc, argv);
        TVector<TString> freeArgs = res.GetFreeArgs();
        freeArgs.resize(2);
        TableFile = freeArgs[0];
        OutputFile = freeArgs[1];
        Run();
        return 0;
    }

    void Run() {
        TTable<ui64, TChdMinHashFunc> tbl(TableFile);
        TAutoPtr<IOutputStream> out = OpenOutput(OutputFile);
        TChdMinHashFunc* hash = tbl.Hash();
        *out << "num keys: " << hash->Size()
             << ", num cols: " << tbl.Columns().size()
             << ", error bits: " << (ui16)hash->ErrorBits()
             << ", seed: " << (ui16)hash->Seed()
             << Endl;

        *out << "keys: " << hash->Space() << "(" << 1.0 * hash->Space() / hash->Size() << ")" << Endl;
        *out << "vals: " << tbl.Space() << "(" << 1.0 * tbl.Space() / hash->Size() << ")" << Endl;
    }
};

int main(int argc, const char** argv) {
    try {
        TModChooser mod;
        mod.AddMode("generate", new TGenerateTableMode(), "generate compact map");
        mod.AddMode("query", new TQueryTableMode(), "query compact map");
        mod.AddMode("info", new TInfoTableMode(), "info about compact map");
        return mod.Run(argc, argv);
    } catch (yexception& e) {
        Cerr << CurrentExceptionMessage() << Endl;
        return 1;
    }
}
