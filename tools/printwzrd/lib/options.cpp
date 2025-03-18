#include "options.h"

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/svnversion/svnversion.h>

#include <util/generic/string.h>
#include <util/stream/output.h>
#include <library/cpp/streams/factory/factory.h>
#include <util/string/cast.h>
#include <util/string/strip.h>
#include <util/string/vector.h>
#include <util/folder/path.h>
#include <util/string/split.h>

TPrintwzrdOptions::TPrintwzrdOptions()
    : FastLoader(true)
    , PrintDolbilka(false)
    , PrintExtraOutput(true)
    , PrintLemmaCount(false)
    , PrintQtree(false)
    , PrintSuccessfulRules(false)
    , PrintRichTree(false)
    , TabbedInput(false)
    , VerifyGazetteer(false)
    , CompareReqRemote(false)
    , NoCache(false)
    , SortOutput(false)
    , PrintMarkup(false)
    , Port(0)
    , Host("localhost")
    , CgiInput(false)
    , DebugMode(false)
    , CollectStat(false)
    , PrintSrc(true)
    , ThreadCount(0)
{
}

static void ParseCompareRemote(TPrintwzrdOptions& options, const TString& optValue) {
    TVector<TString> names;
    StringSplitter(optValue).Split(' ').SkipEmpty().Collect(&names);
    if (names.size() != 2) {
        Cerr << "Error with -r option. Use: -r 'file1 file2'." << Endl;
        exit(1);
    }

    options.CompareReqRemote = true;
    options.LocalWizardOutFile = names[0];
    options.RemoteWizardOutFile = names[1];
}

void TPrintwzrdOptions::Reset(int argc, char *argv[]) {
    using namespace NLastGetopt;
    TOpts opts;

    TOpt& optVersion = opts.AddLongOption('v', "version", "Print revision and exit").NoArgument();

    TOpt& optArcadiaTestsData = opts.AddLongOption('a', "data", "Path to arcadia_tests_data").RequiredArgument("PATH").DefaultValue("");
    TOpt& optWizardRuntime = opts.AddLongOption('R', "runtime-data", "Path to wizard.runtime").OptionalArgument("PATH").DefaultValue("");
    TOpt& optWizardRealtime = opts.AddLongOption('E', "realtime-data", "Path to wizard.realtime").OptionalArgument("PATH").DefaultValue("");

    TOpt& optConfig = opts.AddLongOption('s', "config", "Path to config").RequiredArgument("PATH").DefaultValue("");
    TOpt& optEventLong = opts.AddLongOption('l', "evlog", "Eventlog path").RequiredArgument("PATH").DefaultValue("");

    TOpt& optHost = opts.AddLongOption('h', "host", "Remote wizard host (not supported for >3 years, may fail)").RequiredArgument("HOSTNAME").DefaultValue("");
    TOpt& optPort = opts.AddLongOption('p', "port", "Remote wizard port (not supported for >3 years, may fail)").RequiredArgument("PORT").DefaultValue("8891");
    TOpt& optProduction = opts.AddLongOption("production", "Use reqwizard.yandex.net:8891 as a remote wizard").NoArgument().Hidden();

    TOpt& optCgiInput = opts.AddLongOption('i', "cgi-input", "Cgi input: [text=%EC%E8%F0%F1%E8%E4%E5%F1&lr=213]").NoArgument();
    TOpt& optTabbedInput = opts.AddLongOption('b', "tabbed-input", "Alternative query format: [query \\t lr]").NoArgument();

    TOpt& optDebugMode = opts.AddLongOption('w', "debug", "Add dbgwzr=2 to query").NoArgument();
    TOpt& optCollectStat = opts.AddLongOption("stat", "Collect cache statistics and print it on exit").NoArgument();

    TOpt& optCompactOutput = opts.AddLongOption('c', "compact", "Do not print extra output").NoArgument();
    /*TOpt& optSimpleOutput =*/ opts.AddLongOption('x', "simple-output", "DEPRECATED, has ho effect").NoArgument();

    TOpt& optMarkup = opts.AddLongOption('m', "markup", "Print external markup (JSON)").NoArgument();

    TOpt& optNoMmap = opts.AddLongOption('k', "no-mmap", "Do not use mmap to load files (precharge)").NoArgument();
    TOpt& optNoCache = opts.AddLongOption("nocache", "Do not use cache").NoArgument();
    TOpt& optSortOutput = opts.AddLongOption("sort", "Sort rearr, relev, and rules in alphabetic order").NoArgument();
    TOpt& optTestRun = opts.AddLongOption("test", "Signal that printwzrd was run by automated tests").NoArgument();
    TOpt& optNoSrc = opts.AddLongOption("nosrc", "Do not print src section").NoArgument();


    TOpt& optVerifyGzt = opts.AddLongOption('g', "verify-gzt",
        "Verify main gazetteer binary to be consistent with its sources").NoArgument();

    TOpt& optCompareRemote = opts.AddLongOption('r', "compare-remote",
        "Compare results of request wizard and remote wizard ['RequestOutFile RemoteOutFile']").RequiredArgument("'FILE1 FILE2'");

    TOpt& optDolbilka = opts.AddLongOption('d', "dolbilka", "Print results for dbg_dolbilka").NoArgument();
    TOpt& optLemmaCount = opts.AddLongOption('z', "lemma-count", "Print lemma count").NoArgument();
    TOpt& optPrintQtree = opts.AddLongOption('t', "qtree", "Print qtree").NoArgument();
    TOpt& optPrintSuccessfulRules = opts.AddLongOption('f', "success-rules", "Print successful rules list").NoArgument();
    TOpt& optPrintRichTree = opts.AddLongOption('e', "richtree", "Print richtree").NoArgument();

    TOpt& optAddCgi = opts.AddLongOption("add-cgi", "Append specified cgi-params to each query").RequiredArgument("CGI");

    TOpt& optRulesList = opts.AddLongOption("rules", "Use default config and the specified list of rules").RequiredArgument("LIST");
    TOpt& optRulesToPrint = opts.AddLongOption("print", "Print only the output of the specified list of rules").RequiredArgument("LIST");

    TOpt& optThreadCount = opts.AddLongOption('j', "thread-count", "Count of parallel wizard threads").RequiredArgument("COUNT");

    TOpt& optInput = opts.AddLongOption("input", "Input file name").RequiredArgument("FILE");
    TOpt& optOutput = opts.AddLongOption("output", "Output file name").RequiredArgument("FILE");
    opts.SetFreeArgsNum(0);

    TOptsParseResult r(&opts, argc, argv);

    if (r.Has(&optInput))
        InputFileName = r.Get(&optInput);

    if (r.Has(&optOutput))
        OutputFileName = r.Get(&optOutput);

    if (r.Has(&optVersion)) {
        *OpenOutput(OutputFileName) << GetProgramSvnRevision() << Endl;
        exit(0);
    }

    if (r.Has(&optCompareRemote))
        ParseCompareRemote(*this, r.Get(&optCompareRemote));


    WorkDir = r.Get(&optArcadiaTestsData);
    PathToConfig = r.Get(&optConfig);
    EventLogPath = r.Get(&optEventLong);

    if (r.Has(&optRulesList))
        RulesList = r.Get(&optRulesList);

    if (r.Has(&optRulesToPrint))
        RulesToPrint = r.Get(&optRulesToPrint);

    if (r.Has(&optProduction) || r.Has(&optPort) || r.Has(&optHost)) {
        Cerr << "WARNING: use -h and -p options at your own risk: they received no support for over 3 years and may not work (but may work). Anyway, there is no guarantee.\n";
    }

    if (r.Has(&optProduction)) {
        Host = "reqwizard.yandex.net";
        Port = 8891;
    } else {
        Host = r.Get(&optHost);
        if (r.Has(&optPort)) {
            try {
                Port = FromString<int>(StripString(TString(r.Get(&optPort))));
            } catch (yexception&) {
                Cerr << "Invalid port specified: " << r.Get(&optPort) << Endl;
                exit(1);
            }
        }
    }

    auto checkDataDir = [&r, &opts](const TOpt* option, TString& dirPath) {
        if (r.Has(option)) {
            dirPath = r.Get(option);
            if (!TFsPath(dirPath).IsDirectory()) {
                Cerr << "Runtime directory " << dirPath <<  " doesn't exist" << Endl;
                opts.PrintUsage("wizard");
                exit(1);
            }
        }
    };

    checkDataDir(&optWizardRuntime, CustomRuntimeData);
    checkDataDir(&optWizardRealtime, CustomRealtimeData);



    CgiInput = r.Has(&optCgiInput);
    TabbedInput = r.Has(&optTabbedInput);

    PrintExtraOutput = !r.Has(&optCompactOutput);

    PrintDolbilka = r.Has(&optDolbilka);
    PrintLemmaCount = r.Has(&optLemmaCount);
    PrintQtree = r.Has(&optPrintQtree);
    PrintSuccessfulRules = r.Has(&optPrintSuccessfulRules);
    PrintRichTree = r.Has(&optPrintRichTree);

    FastLoader = !r.Has(&optNoMmap);
    NoCache = r.Has(&optNoCache);
    SortOutput = r.Has(&optSortOutput);
    IsTestRun = r.Has(&optTestRun);
    VerifyGazetteer = r.Has(&optVerifyGzt);

    DebugMode = r.Has(&optDebugMode);
    CollectStat = r.Has(&optCollectStat);
    PrintMarkup = r.Has(&optMarkup);
    PrintSrc = !r.Has(&optNoSrc);

    if (r.Has(&optThreadCount)) {
        try {
            ThreadCount = FromString<int>(StripString(TString(r.Get(&optThreadCount))));
        } catch (yexception&) {
            Cerr << "Invalid thread count specified: " << r.Get(&optThreadCount) << Endl;
            exit(1);
        }
    }

    if (r.Has(&optAddCgi))
        AppendCgi = r.Get(&optAddCgi);

    if (CompareReqRemote && (!OkLocal() || !OkRemote()) && (!!LocalWizardOutFile || !!RemoteWizardOutFile))  {
        Cerr << "{-r|--compare-remote} and {-s|--config} and {-a|--data} and {-h|--host} and {-p|--port} options are required" << Endl;
        opts.PrintUsage("printwzrd");
        exit(1);
    }

    if (!OkLocal() && !OkRemote()) {
        Cerr << "{-s|--config} and {-a|--data} or {-r|--compare-remote} and {-a|--data} or {-h|--host} and {-p|--port} or {-v|--version} options are required" << Endl;
        opts.PrintUsage("printwzrd");
        exit(1);
    }
}
