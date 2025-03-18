#include <tools/clustermaster/communism/client/client.h>
#include <tools/clustermaster/communism/client/parser.h>

#include <sys/wait.h>

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/svnversion/svnversion.h>

#include <util/generic/hash_set.h>
#include <util/generic/intrlist.h>
#include <util/generic/strbuf.h>
#include <util/generic/yexception.h>
#include <util/stream/output.h>
#include <util/string/vector.h>
#include <util/system/daemon.h>

void PrintVersionAndExit() noexcept {
    Cout << GetProgramSvnVersion() << Endl;
    exit(0);
}

void PrintSyntaxAndExit() noexcept {
    Cout << "Request syntax: key[:value][%name][,...][@solver]" << Endl;
    Cout << Endl;
    Cout << "Where: " << Endl;
    Cout << "    Key is a arbitrary string name of virtual resource to be acquired" << Endl;
    Cout << "    Value's syntax is numerator[/denominator], if it is omitted \"key\" will be acquired entirely" << Endl;
    Cout << "        If both numerator and denominator are specified, they define relative value to be acquired" << Endl;
    Cout << "        If no denominator specified, numerator defines absolute value to be acquired" << Endl;
    Cout << "    Name specifies a sharing name, if it is omitted acquiring will not be shared" << Endl;
    Cout << "    Solver's syntax is (host[:port])|path, if it is omitted 127.0.0.1:" << NCommunism::GetDefaultPort() << " is supposed" << Endl;
    Cout << "        Path specifies UNIX domain socket and should have at least one \"/\" to be distinguished from hostname" << Endl;
    Cout << Endl;
    Cout << "Examples: " << Endl;
    Cout << "    cpu" << Endl;
    Cout << "    mem:512,mem:1024%mapped_file,cpu:2.5/16@./solver.socket" << Endl;
    Cout << "    net:1/2,'midnight sync'@myhost:4321" << Endl;
    exit(0);
}

int main(int argc, char** argv) try {
    NCommunism::TClient<int> client;

    TString requestString;

    NCommunism::TDefinition definition;
    THolder<ISockAddr> solver(new TSockAddrInet(IpFromString("127.0.0.1"), NCommunism::GetDefaultPort()));
    NCommunism::TDetails details;

    TDuration timeout(TDuration::Max());
    bool claiming = false;
    bool asynchronousClaiming = false;
    bool reconnectUnreachable = false;

    TVector<char*> command;

    do {
        NLastGetopt::TOpts opts;

        opts.SetFreeArgsMin(1);
        opts.SetFreeArgsMax(1);
        opts.SetFreeArgTitle(0, "REQUEST -- COMMAND", "virtual resources to be acquired and command to execute");
        opts.AddCharOption('p', "priority in range [0.0, 1.0]").RequiredArgument("value");
        opts.AddCharOption('c', "claim instead of requesting").NoArgument();
        opts.AddCharOption('f', "immediately run command and asynchronously try to claim").NoArgument();
        opts.AddCharOption('t', "timeout").RequiredArgument("value");
        opts.AddCharOption('d', "expected duration").RequiredArgument("value");
        opts.AddCharOption('r', "try to reconnect unreachable solver").NoArgument();
        opts.AddLongOption("version", "print version and exit").NoArgument().Handler(&PrintVersionAndExit);
        opts.AddLongOption("syntax", "print request syntax and exit").NoArgument().Handler(&PrintSyntaxAndExit);
        opts.AddHelpOption('?');

        if (argc <= 1) {
            opts.PrintUsage(argv[0] ? argv[0] : "commie");
            exit(0);
        }

        int arg = 0;

        while (arg < argc && TStringBuf(argv[arg]) != "--") {
            ++arg;
        }

        NLastGetopt::TOptsParseResult optr(&opts, arg, argv);

        if (optr.Has('p')) {
            const double priority = FromString<double>(optr.Get('p'));

            if (priority < 0.0 || priority > 1.0) {
                throw yexception() << "\"" << requestString << "\": invalid priority value specified";
            }

            details.SetPriority(priority);
        }

        claiming = optr.Has('c');

        asynchronousClaiming = optr.Has('f');

        if (optr.Has('t')) {
            timeout = TDuration::Parse(optr.Get('t'));
        }

        if (optr.Has('d')) {
            details.SetDuration(TDuration::Parse(optr.Get('d')));
        }

        reconnectUnreachable = optr.Has('r');

        const TVector<TString>& freeArgs = optr.GetFreeArgs();

        if (!freeArgs.empty()) {
            requestString = *freeArgs.begin();
        }

        for (++arg; arg < argc; ++arg) {
            command.push_back(argv[arg]);
        }

        details.SetLabel(JoinVectorIntoString(command, TString(" ")));

        if (!command.empty()) {
            command.push_back(nullptr);
        }
    } while (false);

    NCommunism::ParseDefinition(requestString, definition, solver);

    if (!definition.GetClaim().size()) {
        throw yexception() << "\"" << requestString << "\": no claims specified";
    }

    client.Start();

    client.Define(0, *solver, definition);
    client.Details(0, details);

    if (claiming || asynchronousClaiming) {
        client.Claim(0, asynchronousClaiming);
    } else {
        client.Request(0, timeout);
    }

    for (bool granted = false; !granted;) {
        NCommunism::TClient<int>::TEvents events;

        client.WaitI();
        client.PullEvents(events);

        for (NCommunism::TClient<int>::TEvents::const_iterator i = events.begin(); i != events.end(); ++i) {
            if (i->Userdata != 0) {
                ythrow yexception() << "invalid event received";
            }

            switch (i->Status) {
            case NCommunism::BADVERSION:
                Cout << "\"" << requestString << "\": " << solver->ToString() << " has bad protocol version: " << i->Message << Endl;
                return 1;
                break;
            case NCommunism::CONNECTED:
                break;
            case NCommunism::RECONNECTING:
                if (reconnectUnreachable) {
                    Cout << "\"" << requestString << "\": reconnecting " << solver->ToString() << Endl;
                    break;
                } else {
                    Cout << "\"" << requestString << "\": " << solver->ToString() << " is unreachable" << Endl;
                    return 1;
                }
            case NCommunism::GRANTED:
                granted = true;
                break;
            case NCommunism::EXPIRED:
                Cout << "\"" << requestString << "\": expired" << Endl;
                return 0;
            case NCommunism::REJECTED:
                Cout << "\"" << requestString << "\": rejected by " << solver->ToString() << ": " << i->Message << Endl;
                return 1;
            default:
                ythrow yexception() << "unknown request status got";
            }
        }
    }

    if (!command.empty()) {
        const pid_t child = fork();

        if (child == -1) {
            ythrow TSystemError() << "fork failed";
        }

        if (child == 0) {
            // NDaemonMaker::CloseFrom(3);
            execvp(*command.begin(), command.data());
            Cerr << "execvp failed: " << LastSystemErrorText() << Endl;
            exit(1);
        } else {
            int status = 0;

            while (waitpid(child, &status, 0) == -1) {
                if (LastSystemError() != EINTR) {
                    ythrow TSystemError() << "waitpid failed";
                }
            }

            return WEXITSTATUS(status);
        }
    }

    return 0;
} catch (...) {
    Cout << CurrentExceptionMessage() << Endl;
    return 1;
}
