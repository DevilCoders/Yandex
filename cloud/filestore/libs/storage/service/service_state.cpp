#include "service_state.h"

namespace NCloud::NFileStore::NStorage {

using namespace NActors;

////////////////////////////////////////////////////////////////////////////////

TSessionInfo* TStorageServiceState::CreateSession(
    TString clientId,
    TString fileSystemId,
    TString sessionId,
    TString sessionState,
    const TActorId& sessionActor,
    ui64 tabletId)
{
    auto session = std::make_unique<TSessionInfo>();
    session->ClientId = std::move(clientId);
    session->FileSystemId = std::move(fileSystemId);
    session->SessionId = std::move(sessionId);
    session->SessionState = std::move(sessionState);
    session->SessionActor = sessionActor;
    session->TabletId = tabletId;

    Sessions.PushBack(session.get());
    SessionById.emplace(session->SessionId, session.get());

    return session.release();
}

TSessionInfo* TStorageServiceState::FindSession(
    const TString& fileSystemId,
    const TString& clientId)
{
    for (auto& session: Sessions) {
        if (session.ClientId == clientId &&
            session.FileSystemId == fileSystemId)
        {
            return &session;
        }
    }
    return nullptr;
}

TSessionInfo* TStorageServiceState::FindSession(const TString& sessionId)
{
    auto it = SessionById.find(sessionId);
    if (it != SessionById.end()) {
        return it->second;
    }
    return nullptr;
}

void TStorageServiceState::RemoveSession(TSessionInfo* session)
{
    std::unique_ptr<TSessionInfo> holder(session);

    SessionById.erase(session->SessionId);
    session->Unlink();
}

void TStorageServiceState::VisitSessions(const TSessionVisitor& visitor) const
{
    for (const auto& session: Sessions) {
        visitor(session);
    }
}

}   // namespace NCloud::NFileStore::NStorage
