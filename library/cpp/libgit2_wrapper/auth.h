#pragma once

#include <contrib/libs/libgit2/include/git2.h>

#include <util/folder/path.h>
#include <util/generic/ptr.h>

namespace NLibgit2 {

    class TCredentials {
    public:
        TCredentials();
        TCredentials(const TString& username);
        TCredentials(const TString& username, const TString& password);
        TCredentials(const TString& username, const TFsPath& privateKey, const TString& passphrase);

        [[nodiscard]] git_cred* Get();
        TString UserName() const;
    private:
        git_cred* Credentials_;
        TString UserName_;
    };

    TCredentials CredentialsFromEnvionment(const TString& url, const TString& usernameFromUrl, int allowedTypes);
    TCredentials DefaultCredentials(const TString& url, const TString& usernameFromUrl, int allowedTypes);

} // namespace NLibgit2
