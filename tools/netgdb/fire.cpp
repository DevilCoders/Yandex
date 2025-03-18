#include <library/cpp/deprecated/prog_options/prog_options.h>
#include <sys/signal.h>
#include <util/stream/file.h>
#include <util/stream/str.h>
#include <util/string/strip.h>
#include <util/string/split.h>
#include <util/system/daemon.h>
#include <util/system/env.h>
#include <library/cpp/regex/pcre/regexp.h>

#include "comm.h"
#include "client.h"


const char* const OptionsList = "|-help|?|h|+|w|+|p|+|t|+|m|+|o|+|c|+|f|+|q|x|#|-ssh|-sky|-threads|+|-force|-user|+|";
void PrintHelp()
{
    Cout << "netgdb (v1.5)\tcluster wide debugging.\n"
            "\n"
            "   Information:\n"
            "    -?|--help         prints this screen\n"
            "    -q                don't print current task stages and their statuses\n"
            "   Effective hosts list:\n"
            "    -c <cluster-name> hosts list is taken from searcherlookup/yr for cluster name provided\n"
            "                     (both utils must be in /Berkanavt/bin/scripts or in PATH)\n"
            "    -h <hosts-file>   hosts list work with (hosts.txt by default)\n"
            "    -x <hosts-file>   excludes hosts from cluster\n"
//            "    -m <host>         master (where to get host list) host address\n"
            "    -f <regexp>       filter on hostnames\n"
            "   Remote startup:\n"
            "    -o <out-dir>      stores result of cluster task to local directory provided (result from concrete host is stored to file with host name)\n"
            "    -w <work-dir>     sets up work directory for debugged application\n"
            "    -t <temp-dir>     sets temporary dir on cluster hosts (never use)\n"
            "    -p <port>         enforce netgdb port (never use)\n"
            "   Modes:\n"
            "    --ssh             creates your own netgdb network on cluster\n"
            "    --sky             creates your own netgdb network on cluster upon skynet\n"
            "    --force           skip all errors and run command whereever it can be done\n"
            "    --threads <N>     use N threads within rsh tasks (start/distribute/etc.)\n"
            "\n"
            "Commands:\n"
            " ping\n"
            " start                       starts netgdb daemon on cluster\n"
            " stop                        stops debugger processes\n"
            " cleanup                     stops netgdb daemons and makes cleanup\n"
            " distribute <file>           distributes file to cluster\n"
            " launch <file> [<option>]*   launches command and exits (command rc is skipped!)\n"
            " debug <file> [<option>]*    runs <file> on cluster with provided options, gdb is used to startup\n"
            " shell <command>             execute shell command cluster wide (><\"&|' must be correctly quoted!)\n"
            " status [<task-id>]?         acquires tasks from effective hosts list/per host status of task (if <task-id> provided)\n"
            " verbose-status [<task-id>]? acquires tasks from effective hosts list/per host status of task (if <task-id> provided)\n"
            " kill <task-id>              kills task by <task-id> (taken from status command)\n"
            " killall                     kills all your tasks\n"
            " print-hosts                 print effective hosts list\n"
            " daemon                      you will never deal with this command until you want to debug netgdb itself"
         << Endl;
}

TIntrusivePtr<NNetliba_v12::IRequester> Comm;
NNetliba_v12::TColors RegularColor;
int RemotePortBase;
int HostIndexModule = 0;

const int DEFAULT_PORT = 20140;
TString ClientTempPath;
TString ClientWorkDir;
TString NetGDBName;
TString OutputPath;
TString HostsFile;
TString User;
int ThreadsCount;
bool Verbose;
bool RemoteStart = true;
ESHTransport RemoteTransport = SSH;//SKY;
bool ForceMode;

class TRemoteStarterChecker: public IResultChecker {
    TVector<TString> Hosts;
public:
    bool Check(const TString& /*host*/, TVector<char>& result, TRequestSet*) override
    {
        result.push_back(0);
        StringSplitter(result.begin()).Split('\t').SkipEmpty().Collect(&Hosts);
        TString status = Hosts[0];
        Hosts.erase(Hosts.begin());
        return status == "0";
    }

    const TVector<TString>& GetHosts()
    {
        return Hosts;
    }
};

TString GetDefaultMaster(const TString& cluster) {
    if (cluster == "C1") {
        return "porter026";
    }
    return "";
}

int main (int argc, const char** argv)
{
    NetGDBName = argv[0];
    TVector<TString> commandArgs;
    THolder<TClientCommand> command;

    for (int i = 0; i < argc; i++) {
        if (!command)
            command.Reset(CreateClientCommand(argv[i]));
        else
            commandArgs.push_back(argv[i]);
    }

    TProgramOptions options(OptionsList);
    options.Init(argc - commandArgs.size() - 1, argv);

    if (options.GetOption("?").first || options.GetOption("-help").first || !command) {
        PrintHelp();
        return 0;
    }
    InitNetworkSubSystem();
    RemotePortBase = atoi(options.GetOptVal("p", ::ToString(DEFAULT_PORT).data()));
    OutputPath = options.GetOptVal("o", "");

    HostsFile = options.GetOptVal("h", "hosts.txt");
    Verbose = !options.GetOption("q").first;
    ForceMode = options.GetOption("-force").first;
    ThreadsCount = 3;
    User = options.GetOptVal("-user", TString(GetEnv("USER")));

    if (dynamic_cast<TStartDaemonCommand*>(command.Get())) {
        RemoteStart = true;
        HostIndexModule = 101;
        signal(SIGHUP, SIG_IGN);
        NDaemonMaker::CloseFrom(3);
        const int port = atoi(options.GetReqOptVal("p"));
        if (!(Comm = NNetliba_v12::CreateHttpUdpRequester(port)))
            throw yexception() << "Unable to bind on port " << port;
        TString tempPath = options.GetOptVal("t", NFs::Exists("/phr") ? "/phr/tmp" : (NFs::Exists("/var/tmp") ? "/var/tmp" : "/tmp"));
        StartDaemon(tempPath + "/netgdb-sandboxes");
    }
    else { // client commands
        Comm = NNetliba_v12::CreateHttpUdpRequester(0);
        ClientTempPath = options.GetOptVal("t", "");
        ClientWorkDir = options.GetOptVal("w", "");
        if (options.GetOption("-ssh").first) {
            RemoteStart = true;
            RemoteTransport = SSH;
        }
        if (options.GetOption("-sky").first) {
            RemoteStart = true;
            RemoteTransport = SKY;
        }
        ThreadsCount = atoi(options.GetOptVal("-threads", "3"));

        TVector<TString> hosts;
        TVector<TString> failed;

        if (options.GetOption("c").first){
            TBufferOutputHandler handler;
            TString command;
            TString clusterName = options.GetOption("c").second;

            if (clusterName == "C1")
                command = "bsconfig slookup stag=RusTier0 stag=UkrTier0 stag=GeoTier0 stag=EngTier0 itag=testws-production-replica cfg=HEAD | cut -f 1 -d '#' | cut -d ' ' -f 2 | sort | uniq";
            else
                if (NFs::Exists("/Berkanavt/bin/scripts/yr"))
                    command = TString("/Berkanavt/bin/scripts/yr +") + clusterName + " LIST";
                else
                    command = TString("yr +") + clusterName + " LIST";

            RunSHCommand(command, "", nullptr, false, &handler);
            TString result(handler.Buffer.begin(), handler.Buffer.end());
            TStringInput hostsIF(result);
            TString host;
            hosts.clear();
            while (hostsIF.ReadLine(host)) {
                StripInPlace(host);
                if (!host.empty() && host.at(0) != '#')
                    hosts.push_back(host);
            }
            if (Verbose)
                Cout << hosts.size() << " hosts found in list" << Endl;
        }

        TString masterHost = options.GetOptVal("m", "");
        if (!masterHost.empty()) {
            if (Verbose)
                Cout << "Communicating with master host..." << Endl;
            TRemoteStarterChecker checker;
            hosts.push_back(masterHost);
            if (CheckResult(hosts, &failed, "start-all", &checker, 0)) {
                hosts.clear();
                hosts.insert(hosts.begin(), checker.GetHosts().begin(), checker.GetHosts().end());
            }
            else {
                if (!checker.GetHosts().empty()) {
                    PrintFailed(checker.GetHosts());
                    Comm->StopNoWait();
                    exit(-1);
                }
                else
                    throw yexception() << "Failed to communicate master host";
            }
            if (Verbose)
                Cout << "Complete. " << hosts.size() << " hosts found" << Endl;
        }
        if (RemoteStart) {
            RemotePortBase = atoi(options.GetOptVal("p", ::ToString(DEFAULT_PORT + getuid() % 1000 + (RemoteTransport != SSH ? 3030 : 0)).data()));
            HostIndexModule = 101;
        }

        if (hosts.empty())
            hosts = ReReadHostsList();

        if (options.HasMultOption("x")) {
            const TVector<const char*>& opts = options.GetMultOptions("x");
            for (int i = 0; i < opts.ysize(); i++) {
                TIFStream hostsIF(opts[i]);
                TString host;
                while (hostsIF.ReadLine(host)) {
                    StripInPlace(host);
                    if (!host.empty() && host.at(0) != '#') {
                        TVector<TString>::iterator found = std::find(hosts.begin(), hosts.end(), host);
                        if (found != hosts.end())
                            hosts.erase(found);
                    }
                }
            }
        }

        if (options.GetOption("f").first) {
            TRegExMatch regexp(options.GetOption("f").second);
            TVector<TString>::iterator iter = hosts.begin();
            while (iter != hosts.end()) {
                if (!regexp.Match(iter->data()))
                    iter = hosts.erase(iter);
                else
                    iter++;
            }
        }

        if (hosts.empty())
            throw yexception() << "No hosts found to run command";

        if (!command->Perform(hosts, commandArgs, &failed)) {
            PrintFailed(failed);
            Comm->StopNoWait();
            return 1;
        }
    }
    Comm->StopNoWait();
    return 0;
}
