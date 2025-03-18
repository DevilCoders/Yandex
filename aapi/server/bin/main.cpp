#include <aapi/lib/sensors/sensors.h>
#include <aapi/lib/trace/trace.h>
#include <aapi/server/config.h>
#include <aapi/server/server.h>

#include <library/cpp/getopt/opt.h>

#include <util/string/join.h>
#include <util/string/strip.h>

#include <contrib/libs/grpc/include/grpc++/server_builder.h>

using namespace NLastGetopt;

int main(int argc, const char** argv) {
    NYT::Initialize(argc, argv);

    TString configPath;
    TString ytTokenPath;

    {
        TOpts opts;
        opts.AddHelpOption('h');
        opts.AddLongOption('c', "config-path", "path to config file")
            .Required()
            .StoreResult(&configPath);
        opts.AddLongOption("yt-token-path", "YT Token Path")
            .Optional()
            .StoreResult(&ytTokenPath);

        TOptsParseResult res(&opts, argc, argv);
    }

    NAapi::TConfig config = NAapi::TConfig::FromFile(configPath);

    NAapi::InitTrace(config.EventlogPath, config.DebugOutput);
    NAapi::InitSolomonSensors(config.Host, config.SensorsPort);

    if (ytTokenPath) {
        config.YtToken = Strip(TFileInput(ytTokenPath).ReadAll());
    }

    NAapi::TVcsServer server(config);
    Cerr << "Server listening on " << Join(":", "[::]", config.AsyncPort) << Endl;
    server.JoinThreads();

    return 0;
}
