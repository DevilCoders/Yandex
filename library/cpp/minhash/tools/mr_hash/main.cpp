#include "mr_hash.h"

#include <library/cpp/minhash/minhash_builder.h>
#include <library/cpp/minhash/minhash_func.h>
#include <library/cpp/minhash/minhash_helpers.h>
#include <library/cpp/minhash/table.h>

#include <library/cpp/getopt/last_getopt.h>

#include <mapreduce/lib/all.h>

#include <library/cpp/streams/factory/factory.h>
#include <util/stream/output.h>
#include <util/system/defaults.h>

using namespace NLastGetopt;

struct TMain {
    TString Server;
    TString FileName;
    double LoadFactor;
    ui32 KeysPerBucket;
    ui16 FprSize;
    ui32 NumPortions;
    bool Wide;
    TString Table;
    TMain(int argc, char* argv[])
        : LoadFactor(0.99)
        , KeysPerBucket(5)
        , FprSize(0)
        , NumPortions(1)
        , Wide(false)
    {
        TOpts opts = NLastGetopt::TOpts::Default();
        opts.AddLongOption('s', "server", "mapreduce server").StoreResult(&Server);
        opts.AddLongOption('f', "file", "data file").StoreResult(&FileName).Required();
        opts.AddLongOption('l', "load-factor", "load factor").StoreResult(&LoadFactor).DefaultValue("0.99");
        opts.AddLongOption('b', "bucket-size", "bucket size (number of keys per bucket)").StoreResult(&KeysPerBucket).DefaultValue("5");
        opts.AddLongOption('e', "fpr-size", "number of bits for checksum").StoreResult(&FprSize).DefaultValue("0");
        opts.AddLongOption('w', "wide", "convert to wide bytes").NoArgument();
        opts.AddLongOption('n', "portions", "number of portions").StoreResult(&NumPortions).DefaultValue("1");
        opts.SetFreeArgTitle(0, "INPUT", "input table");
        opts.SetFreeArgsMin(1);
        opts.SetFreeArgsMax(1);
        TOptsParseResult res(&opts, argc, argv);
        Table = res.GetFreeArgs()[0];
    }

    void Run() {
        NMinHash::CreateHash(Server, Table, NumPortions, LoadFactor, KeysPerBucket, FprSize, Wide, FileName);
        NMinHash::TTable<ui64, NMinHash::TDistChdMinHashFunc> tbl(FileName);
        NMinHash::TDistChdMinHashFunc* hash = tbl.Hash();
        Cerr << "keys: " << hash->Space() << "(" << 1.0 * hash->Space() / hash->Size() << ")" << Endl;
        Cerr << "vals: " << tbl.Space() << "(" << 1.0 * tbl.Space() / hash->Size() << ")" << Endl;
    }
};

int main(int argc, char* argv[]) {
    try {
        NMR::Initialize(argc, (const char**)argv);
        TMain(argc, argv).Run();
        return 0;
    } catch (...) {
        Cerr << CurrentExceptionMessage() << Endl;
    }
    return 1;
}
