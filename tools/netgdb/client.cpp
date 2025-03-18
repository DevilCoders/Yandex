/*
 * client.cpp
 *
 *  Created on: 13.11.2009
 *      Author: solar
 */

#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <util/folder/dirut.h>
#include <util/string/strip.h>
#include <util/generic/hash_set.h>

#include "parallel.h"
#include "distribute.h"
#include "client.h"
#include "comm.h"
#include "daemon.h"


time_ms GetTimeInMillis()
{
    timeval tv;
    gettimeofday(&tv, nullptr);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

class TCheckCommand: public TClientCommand
{
    TString Name;
public:
    TCheckCommand(TString name)
        : Name(name)
    {}

    TString GetName() override
    {
        TString name = Name;
        name.to_title();
        return name;
    }

    bool PerformInner(const TVector<TString>& hosts, TVector<TString>&, TVector<TString>* failed) override
    {
        return CheckResult(hosts, failed, Name + "\t" + GenID(Name), nullptr, 3000);
    }
};

class TKillCommand: public TClientCommand
{
protected:
    TString GetName() override
    {
        return "Kill";
    }

    bool PerformInner(const TVector<TString>& hosts, TVector<TString>& args, TVector<TString>* failed) override
    {
        TString cmd = "kill\t" + GenID("kill") + "\t" + args[0];
        return CheckResult(hosts, failed, cmd, nullptr, 3000);
    }
};

class TKillAllCommand: public TClientCommand
{
protected:
    TString GetName() override
    {
        return "KillAll";
    }

    bool PerformInner(const TVector<TString>& hosts, TVector<TString>& /*args*/, TVector<TString>* failed) override
    {
        TString cmd = "killall\t" + GenID("killall");
        return CheckResult(hosts, failed, cmd, nullptr, 3000);
    }
};

class TStart
{
public:
    bool operator ()(TString& host)
    {
        TString command;
        {
            static TMutex lock;
            TGuard<TMutex> guard(lock);
            TString remoteNetGdb = "netgdb-" + ::ToString(getuid());
            TString localNetGdb = NFs::Exists(NetGDBName) ? NetGDBName : "`which netgdb`";
            TString sshParams = "-o BatchMode=yes -o StrictHostKeyChecking=no -o ConnectTimeout=10";
            static const TString AUTODETECT = "if [ -e /phr/tmp ]; then cd /phr/tmp; elif [ -e /var/tmp ]; then cd /var/tmp; else cd /tmp; fi;";
            command = "cat " + localNetGdb + " 2>/dev/null | ssh " + sshParams + " " + host + " \'"
                + (ClientTempPath.empty() ? AUTODETECT : "cd " + ClientTempPath + ";")
                + "rm -rf " + remoteNetGdb + ";"
                "cat >" + remoteNetGdb + "; chmod 0755 " + remoteNetGdb + ";"
                "./" + remoteNetGdb +
                " -p " + ::ToString(RemotePort(host)) +
                " -t `pwd`/netgdb-${USER}" +
                " daemon >>./" + remoteNetGdb + ".log 2>&1 &'";
        }

        return !RunSHCommand(command);
    }
};

bool TStartCommand::PerformInner(const TVector<TString>& hosts, TVector<TString>& args, TVector<TString>* failed)
{
    int tryIndex = 0;
    THolder<TClientCommand> ping(CreateClientCommand("ping"));
//    ping->Perform(hosts, args, failed); // now daemons should be started in loop by search-admin@ team.

    while (!ping->Perform(hosts, args, failed) && RemoteStart && tryIndex++ < 3) {
        TVector<TString> failedCopy(failed->begin(), failed->end());
        switch (RemoteTransport) {
            case SSH:
                ParallelTask<TString, TStart>(failedCopy, failed, ThreadsCount);
                break;
            case SKY:
                TString remoteNetGdb = "netgdb-" + User  + "-sky-bin";
                TString localNetGdb = NFs::Exists(NetGDBName) ? NetGDBName : "`which netgdb`";
                TString localNetGdbDirName = GetDirName(localNetGdb);
                TString command = "prev=`pwd`; if [ -e /phr/tmp ]; then cd /phr/tmp; elif [ -e /var/tmp ]; then cd /var/tmp; else cd /tmp; fi;"
                    "mv -f /tmp/netgdb " +remoteNetGdb+"; chmod 0755 " + remoteNetGdb + ";"
                    "./" + remoteNetGdb +
                    " -p " + ::ToString(RemotePort("")) +
                    " -t `pwd`/netgdb-" + User + "-sky" +
                    " --user " + User +
                    " daemon >>./" + remoteNetGdb + "-sky.log 2>&1";

                while (!failedCopy.empty()) {
                    TString hosts;

                    for (int i = 0; i < 100 && !failedCopy.empty(); i++) {
                        hosts += " +" + *failedCopy.rbegin();
                        failedCopy.erase(&*failedCopy.rbegin());
                    }

                    RunSHCommand("/skynet/tools/sky run \"rm -rf ./" + remoteNetGdb + "\" " + hosts + " >/dev/null 2>&1");
                    RunSHCommand("/skynet/tools/sky upload -d " + localNetGdbDirName + " netgdb /tmp/ " + hosts + " >/dev/null 2>&1");
                    RunSHCommand("/skynet/tools/sky run --no_wait '" + command + "' " + hosts + " >/dev/null 2>&1");
//                    Cout << "/skynet/tools/sky run --no_wait '" + command + "' " + hosts << Endl;
                }
                break;
        }
    }
    return failed->empty();
}

const size_t PART_SIZE = 10 * 1024 * 1024; // 10M
class TDistributeCommand: public TClientCommand
{
    TString FileName;
    TString Hash;
protected:
    TString GetName() override
    {
        return "Distribute";
    }

    bool PerformInner(const TVector<TString>& hostsArg, TVector<TString>& args, TVector<TString>* failed) override
    {
        TVector<TString> hosts = hostsArg;
        THolder<TClientCommand> start(CreateClientCommand("start"));
        int srcIdx = -1;
        for (int i = 0; i < args.ysize(); i++) {
            if (args[i].find('=') == TString::npos) {
                srcIdx = i;
                break;
            }
        }
        Y_VERIFY(srcIdx >= 0, "nothing to distribute");
        TString src = args[srcIdx];

        if (start->Perform(hosts, args, failed)) {
            FileName = src.find_last_of('/') != TString::npos ? src.substr(src.find_last_of('/') + 1) : src;
            TVector<char> contents;
            char buffer[4096];
            {
                TIFStream file(src);
                int read;
                while ((read = file.Read(buffer, sizeof(buffer))) > 0) {
                    contents.insert(contents.end(), buffer, buffer + read);
                }
            }
            Hash = GetHashForFileContents(FileName, contents.begin(), contents.end());
            args[srcIdx] = FileName + "." + Hash;
            TString id = GenID("distribute");

            CheckResult(hosts, failed, "check\t" + id + "\t" + FileName + "\t" + Hash, "0", 30000);
            if (failed->empty())
                return true;
            hosts = *failed;
            TRedistributeFileNew redist (FileName, Hash, *failed);
            failed->clear();

            if (redist.InitNetwork(id)) {
                TVector<char>::const_iterator ptr = contents.begin();
                TVector<char>::const_iterator next = (ptr + PART_SIZE <= contents.end()) ? ptr + PART_SIZE : contents.end();
                while (ptr != next) {
                    if (!redist.ProcessPart(id, ptr, next))
                        break;
                    if (Verbose) {
                        Cout << "\rDistributing: " << (ptr - contents.begin())/(1024. * 1024) << "Mb sent";
                        Cout.Flush();
                    }
                    ptr = next;
                    next = (ptr + PART_SIZE <= contents.end()) ? ptr + PART_SIZE : contents.end();
                }
                redist.SetHosts(hosts);
                redist.ProcessPart(id, ptr, ptr); // empty package must be sent anyway to end up the communication
                if (Verbose)
                    Cout << "\rDistributed " <<  (ptr - contents.begin())/(1024. * 1024) << "Mb                  " << Endl;
            }
            if (!redist.GetFailedHosts().empty()) {
                THashMap<TString, TVector<TString> > sortedByMessage;
                THashMap<TString,TString>::const_iterator fhiter = redist.GetFailedHosts().begin();
                while (fhiter != redist.GetFailedHosts().end()) {
                    sortedByMessage[fhiter->second].push_back(fhiter->first);
                    fhiter++;
                }
                THashMap<TString, TVector<TString> >::iterator miter = sortedByMessage.begin();
                while (miter != sortedByMessage.end()) {
                    Cout << "\t" << miter->first << Endl;
                    Cout << "\t";
                    for (int i = 0; i < miter->second.ysize(); i++) {
                        Cout << " " << miter->second[i];
                    }
                    Cout << Endl;
                    miter++;
                }
                failed->clear();
                THashMap<TString, TString>::const_iterator it = redist.GetFailedHosts().begin();
                while (it != redist.GetFailedHosts().end()) {
                    failed->push_back(it->first);
                    it++;
                }
            }
        }
        return failed->empty();
    }

public:
    TString GetFileName()
    {
        return FileName;
    }

    TString GetHash()
    {
        return Hash;
    }
};

class TDistributeOldCommand: public TClientCommand
{
    TString FileName;
    TString Hash;
protected:
    TString GetName() override
    {
        return "Distribute OLD";
    }

    bool PerformInner(const TVector<TString>& hosts, TVector<TString>& args, TVector<TString>* failed) override
    {
        THolder<TClientCommand> start(CreateClientCommand("start"));
        TString src = args[0];
        args.erase(args.begin());

        if (start->Perform(hosts, args, failed)) {
            FileName = src.find_last_of('/') != TString::npos ? src.substr(src.find_last_of('/') + 1) : src;
            Distribute(src, hosts, failed, &Hash, GenID("distribute"));
        }
        return failed->empty();
    }

public:
    TString GetFileName()
    {
        return FileName;
    }

    TString GetHash()
    {
        return Hash;
    }
};

class TDebugCommand: public TClientCommand
{
protected:
    TString GetName() override
    {
        return "Debug";
    }

    bool PerformInner(const TVector<TString>& hosts, TVector<TString>& args, TVector<TString>* failed) override
    {
        THolder<TDistributeCommand> distribute(new TDistributeCommand());
        if (!distribute->Perform(hosts, args, failed))
            return false;

        TRequestSet rs(Comm);

        failed->resize(0);

        const TString id = GenID("debug");
        const TString wd = ClientWorkDir;
        const TString progArgs = JoinStrings(args, " ");

        const TString command = "debug\t" + id + "\t" + distribute->GetFileName() + "\t" + distribute->GetHash() + "\t" + progArgs + "\t" + wd;

        for (int i = 0; i < hosts.ysize(); ++i) {
            const TString &h = hosts[i];
            TVector<char> pack(command.data(), command.data() + command.size());
            rs.SendRequest(h, "xxx", &pack);
        }

        bool broken = false;
        while (!rs.IsEmpty()) {
            TString host;
            TAutoPtr<NNetliba_v12::TUdpHttpResponse> resp = rs.GetResponse(&host);
            if (resp.Get()) {
                resp->Data.push_back(0);

                char* ptr = resp->Data.begin();
                TString status (strsep(&ptr, "\t"));
                if (status == "debug") {
                    NNetliba_v12::TUdpAddress address = NNetliba_v12::CreateAddress(host.data(), RemotePort(host));
                    if (broken) {
                        Communicate(address, "\t" + id + "\tquit");
                        continue;
                    }
                    Cout << "Starting debugger from: " << host << Endl;
                    Cout << ptr;
                    TString command;
                    while (!broken) {
                        if(Cin.ReadLine(command)) {
                            TString messages = Communicate(address, "\t" + id + "\t" + command);
                            TString status = messages.substr(0, messages.find('\t'));
                            messages = messages.substr(messages.find('\t') + 1);
                            if (status == "quit") {
                                Cout << "Quit status received from debugger" << Endl;
                                break;
                            }
                            Cout << messages;
                        } else {
                            broken = true;
                        }
                    }
                    if (broken) {
                        Communicate(address, "\t" + id + "\tquit");
                    }
                } else {
                    status = strsep(&ptr, "\t");
                    int rc = status.empty() ? -1 : atoi(status.data());
                    if (rc == 0) {
                        // ok
                    } else {
                        Y_VERIFY(Find(failed->begin(), failed->end(), host) == failed->end());
                        failed->push_back(host);
                    }
                }
            } else {
                rs.Wait();
            }
        }
        return failed->empty();
    }
};

class TShellCommand: public TClientCommand, public IResultChecker
{
    TString ID;
    THashSet<TString> Created;
    TString FileName;
    TString Hash;
public:
    TShellCommand()
    {}

    TShellCommand(const TString &fileName, const TString &hash)
        : FileName(fileName)
        , Hash(hash)
    {}

protected:
    TString GetName() override
    {
        return "Shell";
    }

    bool Check(const TString& host, TVector<char>& result, TRequestSet *rs) override
    {
        if (result.size() < ID.length() || strncmp(&result[0], ID.data(), ID.length())) {
//            Cout << result.size() << Endl;
            if (!OutputPath.empty()) {
                TAutoPtr<IOutputStream> stream;
                if (OutputPath != "-") {
                    TString fname = OutputPath + "/" + host;
                    if (!Created.contains(host)) {
                        MakePathIfNotExist(OutputPath.data());
                        stream = new TOFStream(TFile(fname, CreateAlways));
                        Created.insert(host);
                    } else {
                        stream = new TOFStream(TFile(fname, OpenExisting | ForAppend));
                    }
                }
                if (!result.empty()) {
                    IOutputStream &out = stream.Get() ? *stream : Cout;
                    out.Write(&result[0], result.size());
                }
            }
            TPack pack = ConvertToPackage("\t" + ID);
            rs->SendRequest(host, "xxx", pack.Get());
            return true;
        } else {
            result.push_back(0);
            char* ptr = &result[0];
            strsep(&ptr, "\t"); // id
            return TString("0") == strsep(&ptr, "\t");
        }
    }

    bool PerformInner(const TVector<TString>& hosts, TVector<TString>& args, TVector<TString>* failed) override
    {
        THolder<TClientCommand> start(CreateClientCommand("start"));
        ID = GenID("shell");

        if (start->Perform(hosts, args, failed)) {
            const TString cmd = "shell\t" + ID + "\t" + FileName + "\t" + Hash + "\t" + ClientWorkDir + "\t" + (OutputPath.empty() ? "0" : "1") + "\t" + JoinStrings(args, "\t");
            //FIMXE: escape tabs inside args.
            CheckResult(hosts, failed, cmd, this, 0);
        }
        return failed->empty();
    }
};

class TRunCommand: public TClientCommand
{
    TString ID;
    THashMap<TString, TOFStream*> Streams;
    TVector<TAutoPtr<TOFStream> > StreamsHolder;

protected:
    TString GetName() override
    {
        return "Run";
    }

    bool PerformInner(const TVector<TString>& hosts, TVector<TString>& args, TVector<TString>* failed) override
    {
        THolder<TClientCommand> start(CreateClientCommand("start"));
        ID = GenID("run");
        THolder<TDistributeCommand> distribute(new TDistributeCommand());

        if (distribute->Perform(hosts, args, failed)) {
            THolder<TShellCommand> shell(new TShellCommand(distribute->GetFileName(), distribute->GetHash()));
            shell->Perform(hosts, args, failed);
        }

        return failed->empty();
    }
};

class TLaunchCommand: public TClientCommand
{
protected:
    TString GetName() override
    {
        return "Launch";
    }

    bool PerformInner(const TVector<TString>& hosts, TVector<TString>& args, TVector<TString>* failed) override
    {
        THolder<TDistributeCommand> distribute(new TDistributeCommand());

        if (distribute->Perform(hosts, args, failed)) {
            const TString wd = ClientWorkDir;
            const TString progArgs = JoinStrings(args, "\t"); //FIXME: escape tabs in args
            const TString id = GenID("launch");
            const TString cmd = "launch\t" + id + "\t" + distribute->GetFileName() + "\t" + distribute->GetHash() + "\t" + wd + "\t" + progArgs;
            bool result = CheckResult(hosts, failed, cmd);
            if (result)
                Cout << id << Endl;
            return result;
        }
        return failed->empty();
    }
};

class TPrintHostsCommand: public TClientCommand {
    TString GetName() override
    {
        return "Print hosts";
    }

    bool PerformInner(const TVector<TString>& hosts, TVector<TString>& /*args*/, TVector<TString>* /*failed*/) override
    {
        TVector<TString>::const_iterator iter = hosts.begin();
        while (iter != hosts.end()) {
            Cout << *iter++ << Endl;
        }
        return true;
    }
};

class TStatusCommandBase: public TClientCommand
{
protected:
    virtual bool IsVerbose() const = 0;

    bool PerformInner(const TVector<TString>& hosts, TVector<TString>& args, TVector<TString>* failed) override
    {
        failed->resize(0);

        TRequestSet rs(Comm);
        {
            TString command = "status";
            for (int i = 0; i < hosts.ysize(); ++i) {
                TString h = hosts[i];
                TVector<char> pack(command.data(), command.data() + command.size());
                rs.SendRequest(h, "xxx", &pack);
            }
        }

        THashMap< TString, THashMap<TString, TVector<TString> > > tasks;
        THashMap< TString, TString> tasksStr;
        while (!rs.IsEmpty()) {
            TString host;
            TAutoPtr<NNetliba_v12::TUdpHttpResponse> resp = rs.GetResponse(&host);
            if (resp.Get()) {
                if (resp->Ok) {
                    if (!resp->Data.empty()) {
                        resp->Data.push_back(0);
                        char* next = resp->Data.begin();
                        while (next && *next) {
                            char* ptr = strsep(&next, "\n");
                            TString taskId = strsep(&ptr, "\t");
                            TString taskToString = strsep(&ptr, "\t");
                            TString status = ptr;
                            tasks[taskId][status].push_back(host);
                            tasksStr[taskId] = taskToString;
                        }
                    }
                } else {
                    failed->push_back(host);
                }
            } else {
                rs.Wait();
            }
        }
        if (args.empty()) {
            THashMap<TString, TString>::iterator iter = tasksStr.begin();
            while (iter != tasksStr.end()) {
                THashMap<TString, TVector<TString> >::const_iterator statuses = tasks[iter->first].begin();
                Cout << iter->first << "\t" << iter->second;
                while (statuses != tasks[iter->first].end()) {
                    Cout << "\t" << statuses->first << " (" << statuses->second.size() << ")";
                    if (IsVerbose()) {
                        TString res;
                        for (TVector<TString>::const_iterator toHost = statuses->second.begin(); toHost != statuses->second.end(); ++toHost) {
                            if (res.size())
                                res += ", ";
                            res += *toHost;
                        }
                        Cout << " [" << res << "]";
                    }
                    ++statuses;
                }
                Cout << Endl;
                ++iter;
            }
        } else {
            TVector<TString>::const_iterator iter = args.begin();
            while (iter != args.end()) {
                THashMap<TString, TVector<TString> >::iterator statuses = tasks[*iter].begin();
                Cout << *iter;
                while (statuses != tasks[*iter].end()) {
                    Cout << "\t" << statuses->first << Endl;
                    TVector<TString>::const_iterator hostsIter = statuses->second.begin();
                    while (hostsIter != statuses->second.end()) {
                        Cout << "\t\t" << *hostsIter << Endl;
                        hostsIter++;
                    }
                    statuses++;
                }
                Cout << Endl;
                iter++;
            }
        }
        return failed->empty();
    }
};

class TStatusCommand: public TStatusCommandBase {
protected:
    bool IsVerbose() const override {
        return false;
    }

    TString GetName() override
    {
        return "Status";
    }
};

class TVerboseStatusCommand: public TStatusCommandBase {
protected:
    bool IsVerbose() const override {
        return true;
    }

    TString GetName() override {
        return "VerboseStatus";
    }
};

TClientCommand* CreateClientCommand(TString name)
{
    if (name == "ping")
        return new TCheckCommand("ping");
    else if (name == "cleanup")
        return new TCheckCommand("cleanup");
    else if (name == "stop")
        return new TCheckCommand("stop");
    else if (name == "start")
        return new TStartCommand();
    else if (name == "distribute")
        return new TDistributeCommand();
    else if (name == "distribute-old")
        return new TDistributeOldCommand();
    else if (name == "run")
        return new TRunCommand();
    else if (name == "debug")
        return new TDebugCommand();
    else if (name == "shell")
        return new TShellCommand();
    else if (name == "kill")
        return new TKillCommand();
    else if (name == "killall")
        return new TKillAllCommand();
    else if (name == "launch")
        return new TLaunchCommand();
    else if (name == "status")
        return new TStatusCommand();
    else if (name == "verbose-status")
        return new TVerboseStatusCommand();
    else if (name == "print-hosts")
        return new TPrintHostsCommand();
    else if (name == "daemon")
        return new TStartDaemonCommand();
    return nullptr;
}

bool CheckResult(const TVector<TString>& hosts, TVector<TString>* failed, const TString& command, IResultChecker* checker, time_ms to)
{
    (void)to;
    failed->resize(0);
    if (hosts.empty())
        return true;

    TRequestSet rs(Comm);

    for (int i = 0; i < hosts.ysize(); ++i) {
        TString h = hosts[i];
        TVector<char> pack(command.data(), command.data() + command.size());
        rs.SendRequest(h, "xxx", &pack);
    }

    while (!rs.IsEmpty()) {
        TString host;
        TAutoPtr<NNetliba_v12::TUdpHttpResponse> resp = rs.GetResponse(&host);
        if (resp.Get()) {
//            Clog << "Answer " << GetGuidAsString(resp->ReqId) << " from " << connections[resp->ReqId] << "\t" << TString(resp->Data.begin(), resp->Data.end()) << Endl;
            if (resp->Ok) {
                if (checker) {
                    if (!checker->Check(host, resp->Data, &rs)) {
                        failed->push_back(host);
                    }
                }
            } else {
                failed->push_back(host);
            }
        } else {
            rs.Wait();
        }
    }
    return failed->empty();
}

void PrintFailed(const TVector<TString>& failed)
{
    Cout << "Failed on hosts:" << Endl;
    TVector<TString>::const_iterator iter = failed.begin();
    while (iter != failed.end()) {
        Cout << "\t" << *iter << Endl;
        ++iter;
    }
}

static TMutex mutex;
static TVector<TString> HostsList;

TVector<TString> ReReadHostsList()
{
    TGuard<TMutex> lock(mutex);
    static time_t ts = 0;
    struct stat fStats;
    if (!NFs::Exists(HostsFile))
        return TVector<TString>();
    stat(HostsFile.data(), &fStats);
    if (!HostsList.empty() && fStats.st_mtime == ts)
        return HostsList;
    ts = fStats.st_mtime;
    TIFStream hostsIF(HostsFile);
    TString host;
    HostsList.clear();
    while (hostsIF.ReadLine(host)) {
        StripInPlace(host);
        if (!host.empty() && host.at(0) != '#')
            HostsList.push_back(host);
    }
    if (Verbose)
        Cout << HostsList.size() << " hosts found in list" << Endl;
    return HostsList;
}
