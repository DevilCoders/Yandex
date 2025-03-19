#pragma once

#include "ki_actor.h"

#include <ysite/yandex/srchmngr/yrequester.h>

#include <util/generic/ptr.h>
#include <util/generic/vector.h>

namespace NRTYServer {
    class TKIProcessorTraits {
    public:
        static inline TString GetIndexPrefix(const TString& name) {
            return "index." + name + ".";
        }
        static inline TString GetKeyFilename(const TString& name) {
            return GetIndexPrefix(name) + "key";
        }
        static inline TString GetInvFilename(const TString& name) {
            return GetIndexPrefix(name) + "inv";
        }
    };
}

class TRTYKIReader: public NRTYServer::TKIProcessorTraits {
private:
    TString FilePath;
    THolder<TYndexRequester> YR;
    bool LockFiles = false;

public:
    typedef TAtomicSharedPtr<TRTYKIReader> TPtr;

public:
    static bool HasData(const TString& dir, const TString& name) {
        return TFsPath(dir + "/" + GetKeyFilename(name)).Exists() && TFsPath(dir + "/" + GetInvFilename(name)).Exists();
    }

public:
    TRTYKIReader(const TString& dir, const TString& name, const bool lockFiles = false)
        : FilePath(dir + "/" + GetIndexPrefix(name))
        , LockFiles(lockFiles)
    {
    }

    TYndexRequester* GetYR() {
        VERIFY_WITH_LOG(!!YR, "Incorrect TRTYKIReader usage");
        return YR.Get();
    }

    const TString& GetFilePath() const {
        return FilePath;
    }

    bool IsOpen() const {
        return YR && YR->IsOpen();
    }

    void Open();
    void Close();
};

class TRTYKIMaker: public NRTYServer::TKIProcessorTraits {
private:
    TVector<TAutoPtr<TRTYKIActor> > KIActors;
    TString Dir;
    TString Prefix;

    TAdvancedMergeTaskWriter MergeTaskWriter;

    ui32 ActorsCount;
    ui32 MaxPortionDocs;
    const bool BuildByKeysFlag = false;

public:
    typedef TAtomicSharedPtr<TRTYKIMaker> TPtr;

public:
    TRTYKIMaker(ui32 actorsCount, const TString& dir, const TString& prefix, ui32 maxPortionDocs, const bool buildByKeysFlag = false)
        : Dir(dir)
        , Prefix(prefix)
        , ActorsCount(actorsCount)
        , MaxPortionDocs(maxPortionDocs)
        , BuildByKeysFlag(buildByKeysFlag)
    {
    }

    void Start();
    void Stop();

    bool Add(ui32 actorId, IKIStorage& doc, ui32 docId) {
        VERIFY_WITH_LOG(KIActors.size() > actorId, "Incorrect TRTYKIMaker usage");
        return KIActors[actorId]->StoreDoc(doc, docId);
    }

    bool StorePositions(ui32 actorId, const char* key, SUPERLONG* positions, ui32 posCount) {
        VERIFY_WITH_LOG(KIActors.size() > actorId, "Incorrect TRTYKIMaker usage");
        return KIActors[actorId]->StorePositions(key, positions, posCount);
    }

    bool IncDoc(ui32 actorId);
    void Close(const TVector<ui32>* remapTable);
    void Discard();
};
