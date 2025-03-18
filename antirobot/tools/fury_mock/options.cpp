#include "options.h"
#include "config.h"

#include <util/generic/string.h>

namespace {
    const TString OPT_HOST("host");
    const TString OPT_RESPONSE_TIME("response-time-microseconds");
    const char OPT_PORT = 'p';
}

TPathHandler GetGenerateHandler(const TString& host, int port) {
    return TCheckHandler(host, port);
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
    opts.AddLongOption("check", "Strategy for handling / requests.")
        .HasArg(NLastGetopt::REQUIRED_ARGUMENT)
        .RequiredArgument("<string>")
        .DefaultValue("as_image");
    opts.AddLongOption("button-check", "Strategy for handling / requests.")
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
    res[TStringBuf("/")] = GetGenerateHandler(opts.Get(OPT_HOST), opts.Get<ui16, char>(OPT_PORT));
    res[TStringBuf("/set_strategy")] = &SetStrategyHandler;
    STRATEGY_CONFIG.Strategy = opts.Get("check");
    STRATEGY_CONFIG.ButtonStrategy = opts.Get("button-check");
    return res;
}
