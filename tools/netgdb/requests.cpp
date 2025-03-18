/*
 * requests.cpp
 *
 *  Created on: 12.11.2009
 *      Author: solar
 */

#include "requests.h"


static THashMap<TString, TRequest*> Tasks;
static TMutex TasksLock;

IOutputStream& operator <<(IOutputStream& out, TService::ECommandStatus status)
{
    switch (status) {
        case TService::ECS_COMPLETE:
            out << "complete";
            break;
        case TService::ECS_FAILED:
            out << "failed";
            break;
        case TService::ECS_IN_PROGRESS:
            out << "in progress";
            break;
        case TService::ECS_START:
            out << "start";
            break;
        case TService::ECS_SUSPEND:
            out << "suspend";
            break;
    }
    return out;
}

void* StartTask (void* req)
{
    THolder<TRequest> cr((TRequest*)req);
    cr->Th->Detach();
    {
        TGuard<TMutex> lock(TasksLock);
        Tasks[cr->ID] = (TRequest*)req;
    }

    cr->Command->Process(*cr);

    {
        TGuard<TMutex> lock(TasksLock);
        Tasks.erase(cr->ID);
    }
    if (cr->Request.Get())
        Reply(cr->Request->ReqId, cr->ID + "\t" + (cr->Command->GetStatus() == TService::ECS_COMPLETE ? "0" : "1"));
    return nullptr;
}

bool AddRequestToTask (const TString& reqId, NNetliba_v12::TUdpHttpRequest *req)
{
    TGuard<TMutex> lock(TasksLock);
    THashMap<TString,TRequest*>::iterator found = Tasks.find(reqId);
    if (found == Tasks.end())
        return false;
    Cout << "Adding request to " << reqId << Endl;
    if (!req->Data.empty()) // remove tailing 0
        req->Data.erase(req->Data.end() - 1);

    found->second->AddRequest(req);
    return true;
}

TString GetStatus()
{
    TGuard<TMutex> lock(TasksLock);
    THashMap<TString, TRequest*>::const_iterator iter = Tasks.begin();
    TString buffer;
    while (iter != Tasks.end()) {
        buffer += iter->first;
        buffer += "\t";
        buffer += iter->second->Command->ToString();
        buffer += "\t";
        buffer += ::ToString(iter->second->Command->GetStatus());
        buffer += "\n";
        iter++;
    }
    return buffer;
}

void KillAllTasks(const TString& user)
{
    TGuard<TMutex> lock(TasksLock);
    THashMap<TString, TRequest*>::iterator iter = Tasks.begin();
    while (iter != Tasks.end()) {
        if (user.empty() || iter->first.substr(0, user.size()) == user)
            iter->second->Cancel();
        iter++;
    }
}

bool KillTask(const TString& id)
{
    TGuard<TMutex> lock(TasksLock);
    THashMap<TString, TRequest*>::iterator target = Tasks.find(id);
    if (target != Tasks.end()) {
        target->second->Cancel();
        return true;
    }

    return false;
}

void* StartService (const TString& id, TService* cmd, NNetliba_v12::TUdpHttpRequest* req)
{
    TString user = id.substr(0, id.find_first_of('-'));
    TRequest* r = new TRequest(user, id, cmd, req);
    TThread* th = new TThread(StartTask, r);
    r->Th.Reset(th);
    th->Start();
    return nullptr;
}

void TRequest::AddRequest(NNetliba_v12::TUdpHttpRequest* req)
{
    TGuard<TMutex> lock(IncomingLock);
    Incoming.push_back(req);
    NextMessageEvt.Signal();
}

bool TRequest::NextRequest(bool wait)
{
    do {
        {
            TGuard<TMutex> lock(IncomingLock);
            if (!Incoming.empty()) {
                Request.Reset(Incoming.at(0));
                Incoming.erase(Incoming.begin());
                if (Incoming.empty())
                    NextMessageEvt.Reset();
                return true;
            }
        }
        if (wait)
            NextMessageEvt.Wait();
    } while(wait && Active);
    {
        TGuard<TMutex> lock(IncomingLock);
        Request.Reset(nullptr);
    }
    return false;
}

void TRequest::Cancel()
{
    if (!Active)
        return;
    Active = false;
    NextMessageEvt.Signal();
    Command->Kill();
}
