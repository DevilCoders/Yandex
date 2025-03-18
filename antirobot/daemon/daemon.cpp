#include <antirobot/daemon_lib/cl_params.h>
#include <antirobot/daemon_lib/config_global.h>
#include <antirobot/daemon_lib/dynamic_thread_pool.h>
#include <antirobot/daemon_lib/server.h>
#include <antirobot/lib/segv_handler.h>

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/logger/global/global.h>
#include <library/cpp/svnversion/svnversion.h>

#include <util/system/mlock.h>


using namespace NAntiRobot;

namespace {

void PrintVersionAndExit() {
    Cout << PROGRAM_VERSION << Endl;
    exit(0);
}

void ParseArgs(TCommandLineParams& clParams, int argc, char** argv) {
    using namespace NLastGetopt;
    TOpts opts = TOpts::Default();
    opts.SetFreeArgsNum(0);
    if (TOpt* helpOpt = opts.FindLongOption("help")) {
        helpOpt->AddShortName('h');
    }

    opts.AddLongOption('B', "base-dir", "base path for all data, logs, etc.")
        .StoreResult(&clParams.BaseDirName)
        .RequiredArgument("<string>");
    opts.AddLongOption('L', "logs-dir", "Path where logs are stored."
                                        " Relative paths are searched inside base directory "
                                        "(see \"BaseDir\" in config or -B option).")
        .StoreResult(&clParams.LogsDirName)
        .RequiredArgument("<string>");
    opts.AddLongOption('c', "config", "config file name")
        .StoreResult(&clParams.ConfigFileName)
        .Required()
        .RequiredArgument("<string>");
    opts.AddLongOption('p', "port", "service port")
        .StoreResult(&clParams.Port)
        .RequiredArgument("<int>")
        .DefaultValue("0");
    opts.AddLongOption('d', "debug", "more verbosely")
        .SetFlag(&clParams.DebugMode)
        .NoArgument();
    opts.AddLongOption("no-tvm", "don't use TVM client")
        .SetFlag(&clParams.NoTVMClient)
        .NoArgument();
    opts.AddLongOption('v', "version", "print full information about version and exit")
        .Handler(&PrintVersionAndExit)
        .NoArgument();

    TOptsParseResult res(&opts, argc, argv);
}

}


int main(int argc, char** argv) {
#ifndef NDEBUG
    SetFancySegvHandler();
#endif

    InitGlobalLog2Console(TLOG_WARNING);

    TCommandLineParams clParams;
    ParseArgs(clParams, argc, argv);

    if (ANTIROBOT_DAEMON_CONFIG.LockMemory) {
        LockAllMemory(ELockAllMemoryFlag::LockCurrentMemory | ELockAllMemoryFlag::LockFutureMemory);
    }

    TServer server(clParams);
    server.Run();

    GetAntiRobotDynamicThreadPool()->Stop();

    return 0;
}
