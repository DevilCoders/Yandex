#include <util/string/vector.h>
#include <util/string/cast.h>
#include <util/folder/dirut.h>
#include <util/folder/iterator.h>
#include <util/system/fs.h>
#include <util/string/split.h>
#include <cstdlib>
#include <stdio.h>

#include "daemon.h"
#include "client.h"


static void RemoveDirWithContentsFixed(TString dirName) {
    SlashFolderLocal(dirName);

    TDirIterator dir(dirName);

    for (auto it = dir.begin(); it != dir.end(); ++it) {
        switch (it->fts_info) {
            case FTS_F:
            case FTS_SL:
            case FTS_SLNONE:
            case FTS_DP:
                NFs::Remove(it->fts_path);
                break;
        }
    }
}

void StartDaemon(const TString& storage) {
    SetStoragePath(storage);
    while (true) {
        NNetliba_v12::TUdpHttpRequest *req = Comm->GetRequest();
        if (req) {
            try {
                /*
                Clog << GetGuidAsString(req->ReqId) << " caught " << req->Data.size() << " bytes" << Endl;
                Clog << "Start" << Endl << TString(req->Data.begin(), req->Data.end()) << Endl << "End" << Endl;
                */
                req->Data.push_back(0);
                char* ptr = &req->Data[0];
                TString serviceName = strsep(&ptr, "\t");
                TString id = strsep(&ptr, "\t");
                TString user = id.substr(0, id.find_first_of('-'));
                Clog << id << "\t" << serviceName << "\t" << Endl;
                req->Data.erase(req->Data.begin(), ptr ? ptr : req->Data.end());
                ptr = req->Data.begin();
                TAutoPtr<TVector<char> > pack;
                if (serviceName == "ping") {
                    Reply(req->ReqId, "0");
                    delete req;
                }
                else if (serviceName == "status") {
                    Reply(req->ReqId, GetStatus());
                    delete req;
                }
                else if (serviceName == "cleanup") {
                    RemoveDirWithContentsFixed(GetPathForFile("", user));
                    Reply(req->ReqId, "0");
                    delete req;
                }
                else if (serviceName == "stop") {
                    Reply(req->ReqId, "0");
                    delete req;
                    break;
                }
                else if (serviceName == "killall") {
                    KillAllTasks(user);
                    Reply(req->ReqId, "0");
                    delete req;
                }
                else if (serviceName == "kill") {
                    bool rc = KillTask(strsep(&ptr, "\t"));
                    Reply(req->ReqId, rc ? "0" : "1");
                    delete req;
                }
                else if (serviceName == "check") {
                    TString file(strsep(&ptr, "\t"));
                    TString hash(strsep(&ptr, "\t"));

                    TString physPath = GetPathForFile(file, user) + "." + ::ToString(hash);
                    bool success = NFs::Exists(physPath);
                    if (success) {
                        size_t pos = physPath.find_last_of('/');
                        unlink(GetPathForFile(file, user).data());
                        NFs::SymLink((pos != TString::npos ? physPath.substr(pos + 1) : physPath), GetPathForFile(file, user));
                    }
                    Reply(req->ReqId, success ? "0" : "1");
                    Cout << "\t" << physPath << "\t" <<  (success ? "exists" : "does not exist") << Endl;
                    delete req;
                }
                else if (serviceName == "receive") {
                    const TString file = strsep(&ptr, "\t");
                    const TString hash = strsep(&ptr, "\t");
                    if (!req->Data.empty())
                        req->Data.erase(req->Data.end() - 1);
                    req->Data.erase(req->Data.begin(), ptr ? ptr : req->Data.end());
                    StartService(id, new TReceiveFile(file, hash), req);
                }
                else if (serviceName == "redistribute") {
                    TString file(strsep(&ptr, "\t"));
                    TString hash(strsep(&ptr, "\t"));
                    TVector<TString> hosts;
                    while (ptr)
                        hosts.push_back(strsep(&ptr, "\t"));
                    StartService(id, new TRedistributeFile(file, hash, hosts), req);
                }
                else if (serviceName == "redistribute-new") {
                    TString file(strsep(&ptr, "\t"));
                    TString hash(strsep(&ptr, "\t"));
                    TVector<TString> hosts;
                    while (ptr)
                        hosts.push_back(strsep(&ptr, "\t"));
                    StartService(id, new TRedistributeFileNew(file, hash, hosts), req);
                }
                else if (serviceName == "shell") {
                    TStringBuf buf = req->Data.begin();
                    TString file = TString{buf.NextTok('\t')};
                    TString hash = TString{buf.NextTok('\t')};
                    TString wd = TString{buf.NextTok('\t')};
                    bool sendOutput = TString{buf.NextTok('\t')} == "1";
                    TString args;

                    if (!file.empty()) {
                        TString fullFile = file + "." + hash;
                        TString realFile = GetPathForFile(file, user);
                        TVector<TString> split;
                        StringSplitter(buf).Split('\t').AddTo(&split);
                        for (int i = 0; i < split.ysize(); i++) {
                            if (split[i] == fullFile) {
                                split[i] = realFile;
                            }
                        }
                        args = JoinStrings(split, "\t");
                    } else {
                        args = buf;
                    }

                    StartService(id, new TStartShell(args, wd, sendOutput), req);
                }
                else if (serviceName == "debug") {
                    StartService(id, new TStartGDB(strsep(&ptr, "\t"), strsep(&ptr, "\t"), strsep(&ptr, "\t"), strsep(&ptr, "\t")), req);
                }
                else if (serviceName == "launch") {
                    TStringBuf buf = req->Data.begin();
                    TString file = TString{buf.NextTok('\t')};
                    TString hash = TString{buf.NextTok('\t')};
                    TString wd = TString{buf.NextTok('\t')};

                    TString fullFile = file + "." + hash;
                    TString realFile = GetPathForFile(fullFile, user);
                    TVector<TString> split;
                    StringSplitter(buf).Split('\t').AddTo(&split);
                    for (int i = 0; i < split.ysize(); i++) {
                        if (split[i] == fullFile) {
                            split[i] = realFile;
                        }
                    }
                    TString args = JoinStrings(split, "\t");

                    StartService(id, new TLaunchTask(args, wd), req);
                }
                else if (serviceName == "start-all") {
                    TVector<TString> hosts(ReReadHostsList());
                    TStartCommand start;
                    TVector<TString> args;
                    TVector<TString> failed;
                    start.Perform(hosts, args, &failed);
                    if (failed.empty())
                        Reply(req->ReqId, "0\t" + JoinStrings(hosts, "\t"));
                    else
                        Reply(req->ReqId, "1\t" + JoinStrings(failed, "\t"));
                    delete req;
                }
                else if (!AddRequestToTask(id, req)) {
                    TString msg;
                    if (!serviceName.empty())
                        msg = "Unknown request for service " + serviceName;
                    else if (!id.empty())
                        msg = "Unknown request for id " + id;
                    Clog << msg << Endl;
                    Reply(req->ReqId, "1\t" + msg);
                    delete req;
                }
            }
            catch(...) {
                Clog << "Error processing request!" << Endl;
                Reply(req->ReqId, "1");
                delete req;
            }
        }
        else {
            Clog << "Waiting for requests..." << Endl;
            Comm->GetAsyncEvent().Wait();
        }
    }

    KillAllTasks("");
    CleanUp();

    sleep(1);
}


bool ReadUpToNextCommandLine (int fd, TVector<TString>* output)
{
    char buffer[4096];
    int rd;

    output->push_back("");
    while(rd = read(fd, buffer, sizeof(buffer))) {
        buffer[rd] = 0;
        char* current = buffer;
        char* start = buffer;
        while (current < rd + buffer) {
            if (*current == '\n') {
                *current = 0;
                output->rbegin()->append(start);
                start = current + 1;
                output->push_back("");
            }
            current++;
        }
        output->rbegin()->append(start);
        if (output->rbegin()->find("(gdb)") != TString::npos)
            break;
    }
    return rd;
}

/*static bool RunGdbCommand(TRequest& req, int gdbInFD, int gdbOutFD, const TString &command, TVector<TString> *output)
{
    if (command.size() > 0) { // empty command => just read the prompt
        write(gdbInFD, command.c_str(), command.size());
        if (command.back() != '\n') {
            write(gdbInFD, "\n", 1);
        }
    }

    if (!ReadUpToNextCommandLine(gdbOutFD, &output)) {
        Reply(req.Request->ReqId, "quit\t" + ::ToString(-1));
        req.NextRequest(false);
        kill(9, GdbPID);
        return false;
    }

    return true;
}*/

void TStartGDB::Process(TRequest& req)
{
    TString wd = GetPathForFile(WD, req.User);
    Status = ECS_IN_PROGRESS;
    int pipeOut[2];
    int pipeIn[2];

    pipe(pipeOut);
    pipe(pipeIn);
    //FILE* gdbPathP = popen("sh -c \"which gdb77 > /dev/null\"", "r");
    //char gdbPath[1024];
    //fgets(gdbPath, sizeof(gdbPath), gdbPathP);
    //pclose(gdbPathP);
    const TString gdb = "/usr/local/bin/gdb77"; //gdbPath; //FIXME: distribute local `which gdb77`!
    TString fileToDebug = File + "." + Hash;
    const char* gdbParams[] = {(gdb.substr(gdb.rfind('/') + 1)).data(), const_cast<char*>(fileToDebug.data()), nullptr};
    Clog << "\t" << gdb << "\t"  << gdbParams[0] << Endl;

    GdbPID = fork();
    if (GdbPID > 0) {
        close(pipeOut[1]);
        close(pipeIn[0]);
        TVector<TString> output;
        int gdbInFD = pipeIn[1];
        int gdbOutFD = pipeOut[0];

        if (!ReadUpToNextCommandLine(gdbOutFD, &output)) {
            Reply(req.Request->ReqId, "quit\t" + ::ToString(-1));
            kill(9, GdbPID);
            return;
        }

        TString command;
        TVector<TString> outputHidden;

        command = "b _start\n";
        write(pipeIn[1], command.c_str(), command.size());
        if (!ReadUpToNextCommandLine(gdbOutFD, &outputHidden)) {
            Reply(req.Request->ReqId, "quit\t" + ::ToString(-1));
            req.NextRequest(false);
            kill(9, GdbPID);
            return;
        }

        command = "r >/dev/null 2>&1 " + Args + "\n";
        Clog << "Debugging: " << command << "\t" << "wd: " << wd << Endl;
        write(pipeIn[1], command.c_str(), command.size());
        if (!ReadUpToNextCommandLine(gdbOutFD, &output)) {
            Reply(req.Request->ReqId, "quit\t" + ::ToString(-1));
            req.NextRequest(false);
            kill(9, GdbPID);
            return;
        }

        command = "p getpid()\n";
        write(pipeIn[1], command.c_str(), command.size());
        TVector<TString> outputPid;
        if (!ReadUpToNextCommandLine(gdbOutFD, &outputPid)) {
            Reply(req.Request->ReqId, "quit\t" + ::ToString(-1));
            req.NextRequest(false);
            kill(9, GdbPID);
            return;
        }
        Y_VERIFY(outputPid.size() == 2);
        PID = FromString<pid_t>(outputPid[0].substr(5)); //'$1 = '
        Clog << "Debugged process PID: " << PID << Endl;

        command = "d 1\n";
        write(pipeIn[1], command.c_str(), command.size());
        if (!ReadUpToNextCommandLine(gdbOutFD, &outputHidden)) {
            Reply(req.Request->ReqId, "quit\t" + ::ToString(-1));
            req.NextRequest(false);
            kill(9, GdbPID);
            return;
        }

        command = "c\n";
        write(pipeIn[1], command.c_str(), command.size());
        if (!ReadUpToNextCommandLine(gdbOutFD, &output)) {
            Reply(req.Request->ReqId, "quit\t" + ::ToString(-1));
            req.NextRequest(false);
            kill(9, GdbPID);
            return;
        }

        bool exitedNormally = false;
        TVector<TString>::const_iterator iter = output.begin();
        while (iter != output.end()) {
            if (iter->find("exited normally") != TString::npos) {
                exitedNormally = true;
                break;
            }
            //FIXME: check 'exited with code NNN'?
            iter++;
        }

        if (exitedNormally) {
            Clog << JoinStrings(output, "\n") << Endl;
        } else {
            Clog << "start remote debugging" << Endl;
            Reply(req.Request->ReqId, "debug\t" + ::ToString(req.ID) + "\t" + JoinStrings(output, "\n"));
        }
        output.resize(0);

        while (true) {
            TString command;
            if (!exitedNormally) {
                Status = ECS_SUSPEND;
                req.NextRequest(true);
                Status = ECS_IN_PROGRESS;
                if (!req.Request)
                    break;
                req.Request->Data.push_back(0);
                char* ptr = req.Request->Data.begin();
                command = strsep(&ptr, "\t");
                Clog << Endl << "(gdb) " << command << Endl;
            } else {
                command = "q";
            }
            if (!command.empty()) {
                write(gdbInFD, command.begin(), command.size());
                write(gdbInFD, "\n", 1);
                if (!ReadUpToNextCommandLine(gdbOutFD, &output)) {
                    Clog << JoinStrings(output, "\n") << Endl;
                    break;
                }
            }
            Y_VERIFY(!exitedNormally); // should break upon quit command
            Reply(req.Request->ReqId, "debug\t" + JoinStrings(output, "\n"));
            output.resize(0);
        }
        close(gdbInFD);
        close(gdbOutFD);

        waitpid(GdbPID, nullptr, 0);

        if (!!req.Request) {
            Reply(req.Request->ReqId, "quit\t0");
            //req.NextRequest(true);
        }
        Status = exitedNormally ? ECS_COMPLETE : ECS_FAILED; // why? programm crash is a successful debug, is it not?
    }
    else {
        setpgid(0, setsid());    // separate group in case we'll kill it
        close(pipeOut[0]);
        dup2(pipeOut[1], 1);
        dup2(pipeOut[1], 2);
        close(pipeIn[1]);
        dup2(pipeIn[0], 0);
        chdir(wd.data());
        Clog << Endl << "\tDebugging: " << fileToDebug << " gdb: " << gdb << Endl;
        execvp(gdb.data(), const_cast<char**>(gdbParams));
        exit(-1);
    }
}

void TStartGDB::Kill()
{
    if (GdbPID) {
        if (kill(GdbPID, SIGKILL)) {
            Cout << "Failed to kill process " << GdbPID << Endl;
        }
        waitpid(GdbPID, nullptr, 0);
    }

    if (PID) {
        if (killpg(PID, SIGKILL)) {
            Cout << "Failed to kill process " << PID << Endl;
        }
        waitpid(PID, nullptr, 0);
    }
}

bool TRedistributeFileNew::CommunicateStopOnFail(const THostStatus& host, TPack command) {
    TString status = Communicate(host.Address, command);
    bool result = true;
    if (status.empty()) {
        FailedHosts[host.Host] = (TString)"Connection failed";
        result = false;
    }
    else if (status != "0") {
        char *ptr = const_cast<char*>(status.c_str());
        strsep(&ptr, "\t");
        ParseErrorMessage(host.Host, ptr);
        ErrorMsg = status.substr(status.find('\t') + 1);
        result = false;
    }
    if (!result) {
        for (int i = 0; i < ActiveHosts.ysize(); i++)
            Communicate(ActiveHosts[i].Address, "\t" + Req->ID + "\t");
    }
    return result;
}

bool TRedistributeFileNew::InitNetwork(const TString& taskId) {
    TVector<TVector<THostStatus*> > clusters;
    Cluster(std::min(2, Hosts.ysize()), Hosts, &clusters);
    TVector<TVector<THostStatus*> >::iterator iter = clusters.begin();

    while (iter != clusters.end()) {
        THostStatus dest = *(*iter)[0];
        iter->erase(iter->begin());
        TString commandStr = "redistribute-new\t" + taskId + "\t" + File + "\t" + Hash;
        for (int i = 0; i < iter->ysize(); i++)
            commandStr += "\t" + (*iter)[i]->Host;

        if (!CommunicateStopOnFail(dest, ConvertToPackage(commandStr)))
            return false;
        ActiveHosts.push_back(dest);
        HostNames.push_back(dest.Host);
        iter++;
    }
    return true;
}

bool TRedistributeFileNew::ProcessPart(const TString& taskId, TVector<char>::const_iterator begin, TVector<char>::const_iterator end) {
    TVector<char> command; // packet is deleted after send
    command.push_back('\t');
    command.insert(command.end(), taskId.begin(), taskId.end());
    command.push_back('\t');
    command.insert(command.end(), begin, end);
    TVector<TString> failed;
    TVector<TString> hosts = HostNames;
    TVector<TString>::iterator hiter = hosts.begin();
    while (hiter != hosts.end()) {
        if (FailedHosts.contains(*hiter))
            hiter = hosts.erase(hiter);
        hiter++;
    }
    CheckResult(hosts, &failed, TString(command.begin(), command.end()), this, 0);
    TVector<TString>::const_iterator fiter = failed.begin();
    while (fiter != failed.end()) {
        if (!FailedHosts.contains(*fiter))
            FailedHosts[*fiter] = (TString)"Connection failed";
        fiter++;
    }
    return FailedHosts.empty();
}

void TRedistributeFileNew::Process(TRequest& req)
{
    Status = ECS_IN_PROGRESS;
    Req = &req;
    FailedHosts.clear();
    if (InitNetwork(req.ID)) {
        Reply(req.Request->ReqId, "0");
        req.NextRequest(true);
        TVector<char> accumulatedContent;

        while (!!req.Request && !req.Request->Data.empty()) {
            accumulatedContent.insert(accumulatedContent.end(), req.Request->Data.begin(), req.Request->Data.end());
            if (!ProcessPart(req.ID, req.Request->Data.begin(), req.Request->Data.end())) // problem on target hosts
                break;
            Clog << req.Request->Data.end() - req.Request->Data.begin() << " bytes processed" << Endl;
            Reply(req.Request->ReqId, "0");
            req.NextRequest(true);
        }
//        ProcessPart(req.ID, req.Request->Data.begin(), req.Request->Data.begin()); // send empty package to all recipients anyway
        // check file consistency and store
        if (GetHashForFileContents(File, accumulatedContent.begin(), accumulatedContent.end()) == Hash) { // content is valid
            if (!StoreFile(GetPathForFile(File, req.User), Hash, accumulatedContent.begin(), accumulatedContent.end())) { // store failed
                ErrorMsg = "Can't store file";
            }
        }
        else ErrorMsg = "Invalid content received";
    }
    else ErrorMsg = "Failed to init";

    if ((!ErrorMsg.empty() || !FailedHosts.empty()) && !!req.Request) {
        TString message(::ToString<int>((ErrorMsg.empty() ? 0 : 1) + FailedHosts.size()) + "\t" + ErrorMsg);
        THashMap<TString, TString>::const_iterator fhiter = FailedHosts.begin();
        while (fhiter != FailedHosts.end()) {
            message += "\t" + fhiter->first + ":" + fhiter->second;
            fhiter++;
        }
        Reply(req.Request->ReqId, message);
        req.NextRequest(false);
        Status = ECS_FAILED;
    }
    else {
        Reply(req.Request->ReqId, "0");
        req.NextRequest(false);
        Status = ECS_COMPLETE;
    }
    Req = nullptr;
}
