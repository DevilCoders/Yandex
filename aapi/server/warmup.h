#pragma once

#include "server.h"
#include <aapi/lib/common/object_types.h>

#include <library/cpp/threading/future/future.h>

#include <util/system/condvar.h>
#include <util/system/mutex.h>

namespace NAapi {

class TWarmupProcessor : public IWalkReceiver {
public:
     TWarmupProcessor(const TVector<TString>& paths, TVcsServer* server);
    ~TWarmupProcessor();

    void Walk(ui64 revision);

public:
    bool Push(const TDirectories& dirs) override;

    void Finish(const grpc::Status& status) override;

private:
    NThreading::TFuture<TString> GetTreeHash(ui64 revision);

    NThreading::TFuture<TMaybe<TString>> GetObject(const TString& hash);

    TMaybe<TString> GetPathHash(const TString& path, const TString& rootHash);

private:
    TVcsServer* Server_;
    TMutex Lock_;
    TCondVar Cond_;
    TVector<TString> Paths_;
    bool Finished_;
};

} // namespace NAapi
