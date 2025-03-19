#pragma once

#include "public.h"

#include "index.h"

#include <cloud/filestore/libs/service/filestore.h>
#include <cloud/storage/core/libs/common/error.h>

#include <util/datetime/base.h>
#include <util/generic/hash.h>
#include <util/system/file.h>
#include <util/system/rwlock.h>

namespace NCloud::NFileStore {

////////////////////////////////////////////////////////////////////////////////

class TSession
{
public:
    const TFsPath Root;
    const TString ClientId;
    const TString SessionId;
    const TLocalIndexPtr Index;

private:
    THashMap<TString, TString> Attrs;
    THashMap<ui32, TFile> Handles;
    TRWMutex Lock;

    TInstant LastPing = TInstant::Now();

public:
    TSession(TFsPath root, TString clientId, TString sessionId, TLocalIndexPtr index)
        : Root(root.RealPath())
        , ClientId(std::move(clientId))
        , SessionId(std::move(sessionId))
        , Index(std::move(index))
    {}

    void InsertHandle(const TFile& handle)
    {
        TWriteGuard guard(Lock);

        auto it = Handles.find(handle.GetHandle());
        Y_VERIFY(it == Handles.end(), "dup file handle for: %d",
            handle.GetHandle());

        Handles[handle.GetHandle()] = handle;
    }

    TFile LookupHandle(ui64 handle)
    {
        TReadGuard guard(Lock);

        auto it = Handles.find(handle);
        if (it == Handles.end()) {
            return {};
        }

        return it->second;
    }

    void DeleteHandle(ui64 handle)
    {
        TWriteGuard guard(Lock);

        Handles.erase(handle);
    }

    TIndexNodePtr LookupNode(ui64 nodeId)
    {
        return Index->LookupNode(nodeId);
    }

    bool TryInsertNode(TIndexNodePtr node)
    {
        return Index->TryInsertNode(std::move(node));
    }

    void ForgetNode(ui64 nodeId)
    {
        Index->ForgetNode(nodeId);
    }

    void Ping()
    {
        LastPing = TInstant::Now();
    }

    TInstant GetLastPing() const
    {
        return LastPing;
    }
};

}   // namespace NCloud::NFileStore
