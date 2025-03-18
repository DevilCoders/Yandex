#include <dict/recognize/queryrec/factormill.h>
#include <dict/recognize/queryrec/queryrecognizer.h>

#include <kernel/geodb/geodb.h>

#include <library/cpp/getopt/opt.h>
#include <library/cpp/langs/langs.h>
#include <library/cpp/streams/factory/factory.h>

#include <util/charset/wide.h>
#include <util/folder/path.h>
#include <util/generic/string.h>
#include <util/stream/file.h>
#include <util/stream/input.h>
#include <util/stream/output.h>
#include <util/string/cast.h>
#include <util/system/hp_timer.h>

using namespace NQueryRecognizer;

namespace {
    enum EOutputMode {
        FULL = 0,
        PREFERED,
        MAIN
    };

    struct TArgs {
        TFsPath InputPath;
        TFsPath OutputPath;
        bool PrintTime;
        TFsPath WeightsPath;
        TFsPath DictionaryPath;
        TFsPath C2PPath;
        TFsPath GeoDBPath;
        TFsPath RegexPath;
        TQueryRecognizer::TUserData UserData;
        EOutputMode OutputMode;
    };
}  // namespace

static TArgs ParseOptions(const int argc, const char* argv[]) {
    auto args = TArgs{};
    auto parser = NLastGetopt::TOpts::Default();
    parser.AddHelpOption('h');
    parser.AddLongOption("output")
          .DefaultValue("-")
          .RequiredArgument("PATH")
          .StoreResult(&args.OutputPath)
          .Help("Output file");
    parser.AddCharOption('t')
          .Optional()
          .NoArgument()
          .SetFlag(&args.PrintTime)
          .Help("Print processing time");
    parser.AddCharOption('w')
          .Optional()
          .RequiredArgument("PATH")
          .StoreResult(&args.WeightsPath)
          .Help("Factor weights");
    parser.AddCharOption('g')
          .Optional()
          .RequiredArgument("PATH")
          .StoreResult(&args.C2PPath)
          .Help("Geo code hierarchy file (*.c2p)");
    parser.AddLongOption("geodb")
          .Optional()
          .RequiredArgument("PATH")
          .StoreResult(&args.GeoDBPath)
          .Help("GeoDB data (will be used instead of one provided by -g option");
    parser.AddCharOption('p')
          .Optional()
          .RequiredArgument("PATH")
          .StoreResult(&args.RegexPath)
          .Help("File with regular expression for query parsing");
    parser.AddCharOption('d')
          .Optional()
          .RequiredArgument("TLD")
          .StoreResult(&args.UserData.Domain)
          .Help("Yandex top-level domain");
    parser.AddCharOption('r')
          .Optional()
          .RequiredArgument("LR")
          .StoreResult(&args.UserData.Region)
          .Help("Region (LR)");
    parser.AddCharOption('u')
          .Optional()
          .RequiredArgument("UIL")
          .StoreMappedResultT<TStringBuf>(&args.UserData.Interface, &LanguageByName);
    parser.AddCharOption('o')
          .DefaultValue("full")
          .RequiredArgument("MODE")
          .Help("Can be 'full', 'prefered', or 'main'")
          .StoreMappedResultT<TStringBuf>(&args.OutputMode, [](const TStringBuf& value) {
              if (value == "prefered") {
                  return PREFERED;
              } else if (value == "main") {
                  return MAIN;
              } else if (value == "full") {
                  return FULL;
              } else {
                  Cerr << "Not valid output mode is specified. Default mode (full) will be used."
                       << Endl;
                  return FULL;
              }
          });
    parser.SetFreeArgsMin(1);
    parser.SetFreeArgsMax(2);
    parser.SetFreeArgTitle(0, "DICT", "Dictionary path");
    parser.SetFreeArgTitle(1, "INPUT", "Input file (default: \"-\")");
    const NLastGetopt::TOptsParseResult parseResult{&parser, argc, argv};
    args.DictionaryPath = parseResult.GetFreeArgs()[0];
    if (parseResult.GetFreeArgCount() == 2) {
        args.InputPath = parseResult.GetFreeArgs()[1];
    } else {
        args.InputPath = "-";
    }

    return args;
}

static void Recognize(const TQueryRecognizer& solver, const TQueryRecognizer::TUserData& userdata,
                      const EOutputMode& outputMode, const NGeoDB::TGeoKeeper* const geodb,
                      IInputStream& in, IOutputStream& out) {
    TString line;
    TUtf16String query;
    size_t linecount = 0;

    while (in.ReadLine(line)) {
        ++linecount;
        if (line.empty() || line.StartsWith('#'))
            continue;

        size_t sep = line.find('\t');
        if (sep == TString::npos) {
            query = UTF8ToWide(line);
        } else {
            query = UTF8ToWide(line.begin(), sep);
        }

        const auto answer = solver.RecognizeParsedQueryLanguage(query, &userdata, geodb);
        if (outputMode == PREFERED) {
            out << line << '\t' << answer.GetBestLangs() << Endl;
        } else if (outputMode == MAIN) {
            out << line << '\t' << NameByLanguage(answer.GetMainLang()) << Endl;
        } else {
            out << line << '\t' << answer << Endl;
        }
    }
}

static void DumpFeatures(const TFsPath& regexPath, const TFactorMill& mill,
                         IInputStream& in, IOutputStream& out) {
    TString line;
    size_t linecount = 0;

    TVector<TFactorSet> factors;

    TQueryCleaner queryCleaner;
    if (regexPath) {
        TFileInput regexrStream(regexPath);
        queryCleaner.LoadRegExpFile(regexrStream);
    }

    while (in.ReadLine(line)) {
        ++linecount;
        if (line.empty() || line.StartsWith('#')) {
            continue;
        }

        TString query = line;
        size_t sep = line.find('\t');
        if (sep != TString::npos) {
            query = TString(line.begin(), sep);
        }

        if (regexPath) {
            query = queryCleaner.ParseQuery(query);
        }

        const TUtf16String wquery = UTF8ToWide(query);
        mill.GetFactors(wquery, factors);
        for (auto  it = factors.begin(); it != factors.end(); ++it) {
            if(!it->empty()) {
                if (factors[0].empty()) {
                    out << line << '\t' << *it << Endl;
                }
                else {
                    if (it != factors.begin()) {
                        out << line << '\t' << *it << Endl;
                    }
                }
            }
        }
    }
}

static int Main(const TArgs& args, IInputStream& input, IOutputStream& output) {
    THPTimer timer;
    if (!args.WeightsPath) {
        TFactorMill factorMill(args.DictionaryPath.c_str());
        DumpFeatures(args.RegexPath, factorMill, input, output);
    } else {
        const auto factorMill = MakeSimpleShared<TFactorMill>(args.DictionaryPath.c_str());
        TQueryRecognizer recognizer(factorMill, args.WeightsPath.c_str(), args.C2PPath.c_str(), args.RegexPath.c_str());
        const auto geodb = args.GeoDBPath ? NGeoDB::TGeoKeeper::LoadToHolder(args.GeoDBPath)
                                          : THolder<NGeoDB::TGeoKeeper>{};
        Recognize(recognizer, args.UserData, args.OutputMode, geodb.Get(), input, output);
    }

    if (args.PrintTime) {
        const auto passed = timer.Passed();
        Cerr << "Time elapsed: " << FloatToString(passed, PREC_POINT_DIGITS, 3) << Endl;
    }

    return EXIT_SUCCESS;
}

static int Main(const TArgs& args) {
    const auto input = OpenInput(args.InputPath);
    const auto output = OpenOutput(args.OutputPath);
    return Main(args, *input, *output);
}

int main(const int argc, const char* argv[]) {
    const auto args = ParseOptions(argc, argv);
    return Main(args);
}
