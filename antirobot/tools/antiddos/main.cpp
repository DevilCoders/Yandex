#include "lib/antiddos_tool.h"


#include <library/cpp/getopt/last_getopt.h>


int main(int argc, char** argv) {
    using namespace NLastGetopt;

    TSettings settings;

    TOpts opts;
    opts.AddLongOption("req-log", "file name with saved requests. Default stdin").StoreResult(&settings.RequestLogName).OptionalArgument("file name");
    opts.AddLongOption("req-log-txt", "saved requests are quoted text-based requestes, one line - one request").SetFlag(&settings.ReqLogTxt).NoArgument();
    opts.AddLongOption("print-full", "print full request").SetFlag(&settings.PrintFullReq).NoArgument();
    opts.AddLongOption("clear", "clear existing rules ").SetFlag(&settings.ClearExistRules).NoArgument();
    opts.AddLongOption("cbb-host", "CBB host:port(default: cbb.yandex.net:80)").StoreResult(&settings.CbbHost).OptionalArgument("cbb-host");
    opts.AddLongOption("cbb-flag", "CBB flag (default: 183 antirobot-ddos-tool").StoreResult(&settings.CbbFlag).OptionalArgument("number");
    opts.AddLongOption("json", "Print result as JSon").SetFlag(&settings.JsonOutput).NoArgument();
    opts.AddLongOption("stat", "Print statistics when check requests").SetFlag(&settings.PrintStat).NoArgument();
    opts.AddLongOption("print-matched", "Print matched requests").SetFlag(&settings.PrintMatched).NoArgument();
    opts.AddHelpOption();

    opts.SetFreeArgsMin(2);
    opts.SetFreeArgsMax(2);
    opts.SetFreeArgTitle(0, "<command>", "check, parse, store");
    opts.SetFreeArgTitle(1, "<rules-file>", "File with rules. One line - one rule. '-' for stdin");

    TOptsParseResult res(&opts, argc, argv);
    TVector<TString> freeArgs = res.GetFreeArgs();

    settings.Cmd = freeArgs[0];
    settings.RulesFile = freeArgs[1];

    try {
        if (settings.Cmd == "parse") {
            ParseRules(ReadRules(settings.RulesFile));
            Cout << PrintOK(settings.JsonOutput, "Parsed OK!") << Endl;
        } else if (settings.Cmd == "check") {
            CheckSavedReqsWithRules(settings);
        } else if (settings.Cmd == "store") {
            StoreNewRules(settings);
        } else {
            Cerr << "Unknown command." << Endl;
            return 2;
        }

        return 0;
    } catch (...) {
        const auto exception = CurrentExceptionMessage();
        if (!settings.JsonOutput) {
            Cerr << exception << Endl;
            return 1;
        }

        Cout << PrintError(settings.JsonOutput, exception) << Endl;
    }

    return 0;
}
