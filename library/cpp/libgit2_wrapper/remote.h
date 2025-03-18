#pragma once

#include "callbacks.h"
#include "holder.h"

#include <contrib/libs/libgit2/include/git2.h>

#include <util/generic/ptr.h>

namespace NLibgit2 {

class TRemote {
public:
    TRemote(git_repository *repo, const TString& ref);
    void Push(TVector<TString>&& refspecs, TCredentialCallback credCb, TCheckCertCallback cb);
    void PushToBranch(const TString& branchName, TCredentialCallback credCb, TCheckCertCallback cb);
private:
    NPrivate::THolder<git_remote, git_remote_free> Remote_;
};

} // namespace NLibgit2
