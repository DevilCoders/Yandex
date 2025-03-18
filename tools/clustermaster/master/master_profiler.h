#pragma once

#include "time_counters.h"

#include <tools/clustermaster/proto/messages.pb.h>

#include <util/datetime/cputimer.h>
#include <util/system/datetime.h>

struct TMasterProfiler {
    enum EMasterProfilerId {
        ID_WHOLE_MAIN_LOOP_TIME,
        ID_RELOAD_SCRIPT_TIME,
        ID_PROCESS_CONNECTIONS_TIME,
        ID_NETWORK_TIME,
        ID_PROCESS_EVENTS_TIME,
        ID_MESSAGES_ALL_TIME,
        ID_FULL_STATUS_MSG_TIME,
        ID_FULL_STATUS_MSG_POKES_TIME,
        ID_SINGLE_STATUS_MSG_TIME,
        ID_SINGLE_STATUS_MSG_POKES_TIME,
        ID_THIN_STATUS_MSG_TIME,
        ID_THIN_STATUS_MSG_POKES_TIME,
        ID_ASYNC_POKES_TIME,
        ID_FAILURE_EMAILS_DISPATCH,
        ID_MAIN_LOOP_ITERATIONS,
        ID_HTTP_REQUESTS_TOTAL,
        ID_HTTP_REQUESTS_COMMAND,
        ID_HTTP_REQUESTS_STATUS_TARGETS,
        ID_HTTP_REQUESTS_STATUS_WORKERS,
    };

    TMasterProfiler()
    {
        TTimeCounters<EMasterProfilerId>::TNamesVector names;
        names.push_back(TCounterName<EMasterProfilerId>(ID_WHOLE_MAIN_LOOP_TIME, "WHOLE_MAIN_LOOP_TIME"));
        names.push_back(TCounterName<EMasterProfilerId>(ID_RELOAD_SCRIPT_TIME, "RELOAD_SCRIPT_TIME"));
        names.push_back(TCounterName<EMasterProfilerId>(ID_PROCESS_CONNECTIONS_TIME, "PROCESS_CONNECTIONS_TIME"));
        names.push_back(TCounterName<EMasterProfilerId>(ID_NETWORK_TIME, "NETWORK_TIME"));
        names.push_back(TCounterName<EMasterProfilerId>(ID_PROCESS_EVENTS_TIME, "PROCESS_EVENTS_TIME"));
        names.push_back(TCounterName<EMasterProfilerId>(ID_MESSAGES_ALL_TIME, "MESSAGES_ALL_TIME"));
        names.push_back(TCounterName<EMasterProfilerId>(ID_FULL_STATUS_MSG_TIME, "FULL_STATUS_MSG_TIME"));
        names.push_back(TCounterName<EMasterProfilerId>(ID_FULL_STATUS_MSG_POKES_TIME, "FULL_STATUS_MSG_POKES_TIME"));
        names.push_back(TCounterName<EMasterProfilerId>(ID_SINGLE_STATUS_MSG_TIME, "SINGLE_STATUS_MSG_TIME"));
        names.push_back(TCounterName<EMasterProfilerId>(ID_SINGLE_STATUS_MSG_POKES_TIME, "SINGLE_STATUS_MSG_POKES_TIME"));
        names.push_back(TCounterName<EMasterProfilerId>(ID_THIN_STATUS_MSG_TIME, "THIN_STATUS_MSG_TIME"));
        names.push_back(TCounterName<EMasterProfilerId>(ID_THIN_STATUS_MSG_POKES_TIME, "THIN_STATUS_MSG_POKES_TIME"));
        names.push_back(TCounterName<EMasterProfilerId>(ID_ASYNC_POKES_TIME, "ASYNC_POKES_TIME"));
        names.push_back(TCounterName<EMasterProfilerId>(ID_FAILURE_EMAILS_DISPATCH, "ID_FAILURE_EMAILS_DISPATCH"));
        names.push_back(TCounterName<EMasterProfilerId>(ID_MAIN_LOOP_ITERATIONS, "MAIN_LOOP_ITERATIONS"));
        names.push_back(TCounterName<EMasterProfilerId>(ID_HTTP_REQUESTS_TOTAL, "HTTP_REQUESTS_TOTAL"));
        names.push_back(TCounterName<EMasterProfilerId>(ID_HTTP_REQUESTS_COMMAND, "HTTP_REQUESTS_COMMAND"));
        names.push_back(TCounterName<EMasterProfilerId>(ID_HTTP_REQUESTS_STATUS_TARGETS, "HTTP_REQUESTS_STATUS_TARGETS"));
        names.push_back(TCounterName<EMasterProfilerId>(ID_HTTP_REQUESTS_STATUS_WORKERS, "HTTP_REQUESTS_STATUS_WORKERS"));

        Counters.Reset(new TTimeCounters<EMasterProfilerId>(names));

        MainLoopTimeCounterIds.push_back(ID_WHOLE_MAIN_LOOP_TIME);
        MainLoopTimeCounterIds.push_back(ID_RELOAD_SCRIPT_TIME);
        MainLoopTimeCounterIds.push_back(ID_PROCESS_CONNECTIONS_TIME);
        MainLoopTimeCounterIds.push_back(ID_NETWORK_TIME);
        MainLoopTimeCounterIds.push_back(ID_PROCESS_EVENTS_TIME);
        MainLoopTimeCounterIds.push_back(ID_MESSAGES_ALL_TIME);
        MainLoopTimeCounterIds.push_back(ID_FULL_STATUS_MSG_TIME);
        MainLoopTimeCounterIds.push_back(ID_FULL_STATUS_MSG_POKES_TIME);
        MainLoopTimeCounterIds.push_back(ID_SINGLE_STATUS_MSG_TIME);
        MainLoopTimeCounterIds.push_back(ID_SINGLE_STATUS_MSG_POKES_TIME);
        MainLoopTimeCounterIds.push_back(ID_THIN_STATUS_MSG_TIME);
        MainLoopTimeCounterIds.push_back(ID_THIN_STATUS_MSG_POKES_TIME);
        MainLoopTimeCounterIds.push_back(ID_ASYNC_POKES_TIME);
        MainLoopTimeCounterIds.push_back(ID_FAILURE_EMAILS_DISPATCH);

        HttpCounterIds.push_back(ID_HTTP_REQUESTS_TOTAL);
        HttpCounterIds.push_back(ID_HTTP_REQUESTS_COMMAND);
        HttpCounterIds.push_back(ID_HTTP_REQUESTS_STATUS_TARGETS);
        HttpCounterIds.push_back(ID_HTTP_REQUESTS_STATUS_WORKERS);
    }

    THolder<TTimeCounters<EMasterProfilerId> > Counters;
    TVector<EMasterProfilerId> MainLoopTimeCounterIds;
    TVector<EMasterProfilerId> HttpCounterIds;
};

struct TMessageProfiler2 {
    int SendCode(NProto::EMessageType msgType) {
        return msgType;
    }

    int RecvCode(NProto::EMessageType msgType) {
        return NProto::MT_NUMMESSAGES + msgType;
    }

    struct TNamesByIdComparer {
        bool operator() (const TCounterName<int>& a, const TCounterName<int>& b) {
            return a.Id < b.Id;
        }
    };

    TMessageProfiler2()
    {
        TTimeCounters<int>::TNamesVector names;
        names.push_back(TCounterName<int>(SendCode(NProto::MT_UNKNOWN), "MSG_UNKNOWN_SEND"));
        names.push_back(TCounterName<int>(RecvCode(NProto::MT_UNKNOWN), "MSG_UNKNOWN_RECV"));
        names.push_back(TCounterName<int>(SendCode(NProto::MT_ERROR), "MSG_ERROR_SEND"));
        names.push_back(TCounterName<int>(RecvCode(NProto::MT_ERROR), "MSG_ERROR_RECV"));
        names.push_back(TCounterName<int>(SendCode(NProto::MT_PING), "MSG_PING_SEND"));
        names.push_back(TCounterName<int>(RecvCode(NProto::MT_PING), "MSG_PING_RECV"));
        names.push_back(TCounterName<int>(SendCode(NProto::MT_TEST), "MSG_TEST_SEND"));
        names.push_back(TCounterName<int>(RecvCode(NProto::MT_TEST), "MSG_TEST_RECV"));
        names.push_back(TCounterName<int>(SendCode(NProto::MT_WORKERHELLO), "MSG_WORKERHELLO_SEND"));
        names.push_back(TCounterName<int>(RecvCode(NProto::MT_WORKERHELLO), "MSG_WORKERHELLO_RECV"));
        names.push_back(TCounterName<int>(SendCode(NProto::MT_AUTHREPLY), "MSG_AUTHREPLY_SEND"));
        names.push_back(TCounterName<int>(RecvCode(NProto::MT_AUTHREPLY), "MSG_AUTHREPLY_RECV"));
        names.push_back(TCounterName<int>(SendCode(NProto::MT_AUTHSUCCESS), "MSG_AUTHSUCCESS_SEND"));
        names.push_back(TCounterName<int>(RecvCode(NProto::MT_AUTHSUCCESS), "MSG_AUTHSUCCESS_RECV"));
        names.push_back(TCounterName<int>(SendCode(NProto::MT_NEWCONFIG), "MSG_NEWCONFIG_SEND"));
        names.push_back(TCounterName<int>(RecvCode(NProto::MT_NEWCONFIG), "MSG_NEWCONFIG_RECV"));
        names.push_back(TCounterName<int>(SendCode(NProto::MT_FULLSTATUS), "MSG_FULLSTATUS_SEND"));
        names.push_back(TCounterName<int>(RecvCode(NProto::MT_FULLSTATUS), "MSG_FULLSTATUS_RECV"));
        names.push_back(TCounterName<int>(SendCode(NProto::MT_SINGLESTATUS), "MSG_SINGLESTATUS_SEND"));
        names.push_back(TCounterName<int>(RecvCode(NProto::MT_SINGLESTATUS), "MSG_SINGLESTATUS_RECV"));
        names.push_back(TCounterName<int>(SendCode(NProto::MT_THINSTATUS), "MSG_THINSTATUS_SEND"));
        names.push_back(TCounterName<int>(RecvCode(NProto::MT_THINSTATUS), "MSG_THINSTATUS_RECV"));
        names.push_back(TCounterName<int>(SendCode(NProto::MT_FULLCROSSNODE), "MSG_FULLCROSSNODE_SEND"));
        names.push_back(TCounterName<int>(RecvCode(NProto::MT_FULLCROSSNODE), "MSG_FULLCROSSNODE_RECV"));
        names.push_back(TCounterName<int>(SendCode(NProto::MT_SINGLECROSSNODE), "MSG_SINGLECROSSNODE_SEND"));
        names.push_back(TCounterName<int>(RecvCode(NProto::MT_SINGLECROSSNODE), "MSG_SINGLECROSSNODE_RECV"));
        names.push_back(TCounterName<int>(SendCode(NProto::MT_COMMAND), "MSG_COMMAND_SEND"));
        names.push_back(TCounterName<int>(RecvCode(NProto::MT_COMMAND), "MSG_COMMAND_RECV"));
        names.push_back(TCounterName<int>(SendCode(NProto::MT_POKE), "MSG_POKE_SEND"));
        names.push_back(TCounterName<int>(RecvCode(NProto::MT_POKE), "MSG_POKE_RECV"));
        names.push_back(TCounterName<int>(SendCode(NProto::MT_VARIABLES), "MSG_VARIABLES_SEND"));
        names.push_back(TCounterName<int>(RecvCode(NProto::MT_VARIABLES), "MSG_VARIABLES_RECV"));
        names.push_back(TCounterName<int>(SendCode(NProto::MT_DISKSPACE), "MSG_DISKSPACE_SEND"));
        names.push_back(TCounterName<int>(RecvCode(NProto::MT_DISKSPACE), "MSG_DISKSPACE_RECV"));
        names.push_back(TCounterName<int>(SendCode(NProto::MT_MULTIPOKE), "MSG_MULTIPOKE_SEND"));
        names.push_back(TCounterName<int>(RecvCode(NProto::MT_MULTIPOKE), "MSG_MULTIPOKE_RECV"));
        names.push_back(TCounterName<int>(SendCode(NProto::MT_COMMAND2), "MSG_COMMAND2_SEND"));
        names.push_back(TCounterName<int>(RecvCode(NProto::MT_COMMAND2), "MSG_COMMAND2_RECV"));
        Sort(names.begin(), names.end(), TNamesByIdComparer());

        Counters.Reset(new TTimeCounters<int>(names));
    }

    THolder<TTimeCounters<int> > Counters;

    void Sent(NProto::EMessageType msgType) {
        Counters->AddOne(SendCode(msgType));
    }

    void Received(NProto::EMessageType msgType) {
        Counters->AddOne(RecvCode(msgType));
    }
};

inline TMasterProfiler& GetMasterProfiler() {
    return *Singleton<TMasterProfiler>();
}

inline TMessageProfiler2& GetMessageProfiler2() {
    return *Singleton<TMessageProfiler2>();
}

template<class T>
struct TProfilerGuard {
    TProfilerGuard(TTimeCounters<T>* timeCounters, T id)
        : TimeCounters(timeCounters)
        , Id(id)
        , Start()
        , Aquired(false)
    {
        Aquire();
    }

    ~TProfilerGuard() {
        Release();
    }

    void Aquire() {
        Aquired = true;
        Start = GetCycleCount();
    }

    void Release() {
        if (Aquired) {
            TDuration duration = CyclesToDuration(GetCycleCount() - Start);
            TimeCounters->Add(Id, duration.MicroSeconds());
            Aquired = false;
        }
    }

    TTimeCounters<T>* TimeCounters;
    T Id;
    ui64 Start;
    bool Aquired;
};

#define PROFILE_ACQUIRE(ID) TProfilerGuard<TMasterProfiler::EMasterProfilerId> __ ## ID ## _(Singleton<TMasterProfiler>()->Counters.Get(), TMasterProfiler::ID);
#define PROFILE_RELEASE(ID) __ ## ID ## _.Release();
