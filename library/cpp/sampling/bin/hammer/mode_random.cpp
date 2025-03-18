#include "common.h"
#include "modes.h"

#include <library/cpp/sampling/alias_method.h>
#include <library/cpp/sampling/roulette_wheel.h>

#include <library/cpp/getopt/small/last_getopt.h>
#include <library/cpp/streams/factory/factory.h>

#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/random/fast.h>
#include <util/stream/output.h>

namespace {
    struct TArgs {
        size_t Repeat = {};
        TString InputPath = {};
    };
}

static TArgs ParseOptions(const int argc, const char* argv[]) {
    auto parser = NLastGetopt::TOpts::Default();
    auto args = TArgs{};
    parser.AddLongOption('i', "input")
        .RequiredArgument("FILE")
        .Required()
        .StoreResult(&args.InputPath);
    parser.AddLongOption("repeat")
        .RequiredArgument("UINT")
        .DefaultValue("100")
        .StoreResult(&args.Repeat);
    parser.SetFreeArgsNum(0);
    const NLastGetopt::TOptsParseResult parseResult{&parser, argc, argv};
    return args;
}

static TVector<double> Load(const TString& path) {
    const auto input = OpenInput(path);
    auto size = size_t{};
    *input >> size;

    auto res = TVector<double>(size);
    for (auto& value : res) {
        *input >> value;
    }

    return res;
}

static int Main(const TArgs& args) {
    const auto probs = Load(args.InputPath);

    Cout << '\t' << "TRouletteWheel..." << Endl;
    if (DoTest<NSampling::TRouletteWheel>(probs, probs.size(), args.Repeat)) {
        Cout << '\t' << "OK" << Endl;
    } else {
        Cout << '\t' << "FAIL" << Endl;
        return EXIT_FAILURE;
    }

    Cout << '\t' << "TAliasMethod..." << Endl;
    if (DoTest<NSampling::TAliasMethod>(probs, probs.size(), args.Repeat)) {
        Cout << '\t' << "OK" << Endl;
    } else {
        Cout << '\t' << "FAIL" << Endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int MainLoad(const int argc, const char* argv[]) {
    const auto args = ParseOptions(argc, argv);
    return Main(args);
}
