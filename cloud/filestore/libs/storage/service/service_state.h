#pragma once

#include "public.h"

#include <cloud/filestore/libs/diagnostics/public.h>
#include <cloud/filestore/public/api/protos/session.pb.h>

#include <library/cpp/actors/core/actorid.h>

#include <util/datetime/base.h>
#include <util/generic/hash.h>
#include <util/generic/intrlist.h>
#include <util/generic/string.h>

namespace NCloud::NFileStore::NStorage {

////////////////////////////////////////////////////////////////////////////////

struct TSessionInfo
    : public TIntrusiveListItem<TSessionInfo>
{
    TString ClientId;
    TString FileSystemId;
    TString SessionId;
    TString SessionState;

    NActors::TActorId SessionActor;
    ui64 TabletId = 0;

    bool ShouldStop = false;

    void GetInfo(NProto::TSessionInfo& info)
    {
        info.SetSessionId(SessionId);
        info.SetSessionState(SessionState);
    }
};

using TSessionList = TIntrusiveListWithAutoDelete<TSessionInfo, TDelete>;
using TSessionMap = THashMap<TString, TSessionInfo*>;
using TSessionVisitor = std::function<void(const TSessionInfo& sessionInfo)>;

////////////////////////////////////////////////////////////////////////////////

class TStorageServiceState
{
private:
    TSessionList Sessions;
    TSessionMap SessionById;

public:
    TSessionInfo* CreateSession(
        TString clientId,
        TString fileSystemId,
        TString sessionId,
        TString sessionState,
        const NActors::TActorId& sessionActor,
        ui64 tabletId);

    TSessionInfo* FindSession(
        const TString& clientId,
        const TString& fileSystemId);

    TSessionInfo* FindSession(const TString& sessionId);

    void RemoveSession(TSessionInfo* session);

    void VisitSessions(const TSessionVisitor& visitor) const;
};

}   // namespace NCloud::NFileStore::NStorage
