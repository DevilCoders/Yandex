#include "fs.h"

#include <util/generic/guid.h>
#include <util/string/builder.h>

namespace NCloud::NFileStore {

////////////////////////////////////////////////////////////////////////////////

NProto::TCreateSessionResponse TLocalFileSystem::CreateSession(
    const NProto::TCreateSessionRequest& request)
{
    STORAGE_TRACE("CreateSession " << DumpMessage(request));

    const auto& clientId = request.GetHeaders().GetClientId();
    auto sessionId = request.GetHeaders().GetSessionId();

    TWriteGuard guard(SessionsLock);

    if (sessionId) {
        auto it = Sessions.find(sessionId);
        if (it == Sessions.end() || (*it->second)->ClientId != clientId) {
            return TErrorResponse(E_FS_INVALID_SESSION, TStringBuilder()
                << "cannot restore session: " << sessionId.Quote());
        }

        // TODO: ping session
        return TErrorResponse(S_ALREADY, TStringBuilder()
            << "session already exists: " << sessionId.Quote());
    }  else {
        sessionId = CreateGuidAsString();
    }

    auto session = std::make_shared<TSession>(
        Root.GetPath(),
        clientId,
        sessionId,
        Index);

    SessionsList.push_front(session);

    auto [it, inserted] = Sessions.emplace(sessionId, SessionsList.begin());
    Y_VERIFY(inserted);

    NProto::TCreateSessionResponse response;

    auto* info = response.MutableSession();
    info->SetSessionId(sessionId);

    return response;
}

NProto::TPingSessionResponse TLocalFileSystem::PingSession(
    const NProto::TPingSessionRequest& request)
{
    const auto& clientId = request.GetHeaders().GetClientId();
    const auto& sessionId = request.GetHeaders().GetSessionId();

    TWriteGuard guard(SessionsLock);

    auto it = Sessions.find(sessionId);
    if (it == Sessions.end()) {
        return TErrorResponse(E_FS_INVALID_SESSION, TStringBuilder()
            << "invalid session: " << sessionId.Quote());
    }

    TSessionPtr session = *it->second;
    if (session->ClientId != clientId) {
        return TErrorResponse(E_FS_INVALID_SESSION, TStringBuilder()
            << "invalid session: " << sessionId.Quote());
    }

    session->Ping();
    SessionsList.splice(SessionsList.begin(), SessionsList, it->second);

    return {};
}

NProto::TDestroySessionResponse TLocalFileSystem::DestroySession(
    const NProto::TDestroySessionRequest& request)
{
    STORAGE_TRACE("DestroySession " << DumpMessage(request));

    const auto& clientId = request.GetHeaders().GetClientId();
    const auto& sessionId = request.GetHeaders().GetSessionId();

    TWriteGuard guard(SessionsLock);

    auto it = Sessions.find(sessionId);
    if (it == Sessions.end()) {
        return TErrorResponse(S_FALSE, TStringBuilder()
            << "invalid session: " << sessionId.Quote());
    }

    TSessionPtr session = *it->second;
    if (session->ClientId != clientId) {
        return TErrorResponse(E_FS_INVALID_SESSION, TStringBuilder()
            << "invalid session: " << sessionId.Quote());
    }

    SessionsList.erase(it->second);
    Sessions.erase(it);

    return {};
}

NProto::TResetSessionResponse TLocalFileSystem::ResetSession(
    const NProto::TResetSessionRequest& request)
{
    STORAGE_TRACE("ResetSession " << DumpMessage(request));
    return {};
}

////////////////////////////////////////////////////////////////////////////////

TSessionPtr TLocalFileSystem::GetSession(
    const TString& clientId,
    const TString& sessionId)
{
    TReadGuard guard(SessionsLock);

    auto it = Sessions.find(sessionId);
    Y_ENSURE_EX(it != Sessions.end(), TServiceError(E_FS_INVALID_SESSION)
        << "invalid session: " << sessionId.Quote());

    TSessionPtr session = *it->second;
    Y_ENSURE_EX(session->ClientId == clientId, TServiceError(E_FS_INVALID_SESSION)
        << "invalid session: " << sessionId.Quote());

    return session;
}

void TLocalFileSystem::ScheduleCleanupSessions()
{
    auto weak_ptr = weak_from_this();

    Scheduler->Schedule(
        Timer->Now() + TDuration::Seconds(1),  // TODO
        [=] () {
            if (auto ptr = weak_ptr.lock()) {
                ptr->CleanupSessions();
            }
        });
}

void TLocalFileSystem::CleanupSessions()
{
    TWriteGuard guard(SessionsLock);

    auto deadline = TInstant::Now() - Config->GetIdleSessionTimeout();
    while (!SessionsList.empty()
        && SessionsList.back()->GetLastPing() < deadline)
    {
        Sessions.erase(SessionsList.back()->SessionId);
        SessionsList.pop_back();
    }

    ScheduleCleanupSessions();
}

}   // namespace NCloud::NFileStore
