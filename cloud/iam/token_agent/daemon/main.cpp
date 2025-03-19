#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/logger/global/global.h>
#include <library/cpp/sighandler/async_signals_handler.h>
#include <google/protobuf/message_lite.h>
#include <util/generic/serialized_enum.h>

#include <logging_interceptor.h>
#include <server.h>

const std::string DEFAULT_CONFIG("/etc/yc/token-agent/config.yaml");

using namespace NTokenAgent;

int main(int argc, char** argv) {
    try {
        NLastGetopt::TOpts opts;
        NLastGetopt::TOpt& opt_config_file =
            opts.AddLongOption('c', "config", "set configuration file path")
                .DefaultValue(DEFAULT_CONFIG);
        NLastGetopt::TOpt& opt_log_level =
            opts.AddLongOption('l', "log-level", "set log level: " + GetEnumAllNames<ELogPriority>())
                .DefaultValue(TLOG_INFO);
        NLastGetopt::TOpt& opt_log_output =
            opts.AddLongOption('o', "log-output", "set log output")
                .DefaultValue("");
        NLastGetopt::TOpt& opt_access_log_output =
            opts.AddLongOption('a', "access-log-output", "set access log output")
                .DefaultValue("");
        opts.AddHelpOption('\0');

        THolder<NLastGetopt::TOptsParseResult> r;
        r.Reset(new NLastGetopt::TOptsParseResult(&opts, argc, argv));
        std::string log_level(r->Get(&opt_log_level));
        std::transform(log_level.begin(), log_level.end(), log_level.begin(), ::toupper);
        ::DoInitGlobalLog(LogBackend(r->Get(&opt_log_output), FromString<ELogPriority>(log_level)));
        LoggingInterceptor::Init(LogBackend(r->Get(&opt_access_log_output)));

        auto config = TConfig::FromFile(r->Get(&opt_config_file));
        auto server = TServer(config);

        auto signalFunction = [&server](int signum) {
            INFO_LOG << "Break signalled (" << signum << ")\n";
            server.StopServer();
        };
        ::SetAsyncSignalFunction(SIGTERM, signalFunction);
        ::SetAsyncSignalFunction(SIGINT, signalFunction);

        auto updateFunction = [&server](int signum) {
            INFO_LOG << "Force refresh tokens (" << signum << ")\n";
            server.UpdateTokens();
        };
        ::SetAsyncSignalFunction(SIGHUP, updateFunction);

        server.RunServer();

        ::SetAsyncSignalFunction(SIGTERM, nullptr);
        ::SetAsyncSignalFunction(SIGINT, nullptr);
        ::SetAsyncSignalFunction(SIGHUP, nullptr);

        INFO_LOG << "All done\n";
    } catch (const std::exception& ex) {
        ERROR_LOG << "Unhandled exception in main: " << ex.what();
        return 1;
    }
    google::protobuf::ShutdownProtobufLibrary();
    return 0;
}
