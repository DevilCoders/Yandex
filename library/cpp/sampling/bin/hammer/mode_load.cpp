#include "common.h"
#include "modes.h"

#include <library/cpp/sampling/alias_method.h>
#include <library/cpp/sampling/roulette_wheel.h>

#include <library/cpp/getopt/small/last_getopt.h>
#include <library/cpp/streams/factory/factory.h>

#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/random/entropy.h>
#include <util/random/fast.h>
#include <util/stream/format.h>
#include <util/stream/output.h>

namespace {
    struct TArgs {
        size_t Min = {};
        size_t Max = {};
        size_t Step = {};
        size_t Repeat = {};
        TString FailedPath = {};
    };
}

static TArgs ParseOptions(const int argc, const char* argv[]) {
    auto parser = NLastGetopt::TOpts::Default();
    auto args = TArgs{};
    parser.AddLongOption("min")
        .RequiredArgument("UINT")
        .Required()
        .StoreResult(&args.Min);
    parser.AddLongOption("max")
        .RequiredArgument("UINT")
        .Required()
        .StoreResult(&args.Max);
    parser.AddLongOption("step")
        .RequiredArgument("UINT")
        .Required()
        .StoreResult(&args.Step);
    parser.AddLongOption("repeat")
        .RequiredArgument("UINT")
        .DefaultValue("100")
        .StoreResult(&args.Repeat);
    parser.AddLongOption("failed")
        .RequiredArgument("FILE")
        .StoreResult(&args.FailedPath);
    parser.SetFreeArgsNum(0);
    const NLastGetopt::TOptsParseResult parseResult{&parser, argc, argv};
    return args;
}

static TVector<double> Generate(const size_t size) {
    auto res = TVector<double>(size);
    auto&& RNG = TFastRng<ui64>{Seed()};
    for (auto& value : res) {
        value = RNG.GenRandReal3();
    }

    return res;
}

static void Write(const TVector<double>& data, const size_t size, const TString& path) {
    const auto output = OpenOutput(path);
    *output << size << '\n';
    for (auto index = size_t{}; index < size; ++index) {
        *output << Prec(data[index], PREC_POINT_DIGITS, 16) << '\n';
    }
}

static int Main(const TArgs& args) {
    const auto probs = Generate(args.Max);
    for (auto size = args.Min; size <= args.Max; size += args.Step) {
        Cout << "size: " << size << Endl;

        Cout << '\t' << "TRouletteWheel..." << Endl;
        if (DoTest<NSampling::TRouletteWheel>(probs, size, args.Repeat)) {
            Cout << '\t' << "OK" << Endl;
        } else {
            Cout << '\t' << "FAIL" << Endl;
            Write(probs, size, args.FailedPath);
            return EXIT_FAILURE;
        }

        Cout << '\t' << "TAliasMethod..." << Endl;
        if (DoTest<NSampling::TAliasMethod>(probs, size, args.Repeat)) {
            Cout << '\t' << "OK" << Endl;
        } else {
            Cout << '\t' << "FAIL" << Endl;
            Write(probs, size, args.FailedPath);
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}

int MainRandom(const int argc, const char* argv[]) {
    const auto args = ParseOptions(argc, argv);
    return Main(args);
}
