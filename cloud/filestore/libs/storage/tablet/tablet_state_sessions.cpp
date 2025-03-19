#include "tablet_state_impl.h"

#include <library/cpp/actors/core/actor.h>
#include <library/cpp/actors/core/log.h>

#include <util/random/random.h>

namespace NCloud::NFileStore::NStorage {

using namespace NActors;

////////////////////////////////////////////////////////////////////////////////
// Sessions

void TIndexTabletState::LoadSessions(
    TInstant idleSessionDeadline,
    const TVector<NProto::TSession>& sessions,
    const TVector<NProto::TSessionHandle>& handles,
    const TVector<NProto::TSessionLock>& locks,
    const TVector<NProto::TDupCacheEntry>& cacheEntries)
{
    for (const auto& proto: sessions) {
        LOG_INFO(*TlsActivationContext, TFileStoreComponents::TABLET,
            "restoring session c: %s, s: %s",
            proto.GetClientId().Quote().data(),
            proto.GetSessionId().Quote().data());

        auto* session = CreateSession(proto, idleSessionDeadline);
        Y_VERIFY(session);
    }

    for (const auto& proto: handles) {
        auto* session = FindSession(proto.GetSessionId());
        Y_VERIFY(session, "no session for %s", proto.ShortDebugString().c_str());


        auto* handle = CreateHandle(session, proto);
        Y_VERIFY(handle, "failed to create handle %s", proto.ShortDebugString().c_str());
    }

    for (const auto& proto: locks) {
        auto* session = FindSession(proto.GetSessionId());
        Y_VERIFY(session, "no session for %s", proto.ShortDebugString().c_str());

        TVector<ui64> removedLocks;
        auto* lock = CreateLock(session, proto, removedLocks);
        Y_VERIFY(lock, "failed to create lock %s", proto.ShortDebugString().c_str());
        Y_VERIFY(removedLocks.empty(), "non empty removed locks %s", proto.ShortDebugString().c_str());
    }

    TSession* session = nullptr;
    for (const auto& entry: cacheEntries) {
        if (!session || session->GetSessionId() != entry.GetSessionId()) {
            session = FindSession(entry.GetSessionId());
            Y_VERIFY(session, "no session for dup cache entry %s",
                entry.ShortDebugString().c_str());
        }

        session->LoadDupCacheEntry(std::move(entry));
    }
}

TSession* TIndexTabletState::CreateSession(
    TIndexTabletDatabase& db,
    const TString& clientId,
    const TString& sessionId,
    const TString& checkpointId,
    const TActorId& owner)
{
    LOG_INFO(*TlsActivationContext, TFileStoreComponents::TABLET,
        "creating session c: %s, s: %s",
        clientId.Quote().data(),
        sessionId.Quote().data());

    NProto::TSession proto;
    proto.SetClientId(clientId);
    proto.SetSessionId(sessionId);
    proto.SetCheckpointId(checkpointId);

    db.WriteSession(proto);
    IncrementUsedSessionsCount(db);

    auto* session = CreateSession(proto, owner);
    Y_VERIFY(session);

    return session;
}

TSession* TIndexTabletState::CreateSession(
    const NProto::TSession& proto,
    TInstant deadline)
{
    auto session = std::make_unique<TSession>(proto);
    session->Deadline = deadline;

    Impl->OrphanSessions.PushBack(session.get());
    Impl->SessionById.emplace(session->GetSessionId(), session.get());
    Impl->SessionByClient.emplace(session->GetClientId(), session.get());

    return session.release();
}

TSession* TIndexTabletState::CreateSession(
    const NProto::TSession& proto,
    const TActorId& owner)
{
    auto session = std::make_unique<TSession>(proto, owner);

    Impl->Sessions.PushBack(session.get());
    Impl->SessionById.emplace(session->GetSessionId(), session.get());
    Impl->SessionByOwner.emplace(session->Owner, session.get());
    Impl->SessionByClient.emplace(session->GetClientId(), session.get());

    return session.release();
}

NActors::TActorId TIndexTabletState::RecoverSession(TSession* session, const TActorId& owner)
{
    auto oldOwner = session->Owner;
    if (oldOwner) {
        Impl->SessionByOwner.erase(oldOwner);
    }

    session->Deadline = {};
    session->Owner = owner;

    session->Unlink();
    Impl->Sessions.PushBack(session);

    Impl->SessionByOwner.emplace(session->Owner, session);

    return oldOwner;
}

TSession* TIndexTabletState::FindSession(const TString& sessionId) const
{
    auto it = Impl->SessionById.find(sessionId);
    if (it != Impl->SessionById.end()) {
        return it->second;
    }

    return nullptr;
}

TSession* TIndexTabletState::FindSessionByClientId(const TString& clientId) const
{
    auto it = Impl->SessionByClient.find(clientId);
    if (it != Impl->SessionByClient.end()) {
        return it->second;
    }

    return nullptr;
}

TSession* TIndexTabletState::FindSession(
    const TString& clientId,
    const TString& sessionId) const
{
    auto session = FindSession(sessionId);
    if (session && session->IsValid() && session->GetClientId() == clientId) {
        return session;
    }

    return nullptr;
}

void TIndexTabletState::OrphanSession(const TActorId& owner, TInstant deadline)
{
    auto it = Impl->SessionByOwner.find(owner);
    if (it == Impl->SessionByOwner.end()) {
        return; // not a session pipe
    }

    auto* session = it->second;

    LOG_INFO(*TlsActivationContext, TFileStoreComponents::TABLET,
        "orphaning session c: %s, s: %s",
        session->GetClientId().Quote().data(),
        session->GetSessionId().Quote().data());

    session->Owner = {};
    session->Deadline = deadline;

    session->Unlink();
    Impl->OrphanSessions.PushBack(session);

    Impl->SessionByOwner.erase(it);
}

void TIndexTabletState::ResetSession(
    TIndexTabletDatabase& db,
    TSession* session,
    const TMaybe<TString>& state)
{
    LOG_INFO(*TlsActivationContext, TFileStoreComponents::TABLET,
        "resetting session c: %s, s: %s",
        session->GetClientId().Quote().data(),
        session->GetSessionId().Quote().data());

    auto handle = session->Handles.begin();
    while (handle != session->Handles.end()) {
        DestroyHandle(db, &*(handle++));
    }

    auto lock = session->Locks.begin();
    while (lock != session->Locks.end()) {
        auto& cur = *(lock++);
        TLockRange range = {
            .NodeId = cur.GetNodeId(),
            .OwnerId = cur.GetOwner(),
            .Offset = cur.GetOffset(),
            .Length = cur.GetLength(),
        };

        ReleaseLock(db, session, range);
    }

    while (auto entryId = session->PopDupCacheEntry(0)) {
        db.DeleteSessionDupCacheEntry(session->GetSessionId(), entryId);
    }

    if (state) {
        session->SetSessionState(*state);
        db.WriteSession(*session);
    }
}

void TIndexTabletState::RemoveSession(
    TIndexTabletDatabase& db,
    const TString& sessionId)
{
    auto* session = FindSession(sessionId);
    Y_VERIFY(session);

    // no need to update state before session deletion
    ResetSession(db, session, {});

    db.DeleteSession(sessionId);
    DecrementUsedSessionsCount(db);

    RemoveSession(session);
}

void TIndexTabletState::RemoveSession(TSession* session)
{
    LOG_INFO(*TlsActivationContext, TFileStoreComponents::TABLET,
        "removing session c: %s, s: %s",
        session->GetClientId().Quote().data(),
        session->GetSessionId().Quote().data());

    std::unique_ptr<TSession> holder(session);
    session->Unlink();

    Impl->SessionById.erase(session->GetSessionId());
    Impl->SessionByOwner.erase(session->Owner);
    Impl->SessionByClient.erase(session->GetClientId());
}

TVector<TSession*> TIndexTabletState::GetTimeoutedSessions(TInstant now) const
{
    TVector<TSession*> result;
    for (auto& session: Impl->OrphanSessions) {
        if (session.Deadline < now) {
            result.push_back(&session);
        } else {
            break;
        }
    }

    return result;
}

TVector<TSession*> TIndexTabletState::GetSessionsToNotify(
    const NProto::TSessionEvent& event) const
{
    // TODO
    Y_UNUSED(event);

    TVector<TSession*> result;
    for (auto& session: Impl->Sessions) {
        if (session.NotifyEvents) {
            result.push_back(&session);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////
// Handles

ui64 TIndexTabletState::GenerateHandle() const
{
    ui64 h;
    do {
        h = RandomNumber<ui64>();
    } while (!h || Impl->HandleById.contains(h));

    return h;
}

TSessionHandle* TIndexTabletState::CreateHandle(
    TSession* session,
    const NProto::TSessionHandle& proto)
{
    auto handle = std::make_unique<TSessionHandle>(session, proto);

    session->Handles.PushBack(handle.get());
    Impl->HandleById.emplace(handle->GetHandle(), handle.get());
    Impl->NodeRefsByHandle[proto.GetNodeId()]++;

    return handle.release();
}

void TIndexTabletState::RemoveHandle(TSessionHandle* handle)
{
    std::unique_ptr<TSessionHandle> holder(handle);

    handle->Unlink();
    Impl->HandleById.erase(handle->GetHandle());

    auto it = Impl->NodeRefsByHandle.find(handle->GetNodeId());
    Y_VERIFY(it != Impl->NodeRefsByHandle.end());
    Y_VERIFY(it->second > 0);

    if (--(it->second) == 0) {
        Impl->NodeRefsByHandle.erase(it);
    }
}

TSessionHandle* TIndexTabletState::FindHandle(ui64 handle) const
{
    auto it = Impl->HandleById.find(handle);
    if (it != Impl->HandleById.end()) {
        return it->second;
    }

    return nullptr;
}

TSessionHandle* TIndexTabletState::CreateHandle(
    TIndexTabletDatabase& db,
    TSession* session,
    ui64 nodeId,
    ui64 commitId,
    ui32 flags)
{
    ui64 handleId = GenerateHandle();

    NProto::TSessionHandle proto;
    proto.SetSessionId(session->GetSessionId());
    proto.SetHandle(handleId);
    proto.SetNodeId(nodeId);
    proto.SetCommitId(commitId);
    proto.SetFlags(flags);

    db.WriteSessionHandle(proto);
    IncrementUsedHandlesCount(db);

    return CreateHandle(session, proto);
}

void TIndexTabletState::DestroyHandle(
    TIndexTabletDatabase& db,
    TSessionHandle* handle)
{
    db.DeleteSessionHandle(
        handle->GetSessionId(),
        handle->GetHandle());

    DecrementUsedHandlesCount(db);

    ReleaseLocks(db, handle->GetHandle());

    RemoveHandle(handle);
}

bool TIndexTabletState::HasOpenHandles(ui64 nodeId) const
{
    auto it = Impl->NodeRefsByHandle.find(nodeId);
    if (it != Impl->NodeRefsByHandle.end()) {
        Y_VERIFY(it->second > 0);
        return true;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////
// Locks

TSessionLock* TIndexTabletState::FindLock(ui64 lockId) const
{
    auto it = Impl->LockById.find(lockId);
    if (it != Impl->LockById.end()) {
        return it->second;
    }

    return nullptr;
}

TSessionLock* TIndexTabletState::CreateLock(
    TSession* session,
    const NProto::TSessionLock& proto,
    TVector<ui64>& removedLocks)
{
    auto lock = std::make_unique<TSessionLock>(session, proto);

    session->Locks.PushBack(lock.get());
    Impl->LockById.emplace(lock->GetLockId(), lock.get());
    Impl->LocksByHandle.emplace(lock->GetHandle(), lock.get());

    TLockRange range = {
        .NodeId = proto.GetNodeId(),
        .OwnerId = proto.GetOwner(),
        .Offset = proto.GetOffset(),
        .Length = proto.GetLength(),
    };

    bool acquired = Impl->RangeLocks.Acquire(
        session->GetSessionId(),
        proto.GetLockId(),
        range,
        static_cast<ELockMode>(proto.GetMode()),
        removedLocks);

    Y_VERIFY(acquired);

    return lock.release();
}

void TIndexTabletState::RemoveLock(TSessionLock* lock)
{
    std::unique_ptr<TSessionLock> holder(lock);

    lock->Unlink();
    Impl->LockById.erase(lock->GetLockId());

    auto [it, end] = Impl->LocksByHandle.equal_range(lock->GetHandle());
    it = std::find(it, end, std::pair<const ui64, TSessionLock*>(lock->GetHandle(), lock));
    Y_VERIFY(it != end, "failed to find lock by handle: %s", lock->ShortDebugString().c_str());

    Impl->LocksByHandle.erase(it);
}

void TIndexTabletState::AcquireLock(
    TIndexTabletDatabase& db,
    TSession* session,
    ui64 handle,
    TLockRange range,
    ELockMode mode)
{
    const auto& sessionId = session->GetSessionId();

    ui64 lockId = IncrementLastLockId(db);

    NProto::TSessionLock proto;
    proto.SetSessionId(sessionId);
    proto.SetLockId(lockId);
    proto.SetHandle(handle);
    proto.SetNodeId(range.NodeId);
    proto.SetOwner(range.OwnerId);
    proto.SetOffset(range.Offset);
    proto.SetLength(range.Length);
    proto.SetMode(static_cast<ui32>(mode));

    IncrementUsedLocksCount(db);
    db.WriteSessionLock(proto);

    TVector<ui64> removedLocks;
    auto* lock = CreateLock(session, proto, removedLocks);
    Y_VERIFY(lock);

    LOG_DEBUG(*TlsActivationContext, TFileStoreComponents::TABLET,
        "acquired lock c: %s, s: %s, o: %lu, n: %lu, o: %lu, l: %lu r: %lu",
        session->GetClientId().c_str(),
        session->GetSessionId().c_str(),
        range.OwnerId, range.NodeId, range.Offset, range.Length, removedLocks.size());

    for (ui64 removedLockId: removedLocks) {
        auto* removedLock = FindLock(removedLockId);
        Y_VERIFY(removedLock && removedLock->Session == session);

        db.DeleteSessionLock(sessionId, removedLockId);
        RemoveLock(removedLock);
    }

    DecrementUsedLocksCount(db, removedLocks.size());
}

void TIndexTabletState::ReleaseLock(
    TIndexTabletDatabase& db,
    TSession* session,
    TLockRange range)
{
    const auto& sessionId = session->GetSessionId();

    TVector<ui64> removedLocks;
    Impl->RangeLocks.Release(
        sessionId,
        range,
        removedLocks);

    LOG_DEBUG(*TlsActivationContext, TFileStoreComponents::TABLET,
        "releasing lock c: %s, s: %s, o: %lu, n: %lu, o: %lu, l: %lu r: %lu",
        session->GetClientId().c_str(),
        session->GetSessionId().c_str(),
        range.OwnerId, range.NodeId, range.Offset, range.Length, removedLocks.size());

    for (ui64 removedLockId: removedLocks) {
        auto* removedLock = FindLock(removedLockId);
        Y_VERIFY(removedLock && removedLock->Session == session);

        db.DeleteSessionLock(sessionId, removedLockId);
        RemoveLock(removedLock);
    }

    DecrementUsedLocksCount(db, removedLocks.size());
}

bool TIndexTabletState::TestLock(
    TSession* session,
    TLockRange range,
    ELockMode mode,
    TLockRange* conflicting) const
{
    return Impl->RangeLocks.Test(
        session->GetSessionId(),
        range,
        mode,
        conflicting);
}

void TIndexTabletState::ReleaseLocks(
    TIndexTabletDatabase& db,
    ui64 handle)
{
    TSmallVec<TSessionLock*> locks;
    auto [it, end] = Impl->LocksByHandle.equal_range(handle);
    for (; it != end; ++it) {
        locks.push_back(it->second);
    }

    for (auto lock: locks) {
        TLockRange range = {
            .NodeId = lock->GetNodeId(),
            .OwnerId = lock->GetOwner(),
            .Offset = lock->GetOffset(),
            .Length = lock->GetLength(),
        };

        ReleaseLock(
            db,
            lock->Session,
            range);
    }
}

#define FILESTORE_IMPLEMENT_DUPCACHE(name, ...)                                \
void TIndexTabletState::AddDupCacheEntry(                                      \
    TIndexTabletDatabase& db,                                                  \
    TSession* session,                                                         \
    ui64 requestId,                                                            \
    const NProto::T##name##Response& response,                                 \
    ui32 maxEntries)                                                           \
{                                                                              \
    if (!requestId || !maxEntries) {                                           \
        return;                                                                \
    }                                                                          \
                                                                               \
    NProto::TDupCacheEntry entry;                                              \
    entry.SetSessionId(session->GetSessionId());                               \
    entry.SetEntryId(session->LastDupCacheEntryId++);                          \
    entry.SetRequestId(requestId);                                             \
    *entry.Mutable##name() = response;                                         \
                                                                               \
    db.WriteSessionDupCacheEntry(entry);                                       \
    session->AddDupCacheEntry(std::move(entry), false);                        \
                                                                               \
    while (auto entryId = session->PopDupCacheEntry(maxEntries)) {             \
        db.DeleteSessionDupCacheEntry(session->GetSessionId(), entryId);       \
    }                                                                          \
}                                                                              \
                                                                               \
void TIndexTabletState::GetDupCacheEntry(                                      \
    const TDupCacheEntry* entry,                                               \
    NProto::T##name##Response& response)                                       \
{                                                                              \
    if (entry->Commited && entry->Has##name()) {                               \
        response = entry->Get##name();                                         \
    } else if (!entry->Commited) {                                             \
        *response.MutableError() = MakeError(                                  \
            E_REJECTED, "request is being processed");                         \
    } else if (!entry->Has##name()) {                                          \
        *response.MutableError() = MakeError(                                  \
            E_ARGUMENT, "invalid request dup cache type");                     \
    }                                                                          \
}                                                                              \
// FILESTORE_IMPLEMENT_DUPCACHE

FILESTORE_DUPCACHE_REQUESTS(FILESTORE_IMPLEMENT_DUPCACHE)

#undef FILESTORE_IMPLEMENT_DUPCACHE

void TIndexTabletState::CommitDupCacheEntry(
    const TString& sessionId,
    ui64 requestId)
{
    if (auto session = FindSession(sessionId)) {
        session->CommitDupCacheEntry(requestId);
    }
}

}   // namespace NCloud::NFileStore::NStorage
