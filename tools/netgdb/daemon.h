#pragma once
/*
 * commands.h
 *
 *  Created on: 12.11.2009
 *      Author: solar
 */

#include <sys/wait.h>
#include <sys/signal.h>

#include <util/string/vector.h>
#include <util/generic/hash.h>

#include "comm.h"
#include "requests.h"
#include "parallel.h"
#include "distribute.h"

class TReceiveFile: public TService
{
    TString File, Hash;
public:
    TReceiveFile (const TString& file, const TString& hash)
        : File(file), Hash(hash)
    {}

    void Process(TRequest& req) override
    {
        Status = ECS_IN_PROGRESS;

        if (GetHashForFileContents(File, req.Request->Data.begin(), req.Request->Data.end()) == Hash
           && StoreFile(GetPathForFile(File, req.User), Hash, req.Request->Data.begin(), req.Request->Data.end()))
            Status = ECS_COMPLETE;
        else
            Status = ECS_FAILED;
    }

    TString ToString() override
    {
        return "receive";
    }


    void Kill() override
    {}
};

class TRedistributeFile: public TService
{
    TString File, Hash;
    TVector<TString> Hosts;

public:
    TRedistributeFile (const TString& file, const TString& hash, const TVector<TString>& hosts)
        : File(file), Hash(hash), Hosts(hosts)
    {}

    void Process(TRequest& req) override
    {
        Status = ECS_IN_PROGRESS;

        TLazyContentsGetter contents(GetPathForFile(File, req.User) + "." + Hash);
        TVector<TString> fails;
        Distribute(File, Hash, contents, Hosts, &fails, req.ID);
        if (!fails.empty()) {
            Reply(req.Request->ReqId, req.ID + "\t1\t" + JoinStrings(fails, "\t"));
            req.Request.Reset(nullptr);
            Status = ECS_FAILED;
        }
        else Status = ECS_COMPLETE;
    }

    void Kill() override
    {}

    TString ToString() override
    {
        return "redistribute";
    }
};

class TRedistributeFileNew: public TService, public IResultChecker
{
    TString File, Hash;
    TVector<THostStatus> Hosts;
    TVector<THostStatus> ActiveHosts;
    TVector<TString> HostNames;
    TRequest* Req;
    TString ErrorMsg;
    THashMap<TString, TString> FailedHosts;

    bool CommunicateStopOnFail(const THostStatus& status, TPack command);
    void ParseErrorMessage(const TString& host, char* ptr) {
        TString msg = strsep(&ptr, "\t");
        if (!FailedHosts.contains(host) && !msg.empty())
            FailedHosts[host] = msg;

        while (ptr) {
            char* msg = strsep(&ptr, "\t");
            TString host = strsep(&msg, ":");
            if (!FailedHosts.contains(host))
                FailedHosts[host] = strsep(&msg, ":");
        }
    }
public:
    TRedistributeFileNew (const TString& file, const TString& hash, const TVector<TString>& hosts)
        : File(file), Hash(hash), Req(nullptr)
    {
        TVector<TString>::const_iterator iter = hosts.begin();
        while (iter != hosts.end()) {
            Hosts.push_back(*iter);
            iter++;
        }
    }

// client public interface
    bool InitNetwork(const TString& taskId);
    bool ProcessPart(const TString& taskId, TVector<char>::const_iterator begin, TVector<char>::const_iterator end);

    const TString& GetError() const
    {
        return ErrorMsg;
    }

    const THashMap<TString, TString>& GetFailedHosts() const
    {
        return FailedHosts;
    }

    void SetHosts(TVector<TString> hosts)
    {
        HostNames = hosts;
    }

public: // service interface
    void Process(TRequest& req) override;

    void Kill() override
    {
        TVector<char> empty;
        ProcessPart(Req->ID, empty.begin(), empty.end());
        if (Req)
            Req->Cancel();
    }

    TString ToString() override
    {
        return "redistribute-new";
    }
public: // IResultsChecker
    bool Check (const TString& host, TVector<char>& result, TRequestSet*) override {
        result.push_back(0);
        char* ptr = result.begin();
        TString status = strsep(&ptr, "\t");
        if (status != "0") {
//            Clog << "Failed to distribute to " << status << " hosts through " << host << ": " << ptr << Endl;
            ParseErrorMessage(host, ptr);
            return false;
        }
        return true;
    }
};

class TStartGDB: public TService
{
    TString File, Hash;
    TString Args, WD;

    volatile pid_t GdbPID;
    volatile pid_t PID;
public:
    TStartGDB (const TString& file, const TString& hash, const TString& args, const TString& wd)
        : File(file), Hash(hash), Args(args), WD(wd), GdbPID(0), PID(0)
    {}

    void Process(TRequest& req) override;
    void Kill() override;

    TString ToString() override
    {
        return "gdb " + File + " " + Args;
    }
};

const int MIN_TRANSITION_COUNT = 4096 * 1024;

class TStartShell: public TService, public TOutputHandler
{
    TString Cmd;
    TString Args, WD;

    pid_t PID;

    TVector<char> Buffer;
    TRequest* Request;
    bool SendOutput;
public:
    TStartShell (const TString& cmd, const TString& wd, bool sendOutput)
        : Cmd(cmd), WD(wd), PID(0), SendOutput(sendOutput)
    {}

    void Write(const char* buffer, int size) override
    {
        if (SendOutput && !!Request->Request) {
            Buffer.insert(Buffer.end(), buffer, buffer+size);
            if (Buffer.ysize() > MIN_TRANSITION_COUNT) {
                Comm->SendResponse(Request->Request->ReqId, &Buffer, RegularColor);
                Request->NextRequest(true);
                Buffer.resize(0);
            }
        }
    }
    void Process(TRequest& req) override
    {
        Request = &req;
        TString wd = GetPathForFile(WD, req.User).data();
        Cout << Cmd << "\t" << wd.data() << Endl;
        Status = ECS_IN_PROGRESS;
        if (RunSHCommand(Cmd, wd, &PID, true, SendOutput ? this : nullptr) == 0)
            Status = ECS_COMPLETE;
        else Status = ECS_FAILED;
        if (SendOutput && !!Request->Request) { // flush buffer
            Comm->SendResponse(Request->Request->ReqId, &Buffer, RegularColor);
            Request->NextRequest(true);
            Buffer.resize(0);
        }
    }

    void Kill() override
    {
        if (PID) {
            if (killpg(PID, SIGKILL)) {
                Cout << "Failed to kill process " << PID << Endl;
            }
            int status;
            waitpid(-PID, &status, 0);
        }
    }

    TString ToString() override
    {
        return "shell " + Cmd + " " + Args;
    }
};

class TLaunchTask: public TStartShell
{
public:
    TLaunchTask (const TString& cmd, const TString& wd)
        : TStartShell(cmd, wd, false)
    {}

    void Process(TRequest& req) override
    {
        Reply(req.Request->ReqId, "0");
        TStartShell::Process(req);
        req.Request.Reset(nullptr);
    }
};
