#pragma once
/*
 * commands.h
 *
 *  Created on: 12.11.2009
 *      Author: solar
 */

#include <util/system/thread.h>

#include "comm.h"

class TService;

class TRequest
{
    TVector<NNetliba_v12::TUdpHttpRequest*> Incoming;
    TMutex IncomingLock;


    TSystemEvent NextMessageEvt;
    bool Active;
public:
    THolder<TThread> Th;

    TString ID;
    TString User;
    THolder<TService> Command;
    THolder<NNetliba_v12::TUdpHttpRequest> Request;

    TRequest(const TString& user, const TString& id, TService* command, NNetliba_v12::TUdpHttpRequest* req)
        : Active(true), ID(id), User(user), Command(command), Request(req)
    {
        NextMessageEvt.Reset();
    }

    void AddRequest(NNetliba_v12::TUdpHttpRequest* req);
    bool NextRequest(bool wait = true);
    void Cancel();

    virtual ~TRequest()
    {
        while (NextRequest(false)) {
            Reply(Request->ReqId, "");
        }
    }
};

class TService {
public:
    enum ECommandStatus {
        ECS_START,
        ECS_IN_PROGRESS,
        ECS_SUSPEND,
        ECS_FAILED,
        ECS_COMPLETE
    };

    virtual void Process(TRequest& req) = 0;
    virtual void Kill() = 0;

protected:
    volatile ECommandStatus Status;

    TService ()
        : Status(ECS_START)
    {}

public:

    virtual ~TService ()
    {}

    ECommandStatus GetStatus ()
    {
        return Status;
    }

    virtual TString ToString()
    {
        return "Unknown";
    }
};

IOutputStream& operator <<(IOutputStream& out, TService::ECommandStatus status);

void* StartService(const TString& id, TService* cmd, NNetliba_v12::TUdpHttpRequest* req);
bool AddRequestToTask (const TString& reqId, NNetliba_v12::TUdpHttpRequest *req);
bool KillTask(const TString& id);
void KillAllTasks(const TString& user);
TString GetStatus();

