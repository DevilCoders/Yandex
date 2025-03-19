#include "lib/factory.h"

#include <library/cpp/getopt/small/last_getopt.h>

#include <util/folder/path.h>
#include <util/string/join.h>
#include <util/system/progname.h>

namespace {

using namespace NCloud::NBlockStore::NClient;

using namespace NLastGetopt;

////////////////////////////////////////////////////////////////////////////////

class TApp
{
private:
    TCommandPtr Handler = nullptr;

public:
    static TApp& Instance()
    {
        return *Singleton<TApp>();
    }

    void Shutdown()
    {
        if (Handler) {
            Handler->Shutdown();
        }
    }

    int Run(int argc, const char* argv[])
    {
        TOpts opts;
        opts.AddHelpOption('h');
        opts.SetFreeArgsNum(1);

        opts.SetTitle("Command line NBS client");
        opts.SetFreeArgTitle(0, "<command>", JoinSeq(" | ", GetHandlerNames()));

        try {
            if (argc < 2) {
                ythrow NLastGetopt::TUsageException() << "not enough arguments";
            }

            if (argc == 2) {
                TStringBuf arg(argv[1]);
                if (arg == "-h" || arg == "--help") {
                    opts.PrintUsage(GetProgramName());
                    return 0;
                }
            }

            auto command = TCommand::NormalizeCommand(argv[1]);
            --argc; ++argv;

            if (command == "mountvolume" || command == "unmountvolume") {
                ythrow yexception()
                    << "MountVolume and UnmountVolume requests are executed "
                    << "automatically as required";
            }

            Handler = GetHandler(command);
            if (!Handler) {
                ythrow yexception() << "unknown command: " << command;
            }

            bool res = Handler->Run(argc, argv);
            if (!res) {
                return 1;
            }
        } catch (const NLastGetopt::TException& e) {
            Cerr << GetProgramName() << "failed: " << e.what() << Endl;
            if (Handler) {
                Handler->PrintUsage();
            } else {
                opts.PrintUsage(GetProgramName());
            }

            return 1;
        } catch (...) {
            TString opts = {};
            if (argc > 1) {
                opts = " " + JoinRange(" ", &argv[0], &argv[argc]);
            }

            Cerr << GetProgramName() << opts << " failed: " << CurrentExceptionMessage() << Endl;

            return 1;
        }

        return 0;
    }
};

void Shutdown(int signum)
{
    Y_UNUSED(signum);
    TApp::Instance().Shutdown();
}

void ConfigureSignals()
{
    std::set_new_handler(abort);

    // make sure that errors can be seen by everybody :)
    setvbuf(stdout, nullptr, _IONBF, 0);
    setvbuf(stderr, nullptr, _IONBF, 0);

    // mask signals
    signal(SIGPIPE, SIG_IGN);

    struct sigaction sa = {};
    sa.sa_handler = Shutdown;

    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);
}

} // namespace

////////////////////////////////////////////////////////////////////////////////

int main(int argc, const char* argv[])
{
    ConfigureSignals();
    return TApp::Instance().Run(argc, argv);
}
