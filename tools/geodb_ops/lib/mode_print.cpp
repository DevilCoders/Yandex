#include "modes.h"

#include <kernel/geodb/geodb.h>

#include <library/cpp/getopt/small/last_getopt.h>
#include <library/cpp/streams/factory/factory.h>

#include <util/folder/path.h>
#include <util/generic/algorithm.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/stream/input.h>
#include <util/stream/output.h>

#include <cstdlib>

namespace {
    struct TArgs {
            TFsPath InputPath = {};
            TFsPath OutputPath = {};
            bool AsArray = {};
            bool AsDictionary = {};
    };
}  // namespace


static TArgs ParseOptions(const int argc, const char** argv) {
    auto args = TArgs{};
    auto parser = NLastGetopt::TOpts::Default();
    parser.SetTitle("Will print each region (TRegionProto) data using AsJSON method."
                    " Please note that the format is different from JSON dump generated"
                    " by geobase itself.");
    parser.AddLongOption('i', "input")
        .Required()
        .RequiredArgument("FILE")
        .Help("File with serialized GeoDB")
        .StoreResult(&args.InputPath);
    parser.AddLongOption('o', "output")
        .RequiredArgument("FILE")
        .DefaultValue("-")
        .StoreResult(&args.OutputPath);
    parser.AddLongOption("as-array")
        .Optional()
        .NoArgument()
        .Help("Print as JSON array (array of jsonified TRegionProto)")
        .SetFlag(&args.AsArray);
    parser.AddLongOption("as-dict")
        .Optional()
        .NoArgument()
        .Help("Print as JSON dictionary")
        .SetFlag(&args.AsDictionary);
    parser.SetFreeArgsNum(0);
    NLastGetopt::TOptsParseResult{&parser, argc, argv};
    Y_ENSURE(CountOf(true, args.AsArray, args.AsDictionary) <= 1,
           "--as-array and --as-dict are mutually exclusive");
    return args;
}

static int Main(const TArgs& args, IInputStream& input, IOutputStream& output) {
    const auto geodb = NGeoDB::TGeoKeeper::LoadToHolder(input);

    if (args.AsArray) {
        output << '[' << '\n';
    } else if (args.AsDictionary) {
        output << '{' << '\n';
    }

    for (auto it = geodb->cbegin(); it != geodb->cend(); ++it) {
        if (args.AsArray) {
            if (geodb->cbegin() != it) {
                output << ',';
            }
        } else if (args.AsDictionary) {
            if (geodb->cbegin() != it ) {
                output << ',';
            }
            output << '"' << it->first << "\":"sv;
        }

        output << it->second.AsJSON() << '\n';
    }

    if (args.AsArray) {
        output << ']' << '\n';
    } else if (args.AsDictionary) {
        output << '}' << '\n';
    }

    return EXIT_SUCCESS;
}

static int Main(const TArgs& args) {
    const auto input = OpenInput(args.InputPath);
    const auto output = OpenOutput(args.OutputPath);
    return Main(args, *input, *output);
}

int NGeoDBOps::MainPrint(const int argc, const char* argv[]) {
    const auto args = ParseOptions(argc, argv);
    return ::Main(args);
}
