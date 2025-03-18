#include "options.h"
#include "config.h"

#include <util/generic/string.h>

namespace {
    const TString OPT_GEN("generate");
    const TString OPT_CHK("check");
    const TString OPT_HOST("host");
    const TString OPT_RESPONSE_TIME("response-time-microseconds");
    const char OPT_PORT = 'p';
}

NLastGetopt::TOpts CreateOptions() {
    NLastGetopt::TOpts opts = NLastGetopt::TOpts::Default();
    opts.SetFreeArgsNum(0);
    if (NLastGetopt::TOpt* helpOpt = opts.FindLongOption("help")) {
        helpOpt->AddShortName('h');
    }

    opts.AddCharOption(OPT_PORT, NLastGetopt::REQUIRED_ARGUMENT, "port")
        .Required()
        .RequiredArgument("<ui16>");
    opts.AddLongOption(OPT_GEN, "Strategy for handling /generate requests. Possible values:\n"
                                "  correct - always reply correct XML, http://doc.yandex-team.ru/Passport/captcha/concepts/api_generate.xml\n"
                                "  incorrect - always reply incorrect XML\n"
                                "  timeout - take too much time to reply")
        .HasArg(NLastGetopt::REQUIRED_ARGUMENT)
        .RequiredArgument("<string>")
        .DefaultValue("correct");
    opts.AddLongOption(OPT_CHK, "Strategy for handling /check requests. Possible values:\n"
                                "  success - check always succeeds\n"
                                "  fail - check always fails\n"
                                "  random - check succeeds with probablity of 50%\n"
                                "  incorrect - always reply incorrect XML\n"
                                "  timeout - take too much time to reply")
        .HasArg(NLastGetopt::REQUIRED_ARGUMENT)
        .RequiredArgument("<string>")
        .DefaultValue("fail");

    opts.AddLongOption(OPT_HOST, "Hostname on which this service is running\n"
                                 "If specified, captcha image willbe  generated and available according to <hostname>:<port>")

        .HasArg(NLastGetopt::REQUIRED_ARGUMENT)
        .RequiredArgument("<string>")
        .DefaultValue("");

    opts.AddLongOption(OPT_RESPONSE_TIME, "Time to wait before response")
        .HasArg(NLastGetopt::OPTIONAL_ARGUMENT)
        .RequiredArgument("<float>")
        .DefaultValue(0.0);

    return opts;
}

ui16 ParsePort(const NLastGetopt::TOptsParseResult& opts) {
    return opts.Get<ui16>(OPT_PORT);
}

TDuration ParseResponseWaitTime(const NLastGetopt::TOptsParseResult& opts) {
    float microseconds = opts.Get<float>(OPT_RESPONSE_TIME);
    return TDuration::MicroSeconds(microseconds);
}

THashMap<TStringBuf, TPathHandler> ParseHandlers(const NLastGetopt::TOptsParseResult& opts) {
    THashMap<TStringBuf, TPathHandler> res;
    res[TStringBuf("/ping")] = &PingHandler;
    res[TStringBuf("/image")] = &ImageHandler;
    res[TStringBuf("/generate")] = TGenerateHandler(opts.Get(OPT_HOST), opts.Get<ui16, char>(OPT_PORT));
    res[TStringBuf("/check")] = TCheckHandler(opts.Get(OPT_HOST), opts.Get<ui16, char>(OPT_PORT));
    res[TStringBuf("/set_strategy")] = &SetStrategyHandler;
    STRATEGY_CONFIG.GenerateStrategy = opts.Get(OPT_GEN);
    STRATEGY_CONFIG.CheckStrategy = opts.Get(OPT_CHK);
    return res;
}
