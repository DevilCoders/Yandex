#include "remote.h"

#include "auth.h"
#include "exception.h"

namespace NLibgit2 {

    TRemote::TRemote(git_repository *repo, const TString& ref) {
        git_remote* remote;
        GitThrowIfError(git_remote_lookup(&remote, repo, ref.c_str()));
        Remote_.Reset(remote);
    }

    void TRemote::PushToBranch(const TString& branchName, TCredentialCallback credCb, TCheckCertCallback callback) {
        Push({"refs/heads/" + branchName}, credCb, callback);
    }

    void TRemote::Push(TVector<TString>&& refspecs, TCredentialCallback credCb, TCheckCertCallback callback) {
        TVector<char*> raw_refspecs;
        for (TString& refspec : refspecs) {
            raw_refspecs.push_back(refspec.begin());
        }
        const git_strarray git_refspecs = {
            .strings = raw_refspecs.data(),
            .count = raw_refspecs.size(),
        };
        git_push_options options;
        GitThrowIfError(git_push_init_options(&options, GIT_PUSH_OPTIONS_VERSION));
        options.callbacks.certificate_check = callback;
        options.callbacks.credentials = [](git_cred **out, const char* url,
            const char* username_from_url, unsigned int allowed_types, void* payload) {
                auto credCb = reinterpret_cast<TCredentialCallback*>(payload);
                TCredentials cred = (*credCb)(url, username_from_url, allowed_types);
                *out = cred.Get();
            return 0;
        };
        options.callbacks.payload = &credCb;
        GitThrowIfError(git_remote_push(Remote_.Get(), &git_refspecs, &options));
    }

} // namespace NLibgit2
