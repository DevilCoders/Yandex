#pragma once

#include <aapi/lib/trace/trace.h>

#include <util/generic/yexception.h>
#include <util/datetime/base.h>

namespace NAapi {

class TRpcState {
public:
    inline void Ok() {
        IsOk = true;
    }

    inline void Error(const TString& msg) {
        ErrorMsg = msg;
    }

protected:
    bool IsOk = false;
    TString ErrorMsg = "";
};

class TWalkTracer: public TRpcState {
public:
    explicit inline TWalkTracer(const TString& hash)
        : Hash(hash)
        , SentObjects(0)
        , CacheHit(0)
        , Elapsed(Now().Seconds())
    {
        Trace(TWalkStarted(Hash));
    }

    inline void ObjectSent() {
        ++SentObjects;
    }

    inline void ObjectInCache() {
        ++CacheHit;
    }

    inline ~TWalkTracer() {
        Elapsed = Now().Seconds() - Elapsed;

        if (IsOk) {
            Trace(TWalkFinished(Hash, SentObjects, CacheHit, Elapsed));
        } else {
            if (UncaughtException()) {
                ErrorMsg = CurrentExceptionMessage();
            }
            Trace(TWalkFailed(Hash, SentObjects, CacheHit, ErrorMsg));
        }
    }

private:
    TString Hash;
    ui64 SentObjects;
    ui64 CacheHit;
    ui64 Elapsed;
};

class TObjectsTracer: public TRpcState {
public:
    inline TObjectsTracer()
        : RecvHashes(0)
        , CacheHit(0)
        , Elapsed(Now().Seconds())
    {
        Trace(TObjectsStarted());
    }

    inline void HashRecieved() {
        ++RecvHashes;
    }

    inline void ObjectInCache() {
        ++CacheHit;
    }

    inline ~TObjectsTracer() {
        Elapsed = Now().Seconds() - Elapsed;

        if (IsOk) {
            Trace(TObjectsFinished(RecvHashes, CacheHit, Elapsed));
        } else {
            if (UncaughtException()) {
                ErrorMsg = CurrentExceptionMessage();
            }
            Trace(TObjectsFailed(RecvHashes, CacheHit, ErrorMsg));
        }
    }

private:
    ui64 RecvHashes;
    ui64 CacheHit;
    ui64 Elapsed;
};

}  // namespace NAapi
