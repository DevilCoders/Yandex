/*
http://wiki.yandex-team.ru/stanislavzadorozhnyjj/minhash
*/
#include <library/cpp/minhash/iterators.h>
#include <library/cpp/minhash/minhash_builder.h>
#include <library/cpp/minhash/minhash_func.h>

#include <library/cpp/getopt/last_getopt.h>

#include <util/datetime/cputimer.h>
#include <util/folder/dirut.h>
#include <util/generic/ptr.h>
#include <library/cpp/streams/factory/factory.h>
#include <util/stream/file.h>
#include <util/stream/output.h>
#include <util/stream/printf.h>

using namespace NLastGetopt;
using namespace NMinHash;

struct TMain {
    size_t NumKeys;
    double LoadFactor;
    ui32 KeysPerBucket;
    ui32 Seed;
    TString FileName;
    ui16 ErrorBits;
    size_t NumBuckets;
    bool Wide;
    TVector<TString> Files;
    bool QueryMode;
    bool GenerateMode;
    TMain(int argc, char* argv[])
        : NumKeys(0)
        , LoadFactor(0.99)
        , KeysPerBucket(5)
        , Seed(0)
        , ErrorBits(0)
        , NumBuckets(0)
        , Wide(false)
        , QueryMode(false)
        , GenerateMode(false)
    {
        TOpts opts = NLastGetopt::TOpts::Default();
        opts.AddHelpOption();
        opts.AddLongOption('g', "generate", "generate mode").NoArgument();
        opts.AddLongOption('q', "query", "query mode").NoArgument();
        opts.AddLongOption('f', "file", "data file").StoreResult(&FileName).Required();
        opts.AddLongOption('n', "num-keys", "number of keys").StoreResult(&NumKeys);
        opts.AddLongOption('l', "load-factor", "load factor").StoreResult(&LoadFactor).DefaultValue("0.99");
        opts.AddLongOption('b', "bucket-size", "bucket size (number of keys per bucket)").StoreResult(&KeysPerBucket).DefaultValue("5");
        opts.AddLongOption('s', "seed", "random seed").StoreResult(&Seed);
        opts.AddLongOption('e', "error", "number of bits for checksum").StoreResult(&ErrorBits);
        opts.AddLongOption('w', "wide", "convert to wide bytes").NoArgument();
        opts.SetFreeArgTitle(0, "INPUT", "input file; tab delimeted");
        opts.SetFreeArgTitle(1, "OUTPUT", "output file");
        opts.SetFreeArgsMax(2);
        TOptsParseResult res(&opts, argc, argv);
        Files = res.GetFreeArgs();
        Files.resize(2);
        Wide = res.Has('w');
        QueryMode = res.Has('q');
        GenerateMode = res.Has('g');
    }

    void Query() {
        TAutoPtr<IInputStream> inp = OpenInput(Files[0]);
        TAutoPtr<IOutputStream> out = OpenOutput(Files[1]);
        TFileInput fi(FileName);
        TChdMinHashFunc hash(&fi);
        TKeysContainer keys(Files[0], Wide);
        for (auto it = keys.begin(); it != keys.end(); ++it) {
            TStringBuf s = *it;
            *out << (*it) << '\t' << hash.Get(s.data(), s.size()) << '\n';
        }
    }

    void Generate() {
        if (!NumKeys) {
            NumKeys = TKeysContainer(Files[0]).Size();
        }
        TOFStream fs(FileName);
        CreateHash(Files[0], LoadFactor, NumKeys, KeysPerBucket, Seed, ErrorBits, Wide, &fs);
    }

    void Info() {
        TFileInput fi(FileName);
        TChdMinHashFunc hash(&fi);
        Cerr << "Space -> " << 1. * hash.Space() / hash.Size() << " bits per key" << Endl;
        Cerr << "NumKeys -> " << hash.Size() << Endl;
        Cerr << "ErrorBits -> " << (ui16)hash.ErrorBits() << Endl;
        Cerr << "Seed -> " << hash.Seed() << Endl;
    }

    void Run() {
        if (!QueryMode && !GenerateMode) {
            Info();
        } else if (QueryMode) {
            Query();
        } else if (GenerateMode) {
            Generate();
        }
    }
};

int main(int argc, char* argv[]) {
    try {
        TMain(argc, argv).Run();
        return 0;
    } catch (...) {
        Cerr << CurrentExceptionMessage() << Endl;
    }
    return 1;
}
