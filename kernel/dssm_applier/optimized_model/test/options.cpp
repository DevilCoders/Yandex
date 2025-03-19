#include "options.h"

#include <library/cpp/getopt/last_getopt.h>

#include <util/generic/serialized_enum.h>

namespace NTest {

    void TOptions::Parse(int argc, const char* argv[]) {
        NLastGetopt::TOpts opts = NLastGetopt::TOpts::Default();

        opts.AddLongOption('m', "mode", "Mode: " + GetEnumAllNames<EMode>())
            .DefaultValue(Mode)
            .RequiredArgument("ENUM")
            .StoreResult(&Mode);

        NLastGetopt::TOptsParseResult res(&opts, argc, argv);
    }

} // namespace NTest
