#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>
#include <util/stream/output.h>
#include <util/stream/input.h>
#include <util/generic/algorithm.h>
#include <util/string/split.h>
#include <util/charset/wide.h>
#include <util/folder/dirut.h>

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/deprecated/omni/write.h>
#include <library/cpp/deprecated/omni/read.h>

using namespace NOmni;

namespace nlg = NLastGetopt;

int main(const int argc, const char** argv) {
    nlg::TOpts opts;
    opts.AddHelpOption('h');

    bool dumpScheme = false;
    opts.AddLongOption("dump-scheme", "Dump index scheme and exit")
        .NoArgument()
        .Optional()
        .StoreValue(&dumpScheme, true);

    opts.SetFreeArgsNum(1);
    opts.SetFreeArgTitle(0, "<index_path>", "path to index.omni");

    const nlg::TOptsParseResult optsres(&opts, argc, argv);
    TVector<TString> freeArgs = optsres.GetFreeArgs();

    TString indexPath = freeArgs[0];

    TOmniReader reader(indexPath);
    if (dumpScheme) {
        TString userSchemeDump = reader.GetUserSchemeDump();
        Cout << userSchemeDump;
        return 0;
    }

    TOmniIterator iter = reader.Root();
    iter.DbgPrint(Cout);

    return 0;
}
