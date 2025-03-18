#include "options.h"
#include <library/cpp/getopt/last_getopt.h>


namespace NSqueezeRunner {

TCliOpts ParseOptions(int argc, const char** argv) {
    TCliOpts opts;
    NLastGetopt::TOpts srcOpts = NLastGetopt::TOpts::Default();

    srcOpts.AddLongOption("server", "YT cluster name")
        .Optional()
        .StoreResult(&opts.Server)
        .DefaultValue("hahn");
    srcOpts.AddLongOption("pool", "YT pool")
        .Optional()
        .StoreResult(&opts.Pool);
    srcOpts.AddLongOption("date", "Day to squeeze, YYYYMMDD")
        //.Required()
        .StoreResult(&opts.Date);
    srcOpts.AddLongOption("lower-key", "Lower key")
        .Optional()
        .StoreResult(&opts.LowerKey);
    srcOpts.AddLongOption("upper-key", "Upper key")
        .Optional()
        .StoreResult(&opts.UpperKey);
    srcOpts.AddLongOption("user-sessions", "Path to user sessions log in YAMR-with-subkey format")
        .StoreResult(&opts.UserSessionsFile);
    srcOpts.AddLongOption("dst-folder", "Destination folder")
        .DefaultValue("//tmp/mstand")
        .StoreResult(&opts.DstFolder);
    srcOpts.AddLongOption("blockstat", "Path to blockstat.dict file")
        .StoreResult(&opts.BlockstatFile)
        .DefaultValue("blockstat.dict");
    srcOpts.AddLongOption("mode", "Run mode: local or yt (default: yt)")
        .DefaultValue(ESqueezeRunMode::YT)
        .StoreResult(&opts.RunMode);
    srcOpts.AddLongOption("testids", "Experiment testid list")
        .Handler1T<TString>([&opts](const TString& testid) {
            opts.Testids.push_back(testid);
        });
    srcOpts.AddLongOption("services", "Services list to squeeze")
        .Handler1T<TString>([&opts](const TString& service) {
            opts.Services.push_back(service);
        });
    srcOpts.AddLongOption("squeeze-params", "Path to file with squeeze params")
        .StoreResult(&opts.SqueezeParamsFile);
    srcOpts.AddLongOption("output", "Path to output file")
        .StoreResult(&opts.OutputFile);

    NLastGetopt::TOptsParseResult resOpts(&srcOpts, argc, argv);

    return opts;
}

};
