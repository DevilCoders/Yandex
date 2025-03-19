#pragma once

#include "public.h"

#include <cloud/filestore/libs/storage/tablet/protos/tablet.pb.h>

#include <library/cpp/actors/core/actorid.h>

#include <util/datetime/base.h>
#include <util/generic/hash.h>
#include <util/generic/intrlist.h>

namespace NCloud::NFileStore::NStorage {

////////////////////////////////////////////////////////////////////////////////

struct TSessionHandle
    : public TIntrusiveListItem<TSessionHandle>
    , public NProto::TSessionHandle
{
    TSession* const Session;

    TSessionHandle(TSession* session, const NProto::TSessionHandle& proto)
        : NProto::TSessionHandle(proto)
        , Session(session)
    {}
};

using TSessionHandleList = TIntrusiveListWithAutoDelete<TSessionHandle, TDelete>;
using TSessionHandleMap = THashMap<ui64, TSessionHandle*>;

using TNodeRefsByHandle = THashMap<ui64, ui64>;

////////////////////////////////////////////////////////////////////////////////

struct TSessionLock
    : public TIntrusiveListItem<TSessionLock>
    , public NProto::TSessionLock
{
    TSession* const Session;

    TSessionLock(TSession* session, const NProto::TSessionLock& proto)
        : NProto::TSessionLock(proto)
        , Session(session)
    {}
};

using TSessionLockList = TIntrusiveListWithAutoDelete<TSessionLock, TDelete>;
using TSessionLockMap = THashMap<ui64, TSessionLock*>;
using TSessionLockMultiMap = THashMultiMap<ui64, TSessionLock*>;

////////////////////////////////////////////////////////////////////////////////

struct TDupCacheEntry
    : public TIntrusiveListItem<TDupCacheEntry>
    , public NProto::TDupCacheEntry
{
    bool Commited = false;

    TDupCacheEntry(const NProto::TDupCacheEntry& proto, bool commited)
        : NProto::TDupCacheEntry(proto)
        , Commited(commited)
    {}
};

using TDupCacheEntryList = TIntrusiveListWithAutoDelete<TDupCacheEntry, TDelete>;
using TDupCacheEntryMap = THashMap<ui64, TDupCacheEntry*>;

////////////////////////////////////////////////////////////////////////////////

struct TSession
    : public TIntrusiveListItem<TSession>
    , public NProto::TSession
{
    NActors::TActorId Owner;

    TSessionHandleList Handles;
    TSessionLockList Locks;

    TInstant Deadline;

    // TODO: notify event stream
    ui32 LastEvent = 0;
    bool NotifyEvents = false;

    ui64 LastDupCacheEntryId = 1;
    TDupCacheEntryList DupCacheEntries;
    TDupCacheEntryMap DupCache;

public:
    TSession(const NProto::TSession& proto, const NActors::TActorId& owner = {})
        : NProto::TSession(proto)
        , Owner(owner)
    {}

    bool IsValid() const
    {
        return (bool)Owner;
    }

    void LoadDupCacheEntry(NProto::TDupCacheEntry entry)
    {
        LastDupCacheEntryId = Max(LastDupCacheEntryId, entry.GetEntryId());
        AddDupCacheEntry(std::move(entry), true);
    }

    const TDupCacheEntry* LookupDupEntry(ui64 requestId)
    {
        if (!requestId) {
            return nullptr;
        }

        auto it = DupCache.find(requestId);
        if (it != DupCache.end()) {
            return it->second;
        }

        return nullptr;
    }

    void AddDupCacheEntry(NProto::TDupCacheEntry proto, bool commited)
    {
        Y_VERIFY(proto.GetRequestId());
        Y_VERIFY(proto.GetEntryId());

        DupCacheEntries.PushBack(new TDupCacheEntry(std::move(proto), commited));

        auto entry = DupCacheEntries.Back();
        auto [_, inserted] = DupCache.emplace(entry->GetRequestId(), entry);
        Y_VERIFY(inserted);
    }

    void CommitDupCacheEntry(ui64 requestId)
    {
        if (auto it = DupCache.find(requestId); it != DupCache.end()) {
            it->second->Commited = true;
        }
    }

    ui64 PopDupCacheEntry(ui64 maxEntries)
    {
        if (DupCacheEntries.Size() <= maxEntries) {
            return 0;
        }

        std::unique_ptr<TDupCacheEntry> entry(DupCacheEntries.PopFront());
        DupCache.erase(entry->GetRequestId());

        return entry->GetEntryId();
    }
};

using TSessionList = TIntrusiveListWithAutoDelete<TSession, TDelete>;
using TSessionMap = THashMap<TString, TSession*>;
using TSessionOwnerMap = THashMap<NActors::TActorId, TSession*>;
using TSessionClientMap = THashMap<TString, TSession*>;

}   // namespace NCloud::NFileStore::NStorage
